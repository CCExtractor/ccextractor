extern crate libc;

use lib_ccxr::isdbs::isdbd::*;
use lib_ccxr::isdbs::isdbs::*;

use std::mem;
use std::os::raw::{c_int, c_uint, c_void};
use std::ptr;

#[no_mangle]
pub extern "C" fn ccxr_isdb_set_global_time(dec_ctx: *mut LibCcDecode, timestamp: u64) -> c_int {
    unsafe {
        let ctx = (*dec_ctx).private_data as *mut ISDBSubContext;
        if !ctx.is_null() {
            isdb_set_global_time(&mut *ctx, timestamp);
            0 // return 0 if everything is CCX_OK
        } else {
            -1 // return an error code if ctx is null
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn ccxr_delete_isdb_decoder(isdb_ctx: *mut *mut c_void) {
    let ctx = *isdb_ctx as *mut ISDBSubContext;
    delete_isdb_decoder(ctx);
    freep(isdb_ctx);
}

#[no_mangle]
pub extern "C" fn ccxr_init_isdb_decoder() -> *mut c_void {
    unsafe {
        let ctx = libc::malloc(mem::size_of::<ISDBSubContext>()) as *mut ISDBSubContext;
        if ctx.is_null() {
            return ptr::null_mut();
        }

        libc::memset(ctx as *mut c_void, 0, mem::size_of::<ISDBSubContext>());

        init_list_head(&mut (*ctx).text_list_head);
        init_list_head(&mut (*ctx).buffered_text);
        (*ctx).prev_timestamp = c_uint::MAX as u64;

        (*ctx).current_state.clut_high_idx = 0;
        (*ctx).current_state.rollup_mode = 0;
        init_layout(&mut (*ctx).current_state.layout_state);

        ctx as *mut c_void
    }
}

#[no_mangle]
pub extern "C" fn ccxr_isdb_parse_data_group(
    codec_ctx: *mut c_void,
    buf: *const u8,
    size: i32,
    sub: *mut CcSubtitle,
) -> i32 {
    let buf_slice = unsafe { std::slice::from_raw_parts(buf, size as usize) };
    isdb_parse_data_group(codec_ctx, buf_slice, unsafe { &mut *sub })
}

#[no_mangle]
pub extern "C" fn ccxr_isdbsub_decode(
    dec_ctx: *mut LibCcDecode,
    buf: *const u8,
    buf_size: usize,
    sub: *mut CcSubtitle,
) -> i32 {
    let buf_slice = unsafe { std::slice::from_raw_parts(buf, buf_size) };
    isdbsub_decode(unsafe { &mut *dec_ctx }, buf_slice, unsafe { &mut *sub })
}
