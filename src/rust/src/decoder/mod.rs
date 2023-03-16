//! CEA 708 decoder
//!
//! Provides a CEA 708 decoder as defined by ANSI/CTA-708-E R-2018

mod commands;
mod output;
mod service_decoder;
mod timing;
mod tv_screen;
mod window;

use crate::{bindings::*, utils::is_true};

use log::{debug, warn};

const CCX_DTVCC_MAX_PACKET_LENGTH: u8 = 128;
const CCX_DTVCC_NO_LAST_SEQUENCE: i32 = -1;
const CCX_DTVCC_SCREENGRID_ROWS: u8 = 75;
const CCX_DTVCC_SCREENGRID_COLUMNS: u8 = 210;
const CCX_DTVCC_MAX_ROWS: u8 = 15;
const CCX_DTVCC_MAX_COLUMNS: u8 = 32 * 2;
const CCX_DTVCC_MAX_SERVICES: usize = 63;

/// Context required for processing 708 data
pub struct Dtvcc<'a> {
    pub is_active: bool,
    pub active_services_count: u8,
    pub services_active: [i32; CCX_DTVCC_MAX_SERVICES],
    pub report_enabled: bool,
    pub report: &'a mut ccx_decoder_dtvcc_report,
    pub decoders: [Option<&'a mut dtvcc_service_decoder>; CCX_DTVCC_MAX_SERVICES],
    pub packet: [u8; CCX_DTVCC_MAX_SERVICES],
    pub packet_length: u8,
    pub is_header_parsed: bool,
    pub last_sequence: i32,
    pub encoder: Option<&'a mut encoder_ctx>,
    pub no_rollup: bool,
    pub timing: &'a mut ccx_common_timing_ctx,
}

impl<'a> Dtvcc<'a> {
    /// Create a new dtvcc context
    pub fn new(opts: &'a mut ccx_decoder_dtvcc_settings) -> Self {
        let report = unsafe { &mut *opts.report };
        report.reset_count = 0;

        const INIT: Option<&mut dtvcc_service_decoder> = None;
        let decoders = [INIT; CCX_DTVCC_MAX_SERVICES];
        decoders.iter_mut().enumerate().for_each(|(i, val)| {
            if !is_true(opts.services_enabled[i]) {
                return;
            }

            let decoder = Box::leak(Box::new(dtvcc_service_decoder {
                tv: Box::leak(Box::new(dtvcc_tv_screen {
                    cc_count: 0,
                    service_number: i as i32 + 1,
                    ..dtvcc_tv_screen::default()
                })),
                ..dtvcc_service_decoder::default()
            }));

            decoder.windows.iter_mut().for_each(|w| {
                w.memory_reserved = 0;
            });

            unsafe { dtvcc_windows_reset(decoder) };

            val = &mut Some(decoder);
        });

        for i in 0..CCX_DTVCC_MAX_SERVICES {
            if !is_true(opts.services_enabled[i]) {
                continue;
            }
        }

        // dtvcc_clear_packet sets current_packet_length, is_current_packet_header_parsed, current_packet to 0

        Self {
            report,
            decoders,
            is_active: false,
            no_rollup: is_true(opts.no_rollup),
            active_services_count: opts.active_services_count as u8,
            services_active: opts.services_enabled.clone(),
            packet_length: 0,
            is_header_parsed: false,
            packet: [0; CCX_DTVCC_MAX_SERVICES],
            last_sequence: CCX_DTVCC_NO_LAST_SEQUENCE,
            report_enabled: is_true(opts.print_file_reports),
            timing: unsafe { &mut *opts.timing },
            encoder: None,
        }

        // notes: import CCX_DTVCC_MAX_SERVICES, initialize the decoders
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

                        // If packet is complete then process the packet
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
                        if self.is_header_parsed {
                            warn!("dtvcc_process_data: Warning: Incorrect packet length specified. Packet will be skipped.");
                            self.clear_packet();
                        }
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
