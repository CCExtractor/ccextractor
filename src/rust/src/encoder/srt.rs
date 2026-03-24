use crate::bindings::{ccx_encoding_type_CCX_ENC_UNICODE, eia608_screen, encoder_ctx};
use crate::encoder::common::encode_line;
use crate::encoder::g608::write_wrapped;
use crate::libccxr_exports::time::ccxr_millis_to_time;
use lib_ccxr::common::CCX_DECODER_608_SCREEN_WIDTH;
use lib_ccxr::debug;
use lib_ccxr::util::log::DebugMessageFlag;
use std::os::raw::c_int;

extern "C" {
    fn get_decoder_line_encoded(
        ctx: *mut encoder_ctx,
        buffer: *mut std::os::raw::c_uchar,
        line_num: c_int,
        data: *const eia608_screen,
    ) -> std::os::raw::c_uint;
    fn get_teletext_output(
        ctx: *mut encoder_ctx,
        teletext_page: u16,
    ) -> *mut crate::bindings::ccx_s_write;
    fn get_teletext_srt_counter(
        ctx: *mut encoder_ctx,
        teletext_page: u16,
    ) -> *mut std::os::raw::c_uint;
}

/// Helper: copy CRLF bytes from context so we don't hold a borrow.
unsafe fn copy_crlf(ctx: &encoder_ctx) -> Vec<u8> {
    let len = ctx.encoded_crlf_length as usize;
    let slice = std::slice::from_raw_parts(ctx.encoded_crlf.as_ptr(), len);
    slice.to_vec()
}

/// Helper: encode into a fresh Vec, avoiding borrow conflicts with ctx.buffer.
unsafe fn encode_to_vec(ctx: &mut encoder_ctx, text: &[u8]) -> Vec<u8> {
    let cap = ctx.capacity as usize;
    let buf = std::slice::from_raw_parts_mut(ctx.buffer, cap);
    let used = encode_line(ctx, buf, text) as usize;
    buf[..used].to_vec()
}

/// Core SRT writer — writes a single subtitle entry to a specific output.
///
/// # Safety
/// Accesses raw pointers from encoder context.
pub unsafe fn write_stringz_as_srt_to_output(
    string: *const i8,
    context: &mut encoder_ctx,
    ms_start: i64,
    ms_end: i64,
    out_fh: c_int,
    srt_counter: *mut u32,
) -> c_int {
    if string.is_null() {
        return 0;
    }
    let c_str = std::ffi::CStr::from_ptr(string);
    let input = match c_str.to_str() {
        Ok(s) if !s.is_empty() => s,
        _ => return 0,
    };

    let mut h1: u32 = 0;
    let mut m1: u32 = 0;
    let mut s1: u32 = 0;
    let mut ms1: u32 = 0;
    let mut h2: u32 = 0;
    let mut m2: u32 = 0;
    let mut s2: u32 = 0;
    let mut ms2: u32 = 0;
    ccxr_millis_to_time(ms_start, &mut h1, &mut m1, &mut s1, &mut ms1);
    ccxr_millis_to_time(ms_end - 1, &mut h2, &mut m2, &mut s2, &mut ms2);

    let crlf = copy_crlf(context);
    let crlf_str = std::str::from_utf8(&crlf).unwrap_or("\r\n");

    // Write counter
    *srt_counter += 1;
    let counter_line = format!("{}{}", *srt_counter, crlf_str);
    let encoded = encode_to_vec(context, counter_line.as_bytes());
    if write_wrapped(out_fh, &encoded).is_err() {
        return -1;
    }

    // Write timestamp
    let timeline = format!(
        "{:02}:{:02}:{:02},{:03} --> {:02}:{:02}:{:02},{:03}{}",
        h1, m1, s1, ms1, h2, m2, s2, ms2, crlf_str
    );
    let encoded = encode_to_vec(context, timeline.as_bytes());

    debug!(msg_type = DebugMessageFlag::DECODER_608; "\n- - - SRT caption - - -\n");
    debug!(msg_type = DebugMessageFlag::DECODER_608; "{}", timeline);

    if write_wrapped(out_fh, &encoded).is_err() {
        return -1;
    }

    // Unescape \\n and write each line
    let parts: Vec<&str> = input.split("\\n").collect();
    for part in &parts {
        if part.is_empty() {
            continue;
        }
        let mut el = vec![0u8; part.len() * 3 + 1];
        let u = encode_line(context, &mut el, part.as_bytes()) as usize;

        if context.encoding != ccx_encoding_type_CCX_ENC_UNICODE {
            debug!(msg_type = DebugMessageFlag::DECODER_608; "\r");
            if let Ok(s) = std::str::from_utf8(&el[..u]) {
                debug!(msg_type = DebugMessageFlag::DECODER_608; "{}\n", s);
            }
        }

        if write_wrapped(out_fh, &el[..u]).is_err() {
            return -1;
        }
        if write_wrapped(out_fh, &crlf).is_err() {
            return -1;
        }
    }

    debug!(msg_type = DebugMessageFlag::DECODER_608; "- - - - - - - - - - - -\r\n");

    if write_wrapped(out_fh, &crlf).is_err() {
        return -1;
    }

    0
}

