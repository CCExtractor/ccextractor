use crate::bindings::*;

impl dtvcc_window_pd {
    pub fn new(direction: i32) -> Result<Self, String> {
        match direction {
            0 => Ok(dtvcc_window_pd::DTVCC_WINDOW_PD_LEFT_RIGHT),
            1 => Ok(dtvcc_window_pd::DTVCC_WINDOW_PD_RIGHT_LEFT),
            2 => Ok(dtvcc_window_pd::DTVCC_WINDOW_PD_TOP_BOTTOM),
            3 => Ok(dtvcc_window_pd::DTVCC_WINDOW_PD_BOTTOM_TOP),
            _ => Err(String::from("Invalid print direction")),
        }
    }
}
