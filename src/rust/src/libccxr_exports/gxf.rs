use crate::bindings::{
    ccx_code_type_CCX_CODEC_ATSC_CC, ccx_gxf, ccx_gxf_ancillary_data_track, ccx_gxf_video_track,
    demuxer_data, lib_ccx_ctx,
};
use crate::ccx_options;
use crate::common::{copy_to_rust, CType};
use crate::ctorust::FromCType;
use crate::demuxer::common_structs::CcxRational;
use crate::demuxer::demuxer_data::DemuxerData;
use crate::gxf_demuxer::common_structs::*;
use crate::gxf_demuxer::gxf::read_packet;
use crate::libccxr_exports::demuxer::{copy_demuxer_from_c_to_rust, copy_demuxer_from_rust_to_c};
use crate::libccxr_exports::demuxerdata::{copy_demuxer_data_from_rust, copy_demuxer_data_to_rust};
use lib_ccxr::common::Options;
use lib_ccxr::debug;
use lib_ccxr::util::log::DebugMessageFlag;
use std::ffi::CString;
use std::os::raw::{c_char, c_int};

// Helper function to convert String to C char array
#[no_mangle]
fn string_to_c_array(s: &str) -> [c_char; 256] {
    let mut array = [0i8; 256];
    let c_string = CString::new(s).unwrap_or_else(|_| CString::new("").unwrap());
    let bytes = c_string.as_bytes_with_nul();
    let len = std::cmp::min(bytes.len(), 255);

    for (i, &byte) in bytes[..len].iter().enumerate() {
        array[i] = byte as c_char;
    }
    array
}

// Write-back functions for the three structs
#[no_mangle]
unsafe fn copy_ccx_gxf_video_track_from_rust_to_c(
    ctx: *mut ccx_gxf_video_track,
    video_track: &CcxGxfVideoTrack,
) {
    (*ctx).track_name = string_to_c_array(&video_track.track_name);
    (*ctx).fs_version = video_track.fs_version;
    (*ctx).frame_rate = video_track.frame_rate.to_ctype();
    (*ctx).line_per_frame = video_track.line_per_frame;
    (*ctx).field_per_frame = video_track.field_per_frame;
    (*ctx).p_code = video_track.p_code.to_ctype();
    (*ctx).p_struct = video_track.p_struct.to_ctype();
}
#[no_mangle]
unsafe fn copy_ccx_gxf_ancillary_data_track_from_rust_to_c(
    ctx: *mut ccx_gxf_ancillary_data_track,
    ad_track: &CcxGxfAncillaryDataTrack,
) {
    (*ctx).track_name = string_to_c_array(&ad_track.track_name);
    (*ctx).id = ad_track.id;
    (*ctx).ad_format = ad_track.ad_format.to_ctype();
    (*ctx).nb_field = ad_track.nb_field;
    (*ctx).field_size = ad_track.field_size;
    (*ctx).packet_size = ad_track.packet_size;
    (*ctx).fs_version = ad_track.fs_version;
    (*ctx).frame_rate = ad_track.frame_rate;
    (*ctx).line_per_frame = ad_track.line_per_frame;
    (*ctx).field_per_frame = ad_track.field_per_frame;
}
#[no_mangle]
unsafe fn copy_ccx_gxf_from_rust_to_c(ctx: *mut ccx_gxf, gxf: &CcxGxf) {
    (*ctx).nb_streams = gxf.nb_streams;
    (*ctx).media_name = string_to_c_array(&gxf.media_name);
    (*ctx).first_field_nb = gxf.first_field_nb;
    (*ctx).last_field_nb = gxf.last_field_nb;
    (*ctx).mark_in = gxf.mark_in;
    (*ctx).mark_out = gxf.mark_out;
    (*ctx).stream_size = gxf.stream_size;

    (*ctx).cdp_len = gxf.cdp_len;
    // Handle ad_track
    if let Some(ref ad_track) = gxf.ad_track {
        if !(*ctx).ad_track.is_null() {
            copy_ccx_gxf_ancillary_data_track_from_rust_to_c((*ctx).ad_track, ad_track);
        }
    }

    // Handle vid_track
    if let Some(ref vid_track) = gxf.vid_track {
        if !(*ctx).vid_track.is_null() {
            copy_ccx_gxf_video_track_from_rust_to_c((*ctx).vid_track, vid_track);
        }
    }

    // Handle cdp buffer
    if let Some(ref cdp_data) = gxf.cdp {
        if !(*ctx).cdp.is_null() && !cdp_data.is_empty() {
            let copy_len = std::cmp::min(cdp_data.len(), gxf.cdp_len);
            std::ptr::copy_nonoverlapping(cdp_data.as_ptr(), (*ctx).cdp, copy_len);
        }
    }
}

// Helper function to convert C char array to String
#[no_mangle]
unsafe fn c_array_to_string(c_array: &[c_char; 256]) -> String {
    let ptr = c_array.as_ptr();
    if ptr.is_null() {
        return String::new();
    }

    // Find the null terminator or end of array
    let mut len = 0;
    while len < 256 && *ptr.add(len) != 0 {
        len += 1;
    }

    if len == 0 {
        return String::new();
    }

    // Convert to CStr and then to String
    let slice = std::slice::from_raw_parts(ptr as *const u8, len);
    String::from_utf8_lossy(slice).into_owned()
}

// Read functions for the three structs
#[no_mangle]
unsafe fn copy_ccx_gxf_video_track_from_c_to_rust(
    ctx: *const ccx_gxf_video_track,
) -> Option<CcxGxfVideoTrack> {
    if ctx.is_null() {
        return None;
    }

    let c_track = &*ctx;

    Some(CcxGxfVideoTrack {
        track_name: c_array_to_string(&c_track.track_name),
        fs_version: c_track.fs_version,
        frame_rate: CcxRational::from_ctype(c_track.frame_rate)?,
        line_per_frame: c_track.line_per_frame,
        field_per_frame: c_track.field_per_frame,
        p_code: MpegPictureCoding::from_ctype(c_track.p_code)?,
        p_struct: MpegPictureStruct::from_ctype(c_track.p_struct)?,
    })
}

