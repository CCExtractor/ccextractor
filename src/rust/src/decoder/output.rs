//! Utilty functions to write captions

#[cfg(unix)]
use std::os::unix::prelude::{FromRawFd, IntoRawFd};
#[cfg(windows)]
use std::os::windows::io::{FromRawHandle, IntoRawHandle};
use std::{fs::File, io::Write};

use crate::{bindings::*, utils::is_true};

use log::{debug, warn};

// Context for writing subtitles to file
pub struct Writer<'a> {
    pub cea_708_counter: &'a mut u32,
    pub subs_delay: LLONG,
    pub end_frame: Vec<u8>,
    pub write_format: ccx_output_format,
    pub writer_ctx: &'a mut dtvcc_writer_ctx,
    pub no_font_color: bool,
    pub transcript_settings: &'a ccx_encoders_transcript_format,
    pub no_bom: i32,
    pub old_cc_time_end: i32,
    pub encoding: ccx_encoding_type,
}

impl<'a> Writer<'a> {
    /// Create a new writer context
    #[allow(clippy::too_many_arguments)]
    pub fn new(
        cea_708_counter: &'a mut u32,
        subs_delay: LLONG,
        write_format: ccx_output_format,
        writer_ctx: &'a mut dtvcc_writer_ctx,
        no_font_color: i32,
        transcript_settings: &'a ccx_encoders_transcript_format,
        no_bom: i32,
        encoded_end_frame: &[u8],
        encoding: ccx_encoding_type,
    ) -> Self {
        Self {
            cea_708_counter,
            subs_delay,
            end_frame: encoded_end_frame.to_vec(),
            write_format,
            writer_ctx,
            no_font_color: is_true(no_font_color),
            transcript_settings,
            no_bom,
            old_cc_time_end: 0,
            encoding,
        }
    }
    /// Write subtitles to the file
    ///
    /// File must already exist
    /// On Unix Platforms, uses the raw fd from context to access the file
    /// On Windows Platforms, uses the raw handle from context to access the file
    pub fn write_to_file(&mut self, buf: &[u8]) -> Result<(), String> {
        #[cfg(unix)]
        {
            let mut file = unsafe { File::from_raw_fd(self.writer_ctx.fd) };
            file.write_all(buf).map_err(|err| err.to_string())?;
            self.writer_ctx.fd = file.into_raw_fd();
        }
        #[cfg(windows)]
        {
            let mut file = unsafe { File::from_raw_handle(self.writer_ctx.fhandle) };
            file.write_all(buf).map_err(|err| err.to_string())?;
            self.writer_ctx.fhandle = file.into_raw_handle();
        }
        Ok(())
    }
    /// Finish writing up any remaining parts
    pub fn write_done(&mut self) {
        if self.write_format == ccx_output_format::CCX_OF_SAMI {
            if let Err(err) = self.write_sami_footer() {
                warn!("{err}");
            }
        } else {
            debug!("dtvcc_write_done: no handling required");
        }
    }
    pub fn write_crlf(&mut self) -> Result<(), String> {
        if self.encoding == ccx_encoding_type_CCX_ENC_UNICODE {
            self.write_to_file(&[0x0D, 0x00, 0x0A, 0x00])
        } else {
            self.write_to_file(b"\r\n")
        }
    }

    pub fn write_encoded_str(&mut self, s: &str) -> Result<(), String> {
        if self.encoding == ccx_encoding_type_CCX_ENC_UNICODE {
            let mut buf = Vec::with_capacity(s.len() * 2);
            for c in s.chars() {
                let u = c as u32;
                if u <= 0xFFFF {
                    buf.extend_from_slice(&(u as u16).to_le_bytes());
                } else {
                    let mut utf16 = [0u16; 2];
                    for unit in c.encode_utf16(&mut utf16) {
                        buf.extend_from_slice(&unit.to_le_bytes());
                    }
                }
            }
            self.write_to_file(&buf)
        } else if self.encoding == ccx_encoding_type_CCX_ENC_LATIN_1 {
            let mut buf = Vec::with_capacity(s.len());
            for c in s.chars() {
                if (c as u32) <= 0xFF {
                    buf.push(c as u8);
                } else if c == '♬' {
                    buf.push(b'#');
                } else {
                    buf.push(b'?');
                }
            }
            self.write_to_file(&buf)
        } else {
            self.write_to_file(s.as_bytes())
        }
    }

    pub fn write_encoded_bytes(&mut self, buf: &[u8]) -> Result<(), String> {
        if let Ok(s) = std::str::from_utf8(buf) {
            self.write_encoded_str(s)
        } else {
            self.write_to_file(buf)
        }
    }

