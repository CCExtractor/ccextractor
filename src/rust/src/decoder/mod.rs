//! CEA 708 decoder
//!
//! This module provides a CEA 708 decoder as defined by ANSI/CTA-708-E R-2018

mod commands;
mod service_decoder;
mod window;

use log::{debug, warn};

use crate::{bindings::*, utils::is_true};

const CCX_DTVCC_MAX_PACKET_LENGTH: u8 = 128;
const CCX_DTVCC_NO_LAST_SEQUENCE: i32 = -1;

/// Stores the context required for processing 708 data
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
            services_active: ctx.services_active.iter().copied().collect(),
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
    /// Process cc data to generate dtvcc packet
    pub fn process_cc_data(&mut self, cc_valid: u8, cc_type: u8, data1: u8, data2: u8) {
        if !self.is_active && !self.report_enabled {
            return;
        }

        match cc_type {
            // type 0 and 1 are for CEA 608 data and are handled before calling this function
            // valid types for CEA 708 data are only 2 and 3
            2 => {
                debug!("dtvcc_process_data: DTVCC Channel Packet Data");
                if cc_valid == 1 && self.is_header_parsed {
                    if self.packet_length > 253 {
                        warn!("dtvcc_process_data: Warning: Legal packet size exceeded (1), data not added.");
                    } else {
                        self.add_data_to_packet(data1, data2);

                        let mut max_len = self.packet[0] & 0x3F;

                        if max_len == 0 {
                            // This is well defined in EIA-708; no magic.
                            max_len = 128;
                        } else {
                            max_len *= 2;
                        }

                        if self.packet_length >= max_len {
                            self.process_current_packet(max_len);
                        }
                    }
                }
            }
            3 => {
                debug!("dtvcc_process_data: DTVCC Channel Packet Start");
                if cc_valid == 1 {
                    if self.packet_length > (CCX_DTVCC_MAX_PACKET_LENGTH - 1) {
                        warn!("dtvcc_process_data: Warning: Legal packet size exceeded (2), data not added.");
                    } else {
                        self.add_data_to_packet(data1, data2);
                        self.is_header_parsed = true;
                    }
                }
            }
            _ => warn!(
                "dtvcc_process_data: shouldn't be here - cc_type: {}",
                cc_type
            ),
        }
    }
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
            "dtvcc_process_current_packet: Sequence: {}, packet length: {}",
            seq, len
        );
        if self.packet_length == 0 {
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
            let mut service_number = (self.packet[pos as usize] & 0xE0) >> 5; // 3 more significant bits
            let block_length = self.packet[pos as usize] & 0x1F; // 5 less significant bits
            debug!("dtvcc_process_current_packet: Standard header Service number: {}, Block length: {}", service_number, block_length);

            if service_number == 7 {
                // There is an extended header
                // CEA-708-E 6.2.2 Extended Service Block Header
                pos += 1;
                service_number = self.packet[pos as usize] & 0x3F; // 6 more significant bits
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
                self.report.services[service_number as usize] = 1;
            }

            if service_number > 0 && is_true(self.services_active[(service_number - 1) as usize]) {
                let decoder = &mut self.decoders[(service_number - 1) as usize];
                decoder.process_service_block(
                    &mut self.packet,
                    pos,
                    block_length,
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
            warn!("dtvcc_process_current_packet: Warning: Null header expected but not found.");
        }
    }
    /// Clear current packet
    pub fn clear_packet(&mut self) {
        self.packet_length = 0;
        self.is_header_parsed = false;
        self.packet.iter_mut().for_each(|x| *x = 0);
    }
}

impl dtvcc_symbol {
    /// Create a new symbol
    pub fn new(sym: u16) -> dtvcc_symbol {
        dtvcc_symbol { init: 1, sym }
    }
    /// Create a new 16 bit symbol
    pub fn new_16(data1: u8, data2: u8) -> dtvcc_symbol {
        let sym = (data1 as u16) << 8 | data2 as u16;
        dtvcc_symbol { init: 1, sym }
    }
}
