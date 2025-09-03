use crate::utils::write_wrapper_os;
use crate::common::encode_line;
use crate::libccxr_exports::time::ccxr_millis_to_time;
use std::ffi::CString;
use std::io;
use std::os::raw::{c_int, c_void};
use crate::encoder::context::EncoderCtx; // assuming you have this
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

/// Rewrite of write_stringz_srt_r using EncoderCtxRust
#[no_mangle]
pub unsafe extern "C" fn ccxr_write_stringz_srt(
    string: &str,
    context_ptr: &mut crate::EncoderCtxRust,
    ms_start: i64,
    ms_end: i64,
) -> Result<(), io::Error> {
    if string.is_empty() {
        return Ok(());
    }
    let mut context = copy_encoder_ctx_c_to_rust(context_ptr);
    // Convert times
    let (h1, m1, s1, ms1) = ccxr_millis_to_time(ms_start);
    let (h2, m2, s2, ms2) = ccxr_millis_to_time(ms_end - 1);

    // Increment counter
    context.srt_counter += 1;

    // Get encoded CRLF string
    let crlf = context.encoded_crlf.as_deref().unwrap_or("\r\n");

    // Get file handle
    let fh = get_out_fh(context.out)?;

    // Timeline counter line
    let timeline = format!("{}{}", context.srt_counter, crlf);
    let used = encode_line(&timeline, &mut context.buffer)?;
    write_wrapper_os(fh, &context.buffer[..used])?;

    // Timeline timecode line
    let timeline = format!(
        "{:02}:{:02}:{:02},{:03} --> {:02}:{:02}:{:02},{:03}{}",
        h1, m1, s1, ms1,
        h2, m2, s2, ms2,
        crlf
    );
    let used = encode_line(&timeline, &mut context.buffer)?;
    write_wrapper_os(fh, &context.buffer[..used])?;

    // --- Handle the text itself ---
    let mut unescaped = Vec::with_capacity(string.len() + 1);

    let mut chars = string.chars().peekable();
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
                let used = encode_line(&line, &mut context.buffer)?;
                write_wrapper_os(fh, &context.buffer[..used])?;
                write_wrapper_os(fh, crlf.as_bytes())?;
            }
            pos += end + 1;
        } else {
            break;
        }
    }

    // Final CRLF
    write_wrapper_os(fh, crlf.as_bytes())?;

    Ok(())
}
