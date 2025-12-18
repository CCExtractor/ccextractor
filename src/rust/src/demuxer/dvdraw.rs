//! McPoodle DVD Raw format parser
//!
//! This module provides functionality to parse McPoodle's DVD raw caption format.
//! The format stores CEA-608 closed caption data in a specific binary structure.
//!
//! # Format Specification
//!
//! ```text
//! [DVD_HEADER][loop_marker][ff d1 d1 fe d2 d2]...
//!
//! DVD_HEADER: 00 00 01 B2 43 43 01 F8 (8 bytes, "CC" at bytes 4-5)
//!
//! Loop markers (vary by loop count):
//! - Loop 1: 8a (1 byte) - 5 caption pairs
//! - Loop 2: 8f (1 byte) - 8 caption pairs
//! - Loop 3: 16 or 16 fe (1-2 bytes) - 11 caption pairs
//! - Loop 4+: 1e or 1e fe (1-2 bytes) - 15 caption pairs
//!
//! Caption data:
//! - ff [byte1] [byte2] = Field 1 (CC1) data
//! - fe [byte1] [byte2] = Field 2 (CC2/XDS) data
//! ```

/// DVD raw format header: 00 00 01 B2 43 43 01 F8
pub const DVD_HEADER: [u8; 8] = [0x00, 0x00, 0x01, 0xB2, 0x43, 0x43, 0x01, 0xF8];

/// Loop marker for loop 1 (5 caption pairs)
pub const LOOP_MARKER_1: u8 = 0x8A;

/// Loop marker for loop 2 (8 caption pairs)
pub const LOOP_MARKER_2: u8 = 0x8F;

/// Loop marker for loop 3+ (first byte)
pub const LOOP_MARKER_3_FIRST: u8 = 0x16;

/// Loop marker for loop 4+ (first byte)
pub const LOOP_MARKER_4_FIRST: u8 = 0x1E;

/// Second byte of 2-byte loop markers
pub const LOOP_MARKER_SECOND: u8 = 0xFE;

/// Field 1 marker
pub const FIELD1_MARKER: u8 = 0xFF;

/// Field 2 marker
pub const FIELD2_MARKER: u8 = 0xFE;

/// Frame duration in 90kHz clock ticks (1001/30 * 90 = 3003 for 29.97fps NTSC)
pub const FRAME_DURATION_TICKS: i64 = 3003;

/// Caption block type for Field 1
pub const CC_TYPE_FIELD1: u8 = 0x04;

/// Caption block type for Field 2
pub const CC_TYPE_FIELD2: u8 = 0x05;

/// Result of parsing a DVD raw buffer
#[derive(Debug, Clone, PartialEq)]
pub struct DvdRawParseResult {
    /// Caption blocks extracted (each is 3 bytes: type, data1, data2)
    pub caption_blocks: Vec<[u8; 3]>,
    /// Number of bytes consumed from the input
    pub bytes_consumed: usize,
}

/// Checks if the buffer starts with the DVD raw header
///
/// # Arguments
/// * `buffer` - The byte buffer to check
///
/// # Returns
/// `true` if the buffer starts with the DVD_HEADER signature
pub fn is_dvdraw_header(buffer: &[u8]) -> bool {
    buffer.len() >= DVD_HEADER.len() && buffer[..DVD_HEADER.len()] == DVD_HEADER
}

/// Checks if a byte is a 1-byte loop marker
fn is_single_byte_loop_marker(byte: u8) -> bool {
    byte == LOOP_MARKER_1 || byte == LOOP_MARKER_2
}

/// Checks if bytes form a 2-byte loop marker
fn is_two_byte_loop_marker(byte1: u8, byte2: u8) -> bool {
    (byte1 == LOOP_MARKER_3_FIRST || byte1 == LOOP_MARKER_4_FIRST) && byte2 == LOOP_MARKER_SECOND
}

