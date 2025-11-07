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

    // Create a safe slice from the raw pointer
    let avc_slice = std::slice::from_raw_parts_mut(avcbuf, avcbuflen);

    // Call the safe Rust version
    process_avc(&mut *enc_ctx, &mut *dec_ctx, avc_slice, &mut *sub).unwrap_or_else(|e| {
        info!("Error in process_avc: {:?}", e);
        0 // Return 0 to indicate error
    })
}
