use crate::decoder608::constants::{pac2_attribs, rowdata_608, CCX_DECODER_608_SCREEN_WIDTH, CCX_DECODER_608_SCREEN_ROWS };
use crate::bindings::*;
use crate::utils::is_true;


impl ccx_decoder_608_context {
    /* Handle Command, special char or attribute and also check for
    * channel changes.
    * Returns 1 if something was written to screen, 0 otherwise */
    pub fn disCommand(&mut self,mut hi:u8 , mut lo:u8, sub: *mut cc_subtitle) -> bool {
        let mut wrote_to_screen : bool= false;

        unsafe {
        /* Full channel changes are only allowed for "GLOBAL CODES",
        * "OTHER POSITIONING CODES", "BACKGROUND COLOR CODES",
        * "MID-ROW CODES".
        * "PREAMBLE ACCESS CODES", "BACKGROUND COLOR CODES" and
        * SPECIAL/SPECIAL CHARACTERS allow only switching
        * between 1&3 or 2&4. */
        self.new_channel = self.check_channel(hi);

        if hi >= 0x18 && hi <= 0x1f {
            hi = hi - 8;
        }

        match hi
        {
            0x10 => {
                if lo >= 0x40 && lo <= 0x5f {
                    self.handle_pac(hi, lo);
                }
            }
            0x11 => {
                if lo >= 0x20 && lo <= 0x2f {
                    self.handle_text_attr(hi, lo);
                }
                if lo >= 0x30 && lo <= 0x3f
                {
                    wrote_to_screen = true;
                    self.handle_double(hi, lo);
                }
                if lo >= 0x40 && lo <= 0x7f {
                    self.handle_pac(hi, lo);
                }
            }
            0x12 | 0x13 => {
                if lo >= 0x20 && lo <= 0x3f
                {
                    wrote_to_screen = self.handle_extended(hi, lo);
                }
                if lo >= 0x40 && lo <= 0x7f {
                    self.handle_pac(hi, lo);
                }
            }
            0x14 | 0x15 => {
                if lo >= 0x20 && lo <= 0x2f {
                    self.handle_command(hi, lo, sub);
                }
                if lo >= 0x40 && lo <= 0x7f {
                    self.handle_pac(hi, lo);
                }
            }
            0x16 => {
                if lo >= 0x40 && lo <= 0x7f {
                    self.handle_pac(hi, lo);
                }
            }
            0x17 => {
                if lo >= 0x21 && lo <= 0x23 {
                    self.handle_command(hi, lo, sub);
                }
                if lo >= 0x2e && lo <= 0x2f {
                    self.handle_text_attr(hi, lo);
                }
                if lo >= 0x40 && lo <= 0x7f {
                    self.handle_pac(hi, lo);
                }
            }
            _ => {}
        }
        }
        return wrote_to_screen;
    }

