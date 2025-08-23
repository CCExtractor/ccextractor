use crate::bindings::{cc_subtitle, encoder_ctx, lib_cc_decode, process_hdcc};
use crate::libccxr_exports::time::{
    ccxr_get_fts_max, ccxr_print_debug_timing, ccxr_set_current_pts, ccxr_set_fts,
    write_gop_time_code,
};
use crate::{
    ccx_options, current_fps, first_gop_time, frames_since_ref_time, fts_at_gop_start, gop_time,
    total_frames_count, MPEG_CLOCK_FREQ,
};
use lib_ccxr::common::{BitStreamRust, BitstreamError};
use lib_ccxr::dbg_es;
use lib_ccxr::time::c_functions::{calculate_ms_gop_time, gop_accepted, print_mstime_static};
use lib_ccxr::time::{GopTimeCode, Timestamp};
use lib_ccxr::util::log::{DebugMessageFlag, ExitCause};
use lib_ccxr::{debug, fatal, info};

// Return TRUE if all was read.  FALSE if a problem occurred:
// If a bitstream syntax problem occurred the bitstream will
// point to after the problem, in case we run out of data the bitstream
// will point to where we want to restart after getting more.
/// # Safety
/// This function is unsafe because it calls `gop_header`.
pub unsafe fn read_gop_info(
    enc_ctx: &mut encoder_ctx,
    dec_ctx: &mut lib_cc_decode,
    esstream: &mut BitStreamRust,
    sub: &mut cc_subtitle,
) -> Result<bool, BitstreamError> {
    dbg_es!("Read GOP Info");

    // We only get here after seeing that start code
    if esstream.bitstream_get_num(4, false)? != 0xB8010000 {
        // LSB first (0x000001B8)
        fatal!(cause = ExitCause::Bug; "In read_gop_info: next_u32(esstream) != 0xB8010000. Please file a bug report on GitHub.");
    }

    // If we get here esstream points to the start of a group_start_code
    // should we run out of data in esstream this is where we want to restart
    // after getting more.
    let gop_info_start_pos = esstream.pos;
    let gop_info_start_bpos = esstream.bpos;

    gop_header(enc_ctx, dec_ctx, esstream, sub)?;
    // extension_and_user_data(esstream);

    if esstream.error {
        return Ok(false);
    }

    if esstream.bits_left < 0 {
        esstream.init_bitstream(gop_info_start_pos, gop_info_start_bpos as usize)?;
        return Ok(false);
    }

    dbg_es!("Read GOP Info - processed\n");

    Ok(true)
}

