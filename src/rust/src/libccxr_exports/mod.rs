//! Provides C-FFI functions that are direct equivalent of functions available in C.

pub mod bitstream;
pub mod demuxer;
pub mod demuxerdata;
pub mod net;
pub mod time;
use crate::ccx_options;
use lib_ccxr::util::log::*;
use lib_ccxr::util::{bits::*, levenshtein::*};

use std::convert::TryInto;
use std::ffi::{c_char, c_int, c_uint};

/// Initializes the logger at the rust side.
///
/// # Safety
///
/// `ccx_options` in C must initialized properly before calling this function.
#[no_mangle]
pub unsafe extern "C" fn ccxr_init_basic_logger() {
    let debug_mask = ccx_options
        .debug_mask
        .try_into()
        .ok()
        .and_then(DebugMessageFlag::from_bits)
        .unwrap_or(DebugMessageFlag::VERBOSE);
    let debug_mask_on_debug = ccx_options
        .debug_mask_on_debug
        .try_into()
        .ok()
        .and_then(DebugMessageFlag::from_bits)
        .unwrap_or(DebugMessageFlag::VERBOSE);
    let mask = DebugMessageMask::new(debug_mask, debug_mask_on_debug);
    let gui_mode_reports = ccx_options.gui_mode_reports != 0;
    // CCX_MESSAGES_QUIET=0, CCX_MESSAGES_STDOUT=1, CCX_MESSAGES_STDERR=2
    let messages_target = match ccx_options.messages_target {
        0 => OutputTarget::Quiet,
        1 => OutputTarget::Stdout,
        2 => OutputTarget::Stderr,
        _ => OutputTarget::Stderr, // Default to stderr for invalid values
    };
    let _ = set_logger(CCExtractorLogger::new(
        messages_target,
        mask,
        gui_mode_reports,
    ));
}

/// Updates the logger target after command-line arguments have been parsed.
/// This is needed because the logger is initialized before argument parsing,
/// and options like --quiet need to be applied afterwards.
///
/// # Safety
///
/// `ccx_options` in C must be properly initialized and the logger must have
/// been initialized via `ccxr_init_basic_logger` before calling this function.
#[no_mangle]
pub unsafe extern "C" fn ccxr_update_logger_target() {
    // CCX_MESSAGES_QUIET=0, CCX_MESSAGES_STDOUT=1, CCX_MESSAGES_STDERR=2
    let messages_target = match ccx_options.messages_target {
        0 => OutputTarget::Quiet,
        1 => OutputTarget::Stdout,
        2 => OutputTarget::Stderr,
        _ => OutputTarget::Stderr,
    };
    if let Some(mut logger) = logger_mut() {
        logger.set_target(messages_target);
    }
}

/// Rust equivalent for `verify_crc32` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `buf` should not be a NULL pointer and the length of buffer pointed by `buf` should be equal to
/// or less than `len`.
#[no_mangle]
pub unsafe extern "C" fn ccxr_verify_crc32(buf: *const u8, len: c_int) -> c_int {
    if buf.is_null() || len < 0 {
        return 0;
    }
    let buf = std::slice::from_raw_parts(buf, len as usize);
    if verify_crc32(buf) {
        1
    } else {
        0
    }
}

/// Rust equivalent for `levenshtein_dist` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `s1` and `s2` must valid slices of data with lengths of `s1len` and `s2len` respectively.
#[no_mangle]
pub unsafe extern "C" fn ccxr_levenshtein_dist(
    s1: *const u64,
    s2: *const u64,
    s1len: c_uint,
    s2len: c_uint,
) -> c_int {
    if s1.is_null() || s2.is_null() {
        return 0;
    }
    let s1 = std::slice::from_raw_parts(s1, s1len as usize);
    let s2 = std::slice::from_raw_parts(s2, s2len as usize);

    let ans = levenshtein_dist(s1, s2);

    ans.min(c_int::MAX as usize) as c_int
}

/// Rust equivalent for `levenshtein_dist_char` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `s1` and `s2` must valid slices of data and therefore not be null. They must have lengths
/// of `s1len` and `s2len` respectively.
#[no_mangle]
pub unsafe extern "C" fn ccxr_levenshtein_dist_char(
    s1: *const c_char,
    s2: *const c_char,
    s1len: c_uint,
    s2len: c_uint,
) -> c_int {
    if s1.is_null() || s2.is_null() {
        return 0;
    }
    let s1 = std::slice::from_raw_parts(s1, s1len as usize);
    let s2 = std::slice::from_raw_parts(s2, s2len as usize);

    let ans = levenshtein_dist_char(s1, s2);

    ans.min(c_int::MAX as usize) as c_int
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::ptr;

    #[test]
    fn test_ffi_safety() {
        unsafe {
            // Test NULL pointer and negative length safety for CRC32
            assert_eq!(ccxr_verify_crc32(ptr::null(), 10), 0);
            assert_eq!(ccxr_verify_crc32(ptr::null(), -1), 0);
            let buf = [1, 2, 3];
            assert_eq!(ccxr_verify_crc32(buf.as_ptr(), -1), 0);

            // Test NULL pointer safety for Levenshtein
            assert_eq!(ccxr_levenshtein_dist(ptr::null(), ptr::null(), 0, 0), 0);
            assert_eq!(
                ccxr_levenshtein_dist_char(ptr::null(), ptr::null(), 0, 0),
                0
            );
        }
    }
}
