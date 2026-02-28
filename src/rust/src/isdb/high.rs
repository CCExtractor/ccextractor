//! High-level parsers for ISDB subtitle data groups and caption statements.
//!
//! Entry points for parsing PES-level ISDB subtitle data, routing through
//! data groups, caption statements, data units, and statement bodies.
//!
//! # Conversion Guide
//!
//! | C (ccx_decoders_isdb.c)              | Rust                                     |
//! |--------------------------------------|------------------------------------------|
//! | `parse_statement`                    | [`parse_statement`]                      |
//! | `parse_data_unit`                    | [`parse_data_unit`]                      |
//! | `parse_caption_statement_data`       | [`parse_caption_statement_data`]         |
//! | `isdb_parse_data_group`              | [`isdb_parse_data_group`]                |

use crate::isdb::leaf::{parse_caption_management_data, rb16, rb24};
use crate::isdb::mid::{append_char, get_text, parse_command};
use crate::isdb::types::{IsdbSubContext, IsdbSubtitleData};

/// Parse a statement body; dispatches bytes as commands or characters
/// Return:  0 on success, -1 on error
pub fn parse_statement(ctx: &mut IsdbSubContext, buf: &[u8], size: usize) -> i32 {
    let mut pos: usize = 0;
    let end = size.min(buf.len());

    while pos < end {
        let code = (buf[pos] & 0xf0) >> 4;
        let code_lo = buf[pos] & 0x0f;

        let ret: usize;

        if code <= 0x1 {
            // Control codes C0/C1
            ret = parse_command(ctx, &buf[pos..]);
        } else if code == 0x2 && code_lo == 0x0 {
            // Special case: SP (space)
            append_char(ctx, buf[pos]);
            ret = 1;
        } else if code == 0x7 && code_lo == 0xF {
            // Special case: DEL
            // TODO: DEL should have block in fg color
            append_char(ctx, buf[pos]);
            ret = 1;
        } else if code <= 0x7 {
            // GL area — printable character
            append_char(ctx, buf[pos]);
            ret = 1;
        } else if code <= 0x9 {
            // Control codes C0/C1 extended
            ret = parse_command(ctx, &buf[pos..]);
        } else if code == 0xA && code_lo == 0x0 {
            // Special case: 10/0
            // TODO handle
            ret = 1;
        } else if code == 0xF && code_lo == 0xF {
            // Special case: 15/15
            // TODO handle
            ret = 1;
        } else {
            // GR area — printable character
            append_char(ctx, buf[pos]);
            ret = 1;
        }

        if ret == 0 {
            break;
        }
        pos += ret;
    }
    0
}

/// Parse a data unit
/// Returns: 0 on success, -1 on error
pub fn parse_data_unit(ctx: &mut IsdbSubContext, buf: &[u8]) -> i32 {
    if buf.len() < 5 {
        return -1;
    }

    // Skip unit separator (1 byte)
    let unit_parameter = buf[1];
    let len = rb24(&buf[2..]) as usize;

    if unit_parameter == 0x20 {
        if buf.len() >= 5 + len {
            parse_statement(ctx, &buf[5..], len);
        } else {
            parse_statement(ctx, &buf[5..], buf.len() - 5);
        }
    }
    0
}

/// Parse caption statement data
/// Returns: IsdbSubtitleData if text was produced, else None
pub fn parse_caption_statement_data(
    ctx: &mut IsdbSubContext,
    buf: &[u8],
) -> Option<IsdbSubtitleData> {
    if buf.is_empty() {
        return None;
    }

    let mut pos: usize = 0;

    let tmd = buf[pos] >> 6;
    pos += 1;

    // Skip timing data
    if tmd == 1 || tmd == 2 {
        pos += 5;
    }

    if pos + 3 > buf.len() {
        return None;
    }

    pos += 3;

    if pos >= buf.len() {
        return None;
    }

    let ret = parse_data_unit(ctx, &buf[pos..]);
    if ret < 0 {
        return None;
    }

    // Copy data if it's there in buffer
    let text = get_text(ctx);
    if text.is_empty() {
        return None;
    }

    let start_time = ctx.prev_timestamp.unwrap_or(ctx.timestamp);
    let mut end_time = ctx.timestamp;
    if start_time == end_time {
        end_time += 2;
    }
    ctx.prev_timestamp = Some(ctx.timestamp);

    Some(IsdbSubtitleData {
        text,
        start_time,
        end_time,
    })
}

