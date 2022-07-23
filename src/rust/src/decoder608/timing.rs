use crate::{bindings::*, cb_field1, cb_field2};
use log::{ error};

pub fn millis_to_time(milli: u64, hours: &mut u8, minutes: &mut u8, seconds: &mut u8, ms: &mut u16) {
    *ms = (milli % 1000) as u16;
    let mut left_time:u64= (milli - *ms as u64) / 1000; // remainder in seconds
    *seconds = (left_time % 60) as u8;
    left_time = (left_time - *seconds as u64) / 60;
    *minutes = (left_time % 60) as u8;
    left_time = (left_time - *minutes as u64) / 60;
    *hours = left_time as u8;
}

pub fn get_fts(ctx: *mut ccx_common_timing_ctx, current_field: i32) -> LLONG {
    match current_field {
        1 => {  unsafe{ (*ctx).fts_now + (*ctx).fts_global + cb_field1 as i64  * 1001 /30} }
        2 => {  unsafe{ (*ctx).fts_now + (*ctx).fts_global + cb_field2 as i64 * 1001 /30} }
        3 => {  unsafe{ (*ctx).fts_now + (*ctx).fts_global + cb_field1 as i64 * 1001 /30} }
        _ => {
            //TODO: fatal message on get_fts
            error!("field unknown");
            0
        }
    }
}
