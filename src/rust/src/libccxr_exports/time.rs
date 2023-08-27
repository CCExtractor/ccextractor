#![allow(clippy::useless_conversion)]

use crate::{
    bindings::*, cb_708, cb_field1, cb_field2, ccx_common_timing_settings as timing_settings,
    current_fps, first_gop_time, frames_since_ref_time, fts_at_gop_start, gop_rollover, gop_time,
    pts_big_change, total_frames_count,
};

use std::convert::TryInto;
use std::ffi::CStr;
use std::os::raw::{c_char, c_int, c_long};

use lib_ccxr::common::FrameType;
use lib_ccxr::util::time::{c_functions as c, *};

/// Helper function that converts a Rust-String (`string`) to C-String (`buffer`).
///
/// # Safety
///
/// `buffer` must have enough allocated space for `string` to fit.
unsafe fn write_string_into_pointer(buffer: *mut c_char, string: &str) {
    let buffer = std::slice::from_raw_parts_mut(buffer as *mut u8, string.len() + 1);
    buffer[..string.len()].copy_from_slice(string.as_bytes());
    buffer[string.len()] = b'\0';
}

/// Rust equivalent for `timestamp_to_srttime` function in C. Uses C-native types as input and
/// output.
///
/// # Safety
///
/// `buffer` must have enough allocated space for the formatted `timestamp` to fit.
#[no_mangle]
pub unsafe extern "C" fn ccxr_timestamp_to_srttime(timestamp: u64, buffer: *mut c_char) {
    let mut s = String::new();
    let timestamp = Timestamp::from_millis(timestamp as i64);

    let _ = c::timestamp_to_srttime(timestamp, &mut s);

    write_string_into_pointer(buffer, &s);
}

/// Rust equivalent for `timestamp_to_vtttime` function in C. Uses C-native types as input and
/// output.
///
/// # Safety
///
/// `buffer` must have enough allocated space for the formatted `timestamp` to fit.
#[no_mangle]
pub unsafe extern "C" fn ccxr_timestamp_to_vtttime(timestamp: u64, buffer: *mut c_char) {
    let mut s = String::new();
    let timestamp = Timestamp::from_millis(timestamp as i64);

    let _ = c::timestamp_to_vtttime(timestamp, &mut s);

    write_string_into_pointer(buffer, &s);
}

/// Rust equivalent for `millis_to_date` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `buffer` must have enough allocated space for the formatted `timestamp` to fit.
#[no_mangle]
pub unsafe extern "C" fn ccxr_millis_to_date(
    timestamp: u64,
    buffer: *mut c_char,
    date_format: ccx_output_date_format,
    millis_separator: c_char,
) {
    let mut s = String::new();
    let timestamp = Timestamp::from_millis(timestamp as i64);
    let date_format = match date_format {
        ccx_output_date_format::ODF_NONE => TimestampFormat::None,
        ccx_output_date_format::ODF_HHMMSS => TimestampFormat::HHMMSS,
        ccx_output_date_format::ODF_HHMMSSMS => TimestampFormat::HHMMSSFFF,
        ccx_output_date_format::ODF_SECONDS => TimestampFormat::Seconds {
            millis_separator: millis_separator as u8 as char,
        },
        ccx_output_date_format::ODF_DATE => TimestampFormat::Date {
            millis_separator: millis_separator as u8 as char,
        },
    };

    let _ = c::millis_to_date(timestamp, &mut s, date_format);

    write_string_into_pointer(buffer, &s);
}

/// Rust equivalent for `stringztoms` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `s` must contain valid utf-8 data and have a nul terminator at the end of the string.
#[no_mangle]
pub unsafe extern "C" fn ccxr_stringztoms(s: *const c_char, bt: *mut ccx_boundary_time) -> c_int {
    let s = CStr::from_ptr(s).to_str().unwrap();

    let option_timestamp = c::stringztoms(s);

    if let Some(timestamp) = option_timestamp {
        if let Ok((h, m, s, _)) = timestamp.as_hms_millis() {
            (*bt).set = 1;
            (*bt).hh = h.into();
            (*bt).mm = m.into();
            (*bt).ss = s.into();
            (*bt).time_in_ms = (timestamp.millis() / 1000) * 1000;
            return 0;
        }
    };

    -1
}

