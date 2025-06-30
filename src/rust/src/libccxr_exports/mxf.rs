use crate::bindings::{
    ccx_demuxer, demuxer_data, MXFCaptionType_MXF_CT_ANC, MXFCaptionType_MXF_CT_VBI, MXFContext,
    MXFTrack,
};
use crate::ccx_options;
use crate::common::{copy_from_rust, copy_to_rust, CType};
use crate::ctorust::FromCType;
use crate::demuxer::common_structs::CcxRational;
use crate::libccxr_exports::demuxer::{copy_demuxer_from_c_to_rust, copy_demuxer_from_rust_to_c};
use crate::libccxr_exports::demuxerdata::{copy_demuxer_data_from_rust, copy_demuxer_data_to_rust};
use crate::mxf_demuxer::common_structs::{MXFCaptionTypeRust, MXFContextRust, MXFTrackRust};
use crate::mxf_demuxer::mxf::read_packet_mxf;
use std::os::raw::c_int;

/// Convert from C MXFContext to Rust MXFContextRust
/// # Safety
/// - `c_ctx` must be a valid pointer to MXFContext
/// - Data pointed by c_ctx must outlive the returned Rust struct
pub unsafe fn copy_mxf_context_to_rust(c_ctx: *const MXFContext) -> MXFContextRust {
    let c = &*c_ctx;
    // Convert caption type
    let caption_type = match c.type_ {
        MXFCaptionType_MXF_CT_ANC => MXFCaptionTypeRust::Anc,
        _ => MXFCaptionTypeRust::Vbi,
    };
    // Convert tracks
    let mut rust_tracks: [MXFTrackRust; 32] = [MXFTrackRust {
        track_id: 0,
        track_number: [0; 4],
    }; 32];
    for (i, rust_track) in rust_tracks
        .iter_mut()
        .enumerate()
        .take((c.nb_tracks as usize).min(32))
    {
        let t: MXFTrack = c.tracks[i];
        *rust_track = MXFTrackRust {
            track_id: t.track_id,
            track_number: t.track_number,
        };
    }

    MXFContextRust {
        caption_type,
        cap_track_id: c.cap_track_id,
        cap_essence_key: c.cap_essence_key,
        tracks: rust_tracks,
        nb_tracks: c.nb_tracks,
        cap_count: c.cap_count,
        edit_rate: CcxRational::from_ctype(c.edit_rate).unwrap_or_default(),
    }
}

