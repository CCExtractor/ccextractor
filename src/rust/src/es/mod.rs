use crate::bindings::{cc_subtitle, encoder_ctx, lib_cc_decode};
use crate::ccx_options;
use crate::es::core::process_m2v;
use lib_ccxr::common::Options;
use lib_ccxr::debug;
use lib_ccxr::util::log::DebugMessageFlag;

pub mod core;
pub mod eau;
pub mod gop;
pub mod pic;
pub mod seq;
pub mod userdata;

/// # Safety
/// This function is unsafe because it dereferences raw pointers from C structs
#[no_mangle]
pub unsafe extern "C" fn ccxr_process_m2v(
    enc_ctx: *mut encoder_ctx,
    dec_ctx: *mut lib_cc_decode,
    data: *const u8,
    length: usize,
    sub: *mut cc_subtitle,
) -> usize {
    if enc_ctx.is_null() || dec_ctx.is_null() || data.is_null() || sub.is_null() {
        // This shouldn't happen
        return 0;
    }
    let mut CcxOptions = Options {
        // that's the only thing that's required for now
        gui_mode_reports: ccx_options.gui_mode_reports != 0,
        ..Default::default()
    };

    let data_slice = std::slice::from_raw_parts(data, length);
    let processedvalue = process_m2v(
        &mut *enc_ctx,
        &mut *dec_ctx,
        data_slice,
        length,
        &mut *sub,
        &mut CcxOptions,
    )
    .unwrap_or(0);
    debug!(msg_type = DebugMessageFlag::VERBOSE;"Data slice is {:p} with length {} \n", data, length);
    processedvalue
}
