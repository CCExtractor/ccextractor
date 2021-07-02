//! CEA 708 decoder
//!
//! This module provides a CEA 708 decoder as defined by ANSI/CTA-708-E R-2018

mod commands;

use log::{debug, warn};

use crate::bindings::*;
use commands::{C0CodeSet, C0Command, C1CodeSet, C1Command};

const CCX_DTVCC_MAX_PACKET_LENGTH: u8 = 128;
const CCX_DTVCC_NO_LAST_SEQUENCE: i32 = -1;
const DTVCC_COMMANDS_C0_CODES_DTVCC_C0_EXT1: u8 = 16;
const CCX_DTVCC_MUSICAL_NOTE_CHAR: u16 = 9836;

/// Process data passed from C
///
/// data has following format:
///
/// | Name | Size |
/// | --- | --- |
/// | cc_valid | 1 byte |
/// | cc_type | 1 byte |
/// | data | 2 bytes |
///
/// # Safety
///
/// dtvcc and data must not be null pointers
#[no_mangle]
pub unsafe extern "C" fn dtvcc_process_cc_data(
    dtvcc: *mut dtvcc_ctx,
    data: *const ::std::os::raw::c_uchar,
) {
    let ctx = &mut *dtvcc;
    let cc_valid = *data;
    let cc_type = *data.add(1);
    let data1 = *data.add(2);
    let data2 = *data.add(3);
    ctx.process_cc_data(cc_valid, cc_type, data1, data2);
}

