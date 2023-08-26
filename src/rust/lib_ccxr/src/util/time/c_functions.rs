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

/// Rust equivalent for `add_current_pts` function in C. Uses Rust-native types as input and output.
pub fn add_current_pts(ctx: &mut TimingContext, pts: MpegClockTick) {
    ctx.add_current_pts(pts)
}

/// Rust equivalent for `set_current_pts` function in C. Uses Rust-native types as input and output.
pub fn set_current_pts(ctx: &mut TimingContext, pts: MpegClockTick) {
    ctx.set_current_pts(pts)
}

/// Rust equivalent for `set_fts` function in C. Uses Rust-native types as input and output.
pub fn set_fts(ctx: &mut TimingContext) -> bool {
    ctx.set_fts()
}

/// Rust equivalent for `get_fts` function in C. Uses Rust-native types as input and output.
pub fn get_fts(ctx: &mut TimingContext, current_field: CaptionField) -> Timestamp {
    ctx.get_fts(current_field)
}

/// Rust equivalent for `get_fts_max` function in C. Uses Rust-native types as input and output.
pub fn get_fts_max(ctx: &mut TimingContext) -> Timestamp {
    ctx.get_fts_max()
}

/// Rust equivalent for `print_mstime_static` function in C. Uses Rust-native types as input and output.
pub fn print_mstime_static(mstime: Timestamp, sep: char) -> String {
    mstime.to_hms_millis_time(sep).unwrap()
}

/// Rust equivalent for `print_debug_timing` function in C. Uses Rust-native types as input and output.
pub fn print_debug_timing(ctx: &mut TimingContext) {
    ctx.print_debug_timing()
}

/// Rust equivalent for `calculate_ms_gop_time` function in C. Uses Rust-native types as input and output.
pub fn calculate_ms_gop_time(g: GopTimeCode) -> Timestamp {
    g.timestamp()
}

/// Rust equivalent for `gop_accepted` function in C. Uses Rust-native types as input and output.
pub fn gop_accepted(g: GopTimeCode) -> bool {
    let mut timing_info = GLOBAL_TIMING_INFO.write().unwrap();

    let gop_time = if let Some(gt) = timing_info.gop_time {
        gt
    } else {
        return true;
    };

    if g.did_rollover(&gop_time) {
        timing_info.gop_rollover = true;
        true
    } else {
        gop_time.timestamp() <= g.timestamp()
    }
}
