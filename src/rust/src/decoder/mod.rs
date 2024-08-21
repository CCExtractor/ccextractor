//! CEA 708 decoder
//!
//! Provides a CEA 708 decoder as defined by ANSI/CTA-708-E R-2018

mod commands;
mod encoding;
mod output;
mod service_decoder;
mod timing;
mod tv_screen;
mod window;

use lib_ccxr::{
    debug, fatal,
    util::log::{DebugMessageFlag, ExitCause},
};

use crate::{bindings::*, utils::is_true};

const CCX_DTVCC_MAX_PACKET_LENGTH: u8 = 128;
const CCX_DTVCC_NO_LAST_SEQUENCE: i32 = -1;
const CCX_DTVCC_SCREENGRID_ROWS: u8 = 75;
const CCX_DTVCC_SCREENGRID_COLUMNS: u8 = 210;
const CCX_DTVCC_MAX_ROWS: u8 = 15;
const CCX_DTVCC_MAX_COLUMNS: u8 = 32 * 2;

/// Context required for processing 708 data
pub struct Dtvcc<'a> {
    pub is_active: bool,
    pub active_services_count: u8,
    pub services_active: Vec<i32>,
    pub report_enabled: bool,
    pub report: &'a mut ccx_decoder_dtvcc_report,
    pub decoders: Vec<&'a mut dtvcc_service_decoder>,
    pub packet: Vec<u8>,
    pub packet_length: u8,
    pub is_header_parsed: bool,
    pub last_sequence: i32,
    pub encoder: &'a mut encoder_ctx,
    pub no_rollup: bool,
    pub timing: &'a mut ccx_common_timing_ctx,
}