    pub fn check_channel(&mut self,c1: u8) -> i32 {
        let mut new_channel = self.channel;
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

    pub unsafe fn handle_pac(&mut self, mut c1: u8, mut c2: u8) {
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

        let row = rowdata_608[(((c1 << 1) & 14) | ((c2 >> 5) & 1)) as usize];

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
        self.current_color = pac2_attribs[c2 as usize].color;
        self.font = pac2_attribs[c2 as usize].font;
        let indent = pac2_attribs[c2 as usize].ident;
        //ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "  --  Position: %d:%d, color: %s,  font: %s\n", row,
        //                            indent, color_text[self.current_color][0], font_text[self.font]);

        if unsafe{(*self.settings).default_color} ==
            ccx_decoder_608_color_code::COL_USERDEFINED &&
            (self.current_color == ccx_decoder_608_color_code::COL_WHITE || self.current_color == ccx_decoder_608_color_code::COL_TRANSPARENT)
        {
            self.current_color = ccx_decoder_608_color_code::COL_USERDEFINED;
        }
        if self.mode != cc_modes::MODE_TEXT
        {
            // According to Robson, row info is discarded in text mode
            // but column is accepted
            self.cursor_row = row - 1; // Since the array is 0 based
        }
        self.rollup_base_row = row - 1;
        self.cursor_column = indent;
        self.have_cursor_position = 1;
        if self.mode == cc_modes::MODE_FAKE_ROLLUP_1 || self.mode == cc_modes::MODE_ROLLUP_2 || self.mode == cc_modes::MODE_ROLLUP_3 || self.mode == cc_modes::MODE_ROLLUP_4
        {
            /* In roll-up, delete lines BELOW the PAC. Not sure (CFS) this is correct (possibly we may have to move the
               buffer around instead) but it's better than leaving old characters in the buffer */
            let use_buffer: *mut eia608_screen = self.get_writing_buffer();

            for j in row..(*CCX_DECODER_608_SCREEN_ROWS as i32) {
                let j:usize = j as usize;
                if unsafe {(*use_buffer).row_used[j] != 0}
                {
                    // memset(use_buffer->characters[j], ' ', CCX_DECODER_608_SCREEN_WIDTH);
                    // equivalent of memset
                    (*use_buffer).characters[j] = [0; *CCX_DECODER_608_SCREEN_WIDTH+1] ;
                    //set last to 0
                    (*use_buffer).characters[j][*CCX_DECODER_608_SCREEN_WIDTH] = 0;
                    for i in 0..*CCX_DECODER_608_SCREEN_WIDTH
                    {
                       let i : usize = i as usize;
                        (*use_buffer).colors[j][i] =(*self.settings).default_color;
                        (*use_buffer).fonts[j][i] = font_bits::FONT_REGULAR;
                    }
                    (*use_buffer).row_used[j] = 0;
                }
            }
        }
    }

    pub fn context_write_u8(&mut self,c: u8) {
        if self.mode != cc_modes::MODE_TEXT {
            let use_buffer: *mut eia608_screen = self.get_writing_buffer();
            unsafe{

                (*use_buffer).characters[self.cursor_row as usize][self.cursor_column as usize] = c;
                (*use_buffer).colors[self.cursor_row as usize][self.cursor_column as usize] = self.current_color;
                (*use_buffer).fonts[self.cursor_row as usize][self.cursor_column as usize] = self.font;
                (*use_buffer).row_used[self.cursor_row as usize] = 1;

                if is_true((*use_buffer).empty)
                {
                    if cc_modes::MODE_POPON != self.mode {
                        self.current_visible_start_ms = (*self.timing).get_visible_start(self.my_field as u8);
                    }
                }
                (*use_buffer).empty = 0;

                if self.cursor_column < (CCX_DECODER_608_SCREEN_WIDTH - 1) as i32 {
                    self.cursor_column+=1;
                }
                if self.ts_start_of_current_line == -1 {
                    self.ts_start_of_current_line = (*self.timing).get_fts(self.my_field as u8);
                }
                self.ts_last_char_received = (*self.timing).get_fts(self.my_field as u8);
            }
        }
    }

    pub fn handle_text_attr(&mut self,c1: u8, c2: u8) {
        self.channel = self.new_channel;
        if self.channel != self.my_channel {
           return;
        }
        // DEBUG text attr
        if (c1 != 0x11 && c1 != 0x19) || (c2 < 0x20 || c2 > 0x2f) {
            // DEBUG text attribute
        }
        else {
            let i = c2 - 0x20;
            self.current_color = pac2_attribs[i as usize].color;
            self.font = pac2_attribs[i as usize].font;
            //Debug
            self.context_write_u8(0x20);
        }
    }

    pub fn get_writing_buffer(&mut self) -> *mut eia608_screen{
        let mut use_buffer: *mut eia608_screen = std::ptr::null_mut();
        match self.mode {
           cc_modes::MODE_POPON => {
               if self.visible_buffer == 1 {
                  use_buffer = &mut self.buffer2;
               }else{
                   use_buffer = &mut self.buffer1;
               }
           }
           cc_modes::MODE_FAKE_ROLLUP_1 |
           cc_modes::MODE_ROLLUP_2 |
           cc_modes::MODE_ROLLUP_3 |
           cc_modes::MODE_ROLLUP_4 |
           cc_modes::MODE_PAINTON => {
               if self.visible_buffer == 1 {
                   use_buffer = &mut self.buffer1;
               }
               else {
                   use_buffer = &mut self.buffer2;
               }
           }
           _ => {
            //DEBUG
            }
        }
        return use_buffer;
    }

    // CEA-608, Anex F 1.1.1. - Character Set Table / Special Characters
    pub fn handle_double(&mut self,c1: u8, c2: u8) {
        if self.channel != self.my_channel {
            return;
        }
        if c2 >= 0x30 && c2 <=0x3f {
            let c = c2 + 0x50;
            //DEBUG
            self.context_write_u8(c);
        }
    }

    pub fn handle_extended(&mut self,hi: u8, lo: u8) -> bool{
        // Handle channel change
        if self.new_channel > 2
        {
            self.new_channel -= 2;
            //DEBUG
        }
        self.channel = self.new_channel;
        if self.channel != self.my_channel {
            return false;
        }

        // For lo values between 0x20-0x3f
        let mut c: u8 = 0;

        // DEBUG
        if lo >= 0x20 && lo <= 0x3f && (hi == 0x12 || hi == 0x13)
        {
            match hi
            {
                0x12 => {
                    c = lo + 0x70; // So if c>=0x90 && c<=0xaf it comes from here
                }
                 0x13 => {
                     c = lo + 0x90; // So if c>=0xb0 && c<=0xcf it comes from here
                 }
                _ => { }
            }
            // This column change is because extended characters replace
            // the previous character (which is sent for basic decoders
            // to show something similar to the real char)
            if self.cursor_column > 0 {
                self.cursor_column -= 1;
            }

            self.context_write_u8(c);
        }
        return true;
    }

    pub fn handle_command(&mut self, mut c1: u8, mut c2: u8, sub: *mut cc_subtitle) {
        let mut changes:bool = false;

        // Handle channel change
        self.channel = self.new_channel;
        if self.channel != self.my_channel {
            return;
        }

        let mut command: command_code = command_code::COM_UNKNOWN;
        if c1 == 0x15 {
            c1 = 0x14;
        }
        if (c1 == 0x14 || c1 == 0x1C) && c2 == 0x2C {
            command = command_code::COM_ERASEDISPLAYEDMEMORY;
        }
        if (c1 == 0x14 || c1 == 0x1C) && c2 == 0x20 {
            command = command_code::COM_RESUMECAPTIONLOADING;
        }
        if (c1 == 0x14 || c1 == 0x1C) && c2 == 0x2F {
            command = command_code::COM_ENDOFCAPTION;
        }
        if (c1 == 0x14 || c1 == 0x1C) && c2 == 0x22 {
            command = command_code::COM_ALARMOFF;
        }
        if (c1 == 0x14 || c1 == 0x1C) && c2 == 0x23 {
            command = command_code::COM_ALARMON;
        }
        if (c1 == 0x14 || c1 == 0x1C) && c2 == 0x24 {
            command = command_code::COM_DELETETOENDOFROW;
        }
        if (c1 == 0x17 || c1 == 0x1F) && c2 == 0x21 {
            command = command_code::COM_TABOFFSET1;
        }
        if (c1 == 0x17 || c1 == 0x1F) && c2 == 0x22 {
            command = command_code::COM_TABOFFSET2;
        }
        if (c1 == 0x17 || c1 == 0x1F) && c2 == 0x23 {
            command = command_code::COM_TABOFFSET3;
        }
        if (c1 == 0x14 || c1 == 0x1C) && c2 == 0x25 {
            command = command_code::COM_ROLLUP2;
        }
        if (c1 == 0x14 || c1 == 0x1C) && c2 == 0x26 {
            command = command_code::COM_ROLLUP3;
        }
        if (c1 == 0x14 || c1 == 0x1C) && c2 == 0x27 {
            command = command_code::COM_ROLLUP4;
        }
        if (c1 == 0x14 || c1 == 0x1C) && c2 == 0x29 {
            command = command_code::COM_RESUMEDIRECTCAPTIONING;
        }
        if (c1 == 0x14 || c1 == 0x1C) && c2 == 0x2D {
            command = command_code::COM_CARRIAGERETURN;
        }
        if (c1 == 0x14 || c1 == 0x1C) && c2 == 0x2E {
            command = command_code::COM_ERASENONDISPLAYEDMEMORY;
        }
        if (c1 == 0x14 || c1 == 0x1C) && c2 == 0x21 {
            command = command_code::COM_BACKSPACE;
        }
        if (c1 == 0x14 || c1 == 0x1C) && c2 == 0x2b {
            command = command_code::COM_RESUMETEXTDISPLAY;
        }

        if (command == command_code::COM_ROLLUP2 || command == command_code::COM_ROLLUP3 || command == command_code::COM_ROLLUP4) && unsafe { (*self.settings).force_rollup == 1 }
        {
            command = command_code::COM_FAKE_RULLUP1;
        }
        if (command == command_code::COM_ROLLUP3 || command == command_code::COM_ROLLUP4) && unsafe { (*self.settings).force_rollup } == 2
        {
            command = command_code::COM_ROLLUP2;
        } else if command == command_code::COM_ROLLUP4 && unsafe { (*self.settings).force_rollup } == 3 {
            command = command_code::COM_ROLLUP3;
        }
        // DEBUG
        // self executing closure for early match break
        (|| {
            unsafe {
            match command {
                command_code::COM_BACKSPACE => {
                    if self.cursor_column > 0
                    {
                        self.cursor_column -= 1;
                        // TODO get_writing_buffer
                        (*self.get_writing_buffer()).characters
                            [self.cursor_row as usize][self.cursor_column as usize] = ' ' as u8;
                    }
                }
                command_code::COM_TABOFFSET1 => {
                    if self.cursor_column < (CCX_DECODER_608_SCREEN_WIDTH - 1) as i32 {
                        self.cursor_column += 1;
                    }
                }
                command_code::COM_TABOFFSET2 => {
                    self.cursor_column += 2;
                    if self.cursor_column > (CCX_DECODER_608_SCREEN_WIDTH - 1) as i32 {
                        self.cursor_column = (CCX_DECODER_608_SCREEN_WIDTH - 1) as i32;
                    }
                }
                command_code::COM_TABOFFSET3 => {
                    self.cursor_column += 3;
                    if self.cursor_column > (CCX_DECODER_608_SCREEN_WIDTH - 1) as i32 {
                        self.cursor_column = (CCX_DECODER_608_SCREEN_WIDTH - 1) as i32;
                    }
                }
                command_code::COM_RESUMECAPTIONLOADING => {
                    self.mode = cc_modes::MODE_POPON;
                }
                command_code::COM_RESUMETEXTDISPLAY => {
                    self.mode = cc_modes::MODE_TEXT;
                }
                command_code::COM_FAKE_RULLUP1 |
                command_code::COM_ROLLUP2 |
                command_code::COM_ROLLUP3 |
                command_code::COM_ROLLUP4 => {
                    if self.mode == cc_modes::MODE_POPON || self.mode == cc_modes::MODE_PAINTON {
                        /* CEA-608 C.10 Style Switching (regulatory)
                [...]if pop-up or paint-on captioning is already present in
                either memory it shall be erased[...] */
                        if self.write_cc_buffer(sub) {
                            self.screenfuls_counter += 1;
                        }
                        self.erase_memory(true);
                    }
                    self.erase_memory(false);

                    // If the reception of data for a row is interrupted by data for the alternate
                    // data channel or for text mode, the display of caption text will resume from the same
                    // cursor position if a roll-up caption command is received and no PAC is given [...]
                    if self.mode != cc_modes::MODE_TEXT && self.have_cursor_position == 0
                    {
                        // If no Preamble Address Code  is received, the base row shall default to row 15
                        self.cursor_row = 14; // Default if the previous mode wasn't roll up already.
                        self.cursor_column = 0;
                        self.have_cursor_position = 1;
                    }

                    match command
                    {
                        command_code::COM_FAKE_RULLUP1 => {
                            self.mode = cc_modes::MODE_FAKE_ROLLUP_1;
                        }
                        command_code::COM_ROLLUP2 => {
                            self.mode = cc_modes::MODE_ROLLUP_2;
                        }
                        command_code::COM_ROLLUP3 => {
                            self.mode = cc_modes::MODE_ROLLUP_3;
                        }
                        command_code::COM_ROLLUP4 => {
                            self.mode = cc_modes::MODE_ROLLUP_4;
                        }
                        _ => {
                            // impossible
                        }
                    }
                }
                command_code::COM_CARRIAGERETURN => {
                    if self.mode == cc_modes::MODE_PAINTON {// CR has no effect on painton mode according to zvbis' code
                        return;
                    }
                    if self.mode == cc_modes::MODE_POPON   // CFS: Not sure about this. Is there a valid reason for CR in popup?
                    {
                        self.cursor_column = 0;
                        if self.cursor_row < *CCX_DECODER_608_SCREEN_ROWS as i32 {
                            self.cursor_row += 1;
                        }
                        return;
                    }
                    if self.output_format == ccx_output_format::CCX_OF_TRANSCRIPT
                    {
                        self.write_cc_line(sub);
                    }

                    // In transcript mode, CR doesn't write the whole screen, to avoid
                    // repeated lines.
                    changes = self.check_roll_up();
                    if changes
                    {
                        // Only if the roll up would actually cause a line to disappear we write the buffer
                        if self.output_format != ccx_output_format::CCX_OF_TRANSCRIPT
                        {
                            if self.write_cc_buffer(sub) {
                                self.screenfuls_counter += 1;
                            }
                            if unsafe { is_true((*self.settings).no_rollup) } {
                                self.erase_memory(true); // Make sure the lines we just wrote aren't written again
                            }
                        }
                    }
                    self.roll_up();            // The roll must be done anyway of course.
                    self.ts_start_of_current_line = -1; // Unknown.
                    if changes {
                        self.current_visible_start_ms = get_visible_start(self.timing, self.my_field);
                    }
                    self.cursor_column = 0;
                }
                command_code::COM_ERASENONDISPLAYEDMEMORY => {
                    self.erase_memory(false);
                }
                command_code::COM_ERASEDISPLAYEDMEMORY => {
                    // Write it to disk before doing this, and make a note of the new
                    // time it became clear.
                    if self.output_format == ccx_output_format::CCX_OF_TRANSCRIPT &&
                        (self.mode == cc_modes::MODE_FAKE_ROLLUP_1 ||
                            self.mode == cc_modes::MODE_ROLLUP_2 ||
                            self.mode == cc_modes::MODE_ROLLUP_3 ||
                            self.mode == cc_modes::MODE_ROLLUP_4)
                    {
                        // In transcript mode we just write the cursor line. The previous lines
                        // should have been written already, so writing everything produces
                        // duplicate lines.
                        self.write_cc_line(sub);
                    } else {
                        if self.output_format == ccx_output_format::CCX_OF_TRANSCRIPT {
                            self.ts_start_of_current_line = self.current_visible_start_ms;
                        }
                        if self.write_cc_buffer(sub) {
                            self.screenfuls_counter += 1;
                        }
                    }
                    self.erase_memory(true);
                    self.current_visible_start_ms = (*self.timing).get_visible_start(self.my_field as u8);
                }
                command_code::COM_ENDOFCAPTION => { // Switch buffers
                    // The currently *visible* buffer is leaving, so now we know its ending
                    // time. Time to actually write it to file.
                    if self.write_cc_buffer(sub) {
                        self.screenfuls_counter += 1;
                    }
                    self.visible_buffer = if self.visible_buffer == 1 { 2 } else { 1 };
                    self.current_visible_start_ms = (*self.timing).get_visible_start(self.my_field as u8);
                    self.cursor_column = 0;
                    self.cursor_row = 0;
                    self.current_color = unsafe { (*self.settings).default_color };
                    self.font = font_bits::FONT_REGULAR;
                    self.mode = cc_modes::MODE_POPON;
                }
                command_code::COM_DELETETOENDOFROW => {
                    self.delete_to_end_of_row();
                }
                command_code::COM_ALARMOFF |
                command_code::COM_ALARMON => {
                    // These two are unused according to Robson's, and we wouldn't be able to do anything useful anyway
                }
                command_code::COM_RESUMEDIRECTCAPTIONING => {
                    self.mode = cc_modes::MODE_PAINTON;
                }
                _ => {
                    // DEBUG
                }
            }
        }
            })();
        }

    pub fn check_roll_up(&mut self) -> bool {
       let mut keep_lines: u8 = 0;
       let (mut first_row, mut last_row )= (-1,-1);
       let mut use_buffer : *mut eia608_screen = std::ptr::null_mut();
       if self.visible_buffer == 1 {
            use_buffer = &mut self.buffer1;
        }else {
             use_buffer = &mut self.buffer2;
       }

        match self.mode
        {
            cc_modes::MODE_FAKE_ROLLUP_1 => {
                keep_lines = 1;
            }
            cc_modes::MODE_ROLLUP_2 => {
                keep_lines = 2;
            }
            cc_modes::MODE_ROLLUP_3 => {
                keep_lines = 3;
            }
            cc_modes::MODE_ROLLUP_4 => {
                keep_lines = 4;
            }
            cc_modes::MODE_TEXT => {
                keep_lines = 7; // CFS: can be 7 to 15 according to the handbook. No idea how this is selected.
            }
            _ => { return false;} // Shouldn't happen
        }
        if unsafe {is_true((*use_buffer).row_used[0])} { // If top line is used it will go off the screen no matter what
            return true;
        }
        let mut rows_orig = 0; // Number of rows in use right now
        for i in 0..*CCX_DECODER_608_SCREEN_ROWS as i32
        {
            if is_true(unsafe {(*use_buffer).row_used[i as usize]})
            {
                rows_orig+=1;
                if first_row == -1 {
                    first_row = i;
                }
                last_row = i;
            }
        }
        if last_row == -1 { // Empty screen, nothing to rollup
            return false;
        }
        if (last_row - first_row + 1) >= keep_lines as i32{
            return true; // We have the roll-up area full, so yes
        }
        if (first_row - 1) <= self.cursor_row - keep_lines as i32 { // Roll up will delete those lines.
            return true;
        }
        return false;
    }

    pub fn write_cc_buffer(&mut self, sub: *mut cc_subtitle) -> bool {
       unimplemented!();
    }

    pub fn write_cc_line(&mut self, sub: *mut cc_subtitle) -> bool {
        unimplemented!();
    }
    pub fn erase_memory(&mut self, displayed:bool) {
        unimplemented!();
    }
    pub fn roll_up(&self) {
        unimplemented!();
    }
    pub fn delete_to_end_of_row(&self) {
        unimplemented!();
    }
    pub fn handle_single(&self, lo:u8) {}
}