/// Parse an ISDB data group
/// Returns: (bytes_consumed, optional on IsdbSubtitleData)
//  Acc to http://www.bocra.org.bw/sites/default/files/documents/Appendix%201%20-%20Operational%20Guideline%20for%20ISDB-Tbw.pdf
//  In table AP8-1 there are modification to ARIB TR-B14 in volume 3 Section 2 4.4.1 character encoding is UTF-8
//  instead of 8 bit character, just now we don't have any means to detect which country this video is
//  therefore we have hardcoded UTF-8 as encoding
pub fn isdb_parse_data_group(
    ctx: &mut IsdbSubContext,
    buf: &[u8],
) -> (i32, Option<IsdbSubtitleData>) {
    if buf.len() < 6 {
        return (-1, None);
    }

    let mut pos: usize = 0;

    let id = buf[pos] >> 2;
    log::debug!("ISDB (Data group) id: {}", id);

    if (id >> 4) == 0 {
        log::debug!("ISDB group A");
    }
    // else if (id >> 4) == 0 { // TODO : FIX this as C code has similar behaviour
    //     log::debug!("ISDB group B");
    // }

    pos += 1;

    // Skip link_number and last_link_number
    log::debug!(
        "ISDB (Data group) link_number {} last_link_number {}",
        buf[pos],
        buf[pos + 1]
    );
    pos += 2;

    if pos + 2 > buf.len() {
        return (-1, None);
    }

    let group_size = rb16(&buf[pos..]) as usize;
    pos += 2;
    log::debug!("ISDB (Data group) group_size {}", group_size);

    match ctx.prev_timestamp {
        None => ctx.prev_timestamp = Some(ctx.timestamp),
        Some(prev) if prev > ctx.timestamp => ctx.prev_timestamp = Some(ctx.timestamp),
        _ => {}
    }

    let mut subtitle = None;
    let data_end = (pos + group_size).min(buf.len());

    if (id & 0x0F) == 0 {
        // Caption management
        log::debug!("ISDB caption management data");
        parse_caption_management_data(ctx, &buf[pos..data_end]);
    } else if (id & 0x0F) < 8 {
        // Caption statement data
        log::debug!("ISDB {} language", id);
        subtitle = parse_caption_statement_data(ctx, &buf[pos..data_end]);
    }

    pos += group_size;

    // Skip CRC (2 bytes) (TODO : to be checked)
    pos += 2;

    (pos as i32, subtitle)
}

#[cfg(test)]
mod tests {
    use crate::isdb::high::*;
    use crate::isdb::leaf::*;
    use crate::isdb::mid::*;
    use crate::isdb::types::*;

    // ---- parse_statement ----

    #[test]
    fn test_parse_statement_printable_chars() {
        let mut ctx = IsdbSubContext::new();
        // Bytes 0x21-0x7E are printable GL characters
        let buf = [0x41, 0x42, 0x43]; // 'A', 'B', 'C'
        let ret = parse_statement(&mut ctx, &buf, buf.len());
        assert_eq!(ret, 0);
        assert_eq!(ctx.text_list.len(), 1);
        assert_eq!(ctx.text_list[0].buf, vec![0x41, 0x42, 0x43]);
    }

    #[test]
    fn test_parse_statement_space() {
        let mut ctx = IsdbSubContext::new();
        // 0x20 = SP (space), special case
        let buf = [0x20];
        parse_statement(&mut ctx, &buf, buf.len());
        assert_eq!(ctx.text_list.len(), 1);
        assert_eq!(ctx.text_list[0].buf, vec![0x20]);
    }

    #[test]
    fn test_parse_statement_command() {
        let mut ctx = IsdbSubContext::new();
        // 0x0D = APR (code_hi=0x00, code_lo=0xD) — a control command
        // followed by printable char 0x41
        let buf = [0x0D, 0x41];
        parse_statement(&mut ctx, &buf, buf.len());
        // APR consumes 1 byte, then 0x41 appends
        assert_eq!(ctx.text_list.len(), 1);
        assert_eq!(ctx.text_list[0].buf, vec![0x41]);
    }

