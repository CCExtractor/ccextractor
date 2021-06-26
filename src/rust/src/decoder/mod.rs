//! CEA 708 decoder
//!
//! This module provides a CEA 708 decoder as defined by ANSI/CTA-708-E R-2018

use log::{debug, warn};

use crate::bindings::*;

const CCX_DTVCC_MAX_PACKET_LENGTH: u8 = 128;

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
                            self.process_current_packet(max_len as i32);
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
    pub fn process_current_packet(&mut self, len: i32) {
        unsafe {
            let ctx = self as *mut dtvcc_ctx;
            dtvcc_process_current_packet(ctx, len);
        }
    }
}
