//! Mid-level control and command processing for ISDB subtitle decoding.
//!
//! # Conversion Guide
//!
//! | C (ccx_decoders_isdb.c)        | Rust                             |
//! |--------------------------------|----------------------------------|
//! | `set_position`                 | [`set_position`]                 |
//! | `append_char`                  | [`append_char`]                  |
//! | `get_text`                     | [`get_text`]                     |
//! | `parse_csi`                    | [`parse_csi`]                    |
//! | `parse_command`                | [`parse_command`]                |

use crate::isdb::leaf::{get_csi_params, move_penpos, set_writing_format, strstr_ignorespace};
use crate::isdb::types::{IsdbCcComposition, IsdbSubContext, IsdbText, DEFAULT_CLUT};

pub fn set_position(ctx: &mut IsdbSubContext, p1: u32, p2: u32) {
    let ls = &ctx.current_state.layout_state;
    let is_horizontal = ls.format.is_horizontal();

    let (cw, ch) = if is_horizontal {
        (
            (ls.font_size + ls.cell_spacing.col) * ls.font_scale.fscx / 100,
            (ls.font_size + ls.cell_spacing.row) * ls.font_scale.fscy / 100,
        )
    } else {
        (
            (ls.font_size + ls.cell_spacing.col) * ls.font_scale.fscy / 100,
            (ls.font_size + ls.cell_spacing.row) * ls.font_scale.fscx / 100,
        )
    };

    let col = p2 as i32 * cw;
    let row = if is_horizontal {
        p1 as i32 * ch + ch
        // pen position is at bottom left
    } else {
        p1 as i32 * ch + ch / 2
        // pen position is at upper center,
        // but in -90deg rotated coordinates, it is at middle left.
    };

    move_penpos(ctx, col, row);
}

/// Append a character byte to the appropriate text node.
/// Finds or creates a text node at the current cursor line position.
pub fn append_char(ctx: &mut IsdbSubContext, ch: u8) -> i32 {
    let is_horiz = ctx.current_state.layout_state.format.is_horizontal();
    let cursor = ctx.current_state.layout_state.cursor_pos;

    // Space taken by character
    let csp = if is_horiz {
        ctx.current_state.layout_state.font_size * ctx.current_state.layout_state.font_scale.fscx
            / 100
    } else {
        ctx.current_state.layout_state.font_size * ctx.current_state.layout_state.font_scale.fscy
            / 100
    };

    // Current Line Position
    let cur_lpos = if is_horiz { cursor.x } else { cursor.y };

    // Find existing node or insertion point (list is sorted by line position)
    let mut found_idx: Option<usize> = None;
    let mut insert_at: Option<usize> = None;

    for (i, text) in ctx.text_list.iter().enumerate() {
        // Text Line Position
        let text_lpos = if is_horiz { text.pos.x } else { text.pos.y };
        if text_lpos == cur_lpos {
            found_idx = Some(i);
            break;
        } else if text_lpos > cur_lpos {
            // Allocate Text here so that list is always sorted
            insert_at = Some(i);
            break;
        }
    }

    let idx = if let Some(i) = found_idx {
        i
    } else if let Some(i) = insert_at {
        ctx.text_list.insert(i, IsdbText::new(cursor));
        i
    } else {
        ctx.text_list.push(IsdbText::new(cursor));
        ctx.text_list.len() - 1
    };

    // Check backward movement — if cursor is behind text pos, reset text
    if is_horiz {
        if ctx.current_state.layout_state.cursor_pos.y < ctx.text_list[idx].pos.y {
            ctx.text_list[idx].pos.y = ctx.current_state.layout_state.cursor_pos.y;
            ctx.text_list[idx].buf.clear();
        }
        ctx.current_state.layout_state.cursor_pos.y += csp;
        ctx.text_list[idx].pos.y += csp;
    } else {
        if ctx.current_state.layout_state.cursor_pos.y < ctx.text_list[idx].pos.y {
            ctx.text_list[idx].pos.y = ctx.current_state.layout_state.cursor_pos.y;
            ctx.text_list[idx].buf.clear();
        }
        ctx.current_state.layout_state.cursor_pos.x += csp;
        ctx.text_list[idx].pos.x += csp;
    }

    ctx.text_list[idx].buf.push(ch);
    1
}