#[no_mangle]
unsafe fn copy_ccx_gxf_ancillary_data_track_from_c_to_rust(
    ctx: *const ccx_gxf_ancillary_data_track,
) -> Option<CcxGxfAncillaryDataTrack> {
    if ctx.is_null() {
        return None;
    }

    let c_track = &*ctx;

    Some(CcxGxfAncillaryDataTrack {
        track_name: c_array_to_string(&c_track.track_name),
        id: c_track.id,
        ad_format: GXF_Anc_Data_Pres_Format::from_ctype(c_track.ad_format)?,
        nb_field: c_track.nb_field,
        field_size: c_track.field_size,
        packet_size: c_track.packet_size,
        fs_version: c_track.fs_version,
        frame_rate: c_track.frame_rate,
        line_per_frame: c_track.line_per_frame,
        field_per_frame: c_track.field_per_frame,
    })
}
/// # Safety
/// This function is unsafe because it requires a valid pointer to a `ccx_gxf` struct and we are dereferencing raw pointers.
#[no_mangle]
pub unsafe fn copy_ccx_gxf_from_c_to_rust(ctx: *const ccx_gxf) -> Option<CcxGxf> {
    if ctx.is_null() {
        return None;
    }

    let c_gxf = &*ctx;

    // Handle ad_track
    let ad_track = if !c_gxf.ad_track.is_null() {
        copy_ccx_gxf_ancillary_data_track_from_c_to_rust(c_gxf.ad_track)
    } else {
        None
    };

    // Handle vid_track
    let vid_track = if !c_gxf.vid_track.is_null() {
        copy_ccx_gxf_video_track_from_c_to_rust(c_gxf.vid_track)
    } else {
        None
    };

    // Handle cdp buffer
    let cdp = if !c_gxf.cdp.is_null() && c_gxf.cdp_len > 0 {
        let slice = std::slice::from_raw_parts(c_gxf.cdp, c_gxf.cdp_len);
        Some(slice.to_vec())
    } else {
        None
    };

    Some(CcxGxf {
        nb_streams: c_gxf.nb_streams,
        media_name: c_array_to_string(&c_gxf.media_name),
        first_field_nb: c_gxf.first_field_nb,
        last_field_nb: c_gxf.last_field_nb,
        mark_in: c_gxf.mark_in,
        mark_out: c_gxf.mark_out,
        stream_size: c_gxf.stream_size,
        ad_track,
        vid_track,
        cdp,
        cdp_len: c_gxf.cdp_len,
    })
}
/// # Safety
/// This function calls a lot of unsafe functions and calls as_mut.
#[no_mangle]
pub fn get_more_data(
    ctx: &mut lib_ccx_ctx,
    ppdata: &mut Option<&mut DemuxerData>,
    CcxOptions: &mut Options,
) -> i32 {
    // If ppdata is None, we can't allocate here since we need a reference
    // This case should be handled by the caller
    let data = match ppdata.as_mut() {
        Some(data) => data,
        None => {
            // This case should not happen in the FFI context
            // since the caller needs to provide storage
            return -1;
        }
    };

    // Call into your existing read_packet parser
    unsafe {
        let demuxer = &mut copy_demuxer_from_c_to_rust(ctx.demux_ctx);
        if demuxer.private_data.is_null() {
            return -102;
        }
        let c_gxf = demuxer.private_data as *mut ccx_gxf;
        let rust_gxf = copy_ccx_gxf_from_c_to_rust(c_gxf);
        if rust_gxf.is_none() {
            return -102; // Error reading GXF data
        }
        demuxer.private_data = rust_gxf
            .as_ref()
            .map(|gxf| gxf as *const CcxGxf as *mut std::ffi::c_void)
            .unwrap_or(std::ptr::null_mut());
        let returnvalue = match read_packet(demuxer, data, CcxOptions) {
            Ok(()) => 0,
            Err(e) => e as i32,
        };
        let new_rust_gxf = match (demuxer.private_data as *mut CcxGxf).as_mut() {
            Some(ctx) => ctx,
            None => {
                debug!(msg_type = DebugMessageFlag::PARSE;"Failed to convert private_data back to CcxGxf, report this as a bug.");
                return -102;
            }
        };
        // Copy the modified GXF data back to C
        copy_ccx_gxf_from_rust_to_c(c_gxf, new_rust_gxf);
        demuxer.private_data = c_gxf as *mut std::ffi::c_void;
        copy_demuxer_from_rust_to_c(ctx.demux_ctx, demuxer);
        returnvalue
    }
}

/// # Safety
/// This function is unsafe because it dereferences raw pointers and copies data from C to Rust and vice versa.
#[no_mangle]
pub unsafe fn ccxr_gxf_get_more_data(ctx: *mut lib_ccx_ctx, data: *mut *mut demuxer_data) -> c_int {
    if ctx.is_null() || data.is_null() {
        return -1; // Invalid pointers
    }

    let mut CcxOptions: Options = copy_to_rust(&raw const ccx_options);

    // Check if we need to allocate new demuxer_data
    if (*data).is_null() {
        // Allocate new demuxer_data in C
        *data = Box::into_raw(Box::new(demuxer_data::default()));
        if (*data).is_null() {
            return -1; // Allocation failed
        }

        // Initialize the allocated data
        (**data).program_number = 1;
        (**data).stream_pid = 1;
        (**data).codec = ccx_code_type_CCX_CODEC_ATSC_CC;
    }

    // Convert C demuxer_data to Rust
    let mut demuxer_data_rust = copy_demuxer_data_to_rust(*data);

    // Call the Rust function with a mutable reference
    let returnvalue = get_more_data(
        &mut *ctx,
        &mut Some(&mut demuxer_data_rust),
        &mut CcxOptions,
    ) as c_int;

    // Copy the modified data back to C
    copy_demuxer_data_from_rust(*data, &demuxer_data_rust);

    returnvalue
}
#[cfg(test)]
mod tests_rust_to_c {
    use super::*;
    use crate::bindings::{
        ccx_ad_pres_format_PRES_FORMAT_HD, ccx_ad_pres_format_PRES_FORMAT_SD,
        mpeg_picture_coding_CCX_MPC_I_FRAME, mpeg_picture_coding_CCX_MPC_P_FRAME,
        mpeg_picture_struct_CCX_MPS_FRAME, mpeg_picture_struct_CCX_MPS_TOP_FIELD,
    };
    use std::alloc::{alloc, dealloc, Layout};
    use std::ptr;

    #[test]
    fn test_gxf_pkt_type_conversion() {
        unsafe {
            assert_eq!(GXF_Pkt_Type::PKT_MAP.to_ctype(), 0xbc);
            assert_eq!(GXF_Pkt_Type::PKT_MEDIA.to_ctype(), 0xbf);
            assert_eq!(GXF_Pkt_Type::PKT_EOS.to_ctype(), 0xfb);
            assert_eq!(GXF_Pkt_Type::PKT_FLT.to_ctype(), 0xfc);
            assert_eq!(GXF_Pkt_Type::PKT_UMF.to_ctype(), 0xfd);
        }
    }

    #[test]
    fn test_gxf_mat_tag_conversion() {
        unsafe {
            assert_eq!(GXF_Mat_Tag::MAT_NAME.to_ctype(), 0x40);
            assert_eq!(GXF_Mat_Tag::MAT_FIRST_FIELD.to_ctype(), 0x41);
            assert_eq!(GXF_Mat_Tag::MAT_LAST_FIELD.to_ctype(), 0x42);
            assert_eq!(GXF_Mat_Tag::MAT_MARK_IN.to_ctype(), 0x43);
            assert_eq!(GXF_Mat_Tag::MAT_MARK_OUT.to_ctype(), 0x44);
            assert_eq!(GXF_Mat_Tag::MAT_SIZE.to_ctype(), 0x45);
        }
    }

    #[test]
    fn test_gxf_track_tag_conversion() {
        unsafe {
            assert_eq!(GXF_Track_Tag::TRACK_NAME.to_ctype(), 0x4c);
            assert_eq!(GXF_Track_Tag::TRACK_AUX.to_ctype(), 0x4d);
            assert_eq!(GXF_Track_Tag::TRACK_VER.to_ctype(), 0x4e);
            assert_eq!(GXF_Track_Tag::TRACK_MPG_AUX.to_ctype(), 0x4f);
            assert_eq!(GXF_Track_Tag::TRACK_FPS.to_ctype(), 0x50);
            assert_eq!(GXF_Track_Tag::TRACK_LINES.to_ctype(), 0x51);
            assert_eq!(GXF_Track_Tag::TRACK_FPF.to_ctype(), 0x52);
        }
    }

    #[test]
    fn test_gxf_track_type_conversion() {
        unsafe {
            assert_eq!(GXF_Track_Type::TRACK_TYPE_MOTION_JPEG_525.to_ctype(), 3);
            assert_eq!(GXF_Track_Type::TRACK_TYPE_MOTION_JPEG_625.to_ctype(), 4);
            assert_eq!(GXF_Track_Type::TRACK_TYPE_TIME_CODE_525.to_ctype(), 7);
            assert_eq!(GXF_Track_Type::TRACK_TYPE_TIME_CODE_625.to_ctype(), 8);
            assert_eq!(GXF_Track_Type::TRACK_TYPE_AUDIO_PCM_24.to_ctype(), 9);
            assert_eq!(GXF_Track_Type::TRACK_TYPE_AUDIO_PCM_16.to_ctype(), 10);
            assert_eq!(GXF_Track_Type::TRACK_TYPE_MPEG2_525.to_ctype(), 11);
            assert_eq!(GXF_Track_Type::TRACK_TYPE_MPEG2_625.to_ctype(), 12);
            assert_eq!(GXF_Track_Type::TRACK_TYPE_ANCILLARY_DATA.to_ctype(), 21);
            assert_eq!(GXF_Track_Type::TRACK_TYPE_TIME_CODE_HD.to_ctype(), 24);
        }
    }

    #[test]
    fn test_gxf_anc_data_pres_format_conversion() {
        unsafe {
            assert_eq!(GXF_Anc_Data_Pres_Format::PRES_FORMAT_SD.to_ctype(), 1);
            assert_eq!(GXF_Anc_Data_Pres_Format::PRES_FORMAT_HD.to_ctype(), 2);
        }
    }

