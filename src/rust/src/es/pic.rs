use crate::bindings::{cc_subtitle, encoder_ctx, lib_cc_decode};
use crate::libccxr_exports::time::{ccxr_get_fts, ccxr_print_debug_timing, ccxr_set_fts};
use crate::{anchor_hdcc, process_hdcc};
use crate::{ccx_options, frames_since_ref_time, fts_at_gop_start, total_frames_count};
use lib_ccxr::common::{BitStreamRust, BitstreamError, FrameType};
use lib_ccxr::debug;
use lib_ccxr::util::log::DebugMessageFlag;
use lib_ccxr::util::log::ExitCause;
use lib_ccxr::{dbg_es, fatal};
use std::os::raw::c_int;

// Return TRUE if the data parsing finished, FALSE otherwise.
// estream->pos is advanced. Data is only processed if esstream->error
// is FALSE, parsing can set esstream->error to TRUE.
fn pic_header(
    ctx: &mut lib_cc_decode,
    esstream: &mut BitStreamRust,
) -> Result<bool, BitstreamError> {
    dbg_es!("PIC header");

    if esstream.error || esstream.bits_left <= 0 {
        return Ok(false);
    }

    // We only get here after seeing that start code
    if esstream.bitstream_get_num(4, true)? != 0x00010000 {
        // LSB first (0x00000100)
        fatal!(cause = ExitCause::Bug; "In pic_header: read_u32(esstream) != 0x00010000. Please file a bug report on GitHub.");
    }

    ctx.temporal_reference = esstream.read_bits(10)? as i32;
    ctx.picture_coding_type = esstream.read_bits(3)? as _;

    if ctx.picture_coding_type == FrameType::IFrame as _ {
        unsafe {
            // Write I-Frame in ffprobe format for easy comparison
            ctx.num_key_frames += 1;
            debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "key_frame=1|pkt_pts={}|pict_type=I", ((*ctx.timing).current_pts));
        }
    }

    // Discard vbv_delay
    esstream.skip_bits(16)?;

    // Discard some information
    if ctx.picture_coding_type == FrameType::PFrame as _
        || ctx.picture_coding_type == FrameType::BFrame as _
    {
        esstream.skip_bits(4)?;
    }
    if ctx.picture_coding_type == FrameType::BFrame as _ {
        esstream.skip_bits(4)?;
    }

    // extra_information
    while esstream.read_bits(1)? == 1 {
        esstream.skip_bits(8)?;
    }

    if esstream.bits_left < 0 {
        return Ok(false);
    }

    if !(ctx.picture_coding_type == FrameType::IFrame as _
        || ctx.picture_coding_type == FrameType::PFrame as _
        || ctx.picture_coding_type == FrameType::BFrame as _)
    {
        if esstream.bits_left >= 0 {
            // When bits left, this is wrong
            esstream.error = true;
        }

        if esstream.error {
            dbg_es!("pic_header: syntax problem.");
        }
        return Ok(false);
    }

    Ok(true)
}

// Return TRUE if the data parsing finished, FALSE otherwise.
// estream->pos is advanced. Data is only processed if esstream->error
// is FALSE, parsing can set esstream->error to TRUE.
fn pic_coding_ext(
    ctx: &mut lib_cc_decode,
    esstream: &mut BitStreamRust,
) -> Result<bool, BitstreamError> {
    dbg_es!("Picture coding extension {}", esstream.bits_left);

    if esstream.error || esstream.bits_left <= 0 {
        return Ok(false);
    }

    // Syntax check
    if esstream.next_start_code()? != 0xB5 {
        dbg_es!("pic_coding_ext: syntax problem.");
        return Ok(false);
    }

    esstream.bitstream_get_num(4, true)?; // skip_u32(esstream); // Advance

    // Read extension_start_code_identifier
    let extension_id = esstream.read_bits(4)? as u32;
    if extension_id != 0x8 {
        // Picture Coding Extension ID
        if esstream.bits_left >= 0 {
            // When bits left, this is wrong
            esstream.error = true;
        }

        if esstream.error {
            dbg_es!("pic_coding_ext: syntax problem.");
        }
        return Ok(false);
    }

    // Discard some information
    esstream.skip_bits(4 * 4 + 2)?;
    ctx.picture_structure = esstream.read_bits(2)? as u32;
    ctx.top_field_first = esstream.read_bits(1)? as u32;
    esstream.skip_bits(5)?;
    ctx.repeat_first_field = esstream.read_bits(1)? as u32;
    esstream.skip_bits(1)?; // chroma
    ctx.progressive_frame = esstream.read_bits(1)? as u32;
    let composite_display = esstream.read_bits(1)? as u32;
    if composite_display != 0 {
        esstream.skip_bits(1 + 3 + 1 + 7 + 8)?;
    }

    if esstream.bits_left < 0 {
        return Ok(false);
    }

    dbg_es!("Picture coding extension - processed");

    // Read complete
    Ok(true)
}

