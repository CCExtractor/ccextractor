//! SCC (Scenarist Closed Caption) format parser
//!
//! This module provides functionality to parse SCC files and extract CEA-608 caption data.
//! SCC is a text-based format commonly used in professional video production.
//!
//! # Format Specification
//!
//! ```text
//! Scenarist_SCC V1.0
//!
//! 00:00:00:00    9420 9420 94ad 94ad 9470 9470 4c6f 7265
//! 00:00:02:15    942c 942c
//! ```
//!
//! - **Header:** `Scenarist_SCC V1.0` (first line)
//! - **Lines:** `HH:MM:SS:FF\t<hex pairs>` (SMPTE timecode + TAB + space-separated hex bytes)
//! - **Hex pairs:** CEA-608 byte pairs with odd parity bits (e.g., `9420` = RCL command)
//! - **Frame rate:** 29.97fps NTSC drop-frame (default), also supports 23.976/24/25/30fps

/// SCC format header
pub const SCC_HEADER: &[u8] = b"Scenarist_SCC V1.0";

/// Frame rate options for SMPTE timecode conversion
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum SccFrameRate {
    /// NTSC drop-frame (default) - 29.97fps
    #[default]
    Fps29_97,
    /// Film - 24fps
    Fps24,
    /// PAL - 25fps
    Fps25,
    /// NTSC non-drop-frame - 30fps
    Fps30,
    /// NTSC film - 23.976fps
    Fps23_976,
}

impl SccFrameRate {
    /// Convert from integer value (used in FFI)
    /// 0=29.97 (default), 1=24, 2=25, 3=30, 4=23.976
    pub fn from_int(value: i32) -> Self {
        match value {
            1 => SccFrameRate::Fps24,
            2 => SccFrameRate::Fps25,
            3 => SccFrameRate::Fps30,
            4 => SccFrameRate::Fps23_976,
            _ => SccFrameRate::Fps29_97,
        }
    }

    fn nominal_fps(&self) -> u32 {
        match self {
            SccFrameRate::Fps29_97 | SccFrameRate::Fps30 => 30,
            SccFrameRate::Fps24 | SccFrameRate::Fps23_976 => 24,
            SccFrameRate::Fps25 => 25,
        }
    }

    /// Get milliseconds per frame for this frame rate
    pub fn ms_per_frame(&self) -> f64 {
        match self {
            SccFrameRate::Fps29_97 => 1001.0 / 30.0,  // ~33.37ms
            SccFrameRate::Fps24 => 1000.0 / 24.0,     // ~41.67ms
            SccFrameRate::Fps25 => 1000.0 / 25.0,     // 40ms
            SccFrameRate::Fps30 => 1000.0 / 30.0,     // ~33.33ms
            SccFrameRate::Fps23_976 => 1001.0 / 24.0, // ~41.71ms
        }
    }
}

/// Result of parsing an SCC file
#[derive(Debug, Clone)]
pub struct SccParseResult {
    /// Parsed caption lines with timing
    pub lines: Vec<SccLine>,
    /// Number of bytes consumed from the input
    pub bytes_consumed: usize,
}

/// Single line from SCC file with timing and caption data
#[derive(Debug, Clone)]
pub struct SccLine {
    /// Timestamp in milliseconds
    pub time_ms: i64,
    /// CEA-608 byte pairs
    pub pairs: Vec<[u8; 2]>,
}

/// Caption block type for Field 1 (CC1/CC3)
pub const CC_TYPE_FIELD1: u8 = 0x04;

/// Caption block type for Field 2 (CC2/CC4)
#[allow(dead_code)]
pub const CC_TYPE_FIELD2: u8 = 0x05;

/// Check if buffer starts with SCC header
///
/// # Arguments
/// * `buffer` - The byte buffer to check
///
/// # Returns
/// `true` if the buffer starts with the SCC header signature
pub fn is_scc_file(buffer: &[u8]) -> bool {
    // Skip UTF-8 BOM if present
    let data = if buffer.len() >= 3 && buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF {
        &buffer[3..]
    } else {
        buffer
    };

    data.len() >= SCC_HEADER.len() && &data[..SCC_HEADER.len()] == SCC_HEADER
}