impl<'a> Dtvcc<'a> {
    /// Create a new dtvcc context
    pub fn new(ctx: &'a mut dtvcc_ctx) -> Self {
        let report = unsafe { &mut *ctx.report };
        let encoder = unsafe { &mut *(ctx.encoder as *mut encoder_ctx) };
        let timing = unsafe { &mut *ctx.timing };

        Self {
            is_active: is_true(ctx.is_active),
            active_services_count: ctx.active_services_count as u8,
            services_active: ctx.services_active.to_vec(),
            report_enabled: is_true(ctx.report_enabled),
            report,
            decoders: ctx.decoders.iter_mut().collect(),
            packet: ctx.current_packet.to_vec(),
            packet_length: ctx.current_packet_length as u8,
            is_header_parsed: is_true(ctx.is_current_packet_header_parsed),
            last_sequence: ctx.last_sequence,
            encoder,
            no_rollup: is_true(ctx.no_rollup),
            timing,
        }
    }
    /// Process cc data and add it to the dtvcc packet
    pub fn process_cc_data(&mut self, cc_valid: u8, cc_type: u8, data1: u8, data2: u8) {
        if !self.is_active && !self.report_enabled {
            return;
        }

        match cc_type {
            // type 0 and 1 are for CEA 608 data and are handled before calling this function
            // valid types for CEA 708 data are only 2 and 3
            2 => {
                debug!( msg_type = DebugMessageFlag::DECODER_708; "dtvcc_process_data: DTVCC Channel Packet Data");
                if cc_valid == 1 && self.is_header_parsed {
                    if self.packet_length > 253 {
                        debug!(msg_type = DebugMessageFlag::DECODER_708;"dtvcc_process_data: Warning: Legal packet size exceeded (1), data not added.");
                    } else {
                        self.add_data_to_packet(data1, data2);

                        let mut max_len = self.packet[0] & 0x3F;

                        if max_len == 0 {
                            // This is well defined in EIA-708; no magic.
                            max_len = 128;
                        } else {
                            max_len *= 2;
                        }

                        // If packet is complete then process the packet
                        if self.packet_length >= max_len {
                            self.process_current_packet(max_len);
                        }
                    }
                }
            }
            3 => {
                debug!(msg_type = DebugMessageFlag::DECODER_708;"dtvcc_process_data: DTVCC Channel Packet Start");
                if cc_valid == 1 {
                    if self.packet_length > (CCX_DTVCC_MAX_PACKET_LENGTH - 1) {
                        debug!(msg_type = DebugMessageFlag::DECODER_708;"dtvcc_process_data: Warning: Legal packet size exceeded (2), data not added.");
                    } else {
                        if self.is_header_parsed {
                            debug!(msg_type = DebugMessageFlag::DECODER_708;"dtvcc_process_data: Warning: Incorrect packet length specified. Packet will be skipped.");
                            self.clear_packet();
                        }
                        self.add_data_to_packet(data1, data2);
                        self.is_header_parsed = true;
                    }
                }
            }
            _ => fatal!(cause = ExitCause::Bug;
                "dtvcc_process_data: shouldn't be here - cc_type: {}",
                cc_type
            ),
        }
    }
    /// Add data to the packet
    pub fn add_data_to_packet(&mut self, data1: u8, data2: u8) {
        self.packet[self.packet_length as usize] = data1;
        self.packet_length += 1;
        self.packet[self.packet_length as usize] = data2;
        self.packet_length += 1;
    }
    /// Process current packet into service blocks
    pub fn process_current_packet(&mut self, len: u8) {
        let seq = (self.packet[0] & 0xC0) >> 6;
        debug!(
            msg_type = DebugMessageFlag::DECODER_708;
            "dtvcc_process_current_packet: Sequence: {}, packet length: {}",
            seq, len
        );
        if self.packet_length == 0 {
            return;
        }

        // Check if current sequence is correct
        // Sequence number is a 2 bit rolling sequence from (0-3)
        if self.last_sequence != CCX_DTVCC_NO_LAST_SEQUENCE
            && (self.last_sequence + 1) % 4 != seq as i32
        {
            debug!(msg_type = DebugMessageFlag::DECODER_708;"dtvcc_process_current_packet: Unexpected sequence number, it is {} but should be {}", seq, (self.last_sequence +1) % 4);
        }
        self.last_sequence = seq as i32;

        let mut pos: u8 = 1;
        while pos < len {
            let mut service_number = (self.packet[pos as usize] & 0xE0) >> 5; // 3 more significant bits
            let block_length = self.packet[pos as usize] & 0x1F; // 5 less significant bits
            debug!(msg_type = DebugMessageFlag::DECODER_708;"dtvcc_process_current_packet: Standard header Service number: {}, Block length: {}", service_number, block_length);

            if service_number == 7 {
                // There is an extended header
                // CEA-708-E 6.2.2 Extended Service Block Header
                pos += 1;
                service_number = self.packet[pos as usize] & 0x3F; // 6 more significant bits
                if service_number > 7 {
                    debug!(msg_type = DebugMessageFlag::DECODER_708;"dtvcc_process_current_packet: Illegal service number in extended header: {}", service_number);
                }
            }

            pos += 1;

            if service_number == 0 && block_length != 0 {
                // Illegal, but specs say what to do...
                pos = len; // Move to end
                break;
            }

            if block_length != 0 {
                self.report.services[service_number as usize] = 1;
            }

            if service_number > 0 && is_true(self.services_active[(service_number - 1) as usize]) {
                let decoder = &mut self.decoders[(service_number - 1) as usize];
                decoder.process_service_block(
                    &self.packet[pos as usize..(pos + block_length) as usize],
                    self.encoder,
                    self.timing,
                    self.no_rollup,
                );
            }

            pos += block_length // Skip data
        }

        self.clear_packet();

        if len < 128 && self.packet[pos as usize] != 0 {
            // Null header is mandatory if there is room
            debug!(msg_type = DebugMessageFlag::DECODER_708;"dtvcc_process_current_packet: Warning: Null header expected but not found.");
        }
    }
    /// Clear current packet
    pub fn clear_packet(&mut self) {
        self.packet_length = 0;
        self.is_header_parsed = false;
        self.packet.iter_mut().for_each(|x| *x = 0);
    }
}

/// A single character symbol
///
/// sym stores the symbol
/// init is used to know if the symbol is initialized
impl dtvcc_symbol {
    /// Create a new symbol
    pub fn new(sym: u16) -> Self {
        Self { init: 1, sym }
    }
    /// Create a new 16 bit symbol
    pub fn new_16(data1: u8, data2: u8) -> Self {
        let sym = (data1 as u16) << 8 | data2 as u16;
        Self { init: 1, sym }
    }
    /// Check if symbol is initialized
    pub fn is_set(&self) -> bool {
        is_true(self.init)
    }
}

impl Default for dtvcc_symbol {
    /// Create a blank uninitialized symbol
    fn default() -> Self {
        Self { sym: 0, init: 0 }
    }
}

impl PartialEq for dtvcc_symbol {
    fn eq(&self, other: &Self) -> bool {
        self.sym == other.sym && self.init == other.init
    }
}

