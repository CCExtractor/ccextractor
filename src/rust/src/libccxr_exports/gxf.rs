use crate::demuxer::common_structs::*;
use crate::demuxer::demux::*;
use crate::gxf_demuxer::gxf::*;
use std::os::raw::c_int;
use std::os::raw::c_uchar;
use std::slice;
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe function `parse_packet_header`
#[no_mangle]
pub unsafe extern "C" fn ccxr_parse_packet_header(
    ctx: *mut CcxDemuxer,
    pkt_type: *mut GXFPktType,
    length: *mut c_int,
) -> c_int {
    // Ensure none of the input pointers are null.
    if ctx.is_null() || pkt_type.is_null() || length.is_null() {
        return CCX_EINVAL;
    }

    // Convert the raw pointers into mutable references and call the Rust implementation.
    parse_packet_header(ctx, &mut *pkt_type, &mut *length)
}

/// # Safety
/// This function is unsafe because it calls unsafe function `parse_material_sec`
#[no_mangle]
pub unsafe extern "C" fn ccxr_parse_material_sec(demux: *mut CcxDemuxer, len: c_int) -> c_int {
    // Pass the demux pointer and length (converted to i32) to the Rust function.
    parse_material_sec(demux, len)
}
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe function `parse_mpeg525_track_desc`
#[no_mangle]
pub unsafe extern "C" fn ccxr_parse_mpeg525_track_desc(
    demux: *mut CcxDemuxer,
    len: c_int,
) -> c_int {
    // Check for a valid pointer.
    if demux.is_null() {
        return CCX_EINVAL;
    }
    // Convert the raw pointer into a mutable reference and call the Rust function.
    parse_mpeg525_track_desc(&mut *demux, len)
}
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe function `parse_ad_track_desc`
#[no_mangle]
pub unsafe extern "C" fn ccxr_parse_ad_track_desc(demux: *mut CcxDemuxer, len: c_int) -> c_int {
    // Check for a valid pointer.
    if demux.is_null() {
        return CCX_EINVAL;
    }
    // Convert the raw pointer into a mutable reference and call the Rust function.
    parse_ad_track_desc(&mut *demux, len)
}
/// # Safety
/// This function is unsafe because it dereferences raw pointers
#[no_mangle]
pub unsafe extern "C" fn ccxr_set_track_frame_rate(vid_track: *mut CcxGxfVideoTrack, val: i8) {
    if vid_track.is_null() {
        return;
    }
    set_track_frame_rate(&mut *vid_track, val);
}
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe function `parse_track_sec`
#[no_mangle]
pub unsafe extern "C" fn ccxr_parse_track_sec(
    demux: *mut CcxDemuxer,
    len: c_int,
    data: *mut DemuxerData,
) -> c_int {
    if demux.is_null() || data.is_null() {
        return CCX_EINVAL;
    }
    // Convert the raw pointers into mutable references.
    parse_track_sec(&mut *demux, len, &mut *data)
}

/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe function `from_raw_parts`
#[no_mangle]
pub unsafe extern "C" fn ccxr_parse_ad_cdp(
    cdp: *const c_uchar,
    len: usize,
    data: *mut DemuxerData,
) -> c_int {
    if cdp.is_null() || data.is_null() {
        return CCX_EINVAL;
    }
    // Create a slice from the raw pointer.
    let cdp_slice = slice::from_raw_parts(cdp, len);
    // Convert the data pointer to a mutable reference.
    let data_ref = &mut *data;
    match parse_ad_cdp(cdp_slice, data_ref) {
        Ok(()) => CCX_OK,
        Err(_) => CCX_EINVAL,
    }
}
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe function `parse_ad_pyld`
#[no_mangle]
pub unsafe extern "C" fn ccxr_parse_ad_pyld(
    demux: *mut CcxDemuxer,
    len: c_int,
    data: *mut DemuxerData,
) -> c_int {
    if demux.is_null() || data.is_null() {
        return CCX_EINVAL;
    }
    parse_ad_pyld(&mut *demux, len, &mut *data)
}
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe function `parse_ad_vbi`
#[no_mangle]
pub unsafe extern "C" fn ccxr_parse_ad_vbi(
    demux: *mut CcxDemuxer,
    len: c_int,
    data: *mut DemuxerData,
) -> c_int {
    if demux.is_null() || data.is_null() {
        return CCX_EINVAL;
    }
    parse_ad_vbi(&mut *demux, len, &mut *data)
}
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe function `parse_ad_field`
#[no_mangle]
pub unsafe extern "C" fn ccxr_parse_ad_field(
    demux: *mut CcxDemuxer,
    len: c_int,
    data: *mut DemuxerData,
) -> c_int {
    if demux.is_null() || data.is_null() {
        return CCX_EINVAL;
    }
    parse_ad_field(&mut *demux, len, &mut *data)
}
/// # Safety
/// This function is unsafe because it dereferences raw pointers
#[no_mangle]
pub unsafe extern "C" fn ccxr_set_data_timebase(vid_format: c_int, data: *mut DemuxerData) {
    if data.is_null() {
        return;
    }
    set_data_timebase(vid_format, &mut *data)
}
//
// Extern wrapper for parse_mpeg_packet
//
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe function `parse_mpeg_packet`
#[no_mangle]
pub unsafe extern "C" fn ccxr_parse_mpeg_packet(
    demux: *mut CcxDemuxer,
    len: c_int,
    data: *mut DemuxerData,
) -> c_int {
    if demux.is_null() || data.is_null() {
        return CCX_EINVAL;
    }
    // Note: the Rust function expects len as usize.
    parse_mpeg_packet(&mut *demux, len as usize, &mut *data)
}

