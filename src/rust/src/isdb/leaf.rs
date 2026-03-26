//! Low-level helper functions for ISDB subtitle decoding.
//! These are leaf-level functions with no dependencies on mid/high-level logic.
//!
//! # Conversion Guide
//!
//! | C (ccx_decoders_isdb.c)            | Rust                                 |
//! |------------------------------------|--------------------------------------|
//! | `RB16` macro                       | [`rb16`]                             |
//! | `RB24` macro                       | [`rb24`]                             |
//! | `move_penpos`                      | [`move_penpos`]                      |
//! | `get_csi_params`                   | [`get_csi_params`]                   |
//! | `ccx_strstr_ignorespace`           | [`strstr_ignorespace`]               |
//! | `set_writing_format`               | [`set_writing_format`]               |
//! | `parse_caption_management_data`    | [`parse_caption_management_data`]    |

use crate::isdb::types::{IsdbSubContext, IsdbTmd, WritingFormat};
use log::debug;

#[inline]
/// Read big-endian u16 from buffer (2 bytes)
pub fn rb16(buf: &[u8]) -> u16 {
    u16::from_be_bytes([buf[0], buf[1]])
}

#[inline]
/// Read big-endian u24 (as u32) from buffer (3 bytes)
pub fn rb24(buf: &[u8]) -> u32 {
    u32::from_be_bytes([0, buf[0], buf[1], buf[2]])
}

pub fn move_penpos(ctx: &mut IsdbSubContext, col: i32, row: i32) {
    let ls = &mut ctx.current_state.layout_state;
    ls.cursor_pos.x = row;
    ls.cursor_pos.y = col;
}

pub fn get_csi_params(buf: &[u8]) -> Option<(usize, u32, Option<u32>)> {
    let mut i = 0;

    // Parse p1
    let mut p1: u32 = 0;
    while i < buf.len() && buf[i] >= 0x30 && buf[i] <= 0x39 {
        p1 = p1 * 10 + (buf[i] - 0x30) as u32;
        i += 1;
    }

    if i >= buf.len() {
        return None;
    }

    // Must be followed by 0x20 (space) or 0x3B (semicolon)
    if buf[i] != 0x20 && buf[i] != 0x3B {
        return None;
    }

    // If terminator is 0x20 (space), only one param
    if buf[i] == 0x20 {
        i += 1;
        return Some((i, p1, None));
    }

    // Skip semicolon separator
    i += 1;

    // Parse p2
    let mut p2: u32 = 0;
    while i < buf.len() && buf[i] >= 0x30 && buf[i] <= 0x39 {
        p2 = p2 * 10 + (buf[i] - 0x30) as u32;
        i += 1;
    }

    // Skip terminator
    i += 1;

    Some((i, p1, Some(p2)))
}

pub fn strstr_ignorespace(str1: &[u8], str2: &[u8]) -> bool {
    for (i, &ch) in str2.iter().enumerate() {
        if ch == b' ' || ch == b'\t' || ch == b'\n' || ch == b'\r' {
            continue;
        }
        if i >= str1.len() || str1[i] != ch {
            return false;
        }
    }
    true
}

pub fn set_writing_format(ctx: &mut IsdbSubContext, arg: &[u8]) {
    let ls = &mut ctx.current_state.layout_state;

    if arg.len() < 2 {
        return;
    }

    // One param: init
    if arg[1] == 0x20 {
        ls.format = WritingFormat::from_i32((arg[0] & 0x0F) as i32);
        return;
    }

    let mut pos: usize = 0;

    // P1 I1 p2 I2 P31~P3i I3 P41~P4j I4 F
    if arg.get(1) == Some(&0x3B) {
        ls.format = WritingFormat::HorizontalCustom;
        pos += 2;
    }

    if pos < arg.len() && arg.get(pos + 1) == Some(&0x3B) {
        // font size param (also commented out in og code)
        // match arg[pos] & 0x0f { 0 => Smol, 1 => Middle, 2 => Standard, _ => {} }
        pos += 2;
    }

    // P3
    debug!("character numbers in one line in decimal");
    while pos < arg.len() && arg[pos] != 0x3B && arg[pos] != 0x20 {
        ctx.nb_char = arg[pos] as i32;
        pos += 1;
    }

    if pos >= arg.len() || arg[pos] == 0x20 {
        return;
    }
    pos += 1; // skip 0x3B

    debug!("line numbers in decimal");
    while pos < arg.len() && arg[pos] != 0x20 {
        ctx.nb_line = arg[pos] as i32;
        pos += 1;
    }
}