pub fn get_text(ctx: &mut IsdbSubContext) -> Vec<u8> {
    let mut output: Vec<u8> = Vec::new();

    // deduplication path: if no_rollup enabled OR rollup_mode is off
    // og C code says : Abhinav95: Forcing -noru to perform deduplication even if stream doesn't honor it
    if ctx.cfg_no_rollup || (ctx.cfg_no_rollup == ctx.current_state.rollup_mode) {
        // firt call: copy text_list → buffered_text, return empty
        if ctx.buffered_text.is_empty() {
            for text in &ctx.text_list {
                if !text.buf.is_empty() {
                    let mut clone = IsdbText::new(text.pos);
                    clone.buf = text.buf.clone();
                    ctx.buffered_text.push(clone);
                }
            }
            return output;
        }

        // update buffered_text with new/changed entries from text_list
        for text in &ctx.text_list {
            let mut found = false;
            for sb in &mut ctx.buffered_text {
                if strstr_ignorespace(&text.buf, &sb.buf) {
                    found = true;
                    // See if complete string is there if not update that string
                    if !strstr_ignorespace(&sb.buf, &text.buf) {
                        sb.buf = text.buf.clone();
                    }
                    break;
                }
            }
            if !found {
                let mut new_sb = IsdbText::new(text.pos);
                new_sb.buf = text.buf.clone();
                ctx.buffered_text.push(new_sb);
            }
        }

        // flush buffered entries not found in text_list
        let text_list_ref: Vec<Vec<u8>> = ctx.text_list.iter().map(|t| t.buf.clone()).collect();
        ctx.buffered_text.retain(|sb| {
            if sb.buf.is_empty() {
                return true;
            }
            let found = text_list_ref.iter().any(|t| strstr_ignorespace(t, &sb.buf));
            if !found {
                output.extend_from_slice(&sb.buf);
                output.push(b'\n');
                output.push(b'\r');
                return false; // remove from buffered_text
            }
            true
        });
    } else {
        // simple path: output all text entries
        for text in &mut ctx.text_list {
            if !text.buf.is_empty() {
                output.extend_from_slice(&text.buf);
                output.push(b'\n');
                output.push(b'\r');
                text.buf.clear();
            }
        }
    }

    output
}

/// parse a CSI (Control Sequence Introducer) command
/// Returns the number of bytes consumed.
pub fn parse_csi(ctx: &mut IsdbSubContext, buf: &[u8]) -> usize {
    let mut pos: usize = 0;
    let mut arg = [0u8; 10];
    let mut i: usize = 0;

    // copy bytes into arg until 0x20 terminator
    while pos < buf.len() && buf[pos] != 0x20 {
        if i >= 9 {
            log::debug!("Unexpected CSI: too long");
            break;
        }
        arg[i] = buf[pos];
        pos += 1;
        i += 1;
    }

    // Include terminating 0x20 in arg
    if i < 10 && pos < buf.len() {
        arg[i] = buf[pos];
        pos += 1;
    }

    if pos >= buf.len() {
        return pos;
    }

    let cmd = buf[pos];

    match cmd {
        // SWF - Set Writing Format
        0x53 => {
            log::debug!("Command:CSI: SWF");
            set_writing_format(ctx, &arg);
        }
        // CCC - Composite Character Composition
        0x54 => {
            log::debug!("Command:CSI: CCC");
            if let Some((_, p1, _)) = get_csi_params(&arg) {
                ctx.current_state.layout_state.ccc = IsdbCcComposition::from_i32(p1 as i32);
            }
        }
        // SDF - Set Display Format
        0x56 => {
            if let Some((_, p1, Some(p2))) = get_csi_params(&arg) {
                ctx.current_state.layout_state.display_area.w = p1 as i32;
                ctx.current_state.layout_state.display_area.h = p2 as i32;
            }
            log::debug!("Command:CSI: SDF");
        }
        // SSM - Character composition dot designation
        0x57 => {
            if let Some((_, p1, _)) = get_csi_params(&arg) {
                ctx.current_state.layout_state.font_size = p1 as i32;
            }
            log::debug!("Command:CSI: SSM");
        }
        // SHS - Set Horizontal Spacing
        0x58 => {
            if let Some((_, p1, _)) = get_csi_params(&arg) {
                ctx.current_state.layout_state.cell_spacing.col = p1 as i32;
            }
            log::debug!("Command:CSI: SHS");
        }
        // SVS - Set Vertical Spacing
        0x59 => {
            if let Some((_, p1, _)) = get_csi_params(&arg) {
                ctx.current_state.layout_state.cell_spacing.row = p1 as i32;
            }
            log::debug!("Command:CSI: SVS");
        }
        // SDP - Set Display Position
        0x5F => {
            if let Some((_, p1, Some(p2))) = get_csi_params(&arg) {
                ctx.current_state.layout_state.display_area.x = p1 as i32;
                ctx.current_state.layout_state.display_area.y = p2 as i32;
            }
            log::debug!("Command:CSI: SDP");
        }
        // ACPS - Active Coordinate Position Set
        0x61 => {
            log::debug!("Command:CSI: ACPS");
            if let Some((_, p1, Some(_))) = get_csi_params(&arg) {
                ctx.current_state.layout_state.acps[0] = p1 as i32;
                ctx.current_state.layout_state.acps[1] = p1 as i32;
                // C code sets both to p1
            }
        }
        // RCS - Raster Color Command
        0x6E => {
            if let Some((_, p1, _)) = get_csi_params(&arg) {
                let idx = (ctx.current_state.clut_high_idx as usize) << 4 | (p1 as usize);
                if idx < DEFAULT_CLUT.len() {
                    ctx.current_state.raster_color = DEFAULT_CLUT[idx];
                }
            }
            log::debug!("Command:CSI: RCS");
        }
        _ => {
            log::debug!("Command:CSI: Unknown command 0x{:x}", cmd);
        }
    }

    pos += 1; // skip command byte
    pos
}

