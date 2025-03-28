use lib_ccxr::decoder_isdb::exit_codes::*;
use lib_ccxr::decoder_isdb::functions_isdb::*;
use lib_ccxr::decoder_isdb::structs_ccdecode::*;
use lib_ccxr::decoder_isdb::structs_isdb::*;
use lib_ccxr::decoder_isdb::structs_xds::*;

/// # Safety
///
/// - The caller must ensure that `isdb_ctx` is a valid, non-null pointer to a mutable `Option<Box<ISDBSubContext>>`.
/// - The function dereferences `isdb_ctx` and calls `as_mut()` on it, so passing an invalid or null pointer will result in undefined behavior.
/// - After calling this function, the memory associated with the `ISDBSubContext` may be freed, so the caller must not use the pointer again.
#[no_mangle]
pub unsafe extern "C" fn ccxr_delete_isdb_decoder(isdb_ctx: *mut Option<Box<ISDBSubContext>>) {
    if let Some(ctx) = unsafe { isdb_ctx.as_mut() } {
        delete_isdb_decoder(ctx);
    }
}

/// # Safety
///
/// - The caller is responsible for managing the lifetime of the returned pointer.
/// - The returned pointer must eventually be passed to a function that properly deallocates it (e.g., `ccxr_delete_isdb_decoder`).
/// - Using the pointer after it has been deallocated will result in undefined behavior.
#[no_mangle]
pub extern "C" fn ccxr_init_isdb_decoder() -> *mut ISDBSubContext {
    if let Some(ctx) = init_isdb_decoder() {
        Box::into_raw(ctx)
    } else {
        std::ptr::null_mut()
    }
}

/// # Safety
///
/// - The caller must ensure that `codec_ctx` is a valid, non-null pointer to an `ISDBSubContext`.
/// - The caller must ensure that `buf` points to a valid, null-terminated buffer of `u8` values.
/// - The caller must ensure that `sub` is a valid, non-null pointer to a `CcSubtitle`.
/// - Passing invalid or null pointers to any of these parameters will result in undefined behavior.
#[no_mangle]
pub unsafe extern "C" fn ccxr_isdb_parse_data_group(
    codec_ctx: *mut ISDBSubContext,
    buf: *const u8,
    sub: *mut CcSubtitle,
) -> i32 {
    if let (Some(codec_ctx), Some(sub)) = (unsafe { codec_ctx.as_mut() }, unsafe { sub.as_mut() }) {
        let mut len = 0;
        while unsafe { *buf.add(len) } != 0 {
            len += 1;
        }
        let buf_slice = unsafe { std::slice::from_raw_parts(buf, len) };
        isdb_parse_data_group(codec_ctx, buf_slice, sub)
    } else {
        -1
    }
}

/// # Safety
///
/// - The caller must ensure that `dec_ctx` is a valid, non-null pointer to a `LibCcDecode`.
/// - The caller must ensure that `buf` points to a valid buffer of at least `buf_len` bytes.
/// - The caller must ensure that `sub` is a valid, non-null pointer to a `CcSubtitle`.
/// - Passing invalid or null pointers to any of these parameters will result in undefined behavior.
#[no_mangle]
pub unsafe extern "C" fn ccxr_isdbsub_decode(
    dec_ctx: *mut LibCcDecode,
    buf: *const u8,
    buf_len: usize,
    sub: *mut CcSubtitle,
) -> i32 {
    if let (Some(dec_ctx), Some(sub)) = (unsafe { dec_ctx.as_mut() }, unsafe { sub.as_mut() }) {
        let buf_slice = unsafe { std::slice::from_raw_parts(buf, buf_len) };
        isdbsub_decode(dec_ctx, buf_slice, sub)
    } else {
        -1
    }
}

/// # Safety
///
/// - The caller must ensure that `dec_ctx` is a valid, non-null pointer to a `LibCcDecode`.
/// - The `private_data` field of `LibCcDecode` must point to a valid `ISDBSubContext`.
/// - Passing an invalid or null pointer, or an invalid `private_data` field, will result in undefined behavior.
#[no_mangle]
pub unsafe extern "C" fn ccxr_isdb_set_global_time(
    dec_ctx: *mut LibCcDecode,
    timestamp: u64,
) -> i32 {
    if let Some(ctx) =
        unsafe { (dec_ctx.as_mut()).and_then(|d| (d.private_data as *mut ISDBSubContext).as_mut()) }
    {
        isdb_set_global_time(ctx, timestamp)
    } else {
        CCX_EINVAL as i32
    }
}
