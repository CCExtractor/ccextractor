use crate::bindings::{
    cc_subtitle, ccx_encoding_type_CCX_ENC_UNICODE, eia608_screen, encoder_ctx, subtype_CC_TEXT,
};
use crate::encoder::common::{encode_line, write_raw};
use crate::libccxr_exports::time::ccxr_millis_to_time;
use lib_ccxr::debug;
use lib_ccxr::util::log::DebugMessageFlag;

use std::ffi::CStr;
use std::io;
use std::os::raw::{c_char, c_int, c_void};

// Constants from C code
const ROWS: i32 = 15;
const COLUMNS: i32 = 32;

fn escape_xml(s: &str) -> String {
    let mut out = String::with_capacity(s.len());
    for c in s.chars() {
        match c {
            '&' => out.push_str("&amp;"),
            '<' => out.push_str("&lt;"),
            '>' => out.push_str("&gt;"),
            '"' => out.push_str("&quot;"),
            '\'' => out.push_str("&apos;"),
            _ => out.push(c),
        }
    }
    out
}

/// Write data to file descriptor with retry logic
fn write_wrapped(fd: c_int, buf: &[u8]) -> Result<(), io::Error> {
    let mut remaining = buf.len();
    let mut current_buf = buf.as_ptr();

    while remaining > 0 {
        let written = write_raw(fd, current_buf as *const c_void, remaining);
        if written == -1 {
            return Err(io::Error::last_os_error());
        }
        unsafe {
            current_buf = current_buf.add(written as usize);
        }
        remaining -= written as usize;
    }

    Ok(())
}

/// Internal implementation of write_stringz_as_smptett
fn write_stringz_as_smptett_internal(
    text: &str,
    context: &mut encoder_ctx,
    ms_start: i64,
    ms_end: i64,
) {
    let (mut h1, mut m1, mut s1, mut ms1) = (0, 0, 0, 0);
    let (mut h2, mut m2, mut s2, mut ms2) = (0, 0, 0, 0);

    unsafe {
        ccxr_millis_to_time(ms_start, &mut h1, &mut m1, &mut s1, &mut ms1);
        ccxr_millis_to_time(ms_end - 1, &mut h2, &mut m2, &mut s2, &mut ms2);
    }

    // Convert raw pointer buffer → Rust slice
    let buffer: &mut [u8] =
        unsafe { std::slice::from_raw_parts_mut(context.buffer, context.capacity as usize) };

    let header = format!(
        "<p begin=\"{:02}:{:02}:{:02}.{:03}\" end=\"{:02}:{:02}:{:02}.{:03}\">\r\n",
        h1, m1, s1, ms1, h2, m2, s2, ms2
    );

    if context.encoding != ccx_encoding_type_CCX_ENC_UNICODE {
        debug!(msg_type = DebugMessageFlag::DECODER_608; "\r{}\n", header.trim_end());
    }

    let used = encode_line(context, buffer, header.as_bytes());
    unsafe {
        let out = &*context.out;
        let _ = write_wrapped(out.fh, &buffer[..used as usize]);
    }

    // Process lines - split by \n in the string
    for line in text.split('\n') {
        if !line.is_empty() {
            let escaped = escape_xml(line);
            let used = encode_line(context, buffer, escaped.as_bytes());

            if context.encoding != ccx_encoding_type_CCX_ENC_UNICODE {
                debug!(msg_type = DebugMessageFlag::DECODER_608; "\r{}\n", escaped);
            }

            unsafe {
                let out = &*context.out;
                let _ = write_wrapped(out.fh, &buffer[..used as usize]);

                // Write CRLF
                let crlf_slice = std::slice::from_raw_parts(
                    context.encoded_crlf.as_ptr(),
                    context.encoded_crlf_length as usize,
                );
                let _ = write_wrapped(out.fh, crlf_slice);
            }
        }
    }

    let footer = b"</p>\n";
    let used = encode_line(context, buffer, footer);

    if context.encoding != ccx_encoding_type_CCX_ENC_UNICODE {
        debug!(msg_type = DebugMessageFlag::DECODER_608; "\r</p>\n");
    }

    unsafe {
        let out = &*context.out;
        let _ = write_wrapped(out.fh, &buffer[..used as usize]);
    }
}