/// parse control command byte seq
/// Return number of bytes consumed
pub fn parse_command(ctx: &mut IsdbSubContext, buf: &[u8]) -> usize {
    if buf.is_empty() {
        return 0;
    }

    let code_lo = buf[0] & 0x0f;
    let code_hi = (buf[0] & 0xf0) >> 4;
    let mut pos: usize = 1; // skip command byte

    match code_hi {
        0x00 => match code_lo {
            /* NUL Control code, which can be added or deleted without effecting to
            information content. */
            0x0 => log::debug!("Command: NUL"),
            /* BEL Control code used when calling attention (alarm or signal) */
            // TODO add bell character here
            0x7 => log::debug!("Command: BEL"),
            /*
             *  APB: Active position goes backward along character path in the length of
             *	character path of character field. When the reference point of the character
             *	field exceeds the edge of display area by this movement, move in the
             *	opposite side of the display area along the character path of the active
             * 	position, for active position up.
             */
            0x8 => log::debug!("Command: APB"),
            /*
             *  APF: Active position goes forward along character path in the length of
             *	character path of character field. When the reference point of the character
             *	field exceeds the edge of display area by this movement, move in the
             * 	opposite side of the display area along the character path of the active
             *	position, for active position down.
             */
            0x9 => log::debug!("Command: APF"),
            /*
             *  APD: Moves to next line along line direction in the length of line direction of
             *	the character field. When the reference point of the character field exceeds
             *	the edge of display area by this movement, move to the first line of the
             *	display area along the line direction.
             */
            0xA => log::debug!("Command: APD"),
            /*
             * APU: Moves to the previous line along line direction in the length of line
             *	direction of the character field. When the reference point of the character
             *	field exceeds the edge of display area by this movement, move to the last
             *	line of the display area along the line direction.
             */
            0xB => log::debug!("Command: APU"),
            /*
             * CS: Display area of the display screen is erased.
             * Specs does not say clearly about whether we have to clear cursor
             * Need Samples to see whether CS is called after pen move or before it
             */
            0xC => log::debug!("Command: CS clear Screen"),
            /* APR: Active position down is made, moving to the first position of the same
             *	line.
             */
            0xD => log::debug!("Command: APR"),
            /* LS1: Code to invoke character code set. */
            0xE => log::debug!("Command: LS1"),
            /* LS0: Code to invoke character code set. */
            0xF => log::debug!("Command: LS0"),
            /* Verify the new version of specs or packet is corrupted */
            _ => log::debug!("Command: Unknown"),
        },
        0x01 => {
            match code_lo {
                /*
                 * PAPF: Active position forward is made in specified times by parameter P1 (1byte).
                 *	Parameter P1 shall be within the range of 04/0 to 07/15 and time shall be
                 *	specified within the range of 0 to 63 in binary value of 6-bit from b6 to b1.
                 *	(b8 and b7 are not used.)
                 */
                0x6 => log::debug!("Command: PAPF"),
                /*
                 * CAN: From the current active position to the end of the line is covered with
                 *	background colour in the width of line direction in the current character
                 *	field. Active position is not moved.
                 */
                0x8 => log::debug!("Command: CAN"),
                /* SS2: Code to invoke character code set. */
                0x9 => log::debug!("Command: SS2"),
                /* ESC:Code for code extension. */
                0xB => log::debug!("Command: ESC"),
                /* APS(Active Position Set): Specified times of active position down is made by P1 (1 byte) of the first
                 *	parameter in line direction length of character field from the first position
                 *	of the first line of the display area. Then specified times of active position
                 *	forward is made by the second parameter P2 (1 byte) in the character path
                 *	length of character field. Each parameter shall be within the range of 04/0
                 *	to 07/15 and specify time within the range of 0 to 63 in binary value of 6-
                 *	bit from b6 to b1. (b8 and b7 are not used.)
                 */
                0xC => {
                    log::debug!("Command: APS");
                    if pos + 1 < buf.len() {
                        set_position(ctx, (buf[pos] & 0x3F) as u32, (buf[pos + 1] & 0x3F) as u32);
                        pos += 2;
                    }
                }
                /* SS3: Code to invoke character code set. */
                0xD => log::debug!("Command: SS3"),
                /*
                 * RS: It is information division code and declares identification and
                 * 	introduction of data header.
                 */
                0xE => log::debug!("Command: RS"),
                /*
                 * US: It is information division code and declares identification and
                 *	introduction of data unit.
                 */
                0xF => log::debug!("Command: US"),
                /* Verify the new version of specs or packet is corrupted */
                _ => log::debug!("Command: Unknown"),
            }
        }

        0x08 => {
            match code_lo {
                // BKF .. WHF - foreground color
                /* BKF */
                /* RDF */
                /* GRF */
                /* YLF */
                /* BLF 	*/
                /* MGF */
                /* CNF */
                /* WHF */
                0x0..=0x7 => {
                    log::debug!(
                        "Command: Foreground color (0x{:X})",
                        DEFAULT_CLUT[code_lo as usize]
                    );
                    ctx.current_state.fg_color = DEFAULT_CLUT[code_lo as usize];
                }
                // SSZ - Small size
                0x8 => {
                    log::debug!("Command: SSZ");
                    ctx.current_state.layout_state.font_scale.fscx = 50;
                    ctx.current_state.layout_state.font_scale.fscy = 50;
                }
                // MSZ - Medium size
                0x9 => {
                    log::debug!("Command: MSZ");
                    ctx.current_state.layout_state.font_scale.fscx = 200;
                    ctx.current_state.layout_state.font_scale.fscy = 200;
                }
                // NSZ - Normal size
                0xA => {
                    log::debug!("Command: NSZ");
                    ctx.current_state.layout_state.font_scale.fscx = 100;
                    ctx.current_state.layout_state.font_scale.fscy = 100;
                }
                // SZX
                0xB => {
                    log::debug!("Command: SZX");
                    pos += 1;
                }
                // Verify the new version of specs or packet is corrupted
                _ => log::debug!("Command: Unknown"),
            }
        }
        0x09 => {
            match code_lo {
                // COL
                0x0 => {
                    // Palette Col
                    if pos < buf.len() {
                        if buf[pos] == 0x20 {
                            log::debug!("Command: COL: Set Clut");
                            pos += 1;
                            if pos < buf.len() {
                                ctx.current_state.clut_high_idx = buf[pos] & 0x0F;
                            }
                        } else if (buf[pos] & 0xF0) == 0x40 {
                            let ci = (buf[pos] & 0x0F) as usize;
                            log::debug!("Command: COL: Set Foreground 0x{:08X}", DEFAULT_CLUT[ci]);
                            ctx.current_state.fg_color = DEFAULT_CLUT[ci];
                        } else if (buf[pos] & 0xF0) == 0x50 {
                            let ci = (buf[pos] & 0x0F) as usize;
                            log::debug!("Command: COL: Set Background 0x{:08X}", DEFAULT_CLUT[ci]);
                            ctx.current_state.bg_color = DEFAULT_CLUT[ci];
                        } else if (buf[pos] & 0xF0) == 0x60 {
                            let ci = (buf[pos] & 0x0F) as usize;
                            log::debug!(
                                "Command: COL: Set Half Foreground 0x{:08X}",
                                DEFAULT_CLUT[ci]
                            );
                            ctx.current_state.hfg_color = DEFAULT_CLUT[ci];
                        } else if (buf[pos] & 0xF0) == 0x70 {
                            let ci = (buf[pos] & 0x0F) as usize;
                            log::debug!(
                                "Command: COL: Set Half Background 0x{:08X}",
                                DEFAULT_CLUT[ci]
                            );
                            ctx.current_state.hbg_color = DEFAULT_CLUT[ci];
                        }
                        pos += 1;
                    }
                }
                // FLC
                0x1 => {
                    log::debug!("Command: FLC");
                    pos += 1;
                }
                // CDC
                0x2 => {
                    log::debug!("Command: CDC");
                    pos += 3;
                }
                // POL
                0x3 => {
                    log::debug!("Command: POL");
                    pos += 1;
                }
                // WMM
                0x4 => {
                    log::debug!("Command: WMM");
                    pos += 3;
                }
                // MACRO
                0x5 => {
                    log::debug!("Command: MACRO");
                    pos += 1;
                }
                // HLC
                0x7 => {
                    log::debug!("Command: HLC");
                    pos += 1;
                }
                // RPC
                0x8 => {
                    log::debug!("Command: RPC");
                    pos += 1;
                }
                // SPL
                0x9 => log::debug!("Command: SPL"),
                // STL
                0xA => log::debug!("Command: STL"),
                // CSI Code for code system extension indicated
                0xB => {
                    let ret = parse_csi(ctx, &buf[pos..]);
                    pos += ret;
                }
                // TIME
                0xD => {
                    log::debug!("Command: TIME");
                    pos += 2;
                }
                // Verify the new version of specs or packet is corrupted
                _ => log::debug!("Command: Unknown"),
            }
        }
        _ => {}
    }

    pos
}

