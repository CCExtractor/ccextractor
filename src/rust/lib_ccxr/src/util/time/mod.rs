//! Provide types for storing time in different formats
//!
//! Time can be represented in one of following formats:
//! - [`Timestamp`] as number of milliseconds
//! - [`MpegClockTick`] as number of clock ticks (as defined in the MPEG standard)
//! - [`FrameCount`] as number of frames
//! - [`GopTimeCode`] as a GOP time code (as defined in the MPEG standard)
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
//! | `print_mstime_static`                                                                                                                                                                                              | [`Timestamp::to_hms_millis_time`]                     |
//! | `gop_accepted`                                                                                                                                                                                                     | [`GopTimeCode::did_rollover`] + some additional logic |
//! | `calculate_ms_gop_time`                                                                                                                                                                                            | [`GopTimeCode::new`], [`GopTimeCode::timestamp`]      |

mod units;

pub mod c_functions;

pub use units::*;