/// Write a string as SRT to the default output.
///
/// # Safety
/// Accesses raw pointers from encoder context.
#[no_mangle]
pub unsafe extern "C" fn ccxr_write_stringz_as_srt(
    string: *const i8,
    context: *mut encoder_ctx,
    ms_start: i64,
    ms_end: i64,
) -> c_int {
    if context.is_null() {
        return -1;
    }
    let ctx = &mut *context;
    let fh = (*ctx.out).fh;
    let counter_ptr = &mut ctx.srt_counter as *mut u32;
    write_stringz_as_srt_to_output(string, ctx, ms_start, ms_end, fh, counter_ptr)
}

/// Write a CEA-608 screen buffer as SRT.
///
/// # Safety
/// Accesses raw pointers from encoder context.
#[no_mangle]
pub unsafe extern "C" fn ccxr_write_cc_buffer_as_srt(
    data: *const eia608_screen,
    context: *mut encoder_ctx,
) -> c_int {
    if context.is_null() || data.is_null() {
        return 0;
    }
    let ctx = &mut *context;
    let screen = &*data;

    let mut wrote_something: c_int = 0;
    let mut prev_line_start: i32 = -1;
    let mut prev_line_end: i32 = -1;
    let mut prev_line_center1: i32 = -1;
    let mut prev_line_center2: i32 = -1;

    // Skip empty screens
    let empty = (0..15).all(|i| screen.row_used[i] == 0);
    if empty {
        return 0;
    }

    let mut h1: u32 = 0;
    let mut m1: u32 = 0;
    let mut s1: u32 = 0;
    let mut ms1: u32 = 0;
    let mut h2: u32 = 0;
    let mut m2: u32 = 0;
    let mut s2: u32 = 0;
    let mut ms2: u32 = 0;
    ccxr_millis_to_time(screen.start_time, &mut h1, &mut m1, &mut s1, &mut ms1);
    ccxr_millis_to_time(screen.end_time - 1, &mut h2, &mut m2, &mut s2, &mut ms2);

    let crlf = copy_crlf(ctx);
    let crlf_str = std::str::from_utf8(&crlf).unwrap_or("\r\n");
    let out_fh = (*ctx.out).fh;

    // Write counter
    ctx.srt_counter += 1;
    let counter_line = format!("{}{}", ctx.srt_counter, crlf_str);
    let encoded = encode_to_vec(ctx, counter_line.as_bytes());
    if write_wrapped(out_fh, &encoded).is_err() {
        return 0;
    }

    // Write timestamp
    let timeline = format!(
        "{:02}:{:02}:{:02},{:03} --> {:02}:{:02}:{:02},{:03}{}",
        h1, m1, s1, ms1, h2, m2, s2, ms2, crlf_str
    );
    let encoded = encode_to_vec(ctx, timeline.as_bytes());
    if write_wrapped(out_fh, &encoded).is_err() {
        return 0;
    }

    debug!(msg_type = DebugMessageFlag::DECODER_608; "\n- - - SRT caption ( {}) - - -\n", ctx.srt_counter);
    debug!(msg_type = DebugMessageFlag::DECODER_608; "{}", timeline);

    for i in 0..15usize {
        if screen.row_used[i] == 0 {
            continue;
        }

        // Autodash logic
        if ctx.autodash != 0 && ctx.trim_subs != 0 {
            let line = &screen.characters[i];
            let width = CCX_DECODER_608_SCREEN_WIDTH;
            let mut first: i32 = -1;
            let mut last: i32 = -1;

            for (j, &ch) in line.iter().enumerate().take(width) {
                if ch != b' ' && ch != 0 {
                    if first == -1 {
                        first = j as i32;
                    }
                    last = j as i32;
                }
            }

            if first != -1 && last != -1 {
                let mut do_dash = true;
                let mut colon_pos: i32 = -1;

                for j in first..=last {
                    let ch = line[j as usize];
                    if ch == b':' {
                        colon_pos = j;
                        break;
                    }
                    if !(ch as char).is_ascii_uppercase() {
                        break;
                    }
                }

                if prev_line_start == -1 {
                    do_dash = false;
                }
                if first == prev_line_start {
                    do_dash = false;
                }
                if last == prev_line_end {
                    do_dash = false;
                }
                if first > prev_line_start && last < prev_line_end {
                    do_dash = false;
                }
                if (first > prev_line_start && first < prev_line_end)
                    || (last > prev_line_start && last < prev_line_end)
                {
                    do_dash = false;
                }

                let center1 = (first + last) / 2;
                let center2 = if colon_pos != -1 {
                    let mut cp = colon_pos;
                    while (cp as usize) < width {
                        let ch = line[cp as usize];
                        if ch != b':' && ch != b' ' && ch != 0x89 {
                            break;
                        }
                        cp += 1;
                    }
                    (cp + last) / 2
                } else {
                    center1
                };

                if center1 >= prev_line_center1 - 1
                    && center1 <= prev_line_center1 + 1
                    && center1 != -1
                {
                    do_dash = false;
                }
                if center2 >= prev_line_center2 - 2
                    && center2 <= prev_line_center2 + 2
                    && center2 != -1
                {
                    do_dash = false;
                }

                if do_dash {
                    let _ = write_wrapped(out_fh, b"- ");
                }

                prev_line_start = first;
                prev_line_end = last;
                prev_line_center1 = center1;
                prev_line_center2 = center2;
            }
        }

        // Encode and write the line
        let length = get_decoder_line_encoded(context, ctx.subline, i as c_int, data);
        let sub_slice = std::slice::from_raw_parts(ctx.subline, length as usize);

        if ctx.encoding != ccx_encoding_type_CCX_ENC_UNICODE {
            debug!(msg_type = DebugMessageFlag::DECODER_608; "\r");
            if let Ok(s) = std::str::from_utf8(sub_slice) {
                debug!(msg_type = DebugMessageFlag::DECODER_608; "{}\n", s);
            }
        }

        if write_wrapped(out_fh, sub_slice).is_ok() {
            wrote_something = 1;
        }
        let _ = write_wrapped(out_fh, &crlf);
    }

    debug!(msg_type = DebugMessageFlag::DECODER_608; "- - - - - - - - - - - -\r\n");

    let _ = write_wrapped(out_fh, &crlf);

    wrote_something
}

