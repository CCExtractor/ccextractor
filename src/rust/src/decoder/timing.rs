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
    pub fn get_visible_end(&mut self, current_field: u8) -> LLONG {
        let fts = self.get_fts(current_field);
        if fts > self.minimum_fts {
            self.minimum_fts = fts;
        }
        debug!("Visible End time={}", get_time_str(fts));
        fts
    }
    /// Returns a FTS that is guaranteed to be at least 1 ms later than the end of the previous screen, so that there's no timing overlap
    pub fn get_visible_start(self, current_field: u8) -> LLONG {
        let mut fts = self.get_fts(current_field);
        if fts <= self.minimum_fts {
            fts = self.minimum_fts + 1;
        }
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
    format!("{:02}:{:02}:{:02},{:03}", hh, mm, ss, ms)
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
    let frame: u8 = (((time.time_in_ms
        - 1000 * ((time.ss as i64) + 60 * ((time.mm as i64) + 60 * (time.hh as i64))))
        as f64)
        * 29.97
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

        // Case 1: fts < minimum_fts
        assert_eq!(ctx.get_visible_end(3), 393);
        assert_eq!(ctx.minimum_fts, 500); // No change in minimum_fts

        // Case 2: fts > minimum_fts
        assert_eq!(ctx.get_visible_end(1), 727);
        assert_eq!(ctx.minimum_fts, 727); // Change minimum_fts
    }

    #[test]
    fn test_get_visible_start() {
        set_tmp_cb_values();
        let mut ctx = get_temp_timing_ctx();
        ctx.minimum_fts = 500;

        // Case 1: fts <= minimum_fts
        assert_eq!(ctx.get_visible_start(3), 501);

        // Case 2: fts <= minimum_fts
        assert_eq!(ctx.get_visible_start(1), 727);
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
