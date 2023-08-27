//! Provides C-FFI functions that are direct equivalent of functions available in C.

use lib_ccxr::util::c_functions::*;
use std::convert::TryInto;
use std::os::raw::{c_char, c_int, c_uint};

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

    ans.try_into().unwrap()
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

    ans.try_into().unwrap()
}
