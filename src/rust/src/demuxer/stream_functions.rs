use crate::bindings::ccx_demuxer;
use crate::demuxer::common_types::{CcxDemuxer, CcxStreamMp4Box, STARTBYTESLENGTH};
use crate::file_functions::file::{buffered_read_opt, return_to_buffer};
use crate::libccxr_exports::demuxer::{alloc_new_demuxer, copy_demuxer_from_rust_to_c};
use cfg_if::cfg_if;
use lib_ccxr::common::{Options, StreamMode};
use lib_ccxr::fatal;
use lib_ccxr::util::log::{debug, info, DebugMessageFlag, ExitCause};

cfg_if! {
    if #[cfg(test)] {
        use crate::demuxer::stream_functions::tests::{ccx_gxf_init, ccx_gxf_probe, ccx_mxf_init, ccx_probe_mxf};
    }
    else {
        use crate::{ccx_gxf_init, ccx_gxf_probe, ccx_mxf_init, ccx_probe_mxf};
    }
}

pub const CCX_STREAM_MP4_BOXES: [CcxStreamMp4Box; 16] = [
    CcxStreamMp4Box {
        box_type: *b"ftyp",
        score: 6,
    }, // File type
    CcxStreamMp4Box {
        box_type: *b"pdin",
        score: 1,
    }, // Progressive download info
    CcxStreamMp4Box {
        box_type: *b"moov",
        score: 5,
    }, // Container for all metadata
    CcxStreamMp4Box {
        box_type: *b"moof",
        score: 4,
    }, // Movie fragment
    CcxStreamMp4Box {
        box_type: *b"mfra",
        score: 1,
    }, // Movie fragment random access
    CcxStreamMp4Box {
        box_type: *b"mdat",
        score: 2,
    }, // Media data container
    CcxStreamMp4Box {
        box_type: *b"free",
        score: 1,
    }, // Free space
    CcxStreamMp4Box {
        box_type: *b"skip",
        score: 1,
    }, // Free space
    CcxStreamMp4Box {
        box_type: *b"meta",
        score: 1,
    }, // Metadata
    CcxStreamMp4Box {
        box_type: *b"wide",
        score: 1,
    }, // Boxes > 2^32 bytes
    CcxStreamMp4Box {
        box_type: *b"void",
        score: 1,
    }, // Assume free space
    CcxStreamMp4Box {
        box_type: *b"meco",
        score: 1,
    }, // Additional metadata container
    CcxStreamMp4Box {
        box_type: *b"styp",
        score: 1,
    }, // Segment type
    CcxStreamMp4Box {
        box_type: *b"sidx",
        score: 1,
    }, // Segment index
    CcxStreamMp4Box {
        box_type: *b"ssix",
        score: 1,
    }, // Subsegment index
    CcxStreamMp4Box {
        box_type: *b"prft",
        score: 1,
    }, // Producer reference time
];
/// C `detect_stream_type` function
/// # Safety
/// This function is unsafe because it calls unsafe function buffered_read_opt.
pub unsafe fn detect_stream_type(ctx: &mut CcxDemuxer, ccx_options: &mut Options) {
    // Not found
    ctx.stream_mode = StreamMode::ElementaryOrNotFound;

    // Attempt a buffered read
    let startbytes_ptr = ctx.startbytes.as_mut_ptr();
    ctx.startbytes_avail =
        buffered_read_opt(ctx, startbytes_ptr, STARTBYTESLENGTH, ccx_options) as i32;

    if ctx.startbytes_avail == -1 {
        fatal!(cause = ExitCause::ReadError; "Error reading input file!\n");
    }

    // Call the common detection logic
    detect_stream_type_common(ctx, ccx_options);
}