// Return TRUE if the data parsing finished, FALSE otherwise.
// estream->pos is advanced. Data is only processed if esstream->error
// is FALSE, parsing can set esstream->error to TRUE.
unsafe fn gop_header(
    enc_ctx: &mut encoder_ctx,
    dec_ctx: &mut lib_cc_decode,
    esstream: &mut BitStreamRust,
    sub: &mut cc_subtitle,
) -> Result<bool, BitstreamError> {
    dbg_es!("GOP header");

    if esstream.error || esstream.bits_left <= 0 {
        return Ok(false);
    }

    // We only get here after seeing that start code
    if esstream.bitstream_get_num(4, true)? != 0xB8010000 {
        // LSB first (0x000001B8)
        fatal!(cause = ExitCause::Bug; "In gop_header: read_u32(esstream) != 0xB8010000. Please file a bug report on GitHub.");
    }

    let drop_frame_flag = esstream.read_bits(1)? as u32;
    let mut gtc = GopTimeCode {
        drop_frame: drop_frame_flag != 0,
        time_code_hours: esstream.read_bits(5)? as u8,
        time_code_minutes: esstream.read_bits(6)? as u8,
        time_code_seconds: 0,
        time_code_pictures: 0,
        timestamp: Default::default(),
    };
    esstream.skip_bits(1)?; // Marker bit
    gtc.time_code_seconds = esstream.read_bits(6)? as u8;
    gtc.time_code_pictures = esstream.read_bits(6)? as u8;
    calculate_ms_gop_time(gtc);

    if esstream.bits_left < 0 {
        return Ok(false);
    }

    if gop_accepted(gtc) {
        // Do GOP padding during GOP header. The previous GOP and all
        // included captions are written. Use the current GOP time to
        // do the padding.

        // Flush buffered cc blocks before doing the housekeeping
        if dec_ctx.has_ccdata_buffered != 0 {
            process_hdcc(enc_ctx, dec_ctx, sub);
        }

        // Last GOPs pulldown frames
        if (dec_ctx.current_pulldownfields > 0) != (dec_ctx.pulldownfields > 0) {
            dec_ctx.current_pulldownfields = dec_ctx.pulldownfields;
            dbg_es!(
                "Pulldown: {}",
                if dec_ctx.pulldownfields != 0 {
                    "on"
                } else {
                    "off"
                }
            );
            if dec_ctx.pulldownfields != 0 {
                dbg_es!(" - {} fields in last GOP", dec_ctx.pulldownfields);
            }
            dbg_es!("");
        }
        dec_ctx.pulldownfields = 0;

        // Report synchronization jumps between GOPs. Warn if there
        // are 20% or more deviation.
        if (ccx_options.debug_mask & 4 != 0)
            && ((gtc.timestamp.millis() - gop_time.ms // more than 20% longer
                > (dec_ctx.frames_since_last_gop as f64 * 1000.0 / current_fps * 1.2) as i64)
                || (gtc.timestamp.millis() - gop_time.ms // or 20% shorter
                    < (dec_ctx.frames_since_last_gop as f64 * 1000.0 / current_fps * 0.8) as i64))
            && first_gop_time.inited != 0
        {
            info!("\rWarning: Jump in GOP timing.");
            info!(
                "  (old) {}",
                print_mstime_static(Timestamp::from_millis(gop_time.ms), ':')
            );
            info!(
                "  +  {} ({}F)",
                print_mstime_static(
                    Timestamp::from_millis(
                        (dec_ctx.frames_since_last_gop as f64 * 1000.0 / current_fps) as i64
                    ),
                    ':'
                ),
                dec_ctx.frames_since_last_gop
            );
            info!("  !=  (new) {}", print_mstime_static(gtc.timestamp, ':'));
        }

        if first_gop_time.inited == 0 {
            first_gop_time = write_gop_time_code(Some(gtc));

            // It needs to be "+1" because the frame count starts at 0 and we
            // need the length of all frames.
            if total_frames_count == 0 {
                // If this is the first frame there cannot be an offset
                (*dec_ctx.timing).fts_fc_offset = 0;
                // first_gop_time.ms stays unchanged
            } else {
                (*dec_ctx.timing).fts_fc_offset =
                    ((total_frames_count + 1) as f64 * 1000.0 / current_fps) as i64;
                // Compensate for those written before
                first_gop_time.ms -= (*dec_ctx.timing).fts_fc_offset;
            }

            debug!(msg_type = DebugMessageFlag::TIME; "\nFirst GOP time: {:02}:{:02}:{:02}:{:03} {:+}ms",
                      gtc.time_code_hours,
                      gtc.time_code_minutes,
                      gtc.time_code_seconds,
                      (1000.0 * gtc.time_code_pictures as f64 / current_fps) as u32,
                      (*dec_ctx.timing).fts_fc_offset);
        }

        gop_time = write_gop_time_code(Some(gtc));

        dec_ctx.frames_since_last_gop = 0;
        // Indicate that we read a gop header (since last frame number 0)
        dec_ctx.saw_gop_header = 1;

        // If we use GOP timing, reconstruct the PTS from the GOP
        if ccx_options.use_gop_as_pts == 1 {
            ccxr_set_current_pts(
                dec_ctx.timing,
                gtc.timestamp.millis() * (MPEG_CLOCK_FREQ as i64 / 1000),
            );
            (*dec_ctx.timing).current_tref = 0;
            frames_since_ref_time = 0;
            ccxr_set_fts(dec_ctx.timing);
            fts_at_gop_start = ccxr_get_fts_max(dec_ctx.timing);
        } else {
            // FIXME: Wrong when PTS are not increasing but are identical
            // throughout the GOP and then jump to the next time for the
            // next GOP.
            // This effect will also lead to captions being one GOP early
            // for DVD captions.
            fts_at_gop_start = ccxr_get_fts_max(dec_ctx.timing) + (1000.0 / current_fps) as i64;
        }

        if ccx_options.debug_mask & 4 != 0 {
            debug!(msg_type = DebugMessageFlag::TIME; "\nNew GOP:");
            debug!(msg_type = DebugMessageFlag::TIME;  "\nDrop frame flag: {}:", drop_frame_flag);
            ccxr_print_debug_timing(dec_ctx.timing);
        }
    }

    Ok(true)
}
