use crate::avc::core::*;
use crate::bindings::{cc_subtitle, encoder_ctx, lib_cc_decode};
use lib_ccxr::info;

pub mod common_types;
pub mod core;
pub mod nal;
pub mod sei;

/// # Safety
/// This function is unsafe because it dereferences raw pointers from C and calls `process_avc`.
#[no_mangle]
pub unsafe extern "C" fn ccxr_process_avc(
    enc_ctx: *mut encoder_ctx,
    dec_ctx: *mut lib_cc_decode,
    avcbuf: *mut u8,
    avcbuflen: usize,
    sub: *mut cc_subtitle,
) -> usize {
    if avcbuf.is_null() || avcbuflen == 0 {
        return 0;
    }

    // In report-only mode (-out=report), enc_ctx is NULL because no encoder is created.
    // Skip AVC processing in this case since we can't output captions without an encoder.
    // Return the full buffer length to indicate we've "consumed" the data.
    if enc_ctx.is_null() {
        return avcbuflen;
    }

    // dec_ctx and sub should never be NULL in normal operation, but check defensively
    if dec_ctx.is_null() || sub.is_null() {
        info!("Warning: dec_ctx or sub is NULL in ccxr_process_avc");
        return avcbuflen;
    }

    // Create a safe slice from the raw pointer
    let avc_slice = std::slice::from_raw_parts_mut(avcbuf, avcbuflen);

    // Call the safe Rust version
    process_avc(&mut *enc_ctx, &mut *dec_ctx, avc_slice, &mut *sub).unwrap_or_else(|e| {
        info!("Error in process_avc: {:?}", e);
        0 // Return 0 to indicate error
    })
}