/// Construct a [`TimingContext`] from a pointer of `ccx_common_timing_ctx`.
///
/// It is used to move data of [`TimingContext`] from C to Rust.
///
/// # Safety
///
/// `ctx` should not be null.
unsafe fn generate_timing_context(ctx: *const ccx_common_timing_ctx) -> TimingContext {
    let pts_set = match (*ctx).pts_set {
        0 => PtsSet::No,
        1 => PtsSet::Received,
        2 => PtsSet::MinPtsSet,
        _ => panic!("incorrect value for pts_set"),
    };

    let min_pts_adjusted = (*ctx).min_pts_adjusted != 0;
    let current_pts = MpegClockTick::new((*ctx).current_pts);

    let current_picture_coding_type = match (*ctx).current_picture_coding_type {
        ccx_frame_type::CCX_FRAME_TYPE_RESET_OR_UNKNOWN => FrameType::ResetOrUnknown,
        ccx_frame_type::CCX_FRAME_TYPE_I_FRAME => FrameType::IFrame,
        ccx_frame_type::CCX_FRAME_TYPE_P_FRAME => FrameType::PFrame,
        ccx_frame_type::CCX_FRAME_TYPE_B_FRAME => FrameType::BFrame,
        ccx_frame_type::CCX_FRAME_TYPE_D_FRAME => FrameType::DFrame,
    };

    let current_tref = FrameCount::new((*ctx).current_tref.try_into().unwrap());
    let min_pts = MpegClockTick::new((*ctx).min_pts);
    let sync_pts = MpegClockTick::new((*ctx).sync_pts);
    let minimum_fts = Timestamp::from_millis((*ctx).minimum_fts);
    let fts_now = Timestamp::from_millis((*ctx).fts_now);
    let fts_offset = Timestamp::from_millis((*ctx).fts_offset);
    let fts_fc_offset = Timestamp::from_millis((*ctx).fts_fc_offset);
    let fts_max = Timestamp::from_millis((*ctx).fts_max);
    let fts_global = Timestamp::from_millis((*ctx).fts_global);
    let sync_pts2fts_set = (*ctx).sync_pts2fts_set != 0;
    let sync_pts2fts_fts = Timestamp::from_millis((*ctx).sync_pts2fts_fts);
    let sync_pts2fts_pts = MpegClockTick::new((*ctx).sync_pts2fts_pts);
    let pts_reset = (*ctx).pts_reset != 0;

    TimingContext::from_raw_parts(
        pts_set,
        min_pts_adjusted,
        current_pts,
        current_picture_coding_type,
        current_tref,
        min_pts,
        sync_pts,
        minimum_fts,
        fts_now,
        fts_offset,
        fts_fc_offset,
        fts_max,
        fts_global,
        sync_pts2fts_set,
        sync_pts2fts_fts,
        sync_pts2fts_pts,
        pts_reset,
    )
}

