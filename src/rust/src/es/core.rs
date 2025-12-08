use crate::bindings::{cc_subtitle, encoder_ctx, lib_cc_decode};
use crate::es::eau::read_eau_info;
use crate::es::gop::read_gop_info;
use crate::es::pic::{read_pic_data, read_pic_info};
use crate::es::seq::read_seq_info;
use lib_ccxr::common::{BitStreamRust, BitstreamError, Options};
use lib_ccxr::dbg_es;
use lib_ccxr::util::log::DebugMessageFlag;
use lib_ccxr::{debug, info};
use std::slice;
use std::sync::atomic::{AtomicBool, Ordering};

static NOSKIPMESSAGE: AtomicBool = AtomicBool::new(true);
/* Process a mpeg-2 data stream with "length" bytes in buffer "data".
 * The number of processed bytes is returned.
 * Defined in ISO/IEC 13818-2 6.2 */
pub fn process_m2v(
    enc_ctx: &mut encoder_ctx,
    dec_ctx: &mut lib_cc_decode,
    data: &[u8],
    length: usize,
    sub: &mut cc_subtitle,
    ccx_options: &mut Options,
) -> Result<usize, BitstreamError> {
    if length < 8 {
        // Need to look ahead 8 bytes
        return Ok(length);
    }

    // Init bitstream
    let mut esstream = BitStreamRust::new(data)?;
    esstream.init_bitstream(0, length)?;

    // Process data. The return value is ignored as esstream.pos holds
    // the information how far the parsing progressed.
    let _ = es_video_sequence(enc_ctx, dec_ctx, &mut esstream, sub, ccx_options)?;

    // This returns how many bytes were processed and can therefore
    // be discarded from "buffer". "esstream.pos" points to the next byte
    // where processing will continue.
    Ok(esstream.pos)
}