    #[test]
    fn test_parse_statement_mixed() {
        let mut ctx = IsdbSubContext::new();
        let buf = [0x41, 0x00, 0x42];
        parse_statement(&mut ctx, &buf, buf.len());
        assert_eq!(ctx.text_list[0].buf, vec![0x41, 0x42]);
    }

    #[test]
    fn test_parse_statement_size_limit() {
        let mut ctx = IsdbSubContext::new();
        let buf = [0x41, 0x42, 0x43, 0x44];
        parse_statement(&mut ctx, &buf, 2);
        assert_eq!(ctx.text_list[0].buf, vec![0x41, 0x42]);
    }

    #[test]
    fn test_parse_statement_gr_chars() {
        let mut ctx = IsdbSubContext::new();
        let buf = [0xA1, 0xB2, 0xC3];
        parse_statement(&mut ctx, &buf, buf.len());
        assert_eq!(ctx.text_list[0].buf, vec![0xA1, 0xB2, 0xC3]);
    }

    // ---- parse_data_unit ----

    #[test]
    fn test_parse_data_unit_statement_body() {
        let mut ctx = IsdbSubContext::new();
        let buf = [
            0x1F, // unit separator
            0x20, // unit_parameter = statement body
            0x00, 0x00, 0x03, // len = 3
            0x41, 0x42, 0x43, // "ABC"
        ];
        let ret = parse_data_unit(&mut ctx, &buf);
        assert_eq!(ret, 0);
        assert_eq!(ctx.text_list[0].buf, vec![0x41, 0x42, 0x43]);
    }

    #[test]
    fn test_parse_data_unit_unknown_type() {
        let mut ctx = IsdbSubContext::new();
        let buf = [
            0x1F, // unit separator
            0x30, // unknown unit_parameter
            0x00, 0x00, 0x01, // len = 1
            0x41,
        ];
        let ret = parse_data_unit(&mut ctx, &buf);
        assert_eq!(ret, 0);
        assert!(ctx.text_list.is_empty()); // nothing parsed
    }

    #[test]
    fn test_parse_data_unit_too_short() {
        let mut ctx = IsdbSubContext::new();
        let buf = [0x1F, 0x20]; // too short for header
        let ret = parse_data_unit(&mut ctx, &buf);
        assert_eq!(ret, -1);
    }

    // ---- parse_caption_statement_data ----

    #[test]
    fn test_caption_statement_data_basic() {
        let mut ctx = IsdbSubContext::new();
        ctx.current_state.rollup_mode = true;
        ctx.cfg_no_rollup = false;
        ctx.timestamp = 1000;
        ctx.prev_timestamp = Some(500);

        let buf = [
            0x00, // tmd=0
            0x00, 0x00, 0x08, // len=8
            0x1F, // unit separator
            0x20, // statement body
            0x00, 0x00, 0x03, // unit len=3
            0x48, 0x69, 0x21, // "Hi!"
        ];

        let result = parse_caption_statement_data(&mut ctx, &buf);
        assert!(result.is_some());
        let sub = result.unwrap();
        assert_eq!(sub.text, b"Hi!\n\r");
        assert_eq!(sub.start_time, 500);
        assert_eq!(sub.end_time, 1000);
    }

    #[test]
    fn test_caption_statement_data_with_timing() {
        let mut ctx = IsdbSubContext::new();
        ctx.current_state.rollup_mode = true;
        ctx.cfg_no_rollup = false;
        ctx.timestamp = 2000;
        ctx.prev_timestamp = Some(1000);

        // skip 5 bytes timing
        let buf = [
            0x40, // tmd=1
            0x00, 0x00, 0x00, 0x00, 0x00, // 5 bytes timing data
            0x00, 0x00, 0x08, // len=8
            0x1F, // unit separator
            0x20, // statement body
            0x00, 0x00, 0x03, // unit len=3
            0x41, 0x42, 0x43, // "ABC"
        ];

        let result = parse_caption_statement_data(&mut ctx, &buf);
        assert!(result.is_some());
        let sub = result.unwrap();
        assert_eq!(sub.text, b"ABC\n\r");
    }

