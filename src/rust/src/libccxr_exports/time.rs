#![allow(clippy::useless_conversion)]

use crate::bindings::*;

use std::ffi::CStr;
use std::os::raw::{c_char, c_int};

use lib_ccxr::util::time::{c_functions as c, *};

/// Helper function that converts a Rust-String (`string`) to C-String (`buffer`).
///
/// # Safety
///
/// `buffer` must have enough allocated space for `string` to fit.
unsafe fn write_string_into_pointer(buffer: *mut c_char, string: &str) {
    let buffer = std::slice::from_raw_parts_mut(buffer as *mut u8, string.len() + 1);
    buffer[..string.len()].copy_from_slice(string.as_bytes());
    buffer[string.len()] = b'\0';
}

/// Rust equivalent for `timestamp_to_srttime` function in C. Uses C-native types as input and
/// output.
///
/// # Safety
///
/// `buffer` must have enough allocated space for the formatted `timestamp` to fit.
#[no_mangle]
pub unsafe extern "C" fn ccxr_timestamp_to_srttime(timestamp: u64, buffer: *mut c_char) {
    let mut s = String::new();
    let timestamp = Timestamp::from_millis(timestamp as i64);

    let _ = c::timestamp_to_srttime(timestamp, &mut s);

    write_string_into_pointer(buffer, &s);
}

/// Rust equivalent for `timestamp_to_vtttime` function in C. Uses C-native types as input and
/// output.
///
/// # Safety
///
/// `buffer` must have enough allocated space for the formatted `timestamp` to fit.
#[no_mangle]
pub unsafe extern "C" fn ccxr_timestamp_to_vtttime(timestamp: u64, buffer: *mut c_char) {
    let mut s = String::new();
    let timestamp = Timestamp::from_millis(timestamp as i64);

    let _ = c::timestamp_to_vtttime(timestamp, &mut s);

    write_string_into_pointer(buffer, &s);
}

/// Rust equivalent for `millis_to_date` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `buffer` must have enough allocated space for the formatted `timestamp` to fit.
#[no_mangle]
pub unsafe extern "C" fn ccxr_millis_to_date(
    timestamp: u64,
    buffer: *mut c_char,
    date_format: ccx_output_date_format,
    millis_separator: c_char,
) {
    let mut s = String::new();
    let timestamp = Timestamp::from_millis(timestamp as i64);
    let date_format = match date_format {
        ccx_output_date_format::ODF_NONE => TimestampFormat::None,
        ccx_output_date_format::ODF_HHMMSS => TimestampFormat::HHMMSS,
        ccx_output_date_format::ODF_HHMMSSMS => TimestampFormat::HHMMSSFFF,
        ccx_output_date_format::ODF_SECONDS => TimestampFormat::Seconds {
            millis_separator: millis_separator as u8 as char,
        },
        ccx_output_date_format::ODF_DATE => TimestampFormat::Date {
            millis_separator: millis_separator as u8 as char,
        },
    };

    let _ = c::millis_to_date(timestamp, &mut s, date_format);

    write_string_into_pointer(buffer, &s);
}

/// Rust equivalent for `stringztoms` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `s` must contain valid utf-8 data and have a nul terminator at the end of the string.
#[no_mangle]
pub unsafe extern "C" fn ccxr_stringztoms(s: *const c_char, bt: *mut ccx_boundary_time) -> c_int {
    let s = CStr::from_ptr(s)
        .to_str()
        .expect("Failed to convert buffer `s` into a &str");

    let option_timestamp = c::stringztoms(s);

    if let Some(timestamp) = option_timestamp {
        if let Ok((h, m, s, _)) = timestamp.as_hms_millis() {
            (*bt).set = 1;
            (*bt).hh = h.into();
            (*bt).mm = m.into();
            (*bt).ss = s.into();
            (*bt).time_in_ms = (timestamp.millis() / 1000) * 1000;
            return 0;
        }
    };

    -1
}
