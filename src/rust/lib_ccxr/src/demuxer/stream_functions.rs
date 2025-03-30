#![allow(unexpected_cfgs)]

use crate::common::{Options, StreamMode};
use crate::demuxer::demux::{CcxDemuxer, STARTBYTESLENGTH};
use crate::fatal;
use crate::file_functions::file::{buffered_read_opt, return_to_buffer};
use crate::gxf_demuxer::gxf::{ccx_gxf_probe, CcxGxf};
use crate::util::log::{debug, info, DebugMessageFlag, ExitCause};
use std::sync::{LazyLock, Mutex};
pub static CCX_OPTIONS: LazyLock<Mutex<Options>> = LazyLock::new(|| Mutex::new(Options::default()));

/// Rust equivalent of the `ccx_stream_mp4_box` array.
#[derive(Debug)]
pub struct CcxStreamMp4Box {
    pub box_type: [u8; 4],
    pub score: i32,
}

pub static CCX_STREAM_MP4_BOXES: [CcxStreamMp4Box; 16] = [
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
pub unsafe fn detect_stream_type(ctx: &mut CcxDemuxer) {
    #[allow(unused_mut)]
    let mut ccx_options = CCX_OPTIONS.lock().unwrap();

    // Not found
    ctx.stream_mode = StreamMode::ElementaryOrNotFound;

    // Attempt a buffered read
    // ctx.startbytes_avail = buffered_read_opt(ctx, &mut ctx.startbytes, STARTBYTESLENGTH) as i32;
    let mut startbytes = ctx.startbytes.to_vec(); // Copy the value of `ctx.startbytes`
    let startbytes_avail = buffered_read_opt(ctx, &mut startbytes, STARTBYTESLENGTH) as i32;
    ctx.startbytes_avail = startbytes_avail;
    if ctx.startbytes_avail == -1 {
        fatal!(cause = ExitCause::ReadError; "Error reading input file!\n");
    }

    // Check for ASF magic bytes
    if ctx.startbytes_avail >= 4 && ctx.startbytes[0] == 0x30
        && ctx.startbytes[1] == 0x26
        && ctx.startbytes[2] == 0xb2 && ctx.startbytes[3] == 0x75 {
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
    if ctx.stream_mode == StreamMode::ElementaryOrNotFound && ccx_gxf_probe(&ctx.startbytes) {
        ctx.stream_mode = StreamMode::Gxf;
        // ctx.private_data = CcxGxf::default() as *mut std::ffi::c_void;
        ctx.private_data = Box::into_raw(Box::new(CcxGxf::default())) as *mut core::ffi::c_void;
    }

    // WTV check
    if ctx.stream_mode == StreamMode::ElementaryOrNotFound && ctx.startbytes_avail >= 4 && ctx.startbytes[0] == 0xb7
        && ctx.startbytes[1] == 0xd8
        && ctx.startbytes[2] == 0x00 && ctx.startbytes[3] == 0x20 {
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
    if ctx.stream_mode == StreamMode::ElementaryOrNotFound && ctx.startbytes_avail >= 11 && ctx.startbytes[0] == 0xCC
        && ctx.startbytes[1] == 0xCC
        && ctx.startbytes[2] == 0xED
        && ctx.startbytes[8] == 0
        && ctx.startbytes[9] == 0 && ctx.startbytes[10] == 0 {
        ctx.stream_mode = StreamMode::Rcwt;
    }
    // MP4 check. "Still not found" or we want file reports.
    if (ctx.stream_mode == StreamMode::ElementaryOrNotFound
        || ccx_options.print_file_reports)
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
        // if ccx_probe_mxf(ctx) == CCX_TRUE { //TODO
        //     ctx.stream_mode = StreamMode::Mxf; //TODO
        //     ctx.private_data = ccx_mxf_init(ctx); //TODO
        // } //TODO
    }

    // Still not found
    if ctx.stream_mode == StreamMode::ElementaryOrNotFound {
        // Otherwise, assume no TS
        if ctx.startbytes_avail as usize > 188 * 8 {
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
                // return_to_buffer(ctx, &ctx.startbytes, ctx.startbytes_avail as u32);
                let startbytes_copy = ctx.startbytes.to_vec(); // Copy the value of `ctx.startbytes`
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
                // return_to_buffer(ctx, &ctx.startbytes, ctx.startbytes_avail as u32);
                let startbytes_copy = ctx.startbytes.to_vec(); // Copy the value of `ctx.startbytes`
                return_to_buffer(ctx, &startbytes_copy, ctx.startbytes_avail as u32);

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
    let startbytes_copy = ctx.startbytes.to_vec(); // Copy the value of `ctx.startbytes`
    return_to_buffer(ctx, &startbytes_copy, ctx.startbytes_avail as u32);

    // return_to_buffer(ctx, &ctx.startbytes, ctx.startbytes_avail as u32);
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

/// Translated version of the C `isValidMP4Box` function with comments preserved.
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
