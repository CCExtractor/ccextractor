use crate::info;
use crate::util::encoding::*;
use std::cmp;
use std::convert::TryFrom;

pub enum EncoderError {
    Retry = -100,           // CCX_EAGAIN
    EOF = -101,             // CCX_EOF
    InvalidArgument = -102, // CCX_EINVAL
    Unsupported = -103,     // CCX_ENOSUPP
    OutOfMemory = -104,     // CCX_ENOMEM
}
fn find_limit_characters(
    line: &[u8],
    first_non_blank: &mut i32,
    last_non_blank: &mut i32,
    max_len: usize,
) {
    // initialize to -1 (meaning “not found”)
    *first_non_blank = -1;
    *last_non_blank = -1;

    // clamp to avoid out‑of‑bounds
    let limit = cmp::min(line.len(), max_len);

    for (i, &c) in line.iter().take(limit).enumerate() {
        // detect non‑blank
        if c != b' ' && c != 0x89 {
            if *first_non_blank < 0 {
                *first_non_blank = i as i32;
            }
            *last_non_blank = i as i32;
        }
        // break on end‑of‑string or newline
        if c == b'\0' || c == b'\n' || c == b'\r' {
            break;
        }
    }
}
fn change_utf8_encoding(dest: &mut [u8], src: &[u8], len: i32, out_enc: Encoding) -> i32 {
    let mut dest_idx = 0;
    let mut src_idx = 0;
    let max = usize::min(src.len(), len as usize);

    while src_idx < max {
        let c = src[src_idx];
        let c_len: usize;

        if c < 0x80 {
            c_len = 1;
        } else if (c & 0x20) == 0 {
            c_len = 2;
        } else if (c & 0x10) == 0 {
            c_len = 3;
        } else if (c & 0x08) == 0 {
            c_len = 4;
        } else if (c & 0x04) == 0 {
            c_len = 5;
        } else {
            c_len = 1; // Invalid UTF-8, treat as single byte
        }

        match out_enc {
            Encoding::UTF8 => {
                let to_copy = max;
                if to_copy <= dest.len() {
                    dest[..to_copy].copy_from_slice(&src[..to_copy]);
                    return to_copy as i32;
                } else {
                    return EncoderError::Unsupported as i32;
                }
            }
            Encoding::Latin1 => {
                if c_len == 1 {
                    dest[dest_idx] = src[src_idx];
                    dest_idx += 1;
                } else if c_len == 2 {
                    if (src[src_idx + 1] & 0x40) == 0 {
                        let cp = utf8_to_latin1_map(
                            (((src[src_idx] & 0x1F) as u32) << 6)
                                | ((src[src_idx + 1] & 0x3F) as u32),
                        ) as u16;
                        if cp <= 255 {
                            dest[dest_idx] = cp as u8;
                        } else {
                            dest[dest_idx] = b'?';
                        }
                        dest_idx += 1;
                    } else {
                        dest[dest_idx] = b'?';
                        dest_idx += 1;
                    }
                } else if c_len == 3 {
                    if (src[src_idx + 1] & 0x40) == 0 && (src[src_idx + 2] & 0x40) == 0 {
                        let cp = utf8_to_latin1_map(
                            (((src[src_idx] & 0x0F) as u32) << 12)
                                | (((src[src_idx + 1] & 0x3F) as u32) << 6)
                                | ((src[src_idx + 2] & 0x3F) as u32),
                        ) as u16;
                        if cp <= 255 {
                            dest[dest_idx] = cp as u8;
                        } else {
                            dest[dest_idx] = b'?';
                        }
                        dest_idx += 1;
                    } else {
                        dest[dest_idx] = b'?';
                        dest_idx += 1;
                    }
                } else if c_len == 4 {
                    if (src[src_idx + 1] & 0x40) == 0
                        && (src[src_idx + 2] & 0x40) == 0
                        && (src[src_idx + 3] & 0x40) == 0
                    {
                        let cp = utf8_to_latin1_map(
                            (((src[src_idx] & 0x07) as u32) << 18)
                                | (((src[src_idx + 1] & 0x3F) as u32) << 12)
                                | (((src[src_idx + 2] & 0x3F) as u32) << 6)
                                | ((src[src_idx + 3] & 0x3F) as u32),
                        ) as u16;
                        if cp <= 255 {
                            dest[dest_idx] = cp as u8;
                        } else {
                            dest[dest_idx] = b'?';
                        }
                        dest_idx += 1;
                    } else {
                        dest[dest_idx] = b'?';
                        dest_idx += 1;
                    }
                } else if c_len == 5 {
                    if (src[src_idx + 1] & 0x40) == 0
                        && (src[src_idx + 2] & 0x40) == 0
                        && (src[src_idx + 3] & 0x40) == 0
                        && (src[src_idx + 4] & 0x40) == 0
                    {
                        let cp = utf8_to_latin1_map(
                            (((src[src_idx] & 0x03) as u32) << 24u32)
                                | (((src[src_idx + 1] & 0x3F) as u32) << 18u32)
                                | (((src[src_idx + 2] & 0x3F) as u32) << 12u32)
                                | (((src[src_idx + 3] & 0x3F) as u32) << 6u32)
                                | ((src[src_idx + 4] & 0x3F) as u32),
                        ) as u16;
                        if cp <= 255 {
                            dest[dest_idx] = cp as u8;
                        } else {
                            dest[dest_idx] = b'?';
                        }
                        dest_idx += 1;
                    } else {
                        dest[dest_idx] = b'?';
                        dest_idx += 1;
                    }
                } else {
                    dest[dest_idx] = b'?';
                    dest_idx += 1;
                }
            }
            Encoding::UCS2 => {
                return EncoderError::Unsupported as i32;
            }
            Encoding::ASCII => {
                if c_len == 1 {
                    dest[dest_idx] = src[src_idx];
                    dest_idx += 1;
                } else {
                    dest[dest_idx] = b'?';
                    dest_idx += 1;
                }
            }
        }
        src_idx += c_len;
    }

    if dest_idx < dest.len() {
        dest[dest_idx] = 0;
    }
    dest_idx as i32
}
#[allow(unused_variables)]
fn change_latin1_encoding(dest: &mut [u8], src: &[u8], len: i32, out_enc: Encoding) -> i32 {
    EncoderError::Unsupported as i32
}

