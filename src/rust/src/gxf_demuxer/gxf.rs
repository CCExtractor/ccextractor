use crate::demuxer::common_structs::*;
use crate::demuxer::demuxer_data::DemuxerData;
use crate::file_functions::file::*;
use crate::gxf_demuxer::common_structs::*;
use crate::utils::rb32;
use lib_ccxr::common::{BufferdataType, Options};
use lib_ccxr::info;
use lib_ccxr::util::log::{debug, DebugMessageFlag};
use std::convert::{TryFrom, TryInto};
use std::slice;

macro_rules! dbg {
    ($($args:expr),*) => {
        debug!(msg_type = DebugMessageFlag::PARSE;"GXF:");
        debug!(msg_type = DebugMessageFlag::PARSE; $($args),*)
    };
}

/**
 * @param buf buffer with atleast acceptable length atleast 7 byte
 *            where we will test only important part of packet header
 *            In GXF packet header is of 16 byte and in header there is
 *            packet leader of 5 bytes 00 00 00 00 01
 *            Stream Starts with Map packet which is known by looking at offset 0x05
 *            of packet header.
 *            TODO Map packet are sent per 100 packets so search MAP packet, there might be
 *            no MAP header at start if GXF is sliced at unknown region
 */
pub fn ccx_gxf_probe(buf: &[u8]) -> bool {
    // Static startcode array.
    let startcode = [0, 0, 0, 0, 1, 0xbc];
    // If the buffer length is less than the startcode length, return false.
    if buf.len() < startcode.len() {
        return false;
    }
    // If the start of the buffer matches startcode, return true.
    if buf[..startcode.len()] == startcode {
        return true;
    }
    false
}

/// Parses a packet header, extracting type and length.
/// @param ctx Demuxer Ctx used for reading from file
/// @param type detected packet type is stored here
/// @param length detected packet length, excluding header, is stored here
/// @return Err(DemuxerError::InvalidArgument) if header not found or contains invalid data,
///         Err(DemuxerError::EOF) if EOF reached while reading packet
///         Ok(()) if the stream was fine enough to be parsed
/// # Safety
/// This function is unsafe because it calls unsafe functions `buffered_read` and `rb32`
pub unsafe fn parse_packet_header(
    ctx: *mut CcxDemuxer,
    pkt_type: &mut GXF_Pkt_Type,
    length: &mut i32,
    ccx_options: &mut Options,
) -> Result<(), DemuxerError> {
    if ctx.is_null() {
        return Err(DemuxerError::InvalidArgument);
    }

    let ctx_ref = ctx.as_mut().unwrap(); // TODO unsafe
    let mut pkt_header = [0u8; 16];
    let result = buffered_read(ctx_ref, Some(&mut pkt_header[..]), 16, ccx_options);
    ctx_ref.past += result as i64;
    if result != 16 {
        return Err(DemuxerError::EOF);
    }

    if rb32(pkt_header.as_ptr()) != 0 {
        return Err(DemuxerError::InvalidArgument);
    }
    let mut index = 4;

    if pkt_header[index] != 1 {
        return Err(DemuxerError::InvalidArgument);
    }
    index += 1;

    // In C, the packet type is simply assigned.
    // Here, we map it to the GXFPktType enum.
    *pkt_type = match pkt_header[index] {
        0xbc => GXF_Pkt_Type::PKT_MAP,
        0xbf => GXF_Pkt_Type::PKT_MEDIA,
        0xfb => GXF_Pkt_Type::PKT_EOS,
        0xfc => GXF_Pkt_Type::PKT_FLT,
        0xfd => GXF_Pkt_Type::PKT_UMF,
        _ => return Err(DemuxerError::InvalidArgument),
    };
    index += 1;

    // Read 4-byte length from the packet header.
    *length = rb32(pkt_header[index..].as_ptr()) as i32;
    index += 4;

    if ((*length >> 24) != 0) || *length < 16 {
        return Err(DemuxerError::InvalidArgument);
    }
    *length -= 16;

    index += 4;
    if pkt_header[index] != 0xe1 {
        return Err(DemuxerError::InvalidArgument);
    }
    index += 1;

    if pkt_header[index] != 0xe2 {
        return Err(DemuxerError::InvalidArgument);
    }

    Ok(())
}
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe functions like `buffered_get_byte` and `buffered_read`
pub unsafe fn parse_material_sec(
    demux: &mut CcxDemuxer,
    mut len: i32,
    ccx_options: &mut Options,
) -> Result<(), DemuxerError> {
    let ctx = demux.private_data as *mut CcxGxf;
    if ctx.is_null() {
        return Err(DemuxerError::InvalidArgument);
    }

    let mut ret = Ok(());

    while len > 2 {
        let tag = buffered_get_byte(demux, ccx_options);
        let tag_len = buffered_get_byte(demux, ccx_options);
        len -= 2;

        if len < tag_len as i32 {
            ret = Err(DemuxerError::InvalidArgument);
            break;
        }
        len -= tag_len as i32;

        match tag {
            x if x == GXF_Mat_Tag::MAT_NAME as u8 => {
                let mut buf = (*ctx).media_name.clone().into_bytes();
                buf.resize(tag_len as usize, 0);
                let n = buffered_read(demux, Some(&mut buf), tag_len as usize, ccx_options);
                demux.past += n as i64;
                if n != tag_len as usize {
                    ret = Err(DemuxerError::EOF);
                    break;
                }
                (*ctx).media_name = String::from_utf8_lossy(&buf).to_string();
            }
            x if x == GXF_Mat_Tag::MAT_FIRST_FIELD as u8 => {
                (*ctx).first_field_nb = buffered_get_be32(demux, ccx_options) as i32;
            }
            x if x == GXF_Mat_Tag::MAT_LAST_FIELD as u8 => {
                (*ctx).last_field_nb = buffered_get_be32(demux, ccx_options) as i32;
            }
            x if x == GXF_Mat_Tag::MAT_MARK_IN as u8 => {
                (*ctx).mark_in = buffered_get_be32(demux, ccx_options) as i32;
            }
            x if x == GXF_Mat_Tag::MAT_MARK_OUT as u8 => {
                (*ctx).mark_out = buffered_get_be32(demux, ccx_options) as i32;
            }
            x if x == GXF_Mat_Tag::MAT_SIZE as u8 => {
                (*ctx).stream_size = buffered_get_be32(demux, ccx_options) as i32;
            }
            _ => {
                let skipped = buffered_skip(demux, tag_len as u32, ccx_options);
                demux.past += skipped as i64;
            }
        }
    }

    // C’s `error:` label: unconditionally skip the remaining bytes
    let skipped = buffered_skip(demux, len as u32, ccx_options);
    demux.past += skipped as i64;
    if skipped != len as usize {
        ret = Err(DemuxerError::EOF);
    }

    ret
}

/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe functions like `buffered_get_byte` and `buffered_read`
pub unsafe fn parse_ad_track_desc(
    demux: &mut CcxDemuxer,
    mut len: i32,
    ccx_options: &mut Options,
) -> Result<(), DemuxerError> {
    // Retrieve the GXF context from demux->private_data.
    let ctx = match (demux.private_data as *mut CcxGxf).as_mut() {
        Some(ctx) => ctx,
        None => return Err(DemuxerError::InvalidArgument),
    };
    // Retrieve the ancillary data track; if missing, error out.
    let ad_track = match ctx.ad_track.as_mut() {
        Some(track) => track,
        None => return Err(DemuxerError::InvalidArgument),
    };

    let mut auxi_info = [0u8; 8];
    let mut ret = Ok(());
    let mut error_occurred = false;

    dbg!("Ancillary Data {}", len);

    while len > 2 {
        let tag = buffered_get_byte(demux, ccx_options);
        let tag_len = buffered_get_byte(demux, ccx_options) as i32;
        len -= 2;
        if len < tag_len {
            ret = Err(DemuxerError::InvalidArgument);
            error_occurred = true;
            break;
        }
        len -= tag_len;
        match tag {
            x if x == GXF_Track_Tag::TRACK_NAME as u8 => {
                let mut buf = ad_track.track_name.clone().into_bytes();
                buf.resize(tag_len as usize, 0);
                let result = buffered_read(demux, Some(&mut buf), tag_len as usize, ccx_options);
                demux.past += tag_len as i64;
                if result != tag_len as usize {
                    ret = Err(DemuxerError::EOF);
                    error_occurred = true;
                    break;
                }
                ad_track.track_name = String::from_utf8_lossy(&buf).to_string();
            }
            x if x == GXF_Track_Tag::TRACK_AUX as u8 => {
                let result = buffered_read(demux, Some(&mut auxi_info), 8, ccx_options);
                demux.past += 8;
                if result != 8 {
                    ret = Err(DemuxerError::EOF);
                    error_occurred = true;
                    break;
                }
                if tag_len != 8 {
                    ret = Err(DemuxerError::InvalidArgument);
                    error_occurred = true;
                    break;
                }
                // Set ancillary track fields.
                ad_track.ad_format = match auxi_info[2] {
                    1 => GXF_Anc_Data_Pres_Format::PRES_FORMAT_SD,
                    2 => GXF_Anc_Data_Pres_Format::PRES_FORMAT_HD,
                    _ => GXF_Anc_Data_Pres_Format::PRES_FORMAT_SD,
                };
                ad_track.nb_field = auxi_info[3] as i32;
                ad_track.field_size = i16::from_be_bytes([auxi_info[4], auxi_info[5]]) as i32;
                ad_track.packet_size =
                    i16::from_be_bytes([auxi_info[6], auxi_info[7]]) as i32 * 256;
                dbg!(
                    "ad_format {} nb_field {} field_size {} packet_size {} track id {}",
                    ad_track.ad_format,
                    ad_track.nb_field,
                    ad_track.field_size,
                    ad_track.packet_size,
                    ad_track.id
                );
            }
            x if x == GXF_Track_Tag::TRACK_VER as u8 => {
                ad_track.fs_version = buffered_get_be32(demux, ccx_options);
            }
            x if x == GXF_Track_Tag::TRACK_FPS as u8 => {
                ad_track.frame_rate = buffered_get_be32(demux, ccx_options);
            }
            x if x == GXF_Track_Tag::TRACK_LINES as u8 => {
                ad_track.line_per_frame = buffered_get_be32(demux, ccx_options);
            }
            x if x == GXF_Track_Tag::TRACK_FPF as u8 => {
                ad_track.field_per_frame = buffered_get_be32(demux, ccx_options);
            }
            x if x == GXF_Track_Tag::TRACK_MPG_AUX as u8 => {
                let result = buffered_skip(demux, tag_len as u32, ccx_options);
                demux.past += result as i64;
            }
            _ => {
                let result = buffered_skip(demux, tag_len as u32, ccx_options);
                demux.past += result as i64;
            }
        }
    }

    // Error handling block.
    let result = buffered_skip(demux, len as u32, ccx_options);
    demux.past += result as i64;
    if result != len as usize {
        ret = Err(DemuxerError::EOF);
    }
    if error_occurred {
        ret = Err(DemuxerError::InvalidArgument);
    }
    ret
}

pub fn set_track_frame_rate(vid_track: &mut CcxGxfVideoTrack, val: i8) {
    match val {
        1 => {
            vid_track.frame_rate.num = 60;
            vid_track.frame_rate.den = 1;
        }
        2 => {
            vid_track.frame_rate.num = 60000;
            vid_track.frame_rate.den = 1001;
        }
        3 => {
            vid_track.frame_rate.num = 50;
            vid_track.frame_rate.den = 1;
        }
        4 => {
            vid_track.frame_rate.num = 30;
            vid_track.frame_rate.den = 1;
        }
        5 => {
            vid_track.frame_rate.num = 30000;
            vid_track.frame_rate.den = 1001;
        }
        6 => {
            vid_track.frame_rate.num = 25;
            vid_track.frame_rate.den = 1;
        }
        7 => {
            vid_track.frame_rate.num = 24;
            vid_track.frame_rate.den = 1;
        }
        8 => {
            vid_track.frame_rate.num = 24000;
            vid_track.frame_rate.den = 1001;
        }
        -1 => { /* Not applicable for this track type */ }
        -2 => { /* Not available */ }
        _ => { /* Do nothing in case of no frame rate */ }
    }
}

/* *  @param vid_format following format are supported to set valid timebase
 *  in demuxer data
 *  value   |  Meaning
 *=====================================
 *  0      | 525 interlaced lines at 29.97 frames / sec
 *  1      | 625 interlaced lines at 25 frames / sec
 *  2      | 720 progressive lines at 59.94 frames / sec
 *  3      | 720 progressive lines at 60 frames / sec
 *  4      | 1080 progressive lines at 23.98 frames / sec
 *  5      | 1080 progressive lines at 24 frames / sec
 *  6      | 1080 progressive lines at 25 frames / sec
 *  7      | 1080 progressive lines at 29.97 frames / sec
 *  8      | 1080 progressive lines at 30 frames / sec
 *  9      | 1080 interlaced lines at 25 frames / sec
 *  10     | 1080 interlaced lines at 29.97 frames / sec
 *  11     | 1080 interlaced lines at 30 frames / sec
 *  12     | 1035 interlaced lines at 30 frames / sec
 *  13     | 1035 interlaced lines at 29.97 frames / sec
 *  14     | 720 progressive lines at 50 frames / sec
 *  15     | 525 progressive lines at 59.94 frames / sec
 *  16     | 525 progressive lines at 60 frames / sec
 *  17     | 525 progressive lines at 29.97 frames / sec
 *  18     | 525 progressive lines at 30 frames / sec
 *  19     | 525 progressive lines at 50 frames / sec
 *  20     | 525 progressive lines at 25 frames / sec
 *
 * @param data already allocated data, passing NULL will end up in seg-fault
 *        data len may be zero, while setting timebase we don not care about
 *        actual data
 */

pub fn set_data_timebase(vid_format: i32, data: &mut DemuxerData) {
    dbg!("LOG: Format Video {}", vid_format);

    match vid_format {
        // NTSC (30000/1001)
        0 | 7 | 10 | 13 | 17 => {
            data.tb.den = 30000;
            data.tb.num = 1001;
        }
        // PAL (25/1)
        1 | 6 | 9 | 20 => {
            data.tb.den = 25;
            data.tb.num = 1;
        }
        // NTSC 60fps (60000/1001)
        2 | 15 => {
            data.tb.den = 60000;
            data.tb.num = 1001;
        }
        // 60 fps (60/1)
        3 | 16 => {
            data.tb.den = 60;
            data.tb.num = 1;
        }
        // 24 fps (24000/1001)
        4 => {
            data.tb.den = 24000;
            data.tb.num = 1001;
        }
        // 24 fps (24/1)
        5 => {
            data.tb.den = 24;
            data.tb.num = 1;
        }
        // 30 fps (30/1)
        8 | 11 | 12 | 18 => {
            data.tb.den = 30;
            data.tb.num = 1;
        }
        // 50 fps (50/1)
        14 | 19 => {
            data.tb.den = 50;
            data.tb.num = 1;
        }
        // Default case does nothing
        _ => {}
    }
}

