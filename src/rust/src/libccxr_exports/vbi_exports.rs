use lib_ccxr::decoder_vbi::exit_codes::CCX_EINVAL;
use lib_ccxr::decoder_vbi::functions_vbi_main::*;
use lib_ccxr::decoder_vbi::structs_ccdecode::LibCcDecode;
use lib_ccxr::decoder_vbi::structs_isdb::CcxDecoderVbiCtx;
use lib_ccxr::decoder_vbi::structs_vbi_decode::CcxDecoderVbiCfg;
use lib_ccxr::decoder_vbi::structs_xds::CcSubtitle;

use std::ptr;
use std::slice;

/// Deletes a VBI decoder context.
///
/// # Safety
/// - `arg` must be a valid, non‐null pointer to a `*mut CcxDecoderVbiCtx` returned by `ccxr_init_decoder_vbi`.
/// - The pointer `*arg` must not have been freed already.
/// - After this call, `*arg` is set to null to prevent double‐free.
#[no_mangle]
pub extern "C" fn ccxr_delete_decoder_vbi(arg: *mut *mut CcxDecoderVbiCtx) {
    if !arg.is_null() {
        unsafe {
            let mut arg_option = Some(*arg);
            delete_decoder_vbi(&mut arg_option);
            *arg = arg_option.unwrap_or(std::ptr::null_mut());
        }
    }
}

/// Initializes a VBI decoder context.
///
/// # Safety
/// - `cfg` may be null, in which case defaults are used.
/// - If non‐null, `cfg` must point to a valid `CcxDecoderVbiCfg`.
/// - The returned pointer must be freed with `ccxr_delete_decoder_vbi`.
#[no_mangle]
pub extern "C" fn ccxr_init_decoder_vbi(cfg: *const CcxDecoderVbiCfg) -> *mut CcxDecoderVbiCtx {
    let opt = if cfg.is_null() {
        None
    } else {
        Some(unsafe { &*cfg })
    };
    init_decoder_vbi(opt)
        .map(|b| Box::into_raw(b))
        .unwrap_or(ptr::null_mut())
}

/// Feeds raw VBI data into the decoder and produces subtitles.
///
/// # Safety
/// - `ctx` must be a valid, non‐null `*mut LibCcDecode` returned by `ccxr_init_decoder_vbi`.
/// - `buf` must point to at least `len` bytes of valid memory.
/// - `sub` must be a valid, non‐null pointer to a `CcSubtitle` struct for output.
/// - Caller must ensure thread‑safety when reusing the same context.
#[no_mangle]
pub extern "C" fn ccxr_decode_vbi(
    ctx: *mut LibCcDecode,
    field: u8,
    buf: *const u8,
    len: usize,
    sub: *mut CcSubtitle,
) -> i64 {
    if ctx.is_null() || buf.is_null() || sub.is_null() {
        return CCX_EINVAL;
    }
    let dec = unsafe { &mut *ctx };
    let data = unsafe { slice::from_raw_parts(buf, len) };
    let sub = unsafe { &mut *sub };
    decode_vbi(dec, field, data, sub)
}