#[cfg(test)]
mod tests {
    use crate::isdb::mid::*;
    use crate::isdb::types::*;

    // ---- set_position ----

    #[test]
    fn test_set_position_horizontal() {
        let mut ctx = IsdbSubContext::new();
        set_position(&mut ctx, 1, 2);
        assert_eq!(ctx.current_state.layout_state.cursor_pos.x, 72);
        assert_eq!(ctx.current_state.layout_state.cursor_pos.y, 72);
    }

    #[test]
    fn test_set_position_vertical() {
        let mut ctx = IsdbSubContext::new();
        ctx.current_state.layout_state.format = WritingFormat::VerticalStdDensity;
        set_position(&mut ctx, 1, 2);
        assert_eq!(ctx.current_state.layout_state.cursor_pos.x, 54);
        assert_eq!(ctx.current_state.layout_state.cursor_pos.y, 72);
    }

    #[test]
    fn test_set_position_with_spacing() {
        let mut ctx = IsdbSubContext::new();
        ctx.current_state.layout_state.cell_spacing.col = 4;
        ctx.current_state.layout_state.cell_spacing.row = 4;
        set_position(&mut ctx, 1, 1);
        assert_eq!(ctx.current_state.layout_state.cursor_pos.x, 80);
        assert_eq!(ctx.current_state.layout_state.cursor_pos.y, 40);
    }