pub fn parse_caption_management_data(ctx: &mut IsdbSubContext, buf: &[u8]) -> usize {
    let mut pos: usize = 0;

    if buf.is_empty() {
        return 0;
    }

    ctx.tmd = IsdbTmd::from_i32((buf[pos] >> 6) as i32);
    debug!("CC MGMT DATA: TMD: {}", (buf[0] >> 6));
    pos += 1;

    if ctx.tmd == IsdbTmd::Free {
        debug!("Playback time is not restricted to synchronize to the clock.");
    } else if ctx.tmd == IsdbTmd::OffsetTime {
        /*
         * This 36-bit field indicates offset time to add to the playback time when the
         * clock control mode is in offset time mode. Offset time is coded in the
         * order of hour, minute, second and millisecond, using nine 4-bit binary
         * coded decimals (BCD).
         *
         * +-----------+-----------+---------+--------------+
         * |  hour     |   minute  |   sec   |  millisecond |
         * +-----------+-----------+---------+--------------+
         * |  2 (4bit) | 2 (4bit)  | 2 (4bit)|    3 (4bit)  |
         * +-----------+-----------+---------+--------------+
         */
        if pos + 5 > buf.len() {
            return pos;
        }
        ctx.offset_time.hour = ((buf[pos] >> 4) * 10 + (buf[pos] & 0x0f)) as i32;
        pos += 1;
        ctx.offset_time.min = ((buf[pos] >> 4) * 10 + (buf[pos] & 0x0f)) as i32;
        pos += 1;
        ctx.offset_time.sec = ((buf[pos] >> 4) * 10 + (buf[pos] & 0x0f)) as i32;
        pos += 1;
        ctx.offset_time.milli = ((buf[pos] >> 4) as i32 * 100)
            + ((buf[pos] & 0x0f) as i32 * 10)
            + (buf[pos + 1] & 0x0f) as i32;
        debug!(
            "CC MGMT DATA: OTD( h:{} m:{} s:{} millis: {}",
            ctx.offset_time.hour, ctx.offset_time.min, ctx.offset_time.sec, ctx.offset_time.milli
        );
        pos += 2;
    } else {
        debug!(
            "Playback time is in accordance with the time of the clock, \
            which is calibrated by clock signal (TDT). Playback time is \
            given by PTS."
        );
    }

    if pos >= buf.len() {
        return pos;
    }

    // no. of langs
    ctx.nb_lang = buf[pos] as i32;
    debug!("CC MGMT DATA: nb languages: {}", ctx.nb_lang);
    pos += 1;

    for _ in 0..ctx.nb_lang {
        if pos >= buf.len() {
            break;
        }
        debug!("CC MGMT DATA: {}", (buf[pos] & 0x1F) >> 5);
        ctx.dmf = buf[pos] & 0x0F;
        debug!("CC MGMT DATA: DMF 0x{:X}", ctx.dmf);
        pos += 1;

        if (ctx.dmf == 0x0C || ctx.dmf == 0x0D || ctx.dmf == 0x0E) && pos < buf.len() {
            ctx.dc = buf[pos];
            debug!("Attenuation Due to Rain");
        }

        debug!("CC MGMT DATA: languages: {:?}", &buf[pos..pos+3]);
        if pos + 3 > buf.len() {
            break;
        }
        pos += 3;

        if pos >= buf.len() {
            break;
        }
        debug!("CC MGMT DATA: Format: 0x{:X}", buf[pos] >> 4);
        debug!("CC MGMT DATA: TCS: 0x{:X}", (buf[pos] >> 2) & 0x3);

        ctx.current_state.rollup_mode = (buf[pos] & 0x03) != 0;
        debug!(
            "CC MGMT DATA: Rollup mode: {}",
            ctx.current_state.rollup_mode
        );
    }

    pos
}