/// C-callable wrapper for write_stringz_as_smptett
/// # Safety
/// This function is unsafe because it dereferences raw pointers
#[no_mangle]
pub unsafe extern "C" fn write_stringz_as_smptett(
    text: *const c_char,
    context: *mut encoder_ctx,
    ms_start: i64,
    ms_end: i64,
) {
    if text.is_null() || context.is_null() {
        return;
    }

    let text_str = match CStr::from_ptr(text).to_str() {
        Ok(s) => s,
        Err(_) => return,
    };

    write_stringz_as_smptett_internal(text_str, &mut *context, ms_start, ms_end);
}

/// Write EIA-608 screen buffer as SMPTE-TT
/// # Safety
/// This function is unsafe because it dereferences raw pointers and performs FFI operations
#[no_mangle]
pub unsafe extern "C" fn write_cc_buffer_as_smptett(
    data: *mut eia608_screen,
    context: *mut encoder_ctx,
) -> c_int {
    if data.is_null() || context.is_null() {
        return 0;
    }
    let data = &mut *data;
    let context = &mut *context;

    let (mut h1, mut m1, mut s1, mut ms1) = (0, 0, 0, 0);
    let (mut h2, mut m2, mut s2, mut ms2) = (0, 0, 0, 0);
    let mut wrote_something = 0;

    ccxr_millis_to_time(data.start_time, &mut h1, &mut m1, &mut s1, &mut ms1);
    ccxr_millis_to_time(data.end_time - 1, &mut h2, &mut m2, &mut s2, &mut ms2);

    let buffer: &mut [u8] =
        std::slice::from_raw_parts_mut(context.buffer, context.capacity as usize);

    for row in 0..15 {
        if data.row_used[row] == 0 {
            continue;
        }

        // Calculate row position (10% + row position in 80% of screen)
        let row1 = ((100.0 * row as f32) / (ROWS as f32 / 0.8)) + 10.0;

        // Find first non-space column
        let mut firstcol = -1;
        for column in 0..COLUMNS as usize {
            let ch = data.characters[row][column];
            // Check for non-space character (0x20 is space)
            if ch != 0x20 && ch != 0 {
                firstcol = column as i32;
                break;
            }
        }

        if firstcol < 0 {
            continue;
        }

        // Calculate column position
        let col1 = ((100.0 * firstcol as f32) / (COLUMNS as f32 / 0.8)) + 10.0;

        wrote_something = 1;

        // Write opening tag with positioning
        let opening = format!(
            "      <p begin=\"{:02}:{:02}:{:02}.{:03}\" end=\"{:02}:{:02}:{:02}.{:03}\" tts:origin=\"{:.3}% {:.3}%\">\n        <span>",
            h1, m1, s1, ms1, h2, m2, s2, ms2, col1, row1
        );

        if context.encoding != ccx_encoding_type_CCX_ENC_UNICODE {
            debug!(msg_type = DebugMessageFlag::DECODER_608; "\r{}\n", opening.trim_end());
        }

        let used = encode_line(context, buffer, opening.as_bytes());
        let out = &*context.out;
        let _ = write_wrapped(out.fh, &buffer[..used as usize]);

        // Get the decoded line - store it in context.subline
        extern "C" {
            fn get_decoder_line_encoded(
                ctx: *mut encoder_ctx,
                buf: *mut u8,
                line_num: c_int,
                data: *const eia608_screen,
            ) -> u32;
        }

        get_decoder_line_encoded(
            context as *mut encoder_ctx,
            context.subline,
            row as c_int,
            data as *const eia608_screen,
        );

        // Convert subline to Rust string for processing
        let line_text = {
            let subline_slice = std::slice::from_raw_parts(
                context.subline,
                4096, // Max line length
            );
            let null_pos = subline_slice.iter().position(|&b| b == 0).unwrap_or(4096);
            String::from_utf8_lossy(&subline_slice[..null_pos]).to_string()
        };

        // Process styling (italics, bold, underline, colors)
        let formatted = process_line_styling(&line_text);

        let used = encode_line(context, buffer, formatted.as_bytes());
        let out = &*context.out;
        let _ = write_wrapped(out.fh, &buffer[..used as usize]);

        // Write CRLF
        let crlf_slice = std::slice::from_raw_parts(
            context.encoded_crlf.as_ptr(),
            context.encoded_crlf_length as usize,
        );
        let _ = write_wrapped(out.fh, crlf_slice);

        // Write closing tags
        let closing = b"        <style tts:backgroundColor=\"#000000FF\" tts:fontSize=\"18px\"/></span>\n      </p>\n";
        let used = encode_line(context, buffer, closing);

        if context.encoding != ccx_encoding_type_CCX_ENC_UNICODE {
            debug!(msg_type = DebugMessageFlag::DECODER_608; "\r{}\n", 
                std::str::from_utf8(closing).unwrap_or(""));
        }

        let out = &*context.out;
        let _ = write_wrapped(out.fh, &buffer[..used as usize]);
    }

    wrote_something
}