/// Parse SMPTE timecode to milliseconds
///
/// Format: HH:MM:SS:FF or HH:MM:SS;FF (drop-frame uses semicolon)
///
/// # Arguments
/// * `timecode` - The SMPTE timecode string
/// * `fps` - The frame rate to use for conversion
///
/// # Returns
/// The time in milliseconds, or None if parsing fails
pub fn parse_smpte_timecode(timecode: &str, fps: SccFrameRate) -> Option<i64> {
    let bytes = timecode.as_bytes();
    if bytes.len() != 11
        || bytes[2] != b':'
        || bytes[5] != b':'
        || (bytes[8] != b':' && bytes[8] != b';')
    {
        return None;
    }

    let digit_positions = [0, 1, 3, 4, 6, 7, 9, 10];
    if digit_positions
        .iter()
        .any(|&index| !bytes[index].is_ascii_digit())
    {
        return None;
    }

    let h = ((bytes[0] - b'0') * 10 + (bytes[1] - b'0')) as u32;
    let m = ((bytes[3] - b'0') * 10 + (bytes[4] - b'0')) as u32;
    let s = ((bytes[6] - b'0') * 10 + (bytes[7] - b'0')) as u32;
    let f = ((bytes[9] - b'0') * 10 + (bytes[10] - b'0')) as u32;

    let nominal_fps = fps.nominal_fps();
    let is_drop_frame = bytes[8] == b';' && fps == SccFrameRate::Fps29_97;
    if is_drop_frame {
        let total_minutes = h * 60 + m;
        let frame = if m % 10 != 0 && s == 0 && f < 2 {
            // Real SCC writers may emit labels from wall-clock milliseconds at
            // skipped drop-frame positions (00 or 01).  Treat these unusual
            // labels as the next valid frame (:02) while retaining the line.
            2
        } else {
            f
        };

        let total_nominal_frames = ((h * 3600 + m * 60 + s) * nominal_fps + frame) as i64;
        let dropped_frames = 2 * (total_minutes - total_minutes / 10);
        let frame_number = total_nominal_frames - dropped_frames as i64;

        Some(frame_number * 1001 / 30)
    } else {
        let total_ms = h as i64 * 3600000 + m as i64 * 60000 + s as i64 * 1000;

        match fps {
            SccFrameRate::Fps29_97 => Some(total_ms + f as i64 * 1001 / 30),
            SccFrameRate::Fps24 => Some(total_ms + f as i64 * 1000 / 24),
            SccFrameRate::Fps25 => Some(total_ms + f as i64 * 40),
            SccFrameRate::Fps30 => Some(total_ms + f as i64 * 1000 / 30),
            SccFrameRate::Fps23_976 => Some(total_ms + f as i64 * 1001 / 24),
        }
    }
}

/// Parse hex pair string (e.g., "9420") to bytes
///
/// # Arguments
/// * `s` - A 4-character hex string
///
/// # Returns
/// The two bytes, or None if parsing fails
pub fn parse_hex_pair(s: &str) -> Option<[u8; 2]> {
    if s.len() != 4 || !s.as_bytes().iter().all(u8::is_ascii_hexdigit) {
        return None;
    }
    let b1 = u8::from_str_radix(&s[0..2], 16).ok()?;
    let b2 = u8::from_str_radix(&s[2..4], 16).ok()?;
    Some([b1, b2])
}

/// Parse space-separated hex pairs from SCC line
///
/// # Arguments
/// * `hex_str` - Space-separated hex pairs (e.g., "9420 9420 94ad")
///
/// # Returns
/// Vector of byte pairs
pub fn parse_hex_pairs(hex_str: &str) -> Vec<[u8; 2]> {
    hex_str
        .split_whitespace()
        .filter_map(parse_hex_pair)
        .collect()
}

