//! C-callable FFI exports for the FFmpeg MP4 demuxer.
//!
//! These `#[no_mangle] extern "C"` functions are called from the C side
//! (ccextractor.c) when `ENABLE_FFMPEG_MP4` is defined.

use std::ffi::CStr;
use std::os::raw::{c_char, c_int};

use crate::bindings::{cc_subtitle, lib_ccx_ctx};
use crate::demuxer::mp4;

/// Process an MP4 file using FFmpeg, extracting closed captions.
///
/// This is the Rust replacement for `processmp4()` when building with
/// `ENABLE_FFMPEG_MP4`. Called from C code in ccextractor.c.
///
/// # Safety
/// - `ctx` must be a valid pointer to an initialized `lib_ccx_ctx`
/// - `file` must be a valid null-terminated C string
#[no_mangle]
pub unsafe extern "C" fn ccxr_processmp4(ctx: *mut lib_ccx_ctx, file: *const c_char) -> c_int {
    if ctx.is_null() || file.is_null() {
        return -1;
    }

    let path = CStr::from_ptr(file);
    let mut sub: cc_subtitle = std::mem::zeroed();

    mp4::processmp4_rust(ctx, path, &mut sub)
}

/// Dump chapters from an MP4 file using FFmpeg.
///
/// This is the Rust replacement for `dumpchapters()` when building with
/// `ENABLE_FFMPEG_MP4`. Called from C code in ccextractor.c.
///
/// # Safety
/// - `ctx` must be a valid pointer to an initialized `lib_ccx_ctx`
/// - `file` must be a valid null-terminated C string
#[no_mangle]
pub unsafe extern "C" fn ccxr_dumpchapters(ctx: *mut lib_ccx_ctx, file: *const c_char) -> c_int {
    if ctx.is_null() || file.is_null() {
        return 5;
    }

    let path = CStr::from_ptr(file);

    mp4::dumpchapters_rust(ctx, path)
}