unsafe fn detect_stream_type_common(ctx: &mut CcxDemuxer, ccx_options: &mut Options) {
    // Check for ASF magic bytes
    if ctx.startbytes_avail >= 4
        && ctx.startbytes[0] == 0x30
        && ctx.startbytes[1] == 0x26
        && ctx.startbytes[2] == 0xb2
        && ctx.startbytes[3] == 0x75
    {
        ctx.stream_mode = StreamMode::Asf;
    }

    // WARNING: Always check containers first (such as Matroska),
    // because they contain bytes that can be mistaken as a separate video file.
    if ctx.stream_mode == StreamMode::ElementaryOrNotFound && ctx.startbytes_avail >= 4 {
        // Check for Matroska magic bytes - EBML head
        if ctx.startbytes[0] == 0x1a
            && ctx.startbytes[1] == 0x45
            && ctx.startbytes[2] == 0xdf
            && ctx.startbytes[3] == 0xa3
        {
            ctx.stream_mode = StreamMode::Mkv;
        }
        // Check for Matroska magic bytes - Segment
        if ctx.stream_mode == StreamMode::ElementaryOrNotFound
            && ctx.startbytes[0] == 0x18
            && ctx.startbytes[1] == 0x53
            && ctx.startbytes[2] == 0x80
            && ctx.startbytes[3] == 0x67
        {
            ctx.stream_mode = StreamMode::Mkv;
        }
    }

    // GXF probe
    if ctx.stream_mode == StreamMode::ElementaryOrNotFound
        && ccx_gxf_probe(ctx.startbytes.as_ptr(), ctx.startbytes_avail) != 0
    {
        ctx.stream_mode = StreamMode::Gxf;

        let demuxer: *mut ccx_demuxer = alloc_new_demuxer();
        copy_demuxer_from_rust_to_c(demuxer, ctx);
        let private = ccx_gxf_init(demuxer);
        ctx.private_data = private as *mut core::ffi::c_void;
        drop(Box::from_raw(demuxer));
    }

    // WTV check
    if ctx.stream_mode == StreamMode::ElementaryOrNotFound
        && ctx.startbytes_avail >= 4
        && ctx.startbytes[0] == 0xb7
        && ctx.startbytes[1] == 0xd8
        && ctx.startbytes[2] == 0x00
        && ctx.startbytes[3] == 0x20
    {
        ctx.stream_mode = StreamMode::Wtv;
    }

    // Hex dump check
    #[cfg(feature = "wtv_debug")]
    {
        if ctx.stream_mode == StreamMode::ElementaryOrNotFound && ctx.startbytes_avail >= 6 {
            // Check for hexadecimal dump generated by wtvccdump
            // ; CCHD
            if ctx.startbytes[0] == b';'
                && ctx.startbytes[1] == b' '
                && ctx.startbytes[2] == b'C'
                && ctx.startbytes[3] == b'C'
                && ctx.startbytes[4] == b'H'
                && ctx.startbytes[5] == b'D'
            {
                ctx.stream_mode = StreamMode::HexDump;
            }
        }
    }

    // Check for CCExtractor magic bytes
    if ctx.stream_mode == StreamMode::ElementaryOrNotFound
        && ctx.startbytes_avail >= 11
        && ctx.startbytes[0] == 0xCC
        && ctx.startbytes[1] == 0xCC
        && ctx.startbytes[2] == 0xED
        && ctx.startbytes[8] == 0
        && ctx.startbytes[9] == 0
        && ctx.startbytes[10] == 0
    {
        ctx.stream_mode = StreamMode::Rcwt;
    }

    // MP4 check. "Still not found" or we want file reports.
    if (ctx.stream_mode == StreamMode::ElementaryOrNotFound || ccx_options.print_file_reports)
        && ctx.startbytes_avail >= 4
    {
        let mut idx = 0usize;
        let mut box_score = 0;
        // Scan the buffer for valid succeeding MP4 boxes.
        while idx + 8 <= ctx.startbytes_avail as usize {
            // Check if we have a valid box
            let mut next_box_location = 0usize;
            if is_valid_mp4_box(&ctx.startbytes, idx, &mut next_box_location, &mut box_score) != 0
                && next_box_location > idx
            {
                // If the box is valid, a new box should be found
                idx = next_box_location;
                if box_score > 7 {
                    break;
                }
            } else {
                // Not a valid box, reset score. We need a couple of successive boxes to identify an MP4 file.
                box_score = 0;
                idx += 1;
            }
        }
        // We had at least one box (or multiple) to claim MP4
        if box_score > 1 {
            ctx.stream_mode = StreamMode::Mp4;
        }
    }

    // Search for MXF header
    if ctx.stream_mode == StreamMode::ElementaryOrNotFound {
        let demuxer: *mut ccx_demuxer = alloc_new_demuxer();
        copy_demuxer_from_rust_to_c(demuxer, ctx);
        if ccx_probe_mxf(demuxer) == 1 {
            ctx.stream_mode = StreamMode::Mxf;
            let private = ccx_mxf_init(demuxer);
            ctx.private_data = private as *mut core::ffi::c_void;
        }
        drop(Box::from_raw(demuxer));
    }

    // Still not found
    if ctx.stream_mode == StreamMode::ElementaryOrNotFound {
        // Otherwise, assume no TS
        if ctx.startbytes_avail > 188 * 8 {
            // First check for TS
            for i in 0..188 {
                let base = i as usize;
                if ctx.startbytes[base] == 0x47
                    && ctx.startbytes[base + 188] == 0x47
                    && ctx.startbytes[base + 188 * 2] == 0x47
                    && ctx.startbytes[base + 188 * 3] == 0x47
                    && ctx.startbytes[base + 188 * 4] == 0x47
                    && ctx.startbytes[base + 188 * 5] == 0x47
                    && ctx.startbytes[base + 188 * 6] == 0x47
                    && ctx.startbytes[base + 188 * 7] == 0x47
                {
                    // Eight sync bytes, that's good enough
                    ctx.startbytes_pos = i as u32;
                    ctx.stream_mode = StreamMode::Transport;
                    ctx.m2ts = 0;
                    break;
                }
            }
            if ctx.stream_mode == StreamMode::Transport {
                debug!(msg_type = DebugMessageFlag::PARSE; "detect_stream_type: detected as TS\n");
                let startbytes_copy = ctx.startbytes.to_vec();
                return_to_buffer(ctx, &startbytes_copy, ctx.startbytes_avail as u32);
                return;
            }

            // Check for M2TS
            for i in 0..192 {
                let base = i as usize + 4;
                if ctx.startbytes[base] == 0x47
                    && ctx.startbytes[base + 192] == 0x47
                    && ctx.startbytes[base + 192 * 2] == 0x47
                    && ctx.startbytes[base + 192 * 3] == 0x47
                    && ctx.startbytes[base + 192 * 4] == 0x47
                    && ctx.startbytes[base + 192 * 5] == 0x47
                    && ctx.startbytes[base + 192 * 6] == 0x47
                    && ctx.startbytes[base + 192 * 7] == 0x47
                {
                    // Eight sync bytes, that's good enough
                    ctx.startbytes_pos = i as u32;
                    ctx.stream_mode = StreamMode::Transport;
                    ctx.m2ts = 1;
                    break;
                }
            }
            if ctx.stream_mode == StreamMode::Transport {
                debug!(msg_type = DebugMessageFlag::PARSE; "detect_stream_type: detected as M2TS\n");
                let startbytes_ptr = ctx.startbytes.as_ptr();
                let startbytes_slice =
                    std::slice::from_raw_parts(startbytes_ptr, ctx.startbytes_avail as usize);
                return_to_buffer(ctx, startbytes_slice, ctx.startbytes_avail as u32);
                return;
            }

            // Now check for PS (Needs PACK header)
            let limit = if ctx.startbytes_avail < 50000 {
                ctx.startbytes_avail - 3
            } else {
                49997
            } as usize;
            for i in 0..limit {
                if ctx.startbytes[i] == 0x00
                    && ctx.startbytes[i + 1] == 0x00
                    && ctx.startbytes[i + 2] == 0x01
                    && ctx.startbytes[i + 3] == 0xBA
                {
                    // If we find a PACK header it is not an ES
                    ctx.startbytes_pos = i as u32;
                    ctx.stream_mode = StreamMode::Program;
                    break;
                }
            }
            if ctx.stream_mode == StreamMode::Program {
                debug!(msg_type = DebugMessageFlag::PARSE; "detect_stream_type: detected as PS\n");
            }

            // TiVo is also a PS
            if ctx.startbytes[0] == b'T'
                && ctx.startbytes[1] == b'i'
                && ctx.startbytes[2] == b'V'
                && ctx.startbytes[3] == b'o'
            {
                // The TiVo header is longer, but the PS loop will find the beginning
                debug!(msg_type = DebugMessageFlag::PARSE; "detect_stream_type: detected as Tivo PS\n");
                ctx.startbytes_pos = 187;
                ctx.stream_mode = StreamMode::Program;
                ctx.strangeheader = 1; // Avoid message about unrecognized header
            }
        } else {
            ctx.startbytes_pos = 0;
            ctx.stream_mode = StreamMode::ElementaryOrNotFound;
        }
    }

    // Don't use STARTBYTESLENGTH. It might be longer than the file length!
    let startbytes_ptr = ctx.startbytes.as_ptr();
    let startbytes_slice =
        std::slice::from_raw_parts(startbytes_ptr, ctx.startbytes_avail as usize);
    return_to_buffer(ctx, startbytes_slice, ctx.startbytes_avail as u32);
}
pub fn detect_myth(ctx: &mut CcxDemuxer) -> i32 {
    let mut vbi_blocks = 0;
    // VBI data? If yes, use MythTV loop
    // STARTBYTESLENGTH is 1MB; if the file is shorter, we will never detect
    // it as a MythTV file
    if ctx.startbytes_avail == STARTBYTESLENGTH as i32 {
        let mut uc = [0u8; 3];
        uc.copy_from_slice(&ctx.startbytes[..3]);

        for &byte in &ctx.startbytes[3..ctx.startbytes_avail as usize] {
            if (uc == [b't', b'v', b'0']) || (uc == [b'T', b'V', b'0']) {
                vbi_blocks += 1;
            }

            uc[0] = uc[1];
            uc[1] = uc[2];
            uc[2] = byte;
        }
    }

    if vbi_blocks > 10 {
        return 1;
    }

    0
}

