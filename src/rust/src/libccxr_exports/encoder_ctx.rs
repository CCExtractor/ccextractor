use crate::bindings::{encoder_ctx,ccx_s_write};
use std::os::raw::{c_int, c_uint};

/// Rust representation of encoder_ctx
#[derive(Debug)]
pub struct EncoderCtxRust {
    pub srt_counter: u32,
    pub buffer: Option<Vec<u8>>,      // Owned Vec, not slice
    pub encoded_crlf: Option<String>, // Owned String
    pub out: Option<*mut ccx_s_write>,// Single pointer
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
    }
}