#[cfg(test)]
mod tests {
    use crate::isdb::leaf::*;
    use crate::isdb::types::*;

    // ---- move_penpos ----

    #[test]
    fn test_move_penpos() {
        let mut ctx = IsdbSubContext::new();
        move_penpos(&mut ctx, 10, 20);
        assert_eq!(ctx.current_state.layout_state.cursor_pos.x, 20);
        assert_eq!(ctx.current_state.layout_state.cursor_pos.y, 10);
    }

    #[test]
    fn test_move_penpos_zero() {
        let mut ctx = IsdbSubContext::new();
        move_penpos(&mut ctx, 50, 100);
        move_penpos(&mut ctx, 0, 0);
        assert_eq!(ctx.current_state.layout_state.cursor_pos.x, 0);
        assert_eq!(ctx.current_state.layout_state.cursor_pos.y, 0);
    }

    // ---- get_csi_params ----

    #[test]
    fn test_get_csi_params_single() {
        let buf = [0x31, 0x32, 0x33, 0x20]; // "123" + space
        let result = get_csi_params(&buf);
        assert!(result.is_some());
        let (consumed, p1, p2) = result.unwrap();
        assert_eq!(p1, 123);
        assert!(p2.is_none());
        assert_eq!(consumed, 4);
    }

    #[test]
    fn test_get_csi_params_two() {
        let buf = [0x34, 0x35, 0x3B, 0x36, 0x37, 0x20]; // "45;67" + space
        let result = get_csi_params(&buf);
        assert!(result.is_some());
        let (consumed, p1, p2) = result.unwrap();
        assert_eq!(p1, 45);
        assert_eq!(p2, Some(67));
        assert_eq!(consumed, 6);
    }

    #[test]
    fn test_get_csi_params_zero() {
        let buf = [0x30, 0x20];
        let result = get_csi_params(&buf);
        assert!(result.is_some());
        let (_, p1, p2) = result.unwrap();
        assert_eq!(p1, 0);
        assert!(p2.is_none());
    }

    #[test]
    fn test_get_csi_params_invalid() {
        let buf = [0x31, 0x32, 0x58];
        let result = get_csi_params(&buf);
        assert!(result.is_none());
    }

    #[test]
    fn test_get_csi_params_empty_digits() {
        let buf = [0x20];
        let result = get_csi_params(&buf);
        assert!(result.is_some());
        let (_, p1, _) = result.unwrap();
        assert_eq!(p1, 0);
    }

    // ---- strstr_ignorespace ----

    #[test]
    fn test_strstr_ignorespace_match() {
        assert!(strstr_ignorespace(b"hello", b"hello"));
    }

    #[test]
    fn test_strstr_ignorespace_with_spaces() {
        assert!(strstr_ignorespace(b"h l l o", b"h l l o")); // identical
        assert!(!strstr_ignorespace(b"hllo", b"h l l o")); // different lengths
    }

    #[test]
    fn test_strstr_ignorespace_no_match() {
        assert!(!strstr_ignorespace(b"hello", b"hxllo"));
    }

    #[test]
    fn test_strstr_ignorespace_empty_str2() {
        assert!(strstr_ignorespace(b"anything", b""));
    }

    #[test]
    fn test_strstr_ignorespace_str1_shorter() {
        assert!(!strstr_ignorespace(b"hi", b"hello"));
    }

    // ---- set_writing_format ----

    #[test]
    fn test_swf_init_format() {
        let mut ctx = IsdbSubContext::new();
        set_writing_format(&mut ctx, &[0x45, 0x20]);
        assert_eq!(
            ctx.current_state.layout_state.format,
            WritingFormat::Horizontal1920x1080
        );
    }

    #[test]
    fn test_swf_init_format_zero() {
        let mut ctx = IsdbSubContext::new();
        set_writing_format(&mut ctx, &[0x40, 0x20]);
        assert_eq!(
            ctx.current_state.layout_state.format,
            WritingFormat::HorizontalStdDensity
        );
    }

    #[test]
    fn test_swf_custom_format() {
        let mut ctx = IsdbSubContext::new();
        let arg = [0x30, 0x3B, 0x32, 0x3B, 0x41, 0x20];
        set_writing_format(&mut ctx, &arg);
        assert_eq!(
            ctx.current_state.layout_state.format,
            WritingFormat::HorizontalCustom
        );
        assert_eq!(ctx.nb_char, 0x41);
    }