/// C `isValidMP4Box` function
pub fn is_valid_mp4_box(
    buffer: &[u8],
    position: usize,
    next_box_location: &mut usize,
    box_score: &mut i32,
) -> i32 {
    // For each known MP4 box type, check if there's a match.
    for (idx, _) in CCX_STREAM_MP4_BOXES.iter().enumerate() {
        // Compare the 4 bytes in the provided buffer to the boxType in ccx_stream_mp4_boxes.
        if buffer[position + 4] == CCX_STREAM_MP4_BOXES[idx].box_type[0]
            && buffer[position + 5] == CCX_STREAM_MP4_BOXES[idx].box_type[1]
            && buffer[position + 6] == CCX_STREAM_MP4_BOXES[idx].box_type[2]
            && buffer[position + 7] == CCX_STREAM_MP4_BOXES[idx].box_type[3]
        {
            // Print detected MP4 box name
            info!(
                "{}",
                &format!(
                    "Detected MP4 box with name: {}\n",
                    std::str::from_utf8(&CCX_STREAM_MP4_BOXES[idx].box_type).unwrap_or("???")
                )
            );

            // If the box type is "moov", check if it contains a valid movie header (mvhd)
            if idx == 2
                && !(buffer[position + 12] == b'm'
                    && buffer[position + 13] == b'v'
                    && buffer[position + 14] == b'h'
                    && buffer[position + 15] == b'd')
            {
                // If "moov" doesn't have "mvhd", skip it.
                continue;
            }

            // Box name matches. Do a crude validation of possible box size,
            // and if valid, add points for "valid" box.
            let mut box_size = (buffer[position] as usize) << 24;
            box_size |= (buffer[position + 1] as usize) << 16;
            box_size |= (buffer[position + 2] as usize) << 8;
            box_size |= buffer[position + 3] as usize;

            *box_score += CCX_STREAM_MP4_BOXES[idx].score;

            if box_size == 0 || box_size == 1 {
                // If box size is 0 or 1, we can't reliably parse further.
                // nextBoxLocation is set to the max possible to skip further checks.
                *next_box_location = u32::MAX as usize;
            } else {
                // Valid box length detected
                *next_box_location = position + box_size;
            }
            return 1; // Found a valid box
        }
    }
    // No match
    0
}
#[cfg(test)]
mod tests {
    use super::*;
    use crate::bindings::{ccx_demuxer, ccx_gxf, MXFContext};
    use crate::file_functions::file::FILEBUFFERSIZE;
    use lib_ccxr::util::log::{set_logger, CCExtractorLogger, DebugMessageMask, OutputTarget};
    use std::os::raw::{c_int, c_uchar};
    use std::ptr;
    use std::sync::Once;

