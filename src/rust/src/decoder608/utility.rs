use crate::{bindings::*, utils::is_true};

extern "C" {
    static mut temp_debug: i32;
 //   static mut ccx_options: ccx_s_options;
}

pub fn dump(mask: i64, start: &str, l: i32, abs_start: u32, clear_high_bit: u32) {
    //TODO ccx_options
    unsafe {
//        let t: i64 = if is_true(temp_debug) { ccx_options.debug_mask_on_debug | ccx_options.debug_mask } else { ccx_options.debug_mask };
    }
    if !is_true((mask & l as i64 ) as i32) {return}

    for x in (0..l).step_by(16){
       // TODO
    }
    unimplemented!();
}