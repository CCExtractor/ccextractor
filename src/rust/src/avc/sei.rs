use crate::avc::common_types::*;
use crate::avc::core::{copy_ccdata_to_buffer, dump};
use lib_ccxr::util::log::{DebugMessageFlag, ExitCause};
use lib_ccxr::{debug, fatal, info};

pub fn sei_rbsp(ctx: &mut AvcContextRust, seibuf: &[u8]) {
    if seibuf.is_empty() {
        return;
    }

    let seiend_idx = seibuf.len();
    let mut tbuf_idx = 0;

    while tbuf_idx < seiend_idx - 1 {
        // Use -1 because of trailing marker
        let remaining_slice = &seibuf[tbuf_idx..];
        let consumed = sei_message(ctx, remaining_slice);
        if consumed == 0 {
            // Prevent infinite loop if sei_message doesn't consume any bytes
            break;
        }
        tbuf_idx += consumed;
    }

    if tbuf_idx == seiend_idx - 1 {
        if seibuf[tbuf_idx] != 0x80 {
            info!("Strange rbsp_trailing_bits value: {:02X}", seibuf[tbuf_idx]);
        } else {
            debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "");
        }
    } else {
        // TODO: This really really looks bad
        info!("WARNING: Unexpected SEI unit length...trying to continue.");
        // temp_debug = 1; // This appears to be a global debug flag - omitted in Rust version
        info!("Failed block (at sei_rbsp) was:");

        // Call dump function using the existing unsafe dump function
        unsafe {
            dump(seibuf.as_ptr(), seibuf.len() as i32, 0, 0);
        }

        ctx.num_unexpected_sei_length += 1;
    }
}
// This combines sei_message() and sei_payload().
pub fn sei_message(ctx: &mut AvcContextRust, seibuf: &[u8]) -> usize {
    let mut seibuf_idx = 0;

    if seibuf.is_empty() {
        return 0;
    }

    let mut payload_type = 0;
    while seibuf_idx < seibuf.len() && seibuf[seibuf_idx] == 0xff {
        payload_type += 255;
        seibuf_idx += 1;
    }

    if seibuf_idx >= seibuf.len() {
        return seibuf_idx;
    }

    payload_type += seibuf[seibuf_idx] as i32;
    seibuf_idx += 1;

    let mut payload_size = 0;
    while seibuf_idx < seibuf.len() && seibuf[seibuf_idx] == 0xff {
        payload_size += 255;
        seibuf_idx += 1;
    }

    if seibuf_idx >= seibuf.len() {
        return seibuf_idx;
    }

    payload_size += seibuf[seibuf_idx] as i32;
    seibuf_idx += 1;

    let mut broken = false;
    let payload_start = seibuf_idx;
    seibuf_idx += payload_size as usize;

    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "Payload type: {} size: {} - ", payload_type, payload_size);

    if seibuf_idx > seibuf.len() {
        // TODO: What do we do here?
        broken = true;
        if payload_type == 4 {
            debug!(msg_type = DebugMessageFlag::VERBOSE; "Warning: Subtitles payload seems incorrect (too long), continuing but it doesn't look good..");
        } else {
            debug!(msg_type = DebugMessageFlag::VERBOSE; "Warning: Non-subtitles payload seems incorrect (too long), continuing but it doesn't look good..");
        }
    }
    debug!(msg_type = DebugMessageFlag::VERBOSE; "");

    // Ignore all except user_data_registered_itu_t_t35() payload
    if !broken && payload_type == 4 && payload_start + payload_size as usize <= seibuf.len() {
        let payload_data = &seibuf[payload_start..payload_start + payload_size as usize];
        user_data_registered_itu_t_t35(ctx, payload_data);
    }

    seibuf_idx
}
pub fn user_data_registered_itu_t_t35(ctx: &mut AvcContextRust, userbuf: &[u8]) {
    if userbuf.len() < 3 {
        return;
    }

    let mut tbuf_idx = 0;
    let user_data_len: usize;
    let local_cc_count: usize;

    let itu_t_t35_country_code = userbuf[tbuf_idx];
    tbuf_idx += 1;

    let itu_t_35_provider_code = (userbuf[tbuf_idx] as u16) * 256 + (userbuf[tbuf_idx + 1] as u16);
    tbuf_idx += 2;

    // ANSI/SCTE 128 2008:
    // itu_t_t35_country_code == 0xB5
    // itu_t_35_provider_code == 0x0031
    // see spec for details - no example -> no support

    // Example files (sample.ts, ...):
    // itu_t_t35_country_code == 0xB5
    // itu_t_35_provider_code == 0x002F
    // user_data_type_code == 0x03 (cc_data)
    // user_data_len == next byte (length after this byte up to (incl) marker.)
    // cc_data struct (CEA-708)
    // marker == 0xFF

    if itu_t_t35_country_code != 0xB5 {
        info!("Not a supported user data SEI");
        info!("  itu_t_35_country_code: {:02x}", itu_t_t35_country_code);
        return;
    }

    match itu_t_35_provider_code {
        0x0031 => {
            // ANSI/SCTE 128
            debug!(msg_type = DebugMessageFlag::VERBOSE; "Caption block in ANSI/SCTE 128...");
            if tbuf_idx + 4 <= userbuf.len()
                && userbuf[tbuf_idx] == 0x47
                && userbuf[tbuf_idx + 1] == 0x41
                && userbuf[tbuf_idx + 2] == 0x39
                && userbuf[tbuf_idx + 3] == 0x34
            {
                // ATSC1_data() - GA94

                debug!(msg_type = DebugMessageFlag::VERBOSE; "ATSC1_data()...");
                tbuf_idx += 4;

                if tbuf_idx >= userbuf.len() {
                    return;
                }

                let user_data_type_code = userbuf[tbuf_idx];
                tbuf_idx += 1;

                match user_data_type_code {
                    0x03 => {
                        debug!(msg_type = DebugMessageFlag::VERBOSE; "cc_data (finally)!");
                        /*
                          cc_count = 2; // Forced test
                          process_cc_data_flag = (*tbuf & 2) >> 1;
                          mandatory_1 = (*tbuf & 1);
                          mandatory_0 = (*tbuf & 4) >>2;
                          if (!mandatory_1 || mandatory_0)
                          {
                          printf ("Essential tests not passed.\n");
                          break;
                          }
                        */
                        if tbuf_idx >= userbuf.len() {
                            return;
                        }

                        local_cc_count = (userbuf[tbuf_idx] & 0x1F) as usize;

                        /*
                        let process_cc_data_flag = (userbuf[tbuf_idx] & 0x40) >> 6;
                        if (!process_cc_data_flag)
                        {
                        mprint ("process_cc_data_flag == 0, skipping this caption block.\n");
                        break;
                        } */
                        /*
                        The following tests are not passed in Comcast's sample videos. *tbuf here is always 0x41.
                        if (! (*tbuf & 0x80)) // First bit must be 1
                        {
                        printf ("Fixed bit should be 1, but it's 0 - skipping this caption block.\n");
                        break;
                        }
                        if (*tbuf & 0x20) // Third bit must be 0
                        {
                        printf ("Fixed bit should be 0, but it's 1 - skipping this caption block.\n");
                        break;
                        } */
                        tbuf_idx += 1;
                        /*
                        Another test that the samples ignore. They contain 00!
                        if (*tbuf!=0xFF)
                        {
                        printf ("Fixed value should be 0xFF, but it's %02X - skipping this caption block.\n", *tbuf);
                        } */
                        // OK, all checks passed!
                        tbuf_idx += 1;
                        let cc_tmp_data_start = tbuf_idx;

                        /* TODO: I don't think we have user_data_len here
                        if (cc_count*3+3 != user_data_len)
                        fatal(CCX_COMMON_EXIT_BUG_BUG,
                        "Syntax problem: user_data_len != cc_count*3+3."); */

                        // Enough room for CC captions?
                        if cc_tmp_data_start + local_cc_count * 3 >= userbuf.len() {
                            fatal!(cause = ExitCause::Bug; "Syntax problem: Too many caption blocks.");
                        }

                        if cc_tmp_data_start + local_cc_count * 3 < userbuf.len()
                            && userbuf[cc_tmp_data_start + local_cc_count * 3] != 0xFF
                        {
                            // See GitHub Issue #1001 for the related change
                            info!("\rWarning! Syntax problem: Final 0xFF marker missing. Continuing...");
                            return; // Skip Block
                        }

                        // Save the data and process once we know the sequence number
                        if ((ctx.cc_count as usize + local_cc_count) * 3) + 1 > ctx.cc_databufsize {
                            let new_size = ((ctx.cc_count as usize + local_cc_count) * 6) + 1;
                            ctx.cc_data.resize(new_size, 0);
                            ctx.cc_databufsize = new_size;
                        }

                        // Copy new cc data into cc_data
                        let cc_tmp_data = &userbuf[cc_tmp_data_start..];
                        copy_ccdata_to_buffer(ctx, cc_tmp_data, local_cc_count);
                    }
                    0x06 => {
                        debug!(msg_type = DebugMessageFlag::VERBOSE; "bar_data (unsupported for now)");
                    }
                    _ => {
                        debug!(msg_type = DebugMessageFlag::VERBOSE; "SCTE/ATSC reserved.");
                    }
                }
            } else if tbuf_idx + 4 <= userbuf.len()
                && userbuf[tbuf_idx] == 0x44
                && userbuf[tbuf_idx + 1] == 0x54
                && userbuf[tbuf_idx + 2] == 0x47
                && userbuf[tbuf_idx + 3] == 0x31
            { // afd_data() - DTG1
                 // Active Format Description Data. Actually unrelated to captions. Left
                 // here in case we want to do some reporting eventually. From specs:
                 // "Active Format Description (AFD) should be included in video user
                 // data whenever the rectangular picture area containing useful
                 // information does not extend to the full height or width of the coded
                 // frame. AFD data may also be included in user data when the
                 // rectangular picture area containing
                 // useful information extends to the fullheight and width of the
                 // coded frame."
            } else {
                debug!(msg_type = DebugMessageFlag::VERBOSE; "SCTE/ATSC reserved.");
            }
        }
        0x002F => {
            debug!(msg_type = DebugMessageFlag::VERBOSE; "ATSC1_data() - provider_code = 0x002F");

            if tbuf_idx >= userbuf.len() {
                return;
            }

            let user_data_type_code = userbuf[tbuf_idx];
            if user_data_type_code != 0x03 {
                debug!(msg_type = DebugMessageFlag::VERBOSE;
                       "Not supported  user_data_type_code: {:02x}", user_data_type_code);
                return;
            }
            tbuf_idx += 1;

            if tbuf_idx >= userbuf.len() {
                return;
            }

            user_data_len = userbuf[tbuf_idx] as usize;
            tbuf_idx += 1;

            if tbuf_idx >= userbuf.len() {
                return;
            }

            local_cc_count = (userbuf[tbuf_idx] & 0x1F) as usize;
            let process_cc_data_flag = (userbuf[tbuf_idx] & 0x40) >> 6;

            if process_cc_data_flag == 0 {
                info!("process_cc_data_flag == 0, skipping this caption block.");
                return;
            }

            let cc_tmp_data_start = tbuf_idx + 2;

            if local_cc_count * 3 + 3 != user_data_len {
                fatal!(cause = ExitCause::Bug; "Syntax problem: user_data_len != cc_count*3+3.");
            }

            // Enough room for CC captions?
            if cc_tmp_data_start + local_cc_count * 3 >= userbuf.len() {
                fatal!(cause = ExitCause::Bug; "Syntax problem: Too many caption blocks.");
            }

            if cc_tmp_data_start + local_cc_count * 3 < userbuf.len()
                && userbuf[cc_tmp_data_start + local_cc_count * 3] != 0xFF
            {
                fatal!(cause = ExitCause::Bug; "Syntax problem: Final 0xFF marker missing.");
            }

            // Save the data and process once we know the sequence number
            if (((local_cc_count + ctx.cc_count as usize) * 3) + 1) > ctx.cc_databufsize {
                let new_size = ((local_cc_count + ctx.cc_count as usize) * 6) + 1;
                ctx.cc_data.resize(new_size, 0);
                ctx.cc_databufsize = new_size;
            }

            // Copy new cc data into cc_data - replace command below.
            let cc_tmp_data = &userbuf[cc_tmp_data_start..];
            copy_ccdata_to_buffer(ctx, cc_tmp_data, local_cc_count);

            // dump(tbuf,user_data_len-1,0);
        }
        _ => {
            info!("Not a supported user data SEI");
            info!("  itu_t_35_provider_code: {:04x}", itu_t_35_provider_code);
        }
    }
}