    pub fn ccx_probe_mxf(_ctx: *mut ccx_demuxer) -> c_int {
        0
    }
    pub unsafe fn ccx_mxf_init(_demux: *mut ccx_demuxer) -> *mut MXFContext {
        Box::into_raw(Box::new(MXFContext::default()))
    }

    pub fn ccx_gxf_probe(_buf: *const c_uchar, _len: c_int) -> c_int {
        0
    }
    pub fn ccx_gxf_init(_arg: *mut ccx_demuxer) -> *mut ccx_gxf {
        Box::into_raw(Box::new(ccx_gxf::default()))
    }

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
    /// Allocates `ctx.filebuffer` via a `Vec<u8>` leak, sets lengths to zero.
    fn make_ctx_and_options() -> (CcxDemuxer<'static>, Options) {
        let mut ctx = CcxDemuxer::default();
        let mut opts = Options::default();
        opts.live_stream = None;
        opts.buffer_input = false;
        opts.print_file_reports = false;
        opts.binary_concat = false;

        // Create a Vec<u8> of length FILEBUFFERSIZE, leak it for ctx.filebuffer
        let mut vec_buf = vec![0u8; FILEBUFFERSIZE];
        let ptr = vec_buf.as_mut_ptr();
        std::mem::forget(vec_buf); // Leak the Vec; we'll reclaim later
        ctx.filebuffer = ptr;
        ctx.bytesinbuffer = 0;
        ctx.filebuffer_pos = 0;

        (ctx, opts)
    }
    unsafe fn detect_stream_type_from_bytes(
        ctx: &mut CcxDemuxer,
        bytes: &[u8],
        ccx_options: &mut Options,
    ) {
        // Safety: `bytes.len()` must be <= STARTBYTESLENGTH.
        let n = bytes.len();
        assert_ne!(n, 0, "Test‐slice must not be empty");
        assert!(n <= STARTBYTESLENGTH, "Test‐slice too large");

        // Zero the entire buffer first:
        for slot in ctx.startbytes.iter_mut() {
            *slot = 0;
        }
        // Copy the test bytes into the front:
        ctx.startbytes[..n].copy_from_slice(bytes);
        ctx.startbytes_avail = n as i32;
        assert!(
            ctx.startbytes.len() >= STARTBYTESLENGTH,
            "startbytes too small"
        );
        assert!(
            ctx.startbytes.len() >= ctx.startbytes_avail as usize,
            "startbytes could be empty"
        );

        // Not found initially
        ctx.stream_mode = StreamMode::ElementaryOrNotFound;

        // Call the common detection logic
        detect_stream_type_common(ctx, ccx_options);
    }

