use crate::common::FrameType;
use crate::util::log::{debug, info, DebugMessageFlag};
use crate::util::time::{FrameCount, GopTimeCode, MpegClockTick, Timestamp};
use std::sync::RwLock;

/// The maximum allowed difference between [`TimingContext::current_pts`] and [`TimingContext::sync_pts`] in seconds.
///
/// If the difference crosses this value, a PTS jump has occured and is handled accordingly.
const MAX_DIF: i64 = 5;

/// A unique global instance of [`GlobalTimingInfo`] to be used throughout the program.
pub static GLOBAL_TIMING_INFO: RwLock<GlobalTimingInfo> = RwLock::new(GlobalTimingInfo::new());

const DEFAULT_FRAME_RATE: f64 = 30000.0 / 1001.0;

/// Represents the status of [`TimingContext::current_pts`] and [`TimingContext::min_pts`]
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum PtsSet {
    No,
    Received,
    MinPtsSet,
}

/// Represent the field of 608 or 708 caption.
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum CaptionField {
    Field1,
    Field2,
    Cea708,
}

/// A collective struct for storing timing-related information when decoding a file.
///
/// [`GlobalTimingInfo`] serves a similar purpose. The only difference is that its lifetime is
/// global.
#[derive(Debug)]
pub struct TimingContext {
    pub pts_set: PtsSet,
    /// if true then don't adjust again.
    min_pts_adjusted: bool,
    pub current_pts: MpegClockTick,
    pub current_picture_coding_type: FrameType,
    /// Store temporal reference of current frame.
    pub current_tref: FrameCount,
    pub min_pts: MpegClockTick,
    pub sync_pts: MpegClockTick,
    /// No screen should start before this FTS
    pub minimum_fts: Timestamp,
    /// Time stamp of current file (w/ fts_offset, w/o fts_global).
    pub fts_now: Timestamp,
    /// Time before first sync_pts.
    pub fts_offset: Timestamp,
    /// Time before first GOP.
    pub fts_fc_offset: Timestamp,
    /// Remember the maximum fts that we saw in current file.
    pub fts_max: Timestamp,
    /// Duration of previous files (-ve mode).
    pub fts_global: Timestamp,
    pub sync_pts2fts_set: bool,
    pub sync_pts2fts_fts: Timestamp,
    pub sync_pts2fts_pts: MpegClockTick,
    /// PTS resets when current_pts is lower than prev.
    pts_reset: bool,
}

/// Settings for overall timing functionality in [`TimingContext`].
#[derive(Debug)]
pub struct TimingSettings {
    /// If true, timeline jumps will be ignored. This is important in several input formats that are assumed to have correct timing, no matter what.
    pub disable_sync_check: bool,

    /// If true, there will be no sync at all. Mostly useful for debugging.
    pub no_sync: bool,

    // Needs to be set, as it's used in set_fts.
    pub is_elementary_stream: bool,
}

/// A collective struct to store global timing-related information while decoding a file.
///
/// [`TimingContext`] serves a similar purpose. The only difference is that its lifetime is not
/// global and its information could be reset while execution of program.
#[derive(Debug)]
pub struct GlobalTimingInfo {
    // Count 608 (per field) and 708 blocks since last set_fts() call
    pub cb_field1: u64,
    pub cb_field2: u64,
    pub cb_708: u64,
    pub pts_big_change: bool,
    pub current_fps: f64,
    pub frames_since_ref_time: FrameCount,
    pub total_frames_count: FrameCount,
    pub gop_time: Option<GopTimeCode>,
    pub first_gop_time: Option<GopTimeCode>,
    pub fts_at_gop_start: Timestamp,
    pub gop_rollover: bool,
    pub timing_settings: TimingSettings,
}

impl TimingContext {
    /// Create a new [`TimingContext`].
    pub fn new() -> TimingContext {
        TimingContext {
            pts_set: PtsSet::No,
            min_pts_adjusted: false,
            current_pts: MpegClockTick::new(0),
            current_picture_coding_type: FrameType::ResetOrUnknown,
            current_tref: FrameCount::new(0),
            min_pts: MpegClockTick::new(0x01FFFFFFFF),
            sync_pts: MpegClockTick::new(0),
            minimum_fts: Timestamp::from_millis(0),
            fts_now: Timestamp::from_millis(0),
            fts_offset: Timestamp::from_millis(0),
            fts_fc_offset: Timestamp::from_millis(0),
            fts_max: Timestamp::from_millis(0),
            fts_global: Timestamp::from_millis(0),
            sync_pts2fts_set: false,
            sync_pts2fts_fts: Timestamp::from_millis(0),
            sync_pts2fts_pts: MpegClockTick::new(0),
            pts_reset: false,
        }
    }