    #[test]
    fn test_mpeg_picture_coding_conversion() {
        unsafe {
            assert_eq!(MpegPictureCoding::CCX_MPC_NONE.to_ctype(), 0);
            assert_eq!(MpegPictureCoding::CCX_MPC_I_FRAME.to_ctype(), 1);
            assert_eq!(MpegPictureCoding::CCX_MPC_P_FRAME.to_ctype(), 2);
            assert_eq!(MpegPictureCoding::CCX_MPC_B_FRAME.to_ctype(), 3);
        }
    }

    #[test]
    fn test_mpeg_picture_struct_conversion() {
        unsafe {
            assert_eq!(MpegPictureStruct::CCX_MPS_NONE.to_ctype(), 0);
            assert_eq!(MpegPictureStruct::CCX_MPS_TOP_FIELD.to_ctype(), 1);
            assert_eq!(MpegPictureStruct::CCX_MPS_BOTTOM_FIELD.to_ctype(), 2);
            assert_eq!(MpegPictureStruct::CCX_MPS_FRAME.to_ctype(), 3);
        }
    }

    #[test]
    fn test_ccx_rational_conversion() {
        unsafe {
            let rust_rational = CcxRational { num: 30, den: 1 };
            let c_rational = rust_rational.to_ctype();
            assert_eq!(c_rational.num, 30);
            assert_eq!(c_rational.den, 1);

            let rust_rational2 = CcxRational {
                num: 24000,
                den: 1001,
            };
            let c_rational2 = rust_rational2.to_ctype();
            assert_eq!(c_rational2.num, 24000);
            assert_eq!(c_rational2.den, 1001);
        }
    }

    #[test]
    fn test_video_track_write_back() {
        unsafe {
            let layout = Layout::new::<ccx_gxf_video_track>();
            let ptr = alloc(layout) as *mut ccx_gxf_video_track;
            assert!(!ptr.is_null());

            // Initialize with zeros
            ptr::write_bytes(ptr, 0, 1);

            let rust_track = CcxGxfVideoTrack {
                track_name: "Test Video Track".to_string(),
                fs_version: 12345,
                frame_rate: CcxRational {
                    num: 30000,
                    den: 1001,
                },
                line_per_frame: 525,
                field_per_frame: 2,
                p_code: MpegPictureCoding::CCX_MPC_I_FRAME,
                p_struct: MpegPictureStruct::CCX_MPS_FRAME,
            };

            copy_ccx_gxf_video_track_from_rust_to_c(ptr, &rust_track);

            // Verify the data was copied correctly
            assert_eq!((*ptr).fs_version, 12345);
            assert_eq!((*ptr).frame_rate.num, 30000);
            assert_eq!((*ptr).frame_rate.den, 1001);
            assert_eq!((*ptr).line_per_frame, 525);
            assert_eq!((*ptr).field_per_frame, 2);
            assert_eq!((*ptr).p_code, mpeg_picture_coding_CCX_MPC_I_FRAME);
            assert_eq!((*ptr).p_struct, mpeg_picture_struct_CCX_MPS_FRAME);

            // Check track name (first few characters)
            let expected_name = b"Test Video Track\0";
            for (i, &expected_byte) in expected_name.iter().enumerate() {
                assert_eq!((*ptr).track_name[i], expected_byte as c_char);
            }

            dealloc(ptr as *mut u8, layout);
        }
    }

    #[test]
    fn test_ancillary_data_track_write_back() {
        unsafe {
            let layout = Layout::new::<ccx_gxf_ancillary_data_track>();
            let ptr = alloc(layout) as *mut ccx_gxf_ancillary_data_track;
            assert!(!ptr.is_null());

            ptr::write_bytes(ptr, 0, 1);

            let rust_track = CcxGxfAncillaryDataTrack {
                track_name: "Test AD Track".to_string(),
                id: 42,
                ad_format: GXF_Anc_Data_Pres_Format::PRES_FORMAT_HD,
                nb_field: 100,
                field_size: 256,
                packet_size: 65536,
                fs_version: 54321,
                frame_rate: 25,
                line_per_frame: 1080,
                field_per_frame: 1,
            };

            copy_ccx_gxf_ancillary_data_track_from_rust_to_c(ptr, &rust_track);

            assert_eq!((*ptr).id, 42);
            assert_eq!((*ptr).ad_format, ccx_ad_pres_format_PRES_FORMAT_HD);
            assert_eq!((*ptr).nb_field, 100);
            assert_eq!((*ptr).field_size, 256);
            assert_eq!((*ptr).packet_size, 65536);
            assert_eq!((*ptr).fs_version, 54321);
            assert_eq!((*ptr).frame_rate, 25);
            assert_eq!((*ptr).line_per_frame, 1080);
            assert_eq!((*ptr).field_per_frame, 1);

            let expected_name = b"Test AD Track\0";
            for (i, &expected_byte) in expected_name.iter().enumerate() {
                assert_eq!((*ptr).track_name[i], expected_byte as c_char);
            }

            dealloc(ptr as *mut u8, layout);
        }
    }

    #[test]
    fn test_ccx_gxf_write_back() {
        unsafe {
            let layout = Layout::new::<ccx_gxf>();
            let ptr = alloc(layout) as *mut ccx_gxf;
            assert!(!ptr.is_null());

            // Allocate memory for nested structs
            let ad_layout = Layout::new::<ccx_gxf_ancillary_data_track>();
            let ad_ptr = alloc(ad_layout) as *mut ccx_gxf_ancillary_data_track;

            let vid_layout = Layout::new::<ccx_gxf_video_track>();
            let vid_ptr = alloc(vid_layout) as *mut ccx_gxf_video_track;

            let cdp_size = 1024;
            let cdp_layout = Layout::array::<u8>(cdp_size).unwrap();
            let cdp_ptr = alloc(cdp_layout);

            // Initialize with zeros
            ptr::write_bytes(ptr, 0, 1);
            ptr::write_bytes(ad_ptr, 0, 1);
            ptr::write_bytes(vid_ptr, 0, 1);
            ptr::write_bytes(cdp_ptr, 0, cdp_size);

            // Set up the C struct pointers
            (*ptr).ad_track = ad_ptr;
            (*ptr).vid_track = vid_ptr;
            (*ptr).cdp = cdp_ptr;

            let test_cdp_data = vec![0x01, 0x02, 0x03, 0x04, 0x05];
            let rust_gxf = CcxGxf {
                nb_streams: 3,
                media_name: "Test Media File".to_string(),
                first_field_nb: 1000,
                last_field_nb: 2000,
                mark_in: 1100,
                mark_out: 1900,
                stream_size: 1048576,
                ad_track: Some(*Box::new(CcxGxfAncillaryDataTrack {
                    track_name: "Test AD".to_string(),
                    id: 99,
                    ad_format: GXF_Anc_Data_Pres_Format::PRES_FORMAT_SD,
                    nb_field: 50,
                    field_size: 128,
                    packet_size: 32768,
                    fs_version: 11111,
                    frame_rate: 30,
                    line_per_frame: 525,
                    field_per_frame: 2,
                })),
                vid_track: Some(*Box::new(CcxGxfVideoTrack {
                    track_name: "Test Video".to_string(),
                    fs_version: 22222,
                    frame_rate: CcxRational {
                        num: 24000,
                        den: 1001,
                    },
                    line_per_frame: 1080,
                    field_per_frame: 1,
                    p_code: MpegPictureCoding::CCX_MPC_P_FRAME,
                    p_struct: MpegPictureStruct::CCX_MPS_TOP_FIELD,
                })),
                cdp: Some(test_cdp_data.clone()),
                cdp_len: test_cdp_data.len(),
            };

            copy_ccx_gxf_from_rust_to_c(ptr, &rust_gxf);

            // Verify main struct
            assert_eq!((*ptr).nb_streams, 3);
            assert_eq!((*ptr).first_field_nb, 1000);
            assert_eq!((*ptr).last_field_nb, 2000);
            assert_eq!((*ptr).mark_in, 1100);
            assert_eq!((*ptr).mark_out, 1900);
            assert_eq!((*ptr).stream_size, 1048576);
            assert_eq!((*ptr).cdp_len, test_cdp_data.len());

            let expected_name = b"Test Media File\0";
            for (i, &expected_byte) in expected_name.iter().enumerate() {
                assert_eq!((*ptr).media_name[i], expected_byte as c_char);
            }

            // Verify ad_track was copied
            assert_eq!((*ad_ptr).id, 99);
            assert_eq!((*ad_ptr).ad_format, ccx_ad_pres_format_PRES_FORMAT_SD);
            assert_eq!((*ad_ptr).nb_field, 50);
            assert_eq!((*ad_ptr).field_size, 128);
            assert_eq!((*ad_ptr).packet_size, 32768);
            assert_eq!((*ad_ptr).fs_version, 11111);

            let expected_ad_name = b"Test AD\0";
            for (i, &expected_byte) in expected_ad_name.iter().enumerate() {
                assert_eq!((*ad_ptr).track_name[i], expected_byte as c_char);
            }

            // Verify vid_track was copied
            assert_eq!((*vid_ptr).fs_version, 22222);
            assert_eq!((*vid_ptr).frame_rate.num, 24000);
            assert_eq!((*vid_ptr).frame_rate.den, 1001);
            assert_eq!((*vid_ptr).line_per_frame, 1080);
            assert_eq!((*vid_ptr).field_per_frame, 1);
            assert_eq!((*vid_ptr).p_code, mpeg_picture_coding_CCX_MPC_P_FRAME);
            assert_eq!((*vid_ptr).p_struct, mpeg_picture_struct_CCX_MPS_TOP_FIELD);

            let expected_vid_name = b"Test Video\0";
            for (i, &expected_byte) in expected_vid_name.iter().enumerate() {
                assert_eq!((*vid_ptr).track_name[i], expected_byte as c_char);
            }

            // Verify cdp buffer was copied
            for (i, &expected_byte) in test_cdp_data.iter().enumerate() {
                assert_eq!(*cdp_ptr.add(i), expected_byte);
            }

            // Clean up
            dealloc(ptr as *mut u8, layout);
            dealloc(ad_ptr as *mut u8, ad_layout);
            dealloc(vid_ptr as *mut u8, vid_layout);
            dealloc(cdp_ptr, cdp_layout);
        }
    }

