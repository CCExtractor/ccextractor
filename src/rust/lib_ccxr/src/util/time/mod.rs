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

mod timing;
mod units;

pub use timing::*;
pub use units::*;
