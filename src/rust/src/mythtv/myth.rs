use crate::demuxer::common_structs::CcxDemuxer;
use crate::file_functions::file::{
    buffered_get_be16, buffered_get_be32, buffered_get_byte, buffered_read, buffered_read_byte,
    buffered_seek, position_sanity_check,
};
use crate::mythtv::common_structs::*;
use lib_ccxr::common::Options;
use lib_ccxr::info;
/// # Safety
/// This function is unsafe because it calls file operations that read from
/// raw file descriptors and manipulate raw pointers.
pub unsafe fn get_pts_mythtv(ctx: &mut CcxDemuxer, c: i32, ccx_options: &mut Options) -> i64 {
    let mut pts: i64;
    let mut val: i32;

    let c = if c < 0 {
        buffered_get_byte(ctx, ccx_options) as i32
    } else {
        c
    };
    pts = (((c >> 1) & 0x07) as i64) << 30;
    val = buffered_get_be16(ctx, ccx_options) as i32;
    pts |= ((val >> 1) as i64) << 15;
    val = buffered_get_be16(ctx, ccx_options) as i32;
    pts |= (val >> 1) as i64;
    pts
}

/// Scan up to `*size_ptr` bytes for the next 0x000001 start‑code prefix.
/// Updates `*header_state` and `*size_ptr` just like the C version, and
/// returns the 3‑byte start code (>=0) or –1 if none was found (or on EOF).
/// # Safety
/// This function is unsafe because it calls file operations in `buffered_read_byte` that read from
/// raw file descriptors and manipulate raw pointers.
pub unsafe fn find_next_start_code(
    ctx: &mut CcxDemuxer,
    size_ptr: &mut i32,
    header_state: &mut u32,
    ccx_options: &mut Options,
) -> i32 {
    let mut state = *header_state;
    let mut n = *size_ptr;
    let mut val: i32 = -1;

    while n > 0 {
        // read one raw byte exactly like the C helper
        let mut cx: u8 = 0;
        let bytes_read = buffered_read_byte(ctx, Some(&mut cx), ccx_options);
        if bytes_read != 1 {
            // EOF or error → give up
            break;
        }

        ctx.past += 1;
        let v = cx as u32;
        n -= 1;

        if state == 0x000001 {
            // we had already seen 0x00 00 01 in the previous bytes,
            // so this byte `v` completes the 4‑byte code
            state = ((state << 8) | v) & 0x00FF_FFFF;
            val = state as i32;
            break;
        }

        // roll the 24‑bit window
        state = ((state << 8) | v) & 0x00FF_FFFF;
    }

    *header_state = state;
    *size_ptr = n;
    val
}

/// # Safety
/// This function is unsafe because it calls file operations in `buffered_seek` that read from
/// raw file descriptors and manipulate raw pointers.
pub unsafe fn url_fskip(ctx: &mut CcxDemuxer, length: i32, ccx_options: &mut Options) {
    // Seek forward by `length` bytes
    buffered_seek(ctx, length, ccx_options);
    ctx.past += length as i64;
}

