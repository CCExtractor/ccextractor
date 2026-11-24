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

    // dec_ctx and sub should never be NULL in normal operation, but check defensively
    if dec_ctx.is_null() || sub.is_null() {
        info!("Warning: dec_ctx or sub is NULL in ccxr_process_avc");
        return avcbuflen;
    }

    // Create a safe slice from the raw pointer
    let avc_slice = std::slice::from_raw_parts_mut(avcbuf, avcbuflen);

    // Call the safe Rust version
    process_avc(enc_ctx, &mut *dec_ctx, avc_slice, &mut *sub).unwrap_or_else(|e| {
        info!("Error in process_avc: {:?}", e);
        0 // Return 0 to indicate error
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::bindings::avc_ctx;
    use lib_ccxr::util::log::{
        set_logger, CCExtractorLogger, DebugMessageFlag, DebugMessageMask, OutputTarget,
    };
    use std::ptr::null_mut;

    // With --out=report no encoder exists, so C passes a NULL enc_ctx.
    // The stream must still be parsed: the report's EIA-608/CEA-708
    // lines come from what the decoder saw.
    #[test]
    fn test_null_enc_ctx_still_parses() {
        // Fails if another test set it first, which is fine.
        set_logger(CCExtractorLogger::new(
            OutputTarget::Stdout,
            DebugMessageMask::new(DebugMessageFlag::VERBOSE, DebugMessageFlag::VERBOSE),
            false,
        ))
        .ok();

        // H.264 SPS: High profile, level 3.1, 1280x720
        let mut sps = [
            0x00, 0x00, 0x00, 0x01, 0x67, 0x64, 0x00, 0x1f, 0xac, 0xd9, 0x40, 0x50, 0x05, 0xbb,
            0x01, 0x10, 0x00, 0x00, 0x03, 0x00, 0x10, 0x00, 0x00, 0x03, 0x03, 0xc0, 0xf1, 0x83,
            0x19, 0x60,
        ];

        unsafe {
            let mut avc: avc_ctx = std::mem::zeroed();
            let mut dec_ctx: lib_cc_decode = std::mem::zeroed();
            dec_ctx.avc_ctx = &mut avc;
            let mut sub: cc_subtitle = std::mem::zeroed();

            ccxr_process_avc(
                null_mut(),
                &mut dec_ctx,
                sps.as_mut_ptr(),
                sps.len(),
                &mut sub,
            );

            assert_eq!(avc.got_seq_para, 1);
        }
    }
}
