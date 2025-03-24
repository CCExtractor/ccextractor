extern crate libc;

use lib_ccxr::vbis::vbis::*;
use lib_ccxr::vbis::vbid::*;

use std::os::raw::{c_int, c_long, c_char, c_ulonglong, c_uchar, c_uint, c_void};
use std::ptr;
use std::mem;

#[no_mangle]
pub unsafe extern "C" fn ccxr_delete_decoder_vbi(arg: *mut *mut CcxDecoderVbiCtx) {
    delete_decoder_vbi(arg);
}

#[no_mangle]
pub extern "C" fn ccxr_init_decoder_vbi(cfg: *mut CcxDecoderVbiCfg) -> *mut CcxDecoderVbiCtx {
    let cfg_option = if cfg.is_null() {
        None
    } else {
        Some(unsafe { &*cfg })
    };

    init_decoder_vbi(cfg_option).unwrap_or(std::ptr::null_mut())
}

#[no_mangle]
pub extern "C" fn ccxr_decode_vbi(
    dec_ctx: *mut LibCcDecode,
    field: u8,
    buffer: *mut u8,
    len: usize,
    sub: *mut CcSubtitle,
) -> i32 {
    decode_vbi(dec_ctx, field, buffer, len, sub)
}