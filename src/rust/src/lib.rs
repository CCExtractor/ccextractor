//! Rust library for CCExtractor
//!
//! Currently we are in the process of porting the 708 decoder to rust. See [decoder]

// Allow C naming style
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

/// CCExtractor C bindings generated by bindgen
#[allow(clippy::all)]
pub mod bindings {
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}
pub mod decoder;
#[cfg(feature = "hardsubx_ocr")]
pub mod hardsubx;
pub mod utils;

#[cfg(windows)]
use std::os::windows::io::{FromRawHandle, RawHandle};
use std::{io::Write, os::raw::c_int};

use bindings::*;
use decoder::Dtvcc;
use utils::is_true;

use env_logger::{builder, Target};
use log::{warn, LevelFilter};

extern "C" {
    static mut cb_708: c_int;
    static mut cb_field1: c_int;
    static mut cb_field2: c_int;
}

/// Initialize env logger with custom format, using stdout as target
#[no_mangle]
pub extern "C" fn ccxr_init_logger() {
    builder()
        .format(|buf, record| writeln!(buf, "[CEA-708] {}", record.args()))
        .filter_level(LevelFilter::Debug)
        .target(Target::Stdout)
        .init();
}

/// Create a `dtvcc_rust`
///
/// SAFETY:
/// The following should not be `NULL`:
///     - opts
///     - opts.report
///     - opts.timing
#[no_mangle]
extern "C" fn ccxr_dtvcc_init<'a>(opts: *mut ccx_decoder_dtvcc_settings) -> *mut Dtvcc<'a> {
    Box::into_raw(Box::new(Dtvcc::new(unsafe { opts.as_mut() }.unwrap())))
}

/// Frees `dtvcc_rust`
///
/// SAFETY:
/// The following should not be `NULL`:
///     - dtvcc_rust
///     - dtvcc_rust.decoders[i] if dtvcc_rust.services_active[i] is true
///     - dtvcc_rust.decoders[i].windows[j].rows[k] if
///         dtvcc_rust.decoders[i].windows[j].memory_reserved is true
///     - dtvcc_rust.decoders[i].tv
#[no_mangle]
extern "C" fn ccxr_dtvcc_free(dtvcc_rust: *mut Dtvcc) {
    unsafe { dtvcc_rust.drop_in_place() };
}

#[no_mangle]
extern "C" fn ccxr_dtvcc_set_encoder(dtvcc_rust: *mut Dtvcc, encoder: *mut encoder_ctx) {
    unsafe { dtvcc_rust.as_mut() }.unwrap().encoder = Some(unsafe { encoder.as_mut() }.unwrap());
}

/// Process cc_data
///
/// # Safety
/// dec_ctx should not be a null pointer
/// data should point to cc_data of length cc_count
#[no_mangle]
extern "C" fn ccxr_process_cc_data(
    dec_ctx: *mut lib_cc_decode,
    data: *const ::std::os::raw::c_uchar,
    cc_count: c_int,
) -> c_int {
    let mut ret = -1;
    let mut cc_data: Vec<u8> = (0..cc_count * 3)
        .map(|x| unsafe { *data.add(x as usize) })
        .collect();
    let dec_ctx = unsafe { &mut *dec_ctx };
    for cc_block in cc_data.chunks_exact_mut(3) {
        if !validate_cc_pair(cc_block) {
            continue;
        }
        let success = do_cb(dec_ctx, cc_block);
        if success {
            ret = 0;
        }
    }
    ret
}

/// Returns `true` if cc_block pair is valid
///
/// For CEA-708 data, only cc_valid is checked
/// For CEA-608 data, parity is also checked
pub fn validate_cc_pair(cc_block: &mut [u8]) -> bool {
    let cc_valid = (cc_block[0] & 4) >> 2;
    let cc_type = cc_block[0] & 3;
    if cc_valid == 0 {
        return false;
    }
    if cc_type == 0 || cc_type == 1 {
        // For CEA-608 data we verify parity.
        if verify_parity(cc_block[2]) {
            // If the second byte doesn't pass parity, ignore pair
            return false;
        }
        if verify_parity(cc_block[1]) {
            // If the first byte doesn't pass parity,
            // we replace it with a solid blank and process the pair.
            cc_block[1] = 0x7F;
        }
    }
    true
}

/// Returns `true` if data has odd parity
///
/// CC uses odd parity (i.e., # of 1's in byte is odd.)
pub fn verify_parity(data: u8) -> bool {
    if data.count_ones() & 1 == 1 {
        return true;
    }
    false
}

/// Process CC data according to its type
pub fn do_cb(ctx: &mut lib_cc_decode, cc_block: &[u8]) -> bool {
    let dtvcc = unsafe { &mut *(ctx.dtvcc_rust as *mut decoder::Dtvcc<'_>) };
    let cc_valid = (cc_block[0] & 4) >> 2;
    let cc_type = cc_block[0] & 3;
    let mut timeok = true;

    if ctx.write_format != ccx_output_format::CCX_OF_DVDRAW
        && ctx.write_format != ccx_output_format::CCX_OF_RAW
        && (cc_block[0] == 0xFA || cc_block[0] == 0xFC || cc_block[0] == 0xFD)
        && (cc_block[1] & 0x7F) == 0
        && (cc_block[2] & 0x7F) == 0
    {
        return true;
    }

    if cc_valid == 1 || cc_type == 3 {
        ctx.cc_stats[cc_type as usize] += 1;
        match cc_type {
            // Type 0 and 1 are for CEA-608 data. Handled by C code, do nothing
            0 | 1 => {}
            // Type 2 and 3 are for CEA-708 data.
            2 | 3 => {
                let current_time = unsafe { (*ctx.timing).get_fts(ctx.current_field as u8) };
                ctx.current_field = 3;

                // Check whether current time is within start and end bounds
                if is_true(ctx.extraction_start.set)
                    && current_time < ctx.extraction_start.time_in_ms
                {
                    timeok = false;
                }
                if is_true(ctx.extraction_end.set) && current_time > ctx.extraction_end.time_in_ms {
                    timeok = false;
                    ctx.processed_enough = 1;
                }

                if timeok && ctx.write_format != ccx_output_format::CCX_OF_RAW {
                    dtvcc.process_cc_data(cc_valid, cc_type, cc_block[1], cc_block[2]);
                }
                unsafe { cb_708 += 1 }
            }
            _ => warn!("Invalid cc_type"),
        }
    }
    true
}

#[cfg(windows)]
#[no_mangle]
extern "C" fn ccxr_close_handle(handle: RawHandle) {
    use std::fs::File;

    if handle.is_null() {
        return;
    }
    unsafe {
        // File will close automatically (due to Drop) once it goes out of scope
        let _file = File::from_raw_handle(handle);
    }
}
