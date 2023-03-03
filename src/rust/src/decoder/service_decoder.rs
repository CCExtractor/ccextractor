//! Caption Service Decoder
//!
//! Caption Service decoder processes service blocks and handles the different [commands][super::commands] received

use std::{
    alloc::{alloc, dealloc, Layout},
    os::raw::c_uchar,
};

use super::commands::{self, C0CodeSet, C0Command, C1CodeSet, C1Command};
use super::window::{PenPreset, WindowPreset};
use super::{
    CCX_DTVCC_MAX_COLUMNS, CCX_DTVCC_MAX_ROWS, CCX_DTVCC_SCREENGRID_COLUMNS,
    CCX_DTVCC_SCREENGRID_ROWS,
};
use crate::{
    bindings::*,
    decoder::output::Writer,
    utils::{is_false, is_true},
};

use log::{debug, error, warn};

const CCX_DTVCC_MUSICAL_NOTE_CHAR: u16 = 9836;
const CCX_DTVCC_MAX_WINDOWS: u8 = 8;
const DTVCC_COMMANDS_C0_CODES_DTVCC_C0_EXT1: u8 = 16;

impl dtvcc_service_decoder {
    /// Process service block and call handlers for the respective codesets
    pub fn process_service_block(
        &mut self,
        block: &[u8],
        encoder: &mut encoder_ctx,
        timing: &mut ccx_common_timing_ctx,
        no_rollup: bool,
    ) {
        let mut i = 0;
        while i < block.len() {
            let consumed = if block[i] != DTVCC_COMMANDS_C0_CODES_DTVCC_C0_EXT1 {
                let used = match block[i] {
                    0..=0x1F => self.handle_C0(&block[i..], encoder, timing, no_rollup),
                    0x20..=0x7F => self.handle_G0(&block[i..]),
                    0x80..=0x9F => self.handle_C1(&block[i..], encoder, timing),
                    _ => self.handle_G1(&block[i..]),
                };
                if used == -1 {
                    warn!("dtvcc_process_service_block: There was a problem handling the data.");
                    return;
                }
                used as u8
            } else {
                let mut used = self.handle_extended_char(&block[1..]);
                used += 1; // Since we had CCX_DTVCC_C0_EXT1
                used
            };
            i += consumed as usize;
        }
    }

    // -------------------------- C0 Commands-------------------------