    #[test]
    fn test_swf_custom_with_lines() {
        let mut ctx = IsdbSubContext::new();
        let arg = [0x30, 0x3B, 0x32, 0x3B, 0x41, 0x3B, 0x42, 0x20];
        set_writing_format(&mut ctx, &arg);
        assert_eq!(ctx.nb_char, 0x41);
        assert_eq!(ctx.nb_line, 0x42);
    }

    // ---- parse_caption_management_data ----

    #[test]
    fn test_mgmt_tmd_free() {
        let mut ctx = IsdbSubContext::new();
        let buf = [0x00, 0x00];
        let consumed = parse_caption_management_data(&mut ctx, &buf);
        assert_eq!(ctx.tmd, IsdbTmd::Free);
        assert_eq!(ctx.nb_lang, 0);
        assert_eq!(consumed, 2);
    }

    #[test]
    fn test_mgmt_tmd_realtime() {
        let mut ctx = IsdbSubContext::new();
        let buf = [0x40, 0x00];
        let consumed = parse_caption_management_data(&mut ctx, &buf);
        assert_eq!(ctx.tmd, IsdbTmd::RealTime);
        assert_eq!(ctx.nb_lang, 0);
        assert_eq!(consumed, 2);
    }

    #[test]
    fn test_mgmt_tmd_offset() {
        let mut ctx = IsdbSubContext::new();
        let buf = [0x80, 0x12, 0x34, 0x56, 0x78, 0x09, 0x00];
        let consumed = parse_caption_management_data(&mut ctx, &buf);
        assert_eq!(ctx.tmd, IsdbTmd::OffsetTime);
        assert_eq!(ctx.offset_time.hour, 12);
        assert_eq!(ctx.offset_time.min, 34);
        assert_eq!(ctx.offset_time.sec, 56);
        assert_eq!(ctx.offset_time.milli, 789);
        assert_eq!(consumed, 7);
    }

    #[test]
    fn test_mgmt_with_language() {
        let mut ctx = IsdbSubContext::new();
        let buf = [
            0x00, // TMD=0
            0x01, // nb_lang=1
            0x0C, // dmf=0x0C (triggers dc read)
            b'p', b'o', b'r', // language "por"
            0x03, // rollup_mode = (0x03 & 0x03) != 0 = true
        ];
        let consumed = parse_caption_management_data(&mut ctx, &buf);
        assert_eq!(ctx.nb_lang, 1);
        assert_eq!(ctx.dmf, 0x0C);
        assert_eq!(ctx.dc, b'p');
        assert!(ctx.current_state.rollup_mode);
        assert_eq!(consumed, 6);
    }

    #[test]
    fn test_mgmt_rollup_off() {
        let mut ctx = IsdbSubContext::new();
        let buf = [
            0x00, // TMD=0
            0x01, // nb_lang=1
            0x03, // dmf=0x03
            b'e', b'n', b'g', // lang
            0x00, // rollup_mode = 0
        ];
        parse_caption_management_data(&mut ctx, &buf);
        assert!(!ctx.current_state.rollup_mode);
    }

    // ---- WritingFormat::is_horizontal ----

    #[test]
    fn test_is_horizontal() {
        assert!(WritingFormat::HorizontalStdDensity.is_horizontal());

        assert!(WritingFormat::HorizontalHighDensity.is_horizontal());

        assert!(WritingFormat::HorizontalWesternLang.is_horizontal());
        assert!(WritingFormat::Horizontal1920x1080.is_horizontal());
        assert!(WritingFormat::Horizontal960x540.is_horizontal());
        assert!(WritingFormat::Horizontal720x480.is_horizontal());
        assert!(WritingFormat::Horizontal1280x720.is_horizontal());
        assert!(WritingFormat::HorizontalCustom.is_horizontal());

        assert!(!WritingFormat::VerticalStdDensity.is_horizontal());
        assert!(!WritingFormat::Vertical1920x1080.is_horizontal());
        assert!(!WritingFormat::None.is_horizontal());
    }
}
