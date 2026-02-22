use crate::bindings::{
    eia608_screen, encoder_ctx, font_bits_FONT_ITALICS, font_bits_FONT_REGULAR,
    font_bits_FONT_UNDERLINED, font_bits_FONT_UNDERLINED_ITALICS,
};
use crate::encoder::common::{encode_line, write_raw};
use crate::encoder::FromCType;
use crate::libccxr_exports::time::ccxr_millis_to_time;
use lib_ccxr::util::encoding::{line21_to_latin1, line21_to_ucs2, line21_to_utf8, Encoding};
use std::io;
use std::os::raw::{c_int, c_uchar, c_uint, c_void};

/// Write data to file descriptor with retry logic
pub fn write_wrapped(fd: c_int, buf: &[u8]) -> Result<(), io::Error> {
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

/// Get line characters encoded according to context encoding
pub fn get_line_encoded(
    ctx: &encoder_ctx,
    buffer: &mut [c_uchar],
    line_num: usize,
    data: &eia608_screen,
) -> c_uint {
    let mut buffer_pos = 0;
    let line = &data.characters[line_num];

    for i in line.iter().take(33) {
        let enc = unsafe { Encoding::from_ctype(ctx.encoding).unwrap_or(Encoding::default()) };
        let bytes_written = match enc {
            Encoding::UTF8 => {
                let utf8_char = line21_to_utf8(*i);
                let first_non_zero = (utf8_char.leading_zeros() / 8) as usize;
                let bytes = utf8_char.to_be_bytes(); // UTF-8, big-endian
                                                     // Since we use u32, it would have leading zeroes
                let byte_count = bytes.len() - first_non_zero;
                if buffer_pos + byte_count > buffer.len() {
                    return 0;
                }
                buffer[buffer_pos..(byte_count + buffer_pos)]
                    .copy_from_slice(&bytes[first_non_zero..(first_non_zero + byte_count)]);
                byte_count
            }
            Encoding::Latin1 => {
                if buffer_pos < buffer.len() {
                    let latin1_char = line21_to_latin1(*i);
                    buffer[buffer_pos] = latin1_char as c_uchar;
                    1
                } else {
                    0
                }
            }
            Encoding::UCS2 => {
                if buffer_pos + 1 < buffer.len() {
                    let ucs2_char = line21_to_ucs2(*i);
                    // UCS-2 is 2 bytes, little-endian
                    let bytes = ucs2_char.to_le_bytes();
                    buffer[buffer_pos] = bytes[0] as c_uchar;
                    buffer[buffer_pos + 1] = bytes[1] as c_uchar;
                    2
                } else {
                    0
                }
            }
            Encoding::Line21 => {
                if buffer_pos < buffer.len() {
                    buffer[buffer_pos] = *i;
                    1
                } else {
                    0
                }
            }
        };

        buffer_pos += bytes_written;

        // Break if we've run out of buffer space
        if bytes_written == 0 {
            break;
        }
    }

    buffer_pos as c_uint
}
/// Get color information encoded as characters
pub fn get_color_encoded(
    _ctx: &encoder_ctx,
    buffer: &mut [c_uchar],
    line_num: usize,
    data: &eia608_screen,
) -> c_uint {
    let mut buffer_pos = 0;
    for i in 0..32 {
        if buffer_pos >= buffer.len() {
            break;
        }
        let color_val = data.colors[line_num][i] as u8;
        buffer[buffer_pos] = if color_val < 10 {
            color_val + b'0'
        } else {
            b'E'
        };
        buffer_pos += 1;
    }
    if buffer_pos < buffer.len() {
        buffer[buffer_pos] = 0;
    }
    buffer_pos as c_uint
}

/// Get font information encoded as characters
pub fn get_font_encoded(
    _ctx: &encoder_ctx,
    buffer: &mut [c_uchar],
    line_num: usize,
    data: &eia608_screen,
) -> c_uint {
    let mut buffer_pos = 0;

    for i in 0..32 {
        if buffer_pos >= buffer.len() {
            break;
        }

        let font_val = data.fonts[line_num][i];
        buffer[buffer_pos] = match font_val {
            font_bits_FONT_REGULAR => b'R',
            font_bits_FONT_UNDERLINED_ITALICS => b'B',
            font_bits_FONT_UNDERLINED => b'U',
            font_bits_FONT_ITALICS => b'I',
            _ => b'E',
        };
        buffer_pos += 1;
    }

    buffer_pos as c_uint
}

/// Write CC buffer in G608 format
pub fn write_cc_buffer_as_g608(data: &eia608_screen, context: &mut encoder_ctx) -> c_int {
    let mut wrote_something = 0;

    // Convert start and end times
    let mut h1: u32 = 0;
    let mut m1: u32 = 0;
    let mut s1: u32 = 0;
    let mut ms1: u32 = 0;

    let mut h2: u32 = 0;
    let mut m2: u32 = 0;
    let mut s2: u32 = 0;
    let mut ms2: u32 = 0;

    unsafe {
        ccxr_millis_to_time(data.start_time, &mut h1, &mut m1, &mut s1, &mut ms1);
        ccxr_millis_to_time(data.end_time - 1, &mut h2, &mut m2, &mut s2, &mut ms2);
        // -1 To prevent overlapping with the next line.
    }

    // Increment counter
    context.srt_counter += 1;
    let encoded_clrf = unsafe {
        std::str::from_utf8_unchecked(std::slice::from_raw_parts(
            context.encoded_crlf.as_ptr(),
            context.encoded_crlf_length as usize,
        ))
    };
    // Create timeline string for counter
    let counter_line = format!("{}{}", context.srt_counter, encoded_clrf);

    // Encode and write counter line
    let buffer_slice =
        unsafe { std::slice::from_raw_parts_mut(context.buffer, context.capacity as usize) };
    let used = encode_line(context, buffer_slice, counter_line.as_bytes());

    if write_wrapped(unsafe { (*context.out).fh }, &buffer_slice[..used as usize]).is_err() {
        return 0;
    }

    // Create timeline string for timestamps
    // TODO (GSoC): Expand this to calculate proper coordinate mapping for vertical/Japanese text 
    let is_webvtt = context.write_format == crate::bindings::ccx_output_format::CCX_OF_WEBVTT;
    
    let timestamp_line = if is_webvtt {
        // WebVTT Standard requires dots instead of commas for milliseconds
        // and supports trailing settings (e.g., vertical text direction, alignment)
        format!(
            "{h1:02}:{m1:02}:{s1:02}.{ms1:03} --> {h2:02}:{m2:02}:{s2:02}.{ms2:03} align:start line:100%{encoded_clrf}"
        )
    } else {
        // Legacy SRT Standard requires commas
        format!(
            "{h1:02}:{m1:02}:{s1:02},{ms1:03} --> {h2:02}:{m2:02}:{s2:02},{ms2:03}{encoded_clrf}"
        )
    };

    // Encode and write timestamp line
    let used = encode_line(context, buffer_slice, timestamp_line.as_bytes());

    if write_wrapped(unsafe { (*context.out).fh }, &buffer_slice[..used as usize]).is_err() {
        return 0;
    }

    // Write all 15 lines with their encoding information
    for i in 0..15 {
        let subline_slice = unsafe {
            std::slice::from_raw_parts_mut(context.subline, 1024) // temporary, should be inputted after encoder_ctx => EncoderCtx
        };

        type EncodeFn = fn(&encoder_ctx, &mut [c_uchar], usize, &eia608_screen) -> c_uint;
        let steps: &[EncodeFn] = &[get_line_encoded, get_color_encoded, get_font_encoded];
        for &encode_fn in steps {
            let length = encode_fn(context, subline_slice, i, data);
            if write_wrapped(
                unsafe { (*context.out).fh },
                &subline_slice[..length as usize],
            )
            .is_err()
            {
                return 0;
            }
        }

        // Write CRLF
        let crlf_slice = unsafe {
            std::slice::from_raw_parts(
                context.encoded_crlf.as_ptr(),
                context.encoded_crlf_length as usize,
            )
        };

        if write_wrapped(unsafe { (*context.out).fh }, crlf_slice).is_err() {
            return 0;
        }

        wrote_something = 1;
    }

    // Write final CRLF
    let crlf_slice = unsafe {
        std::slice::from_raw_parts(
            context.encoded_crlf.as_ptr(),
            context.encoded_crlf_length as usize,
        )
    };

    if write_wrapped(unsafe { (*context.out).fh }, crlf_slice).is_err() {
        return 0;
    }

    wrote_something
}