#[allow(unused_variables)]
fn change_unicode_encoding(dest: &mut [u8], src: &[u8], len: i32, out_enc: Encoding) -> i32 {
    EncoderError::Unsupported as i32
}

pub fn change_ascii_encoding(
    dest: &mut Vec<u8>,
    src: &[u8],
    out_enc: Encoding,
) -> Result<usize, i32> {
    dest.clear();

    for &c in src {
        match out_enc {
            Encoding::UTF8 => {
                let utf8_packed = line21_to_utf8(c);
                let bytes = if utf8_packed <= 0xff {
                    vec![utf8_packed as u8]
                } else if utf8_packed <= 0xffff {
                    vec![(utf8_packed >> 8) as u8, utf8_packed as u8]
                } else if utf8_packed <= 0xffffff {
                    vec![
                        (utf8_packed >> 16) as u8,
                        (utf8_packed >> 8) as u8,
                        utf8_packed as u8,
                    ]
                } else {
                    return Err(-1); // Invalid byte count
                };
                dest.extend(bytes);
            }
            Encoding::Latin1 => {
                let latin1_char = line21_to_latin1(c);
                dest.push(latin1_char);
            }
            Encoding::UCS2 => {
                let ucs2_char = line21_to_ucs2(c);
                // UCS-2 is 2 bytes, little-endian
                dest.extend_from_slice(&ucs2_char.to_le_bytes());
            }
            Encoding::ASCII => {
                dest.extend_from_slice(src);
                return Ok(src.len());
            }
        }
    }

    // Add null terminator
    dest.push(0);

    Ok(dest.len() - 1) // Return length without null terminator
}