    #[test]
    fn test_caption_statement_data_no_text() {
        let mut ctx = IsdbSubContext::new();
        ctx.current_state.rollup_mode = true;
        ctx.cfg_no_rollup = false;

        // produces no text
        let buf = [
            0x00, // tmd=0
            0x00, 0x00, 0x06, // len=6
            0x1F, // unit separator
            0x30, // unknown type
            0x00, 0x00, 0x01, // len=1
            0x41,
        ];

        let result = parse_caption_statement_data(&mut ctx, &buf);
        assert!(result.is_none());
    }

    #[test]
    fn test_caption_statement_data_equal_timestamps() {
        let mut ctx = IsdbSubContext::new();
        ctx.current_state.rollup_mode = true;
        ctx.cfg_no_rollup = false;
        ctx.timestamp = 500;
        ctx.prev_timestamp = Some(500);

        let buf = [
            0x00, 0x00, 0x00, 0x08, 0x1F, 0x20, 0x00, 0x00, 0x03, 0x41, 0x42, 0x43,
        ];

        let result = parse_caption_statement_data(&mut ctx, &buf);
        assert!(result.is_some());
        let sub = result.unwrap();
        // When start == end, end_time gets +2
        assert_eq!(sub.end_time, 502);
    }

    // ---- isdb_parse_data_group ----

    #[test]
    fn test_data_group_management() {
        let mut ctx = IsdbSubContext::new();
        let buf = [
            0x00, // id=0 (management), version=0
            0x00, // link_number
            0x00, // last_link_number
            0x00, 0x02, // group_size=2
            0x00, // TMD=0 (Free)
            0x00, // nb_lang=0
            0x00, 0x00, // CRC (2 bytes)
        ];

        let (consumed, subtitle) = isdb_parse_data_group(&mut ctx, &buf);
        assert!(subtitle.is_none());
        assert_eq!(ctx.tmd, IsdbTmd::Free);
        assert_eq!(consumed, 9); // 5 header + 2 data + 2 CRC
    }

    #[test]
    fn test_data_group_caption_statement() {
        let mut ctx = IsdbSubContext::new();
        ctx.current_state.rollup_mode = true;
        ctx.cfg_no_rollup = false;
        ctx.timestamp = 1000;
        ctx.prev_timestamp = Some(500);

        // id=1 (caption statement, lang 1)
        let caption_data = [
            0x00, // tmd=0
            0x00, 0x00, 0x08, // len=8
            0x1F, 0x20, // unit: separator + statement body
            0x00, 0x00, 0x03, // unit len=3
            0x58, 0x59, 0x5A, // "XYZ"
        ];
        let group_size = caption_data.len() as u16;

        let mut buf = vec![
            0x04, // id=1 (0x04 >> 2 = 1)
            0x00, // link_number
            0x00, // last_link_number
        ];
        buf.extend_from_slice(&group_size.to_be_bytes());
        buf.extend_from_slice(&caption_data);
        buf.extend_from_slice(&[0x00, 0x00]); // CRC

        let (consumed, subtitle) = isdb_parse_data_group(&mut ctx, &buf);
        assert!(consumed > 0);
        assert!(subtitle.is_some());
        let sub = subtitle.unwrap();
        assert_eq!(sub.text, b"XYZ\n\r");
    }

    #[test]
    fn test_data_group_too_short() {
        let ctx = &mut IsdbSubContext::new();
        let buf = [0x00, 0x00];
        let (consumed, subtitle) = isdb_parse_data_group(ctx, &buf);
        assert_eq!(consumed, -1);
        assert!(subtitle.is_none());
    }

    #[test]
    fn test_data_group_fixes_timestamp() {
        let mut ctx = IsdbSubContext::new();
        ctx.timestamp = 100;
        ctx.prev_timestamp = Some(500);

        let buf = [
            0x00, // management
            0x00, 0x00, 0x00, 0x02, // group_size=2
            0x00, 0x00, // mgmt data
            0x00, 0x00, // CRC
        ];

        isdb_parse_data_group(&mut ctx, &buf);
        // prev_timestamp should be fixed to current
        assert_eq!(ctx.prev_timestamp, Some(100));
    }
}