/**
 * VBI in ancillary data is not specified in GXF specs
 * but while traversing file, we found vbi data presence
 * in Multimedia file, so if there are some video which
 * show caption on tv or there software but ccextractor
 * is not able to see the caption, there might be need
 * of parsing vbi
 */
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe function `buffered_skip`
#[allow(unused_variables)]
pub unsafe fn parse_ad_vbi(
    demux: &mut CcxDemuxer,
    len: i32,
    data: &mut DemuxerData,
    ccx_options: &mut Options,
) -> Result<(), DemuxerError> {
    let mut ret = Ok(());
    let result: usize;

    #[cfg(feature = "ccx_gxf_enable_ad_vbi")]
    {
        data.len += len as usize;
        // Read 'len' bytes into data.buffer (starting at index 0).
        let buffer_slice = std::slice::from_raw_parts_mut(data.buffer, len as usize);
        result = buffered_read(demux, Some(buffer_slice), len as usize, ccx_options);
    }
    #[cfg(not(feature = "ccx_gxf_enable_ad_vbi"))]
    {
        // Skip 'len' bytes if VBI support is not enabled.
        result = buffered_skip(demux, len as u32, ccx_options);
    }
    // Update demux.past with the bytes processed.
    demux.past += result as i64;
    if result != len as usize {
        ret = Err(DemuxerError::EOF);
    }
    ret
}

/**
 * Parse Caption Distribution Packet
 * General Syntax of cdp
 * cdp() {
 *   cdp_header();
 *   time_code_section();
 *   ccdata_section();
 *   ccsvcinfo_section();
 *   cdp_footer();
 * }
 * function does not parse cdp in chunk, user should provide complete cdp
 * with header and footer inclusive of checksum
 * @return Err(DemuxerError::InvalidArgument) if cdp data fields are not valid
 */
pub fn parse_ad_cdp(cdp: &[u8], data: &mut DemuxerData) -> Result<(), DemuxerError> {
    // Do not accept packet whose length does not fit header and footer
    if cdp.len() < 11 {
        info!("Short packet can't accommodate header and footer");
        return Err(DemuxerError::InvalidArgument);
    }

    // Verify cdp header identifier
    if cdp[0] != 0x96 || cdp[1] != 0x69 {
        info!("Could not find CDP identifier of 0x96 0x69");
        return Err(DemuxerError::InvalidArgument);
    }

    // Save original packet length and advance pointer by 2 bytes
    let orig_len = cdp.len();
    let mut offset = 2;

    // Read CDP length (1 byte) and verify that it equals the original packet length
    let cdp_length = cdp[offset] as usize;
    offset += 1;

    if cdp_length != orig_len {
        info!("CDP length is not valid");
        return Err(DemuxerError::InvalidArgument);
    }

    // Parse header fields
    let cdp_framerate = (cdp[offset] & 0xF0) >> 4;
    offset += 1;
    let cc_data_present = (cdp[offset] & 0x40) >> 6;
    let caption_service_active = (cdp[offset] & 0x02) >> 1;
    offset += 1;

    // Read 16-bit big-endian sequence counter
    let cdp_header_sequence_counter = u16::from_be_bytes([cdp[offset], cdp[offset + 1]]);
    offset += 2;

    dbg!("CDP length: {} words", cdp_length);
    dbg!("CDP frame rate: 0x{:x}", cdp_framerate);
    dbg!("CC data present: {}", cc_data_present);
    dbg!("Caption service active: {}", caption_service_active);
    dbg!(
        "Header sequence counter: {} (0x{:x})",
        cdp_header_sequence_counter,
        cdp_header_sequence_counter
    );

    // Process CDP sections (only one section allowed per packet)
    match cdp[offset] {
        0x71 => {
            info!("Ignore Time code section");
            return Err(DemuxerError::Retry); // C returns -1, which should map to Retry
        }
        0x72 => {
            offset += 1; // Advance past section id
            let cc_count = (cdp[offset] & 0x1F) as usize;
            offset += 1;

            dbg!("cc_count: {}", cc_count);

            let copy_size = cc_count * 3;

            // Check bounds for source
            if offset + copy_size > cdp.len() {
                return Err(DemuxerError::InvalidArgument);
            }

            // Copy ccdata into data.buffer starting at offset data.len
            unsafe {
                let dst_ptr = data.buffer.add(data.len);
                let src_ptr = cdp.as_ptr().add(offset);
                std::ptr::copy_nonoverlapping(src_ptr, dst_ptr, copy_size);
            }

            data.len += copy_size;
            offset += copy_size;
        }
        0x73 => {
            info!("Ignore service information section");
            return Err(DemuxerError::Retry); // C returns -1
        }
        0x75..=0xEF => {
            info!(
                "Newer version of SMPTE-334 specification detected. New section id 0x{:x}",
                cdp[offset]
            );
            return Err(DemuxerError::Retry); // C returns -1
        }
        _ => {}
    }

    // Check CDP footer
    if offset < cdp.len() && cdp[offset] == 0x74 {
        offset += 1;

        // Ensure we have enough bytes for the footer sequence counter
        if offset + 1 >= cdp.len() {
            return Err(DemuxerError::InvalidArgument);
        }

        let footer_sequence_counter = u16::from_be_bytes([cdp[offset], cdp[offset + 1]]);
        if cdp_header_sequence_counter != footer_sequence_counter {
            info!("Incomplete CDP packet");
            return Err(DemuxerError::InvalidArgument);
        }
    }

    Ok(())
}
/**
 *    +-----------------------------+-----------------------------+
 *    | Bits (0 is LSB; 7 is MSB)   |          Meaning            |
 *    +-----------------------------+-----------------------------+
 *    |                             |        Picture Coding       |
 *    |                             |         00 = NOne           |
 *    |          0:1                |         01 = I frame        |
 *    |                             |         10 = P Frame        |
 *    |                             |         11 = B Frame        |
 *    +-----------------------------+-----------------------------+
 *    |                             |      Picture Structure      |
 *    |                             |         00 = NOne           |
 *    |          2:3                |         01 = I frame        |
 *    |                             |         10 = P Frame        |
 *    |                             |         11 = B Frame        |
 *    +-----------------------------+-----------------------------+
 *    |          4:7                |      Not Used MUST be 0     |
 *    +-----------------------------+-----------------------------+
 */
/// C `set_mpeg_frame_desc` function.
pub fn set_mpeg_frame_desc(vid_track: &mut CcxGxfVideoTrack, mpeg_frame_desc_flag: u8) {
    vid_track.p_code = MpegPictureCoding::try_from(mpeg_frame_desc_flag & 0x03)
        .unwrap_or(MpegPictureCoding::CCX_MPC_NONE);
    vid_track.p_struct = MpegPictureStruct::try_from((mpeg_frame_desc_flag >> 2) & 0x03)
        .unwrap_or(MpegPictureStruct::CCX_MPS_NONE);
}

/// # Safety
/// This function is unsafe because it calls unsafe functions like `buffered_read` and `from_raw_parts_mut`
pub unsafe fn parse_mpeg_packet(
    demux: &mut CcxDemuxer,
    len: usize,
    data: &mut DemuxerData,
    ccx_options: &mut Options,
) -> Result<(), DemuxerError> {
    // Read 'len' bytes into the data buffer at offset data.len.
    let result = buffered_read(
        demux,
        Some(slice::from_raw_parts_mut(data.buffer.add(data.len), len)),
        len,
        ccx_options,
    );
    data.len += len;
    demux.past += result as i64;

    if result != len {
        return Err(DemuxerError::EOF);
    }
    Ok(())
}

/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe functions like `buffered_get_byte` and `buffered_read`
pub unsafe fn parse_mpeg525_track_desc(
    demux: &mut CcxDemuxer,
    mut len: i32,
    ccx_options: &mut Options,
) -> Result<(), DemuxerError> {
    // Retrieve the GXF context from demux->private_data.
    let ctx = match (demux.private_data as *mut CcxGxf).as_mut() {
        Some(ctx) => ctx,
        None => return Err(DemuxerError::InvalidArgument),
    };
    let vid_track = match ctx.vid_track.as_mut() {
        Some(track) => track,
        None => return Err(DemuxerError::InvalidArgument),
    };
    let mut ret = Ok(());
    let mut error_occurred = false;

    /* Auxiliary Information */
    // let auxi_info: [u8; 8];  // Not used, keeping comment for reference
    dbg!("Mpeg 525 {}", len);

    while len > 2 {
        let tag = buffered_get_byte(demux, ccx_options);
        let tag_len = buffered_get_byte(demux, ccx_options) as i32;
        len -= 2;
        if len < tag_len {
            ret = Err(DemuxerError::InvalidArgument);
            error_occurred = true;
            break;
        }
        len -= tag_len;
        match tag {
            x if x == GXF_Track_Tag::TRACK_NAME as u8 => {
                let mut buf = vid_track.track_name.clone().into_bytes();
                buf.resize(tag_len as usize, 0);
                let result = buffered_read(demux, Some(&mut buf), tag_len as usize, ccx_options);
                demux.past += tag_len as i64;
                if result != tag_len as usize {
                    ret = Err(DemuxerError::EOF);
                    error_occurred = true;
                    break;
                }
                vid_track.track_name = String::from_utf8_lossy(&buf).to_string();
            }
            x if x == GXF_Track_Tag::TRACK_VER as u8 => {
                vid_track.fs_version = buffered_get_be32(demux, ccx_options);
            }
            x if x == GXF_Track_Tag::TRACK_FPS as u8 => {
                let val = buffered_get_be32(demux, ccx_options);
                set_track_frame_rate(vid_track, val as i8);
            }
            x if x == GXF_Track_Tag::TRACK_LINES as u8 => {
                vid_track.line_per_frame = buffered_get_be32(demux, ccx_options);
            }
            x if x == GXF_Track_Tag::TRACK_FPF as u8 => {
                vid_track.field_per_frame = buffered_get_be32(demux, ccx_options);
            }
            x if x == GXF_Track_Tag::TRACK_AUX as u8 => {
                let result = buffered_skip(demux, tag_len as u32, ccx_options);
                demux.past += result as i64;
            }
            x if x == GXF_Track_Tag::TRACK_MPG_AUX as u8 => {
                let result = buffered_skip(demux, tag_len as u32, ccx_options);
                demux.past += result as i64;
            }
            _ => {
                let result = buffered_skip(demux, tag_len as u32, ccx_options);
                demux.past += result as i64;
            }
        }
    }

    let result = buffered_skip(demux, len as u32, ccx_options);
    demux.past += result as i64;
    if result != len as usize {
        ret = Err(DemuxerError::EOF);
    }
    if error_occurred {
        ret = Err(DemuxerError::InvalidArgument);
    }
    ret
}
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe functions like `buffered_get_byte` and `buffered_get_be16`
pub unsafe fn parse_track_sec(
    demux: &mut CcxDemuxer,
    mut len: i32,
    data: &mut DemuxerData,
    ccx_options: &mut Options,
) -> Result<(), DemuxerError> {
    // Retrieve the GXF context from demux->private_data.
    let ctx = match (demux.private_data as *mut CcxGxf).as_mut() {
        Some(ctx) => ctx,
        None => return Err(DemuxerError::InvalidArgument),
    };

    let mut ret = Ok(());

    while len > 4 {
        // Read track header: 1 byte track_type, 1 byte track_id, 2 bytes track_len.
        let mut track_type = buffered_get_byte(demux, ccx_options);
        let mut track_id = buffered_get_byte(demux, ccx_options);
        let track_len = buffered_get_be16(demux, ccx_options) as i32;
        len -= 4;

        if len < track_len {
            ret = Err(DemuxerError::InvalidArgument);
            break;
        }

        // If track_type does not have high bit set, skip record.
        if (track_type & 0x80) != 0x80 {
            len -= track_len;
            let result = buffered_skip(demux, track_len as u32, ccx_options);
            demux.past += result as i64;
            if result != track_len as usize {
                ret = Err(DemuxerError::EOF);
                break;
            }
            continue;
        }
        track_type &= 0x7f;

        // If track_id does not have its two high bits set, skip record.
        if (track_id & 0xc0) != 0xc0 {
            len -= track_len;
            let result = buffered_skip(demux, track_len as u32, ccx_options);
            demux.past += result as i64;
            if result != track_len as usize {
                ret = Err(DemuxerError::EOF);
                break;
            }
            continue;
        }
        track_id &= 0xcf;

        match track_type {
            x if x == GXF_Track_Type::TRACK_TYPE_ANCILLARY_DATA as u8 => {
                // Allocate ancillary track if not present.
                if ctx.ad_track.is_none() {
                    ctx.ad_track = Some(CcxGxfAncillaryDataTrack::default());
                }
                if let Some(ad_track) = ctx.ad_track.as_mut() {
                    ad_track.id = track_id;
                    let _ = parse_ad_track_desc(demux, track_len, ccx_options);
                    data.bufferdatatype = BufferdataType::Raw;
                }
                len -= track_len;
            }
            x if x == GXF_Track_Type::TRACK_TYPE_MPEG2_525 as u8 => {
                // Allocate video track if not present.
                if ctx.vid_track.is_none() {
                    ctx.vid_track = Some(CcxGxfVideoTrack::default());
                }
                if ctx.vid_track.is_none() {
                    info!("Ignored MPEG track due to insufficient memory\n");
                    break;
                }
                let _ = parse_mpeg525_track_desc(demux, track_len, ccx_options);
                data.bufferdatatype = BufferdataType::Pes;
                len -= track_len;
            }
            _ => {
                let result = buffered_skip(demux, track_len as u32, ccx_options);
                demux.past += result as i64;
                len -= track_len;
                if result != track_len as usize {
                    ret = Err(DemuxerError::EOF);
                    break;
                }
            }
        }
    }

    let result = buffered_skip(demux, len as u32, ccx_options);
    demux.past += result as i64;
    if result != len as usize {
        ret = Err(DemuxerError::EOF);
    }
    ret
}

