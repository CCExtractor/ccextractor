//! Provides Rust equivalent for functions in C. Uses Rust-native types as input and output.

use super::*;

/// Rust equivalent for `timestamp_to_srttime` function in C. Uses Rust-native types as input and
/// output.
pub fn timestamp_to_srttime(
    timestamp: Timestamp,
    buffer: &mut String,
) -> Result<(), TimestampError> {
    timestamp.write_srt_time(buffer)
}

/// Rust equivalent for `timestamp_to_vtttime` function in C. Uses Rust-native types as input and
/// output.
pub fn timestamp_to_vtttime(
    timestamp: Timestamp,
    buffer: &mut String,
) -> Result<(), TimestampError> {
    timestamp.write_vtt_time(buffer)
}

/// Rust equivalent for `millis_to_date` function in C. Uses Rust-native types as input and output.
pub fn millis_to_date(
    timestamp: Timestamp,
    buffer: &mut String,
    date_format: TimestampFormat,
) -> Result<(), TimestampError> {
    timestamp.write_formatted_time(buffer, date_format)
}

/// Rust equivalent for `stringztoms` function in C. Uses Rust-native types as input and output.
pub fn stringztoms(s: &str) -> Option<Timestamp> {
    Timestamp::parse_optional_hhmmss_from_str(s).ok()
}