    /// Reconstructs the `Vec<u8>` from `ctx.filebuffer` and frees it.
    unsafe fn free_ctx_filebuffer(ctx: &mut CcxDemuxer) {
        if !ctx.filebuffer.is_null() {
            // Rebuild Vec<u8> so it gets dropped
            let _ = Vec::from_raw_parts(ctx.filebuffer, FILEBUFFERSIZE, FILEBUFFERSIZE);
            ctx.filebuffer = ptr::null_mut();
            ctx.bytesinbuffer = 0;
            ctx.filebuffer_pos = 0;
        }
    }

    /// 1. ASF (0x30 0x26 0xB2 0x75)
    #[test]
    fn detects_asf() {
        let (mut ctx, mut opts) = make_ctx_and_options();
        let mut bytes = [0u8; STARTBYTESLENGTH];
        bytes[0] = 0x30;
        bytes[1] = 0x26;
        bytes[2] = 0xB2;
        bytes[3] = 0x75;
        unsafe {
            detect_stream_type_from_bytes(&mut ctx, &bytes[..4], &mut opts);
        }
        assert_eq!(ctx.stream_mode, StreamMode::Asf);
        unsafe { free_ctx_filebuffer(&mut ctx) };
    }

    /// 2. MKV – EBML head (0x1A 0x45 0xDF 0xA3)
    #[test]
    fn detects_mkv_ebml() {
        let (mut ctx, mut opts) = make_ctx_and_options();
        let mut bytes = [0u8; STARTBYTESLENGTH];
        bytes[0] = 0x1A;
        bytes[1] = 0x45;
        bytes[2] = 0xDF;
        bytes[3] = 0xA3;
        unsafe {
            detect_stream_type_from_bytes(&mut ctx, &bytes[..4], &mut opts);
        }
        assert_eq!(ctx.stream_mode, StreamMode::Mkv);
        unsafe { free_ctx_filebuffer(&mut ctx) };
    }