    // ---- append_char ----

    #[test]
    fn test_append_char_creates_node() {
        let mut ctx = IsdbSubContext::new();
        assert!(ctx.text_list.is_empty());
        append_char(&mut ctx, b'A');
        assert_eq!(ctx.text_list.len(), 1);
        assert_eq!(ctx.text_list[0].buf, vec![b'A']);
    }

    #[test]
    fn test_append_char_multiple() {
        let mut ctx = IsdbSubContext::new();
        append_char(&mut ctx, b'H');
        append_char(&mut ctx, b'i');
        assert_eq!(ctx.text_list.len(), 1);
        assert_eq!(ctx.text_list[0].buf, vec![b'H', b'i']);
    }

    #[test]
    fn test_append_char_advances_cursor() {
        let mut ctx = IsdbSubContext::new();
        let initial_y = ctx.current_state.layout_state.cursor_pos.y;
        append_char(&mut ctx, b'X');
        assert_eq!(ctx.current_state.layout_state.cursor_pos.y, initial_y + 36);
    }

    #[test]
    fn test_append_char_different_lines() {
        let mut ctx = IsdbSubContext::new();
        append_char(&mut ctx, b'A');
        // Move cursor to different line position
        ctx.current_state.layout_state.cursor_pos.x = 100;
        ctx.current_state.layout_state.cursor_pos.y = 0;
        append_char(&mut ctx, b'B');
        assert_eq!(ctx.text_list.len(), 2);
    }