/// # Safety
/// This function is unsafe because it calls file operations that read from
/// raw file descriptors and manipulate raw pointers.
pub unsafe fn mpegps_psm_parse(
    ctx: &mut CcxDemuxer,
    psm_es_type: &mut [u8; 256],
    ccx_options: &mut Options,
) -> i64 {
    let psm_length = buffered_get_be16(ctx, ccx_options) as i32;

    // Skip two bytes
    buffered_get_byte(ctx, ccx_options);
    buffered_get_byte(ctx, ccx_options);

    let ps_info_length = buffered_get_be16(ctx, ccx_options) as i32;

    // Skip program_stream_info
    url_fskip(ctx, ps_info_length, ccx_options);

    let mut es_map_length = buffered_get_be16(ctx, ccx_options) as i32;

    // At least one es available?
    while es_map_length >= 4 {
        let stream_type = buffered_get_byte(ctx, ccx_options);
        let es_id = buffered_get_byte(ctx, ccx_options);
        let es_info_length = buffered_get_be16(ctx, ccx_options) as u32;

        // Remember mapping from stream id to stream type
        psm_es_type[es_id as usize] = stream_type;

        // Skip program_stream_info
        url_fskip(ctx, es_info_length as i32, ccx_options);

        es_map_length -= 4 + es_info_length as i32;
    }

    // Read and discard CRC32
    buffered_get_be32(ctx, ccx_options);

    (2 + psm_length) as i64
}
/// # Safety
/// This function is unsafe because it calls file operations in `buffered_get_be16` and `buffered_get_byte` that read from
/// raw file descriptors and manipulate raw pointers.
pub unsafe fn mpegps_read_pes_header(
    ctx: &mut CcxDemuxer,
    pstart_code: &mut i32,
    ppts: &mut i64,
    pdts: &mut i64,
    header_state: &mut u32,
    psm_es_type: &mut [u8; 256],
    ccx_options: &mut Options,
) -> i32 {
    let mut len: i32;
    let mut size: i32;
    let mut startcode: i32;
    let mut c: i32 = 0;
    let mut flags: i32;
    let mut header_len: i32;
    let mut pts: i64;
    let mut dts: i64;

    loop {
        // Next start code (should be immediately after)
        *header_state = 0xff;
        size = MAX_SYNC_SIZE as i32;
        startcode = find_next_start_code(ctx, &mut size, header_state, ccx_options);
        if startcode < 0 {
            return AVERROR_IO as i32;
        }
        if startcode == PACK_START_CODE as i32 {
            continue;
        }
        if startcode == SYSTEM_HEADER_START_CODE as i32 {
            continue;
        }
        if startcode == Mpeg2StreamTypeMythTv::PaddingStream as i32
            || startcode == Mpeg2StreamTypeMythTv::PrivateStream2 as i32
        {
            // Skip them
            buffered_get_be16(ctx, ccx_options);
            continue;
        }
        position_sanity_check(ctx);

        if startcode == Mpeg2StreamTypeMythTv::ProgramStreamMap as i32 {
            mpegps_psm_parse(ctx, psm_es_type, ccx_options);
            continue;
        }
        // Find matching stream
        if !((0x1c0..=0x1df).contains(&startcode)
            || (0x1e0..=0x1ef).contains(&startcode)
            || (startcode == 0x1bd))
        {
            continue;
        }
        len = buffered_get_be16(ctx, ccx_options) as i32;
        pts = AV_NOPTS_VALUE;
        dts = AV_NOPTS_VALUE;
        position_sanity_check(ctx);
        // Stuffing
        loop {
            if len < 1 {
                break; // This will break inner loop and continue outer loop
            }
            c = buffered_get_byte(ctx, ccx_options) as i32;
            len -= 1;
            // XXX: for mpeg1, should test only bit 7
            if c != 0xff {
                break;
            }
        }
        if len < 1 {
            continue; // goto redo
        }
        position_sanity_check(ctx);

        if (c & 0xc0) == 0x40 {
            // Buffer scale & size
            if len < 2 {
                continue;
            }
            buffered_get_byte(ctx, ccx_options);
            c = buffered_get_byte(ctx, ccx_options) as i32;
            len -= 2;
        }
        position_sanity_check(ctx);

        if (c & 0xf0) == 0x20 {
            if len < 4 {
                continue;
            }
            pts = get_pts_mythtv(ctx, c, ccx_options);
            dts = pts;
            len -= 4;
        } else if (c & 0xf0) == 0x30 {
            if len < 9 {
                continue;
            }
            pts = get_pts_mythtv(ctx, c, ccx_options);
            dts = get_pts_mythtv(ctx, -1, ccx_options);
            len -= 9;
        } else if (c & 0xc0) == 0x80 {
            // MPEG 2 PES
            flags = buffered_get_byte(ctx, ccx_options) as i32;
            header_len = buffered_get_byte(ctx, ccx_options) as i32;
            len -= 2;

            if header_len > len {
                continue;
            }

            if (flags & 0xc0) == 0x80 {
                pts = get_pts_mythtv(ctx, -1, ccx_options);
                dts = pts;
                if header_len < 5 {
                    continue;
                }
                header_len -= 5;
                len -= 5;
            }

            if (flags & 0xc0) == 0xc0 {
                pts = get_pts_mythtv(ctx, -1, ccx_options);
                dts = get_pts_mythtv(ctx, -1, ccx_options);
                if header_len < 10 {
                    continue;
                }
                header_len -= 10;
                len -= 10;
            }

            len -= header_len;
            while header_len > 0 {
                buffered_get_byte(ctx, ccx_options);
                header_len -= 1;
            }
        } else if c != 0xf {
            continue;
        }
        position_sanity_check(ctx);
        if startcode == Mpeg2StreamTypeMythTv::PrivateStream1 as i32 {
            if len < 1 {
                continue;
            }
            startcode = buffered_get_byte(ctx, ccx_options) as i32;
            len -= 1;

            if (0x80..=0xbf).contains(&startcode) {
                // Audio: skip header
                if len < 3 {
                    continue;
                }
                buffered_get_byte(ctx, ccx_options);
                buffered_get_byte(ctx, ccx_options);
                buffered_get_byte(ctx, ccx_options);
                len -= 3;
            }
        }
        *pstart_code = startcode;
        *ppts = pts;
        *pdts = dts;
        return len;
    }
}
/// # Safety
/// This function is unsafe because it calls file operations that read from
/// raw file descriptors and manipulate raw pointers.
pub unsafe fn mpegps_read_packet(
    ctx: &mut CcxDemuxer,
    header_state: &mut u32,
    psm_es_type: &mut [u8; 256],
    ccx_options: &mut Options,
    av: &mut AVPacketMythTv,
) -> i32 {
    let mut pts: i64 = 0;
    let mut dts: i64 = 0;
    let mut len: i32;
    let mut startcode: i32 = 0;
    let mut codec_id: CodecIDMythTv;
    let mut es_type: Option<StreamTypeMythTv>;
    let mut type_: CodecTypeMythTv;

    'redo: loop {
        // 1. Read PES header
        len = mpegps_read_pes_header(
            ctx,
            &mut startcode,
            &mut pts,
            &mut dts,
            header_state,
            psm_es_type,
            ccx_options,
        );
        if len < 0 {
            return len;
        }
        position_sanity_check(ctx);

        // 2. Determine stream type
        es_type = StreamTypeMythTv::from_c(psm_es_type[(startcode & 0xff) as usize]);
        if es_type.is_some() {
            match es_type.unwrap() {
                x if x == StreamTypeMythTv::VideoMpeg1 || x == StreamTypeMythTv::VideoMpeg2 => {
                    codec_id = CodecIDMythTv::Mpeg2Video;
                    type_ = CodecTypeMythTv::Video;
                }
                x if x == StreamTypeMythTv::AudioMpeg1 || x == StreamTypeMythTv::AudioMpeg2 => {
                    codec_id = CodecIDMythTv::Mp3;
                    type_ = CodecTypeMythTv::Audio;
                }
                StreamTypeMythTv::AudioAAC => {
                    codec_id = CodecIDMythTv::Aac;
                    type_ = CodecTypeMythTv::Audio;
                }
                StreamTypeMythTv::VideoMpeg4 => {
                    codec_id = CodecIDMythTv::Mpeg4;
                    type_ = CodecTypeMythTv::Video;
                }
                StreamTypeMythTv::VideoH264 => {
                    codec_id = CodecIDMythTv::H264;
                    type_ = CodecTypeMythTv::Video;
                }
                StreamTypeMythTv::AudioAC3 => {
                    codec_id = CodecIDMythTv::Ac3;
                    type_ = CodecTypeMythTv::Audio;
                }
                _ => {
                    url_fskip(ctx, len, ccx_options);
                    continue 'redo;
                }
            }
        } else if (0x1e0..=0x1ef).contains(&startcode) {
            // Peek 8 bytes to distinguish CAVS vs. MPEG2
            let mut buf = [0u8; 8];
            buffered_read(ctx, Some(&mut buf), 8, ccx_options);
            ctx.past += 8;
            buffered_seek(ctx, -8, ccx_options);
            ctx.past -= 8;
            codec_id = if buf[0..4] == AVS_SEQH && (buf[6] != 0 || buf[7] != 1) {
                CodecIDMythTv::Cavs
            } else {
                CodecIDMythTv::Mpeg2Video
            };
            type_ = CodecTypeMythTv::Video;
        } else if (0x1c0..=0x1df).contains(&startcode) {
            codec_id = CodecIDMythTv::Mp2;
            type_ = CodecTypeMythTv::Audio;
        } else if (0x80..=0x87).contains(&startcode) {
            codec_id = CodecIDMythTv::Ac3;
            type_ = CodecTypeMythTv::Audio;
        } else if (0x88..=0x9f).contains(&startcode) {
            codec_id = CodecIDMythTv::Dts;
            type_ = CodecTypeMythTv::Audio;
        } else if (0xa0..=0xbf).contains(&startcode) {
            codec_id = CodecIDMythTv::PcmS16be;
            type_ = CodecTypeMythTv::Audio;
        } else if (0x20..=0x3f).contains(&startcode) {
            codec_id = CodecIDMythTv::DvdSubtitle;
            type_ = CodecTypeMythTv::Subtitle;
        } else if startcode == 0x69 || startcode == 0x49 {
            codec_id = CodecIDMythTv::Mpeg2vbi;
            type_ = CodecTypeMythTv::Data;
        } else {
            url_fskip(ctx, len, ccx_options);
            continue 'redo;
        }

        // 3. For LPCM streams, skip 3‑byte header
        if (0xa0..=0xbf).contains(&startcode) {
            if len <= 3 {
                url_fskip(ctx, len, ccx_options);
                continue 'redo;
            }
            buffered_get_byte(ctx, ccx_options);
            buffered_get_byte(ctx, ccx_options);
            buffered_get_byte(ctx, ccx_options);
            len -= 3;
        }

        // 4. Read payload into a growable Vec<u8> buffer
        av.size = len as usize;
        // Use Vec<u8> for data buffer
        av.data
            .try_reserve_exact(av.size)
            .map_err(|_| {
                info!(
                    "MythTV: Failed to reserve {} bytes for AVPacketMythTv data",
                    av.size
                );
            })
            .expect("MythTV: Failed to reserve memory for AVPacketMythTv data");
        unsafe {
            av.data.set_len(av.size);
        }
        let buffer_slice = &mut av.data[..av.size];
        buffered_read(ctx, Some(buffer_slice), av.size, ccx_options);
        ctx.past += av.size as i64;

        // 5. Finalize packet
        position_sanity_check(ctx);
        av.codec_id = codec_id;
        av.ty = type_;
        av.pts = pts;
        av.dts = dts;

        return 0;
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    use crate::demuxer::common_structs::CcxDemuxer;
    use lib_ccxr::common::DataSource;
    use lib_ccxr::common::Options;
    use std::ptr;
    const FILEBUFFERSIZE: usize = 1024 * 1024 * 16;

    // Test helper functions
    fn create_ccx_demuxer_with_buffer() -> CcxDemuxer<'static> {
        let filebuffer = unsafe {
            let layout = std::alloc::Layout::from_size_align(FILEBUFFERSIZE, 1).unwrap();
            let ptr = std::alloc::alloc(layout);
            if ptr.is_null() {
                panic!("Failed to allocate memory for filebuffer");
            }
            ptr::write_bytes(ptr, 0, FILEBUFFERSIZE);
            ptr
        };

        CcxDemuxer {
            filebuffer,
            bytesinbuffer: 0,
            filebuffer_pos: 0,
            startbytes_pos: 0,
            past: 0,
            ..Default::default()
        }
    }

    fn create_test_options() -> Options {
        Options {
            gui_mode_reports: false,
            input_source: DataSource::File,
            ..Default::default()
        }
    }

    fn fill_buffer_with_test_data(ctx: &mut CcxDemuxer, data: &[u8]) {
        unsafe {
            let copy_len = data.len().min(FILEBUFFERSIZE);
            ptr::copy_nonoverlapping(data.as_ptr(), ctx.filebuffer, copy_len);
            ctx.bytesinbuffer = copy_len as u32;
            ctx.filebuffer_pos = 0;
        }
    }

    fn cleanup_demuxer(ctx: &mut CcxDemuxer) {
        unsafe {
            let layout = std::alloc::Layout::from_size_align(FILEBUFFERSIZE, 1).unwrap();
            std::alloc::dealloc(ctx.filebuffer, layout);
        }
    }

    // Tests for get_pts_mythtv function
    #[test]
    fn test_get_pts_mythtv_with_positive_c() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();

        // Create test data for PTS extraction
        let test_data = [
            0x21, 0x00, 0x01, // First 16-bit value: 0x2100, right-shifted becomes 0x1080
            0x00, 0x01, // Second 16-bit value: 0x0001, right-shifted becomes 0x0000
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        unsafe {
            let pts = get_pts_mythtv(&mut ctx, 0x21, &mut options);
            // PTS should be constructed from the bit manipulation
            // c=0x21: ((0x21 >> 1) & 0x07) << 30 = (0x10 & 0x07) << 30 = 0x00 << 30 = 0
            // Plus the two 16-bit values processed
            assert!(pts >= 0, "PTS should be non-negative");
        }

        cleanup_demuxer(&mut ctx);
    }

    #[test]
    fn test_get_pts_mythtv_with_negative_c() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();

        let test_data = [
            0x21, // First byte to be read when c < 0
            0x00, 0x01, // First 16-bit value
            0x00, 0x02, // Second 16-bit value
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        unsafe {
            let pts = get_pts_mythtv(&mut ctx, -1, &mut options);
            assert!(pts >= 0, "PTS should be non-negative");
        }

        cleanup_demuxer(&mut ctx);
    }

    // Tests for find_next_start_code function
    #[test]
    fn test_find_next_start_code_found() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();

        // Create data with a start code (0x000001XX)
        let test_data = [
            0x00, 0x00, 0x01, 0xB3, // Start code 0x000001B3
            0x12, 0x34, 0x56, 0x78, // Additional data
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut size = test_data.len() as i32;
        let mut header_state = 0u32;

        unsafe {
            let result = find_next_start_code(&mut ctx, &mut size, &mut header_state, &mut options);
            assert_eq!(result, 0x0001B3, "Should find the start code 0x000001B3");
            assert!(size >= 0, "Size should be non-negative");
        }

        cleanup_demuxer(&mut ctx);
    }

    #[test]
    fn test_find_next_start_code_not_found() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();

        // Create data without a complete start code
        let test_data = [0x12, 0x34, 0x56, 0x78];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut size = test_data.len() as i32;
        let mut header_state = 0u32;

        unsafe {
            let result = find_next_start_code(&mut ctx, &mut size, &mut header_state, &mut options);
            assert_eq!(result, -1, "Should return -1 when no start code is found");
        }

        cleanup_demuxer(&mut ctx);
    }

    #[test]
    fn test_find_next_start_code_partial_match() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();

        // Create data with partial start code at the end
        let test_data = [0x12, 0x34, 0x00, 0x00];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut size = test_data.len() as i32;
        let mut header_state = 0u32;

        unsafe {
            let result = find_next_start_code(&mut ctx, &mut size, &mut header_state, &mut options);
            assert_eq!(result, -1, "Should return -1 for incomplete start code");
            // Header state should preserve the partial match
            assert_eq!(
                header_state & 0xFFFF,
                0x0000,
                "Header state should contain partial match"
            );
        }

        cleanup_demuxer(&mut ctx);
    }

    // Tests for mpegps_psm_parse function
    #[test]
    fn test_mpegps_psm_parse_basic() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();
        let mut psm_es_type = [0u8; 256];

        // Create a minimal PSM structure
        let test_data = [
            0x00, 0x10, // PSM length (16 bytes)
            0x00, 0x00, // Reserved bytes
            0x00, 0x00, // PS info length (0)
            0x00, 0x04, // ES map length (4 bytes - one entry)
            0x02, // Stream type (MPEG-2 video)
            0xE0, // ES ID (0xE0)
            0x00, 0x00, // ES info length (0)
            0x00, 0x00, 0x00, 0x00, // CRC32
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        unsafe {
            let bytes_consumed = mpegps_psm_parse(&mut ctx, &mut psm_es_type, &mut options);
            assert_eq!(bytes_consumed, 18, "Should consume 2 + PSM length bytes");
            assert_eq!(
                psm_es_type[0xE0], 0x02,
                "Should map ES ID 0xE0 to stream type 0x02"
            );
        }

        cleanup_demuxer(&mut ctx);
    }

    #[test]
    fn test_mpegps_psm_parse_multiple_streams() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();
        let mut psm_es_type = [0u8; 256];

        // Create PSM with multiple stream mappings
        let test_data = [
            0x00, 0x18, // PSM length (24 bytes)
            0x00, 0x00, // Reserved bytes
            0x00, 0x00, // PS info length (0)
            0x00, 0x0C, // ES map length (12 bytes - three entries)
            0x02, 0xE0, 0x00, 0x00, // Stream type 0x02, ES ID 0xE0, no info
            0x03, 0xC0, 0x00, 0x00, // Stream type 0x03, ES ID 0xC0, no info
            0x04, 0xBD, 0x00, 0x00, // Stream type 0x04, ES ID 0xBD, no info
            0x00, 0x00, 0x00, 0x00, // CRC32
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        unsafe {
            let bytes_consumed = mpegps_psm_parse(&mut ctx, &mut psm_es_type, &mut options);
            assert_eq!(bytes_consumed, 26, "Should consume 2 + PSM length bytes");
            assert_eq!(
                psm_es_type[0xE0], 0x02,
                "Should map ES ID 0xE0 to stream type 0x02"
            );
            assert_eq!(
                psm_es_type[0xC0], 0x03,
                "Should map ES ID 0xC0 to stream type 0x03"
            );
            assert_eq!(
                psm_es_type[0xBD], 0x04,
                "Should map ES ID 0xBD to stream type 0x04"
            );
        }

        cleanup_demuxer(&mut ctx);
    }

    // Tests for mpegps_read_pes_header function
    #[test]
    fn test_mpegps_read_pes_header_video_stream() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();
        let mut psm_es_type = [0u8; 256];

        // Create a minimal video PES header
        let test_data = [
            0x00, 0x00, 0x01, 0xE0, // Video stream start code
            0x00, 0x08, // PES packet length
            0x21, 0x00, 0x01, 0x00, 0x01, // PTS data
            0x00, 0x00, 0x00, // Padding to match packet length
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut start_code = 0i32;
        let mut pts = 0i64;
        let mut dts = 0i64;
        let mut header_state = 0u32;

        unsafe {
            let payload_len = mpegps_read_pes_header(
                &mut ctx,
                &mut start_code,
                &mut pts,
                &mut dts,
                &mut header_state,
                &mut psm_es_type,
                &mut options,
            );

            assert!(payload_len >= 0, "Should successfully read PES header");
            assert_eq!(start_code, 0x1E0, "Should identify video stream start code");
            // PTS should be extracted from the header
            assert_ne!(pts, AV_NOPTS_VALUE, "Should extract valid PTS");
        }

        cleanup_demuxer(&mut ctx);
    }

    #[test]
    fn test_mpegps_read_pes_header_audio_stream() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();
        let mut psm_es_type = [0u8; 256];

        // Create a minimal audio PES header
        let test_data = [
            0x00, 0x00, 0x01, 0xC0, // Audio stream start code
            0x00, 0x05, // PES packet length
            0x0F, // No PTS/DTS
            0x00, 0x00, // Padding
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut start_code = 0i32;
        let mut pts = 0i64;
        let mut dts = 0i64;
        let mut header_state = 0u32;

        unsafe {
            let payload_len = mpegps_read_pes_header(
                &mut ctx,
                &mut start_code,
                &mut pts,
                &mut dts,
                &mut header_state,
                &mut psm_es_type,
                &mut options,
            );

            assert!(payload_len >= 0, "Should successfully read PES header");
            assert_eq!(start_code, 0x1C0, "Should identify audio stream start code");
        }

        cleanup_demuxer(&mut ctx);
    }

    // Tests for mpegps_read_packet function
    #[test]
    fn test_mpegps_read_packet_video() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();
        let mut psm_es_type = [0u8; 256];
        let mut av_packet = AVPacketMythTv {
            data: Vec::new(),
            size: 0,
            pts: AV_NOPTS_VALUE,
            dts: AV_NOPTS_VALUE,
            ..Default::default()
        };

        // Create a complete video packet
        let test_data = [
            0x00, 0x00, 0x01, 0xE0, // Video stream start code
            0x00, 0x08, // PES packet length
            0x21, 0x00, 0x01, 0x00, 0x01, // PTS data
            0xAA, 0xBB, 0xCC, // Payload data
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut header_state = 0u32;

        unsafe {
            let result = mpegps_read_packet(
                &mut ctx,
                &mut header_state,
                &mut psm_es_type,
                &mut options,
                &mut av_packet,
            );

            assert_eq!(result, 0, "Should successfully read packet");
            assert_eq!(
                av_packet.codec_id,
                CodecIDMythTv::Mpeg2Video,
                "Should identify MPEG-2 video codec"
            );
            assert_eq!(
                av_packet.ty,
                CodecTypeMythTv::Video,
                "Should identify video type"
            );
            assert!(av_packet.size > 0, "Should have payload data");
            assert_ne!(av_packet.pts, AV_NOPTS_VALUE, "Should have valid PTS");
        }

        cleanup_demuxer(&mut ctx);
    }

    #[test]
    fn test_mpegps_read_packet_audio() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();
        let mut psm_es_type = [0u8; 256];
        let mut av_packet = AVPacketMythTv {
            data: Vec::new(),
            size: 0,
            pts: AV_NOPTS_VALUE,
            dts: AV_NOPTS_VALUE,
            ..Default::default()
        };

        // Create a complete audio packet
        let test_data = [
            0x00, 0x00, 0x01, 0xC0, // Audio stream start code
            0x00, 0x06, // PES packet length
            0x0F, // No PTS/DTS
            0x11, 0x22, 0x33, 0x44, 0x55, // Payload data
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut header_state = 0u32;

        unsafe {
            let result = mpegps_read_packet(
                &mut ctx,
                &mut header_state,
                &mut psm_es_type,
                &mut options,
                &mut av_packet,
            );

            assert_eq!(result, 0, "Should successfully read packet");
            assert_eq!(
                av_packet.codec_id,
                CodecIDMythTv::Mp2,
                "Should identify MP2 audio codec"
            );
            assert_eq!(
                av_packet.ty,
                CodecTypeMythTv::Audio,
                "Should identify audio type"
            );
            assert!(av_packet.size > 0, "Should have payload data");
        }

        cleanup_demuxer(&mut ctx);
    }

    #[test]
    fn test_mpegps_read_packet_cavs_detection() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();
        let mut psm_es_type = [0u8; 256];
        let mut av_packet = AVPacketMythTv {
            data: Vec::new(),
            size: 0,
            pts: AV_NOPTS_VALUE,
            dts: AV_NOPTS_VALUE,
            ..Default::default()
        };

        // Create a packet with CAVS sequence header
        let test_data = [
            0x00, 0x00, 0x01, 0xE0, // Video stream start code
            0x00, 0x0C, // PES packet length
            0x0F, // No PTS/DTS
            0x00, 0x00, 0x01, 0xB0, // CAVS sequence header
            0x00, 0x00, 0x00, 0x02, // Additional CAVS data
            0xDD, 0xEE, 0xFF, // More payload
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut header_state = 0u32;

        unsafe {
            let result = mpegps_read_packet(
                &mut ctx,
                &mut header_state,
                &mut psm_es_type,
                &mut options,
                &mut av_packet,
            );

            assert_eq!(result, 0, "Should successfully read packet");
            assert_eq!(
                av_packet.codec_id,
                CodecIDMythTv::Cavs,
                "Should identify CAVS codec"
            );
            assert_eq!(
                av_packet.ty,
                CodecTypeMythTv::Video,
                "Should identify video type"
            );
        }

        cleanup_demuxer(&mut ctx);
    }

    #[test]
    fn test_mpegps_read_packet_ac3_audio() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();
        let mut psm_es_type = [0u8; 256];
        let mut av_packet = AVPacketMythTv {
            data: Vec::new(),
            size: 0,
            pts: AV_NOPTS_VALUE,
            dts: AV_NOPTS_VALUE,
            ..Default::default()
        };

        // Create a packet with AC3 audio (private stream with AC3 substream)
        let test_data = [
            0x00, 0x00, 0x01, 0xBD, // Private stream 1 start code
            0x00, 0x08, // PES packet length
            0x0F, // No PTS/DTS
            0x80, // AC3 substream ID
            0x00, 0x00, 0x00, // AC3 header (3 bytes)
            0x77, 0x88, // AC3 payload
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut header_state = 0u32;

        unsafe {
            let result = mpegps_read_packet(
                &mut ctx,
                &mut header_state,
                &mut psm_es_type,
                &mut options,
                &mut av_packet,
            );

            assert_eq!(result, 0, "Should successfully read packet");
            assert_eq!(
                av_packet.codec_id,
                CodecIDMythTv::Ac3,
                "Should identify AC3 codec"
            );
            assert_eq!(
                av_packet.ty,
                CodecTypeMythTv::Audio,
                "Should identify audio type"
            );
        }

        cleanup_demuxer(&mut ctx);
    }

    #[test]
    fn test_mpegps_read_packet_lpcm_audio() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();
        let mut psm_es_type = [0u8; 256];
        let mut av_packet = AVPacketMythTv {
            data: Vec::new(),
            size: 0,
            pts: AV_NOPTS_VALUE,
            dts: AV_NOPTS_VALUE,
            ..Default::default()
        };

        // Create a packet with LPCM audio
        let test_data = [
            0x00, 0x00, 0x01, 0xBD, // Private stream 1 start code
            0x00, 0x0A, // PES packet length
            0x0F, // No PTS/DTS
            0xA0, // LPCM substream ID
            0x00, 0x00, 0x00, // LPCM header (3 bytes)
            0x11, 0x22, 0x33, 0x44, 0x55, // LPCM payload
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut header_state = 0u32;

        unsafe {
            let result = mpegps_read_packet(
                &mut ctx,
                &mut header_state,
                &mut psm_es_type,
                &mut options,
                &mut av_packet,
            );

            assert_eq!(result, 0, "Should successfully read packet");
            assert_eq!(
                av_packet.codec_id,
                CodecIDMythTv::PcmS16be,
                "Should identify PCM codec"
            );
            assert_eq!(
                av_packet.ty,
                CodecTypeMythTv::Audio,
                "Should identify audio type"
            );
        }

        cleanup_demuxer(&mut ctx);
    }

    #[test]
    fn test_mpegps_read_packet_dvd_subtitle() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();
        let mut psm_es_type = [0u8; 256];
        let mut av_packet = AVPacketMythTv {
            data: Vec::new(),
            size: 0,
            pts: AV_NOPTS_VALUE,
            dts: AV_NOPTS_VALUE,
            ..Default::default()
        };

        // Create a packet with DVD subtitle
        let test_data = [
            0x00, 0x00, 0x01, 0xBD, // Private stream 1 start code
            0x00, 0x08, // PES packet length
            0x0F, // No PTS/DTS
            0x20, // DVD subtitle substream ID
            0x00, 0x00, 0x00, // Subtitle header
            0xAA, 0xBB, // Subtitle payload
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut header_state = 0u32;

        unsafe {
            let result = mpegps_read_packet(
                &mut ctx,
                &mut header_state,
                &mut psm_es_type,
                &mut options,
                &mut av_packet,
            );

            assert_eq!(result, 0, "Should successfully read packet");
            assert_eq!(
                av_packet.codec_id,
                CodecIDMythTv::DvdSubtitle,
                "Should identify DVD subtitle codec"
            );
            assert_eq!(
                av_packet.ty,
                CodecTypeMythTv::Subtitle,
                "Should identify subtitle type"
            );
        }

        cleanup_demuxer(&mut ctx);
    }

    #[test]
    fn test_mpegps_read_packet_with_psm_mapping() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();
        let mut psm_es_type = [0u8; 256];
        let mut av_packet = AVPacketMythTv {
            data: Vec::new(),
            size: 0,
            pts: AV_NOPTS_VALUE,
            dts: AV_NOPTS_VALUE,
            ..Default::default()
        };

        // Set up PSM mapping for H.264
        psm_es_type[0xE0] = StreamTypeMythTv::VideoH264 as u8;

        // Create a video packet that should be identified as H.264 via PSM
        let test_data = [
            0x00, 0x00, 0x01, 0xE0, // Video stream start code
            0x00, 0x08, // PES packet length
            0x0F, // No PTS/DTS
            0x00, 0x00, 0x01, 0x67, // H.264 NAL unit
            0x11, 0x22, 0x33, // H.264 payload
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut header_state = 0u32;

        unsafe {
            let result = mpegps_read_packet(
                &mut ctx,
                &mut header_state,
                &mut psm_es_type,
                &mut options,
                &mut av_packet,
            );

            assert_eq!(result, 0, "Should successfully read packet");
            assert_eq!(
                av_packet.codec_id,
                CodecIDMythTv::H264,
                "Should identify H.264 codec via PSM"
            );
            assert_eq!(
                av_packet.ty,
                CodecTypeMythTv::Video,
                "Should identify video type"
            );
        }

        cleanup_demuxer(&mut ctx);
    }

    // Tests for url_fskip function
    #[test]
    fn test_url_fskip_basic() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();

        let test_data = [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let initial_past = ctx.past;

        unsafe {
            url_fskip(&mut ctx, 4, &mut options);
            assert_eq!(ctx.past, initial_past + 4, "Should update past counter");
        }

        cleanup_demuxer(&mut ctx);
    }

    // Error handling tests
    #[test]
    fn test_mpegps_read_packet_insufficient_data() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();
        let mut psm_es_type = [0u8; 256];
        let mut av_packet = AVPacketMythTv {
            data: Vec::new(),
            size: 0,
            pts: AV_NOPTS_VALUE,
            dts: AV_NOPTS_VALUE,
            ..Default::default()
        };

        // Create insufficient data (incomplete header)
        let test_data = [0x00, 0x00, 0x01]; // Incomplete start code
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut header_state = 0u32;

        unsafe {
            let result = mpegps_read_packet(
                &mut ctx,
                &mut header_state,
                &mut psm_es_type,
                &mut options,
                &mut av_packet,
            );

            assert_eq!(
                result, AVERROR_IO as i32,
                "Should return error for insufficient data"
            );
        }

        cleanup_demuxer(&mut ctx);
    }
    // Additional tests for mpegps_read_packet function - comprehensive coverage

    #[test]
    fn test_mpegps_read_packet_dts_audio() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();
        let mut psm_es_type = [0u8; 256];
        let mut av_packet = AVPacketMythTv {
            data: Vec::new(),
            size: 0,
            pts: AV_NOPTS_VALUE,
            dts: AV_NOPTS_VALUE,
            ..Default::default()
        };

        // Create a packet with DTS audio (private stream with DTS substream)
        let test_data = [
            0x00, 0x00, 0x01, 0xBD, // Private stream 1 start code
            0x00, 0x08, // PES packet length
            0x0F, // No PTS/DTS
            0x88, // DTS substream ID (0x88-0x9F range)
            0x00, 0x00, 0x00, // DTS header (3 bytes)
            0x7F, 0xFE, // DTS payload
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut header_state = 0u32;

        unsafe {
            let result = mpegps_read_packet(
                &mut ctx,
                &mut header_state,
                &mut psm_es_type,
                &mut options,
                &mut av_packet,
            );

            assert_eq!(result, 0, "Should successfully read DTS packet");
            assert_eq!(
                av_packet.codec_id,
                CodecIDMythTv::Dts,
                "Should identify DTS codec"
            );
            assert_eq!(
                av_packet.ty,
                CodecTypeMythTv::Audio,
                "Should identify audio type"
            );
        }

        cleanup_demuxer(&mut ctx);
    }

    #[test]
    fn test_mpegps_read_packet_mpeg2vbi_data() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();
        let mut psm_es_type = [0u8; 256];
        let mut av_packet = AVPacketMythTv {
            data: Vec::new(),
            size: 0,
            pts: AV_NOPTS_VALUE,
            dts: AV_NOPTS_VALUE,
            ..Default::default()
        };

        // Create a packet with MPEG2 VBI data
        let test_data = [
            0x00, 0x00, 0x01, 0xBD, // Private stream 1 start code
            0x00, 0x08, // PES packet length
            0x0F, // No PTS/DTS
            0x69, // MPEG2 VBI substream ID
            0x00, 0x00, 0x00, // VBI header (3 bytes)
            0x12, 0x34, // VBI payload
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut header_state = 0u32;

        unsafe {
            let result = mpegps_read_packet(
                &mut ctx,
                &mut header_state,
                &mut psm_es_type,
                &mut options,
                &mut av_packet,
            );

            assert_eq!(result, 0, "Should successfully read MPEG2 VBI packet");
            assert_eq!(
                av_packet.codec_id,
                CodecIDMythTv::Mpeg2vbi,
                "Should identify MPEG2 VBI codec"
            );
            assert_eq!(
                av_packet.ty,
                CodecTypeMythTv::Data,
                "Should identify data type"
            );
        }

        cleanup_demuxer(&mut ctx);
    }

    #[test]
    fn test_mpegps_read_packet_mpeg2vbi_alternate_id() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();
        let mut psm_es_type = [0u8; 256];
        let mut av_packet = AVPacketMythTv {
            data: Vec::new(),
            size: 0,
            pts: AV_NOPTS_VALUE,
            dts: AV_NOPTS_VALUE,
            ..Default::default()
        };

        // Create a packet with alternate MPEG2 VBI ID (0x49)
        let test_data = [
            0x00, 0x00, 0x01, 0xBD, // Private stream 1 start code
            0x00, 0x08, // PES packet length
            0x0F, // No PTS/DTS
            0x49, // Alternate MPEG2 VBI substream ID
            0x00, 0x00, 0x00, // VBI header (3 bytes)
            0x56, 0x78, // VBI payload
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut header_state = 0u32;

        unsafe {
            let result = mpegps_read_packet(
                &mut ctx,
                &mut header_state,
                &mut psm_es_type,
                &mut options,
                &mut av_packet,
            );

            assert_eq!(
                result, 0,
                "Should successfully read MPEG2 VBI packet with alternate ID"
            );
            assert_eq!(
                av_packet.codec_id,
                CodecIDMythTv::Mpeg2vbi,
                "Should identify MPEG2 VBI codec"
            );
            assert_eq!(
                av_packet.ty,
                CodecTypeMythTv::Data,
                "Should identify data type"
            );
        }

        cleanup_demuxer(&mut ctx);
    }

    #[test]
    fn test_mpegps_read_packet_lpcm_insufficient_data() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();
        let mut psm_es_type = [0u8; 256];
        let mut av_packet = AVPacketMythTv {
            data: Vec::new(),
            size: 0,
            pts: AV_NOPTS_VALUE,
            dts: AV_NOPTS_VALUE,
            ..Default::default()
        };

        // Create LPCM packet with insufficient data (less than 3 bytes for header)
        let test_data = [
            0x00, 0x00, 0x01, 0xBD, // Private stream 1 start code
            0x00, 0x04, // PES packet length (only 4 bytes total)
            0x0F, // No PTS/DTS
            0xA0, // LPCM substream ID
            0x00, 0x01, // Only 2 bytes instead of required 3
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut header_state = 0u32;

        unsafe {
            let result = mpegps_read_packet(
                &mut ctx,
                &mut header_state,
                &mut psm_es_type,
                &mut options,
                &mut av_packet,
            );

            // Should handle gracefully by skipping the packet and continuing
            assert_eq!(
                result, AVERROR_IO as i32,
                "Should return error for insufficient LPCM data"
            );
        }

        cleanup_demuxer(&mut ctx);
    }

    #[test]
    fn test_mpegps_read_packet_psm_with_different_stream_types() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();
        let mut psm_es_type = [0u8; 256];
        let mut av_packet = AVPacketMythTv {
            data: Vec::new(),
            size: 0,
            pts: AV_NOPTS_VALUE,
            dts: AV_NOPTS_VALUE,
            ..Default::default()
        };

        // Set up PSM mapping for MPEG-4 video
        psm_es_type[0xE0] = StreamTypeMythTv::VideoMpeg4 as u8;

        let test_data = [
            0x00, 0x00, 0x01, 0xE0, // Video stream start code
            0x00, 0x08, // PES packet length
            0x0F, // No PTS/DTS
            0x00, 0x00, 0x01, 0xB6, // MPEG-4 visual object sequence
            0x11, 0x22, 0x33, // MPEG-4 payload
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut header_state = 0u32;

        unsafe {
            let result = mpegps_read_packet(
                &mut ctx,
                &mut header_state,
                &mut psm_es_type,
                &mut options,
                &mut av_packet,
            );

            assert_eq!(result, 0, "Should successfully read MPEG-4 packet");
            assert_eq!(
                av_packet.codec_id,
                CodecIDMythTv::Mpeg4,
                "Should identify MPEG-4 codec via PSM"
            );
            assert_eq!(
                av_packet.ty,
                CodecTypeMythTv::Video,
                "Should identify video type"
            );
        }

        cleanup_demuxer(&mut ctx);
    }

    #[test]
    fn test_mpegps_read_packet_psm_audio_aac() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();
        let mut psm_es_type = [0u8; 256];
        let mut av_packet = AVPacketMythTv {
            data: Vec::new(),
            size: 0,
            pts: AV_NOPTS_VALUE,
            dts: AV_NOPTS_VALUE,
            ..Default::default()
        };

        // Set up PSM mapping for AAC audio
        psm_es_type[0xC0] = StreamTypeMythTv::AudioAAC as u8;

        let test_data = [
            0x00, 0x00, 0x01, 0xC0, // Audio stream start code
            0x00, 0x08, // PES packet length
            0x0F, // No PTS/DTS
            0xFF, 0xF1, // AAC ADTS header
            0x50, 0x80, 0x00, // AAC payload
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut header_state = 0u32;

        unsafe {
            let result = mpegps_read_packet(
                &mut ctx,
                &mut header_state,
                &mut psm_es_type,
                &mut options,
                &mut av_packet,
            );

            assert_eq!(result, 0, "Should successfully read AAC packet");
            assert_eq!(
                av_packet.codec_id,
                CodecIDMythTv::Aac,
                "Should identify AAC codec via PSM"
            );
            assert_eq!(
                av_packet.ty,
                CodecTypeMythTv::Audio,
                "Should identify audio type"
            );
        }

        cleanup_demuxer(&mut ctx);
    }

    #[test]
    fn test_mpegps_read_packet_psm_audio_ac3() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();
        let mut psm_es_type = [0u8; 256];
        let mut av_packet = AVPacketMythTv {
            data: Vec::new(),
            size: 0,
            pts: AV_NOPTS_VALUE,
            dts: AV_NOPTS_VALUE,
            ..Default::default()
        };

        // Set up PSM mapping for AC3 audio
        psm_es_type[0xC0] = StreamTypeMythTv::AudioAC3 as u8;

        let test_data = [
            0x00, 0x00, 0x01, 0xC0, // Audio stream start code
            0x00, 0x08, // PES packet length
            0x0F, // No PTS/DTS
            0x0B, 0x77, // AC3 sync frame
            0x00, 0x00, 0x00, // AC3 payload
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut header_state = 0u32;

        unsafe {
            let result = mpegps_read_packet(
                &mut ctx,
                &mut header_state,
                &mut psm_es_type,
                &mut options,
                &mut av_packet,
            );

            assert_eq!(result, 0, "Should successfully read AC3 packet");
            assert_eq!(
                av_packet.codec_id,
                CodecIDMythTv::Ac3,
                "Should identify AC3 codec via PSM"
            );
            assert_eq!(
                av_packet.ty,
                CodecTypeMythTv::Audio,
                "Should identify audio type"
            );
        }

        cleanup_demuxer(&mut ctx);
    }

    #[test]
    fn test_mpegps_read_packet_psm_audio_mpeg1() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();
        let mut psm_es_type = [0u8; 256];
        let mut av_packet = AVPacketMythTv {
            data: Vec::new(),
            size: 0,
            pts: AV_NOPTS_VALUE,
            dts: AV_NOPTS_VALUE,
            ..Default::default()
        };

        // Set up PSM mapping for MPEG-1 audio
        psm_es_type[0xC0] = StreamTypeMythTv::AudioMpeg1 as u8;

        let test_data = [
            0x00, 0x00, 0x01, 0xC0, // Audio stream start code
            0x00, 0x08, // PES packet length
            0x0F, // No PTS/DTS
            0xFF, 0xFB, // MPEG-1 audio header
            0x90, 0x00, 0x00, // MPEG-1 audio payload
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut header_state = 0u32;

        unsafe {
            let result = mpegps_read_packet(
                &mut ctx,
                &mut header_state,
                &mut psm_es_type,
                &mut options,
                &mut av_packet,
            );

            assert_eq!(result, 0, "Should successfully read MPEG-1 audio packet");
            assert_eq!(
                av_packet.codec_id,
                CodecIDMythTv::Mp3,
                "Should identify MP3 codec via PSM"
            );
            assert_eq!(
                av_packet.ty,
                CodecTypeMythTv::Audio,
                "Should identify audio type"
            );
        }

        cleanup_demuxer(&mut ctx);
    }

    #[test]
    fn test_mpegps_read_packet_psm_video_mpeg1() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();
        let mut psm_es_type = [0u8; 256];
        let mut av_packet = AVPacketMythTv {
            data: Vec::new(),
            size: 0,
            pts: AV_NOPTS_VALUE,
            dts: AV_NOPTS_VALUE,
            ..Default::default()
        };

        // Set up PSM mapping for MPEG-1 video
        psm_es_type[0xE0] = StreamTypeMythTv::VideoMpeg1 as u8;

        let test_data = [
            0x00, 0x00, 0x01, 0xE0, // Video stream start code
            0x00, 0x08, // PES packet length
            0x0F, // No PTS/DTS
            0x00, 0x00, 0x01, 0xB3, // MPEG-1 sequence header
            0x11, 0x22, 0x33, // MPEG-1 payload
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut header_state = 0u32;

        unsafe {
            let result = mpegps_read_packet(
                &mut ctx,
                &mut header_state,
                &mut psm_es_type,
                &mut options,
                &mut av_packet,
            );

            assert_eq!(result, 0, "Should successfully read MPEG-1 video packet");
            assert_eq!(
                av_packet.codec_id,
                CodecIDMythTv::Mpeg2Video,
                "Should identify MPEG-2 video codec via PSM"
            );
            assert_eq!(
                av_packet.ty,
                CodecTypeMythTv::Video,
                "Should identify video type"
            );
        }

        cleanup_demuxer(&mut ctx);
    }

    #[test]
    fn test_mpegps_read_packet_cavs_vs_mpeg2_distinction() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();
        let mut psm_es_type = [0u8; 256];
        let mut av_packet = AVPacketMythTv {
            data: Vec::new(),
            size: 0,
            pts: AV_NOPTS_VALUE,
            dts: AV_NOPTS_VALUE,
            ..Default::default()
        };

        // Create a packet that looks like MPEG-2 (not CAVS)
        let test_data = [
            0x00, 0x00, 0x01, 0xE0, // Video stream start code
            0x00, 0x0C, // PES packet length
            0x0F, // No PTS/DTS
            0x00, 0x00, 0x01, 0xB3, // MPEG-2 sequence header
            0x00, 0x00, 0x00,
            0x01, // This makes it look like MPEG-2 (buf[6] == 0 && buf[7] == 1)
            0xDD, 0xEE, 0xFF, // More payload
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut header_state = 0u32;

        unsafe {
            let result = mpegps_read_packet(
                &mut ctx,
                &mut header_state,
                &mut psm_es_type,
                &mut options,
                &mut av_packet,
            );

            assert_eq!(result, 0, "Should successfully read packet");
            assert_eq!(
                av_packet.codec_id,
                CodecIDMythTv::Mpeg2Video,
                "Should identify MPEG-2 video (not CAVS)"
            );
            assert_eq!(
                av_packet.ty,
                CodecTypeMythTv::Video,
                "Should identify video type"
            );
        }

        cleanup_demuxer(&mut ctx);
    }

    #[test]
    fn test_mpegps_read_packet_skip_unsupported_stream() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();
        let mut psm_es_type = [0u8; 256];
        let mut av_packet = AVPacketMythTv {
            data: Vec::new(),
            size: 0,
            pts: AV_NOPTS_VALUE,
            dts: AV_NOPTS_VALUE,
            ..Default::default()
        };

        // Create data with unsupported stream followed by supported stream
        let test_data = [
            // First packet: unsupported stream
            0x00, 0x00, 0x01, 0xBD, // Private stream 1 start code
            0x00, 0x05, // PES packet length
            0x0F, // No PTS/DTS
            0x70, // Unsupported substream ID
            0x00, 0x00, 0x00, // Payload to skip
            // Second packet: supported audio stream
            0x00, 0x00, 0x01, 0xC0, // Audio stream start code
            0x00, 0x05, // PES packet length
            0x0F, // No PTS/DTS
            0x11, 0x22, // Audio payload
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut header_state = 0u32;

        unsafe {
            let result = mpegps_read_packet(
                &mut ctx,
                &mut header_state,
                &mut psm_es_type,
                &mut options,
                &mut av_packet,
            );

            // Should skip the unsupported stream and return the audio stream
            assert_eq!(
                result, 0,
                "Should successfully read supported packet after skipping unsupported"
            );
            assert_eq!(
                av_packet.codec_id,
                CodecIDMythTv::Mp2,
                "Should identify MP2 audio codec"
            );
            assert_eq!(
                av_packet.ty,
                CodecTypeMythTv::Audio,
                "Should identify audio type"
            );
        }

        cleanup_demuxer(&mut ctx);
    }

    #[test]
    fn test_mpegps_read_packet_memory_allocation_edge_case() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();
        let mut psm_es_type = [0u8; 256];
        let mut av_packet = AVPacketMythTv {
            data: Vec::new(),
            size: 0,
            pts: AV_NOPTS_VALUE,
            dts: AV_NOPTS_VALUE,
            ..Default::default()
        };

        // Create packet with zero-length payload
        let test_data = [
            0x00, 0x00, 0x01, 0xC0, // Audio stream start code
            0x00, 0x03, // PES packet length (3 bytes)
            0x0F, // No PTS/DTS
                  // No payload data (length = 0 after header)
        ];
        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut header_state = 0u32;

        unsafe {
            let result = mpegps_read_packet(
                &mut ctx,
                &mut header_state,
                &mut psm_es_type,
                &mut options,
                &mut av_packet,
            );

            // Should handle zero-length payload gracefully
            assert_eq!(
                result, 0,
                "Should successfully read packet with zero-length payload"
            );
            assert_eq!(
                av_packet.codec_id,
                CodecIDMythTv::Mp2,
                "Should identify MP2 audio codec"
            );
        }

        cleanup_demuxer(&mut ctx);
    }

    #[test]
    fn test_mpegps_read_packet_large_payload() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let mut options = create_test_options();
        let mut psm_es_type = [0u8; 256];
        let mut av_packet = AVPacketMythTv {
            data: Vec::new(),
            size: 0,
            pts: AV_NOPTS_VALUE,
            dts: AV_NOPTS_VALUE,
            ..Default::default()
        };

        // Create packet with larger payload (within buffer limits)
        let mut test_data = vec![
            0x00, 0x00, 0x01, 0xC0, // Audio stream start code
            0x00, 0x64, // PES packet length (100 bytes)
            0x0F, // No PTS/DTS
        ];

        // Add 97 bytes of payload data
        for i in 0..97 {
            test_data.push((i % 256) as u8);
        }

        fill_buffer_with_test_data(&mut ctx, &test_data);

        let mut header_state = 0u32;

        unsafe {
            let result = mpegps_read_packet(
                &mut ctx,
                &mut header_state,
                &mut psm_es_type,
                &mut options,
                &mut av_packet,
            );

            assert_eq!(
                result, 0,
                "Should successfully read packet with large payload"
            );
            assert_eq!(av_packet.size, 99, "Should have correct payload size");
            assert_eq!(
                av_packet.codec_id,
                CodecIDMythTv::Mp2,
                "Should identify MP2 audio codec"
            );
        }

        cleanup_demuxer(&mut ctx);
    }
}