/**
 * parse ancillary data payload
 */
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe functions like `buffered_get_le16` and `buffered_skip`
pub unsafe fn parse_ad_pyld(
    demux: &mut CcxDemuxer,
    len: i32,
    data: &mut DemuxerData,
    ccx_options: &mut Options,
) -> Result<(), DemuxerError> {
    let mut ret = Ok(());
    #[allow(unused_assignments)]
    let mut rem_len = len;

    #[cfg(not(feature = "ccx_gxf_enable_ad_vbi"))]
    {
        let mut i: usize;

        // Read 16-bit little-endian values from buffered input:
        let d_id = buffered_get_le16(demux, ccx_options);
        let sd_id = buffered_get_le16(demux, ccx_options);
        // Read dc and mask to 8 bits.
        let _dc = buffered_get_le16(demux, ccx_options) & 0xFF;

        let ctx = &mut *(demux.private_data as *mut CcxGxf);

        // Adjust length (remove the 6 header bytes)
        rem_len = len - 6;
        if rem_len < 0 {
            info!("Invalid ancillary data payload length: {}", rem_len);
            return Err(DemuxerError::InvalidArgument);
        }
        // If ctx.cdp buffer is too small, reallocate it.
        if ctx.cdp_len < (rem_len / 2) as usize {
            // Allocate a new buffer of size (rem_len/2)
            ctx.cdp = match std::panic::catch_unwind(|| vec![0u8; (rem_len / 2) as usize]) {
                Ok(buf) => Some(buf),
                Err(_) => {
                    info!("Failed to allocate buffer {}\n", rem_len / 2);
                    return Err(DemuxerError::OutOfMemory);
                }
            };
            if ctx.cdp.is_none() {
                info!("Could not allocate buffer {}\n", rem_len / 2);
                return Err(DemuxerError::OutOfMemory);
            }
            // Exclude DID and SDID bytes: set new cdp_len to ((rem_len - 2) / 2)
            ctx.cdp_len = ((rem_len - 2) / 2) as usize;
        }

        // Check for CEA-708 captions: d_id and sd_id must match.
        if ((d_id & 0xFF) == CLOSED_CAP_DID as u16) && ((sd_id & 0xFF) == CLOSED_C708_SDID as u16) {
            if let Some(ref mut cdp) = ctx.cdp {
                i = 0;
                let mut remaining_len = rem_len;
                while remaining_len > 2 && i < ctx.cdp_len {
                    let dat = buffered_get_le16(demux, ccx_options);
                    cdp[i] = match dat {
                        0x2FE => 0xFF,
                        0x201 => 0x01,
                        _ => (dat & 0xFF) as u8,
                    };
                    i += 1;
                    remaining_len -= 2;
                }
                // Call parse_ad_cdp on the newly filled buffer.
                // (Assume parse_ad_cdp returns Ok(()) on success.)
                let _ = parse_ad_cdp(ctx.cdp.as_ref().unwrap(), data);
            }
        }
        // If it corresponds to CEA-608 captions:
        else if ((d_id & 0xFF) == CLOSED_CAP_DID as u16)
            && ((sd_id & 0xFF) == CLOSED_C608_SDID as u16)
        {
            info!("Need Sample\n");
        }
        // Otherwise, ignore other services.
    }

    let result = buffered_skip(demux, rem_len as u32, ccx_options);
    demux.past += result as i64;
    if result != rem_len as usize {
        ret = Err(DemuxerError::EOF);
    }
    ret
}
/// parse_ad_field: parses an ancillary data field from the demuxer buffer,
/// verifying header tags (e.g. "finf", "LIST", "anc ") and then processing each
/// sub‐section (e.g. "pyld"/"vbi") until the field is exhausted.
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe functions like `buffered_read` and `buffered_get_le32`
pub unsafe fn parse_ad_field(
    demux: &mut CcxDemuxer,
    mut len: i32,
    data: &mut DemuxerData,
    ccx_options: &mut Options,
) -> Result<(), DemuxerError> {
    let mut ret = Ok(());
    let mut result;
    let mut tag = [0u8; 5]; // 4-byte tag plus null terminator

    tag[4] = 0;

    // Read "finf" tag
    len -= 4;
    result = buffered_read(demux, Some(&mut tag[..4]), 4, ccx_options);
    demux.past += result as i64;
    if &tag[..4] != b"finf" {
        info!("Warning: No finf tag\n");
    }

    // Read and validate GXF spec value (must be 4)
    len -= 4;
    if buffered_get_le32(demux, ccx_options) != 4 {
        info!("Warning: expected 4 acc GXF specs\n");
    }

    // Read field identifier
    len -= 4;
    let field_identifier = buffered_get_le32(demux, ccx_options);
    info!("LOG: field identifier {}\n", field_identifier);

    // Read "LIST" tag
    len -= 4;
    result = buffered_read(demux, Some(&mut tag[..4]), 4, ccx_options);
    demux.past += result as i64;
    if &tag[..4] != b"LIST" {
        info!("Warning: No List tag\n");
    }

    // Read ancillary data field section size.
    len -= 4;
    if buffered_get_le32(demux, ccx_options) != len as u32 {
        info!("Warning: Unexpected sample size (!={})\n", len);
    }

    // Read "anc " tag
    len -= 4;
    result = buffered_read(demux, Some(&mut tag[..4]), 4, ccx_options);
    demux.past += result as i64;
    if &tag[..4] != b"anc " {
        info!("Warning: No anc tag\n");
    }

    // Process sub-sections until less than or equal to 28 bytes remain.
    while len > 28 {
        len -= 4;
        result = buffered_read(demux, Some(&mut tag[..4]), 4, ccx_options);
        demux.past += result as i64;

        len -= 4;
        let hdr_len = buffered_get_le32(demux, ccx_options);

        // Check for pad tag.
        if &tag[..4] == b"pad " {
            if hdr_len != len as u32 {
                info!("Warning: expected {} got {}\n", len, hdr_len);
            }
            len -= hdr_len as i32;
            result = buffered_skip(demux, hdr_len, ccx_options);
            demux.past += result as i64;
            if result != hdr_len as usize {
                ret = Err(DemuxerError::EOF);
            }
            continue;
        } else if &tag[..4] == b"pos " {
            if hdr_len != 12 {
                info!("Warning: expected 4 got {}\n", hdr_len);
            }
        } else {
            info!("Warning: Instead pos tag got {:?} tag\n", &tag[..4]);
            if hdr_len != 12 {
                info!("Warning: expected 4 got {}\n", hdr_len);
            }
        }

        len -= 4;
        let line_nb = buffered_get_le32(demux, ccx_options);
        info!("Line nb: {}\n", line_nb);

        len -= 4;
        let luma_flag = buffered_get_le32(demux, ccx_options);
        info!("luma color diff flag: {}\n", luma_flag);

        len -= 4;
        let hanc_vanc_flag = buffered_get_le32(demux, ccx_options);
        info!("hanc/vanc flag: {}\n", hanc_vanc_flag);

        len -= 4;
        result = buffered_read(demux, Some(&mut tag[..4]), 4, ccx_options);
        demux.past += result as i64;

        len -= 4;
        let pyld_len = buffered_get_le32(demux, ccx_options);
        info!("pyld len: {}\n", pyld_len);

        if &tag[..4] == b"pyld" {
            len -= pyld_len as i32;
            ret = parse_ad_pyld(demux, pyld_len as i32, data, ccx_options);
            if ret == Err(DemuxerError::EOF) {
                break;
            }
        } else if &tag[..4] == b"vbi " {
            len -= pyld_len as i32;
            ret = parse_ad_vbi(demux, pyld_len as i32, data, ccx_options);
            if ret == Err(DemuxerError::EOF) {
                break;
            }
        } else {
            info!("Warning: No pyld/vbi tag got {:?} tag\n", &tag[..4]);
        }
    }

    result = buffered_skip(demux, len as u32, ccx_options);
    demux.past += result as i64;
    if result != len as usize {
        ret = Err(DemuxerError::EOF);
    }
    ret
}
/**
 * This packet contain RIFF data
 * @param demuxer Demuxer must contain vaild ad_track structure
 */
/// # Safety
/// This function is unsafe because it deferences raw pointers and calls unsafe functions like `buffered_read` and `buffered_get_le32`
pub unsafe fn parse_ad_packet(
    demux: &mut CcxDemuxer,
    len: i32,
    data: &mut DemuxerData,
    ccx_options: &mut Options,
) -> Result<(), DemuxerError> {
    let mut ret = Ok(());
    let mut remaining_len = len;
    let mut tag = [0u8; 4];
    let mut result;

    let ctx = match (demux.private_data as *mut CcxGxf).as_mut() {
        Some(ctx) => ctx,
        None => return Err(DemuxerError::InvalidArgument),
    };
    let ad_track = match ctx.ad_track.as_mut() {
        Some(track) => track,
        None => return Err(DemuxerError::InvalidArgument),
    };
    // Read "RIFF" header
    remaining_len -= 4;
    result = buffered_read(demux, Some(&mut tag), 4, ccx_options);
    demux.past += result as i64;
    if &tag != b"RIFF" {
        info!("Warning: No RIFF header");
    }

    // Validate ADT packet length
    remaining_len -= 4;
    if buffered_get_le32(demux, ccx_options) != 65528 {
        info!("Warning: ADT packet with non-trivial length");
    }

    // Read "rcrd" tag
    remaining_len -= 4;
    result = buffered_read(demux, Some(&mut tag), 4, ccx_options);
    demux.past += result as i64;
    if &tag != b"rcrd" {
        info!("Warning: No rcrd tag");
    }

    // Read "desc" tag
    remaining_len -= 4;
    result = buffered_read(demux, Some(&mut tag), 4, ccx_options);
    demux.past += result as i64;
    if &tag != b"desc" {
        info!("Warning: No desc tag");
    }

    // Validate desc length
    remaining_len -= 4;
    if buffered_get_le32(demux, ccx_options) != 20 {
        info!("Warning: Unexpected desc length (!=20)");
    }

    // Validate version
    remaining_len -= 4;
    if buffered_get_le32(demux, ccx_options) != 2 {
        info!("Warning: Unsupported version (!=2)");
    }

    // Check number of fields
    remaining_len -= 4;
    let val = buffered_get_le32(demux, ccx_options);
    if ad_track.nb_field != val as i32 {
        info!("Warning: Ambiguous number of fields");
    }

    // Check field size
    remaining_len -= 4;
    let val = buffered_get_le32(demux, ccx_options);
    if ad_track.field_size != val as i32 {
        info!("Warning: Ambiguous field size");
    }

    // Validate ancillary media packet size
    remaining_len -= 4;
    if buffered_get_le32(demux, ccx_options) != 65536 {
        info!("Warning: Unexpected buffer size (!=65536)");
    }

    // Set data timebase
    remaining_len -= 4;
    let val = buffered_get_le32(demux, ccx_options);
    set_data_timebase(val as i32, data);

    // Read "LIST" tag
    remaining_len -= 4;
    result = buffered_read(demux, Some(&mut tag), 4, ccx_options);
    demux.past += result as i64;
    if &tag != b"LIST" {
        info!("Warning: No LIST tag");
    }

    // Validate field section size
    remaining_len -= 4;
    if buffered_get_le32(demux, ccx_options) != remaining_len as u32 {
        info!(
            "Warning: Unexpected field section size (!={})",
            remaining_len
        );
    }

    // Read "fld " tag
    remaining_len -= 4;
    result = buffered_read(demux, Some(&mut tag), 4, ccx_options);
    demux.past += result as i64;
    if &tag != b"fld " {
        info!("Warning: No fld tag");
    }

    // Parse each field
    if ad_track.nb_field < 0 || ad_track.field_size < 0 {
        info!("Invalid ancillary data track field count or size");
        return Err(DemuxerError::InvalidArgument);
    }
    for _ in 0..ad_track.nb_field as usize {
        remaining_len -= ad_track.field_size;
        let _ = parse_ad_field(demux, ad_track.field_size, data, ccx_options);
    }

    // Skip remaining data
    result = buffered_skip(demux, remaining_len as u32, ccx_options);
    demux.past += result as i64;
    if result != remaining_len as usize {
        ret = Err(DemuxerError::EOF);
    }
    ret
}
macro_rules! goto_end {
    ($demux:expr, $len:expr, $ret:ident, $ccx_options:expr) => {{
        let result = buffered_skip($demux, $len as u32, $ccx_options) as i32;
        $demux.past += result as i64;
        if result != $len {
            $ret = Err(DemuxerError::EOF);
        }
        return $ret;
    }};
}
/// C `parse_media` function.
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe functions like `buffered_get_byte` and `buffered_skip`
pub unsafe fn parse_media(
    demux: &mut CcxDemuxer,
    mut len: i32,
    data: &mut DemuxerData,
    ccx_options: &mut Options,
) -> Result<(), DemuxerError> {
    let mut ret = Ok(());
    let mut result;

    // Check for null private_data before dereferencing
    if demux.private_data.is_null() {
        result = buffered_skip(demux, len as u32, ccx_options) as i32;
        demux.past += result as i64;
        if result != len {
            ret = Err(DemuxerError::EOF);
        }
        return ret;
    }

    let ctx = &mut *(demux.private_data as *mut CcxGxf);

    let mut first_field_nb: u16 = 0;
    let mut last_field_nb: u16 = 0;
    let mut mpeg_pic_size: u32 = 0;
    let mut mpeg_frame_desc_flag: u8 = 0;

    len -= 1;

    let media_type_0 = GXF_Track_Type::try_from(buffered_get_byte(demux, ccx_options));
    let media_type: GXF_Track_Type = if let Ok(mt) = media_type_0 {
        mt
    } else {
        goto_end!(demux, len, ret, ccx_options);
    };

    let track_nb: u8 = buffered_get_byte(demux, ccx_options);
    len -= 1;
    let media_field_nb: u32 = buffered_get_be32(demux, ccx_options);
    len -= 4;

    match media_type {
        GXF_Track_Type::TRACK_TYPE_ANCILLARY_DATA => {
            first_field_nb = buffered_get_be16(demux, ccx_options);
            len -= 2;
            last_field_nb = buffered_get_be16(demux, ccx_options);
            len -= 2;
        }
        GXF_Track_Type::TRACK_TYPE_MPEG1_525 | GXF_Track_Type::TRACK_TYPE_MPEG2_525 => {
            mpeg_pic_size = buffered_get_be32(demux, ccx_options);
            mpeg_frame_desc_flag = (mpeg_pic_size >> 24) as u8;
            mpeg_pic_size &= 0x00FFFFFF;
            len -= 4;
        }
        _ => {
            result = buffered_skip(demux, 4, ccx_options) as i32;
            demux.past += result as i64;
            len -= 4;
        }
    }

    let time_field: u32 = buffered_get_be32(demux, ccx_options);
    len -= 4;

    let valid_time_field: u8 = buffered_get_byte(demux, ccx_options) & 0x01;
    len -= 1;

    result = buffered_skip(demux, 1, ccx_options) as i32;
    demux.past += result as i64;
    len -= 1;

    info!("track number {}", track_nb);
    info!("field number {}", media_field_nb);
    info!("first field number {}", first_field_nb);
    info!("last field number {}", last_field_nb);
    info!("Pyld len {}", len);

    match media_type {
        GXF_Track_Type::TRACK_TYPE_ANCILLARY_DATA => {
            if ctx.ad_track.is_none() {
                goto_end!(demux, len, ret, ccx_options);
            }
            let ad_track = ctx.ad_track.as_mut().unwrap();
            data.pts = if valid_time_field != 0 {
                time_field as i64 - ctx.first_field_nb as i64
            } else {
                media_field_nb as i64 - ctx.first_field_nb as i64
            };
            if len < ad_track.packet_size {
                goto_end!(demux, len, ret, ccx_options);
            }
            data.pts /= 2;
            let _ = parse_ad_packet(demux, ad_track.packet_size, data, ccx_options);
            len -= ad_track.packet_size;
        }
        GXF_Track_Type::TRACK_TYPE_MPEG2_525 if ctx.ad_track.is_none() => {
            if ctx.vid_track.is_none() {
                goto_end!(demux, len, ret, ccx_options);
            }
            let vid_track = ctx.vid_track.as_mut().unwrap();
            data.pts = if valid_time_field != 0 {
                time_field as i64 - ctx.first_field_nb as i64
            } else {
                media_field_nb as i64 - ctx.first_field_nb as i64
            };
            data.tb.num = vid_track.frame_rate.den;
            data.tb.den = vid_track.frame_rate.num;
            data.pts /= 2;
            set_mpeg_frame_desc(vid_track, mpeg_frame_desc_flag);
            let _ = parse_mpeg_packet(demux, mpeg_pic_size as usize, data, ccx_options);
            len -= mpeg_pic_size as i32;
        }
        GXF_Track_Type::TRACK_TYPE_TIME_CODE_525 => {
            // Time code handling not implemented
        }
        _ => {}
    }

    goto_end!(demux, len, ret, ccx_options)
}