    /// 3. MKV – Segment (0x18 0x53 0x80 0x67)
    #[test]
    fn detects_mkv_segment() {
        let (mut ctx, mut opts) = make_ctx_and_options();
        let mut bytes = [0u8; STARTBYTESLENGTH];
        bytes[0] = 0x18;
        bytes[1] = 0x53;
        bytes[2] = 0x80;
        bytes[3] = 0x67;
        unsafe {
            detect_stream_type_from_bytes(&mut ctx, &bytes[..4], &mut opts);
        }
        assert_eq!(ctx.stream_mode, StreamMode::Mkv);
        unsafe { free_ctx_filebuffer(&mut ctx) };
    }

    /// 5. WTV (0xB7 0xD8 0x00 0x20)
    #[test]
    fn detects_wtv() {
        let (mut ctx, mut opts) = make_ctx_and_options();
        let mut bytes = [0u8; STARTBYTESLENGTH];
        bytes[0] = 0xB7;
        bytes[1] = 0xD8;
        bytes[2] = 0x00;
        bytes[3] = 0x20;
        unsafe {
            detect_stream_type_from_bytes(&mut ctx, &bytes[..4], &mut opts);
        }
        assert_eq!(ctx.stream_mode, StreamMode::Wtv);
        unsafe { free_ctx_filebuffer(&mut ctx) };
    }

    /// 6. Hex‐dump (“; CCHD”)
    #[test]
    fn detects_hexdump() {
        let (mut ctx, mut opts) = make_ctx_and_options();
        let mut bytes = [0u8; STARTBYTESLENGTH];
        bytes[0] = b';';
        bytes[1] = b' ';
        bytes[2] = b'C';
        bytes[3] = b'C';
        bytes[4] = b'H';
        bytes[5] = b'D';
        unsafe {
            detect_stream_type_from_bytes(&mut ctx, &bytes[..6], &mut opts);
        }
        #[cfg(feature = "wtv_debug")]
        {
            assert_eq!(ctx.stream_mode, StreamMode::HexDump);
        }
        #[cfg(not(feature = "wtv_debug"))]
        {
            assert_eq!(ctx.stream_mode, StreamMode::ElementaryOrNotFound);
        }
        unsafe { free_ctx_filebuffer(&mut ctx) };
    }

    /// 7. RCWT (0xCC 0xCC 0xED … 0 0 0)
    #[test]
    fn detects_rcwt() {
        let (mut ctx, mut opts) = make_ctx_and_options();
        let mut bytes = [0u8; STARTBYTESLENGTH];
        bytes[0] = 0xCC;
        bytes[1] = 0xCC;
        bytes[2] = 0xED;
        bytes[8] = 0;
        bytes[9] = 0;
        bytes[10] = 0;
        unsafe {
            detect_stream_type_from_bytes(&mut ctx, &bytes[..11], &mut opts);
        }
        assert_eq!(ctx.stream_mode, StreamMode::Rcwt);
        unsafe { free_ctx_filebuffer(&mut ctx) };
    }

    /// 8. MP4 – two boxes: “ftyp” + “free”
    #[test]
    fn detects_mp4() {
        initialize_logger();
        let (mut ctx, mut opts) = make_ctx_and_options();
        let mut bytes = [0u8; STARTBYTESLENGTH];
        // box1: size=8, type="ftyp"
        bytes[0] = 0x00;
        bytes[1] = 0x00;
        bytes[2] = 0x00;
        bytes[3] = 0x08;
        bytes[4] = b'f';
        bytes[5] = b't';
        bytes[6] = b'y';
        bytes[7] = b'p';
        // box2: size=8, type="free" at offset=8
        bytes[8] = 0x00;
        bytes[9] = 0x00;
        bytes[10] = 0x00;
        bytes[11] = 0x08;
        bytes[12] = b'f';
        bytes[13] = b'r';
        bytes[14] = b'e';
        bytes[15] = b'e';
        unsafe {
            detect_stream_type_from_bytes(&mut ctx, &bytes[..16], &mut opts);
        }
        assert_eq!(ctx.stream_mode, StreamMode::Mp4);
        unsafe { free_ctx_filebuffer(&mut ctx) };
    }

