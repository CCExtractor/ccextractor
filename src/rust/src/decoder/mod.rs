//! CEA 708 decoder
//!
//! This module provides a CEA 708 decoder as defined by ANSI/CTA-708-E R-2018

use log::{debug, warn};

use crate::bindings::*;

const CCX_DTVCC_MAX_PACKET_LENGTH: u8 = 128;
const CCX_DTVCC_NO_LAST_SEQUENCE: i32 = -1;

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

        if self.current_packet_length != len as i32 {
            // Is this possible?
            self.reset_decoders();
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
        if pos != len {
            // For some reason we didn't parse the whole packet
            warn!("dtvcc_process_current_packet:  There was a problem with this packet, reseting");
            self.reset_decoders();
        }

        if len < 128 && self.current_packet[pos as usize] != 0 {
            // Null header is mandatory if there is room
            warn!("dtvcc_process_current_packet: Warning: Null header expected but not found.");
        }
    }
    pub fn process_service_block(&mut self, decoder: u8, pos: u8, block_length: u8) {
        unsafe {
            let ctx = self as *mut dtvcc_ctx;
            let service_decoder =
                (&mut self.decoders[decoder as usize]) as *mut dtvcc_service_decoder;
            let data = (&mut self.current_packet[pos as usize]) as *mut std::os::raw::c_uchar;
            dtvcc_process_service_block(ctx, service_decoder, data, block_length as i32);
        }
    }
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