/// Copy the contents [`TimingContext`] to a `ccx_common_timing_ctx`.
///
/// It is used to move data of [`TimingContext`] from Rust to C.
///
/// # Safety
///
/// `ctx` should not be null.
unsafe fn write_back_to_common_timing_ctx(
    ctx: *mut ccx_common_timing_ctx,
    timing_ctx: &TimingContext,
) {
    let (
        pts_set,
        min_pts_adjusted,
        current_pts,
        current_picture_coding_type,
        current_tref,
        min_pts,
        sync_pts,
        minimum_fts,
        fts_now,
        fts_offset,
        fts_fc_offset,
        fts_max,
        fts_global,
        sync_pts2fts_set,
        sync_pts2fts_fts,
        sync_pts2fts_pts,
        pts_reset,
    ) = timing_ctx.as_raw_parts();

    (*ctx).pts_set = match pts_set {
        PtsSet::No => 0,
        PtsSet::Received => 1,
        PtsSet::MinPtsSet => 2,
    };

    (*ctx).min_pts_adjusted = if min_pts_adjusted { 1 } else { 0 };
    (*ctx).current_pts = current_pts.as_i64();

    (*ctx).current_picture_coding_type = match current_picture_coding_type {
        FrameType::ResetOrUnknown => ccx_frame_type::CCX_FRAME_TYPE_RESET_OR_UNKNOWN,
        FrameType::IFrame => ccx_frame_type::CCX_FRAME_TYPE_I_FRAME,
        FrameType::PFrame => ccx_frame_type::CCX_FRAME_TYPE_P_FRAME,
        FrameType::BFrame => ccx_frame_type::CCX_FRAME_TYPE_B_FRAME,
        FrameType::DFrame => ccx_frame_type::CCX_FRAME_TYPE_D_FRAME,
    };

    (*ctx).current_tref = current_tref.as_u64().try_into().unwrap();
    (*ctx).min_pts = min_pts.as_i64();
    (*ctx).sync_pts = sync_pts.as_i64();
    (*ctx).minimum_fts = minimum_fts.millis();
    (*ctx).fts_now = fts_now.millis();
    (*ctx).fts_offset = fts_offset.millis();
    (*ctx).fts_fc_offset = fts_fc_offset.millis();
    (*ctx).fts_max = fts_max.millis();
    (*ctx).fts_global = fts_global.millis();
    (*ctx).sync_pts2fts_set = if sync_pts2fts_set { 1 } else { 0 };
    (*ctx).sync_pts2fts_fts = sync_pts2fts_fts.millis();
    (*ctx).sync_pts2fts_pts = sync_pts2fts_pts.as_i64();
    (*ctx).pts_reset = if pts_reset { 1 } else { 0 };
}

/// Write to [`GLOBAL_TIMING_INFO`] from the equivalent static variables in C.
///
/// It is used to move data of [`GLOBAL_TIMING_INFO`] from C to Rust.
///
/// # Safety
///
/// All the static variables should be initialized and in valid state.
unsafe fn apply_timing_info() {
    let mut timing_info = GLOBAL_TIMING_INFO.write().unwrap();

    timing_info.cb_field1 = cb_field1.try_into().unwrap();
    timing_info.cb_field2 = cb_field2.try_into().unwrap();
    timing_info.cb_708 = cb_708.try_into().unwrap();
    timing_info.pts_big_change = pts_big_change != 0;
    timing_info.current_fps = current_fps;
    timing_info.frames_since_ref_time = FrameCount::new(frames_since_ref_time.try_into().unwrap());
    timing_info.total_frames_count = FrameCount::new(total_frames_count.try_into().unwrap());
    timing_info.gop_time = generate_gop_time_code(gop_time);
    timing_info.first_gop_time = generate_gop_time_code(first_gop_time);
    timing_info.fts_at_gop_start = Timestamp::from_millis(fts_at_gop_start.try_into().unwrap());
    timing_info.gop_rollover = gop_rollover != 0;
    timing_info.timing_settings.disable_sync_check = timing_settings.disable_sync_check != 0;
    timing_info.timing_settings.no_sync = timing_settings.no_sync != 0;
    timing_info.timing_settings.is_elementary_stream = timing_settings.is_elementary_stream != 0;
}

