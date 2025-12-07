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

use log::debug as log_debug;

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

/// Maximum number of CEA-708 services
pub const CCX_DTVCC_MAX_SERVICES: usize = 63;

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
        let mut encoder = Box::into_raw(Box::new(encoder_ctx::default()));
        if !ctx.encoder.is_null() {
            encoder = unsafe { &mut *(ctx.encoder as *mut encoder_ctx) };
        }
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
            encoder: unsafe { &mut *encoder },
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

// =============================================================================
// DtvccRust: Persistent CEA-708 decoder context for Rust-owned state
// =============================================================================
//
// This struct is designed to be created once and persist throughout the program's
// lifetime, solving the issue where state was being reset on each call.
// See: https://github.com/CCExtractor/ccextractor/issues/1499

/// Persistent CEA-708 decoder context that owns its data.
///
/// Unlike `Dtvcc` which borrows from C structures, `DtvccRust` owns all its
/// decoder state and is designed to persist across multiple processing calls.
/// This is created once via `ccxr_dtvcc_init()` and freed via `ccxr_dtvcc_free()`.
pub struct DtvccRust {
    pub is_active: bool,
    pub active_services_count: u8,
    pub services_active: Vec<i32>,
    pub report_enabled: bool,
    pub report: *mut ccx_decoder_dtvcc_report,
    pub decoders: [Option<Box<dtvcc_service_decoder>>; CCX_DTVCC_MAX_SERVICES],
    pub packet: Vec<u8>,
    pub packet_length: u8,
    pub is_header_parsed: bool,
    pub last_sequence: i32,
    pub encoder: *mut encoder_ctx,
    pub no_rollup: bool,
    pub timing: *mut ccx_common_timing_ctx,
}

impl DtvccRust {
    /// Create a new persistent dtvcc context from settings.
    ///
    /// This closely follows `dtvcc_init` at `src/lib_ccx/ccx_dtvcc.c:82`
    ///
    /// # Safety
    /// The following pointers in `opts` must not be null:
    /// - `opts.report`
    /// - `opts.timing`
    pub fn new(opts: &ccx_decoder_dtvcc_settings) -> Self {
        let is_active = is_true(opts.enabled);
        let active_services_count = opts.active_services_count as u8;
        let services_active = opts.services_enabled.to_vec();
        let report_enabled = is_true(opts.print_file_reports);

        // Reset the report counter
        if !opts.report.is_null() {
            unsafe {
                (*opts.report).reset_count = 0;
            }
        }

        // Initialize packet state (equivalent to dtvcc_clear_packet)
        let packet_length = 0;
        let is_header_parsed = false;
        let packet = vec![0u8; CCX_DTVCC_MAX_PACKET_LENGTH as usize];

        let last_sequence = CCX_DTVCC_NO_LAST_SEQUENCE;
        let no_rollup = is_true(opts.no_rollup);

        // Initialize decoders - only for active services
        // Note: dtvcc_service_decoder is a large struct, so we must allocate it
        // directly on the heap to avoid stack overflow.
        let decoders = {
            const INIT: Option<Box<dtvcc_service_decoder>> = None;
            let mut decoders = [INIT; CCX_DTVCC_MAX_SERVICES];

            for (i, d) in decoders.iter_mut().enumerate() {
                if i >= opts.services_enabled.len() || !is_true(opts.services_enabled[i]) {
                    continue;
                }

                // Create owned tv_screen on the heap using zeroed allocation
                // to avoid stack overflow (dtvcc_tv_screen is also large)
                let tv_layout = std::alloc::Layout::new::<dtvcc_tv_screen>();
                let tv_ptr = unsafe { std::alloc::alloc_zeroed(tv_layout) } as *mut dtvcc_tv_screen;
                if tv_ptr.is_null() {
                    panic!("Failed to allocate dtvcc_tv_screen");
                }
                let mut tv_screen = unsafe { Box::from_raw(tv_ptr) };
                tv_screen.cc_count = 0;
                tv_screen.service_number = i as i32 + 1;

                // Allocate decoder directly on heap using zeroed memory to avoid
                // stack overflow (dtvcc_service_decoder is very large)
                let decoder_layout = std::alloc::Layout::new::<dtvcc_service_decoder>();
                let decoder_ptr = unsafe { std::alloc::alloc_zeroed(decoder_layout) } as *mut dtvcc_service_decoder;
                if decoder_ptr.is_null() {
                    panic!("Failed to allocate dtvcc_service_decoder");
                }

                let mut decoder = unsafe { Box::from_raw(decoder_ptr) };

                // Set the tv pointer
                decoder.tv = Box::into_raw(tv_screen);

                // Initialize windows
                for window in decoder.windows.iter_mut() {
                    window.memory_reserved = 0;
                }

                // Call reset handler
                decoder.handle_reset();

                *d = Some(decoder);
            }

            decoders
        };

        // Encoder is set later via set_encoder()
        let encoder = std::ptr::null_mut();

        DtvccRust {
            is_active,
            active_services_count,
            services_active,
            report_enabled,
            report: opts.report,
            decoders,
            packet,
            packet_length,
            is_header_parsed,
            last_sequence,
            no_rollup,
            timing: opts.timing,
            encoder,
        }
    }