    #[test]
    fn test_string_to_c_array_conversion() {
        let test_string = "Hello, World!";
        let c_array = string_to_c_array(test_string);

        let expected = b"Hello, World!\0";
        for (i, &expected_byte) in expected.iter().enumerate() {
            assert_eq!(c_array[i], expected_byte as c_char);
        }

        // Rest should be zeros
        for i in expected.len()..256 {
            assert_eq!(c_array[i], 0);
        }
    }

    #[test]
    fn test_long_string_truncation() {
        let long_string = "a".repeat(300); // Longer than 255 characters
        let c_array = string_to_c_array(&long_string);

        // Should be truncated to fit in 255 characters + null terminator
        for i in 0..255 {
            assert_eq!(c_array[i], b'a' as c_char);
        }
        assert_eq!(c_array[255], 0); // Null terminator
    }

    #[test]
    fn test_ccx_gxf_write_back_with_null_pointers() {
        unsafe {
            let layout = Layout::new::<ccx_gxf>();
            let ptr = alloc(layout) as *mut ccx_gxf;
            assert!(!ptr.is_null());

            // Initialize with zeros (null pointers)
            ptr::write_bytes(ptr, 0, 1);

            let rust_gxf = CcxGxf {
                nb_streams: 2,
                media_name: "Test With Nulls".to_string(),
                first_field_nb: 500,
                last_field_nb: 1500,
                mark_in: 600,
                mark_out: 1400,
                stream_size: 512000,
                ad_track: Some(*Box::new(CcxGxfAncillaryDataTrack {
                    track_name: "Should Not Copy".to_string(),
                    id: 123,
                    ad_format: GXF_Anc_Data_Pres_Format::PRES_FORMAT_HD,
                    nb_field: 10,
                    field_size: 64,
                    packet_size: 16384,
                    fs_version: 99999,
                    frame_rate: 60,
                    line_per_frame: 720,
                    field_per_frame: 1,
                })),
                vid_track: Some(*Box::new(CcxGxfVideoTrack {
                    track_name: "Should Not Copy Either".to_string(),
                    fs_version: 88888,
                    frame_rate: CcxRational {
                        num: 60000,
                        den: 1001,
                    },
                    line_per_frame: 720,
                    field_per_frame: 1,
                    p_code: MpegPictureCoding::CCX_MPC_B_FRAME,
                    p_struct: MpegPictureStruct::CCX_MPS_BOTTOM_FIELD,
                })),
                cdp: Some(vec![0xAA, 0xBB, 0xCC]),
                cdp_len: 3,
            };

            copy_ccx_gxf_from_rust_to_c(ptr, &rust_gxf);

            // Should copy basic fields but not nested structs/buffers because pointers are null
            assert_eq!((*ptr).nb_streams, 2);
            assert_eq!((*ptr).first_field_nb, 500);
            assert_eq!((*ptr).last_field_nb, 1500);
            assert_eq!((*ptr).mark_in, 600);
            assert_eq!((*ptr).mark_out, 1400);
            assert_eq!((*ptr).stream_size, 512000);
            assert_eq!((*ptr).cdp_len, 3);

            // Pointers should still be null
            assert!((*ptr).ad_track.is_null());
            assert!((*ptr).vid_track.is_null());
            assert!((*ptr).cdp.is_null());

            let expected_name = b"Test With Nulls\0";
            for (i, &expected_byte) in expected_name.iter().enumerate() {
                assert_eq!((*ptr).media_name[i], expected_byte as c_char);
            }

            dealloc(ptr as *mut u8, layout);
        }
    }

    #[test]
    fn test_cdp_buffer_boundary_conditions() {
        unsafe {
            let layout = Layout::new::<ccx_gxf>();
            let ptr = alloc(layout) as *mut ccx_gxf;

            let cdp_size = 10;
            let cdp_layout = Layout::array::<u8>(cdp_size).unwrap();
            let cdp_ptr = alloc(cdp_layout);

            ptr::write_bytes(ptr, 0, 1);
            ptr::write_bytes(cdp_ptr, 0xFF, cdp_size); // Fill with 0xFF to detect changes

            (*ptr).cdp = cdp_ptr;

            // Test with larger data than allocated buffer
            let large_cdp_data = vec![
                0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE,
                0xFF,
            ];
            let rust_gxf = CcxGxf {
                nb_streams: 1,
                media_name: "CDP Test".to_string(),
                first_field_nb: 0,
                last_field_nb: 0,
                mark_in: 0,
                mark_out: 0,
                stream_size: 0,
                ad_track: None,
                vid_track: None,
                cdp: Some(large_cdp_data.clone()),
                cdp_len: cdp_size, // Smaller than actual data
            };

            copy_ccx_gxf_from_rust_to_c(ptr, &rust_gxf);

            // Should only copy up to cdp_len bytes
            for i in 0..cdp_size {
                assert_eq!(*cdp_ptr.add(i), large_cdp_data[i]);
            }

            dealloc(ptr as *mut u8, layout);
            dealloc(cdp_ptr, cdp_layout);
        }
    }
}

#[cfg(test)]
mod tests_c_to_rust {
    use super::*;
    use crate::bindings::{
        ccx_ad_pres_format_PRES_FORMAT_HD, ccx_ad_pres_format_PRES_FORMAT_SD, ccx_rational,
        mpeg_picture_coding_CCX_MPC_B_FRAME, mpeg_picture_coding_CCX_MPC_I_FRAME,
        mpeg_picture_coding_CCX_MPC_P_FRAME, mpeg_picture_struct_CCX_MPS_BOTTOM_FIELD,
        mpeg_picture_struct_CCX_MPS_FRAME, mpeg_picture_struct_CCX_MPS_TOP_FIELD,
    };
    use crate::ctorust::FromCType;
    use std::alloc::{alloc, dealloc, Layout};
    use std::os::raw::{c_int, c_uchar};
    use std::ptr;

    #[test]
    fn test_gxf_pkt_type_from_c_conversion() {
        unsafe {
            assert_eq!(GXF_Pkt_Type::from_ctype(0xbc), Some(GXF_Pkt_Type::PKT_MAP));
            assert_eq!(
                GXF_Pkt_Type::from_ctype(0xbf),
                Some(GXF_Pkt_Type::PKT_MEDIA)
            );
            assert_eq!(GXF_Pkt_Type::from_ctype(0xfb), Some(GXF_Pkt_Type::PKT_EOS));
            assert_eq!(GXF_Pkt_Type::from_ctype(0xfc), Some(GXF_Pkt_Type::PKT_FLT));
            assert_eq!(GXF_Pkt_Type::from_ctype(0xfd), Some(GXF_Pkt_Type::PKT_UMF));
            assert_eq!(GXF_Pkt_Type::from_ctype(0x99), None); // Invalid value
        }
    }