// FFI declarations for teletext multi-page support

/// Write a cc_subtitle linked list as SRT.
///
/// Walks the subtitle chain, handles teletext multi-page output (issue #665),
/// and frees processed subtitle data.
///
/// # Safety
/// Accesses raw pointers from encoder context and subtitle chain.
#[no_mangle]
pub unsafe extern "C" fn ccxr_write_cc_subtitle_as_srt(
    sub: *mut crate::bindings::cc_subtitle,
    context: *mut encoder_ctx,
) -> c_int {
    if context.is_null() || sub.is_null() {
        return 0;
    }

    let ctx = &mut *context;
    let mut ret: c_int = 0;
    let osub = sub;
    let mut current = sub;
    let mut lsub = sub;

    while !current.is_null() {
        let s = &mut *current;
        if s.type_ == crate::bindings::subtype_CC_TEXT {
            // Teletext multi-page: use page-specific output if available
            let out = get_teletext_output(context, s.teletext_page);
            let counter = get_teletext_srt_counter(context, s.teletext_page);

            if !out.is_null() && !counter.is_null() {
                let _ = write_stringz_as_srt_to_output(
                    s.data as *const i8,
                    ctx,
                    s.start_time,
                    s.end_time,
                    (*out).fh,
                    &mut *counter,
                );
            } else {
                let fh = (*ctx.out).fh;
                let counter_ptr = &mut ctx.srt_counter as *mut u32;
                let _ = write_stringz_as_srt_to_output(
                    s.data as *const i8,
                    ctx,
                    s.start_time,
                    s.end_time,
                    fh,
                    &mut *counter_ptr,
                );
            }

            // Free subtitle data
            if !s.data.is_null() {
                crate::ffi_alloc::c_free(s.data);
                s.data = std::ptr::null_mut();
            }
            s.nb_data = 0;
            ret = 1;
        }
        lsub = current;
        current = s.next;
    }

    // Free the linked list (except the original node)
    while lsub != osub {
        let prev = (*lsub).prev;
        crate::ffi_alloc::c_free(lsub);
        lsub = prev;
    }

    ret
}

#[cfg(feature = "hardsubx_ocr")]
extern "C" {
    fn paraof_ocrtext(sub: *mut crate::bindings::cc_subtitle, context: *mut encoder_ctx)
        -> *mut i8;
}