    /// Writes the footer according to the SAMI format
    pub fn write_sami_footer(&mut self) -> Result<(), String> {
        self.write_encoded_str("</body></sami>")?;
        Ok(())
    }
}

pub fn write_symbol(sym: &dtvcc_symbol, buf: &mut Vec<u8>) {
    if !sym.is_set() || sym.sym == 0 {
        buf.push(b' ');
    } else if sym.sym == 9836 || sym.sym == 0x7F {
        buf.extend_from_slice(&[0xe2, 0x99, 0xac]);
    } else if sym.sym < 0x80 {
        buf.push(sym.sym as u8);
    } else if sym.sym <= 0x7FF {
        buf.push((0xC0 | (sym.sym >> 6)) as u8);
        buf.push((0x80 | (sym.sym & 0x3F)) as u8);
    } else if let Some(c) = char::from_u32(sym.sym as u32) {
        let mut tmp = [0u8; 4];
        buf.extend_from_slice(c.encode_utf8(&mut tmp).as_bytes());
    } else {
        buf.push(b'?');
    }
}

/// Write the symbol to the provided buffer
///
/// The `use_utf16` parameter controls the output format:
/// - `true`: Always writes 2 bytes (UTF-16BE format). Use for UTF-16/UCS-2 charsets.
/// - `false`: Writes 1 byte for ASCII (high byte == 0), 2 bytes for extended chars.
///   Use for variable-width encodings like EUC-KR, CP949, Shift-JIS, etc.
///
/// Issue #1451: Japanese/Chinese with UTF-16BE need 2 bytes for all characters.
/// Issue #1065: Korean with EUC-KR needs 1 byte for ASCII, 2 bytes for Korean.
pub fn write_char(sym: &dtvcc_symbol, buf: &mut Vec<u8>, use_utf16: bool) {
    if sym.sym == 0 {
        if use_utf16 {
            buf.push(0);
        }
        buf.push(b' ');
        return;
    }
    let high = (sym.sym >> 8) as u8;
    let low = (sym.sym & 0xff) as u8;

    if use_utf16 {
        // UTF-16BE: Always write 2 bytes
        buf.push(high);
        buf.push(low);
    } else {
        // Variable-width: Only write high byte if non-zero
        if high != 0 {
            buf.push(high);
        }
        buf.push(low);
    }
}

/// Check if a charset name indicates UTF-16 or UCS-2 encoding
///
/// These are fixed-width 16-bit encodings where even ASCII needs 2 bytes.
pub fn is_utf16_charset(charset: &str) -> bool {
    let upper = charset.to_uppercase();
    upper.contains("UTF-16")
        || upper.contains("UTF16")
        || upper.contains("UCS-2")
        || upper.contains("UCS2")
        || upper.contains("UTF_16")
        || upper.contains("UCS_2")
}

