use crate::utils::write_wrapper_os;
use crate::encoder::common::encode_line;
use crate::libccxr_exports::time::ccxr_millis_to_time;
use crate::libccxr_exports::encoder_ctx::{copy_encoder_ctx_c_to_rust, copy_encoder_ctx_rust_to_c, EncoderCtxRust};
use std::io;
use std::os::raw::c_int;
use crate::bindings::ccx_s_write;

/// Safe wrapper to get the file handle from EncoderCtxRust
fn get_out_fh(out: Option<*mut ccx_s_write>) -> io::Result<i32> {
    if let Some(ptr) = out {
        if ptr.is_null() {
            return Err(io::Error::new(io::ErrorKind::Other, "Null out pointer"));
        }
        let out_ref = unsafe { &mut *ptr };
        Ok(out_ref.fh) // assuming fh is i32 in C
    } else {
        Err(io::Error::new(io::ErrorKind::Other, "No out pointer set"))
    }
}

/// Safe wrapper for the existing encode_line function that works with EncoderCtxRust
fn safe_encode_line(
    rust_ctx: &mut EncoderCtxRust,
    c_ctx: *mut crate::bindings::encoder_ctx,
    text: &[u8],
) -> Result<usize, io::Error> {
    if rust_ctx.buffer.is_none() {
        return Err(io::Error::new(io::ErrorKind::Other, "No buffer available"));
    }
    
    // Temporarily copy Rust context to C context for encoding
    unsafe {
        copy_encoder_ctx_rust_to_c(rust_ctx, c_ctx);
    }
    
    let c_ctx_ref = unsafe { &mut *c_ctx };
    let buffer_slice = unsafe {
        std::slice::from_raw_parts_mut(c_ctx_ref.buffer, c_ctx_ref.capacity as usize)
    };
    
    // Use the existing encode_line function from common.rs
    let used = encode_line(c_ctx_ref, buffer_slice, text);
    
    // Copy the encoded buffer back to Rust context
    if let Some(ref mut buffer) = rust_ctx.buffer {
        let copy_len = std::cmp::min(used as usize, buffer.len());
        buffer[..copy_len].copy_from_slice(&buffer_slice[..copy_len]);
    }
    
    Ok(used as usize)
}

/// Rewrite of write_stringz_srt_r using EncoderCtxRust
#[no_mangle]
pub unsafe extern "C" fn ccxr_write_stringz_srt(
    context: *mut crate::bindings::encoder_ctx,
    string: *const std::os::raw::c_char,
    ms_start: i64,
    ms_end: i64,
) -> c_int {
    if context.is_null() || string.is_null() {
        return 0;
    }

    let string_cstr = match std::ffi::CStr::from_ptr(string).to_str() {
        Ok(s) => s,
        Err(_) => return 0,
    };

    if string_cstr.is_empty() {
        return 0;
    }

    // Copy C context to safe Rust context
    let mut rust_context = copy_encoder_ctx_c_to_rust(context);
    
    // Convert times
    let mut h1 = 0u32;
    let mut m1 = 0u32;
    let mut s1 = 0u32;
    let mut ms1 = 0u32;
    unsafe {
        ccxr_millis_to_time(ms_start, &mut h1, &mut m1, &mut s1, &mut ms1);
    }
    
    let mut h2 = 0u32;
    let mut m2 = 0u32;
    let mut s2 = 0u32;
    let mut ms2 = 0u32;
    unsafe {
        ccxr_millis_to_time(ms_end - 1, &mut h2, &mut m2, &mut s2, &mut ms2);
    }

    // Increment counter
    rust_context.srt_counter += 1;

    // Get encoded CRLF string
    let crlf = rust_context.encoded_crlf.as_deref().unwrap_or("\r\n").to_string();

    // Get file handle
    let fh = match get_out_fh(rust_context.out) {
        Ok(fh) => fh,
        Err(_) => return 0,
    };

    // Timeline counter line
    let timeline = format!("{}{}", rust_context.srt_counter, crlf);
    let used = match safe_encode_line(&mut rust_context, context, timeline.as_bytes()) {
        Ok(u) => u,
        Err(_) => return 0,
    };
    if write_wrapper_os(fh, &rust_context.buffer.as_ref().unwrap()[..used]).is_err() {
        return 0;
    }

    // Timeline timecode line
    let timeline = format!(
        "{:02}:{:02}:{:02},{:03} --> {:02}:{:02}:{:02},{:03}{}",
        h1, m1, s1, ms1,
        h2, m2, s2, ms2,
        crlf
    );
    let used = match safe_encode_line(&mut rust_context, context, timeline.as_bytes()) {
        Ok(u) => u,
        Err(_) => return 0,
    };
    if write_wrapper_os(fh, &rust_context.buffer.as_ref().unwrap()[..used]).is_err() {
        return 0;
    }

    // --- Handle the text itself ---
    let mut unescaped = Vec::with_capacity(string_cstr.len() + 1);

    let mut chars = string_cstr.chars().peekable();
    while let Some(c) = chars.next() {
        if c == '\\' && chars.peek() == Some(&'n') {
            unescaped.push(0u8);
            chars.next();
        } else {
            unescaped.extend(c.to_string().as_bytes());
        }
    }
    unescaped.push(0);

    let mut pos = 0;
    while pos < unescaped.len() {
        if let Some(end) = unescaped[pos..].iter().position(|&b| b == 0) {
            let slice = &unescaped[pos..pos + end];
            if !slice.is_empty() {
                let line = String::from_utf8_lossy(slice);
                let used = match safe_encode_line(&mut rust_context, context, line.as_bytes()) {
                    Ok(u) => u,
                    Err(_) => return 0,
                };
                if write_wrapper_os(fh, &rust_context.buffer.as_ref().unwrap()[..used]).is_err() {
                    return 0;
                }
                if write_wrapper_os(fh, crlf.as_bytes()).is_err() {
                    return 0;
                }
            }
            pos += end + 1;
        } else {
            break;
        }
    }

    // Final CRLF
    if write_wrapper_os(fh, crlf.as_bytes()).is_err() {
        return 0;
    }

    // Copy the updated Rust context back to C context
    copy_encoder_ctx_rust_to_c(&mut rust_context, context);

    1 // Success
}