/// SUB_EOD_MARKER flag (from ccx_decoders_structs.h)
#[cfg(feature = "hardsubx_ocr")]
const SUB_EOD_MARKER: c_int = 1 << 0;

/// Write OCR bitmap subtitle as SRT.
///
/// Calls tesseract via paraof_ocrtext to convert bitmap subtitles to text,
/// then writes the result as SRT. Only active when ENABLE_OCR is set.
///
/// # Safety
/// Accesses raw pointers from encoder context and subtitle data.
#[cfg(feature = "hardsubx_ocr")]
#[no_mangle]
pub unsafe extern "C" fn ccxr_write_cc_bitmap_as_srt(
    sub: *mut crate::bindings::cc_subtitle,
    context: *mut encoder_ctx,
) -> c_int {
    if context.is_null() || sub.is_null() {
        return 0;
    }
    let ctx = &mut *context;
    let s = &mut *sub;

    if s.nb_data == 0 {
        return 0;
    }

    // prev_start is initialized to -1 in encoder_init (ccx_encoders_common.c:791)
    if s.flags & SUB_EOD_MARKER != 0 {
        ctx.prev_start = s.start_time;
    }

    let str_ptr = paraof_ocrtext(sub, context);
    if !str_ptr.is_null() {
        if ctx.is_mkv == 1 {
            // Save recognized string for later use in matroska.c
            ctx.last_string = str_ptr;
        } else {
            if ctx.prev_start != -1 || (s.flags & SUB_EOD_MARKER) == 0 {
                let crlf = copy_crlf(ctx);
                let crlf_str = std::str::from_utf8(&crlf).unwrap_or("\r\n");
                let out_fh = (*ctx.out).fh;

                let mut h1: u32 = 0;
                let mut m1: u32 = 0;
                let mut s1: u32 = 0;
                let mut ms1: u32 = 0;
                let mut h2: u32 = 0;
                let mut m2: u32 = 0;
                let mut s2: u32 = 0;
                let mut ms2: u32 = 0;
                ccxr_millis_to_time(s.start_time, &mut h1, &mut m1, &mut s1, &mut ms1);
                ccxr_millis_to_time(s.end_time - 1, &mut h2, &mut m2, &mut s2, &mut ms2);

                // Write counter
                ctx.srt_counter += 1;
                let counter_line = format!("{}{}", ctx.srt_counter, crlf_str);
                let encoded = encode_to_vec(ctx, counter_line.as_bytes());
                let _ = write_wrapped(out_fh, &encoded);

                // Write timestamp
                let timeline = format!(
                    "{:02}:{:02}:{:02},{:03} --> {:02}:{:02}:{:02},{:03}{}",
                    h1, m1, s1, ms1, h2, m2, s2, ms2, crlf_str
                );
                let encoded = encode_to_vec(ctx, timeline.as_bytes());
                let _ = write_wrapped(out_fh, &encoded);

                // Write OCR text
                let len = std::ffi::CStr::from_ptr(str_ptr).to_bytes().len();
                let text_slice = std::slice::from_raw_parts(str_ptr as *const u8, len);
                let _ = write_wrapped(out_fh, text_slice);
                let _ = write_wrapped(out_fh, &crlf);
            }
            crate::ffi_alloc::c_free(str_ptr);
        }
    }

    // Free bitmap data
    // cc_bitmap is not in bindgen output, define layout manually
    // TODO: add cc_bitmap to bindgen allowlist in build.rs instead of manual repr
    #[repr(C)]
    struct CcBitmap {
        x: c_int,
        y: c_int,
        w: c_int,
        h: c_int,
        nb_colors: c_int,
        data0: *mut std::os::raw::c_uchar,
        data1: *mut std::os::raw::c_uchar,
        linesize0: c_int,
        linesize1: c_int,
    }
    let data_ptr = s.data as *mut CcBitmap;
    if !data_ptr.is_null() {
        for i in 0..s.nb_data as isize {
            let rect = &mut *data_ptr.offset(i);
            if !rect.data0.is_null() {
                crate::ffi_alloc::c_free(rect.data0);
                rect.data0 = std::ptr::null_mut();
            }
            if !rect.data1.is_null() {
                crate::ffi_alloc::c_free(rect.data1);
                rect.data1 = std::ptr::null_mut();
            }
        }
    }

    s.nb_data = 0;
    if !s.data.is_null() {
        crate::ffi_alloc::c_free(s.data);
        s.data = std::ptr::null_mut();
    }

    0
}