/// Copy from Rust MXFContextRust to C MXFContext
/// # Safety
/// - `c_ctx` must be a valid, mutable pointer to MXFContext
/// - Must not use libc; only pointer writes
pub unsafe fn copy_mxf_context_from_rust(c_ctx: *mut MXFContext, rust_ctx: &MXFContextRust) {
    let c = &mut *c_ctx;
    c.type_ = match rust_ctx.caption_type {
        MXFCaptionTypeRust::Anc => MXFCaptionType_MXF_CT_ANC,
        _ => MXFCaptionType_MXF_CT_VBI,
    };
    c.cap_track_id = rust_ctx.cap_track_id;
    c.cap_essence_key = rust_ctx.cap_essence_key;
    // Copy tracks up to nb_tracks
    let count = rust_ctx.nb_tracks as usize;
    for i in 0..32 {
        if i < count {
            let rt = &rust_ctx.tracks[i];
            c.tracks[i].track_id = rt.track_id;
            c.tracks[i].track_number = rt.track_number;
        } else {
            c.tracks[i] = MXFTrack {
                track_id: 0,
                track_number: [0; 4],
            };
        }
    }
    c.nb_tracks = rust_ctx.nb_tracks;
    c.cap_count = rust_ctx.cap_count;
    c.edit_rate = rust_ctx.edit_rate.to_ctype();
}
/// # Safety
/// Wraps C FFI for MXF demuxer, the C `ccx_mxf_getmoredata` behavior.
#[no_mangle]
pub unsafe extern "C" fn ccxr_mxf_getmoredata(
    ctx: *mut ccx_demuxer,
    data: *mut demuxer_data,
) -> c_int {
    if ctx.is_null() || data.is_null() {
        return -102;
    }
    // Copy C demuxer_data into Rust
    let mut demuxer_data_rust = copy_demuxer_data_to_rust(data);

    // Handle MXFContext stored in ctx->demux_ctx->private_data
    let ctx_ref = &mut *ctx;
    let c_mxf = ctx_ref.private_data as *mut MXFContext;
    if c_mxf.is_null() {
        return -103;
    }
    let mut mxf_context_rust = copy_mxf_context_to_rust(c_mxf);
    let mut demuxer_rust = copy_demuxer_from_c_to_rust(ctx);
    demuxer_rust.private_data =
        &mut mxf_context_rust as *mut MXFContextRust as *mut core::ffi::c_void;
    let mut CcxOptions = copy_to_rust(&raw const ccx_options);
    // Call the read_packet for MXF, injecting the MXFContextRust
    let ret = match read_packet_mxf(&mut demuxer_rust, &mut demuxer_data_rust, &mut CcxOptions) {
        Ok(v) => v,
        Err(e) => return e as c_int,
    };

    let new_rust_mxf = match (demuxer_rust.private_data as *mut MXFContextRust).as_mut() {
        Some(ctx) => ctx,
        None => {
            println!("INVALID MXFCONTEXT - FAILED");
            return -102;
        }
    };
    copy_mxf_context_from_rust(c_mxf, new_rust_mxf);
    demuxer_rust.private_data = c_mxf as *mut core::ffi::c_void;
    copy_demuxer_from_rust_to_c(ctx, &demuxer_rust);
    // Copy back demuxer_data
    copy_demuxer_data_from_rust(data, &demuxer_data_rust);
    copy_from_rust(&raw mut ccx_options, CcxOptions);
    ret
}
#[cfg(test)]
mod tests {
    use super::*;
    use crate::bindings::{MXFCaptionType_MXF_CT_VBI, MXFContext, MXFTrack};
    fn make_test_c_context() -> MXFContext {
        let mut ctx = MXFContext {
            type_: MXFCaptionType_MXF_CT_VBI,
            cap_track_id: 7,
            cap_essence_key: [1u8; 16],
            tracks: [MXFTrack {
                track_id: 0,
                track_number: [0; 4],
            }; 32],
            nb_tracks: 2,
            cap_count: 5,
            edit_rate: crate::bindings::ccx_rational { num: 25, den: 1 },
        };
        ctx.tracks[0] = MXFTrack {
            track_id: 10,
            track_number: [1, 2, 3, 4],
        };
        ctx.tracks[1] = MXFTrack {
            track_id: 20,
            track_number: [5, 6, 7, 8],
        };
        ctx
    }

    #[test]
    fn test_copy_to_rust() {
        let c_ctx = make_test_c_context();
        let rust_ctx = unsafe { copy_mxf_context_to_rust(&c_ctx) };
        assert_eq!(rust_ctx.caption_type, MXFCaptionTypeRust::Vbi);
        assert_eq!(rust_ctx.cap_track_id, 7);
        assert_eq!(rust_ctx.cap_essence_key, [1u8; 16]);
        assert_eq!(rust_ctx.nb_tracks, 2);
        assert_eq!(rust_ctx.cap_count, 5);
        assert_eq!(rust_ctx.edit_rate.num, 25);
        assert_eq!(rust_ctx.edit_rate.den, 1);
        assert_eq!(rust_ctx.tracks[0].track_id, 10);
        assert_eq!(rust_ctx.tracks[0].track_number, [1, 2, 3, 4]);
        assert_eq!(rust_ctx.tracks[1].track_id, 20);
    }

    #[test]
    fn test_copy_from_rust() {
        let mut c_ctx = make_test_c_context();
        let mut rust_ctx = unsafe { copy_mxf_context_to_rust(&c_ctx) };
        // Modify Rust ctx
        rust_ctx.caption_type = MXFCaptionTypeRust::Anc;
        rust_ctx.cap_track_id = 9;
        rust_ctx.tracks[0].track_id = 99;
        rust_ctx.nb_tracks = 1;
        rust_ctx.cap_count = 3;
        rust_ctx.edit_rate = CcxRational { num: 30, den: 2 };

        unsafe { copy_mxf_context_from_rust(&mut c_ctx, &rust_ctx) };
        // Validate C context updated
        assert_eq!(c_ctx.type_, crate::bindings::MXFCaptionType_MXF_CT_ANC);
        assert_eq!(c_ctx.cap_track_id, 9);
        assert_eq!(c_ctx.nb_tracks, 1);
        assert_eq!(c_ctx.cap_count, 3);
        assert_eq!(c_ctx.edit_rate.num, 30);
        assert_eq!(c_ctx.edit_rate.den, 2);
        assert_eq!(c_ctx.tracks[0].track_id, 99);
        // Other track cleared
        assert_eq!(c_ctx.tracks[1].track_id, 0);
    }
}