    #[test]
    fn test_gxf_mat_tag_from_c_conversion() {
        unsafe {
            assert_eq!(GXF_Mat_Tag::from_ctype(0x40), Some(GXF_Mat_Tag::MAT_NAME));
            assert_eq!(
                GXF_Mat_Tag::from_ctype(0x41),
                Some(GXF_Mat_Tag::MAT_FIRST_FIELD)
            );
            assert_eq!(
                GXF_Mat_Tag::from_ctype(0x42),
                Some(GXF_Mat_Tag::MAT_LAST_FIELD)
            );
            assert_eq!(
                GXF_Mat_Tag::from_ctype(0x43),
                Some(GXF_Mat_Tag::MAT_MARK_IN)
            );
            assert_eq!(
                GXF_Mat_Tag::from_ctype(0x44),
                Some(GXF_Mat_Tag::MAT_MARK_OUT)
            );
            assert_eq!(GXF_Mat_Tag::from_ctype(0x45), Some(GXF_Mat_Tag::MAT_SIZE));
            assert_eq!(GXF_Mat_Tag::from_ctype(0x99), None); // Invalid value
        }
    }

    #[test]
    fn test_gxf_track_tag_from_c_conversion() {
        unsafe {
            assert_eq!(
                GXF_Track_Tag::from_ctype(0x4c),
                Some(GXF_Track_Tag::TRACK_NAME)
            );
            assert_eq!(
                GXF_Track_Tag::from_ctype(0x4d),
                Some(GXF_Track_Tag::TRACK_AUX)
            );
            assert_eq!(
                GXF_Track_Tag::from_ctype(0x4e),
                Some(GXF_Track_Tag::TRACK_VER)
            );
            assert_eq!(
                GXF_Track_Tag::from_ctype(0x4f),
                Some(GXF_Track_Tag::TRACK_MPG_AUX)
            );
            assert_eq!(
                GXF_Track_Tag::from_ctype(0x50),
                Some(GXF_Track_Tag::TRACK_FPS)
            );
            assert_eq!(
                GXF_Track_Tag::from_ctype(0x51),
                Some(GXF_Track_Tag::TRACK_LINES)
            );
            assert_eq!(
                GXF_Track_Tag::from_ctype(0x52),
                Some(GXF_Track_Tag::TRACK_FPF)
            );
            assert_eq!(GXF_Track_Tag::from_ctype(0x99), None); // Invalid value
        }
    }

    #[test]
    fn test_gxf_track_type_from_c_conversion() {
        unsafe {
            assert_eq!(
                GXF_Track_Type::from_ctype(3),
                Some(GXF_Track_Type::TRACK_TYPE_MOTION_JPEG_525)
            );
            assert_eq!(
                GXF_Track_Type::from_ctype(4),
                Some(GXF_Track_Type::TRACK_TYPE_MOTION_JPEG_625)
            );
            assert_eq!(
                GXF_Track_Type::from_ctype(7),
                Some(GXF_Track_Type::TRACK_TYPE_TIME_CODE_525)
            );
            assert_eq!(
                GXF_Track_Type::from_ctype(8),
                Some(GXF_Track_Type::TRACK_TYPE_TIME_CODE_625)
            );
            assert_eq!(
                GXF_Track_Type::from_ctype(9),
                Some(GXF_Track_Type::TRACK_TYPE_AUDIO_PCM_24)
            );
            assert_eq!(
                GXF_Track_Type::from_ctype(10),
                Some(GXF_Track_Type::TRACK_TYPE_AUDIO_PCM_16)
            );
            assert_eq!(
                GXF_Track_Type::from_ctype(11),
                Some(GXF_Track_Type::TRACK_TYPE_MPEG2_525)
            );
            assert_eq!(
                GXF_Track_Type::from_ctype(12),
                Some(GXF_Track_Type::TRACK_TYPE_MPEG2_625)
            );
            assert_eq!(
                GXF_Track_Type::from_ctype(21),
                Some(GXF_Track_Type::TRACK_TYPE_ANCILLARY_DATA)
            );
            assert_eq!(
                GXF_Track_Type::from_ctype(24),
                Some(GXF_Track_Type::TRACK_TYPE_TIME_CODE_HD)
            );
            assert_eq!(GXF_Track_Type::from_ctype(99), None); // Invalid value
        }
    }

    #[test]
    fn test_gxf_anc_data_pres_format_from_c_conversion() {
        unsafe {
            assert_eq!(
                GXF_Anc_Data_Pres_Format::from_ctype(1),
                Some(GXF_Anc_Data_Pres_Format::PRES_FORMAT_SD)
            );
            assert_eq!(
                GXF_Anc_Data_Pres_Format::from_ctype(2),
                Some(GXF_Anc_Data_Pres_Format::PRES_FORMAT_HD)
            );
            assert_eq!(GXF_Anc_Data_Pres_Format::from_ctype(99), None); // Invalid value
        }
    }

    #[test]
    fn test_mpeg_picture_coding_from_c_conversion() {
        unsafe {
            assert_eq!(
                MpegPictureCoding::from_ctype(0),
                Some(MpegPictureCoding::CCX_MPC_NONE)
            );
            assert_eq!(
                MpegPictureCoding::from_ctype(1),
                Some(MpegPictureCoding::CCX_MPC_I_FRAME)
            );
            assert_eq!(
                MpegPictureCoding::from_ctype(2),
                Some(MpegPictureCoding::CCX_MPC_P_FRAME)
            );
            assert_eq!(
                MpegPictureCoding::from_ctype(3),
                Some(MpegPictureCoding::CCX_MPC_B_FRAME)
            );
            assert_eq!(MpegPictureCoding::from_ctype(99), None); // Invalid value
        }
    }

    #[test]
    fn test_mpeg_picture_struct_from_c_conversion() {
        unsafe {
            assert_eq!(
                MpegPictureStruct::from_ctype(0),
                Some(MpegPictureStruct::CCX_MPS_NONE)
            );
            assert_eq!(
                MpegPictureStruct::from_ctype(1),
                Some(MpegPictureStruct::CCX_MPS_TOP_FIELD)
            );
            assert_eq!(
                MpegPictureStruct::from_ctype(2),
                Some(MpegPictureStruct::CCX_MPS_BOTTOM_FIELD)
            );
            assert_eq!(
                MpegPictureStruct::from_ctype(3),
                Some(MpegPictureStruct::CCX_MPS_FRAME)
            );
            assert_eq!(MpegPictureStruct::from_ctype(99), None); // Invalid value
        }
    }

    #[test]
    fn test_ccx_rational_from_c_conversion() {
        unsafe {
            let c_rational = ccx_rational { num: 30, den: 1 };
            let rust_rational = CcxRational::from_ctype(c_rational).unwrap();
            assert_eq!(rust_rational.num, 30);
            assert_eq!(rust_rational.den, 1);

            let c_rational2 = ccx_rational {
                num: 24000,
                den: 1001,
            };
            let rust_rational2 = CcxRational::from_ctype(c_rational2).unwrap();
            assert_eq!(rust_rational2.num, 24000);
            assert_eq!(rust_rational2.den, 1001);
        }
    }

    #[test]
    fn test_c_array_to_string_conversion() {
        unsafe {
            let mut c_array = [0i8; 256];
            let test_string = b"Hello, World!\0";

            for (i, &byte) in test_string.iter().enumerate() {
                c_array[i] = byte as c_char;
            }

            let rust_string = c_array_to_string(&c_array);
            assert_eq!(rust_string, "Hello, World!");
        }
    }

    #[test]
    fn test_c_array_to_string_empty() {
        unsafe {
            let c_array = [0i8; 256]; // All zeros
            let rust_string = c_array_to_string(&c_array);
            assert_eq!(rust_string, "");
        }
    }

    #[test]
    fn test_c_array_to_string_no_null_terminator() {
        unsafe {
            let c_array = [b'A' as c_char; 256]; // Fill entire array
            let rust_string = c_array_to_string(&c_array);
            assert_eq!(rust_string.len(), 256);
            assert!(rust_string.chars().all(|c| c == 'A'));
        }
    }