//
// Extern wrapper for parse_ad_packet
//
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe function `parse_ad_packet`
#[no_mangle]
pub unsafe extern "C" fn ccxr_parse_ad_packet(
    demux: *mut CcxDemuxer,
    len: c_int,
    data: *mut DemuxerData,
) -> c_int {
    if demux.is_null() || data.is_null() {
        return CCX_EINVAL;
    }
    parse_ad_packet(&mut *demux, len, &mut *data)
}

//
// Extern wrapper for set_mpeg_frame_desc
//
/// # Safety
/// This function is unsafe because it dereferences raw pointers
#[no_mangle]
pub unsafe extern "C" fn ccxr_set_mpeg_frame_desc(
    vid_track: *mut CcxGxfVideoTrack,
    mpeg_frame_desc_flag: u8,
) {
    if vid_track.is_null() {
        return;
    }
    set_mpeg_frame_desc(&mut *vid_track, mpeg_frame_desc_flag);
}

//
// Extern wrapper for parse_media
//
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe function `parse_media`
#[no_mangle]
pub unsafe extern "C" fn ccxr_parse_media(
    demux: *mut CcxDemuxer,
    len: c_int,
    data: *mut DemuxerData,
) -> c_int {
    if demux.is_null() || data.is_null() {
        return CCX_EINVAL;
    }
    parse_media(&mut *demux, len, &mut *data)
}

//
// Extern wrapper for parse_flt
//
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe function `parse_flt`
#[no_mangle]
pub unsafe extern "C" fn ccxr_parse_flt(ctx: *mut CcxDemuxer, len: c_int) -> c_int {
    if ctx.is_null() {
        return CCX_EINVAL;
    }
    parse_flt(&mut *ctx, len)
}

//
// Extern wrapper for parse_umf
//
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe function `parse_umf`
#[no_mangle]
pub unsafe extern "C" fn ccxr_parse_umf(ctx: *mut CcxDemuxer, len: c_int) -> c_int {
    if ctx.is_null() {
        return CCX_EINVAL;
    }
    parse_umf(&mut *ctx, len)
}

//
// Extern wrapper for parse_map
//
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe function `parse_map`
#[no_mangle]
pub unsafe extern "C" fn ccxr_parse_map(
    ctx: *mut CcxDemuxer,
    len: c_int,
    data: *mut DemuxerData,
) -> c_int {
    if ctx.is_null() || data.is_null() {
        return CCX_EINVAL;
    }
    parse_map(&mut *ctx, len, &mut *data)
}

//
// Extern wrapper for read_packet
//
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe function `read_packet`
#[no_mangle]
pub unsafe extern "C" fn ccxr_read_packet(ctx: *mut CcxDemuxer, data: *mut DemuxerData) -> c_int {
    if ctx.is_null() || data.is_null() {
        return CCX_EINVAL;
    }
    read_packet(&mut *ctx, &mut *data)
}

//
// Extern wrapper for ccx_gxf_probe
//
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe function `ccx_gxf_probe`
#[no_mangle]
pub unsafe extern "C" fn ccxr_gxf_probe(buf: *const c_uchar, len: c_int) -> c_int {
    // Here we assume CCX_TRUE and CCX_FALSE are defined (typically 1 and 0).
    if buf.is_null() {
        return CCX_FALSE;
    }
    // Create a slice from the raw pointer.
    let slice = unsafe { slice::from_raw_parts(buf, len as usize) };
    if ccx_gxf_probe(slice) {
        CCX_TRUE
    } else {
        CCX_FALSE
    }
}
