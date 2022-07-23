mod timing;
mod encoder;
mod utility;
use crate::{bindings::*, utils::is_true};
use std::slice;
use crate::decoder608::timing::get_fts;

static oowdata: &'static [i8] = &[11, -1, 1, 2, 3, 4, 12, 13, 14, 15, 5, 6, 7, 8, 9, 10];

extern "C" {
    static mut ts_start_of_xds: i64;
    static mut in_xds_mode: i32;
    static ccx_common_logging: ccx_common_logging_t;
}

impl ccx_decoder_608_context {
    /* Handle Command, special char or attribute and also check for
    * channel changes.
    * Returns 1 if something was written to screen, 0 otherwise */
   pub fn disCommand(&mut self,mut hi:u8 , mut lo:u8, sub: *mut cc_subtitle) -> i32 {
       let mut wrote_to_screen = 0;

       /* Full channel changes are only allowed for "GLOBAL CODES",
       * "OTHER POSITIONING CODES", "BACKGROUND COLOR CODES",
       * "MID-ROW CODES".
       * "PREAMBLE ACCESS CODES", "BACKGROUND COLOR CODES" and
       * SPECIAL/SPECIAL CHARACTERS allow only switching
       * between 1&3 or 2&4. */
       self.new_channel = check_channel(hi, context);

       if hi >= 0x18 && hi <= 0x1f {
           hi = hi - 8;
       }

       match hi
       {
           0x10 => {
               if lo >= 0x40 && lo <= 0x5f
               handle_pac(hi, lo, context);
           }
           0x11 => {
               if lo >= 0x20 && lo <= 0x2f
               handle_text_attr(hi, lo, context);
               if lo >= 0x30 && lo <= 0x3f
               {
                   wrote_to_screen = 1;
                   handle_double(hi, lo, context);
               }
               if lo >= 0x40 && lo <= 0x7f
               handle_pac(hi, lo, context);
           }
          0x12 | 0x13 => {
              if lo >= 0x20 && lo <= 0x3f
              {
                  wrote_to_screen = handle_extended(hi, lo, context);
              }
              if lo >= 0x40 && lo <= 0x7f {
                  handle_pac(hi, lo, context);
              }
          }
          (0x14 | 0x15) => {
                  if lo >= 0x20 && lo <= 0x2f {
                      handle_command(hi, lo, context, sub);
                  }
                  if lo >= 0x40 && lo <= 0x7f {
                      handle_pac(hi, lo, context);
                  }
          }
          0x16 => {
                  if lo >= 0x40 && lo <= 0x7f {
                      handle_pac(hi, lo, context);
                      }
              }
          0x17 => {
              if lo >= 0x21 && lo <= 0x23 {
                  handle_command(hi, lo, context, sub);
              }
              if lo >= 0x2e && lo <= 0x2f {
                  handle_text_attr(hi, lo, context);
              }
              if lo >= 0x40 && lo <= 0x7f {
                  handle_pac(hi, lo, context);
              }
          }
       }
       return wrote_to_screen;
   }

    pub fn check_channel(&mut self,c1: u8) -> i32 {
       let new_channel = self.channel;
        if c1 >= 0x10 && c1 <= 0x17 {
            new_channel = 1;
        } else if c1 >= 0x18 && c1 <= 0x1e {
            new_channel = 2;
        }
        if new_channel != self.channel
        {
            // TODO debug here
            //ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "\nChannel change, now %d\n", new_channel);
            if self.channel != 3 // Don't delete memories if returning from XDS.
            {
                // erase_both_memories (wb); // 47cfr15.119.pdf, page 859, part f
                // CFS: Removed this because the specs say memories should be deleted if THE USER
                // changes the channel.
            }
        }
        return new_channel;
    }