    #[test]
    fn test_video_track_read_from_c() {
        unsafe {
            let layout = Layout::new::<ccx_gxf_video_track>();
            let ptr = alloc(layout) as *mut ccx_gxf_video_track;
            assert!(!ptr.is_null());

            // Initialize with test data
            ptr::write_bytes(ptr, 0, 1);

            (*ptr).fs_version = 12345;
            (*ptr).frame_rate = ccx_rational {
                num: 30000,
                den: 1001,
            };
            (*ptr).line_per_frame = 525;
            (*ptr).field_per_frame = 2;
            (*ptr).p_code = mpeg_picture_coding_CCX_MPC_I_FRAME;
            (*ptr).p_struct = mpeg_picture_struct_CCX_MPS_FRAME;

            // Set track name
            let name_bytes = b"Test Video Track\0";
            for (i, &byte) in name_bytes.iter().enumerate() {
                (*ptr).track_name[i] = byte as c_char;
            }

            // Convert to Rust
            let rust_track = copy_ccx_gxf_video_track_from_c_to_rust(ptr).unwrap();

            // Verify conversion
            assert_eq!(rust_track.track_name, "Test Video Track");
            assert_eq!(rust_track.fs_version, 12345);
            assert_eq!(rust_track.frame_rate.num, 30000);
            assert_eq!(rust_track.frame_rate.den, 1001);
            assert_eq!(rust_track.line_per_frame, 525);
            assert_eq!(rust_track.field_per_frame, 2);
            assert_eq!(rust_track.p_code, MpegPictureCoding::CCX_MPC_I_FRAME);
            assert_eq!(rust_track.p_struct, MpegPictureStruct::CCX_MPS_FRAME);

            dealloc(ptr as *mut u8, layout);
        }
    }

    #[test]
    fn test_video_track_read_null_pointer() {
        unsafe {
            let result = copy_ccx_gxf_video_track_from_c_to_rust(ptr::null());
            assert!(result.is_none());
        }
    }

    #[test]
    fn test_ancillary_data_track_read_from_c() {
        unsafe {
            let layout = Layout::new::<ccx_gxf_ancillary_data_track>();
            let ptr = alloc(layout) as *mut ccx_gxf_ancillary_data_track;
            assert!(!ptr.is_null());

            ptr::write_bytes(ptr, 0, 1);

            (*ptr).id = 42;
            (*ptr).ad_format = ccx_ad_pres_format_PRES_FORMAT_HD;
            (*ptr).nb_field = 100;
            (*ptr).field_size = 256;
            (*ptr).packet_size = 65536;
            (*ptr).fs_version = 54321;
            (*ptr).frame_rate = 25;
            (*ptr).line_per_frame = 1080;
            (*ptr).field_per_frame = 1;

            let name_bytes = b"Test AD Track\0";
            for (i, &byte) in name_bytes.iter().enumerate() {
                (*ptr).track_name[i] = byte as c_char;
            }

            let rust_track = copy_ccx_gxf_ancillary_data_track_from_c_to_rust(ptr).unwrap();

            assert_eq!(rust_track.track_name, "Test AD Track");
            assert_eq!(rust_track.id, 42);
            assert_eq!(
                rust_track.ad_format,
                GXF_Anc_Data_Pres_Format::PRES_FORMAT_HD
            );
            assert_eq!(rust_track.nb_field, 100);
            assert_eq!(rust_track.field_size, 256);
            assert_eq!(rust_track.packet_size, 65536);
            assert_eq!(rust_track.fs_version, 54321);
            assert_eq!(rust_track.frame_rate, 25);
            assert_eq!(rust_track.line_per_frame, 1080);
            assert_eq!(rust_track.field_per_frame, 1);

            dealloc(ptr as *mut u8, layout);
        }
    }

    #[test]
    fn test_ancillary_data_track_read_null_pointer() {
        unsafe {
            let result = copy_ccx_gxf_ancillary_data_track_from_c_to_rust(ptr::null());
            assert!(result.is_none());
        }
    }

    #[test]
    fn test_ccx_gxf_read_from_c_complete() {
        unsafe {
            let layout = Layout::new::<ccx_gxf>();
            let ptr = alloc(layout) as *mut ccx_gxf;
            assert!(!ptr.is_null());

            // Allocate memory for nested structs
            let ad_layout = Layout::new::<ccx_gxf_ancillary_data_track>();
            let ad_ptr = alloc(ad_layout) as *mut ccx_gxf_ancillary_data_track;
            assert!(!ad_ptr.is_null());

            let vid_layout = Layout::new::<ccx_gxf_video_track>();
            let vid_ptr = alloc(vid_layout) as *mut ccx_gxf_video_track;
            assert!(!vid_ptr.is_null());

            // Initialize ancillary data track
            ptr::write_bytes(ad_ptr, 0, 1);
            (*ad_ptr).id = 42;
            (*ad_ptr).ad_format = ccx_ad_pres_format_PRES_FORMAT_HD;
            (*ad_ptr).nb_field = 100;
            (*ad_ptr).field_size = 256;
            (*ad_ptr).packet_size = 65536;
            (*ad_ptr).fs_version = 54321;
            (*ad_ptr).frame_rate = 25;
            (*ad_ptr).line_per_frame = 1080;
            (*ad_ptr).field_per_frame = 1;

            let ad_name_bytes = b"Test AD Track\0";
            for (i, &byte) in ad_name_bytes.iter().enumerate() {
                (*ad_ptr).track_name[i] = byte as c_char;
            }

            // Initialize video track
            ptr::write_bytes(vid_ptr, 0, 1);
            (*vid_ptr).fs_version = 12345;
            (*vid_ptr).frame_rate = ccx_rational {
                num: 30000,
                den: 1001,
            };
            (*vid_ptr).line_per_frame = 525;
            (*vid_ptr).field_per_frame = 2;
            (*vid_ptr).p_code = mpeg_picture_coding_CCX_MPC_I_FRAME;
            (*vid_ptr).p_struct = mpeg_picture_struct_CCX_MPS_FRAME;

            let vid_name_bytes = b"Test Video Track\0";
            for (i, &byte) in vid_name_bytes.iter().enumerate() {
                (*vid_ptr).track_name[i] = byte as c_char;
            }

            // Initialize ccx_gxf
            ptr::write_bytes(ptr, 0, 1);
            (*ptr).nb_streams = 2;
            (*ptr).media_name = [0; 256];
            let media_name_bytes = b"Test Media\0";
            for (i, &byte) in media_name_bytes.iter().enumerate() {
                (*ptr).media_name[i] = byte as c_char;
            }
            (*ptr).first_field_nb = 1;
            (*ptr).last_field_nb = 100;
            (*ptr).mark_in = 10;
            (*ptr).mark_out = 90;
            (*ptr).stream_size = 1024;
            (*ptr).ad_track = ad_ptr;
            (*ptr).vid_track = vid_ptr;
            (*ptr).cdp_len = 0;

            // Convert to Rust
            let rust_gxf = copy_ccx_gxf_from_c_to_rust(ptr).unwrap();

            // Verify conversion
            assert_eq!(rust_gxf.nb_streams, 2);
            assert_eq!(rust_gxf.media_name, "Test Media");
            assert_eq!(rust_gxf.first_field_nb, 1);
            assert_eq!(rust_gxf.last_field_nb, 100);
            assert_eq!(rust_gxf.mark_in, 10);
            assert_eq!(rust_gxf.mark_out, 90);
            assert_eq!(rust_gxf.stream_size, 1024);

            let rust_ad_track = rust_gxf.ad_track.unwrap();
            assert_eq!(rust_ad_track.track_name, "Test AD Track");
            assert_eq!(rust_ad_track.id, 42);
            assert_eq!(
                rust_ad_track.ad_format,
                GXF_Anc_Data_Pres_Format::PRES_FORMAT_HD
            );
            assert_eq!(rust_ad_track.nb_field, 100);
            assert_eq!(rust_ad_track.field_size, 256);
            assert_eq!(rust_ad_track.packet_size, 65536);
            assert_eq!(rust_ad_track.fs_version, 54321);
            assert_eq!(rust_ad_track.frame_rate, 25);
            assert_eq!(rust_ad_track.line_per_frame, 1080);
            assert_eq!(rust_ad_track.field_per_frame, 1);

            let rust_vid_track = rust_gxf.vid_track.unwrap();
            assert_eq!(rust_vid_track.track_name, "Test Video Track");
            assert_eq!(rust_vid_track.fs_version, 12345);
            assert_eq!(rust_vid_track.frame_rate.num, 30000);
            assert_eq!(rust_vid_track.frame_rate.den, 1001);
            assert_eq!(rust_vid_track.line_per_frame, 525);
            assert_eq!(rust_vid_track.field_per_frame, 2);
            assert_eq!(rust_vid_track.p_code, MpegPictureCoding::CCX_MPC_I_FRAME);
            assert_eq!(rust_vid_track.p_struct, MpegPictureStruct::CCX_MPS_FRAME);

            // Cleanup
            dealloc(ad_ptr as *mut u8, ad_layout);
            dealloc(vid_ptr as *mut u8, vid_layout);
            dealloc(ptr as *mut u8, layout);
        }
    }

