use crate::bindings::{encoder_ctx,ccx_s_write};
use lib_ccxr::util::encoding::Encoding;
use std::os::raw::{c_int, c_uint};

/// Rust representation of encoder_ctx
#[derive(Debug)]
pub struct EncoderCtxRust {
    pub srt_counter: u32,
    pub buffer: Option<Vec<u8>>,      // Owned Vec, not slice
    pub encoded_crlf: Option<String>, // Owned String
    pub out: Option<*mut ccx_s_write>,// Single pointer
    pub encoding: Encoding,           // Encoding type
}
pub fn copy_encoder_ctx_c_to_rust(c_ctx: *mut encoder_ctx) -> EncoderCtxRust{
    assert!(!c_ctx.is_null());
    let c = unsafe { &* c_ctx };

    // Copy buffer into Vec<u8>
    let buffer = if !c.buffer.is_null() && c.capacity > 0 {
        let slice = unsafe { std::slice::from_raw_parts(c.buffer, c.capacity as usize) };
        Some(slice.to_vec())
    } else {
        None
    };

    // Copy encoded_crlf into String
    let encoded_crlf = if !c.encoded_crlf.is_null() {
        let bytes = unsafe { std::slice::from_raw_parts(c.encoded_crlf, 16) };
        // trim at first 0
        let len = bytes.iter().position(|&b| b == 0).unwrap_or(16);
        Some(String::from_utf8_lossy(&bytes[..len]).to_string())
    } else {
        None
    };

    // Single output pointer
    let out = if !c.out.is_null() {
        Some(c.out)
    } else {
        None
    };

    EncoderCtxRust {
        srt_counter: c.srt_counter,
        buffer,
        encoded_crlf,
        out,
        encoding: unsafe { Encoding::from_ctype(c.encoding).unwrap_or(Encoding::default()) },
    }
}

/// Copy the Rust encoder context back to the C encoder context
pub unsafe fn copy_encoder_ctx_rust_to_c(rust_ctx: &mut EncoderCtxRust, c_ctx: *mut encoder_ctx) {
    assert!(!c_ctx.is_null());
    let c = &mut *c_ctx;
    
    // Update the srt_counter
    c.srt_counter = rust_ctx.srt_counter;
    
    // Copy buffer back if it exists
    if let Some(ref buffer) = rust_ctx.buffer {
        if !c.buffer.is_null() && c.capacity > 0 {
            let copy_len = std::cmp::min(buffer.len(), c.capacity as usize);
            std::ptr::copy_nonoverlapping(buffer.as_ptr(), c.buffer, copy_len);
        }
    }
    
    // Copy encoded_crlf back if it exists
    if let Some(ref crlf) = rust_ctx.encoded_crlf {
        if !c.encoded_crlf.is_null() {
            let crlf_bytes = crlf.as_bytes();
            let copy_len = std::cmp::min(crlf_bytes.len(), 15); // Leave room for null terminator
            std::ptr::copy_nonoverlapping(crlf_bytes.as_ptr(), c.encoded_crlf, copy_len);
            *c.encoded_crlf.add(copy_len) = 0; // Null terminate
        }
    }
}