impl dtvcc_ctx {
    /// Process cc data to generate dtvcc packet
    pub fn process_cc_data(&mut self, cc_valid: u8, cc_type: u8, data1: u8, data2: u8) {
        match cc_type {
            // type 0 and 1 are for CEA 608 data and are handled before calling this function
            // valid types for CEA 708 data are only 2 and 3
            2 => {
                debug!("dtvcc_process_data: DTVCC Channel Packet Data");
                if cc_valid == 1 && self.is_current_packet_header_parsed == 1 {
                    if self.current_packet_length > 253 {
                        warn!("dtvcc_process_data: Warning: Legal packet size exceeded (1), data not added.");
                    } else {
                        self.current_packet[self.current_packet_length as usize] = data1;
                        self.current_packet_length += 1;
                        self.current_packet[self.current_packet_length as usize] = data2;
                        self.current_packet_length += 1;

                        let mut max_len = self.current_packet[0] & 0x3F;

                        if max_len == 0 {
                            // This is well defined in EIA-708; no magic.
                            max_len = 128;
                        } else {
                            max_len *= 2;
                        }

                        if self.current_packet_length >= max_len as i32 {
                            self.process_current_packet(max_len);
                        }
                    }
                }
            }
            3 => {
                debug!("dtvcc_process_data: DTVCC Channel Packet Start");
                if cc_valid == 1 {
                    if self.current_packet_length > (CCX_DTVCC_MAX_PACKET_LENGTH - 1) as i32 {
                        warn!("dtvcc_process_data: Warning: Legal packet size exceeded (2), data not added.");
                    } else {
                        self.current_packet[self.current_packet_length as usize] = data1;
                        self.current_packet_length += 1;
                        self.current_packet[self.current_packet_length as usize] = data2;
                        self.current_packet_length += 1;
                        self.is_current_packet_header_parsed = 1;
                    }
                }
            }
            _ => warn!(
                "dtvcc_process_data: shouldn't be here - cc_type: {}",
                cc_type
            ),
        }
    }
    /// Process current packet into service blocks
    pub fn process_current_packet(&mut self, len: u8) {
        let seq = (self.current_packet[0] & 0xC0) >> 6;
        debug!(
            "dtvcc_process_current_packet: Sequence: {}, packet length: {}",
            seq, len
        );
        if self.current_packet_length == 0 {
            return;
        }

        if self.last_sequence != CCX_DTVCC_NO_LAST_SEQUENCE
            && (self.last_sequence + 1) % 4 != seq as i32
        {
            warn!("dtvcc_process_current_packet: Unexpected sequence number, it is {} but should be {}", seq, (self.last_sequence +1) % 4);
        }
        self.last_sequence = seq as i32;

        let mut pos: u8 = 1;
        while pos < len {
            let mut service_number = (self.current_packet[pos as usize] & 0xE0) >> 5; // 3 more significant bits
            let block_length = self.current_packet[pos as usize] & 0x1F; // 5 less significant bits
            debug!("dtvcc_process_current_packet: Standard header Service number: {}, Block length: {}", service_number, block_length);

            if service_number == 7 {
                // There is an extended header
                // CEA-708-E 6.2.2 Extended Service Block Header
                pos += 1;
                service_number = self.current_packet[pos as usize] & 0x3F; // 6 more significant bits
                if service_number > 7 {
                    warn!("dtvcc_process_current_packet: Illegal service number in extended header: {}", service_number);
                }
            }

            pos += 1;

            if service_number == 0 && block_length != 0 {
                // Illegal, but specs say what to do...
                pos = len; // Move to end
                break;
            }

            if block_length != 0 {
                unsafe {
                    let mut report = *self.report;
                    report.services[service_number as usize] = 1;
                }
            }

            if service_number > 0 && self.services_active[(service_number - 1) as usize] == 1 {
                self.process_service_block(service_number - 1, pos, block_length);
            }

            pos += block_length // Skip data
        }

        self.clear_packet();

        if len < 128 && self.current_packet[pos as usize] != 0 {
            // Null header is mandatory if there is room
            warn!("dtvcc_process_current_packet: Warning: Null header expected but not found.");
        }
    }
    /// Process service block and call handlers for the respective codesets
    pub fn process_service_block(&mut self, decoder: u8, pos: u8, block_length: u8) {
        let mut i = 0;
        while i < block_length {
            let curr: usize = (pos + i) as usize;

            let consumed = if self.current_packet[curr] != DTVCC_COMMANDS_C0_CODES_DTVCC_C0_EXT1 {
                let used = match self.current_packet[curr] {
                    0..=0x1F => self.handle_C0(decoder, pos + i, block_length - i),
                    0x20..=0x7F => self.handle_G0(decoder, pos + i, block_length - i),
                    0x80..=0x9F => self.handle_C1(decoder, pos + i, block_length - i),
                    _ => self.handle_G1(decoder, pos + i, block_length - i),
                };
                if used == -1 {
                    warn!("dtvcc_process_service_block: There was a problem handling the data.");
                    return;
                }
                used
            } else {
                let mut used = self.handle_extended_char(decoder, pos + i, block_length - i);
                used += 1; // Since we had CCX_DTVCC_C0_EXT1
                used
            };
            i += consumed as u8;
        }
    }
    /// Handle C0 - Code Set - Miscellaneous Control Codes
    pub fn handle_C0(&mut self, decoder: u8, pos: u8, block_length: u8) -> i32 {
        let ctx = self as *mut dtvcc_ctx;
        let service_decoder = (&mut self.decoders[decoder as usize]) as *mut dtvcc_service_decoder;
        let data = (&mut self.current_packet[pos as usize]) as *mut std::os::raw::c_uchar;

        let code = self.current_packet[pos as usize];
        let C0Command { command, length } = C0Command::new(code);
        debug!("C0: [{:?}] ({})", command, block_length);
        unsafe {
            match command {
                // NUL command does nothing
                C0CodeSet::NUL => {}
                C0CodeSet::ETX => dtvcc_process_etx(service_decoder),
                C0CodeSet::BS => dtvcc_process_bs(service_decoder),
                C0CodeSet::FF => dtvcc_process_ff(service_decoder),
                C0CodeSet::CR => dtvcc_process_cr(ctx, service_decoder),
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
    pub fn handle_C1(&mut self, decoder: u8, pos: u8, block_length: u8) -> i32 {
        let ctx = self as *mut dtvcc_ctx;
        let service_decoder = (&mut self.decoders[decoder as usize]) as *mut dtvcc_service_decoder;
        let data = (&mut self.current_packet[pos as usize]) as *mut std::os::raw::c_uchar;

        let code = self.current_packet[pos as usize];
        let next_code = self.current_packet[(pos + 1) as usize] as i32;
        let C1Command {
            command,
            length,
            name,
        } = C1Command::new(code);

        if length > block_length {
            warn!("Warning: Not enough bytes for command.");
            return -1;
        }

        debug!("C0: [{:?}] [{}] ({})", command, name, length);
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
                    service_decoder,
                    (code - DTVCC_COMMANDS_C1_CODES_DTVCC_C1_CW0 as u8) as i32,
                ),
                C1CodeSet::CLW => dtvcc_handle_CLW_ClearWindows(ctx, service_decoder, next_code),
                C1CodeSet::DSW => {
                    dtvcc_handle_DSW_DisplayWindows(service_decoder, next_code, self.timing)
                }
                C1CodeSet::HDW => dtvcc_handle_HDW_HideWindows(ctx, service_decoder, next_code),
                C1CodeSet::TGW => dtvcc_handle_TGW_ToggleWindows(ctx, service_decoder, next_code),
                C1CodeSet::DLW => dtvcc_handle_DLW_DeleteWindows(ctx, service_decoder, next_code),
                C1CodeSet::DLY => dtvcc_handle_DLY_Delay(service_decoder, next_code),
                C1CodeSet::DLC => dtvcc_handle_DLC_DelayCancel(service_decoder),
                C1CodeSet::RST => dtvcc_handle_RST_Reset(service_decoder),
                C1CodeSet::SPA => dtvcc_handle_SPA_SetPenAttributes(service_decoder, data),
                C1CodeSet::SPC => dtvcc_handle_SPC_SetPenColor(service_decoder, data),
                C1CodeSet::SPL => dtvcc_handle_SPL_SetPenLocation(service_decoder, data),
                C1CodeSet::SWA => dtvcc_handle_SWA_SetWindowAttributes(service_decoder, data),
                C1CodeSet::DF0
                | C1CodeSet::DF1
                | C1CodeSet::DF2
                | C1CodeSet::DF3
                | C1CodeSet::DF4
                | C1CodeSet::DF5
                | C1CodeSet::DF6
                | C1CodeSet::DF7 => dtvcc_handle_DFx_DefineWindow(
                    service_decoder,
                    (code - DTVCC_COMMANDS_C1_CODES_DTVCC_C1_DF0 as u8) as i32,
                    data,
                    self.timing,
                ),
                C1CodeSet::RESERVED => {
                    warn!("Warning, Found Reserved codes, ignored");
                }
            };
        }
        length as i32
    }
    /// Handle G0 - Code Set - ASCII printable characters
    pub fn handle_G0(&mut self, decoder: u8, pos: u8, block_length: u8) -> i32 {
        let service_decoder = &self.decoders[decoder as usize];
        if service_decoder.current_window == -1 {
            warn!("dtvcc_handle_G0: Window has to be defined first");
            return block_length as i32;
        }

        let character = self.current_packet[pos as usize];
        debug!("G0: [{:2X}] ({})", character, character as char);
        let sym = if character == 0x7F {
            dtvcc_symbol::new(CCX_DTVCC_MUSICAL_NOTE_CHAR)
        } else {
            dtvcc_symbol::new(character as u16)
        };
        self.process_character(decoder, sym);
        1
    }
    /// Handle G1 - Code Set - ISO 8859-1 LATIN-1 Character Set
    pub fn handle_G1(&mut self, decoder: u8, pos: u8, block_length: u8) -> i32 {
        let service_decoder = &self.decoders[decoder as usize];
        if service_decoder.current_window == -1 {
            warn!("dtvcc_handle_G1: Window has to be defined first");
            return block_length as i32;
        }

        let character = self.current_packet[pos as usize];
        debug!("G1: [{:2X}] ({})", character, character as char);
        let sym = dtvcc_symbol::new(character as u16);
        self.process_character(decoder, sym);
        1
    }
    /// Handle extended codes (EXT1 + code), from the extended sets
    pub fn handle_extended_char(&mut self, decoder: u8, pos: u8, block_length: u8) -> i32 {
        unsafe {
            let service_decoder =
                (&mut self.decoders[decoder as usize]) as *mut dtvcc_service_decoder;
            let data = (&mut self.current_packet[pos as usize]) as *mut std::os::raw::c_uchar;
            dtvcc_handle_extended_char(service_decoder, data, block_length as i32)
        }
    }
    /// Process the character and add it to the current window
    pub fn process_character(&mut self, decoder: u8, sym: dtvcc_symbol) {
        unsafe {
            let service_decoder =
                (&mut self.decoders[decoder as usize]) as *mut dtvcc_service_decoder;
            dtvcc_process_character(service_decoder, sym);
        }
    }
    /// Reset all decoders to their initial states
    pub fn reset_decoders(&mut self) {
        unsafe {
            let ctx = self as *mut dtvcc_ctx;
            dtvcc_decoders_reset(ctx);
        }
    }
    /// Clear current packet
    pub fn clear_packet(&mut self) {
        self.current_packet_length = 0;
        self.is_current_packet_header_parsed = 0;
        self.current_packet.iter_mut().for_each(|x| *x = 0);
    }
}

impl dtvcc_symbol {
    /// Create a new symbol
    pub fn new(sym: u16) -> dtvcc_symbol {
        dtvcc_symbol { init: 1, sym }
    }
}