    #[test]
    fn test_ccx_gxf_read_null_pointer() {
        unsafe {
            let result = copy_ccx_gxf_from_c_to_rust(ptr::null());
            assert!(result.is_none());
        }
    }

    #[test]
    fn test_ccx_gxf_read_with_null_tracks() {
        unsafe {
            let layout = Layout::new::<ccx_gxf>();
            let ptr = alloc(layout) as *mut ccx_gxf;
            assert!(!ptr.is_null());

            ptr::write_bytes(ptr, 0, 1);
            (*ptr).nb_streams = 1;
            (*ptr).first_field_nb = 5;
            (*ptr).last_field_nb = 50;
            (*ptr).mark_in = 0;
            (*ptr).mark_out = 45;
            (*ptr).stream_size = 2048;
            (*ptr).ad_track = ptr::null_mut();
            (*ptr).vid_track = ptr::null_mut();
            (*ptr).cdp = ptr::null_mut();
            (*ptr).cdp_len = 0;

            let media_name_bytes = b"Null Tracks Media\0";
            for (i, &byte) in media_name_bytes.iter().enumerate() {
                (*ptr).media_name[i] = byte as c_char;
            }

            let rust_gxf = copy_ccx_gxf_from_c_to_rust(ptr).unwrap();

            assert_eq!(rust_gxf.nb_streams, 1);
            assert_eq!(rust_gxf.media_name, "Null Tracks Media");
            assert_eq!(rust_gxf.first_field_nb, 5);
            assert_eq!(rust_gxf.last_field_nb, 50);
            assert_eq!(rust_gxf.mark_in, 0);
            assert_eq!(rust_gxf.mark_out, 45);
            assert_eq!(rust_gxf.stream_size, 2048);
            assert!(rust_gxf.ad_track.is_none());
            assert!(rust_gxf.vid_track.is_none());
            assert!(rust_gxf.cdp.is_none());
            assert_eq!(rust_gxf.cdp_len, 0);

            dealloc(ptr as *mut u8, layout);
        }
    }

    #[test]
    fn test_ccx_gxf_read_with_cdp_buffer() {
        unsafe {
            let layout = Layout::new::<ccx_gxf>();
            let ptr = alloc(layout) as *mut ccx_gxf;
            assert!(!ptr.is_null());

            // Create CDP buffer
            let cdp_data = vec![0x96u8, 0x69, 0x55, 0x3F, 0x43, 0x00, 0x00, 0x72];
            let cdp_len = cdp_data.len();
            let cdp_ptr = cdp_data.as_ptr();

            ptr::write_bytes(ptr, 0, 1);
            (*ptr).nb_streams = 1;
            (*ptr).cdp = cdp_ptr as *mut c_uchar;
            (*ptr).cdp_len = cdp_len;
            (*ptr).ad_track = ptr::null_mut();
            (*ptr).vid_track = ptr::null_mut();

            let rust_gxf = copy_ccx_gxf_from_c_to_rust(ptr).unwrap();

            let rust_cdp = rust_gxf.cdp.unwrap();
            assert_eq!(rust_cdp.len(), cdp_len);
            assert_eq!(rust_cdp, cdp_data);
            assert_eq!(rust_gxf.cdp_len, cdp_len);

            dealloc(ptr as *mut u8, layout);
        }
    }

    #[test]
    fn test_ccx_gxf_read_with_zero_cdp_len() {
        unsafe {
            let layout = Layout::new::<ccx_gxf>();
            let ptr = alloc(layout) as *mut ccx_gxf;
            assert!(!ptr.is_null());

            let cdp_data = [0x96u8, 0x69, 0x55, 0x3F];
            let cdp_ptr = cdp_data.as_ptr();

            ptr::write_bytes(ptr, 0, 1);
            (*ptr).cdp = cdp_ptr as *mut c_uchar;
            (*ptr).cdp_len = 0; // Zero length should result in None
            (*ptr).ad_track = ptr::null_mut();
            (*ptr).vid_track = ptr::null_mut();

            let rust_gxf = copy_ccx_gxf_from_c_to_rust(ptr).unwrap();

            assert!(rust_gxf.cdp.is_none());
            assert_eq!(rust_gxf.cdp_len, 0);

            dealloc(ptr as *mut u8, layout);
        }
    }

    #[test]
    fn test_video_track_with_different_picture_types() {
        unsafe {
            let layout = Layout::new::<ccx_gxf_video_track>();
            let ptr = alloc(layout) as *mut ccx_gxf_video_track;

            // Test P-frame
            ptr::write_bytes(ptr, 0, 1);
            (*ptr).p_code = mpeg_picture_coding_CCX_MPC_P_FRAME;
            (*ptr).p_struct = mpeg_picture_struct_CCX_MPS_TOP_FIELD;
            (*ptr).frame_rate = ccx_rational { num: 25, den: 1 };

            let rust_track = copy_ccx_gxf_video_track_from_c_to_rust(ptr).unwrap();
            assert_eq!(rust_track.p_code, MpegPictureCoding::CCX_MPC_P_FRAME);
            assert_eq!(rust_track.p_struct, MpegPictureStruct::CCX_MPS_TOP_FIELD);

            // Test B-frame
            (*ptr).p_code = mpeg_picture_coding_CCX_MPC_B_FRAME;
            (*ptr).p_struct = mpeg_picture_struct_CCX_MPS_BOTTOM_FIELD;

            let rust_track = copy_ccx_gxf_video_track_from_c_to_rust(ptr).unwrap();
            assert_eq!(rust_track.p_code, MpegPictureCoding::CCX_MPC_B_FRAME);
            assert_eq!(rust_track.p_struct, MpegPictureStruct::CCX_MPS_BOTTOM_FIELD);

            dealloc(ptr as *mut u8, layout);
        }
    }

    #[test]
    fn test_video_track_with_invalid_enum_values() {
        unsafe {
            let layout = Layout::new::<ccx_gxf_video_track>();
            let ptr = alloc(layout) as *mut ccx_gxf_video_track;

            ptr::write_bytes(ptr, 0, 1);
            (*ptr).p_code = 999; // Invalid value
            (*ptr).p_struct = mpeg_picture_struct_CCX_MPS_FRAME;
            (*ptr).frame_rate = ccx_rational { num: 30, den: 1 };

            let result = copy_ccx_gxf_video_track_from_c_to_rust(ptr);
            assert!(result.is_none()); // Should fail due to invalid p_code

            // Test invalid p_struct
            (*ptr).p_code = mpeg_picture_coding_CCX_MPC_I_FRAME;
            (*ptr).p_struct = 888; // Invalid value

            let result = copy_ccx_gxf_video_track_from_c_to_rust(ptr);
            assert!(result.is_none()); // Should fail due to invalid p_struct

            dealloc(ptr as *mut u8, layout);
        }
    }

    #[test]
    fn test_ancillary_data_track_with_invalid_ad_format() {
        unsafe {
            let layout = Layout::new::<ccx_gxf_ancillary_data_track>();
            let ptr = alloc(layout) as *mut ccx_gxf_ancillary_data_track;

            ptr::write_bytes(ptr, 0, 1);
            (*ptr).ad_format = 777; // Invalid value

            let result = copy_ccx_gxf_ancillary_data_track_from_c_to_rust(ptr);
            assert!(result.is_none()); // Should fail due to invalid ad_format

            dealloc(ptr as *mut u8, layout);
        }
    }

