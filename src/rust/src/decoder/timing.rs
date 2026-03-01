//! Utilty functions to get timing for captions

use crate::{bindings::*, cb_708, cb_field1, cb_field2};

use log::{debug, error};

impl ccx_common_timing_ctx {
    /// Return the current FTS
    pub fn get_fts(&self, current_field: u8) -> LLONG {
        unsafe {
            match current_field {
                1 => self.fts_now + self.fts_global + cb_field1 as i64 * 1001 / 30,
                2 => self.fts_now + self.fts_global + cb_field2 as i64 * 1001 / 30,
                3 => self.fts_now + self.fts_global + cb_708 as i64 * 1001 / 30,
                _ => {
                    error!("get_fts: Unknown field");
                    0
                }
            }
        }
    }
    /// Returns the current FTS and saves it so it can be used by [get_visible_start][Self::get_visible_start()]
    ///
    /// Uses the base FTS (fts_now + fts_global) without the cb_field offset for accurate timing.
    pub fn get_visible_end(&mut self, _current_field: u8) -> LLONG {
        // Use base FTS without cb_field offset for accurate timing
        let fts = self.fts_now + self.fts_global;
        if fts > self.minimum_fts {
            self.minimum_fts = fts;
        }
        debug!("Visible End time={}", get_time_str(fts));
        fts
    }
    /// Returns a FTS that is guaranteed to be at least 1 ms later than the end of the previous screen, so that there's no timing overlap
    ///
    /// Uses the base FTS (fts_now + fts_global) without the cb_field offset for accurate timing.
    /// This provides correct timing for container formats like MP4 where all caption data
    /// for a frame is bundled together and should use the frame's PTS directly.
    pub fn get_visible_start(self, _current_field: u8) -> LLONG {
        // Use base FTS without cb_field offset for accurate timing
        let base_fts = self.fts_now + self.fts_global;
        let fts = if base_fts <= self.minimum_fts {
            self.minimum_fts + 1
        } else {
            base_fts
        };
        debug!("Visible Start time={}", get_time_str(fts));
        fts
    }
}

/// Returns a hh:mm:ss,ms string of time
pub fn get_time_str(time: LLONG) -> String {
    let hh = time / 1000 / 60 / 60;
    let mm = time / 1000 / 60 - 60 * hh;
    let ss = time / 1000 - 60 * (mm + 60 * hh);
    let ms = time - 1000 * (ss + 60 * (mm + 60 * hh));
    format!("{hh:02}:{mm:02}:{ss:02},{ms:03}")
}

impl ccx_boundary_time {
    /// Returns ccx_boundary_time from given time
    pub fn get_time(time: LLONG) -> Self {
        let hh = time / 1000 / 60 / 60;
        let mm = time / 1000 / 60 - 60 * hh;
        let ss = time / 1000 - 60 * (mm + 60 * hh);

        Self {
            hh: hh as i32,
            mm: mm as i32,
            ss: ss as i32,
            time_in_ms: time,
            set: Default::default(),
        }
    }
}

impl PartialEq for ccx_boundary_time {
    fn eq(&self, other: &Self) -> bool {
        self.hh == other.hh
            && self.mm == other.mm
            && self.ss == other.ss
            && self.time_in_ms == other.time_in_ms
            && self.set == other.set
    }
}

