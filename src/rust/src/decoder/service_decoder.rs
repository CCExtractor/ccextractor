use log::debug;

use crate::bindings::*;

impl dtvcc_service_decoder {
    pub fn screen_print(&mut self, encoder: &mut encoder_ctx, timing: &mut ccx_common_timing_ctx) {
        debug!("dtvcc_screen_print rust");
        self.cc_count += 1;
        unsafe {
            (*self.tv).cc_count += 1;
            let sn = (*self.tv).service_number;
            let writer = &mut encoder.dtvcc_writers[(sn - 1) as usize];
            dtvcc_screen_update_time_hide(self.tv, get_visible_end(timing, 3));
            dtvcc_writer_output(writer, self, encoder);
            dtvcc_tv_clear(self);
        }
    }
}