// Return TRUE if all was read.  FALSE if a problem occurred:
// If a bitstream syntax problem occurred the bitstream will
// point to after the problem, in case we run out of data the bitstream
// will point to where we want to restart after getting more.
/// # Safety
/// This function is unsafe because it calls C functions like `process_hdcc` or `anchor_hdcc`
pub unsafe fn read_pic_info(
    enc_ctx: &mut encoder_ctx,
    dec_ctx: &mut lib_cc_decode,
    esstream: &mut BitStreamRust,
    sub: &mut cc_subtitle,
) -> Result<bool, BitstreamError> {
    dbg_es!("Read PIC Info");

    // We only get here after seeing that start code
    if esstream.bitstream_get_num(4, false)? != 0x00010000 {
        // LSB first (0x00000100)
        fatal!(cause = ExitCause::Bug; "In read_pic_info: next_u32(esstream) != 0x00010000. Please file a bug report on GitHub.");
    }

    // If we get here esstream points to the start of a group_start_code
    // should we run out of data in esstream this is where we want to restart
    // after getting more.
    let pic_info_start_pos = esstream.pos;
    let pic_info_start_end = esstream.data.len();

    pic_header(dec_ctx, esstream)?;
    pic_coding_ext(dec_ctx, esstream)?;

    if esstream.error {
        return Ok(false);
    }

    if esstream.bits_left < 0 {
        esstream.init_bitstream(pic_info_start_pos, pic_info_start_end)?;
        return Ok(false);
    }

    // A new anchor frame - flush buffered caption data. Might be flushed
    // in GOP header already.
    if (dec_ctx.picture_coding_type == FrameType::IFrame as _
        || dec_ctx.picture_coding_type == FrameType::PFrame as _)
        && (((dec_ctx.picture_structure != 0x1) && (dec_ctx.picture_structure != 0x2))
            || (dec_ctx.temporal_reference != (*dec_ctx.timing).current_tref))
    {
        // NOTE: process_hdcc() needs to be called before set_fts() as it
        // uses fts_now to re-create the timeline !!!!!
        if dec_ctx.has_ccdata_buffered != 0 {
            process_hdcc(enc_ctx, dec_ctx, sub);
        }
        anchor_hdcc(dec_ctx, dec_ctx.temporal_reference);
    }

    (*dec_ctx.timing).current_tref = dec_ctx.temporal_reference;
    (*dec_ctx.timing).current_picture_coding_type = dec_ctx.picture_coding_type;

    // We mostly use PTS, but when the GOP mode is enabled do not set
    // the FTS time here.
    if ccx_options.use_gop_as_pts != 1 {
        ccxr_set_fts(dec_ctx.timing); // Initialize fts
    }

    // Set min_pts/sync_pts according to the current time stamp.
    // Use fts_at_gop_start as reference when a GOP header was seen
    // since the last frame 0. If not this is most probably a
    // TS without GOP headers but with USER DATA after each picture
    // header. Use the current FTS values as reference.
    // Note: If a GOP header was present the reference time is from
    // the beginning of the GOP, otherwise it is now.
    if dec_ctx.temporal_reference == 0 {
        dec_ctx.last_gop_length = dec_ctx.maxtref + 1;
        dec_ctx.maxtref = dec_ctx.temporal_reference;

        // frames_since_ref_time is used in set_fts()

        if dec_ctx.saw_gop_header != 0 {
            // This time (fts_at_gop_start) that was set in the
            // GOP header and it might be off by one GOP. See the comment there.
            frames_since_ref_time = dec_ctx.frames_since_last_gop; // Should this be 0?
        } else {
            // No GOP header, use the current values
            fts_at_gop_start = ccxr_get_fts(dec_ctx.timing, dec_ctx.current_field);
            frames_since_ref_time = 0;
        }

        if ccx_options.debug_mask & 4 != 0 {
            debug!(msg_type = DebugMessageFlag::TIME; "\nNew temporal reference:");
            ccxr_print_debug_timing(dec_ctx.timing);
        }

        dec_ctx.saw_gop_header = 0; // Reset the value
    }

    if dec_ctx.saw_gop_header == 0 && dec_ctx.picture_coding_type == FrameType::IFrame as _ {
        // A new GOP begins with an I-frame. Lets hope there are
        // never more than one per GOP
        dec_ctx.frames_since_last_gop = 0;
    }

    // Set maxtref
    if dec_ctx.temporal_reference > dec_ctx.maxtref {
        dec_ctx.maxtref = dec_ctx.temporal_reference;
        if dec_ctx.maxtref + 1 > dec_ctx.max_gop_length {
            dec_ctx.max_gop_length = dec_ctx.maxtref + 1;
        }
    }

    let mut extraframe = 0u32;
    if dec_ctx.repeat_first_field != 0 {
        dec_ctx.pulldownfields += 1;
        dec_ctx.total_pulldownfields += 1;
        if dec_ctx.current_progressive_sequence != 0 || dec_ctx.total_pulldownfields.is_multiple_of(2) {
            extraframe = 1;
        }
        if dec_ctx.current_progressive_sequence != 0 && dec_ctx.top_field_first != 0 {
            extraframe = 2;
        }
        debug!(
            msg_type = DebugMessageFlag::VIDEO_STREAM;
            "Pulldown: total pd fields: {} - {} extra frames",
            dec_ctx.total_pulldownfields,
            extraframe
        );
    }

    dec_ctx.total_pulldownframes += extraframe;
    total_frames_count += 1 + extraframe;
    dec_ctx.frames_since_last_gop += (1 + extraframe) as c_int;
    frames_since_ref_time += (1 + extraframe) as c_int;

    dbg_es!("Read PIC Info - processed\n");

    Ok(true)
}

