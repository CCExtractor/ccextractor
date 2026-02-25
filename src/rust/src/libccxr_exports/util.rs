use std::ffi::{c_char, c_int};

/// Rust equivalent for `hex_to_int` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `high` and `low` must be valid ASCII characters. Invalid characters will
/// return -1 rather than causing undefined behavior.
#[no_mangle]
pub unsafe extern "C" fn ccxr_hex_to_int(high: c_char, low: c_char) -> c_int {
    lib_ccxr::util::hex::hex_to_int(high as u8 as char, low as u8 as char).unwrap_or(-1)
}
#[no_mangle]
pub unsafe extern "C" fn ccxr_hex_string_to_int(string: *const c_char, len: c_int) -> c_int {
    let s = std::ffi::CStr::from_ptr(string).to_str().unwrap_or("");
    lib_ccxr::util::hex::hex_string_to_int(s, len as usize).unwrap_or(-1)
}