/// Parse McPoodle's DVD raw format and extract caption blocks.
///
/// This function parses the DVD raw binary format and extracts individual
/// caption blocks that can be passed to the 608 decoder. Each caption block
/// is 3 bytes: [cc_type, data1, data2].
///
/// # Arguments
/// * `buffer` - The raw bytes to parse
///
/// # Returns
/// A `DvdRawParseResult` containing the extracted caption blocks
///
/// # Example
/// ```
/// use ccx_rust::demuxer::dvdraw::{parse_dvdraw, DVD_HEADER, CC_TYPE_FIELD1};
///
/// let mut data = DVD_HEADER.to_vec();
/// data.push(0x8A); // Loop marker 1
/// data.extend_from_slice(&[0xFF, 0x80, 0x80]); // Field 1 padding
/// data.extend_from_slice(&[0xFE, 0x80, 0x80]); // Field 2 padding
///
/// let result = parse_dvdraw(&data);
/// assert_eq!(result.caption_blocks.len(), 2);
/// assert_eq!(result.caption_blocks[0][0], CC_TYPE_FIELD1);
/// ```
pub fn parse_dvdraw(buffer: &[u8]) -> DvdRawParseResult {
    let mut caption_blocks = Vec::new();
    let mut i = 0;
    let len = buffer.len();

    while i < len {
        // Check for DVD_HEADER
        if i + 8 <= len && buffer[i..i + 8] == DVD_HEADER {
            i += 8; // Skip header

            // Skip loop marker
            // Note: McPoodle's format uses single-byte markers (0x16, 0x1E) while
            // CCExtractor output may use 2-byte markers (0x16 0xFE, 0x1E 0xFE).
            // To distinguish: if 0x16/0x1E is followed by 0xFE and then by non-marker
            // data (not 0xFF or 0xFE), it's McPoodle's format where 0xFE is field 2 data.
            if i < len {
                if is_single_byte_loop_marker(buffer[i]) {
                    i += 1; // 1-byte marker (0x8A or 0x8F)
                } else if buffer[i] == LOOP_MARKER_3_FIRST || buffer[i] == LOOP_MARKER_4_FIRST {
                    // Check if this is a 2-byte marker or McPoodle's single-byte format
                    // In 2-byte format: 0x16 0xFE followed by field marker (0xFF/0xFE)
                    // In McPoodle's format: 0x16 followed by 0xFE (field 2 data start)
                    if i + 2 < len
                        && buffer[i + 1] == LOOP_MARKER_SECOND
                        && (buffer[i + 2] == FIELD1_MARKER || buffer[i + 2] == FIELD2_MARKER)
                    {
                        // CCExtractor format: 2-byte marker followed by field marker
                        i += 2;
                    } else {
                        // McPoodle's format: single-byte marker, 0xFE is field 2 data
                        i += 1;
                    }
                }
            }
            continue;
        }

        // Look for ff marker (field 1 data follows)
        if buffer[i] == FIELD1_MARKER && i + 3 <= len {
            caption_blocks.push([CC_TYPE_FIELD1, buffer[i + 1], buffer[i + 2]]);
            i += 3;
            continue;
        }

        // Look for fe marker (field 2 data follows)
        if buffer[i] == FIELD2_MARKER && i + 3 <= len {
            caption_blocks.push([CC_TYPE_FIELD2, buffer[i + 1], buffer[i + 2]]);
            i += 3;
            continue;
        }

        // Unknown byte, skip it
        i += 1;
    }

    DvdRawParseResult {
        caption_blocks,
        bytes_consumed: i,
    }
}