    /// Set the encoder for this context.
    ///
    /// The encoder is typically not available at initialization time,
    /// so it must be set separately before processing.
    pub fn set_encoder(&mut self, encoder: *mut encoder_ctx) {
        self.encoder = encoder;
    }

    /// Process cc data and add it to the dtvcc packet.
    ///
    /// This is the main entry point for CEA-708 data processing.
    pub fn process_cc_data(&mut self, cc_valid: u8, cc_type: u8, data1: u8, data2: u8) {
        if !self.is_active && !self.report_enabled {
            return;
        }

        match cc_type {
            // type 0 and 1 are for CEA 608 data and are handled before calling this function
            // valid types for CEA 708 data are only 2 and 3
            2 => {
                log_debug!("dtvcc_process_data: DTVCC Channel Packet Data");
                if cc_valid == 1 && self.is_header_parsed {
                    if self.packet_length > 253 {
                        log_debug!("dtvcc_process_data: Warning: Legal packet size exceeded (1), data not added.");
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
                log_debug!("dtvcc_process_data: DTVCC Channel Packet Start");
                if cc_valid == 1 {
                    if self.packet_length > (CCX_DTVCC_MAX_PACKET_LENGTH - 1) {
                        log_debug!("dtvcc_process_data: Warning: Legal packet size exceeded (2), data not added.");
                    } else {
                        if self.is_header_parsed {
                            log_debug!("dtvcc_process_data: Warning: Incorrect packet length specified. Packet will be skipped.");
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
    fn add_data_to_packet(&mut self, data1: u8, data2: u8) {
        self.packet[self.packet_length as usize] = data1;
        self.packet_length += 1;
        self.packet[self.packet_length as usize] = data2;
        self.packet_length += 1;
    }

    /// Process current packet into service blocks
    fn process_current_packet(&mut self, len: u8) {
        let seq = (self.packet[0] & 0xC0) >> 6;
        log_debug!(
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
            log_debug!(
                "dtvcc_process_current_packet: Unexpected sequence number, it is {} but should be {}",
                seq, (self.last_sequence + 1) % 4
            );
        }
        self.last_sequence = seq as i32;

        let mut pos: u8 = 1;
        while pos < len {
            let mut service_number = (self.packet[pos as usize] & 0xE0) >> 5; // 3 more significant bits
            let block_length = self.packet[pos as usize] & 0x1F; // 5 less significant bits
            log_debug!(
                "dtvcc_process_current_packet: Standard header Service number: {}, Block length: {}",
                service_number, block_length
            );

            if service_number == 7 {
                // There is an extended header
                // CEA-708-E 6.2.2 Extended Service Block Header
                pos += 1;
                service_number = self.packet[pos as usize] & 0x3F; // 6 more significant bits
                if service_number > 7 {
                    log_debug!(
                        "dtvcc_process_current_packet: Illegal service number in extended header: {}",
                        service_number
                    );
                }
            }

            pos += 1;

            if service_number == 0 && block_length != 0 {
                // Illegal, but specs say what to do...
                pos = len; // Move to end
                break;
            }

            if block_length != 0 && !self.report.is_null() {
                unsafe {
                    (*self.report).services[service_number as usize] = 1;
                }
            }

            if service_number > 0 && is_true(self.services_active[(service_number - 1) as usize]) {
                if let Some(decoder) = &mut self.decoders[(service_number - 1) as usize] {
                    // Get encoder and timing references
                    if !self.encoder.is_null() && !self.timing.is_null() {
                        let encoder = unsafe { &mut *self.encoder };
                        let timing = unsafe { &mut *self.timing };
                        decoder.process_service_block(
                            &self.packet[pos as usize..(pos + block_length) as usize],
                            encoder,
                            timing,
                            self.no_rollup,
                        );
                    }
                }
            }

            pos += block_length // Skip data
        }

        self.clear_packet();

        if len < 128 && self.packet[pos as usize] != 0 {
            // Null header is mandatory if there is room
            log_debug!("dtvcc_process_current_packet: Warning: Null header expected but not found.");
        }
    }

    /// Clear current packet
    fn clear_packet(&mut self) {
        self.packet_length = 0;
        self.is_header_parsed = false;
        self.packet.iter_mut().for_each(|x| *x = 0);
    }

    /// Flush all active service decoders.
    ///
    /// This writes out any pending caption data from all active services.
    /// Called when processing is complete or when switching contexts.
    pub fn flush_active_decoders(&mut self) {
        if !self.is_active {
            return;
        }

        for i in 0..CCX_DTVCC_MAX_SERVICES {
            if i >= self.services_active.len() || !is_true(self.services_active[i]) {
                continue;
            }

            if let Some(decoder) = &mut self.decoders[i] {
                if decoder.cc_count > 0 {
                    // Flush this decoder
                    self.flush_decoder(i);
                }
            }
        }
    }

    /// Flush a specific service decoder by index.
    fn flush_decoder(&mut self, service_index: usize) {
        log_debug!("dtvcc_decoder_flush: Flushing decoder for service {}", service_index + 1);

        // Need encoder and timing to flush
        if self.encoder.is_null() || self.timing.is_null() {
            log_debug!("dtvcc_decoder_flush: Cannot flush - encoder or timing is null");
            return;
        }

        if let Some(decoder) = &mut self.decoders[service_index] {
            let timing = unsafe { &mut *self.timing };
            let encoder = unsafe { &mut *self.encoder };

            let mut screen_content_changed = false;

            // Process all visible windows
            for i in 0..CCX_DTVCC_MAX_WINDOWS {
                let window = &mut decoder.windows[i as usize];
                if is_true(window.visible) {
                    screen_content_changed = true;
                    window.update_time_hide(timing);
                    // Copy window content to screen
                    decoder.copy_to_screen(&decoder.windows[i as usize]);
                    decoder.windows[i as usize].visible = 0;
                }
            }

            if screen_content_changed {
                decoder.screen_print(encoder, timing);
            }
            decoder.flush(encoder);
        }
    }
}

const CCX_DTVCC_MAX_WINDOWS: u8 = 8;

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
        let sym = ((data1 as u16) << 8) | data2 as u16;
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
pub mod test {
    use lib_ccxr::util::log::{set_logger, CCExtractorLogger, DebugMessageMask, OutputTarget};

    use crate::utils::get_zero_allocated_obj;

    use super::*;

    pub fn initialize_dtvcc_ctx() -> Box<dtvcc_ctx> {
        let mut ctx = get_zero_allocated_obj::<dtvcc_ctx>();

        // Initialize the required pointers to avoid null pointer dereference
        let report = Box::new(ccx_decoder_dtvcc_report::default());
        ctx.report = Box::into_raw(report);

        let encoder = Box::new(encoder_ctx::default());
        ctx.encoder = Box::into_raw(encoder) as *mut _ as *mut std::os::raw::c_void;

        let timing = Box::new(ccx_common_timing_ctx::default());
        ctx.timing = Box::into_raw(timing);
        ctx
    }
    #[test]
    fn test_process_cc_data() {
        set_logger(CCExtractorLogger::new(
            OutputTarget::Stdout,
            DebugMessageMask::new(DebugMessageFlag::VERBOSE, DebugMessageFlag::VERBOSE),
            false,
        ))
        .ok();

        let mut dtvcc_ctx = initialize_dtvcc_ctx();

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
        assert!(decoder.is_header_parsed);
    }

    #[test]
    fn test_process_current_packet() {
        set_logger(CCExtractorLogger::new(
            OutputTarget::Stdout,
            DebugMessageMask::new(DebugMessageFlag::VERBOSE, DebugMessageFlag::VERBOSE),
            false,
        ))
        .ok();

        let mut dtvcc_ctx = initialize_dtvcc_ctx();

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

    // =========================================================================
    // Tests for DtvccRust (persistent CEA-708 decoder)
    // =========================================================================

    /// Helper function to create a test ccx_decoder_dtvcc_settings
    /// Uses heap allocation to avoid stack overflow with large structs
    pub fn create_test_dtvcc_settings() -> Box<ccx_decoder_dtvcc_settings> {
        let mut settings = get_zero_allocated_obj::<ccx_decoder_dtvcc_settings>();

        // Initialize required pointers using heap allocation
        let report = get_zero_allocated_obj::<ccx_decoder_dtvcc_report>();
        settings.report = Box::into_raw(report);

        let timing = get_zero_allocated_obj::<ccx_common_timing_ctx>();
        settings.timing = Box::into_raw(timing);

        // Enable the decoder and first service
        settings.enabled = 1;
        settings.active_services_count = 1;
        settings.services_enabled[0] = 1;

        settings
    }

    #[test]
    fn test_dtvcc_rust_new() {
        let settings = create_test_dtvcc_settings();
        let dtvcc = DtvccRust::new(&settings);

        // Verify basic initialization
        assert!(dtvcc.is_active);
        assert_eq!(dtvcc.active_services_count, 1);
        assert_eq!(dtvcc.packet_length, 0);
        assert!(!dtvcc.is_header_parsed);
        assert_eq!(dtvcc.last_sequence, CCX_DTVCC_NO_LAST_SEQUENCE);

        // Verify encoder is initially null (set later)
        assert!(dtvcc.encoder.is_null());

        // Verify first decoder is created (service 0 is enabled)
        assert!(dtvcc.decoders[0].is_some());

        // Verify other decoders are not created
        assert!(dtvcc.decoders[1].is_none());
    }

    #[test]
    fn test_dtvcc_rust_set_encoder() {
        let settings = create_test_dtvcc_settings();
        let mut dtvcc = DtvccRust::new(&settings);

        // Initially null
        assert!(dtvcc.encoder.is_null());

        // Create an encoder and set it
        let mut encoder = Box::new(encoder_ctx::default());
        let encoder_ptr = &mut *encoder as *mut encoder_ctx;
        dtvcc.set_encoder(encoder_ptr);

        // Verify encoder is set
        assert!(!dtvcc.encoder.is_null());
        assert_eq!(dtvcc.encoder, encoder_ptr);
    }

    #[test]
    fn test_dtvcc_rust_process_cc_data() {
        let settings = create_test_dtvcc_settings();
        let mut dtvcc = DtvccRust::new(&settings);

        // Process cc_type = 3 (packet start) - should set is_header_parsed
        dtvcc.process_cc_data(1, 3, 0xC2, 0x00);

        assert!(dtvcc.is_header_parsed);
        assert_eq!(dtvcc.packet_length, 2);
        assert_eq!(dtvcc.packet[0], 0xC2);
        assert_eq!(dtvcc.packet[1], 0x00);
    }

    #[test]
    fn test_dtvcc_rust_clear_packet() {
        let settings = create_test_dtvcc_settings();
        let mut dtvcc = DtvccRust::new(&settings);

        // Add some data
        dtvcc.process_cc_data(1, 3, 0xC2, 0x00);
        assert!(dtvcc.is_header_parsed);
        assert_eq!(dtvcc.packet_length, 2);

        // Process more data that triggers clear (when packet is malformed)
        // Simulate by directly testing the packet processing
        dtvcc.is_header_parsed = true;
        dtvcc.packet[0] = 0x02; // Very short packet length (2*1 = 2 bytes)
        dtvcc.packet_length = 2;

        // This should process and clear the packet
        dtvcc.process_cc_data(1, 2, 0x00, 0x00);

        // After processing a complete packet, it should be cleared
        assert_eq!(dtvcc.packet_length, 0);
        assert!(!dtvcc.is_header_parsed);
    }

    #[test]
    fn test_dtvcc_rust_state_persistence() {
        // This test verifies the key fix: state persists across calls
        let settings = create_test_dtvcc_settings();
        let mut dtvcc = DtvccRust::new(&settings);

        // First call: start a packet
        dtvcc.process_cc_data(1, 3, 0xC4, 0x00); // Packet with length 4*2=8 bytes
        assert!(dtvcc.is_header_parsed);
        assert_eq!(dtvcc.packet_length, 2);

        // Second call: add more data (this is where the old code would fail)
        dtvcc.process_cc_data(1, 2, 0x21, 0x00);
        assert_eq!(dtvcc.packet_length, 4);

        // Third call: add more data
        dtvcc.process_cc_data(1, 2, 0x00, 0x00);
        assert_eq!(dtvcc.packet_length, 6);

        // State is preserved across all calls!
        assert!(dtvcc.is_header_parsed);
        assert_eq!(dtvcc.last_sequence, CCX_DTVCC_NO_LAST_SEQUENCE); // Not processed yet
    }
}