#[cfg(test)]
mod test {
    use lib_ccxr::util::log::{set_logger, CCExtractorLogger, DebugMessageMask, OutputTarget};

    use crate::utils::get_zero_allocated_obj;

    use super::*;

    #[test]
    fn test_process_cc_data() {
        set_logger(CCExtractorLogger::new(
            OutputTarget::Stdout,
            DebugMessageMask::new(DebugMessageFlag::VERBOSE, DebugMessageFlag::VERBOSE),
            false,
        ))
        .ok();
        let mut dtvcc_ctx = get_zero_allocated_obj::<dtvcc_ctx>();
        let mut decoder = Dtvcc::new(&mut dtvcc_ctx);

        // Case 1:  cc_type = 2
        let mut dtvcc_report = ccx_decoder_dtvcc_report::default();
        decoder.report = &mut dtvcc_report;
        decoder.is_header_parsed = true;
        decoder.is_active = true;
        decoder.report_enabled = true;
        decoder.packet = vec![0xC2, 0x23, 0x45, 0x67, 0x00, 0x00];
        decoder.packet_length = 4;

        decoder.process_cc_data(1, 2, 0x01, 0x02);

        assert_eq!(decoder.report.services[1], 1);
        assert!(decoder.packet.iter().all(|&ele| ele == 0));
        assert_eq!(decoder.packet_length, 0);

        // Case 2:  cc_type = 3 with `is_header_parsed = true`
        decoder.is_header_parsed = true;
        decoder.packet = vec![0xC2, 0x23, 0x45, 0x67, 0x00, 0x00];
        decoder.packet_length = 4;

        decoder.process_cc_data(1, 3, 0x01, 0x02);

        assert_eq!(decoder.packet[0], 0x01);
        assert_eq!(decoder.packet[1], 0x02);
        assert_eq!(decoder.packet_length, 2);

        // Case 3:  cc_type = 3 with `is_header_parsed = false`
        decoder.is_header_parsed = false;
        decoder.packet = vec![0xC2, 0x23, 0x45, 0x67, 0x00, 0x00];
        decoder.packet_length = 4;

        decoder.process_cc_data(1, 3, 0x01, 0x02);

        assert_eq!(decoder.packet, vec![0xC2, 0x23, 0x45, 0x67, 0x01, 0x02]);
        assert_eq!(decoder.packet_length, 6);
        assert_eq!(decoder.is_header_parsed, true);
    }

    #[test]
    fn test_process_current_packet() {
        set_logger(CCExtractorLogger::new(
            OutputTarget::Stdout,
            DebugMessageMask::new(DebugMessageFlag::VERBOSE, DebugMessageFlag::VERBOSE),
            false,
        ))
        .ok();
        let mut dtvcc_ctx = get_zero_allocated_obj::<dtvcc_ctx>();
        let mut decoder = Dtvcc::new(&mut dtvcc_ctx);

        // Case 1: Without providing last_sequence
        let mut dtvcc_report = ccx_decoder_dtvcc_report::default();
        decoder.report = &mut dtvcc_report;
        decoder.packet = vec![0xC2, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF];
        decoder.packet_length = 8;
        decoder.process_current_packet(4);

        assert_eq!(decoder.report.services[1], 1);
        assert_eq!(decoder.packet_length, 0); // due to `clear_packet()` fn call

        // Case 2: With providing last_sequence
        let mut dtvcc_report = ccx_decoder_dtvcc_report::default();
        decoder.report = &mut dtvcc_report;
        decoder.packet = vec![0xC7, 0xC2, 0x12, 0x67, 0x29, 0xAB, 0xCD, 0xEF];
        decoder.packet_length = 8;
        decoder.last_sequence = 6;
        decoder.process_current_packet(4);

        assert_eq!(decoder.report.services[6], 1);
        assert_eq!(decoder.packet_length, 0); // due to `clear_packet()` fn call

        // Test case 3: Packet with extended header and multiple service blocks
        let mut dtvcc_report = ccx_decoder_dtvcc_report::default();
        decoder.report = &mut dtvcc_report;
        decoder.packet = vec![
            0xC0, 0xE7, 0x08, 0x02, 0x01, 0x02, 0x07, 0x03, 0x03, 0x04, 0x05,
        ];
        decoder.packet_length = 11;
        decoder.last_sequence = 6;
        decoder.process_current_packet(6);

        assert_eq!(decoder.report.services[8], 1);
        assert_eq!(decoder.packet_length, 0); // due to `clear_packet()` fn call
    }
}
