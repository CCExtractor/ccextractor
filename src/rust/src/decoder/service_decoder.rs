use std::os::raw::c_uchar;

use log::{debug, warn};

use super::commands::{C0CodeSet, C0Command, C1CodeSet, C1Command};
use crate::bindings::*;

const CCX_DTVCC_MUSICAL_NOTE_CHAR: u16 = 9836;
const CCX_DTVCC_MAX_WINDOWS: u8 = 8;
const DTVCC_COMMANDS_C0_CODES_DTVCC_C0_EXT1: u8 = 16;

impl dtvcc_service_decoder {
    /// Process service block and call handlers for the respective codesets
    pub fn process_service_block(
        &mut self,
        current_packet: &mut [c_uchar],
        pos: u8,
        block_length: u8,
        encoder: &mut encoder_ctx,
        timing: &mut ccx_common_timing_ctx,
        no_rollup: bool,
    ) {
        let mut i = 0;
        while i < block_length {
            let curr: usize = (pos + i) as usize;

            let consumed = if current_packet[curr] != DTVCC_COMMANDS_C0_CODES_DTVCC_C0_EXT1 as u8 {
                let used = match current_packet[curr] {
                    0..=0x1F => self.handle_C0(
                        current_packet,
                        pos + i,
                        block_length - i,
                        encoder,
                        timing,
                        no_rollup,
                    ),
                    0x20..=0x7F => self.handle_G0(current_packet, pos + i, block_length - i),
                    0x80..=0x9F => {
                        self.handle_C1(current_packet, pos + i, block_length - i, encoder, timing)
                    }
                    _ => self.handle_G1(current_packet, pos + i, block_length - i),
                };
                if used == -1 {
                    warn!("dtvcc_process_service_block: There was a problem handling the data.");
                    return;
                }
                used
            } else {
                let mut used = self.handle_extended_char(current_packet, pos + i, block_length - i);
                used += 1; // Since we had CCX_DTVCC_C0_EXT1
                used
            };
            i += consumed as u8;
        }
    }
    /// Handle C0 - Code Set - Miscellaneous Control Codes
    pub fn handle_C0(
        &mut self,
        current_packet: &mut [c_uchar],
        pos: u8,
        block_length: u8,
        encoder: &mut encoder_ctx,
        timing: &mut ccx_common_timing_ctx,
        no_rollup: bool,
    ) -> i32 {
        let service_decoder = self as *mut dtvcc_service_decoder;
        let data = (&mut current_packet[pos as usize]) as *mut std::os::raw::c_uchar;

        let code = current_packet[pos as usize];
        let C0Command { command, length } = C0Command::new(code);
        debug!("C0: [{:?}] ({})", command, block_length);
        unsafe {
            match command {
                // NUL command does nothing
                C0CodeSet::NUL => {}
                C0CodeSet::ETX => dtvcc_process_etx(service_decoder),
                C0CodeSet::BS => dtvcc_process_bs(service_decoder),
                C0CodeSet::FF => dtvcc_process_ff(service_decoder),
                C0CodeSet::CR => self.process_cr(encoder, timing, no_rollup),
                C0CodeSet::HCR => dtvcc_process_hcr(service_decoder),
                // EXT1 is handled elsewhere as an extended command
                C0CodeSet::EXT1 => {}
                C0CodeSet::P16 => dtvcc_handle_C0_P16(service_decoder, data.add(1)),
                C0CodeSet::RESERVED => {}
            }
        }
        if length > block_length {
            warn!(
                "dtvcc_handle_C0: command is {} bytes long but we only have {}",
                length, block_length
            );
            return -1;
        }
        length as i32
    }
    /// Handle C1 - Code Set - Captioning Command Control Codes
    pub fn handle_C1(
        &mut self,
        current_packet: &mut [c_uchar],
        pos: u8,
        block_length: u8,
        encoder: &mut encoder_ctx,
        timing: &mut ccx_common_timing_ctx,
    ) -> i32 {
        let data = (&mut current_packet[pos as usize]) as *mut std::os::raw::c_uchar;

        let code = current_packet[pos as usize];
        let next_code = current_packet[(pos + 1) as usize] as i32;
        let C1Command {
            command,
            length,
            name,
        } = C1Command::new(code);

        if length > block_length {
            warn!("Warning: Not enough bytes for command.");
            return -1;
        }

        debug!("C1: [{:?}] [{}] ({})", command, name, length);
        unsafe {
            match command {
                C1CodeSet::CW0
                | C1CodeSet::CW1
                | C1CodeSet::CW2
                | C1CodeSet::CW3
                | C1CodeSet::CW4
                | C1CodeSet::CW5
                | C1CodeSet::CW6
                | C1CodeSet::CW7 => dtvcc_handle_CWx_SetCurrentWindow(
                    self,
                    (code - DTVCC_COMMANDS_C1_CODES_DTVCC_C1_CW0 as u8) as i32,
                ),
                C1CodeSet::CLW => self.handle_clear_windows(next_code, encoder, timing),
                C1CodeSet::HDW => self.handle_hide_windows(next_code, encoder, timing),
                C1CodeSet::TGW => self.handle_toggle_windows(next_code, encoder, timing),
                C1CodeSet::DLW => self.handle_delete_windows(next_code, encoder, timing),
                C1CodeSet::DSW => dtvcc_handle_DSW_DisplayWindows(self, next_code, timing),
                C1CodeSet::DLY => dtvcc_handle_DLY_Delay(self, next_code),
                C1CodeSet::DLC => dtvcc_handle_DLC_DelayCancel(self),
                C1CodeSet::RST => dtvcc_handle_RST_Reset(self),
                C1CodeSet::SPA => dtvcc_handle_SPA_SetPenAttributes(self, data),
                C1CodeSet::SPC => dtvcc_handle_SPC_SetPenColor(self, data),
                C1CodeSet::SPL => dtvcc_handle_SPL_SetPenLocation(self, data),
                C1CodeSet::SWA => dtvcc_handle_SWA_SetWindowAttributes(self, data),
                C1CodeSet::DF0
                | C1CodeSet::DF1
                | C1CodeSet::DF2
                | C1CodeSet::DF3
                | C1CodeSet::DF4
                | C1CodeSet::DF5
                | C1CodeSet::DF6
                | C1CodeSet::DF7 => dtvcc_handle_DFx_DefineWindow(
                    self,
                    (code - DTVCC_COMMANDS_C1_CODES_DTVCC_C1_DF0 as u8) as i32,
                    data,
                    timing,
                ),
                C1CodeSet::RESERVED => {
                    warn!("Warning, Found Reserved codes, ignored");
                }
            };
        }
        length as i32
    }
    /// Handle G0 - Code Set - ASCII printable characters
    pub fn handle_G0(&mut self, current_packet: &mut [c_uchar], pos: u8, block_length: u8) -> i32 {
        if self.current_window == -1 {
            warn!("dtvcc_handle_G0: Window has to be defined first");
            return block_length as i32;
        }

        let character = current_packet[pos as usize];
        debug!("G0: [{:2X}] ({})", character, character as char);
        let sym = if character == 0x7F {
            dtvcc_symbol::new(CCX_DTVCC_MUSICAL_NOTE_CHAR)
        } else {
            dtvcc_symbol::new(character as u16)
        };
        self.process_character(sym);
        1
    }
    /// Handle G1 - Code Set - ISO 8859-1 LATIN-1 Character Set
    pub fn handle_G1(&mut self, current_packet: &mut [c_uchar], pos: u8, block_length: u8) -> i32 {
        if self.current_window == -1 {
            warn!("dtvcc_handle_G1: Window has to be defined first");
            return block_length as i32;
        }

        let character = current_packet[pos as usize];
        debug!("G1: [{:2X}] ({})", character, character as char);
        let sym = dtvcc_symbol::new(character as u16);
        self.process_character(sym);
        1
    }
    /// Handle extended codes (EXT1 + code), from the extended sets
    pub fn handle_extended_char(
        &mut self,
        current_packet: &mut [c_uchar],
        pos: u8,
        block_length: u8,
    ) -> i32 {
        unsafe {
            let data = (&mut current_packet[pos as usize]) as *mut std::os::raw::c_uchar;
            dtvcc_handle_extended_char(self, data, block_length as i32)
        }
    }
    /// Handle CLW Clear Windows
    pub fn handle_clear_windows(
        &mut self,
        mut windows_bitmap: i32,
        encoder: &mut encoder_ctx,
        timing: &mut ccx_common_timing_ctx,
    ) {
        debug!("dtvcc_handle_CLW_ClearWindows: windows:");
        let mut screen_content_changed = false;
        unsafe {
            if windows_bitmap == 0 {
                debug!("none");
            } else {
                for i in 0..CCX_DTVCC_MAX_WINDOWS {
                    if windows_bitmap & 1 == 1 {
                        let window = &mut self.windows[i as usize];
                        debug!("[W{}]", i);
                        let window_had_content =
                            window.is_defined == 1 && window.visible == 1 && window.is_empty == 0;
                        if window_had_content {
                            screen_content_changed = true;
                            let window = window as *mut dtvcc_window;
                            dtvcc_window_update_time_hide(window, timing);
                            dtvcc_window_copy_to_screen(self, window)
                        }
                        dtvcc_window_clear(self, i as i32);
                    }
                    windows_bitmap >>= 1;
                }
            }
            if screen_content_changed {
                self.screen_print(encoder, timing);
            }
        }
    }
    /// Handle HDW Hide Windows
    pub fn handle_hide_windows(
        &mut self,
        mut windows_bitmap: i32,
        encoder: &mut encoder_ctx,
        timing: &mut ccx_common_timing_ctx,
    ) {
        debug!("dtvcc_handle_HDW_HideWindows: windows:");
        if windows_bitmap == 0 {
            debug!("none");
        } else {
            let mut screen_content_changed = false;
            unsafe {
                for i in 0..CCX_DTVCC_MAX_WINDOWS {
                    if windows_bitmap & 1 == 1 {
                        let window = &mut self.windows[i as usize];
                        debug!("[W{}]", i);
                        if window.visible == 1 {
                            screen_content_changed = true;
                            window.visible = 0;
                            let window_ctx = window as *mut dtvcc_window;
                            dtvcc_window_update_time_hide(window_ctx, timing);
                            if window.is_empty == 0 {
                                dtvcc_window_copy_to_screen(self, window_ctx);
                            }
                        }
                    }
                    windows_bitmap >>= 1;
                }
                if screen_content_changed && dtvcc_decoder_has_visible_windows(self) == 0 {
                    self.screen_print(encoder, timing);
                }
            }
        }
    }
    /// Handle TGW Toggle Windows
    pub fn handle_toggle_windows(
        &mut self,
        mut windows_bitmap: i32,
        encoder: &mut encoder_ctx,
        timing: &mut ccx_common_timing_ctx,
    ) {
        debug!("dtvcc_handle_TGW_ToggleWindows: windows:");
        if windows_bitmap == 0 {
            debug!("none");
        } else {
            let mut screen_content_changed = false;
            unsafe {
                for i in 0..CCX_DTVCC_MAX_WINDOWS {
                    let window = &mut self.windows[i as usize];
                    let window_ctx = window as *mut dtvcc_window;
                    if windows_bitmap & 1 == 1 && window.is_defined == 1 {
                        if window.visible == 0 {
                            debug!("[W-{}: 0->1]", i);
                            window.visible = 1;
                            dtvcc_window_update_time_show(window_ctx, timing);
                        } else {
                            debug!("[W-{}: 1->0]", i);
                            window.visible = 0;
                            dtvcc_window_update_time_hide(window_ctx, timing);
                            if window.is_empty == 0 {
                                screen_content_changed = true;
                                dtvcc_window_copy_to_screen(self, window_ctx);
                            }
                        }
                    }
                    windows_bitmap >>= 1;
                }
                if screen_content_changed && dtvcc_decoder_has_visible_windows(self) == 0 {
                    self.screen_print(encoder, timing);
                }
            }
        }
    }
    /// Handle DLW Delete Windows
    pub fn handle_delete_windows(
        &mut self,
        mut windows_bitmap: i32,
        encoder: &mut encoder_ctx,
        timing: &mut ccx_common_timing_ctx,
    ) {
        debug!("dtvcc_handle_DLW_DeleteWindows: windows:");
        let mut screen_content_changed = false;
        unsafe {
            if windows_bitmap == 0 {
                debug!("none");
            } else {
                for i in 0..CCX_DTVCC_MAX_WINDOWS {
                    if windows_bitmap & 1 == 1 {
                        debug!("Deleting [W{}]", i);
                        let window = &mut self.windows[i as usize];
                        let window_had_content =
                            window.is_defined == 1 && window.visible == 1 && window.is_empty == 0;
                        if window_had_content {
                            screen_content_changed = true;
                            let window = window as *mut dtvcc_window;
                            dtvcc_window_update_time_hide(window, timing);
                            dtvcc_window_copy_to_screen(self, window);
                            if self.current_window == i as i32 {
                                self.screen_print(encoder, timing);
                            }
                        }
                        let window = &mut self.windows[i as usize];
                        window.is_defined = 0;
                        window.visible = 0;
                        window.time_ms_hide = -1;
                        window.time_ms_show = -1;
                        if self.current_window == i as i32 {
                            self.current_window = -1;
                        }
                    }
                    windows_bitmap >>= 1;
                }
            }
            if screen_content_changed && dtvcc_decoder_has_visible_windows(self) == 0 {
                self.screen_print(encoder, timing);
            }
        }
    }
    /// Process Carriage Return(CR)
    ///
    /// Refer Section 7.1.4.1 and 8.4.9.2 CEA-708-E
    pub fn process_cr(
        &mut self,
        encoder: &mut encoder_ctx,
        timing: &mut ccx_common_timing_ctx,
        no_rollup: bool,
    ) {
        if self.current_window == -1 {
            warn!("dtvcc_process_cr: Window has to be defined first");
            return;
        }
        let window = &mut self.windows[self.current_window as usize];
        let mut rollup_required = false;
        let pd = match dtvcc_window_pd::new(window.attribs.print_direction) {
            Ok(val) => val,
            Err(e) => {
                warn!("{}", e);
                return;
            }
        };
        match pd {
            dtvcc_window_pd::DTVCC_WINDOW_PD_LEFT_RIGHT => {
                window.pen_column = 0;
                if window.pen_row + 1 < window.row_count {
                    window.pen_row += 1;
                } else {
                    rollup_required = true;
                }
            }
            dtvcc_window_pd::DTVCC_WINDOW_PD_RIGHT_LEFT => {
                window.pen_column = window.col_count;
                if window.pen_row + 1 < window.row_count {
                    window.pen_row += 1;
                } else {
                    rollup_required = true;
                }
            }
            dtvcc_window_pd::DTVCC_WINDOW_PD_TOP_BOTTOM => {
                window.pen_row = 0;
                if window.pen_column + 1 < window.col_count {
                    window.pen_column += 1;
                } else {
                    rollup_required = true;
                }
            }
            dtvcc_window_pd::DTVCC_WINDOW_PD_BOTTOM_TOP => {
                window.pen_row = window.row_count;
                if window.pen_column + 1 < window.col_count {
                    window.pen_column += 1;
                } else {
                    rollup_required = true;
                }
            }
        };

        if window.is_defined == 1 {
            debug!("dtvcc_process_cr: rolling up");

            let pen_row = window.pen_row;
            unsafe {
                let window = window as *mut dtvcc_window;
                dtvcc_window_update_time_hide(window, timing);
                dtvcc_window_copy_to_screen(self, window);
                self.screen_print(encoder, timing);

                if rollup_required {
                    if no_rollup {
                        dtvcc_window_clear_row(window, pen_row);
                    } else {
                        dtvcc_window_rollup(self, window);
                    }
                }
                dtvcc_window_update_time_show(window, timing);
            }
        }
    }
    /// Process the character and add it to the current window
    pub fn process_character(&mut self, sym: dtvcc_symbol) {
        unsafe {
            dtvcc_process_character(self, sym);
        }
    }
    pub fn screen_print(&mut self, encoder: &mut encoder_ctx, timing: &mut ccx_common_timing_ctx) {
        debug!("dtvcc_screen_print rust");
        self.cc_count += 1;
        unsafe {
            (*self.tv).cc_count += 1;
            let sn = (*self.tv).service_number;
            let writer = &mut encoder.dtvcc_writers[(sn - 1) as usize];
            dtvcc_screen_update_time_hide(self.tv, get_visible_end(timing, 3));
            dtvcc_writer_output(writer, self, encoder);
            dtvcc_tv_clear(self);
        }
    }
}
