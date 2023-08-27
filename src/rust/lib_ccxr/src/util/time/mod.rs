//! Provide types for storing time in different formats and manage timing information while
//! decoding.
//!
//! Time can be represented in one of following formats:
//! - [`Timestamp`] as number of milliseconds
//! - [`MpegClockTick`] as number of clock ticks (as defined in the MPEG standard)
//! - [`FrameCount`] as number of frames
//! - [`GopTimeCode`] as a GOP time code (as defined in the MPEG standard)
//!
//! [`GLOBAL_TIMING_INFO`] and [`TimingContext`] are used for managing time-related information
//! during the deocoding process.
//!
//! # Conversion Guide
//!
//! | From                                                                                                                                                                                                               | To                                                    |
//! |--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------|
//! | `ccx_boundary_time`                                                                                                                                                                                                | [`Option<Timestamp>`](Timestamp)                      |
//! | any fts                                                                                                                                                                                                            | [`Timestamp`]                                         |
//! | `ccx_output_date_format`                                                                                                                                                                                           | [`TimestampFormat`]                                   |
//! | any pts                                                                                                                                                                                                            | [`MpegClockTick`]                                     |
//! | any frame count                                                                                                                                                                                                    | [`FrameCount`]                                        |
//! | `gop_time_code`                                                                                                                                                                                                    | [`GopTimeCode`]                                       |
//! | `current_field`                                                                                                                                                                                                    | [`CaptionField`]                                      |
//! | `ccx_common_timing_ctx.pts_set`                                                                                                                                                                                    | [`PtsSet`]                                            |
//! | `ccx_common_timing_settings_t`                                                                                                                                                                                     | [`TimingSettings`]                                    |
//! | `ccx_common_timing_ctx`                                                                                                                                                                                            | [`TimingContext`]                                     |
//! | `cb_708`, `cb_field1`, `cb_field2`, `pts_big_change`, `current_fps`, `frames_since_ref_time`, `total_frames_count`, `gop_time`, `first_gop_time`, `fts_at_gop_start`, `gop_rollover`, `ccx_common_timing_settings` | [`GlobalTimingInfo`], [`GLOBAL_TIMING_INFO`]          |
//! | `init_timing_ctx`                                                                                                                                                                                                  | [`TimingContext::new`]                                |
//! | `add_current_pts`                                                                                                                                                                                                  | [`TimingContext::add_current_pts`]                    |
//! | `set_current_pts`                                                                                                                                                                                                  | [`TimingContext::set_current_pts`]                    |
//! | `set_fts`                                                                                                                                                                                                          | [`TimingContext::set_fts`]                            |
//! | `get_fts`                                                                                                                                                                                                          | [`TimingContext::get_fts`]                            |
//! | `get_fts_max`                                                                                                                                                                                                      | [`TimingContext::get_fts_max`]                        |
//! | `print_mstime_static`                                                                                                                                                                                              | [`Timestamp::to_hms_millis_time`]                     |
//! | `print_debug_timing`                                                                                                                                                                                               | [`TimingContext::print_debug_timing`]                 |
//! | `gop_accepted`                                                                                                                                                                                                     | [`GopTimeCode::did_rollover`] + some additional logic |
//! | `calculate_ms_gop_time`                                                                                                                                                                                            | [`GopTimeCode::new`], [`GopTimeCode::timestamp`]      |

mod timing;
mod units;

pub mod c_functions;

pub use timing::*;
pub use units::*;
