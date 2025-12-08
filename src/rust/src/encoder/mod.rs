use crate::bindings::ccx_encoding_type;
use crate::ctorust::FromCType;
use lib_ccxr::encoder::txt_helpers::get_str_basic;
use lib_ccxr::util::encoding::Encoding;
use std::os::raw::{c_int, c_uchar};

pub mod common;
pub mod g608;
pub mod simplexml;
/// # Safety
/// This function is unsafe because it deferences to raw pointers and performs operations on pointer slices.
#[no_mangle]
pub unsafe fn ccxr_get_str_basic(
    out_buffer: *mut c_uchar,
    in_buffer: *mut c_uchar,
    trim_subs: c_int,
    in_enc: ccx_encoding_type,
    out_enc: ccx_encoding_type,
    max_len: c_int,
) -> c_int {
    let trim_subs_bool = trim_subs != 0;
    let in_encoding = match Encoding::from_ctype(in_enc) {
        Some(enc) => enc,
        None => return 0,
    };
    let out_encoding = match Encoding::from_ctype(out_enc) {
        Some(enc) => enc,
        None => return 0,
    };
    let in_buffer_slice = if in_buffer.is_null() || max_len <= 0 {
        return 0;
    } else {
        std::slice::from_raw_parts(in_buffer, max_len as usize)
    };
    let mut out_buffer_vec = Vec::with_capacity(max_len as usize * 4); // UTF-8 can be up to 4 bytes per character
    let result = get_str_basic(
        &mut out_buffer_vec,
        in_buffer_slice,
        trim_subs_bool,
        in_encoding,
        out_encoding,
        max_len,
    );
    if result > 0 && !out_buffer.is_null() {
        let copy_len = std::cmp::min(result as usize, out_buffer_vec.len());
        if copy_len > 0 {
            std::ptr::copy_nonoverlapping(out_buffer_vec.as_ptr(), out_buffer, copy_len);
        }
        if copy_len < max_len as usize {
            *out_buffer.add(copy_len) = 0;
        }
    } else if !out_buffer.is_null() {
        *out_buffer = 0;
    }
    result
}