/// Write from [`GLOBAL_TIMING_INFO`] to the equivalent static variables in C.
///
/// It is used to move data of [`GLOBAL_TIMING_INFO`] from Rust to C.
///
/// # Safety
///
/// All the static variables should be initialized and in valid state.
unsafe fn write_back_from_timing_info() {
    let timing_info = GLOBAL_TIMING_INFO.read().unwrap();

    cb_field1 = timing_info.cb_field1.try_into().unwrap();
    cb_field2 = timing_info.cb_field2.try_into().unwrap();
    cb_708 = timing_info.cb_708.try_into().unwrap();
    pts_big_change = if timing_info.pts_big_change { 1 } else { 0 };
    current_fps = timing_info.current_fps;
    frames_since_ref_time = timing_info
        .frames_since_ref_time
        .as_u64()
        .try_into()
        .unwrap();
    total_frames_count = timing_info.total_frames_count.as_u64().try_into().unwrap();
    gop_time = write_gop_time_code(timing_info.gop_time);
    first_gop_time = write_gop_time_code(timing_info.first_gop_time);
    fts_at_gop_start = timing_info.fts_at_gop_start.millis().try_into().unwrap();
    gop_rollover = if timing_info.gop_rollover { 1 } else { 0 };
    timing_settings.disable_sync_check = if timing_info.timing_settings.disable_sync_check {
        1
    } else {
        0
    };
    timing_settings.no_sync = if timing_info.timing_settings.no_sync {
        1
    } else {
        0
    };
    timing_settings.is_elementary_stream = if timing_info.timing_settings.is_elementary_stream {
        1
    } else {
        0
    };
}

/// Construct a [`GopTimeCode`] from `gop_time_code`.
unsafe fn generate_gop_time_code(g: gop_time_code) -> Option<GopTimeCode> {
    if g.inited == 0 {
        None
    } else {
        Some(GopTimeCode::from_raw_parts(
            g.drop_frame_flag != 0,
            g.time_code_hours.try_into().unwrap(),
            g.time_code_minutes.try_into().unwrap(),
            g.time_code_seconds.try_into().unwrap(),
            g.time_code_pictures.try_into().unwrap(),
            Timestamp::from_millis(g.ms),
        ))
    }
}

/// Construct a `gop_time_code` from [`GopTimeCode`].
unsafe fn write_gop_time_code(g: Option<GopTimeCode>) -> gop_time_code {
    if let Some(gop) = g {
        let (
            drop_frame,
            time_code_hours,
            time_code_minutes,
            time_code_seconds,
            time_code_pictures,
            timestamp,
        ) = gop.as_raw_parts();

        gop_time_code {
            drop_frame_flag: if drop_frame { 1 } else { 0 },
            time_code_hours: time_code_hours.try_into().unwrap(),
            time_code_minutes: time_code_minutes.try_into().unwrap(),
            marker_bit: 0,
            time_code_seconds: time_code_seconds.try_into().unwrap(),
            time_code_pictures: time_code_pictures.try_into().unwrap(),
            inited: 1,
            ms: timestamp.millis(),
        }
    } else {
        gop_time_code {
            drop_frame_flag: 0,
            time_code_hours: 0,
            time_code_minutes: 0,
            marker_bit: 0,
            time_code_seconds: 0,
            time_code_pictures: 0,
            inited: 0,
            ms: 0,
        }
    }
}

/// Rust equivalent for `add_current_pts` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `ctx` must not be null.
#[no_mangle]
pub unsafe extern "C" fn ccxr_add_current_pts(ctx: *mut ccx_common_timing_ctx, pts: c_long) {
    apply_timing_info();
    let mut context = generate_timing_context(ctx);

    c::add_current_pts(&mut context, MpegClockTick::new(pts.try_into().unwrap()));

    write_back_to_common_timing_ctx(ctx, &context);
    write_back_from_timing_info();
}

/// Rust equivalent for `set_current_pts` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `ctx` must not be null.
#[no_mangle]
pub unsafe extern "C" fn ccxr_set_current_pts(ctx: *mut ccx_common_timing_ctx, pts: c_long) {
    apply_timing_info();
    let mut context = generate_timing_context(ctx);

    c::set_current_pts(&mut context, MpegClockTick::new(pts.try_into().unwrap()));

    write_back_to_common_timing_ctx(ctx, &context);
    write_back_from_timing_info();
}

