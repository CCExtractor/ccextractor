use log::debug;

use super::{timing::get_time_str, CCX_DTVCC_SCREENGRID_ROWS};
use crate::bindings::*;

impl dtvcc_tv_screen {
    /// Clear text from tv screen
    pub fn clear(&mut self) {
        for row in 0..CCX_DTVCC_SCREENGRID_ROWS as usize {
            self.chars[row].fill(dtvcc_symbol::default());
        }
        self.time_ms_hide = -1;
        self.time_ms_show = -1;
    }
    pub fn update_time_show(&mut self, time: LLONG) {
        let prev_time_str = get_time_str(self.time_ms_show);
        let curr_time_str = get_time_str(time);
        debug!("Screen show time: {} -> {}", prev_time_str, curr_time_str);
        if self.time_ms_show == -1 || self.time_ms_show > time {
            self.time_ms_show = time;
        }
    }
    pub fn update_time_hide(&mut self, time: LLONG) {
        let prev_time_str = get_time_str(self.time_ms_hide);
        let curr_time_str = get_time_str(time);
        debug!("Screen hide time: {} -> {}", prev_time_str, curr_time_str);
        if self.time_ms_hide == -1 || self.time_ms_hide < time {
            self.time_ms_hide = time;
        }
    }
}