    #[test]
    fn test_c_array_to_string_with_unicode() {
        unsafe {
            let mut c_array = [0i8; 256];
            let test_string = "Hello, 世界!\0".as_bytes();

            for (i, &byte) in test_string.iter().enumerate() {
                if i < 256 {
                    c_array[i] = byte as c_char;
                }
            }

            let rust_string = c_array_to_string(&c_array);
            assert_eq!(rust_string, "Hello, 世界!");
        }
    }

    #[test]
    fn test_c_array_to_string_with_special_chars() {
        unsafe {
            let mut c_array = [0i8; 256];
            let test_string = b"Test\t\n\r\x00Special\0";

            for (i, &byte) in test_string.iter().enumerate() {
                if i < 256 {
                    c_array[i] = byte as c_char;
                }
            }

            let rust_string = c_array_to_string(&c_array);
            assert_eq!(rust_string, "Test\t\n\r");
        }
    }

    #[test]
    fn test_rational_with_zero_denominator() {
        unsafe {
            let c_rational = ccx_rational { num: 30, den: 0 };
            let rust_rational = CcxRational::from_ctype(c_rational).unwrap();
            assert_eq!(rust_rational.num, 30);
            assert_eq!(rust_rational.den, 0);
        }
    }

    #[test]
    fn test_rational_with_negative_values() {
        unsafe {
            let c_rational = ccx_rational { num: -30, den: 1 };
            let rust_rational = CcxRational::from_ctype(c_rational).unwrap();
            assert_eq!(rust_rational.num, -30);
            assert_eq!(rust_rational.den, 1);

            let c_rational2 = ccx_rational { num: 30, den: -1 };
            let rust_rational2 = CcxRational::from_ctype(c_rational2).unwrap();
            assert_eq!(rust_rational2.num, 30);
            assert_eq!(rust_rational2.den, -1);
        }
    }

    #[test]
    fn test_ccx_gxf_with_only_video_track() {
        unsafe {
            let layout = Layout::new::<ccx_gxf>();
            let ptr = alloc(layout) as *mut ccx_gxf;

            let vid_layout = Layout::new::<ccx_gxf_video_track>();
            let vid_ptr = alloc(vid_layout) as *mut ccx_gxf_video_track;

            // Initialize only video track
            ptr::write_bytes(vid_ptr, 0, 1);
            (*vid_ptr).fs_version = 999;
            (*vid_ptr).frame_rate = ccx_rational { num: 24, den: 1 };
            (*vid_ptr).line_per_frame = 1080;
            (*vid_ptr).field_per_frame = 1;
            (*vid_ptr).p_code = mpeg_picture_coding_CCX_MPC_B_FRAME;
            (*vid_ptr).p_struct = mpeg_picture_struct_CCX_MPS_FRAME;

            let name = b"Only Video\0";
            for (i, &byte) in name.iter().enumerate() {
                (*vid_ptr).track_name[i] = byte as c_char;
            }

            ptr::write_bytes(ptr, 0, 1);
            (*ptr).nb_streams = 1;
            (*ptr).vid_track = vid_ptr;
            (*ptr).ad_track = ptr::null_mut();
            (*ptr).cdp = ptr::null_mut();
            (*ptr).cdp_len = 0;

            let rust_gxf = copy_ccx_gxf_from_c_to_rust(ptr).unwrap();

            assert_eq!(rust_gxf.nb_streams, 1);
            assert!(rust_gxf.ad_track.is_none());
            assert!(rust_gxf.vid_track.is_some());
            assert!(rust_gxf.cdp.is_none());

            let vid_track = rust_gxf.vid_track.unwrap();
            assert_eq!(vid_track.track_name, "Only Video");
            assert_eq!(vid_track.fs_version, 999);

            dealloc(vid_ptr as *mut u8, vid_layout);
            dealloc(ptr as *mut u8, layout);
        }
    }

    #[test]
    fn test_ccx_gxf_with_only_ancillary_track() {
        unsafe {
            let layout = Layout::new::<ccx_gxf>();
            let ptr = alloc(layout) as *mut ccx_gxf;

            let ad_layout = Layout::new::<ccx_gxf_ancillary_data_track>();
            let ad_ptr = alloc(ad_layout) as *mut ccx_gxf_ancillary_data_track;

            // Initialize only ancillary track
            ptr::write_bytes(ad_ptr, 0, 1);
            (*ad_ptr).id = 123;
            (*ad_ptr).ad_format = ccx_ad_pres_format_PRES_FORMAT_SD;
            (*ad_ptr).nb_field = 50;
            (*ad_ptr).field_size = 128;
            (*ad_ptr).packet_size = 1024;

            let name = b"Only Ancillary\0";
            for (i, &byte) in name.iter().enumerate() {
                (*ad_ptr).track_name[i] = byte as c_char;
            }

            ptr::write_bytes(ptr, 0, 1);
            (*ptr).nb_streams = 1;
            (*ptr).ad_track = ad_ptr;
            (*ptr).vid_track = ptr::null_mut();
            (*ptr).cdp = ptr::null_mut();
            (*ptr).cdp_len = 0;

            let rust_gxf = copy_ccx_gxf_from_c_to_rust(ptr).unwrap();

            assert_eq!(rust_gxf.nb_streams, 1);
            assert!(rust_gxf.ad_track.is_some());
            assert!(rust_gxf.vid_track.is_none());
            assert!(rust_gxf.cdp.is_none());

            let ad_track = rust_gxf.ad_track.unwrap();
            assert_eq!(ad_track.track_name, "Only Ancillary");
            assert_eq!(ad_track.id, 123);
            assert_eq!(ad_track.ad_format, GXF_Anc_Data_Pres_Format::PRES_FORMAT_SD);

            dealloc(ad_ptr as *mut u8, ad_layout);
            dealloc(ptr as *mut u8, layout);
        }
    }

    #[test]
    fn test_large_cdp_buffer() {
        unsafe {
            let layout = Layout::new::<ccx_gxf>();
            let ptr = alloc(layout) as *mut ccx_gxf;

            // Create large CDP buffer
            let cdp_data: Vec<u8> = (0..1000).map(|i| (i % 256) as u8).collect();
            let cdp_len = cdp_data.len();
            let cdp_ptr = cdp_data.as_ptr();

            ptr::write_bytes(ptr, 0, 1);
            (*ptr).nb_streams = 0;
            (*ptr).cdp = cdp_ptr as *mut c_uchar;
            (*ptr).cdp_len = cdp_len;
            (*ptr).ad_track = ptr::null_mut();
            (*ptr).vid_track = ptr::null_mut();

            let rust_gxf = copy_ccx_gxf_from_c_to_rust(ptr).unwrap();

            let rust_cdp = rust_gxf.cdp.unwrap();
            assert_eq!(rust_cdp.len(), 1000);

            // Verify the pattern
            for (i, &byte) in rust_cdp.iter().enumerate() {
                assert_eq!(byte, (i % 256) as u8);
            }

            dealloc(ptr as *mut u8, layout);
        }
    }

    #[test]
    fn test_edge_case_field_values() {
        unsafe {
            let layout = Layout::new::<ccx_gxf>();
            let ptr = alloc(layout) as *mut ccx_gxf;

            ptr::write_bytes(ptr, 0, 1);
            (*ptr).nb_streams = u32::MAX as c_int;
            (*ptr).first_field_nb = u32::MAX as i32;
            (*ptr).last_field_nb = 0;
            (*ptr).mark_in = u32::MAX as i32;
            (*ptr).mark_out = 0;
            (*ptr).stream_size = u32::MAX as i32;
            (*ptr).ad_track = ptr::null_mut();
            (*ptr).vid_track = ptr::null_mut();
            (*ptr).cdp = ptr::null_mut();
            (*ptr).cdp_len = 0;

            let rust_gxf = copy_ccx_gxf_from_c_to_rust(ptr).unwrap();

            assert_eq!(rust_gxf.nb_streams, u32::MAX as i32);
            assert_eq!(rust_gxf.first_field_nb, u32::MAX as i32);
            assert_eq!(rust_gxf.last_field_nb, 0);
            assert_eq!(rust_gxf.mark_in, u32::MAX as i32);
            assert_eq!(rust_gxf.mark_out, 0);
            assert_eq!(rust_gxf.stream_size, u32::MAX as i32);

            dealloc(ptr as *mut u8, layout);
        }
    }
}