/// Rust equivalent for `set_fts` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `ctx` must not be null.
#[no_mangle]
pub unsafe extern "C" fn ccxr_set_fts(ctx: *mut ccx_common_timing_ctx) -> c_int {
    apply_timing_info();
    let mut context = generate_timing_context(ctx);

    let ans = c::set_fts(&mut context);

    write_back_to_common_timing_ctx(ctx, &context);
    write_back_from_timing_info();

    if ans {
        1
    } else {
        0
    }
}

/// Rust equivalent for `get_fts` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `ctx` must not be null. `current_field` must be 1, 2 or 3.
#[no_mangle]
pub unsafe extern "C" fn ccxr_get_fts(
    ctx: *mut ccx_common_timing_ctx,
    current_field: c_int,
) -> c_long {
    apply_timing_info();
    let mut context = generate_timing_context(ctx);

    let caption_field = match current_field {
        1 => CaptionField::Field1,
        2 => CaptionField::Field2,
        3 => CaptionField::Cea708,
        _ => panic!("incorrect value for caption field"),
    };

    let ans = c::get_fts(&mut context, caption_field);

    write_back_to_common_timing_ctx(ctx, &context);
    write_back_from_timing_info();

    ans.millis().try_into().unwrap()
}

/// Rust equivalent for `get_fts_max` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `ctx` must not be null.
#[no_mangle]
pub unsafe extern "C" fn ccxr_get_fts_max(ctx: *mut ccx_common_timing_ctx) -> c_long {
    apply_timing_info();
    let mut context = generate_timing_context(ctx);

    let ans = c::get_fts_max(&mut context);

    write_back_to_common_timing_ctx(ctx, &context);
    write_back_from_timing_info();

    ans.millis().try_into().unwrap()
}

/// Rust equivalent for `print_mstime_static` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `buf` must not be null. It must have sufficient length to hold the time in string form.
#[no_mangle]
pub unsafe extern "C" fn ccxr_print_mstime_static(mstime: c_long, buf: *mut c_char) -> *mut c_char {
    let time = Timestamp::from_millis(mstime.try_into().unwrap());
    let ans = c::print_mstime_static(time, ':');
    write_string_into_pointer(buf, &ans);
    buf
}

/// Rust equivalent for `print_debug_timing` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `ctx` must not be null.
#[no_mangle]
pub unsafe extern "C" fn ccxr_print_debug_timing(ctx: *mut ccx_common_timing_ctx) {
    apply_timing_info();
    let mut context = generate_timing_context(ctx);

    c::print_debug_timing(&mut context);

    write_back_to_common_timing_ctx(ctx, &context);
    write_back_from_timing_info();
}

/// Rust equivalent for `calculate_ms_gop_time` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `g` must not be null.
#[no_mangle]
pub unsafe extern "C" fn ccxr_calculate_ms_gop_time(g: *mut gop_time_code) {
    apply_timing_info();
    let timing_info = GLOBAL_TIMING_INFO.read().unwrap();

    (*g).ms = GopTimeCode::new(
        (*g).drop_frame_flag != 0,
        (*g).time_code_hours.try_into().unwrap(),
        (*g).time_code_minutes.try_into().unwrap(),
        (*g).time_code_seconds.try_into().unwrap(),
        (*g).time_code_pictures.try_into().unwrap(),
        timing_info.current_fps,
        timing_info.gop_rollover,
    )
    .unwrap()
    .timestamp()
    .millis()
}

/// Rust equivalent for `gop_accepted` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `g` must not be null.
#[no_mangle]
pub unsafe extern "C" fn ccxr_gop_accepted(g: *mut gop_time_code) -> c_int {
    if let Some(gop) = generate_gop_time_code(*g) {
        let ans = c::gop_accepted(gop);
        if ans {
            1
        } else {
            0
        }
    } else {
        0
    }
}