    pub fn handle_pac(&mut self, mut c1: u8,mut c2: u8) {
        // Handle channel change
        if self.new_channel > 2
        {
            self.new_channel -= 2;
            //TODO debug
            //ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "\nChannel correction, now %d\n", self.new_channel);
        }
        self.channel = self.new_channel;
        if self.channel != self.my_channel {
            return;
        }

        let row = rowdata[((c1 << 1) & 14) | ((c2 >> 5) & 1)];

        //ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "\rPAC: %02X %02X", c1, c2);

        if c2 >= 0x40 && c2 <= 0x5f
        {
            c2 = c2 - 0x40;
        }
        else
        {
            if c2 >= 0x60 && c2 <= 0x7f
            {
                c2 = c2 - 0x60;
            }
            else
            {
                //ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "\rThis is not a PAC!!!!!\n");
                return;
            }
        }
        self.current_color = pac2_attribs[c2][0];
        self.font = pac2_attribs[c2][1];
        let indent = pac2_attribs[c2][2];
        //ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "  --  Position: %d:%d, color: %s,  font: %s\n", row,
         //                            indent, color_text[self.current_color][0], font_text[self.font]);

        if unsafe{(*self.settings).default_color} == COL_USERDEFINED && (self.current_color == COL_WHITE || self.current_color == COL_TRANSPARENT)
        {
            self.current_color = COL_USERDEFINED;
        }
        if self.mode != MODE_TEXT
        {
            // According to Robson, row info is discarded in text mode
            // but column is accepted
            self.cursor_row = row - 1; // Since the array is 0 based
        }
        self.rollup_base_row = row - 1;
        self.cursor_column = indent;
        self.have_cursor_position = 1;
        if self.mode == MODE_FAKE_ROLLUP_1 || self.mode == MODE_ROLLUP_2 || self.mode == MODE_ROLLUP_3 || self.mode == MODE_ROLLUP_4
        {
            /* In roll-up, delete lines BELOW the PAC. Not sure (CFS) this is correct (possibly we may have to move the
               buffer around instead) but it's better than leaving old characters in the buffer */
            let use_buffer: *mut eia608_screen = get_writing_buffer(context); // &wb->data608->buffer1;

            for j in row..CCX_DECODER_608_SCREEN_ROWS {
                if unsafe {(*use_buffer).row_used[j] != 0}
                {
                    memset(use_buffer->characters[j], ' ', CCX_DECODER_608_SCREEN_WIDTH);
                    use_buffer->characters[j][CCX_DECODER_608_SCREEN_WIDTH] = 0;

                    for (int i = 0; i < CCX_DECODER_608_SCREEN_WIDTH; ++i)
                    {
                        use_buffer->colors[j][i] = self.settings->default_color;
                        use_buffer->fonts[j][i] = FONT_REGULAR;
                    }
                    use_buffer->row_used[j] = 0;
                }
            }
        }
    }
}

