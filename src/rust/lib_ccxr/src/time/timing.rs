use crate::common::{FrameType, FRAMERATES_VALUES};
use crate::time::units::{FrameCount, GopTimeCode, MpegClockTick, Timestamp};
use crate::util::log::{debug, info, DebugMessageFlag};
use std::sync::RwLock;

/// The maximum allowed difference between [`TimingContext::current_pts`] and [`TimingContext::sync_pts`] in seconds.
///
/// If the difference crosses this value, a PTS jump has occured and is handled accordingly.
const MAX_DIF: i64 = 5;

/// A unique global instance of [`GlobalTimingInfo`] to be used throughout the program.
pub static GLOBAL_TIMING_INFO: RwLock<GlobalTimingInfo> = RwLock::new(GlobalTimingInfo::new());

pub const DEFAULT_FRAME_RATE: f64 = 30000.0 / 1001.0;

/// Represents the status of [`TimingContext::current_pts`] and [`TimingContext::min_pts`]
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum PtsSet {
    No = 0,
    Received = 1,
    MinPtsSet = 2,
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
    /// Tracks if we've seen a frame with known type (IFrame/BFrame/PFrame).
    /// Used to detect whether this is a format with known frame types (MPEG-2) or not (H.264).
    seen_known_frame_type: bool,
    /// Tracks the minimum PTS seen while waiting for frame type determination.
    /// Used for H.264 streams where frame types are never set.
    pending_min_pts: MpegClockTick,
    /// Counts set_fts() calls with unknown frame type. Used to trigger fallback for H.264.
    unknown_frame_count: u32,
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
    pub mpeg_clock_freq: i64,
}

impl TimingContext {
    /// Create a new [`TimingContext`].
    pub fn new() -> TimingContext {
        TimingContext {
            pts_set: PtsSet::No,
            min_pts_adjusted: false,
            seen_known_frame_type: false,
            pending_min_pts: MpegClockTick::new(0x01FFFFFFFF),
            unknown_frame_count: 0,
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
        let timing_info = GLOBAL_TIMING_INFO.read().unwrap();

        let prev_pts = self.current_pts;
        self.current_pts = pts;
        if self.pts_set == PtsSet::No {
            self.pts_set = PtsSet::Received
        }
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "PTS: {} ({:8})", self.current_pts.as_timestamp(timing_info.mpeg_clock_freq).to_hms_millis_time(':').unwrap(), self.current_pts.as_i64());
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
                (self.current_pts - self.sync_pts)
                    .as_timestamp(timing_info.mpeg_clock_freq)
                    .seconds()
            };