    // ---- get_text ----

    #[test]
    fn test_get_text_simple() {
        let mut ctx = IsdbSubContext::new();
        ctx.text_list.push(IsdbText {
            buf: b"Hello".to_vec(),
            pos: IsdbPos { x: 0, y: 0 },
        });
        let result = get_text(&mut ctx);
        assert!(result.is_empty());
        assert_eq!(ctx.buffered_text.len(), 1);
    }

    #[test]
    fn test_get_text_rollup_mode() {
        let mut ctx = IsdbSubContext::new();
        ctx.current_state.rollup_mode = true;
        ctx.cfg_no_rollup = false;
        ctx.text_list.push(IsdbText {
            buf: b"Line1".to_vec(),
            pos: IsdbPos { x: 0, y: 0 },
        });
        let result = get_text(&mut ctx);
        assert_eq!(result, b"Line1\n\r");
        assert!(ctx.text_list[0].buf.is_empty()); // cleared after output
    }

    #[test]
    fn test_get_text_multiple_lines_rollup() {
        let mut ctx = IsdbSubContext::new();
        ctx.current_state.rollup_mode = true;
        ctx.cfg_no_rollup = false;
        ctx.text_list.push(IsdbText {
            buf: b"Line1".to_vec(),
            pos: IsdbPos { x: 0, y: 0 },
        });
        ctx.text_list.push(IsdbText {
            buf: b"Line2".to_vec(),
            pos: IsdbPos { x: 36, y: 0 },
        });
        let result = get_text(&mut ctx);
        assert_eq!(result, b"Line1\n\rLine2\n\r");
    }

    #[test]
    fn test_get_text_empty() {
        let mut ctx = IsdbSubContext::new();
        ctx.current_state.rollup_mode = true;
        ctx.cfg_no_rollup = false;
        let result = get_text(&mut ctx);
        assert!(result.is_empty());
    }

    // ---- parse_csi ----

    #[test]
    fn test_parse_csi_swf() {
        let mut ctx = IsdbSubContext::new();
        let buf = [0x35, 0x20, 0x53];
        let consumed = parse_csi(&mut ctx, &buf);
        assert_eq!(consumed, 3);
        assert_eq!(
            ctx.current_state.layout_state.format,
            WritingFormat::Horizontal1920x1080
        );
    }

    #[test]
    fn test_parse_csi_sdf() {
        let mut ctx = IsdbSubContext::new();
        let buf = [0x39, 0x36, 0x30, 0x3B, 0x35, 0x34, 0x30, 0x20, 0x56];
        let consumed = parse_csi(&mut ctx, &buf);
        assert_eq!(consumed, 9);
        assert_eq!(ctx.current_state.layout_state.display_area.w, 960);
        assert_eq!(ctx.current_state.layout_state.display_area.h, 540);
    }

    #[test]
    fn test_parse_csi_ssm() {
        let mut ctx = IsdbSubContext::new();
        let buf = [0x32, 0x34, 0x3B, 0x32, 0x34, 0x20, 0x57];
        let consumed = parse_csi(&mut ctx, &buf);
        assert_eq!(consumed, 7);
        assert_eq!(ctx.current_state.layout_state.font_size, 24);
    }

    #[test]
    fn test_parse_csi_shs() {
        let mut ctx = IsdbSubContext::new();
        // "4 " + SHS command (0x58)
        let buf = [0x34, 0x20, 0x58];
        let consumed = parse_csi(&mut ctx, &buf);
        assert_eq!(consumed, 3);
        assert_eq!(ctx.current_state.layout_state.cell_spacing.col, 4);
    }

    #[test]
    fn test_parse_csi_svs() {
        let mut ctx = IsdbSubContext::new();
        let buf = [0x32, 0x34, 0x20, 0x59];
        let consumed = parse_csi(&mut ctx, &buf);
        assert_eq!(consumed, 4);
        assert_eq!(ctx.current_state.layout_state.cell_spacing.row, 24);
    }