/// Returns a hh:mm:ss;frame string of time for SCC format
pub fn get_scc_time_str(time: ccx_boundary_time) -> String {
    // Feel sorry for formatting:(
    let fps = unsafe { crate::current_fps };
    let frame: u8 = (((time.time_in_ms
        - 1000 * ((time.ss as i64) + 60 * ((time.mm as i64) + 60 * (time.hh as i64))))
        as f64)
        * fps
        / 1000.0) as u8;
    format!("{:02}:{:02}:{:02};{:02}", time.hh, time.mm, time.ss, frame)
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn test_get_fts() {
        set_tmp_cb_values();
        let timing_ctx = get_temp_timing_ctx();

        // Case 1
        assert_eq!(timing_ctx.get_fts(1), 727);
        // Case 2
        assert_eq!(timing_ctx.get_fts(2), 1061);
        // Case 3
        assert_eq!(timing_ctx.get_fts(3), 393);
        // Case 4: Unknown field
        assert_eq!(timing_ctx.get_fts(43), 0);
    }

    #[test]
    fn test_get_visible_end() {
        set_tmp_cb_values();
        let mut ctx = get_temp_timing_ctx();
        ctx.minimum_fts = 500;

        // get_visible_end now uses base FTS (fts_now + fts_global = 20 + 40 = 60) without cb_field offset
        // Case 1: base_fts (60) < minimum_fts (500), minimum_fts unchanged
        assert_eq!(ctx.get_visible_end(3), 60);
        assert_eq!(ctx.minimum_fts, 500); // No change in minimum_fts

        // Case 2: base_fts (60) < minimum_fts (500), still unchanged
        assert_eq!(ctx.get_visible_end(1), 60);
        assert_eq!(ctx.minimum_fts, 500); // Still no change

        // Case 3: base_fts > minimum_fts
        ctx.minimum_fts = 50;
        assert_eq!(ctx.get_visible_end(1), 60);
        assert_eq!(ctx.minimum_fts, 60); // Updated to base_fts
    }

    #[test]
    fn test_get_visible_start() {
        set_tmp_cb_values();
        let mut ctx = get_temp_timing_ctx();
        ctx.minimum_fts = 500;

        // get_visible_start now uses base FTS (fts_now + fts_global = 20 + 40 = 60) without cb_field offset
        // Case 1: base_fts (60) <= minimum_fts (500), so return minimum_fts + 1
        assert_eq!(ctx.get_visible_start(3), 501);

        // Case 2: same, base_fts (60) <= minimum_fts (500)
        assert_eq!(ctx.get_visible_start(1), 501);

        // Case 3: base_fts > minimum_fts
        ctx.minimum_fts = 50;
        assert_eq!(ctx.get_visible_start(1), 60);
    }

    #[test]
    fn test_get_time_str() {
        assert_eq!(get_time_str(0), "00:00:00,000");
        assert_eq!(get_time_str(1000), "00:00:01,000");
        assert_eq!(get_time_str(60000), "00:01:00,000");
        assert_eq!(get_time_str(3600000), "01:00:00,000");
        assert_eq!(get_time_str(86400000), "24:00:00,000");
    }

    #[test]
    fn test_ccx_boundary_get_time() {
        assert_eq!(ccx_boundary_time::get_time(0), ccx_boundary_time::default());
        assert_eq!(
            ccx_boundary_time::get_time(60000),
            ccx_boundary_time {
                hh: 0,
                mm: 1,
                ss: 0,
                time_in_ms: 60000,
                set: Default::default()
            }
        );
        assert_eq!(
            ccx_boundary_time::get_time(3600000),
            ccx_boundary_time {
                hh: 1,
                mm: 0,
                ss: 0,
                time_in_ms: 3600000,
                set: Default::default()
            }
        );
        assert_eq!(
            ccx_boundary_time::get_time(86400000),
            ccx_boundary_time {
                hh: 24,
                mm: 0,
                ss: 0,
                time_in_ms: 86400000,
                set: Default::default()
            }
        );
    }

    #[test]
    fn test_get_scc_time_str() {
        assert_eq!(
            get_scc_time_str(ccx_boundary_time::default()),
            "00:00:00;00"
        );
        assert_eq!(
            get_scc_time_str(ccx_boundary_time {
                hh: 0,
                mm: 0,
                ss: 1,
                time_in_ms: 1000,
                set: Default::default()
            }),
            "00:00:01;00"
        );
        assert_eq!(
            get_scc_time_str(ccx_boundary_time {
                hh: 0,
                mm: 1,
                ss: 0,
                time_in_ms: 60000,
                set: Default::default()
            }),
            "00:01:00;00"
        );
        assert_eq!(
            get_scc_time_str(ccx_boundary_time {
                hh: 1,
                mm: 0,
                ss: 0,
                time_in_ms: 3600000,
                set: Default::default()
            }),
            "01:00:00;00"
        );
        assert_eq!(
            get_scc_time_str(ccx_boundary_time {
                hh: 24,
                mm: 0,
                ss: 0,
                time_in_ms: 86400000,
                set: Default::default()
            }),
            "24:00:00;00"
        );
    }

    fn set_tmp_cb_values() {
        unsafe {
            cb_708 = 10;
            cb_field1 = 20;
            cb_field2 = 30;
        }
    }

    fn get_temp_timing_ctx() -> ccx_common_timing_ctx {
        ccx_common_timing_ctx {
            fts_now: 20,
            fts_global: 40,
            ..Default::default()
        }
    }
}