/// Convert from CEA-708 color representation to hex code
///
/// Two bits are specified for each red, green, and blue color value which defines the
/// intensity of each individual color component.
///
/// Refer section 8.8 CEA-708-E
pub fn color_to_hex(color: u8) -> (u8, u8, u8) {
    let red = color >> 4;
    let green = (color >> 2) & 0x3;
    let blue = color & 0x3;
    debug!("Color: {color} [{color:06x}] {red} {green} {blue}");
    (red, green, blue)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_write_char_utf16_mode() {
        let mut buf = Vec::new();

        // UTF-16 mode: ASCII symbol 'A' (0x41) becomes [0x00, 0x41]
        let sym = dtvcc_symbol { sym: 0x41, init: 0 };
        write_char(&sym, &mut buf, true);
        assert_eq!(buf, vec![0x00, 0x41]);

        buf.clear();

        // UTF-16 mode: Non-ASCII symbol writes as [high_byte, low_byte]
        let sym = dtvcc_symbol {
            sym: 0x1234,
            init: 0,
        };
        write_char(&sym, &mut buf, true);
        assert_eq!(buf, vec![0x12, 0x34]);
    }

    #[test]
    fn test_write_char_variable_width_mode() {
        let mut buf = Vec::new();

        // Variable-width mode: ASCII symbol 'A' (0x41) becomes [0x41] (1 byte)
        let sym = dtvcc_symbol { sym: 0x41, init: 0 };
        write_char(&sym, &mut buf, false);
        assert_eq!(buf, vec![0x41]);

        buf.clear();

        // Variable-width mode: Korean EUC-KR char becomes [high, low] (2 bytes)
        // Example: Korean '인' = 0xC0CE in EUC-KR
        let sym = dtvcc_symbol {
            sym: 0xC0CE,
            init: 0,
        };
        write_char(&sym, &mut buf, false);
        assert_eq!(buf, vec![0xC0, 0xCE]);

        buf.clear();

        // Variable-width mode: Space (0x20) becomes [0x20] (1 byte, no NUL)
        let sym = dtvcc_symbol { sym: 0x20, init: 0 };
        write_char(&sym, &mut buf, false);
        assert_eq!(buf, vec![0x20]);
    }

    #[test]
    fn test_is_utf16_charset() {
        // Should return true for UTF-16 variants
        assert!(is_utf16_charset("UTF-16BE"));
        assert!(is_utf16_charset("UTF-16LE"));
        assert!(is_utf16_charset("utf-16"));
        assert!(is_utf16_charset("UTF16"));
        assert!(is_utf16_charset("UCS-2"));
        assert!(is_utf16_charset("UCS2"));

        // Should return false for variable-width encodings
        assert!(!is_utf16_charset("EUC-KR"));
        assert!(!is_utf16_charset("CP949"));
        assert!(!is_utf16_charset("Shift-JIS"));
        assert!(!is_utf16_charset("UTF-8"));
        assert!(!is_utf16_charset("ISO-8859-1"));
    }

    #[test]
    fn test_color_to_hex() {
        assert_eq!(color_to_hex(0b00_00_00), (0, 0, 0)); // Black
        assert_eq!(color_to_hex(0b11_11_11), (3, 3, 3)); // White
        assert_eq!(color_to_hex(0b10_01_00), (2, 1, 0)); // Red
        assert_eq!(color_to_hex(0b01_10_01), (1, 2, 1)); // Green
        assert_eq!(color_to_hex(0b00_11_10), (0, 3, 2)); // Blue
    }

    #[test]
    fn test_write_char_empty_cell() {
        let sym = dtvcc_symbol { sym: 0, init: 0 };
        let mut buf_var = Vec::new();
        write_char(&sym, &mut buf_var, false);
        assert_eq!(buf_var, vec![0x20]);

        let mut buf_utf16 = Vec::new();
        write_char(&sym, &mut buf_utf16, true);
        assert_eq!(buf_utf16, vec![0x00, 0x20]);
    }

    #[test]
    fn test_write_symbol_voce() {
        let chars = [0x76u16, 0x6Fu16, 0x63u16, 0xEAu16];
        let mut buf = Vec::new();
        for &c in &chars {
            let sym = dtvcc_symbol::new(c);
            write_symbol(&sym, &mut buf);
        }
        assert_eq!(buf, vec![0x76, 0x6f, 0x63, 0xc3, 0xaa]);
    }

    #[test]
    fn test_write_symbol_empty_cell() {
        let mut buf = Vec::new();
        let sym_uninit = dtvcc_symbol { sym: 0, init: 0 };
        write_symbol(&sym_uninit, &mut buf);
        assert_eq!(buf, vec![0x20]);

        buf.clear();
        let sym_zero = dtvcc_symbol { sym: 0, init: 1 };
        write_symbol(&sym_zero, &mut buf);
        assert_eq!(buf, vec![0x20]);
    }

    #[test]
    fn test_write_symbol_musical_note() {
        for sym_val in [9836u16, 0x7Fu16] {
            let sym = dtvcc_symbol::new(sym_val);
            let mut buf = Vec::new();
            write_symbol(&sym, &mut buf);
            assert_eq!(buf, vec![0xe2, 0x99, 0xac]);
        }
    }

    #[test]
    fn test_write_symbol_16bit() {
        let sym = dtvcc_symbol::new(0x4e00);
        let mut buf = Vec::new();
        write_symbol(&sym, &mut buf);
        assert_eq!(buf, vec![0xe4, 0xb8, 0x80]);
    }

    #[test]
    fn test_write_symbol_g2_g3() {
        let sym_g2 = dtvcc_symbol::new(0x00A0);
        let mut buf_utf8 = Vec::new();
        write_symbol(&sym_g2, &mut buf_utf8);
        assert_eq!(buf_utf8, vec![0xc2, 0xa0]);

        let sym_special = dtvcc_symbol::new(0x2022);
        let mut buf_bullet = Vec::new();
        write_symbol(&sym_special, &mut buf_bullet);
        assert_eq!(buf_bullet, vec![0xe2, 0x80, 0xa2]);
    }

    #[test]
    #[cfg(unix)]
    fn test_write_encoded_bytes_encodings() {
        use crate::utils::get_zero_allocated_obj;
        use std::fs::File;
        use std::io::{Read, Seek, SeekFrom};
        use std::os::unix::io::{FromRawFd, IntoRawFd};
        use tempfile::tempfile;

        let transcript_settings = get_zero_allocated_obj::<ccx_encoders_transcript_format>();
        let voce_utf8 = "você".as_bytes();

        {
            let mut writer_ctx = get_zero_allocated_obj::<dtvcc_writer_ctx>();
            let temp_file = tempfile().unwrap();
            writer_ctx.fd = temp_file.into_raw_fd();
            let mut counter = 0u32;
            let mut writer = Writer::new(
                &mut counter,
                0,
                ccx_output_format::CCX_OF_SRT,
                &mut writer_ctx,
                0,
                &transcript_settings,
                0,
                b"\r\n",
                ccx_encoding_type_CCX_ENC_UTF_8,
            );
            writer.write_encoded_bytes(voce_utf8).unwrap();
            let mut file = unsafe { File::from_raw_fd(writer_ctx.fd) };
            file.seek(SeekFrom::Start(0)).unwrap();
            let mut output = Vec::new();
            file.read_to_end(&mut output).unwrap();
            assert_eq!(output, vec![0x76, 0x6f, 0x63, 0xc3, 0xaa]);
        }

        {
            let mut writer_ctx = get_zero_allocated_obj::<dtvcc_writer_ctx>();
            let temp_file = tempfile().unwrap();
            writer_ctx.fd = temp_file.into_raw_fd();
            let mut counter = 0u32;
            let mut writer = Writer::new(
                &mut counter,
                0,
                ccx_output_format::CCX_OF_SRT,
                &mut writer_ctx,
                0,
                &transcript_settings,
                0,
                b"\r\n",
                ccx_encoding_type_CCX_ENC_LATIN_1,
            );
            writer.write_encoded_bytes(voce_utf8).unwrap();
            let mut file = unsafe { File::from_raw_fd(writer_ctx.fd) };
            file.seek(SeekFrom::Start(0)).unwrap();
            let mut output = Vec::new();
            file.read_to_end(&mut output).unwrap();
            assert_eq!(output, vec![0x76, 0x6f, 0x63, 0xea]);
        }

        {
            let mut writer_ctx = get_zero_allocated_obj::<dtvcc_writer_ctx>();
            let temp_file = tempfile().unwrap();
            writer_ctx.fd = temp_file.into_raw_fd();
            let mut counter = 0u32;
            let mut writer = Writer::new(
                &mut counter,
                0,
                ccx_output_format::CCX_OF_SRT,
                &mut writer_ctx,
                0,
                &transcript_settings,
                0,
                b"\r\n",
                ccx_encoding_type_CCX_ENC_UNICODE,
            );
            writer.write_encoded_bytes(voce_utf8).unwrap();
            let mut file = unsafe { File::from_raw_fd(writer_ctx.fd) };
            file.seek(SeekFrom::Start(0)).unwrap();
            let mut output = Vec::new();
            file.read_to_end(&mut output).unwrap();
            assert_eq!(output, vec![0x76, 0x00, 0x6f, 0x00, 0x63, 0x00, 0xea, 0x00]);
        }
    }

    #[test]
    #[cfg(unix)]
    fn test_write_encoded_bytes_invalid_utf8_fallback() {
        use crate::utils::get_zero_allocated_obj;
        use std::fs::File;
        use std::io::{Read, Seek, SeekFrom};
        use std::os::unix::io::{FromRawFd, IntoRawFd};
        use tempfile::tempfile;

        let transcript_settings = get_zero_allocated_obj::<ccx_encoders_transcript_format>();
        let invalid_bytes = [0xff, 0xfe, 0x80];

        let mut writer_ctx = get_zero_allocated_obj::<dtvcc_writer_ctx>();
        let temp_file = tempfile().unwrap();
        writer_ctx.fd = temp_file.into_raw_fd();
        let mut counter = 0u32;
        let mut writer = Writer::new(
            &mut counter,
            0,
            ccx_output_format::CCX_OF_SRT,
            &mut writer_ctx,
            0,
            &transcript_settings,
            0,
            b"\r\n",
            ccx_encoding_type_CCX_ENC_LATIN_1,
        );
        writer.write_encoded_bytes(&invalid_bytes).unwrap();
        let mut file = unsafe { File::from_raw_fd(writer_ctx.fd) };
        file.seek(SeekFrom::Start(0)).unwrap();
        let mut output = Vec::new();
        file.read_to_end(&mut output).unwrap();
        assert_eq!(output, vec![0xff, 0xfe, 0x80]);
    }
}
