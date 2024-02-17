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
