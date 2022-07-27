mod timing;
mod encoder;
mod utility;
mod constants;
mod context;
mod xds;

use crate::{bindings::*, utils::is_true};
use crate::decoder608::timing::print_mstime_buff;
use log::debug;
use std::slice;



extern "C" {
    static mut ts_start_of_xds: i64;
    static mut in_xds_mode: i32;
}


#[no_mangle]
pub extern "C" fn rx_process608(
    data: *const ::std::os::raw::c_uchar,
    length: ::std::os::raw::c_int,
    private_data: *mut ::std::os::raw::c_void,
    sub: *mut cc_subtitle,
) -> ::std::os::raw::c_int
{
    let mut report: Option<&mut ccx_decoder_608_report> = None;
    let dec_ctx = unsafe{ &mut *(private_data as *mut lib_cc_decode)};
    let mut context: Option<&mut ccx_decoder_608_context>;

        match (dec_ctx.current_field, dec_ctx.extract) {
            (1, 1 | 12) => {
                context = Some(unsafe {&mut *(dec_ctx.context_cc608_field_1 as *mut ccx_decoder_608_context)});
            }
            (2, 2 | 12) => {
                context = Some(unsafe{ &mut *(dec_ctx.context_cc608_field_2 as *mut ccx_decoder_608_context)});
            }
            (2, 1) => {
                context = None;
            }
            (_, _) => {
                return -1;
            }
        }

        if let Some(ref mut context) = context {
            report = Some(unsafe { &mut *(context.report as *mut ccx_decoder_608_report)});
            context.bytes_processed_608 += length as i64;
        }

        if data.is_null() {
            return -1;
        }

        let data_vec = unsafe{slice::from_raw_parts(data,length as usize)};
        let mut loop_counter = 0;
        for mut i in (0..length).step_by(2) {
            loop_counter = i;
            let (hi,lo): (u8,u8);
            let mut wrote_to_screen = false;

            hi = data_vec[i as usize] & 0x7F; // Get rid of parity bit
            lo = data_vec[(i+1) as usize] & 0x7F; // Get rid of parity bit

            if hi == 0 && lo == 0 { continue; } // Just padding

                if (0x10..=0x1e).contains(&hi) {
                    unsafe {
                        let mut ch = if hi <= 0x17 {1} else {2};
                        match context {
                            Some(ref context) => {
                                if context.my_field == 2 {
                                    ch += 2;
                                }
                            }
                            None => {
                                ch += 2;
                            }
                        }
                        if let Some(ref mut report) = report {
                            report.cc_channels[ch-1] = 1;
                        }
                    }
                }
                let dad = check_context_field(&context);
                if (0x01..=0x0E).contains(&hi) && (context.is_none() || check_context_field(&context) == 2 ) { //XDS can only exist on field 2
                        if let Some(ref mut context) = context {
                            context.channel = 3;
                        }
                        unsafe {
                        if !is_true(in_xds_mode) {
                            ts_start_of_xds = (*dec_ctx.timing).get_fts(dec_ctx.current_field as u8);
                            in_xds_mode = 1;
                        }
                        }

                        if let Some(ref mut report) = report {
                           report.set_xds(1);
                        }
                }
                if hi == 0x0F && unsafe { is_true(in_xds_mode)}
                    && (context.is_none() || check_context_field(&context) == 2) {
                    unsafe {
                    in_xds_mode = 0;
                    // TODO: do_end_of_xds
                        (*dec_ctx.xds_ctx).do_end_of_xds(sub,lo);

                    if let Some(ref mut context) = context{
                         context.channel = context.new_channel; //switch from channel 3
                    }
                        continue;
                    }
                }

                if (0x10..=0x1F).contains(&hi) {
                    // Non-character code or special/extended char
                    // http://www.theneitherworld.com/mcpoodle/SCC_TOOLS/DOCS/CC_CODES.HTML
                    // http://www.theneitherworld.com/mcpoodle/SCC_TOOLS/DOCS/CC_CHARS.HTML
                   if context.is_none() || check_context_field(&context) == 2  {
                       unsafe{in_xds_mode = 0;}
                   } // Back to normal (CEA 608-8.6.2)

                    match context {
                        None => { continue; } // Not XDS and we don't have a writebuffer, nothing else would have an effect
                        Some(ref mut context) => {

                            // We were writing characters before, start a new line for
                            // diagnostic output from disCommand()
                            if context.textprinted == 1 {
                                // TODO ccx_common_logging.debug_ftn...
                                //ccx_common_logging.debug_ftn(0x10, "\n");
                                context.textprinted = 0;
                            }

                            if context.last_c1 == hi && context.last_c2 == lo {
                                // Duplicate dual code, discard. Correct to do it only in
                                // non-XDS, XDS codes shall not be repeated.

                                // TODO ccx_common_logging.debug_ftn...
                                //ccx_common_logging.debug_ftn(0x10, "Skipping command %02X, %02X Duplicate\n", hi, lo);
                                // Ignore only the first repetition
                                context.last_c1 = 255;
                                context.last_c2 = 255;
                                continue;
                            }
                            context.last_c1 = hi;
                            context.last_c2 = lo;
                            // TODO wrote_to_screen
                            wrote_to_screen = context.disCommand(hi,lo, sub);
                            if unsafe{ is_true((*sub).got_output)} {
                                i += 2; // Otherwise we wouldn't be counting this byte pair
                                break;
                            }
                        }
                    }
                } else {
                    if unsafe{ is_true(in_xds_mode) } && (context.is_none() || check_context_field(&context) == 2) {
                        // TODO process XDS bytes
                        //unsafe{process_xds_bytes(dec_ctx.xds_ctx,hi,lo as i32);}
                        continue;
                    }

                    match context  {
                       None => {
                          continue; // No XDS code after this point, and user doesn't want captions.
                       }
                        Some(ref mut context) => {
                            // Ignore only the first repetition
                            context.last_c1 = 255;
                            context.last_c2 = 255;

                            if hi >= 0x20 { // Standard characters (always in pairs)
                                    // Only print if the channel is active
                                    if context.channel != context.my_channel { continue; }
                                    if context.textprinted == 0 {
                                        //ccx_common_logging.debug_ftn(0x10, "\n");
                                        context.textprinted = 1;
                                    }
                                    //TODO : handle_single
                                    context.handle_single(hi);
                                    context.handle_single(lo);
                                    wrote_to_screen = true;
                                    context.last_c1 = 0;
                                    context.last_c2 = 0;
                                }


                            }
                        }
                    }

                        if let Some(ref mut context) = context {
                            if !is_true(context.textprinted) && context.channel != context.my_channel {
                                // Current FTS information after the characters are shown
                                unsafe {debug!("Current FTS: {}\n", print_mstime_buff((*dec_ctx.timing).get_fts(context.my_field as u8)));}
                            }

                            if is_true(wrote_to_screen) && unsafe {is_true((*context.settings).direct_rollup)} && // If direct_rollup is enabled and
                                (context.mode == cc_modes::MODE_FAKE_ROLLUP_1 ||		          // we are in rollup mode, write now.
                                    context.mode == cc_modes::MODE_ROLLUP_2 ||
                                    context.mode == cc_modes::MODE_ROLLUP_3 ||
                                    context.mode == cc_modes::MODE_ROLLUP_4)
                            {
                                // We don't increase screenfuls_counter here
                                // TODO: write_cc_buffer
                                context.write_cc_buffer(sub);
                                //TODO: get_visible_start
                                context.current_visible_start_ms = unsafe {(*context.timing).get_visible_start(context.my_field as u8)};
                            }

                            if is_true(wrote_to_screen) && is_true(context.cc_to_stdout) {
                                // flushing must be done here; No idea how to do flushing of C IO from Rust
                            }
                        }
        } // for
    loop_counter
}

fn check_context_field(context: &Option<&mut ccx_decoder_608_context>) -> i32{
    if let Some(c) = context {
        return c.my_field;
    }
    return -1;
}



