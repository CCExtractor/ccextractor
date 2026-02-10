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
    /// Tracks the first PTS seen (not minimum). Used for H.264 fallback to avoid
    /// using reordered B-frame PTS as reference.
    first_pts: MpegClockTick,
    /// Counts set_fts() calls with unknown frame type. Used to trigger fallback for H.264.
    unknown_frame_count: u32,
    /// Tracks the PTS when we first detect a large gap (>100ms) between pending_min_pts and current_pts.
    /// This indicates transition from B-frames to I/P frames in H.264 streams.
    first_large_gap_pts: MpegClockTick,
    /// Flag indicating whether we've seen a large gap for H.264 fallback.
    seen_large_gap: bool,
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
            first_pts: MpegClockTick::new(0x01FFFFFFFF),
            unknown_frame_count: 0,
            first_large_gap_pts: MpegClockTick::new(0x01FFFFFFFF),
            seen_large_gap: false,
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
            // Note: We set pts_set = MinPtsSet later, only after we actually set min_pts.
            // This is crucial for formats like MP4 c608 tracks where frame type is never known
            // and min_pts would never be set, causing fts_now calculation to use the initial
            // huge min_pts value (0x01FFFFFFFF) which results in negative timestamps.

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

            // Track the minimum PTS seen (regardless of frame type).
            // This is used to detect if leading B/P frames are garbage (large gap to I-frame)
            // or valid frames (small gap to I-frame).
            if self.current_pts < self.pending_min_pts {
                self.pending_min_pts = self.current_pts;
            }

            // Track the first PTS seen (not minimum, just first).
            // For container formats, the first frame's PTS is the correct reference point.
            // Later B-frames may have lower PTS due to display order reordering.
            if self.first_pts.as_i64() == 0x01FFFFFFFF {
                self.first_pts = self.current_pts;
            }

            if is_frame_type_unknown {
                self.unknown_frame_count += 1;
            }

            // Track the first time we see a large gap between pending_min_pts and current_pts.
            // For H.264 streams with B-frame reordering, this gap indicates the transition
            // from low-PTS B-frames to higher-PTS I/P frames. The current_pts at this moment
            // is near the first I-frame, which is the correct timing reference.
            // Use the same threshold as MPEG-2 garbage detection (defined below).
            if !self.seen_large_gap
                && self.pending_min_pts.as_i64() != 0x01FFFFFFFF
                && self.first_pts.as_i64() != 0x01FFFFFFFF
            {
                let gap_ticks = self.current_pts.as_i64() - self.pending_min_pts.as_i64();
                let gap_ms = if timing_info.mpeg_clock_freq > 0 {
                    gap_ticks * 1000 / timing_info.mpeg_clock_freq
                } else {
                    // Assume 90kHz if not set
                    gap_ticks * 1000 / 90000
                };

                if gap_ms > H264_GAP_THRESHOLD_MS {
                    // Large gap detected: save current_pts as the first I-frame reference
                    self.first_large_gap_pts = self.current_pts;
                    self.seen_large_gap = true;
                }
            }

            // Determine if we should allow setting min_pts on this frame.
            // Strategy:
            // - If frame type is UNKNOWN: DON'T set min_pts yet, defer to pending_min_pts.
            //   This is crucial because set_fts() is often called from PES layer BEFORE
            //   the picture header is parsed, so frame type is unknown even for MPEG-2.
            // - If frame type is KNOWN (I/B/P from MPEG-2 picture header):
            //   - I-frame: Check if leading B/P frames were garbage or valid
            //     - If gap between pending_min_pts and I-frame PTS is large (>100ms/3 frames):
            //       These are garbage frames from truncated GOP, use I-frame PTS
            //     - If gap is small (<=100ms): These are valid B-frames, use pending_min_pts
            //   - B/P-frame: Don't set min_pts yet, wait for I-frame to decide
            // - Fallback: If we've processed many frames without seeing a known frame type
            //   (H.264 in MPEG-PS), eventually use pending_min_pts after 100+ calls.
            const FALLBACK_THRESHOLD: u32 = 100;
            // Threshold for MPEG-2 garbage detection: ~100ms (3 frames at 30fps)
            // Gap larger than this suggests garbage leading frames from truncated GOP
            const GARBAGE_GAP_THRESHOLD_MS: i64 = 100;
            // H.264 gap detection: 500ms — must exceed normal B-frame reordering (up to ~300ms)
            // but catch the real content transition gap (634ms+ in test streams)
            const H264_GAP_THRESHOLD_MS: i64 = 500;
            let (allow_min_pts_set, pts_for_min) = if is_frame_type_unknown {
                // Frame type unknown - check if we should use fallback
                if self.unknown_frame_count >= FALLBACK_THRESHOLD
                    && !self.seen_known_frame_type
                    && self.first_pts.as_i64() != 0x01FFFFFFFF
                {
                    // H.264 fallback: Apply garbage gap detection similar to MPEG-2 I-frame logic.
                    // If we detected a large gap (>100ms) between pending_min_pts and a later PTS,
                    // use the PTS from when the gap was first detected (near the first I-frame).
                    // Otherwise, use pending_min_pts (no B-frame reordering detected).
                    if self.seen_large_gap {
                        (true, self.first_large_gap_pts)
                    } else {
                        (true, self.pending_min_pts)
                    }
                } else {
                    (false, self.current_pts)
                }
            } else if is_i_frame {
                // I-frame: Decide whether to use I-frame PTS or pending_min_pts
                if self.pending_min_pts.as_i64() != 0x01FFFFFFFF {
                    let gap_ticks = self.current_pts.as_i64() - self.pending_min_pts.as_i64();
                    let gap_ms = if timing_info.mpeg_clock_freq > 0 {
                        gap_ticks * 1000 / timing_info.mpeg_clock_freq
                    } else {
                        // Assume 90kHz if not set
                        gap_ticks * 1000 / 90000
                    };
                    if gap_ms > GARBAGE_GAP_THRESHOLD_MS {
                        // Large gap: leading frames are garbage, use I-frame PTS
                        (true, self.current_pts)
                    } else {
                        // Small gap: leading frames are valid B-frames, use pending_min_pts
                        (true, self.pending_min_pts)
                    }
                } else {
                    // No pending_min_pts, use I-frame PTS directly
                    (true, self.current_pts)
                }
            } else {
                // B/P-frame: Don't set min_pts yet, wait for I-frame
                (false, self.current_pts)
            };
            // Only set min_pts once (when min_pts_is_initial is true).
            // This matches FFmpeg's behavior which uses the first I-frame's PTS.
            // B-frames that arrive later (in decode order) with lower PTS (display order)
            // should NOT update min_pts - they're normal B-frame reordering, not garbage frames.
            //
            // The garbage_gap threshold logic handles the case where:
            // - Garbage frames come BEFORE the I-frame in the stream
            // - They set pending_min_pts to a value much lower than I-frame PTS
            // - When we see the I-frame, we check the gap and use pending_min_pts if small
            if pts_for_min < self.min_pts && !pts_jump && min_pts_is_initial && allow_min_pts_set {
                // If this is the first GOP, and seq 0 was not encountered yet
                // we might reset min_pts/fts_offset again

                self.min_pts = pts_for_min;
                // Mark that min_pts has been set - this enables fts_now calculation
                self.pts_set = PtsSet::MinPtsSet;

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

                // Reset sync_pts2fts tracking for the new timeline
                self.sync_pts2fts_set = false;

                // Set new sync_pts accounting for any temporal reference offset.
                // The gap might have occurred on current_tref != 0.
                self.sync_pts = self.current_pts
                    - self
                        .current_tref
                        .as_mpeg_clock_tick(timing_info.current_fps, timing_info.mpeg_clock_freq);

                // Set min_pts to sync_pts and mark as set so fts_now calculation works.
                // This is essential - without setting pts_set to MinPtsSet, fts_now would
                // stop being updated after a PTS jump, causing all subsequent timestamps
                // to be stuck (fixes issue #1277).
                self.min_pts = self.sync_pts;
                self.pts_set = PtsSet::MinPtsSet;

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
        if self.pts_set == PtsSet::MinPtsSet {
            // min_pts has been set, we can calculate fts_now
            self.fts_now = (self.current_pts - self.min_pts)
                .as_timestamp(timing_info.mpeg_clock_freq)
                + self.fts_offset;
            if !self.sync_pts2fts_set {
                self.sync_pts2fts_pts = self.current_pts;
                self.sync_pts2fts_fts = self.fts_now;
                self.sync_pts2fts_set = true;
            }
        } else if self.pts_set == PtsSet::No {
            // No PTS info at all!!
            info!("Set PTS called without any global timestamp set\n");
            return false;
        }
        // pts_set == Received: PTS received but min_pts not yet set (waiting for I-frame)
        // Keep fts_now at its previous value until min_pts is established

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
        first_pts: MpegClockTick,
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
        first_large_gap_pts: MpegClockTick,
        seen_large_gap: bool,
    ) -> TimingContext {
        TimingContext {
            pts_set,
            min_pts_adjusted,
            seen_known_frame_type,
            pending_min_pts,
            first_pts,
            unknown_frame_count,
            first_large_gap_pts,
            seen_large_gap,
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
        MpegClockTick,
        bool,
    ) {
        let TimingContext {
            pts_set,
            min_pts_adjusted,
            seen_known_frame_type,
            pending_min_pts,
            first_pts,
            unknown_frame_count,
            first_large_gap_pts,
            seen_large_gap,
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
            first_pts,
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
            first_large_gap_pts,
            seen_large_gap,
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