    #[test]
    fn test_parse_csi_unknown() {
        let mut ctx = IsdbSubContext::new();
        let buf = [0x30, 0x20, 0xFF];
        let consumed = parse_csi(&mut ctx, &buf);
        assert_eq!(consumed, 3); // still consumes the bytes
    }

    // ---- parse_command ----

    #[test]
    fn test_parse_command_nul() {
        let mut ctx = IsdbSubContext::new();
        let buf = [0x00];
        let consumed = parse_command(&mut ctx, &buf);
        assert_eq!(consumed, 1);
    }

    #[test]
    fn test_parse_command_aps() {
        let mut ctx = IsdbSubContext::new();
        let buf = [0x1C, 0x41, 0x42];
        let consumed = parse_command(&mut ctx, &buf);
        assert_eq!(consumed, 3);
        assert_eq!(ctx.current_state.layout_state.cursor_pos.x, 72);
        assert_eq!(ctx.current_state.layout_state.cursor_pos.y, 72);
    }

    #[test]
    fn test_parse_command_fg_color() {
        let mut ctx = IsdbSubContext::new();
        let buf = [0x82];
        let consumed = parse_command(&mut ctx, &buf);
        assert_eq!(consumed, 1);
        assert_eq!(ctx.current_state.fg_color, DEFAULT_CLUT[2]); // green
    }

    #[test]
    fn test_parse_command_ssz() {
        let mut ctx = IsdbSubContext::new();
        let buf = [0x88];
        parse_command(&mut ctx, &buf);
        assert_eq!(ctx.current_state.layout_state.font_scale.fscx, 50);
        assert_eq!(ctx.current_state.layout_state.font_scale.fscy, 50);
    }

    #[test]
    fn test_parse_command_msz() {
        let mut ctx = IsdbSubContext::new();
        let buf = [0x89]; // MSZ
        parse_command(&mut ctx, &buf);
        assert_eq!(ctx.current_state.layout_state.font_scale.fscx, 200);
        assert_eq!(ctx.current_state.layout_state.font_scale.fscy, 200);
    }

    #[test]
    fn test_parse_command_nsz() {
        let mut ctx = IsdbSubContext::new();
        ctx.current_state.layout_state.font_scale.fscx = 50;
        ctx.current_state.layout_state.font_scale.fscy = 50;
        let buf = [0x8A]; // NSZ
        parse_command(&mut ctx, &buf);
        assert_eq!(ctx.current_state.layout_state.font_scale.fscx, 100);
        assert_eq!(ctx.current_state.layout_state.font_scale.fscy, 100);
    }

    #[test]
    fn test_parse_command_col_foreground() {
        let mut ctx = IsdbSubContext::new();
        let buf = [0x90, 0x43];
        let consumed = parse_command(&mut ctx, &buf);
        assert_eq!(consumed, 2);
        assert_eq!(ctx.current_state.fg_color, DEFAULT_CLUT[3]);
    }

    #[test]
    fn test_parse_command_col_background() {
        let mut ctx = IsdbSubContext::new();
        let buf = [0x90, 0x52];
        let consumed = parse_command(&mut ctx, &buf);
        assert_eq!(consumed, 2);
        assert_eq!(ctx.current_state.bg_color, DEFAULT_CLUT[2]);
    }

    #[test]
    fn test_parse_command_col_set_clut() {
        let mut ctx = IsdbSubContext::new();
        let buf = [0x90, 0x20, 0x42];
        let consumed = parse_command(&mut ctx, &buf);
        assert_eq!(consumed, 3);
        assert_eq!(ctx.current_state.clut_high_idx, 2);
    }

    #[test]
    fn test_parse_command_time() {
        let mut ctx = IsdbSubContext::new();
        let buf = [0x9D, 0x00, 0x00];
        let consumed = parse_command(&mut ctx, &buf);
        assert_eq!(consumed, 3);
    }

    #[test]
    fn test_parse_command_csi() {
        let mut ctx = IsdbSubContext::new();
        let buf = [0x9B, 0x34, 0x20, 0x58];
        let consumed = parse_command(&mut ctx, &buf);
        assert_eq!(consumed, 4);
        assert_eq!(ctx.current_state.layout_state.cell_spacing.col, 4);
    }
}