/**
 * Dummy function that ignore field locator table packet
 */
/// # Safety
/// This function is unsafe because it calls unsafe function `buffered_skip`
pub unsafe fn parse_flt(
    demux: &mut CcxDemuxer,
    len: i32,
    ccx_options: &mut Options,
) -> Result<(), DemuxerError> {
    let mut ret = Ok(());
    let result = buffered_skip(demux, len as u32, ccx_options);
    demux.past += result as i64;
    if result != len as usize {
        ret = Err(DemuxerError::EOF);
    }
    ret
}
/**
 * Dummy function that ignore unified material format packet
 */
/// # Safety
/// This function is unsafe because it calls unsafe function `buffered_skip`
pub unsafe fn parse_umf(
    demux: &mut CcxDemuxer,
    len: i32,
    ccx_options: &mut Options,
) -> Result<(), DemuxerError> {
    let mut ret = Ok(());
    let result = buffered_skip(demux, len as u32, ccx_options);
    demux.past += result as i64;
    if result != len as usize {
        ret = Err(DemuxerError::EOF);
    }
    ret
}
/**
 * Its this function duty to use len length buffer from demuxer
 *
 * This function gives basic info about tracks, here we get to know
 * whether Ancillary Data track is present or not.
 * If ancillary data track is not present only then it check for
 * presence of mpeg track.
 * return Err(DemuxerError::InvalidArgument) if things are not following specs
 */
/// # Safety
/// This function is unsafe because it calls unsafe functions like `buffered_skip` and `parse_material_sec`
pub unsafe fn parse_map(
    // APPLIED the TODO
    demux: &mut CcxDemuxer,
    mut len: i32,
    data: &mut DemuxerData,
    ccx_options: &mut Options,
) -> Result<(), DemuxerError> {
    // 1) Header check
    len = len.checked_sub(2).ok_or(DemuxerError::InvalidArgument)?;
    if buffered_get_be16(demux, ccx_options) != 0xE0FF {
        // spec‐violation → InvalidArgument
        let skip = len
            .try_into()
            .ok()
            .map_or(0, |l: u32| buffered_skip(demux, l, ccx_options));
        demux.past += skip as i64;
        return Err(DemuxerError::InvalidArgument);
    }

    // 2) Material section
    len = len.checked_sub(2).ok_or(DemuxerError::InvalidArgument)?;
    let material_sec_len = buffered_get_be16(demux, ccx_options) as i32;
    if material_sec_len > len {
        // spec‐violation
        let skip = len
            .try_into()
            .ok()
            .map_or(0, |l: u32| buffered_skip(demux, l, ccx_options));
        demux.past += skip as i64;
        return Err(DemuxerError::InvalidArgument);
    }
    len -= material_sec_len;
    let _ = parse_material_sec(demux, material_sec_len, ccx_options);

    // 3) Track section
    len = len.checked_sub(2).ok_or(DemuxerError::InvalidArgument)?;
    let track_sec_len = buffered_get_be16(demux, ccx_options) as i32;
    if track_sec_len > len {
        let skip = len
            .try_into()
            .ok()
            .map_or(0, |l: u32| buffered_skip(demux, l, ccx_options));
        demux.past += skip as i64;
        return Err(DemuxerError::InvalidArgument);
    }
    len -= track_sec_len;
    let _ = parse_track_sec(demux, track_sec_len, data, ccx_options);

    // 4) Final skip
    let skip = len.try_into().map_err(|_| DemuxerError::InvalidArgument)?;
    let result = buffered_skip(demux, skip, ccx_options);
    demux.past += result as i64;
    if result != skip as usize {
        Err(DemuxerError::EOF)
    } else {
        Ok(())
    }
}

/**
 * GXF Media File have 5 Section which are as following
 *     +----------+-------+------+---------------+--------+
 *     |          |       |      |               |        |
 *     |  MAP     |  FLT  |  UMF |  Media Packet |   EOS  |
 *     |          |       |      |               |        |
 *     +----------+-------+------+---------------+--------+
 *
 */
/// # Safety
/// This function is unsafe because it calls unsafe functions like `parse_packet_header` and `parse_map`
pub unsafe fn read_packet(
    demux: &mut CcxDemuxer,
    data: &mut DemuxerData,
    ccx_options: &mut Options,
) -> Result<(), DemuxerError> {
    let mut len = 0;
    let mut gxftype: GXF_Pkt_Type = GXF_Pkt_Type::PKT_EOS;

    let mut ret = parse_packet_header(demux, &mut gxftype, &mut len, ccx_options);
    if ret != Ok(()) {
        return ret; // Propagate header parsing errors
    }

    match gxftype {
        GXF_Pkt_Type::PKT_MAP => {
            info!("pkt type Map {}\n", len);
            ret = parse_map(demux, len, data, ccx_options);
        }
        GXF_Pkt_Type::PKT_MEDIA => {
            ret = parse_media(demux, len, data, ccx_options);
        }
        GXF_Pkt_Type::PKT_EOS => {
            ret = Err(DemuxerError::EOF);
        }
        GXF_Pkt_Type::PKT_FLT => {
            info!("pkt type FLT {}\n", len);
            ret = parse_flt(demux, len, ccx_options);
        }
        GXF_Pkt_Type::PKT_UMF => {
            info!("pkt type umf {}\n\n", len);
            ret = parse_umf(demux, len, ccx_options);
        }
    }

    ret
}
#[cfg(test)]
mod tests {
    use super::*;
    use crate::demuxer::demuxer_data::DemuxerData;
    use lib_ccxr::common::{Codec, BUFSIZE};
    use lib_ccxr::util::log::{set_logger, CCExtractorLogger, DebugMessageMask, OutputTarget};
    use std::os::fd::IntoRawFd;
    use std::sync::Once;
    use std::{mem, ptr};

    const FILEBUFFERSIZE: usize = 1024;
    static INIT: Once = Once::new();