    /// C0 - Code Set - Miscellaneous Control Codes
    pub fn handle_C0(
        &mut self,
        block: &[u8],
        encoder: &mut encoder_ctx,
        timing: &mut ccx_common_timing_ctx,
        no_rollup: bool,
    ) -> i32 {
        let code = block[0];
        let C0Command { command, length } = C0Command::new(code);
        debug!("C0: [{:?}] ({})", command, block.len());
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
            C0CodeSet::P16 => self.process_p16(&block[1..]),
            C0CodeSet::RESERVED => {}
        }
        if length as usize > block.len() {
            warn!(
                "dtvcc_handle_C0: command is {} bytes long but we only have {}",
                length,
                block.len()
            );
            return -1;
        }
        length as i32
    }

    /// Process Carriage Return(CR)
    ///
    /// Refer Section 7.1.4.1 and 8.4.9.2 CEA-708-E
    ///
    /// Carriage Return (CR) moves the current entry point to the beginning of the next row. If the next row is
    /// below the visible window, the window “rolls up"
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
            let pen_row = window.pen_row;
            window.update_time_hide(timing);

            if rollup_required {
                debug!("dtvcc_process_cr: rolling up");
                self.copy_to_screen(&self.windows[self.current_window as usize]);
                self.screen_print(encoder, timing);
                if no_rollup {
                    self.windows[self.current_window as usize].clear_row(pen_row as usize);
                } else {
                    self.windows[self.current_window as usize].rollup();
                }
            }
            self.windows[self.current_window as usize].update_time_show(timing);
        }
    }

    /// Process Horizontal Carriage Return (HCR)
    ///
    /// Refer Section 7.1.4.1 CEA-708-E
    ///
    /// Horizontal Carriage Return (HCR) moves the current entry point to the beginning of the current row
    /// without row increment or decrement. It shall erase all text on the row
    pub fn process_hcr(&mut self) {
        if self.current_window == -1 {
            warn!("dtvcc_process_hcr: Window has to be defined first");
            return;
        }
        let window = &mut self.windows[self.current_window as usize];
        window.pen_column = 0;
        window.clear_row(window.pen_row as usize);
    }

    /// Process Form Feed (FF)
    ///
    /// Refer Section 7.1.4.1 CEA-708-E
    ///
    /// Form Feed (FF) erases all text in the window and moves the cursor to the first position (0,0)
    pub fn process_ff(&mut self) {
        if self.current_window == -1 {
            warn!("dtvcc_process_ff: Window has to be defined first");
            return;
        }
        let window = &mut self.windows[self.current_window as usize];
        window.pen_column = 0;
        window.pen_row = 0;
        window.clear_text();
    }

    /// Process Backspace (BS)
    ///
    /// Backspace (BS) moves the cursor back by one position in the print direction
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
    ///
    /// Used for Code space extension for 16 bit charsets
    pub fn process_p16(&mut self, block: &[c_uchar]) {
        if self.current_window == -1 {
            warn!("dtvcc_process_p16: Window has to be defined first");
            return;
        }
        let sym = dtvcc_symbol::new_16(block[0], block[1]);
        debug!("dtvcc_process_p16: [{:4X}]", sym.sym);
        self.process_character(sym);
    }

    /// Process End of Text (ETX)
    pub fn process_etx(&mut self) {}

    // -------------------------- C1 Commands-------------------------

    /// C1 - Code Set - Captioning Command Control Codes
    pub fn handle_C1(
        &mut self,
        block: &[c_uchar],
        encoder: &mut encoder_ctx,
        timing: &mut ccx_common_timing_ctx,
    ) -> i32 {
        let code = block[0];
        let C1Command {
            command,
            length,
            name,
        } = C1Command::new(code);

        if length as usize > block.len() {
            warn!("Warning: Not enough bytes for command.");
            return -1;
        }

        debug!("C1: [{:?}] [{}] ({})", command, name, length);
        match command {
            C1CodeSet::CW0
            | C1CodeSet::CW1
            | C1CodeSet::CW2
            | C1CodeSet::CW3
            | C1CodeSet::CW4
            | C1CodeSet::CW5
            | C1CodeSet::CW6
            | C1CodeSet::CW7 => {
                self.handle_set_current_window(code - DTVCC_COMMANDS_C1_CODES_DTVCC_C1_CW0 as u8)
            }
            C1CodeSet::CLW => self.handle_clear_windows(block[1], encoder, timing),
            C1CodeSet::HDW => self.handle_hide_windows(block[1], encoder, timing),
            C1CodeSet::TGW => self.handle_toggle_windows(block[1], encoder, timing),
            C1CodeSet::DLW => self.handle_delete_windows(block[1], encoder, timing),
            C1CodeSet::DSW => self.handle_display_windows(block[1], timing),
            C1CodeSet::DLY => self.handle_delay(block[1]),
            C1CodeSet::DLC => self.handle_delay_cancel(),
            C1CodeSet::RST => self.handle_reset(),
            C1CodeSet::SPA => self.handle_set_pen_attributes(&block[1..]),
            C1CodeSet::SPC => self.handle_set_pen_color(&block[1..]),
            C1CodeSet::SPL => self.handle_set_pen_location(&block[1..]),
            C1CodeSet::SWA => self.handle_set_window_attributes(&block[1..]),
            C1CodeSet::DF0
            | C1CodeSet::DF1
            | C1CodeSet::DF2
            | C1CodeSet::DF3
            | C1CodeSet::DF4
            | C1CodeSet::DF5
            | C1CodeSet::DF6
            | C1CodeSet::DF7 => self.handle_define_windows(
                code - DTVCC_COMMANDS_C1_CODES_DTVCC_C1_DF0 as u8,
                &block[1..],
                timing,
            ),
            C1CodeSet::RESERVED => {
                warn!("Warning, Found Reserved codes, ignored");
            }
        };
        length as i32
    }

    /// CLW Clear Windows
    ///
    /// Clear text from all windows specified by bitmap. If window had content then
    /// all text is copied to the TV screen
    pub fn handle_clear_windows(
        &mut self,
        mut windows_bitmap: u8,
        encoder: &mut encoder_ctx,
        timing: &mut ccx_common_timing_ctx,
    ) {
        debug!("dtvcc_handle_CLW_ClearWindows: windows:");
        let mut screen_content_changed = false;
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
                        window.update_time_hide(timing);
                        self.copy_to_screen(&self.windows[i as usize]);
                    }
                    self.windows[i as usize].clear_text();
                }
                windows_bitmap >>= 1;
            }
        }
        if screen_content_changed {
            self.screen_print(encoder, timing);
        }
    }

    /// HDW Hide Windows
    ///
    /// Hide all windows specified by bitmap. If window was not empty then
    /// all text is copied to the TV screen
    pub fn handle_hide_windows(
        &mut self,
        mut windows_bitmap: u8,
        encoder: &mut encoder_ctx,
        timing: &mut ccx_common_timing_ctx,
    ) {
        debug!("dtvcc_handle_HDW_HideWindows: windows:");
        if windows_bitmap == 0 {
            debug!("none");
        } else {
            let mut screen_content_changed = false;
            for i in 0..CCX_DTVCC_MAX_WINDOWS {
                if windows_bitmap & 1 == 1 {
                    let window = &mut self.windows[i as usize];
                    debug!("[W{}]", i);
                    if is_true(window.visible) {
                        screen_content_changed = true;
                        window.visible = 0;
                        window.update_time_hide(timing);
                        if is_false(window.is_empty) {
                            self.copy_to_screen(&self.windows[i as usize]);
                        }
                    }
                }
                windows_bitmap >>= 1;
            }
            if screen_content_changed && !self.has_visible_windows() {
                self.screen_print(encoder, timing);
            }
        }
    }

    /// TGW Toggle Windows
    ///
    /// Toggle visibility of all windows specified by bitmap. If window was not empty then
    /// all text is copied to the TV screen
    pub fn handle_toggle_windows(
        &mut self,
        mut windows_bitmap: u8,
        encoder: &mut encoder_ctx,
        timing: &mut ccx_common_timing_ctx,
    ) {
        debug!("dtvcc_handle_TGW_ToggleWindows: windows:");
        if windows_bitmap == 0 {
            debug!("none");
        } else {
            let mut screen_content_changed = false;
            for i in 0..CCX_DTVCC_MAX_WINDOWS {
                let window = &mut self.windows[i as usize];
                if windows_bitmap & 1 == 1 && is_true(window.is_defined) {
                    if is_false(window.visible) {
                        debug!("[W-{}: 0->1]", i);
                        window.visible = 1;
                        window.update_time_show(timing);
                    } else {
                        debug!("[W-{}: 1->0]", i);
                        window.visible = 0;
                        window.update_time_hide(timing);
                        if is_false(window.is_empty) {
                            screen_content_changed = true;
                            self.copy_to_screen(&self.windows[i as usize]);
                        }
                    }
                }
                windows_bitmap >>= 1;
            }
            if screen_content_changed && !self.has_visible_windows() {
                self.screen_print(encoder, timing);
            }
        }
    }

    /// DLW Delete Windows
    ///
    /// Delete all windows specified by bitmap. If window had content then
    /// all text is copied to the TV screen
    pub fn handle_delete_windows(
        &mut self,
        mut windows_bitmap: u8,
        encoder: &mut encoder_ctx,
        timing: &mut ccx_common_timing_ctx,
    ) {
        debug!("dtvcc_handle_DLW_DeleteWindows: windows:");
        let mut screen_content_changed = false;
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
                        window.update_time_hide(timing);
                        self.copy_to_screen(&self.windows[i as usize]);
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
        if screen_content_changed && !self.has_visible_windows() {
            self.screen_print(encoder, timing);
        }
    }

    /// DSW Display Windows
    ///
    /// Display all windows specified by bitmap.
    pub fn handle_display_windows(
        &mut self,
        mut windows_bitmap: u8,
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
                        window.update_time_show(timing);
                    }
                }
                windows_bitmap >>= 1;
            }
        }
    }

    /// DFx Define Windows
    ///
    /// Define a new window with the provided attributes. New window is now the current window
    pub fn handle_define_windows(
        &mut self,
        window_id: u8,
        block: &[c_uchar],
        timing: &mut ccx_common_timing_ctx,
    ) {
        debug!(
            "dtvcc_handle_DFx_DefineWindow: W[{}], attributes:",
            window_id
        );
        let window = &mut self.windows[window_id as usize];
        let block = &block[..=5];
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
        let priority = (block[0]) & 0x7;
        let col_lock = (block[0] >> 3) & 0x1;
        let row_lock = (block[0] >> 4) & 0x1;
        let visible = (block[0] >> 5) & 0x1;
        let mut anchor_vertical = block[1] & 0x7f;
        let relative_pos = block[1] >> 7;
        let mut anchor_horizontal = block[2];
        let row_count = (block[3] & 0xf) + 1;
        let anchor_point = block[3] >> 4;
        let col_count = (block[4] & 0x3f) + 1;
        let mut pen_style = block[5] & 0x7;
        let mut win_style = (block[5] >> 3) & 0x7;

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
            let preset = PenPreset::get_style(pen_style);
            if let Ok(preset) = preset {
                window.set_pen_style(preset);
            }
            window.pen_style = pen_style as i32;
        }
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
                        let ptr = unsafe { alloc(layout.unwrap()) };
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
            window.clear_text();
        } else if do_clear_window {
            window.clear_text();
        }

        window
            .commands
            .iter_mut()
            .zip(block.iter())
            .for_each(|(command, val)| *command = *val);

        if is_true(window.visible) {
            window.update_time_show(timing);
        }
        if is_false(window.memory_reserved) {
            for i in 0..CCX_DTVCC_MAX_ROWS as usize {
                let layout = Layout::array::<dtvcc_symbol>(CCX_DTVCC_MAX_COLUMNS as usize);
                if let Err(e) = layout {
                    error!("dtvcc_handle_DFx_DefineWindow: Incorrect Layout, {}", e);
                } else {
                    unsafe { dealloc(window.rows[i] as *mut u8, layout.unwrap()) };
                }
            }
        }
        // ...also makes the defined windows the current window
        self.handle_set_current_window(window_id);
    }

    /// SPA Set Pen Attributes
    ///
    /// Change pen attributes
    pub fn handle_set_pen_attributes(&mut self, block: &[c_uchar]) {
        if self.current_window == -1 {
            warn!("dtvcc_handle_SPA_SetPenAttributes: Window has to be defined first");
            return;
        }

        let pen_size = (block[0]) & 0x3;
        let offset = (block[0] >> 2) & 0x3;
        let text_tag = (block[0] >> 4) & 0xf;
        let font_tag = (block[1]) & 0x7;
        let edge_type = (block[1] >> 3) & 0x7;
        let underline = (block[1] >> 6) & 0x1;
        let italic = (block[1] >> 7) & 0x1;
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

    /// SPC Set Pen Color
    ///
    /// Change pen color
    pub fn handle_set_pen_color(&mut self, block: &[c_uchar]) {
        if self.current_window == -1 {
            warn!("dtvcc_handle_SPC_SetPenColor: Window has to be defined first");
            return;
        }

        let fg_color = (block[0]) & 0x3f;
        let fg_opacity = (block[0] >> 6) & 0x03;
        let bg_color = (block[1]) & 0x3f;
        let bg_opacity = (block[1] >> 6) & 0x03;
        let edge_color = (block[2]) & 0x3f;
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

    /// SPL Set Pen Location
    ///
    /// Change pen location
    pub fn handle_set_pen_location(&mut self, block: &[c_uchar]) {
        if self.current_window == -1 {
            warn!("dtvcc_handle_SPL_SetPenLocation: Window has to be defined first");
            return;
        }

        debug!("dtvcc_handle_SPL_SetPenLocation: attributes: ");
        let row = block[0] & 0x0f;
        let col = block[1] & 0x3f;
        debug!("Row: [{}]     Column: [{}]", row, col);

        let window = &mut self.windows[self.current_window as usize];
        window.pen_row = row as i32;
        window.pen_column = col as i32;
    }

    /// SWA Set Window Attributes
    ///
    /// Change window attributes
    pub fn handle_set_window_attributes(&mut self, block: &[c_uchar]) {
        if self.current_window == -1 {
            warn!("dtvcc_handle_SWA_SetWindowAttributes: Window has to be defined first");
            return;
        }

        let fill_color = (block[0]) & 0x3f;
        let fill_opacity = (block[0] >> 6) & 0x03;
        let border_color = (block[1]) & 0x3f;
        let border_type01 = (block[1] >> 6) & 0x03;
        let justify = (block[2]) & 0x03;
        let scroll_dir = (block[2] >> 2) & 0x03;
        let print_dir = (block[2] >> 4) & 0x03;
        let word_wrap = (block[2] >> 6) & 0x01;
        let border_type = ((block[2] >> 5) & 0x04) | border_type01;
        let display_eff = (block[3]) & 0x03;
        let effect_dir = (block[3] >> 2) & 0x03;
        let effect_speed = (block[3] >> 4) & 0x0f;
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

    /// CWx Set Current Window
    ///
    /// Change current window to the window id provided
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

    /// DLY Delay
    pub fn handle_delay(&mut self, tenths_of_sec: u8) {
        debug!(
            "dtvcc_handle_DLY_Delay: dely for {} tenths of second",
            tenths_of_sec
        );
    }

    /// DLC Delay Cancel
    pub fn handle_delay_cancel(&mut self) {
        debug!("dtvcc_handle_DLC_DelayCancel");
    }

    /// RST Reset
    ///
    /// Clear text from all windows and set all windows to undefined and hidden
    pub fn handle_reset(&mut self) {
        for id in 0..CCX_DTVCC_MAX_WINDOWS as usize {
            let window = &mut self.windows[id];
            window.clear_text();
            window.is_defined = 0;
            window.visible = 0;
            window.commands.fill(0);
        }
        self.current_window = -1;
        unsafe { (*self.tv).clear() };
    }

    /// Print the contents of tv screen to the output file
    pub fn screen_print(&mut self, encoder: &mut encoder_ctx, timing: &mut ccx_common_timing_ctx) {
        debug!("dtvcc_screen_print rust");
        self.cc_count += 1;
        unsafe {
            let tv = &mut (*self.tv);
            tv.cc_count += 1;
            let sn = tv.service_number;
            let writer_ctx = &mut encoder.dtvcc_writers[(sn - 1) as usize];

            tv.update_time_hide(timing.get_visible_end(3));
            let mut writer = Writer::new(
                &mut encoder.cea_708_counter,
                encoder.subs_delay,
                encoder.write_format,
                writer_ctx,
                encoder.no_font_color,
                &*encoder.transcript_settings,
                encoder.no_bom,
            );
            tv.writer_output(&mut writer).unwrap();
            tv.clear();
        }
    }

    /// Copy the contents of window to the TV screen
    pub fn copy_to_screen(&self, window: &dtvcc_window) {
        if self.is_window_overlapping(window) {
            debug!("dtvcc_window_copy_to_screen : window needs to be skipped");
            return;
        } else {
            debug!("dtvcc_window_copy_to_screen : no handling required");
        }
        debug!("dtvcc_window_copy_to_screen: W-{}", window.number);
        let anchor = match dtvcc_pen_anchor_point::new(window.anchor_point) {
            Ok(val) => val,
            Err(e) => {
                warn!("{}", e);
                return;
            }
        };
        let (mut top, mut left) = match anchor {
            dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_TOP_LEFT => {
                (window.anchor_vertical, window.anchor_horizontal)
            }
            dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_TOP_CENTER => (
                window.anchor_vertical,
                window.anchor_horizontal - window.col_count / 2,
            ),
            dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_TOP_RIGHT => (
                window.anchor_vertical,
                window.anchor_horizontal - window.col_count,
            ),
            dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_MIDDLE_LEFT => (
                window.anchor_vertical - window.row_count / 2,
                window.anchor_horizontal,
            ),
            dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_MIDDLE_CENTER => (
                window.anchor_vertical - window.row_count / 2,
                window.anchor_horizontal - window.col_count / 2,
            ),
            dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_MIDDLE_RIGHT => (
                window.anchor_vertical - window.row_count / 2,
                window.anchor_horizontal - window.col_count,
            ),
            dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_BOTTOM_LEFT => (
                window.anchor_vertical - window.row_count,
                window.anchor_horizontal,
            ),
            dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_BOTTOM_CENTER => (
                window.anchor_vertical - window.row_count,
                window.anchor_horizontal - window.col_count / 2,
            ),
            dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_BOTTOM_RIGHT => (
                window.anchor_vertical - window.row_count,
                window.anchor_horizontal - window.col_count,
            ),
        };
        debug!(
            "For window {}: Anchor point -> {}, size {}:{}, real position {}:{}",
            window.number, window.anchor_point, window.row_count, window.col_count, top, left
        );
        debug!("we have top [{}] and left [{}]", top, left);
        if top < 0 {
            top = 0
        }
        if left < 0 {
            left = 0
        }
        let copy_rows = if top + window.row_count >= CCX_DTVCC_SCREENGRID_ROWS as i32 {
            CCX_DTVCC_SCREENGRID_ROWS as i32 - top
        } else {
            window.row_count
        };
        let copy_cols = if left + window.col_count >= CCX_DTVCC_SCREENGRID_COLUMNS as i32 {
            CCX_DTVCC_SCREENGRID_COLUMNS as i32 - left
        } else {
            window.col_count
        };
        debug!("{}*{} will be copied to the TV.", copy_rows, copy_cols);

        unsafe {
            let tv = &mut *self.tv;
            for row in 0..copy_rows as usize {
                for col in 0..CCX_DTVCC_SCREENGRID_COLUMNS as usize {
                    if col < copy_cols as usize {
                        tv.chars[top as usize + row][col] = window.rows[row].add(col).read();
                    }
                    tv.pen_attribs[top as usize + row][col] = window.pen_attribs[row][col];
                    tv.pen_colors[top as usize + row][col] = window.pen_colors[row][col];
                }
            }

            tv.update_time_show(window.time_ms_show);
            tv.update_time_hide(window.time_ms_hide);
        }
    }

    /// Returns `true` if the given window is overlapping other windows
    pub fn is_window_overlapping(&self, window: &dtvcc_window) -> bool {
        let mut flag = 0;
        let (a_x1, a_x2, a_y1, a_y2) = match window.get_dimensions() {
            Ok(val) => val,
            Err(e) => {
                warn!("{}", e);
                return false;
            }
        };
        for id in 0..CCX_DTVCC_MAX_WINDOWS as usize {
            let win_compare = &self.windows[id];
            let (b_x1, b_x2, b_y1, b_y2) = match win_compare.get_dimensions() {
                Ok(val) => val,
                Err(e) => {
                    warn!("{}", e);
                    return false;
                }
            };
            if a_x1 == b_x1 && a_x2 == b_x2 && a_y1 == b_y1 && a_y2 == b_y2 {
                continue;
            } else if (a_x1 < b_x2)
                && (a_x2 > b_x1)
                && (a_y1 < b_y2)
                && (a_y2 > b_y1)
                && is_true(win_compare.visible)
            {
                if win_compare.priority < window.priority {
                    flag = 1
                } else {
                    flag = 0
                }
            }
        }
        flag == 1
    }

    /// Returns `true` if decoder has any visible window
    pub fn has_visible_windows(&self) -> bool {
        for id in 0..CCX_DTVCC_MAX_WINDOWS {
            if is_true(self.windows[id as usize].visible) {
                return true;
            }
        }
        false
    }

    // -------------------------- G0, G1 and extended Commands-------------------------

    /// G0 - Code Set - ASCII printable characters
    pub fn handle_G0(&mut self, block: &[c_uchar]) -> i32 {
        if self.current_window == -1 {
            warn!("dtvcc_handle_G0: Window has to be defined first");
            return block.len() as i32;
        }

        let character = block[0];
        debug!("G0: [{:2X}] ({})", character, character as char);
        let sym = if character == 0x7F {
            dtvcc_symbol::new(CCX_DTVCC_MUSICAL_NOTE_CHAR)
        } else {
            dtvcc_symbol::new(character as u16)
        };
        self.process_character(sym);
        1
    }

    /// G1 - Code Set - ISO 8859-1 LATIN-1 Character Set
    pub fn handle_G1(&mut self, block: &[c_uchar]) -> i32 {
        if self.current_window == -1 {
            warn!("dtvcc_handle_G1: Window has to be defined first");
            return block.len() as i32;
        }

        let character = block[0];
        debug!("G1: [{:2X}] ({})", character, character as char);
        let sym = dtvcc_symbol::new(character as u16);
        self.process_character(sym);
        1
    }

    /// Extended codes (EXT1 + code), from the extended sets
    ///
    /// G2 (20-7F) => Mostly unmapped, except for a few characters.
    ///
    /// G3 (A0-FF) => A0 is the CC symbol, everything else reserved for future expansion in EIA708
    ///
    /// C2 (00-1F) => Reserved for future extended misc. control and captions command codes
    ///
    /// C3 (80-9F) => Reserved for future extended misc. control and captions command codes
    ///
    /// WARN: This code is completely untested due to lack of samples. Just following specs!
    /// Returns number of used bytes, usually 1 (since EXT1 is not counted).
    pub fn handle_extended_char(&mut self, block: &[c_uchar]) -> u8 {
        let code = block[0];
        debug!(
            "dtvcc_handle_extended_char, first data code: [{}], length: [{}]",
            code as char,
            block.len()
        );

        match code {
            0..=0x1F => commands::handle_C2(code),
            0x20..=0x7F => {
                let val = unsafe { dtvcc_get_internal_from_G2(code) };
                let sym = dtvcc_symbol::new(val as u16);
                self.process_character(sym);
                1
            }
            0x80..=0x9F => commands::handle_C3(code, block[1]),
            _ => {
                let val = unsafe { dtvcc_get_internal_from_G3(code) };
                let sym = dtvcc_symbol::new(val as u16);
                self.process_character(sym);
                1
            }
        }
    }

    /// Process the character and add it to the current window
    pub fn process_character(&mut self, sym: dtvcc_symbol) {
        debug!("{}", self.current_window);
        let window = &mut self.windows[self.current_window as usize];
        let window_state = if is_true(window.is_defined) {
            "OK"
        } else {
            "undefined"
        };
        let (mut row, mut column) = (-1, -1);
        if self.current_window != -1 {
            row = window.pen_row;
            column = window.pen_column
        }
        debug!(
            "dtvcc_process_character: [{:04X}] - Window {} [{}], Pen: {}:{}",
            sym.sym, self.current_window, window_state, row, column
        );

        if self.current_window == -1 || is_false(window.is_defined) {
            return;
        }

        window.is_empty = 0;
        // Add symbol to window
        unsafe {
            window.rows[window.pen_row as usize]
                .add(window.pen_column as usize)
                .write(sym);
        }
        // "Painting" char by pen - attribs
        window.pen_attribs[window.pen_row as usize][window.pen_column as usize] =
            window.pen_attribs_pattern;
        // "Painting" char by pen - colors
        window.pen_colors[window.pen_row as usize][window.pen_column as usize] =
            window.pen_color_pattern;

        let pd = match dtvcc_window_pd::new(window.attribs.print_direction) {
            Ok(val) => val,
            Err(e) => {
                warn!("{}", e);
                return;
            }
        };
        match pd {
            dtvcc_window_pd::DTVCC_WINDOW_PD_LEFT_RIGHT => {
                if window.pen_column + 1 < window.col_count {
                    window.pen_column += 1;
                }
            }
            dtvcc_window_pd::DTVCC_WINDOW_PD_RIGHT_LEFT => {
                if window.pen_column > 0 {
                    window.pen_column -= 1;
                }
            }
            dtvcc_window_pd::DTVCC_WINDOW_PD_TOP_BOTTOM => {
                if window.pen_row + 1 < window.row_count {
                    window.pen_row += 1;
                }
            }
            dtvcc_window_pd::DTVCC_WINDOW_PD_BOTTOM_TOP => {
                if window.pen_row > 0 {
                    window.pen_row -= 1;
                }
            }
        };
    }
    /// Flush the decoder of any remaining subtitles
    pub fn flush(&self, encoder: &mut encoder_ctx) {
        unsafe {
            let tv = &mut (*self.tv);
            let sn = tv.service_number;
            let writer_ctx = &mut encoder.dtvcc_writers[(sn - 1) as usize];

            let mut writer = Writer::new(
                &mut encoder.cea_708_counter,
                encoder.subs_delay,
                encoder.write_format,
                writer_ctx,
                encoder.no_font_color,
                &*encoder.transcript_settings,
                encoder.no_bom,
            );
            writer.write_done();
        }
    }
}

/// Flush service decoder
#[no_mangle]
extern "C" fn ccxr_flush_decoder(dtvcc: *mut dtvcc_ctx, decoder: *mut dtvcc_service_decoder) {
    debug!("dtvcc_decoder_flush: Flushing decoder");
    let timing = unsafe { &mut *((*dtvcc).timing) };
    let encoder = unsafe { &mut *((*dtvcc).encoder as *mut encoder_ctx) };
    let decoder = unsafe { &mut *decoder };

    let mut screen_content_changed = false;
    for i in 0..CCX_DTVCC_MAX_WINDOWS {
        let window = &mut decoder.windows[i as usize];
        if is_true(window.visible) {
            screen_content_changed = true;
            window.update_time_hide(timing);
            decoder.copy_to_screen(&decoder.windows[i as usize]);
            decoder.windows[i as usize].visible = 0
        }
    }
    if screen_content_changed {
        decoder.screen_print(encoder, timing);
    }
    decoder.flush(encoder);
}
