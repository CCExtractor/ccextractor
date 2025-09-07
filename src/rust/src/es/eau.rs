use crate::bindings::{cc_subtitle, encoder_ctx, lib_cc_decode};
use crate::es::userdata::user_data;
use lib_ccxr::common::{BitStreamRust, BitstreamError};
use lib_ccxr::dbg_es;
use lib_ccxr::util::log::{DebugMessageFlag, ExitCause};
use lib_ccxr::{debug, fatal, info};

// Return TRUE if all was read.  FALSE if a problem occurred:
// If a bitstream syntax problem occurred the bitstream will
// point to after the problem, in case we run out of data the bitstream
// will point to where we want to restart after getting more.
pub fn read_eau_info(
    enc_ctx: &mut encoder_ctx,
    dec_ctx: &mut lib_cc_decode,
    esstream: &mut BitStreamRust,
    udtype: i32,
    sub: &mut cc_subtitle,
) -> Result<bool, BitstreamError> {
    dbg_es!("Read Extension and User Info\n");

    // We only get here after seeing that start code
    let tst = esstream.next_bytes(4)?;
    if tst.len() < 4
        || tst[0] != 0x00
        || tst[1] != 0x00
        || tst[2] != 0x01
        || (tst[3] != 0xB2 && tst[3] != 0xB5)
    {
        // (0x000001 B2||B5)
        fatal!(cause = ExitCause::Bug; "In read_eau_info: Impossible values for tst. Please file a bug report on GitHub.\n");
    }

    // The following extension_and_user_data() function makes sure that
    // user data is not evaluated twice. Should the function run out of
    // data it will make sure that esstream points to where we want to
    // continue after getting more.
    if !extension_and_user_data(enc_ctx, dec_ctx, esstream, udtype, sub)? {
        if esstream.error {
            dbg_es!("\nWarning: Retry while reading Extension and User Data!\n");
        } else {
            dbg_es!("\nBitstream problem while reading Extension and User Data!\n");
        }

        return Ok(false);
    }

    dbg_es!("Read Extension and User Info - processed\n\n");

    Ok(true)
}

// Return TRUE if the data parsing finished, FALSE otherwise.
// estream->pos is advanced. Data is only processed if esstream->error
// is FALSE, parsing can set esstream->error to TRUE.
pub fn extension_and_user_data(
    enc_ctx: &mut encoder_ctx,
    dec_ctx: &mut lib_cc_decode,
    esstream: &mut BitStreamRust,
    udtype: i32,
    sub: &mut cc_subtitle,
) -> Result<bool, BitstreamError> {
    dbg_es!("Extension and user data({})\n", udtype);

    if esstream.error || esstream.bits_left <= 0 {
        return Ok(false);
    }

    // Remember where to continue
    let eau_start_pos = esstream.pos;
    let eau_start_bpos = esstream.bpos;

    loop {
        let startcode = esstream.next_start_code()?;

        if startcode == 0xB2 || startcode == 0xB5 {
            // Skip u32 (advance bitstream)
            esstream.skip_bits(32)?;
            let dstart_pos = esstream.pos;
            let dstart_bpos = esstream.bpos;

            // Advance esstream to the next startcode.  Verify that
            // the whole extension was available and discard blocks
            // followed by PACK headers.  The latter usually indicates
            // a PS treated as an ES.
            let nextstartcode = esstream.search_start_code()?;
            if nextstartcode == 0xBA {
                info!("\nFound PACK header in ES data. Probably wrong stream mode!\n");
                esstream.error = true;
                return Ok(false);
            }

            if esstream.error {
                dbg_es!("Extension and user data - syntax problem\n");
                return Ok(false);
            }

            if esstream.bits_left < 0 {
                dbg_es!("Extension and user data - incomplete\n");
                // Restore to where we need to continue
                esstream.init_bitstream(eau_start_pos, esstream.data.len())?;
                esstream.bpos = eau_start_bpos;
                esstream.bits_left = -1; // Redundant
                return Ok(false);
            }

            if startcode == 0xB2 {
                let mut ustream = BitStreamRust::new(&esstream.data[dstart_pos..])?;
                ustream.bpos = dstart_bpos;
                ustream.bits_left =
                    (esstream.pos - dstart_pos) as i64 * 8 + (esstream.bpos - dstart_bpos) as i64;
                unsafe {
                    user_data(enc_ctx, dec_ctx, &mut ustream, udtype, sub)?;
                }
            } else {
                dbg_es!("Skip {} bytes extension data.\n", esstream.pos - dstart_pos);
            }
            // If we get here esstream points to the end of a block
            // of extension or user data.  Should we run out of data in
            // this loop this is where we want to restart after getting more.
            // eau_start = esstream->pos; (update for next iteration)
        } else {
            break;
        }
    }

    if esstream.error {
        dbg_es!("Extension and user data - syntax problem\n");
        return Ok(false);
    }
    if esstream.bits_left < 0 {
        dbg_es!("Extension and user data - incomplete\n");
        // Restore to where we need to continue
        esstream.init_bitstream(eau_start_pos, esstream.data.len())?;
        esstream.bpos = eau_start_bpos;
        esstream.bits_left = -1; // Redundant
        return Ok(false);
    }

    dbg_es!("Extension and user data - processed\n");

    // Read complete
    Ok(true)
}
