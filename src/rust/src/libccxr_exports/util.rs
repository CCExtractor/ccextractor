use lib_ccxr::util::hex;
use std::ffi::c_char;

#[no_mangle]
pub extern "C" fn ccxr_hex_to_int(high: c_char, low: c_char) -> i32 {
    hex::hex_to_int(high as u8 as char, low as u8 as char)
}
