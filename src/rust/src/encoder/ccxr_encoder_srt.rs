use crate::encoder::g608::write_wrapped;
use crate::encoder::common::encode_line;
use crate::libccxr_exports::time::ccxr_millis_to_time;
//the ccxr_encoder_ctx is called in the function copy_encoder_ctx_c_to_rust
use crate::libccxr_exports::encoder_ctx::{copy_encoder_ctx_c_to_rust, copy_encoder_ctx_rust_to_c};
use std::os::raw::{c_int, c_char};
use crate::bindings::{cc_subtitle, eia608_screen, encoder_ctx};

/// Rewrite of write_stringz_srt_r using ccxr_encoder_ctx
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
    rust_context.srt_counter += 1;
    let crlf = rust_context.encoded_crlf.as_deref().unwrap_or("\r\n").to_string();
    let fh = if let Some(ref out_vec) = rust_context.out {
        if out_vec.is_empty() {
            return 0;
        }
        out_vec[0].fh
    } else {
        return 0;
    };
    let timeline = format!("{}{}", rust_context.srt_counter, crlf);
    let timeline_bytes = timeline.as_bytes();
    let (_used, buffer_slice) = if let Some(ref mut buffer) = rust_context.buffer {
        let used = encode_line(rust_context.encoding, buffer, timeline_bytes);
        (used, &buffer[..used])
    } else {
        return 0;
    };
    if write_wrapped(fh, buffer_slice).is_err() {
        return 0;
    }
    let timeline = format!(
        "{:02}:{:02}:{:02},{:03} --> {:02}:{:02}:{:02},{:03}{}",
        h1, m1, s1, ms1,
        h2, m2, s2, ms2,
        crlf
    );
    let timeline_bytes = timeline.as_bytes();
    let (_used, buffer_slice) = if let Some(ref mut buffer) = rust_context.buffer {
        let used = encode_line(rust_context.encoding, buffer, timeline_bytes);
        (used, &buffer[..used])
    } else {
        return 0;
    };
    if write_wrapped(fh, buffer_slice).is_err() {
        return 0;
    }
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
                        let (_used, buffer_slice) = if let Some(ref mut buffer) = rust_context.buffer {
                            let used = encode_line(rust_context.encoding, buffer, line.as_bytes());
                            (used, &buffer[..used])
                        } else {
                            return 0;
                        };
                        if write_wrapped(fh, buffer_slice).is_err() {
                            return 0;
                        }
                        if write_wrapped(fh, crlf.as_bytes()).is_err() {
                            return 0;
                        }
                    }
            pos += end + 1;
        } else {
            break;
        }
    }
    if write_wrapped(fh, crlf.as_bytes()).is_err() {
        return 0;
    }
    copy_encoder_ctx_rust_to_c(&mut rust_context, context);

    1
}

#[no_mangle]
pub unsafe extern "C" fn ccxr_write_cc_subtitle_as_srt(
    sub: *mut cc_subtitle,
    context: *mut encoder_ctx,
) -> c_int {
    if sub.is_null() || context.is_null() {
        return 0;
    }

    let mut ret = 0;
    let osub = sub;
    let mut lsub = sub; 
    let mut current_sub = sub;
    while !current_sub.is_null() {
        let sub_ref = &mut *current_sub;
        const CC_TEXT: u32 = 0;
        if sub_ref.type_ == CC_TEXT {
            if !sub_ref.data.is_null() {
                let write_result = ccxr_write_stringz_srt(
                    context,
                    sub_ref.data as *const c_char,
                    sub_ref.start_time,
                    sub_ref.end_time,
                );

                extern "C" {
                    fn freep(ptr: *mut *mut std::ffi::c_void);
                }
                freep(&mut sub_ref.data);
                sub_ref.nb_data = 0;
                if write_result > 0 {
                    ret = 1;
                }
            }
        }
        lsub = current_sub;
        current_sub = sub_ref.next;
    }
    extern "C" {
        fn freep(ptr: *mut *mut std::ffi::c_void);
    }   
    while lsub != osub {
        if lsub.is_null() {
            break;
        }
        current_sub = (*lsub).prev;
        freep(&mut (lsub as *mut std::ffi::c_void));
        lsub = current_sub;
    }
    ret
}