#[no_mangle]
pub extern "C" fn rx_process608(
    data: *const ::std::os::raw::c_uchar,
    length: ::std::os::raw::c_int,
    private_data: *mut ::std::os::raw::c_void,
    sub: *mut cc_subtitle,
) -> ::std::os::raw::c_int
{
    let mut report: *mut ccx_decoder_608_report = std::ptr::null_mut();
    let dec_ctx : *mut lib_cc_decode= private_data as *mut lib_cc_decode;
    let context: *mut ccx_decoder_608_context;


    unsafe {
        match ((*dec_ctx).current_field, (*dec_ctx).extract) {
            (1, 1 | 12) => {
                context = (*dec_ctx).context_cc608_field_1 as *mut ccx_decoder_608_context;
            }
            (2, 2 | 12) => {
                context = (*dec_ctx).context_cc608_field_2 as *mut ccx_decoder_608_context;
            }
            (2, 1) => {
                context = std::ptr::null_mut();
            }
            (_, _) => {
                return -1;
            }
        }
        // doubt here. check if context is present
        if !context.is_null() {
            report = (*context).report;
            (*context).bytes_processed_608 += length as i64;
        }
       }

        if data.is_null() {
            return -1;
        }
        let data_vec = unsafe{slice::from_raw_parts(data,length as usize)};
        let mut loop_counter = 0;
        for i in (0..length).step_by(2) {
            loop_counter = i;
            let (hi,lo): (u8,u8);
            let mut wrote_to_screen = 0;
            hi = data_vec[i as usize] & 0x7F; // Get rid of parity bit
            lo = data_vec[(i+1) as usize] & 0x7F; // Get rid of parity bit

            if hi == 0 && lo == 0 { continue; } // Just padding

                if (0x10..=0x1e).contains(&hi) {
                    unsafe {
                        let mut ch = if hi <= 0x17 {1} else {2};
                        if context.is_null()  || unsafe { (*context).my_field == 2 } { ch += 2}; // Originally: current_field from sequencing.c. Seems to be just to change channel, so context->my_field seems good.
                        if !report.is_null() { (*report).cc_channels[ch-1] = 1; }
                    }
                }
                if (0x01..=0x0E).contains(&hi) && (context.is_null() || unsafe{(*context).my_field == 2}) { //XDS can only exist on field 2
                    unsafe {
                        if !context.is_null() { unsafe { (*context).channel = 3; } }
                        if !is_true(in_xds_mode) {
                            ts_start_of_xds = get_fts((*dec_ctx).timing, (*dec_ctx).current_field);
                            in_xds_mode = 1;
                        }

                        if !report.is_null() {
                            (*report).set_xds(1);
                        }
                    }
                }
                if hi == 0x0F && unsafe { is_true(in_xds_mode)} && (context.is_null() || unsafe{ (*context).my_field == 2}) { // End of XDS block
                    unsafe {
                    in_xds_mode = 0;
                    // TODO: do_end_of_xds
                    do_end_of_xds(sub, (*dec_ctx).xds_ctx,lo);
                    if !context.is_null() {
                         (*context).channel = (*context).new_channel; //switch from channel 3
                    }
                        continue;
                    }
                }

                if (0x10..=0x1F).contains(&hi) {
                    // Non-character code or special/extended char
                    // http://www.theneitherworld.com/mcpoodle/SCC_TOOLS/DOCS/CC_CODES.HTML
                    // http://www.theneitherworld.com/mcpoodle/SCC_TOOLS/DOCS/CC_CHARS.HTML
                   if context.is_null() || unsafe{(*context).my_field ==2} {
                       unsafe{in_xds_mode = 0;}
                   } // Back to normal (CEA 608-8.6.2)

                    if context.is_null() { continue; } // Not XDS and we don't have a writebuffer, nothing else would have an effect

                    // We were writing characters before, start a new line for
                    // diagnostic output from disCommand()
                    unsafe {
                       if (*context).textprinted == 1 {
                           // TODO ccx_common_logging.debug_ftn...
                           ccx_common_logging.debug_ftn(0x10, "\n");
                           (*context).textprinted = 0;
                       }

                       if (*context).last_c1 == hi && (*context).last_c2 == lo {
                           // Duplicate dual code, discard. Correct to do it only in
                           // non-XDS, XDS codes shall not be repeated.

                           // TODO ccx_common_logging.debug_ftn...
                           ccx_common_logging.debug_ftn(0x10, "Skipping command %02X, %02X Duplicate\n", hi, lo);
                           // Ignore only the first repetition
                           (*context).last_c1 = -1;
                           (*context).last_c2 = -1;
                           continue;
                       }
                       (*context).last_c1 = hi;
                       (*context).last_c2 = lo;
                       // TODO wrote_to_screen
                       wrote_to_screen = disCommand(hi,lo, context, sub);
                       if (*sub).got_output {
                           i += 2; // Otherwise we wouldn't be counting this byte pair
                           break;
                       }
                   }
                } else {
                    if unsafe{ is_true(in_xds_mode) } && (context.is_null() || unsafe{ (*context).my_field == 2}) {
                        unsafe{process_xds_bytes((*dec_ctx).xds_ctx,hi,lo);}
                        continue;
                    }

                    if context.is_null() { // No XDS code after this point, and user doesn't want captions.
                        continue;
                    }

                    unsafe {
                        // Ignore only the first repetition
                        (*context).last_c1 = 255;
                        (*context).last_c2 = 255;
                    }

                    if hi >= 0x20 { // Standard characters (always in pairs)
                        unsafe {
                            // Only print if the channel is active
                            if (*context).channel != (*context).my_channel { continue; }
                            if (*context).textprinted == 0 {
                                ccx_common_logging.debug_ftn(0x10, "\n");
                                (*context).textprinted = 1;
                            }
                            //TODO : handle_single
                            handle_single(hi, context);
                            handle_single(lo, context);
                            wrote_to_screen = 1;
                            (*context).last_c1 = 0;
                            (*context).last_c2 = 0;
                        }
                    }
                    unsafe {
                        if !(*context).textprinted && (*context).channel != (*context).my_channel {
                            // Current FTS information after the characters are shown
                            // TODO: print_mstime_static
                            ccx_common_logging.debug_ftn(0x10, "Current FTS: %s\n", print_mstime_static(get_fts((*dec_ctx).timing, (*context).my_field)));
                        }
                        if wrote_to_screen && (*(*context).settings).direct_rollup && // If direct_rollup is enabled and
                            ((*context).mode == MODE_FAKE_ROLLUP_1 ||		          // we are in rollup mode, write now.
                            (*context).mode == MODE_ROLLUP_2 ||
                            (*context).mode == MODE_ROLLUP_3 ||
                            (*context).mode == MODE_ROLLUP_4)
                        {
                        // We don't increase screenfuls_counter here
                        // TODO: write_cc_buffer
                        write_cc_buffer(context, sub);
                        //TODO: get_visible_start
                        (*context).current_visible_start_ms = get_visible_start((*context).timing, (*context).my_field);
                        }
                    }
                }

            unsafe{
            if wrote_to_screen && (*context).cc_to_stdout {
                std::libc::fflush();
                }
            }
        } // for
    loop_counter
}



