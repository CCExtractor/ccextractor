use std::ffi::{c_char, c_int};

#[no_mangle]
pub unsafe extern "C" fn ccxr_hex_to_int(high: c_char, low: c_char) -> c_int {
    lib_ccxr::util::hex::hex_to_int(high as u8 as char, low as u8 as char).unwrap_or(-1)
}
