use crate::bindings::lib_cc_decode;
use crate::current_fps;
use lib_ccxr::activity::ActivityExt;
use lib_ccxr::common::{
    BitStreamRust, BitstreamError, Options, ASPECT_RATIO_TYPES, FRAMERATES_TYPES, FRAMERATES_VALUES,
};
use lib_ccxr::debug;
use lib_ccxr::util::log::{DebugMessageFlag, ExitCause};
use lib_ccxr::{dbg_es, fatal, info};

// Return TRUE if the data parsing finished, FALSE otherwise.
// estream->pos is advanced. Data is only processed if esstream->error
// is FALSE, parsing can set esstream->error to TRUE.
fn sequence_header(
    ctx: &mut lib_cc_decode,
    esstream: &mut BitStreamRust,
    ccx_options: &mut Options,
) -> Result<bool, BitstreamError> {
    dbg_es!("Sequence header");

    if esstream.error || esstream.bits_left <= 0 {
        return Ok(false);
    }

    // We only get here after seeing that start code
    if esstream.bitstream_get_num(4, true)? != 0xB3010000 {
        // LSB first (0x000001B3)
        fatal!(cause = ExitCause::Bug; "In sequence_header: read_u32(esstream) != 0xB3010000. Please file a bug report on GitHub.");
    }

    let hor_size = esstream.read_bits(12)? as u32;
    let vert_size = esstream.read_bits(12)? as u32;
    let aspect_ratio = esstream.read_bits(4)? as u32;
    let frame_rate = esstream.read_bits(4)? as u32;

    // Discard some information
    esstream.read_bits(18 + 1 + 10 + 1)?;

    // load_intra_quantiser_matrix
    if esstream.read_bits(1)? != 0 {
        esstream.skip_bits(8 * 64)?;
    }
    // load_non_intra_quantiser_matrix
    if esstream.read_bits(1)? != 0 {
        esstream.skip_bits(8 * 64)?;
    }

    if esstream.bits_left < 0 {
        return Ok(false);
    }

    // If we got the whole sequence, process
    if hor_size != ctx.current_hor_size
        || vert_size != ctx.current_vert_size
        || aspect_ratio != ctx.current_aspect_ratio
        || frame_rate != ctx.current_frame_rate
    {
        // If horizontal/vertical size, framerate and/or aspect
        // ratio are illegal, we discard the
        // whole sequence info.
        if (288..=1088).contains(&vert_size) &&
            (352..=1920).contains(&hor_size) &&
            (hor_size * 100) / vert_size >= (352 * 100) / 576 && // The weird *100 is to avoid using floats
            hor_size / vert_size <= 2 &&
            frame_rate > 0 && frame_rate < 9 &&
            aspect_ratio > 0 && aspect_ratio < 5
        {
            info!("\n\nNew video information found");
            info!("\n");
            info!(
                "[{} * {}] [AR: {}] [FR: {}]",
                hor_size,
                vert_size,
                ASPECT_RATIO_TYPES[aspect_ratio as usize],
                FRAMERATES_TYPES[frame_rate as usize]
            );
            // No newline, force the output of progressive info in picture
            // info part.
            ctx.current_progressive_sequence = 2;

            ctx.current_hor_size = hor_size;
            ctx.current_vert_size = vert_size;
            ctx.current_aspect_ratio = aspect_ratio;
            ctx.current_frame_rate = frame_rate;
            unsafe {
                current_fps = FRAMERATES_VALUES[ctx.current_frame_rate as usize];
            }
            ccx_options.activity_video_info(
                hor_size,
                vert_size,
                ASPECT_RATIO_TYPES[aspect_ratio as usize],
                FRAMERATES_TYPES[frame_rate as usize],
            );
        } else {
            dbg_es!("\nInvalid sequence header:");
            dbg_es!(
                "V: {} H: {} FR: {} AS: {}",
                vert_size,
                hor_size,
                frame_rate,
                aspect_ratio
            );
            esstream.error = true;
            return Ok(false);
        }
    }

    // Read complete
    Ok(true)
}

// Return TRUE if the data parsing finished, FALSE otherwise.
// estream->pos is advanced. Data is only processed if esstream->error
// is FALSE, parsing can set esstream->error to TRUE.
fn sequence_ext(
    ctx: &mut lib_cc_decode,
    esstream: &mut BitStreamRust,
) -> Result<bool, BitstreamError> {
    dbg_es!("Sequence extension");

    if esstream.error || esstream.bits_left <= 0 {
        return Ok(false);
    }

    // Syntax check
    if esstream.next_start_code()? != 0xB5 {
        dbg_es!("sequence_ext: syntax problem.");
        return Ok(false);
    }

    esstream.bitstream_get_num(4, true)?; // skip_u32(esstream); // Advance

    // Read extension_start_code_identifier
    let extension_id = esstream.read_bits(4)? as u32;
    if extension_id != 0x1 {
        // Sequence Extension ID
        if esstream.bits_left >= 0 {
            // When bits left, this is wrong
            esstream.error = true;
        }

        if esstream.error {
            dbg_es!("sequence_ext: syntax problem.");
        }
        return Ok(false);
    }

    // Discard some information
    esstream.skip_bits(8)?;
    let progressive_sequence = esstream.read_bits(1)? as u32;

    if progressive_sequence != ctx.current_progressive_sequence {
        ctx.current_progressive_sequence = progressive_sequence;
        info!(
            " [progressive: {}]\n\n",
            if progressive_sequence != 0 {
                "yes"
            } else {
                "no"
            }
        );
    }

    esstream.skip_bits(2 + 2 + 2 + 12 + 1 + 8 + 1 + 2 + 5)?;

    if esstream.bits_left < 0 {
        return Ok(false);
    }

    // Read complete
    Ok(true)
}
// Return TRUE if all was read.  FALSE if a problem occurred:
// If a bitstream syntax problem occurred the bitstream will
// point to after the problem, in case we run out of data the bitstream
// will point to where we want to restart after getting more.
pub fn read_seq_info(
    ctx: &mut lib_cc_decode,
    esstream: &mut BitStreamRust,
    ccx_options: &mut Options,
) -> Result<bool, BitstreamError> {
    dbg_es!("Read Sequence Info");

    // We only get here after seeing that start code
    if esstream.bitstream_get_num(4, false)? != 0xB3010000 {
        // LSB first (0x000001B3)
        fatal!(cause = ExitCause::Bug; "In read_seq_info: next_u32(esstream) != 0xB3010000. Please file a bug report on GitHub.");
    }

    // If we get here esstream points to the start of a sequence_header_code
    // should we run out of data in esstream this is where we want to restart
    // after getting more.
    let video_seq_start_pos = esstream.pos;
    let video_seq_start_end = esstream.data.len();

    sequence_header(ctx, esstream, ccx_options)?;
    sequence_ext(ctx, esstream)?;
    // FIXME: if sequence extension is missing this is not MPEG-2,
    // or broken.  Set bitstream error.
    // extension_and_user_data(esstream);

    if esstream.error {
        return Ok(false);
    }

    if esstream.bits_left < 0 {
        esstream.init_bitstream(video_seq_start_pos, video_seq_start_end)?;
        return Ok(false);
    }

    dbg_es!("Read Sequence Info - processed\n");

    Ok(true)
}