fn process_line_styling(line: &str) -> String {
    let mut result = String::new();

    // Check for italics
    if let Some(start_pos) = line.find("<i>") {
        if let Some(end_pos) = line.find("</i>") {
            result.push_str(&line[..start_pos]);
            result.push_str("<span>");
            result.push_str(&line[start_pos + 3..end_pos]);
            result.push_str("<style tts:backgroundColor=\"#000000FF\" tts:fontSize=\"18px\" tts:fontStyle=\"italic\"/> </span>");
            result.push_str(&line[end_pos + 4..]);
            return result;
        }
    }

    // Check for bold
    if let Some(start_pos) = line.find("<b>") {
        if let Some(end_pos) = line.find("</b>") {
            result.push_str(&line[..start_pos]);
            result.push_str("<span>");
            result.push_str(&line[start_pos + 3..end_pos]);
            result.push_str("<style tts:backgroundColor=\"#000000FF\" tts:fontSize=\"18px\" tts:fontWeight=\"bold\"/> </span>");
            result.push_str(&line[end_pos + 4..]);
            return result;
        }
    }

    // Check for underline
    if let Some(start_pos) = line.find("<u>") {
        if let Some(end_pos) = line.find("</u>") {
            result.push_str(&line[..start_pos]);
            result.push_str("<span>");
            result.push_str(&line[start_pos + 3..end_pos]);
            result.push_str("<style tts:backgroundColor=\"#000000FF\" tts:fontSize=\"18px\" tts:textDecoration=\"underline\"/> </span>");
            result.push_str(&line[end_pos + 4..]);
            return result;
        }
    }

    // Check for font color
    if let Some(start_pos) = line.find("<font color") {
        if let Some(end_pos) = line.find("</font>") {
            result.push_str(&line[..start_pos]);
            result.push_str("<span>");

            // Extract color code
            if let Some(color_start) = line[start_pos..].find('#') {
                let color_pos = start_pos + color_start;
                if color_pos + 7 <= line.len() {
                    let color_code = &line[color_pos + 1..color_pos + 7];

                    // Find content start (after '>')
                    if let Some(content_start) = line[start_pos..end_pos].find('>') {
                        let content_pos = start_pos + content_start + 1;
                        result.push_str(&line[content_pos..end_pos]);
                        result.push_str(&format!(
                            "<style tts:backgroundColor=\"#FFFF00FF\" tts:color=\"{}\" tts:fontSize=\"18px\"/></span>",
                            color_code
                        ));
                        result.push_str(&line[end_pos + 7..]);
                        return result;
                    }
                }
            }
        }
    }

    // No styling found, return as is
    line.to_string()
}