/// Parse entire SCC file content
///
/// # Arguments
/// * `content` - The SCC file content as a string
/// * `fps` - The frame rate to use for timecode conversion
///
/// # Returns
/// Parse result containing all caption lines
pub fn parse_scc(content: &str, fps: SccFrameRate) -> SccParseResult {
    let mut lines = Vec::new();

    for line in content.lines() {
        let line = line.trim();

        // Skip empty lines, header, and comments
        if line.is_empty() || line.starts_with("Scenarist_SCC") {
            continue;
        }

        // Parse timecode and hex data
        // Format: "HH:MM:SS:FF\t<hex pairs>" (accept whitespace in real-world files)
        if let Some(separator_pos) = line.find(|c: char| c.is_whitespace()) {
            let timecode = &line[..separator_pos];
            let hex_data = &line[separator_pos + 1..];

            if let Some(time_ms) = parse_smpte_timecode(timecode, fps) {
                let pairs = parse_hex_pairs(hex_data);
                if !pairs.is_empty() {
                    lines.push(SccLine { time_ms, pairs });
                }
            }
        }
    }

    SccParseResult {
        lines,
        bytes_consumed: content.len(),
    }
}

/// Process SCC with callbacks (similar to dvdraw pattern)
///
/// This function parses the SCC format and calls callbacks for each caption block
/// and timing update.
///
/// # Arguments
/// * `content` - The SCC file content as a string
/// * `fps` - The frame rate to use for timecode conversion
/// * `callback` - Called for each caption pair: (cc_type, data1, data2)
/// * `timing_callback` - Called with the timestamp in milliseconds before each line
///
/// # Returns
/// The number of bytes consumed
pub fn parse_scc_with_callbacks<F, T>(
    content: &str,
    fps: SccFrameRate,
    mut callback: F,
    mut timing_callback: T,
) -> usize
where
    F: FnMut(u8, u8, u8),
    T: FnMut(i64),
{
    let result = parse_scc(content, fps);

    for line in &result.lines {
        // Call timing callback with this line's timestamp
        timing_callback(line.time_ms);

        // Process each caption pair as field 1 (CC1)
        for pair in &line.pairs {
            callback(CC_TYPE_FIELD1, pair[0], pair[1]);
        }
    }

    result.bytes_consumed
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_is_scc_file() {
        assert!(is_scc_file(b"Scenarist_SCC V1.0\n\n"));
        assert!(is_scc_file(b"\xEF\xBB\xBFScenarist_SCC V1.0\n")); // With BOM
        assert!(!is_scc_file(b"Not an SCC file"));
        assert!(!is_scc_file(b"Scenarist_SCC V2.0")); // Wrong version
        assert!(!is_scc_file(b"")); // Empty
        assert!(!is_scc_file(b"Scenarist")); // Too short
    }

    #[test]
    fn test_scc_frame_rate_default() {
        assert_eq!(SccFrameRate::default(), SccFrameRate::Fps29_97);
    }

    #[test]
    fn test_scc_frame_rate_from_int() {
        assert_eq!(SccFrameRate::from_int(0), SccFrameRate::Fps29_97);
        assert_eq!(SccFrameRate::from_int(1), SccFrameRate::Fps24);
        assert_eq!(SccFrameRate::from_int(2), SccFrameRate::Fps25);
        assert_eq!(SccFrameRate::from_int(3), SccFrameRate::Fps30);
        assert_eq!(SccFrameRate::from_int(4), SccFrameRate::Fps23_976);
        assert_eq!(SccFrameRate::from_int(99), SccFrameRate::Fps29_97); // Invalid defaults
    }

    #[test]
    fn test_parse_scc_uses_23_976_framerate_from_ffi() {
        // Real sample file, loaded at compile time (repo tests/samples/scc_23_976.scc)
        let content = include_str!("../../../../tests/samples/scc_23_976.scc");
        let result = parse_scc(content, SccFrameRate::from_int(4));
        assert_eq!(result.lines.len(), 2);
        assert_eq!(result.lines[0].time_ms, 1500);
        assert_eq!(result.lines[1].time_ms, 3500);
        assert_eq!(
            result.lines[0].pairs,
            vec![
                [0x94, 0x20],
                [0x94, 0x20],
                [0x94, 0xe0],
                [0x94, 0xe0],
                [0xc8, 0x49],
                [0x94, 0x2f],
                [0x94, 0x2f],
            ]
        );
        assert_eq!(result.lines[1].pairs, vec![[0x94, 0x2c], [0x94, 0x2c]]);
        assert!((SccFrameRate::from_int(4).ms_per_frame() - 1001.0 / 24.0).abs() < 0.001);
    }

    #[test]
    fn test_parse_smpte_timecode() {
        // 29.97fps
        assert_eq!(
            parse_smpte_timecode("00:00:00:00", SccFrameRate::Fps29_97),
            Some(0)
        );
        assert_eq!(
            parse_smpte_timecode("00:00:01:00", SccFrameRate::Fps29_97),
            Some(1000)
        );
        assert_eq!(
            parse_smpte_timecode("00:01:00:00", SccFrameRate::Fps29_97),
            Some(60000)
        );
        assert_eq!(
            parse_smpte_timecode("01:00:00:00", SccFrameRate::Fps29_97),
            Some(3600000)
        );
        assert_eq!(
            parse_smpte_timecode("00:00:00:29", SccFrameRate::Fps29_97),
            Some(967)
        );

        // 25fps - 25 frames = 1 second
        assert_eq!(
            parse_smpte_timecode("00:00:00:25", SccFrameRate::Fps25),
            Some(1000)
        );
        assert_eq!(
            parse_smpte_timecode("00:00:00:24", SccFrameRate::Fps25),
            Some(960)
        );

        // 24fps
        assert_eq!(
            parse_smpte_timecode("00:00:00:23", SccFrameRate::Fps24),
            Some(958)
        );
        assert_eq!(
            parse_smpte_timecode("00:00:00:24", SccFrameRate::Fps24),
            Some(1000)
        );

        // 30fps
        assert_eq!(
            parse_smpte_timecode("00:00:00:29", SccFrameRate::Fps30),
            Some(966)
        );
        assert_eq!(
            parse_smpte_timecode("00:00:00:30", SccFrameRate::Fps30),
            Some(1000)
        );

        // Invalid timecodes
        assert_eq!(
            parse_smpte_timecode("00:00:00", SccFrameRate::Fps29_97),
            None
        );
        assert_eq!(
            parse_smpte_timecode("invalid", SccFrameRate::Fps29_97),
            None
        );
        assert_eq!(parse_smpte_timecode("", SccFrameRate::Fps29_97), None);
    }

    #[test]
    fn test_parse_smpte_timecode_valid_drop_frame() {
        assert_eq!(
            parse_smpte_timecode("00:00:00;00", SccFrameRate::Fps29_97),
            Some(0)
        );
        assert_eq!(
            parse_smpte_timecode("00:00:01;00", SccFrameRate::Fps29_97),
            Some(1001)
        );
        assert_eq!(
            parse_smpte_timecode("00:00:59;29", SccFrameRate::Fps29_97),
            Some(60026)
        );
        assert_eq!(
            parse_smpte_timecode("00:01:00;02", SccFrameRate::Fps29_97),
            Some(60060)
        );
    }

    #[test]
    fn test_parse_smpte_timecode_drop_frame_ten_minute_transition() {
        assert_eq!(
            parse_smpte_timecode("00:09:59;29", SccFrameRate::Fps29_97),
            Some(599966)
        );
        assert_eq!(
            parse_smpte_timecode("00:10:00;00", SccFrameRate::Fps29_97),
            Some(599999)
        );
        assert_eq!(
            parse_smpte_timecode("00:10:00;01", SccFrameRate::Fps29_97),
            Some(600032)
        );
    }

    #[test]
    fn test_parse_smpte_timecode_drop_frame_hour_transition() {
        assert_eq!(
            parse_smpte_timecode("01:00:00;00", SccFrameRate::Fps29_97),
            Some(3599996)
        );
        assert_eq!(
            parse_smpte_timecode("01:00:00;02", SccFrameRate::Fps29_97),
            Some(3600063)
        );
    }

    #[test]
    fn test_parse_smpte_timecode_accepts_out_of_range_numeric_components() {
        assert_eq!(
            parse_smpte_timecode("24:00:00:00", SccFrameRate::Fps29_97),
            Some(86400000)
        );
        assert_eq!(
            parse_smpte_timecode("99:60:60:99", SccFrameRate::Fps30),
            Some(360063300)
        );
        assert_eq!(
            parse_smpte_timecode("00:60:00:29", SccFrameRate::Fps29_97),
            Some(3600967)
        );
        assert_eq!(
            parse_smpte_timecode("00:00:60:29", SccFrameRate::Fps29_97),
            Some(60967)
        );

        assert_eq!(
            parse_smpte_timecode("00:00:00:30", SccFrameRate::Fps29_97),
            Some(1001)
        );
        assert_eq!(
            parse_smpte_timecode("00:00:00:24", SccFrameRate::Fps24),
            Some(1000)
        );
        assert_eq!(
            parse_smpte_timecode("00:00:00:25", SccFrameRate::Fps25),
            Some(1000)
        );
        assert_eq!(
            parse_smpte_timecode("00:00:00:30", SccFrameRate::Fps30),
            Some(1000)
        );
    }

    #[test]
    fn test_parse_smpte_timecode_rejects_incorrect_formats() {
        for timecode in [
            "00:00:00",
            "00:00:00:00:00",
            "00;00:00:00",
            "00:00;00:00",
            "00:00:00;00;00",
            "00:00:00-00",
            "1:2:3:4",
            "00:1:03;02",
            "000:00:00:00",
            "00:00:000:00",
            "00:00:00:000",
            "0:00:00:00",
            "00:000:00:00",
            "00:00:000;00",
            "00:00:00:0",
            "00:00:00:+",
            "00:00:00",
            "aa:00:00:00",
            "00:+1:00:00",
            "00:00:1.0:00",
            " 00:00:00:00",
            "00:00:00:00 ",
        ] {
            assert_eq!(
                parse_smpte_timecode(timecode, SccFrameRate::Fps29_97),
                None,
                "timecode: {timecode:?}"
            );
        }
    }

    #[test]
    fn test_parse_smpte_timecode_accepts_unusual_drop_frame_labels() {
        assert_eq!(
            parse_smpte_timecode("00:01:00;00", SccFrameRate::Fps29_97),
            Some(60060)
        );
        assert_eq!(
            parse_smpte_timecode("00:01:00;01", SccFrameRate::Fps29_97),
            Some(60060)
        );

        for timecode in ["00:02:00;00", "00:02:00;01", "00:59:00;00", "00:59:00;01"] {
            assert!(
                parse_smpte_timecode(timecode, SccFrameRate::Fps29_97).is_some(),
                "timecode: {:?}",
                timecode
            );
        }

        assert_eq!(
            parse_smpte_timecode("00:01:00;02", SccFrameRate::Fps29_97),
            Some(60060)
        );
    }

    #[test]
    fn test_parse_smpte_timecode_treats_semicolon_as_non_drop_frame_for_other_rates() {
        assert_eq!(
            parse_smpte_timecode("00:00:00;00", SccFrameRate::Fps24),
            Some(0)
        );
        assert_eq!(
            parse_smpte_timecode("00:00:00;00", SccFrameRate::Fps25),
            Some(0)
        );
        assert_eq!(
            parse_smpte_timecode("00:00:00;00", SccFrameRate::Fps30),
            Some(0)
        );
        assert_eq!(
            parse_smpte_timecode("00:00:00;00", SccFrameRate::Fps23_976),
            Some(0)
        );
        assert_eq!(
            parse_smpte_timecode("00:00:00;23", SccFrameRate::Fps24),
            Some(958)
        );
        assert_eq!(
            parse_smpte_timecode("00:00:00;24", SccFrameRate::Fps25),
            Some(960)
        );
        assert_eq!(
            parse_smpte_timecode("00:00:00;29", SccFrameRate::Fps30),
            Some(966)
        );
        assert_eq!(
            parse_smpte_timecode("00:00:00;23", SccFrameRate::Fps23_976),
            Some(959)
        );
    }

    #[test]
    fn test_parse_hex_pair() {
        assert_eq!(parse_hex_pair("9420"), Some([0x94, 0x20]));
        assert_eq!(parse_hex_pair("80ff"), Some([0x80, 0xFF]));
        assert_eq!(parse_hex_pair("0000"), Some([0x00, 0x00]));
        assert_eq!(parse_hex_pair("ABCD"), Some([0xAB, 0xCD])); // Uppercase
        assert_eq!(parse_hex_pair("xyz"), None); // Invalid
        assert_eq!(parse_hex_pair("94"), None); // Too short
        assert_eq!(parse_hex_pair("942020"), None); // Too long
        assert_eq!(parse_hex_pair(""), None); // Empty
    }

    #[test]
    fn test_parse_hex_pair_rejects_non_hex_tokens() {
        for token in ["a€", "€a", "😀", "éé", "+120", "94+0", "zzzz"] {
            assert_eq!(parse_hex_pair(token), None, "token: {token:?}");
        }
    }

    #[test]
    fn test_parse_scc_skips_unicode_tokens_and_keeps_valid_pairs() {
        let content = "Scenarist_SCC V1.0\n\n00:00:01:00\t9420 a€ 4849 😀 942f\n";
        let result = parse_scc(content, SccFrameRate::Fps30);
        assert_eq!(result.bytes_consumed, content.len());
        assert_eq!(result.lines.len(), 1);
        assert_eq!(result.lines[0].time_ms, 1000);
        assert_eq!(
            result.lines[0].pairs,
            [[0x94, 0x20], [0x48, 0x49], [0x94, 0x2f]]
        );
    }

    #[test]
    fn test_parse_hex_pairs() {
        let pairs = parse_hex_pairs("9420 9420 94ad 94ad");
        assert_eq!(pairs.len(), 4);
        assert_eq!(pairs[0], [0x94, 0x20]);
        assert_eq!(pairs[1], [0x94, 0x20]);
        assert_eq!(pairs[2], [0x94, 0xAD]);
        assert_eq!(pairs[3], [0x94, 0xAD]);

        // With extra whitespace
        let pairs2 = parse_hex_pairs("  9420   9420  ");
        assert_eq!(pairs2.len(), 2);

        // Empty
        let pairs3 = parse_hex_pairs("");
        assert!(pairs3.is_empty());

        // Mixed valid and invalid
        let pairs4 = parse_hex_pairs("9420 invalid 94ad");
        assert_eq!(pairs4.len(), 2);
    }

    #[test]
    fn test_parse_scc_simple() {
        let content = "Scenarist_SCC V1.0\n\n00:00:00:00\t9420 9420\n00:00:01:00\t942c 942c\n";
        let result = parse_scc(content, SccFrameRate::Fps29_97);
        assert_eq!(result.lines.len(), 2);
        assert_eq!(result.lines[0].time_ms, 0);
        assert_eq!(result.lines[0].pairs.len(), 2);
        assert_eq!(result.lines[0].pairs[0], [0x94, 0x20]);
        assert_eq!(result.lines[1].time_ms, 1000);
        assert_eq!(result.lines[1].pairs[0], [0x94, 0x2C]);
    }

    #[test]
    fn test_parse_scc_minimal_drop_frame_sample() {
        let content = "Scenarist_SCC V1.0\n00:01:00;02\t9420\n";
        let result = parse_scc(content, SccFrameRate::Fps29_97);

        assert_eq!(result.lines.len(), 1);
        assert_eq!(result.lines[0].time_ms, 60060);
        assert_eq!(result.lines[0].pairs, [[0x94, 0x20]]);
    }

    #[test]
    fn test_parse_scc_accepts_tab_and_space_separators() {
        for separator in ["\t", " ", "   "] {
            let content = format!("Scenarist_SCC V1.0\n00:00:01;00{separator}9420 9420\n");
            let result = parse_scc(&content, SccFrameRate::Fps29_97);

            assert_eq!(result.lines.len(), 1, "separator: {separator:?}");
            assert_eq!(result.lines[0].time_ms, 1001, "separator: {separator:?}");
            assert_eq!(
                result.lines[0].pairs,
                [[0x94, 0x20], [0x94, 0x20]],
                "separator: {separator:?}"
            );
        }
    }

    #[test]
    fn test_parse_scc_preserves_reviewer_reproduction_sample() {
        let content = "Scenarist_SCC V1.0\n\
00:00:58;00 9420 9420 94ae 94ae 9470 9470 c1c1 942f 942f\n\
00:01:00;00 942c 942c 9420 9420 94ae 94ae 9470 9470 c2c2 942f 942f\n\
00:01:03;00 942c 942c\n";
        let result = parse_scc(content, SccFrameRate::Fps29_97);

        assert_eq!(result.lines.len(), 3);

        // Caption AA starts at 58 seconds and remains intact.
        assert_eq!(result.lines[0].time_ms, 58058);
        assert_eq!(
            result.lines[0].pairs,
            [
                [0x94, 0x20],
                [0x94, 0x20],
                [0x94, 0xae],
                [0x94, 0xae],
                [0x94, 0x70],
                [0x94, 0x70],
                [0xc1, 0xc1],
                [0x94, 0x2f],
                [0x94, 0x2f],
            ]
        );

        // Caption BB at 1:00;00 is the unusual label that previously discarded the line.
        assert_eq!(result.lines[1].time_ms, 60060);
        assert_eq!(
            result.lines[1].pairs,
            [
                [0x94, 0x2c],
                [0x94, 0x2c],
                [0x94, 0x20],
                [0x94, 0x20],
                [0x94, 0xae],
                [0x94, 0xae],
                [0x94, 0x70],
                [0x94, 0x70],
                [0xc2, 0xc2],
                [0x94, 0x2f],
                [0x94, 0x2f],
            ]
        );

        assert_eq!(result.lines[2].time_ms, 62996);
        assert_eq!(result.lines[2].pairs, [[0x94, 0x2c], [0x94, 0x2c]]);
    }

    #[test]
    fn test_parse_scc_preserves_cues_with_semicolon_labels_at_all_rates() {
        let content = "Scenarist_SCC V1.0\n\n\
00:00:01;00    9420 9420 94ae 94ae 9470 9470 c1c1 942f 942f\n\
\n\
00:00:03;00    942c 942c 9420 9420 94ae 94ae 9470 9470 c2c2 942f 942f\n\
00:00:05;00    942c 942c\n";

        for fps in [
            SccFrameRate::Fps29_97,
            SccFrameRate::Fps30,
            SccFrameRate::Fps25,
            SccFrameRate::Fps24,
            SccFrameRate::Fps23_976,
        ] {
            let result = parse_scc(content, fps);

            assert_eq!(result.lines.len(), 3, "frame rate: {fps:?}");
            assert_eq!(result.lines[0].pairs.len(), 9, "frame rate: {fps:?}");
            assert_eq!(result.lines[0].pairs[6], [0xc1, 0xc1]);
            assert_eq!(result.lines[1].pairs.len(), 11, "frame rate: {fps:?}");
            assert_eq!(result.lines[1].pairs[8], [0xc2, 0xc2]);
            assert_eq!(result.lines[2].pairs, [[0x94, 0x2c], [0x94, 0x2c]]);
        }
    }

    #[test]
    fn test_parse_scc_with_bom() {
        let content = "\u{FEFF}Scenarist_SCC V1.0\n\n00:00:00:00\t9420\n";
        let result = parse_scc(content, SccFrameRate::Fps29_97);
        assert_eq!(result.lines.len(), 1);
    }

    #[test]
    fn test_parse_scc_empty_lines() {
        let content = "Scenarist_SCC V1.0\n\n\n\n00:00:00:00\t9420\n\n\n";
        let result = parse_scc(content, SccFrameRate::Fps29_97);
        assert_eq!(result.lines.len(), 1);
    }

    #[test]
    fn test_parse_scc_real_sample() {
        // Sample from the plan with actual caption commands
        // Note: "a1a0" is "!" with parity + space (proper 4-char hex pair)
        let content = r#"Scenarist_SCC V1.0

00:00:00:00	9420 9420 94ad 94ad 9470 9470 4869 2074 6865 7265 a1a0
00:00:02:00	942c 942c
00:00:04:00	9420 9420 9470 9470 5465 7374 696e 6720 5343 4320 696e 7075 742e
00:00:06:00	942c 942c
"#;
        let result = parse_scc(content, SccFrameRate::Fps29_97);
        assert_eq!(result.lines.len(), 4);

        // First line: "Hi there!" with positioning commands
        assert_eq!(result.lines[0].time_ms, 0);
        assert_eq!(result.lines[0].pairs.len(), 11);

        // Second line: EDM (Erase Displayed Memory)
        assert_eq!(result.lines[1].time_ms, 2000);
        assert_eq!(result.lines[1].pairs[0], [0x94, 0x2C]);

        // Third line: "Testing SCC input."
        assert_eq!(result.lines[2].time_ms, 4000);

        // Fourth line: EDM
        assert_eq!(result.lines[3].time_ms, 6000);
    }

    #[test]
    fn test_parse_scc_with_callbacks() {
        let content = "Scenarist_SCC V1.0\n\n00:00:00:00\t9420 9420\n00:00:01:00\t942c\n";

        let mut caption_count = 0;
        let mut timing_count = 0;
        let mut last_time_ms: i64 = -1;

        parse_scc_with_callbacks(
            content,
            SccFrameRate::Fps29_97,
            |cc_type, _d1, _d2| {
                assert_eq!(cc_type, CC_TYPE_FIELD1);
                caption_count += 1;
            },
            |time_ms| {
                timing_count += 1;
                last_time_ms = time_ms;
            },
        );

        assert_eq!(caption_count, 3); // 2 from first line + 1 from second
        assert_eq!(timing_count, 2); // One per line
        assert_eq!(last_time_ms, 1000); // Last timing was at 1 second
    }

    #[test]
    fn test_ms_per_frame() {
        // 29.97fps: ~33.37ms per frame
        let fps_29_97 = SccFrameRate::Fps29_97.ms_per_frame();
        assert!(fps_29_97 > 33.0 && fps_29_97 < 34.0);

        // 24fps: ~41.67ms per frame
        let fps_24 = SccFrameRate::Fps24.ms_per_frame();
        assert!(fps_24 > 41.0 && fps_24 < 42.0);

        // 25fps: 40ms per frame
        let fps_25 = SccFrameRate::Fps25.ms_per_frame();
        assert!((fps_25 - 40.0).abs() < 0.001);

        // 30fps: ~33.33ms per frame
        let fps_30 = SccFrameRate::Fps30.ms_per_frame();
        assert!(fps_30 > 33.0 && fps_30 < 34.0);

        // 23.976fps: ~41.71ms per frame
        let fps_23_976 = SccFrameRate::Fps23_976.ms_per_frame();
        assert!((fps_23_976 - 1001.0 / 24.0).abs() < 0.001);
    }
}
