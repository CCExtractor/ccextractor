use std::{
    alloc::{alloc, dealloc, Layout},
    os::raw::c_uchar,
};

use log::{debug, error, warn};

use super::{
    commands::{self, C0CodeSet, C0Command, C1CodeSet, C1Command},
    window::WindowPreset,
};
use crate::{
    bindings::*,
    utils::{is_false, is_true},
};

const CCX_DTVCC_MUSICAL_NOTE_CHAR: u16 = 9836;
const CCX_DTVCC_MAX_WINDOWS: u8 = 8;
const DTVCC_COMMANDS_C0_CODES_DTVCC_C0_EXT1: u8 = 16;
const CCX_DTVCC_SCREENGRID_ROWS: u8 = 75;
const CCX_DTVCC_SCREENGRID_COLUMNS: u8 = 210;
const CCX_DTVCC_MAX_ROWS: u8 = 15;
const CCX_DTVCC_MAX_COLUMNS: u8 = 32 * 2;

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
                used as u8
            } else {
                let mut used = self.handle_extended_char(current_packet, pos + i, block_length - i);
                used += 1; // Since we had CCX_DTVCC_C0_EXT1
                used
            };
            i += consumed;
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
        let code = current_packet[pos as usize];
        let C0Command { command, length } = C0Command::new(code);
        debug!("C0: [{:?}] ({})", command, block_length);
        match command {
            // NUL command does nothing
            C0CodeSet::NUL => {}
            C0CodeSet::ETX => self.process_etx(),
            C0CodeSet::BS => self.process_bs(),
            C0CodeSet::FF => self.process_ff(),
            C0CodeSet::CR => self.process_cr(encoder, timing, no_rollup),
            C0CodeSet::HCR => self.process_hcr(),
            // EXT1 is handled elsewhere as an extended command
            C0CodeSet::EXT1 => {}
            C0CodeSet::P16 => self.process_p16(current_packet, pos + 1),
            C0CodeSet::RESERVED => {}
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
                | C1CodeSet::CW7 => self
                    .handle_set_current_window(code - DTVCC_COMMANDS_C1_CODES_DTVCC_C1_CW0 as u8),
                C1CodeSet::CLW => self.handle_clear_windows(next_code, encoder, timing),
                C1CodeSet::HDW => self.handle_hide_windows(next_code, encoder, timing),
                C1CodeSet::TGW => self.handle_toggle_windows(next_code, encoder, timing),
                C1CodeSet::DLW => self.handle_delete_windows(next_code, encoder, timing),
                C1CodeSet::DSW => self.handle_display_windows(next_code, timing),
                C1CodeSet::DLY => dtvcc_handle_DLY_Delay(self, next_code),
                C1CodeSet::DLC => dtvcc_handle_DLC_DelayCancel(self),
                C1CodeSet::RST => dtvcc_handle_RST_Reset(self),
                C1CodeSet::SPA => self.handle_set_pen_attributes(current_packet, pos),
                C1CodeSet::SPC => self.handle_set_pen_color(current_packet, pos),
                C1CodeSet::SPL => self.handle_set_pen_location(current_packet, pos),
                C1CodeSet::SWA => self.handle_set_window_attributes(current_packet, pos),
                C1CodeSet::DF0
                | C1CodeSet::DF1
                | C1CodeSet::DF2
                | C1CodeSet::DF3
                | C1CodeSet::DF4
                | C1CodeSet::DF5
                | C1CodeSet::DF6
                | C1CodeSet::DF7 => self.handle_define_windows(
                    code - DTVCC_COMMANDS_C1_CODES_DTVCC_C1_DF0 as u8,
                    current_packet,
                    pos,
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
    /// G2 (20-7F) => Mostly unmapped, except for a few characters.
    /// G3 (A0-FF) => A0 is the CC symbol, everything else reserved for future expansion in EIA708
    /// C2 (00-1F) => Reserved for future extended misc. control and captions command codes
    /// C3 (80-9F) => Reserved for future extended misc. control and captions command codes
    /// WARN: This code is completely untested due to lack of samples. Just following specs!
    /// Returns number of used bytes, usually 1 (since EXT1 is not counted).
    pub fn handle_extended_char(
        &mut self,
        current_packet: &mut [c_uchar],
        pos: u8,
        block_length: u8,
    ) -> u8 {
        let code = current_packet[pos as usize];
        debug!(
            "dtvcc_handle_extended_char, first data code: [{}], length: [{}]",
            code as char, block_length
        );

        match code {
            0..=0x1F => commands::handle_C2(code),
            0x20..=0x7F => {
                let val = unsafe { dtvcc_get_internal_from_G2(code) };
                let sym = dtvcc_symbol::new(val as u16);
                self.process_character(sym);
                1
            }
            0x80..=0x9F => commands::handle_C3(code, current_packet[(pos + 1) as usize]),
            _ => {
                let val = unsafe { dtvcc_get_internal_from_G3(code) };
                let sym = dtvcc_symbol::new(val as u16);
                self.process_character(sym);
                1
            }
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
                        let window_had_content = is_true(window.is_defined)
                            && is_true(window.visible)
                            && is_false(window.is_empty);
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
                        if is_true(window.visible) {
                            screen_content_changed = true;
                            window.visible = 0;
                            let window_ctx = window as *mut dtvcc_window;
                            dtvcc_window_update_time_hide(window_ctx, timing);
                            if is_false(window.is_empty) {
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
                    if windows_bitmap & 1 == 1 && is_true(window.is_defined) {
                        if is_false(window.visible) {
                            debug!("[W-{}: 0->1]", i);
                            window.visible = 1;
                            dtvcc_window_update_time_show(window_ctx, timing);
                        } else {
                            debug!("[W-{}: 1->0]", i);
                            window.visible = 0;
                            dtvcc_window_update_time_hide(window_ctx, timing);
                            if is_false(window.is_empty) {
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
                        let window_had_content = is_true(window.is_defined)
                            && is_true(window.visible)
                            && is_false(window.is_empty);
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
    /// Handle DSW Display Windows
    pub fn handle_display_windows(
        &mut self,
        mut windows_bitmap: i32,
        timing: &mut ccx_common_timing_ctx,
    ) {
        debug!("dtvcc_handle_DSW_DisplayWindows: windows:");
        if windows_bitmap == 0 {
            debug!("none");
        } else {
            for i in 0..CCX_DTVCC_MAX_WINDOWS {
                if windows_bitmap & 1 == 1 {
                    let window = &mut self.windows[i as usize];
                    debug!("[W{}]", i);
                    if is_false(window.is_defined) {
                        error!("Window {} was not defined", i);
                        continue;
                    }
                    if is_false(window.visible) {
                        window.visible = 1;
                        unsafe {
                            dtvcc_window_update_time_show(window, timing);
                        }
                    }
                }
                windows_bitmap >>= 1;
            }
        }
    }
    /// Handle DFx Define Windows
    pub fn handle_define_windows(
        &mut self,
        window_id: u8,
        current_packet: &[c_uchar],
        pos: u8,
        timing: &mut ccx_common_timing_ctx,
    ) {
        debug!(
            "dtvcc_handle_DFx_DefineWindow: W[{}], attributes:",
            window_id
        );
        let window = &mut self.windows[window_id as usize];
        let pos = pos as usize;
        let block = &current_packet[(pos + 1)..=(pos + 6)];
        let is_command_repeated = window
            .commands
            .iter()
            .zip(block.iter())
            .all(|(x, y)| x == y);

        if is_true(window.is_defined) && is_command_repeated {
            // When a decoder receives a DefineWindow command for an existing window, the
            // command is to be ignored if the command parameters are unchanged from the
            // previous window definition.
            debug!("dtvcc_handle_DFx_DefineWindow: Repeated window definition, ignored\n");
            return;
        }

        window.number = window_id as i32;
        let priority = (current_packet[pos + 1]) & 0x7;
        let col_lock = (current_packet[pos + 1] >> 3) & 0x1;
        let row_lock = (current_packet[pos + 1] >> 4) & 0x1;
        let visible = (current_packet[pos + 1] >> 5) & 0x1;
        let mut anchor_vertical = current_packet[pos + 2] & 0x7f;
        let relative_pos = current_packet[pos + 2] >> 7;
        let mut anchor_horizontal = current_packet[pos + 3];
        let row_count = (current_packet[pos + 4] & 0xf) + 1;
        let anchor_point = current_packet[pos + 4] >> 4;
        let col_count = (current_packet[pos + 5] & 0x3f) + 1;
        let mut pen_style = current_packet[pos + 6] & 0x7;
        let mut win_style = (current_packet[pos + 6] >> 3) & 0x7;

        let mut do_clear_window = false;

        debug!("Visible: [{}]", if is_true(visible) { "Yes" } else { "No" });
        debug!("Priority: [{}]", priority);
        debug!("Row count: [{}]", row_count);
        debug!("Column count: [{}]", col_count);
        debug!("Anchor point: [{}]", anchor_point);
        debug!("Anchor vertical: [{}]", anchor_vertical);
        debug!("Anchor horizontal: [{}]", anchor_horizontal);
        debug!(
            "Relative pos: [{}]",
            if is_true(relative_pos) { "Yes" } else { "No" }
        );
        debug!(
            "Row lock: [{}]",
            if is_true(row_lock) { "Yes" } else { "No" }
        );
        debug!(
            "Column lock: [{}]",
            if is_true(col_lock) { "Yes" } else { "No" }
        );
        debug!("Pen style: [{}]", pen_style);
        debug!("Win style: [{}]", win_style);

        // Korean samples have "anchor_vertical" and "anchor_horizontal" mixed up,
        // this seems to be an encoder issue, but we can workaround it
        if anchor_vertical > CCX_DTVCC_SCREENGRID_ROWS - row_count {
            anchor_vertical = CCX_DTVCC_SCREENGRID_ROWS - row_count;
        }
        if anchor_horizontal > CCX_DTVCC_SCREENGRID_COLUMNS - col_count {
            anchor_horizontal = CCX_DTVCC_SCREENGRID_COLUMNS - col_count;
        }

        window.priority = priority as i32;
        window.col_lock = col_lock as i32;
        window.row_lock = row_lock as i32;
        window.visible = visible as i32;
        window.anchor_vertical = anchor_vertical as i32;
        window.relative_pos = relative_pos as i32;
        window.anchor_horizontal = anchor_horizontal as i32;
        window.row_count = row_count as i32;
        window.anchor_point = anchor_point as i32;
        window.col_count = col_count as i32;

        // If changing the style of an existing window delete contents
        if win_style > 0 && is_false(window.is_defined) && window.win_style != win_style as i32 {
            do_clear_window = true;
        }

        /* If the window doesn't exist and win style==0 then default to win_style=1 */
        if win_style == 0 && is_false(window.is_defined) {
            win_style = 1;
        }
        /* If the window doesn't exist and pen style==0 then default to pen_style=1 */
        if pen_style == 0 && is_false(window.is_defined) {
            pen_style = 1;
        }

        if win_style > 0 && win_style < 8 {
            let preset = WindowPreset::get_style(win_style);
            if let Ok(preset) = preset {
                window.set_style(preset);
            }
        }

        if pen_style > 0 {
            //TODO apply static pen_style preset
            window.pen_style = pen_style as i32;
        }
        unsafe {
            if is_false(window.is_defined) {
                // If the window is being created, all character positions in the window
                // are set to the fill color and the pen location is set to (0,0)
                window.pen_column = 0;
                window.pen_row = 0;
                if is_false(window.memory_reserved) {
                    for i in 0..CCX_DTVCC_MAX_ROWS as usize {
                        let layout = Layout::array::<dtvcc_symbol>(CCX_DTVCC_MAX_COLUMNS as usize);
                        if let Err(e) = layout {
                            error!("dtvcc_handle_DFx_DefineWindow: Incorrect Layout, {}", e);
                        } else {
                            let ptr = alloc(layout.unwrap());
                            if ptr.is_null() {
                                error!("dtvcc_handle_DFx_DefineWindow: Not enough memory",);
                            }
                            // Exit here?
                            window.rows[i] = ptr as *mut dtvcc_symbol;
                        }
                    }
                    window.memory_reserved = 1;
                }
                window.is_defined = 1;
                dtvcc_window_clear_text(window);
            } else if do_clear_window {
                dtvcc_window_clear_text(window);
            }

            window
                .commands
                .iter_mut()
                .zip(block.iter())
                .for_each(|(command, val)| *command = *val);

            if is_true(window.visible) {
                dtvcc_window_update_time_show(window, timing);
            }
            if is_false(window.memory_reserved) {
                for i in 0..CCX_DTVCC_MAX_ROWS as usize {
                    let layout = Layout::array::<dtvcc_symbol>(CCX_DTVCC_MAX_COLUMNS as usize);
                    if let Err(e) = layout {
                        error!("dtvcc_handle_DFx_DefineWindow: Incorrect Layout, {}", e);
                    } else {
                        dealloc(window.rows[i] as *mut u8, layout.unwrap());
                    }
                }
            }
            // ...also makes the defined windows the current window
            self.handle_set_current_window(window_id);
        }
    }
    /// Handle SPA Set Pen Attributes
    pub fn handle_set_pen_attributes(&mut self, current_packet: &[c_uchar], pos: u8) {
        if self.current_window == -1 {
            warn!("dtvcc_handle_SPA_SetPenAttributes: Window has to be defined first");
            return;
        }

        let pos = pos as usize;
        let pen_size = (current_packet[pos + 1]) & 0x3;
        let offset = (current_packet[pos + 1] >> 2) & 0x3;
        let text_tag = (current_packet[pos + 1] >> 4) & 0xf;
        let font_tag = (current_packet[pos + 2]) & 0x7;
        let edge_type = (current_packet[pos + 2] >> 3) & 0x7;
        let underline = (current_packet[pos + 2] >> 6) & 0x1;
        let italic = (current_packet[pos + 2] >> 7) & 0x1;
        debug!("dtvcc_handle_SPA_SetPenAttributes: attributes: ");
        debug!(
            "Pen size: [{}]     Offset: [{}]  Text tag: [{}]   Font tag: [{}]",
            pen_size, offset, text_tag, font_tag
        );
        debug!(
            "Edge type: [{}]  Underline: [{}]    Italic: [{}]",
            edge_type, underline, italic
        );

        let window = &mut self.windows[self.current_window as usize];
        if window.pen_row == -1 {
            warn!("dtvcc_handle_SPA_SetPenAttributes: cant't set attributes for undefined row");
            return;
        }

        let pen = &mut window.pen_attribs_pattern;
        pen.pen_size = pen_size as i32;
        pen.offset = offset as i32;
        pen.text_tag = text_tag as i32;
        pen.font_tag = font_tag as i32;
        pen.edge_type = edge_type as i32;
        pen.underline = underline as i32;
        pen.italic = italic as i32;
    }
    /// Handle SPC Set Pen Color
    pub fn handle_set_pen_color(&mut self, current_packet: &[c_uchar], pos: u8) {
        if self.current_window == -1 {
            warn!("dtvcc_handle_SPC_SetPenColor: Window has to be defined first");
            return;
        }

        let pos = pos as usize;
        let fg_color = (current_packet[pos + 1]) & 0x3f;
        let fg_opacity = (current_packet[pos + 1] >> 6) & 0x03;
        let bg_color = (current_packet[pos + 2]) & 0x3f;
        let bg_opacity = (current_packet[pos + 2] >> 6) & 0x03;
        let edge_color = (current_packet[pos + 3]) & 0x3f;
        debug!("dtvcc_handle_SPC_SetPenColor: attributes: ");
        debug!(
            "Foreground color: [{}]     Foreground opacity: [{}]",
            fg_color, fg_opacity
        );
        debug!(
            "Background color: [{}]     Background opacity: [{}]",
            bg_color, bg_opacity
        );
        debug!("Edge color: [{}]", edge_color);

        let window = &mut self.windows[self.current_window as usize];
        if window.pen_row == -1 {
            warn!("dtvcc_handle_SPC_SetPenColor: cant't set attributes for undefined row");
            return;
        }

        let color = &mut window.pen_color_pattern;
        color.fg_color = fg_color as i32;
        color.fg_opacity = fg_opacity as i32;
        color.bg_color = bg_color as i32;
        color.bg_opacity = bg_opacity as i32;
        color.edge_color = edge_color as i32;
    }
    /// Handle SPL Set Pen Location
    pub fn handle_set_pen_location(&mut self, current_packet: &[c_uchar], pos: u8) {
        if self.current_window == -1 {
            warn!("dtvcc_handle_SPL_SetPenLocation: Window has to be defined first");
            return;
        }

        debug!("dtvcc_handle_SPL_SetPenLocation: attributes: ");
        let pos = pos as usize;
        let row = current_packet[pos + 1] & 0x0f;
        let col = current_packet[pos + 2] & 0x3f;
        debug!("Row: [{}]     Column: [{}]", row, col);

        let window = &mut self.windows[self.current_window as usize];
        window.pen_row = row as i32;
        window.pen_column = col as i32;
    }
    /// Handle SWA Set Window Attributes
    pub fn handle_set_window_attributes(&mut self, current_packet: &[c_uchar], pos: u8) {
        if self.current_window == -1 {
            warn!("dtvcc_handle_SWA_SetWindowAttributes: Window has to be defined first");
            return;
        }

        let pos = pos as usize;
        let fill_color = (current_packet[pos + 1]) & 0x3f;
        let fill_opacity = (current_packet[pos + 1] >> 6) & 0x03;
        let border_color = (current_packet[pos + 2]) & 0x3f;
        let border_type01 = (current_packet[pos + 2] >> 6) & 0x03;
        let justify = (current_packet[pos + 3]) & 0x03;
        let scroll_dir = (current_packet[pos + 3] >> 2) & 0x03;
        let print_dir = (current_packet[pos + 3] >> 4) & 0x03;
        let word_wrap = (current_packet[pos + 3] >> 6) & 0x01;
        let border_type = ((current_packet[pos + 3] >> 5) & 0x04) | border_type01;
        let display_eff = (current_packet[pos + 4]) & 0x03;
        let effect_dir = (current_packet[pos + 4] >> 2) & 0x03;
        let effect_speed = (current_packet[pos + 4] >> 4) & 0x0f;
        debug!("dtvcc_handle_SWA_SetWindowAttributes: attributes: ");
        debug!(
            "Fill color: [{}]     Fill opacity: [{}]    Border color: [{}]  Border type: [{}]",
            fill_color, fill_opacity, border_color, border_type
        );
        debug!(
            "Justify: [{}]       Scroll dir: [{}]       Print dir: [{}]    Word wrap: [{}]",
            justify, scroll_dir, print_dir, word_wrap
        );
        debug!(
            "Border type: [{}]      Display eff: [{}]      Effect dir: [{}] Effect speed: [{}]",
            border_type, display_eff, effect_dir, effect_speed
        );

        let window_attribts = &mut self.windows[self.current_window as usize].attribs;
        window_attribts.fill_color = fill_color as i32;
        window_attribts.fill_opacity = fill_opacity as i32;
        window_attribts.border_color = border_color as i32;
        window_attribts.justify = justify as i32;
        window_attribts.scroll_direction = scroll_dir as i32;
        window_attribts.print_direction = print_dir as i32;
        window_attribts.word_wrap = word_wrap as i32;
        window_attribts.border_type = border_type as i32;
        window_attribts.display_effect = display_eff as i32;
        window_attribts.effect_direction = effect_dir as i32;
        window_attribts.effect_speed = effect_speed as i32;
    }
    /// handle CWx Set Current Window
    pub fn handle_set_current_window(&mut self, window_id: u8) {
        debug!("dtvcc_handle_CWx_SetCurrentWindow: [{}]", window_id);
        if is_true(self.windows[window_id as usize].is_defined) {
            self.current_window = window_id as i32;
        } else {
            debug!(
                "dtvcc_handle_CWx_SetCurrentWindow: window [{}] is not defined",
                window_id
            );
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

        if is_true(window.is_defined) {
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
    /// Process Horizontal Carriage Return (HCR)
    pub fn process_hcr(&mut self) {
        if self.current_window == -1 {
            warn!("dtvcc_process_hcr: Window has to be defined first");
            return;
        }
        let window = &mut self.windows[self.current_window as usize];
        window.pen_column = 0;
        unsafe {
            dtvcc_window_clear_row(window, window.pen_row);
        }
    }
    /// Process Form Feed (FF)
    pub fn process_ff(&mut self) {
        if self.current_window == -1 {
            warn!("dtvcc_process_ff: Window has to be defined first");
            return;
        }
        let window = &mut self.windows[self.current_window as usize];
        window.pen_column = 0;
        window.pen_row = 0;
    }
    /// Process Backspace (BS)
    pub fn process_bs(&mut self) {
        if self.current_window == -1 {
            warn!("dtvcc_process_bs: Window has to be defined first");
            return;
        }
        //it looks strange, but in some videos (rarely) we have a backspace command
        //we just print one character over another
        let window = &mut self.windows[self.current_window as usize];
        let pd = match dtvcc_window_pd::new(window.attribs.print_direction) {
            Ok(val) => val,
            Err(e) => {
                warn!("{}", e);
                return;
            }
        };
        match pd {
            dtvcc_window_pd::DTVCC_WINDOW_PD_LEFT_RIGHT => {
                if window.pen_column > 0 {
                    window.pen_column -= 1;
                }
            }
            dtvcc_window_pd::DTVCC_WINDOW_PD_RIGHT_LEFT => {
                if window.pen_column + 1 < window.col_count {
                    window.pen_column += 1;
                }
            }
            dtvcc_window_pd::DTVCC_WINDOW_PD_TOP_BOTTOM => {
                if window.pen_row > 0 {
                    window.pen_row -= 1;
                }
            }
            dtvcc_window_pd::DTVCC_WINDOW_PD_BOTTOM_TOP => {
                if window.pen_row + 1 < window.row_count {
                    window.pen_row += 1;
                }
            }
        };
    }
    /// Process P16
    /// Used for Code space extension for 16 bit charsets
    pub fn process_p16(&mut self, current_packet: &mut [c_uchar], pos: u8) {
        if self.current_window == -1 {
            warn!("dtvcc_process_p16: Window has to be defined first");
            return;
        }
        let pos = pos as usize;
        let sym = dtvcc_symbol::new_16(current_packet[pos], current_packet[pos + 1]);
        debug!("dtvcc_process_p16: [{:4X}]", sym.sym);
        self.process_character(sym);
    }
    /// Process End of Text (ETX)
    pub fn process_etx(&mut self) {}
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