/// Write subtitle as SMPTE-TT
/// # Safety
/// This function is unsafe because it dereferences raw pointers and performs FFI operations
#[no_mangle]
pub unsafe extern "C" fn write_cc_subtitle_as_smptett(
    sub: *mut cc_subtitle,
    context: *mut encoder_ctx,
) -> c_int {
    if sub.is_null() || context.is_null() {
        return 0;
    }
    let sub = &mut *sub;
    let context = &mut *context;

    let mut current = sub as *mut cc_subtitle;
    let original = current;

    // Process all subtitles in the linked list
    while !current.is_null() {
        let sub_ref = &mut *current;

        // Check if it's CC_TEXT type (subtype_CC_TEXT = 0)
        if sub_ref.type_ == subtype_CC_TEXT {
            if !sub_ref.data.is_null() {
                // Convert C string to Rust string
                let c_str = CStr::from_ptr(sub_ref.data as *const i8);
                if let Ok(text) = c_str.to_str() {
                    write_stringz_as_smptett_internal(
                        text,
                        context,
                        sub_ref.start_time,
                        sub_ref.end_time,
                    );
                }

                // Free the data using C free
                extern "C" {
                    fn free(ptr: *mut c_void);
                }
                free(sub_ref.data);
                sub_ref.data = std::ptr::null_mut();
            }
            sub_ref.nb_data = 0;
        }

        current = sub_ref.next;
    }

    // Free the subtitle list (going backwards to original)
    current = original;
    while !current.is_null() {
        let sub_ref = &mut *current;
        let next = sub_ref.next;

        if current != original {
            extern "C" {
                fn free(ptr: *mut c_void);
            }
            free(current as *mut c_void);
        }

        current = next;
    }

    0
}

/// Write bitmap subtitle as SMPTE-TT
/// # Safety
/// This function is unsafe because it dereferences raw pointers and performs FFI operations
#[no_mangle]
pub unsafe extern "C" fn write_cc_bitmap_as_smptett(
    sub: *mut cc_subtitle,
    _context: *mut encoder_ctx,
) -> c_int {
    if sub.is_null() {
        return 0;
    }
    let sub = &mut *sub;

    #[cfg(feature = "hardsubx_ocr")]
    {
        // Would implement bitmap OCR handling here similar to C code
        // For now, just clean up
        if !sub.data.is_null() {
            extern "C" {
                fn free(ptr: *mut c_void);
            }
            free(sub.data);
            sub.data = std::ptr::null_mut();
        }
        sub.nb_data = 0;
        0
    }

    #[cfg(not(feature = "hardsubx_ocr"))]
    {
        // Without OCR, just clean up and return
        if !sub.data.is_null() {
            extern "C" {
                fn free(ptr: *mut c_void);
            }
            free(sub.data);
            sub.data = std::ptr::null_mut();
        }
        sub.nb_data = 0;
        0
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_smptett_escape_basic() {
        assert_eq!(escape_xml("a & b < c"), "a &amp; b &lt; c");
        assert_eq!(
            escape_xml("\"quote\" 'apos' & < >"),
            "&quot;quote&quot; &apos;apos&apos; &amp; &lt; &gt;"
        );
    }

    #[test]
    fn test_styling_italic() {
        let input = "This is <i>italic</i> text";
        let output = process_line_styling(input);
        assert!(output.contains("<span>"));
        assert!(output.contains("fontStyle=\"italic\""));
    }

    #[test]
    fn test_styling_bold() {
        let input = "This is <b>bold</b> text";
        let output = process_line_styling(input);
        assert!(output.contains("fontWeight=\"bold\""));
    }

    #[test]
    fn test_no_styling() {
        let input = "Plain text";
        let output = process_line_styling(input);
        assert_eq!(output, "Plain text");
    }
}
