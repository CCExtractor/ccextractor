//! CEA 708 decoder
//!
//! This module provides a CEA 708 decoder as defined by ANSI/CTA-708-E R-2018

use crate::bindings::*;

const CCX_DTVCC_MAX_PACKET_LENGTH: u8 = 128;

/// Process cc data to generate dtvcc packet
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
    let dtvcc = Dtvcc::new(dtvcc);
    let mut ctx = &mut *dtvcc.ctx;
    let cc_valid = *data;
    let cc_type = *data.add(1);

    if cc_type == 2 {
        println!("[CEA-708] dtvcc_process_data: DTVCC Channel Packet Data");
        if cc_valid == 1 && ctx.is_current_packet_header_parsed == 1 {
            if ctx.current_packet_length > 253 {
                println!("[CEA-708] dtvcc_process_data: Warning: Legal packet size exceeded (1), data not added.");
            } else {
                ctx.current_packet[ctx.current_packet_length as usize] = *data.add(2);
                ctx.current_packet_length += 1;
                ctx.current_packet[ctx.current_packet_length as usize] = *data.add(3);
                ctx.current_packet_length += 1;

                let mut max_len = ctx.current_packet[0] & 0x3F;

                if max_len == 0 {
                    // This is well defined in EIA-708; no magic.
                    max_len = 128;
                } else {
                    max_len *= 2;
                }

                if ctx.current_packet_length >= max_len as i32 {
                    dtvcc.process_current_packet(max_len as i32);
                }
            }
        }
    } else if cc_type == 3 {
        println!("[CEA-708] dtvcc_process_data: DTVCC Channel Packet Start");
        if cc_valid == 1 {
            if ctx.current_packet_length > (CCX_DTVCC_MAX_PACKET_LENGTH - 1) as i32 {
                println!("[CEA-708] dtvcc_process_data: Warning: Legal packet size exceeded (2), data not added.");
            } else {
                ctx.current_packet[ctx.current_packet_length as usize] = *data.add(2);
                ctx.current_packet_length += 1;
                ctx.current_packet[ctx.current_packet_length as usize] = *data.add(3);
                ctx.current_packet_length += 1;
                ctx.is_current_packet_header_parsed = 1;
            }
        }
    } else {
        println!(
            "[CEA-708] dtvcc_process_data: shouldn't be here - cc_type: {}",
            cc_type
        )
    }
}

/// Wrapper for mutable pointer to dtvcc_ctx
pub struct Dtvcc {
    pub ctx: *mut dtvcc_ctx,
}

impl Dtvcc {
    /// Creates a new Dtvcc wrapper and stores the pointer
    pub fn new(ctx: *mut dtvcc_ctx) -> Self {
        Self { ctx }
    }
    pub fn process_current_packet(&self, len: i32) {
        unsafe {
            dtvcc_process_current_packet(self.ctx, len);
        }
    }
}
