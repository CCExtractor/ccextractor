use crate::bindings::{
    ccx_encoding_type_CCX_ENC_ASCII, ccx_encoding_type_CCX_ENC_LATIN_1,
    ccx_encoding_type_CCX_ENC_UNICODE, ccx_encoding_type_CCX_ENC_UTF_8, eia608_screen, encoder_ctx,
    font_bits_FONT_ITALICS, font_bits_FONT_REGULAR, font_bits_FONT_UNDERLINED,
    font_bits_FONT_UNDERLINED_ITALICS,
};
use crate::encoder::headers_and_footers::{encode_line, write_raw};
use crate::libccxr_exports::time::ccxr_millis_to_time;
use lib_ccxr::util::encoding::{line21_to_latin1, line21_to_ucs2, line21_to_utf8};
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
        let bytes_written = written as usize;
        unsafe {
            current_buf = current_buf.add(bytes_written);
        }
        remaining -= bytes_written;
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
        let bytes_written = match ctx.encoding {
            ccx_encoding_type_CCX_ENC_UTF_8 => {
                let (utf8_packed, byte_count) = line21_to_utf8(*i);

                // Extract bytes based on count (big-endian storage)
                match byte_count {
                    1 => {
                        if buffer_pos < buffer.len() {
                            buffer[buffer_pos] = utf8_packed as c_uchar;
                            1
                        } else {
                            0
                        }
                    }
                    2 => {
                        if buffer_pos + 1 < buffer.len() {
                            buffer[buffer_pos] = (utf8_packed >> 8) as c_uchar;
                            buffer[buffer_pos + 1] = utf8_packed as c_uchar;
                            2
                        } else {
                            0
                        }
                    }
                    3 => {
                        if buffer_pos + 2 < buffer.len() {
                            buffer[buffer_pos] = (utf8_packed >> 16) as c_uchar;
                            buffer[buffer_pos + 1] = (utf8_packed >> 8) as c_uchar;
                            buffer[buffer_pos + 2] = utf8_packed as c_uchar;
                            3
                        } else {
                            0
                        }
                    }
                    _ => 0, // Invalid byte count
                }
            }
            ccx_encoding_type_CCX_ENC_LATIN_1 => {
                if buffer_pos < buffer.len() {
                    let latin1_char = line21_to_latin1(*i);
                    buffer[buffer_pos] = latin1_char as c_uchar;
                    1
                } else {
                    0
                }
            }
            ccx_encoding_type_CCX_ENC_UNICODE => {
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
            ccx_encoding_type_CCX_ENC_ASCII => {
                if buffer_pos < buffer.len() {
                    buffer[buffer_pos] = *i;
                    1
                } else {
                    0
                }
            }
            _ => {
                0 // This should never be reached
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
    }

    // Increment counter
    context.srt_counter += 1;

    // Create timeline string for counter
    let counter_line = format!("{}{}", context.srt_counter, unsafe {
        std::str::from_utf8_unchecked(std::slice::from_raw_parts(
            context.encoded_crlf.as_ptr(),
            context.encoded_crlf_length as usize,
        ))
    });

    // Encode and write counter line
    let buffer_slice =
        unsafe { std::slice::from_raw_parts_mut(context.buffer, context.capacity as usize) };
    let used = encode_line(context, buffer_slice, counter_line.as_bytes());

    if write_wrapped(unsafe { (*context.out).fh }, &buffer_slice[..used as usize]).is_err() {
        return 0;
    }

    // Create timeline string for timestamps
    let timestamp_line = format!(
        "{:02}:{:02}:{:02},{:03} --> {:02}:{:02}:{:02},{:03}{}",
        h1,
        m1,
        s1,
        ms1,
        h2,
        m2,
        s2,
        ms2,
        unsafe {
            std::str::from_utf8_unchecked(std::slice::from_raw_parts(
                context.encoded_crlf.as_ptr(),
                context.encoded_crlf_length as usize,
            ))
        }
    );

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

        // Get line encoded
        let length = get_line_encoded(context, subline_slice, i, data);
        if write_wrapped(
            unsafe { (*context.out).fh },
            &subline_slice[..length as usize],
        )
        .is_err()
        {
            return 0;
        }

        // Get color encoded
        let length = get_color_encoded(context, subline_slice, i, data);
        if write_wrapped(
            unsafe { (*context.out).fh },
            &subline_slice[..length as usize],
        )
        .is_err()
        {
            return 0;
        }

        // Get font encoded
        let length = get_font_encoded(context, subline_slice, i, data);
        if write_wrapped(
            unsafe { (*context.out).fh },
            &subline_slice[..length as usize],
        )
        .is_err()
        {
            return 0;
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