// Return TRUE if the video sequence was finished, FALSE
// Otherwise.  estream->pos shall point to the position where
// the next call will continue, i.e. the possible begin of an
// unfinished video sequence or after the finished sequence.
pub fn es_video_sequence(
    enc_ctx: &mut encoder_ctx,
    dec_ctx: &mut lib_cc_decode,
    esstream: &mut BitStreamRust,
    sub: &mut cc_subtitle,
    ccx_options: &mut Options,
) -> Result<bool, BitstreamError> {
    // Avoid "Skip forward" message on first call and later only
    // once per search.

    dbg_es!("es_video_sequence()\n");

    esstream.error = false;

    // Analyze sequence header ...
    if dec_ctx.no_bitstream_error == 0 {
        // We might start here because of a syntax error. Discard
        // all data until a new sequence_header_code or group_start_code
        // is found.

        if !NOSKIPMESSAGE.load(Ordering::Relaxed) {
            // Avoid unnecessary output.
            info!("\nSkip forward to the next Sequence or GOP start.\n");
        } else {
            NOSKIPMESSAGE.store(false, Ordering::Relaxed);
        }

        loop {
            // search_start_code() cannot produce esstream->error
            let startcode = esstream.search_start_code()?;
            if esstream.bits_left < 0 {
                NOSKIPMESSAGE.store(true, Ordering::Relaxed);
                return Ok(false);
            }

            if startcode == 0xB3 || startcode == 0xB8 {
                // found it
                break;
            }

            esstream.skip_bits(4 * 8)?;
        }

        dec_ctx.no_bitstream_error = 1;
        dec_ctx.saw_seqgoppic = 0;
        dec_ctx.in_pic_data = 0;
    }

    loop {
        let startcode = esstream.next_start_code()?;

        dbg_es!(
            "\nM2V - next start code {:02X} {}\n",
            startcode,
            dec_ctx.in_pic_data
        );

        // Syntax check - also returns on bitsleft < 0
        if startcode == 0xB4 {
            if esstream.error {
                dec_ctx.no_bitstream_error = 0;
                dbg_es!("es_video_sequence: syntax problem.\n");
            }

            dbg_es!("es_video_sequence: return on B4 startcode.\n");

            return Ok(false);
        }

        // Sequence_end_code
        if startcode == 0xB7 {
            esstream.skip_bits(32)?; // Advance bitstream
            dec_ctx.no_bitstream_error = 0;
            break; // Exit the main loop - sequence is complete
        }
        // Sequence header
        else if dec_ctx.in_pic_data == 0 && startcode == 0xB3 {
            if !read_seq_info(dec_ctx, esstream, ccx_options)? {
                if esstream.error {
                    dec_ctx.no_bitstream_error = 0;
                }
                return Ok(false);
            }
            dec_ctx.saw_seqgoppic = 1;
            continue; // Continue to next iteration
        }
        // Group of pictures
        else if dec_ctx.in_pic_data == 0 && startcode == 0xB8 {
            if !unsafe { read_gop_info(enc_ctx, dec_ctx, esstream, sub)? } {
                if esstream.error {
                    dec_ctx.no_bitstream_error = 0;
                }
                return Ok(false);
            }
            dec_ctx.saw_seqgoppic = 2;
            continue; // Continue to next iteration
        }
        // Picture
        else if dec_ctx.in_pic_data == 0 && startcode == 0x00 {
            if !unsafe { read_pic_info(enc_ctx, dec_ctx, esstream, sub)? } {
                if esstream.error {
                    dec_ctx.no_bitstream_error = 0;
                }
                return Ok(false);
            }
            dec_ctx.saw_seqgoppic = 3;
            dec_ctx.in_pic_data = 1;
            continue; // Continue to next iteration
        }
        // Only looks for extension and user data if we saw sequence, gop
        // or picture info before.
        // This check needs to be before the "dec_ctx->in_pic_data" part.
        else if dec_ctx.saw_seqgoppic > 0 && (startcode == 0xB2 || startcode == 0xB5) {
            if !read_eau_info(enc_ctx, dec_ctx, esstream, dec_ctx.saw_seqgoppic - 1, sub)? {
                if esstream.error {
                    dec_ctx.no_bitstream_error = 0;
                }
                return Ok(false);
            }
            dec_ctx.saw_seqgoppic = 0;
            continue; // Continue to next iteration
        } else if dec_ctx.in_pic_data != 0 {
            // See comment in read_pic_data()
            if !read_pic_data(esstream)? {
                if esstream.error {
                    dec_ctx.no_bitstream_error = 0;
                }
                return Ok(false);
            }
            dec_ctx.saw_seqgoppic = 0;
            dec_ctx.in_pic_data = 0;
            continue; // Continue to next iteration
        } else {
            // Nothing found - bitstream error
            if startcode == 0xBA {
                info!("\nFound PACK header in ES data.  Probably wrong stream mode!\n");
            } else {
                info!("\nUnexpected startcode: {:02X}\n", startcode);
            }
            dec_ctx.no_bitstream_error = 0;
            return Ok(false);
        }
    }

    Ok(true)
}
/// Temporarily placed here, import from ts_core when ts module is merged
/// # Safety
/// This function is unsafe because it may dereference a raw pointer.
pub unsafe fn dump(start: *const u8, l: i32, abs_start: u64, clear_high_bit: u32) {
    let data = slice::from_raw_parts(start, l as usize);

    let mut x = 0;
    while x < l {
        debug!(msg_type = DebugMessageFlag::VERBOSE;"{:08} | ", x + abs_start as i32);

        for j in 0..16 {
            if x + j < l {
                debug!(msg_type = DebugMessageFlag::VERBOSE;"{:02X} ", data[(x + j) as usize]);
            } else {
                debug!(msg_type = DebugMessageFlag::VERBOSE;"   ");
            }
        }
        debug!(msg_type = DebugMessageFlag::VERBOSE;" | ");

        for j in 0..16 {
            if x + j < l && data[(x + j) as usize] >= b' ' {
                let ch = data[(x + j) as usize] & (if clear_high_bit != 0 { 0x7F } else { 0xFF });
                debug!(msg_type = DebugMessageFlag::VERBOSE;"{}", ch as char);
            } else {
                debug!(msg_type = DebugMessageFlag::VERBOSE;" ");
            }
        }
        debug!(msg_type = DebugMessageFlag::VERBOSE;"\n");
        x += 16;
    }
}