fn utf8_to_latin1_map(character: u32) -> Latin1Char {
    ucs2_to_latin1(char_to_ucs2(char::try_from(character).unwrap()))
}
pub fn get_str_basic(
    out_buffer: &mut Vec<u8>,
    in_buffer: &[u8],
    trim_subs: bool,
    in_enc: Encoding,
    out_enc: Encoding,
    max_len: i32,
) -> i32 {
    let mut last_non_blank: i32 = -1;
    let mut first_non_blank: i32 = -1;
    let len;

    find_limit_characters(
        in_buffer,
        &mut first_non_blank,
        &mut last_non_blank,
        max_len as usize,
    );

    if first_non_blank == -1 {
        // No non-blank characters found, return empty
        out_buffer.clear();
        out_buffer.push(0); // null terminator only
        return 0;
    }

    // Calculate the actual content length (without trailing spaces)
    let mut content_length = last_non_blank - first_non_blank + 1;

    if !trim_subs {
        first_non_blank = 0;
        // If not trimming, we need to recalculate to include leading content
        content_length = last_non_blank + 1;
    }

    if (first_non_blank + content_length) as usize > in_buffer.len() {
        out_buffer.clear();
        out_buffer.push(0);
        return 0;
    }

    // Clear the output buffer
    out_buffer.clear();

    // change encoding only when required
    match in_enc {
        Encoding::UTF8 => {
            let mut temp_buffer = vec![0u8; (content_length * 4) as usize]; // Allow extra space for multi-byte encodings
            len = change_utf8_encoding(
                &mut temp_buffer,
                &in_buffer[first_non_blank as usize..],
                content_length,
                out_enc,
            );
            if len > 0 {
                out_buffer.extend_from_slice(&temp_buffer[..len as usize]);
            }
        }
        Encoding::Latin1 => {
            let mut temp_buffer = vec![0u8; (content_length * 4) as usize];
            len = change_latin1_encoding(
                &mut temp_buffer,
                &in_buffer[first_non_blank as usize..],
                content_length,
                out_enc,
            );
            if len > 0 {
                out_buffer.extend_from_slice(&temp_buffer[..len as usize]);
            }
        }
        Encoding::UCS2 => {
            let mut temp_buffer = vec![0u8; (content_length * 4) as usize];
            len = change_unicode_encoding(
                &mut temp_buffer,
                &in_buffer[first_non_blank as usize..],
                content_length,
                out_enc,
            );
            if len > 0 {
                out_buffer.extend_from_slice(&temp_buffer[..len as usize]);
            }
        }
        Encoding::ASCII => {
            let input_slice =
                &in_buffer[first_non_blank as usize..(first_non_blank + content_length) as usize];
            len = change_ascii_encoding(out_buffer, input_slice, out_enc)
                .unwrap_or(EncoderError::Retry as usize) as i32;
        }
    }

    if len < 0 {
        info!("WARNING: Could not encode in specified format\n");
        out_buffer.clear();
        out_buffer.push(0);
        0
    } else if len == EncoderError::Unsupported as i32 {
        info!("WARNING: Encoding is not yet supported\n");
        out_buffer.clear();
        out_buffer.push(0);
        return 0;
    } else {
        // Add null terminator
        out_buffer.push(0);
        return len; // Return actual content length, not max_len
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_line_with_content() {
        let line = b"  hello world  \n";
        let mut first_non_blank = 0;
        let mut last_non_blank = 0;

        find_limit_characters(line, &mut first_non_blank, &mut last_non_blank, 15);

        assert_eq!(first_non_blank, 2); // 'h' is at index 2
        assert_eq!(last_non_blank, 12); // 'd' is at index 12
    }

    #[test]
    fn test_line_with_special_chars() {
        let line = b" \x89 abc \x89 def \r";
        let mut first_non_blank = 0;
        let mut last_non_blank = 0;

        find_limit_characters(line, &mut first_non_blank, &mut last_non_blank, 20);

        assert_eq!(first_non_blank, 3); // 'a' is at index 3
    }
    #[test]
    fn test_utf8_to_utf8() {
        let src = b"Hello, \xC3\xA9world!"; // "Hello, éworld!"
        let mut dest = [0u8; 20];

        let result = change_utf8_encoding(&mut dest, src, src.len() as i32, Encoding::UTF8);

        assert_eq!(result, src.len() as i32);
        assert_eq!(&dest[..src.len()], src);
    }

    #[test]
    fn test_utf8_to_ascii() {
        let src = b"Hello, \xC3\xA9world!"; // "Hello, éworld!"
        let mut dest = [0u8; 20];

        let result = change_utf8_encoding(&mut dest, src, src.len() as i32, Encoding::ASCII);

        assert_eq!(result, 14); // "Hello, ?world!" (14 chars)
        assert_eq!(&dest[..14], b"Hello, ?world!");
    }

    #[test]
    fn test_unsupported_encoding() {
        let src = b"Hello";
        let mut dest = [0u8; 10];

        let result = change_utf8_encoding(&mut dest, src, src.len() as i32, Encoding::UCS2);

        assert_eq!(result, EncoderError::Unsupported as i32);
    }
    #[test]
    fn test_ascii_to_ascii() {
        let src = b"Hello World!";
        let mut dest = Vec::with_capacity(20);
        let result = change_ascii_encoding(&mut dest, src, Encoding::ASCII);

        assert_eq!(result.unwrap(), src.len());
        assert_eq!(&dest[..src.len()], src);
    }

    #[test]
    fn test_ascii_to_utf8() {
        let src = b"Hello";
        let mut dest = Vec::with_capacity(20);

        let result = change_ascii_encoding(&mut dest, src, Encoding::UTF8);

        assert_eq!(result.unwrap(), 5); // Each ASCII char becomes 1 UTF-8 byte
        assert_eq!(&dest[..5], b"Hello");
        assert_eq!(dest[5], 0); // Null terminator
    }

    #[test]
    fn test_ascii_to_latin1() {
        let src = b"Test";
        let mut dest = Vec::with_capacity(20);

        let result = change_ascii_encoding(&mut dest, src, Encoding::Latin1);

        assert_eq!(result.unwrap(), 4);
        assert_eq!(&dest[..4], b"Test");
        assert_eq!(dest[4], 0); // Null terminator
    }

    #[test]
    fn test_ascii_to_unicode() {
        let src = b"Hi";
        let mut dest = Vec::with_capacity(20);

        let result = change_ascii_encoding(&mut dest, src, Encoding::UCS2);

        assert_eq!(result.unwrap(), 4); // Each ASCII char becomes 2 Unicode bytes
        assert_eq!(dest[4], 0); // Null terminator
    }

    #[test]
    fn test_get_str_basic_with_trim() {
        let in_buffer = b"  Hello  \0";
        let mut out_buffer = Vec::with_capacity(20);

        let result = get_str_basic(
            &mut out_buffer,
            in_buffer,
            true,
            Encoding::ASCII,
            Encoding::ASCII,
            10,
        );

        assert!(result > 0);
    }

    #[test]
    fn test_get_str_basic_without_trim() {
        let in_buffer = b"  Hello  \0";
        let mut out_buffer = Vec::with_capacity(20);

        let result = get_str_basic(
            &mut out_buffer,
            in_buffer,
            false,
            Encoding::ASCII,
            Encoding::ASCII,
            10,
        );

        assert!(result > 0);
    }
}
