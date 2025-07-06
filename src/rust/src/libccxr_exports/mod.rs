//! Provides C-FFI functions that are direct equivalent of functions available in C.

pub mod bitstream;
pub mod demuxer;
pub mod demuxerdata;
pub mod net;
pub mod mythtv;
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
    // Safety: avoid NULL pointer and negative length causing usize wraparound
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

/// Builds a slice from a C buffer, treating NULL as empty.
///
/// C code passes an empty buffer as `(NULL, 0)`, but `slice::from_raw_parts`
/// requires a non-null pointer even for a zero length.
///
/// # Safety
///
/// If `ptr` is not null it must point to at least `len` valid elements.
unsafe fn slice_or_empty<'a, T>(ptr: *const T, len: c_uint) -> &'a [T] {
    if ptr.is_null() {
        &[]
    } else {
        std::slice::from_raw_parts(ptr, len as usize)
    }
}

/// Rust equivalent for `levenshtein_dist` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `s1` and `s2` must be valid slices of data with lengths of `s1len` and `s2len` respectively,
/// or NULL to denote an empty slice.
#[no_mangle]
pub unsafe extern "C" fn ccxr_levenshtein_dist(
    s1: *const u64,
    s2: *const u64,
    s1len: c_uint,
    s2len: c_uint,
) -> c_int {
    let s1 = slice_or_empty(s1, s1len);
    let s2 = slice_or_empty(s2, s2len);

    let ans = levenshtein_dist(s1, s2);

    ans.min(c_int::MAX as usize) as c_int
}

/// Rust equivalent for `levenshtein_dist_char` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `s1` and `s2` must be valid slices of data with lengths of `s1len` and `s2len` respectively,
/// or NULL to denote an empty slice.
#[no_mangle]
pub unsafe extern "C" fn ccxr_levenshtein_dist_char(
    s1: *const c_char,
    s2: *const c_char,
    s1len: c_uint,
    s2len: c_uint,
) -> c_int {
    let s1 = slice_or_empty(s1, s1len);
    let s2 = slice_or_empty(s2, s2len);

    let ans = levenshtein_dist_char(s1, s2);

    ans.min(c_int::MAX as usize) as c_int
}
pub mod util;

#[cfg(test)]
mod tests {
    use super::*;
    use std::ptr::null;

    // C passes an empty compare buffer as (NULL, 0), e.g. telxcc's
    // ucs2_buffer_cur right after a buffer switch.
    #[test]
    fn test_levenshtein_dist_accepts_null_empty_buffers() {
        let s = [1u64, 2, 3];
        unsafe {
            assert_eq!(ccxr_levenshtein_dist(null(), null(), 0, 0), 0);
            assert_eq!(ccxr_levenshtein_dist(s.as_ptr(), null(), 3, 0), 3);
            assert_eq!(ccxr_levenshtein_dist(null(), s.as_ptr(), 0, 3), 3);
            assert_eq!(ccxr_levenshtein_dist(s.as_ptr(), s.as_ptr(), 3, 3), 0);
        }
    }

    #[test]
    fn test_levenshtein_dist_char_accepts_null_empty_buffers() {
        let s = b"abc".map(|b| b as c_char);
        unsafe {
            assert_eq!(ccxr_levenshtein_dist_char(null(), null(), 0, 0), 0);
            assert_eq!(ccxr_levenshtein_dist_char(s.as_ptr(), null(), 3, 0), 3);
            assert_eq!(ccxr_levenshtein_dist_char(null(), s.as_ptr(), 0, 3), 3);
            assert_eq!(ccxr_levenshtein_dist_char(s.as_ptr(), s.as_ptr(), 3, 3), 0);
        }
    }
}