/// This is a port of write_cc_buffer_as_srt from C
/// Uses Rust ccxr_encoder_ctx for all operations
#[no_mangle]
pub unsafe extern "C" fn ccxr_write_cc_buffer_as_srt(
    data: *mut eia608_screen,
    context: *mut encoder_ctx,
) -> c_int {
    if data.is_null() || context.is_null() {
        return 0;
    }

    let screen_data = &*data;
    
    // Convert C context to Rust context
    let mut rust_ctx = copy_encoder_ctx_c_to_rust(context);
    
    // Check if buffer is empty
    let mut empty_buf = true;
    for i in 0..15 {
        if screen_data.row_used[i] != 0 {
            empty_buf = false;
            break;
        }
    }
    if empty_buf {
        return 0;
    } 
    let mut h1 = 0u32;
    let mut m1 = 0u32;
    let mut s1 = 0u32;
    let mut ms1 = 0u32;
    ccxr_millis_to_time(screen_data.start_time, &mut h1, &mut m1, &mut s1, &mut ms1);
    
    let mut h2 = 0u32;
    let mut m2 = 0u32;
    let mut s2 = 0u32;
    let mut ms2 = 0u32;
    ccxr_millis_to_time(screen_data.end_time - 1, &mut h2, &mut m2, &mut s2, &mut ms2);
    rust_ctx.srt_counter += 1;
    let crlf = rust_ctx.encoded_crlf.as_deref().unwrap_or("\r\n").to_string();
    let fh = if let Some(ref out_vec) = rust_ctx.out {
        if out_vec.is_empty() {
            return 0;
        }
        out_vec[0].fh 
    } else {
        return 0;
    };
    let timeline = format!("{}{}", rust_ctx.srt_counter, crlf);
    if let Some(ref mut buffer) = rust_ctx.buffer {
        let encoding = rust_ctx.encoding;
        let used = encode_line(encoding, buffer, timeline.as_bytes());
        let _ = write_wrapped(fh, &buffer[..used]);
    }
    let timeline = format!(
        "{:02}:{:02}:{:02},{:03} --> {:02}:{:02}:{:02},{:03}{}",
        h1, m1, s1, ms1, h2, m2, s2, ms2, crlf
    );
    if let Some(ref mut buffer) = rust_ctx.buffer {
        let encoding = rust_ctx.encoding;
        let used = encode_line(encoding, buffer, timeline.as_bytes());
        let _ = write_wrapped(fh, &buffer[..used]);
    }
    
    let mut wrote_something = false;
    
    // Write each row that has content
    // We still need to call C function get_decoder_line_encoded for now
    // but we copy the updated context back first
    copy_encoder_ctx_rust_to_c(&mut rust_ctx, context);
    
    extern "C" {
        fn get_decoder_line_encoded(
            context: *mut encoder_ctx,
            buffer: *mut u8,
            row: i32,
            data: *mut eia608_screen,
        ) -> i32;
    }
    
    for i in 0..15 {
        if screen_data.row_used[i] != 0 {
            let length = get_decoder_line_encoded(
                context,
                (*context).subline,
                i as i32,
                data,
            );
            
            if length > 0 {
                let line_slice = std::slice::from_raw_parts((*context).subline, length as usize);
                let _ = write_wrapped(fh, line_slice);
                let _ = write_wrapped(fh, crlf.as_bytes());
                wrote_something = true;
            }
        }
    }
    let _ = write_wrapped(fh, crlf.as_bytes());
    let rust_ctx = copy_encoder_ctx_c_to_rust(context);
    copy_encoder_ctx_rust_to_c(&mut rust_ctx.clone(), context);
    
    if wrote_something { 1 } else { 0 }
}