    /// Add `pts` to `TimingContext::current_pts`.
    ///
    /// It also checks for PTS resets.
    pub fn add_current_pts(&mut self, pts: MpegClockTick) {
        self.set_current_pts(self.current_pts + pts)
    }

    /// Set `TimingContext::current_pts` to `pts`.
    ///
    /// It also checks for PTS resets.
    pub fn set_current_pts(&mut self, pts: MpegClockTick) {
        let prev_pts = self.current_pts;
        self.current_pts = pts;
        if self.pts_set == PtsSet::No {
            self.pts_set = PtsSet::Received
        }
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "PTS: {} ({:8})", self.current_pts.as_timestamp().to_hms_millis_time(':').unwrap(), self.current_pts.as_i64());
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "  FTS: {} \n", self.fts_now.to_hms_millis_time(':').unwrap());

        // Check if PTS reset
        if self.current_pts < prev_pts {
            self.pts_reset = true;
        }
    }

    pub fn set_fts(&mut self) -> bool {
        let mut timing_info = GLOBAL_TIMING_INFO.write().unwrap();

        let mut pts_jump = false;

        // ES don't have PTS unless GOP timing is used
        if self.pts_set == PtsSet::No && timing_info.timing_settings.is_elementary_stream {
            return true;
        }

        // First check for timeline jump (only when min_pts was set (implies sync_pts)).
        if self.pts_set == PtsSet::MinPtsSet {
            let dif = if timing_info.timing_settings.disable_sync_check {
                // Disables sync check. Used for several input formats.
                0
            } else {
                (self.current_pts - self.sync_pts).as_timestamp().seconds()
            };

            // This was -0.2 before. TODO: find out why, since dif is integer?
            if !(0..MAX_DIF).contains(&dif) {
                // ATSC specs: More than 3501 ms means missing component
                info!("\nWarning: Reference clock has changed abruptly ({} seconds filepos={}), attempting to synchronize\n", dif, "Unable to get file position"); // TODO: get the file position somehow
                info!("Last sync PTS value: {}\n", self.sync_pts.as_i64());
                info!("Current PTS value: {}\n", self.current_pts.as_i64());
                info!("Note: You can disable this behavior by adding -ignoreptsjumps to the parameters.\n");

                pts_jump = true;
                timing_info.pts_big_change = true;

                // Discard the gap if it is not on an I-frame or temporal reference zero.
                if self.current_tref.as_u64() != 0
                    && self.current_picture_coding_type != FrameType::IFrame
                {
                    self.fts_now = self.fts_max;
                    info!("Change did not occur on first frame - probably a broken GOP\n");
                    return true;
                }
            }
        }

        // If min_pts was set just before a rollover we compensate by "roll-oving" it too
        if self.pts_set == PtsSet::MinPtsSet && !self.min_pts_adjusted {
            // min_pts set
            // We want to be aware of the upcoming rollover, not after it happened, so we don't take
            // the 3 most significant bits but the 3 next ones
            let min_pts_big_bits = (self.min_pts.as_i64() >> 30) & 7;
            let cur_pts_big_bits = (self.current_pts.as_i64() >> 30) & 7;
            if cur_pts_big_bits == 7 && min_pts_big_bits == 0 {
                // Huge difference possibly means the first min_pts was actually just over the boundary
                // Take the current_pts (smaller, accounting for the rollover) instead
                self.min_pts = self.current_pts;
                self.min_pts_adjusted = true;
            } else if (1..=6).contains(&cur_pts_big_bits) {
                // Far enough from the boundary
                // Prevent the eventual difference with min_pts to make a bad adjustment
                self.min_pts_adjusted = true;
            }
        }

        // Set min_pts, fts_offset
        if self.pts_set != PtsSet::No {
            self.pts_set = PtsSet::MinPtsSet;

            // Use this part only the first time min_pts is set. Later treat
            // it as a reference clock change
            if self.current_pts < self.min_pts && !pts_jump {
                // If this is the first GOP, and seq 0 was not encountered yet
                // we might reset min_pts/fts_offset again

                self.min_pts = self.current_pts;

                // Avoid next async test
                self.sync_pts = self.current_pts
                    - self
                        .current_tref
                        .as_mpeg_clock_tick(timing_info.current_fps);

                if self.current_tref.as_u64() == 0
                    || (timing_info.total_frames_count - timing_info.frames_since_ref_time).as_u64()
                        == 0
                {
                    // Earliest time in GOP.
                    // OR
                    // If this is the first frame (PES) there cannot be an offset.
                    // This part is also reached for dvr-ms/NTSC (RAW) as
                    // total_frames_count = frames_since_ref_time = 0 when
                    // this is called for the first time.
                    self.fts_offset = Timestamp::from_millis(0);
                } else {
                    // It needs to be "+1" because the current frame is
                    // not yet counted.
                    let one_frame = FrameCount::new(1);
                    self.fts_offset = (timing_info.total_frames_count
                        - timing_info.frames_since_ref_time
                        + one_frame)
                        .as_timestamp(timing_info.current_fps);
                }
                debug!(
                    msg_type = DebugMessageFlag::TIME;
                    "\nFirst sync time    PTS: {} {:+}ms (time before this PTS)\n",
                    self.min_pts.as_timestamp().to_hms_millis_time(':').unwrap(),
                    self.fts_offset.millis()
                );
                debug!(
                    msg_type = DebugMessageFlag::TIME;
                    "Total_frames_count {} frames_since_ref_time {}\n",
                    timing_info.total_frames_count.as_u64(),
                    timing_info.frames_since_ref_time.as_u64()
                );
            }

            // -nosync disables syncing
            if pts_jump && !timing_info.timing_settings.no_sync {
                // The current time in the old time base is calculated using
                // sync_pts (set at the beginning of the last GOP) plus the
                // time of the frames since then.
                self.fts_offset = self.fts_offset
                    + (self.sync_pts - self.min_pts).as_timestamp()
                    + timing_info
                        .frames_since_ref_time
                        .as_timestamp(timing_info.current_fps);
                self.fts_max = self.fts_offset;

                // Start counting again from here
                self.pts_set = PtsSet::Received; // Force min to be set again
                self.sync_pts2fts_set = false; // Make note of the new conversion values

                // Avoid next async test - the gap might have occured on
                // current_tref != 0.
                self.sync_pts = self.current_pts
                    - self
                        .current_tref
                        .as_mpeg_clock_tick(timing_info.current_fps);
                // Set min_pts = sync_pts as this is used for fts_now
                self.min_pts = self.sync_pts;

                debug!(
                    msg_type = DebugMessageFlag::TIME;
                    "\nNew min PTS time: {} {:+}ms (time before this PTS)\n",
                    self.min_pts.as_timestamp().to_hms_millis_time(':').unwrap(),
                    self.fts_offset.millis()
                );
            }
        }

        // Set sync_pts, fts_offset
        if self.current_tref.as_u64() == 0 {
            self.sync_pts = self.current_pts;
        }

        // Reset counters
        timing_info.cb_field1 = 0;
        timing_info.cb_field2 = 0;
        timing_info.cb_708 = 0;

        // Avoid wrong "Calc. difference" and "Asynchronous by" numbers
        // for uninitialized min_pts
        if true {
            // CFS: Remove or think decent condition
            if self.pts_set != PtsSet::No {
                // If pts_set is TRUE we have min_pts
                self.fts_now = (self.current_pts - self.min_pts).as_timestamp() + self.fts_offset;
                if !self.sync_pts2fts_set {
                    self.sync_pts2fts_pts = self.current_pts;
                    self.sync_pts2fts_fts = self.fts_now;
                    self.sync_pts2fts_set = true;
                }
            } else {
                // No PTS info at all!!
                info!("Set PTS called without any global timestamp set\n");
                return false;
            }
        }
        if self.fts_now > self.fts_max {
            self.fts_max = self.fts_now;
        }

        // If PTS resets, then fix minimum_fts and fts_max
        if self.pts_reset {
            self.minimum_fts = Timestamp::from_millis(0);
            self.fts_max = self.fts_now;
            self.pts_reset = false;
        }

        true
    }

    /// Returns the total FTS.
    ///
    /// Caption block counters are included.
    pub fn get_fts(&self, current_field: CaptionField) -> Timestamp {
        let timing_info = GLOBAL_TIMING_INFO.read().unwrap();
        let count = match current_field {
            CaptionField::Field1 => timing_info.cb_field1,
            CaptionField::Field2 => timing_info.cb_field2,
            CaptionField::Cea708 => timing_info.cb_708,
        };
        self.fts_now + self.fts_global + Timestamp::from_millis((count as i64) * 1001 / 30)
    }

    /// This returns the maximum FTS that belonged to a frame.
    ///
    /// Caption block counters are not applicable.
    pub fn get_fts_max(&self) -> Timestamp {
        self.fts_max + self.fts_global
    }

    /// Log FTS and PTS information for debugging purpose.
    pub fn print_debug_timing(&self) {
        let zero = Timestamp::from_millis(0);
        let timing_info = GLOBAL_TIMING_INFO.read().unwrap();

        let gop_time = timing_info.gop_time.map(|x| x.timestamp()).unwrap_or(zero);
        let first_gop_time = timing_info
            .first_gop_time
            .map(|x| x.timestamp())
            .unwrap_or(zero);

        // Avoid wrong "Calc. difference" and "Asynchronous by" numbers
        // for uninitialized min_pts
        let tempmin_pts = if self.min_pts.as_i64() == 0x01FFFFFFFF {
            self.sync_pts
        } else {
            self.min_pts
        };

        info!(
            "Sync time stamps:  PTS: {}                ",
            self.sync_pts
                .as_timestamp()
                .to_hms_millis_time(':')
                .unwrap()
        );
        info!(
            "      GOP start FTS: {}\n",
            gop_time.to_hms_millis_time(':').unwrap()
        );

        // Length first GOP to last GOP
        let goplenms = gop_time - first_gop_time;
        // Length at last sync point
        let ptslenms = (self.sync_pts - tempmin_pts).as_timestamp() + self.fts_offset;

        info!(
            "Last               FTS: {}",
            self.get_fts_max().to_hms_millis_time(':').unwrap()
        );
        info!(
            "      GOP start FTS: {}\n",
            timing_info
                .fts_at_gop_start
                .to_hms_millis_time(':')
                .unwrap()
        );

        let one_frame = FrameCount::new(1).as_timestamp(timing_info.current_fps);

        // Times are based on last GOP and/or sync time
        info!(
            "Max FTS diff. to   PTS:       {:6}ms              GOP:       {:6}ms\n\n",
            (self.get_fts_max() + one_frame - ptslenms)
                .to_hms_millis_time(':')
                .unwrap(),
            (self.get_fts_max() + one_frame - goplenms)
                .to_hms_millis_time(':')
                .unwrap()
        );
    }

    /// Constructs a [`TimingContext`] from its individual fields.
    ///
    /// # Safety
    ///
    /// Make sure that [`TimingContext`] is in a valid state.
    #[allow(clippy::too_many_arguments)]
    pub unsafe fn from_raw_parts(
        pts_set: PtsSet,
        min_pts_adjusted: bool,
        current_pts: MpegClockTick,
        current_picture_coding_type: FrameType,
        current_tref: FrameCount,
        min_pts: MpegClockTick,
        sync_pts: MpegClockTick,
        minimum_fts: Timestamp,
        fts_now: Timestamp,
        fts_offset: Timestamp,
        fts_fc_offset: Timestamp,
        fts_max: Timestamp,
        fts_global: Timestamp,
        sync_pts2fts_set: bool,
        sync_pts2fts_fts: Timestamp,
        sync_pts2fts_pts: MpegClockTick,
        pts_reset: bool,
    ) -> TimingContext {
        TimingContext {
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
        }
    }

    /// Returns the individual fields of a [`TimingContext`].
    ///
    /// # Safety
    ///
    /// Certain fields are supposed to be private.
    #[allow(clippy::type_complexity)]
    pub unsafe fn as_raw_parts(
        &self,
    ) -> (
        PtsSet,
        bool,
        MpegClockTick,
        FrameType,
        FrameCount,
        MpegClockTick,
        MpegClockTick,
        Timestamp,
        Timestamp,
        Timestamp,
        Timestamp,
        Timestamp,
        Timestamp,
        bool,
        Timestamp,
        MpegClockTick,
        bool,
    ) {
        let TimingContext {
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
        } = *self;

        (
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
}

impl GlobalTimingInfo {
    /// Create a new instance of [`GlobalTimingInfo`].
    const fn new() -> GlobalTimingInfo {
        GlobalTimingInfo {
            cb_field1: 0,
            cb_field2: 0,
            cb_708: 0,
            pts_big_change: false,
            current_fps: DEFAULT_FRAME_RATE, // 29.97
            // TODO: Get from framerates_values[] instead
            frames_since_ref_time: FrameCount::new(0),
            total_frames_count: FrameCount::new(0),
            gop_time: None,
            first_gop_time: None,
            fts_at_gop_start: Timestamp::from_millis(0),
            gop_rollover: false,
            timing_settings: TimingSettings {
                disable_sync_check: false,
                no_sync: false,
                is_elementary_stream: false,
            },
        }
    }
}

impl Default for TimingContext {
    fn default() -> Self {
        Self::new()
    }
}
