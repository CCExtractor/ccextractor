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
    let debug_mask =
        DebugMessageFlag::from_bits(ccx_options.debug_mask.try_into().unwrap()).unwrap();
    let debug_mask_on_debug =
        DebugMessageFlag::from_bits(ccx_options.debug_mask_on_debug.try_into().unwrap()).unwrap();
    let mask = DebugMessageMask::new(debug_mask, debug_mask_on_debug);
    let gui_mode_reports = ccx_options.gui_mode_reports != 0;
    let messages_target = match ccx_options.messages_target {
        0 => OutputTarget::Stdout,
        1 => OutputTarget::Stderr,
        2 => OutputTarget::Quiet,
        _ => panic!("incorrect value for messages_target"),
    };
    set_logger(CCExtractorLogger::new(
        messages_target,
        mask,
        gui_mode_reports,
    ))
    .expect("Failed to initialize and setup the logger");
}

/// Rust equivalent for `verify_crc32` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `buf` should not be a NULL pointer and the length of buffer pointed by `buf` should be equal to
/// or less than `len`.
#[no_mangle]
pub unsafe extern "C" fn ccxr_verify_crc32(buf: *const u8, len: c_int) -> c_int {
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
    let s1 = std::slice::from_raw_parts(s1, s1len.try_into().unwrap());
    let s2 = std::slice::from_raw_parts(s2, s2len.try_into().unwrap());

    let ans = levenshtein_dist(s1, s2);

    ans.try_into()
        .expect("Failed to convert the levenshtein distance to C int")
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
    let s1 = std::slice::from_raw_parts(s1, s1len.try_into().unwrap());
    let s2 = std::slice::from_raw_parts(s2, s2len.try_into().unwrap());

    let ans = levenshtein_dist_char(s1, s2);

    ans.try_into()
        .expect("Failed to convert the levenshtein distance to C int")
}