    /// 10. TS – eight sync bytes at 188‐byte intervals
    #[test]
    fn detects_ts() {
        initialize_logger();
        let (mut ctx, mut opts) = make_ctx_and_options();
        let mut bytes = [0u8; STARTBYTESLENGTH];
        for k in 0..8 {
            let idx = k * 188;
            if idx < STARTBYTESLENGTH {
                bytes[idx] = 0x47;
            }
        }

        let avail = 188 * 8 + 1;
        unsafe {
            detect_stream_type_from_bytes(&mut ctx, &bytes[..avail], &mut opts);
        }
        assert_eq!(ctx.stream_mode, StreamMode::Transport);
        assert_eq!(ctx.m2ts, 0);
        assert_eq!(ctx.startbytes_pos, 0);
        unsafe { free_ctx_filebuffer(&mut ctx) };
    }

    /// 11. M2TS – sync bytes at 4 + (192 * k)
    #[test]
    fn detects_m2ts() {
        initialize_logger();
        let (mut ctx, mut opts) = make_ctx_and_options();
        let mut bytes = [0u8; STARTBYTESLENGTH];
        for k in 0..8 {
            let idx = 4 + k * 192;
            if idx < STARTBYTESLENGTH {
                bytes[idx] = 0x47;
            }
        }
        let avail = 192 * 8 + 5;
        unsafe {
            detect_stream_type_from_bytes(&mut ctx, &bytes[..avail], &mut opts);
        }
        assert_eq!(ctx.stream_mode, StreamMode::Transport);
        assert_eq!(ctx.m2ts, 1);
        assert_eq!(ctx.startbytes_pos, 0);
        unsafe { free_ctx_filebuffer(&mut ctx) };
    }

    /// 12. PS – “0x00 0x00 0x01 0xBA” at index 10
    #[test]
    fn detects_ps() {
        let (mut ctx, mut opts) = make_ctx_and_options();
        let mut bytes = [0u8; STARTBYTESLENGTH];
        bytes[10] = 0x00;
        bytes[11] = 0x00;
        bytes[12] = 0x01;
        bytes[13] = 0xBA;
        // Must pass > 1504 bytes so the code enters the PS branch
        let avail = 2000;
        assert!(avail <= STARTBYTESLENGTH);
        unsafe {
            detect_stream_type_from_bytes(&mut ctx, &bytes[..avail], &mut opts);
        }
        assert_eq!(ctx.stream_mode, StreamMode::Program);
        assert_eq!(ctx.startbytes_pos, 10);
        unsafe { free_ctx_filebuffer(&mut ctx) };
    }

    /// 13. TiVo – “TiVo” in the first four bytes, with avail > 1504
    #[test]
    fn detects_tivo_ps() {
        let (mut ctx, mut opts) = make_ctx_and_options();
        let mut bytes = [0u8; STARTBYTESLENGTH];
        bytes[0] = b'T';
        bytes[1] = b'i';
        bytes[2] = b'V';
        bytes[3] = b'o';
        // Must pass > 1504 bytes so the TiVo check runs
        let avail = 2000;
        assert!(avail <= STARTBYTESLENGTH);
        unsafe {
            detect_stream_type_from_bytes(&mut ctx, &bytes[..avail], &mut opts);
        }
        assert_eq!(ctx.stream_mode, StreamMode::Program);
        assert_eq!(ctx.startbytes_pos, 187);
        assert_eq!(ctx.strangeheader, 1);
        unsafe { free_ctx_filebuffer(&mut ctx) };
    }

    /// 14. Fallback – no valid magic
    #[test]
    fn detects_elementary_fallback() {
        let (mut ctx, mut opts) = make_ctx_and_options();
        let bytes = [0xFFu8; STARTBYTESLENGTH];
        unsafe {
            detect_stream_type_from_bytes(&mut ctx, &bytes[..10], &mut opts);
        }
        assert_eq!(ctx.stream_mode, StreamMode::ElementaryOrNotFound);
        unsafe { free_ctx_filebuffer(&mut ctx) };
    }
}