            // Previously in C equivalent code, its -0.2
            if !(0..=MAX_DIF).contains(&dif) {
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
            // it as a reference clock change.
            // Important: Only set min_pts from the FIRST I-frame (when min_pts
            // is still at its initial high value 0x01FFFFFFFF). Don't set min_pts
            // from B/P frames because:
            // 1. B-frames have lower PTS than I/P-frames due to temporal reordering
            // 2. Trailing B/P frames at stream start (from truncated GOPs) have
            //    earlier PTS than the first decodable I-frame
            // FFmpeg's cc_dec uses the first decoded frame (I-frame) as reference,
            // so we must do the same to match timing.
            let min_pts_is_initial = self.min_pts.as_i64() == 0x01FFFFFFFF;
            let is_i_frame = self.current_picture_coding_type == FrameType::IFrame;
            let is_frame_type_unknown =
                self.current_picture_coding_type == FrameType::ResetOrUnknown;
            // Only set min_pts from an I-frame to match FFmpeg's behavior.
            // FFmpeg's cc_dec uses the first decoded frame (which must be an I-frame) as reference.
            // Streams may have leading B/P frames from truncated GOPs that have earlier PTS.
            // Exception: If frame type is unknown (e.g., H.264 in MPEG-PS where frame type
            // isn't set before set_fts is called), allow setting min_pts on the first frame
            // to avoid timing failures.
            // Track when we first see a frame with known type (not ResetOrUnknown).
            // For MPEG-2, frame type is set after parsing picture header.
            // For H.264 in MPEG-PS, frame type may stay unknown.
            let is_known_frame_type = !is_frame_type_unknown;

            // Track when we first see a frame with known type.
            // This is used to detect whether this is a format where frame types are set (MPEG-2)
            // or never set (H.264 in MPEG-PS).
            if is_known_frame_type && !self.seen_known_frame_type {
                self.seen_known_frame_type = true;
            }

            // Track the minimum PTS seen while frame type is unknown.
            // This is used as a fallback for streams where frame types are never set (H.264).
            if is_frame_type_unknown {
                if self.current_pts < self.pending_min_pts {
                    self.pending_min_pts = self.current_pts;
                }
                self.unknown_frame_count += 1;
            }

            // Determine if we should allow setting min_pts on this frame.
            // Strategy:
            // - If frame type is UNKNOWN: DON'T set min_pts yet, defer to pending_min_pts.
            //   This is crucial because set_fts() is often called from PES layer BEFORE
            //   the picture header is parsed, so frame type is unknown even for MPEG-2.
            // - If frame type is KNOWN (I/B/P from MPEG-2 picture header):
            //   - I-frame: Set min_pts from this frame
            //   - B/P-frame: Don't set min_pts, wait for I-frame
            // - Fallback: If we've processed many frames without seeing a known frame type
            //   (H.264 in MPEG-PS), eventually use pending_min_pts after 100+ calls.
            const FALLBACK_THRESHOLD: u32 = 100;
            let (allow_min_pts_set, pts_for_min) = if is_frame_type_unknown {
                // Frame type unknown - check if we should use fallback
                if self.unknown_frame_count >= FALLBACK_THRESHOLD
                    && !self.seen_known_frame_type
                    && self.pending_min_pts.as_i64() != 0x01FFFFFFFF
                {
                    // H.264 fallback: Use pending_min_pts after threshold
                    (true, self.pending_min_pts)
                } else {
                    (false, self.current_pts)
                }
            } else {
                // Frame type is known (I/B/P), require I-frame
                (is_i_frame, self.current_pts)
            };
            if pts_for_min < self.min_pts && !pts_jump && min_pts_is_initial && allow_min_pts_set {
                // If this is the first GOP, and seq 0 was not encountered yet
                // we might reset min_pts/fts_offset again

                self.min_pts = pts_for_min;

                // Avoid next async test
                self.sync_pts = self.current_pts
                    - self
                        .current_tref
                        .as_mpeg_clock_tick(timing_info.current_fps, timing_info.mpeg_clock_freq);

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
                    self.fts_offset = (timing_info.total_frames_count
                        - timing_info.frames_since_ref_time
                        + FrameCount::new(1))
                    .as_timestamp(timing_info.current_fps);
                }
                debug!(
                    msg_type = DebugMessageFlag::TIME;
                    "\nFirst sync time    PTS: {} {:+}ms (time before this PTS)\n",
                    self.min_pts.as_timestamp(timing_info.mpeg_clock_freq).to_hms_millis_time(':').unwrap(),
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
                    + (self.sync_pts - self.min_pts).as_timestamp(timing_info.mpeg_clock_freq)
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
                        .as_mpeg_clock_tick(timing_info.current_fps, timing_info.mpeg_clock_freq);
                // Set min_pts = sync_pts as this is used for fts_now
                self.min_pts = self.sync_pts;

                debug!(
                    msg_type = DebugMessageFlag::TIME;
                    "\nNew min PTS time: {} {:+}ms (time before this PTS)\n",
                    self.min_pts.as_timestamp(timing_info.mpeg_clock_freq).to_hms_millis_time(':').unwrap(),
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
        // CFS: Remove or think decent condition
        if self.pts_set != PtsSet::No {
            // If pts_set is TRUE we have min_pts
            self.fts_now = (self.current_pts - self.min_pts)
                .as_timestamp(timing_info.mpeg_clock_freq)
                + self.fts_offset;
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
                .as_timestamp(timing_info.mpeg_clock_freq)
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
        let ptslenms = (self.sync_pts - tempmin_pts).as_timestamp(timing_info.mpeg_clock_freq)
            + self.fts_offset;

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
        seen_known_frame_type: bool,
        pending_min_pts: MpegClockTick,
        unknown_frame_count: u32,
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
            seen_known_frame_type,
            pending_min_pts,
            unknown_frame_count,
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
        bool,
        MpegClockTick,
        u32,
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
            seen_known_frame_type,
            pending_min_pts,
            unknown_frame_count,
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
            seen_known_frame_type,
            pending_min_pts,
            unknown_frame_count,
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
            current_fps: FRAMERATES_VALUES[4], // 29.97
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
            mpeg_clock_freq: 0,
        }
    }
}

impl Default for TimingContext {
    fn default() -> Self {
        Self::new()
    }
}