/// Parse DVD raw format with timing callback.
///
/// This is a higher-level function that parses the DVD raw format and calls
/// a callback for each caption block, advancing timing appropriately.
///
/// # Arguments
/// * `buffer` - The raw bytes to parse
/// * `callback` - Callback function called for each caption block with (cc_type, data1, data2)
/// * `timing_callback` - Callback function called before each field 1 block to advance timing
///
/// # Returns
/// The number of bytes consumed
pub fn parse_dvdraw_with_callbacks<F, T>(
    buffer: &[u8],
    mut callback: F,
    mut timing_callback: T,
) -> usize
where
    F: FnMut(u8, u8, u8),
    T: FnMut(),
{
    let mut i = 0;
    let len = buffer.len();

    while i < len {
        // Check for DVD_HEADER
        if i + 8 <= len && buffer[i..i + 8] == DVD_HEADER {
            i += 8; // Skip header

            // Skip loop marker (same logic as parse_dvdraw)
            if i < len {
                if is_single_byte_loop_marker(buffer[i]) {
                    i += 1; // 1-byte marker (0x8A or 0x8F)
                } else if buffer[i] == LOOP_MARKER_3_FIRST || buffer[i] == LOOP_MARKER_4_FIRST {
                    // Check if this is a 2-byte marker or McPoodle's single-byte format
                    if i + 2 < len
                        && buffer[i + 1] == LOOP_MARKER_SECOND
                        && (buffer[i + 2] == FIELD1_MARKER || buffer[i + 2] == FIELD2_MARKER)
                    {
                        // CCExtractor format: 2-byte marker followed by field marker
                        i += 2;
                    } else {
                        // McPoodle's format: single-byte marker
                        i += 1;
                    }
                }
            }
            continue;
        }

        // Look for ff marker (field 1 data follows)
        if buffer[i] == FIELD1_MARKER && i + 3 <= len {
            // Advance timing BEFORE processing this caption
            timing_callback();
            callback(CC_TYPE_FIELD1, buffer[i + 1], buffer[i + 2]);
            i += 3;
            continue;
        }

        // Look for fe marker (field 2 data follows)
        if buffer[i] == FIELD2_MARKER && i + 3 <= len {
            callback(CC_TYPE_FIELD2, buffer[i + 1], buffer[i + 2]);
            i += 3;
            continue;
        }

        // Unknown byte, skip it
        i += 1;
    }

    i
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_is_dvdraw_header() {
        assert!(is_dvdraw_header(&DVD_HEADER));
        assert!(is_dvdraw_header(&[
            0x00, 0x00, 0x01, 0xB2, 0x43, 0x43, 0x01, 0xF8, 0x8A
        ]));
        assert!(!is_dvdraw_header(&[
            0x00, 0x00, 0x01, 0xB2, 0x43, 0x43, 0x01
        ])); // Too short
        assert!(!is_dvdraw_header(&[
            0x00, 0x00, 0x01, 0xB2, 0x43, 0x43, 0x01, 0xF9
        ])); // Wrong byte
        assert!(!is_dvdraw_header(&[0x47, 0x00, 0x00, 0x00])); // TS sync byte
    }

    #[test]
    fn test_parse_empty_buffer() {
        let result = parse_dvdraw(&[]);
        assert!(result.caption_blocks.is_empty());
        assert_eq!(result.bytes_consumed, 0);
    }

    #[test]
    fn test_parse_header_only() {
        let result = parse_dvdraw(&DVD_HEADER);
        assert!(result.caption_blocks.is_empty());
        assert_eq!(result.bytes_consumed, 8);
    }

    #[test]
    fn test_parse_single_caption_pair_loop1() {
        // DVD_HEADER + loop marker 1 + field 1 + field 2
        let mut data = DVD_HEADER.to_vec();
        data.push(LOOP_MARKER_1); // 0x8A
        data.extend_from_slice(&[FIELD1_MARKER, 0x94, 0x2C]); // Field 1: RCL command
        data.extend_from_slice(&[FIELD2_MARKER, 0x80, 0x80]); // Field 2: padding

        let result = parse_dvdraw(&data);
        assert_eq!(result.caption_blocks.len(), 2);
        assert_eq!(result.caption_blocks[0], [CC_TYPE_FIELD1, 0x94, 0x2C]);
        assert_eq!(result.caption_blocks[1], [CC_TYPE_FIELD2, 0x80, 0x80]);
    }

    #[test]
    fn test_parse_single_caption_pair_loop2() {
        let mut data = DVD_HEADER.to_vec();
        data.push(LOOP_MARKER_2); // 0x8F
        data.extend_from_slice(&[FIELD1_MARKER, 0x80, 0x80]);
        data.extend_from_slice(&[FIELD2_MARKER, 0x80, 0x80]);

        let result = parse_dvdraw(&data);
        assert_eq!(result.caption_blocks.len(), 2);
    }

    #[test]
    fn test_parse_two_byte_loop_marker() {
        // Loop marker 3: 0x16 0xFE
        let mut data = DVD_HEADER.to_vec();
        data.extend_from_slice(&[LOOP_MARKER_3_FIRST, LOOP_MARKER_SECOND]); // 0x16 0xFE
        data.extend_from_slice(&[FIELD1_MARKER, 0x45, 0x4C]); // "EL"
        data.extend_from_slice(&[FIELD2_MARKER, 0x80, 0x80]);

        let result = parse_dvdraw(&data);
        assert_eq!(result.caption_blocks.len(), 2);
        assert_eq!(result.caption_blocks[0], [CC_TYPE_FIELD1, 0x45, 0x4C]);
    }

    #[test]
    fn test_parse_mcpoodle_format_loop3() {
        // McPoodle's format: loop marker 0x16 followed directly by field 2 marker
        // This tests the case where 0xFE is NOT part of loop marker but is field 2 data
        let mut data = DVD_HEADER.to_vec();
        data.push(LOOP_MARKER_3_FIRST); // 0x16 (single byte marker)
        data.extend_from_slice(&[FIELD2_MARKER, 0x80, 0x80]); // Field 2 first (McPoodle convention)
        data.extend_from_slice(&[FIELD1_MARKER, 0x80, 0x80]); // Field 1 second

        let result = parse_dvdraw(&data);
        assert_eq!(result.caption_blocks.len(), 2);
        // McPoodle format has field 2 first after loop 3 marker
        assert_eq!(result.caption_blocks[0], [CC_TYPE_FIELD2, 0x80, 0x80]);
        assert_eq!(result.caption_blocks[1], [CC_TYPE_FIELD1, 0x80, 0x80]);
    }

    #[test]
    fn test_parse_multiple_headers() {
        // Two headers with caption data
        let mut data = DVD_HEADER.to_vec();
        data.push(LOOP_MARKER_1);
        data.extend_from_slice(&[FIELD1_MARKER, 0x80, 0x80]);
        data.extend_from_slice(&[FIELD2_MARKER, 0x80, 0x80]);
        // Second header
        data.extend_from_slice(&DVD_HEADER);
        data.push(LOOP_MARKER_2);
        data.extend_from_slice(&[FIELD1_MARKER, 0x94, 0x20]); // RCL
        data.extend_from_slice(&[FIELD2_MARKER, 0x80, 0x80]);

        let result = parse_dvdraw(&data);
        assert_eq!(result.caption_blocks.len(), 4);
        assert_eq!(result.caption_blocks[2], [CC_TYPE_FIELD1, 0x94, 0x20]);
    }

    #[test]
    fn test_parse_with_callbacks() {
        let mut data = DVD_HEADER.to_vec();
        data.push(LOOP_MARKER_1);
        data.extend_from_slice(&[FIELD1_MARKER, 0x80, 0x80]);
        data.extend_from_slice(&[FIELD2_MARKER, 0x80, 0x80]);
        data.extend_from_slice(&[FIELD1_MARKER, 0x94, 0x2C]);
        data.extend_from_slice(&[FIELD2_MARKER, 0x80, 0x80]);

        let mut caption_count = 0;
        let mut timing_count = 0;

        parse_dvdraw_with_callbacks(
            &data,
            |_cc_type, _d1, _d2| {
                caption_count += 1;
            },
            || {
                timing_count += 1;
            },
        );

        assert_eq!(caption_count, 4); // 2 field 1 + 2 field 2
        assert_eq!(timing_count, 2); // Only called for field 1 blocks
    }

    #[test]
    fn test_parse_real_caption_data() {
        // Simulate real caption data: "KISSES DELUXE CHOCOLATES"
        // This tests parsing actual CC commands
        let mut data = DVD_HEADER.to_vec();
        data.push(LOOP_MARKER_1);
        // EDM - Erase Displayed Memory
        data.extend_from_slice(&[FIELD1_MARKER, 0x94, 0x2C]);
        data.extend_from_slice(&[FIELD2_MARKER, 0x80, 0x80]);
        // RCL - Resume Caption Loading
        data.extend_from_slice(&[FIELD1_MARKER, 0x94, 0x20]);
        data.extend_from_slice(&[FIELD2_MARKER, 0x80, 0x80]);
        // Text: "KI"
        data.extend_from_slice(&[FIELD1_MARKER, 0x4B, 0x49]);
        data.extend_from_slice(&[FIELD2_MARKER, 0x80, 0x80]);

        let result = parse_dvdraw(&data);
        assert_eq!(result.caption_blocks.len(), 6);

        // Verify EDM command
        assert_eq!(result.caption_blocks[0], [CC_TYPE_FIELD1, 0x94, 0x2C]);
        // Verify RCL command
        assert_eq!(result.caption_blocks[2], [CC_TYPE_FIELD1, 0x94, 0x20]);
        // Verify "KI" text
        assert_eq!(result.caption_blocks[4], [CC_TYPE_FIELD1, 0x4B, 0x49]);
    }

    #[test]
    fn test_parse_padding_only() {
        // File with only padding data (0x80 0x80)
        let mut data = DVD_HEADER.to_vec();
        data.push(LOOP_MARKER_1);
        for _ in 0..5 {
            data.extend_from_slice(&[FIELD1_MARKER, 0x80, 0x80]);
            data.extend_from_slice(&[FIELD2_MARKER, 0x80, 0x80]);
        }

        let result = parse_dvdraw(&data);
        assert_eq!(result.caption_blocks.len(), 10);
        // All should be padding
        for block in &result.caption_blocks {
            assert_eq!(block[1], 0x80);
            assert_eq!(block[2], 0x80);
        }
    }

    #[test]
    fn test_constants() {
        assert_eq!(DVD_HEADER.len(), 8);
        assert_eq!(FRAME_DURATION_TICKS, 3003);
        assert_eq!(CC_TYPE_FIELD1, 0x04);
        assert_eq!(CC_TYPE_FIELD2, 0x05);
    }

    #[test]
    fn test_loop_marker_detection() {
        assert!(is_single_byte_loop_marker(LOOP_MARKER_1));
        assert!(is_single_byte_loop_marker(LOOP_MARKER_2));
        assert!(!is_single_byte_loop_marker(LOOP_MARKER_3_FIRST));
        assert!(!is_single_byte_loop_marker(FIELD1_MARKER));

        assert!(is_two_byte_loop_marker(
            LOOP_MARKER_3_FIRST,
            LOOP_MARKER_SECOND
        ));
        assert!(is_two_byte_loop_marker(
            LOOP_MARKER_4_FIRST,
            LOOP_MARKER_SECOND
        ));
        assert!(!is_two_byte_loop_marker(LOOP_MARKER_1, LOOP_MARKER_SECOND));
        assert!(!is_two_byte_loop_marker(LOOP_MARKER_3_FIRST, 0x00));
    }

    #[test]
    fn test_incomplete_caption_at_end() {
        // Buffer ends with incomplete caption data
        let mut data = DVD_HEADER.to_vec();
        data.push(LOOP_MARKER_1);
        data.extend_from_slice(&[FIELD1_MARKER, 0x80, 0x80]); // Complete
        data.extend_from_slice(&[FIELD1_MARKER, 0x80]); // Incomplete - only 2 bytes

        let result = parse_dvdraw(&data);
        assert_eq!(result.caption_blocks.len(), 1); // Only the complete one
    }

    #[test]
    fn test_garbage_data_handling() {
        // Mix of valid and invalid data
        let mut data = vec![0x00, 0x01, 0x02, 0x03]; // Garbage
        data.extend_from_slice(&DVD_HEADER);
        data.push(LOOP_MARKER_1);
        data.extend_from_slice(&[FIELD1_MARKER, 0x80, 0x80]);
        data.extend_from_slice(&[0x00, 0x01, 0x02]); // More garbage
        data.extend_from_slice(&[FIELD2_MARKER, 0x80, 0x80]);

        let result = parse_dvdraw(&data);
        // Should skip garbage and find the valid caption blocks
        assert_eq!(result.caption_blocks.len(), 2);
    }
}