// Return TRUE if all was read.  FALSE if a problem occurred:
// If a bitstream syntax problem occurred the bitstream will
// point to after the problem, in case we run out of data the bitstream
// will point to where we want to restart after getting more.
pub fn read_pic_data(esstream: &mut BitStreamRust) -> Result<bool, BitstreamError> {
    dbg_es!("Read PIC Data\n");

    let startcode = esstream.next_start_code()?;

    // Possibly the last call to this function ended with the last
    // bit of the slice? I.e. in_pic_data is still true, but we are
    // seeing the next start code.

    // We only get here after seeing that start code
    if !(0x01..=0xAF).contains(&startcode) {
        dbg_es!("Read Pic Data - processed0\n");

        return Ok(true);
    }

    // If we get here esstream points to the start of a slice_start_code
    // should we run out of data in esstream this is where we want to restart
    // after getting more.
    let mut slice_start_pos = esstream.pos;
    let mut slice_start_bpos = esstream.bpos;

    loop {
        let startcode = esstream.next_start_code()?;
        // Syntax check
        if startcode == 0xB4 {
            if esstream.bits_left < 0 {
                esstream.init_bitstream(slice_start_pos, esstream.data.len())?;
                esstream.bpos = slice_start_bpos;
            }

            if esstream.error {
                dbg_es!("read_pic_data: syntax problem.\n");
            } else {
                dbg_es!("read_pic_data: reached end of bitstream.\n");
            }

            return Ok(false);
        }

        slice_start_pos = esstream.pos; // No need to come back
        slice_start_bpos = esstream.bpos;

        if (0x01..=0xAF).contains(&startcode) {
            esstream.skip_bits(32)?; // Advance bitstream
            esstream.search_start_code()?; // Skip this slice
        } else {
            break;
        }
    }

    if esstream.bits_left < 0 {
        esstream.init_bitstream(slice_start_pos, esstream.data.len())?;
        esstream.bpos = slice_start_bpos;
        return Ok(false);
    }

    dbg_es!("Read Pic Data - processed\n");

    Ok(true)
}