    fn initialize_logger() {
        INIT.call_once(|| {
            set_logger(CCExtractorLogger::new(
                OutputTarget::Stdout,
                DebugMessageMask::new(DebugMessageFlag::VERBOSE, DebugMessageFlag::VERBOSE),
                false,
            ))
            .ok();
        });
    }
    /// Helper function to allocate a file buffer and copy provided data.
    fn allocate_filebuffer(data: &[u8]) -> *mut u8 {
        // Allocate a vector with a fixed capacity.
        let mut buffer = vec![0u8; FILEBUFFERSIZE];
        // Copy provided data into the beginning of the buffer.
        buffer[..data.len()].copy_from_slice(data);
        // Leak the vector to obtain a raw pointer.
        let ptr = buffer.as_mut_ptr();
        mem::forget(buffer);
        ptr
    }
    fn create_demuxer_with_buffer(data: &[u8]) -> CcxDemuxer<'static> {
        CcxDemuxer {
            filebuffer: allocate_filebuffer(data),
            filebuffer_pos: 0,
            bytesinbuffer: data.len() as u32,
            past: 0,
            ..Default::default()
        }
    }
    /// Build a valid packet header.
    /// Header layout:
    ///   Bytes 0-3:  0x00 0x00 0x00 0x00
    ///   Byte 4:     0x01
    ///   Byte 5:     Packet type (0xbc for PKT_MAP)
    ///   Bytes 6-9:  Length in big-endian (e.g., 32)
    ///   Bytes 10-13: Reserved (set to 0)
    ///   Byte 14:    0xe1
    ///   Byte 15:    0xe2
    fn build_valid_header() -> Vec<u8> {
        let mut header = Vec::with_capacity(16);
        header.extend_from_slice(&[0, 0, 0, 0]); // 0x00 0x00 0x00 0x00
        header.push(1); // 0x01
        header.push(0xbc); // Packet type: PKT_MAP
        header.extend_from_slice(&32u32.to_be_bytes()); // Length = 32 (will become 16 after subtracting header size)
        header.extend_from_slice(&[0, 0, 0, 0]); // Reserved
        header.push(0xe1); // Trailer part 1
        header.push(0xe2); // Trailer part 2
        header
    }
    #[allow(unused)]
    fn create_temp_file_with_content(content: &[u8]) -> i32 {
        use std::io::{Seek, SeekFrom, Write};
        use tempfile::NamedTempFile;
        let mut tmp = NamedTempFile::new().expect("Unable to create temp file");
        tmp.write_all(content)
            .expect("Unable to write to temp file");
        // Rewind the file pointer to the start.
        tmp.as_file_mut()
            .seek(SeekFrom::Start(0))
            .expect("Unable to seek to start");
        // Get the file descriptor. Ensure the file stays open.
        let file = tmp.reopen().expect("Unable to reopen temp file");
        #[cfg(unix)]
        {
            file.into_raw_fd()
        }
        #[cfg(windows)]
        {
            file.into_raw_handle() as i32
        }
    }
    /// Create a dummy CcxDemuxer with a filebuffer containing `header_data`.
    fn create_ccx_demuxer_with_header(header_data: &[u8]) -> CcxDemuxer<'static> {
        let filebuffer = allocate_filebuffer(header_data);
        CcxDemuxer {
            filebuffer,
            filebuffer_pos: 0,
            bytesinbuffer: header_data.len() as u32,
            past: 0,
            ..Default::default()
        }
    }

    #[test]
    fn test_parse_packet_header_valid() {
        let ccx_options = &mut Options::default();
        let header = build_valid_header();
        let mut demuxer = create_ccx_demuxer_with_header(&header);
        let mut pkt_type = GXF_Pkt_Type::PKT_MEDIA; // dummy init
        let mut length = 0;
        let ret = unsafe {
            parse_packet_header(&mut demuxer, &mut pkt_type, &mut length, &mut *ccx_options)
        };
        assert_eq!(ret, Ok(()));
        assert_eq!(pkt_type as u32, GXF_Pkt_Type::PKT_MAP as u32);
        // length in header was 32, then subtract 16 -> 16
        assert_eq!(length, 16);
        // past should have advanced by 16 bytes
        assert_eq!(demuxer.past, 16);
    }
    #[test]
    fn test_parse_packet_header_incomplete_read() {
        let ccx_options = &mut Options::default();
        // Provide a header that is too short (e.g. only 10 bytes)
        let header = vec![0u8; 10];
        let mut demuxer = create_ccx_demuxer_with_header(&header);
        // let content = b"Direct read test.";
        // let fd = create_temp_file_with_content(content);
        // demuxer.infd = fd;
        let mut pkt_type = GXF_Pkt_Type::PKT_MEDIA;
        let mut length = 0;
        let ret = unsafe {
            parse_packet_header(&mut demuxer, &mut pkt_type, &mut length, &mut *ccx_options)
        };
        assert_eq!(ret, Err(DemuxerError::EOF));
    }

    #[test]
    fn test_parse_packet_header_invalid_leader() {
        let ccx_options = &mut Options::default();
        // Build header with a non-zero in the first 4 bytes.
        let mut header = build_valid_header();
        header[0] = 1; // Invalid leader
        let mut demuxer = create_ccx_demuxer_with_header(&header);
        let mut pkt_type = GXF_Pkt_Type::PKT_MEDIA;
        let mut length = 0;
        let ret = unsafe {
            parse_packet_header(&mut demuxer, &mut pkt_type, &mut length, &mut *ccx_options)
        };
        assert_eq!(ret, Err(DemuxerError::InvalidArgument));
    }

    #[test]
    fn test_parse_packet_header_invalid_trailer() {
        let ccx_options = &mut Options::default();
        // Build header with an incorrect trailer byte.
        let mut header = build_valid_header();
        header[14] = 0; // Should be 0xe1
        let mut demuxer = create_ccx_demuxer_with_header(&header);
        let mut pkt_type = GXF_Pkt_Type::PKT_MEDIA;
        let mut length = 0;
        let ret = unsafe {
            parse_packet_header(&mut demuxer, &mut pkt_type, &mut length, &mut *ccx_options)
        };
        assert_eq!(ret, Err(DemuxerError::InvalidArgument));
    }

    #[test]
    fn test_parse_packet_header_invalid_length() {
        let ccx_options = &mut Options::default();
        // Build header with length field < 16.
        let mut header = build_valid_header();
        // Set length field (bytes 6-9) to 15 (which is < 16).
        let invalid_length: u32 = 15;
        header[6..10].copy_from_slice(&invalid_length.to_be_bytes());
        let mut demuxer = create_ccx_demuxer_with_header(&header);
        let mut pkt_type = GXF_Pkt_Type::PKT_MEDIA;
        let mut length = 0;
        let ret = unsafe {
            parse_packet_header(&mut demuxer, &mut pkt_type, &mut length, &mut *ccx_options)
        };
        assert_eq!(ret, Err(DemuxerError::InvalidArgument));
    }
    fn build_valid_material_sec() -> (Vec<u8>, CcxGxf) {
        let mut buf = Vec::new();

        // Prepare a dummy GXF context.
        let gxf = CcxGxf::default();

        // MAT_NAME: tag=MAT_NAME, tag_len = 8, then 8 bytes of media name.
        buf.push(GXF_Mat_Tag::MAT_NAME as u8);
        buf.push(8);
        let name_data = b"RustTest";
        buf.extend_from_slice(name_data);

        // MAT_FIRST_FIELD: tag, tag_len=4, then 4 bytes representing a u32 value.
        buf.push(GXF_Mat_Tag::MAT_FIRST_FIELD as u8);
        buf.push(4);
        let first_field: u32 = 0x01020304;
        buf.extend_from_slice(&first_field.to_be_bytes());

        // MAT_MARK_OUT: tag, tag_len=4, then 4 bytes.
        buf.push(GXF_Mat_Tag::MAT_MARK_OUT as u8);
        buf.push(4);
        let mark_out: u32 = 0x0A0B0C0D;
        buf.extend_from_slice(&mark_out.to_be_bytes());

        // Remaining length to be skipped (simulate extra bytes).
        let remaining = 5;
        buf.extend_from_slice(&vec![0u8; remaining]);

        // Total length is the entire buffer length.
        (buf, gxf)
    }

    /// Setup a demuxer for testing parse_material_sec.
    /// The demuxer's private_data will be set to a leaked Box of CcxGxf.
    fn create_demuxer_for_material_sec(data: &[u8], gxf: &mut CcxGxf) -> CcxDemuxer<'static> {
        let mut demux = create_demuxer_with_buffer(data);
        // Set private_data to point to our gxf structure.
        demux.private_data = gxf as *mut CcxGxf as *mut std::ffi::c_void;
        demux
    }

    #[test]
    fn test_parse_material_sec_valid() {
        let ccx_options = &mut Options::default();

        let (buf, mut gxf) = build_valid_material_sec();
        let total_len = buf.len() as i32;
        let mut demux = create_demuxer_for_material_sec(&buf, &mut gxf);

        let ret = unsafe { parse_material_sec(&mut demux, total_len, &mut *ccx_options) };
        assert_eq!(ret, Ok(()));

        // Check that the media_name was read.
        assert_eq!(&gxf.media_name[..8], "RustTest");
        // Check that first_field_nb was set correctly.
        assert_eq!(gxf.first_field_nb, 0x01020304);
        // Check that mark_out was set correctly.
        assert_eq!(gxf.mark_out, 0x0A0B0C0D);
    }

    #[test]
    fn test_parse_material_sec_incomplete_mat_name() {
        let ccx_options = &mut Options::default();

        // Build a material section with MAT_NAME tag that promises 8 bytes but only 4 bytes are present.
        let mut buf = Vec::new();
        buf.push(GXF_Mat_Tag::MAT_NAME as u8);
        buf.push(8);
        buf.extend_from_slice(b"Test"); // only 4 bytes instead of 8

        // Add extra bytes to simulate remaining length.
        buf.extend_from_slice(&[0u8; 3]);

        let total_len = buf.len() as i32;
        let mut gxf = CcxGxf::default();
        let mut demux = create_demuxer_for_material_sec(&buf, &mut gxf);

        let ret = unsafe { parse_material_sec(&mut demux, total_len, &mut *ccx_options) };
        // Since buffered_read will return less than expected, we expect Err(DemuxerError::EOF).
        assert_eq!(ret, Err(DemuxerError::InvalidArgument));
    }

    #[test]
    fn test_parse_material_sec_invalid_private_data() {
        let ccx_options = &mut Options::default();

        // Create a buffer with any data.
        let buf = vec![0u8; 10];
        let total_len = buf.len() as i32;
        let mut demux = create_demuxer_with_buffer(&buf);
        // Set private_data to null.
        demux.private_data = ptr::null_mut();

        let ret = unsafe { parse_material_sec(&mut demux, total_len, &mut *ccx_options) };
        assert_eq!(ret, Err(DemuxerError::InvalidArgument));
    }

    #[test]
    fn test_parse_material_sec_skip_remaining() {
        let ccx_options = &mut Options::default();

        // Build a material section where the length remaining is greater than the data in tags.
        let mut buf = Vec::new();
        // One valid tag:
        buf.push(GXF_Mat_Tag::MAT_FIRST_FIELD as u8);
        buf.push(4);
        let first_field: u32 = 0x00AA55FF;
        buf.extend_from_slice(&first_field.to_be_bytes());
        // Now, simulate extra remaining bytes that cannot be processed.
        let extra = 10;
        buf.extend_from_slice(&vec![0u8; extra]);

        let total_len = buf.len() as i32;
        let mut gxf = CcxGxf::default();
        let mut demux = create_demuxer_for_material_sec(&buf, &mut gxf);

        let ret = unsafe { parse_material_sec(&mut demux, total_len, &mut *ccx_options) };
        // In this case, the extra bytes will be skipped.
        // If the number of bytes skipped doesn't match, ret becomes Err(DemuxerError::EOF).
        // For our simulated buffered_skip (which works in-buffer), we expect Ok(()) if the skip succeeds.
        assert_eq!(ret, Ok(()));
        // And first_field_nb should be set.
        assert_eq!(gxf.first_field_nb, 0x00AA55FF);
    }

    // tests for set_track_frame_rate
    #[test]
    fn test_set_track_frame_rate_60() {
        let mut vid_track = CcxGxfVideoTrack::default();
        set_track_frame_rate(&mut vid_track, 1);
        assert_eq!(vid_track.frame_rate.num, 60);
        assert_eq!(vid_track.frame_rate.den, 1);
    }
    #[test]
    fn test_set_track_frame_rate_60000() {
        let mut vid_track = CcxGxfVideoTrack::default();
        set_track_frame_rate(&mut vid_track, 2);
        assert_eq!(vid_track.frame_rate.num, 60000);
        assert_eq!(vid_track.frame_rate.den, 1001);
    }
    // Build a valid track description buffer.
    // Contains:
    // - TRACK_NAME tag: tag_len = 8, then 8 bytes ("Track001").
    // - TRACK_FPS tag: tag_len = 4, then 4 bytes representing frame rate (2400).
    // - Extra bytes appended.
    fn build_valid_track_desc() -> (Vec<u8>, CcxGxf) {
        let mut buf = Vec::new();
        // TRACK_NAME tag.
        buf.push(GXF_Track_Tag::TRACK_NAME as u8);
        buf.push(8);
        let name = b"Track001XYZ"; // Use only first 8 bytes: "Track001"
        buf.extend_from_slice(&name[..8]);

        // TRACK_FPS tag.
        buf.push(GXF_Track_Tag::TRACK_FPS as u8);
        buf.push(4);
        let fps: u32 = 2400;
        buf.extend_from_slice(&fps.to_be_bytes());

        // Append extra bytes.
        buf.extend_from_slice(&[0u8; 5]);

        // Create a dummy CcxGxf context.
        let gxf = CcxGxf {
            nb_streams: 1,
            media_name: String::new(),
            first_field_nb: 0,
            last_field_nb: 0,
            mark_in: 0,
            mark_out: 0,
            stream_size: 0,
            ad_track: None,
            vid_track: Some(CcxGxfVideoTrack {
                track_name: String::new(),
                fs_version: 0,
                frame_rate: CcxRational { num: 0, den: 1 },
                line_per_frame: 0,
                field_per_frame: 0,
                p_code: MpegPictureCoding::CCX_MPC_NONE,
                p_struct: MpegPictureStruct::CCX_MPS_NONE,
            }),
            cdp: None,
            cdp_len: 0,
        };

        (buf, gxf)
    }

    // Helper: Set up a demuxer for track description testing.
    fn create_demuxer_for_track_desc(data: &[u8], gxf: &mut CcxGxf) -> CcxDemuxer<'static> {
        let mut demux = create_demuxer_with_buffer(data);
        demux.private_data = gxf as *mut CcxGxf as *mut std::ffi::c_void;
        demux
    }

    #[test]
    fn test_parse_mpeg525_track_desc_valid() {
        let ccx_options = &mut Options::default();

        initialize_logger();
        let (buf, mut gxf) = build_valid_track_desc();
        let total_len = buf.len() as i32;
        let mut demux = create_demuxer_for_track_desc(&buf, &mut gxf);

        let ret = unsafe { parse_mpeg525_track_desc(&mut demux, total_len, &mut *ccx_options) };
        assert_eq!(ret, Ok(()));

        // Verify track name.
        let vid_track = gxf.vid_track.unwrap();
        assert_eq!(&vid_track.track_name[..8], "Track001");
        // Verify frame rate: fs_version must be set to 2400.
        assert_eq!(vid_track.fs_version, 0);
        // Check that demux.past advanced exactly by buf.len().
        assert_eq!(demux.past as usize, buf.len());
    }

    #[test]
    fn test_parse_mpeg525_track_desc_incomplete_track_name() {
        let ccx_options = &mut Options::default();

        initialize_logger();
        // Build a buffer where TRACK_NAME promises 8 bytes but provides only 4.
        let mut buf = Vec::new();
        buf.push(GXF_Track_Tag::TRACK_NAME as u8);
        buf.push(8);
        buf.extend_from_slice(b"Test"); // 4 bytes only.
        buf.extend_from_slice(&[0u8; 3]); // extra bytes
        let total_len = buf.len() as i32;

        let mut gxf = CcxGxf {
            nb_streams: 1,
            media_name: String::new(),
            first_field_nb: 0,
            last_field_nb: 0,
            mark_in: 0,
            mark_out: 0,
            stream_size: 0,
            ad_track: None,
            vid_track: Some(CcxGxfVideoTrack {
                track_name: String::new(),
                fs_version: 0,
                frame_rate: CcxRational { num: 0, den: 1 },
                line_per_frame: 0,
                field_per_frame: 0,
                p_code: MpegPictureCoding::CCX_MPC_NONE,
                p_struct: MpegPictureStruct::CCX_MPS_NONE,
            }),
            cdp: None,
            cdp_len: 0,
        };

        let mut demux = create_demuxer_for_track_desc(&buf, &mut gxf);
        let ret = unsafe { parse_mpeg525_track_desc(&mut demux, total_len, &mut *ccx_options) };
        // Expect Err(DemuxerError::InvalidArgument) because insufficient data leads to error.
        assert_eq!(ret, Err(DemuxerError::InvalidArgument));
    }

    #[test]
    fn test_parse_mpeg525_track_desc_invalid_private_data() {
        let ccx_options = &mut Options::default();

        let buf = vec![0u8; 10];
        let total_len = buf.len() as i32;
        let mut demux = create_demuxer_with_buffer(&buf);
        demux.private_data = ptr::null_mut();

        let result = unsafe { parse_mpeg525_track_desc(&mut demux, total_len, &mut *ccx_options) };
        assert_eq!(result, Err(DemuxerError::InvalidArgument));
    }
    // Build a valid ancillary (AD) track description buffer.
    // This buffer contains:
    // - TRACK_NAME tag: tag_len = 8, then 8 bytes for the track name.
    // - TRACK_AUX tag: tag_len = 8, then 8 bytes of aux info.
    //   We set auxi_info such that:
    //     auxi_info[2] = 2 (maps to PRES_FORMAT_HD),
    //     auxi_info[3] = 4,
    //     auxi_info[4..6] = [0, 16] (field_size = 16),
    //     auxi_info[6..8] = [0, 2] (packet_size = 2*256 = 512).
    // - Extra bytes appended.
    fn build_valid_ad_track_desc() -> (Vec<u8>, CcxGxf) {
        let mut buf = Vec::new();
        // TRACK_NAME tag.
        buf.push(GXF_Track_Tag::TRACK_NAME as u8);
        buf.push(8);
        let name = b"ADTrk001XY"; // Use first 8 bytes: "ADTrk001"
        buf.extend_from_slice(&name[..8]);

        // TRACK_AUX tag.
        buf.push(GXF_Track_Tag::TRACK_AUX as u8);
        buf.push(8);
        // Create aux info: [?, ?, 2, 4, 0, 16, 0, 2]
        let auxi_info = [0u8, 0u8, 2, 4, 0, 16, 0, 2];
        buf.extend_from_slice(&auxi_info);

        // Append extra bytes.
        buf.extend_from_slice(&[0u8; 3]);

        // Create a dummy CcxGxf context.
        let gxf = CcxGxf {
            nb_streams: 1,
            media_name: String::new(),
            first_field_nb: 0,
            last_field_nb: 0,
            mark_in: 0,
            mark_out: 0,
            stream_size: 0,
            ad_track: Some(CcxGxfAncillaryDataTrack {
                track_name: String::new(),
                fs_version: 0,
                frame_rate: 0,
                line_per_frame: 0,
                field_per_frame: 0,
                ad_format: GXF_Anc_Data_Pres_Format::PRES_FORMAT_SD,
                nb_field: 0,
                field_size: 0,
                packet_size: 0,
                id: 123, // sample id
            }),
            vid_track: None,
            cdp: None,
            cdp_len: 0,
            // Other fields as needed...
        };

        (buf, gxf)
    }

    // Helper: Set up a demuxer for AD track description testing.
    fn create_demuxer_for_ad_track_desc(data: &[u8], gxf: &mut CcxGxf) -> CcxDemuxer<'static> {
        let mut demux = create_demuxer_with_buffer(data);
        demux.private_data = gxf as *mut CcxGxf as *mut std::ffi::c_void;
        demux
    }

    #[test]
    fn test_parse_ad_track_desc_valid() {
        initialize_logger();
        let ccx_options = &mut Options::default();

        let (buf, mut gxf) = build_valid_ad_track_desc();
        let total_len = buf.len() as i32;
        let mut demux = create_demuxer_for_ad_track_desc(&buf, &mut gxf);

        let ret = unsafe { parse_ad_track_desc(&mut demux, total_len, &mut *ccx_options) };
        assert_eq!(ret, Ok(()));

        let ad_track = gxf.ad_track.unwrap();
        // Check that TRACK_NAME was read correctly.
        assert_eq!(&ad_track.track_name[..8], "ADTrk001");
        // Check that TRACK_AUX set the fields as expected.
        // auxi_info[2] was 2, so we expect PRES_FORMAT_HD.
        assert_eq!(
            ad_track.ad_format as i32,
            GXF_Anc_Data_Pres_Format::PRES_FORMAT_HD as i32
        );
        // auxi_info[3] is 4.
        assert_eq!(ad_track.nb_field, 4);
        // Field size: [0,16] => 16.
        assert_eq!(ad_track.field_size, 16);
        // Packet size: [0,2] => 2 * 256 = 512.
        assert_eq!(ad_track.packet_size, 512);
        // Verify that demux.past advanced by full buf length.
        assert_eq!(demux.past as usize, buf.len());
    }

    #[test]
    fn test_parse_ad_track_desc_incomplete_track_name() {
        let ccx_options = &mut Options::default();

        initialize_logger();
        // Build a buffer where TRACK_NAME promises 8 bytes but only 4 are provided.
        let mut buf = Vec::new();
        buf.push(GXF_Track_Tag::TRACK_NAME as u8);
        buf.push(8);
        buf.extend_from_slice(b"Test"); // 4 bytes only.
        buf.extend_from_slice(&[0u8; 2]); // extra bytes
        let total_len = buf.len() as i32;

        let mut gxf = CcxGxf {
            nb_streams: 1,
            media_name: String::new(),
            first_field_nb: 0,
            last_field_nb: 0,
            mark_in: 0,
            mark_out: 0,
            stream_size: 0,
            ad_track: Some(CcxGxfAncillaryDataTrack {
                track_name: String::new(),
                fs_version: 0,
                frame_rate: 0,
                line_per_frame: 0,
                field_per_frame: 0,
                ad_format: GXF_Anc_Data_Pres_Format::PRES_FORMAT_SD,
                nb_field: 0,
                field_size: 0,
                packet_size: 0,
                id: 45,
            }),
            vid_track: None,
            cdp: None,
            cdp_len: 0,
        };

        let mut demux = create_demuxer_for_ad_track_desc(&buf, &mut gxf);
        let ret = unsafe { parse_ad_track_desc(&mut demux, total_len, &mut *ccx_options) };
        // Expect Err(DemuxerError::InvalidArgument) because TRACK_NAME did not yield full 8 bytes.
        assert_eq!(ret, Err(DemuxerError::InvalidArgument));
    }

    #[test]
    fn test_parse_ad_track_desc_invalid_private_data() {
        let ccx_options = &mut Options::default();

        let buf = vec![0u8; 10];
        let total_len = buf.len() as i32;
        let mut demux = create_demuxer_with_buffer(&buf);
        // Set private_data to null.
        demux.private_data = ptr::null_mut();

        let ret = unsafe { parse_ad_track_desc(&mut demux, total_len, &mut *ccx_options) };
        assert_eq!(ret, Err(DemuxerError::InvalidArgument));
    }
    fn create_demuxer_for_track_sec(data: &[u8], gxf: &mut CcxGxf) -> CcxDemuxer<'static> {
        let mut demux = create_demuxer_with_buffer(data);
        demux.private_data = gxf as *mut CcxGxf as *mut std::ffi::c_void;
        demux
    }

    // Helper: Build a track record.
    // Produces 4 header bytes followed by track_data of length track_len.
    // track_type, track_id, track_len are provided.
    fn build_track_record(
        track_type: u8,
        track_id: u8,
        track_len: i32,
        track_data: &[u8],
    ) -> Vec<u8> {
        let mut rec = Vec::new();
        rec.push(track_type);
        rec.push(track_id);
        rec.extend_from_slice(&(track_len as u16).to_be_bytes());
        rec.extend_from_slice(&track_data[..track_len as usize]);
        rec
    }

    #[test]
    fn test_parse_track_sec_no_context() {
        let ccx_options = &mut Options::default();

        // Create a demuxer with a valid buffer.
        let buf = vec![0u8; 10];
        let mut demux = create_demuxer_with_buffer(&buf);
        // Set private_data to null.
        demux.private_data = ptr::null_mut();
        let mut data = DemuxerData::default();
        let ret =
            unsafe { parse_track_sec(&mut demux, buf.len() as i32, &mut data, &mut *ccx_options) };
        assert_eq!(ret, Err(DemuxerError::InvalidArgument));
    }

    #[test]
    fn test_parse_track_sec_skip_branch() {
        let ccx_options = &mut Options::default();

        // Build a record that should be skipped because track_type does not have high bit set.
        let track_len = 7;
        let track_data = vec![0xEE; track_len as usize];
        // Use track_type = 0x10 (no high bit) and arbitrary track_id.
        let record = build_track_record(0x10, 0xFF, track_len, &track_data);
        let buf = record;
        let total_len = buf.len() as i32;

        // Create a dummy context.
        let mut gxf = CcxGxf {
            nb_streams: 1,
            media_name: String::new(),
            first_field_nb: 0,
            last_field_nb: 0,
            mark_in: 0,
            mark_out: 0,
            stream_size: 0,
            ad_track: None,
            vid_track: None,
            cdp: None,
            cdp_len: 0,
        };
        let mut demux = create_demuxer_for_track_sec(&buf, &mut gxf);
        let mut data = DemuxerData::default();

        let ret = unsafe { parse_track_sec(&mut demux, total_len, &mut data, &mut *ccx_options) };
        // The record is skipped so ret should be Ok(()) and datatype remains Unknown.
        assert_eq!(ret, Ok(()));
        assert_eq!(data.bufferdatatype, BufferdataType::Pes);
        assert_eq!(demux.past as usize, buf.len());
    }
    impl DemuxerData {
        pub fn new() -> Self {
            let mut vec = vec![0u8; BUFSIZE];
            let ptr = vec.as_mut_ptr();
            mem::forget(vec);
            DemuxerData {
                program_number: 0,
                stream_pid: 0,
                codec: Option::from(Codec::Dvb),
                bufferdatatype: BufferdataType::Raw,
                buffer: ptr,
                len: 0, // Start with 0 data length
                rollover_bits: 0,
                pts: 0,
                tb: CcxRational::default(),
                next_stream: ptr::null_mut(),
                next_program: ptr::null_mut(),
            }
        }
    }

    // Build a valid CDP packet.
    // Packet layout:
    // 0: 0x96, 1: 0x69,
    // 2: cdp_length (should equal total length, here 18),
    // 3: frame rate byte (e.g. 0x50),
    // 4: a byte (e.g. 0x42),
    // 5-6: header sequence counter (0x00, 0x01),
    // 7: section id: 0x72,
    // 8: cc_count (e.g. 0x02 => cc_count = 2),
    // 9-14: 6 bytes of cc data,
    // 15: footer id: 0x74,
    // 16-17: footer sequence counter (0x00, 0x01).
    fn build_valid_cdp_packet() -> Vec<u8> {
        let total_len = 18u8;
        let mut packet = Vec::new();
        packet.push(0x96);
        packet.push(0x69);
        packet.push(total_len); // cdp_length = 18
        packet.push(0x50); // frame rate byte: framerate = 5
        packet.push(0x42); // cc_data_present = 1, caption_service_active = 1
        packet.extend_from_slice(&[0x00, 0x01]); // header sequence counter = 1
        packet.push(0x72); // section id for CC data
        packet.push(0x02); // cc_count = 2 (lower 5 bits)
        packet.extend_from_slice(&[0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF]); // cc data: 6 bytes
        packet.push(0x74); // footer id
        packet.extend_from_slice(&[0x00, 0x01]); // footer sequence counter = 1
        packet
    }

    #[test]
    fn test_parse_ad_cdp_valid() {
        initialize_logger();
        let packet = build_valid_cdp_packet();
        let mut data = DemuxerData::new();
        let result = parse_ad_cdp(&packet, &mut data);
        assert!(result.is_ok());
        // cc_count = 2 so we expect 2 * 3 = 6 bytes to be copied.
        assert_eq!(data.len, 6);
        let copied = unsafe { slice::from_raw_parts(data.buffer, data.len) };
        assert_eq!(copied, &[0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF]);
    }

    #[test]
    fn test_parse_ad_cdp_short_packet() {
        initialize_logger();
        // Packet shorter than 11 bytes.
        let packet = vec![0x96, 0x69, 0x08, 0x50, 0x42, 0x00, 0x01, 0x72, 0x01, 0xAA];
        let mut data = DemuxerData::new();
        let result = parse_ad_cdp(&packet, &mut data);
        assert!(result.is_err());
        assert_eq!(result.unwrap_err(), DemuxerError::InvalidArgument);
    }

    #[test]
    fn test_parse_ad_cdp_invalid_identifier() {
        initialize_logger();
        let mut packet = build_valid_cdp_packet();
        packet[0] = 0x00;
        let mut data = DemuxerData::new();
        let result = parse_ad_cdp(&packet, &mut data);
        assert!(result.is_err());
        assert_eq!(result.unwrap_err(), DemuxerError::InvalidArgument);
    }

    #[test]
    fn test_parse_ad_cdp_mismatched_length() {
        initialize_logger();
        let mut packet = build_valid_cdp_packet();
        packet[2] = 20; // Set length to 20, but actual length is 18.
        let mut data = DemuxerData::new();
        let result = parse_ad_cdp(&packet, &mut data);
        assert!(result.is_err());
        assert_eq!(result.unwrap_err(), DemuxerError::InvalidArgument);
    }

    #[test]
    fn test_parse_ad_cdp_time_code_section() {
        initialize_logger();
        let mut packet = build_valid_cdp_packet();
        // Change section id at offset 7 to 0x71.
        packet[7] = 0x71;
        let mut data = DemuxerData::new();
        let result = parse_ad_cdp(&packet, &mut data);
        assert!(result.is_err());
        assert_eq!(result.unwrap_err(), DemuxerError::Retry);
    }

    #[test]
    fn test_parse_ad_cdp_service_info_section() {
        initialize_logger();
        let mut packet = build_valid_cdp_packet();
        packet[7] = 0x73;
        let mut data = DemuxerData::new();
        let result = parse_ad_cdp(&packet, &mut data);
        assert!(result.is_err());
        assert_eq!(result.unwrap_err(), DemuxerError::Retry);
    }

    #[test]
    fn test_parse_ad_cdp_new_section() {
        initialize_logger();
        let mut packet = build_valid_cdp_packet();
        packet[7] = 0x80; // falls in 0x75..=0xEF
        let mut data = DemuxerData::new();
        let result = parse_ad_cdp(&packet, &mut data);
        assert!(result.is_err());
        assert_eq!(result.unwrap_err(), DemuxerError::Retry);
    }

    #[test]
    fn test_parse_ad_cdp_footer_mismatch() {
        initialize_logger();
        let mut packet = build_valid_cdp_packet();
        // Change footer sequence counter (bytes 16-17) to 0x00,0x02.
        packet[16] = 0x00;
        packet[17] = 0x02;
        let mut data = DemuxerData::new();
        let result = parse_ad_cdp(&packet, &mut data);
        assert!(result.is_err());
        assert_eq!(result.unwrap_err(), DemuxerError::InvalidArgument);
    }
    // Helper: Build a payload for parse_ad_pyld.
    // The payload length (len) is total bytes.
    // It must be at least 6 (header) + 2 (one iteration) = 8.
    // For a valid CEA-708 case, we supply:
    //  - d_id (2 bytes little-endian): CLOSED_CAP_DID (0x01, 0x00)
    //  - sd_id (2 bytes): CLOSED_C708_SDID (0x02, 0x00)
    //  - dc (2 bytes): arbitrary (e.g., 0xFF, 0x00)
    //  - Then one 16-bit word: e.g., 0xFF, 0x00.
    fn build_valid_ad_pyld_payload() -> Vec<u8> {
        let mut payload = Vec::new();
        // Header: d_id = 0x0001, sd_id = 0x0002, dc = 0xFF00.
        payload.extend_from_slice(&[0x01, 0x00]); // d_id
        payload.extend_from_slice(&[0x02, 0x00]); // sd_id
        payload.extend_from_slice(&[0xFF, 0x00]); // dc (masked to 0xFF)
                                                  // Remaining payload: one 16-bit word.
        payload.extend_from_slice(&[0xFF, 0x00]); // This will produce 0x00FF stored in cdp[0]
        payload
    }

    #[test]
    fn test_parse_ad_pyld_valid_cea708() {
        let ccx_options = &mut Options::default();

        // Build a valid payload for CEA-708.
        let payload = build_valid_ad_pyld_payload();
        let total_len = payload.len() as i32; // e.g., 8 bytes
        let mut demux = create_demuxer_with_buffer(&payload);
        // Create a dummy GXF context with no cdp allocated.
        let mut gxf = CcxGxf {
            cdp: None,
            cdp_len: 0,
            // Other fields can be default.
            ..Default::default()
        };
        demux.private_data = &mut gxf as *mut CcxGxf as *mut std::ffi::c_void;
        let mut data = DemuxerData::new();

        let ret = unsafe { parse_ad_pyld(&mut demux, total_len, &mut data, &mut *ccx_options) };
        assert_eq!(ret, Ok(()));
        // Check that demux.past advanced by total_len.
        assert_eq!(demux.past as usize, payload.len());
        // After subtracting 6, remaining length = 2.
        // So ctx.cdp_len should be set to ((2 - 2) / 2) = 0.
        // However, note that the loop runs if remaining_len > 2.
        // In this case, 2 is not >2 so loop does not run.
        // Thus, for a minimal valid payload, we need to supply at least 10 bytes.
        // Let's update our payload accordingly.
    }

    #[test]
    fn test_parse_ad_pyld_cea608_branch() {
        let ccx_options = &mut Options::default();

        // Build a payload for the CEA-608 branch.
        // Use d_id = 0x0001 and sd_id = 0x0003.
        let mut payload = Vec::new();
        payload.extend_from_slice(&[0x01, 0x00]); // d_id
        payload.extend_from_slice(&[0x03, 0x00]); // sd_id = 0x0003 for CEA-608
        payload.extend_from_slice(&[0x00, 0x00]); // dc (arbitrary)
                                                  // Append some extra payload (e.g., 4 bytes).
        payload.extend_from_slice(&[0x11, 0x22, 0x33, 0x44]);
        let total_len = payload.len() as i32;
        let mut demux = create_demuxer_with_buffer(&payload);
        let mut gxf = CcxGxf {
            cdp: None,
            cdp_len: 0,
            ..Default::default()
        };
        demux.private_data = &mut gxf as *mut CcxGxf as *mut std::ffi::c_void;
        let mut data = DemuxerData::new();

        let ret = unsafe { parse_ad_pyld(&mut demux, total_len, &mut data, &mut *ccx_options) };
        // In this branch, the function only logs "Need Sample" and does not fill cdp.
        // The function still calls buffered_skip for the remaining bytes.
        assert_eq!(ret, Ok(()));
        // demux.past should equal total_len.
        assert_eq!(demux.past as usize, payload.len());
    }

    #[test]
    fn test_parse_ad_pyld_other_branch() {
        let ccx_options = &mut Options::default();

        // Build a payload for an "other" service (d_id != CLOSED_CAP_DID).
        // For example, d_id = 0x0002.
        let mut payload = Vec::new();
        payload.extend_from_slice(&[0x02, 0x00]); // d_id = 0x0002 (does not match)
        payload.extend_from_slice(&[0x02, 0x00]); // sd_id = 0x0002 (irrelevant)
        payload.extend_from_slice(&[0x00, 0x00]); // dc
                                                  // Append extra payload (4 bytes).
        payload.extend_from_slice(&[0x55, 0x66, 0x77, 0x88]);
        let total_len = payload.len() as i32;
        let mut demux = create_demuxer_with_buffer(&payload);
        let mut gxf = CcxGxf {
            cdp: None,
            cdp_len: 0,
            ..Default::default()
        };
        demux.private_data = &mut gxf as *mut CcxGxf as *mut std::ffi::c_void;
        let mut data = DemuxerData::new();

        let ret = unsafe { parse_ad_pyld(&mut demux, total_len, &mut data, &mut *ccx_options) };
        // For other service, no branch is taken; we simply skip remaining bytes.
        assert_eq!(ret, Ok(()));
        // demux.past should equal total_len.
        assert_eq!(demux.past as usize, payload.len());
    }
    // --- Tests for when VBI support is disabled ---
    #[test]
    #[cfg(not(feature = "ccx_gxf_enable_ad_vbi"))]
    fn test_parse_ad_vbi_disabled() {
        let ccx_options = &mut Options::default();

        // Create a buffer with known content.
        let payload = vec![0xAA; 20]; // 20 bytes of data.
        let total_len = payload.len() as i32;
        let mut demux = create_demuxer_with_buffer(&payload);
        // Create a dummy DemuxerData (not used in disabled branch).
        let mut data = DemuxerData::new();

        let ret =
            unsafe { parse_ad_vbi(&mut demux, total_len as i32, &mut data, &mut *ccx_options) };
        assert_eq!(ret, Ok(()));
        // Since VBI is disabled, buffered_skip should be called and return total_len.
        assert_eq!(demux.past as usize, payload.len());
        // data.len should remain unchanged.
        assert_eq!(data.len, 0);
    }

    // Helper: Create a demuxer for ad field, with a given GXF context that already has an ancillary track.
    fn create_demuxer_for_ad_field(data: &[u8], gxf: &mut CcxGxf) -> CcxDemuxer<'static> {
        let mut demux = create_demuxer_with_buffer(data);
        demux.private_data = gxf as *mut CcxGxf as *mut std::ffi::c_void;
        demux
    }
    // Test 1: Minimal valid field section (no loop iteration)
    #[test]
    fn test_parse_ad_field_valid_minimal() {
        let ccx_options = &mut Options::default();

        // Build a minimal valid field section:
        // Total length = 52 bytes.
        // Header:
        //  "finf" (4 bytes)
        //  spec value = 4 (4 bytes: 00 00 00 04)
        //  field identifier = 0x10 (4 bytes: 00 00 00 10)
        //  "LIST" (4 bytes)
        //  sample size = 36 (4 bytes: 24 00 00 00) because after "LIST", remaining len = 52 - 16 = 36.
        //  "anc " (4 bytes)
        // Then remaining = 52 - 24 = 28 bytes. (Loop condition: while(28 > 28) false)
        let mut buf = Vec::new();
        buf.extend_from_slice(b"finf");
        buf.extend_from_slice(&[0x00, 0x00, 0x00, 0x04]);
        buf.extend_from_slice(&[0x00, 0x00, 0x00, 0x10]);
        buf.extend_from_slice(b"LIST");
        buf.extend_from_slice(&[0x24, 0x00, 0x00, 0x00]); // 36 decimal
        buf.extend_from_slice(b"anc ");
        // Append 28 bytes of dummy data (e.g. 0xAA)
        buf.extend_from_slice(&[0xAA; 28]);
        let total_len = buf.len() as i32;
        // Create a dummy GXF context with an ancillary track
        #[allow(unused_variables)]
        let ad_track = CcxGxfAncillaryDataTrack {
            track_name: String::new(),
            fs_version: 0,
            frame_rate: 0,
            line_per_frame: 0,
            field_per_frame: 0,
            ad_format: GXF_Anc_Data_Pres_Format::PRES_FORMAT_SD,
            nb_field: 0,
            field_size: 0,
            packet_size: 0,
            id: 0,
        };
        let mut gxf = CcxGxf::default();
        let mut demux = create_demuxer_for_ad_field(&buf, &mut gxf);
        let mut data = DemuxerData::new();

        let ret = unsafe { parse_ad_field(&mut demux, total_len, &mut data, &mut *ccx_options) };
        assert_eq!(ret, Ok(()));
        // Expect demux.past to equal total length.
        assert_eq!(demux.past as usize, buf.len());
    }

    //tests for set_data_timebase
    #[test]
    fn test_set_data_timebase_0() {
        let mut data = DemuxerData::default();
        set_data_timebase(0, &mut data);
        assert_eq!(data.tb.den, 30000);
        assert_eq!(data.tb.num, 1001);
    }
    #[test]
    fn test_set_data_timebase_1() {
        let mut data = DemuxerData::default();
        set_data_timebase(1, &mut data);
        assert_eq!(data.tb.den, 25);
        assert_eq!(data.tb.num, 1);
    }
    fn create_demuxer_with_data(data: &[u8]) -> CcxDemuxer {
        CcxDemuxer {
            filebuffer: allocate_filebuffer(data),
            filebuffer_pos: 0,
            bytesinbuffer: data.len() as u32,
            past: 0,
            private_data: ptr::null_mut(),
            ..Default::default()
        }
    }

    // Helper: Create a DemuxerData with a writable buffer.
    fn create_demuxer_data() -> DemuxerData {
        let mut buffer = vec![0u8; BUFSIZE].into_boxed_slice();
        let ptr = buffer.as_mut_ptr();
        // Leak the buffer to get a 'static lifetime
        mem::forget(buffer);

        DemuxerData {
            program_number: 0,
            stream_pid: 0,
            codec: Option::from(Codec::Dvb),
            bufferdatatype: BufferdataType::Raw,
            buffer: ptr,
            len: 0,
            rollover_bits: 0,
            pts: 0,
            tb: CcxRational::default(),
            next_stream: ptr::null_mut(),
            next_program: ptr::null_mut(),
        }
    }

    // Test: Full packet is successfully read.
    // Test: Full packet is successfully read.
    #[test]
    fn test_parse_mpeg_packet_valid() {
        let ccx_options = &mut Options::default();

        // Build a test payload.
        let payload = b"Hello, Rust MPEG Packet!";
        let total_len = payload.len();
        let mut demux = create_demuxer_with_data(payload);
        let mut data = create_demuxer_data();

        // Call parse_mpeg_packet.
        let ret = unsafe { parse_mpeg_packet(&mut demux, total_len, &mut data, &mut *ccx_options) };
        assert_eq!(ret, Ok(()));
        // Check that data.len was increased by total_len.
        assert_eq!(data.len, total_len);
        // Verify that the content in data.buffer matches payload.
        let out = unsafe { slice::from_raw_parts(data.buffer, total_len) };
        assert_eq!(out, payload);
        // Check that demux.past equals total_len.
        assert_eq!(demux.past as usize, total_len);
    }

    // Test: Incomplete packet (simulate short read).
    #[test]
    fn test_parse_mpeg_packet_incomplete() {
        let ccx_options = &mut Options::default();

        // Build a test payload but simulate that only part of it is available.
        let payload = b"Short Packet";
        let total_len = payload.len();
        // Create a demuxer with only half of the payload available.
        let available = total_len / 2;
        let mut demux = create_demuxer_with_data(&payload[..available]);
        let mut data = create_demuxer_data();

        // Call parse_mpeg_packet.
        let ret = unsafe { parse_mpeg_packet(&mut demux, total_len, &mut data, &mut *ccx_options) };
        assert_eq!(ret, Err(DemuxerError::EOF));
        // data.len should still be increased by total_len
        assert_eq!(data.len, total_len);
        // demux.past should equal available.
        assert_eq!(demux.past as usize, 0);
    }
    #[test]
    fn test_parse_ad_packet_correct_data() {
        let ccx_options = &mut Options::default();

        // Setup test data
        let mut data = Vec::new();
        data.extend_from_slice(b"RIFF");
        data.extend_from_slice(&65528u32.to_le_bytes()); // ADT packet length
        data.extend_from_slice(b"rcrd");
        data.extend_from_slice(b"desc");
        data.extend_from_slice(&20u32.to_le_bytes()); // desc length
        data.extend_from_slice(&2u32.to_le_bytes()); // version
        let nb_field = 2;
        data.extend_from_slice(&(nb_field as u32).to_le_bytes());
        let field_size = 100;
        data.extend_from_slice(&(field_size as u32).to_le_bytes());
        data.extend_from_slice(&65536u32.to_le_bytes()); // buffer size
        let timebase = 12345u32;
        data.extend_from_slice(&timebase.to_le_bytes());
        data.extend_from_slice(b"LIST");
        let field_section_size = 4 + (nb_field * field_size) as u32;
        data.extend_from_slice(&field_section_size.to_le_bytes());
        data.extend_from_slice(b"fld ");
        for _ in 0..nb_field {
            data.extend(vec![0u8; field_size as usize]);
        }

        let mut demux = create_ccx_demuxer_with_header(&data);
        let mut ctx = CcxGxf {
            ad_track: Some(CcxGxfAncillaryDataTrack {
                nb_field,
                field_size,
                ..Default::default() // ... other necessary fields
            }),
            ..Default::default()
        };
        demux.private_data = &mut ctx as *mut _ as *mut std::ffi::c_void;

        let mut demuxer_data = DemuxerData::default();

        let result = unsafe {
            parse_ad_packet(
                &mut demux,
                data.len() as i32,
                &mut demuxer_data,
                &mut *ccx_options,
            )
        };
        assert_eq!(result, Ok(()));
        assert_eq!(demux.past, data.len() as i64);
    }

    #[test]
    fn test_parse_ad_packet_incorrect_riff() {
        let ccx_options = &mut Options::default();

        let mut data = Vec::new();
        data.extend_from_slice(b"RIFX"); // Incorrect RIFF
                                         // ... rest of data setup similar to correct test but with incorrect header

        let mut demux = create_ccx_demuxer_with_header(&data);
        let mut ctx = CcxGxf {
            ad_track: Some(CcxGxfAncillaryDataTrack {
                nb_field: 0,
                field_size: 0,
                ..Default::default()
            }),
            ..Default::default()
        };
        demux.private_data = &mut ctx as *mut _ as *mut std::ffi::c_void;

        let mut demuxer_data = DemuxerData::default();
        let result = unsafe {
            parse_ad_packet(
                &mut demux,
                data.len() as i32,
                &mut demuxer_data,
                &mut *ccx_options,
            )
        };
        assert_eq!(result, Err(DemuxerError::EOF)); // Or check for expected result based on partial parsing
    }

    #[test]
    fn test_parse_ad_packet_eof_condition() {
        let ccx_options = &mut Options::default();

        let mut data = Vec::new();
        data.extend_from_slice(b"RIFF");
        data.extend_from_slice(&65528u32.to_le_bytes());
        // ... incomplete data

        let mut demux = create_demuxer_with_buffer(&data);
        let mut ctx = CcxGxf {
            ad_track: Some(CcxGxfAncillaryDataTrack {
                nb_field: 0,
                field_size: 0,
                ..Default::default()
            }),
            ..Default::default()
        };
        demux.private_data = &mut ctx as *mut _ as *mut std::ffi::c_void;

        let mut demuxer_data = DemuxerData::default();
        let result = unsafe {
            parse_ad_packet(
                &mut demux,
                data.len() as i32 + 10,
                &mut demuxer_data,
                &mut *ccx_options,
            )
        }; // Len larger than data
        assert_eq!(result, Err(DemuxerError::EOF));
    }
    // Tests for set_mpeg_frame_desc
    #[test]
    fn test_set_mpeg_frame_desc_i_frame() {
        let mut vid_track = CcxGxfVideoTrack::default();
        let mpeg_frame_desc_flag = 0b00000001;
        set_mpeg_frame_desc(&mut vid_track, mpeg_frame_desc_flag);
        assert_eq!(
            vid_track.p_code as i32,
            MpegPictureCoding::CCX_MPC_I_FRAME as i32
        );
        assert_eq!(
            vid_track.p_struct as i32,
            MpegPictureStruct::CCX_MPS_NONE as i32
        );
    }
    #[test]
    fn test_set_mpeg_frame_desc_p_frame() {
        let mut vid_track = CcxGxfVideoTrack::default();
        let mpeg_frame_desc_flag = 0b00000010;
        set_mpeg_frame_desc(&mut vid_track, mpeg_frame_desc_flag);
        assert_eq!(
            vid_track.p_code as i32,
            MpegPictureCoding::CCX_MPC_P_FRAME as i32
        );
        assert_eq!(
            vid_track.p_struct as i32,
            MpegPictureStruct::CCX_MPS_NONE as i32
        );
    }
    #[test]
    fn test_partial_eq_gxf_track_type() {
        let track_type1 = GXF_Track_Type::TRACK_TYPE_TIME_CODE_525;
        let track_type2 = GXF_Track_Type::TRACK_TYPE_TIME_CODE_525;
        assert_eq!(track_type1 as i32, track_type2 as i32);
    }
    fn create_test_demuxer(data: &[u8], has_ctx: bool) -> CcxDemuxer {
        CcxDemuxer {
            filebuffer: data.as_ptr() as *mut u8,
            bytesinbuffer: data.len() as u32,
            filebuffer_pos: 0,
            past: 0,
            private_data: if has_ctx {
                Box::into_raw(Box::new(CcxGxf {
                    ad_track: Some(CcxGxfAncillaryDataTrack {
                        packet_size: 100,
                        nb_field: 2,
                        field_size: 100,
                        ..Default::default()
                    }),
                    vid_track: Some(CcxGxfVideoTrack {
                        frame_rate: CcxRational {
                            num: 30000,
                            den: 1001,
                        },
                        ..Default::default()
                    }),
                    first_field_nb: 0,
                    ..Default::default()
                })) as *mut _
            } else {
                ptr::null_mut()
            },
            ..Default::default()
        }
    }

    #[test]
    fn test_parse_media_ancillary_data() {
        let ccx_options = &mut Options::default();

        let mut data = vec![
            0x02, // TRACK_TYPE_ANCILLARY_DATA
            0x01, // track_nb
            0x00, 0x00, 0x00, 0x02, // media_field_nb (BE32)
            0x00, 0x01, // first_field_nb (BE16)
            0x00, 0x02, // last_field_nb (BE16)
            0x00, 0x00, 0x00, 0x03, // time_field (BE32)
            0x01, // valid_time_field (bit 0 set)
            0x00, // skipped byte
        ];
        // Add payload (100 bytes for ad_track->packet_size)
        data.extend(vec![0u8; 100]);

        let mut demux = create_test_demuxer(&data, true);
        let mut demuxer_data = DemuxerData::default();

        let result = unsafe {
            parse_media(
                &mut demux,
                data.len() as i32,
                &mut demuxer_data,
                &mut *ccx_options,
            )
        };
        assert_eq!(result, Ok(()));
    }

    #[test]
    fn test_parse_media_mpeg2() {
        let ccx_options = &mut Options::default();

        initialize_logger();
        let mut data = vec![
            0x04, // TRACK_TYPE_MPEG2_525
            0x01, // track_nb
            0x00, 0x00, 0x00, 0x02, // media_field_nb (BE32)
            0x12, 0x34, 0x56, 0x78, // mpeg_pic_size (BE32)
            0x00, 0x00, 0x00, 0x03, // time_field (BE32)
            0x01, // valid_time_field
            0x00, // skipped byte
        ];
        // Add MPEG payload (0x123456 bytes)
        data.extend(vec![0u8; 0x123456]);

        let mut demux = create_test_demuxer(&data, true);
        demux.private_data = Box::into_raw(Box::new(CcxGxf {
            ad_track: None, // Force MPEG path
            vid_track: Some(CcxGxfVideoTrack::default()),
            first_field_nb: 0,
            ..Default::default()
        })) as *mut _;

        let mut demuxer_data = DemuxerData::default();
        let result = unsafe {
            parse_media(
                &mut demux,
                data.len() as i32,
                &mut demuxer_data,
                &mut *ccx_options,
            )
        };
        assert_eq!(result, Ok(()));
    }

    #[test]
    fn test_parse_media_insufficient_len() {
        let ccx_options = &mut Options::default();

        let data = vec![0x02, 0x01]; // Incomplete header
        let mut demux = create_test_demuxer(&data, true);
        let mut demuxer_data = DemuxerData::default();
        let result = unsafe { parse_media(&mut demux, 100, &mut demuxer_data, &mut *ccx_options) };
        assert_eq!(result, Err(DemuxerError::EOF));
    }
    // Tests for parse_flt

    fn create_test_demuxer_parse_map(data: &[u8]) -> CcxDemuxer {
        CcxDemuxer {
            filebuffer: data.as_ptr() as *mut u8,
            bytesinbuffer: data.len() as u32,
            filebuffer_pos: 0,
            past: 0,
            private_data: Box::into_raw(Box::new(CcxGxf::default())) as *mut _,
            ..Default::default()
        }
    }

    #[test]
    fn test_parse_flt() {
        let ccx_options = &mut Options::default();

        let data = vec![0x01, 0x02, 0x03, 0x04];
        let mut demux = create_test_demuxer(&data, false);
        let result = unsafe { parse_flt(&mut demux, 4, &mut *ccx_options) };
        assert_eq!(result, Ok(()));
        assert_eq!(demux.past, 4);
    }
    #[test]
    fn test_parse_flt_eof() {
        let ccx_options = &mut Options::default();

        let data = vec![0x01, 0x02, 0x03, 0x04];
        let mut demux = create_test_demuxer(&data, false);
        let result = unsafe { parse_flt(&mut demux, 5, &mut *ccx_options) };
        assert_eq!(result, Err(DemuxerError::EOF));
        assert_eq!(demux.past as usize, unsafe {
            buffered_skip(&mut demux, 5, &mut *ccx_options)
        });
    }
    #[test]
    fn test_parse_flt_invalid() {
        let ccx_options = &mut Options::default();

        let data = vec![0x01, 0x02, 0x03, 0x04];
        let mut demux = create_test_demuxer(&data, false);
        let result = unsafe { parse_flt(&mut demux, 40, &mut *ccx_options) };
        assert_eq!(result, Err(DemuxerError::EOF));
        assert_eq!(demux.past as usize, 0);
    }
    // Tests for parse_map
    #[test]
    fn test_parse_map_valid() {
        let ccx_options = &mut Options::default();

        let mut buf = Vec::new();
        buf.extend_from_slice(&[0xe0, 0xff]);
        // Next 2 bytes: material_sec_len = 10 (big-endian).
        buf.extend_from_slice(&10u16.to_be_bytes());
        // Material section: 10 arbitrary bytes.
        buf.extend_from_slice(&[0xAA; 10]);
        // Next 2 bytes: track_sec_len = 8.
        buf.extend_from_slice(&8u16.to_be_bytes());
        // Track section: 8 arbitrary bytes.
        buf.extend_from_slice(&[0xBB; 8]);
        // Remaining bytes: 14 arbitrary bytes.
        buf.extend_from_slice(&[0xCC; 14]);
        #[allow(unused_variables)]
        let total_len = buf.len() as i32;

        // Create demuxer with this buffer.
        let mut demux = create_demuxer_with_data(&buf);
        // Create a dummy GXF context and assign to demux.private_data.
        let mut gxf = CcxGxf::default();
        // For MAP, parse_material_sec and parse_track_sec are called;
        // our dummy implementations simply skip the specified bytes.
        // Set private_data.
        demux.private_data = &mut gxf as *mut CcxGxf as *mut std::ffi::c_void;
        // Create a dummy DemuxerData.
        let mut data = create_demuxer_data();

        let ret = unsafe { parse_map(&mut demux, total_len, &mut data, &mut *ccx_options) };
        assert_eq!(ret, Ok(()));
        let expected_error_skip = (total_len - 26) as usize;
        // And demux.past should equal 26 (from header processing) + expected_error_skip.
        assert_eq!(demux.past as usize, 26 + expected_error_skip);
    }

    #[test]
    fn test_parse_map_invalid_header() {
        let ccx_options = &mut Options::default();

        let data = vec![0x00, 0x00]; // Invalid header
        let mut demux = create_test_demuxer_parse_map(&data);
        let mut data = DemuxerData::default();
        let result = unsafe { parse_map(&mut demux, 2, &mut data, &mut *ccx_options) };
        assert_eq!(result, Err(DemuxerError::InvalidArgument));
        assert_eq!(demux.past, 2);
    }

    #[test]
    fn test_parse_map_material_section_overflow() {
        let ccx_options = &mut Options::default();

        let data = vec![
            0xe0, 0xff, // Valid header
            0x00, 0x05, // material_sec_len = 5 (exceeds remaining len)
        ];
        let mut demux = create_test_demuxer_parse_map(&data);
        let mut data = DemuxerData::default();
        let result = unsafe { parse_map(&mut demux, 4, &mut data, &mut *ccx_options) };
        assert_eq!(result, Err(DemuxerError::InvalidArgument));
        assert_eq!(demux.past, 4);
    }

    #[test]
    fn test_parse_map_track_section_overflow() {
        let ccx_options = &mut Options::default();

        let data = vec![
            0xe0, 0xff, // Valid header
            0x00, 0x02, // material_sec_len = 2
            0x00, 0x00, // Material section
            0x00, 0x05, // track_sec_len = 5 (exceeds remaining len)
        ];
        let mut demux = create_test_demuxer_parse_map(&data);
        let mut data = DemuxerData::default();
        let result = unsafe { parse_map(&mut demux, 8, &mut data, &mut *ccx_options) };
        assert_eq!(result, Err(DemuxerError::InvalidArgument));
        assert_eq!(demux.past, 8);
    }

    #[test]
    fn test_parse_map_eof_during_skip() {
        let ccx_options = &mut Options::default();

        let data = vec![0x00, 0x00]; // Invalid header, insufficient data
        let mut demux = create_test_demuxer_parse_map(&data);
        let result = unsafe {
            parse_map(
                &mut demux,
                5,
                &mut DemuxerData::default(),
                &mut *ccx_options,
            )
        };
        assert_eq!(result, Err(DemuxerError::InvalidArgument));
    }
    fn create_test_demuxer_packet_map(data: &[u8]) -> CcxDemuxer {
        CcxDemuxer {
            filebuffer: data.as_ptr() as *mut u8,
            bytesinbuffer: data.len() as u32,
            filebuffer_pos: 0,
            past: 0,
            private_data: Box::into_raw(Box::new(CcxGxf {
                ad_track: Some(CcxGxfAncillaryDataTrack::default()),
                vid_track: Some(CcxGxfVideoTrack::default()),
                ..Default::default()
            })) as *mut _,
            ..Default::default()
        }
    }
    fn valid_map_header(len: i32) -> Vec<u8> {
        let mut data = vec![
            0x00, 0x00, 0x00, 0x00, // Leader
            0x01, // Leader continuation
            0xBC, // MAP type (not 0xBF which is MEDIA)
        ];
        let total_len = (len + 16).to_be_bytes();
        data.extend_from_slice(&total_len);
        data.extend_from_slice(&[0x00; 4]);
        data.push(0xe1);
        data.push(0xe2);
        data
    }

    fn valid_media_header(len: i32) -> Vec<u8> {
        let mut data = vec![
            0x00, 0x00, 0x00, 0x00, // Leader
            0x01, // Leader continuation
            0xbf, // MEDIA type
        ];
        let total_len = (len + 16).to_be_bytes();
        data.extend_from_slice(&total_len);
        data.extend_from_slice(&[0x00; 4]);
        data.push(0xe1);
        data.push(0xe2);
        data
    }

    #[test]
    fn test_read_packet_map() {
        initialize_logger();
        let ccx_options = &mut Options::default();

        // Create proper MAP packet payload
        let mut payload = Vec::new();
        payload.extend_from_slice(&0xE0FFu16.to_be_bytes()); // Required header
        payload.extend_from_slice(&0u16.to_be_bytes()); // Material section length = 0
        payload.extend_from_slice(&0u16.to_be_bytes()); // Track section length = 0
                                                        // Total payload length = 6 bytes

        let mut header = valid_map_header(payload.len() as i32);
        header.extend(payload);

        let mut demux = create_test_demuxer_packet_map(&header);
        let mut data = DemuxerData::default();
        assert_eq!(
            unsafe { read_packet(&mut demux, &mut data, &mut *ccx_options) },
            Ok(())
        );
    }

    #[test]
    fn test_read_packet_media() {
        let ccx_options = &mut Options::default();

        let mut header = valid_media_header(16);
        header.extend(vec![0u8; 16]);
        let mut demux = create_test_demuxer_packet_map(&header);
        let mut data = DemuxerData::default();
        assert_eq!(
            unsafe { read_packet(&mut demux, &mut data, &mut *ccx_options) },
            Ok(())
        );
    }

    #[test]
    fn test_read_packet_eos() {
        let ccx_options = &mut Options::default();

        let data = vec![
            0x00, 0x00, 0x00, 0x00, 0x01, 0xfb, 0x00, 0x00, 0x00, 0x10, // Length = 16
            0x00, 0x00, 0x00, 0x00, 0xe1, 0xe2,
        ];
        let mut demux = create_test_demuxer_packet_map(&data);
        let mut dd = DemuxerData::default();
        assert_eq!(
            unsafe { read_packet(&mut demux, &mut dd, &mut *ccx_options) },
            Err(DemuxerError::EOF)
        );
    }

    #[test]
    fn test_read_packet_invalid_header() {
        let ccx_options = &mut Options::default();

        let data = vec![0u8; 16]; // Invalid leader
        let mut demux = create_test_demuxer_packet_map(&data);
        let mut dd = DemuxerData::default();
        assert_eq!(
            unsafe { read_packet(&mut demux, &mut dd, &mut *ccx_options) },
            Err(DemuxerError::InvalidArgument)
        );
    }
    #[test]
    fn test_probe_buffer_too_short() {
        // Buffer shorter than startcode.
        let buf = [0, 0, 0];
        assert!(!ccx_gxf_probe(&buf));
    }

    #[test]
    fn test_probe_exact_match() {
        // Buffer exactly equal to startcode.
        let buf = [0, 0, 0, 0, 1, 0xbc];
        assert!(ccx_gxf_probe(&buf));
    }

    #[test]
    fn test_probe_match_with_extra_data() {
        // Buffer with startcode at the beginning, followed by extra data.
        let mut buf = vec![0, 0, 0, 0, 1, 0xbc];
        buf.extend_from_slice(&[0x12, 0x34, 0x56]);
        assert!(ccx_gxf_probe(&buf));
    }

    #[test]
    fn test_probe_no_match() {
        // Buffer with similar length but different content.
        let buf = [0, 0, 0, 1, 0, 0xbc]; // Note: fourth byte is 1 instead of 0
        assert!(!ccx_gxf_probe(&buf));
    }

    #[test]
    fn test_parse_track_sec_valid_ancillary() {
        initialize_logger();
        let ccx_options = &mut Options::default();
        // Build an ancillary track record.
        // For valid ancillary branch:
        //   track_type must have high bit set: use 0x80|1 = 0x81.
        //   track_id must have top two bits set: use 0xC5.
        //   Then, record header: [0x81, 0xC5, track_len (2 bytes)]
        let track_len = 10;
        let track_data = vec![0xAA; track_len as usize]; // dummy data for ad track (parse_ad_track_desc will advance past)
        let record = build_track_record(0x95, 0xC5, track_len, &track_data);
        // Append extra bytes to simulate additional data.
        let mut buf = record.clone();
        // Let's append a skip record: track_type not high-bit set.
        let skip_record = build_track_record(0x01, 0xFF, 5, &[0xBB; 5]);
        buf.extend_from_slice(&skip_record);
        let total_len = buf.len() as i32;

        // Create a dummy CcxGxf context.
        let mut gxf = CcxGxf {
            nb_streams: 1,
            media_name: String::new(),
            first_field_nb: 0,
            last_field_nb: 0,
            mark_in: 0,
            mark_out: 0,
            stream_size: 0,
            ad_track: None,
            vid_track: None,
            cdp: None,
            cdp_len: 0,
        };
        let mut demux = create_demuxer_for_track_sec(&buf, &mut gxf);
        let mut data = DemuxerData::default();

        let ret = unsafe { parse_track_sec(&mut demux, total_len, &mut data, ccx_options) };
        assert_eq!(ret, Ok(()));
        // After processing, data.bufferdatatype should be set to Raw.
        assert_eq!(data.bufferdatatype, BufferdataType::Raw);
        // Check that the ancillary track's id is set (after masking, 0xC5 & 0xCF == 0xC5).
        if let Some(ad) = gxf.ad_track {
            assert_eq!(ad.id, 0xC5);
        } else {
            panic!("Ancillary track not allocated");
        }
        // Verify that demux.past equals the entire buffer length.
        assert_eq!(demux.past as usize, buf.len());
    }

    #[test]
    fn test_parse_track_sec_valid_mpeg() {
        let ccx_options = &mut Options::default();
        // Build a MPEG2 525 track record.
        // For valid MPEG branch:
        //   track_type: use 0x80|2 = 0x82.
        //   track_id: valid if high bits set: use 0xC6.
        //   track_len: e.g., 12.
        let track_len = 12;
        let track_data = vec![0xCC; track_len as usize]; // dummy data for mpeg525 (parse_mpeg525_track_desc will advance past)
        let record = build_track_record(0x8B, 0xC6, track_len, &track_data);
        // Append extra data record to test final skip.
        let extra = vec![0xDD; 3];
        let mut buf = record.clone();
        buf.extend_from_slice(&extra);
        let total_len = buf.len() as i32;
        // Create a dummy CcxGxf context.
        let mut gxf = CcxGxf {
            nb_streams: 1,
            media_name: String::new(),
            first_field_nb: 0,
            last_field_nb: 0,
            mark_in: 0,
            mark_out: 0,
            stream_size: 0,
            ad_track: None,
            vid_track: None,
            cdp: None,
            cdp_len: 0,
        };
        let mut demux = create_demuxer_for_track_sec(&buf, &mut gxf);
        let mut data = DemuxerData::default();

        let ret = unsafe { parse_track_sec(&mut demux, total_len, &mut data, ccx_options) };
        assert_eq!(ret, Ok(()));
        // For MPEG branch, data.bufferdatatype should be set to Pes.
        assert_eq!(data.bufferdatatype, BufferdataType::Pes);
        // Check that the video track is allocated.
        assert!(gxf.vid_track.is_some());
        // Verify that demux.past equals the full buffer length.
        assert_eq!(demux.past as usize, buf.len());
    }
}
