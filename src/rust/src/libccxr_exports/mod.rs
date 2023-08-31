//! Provides C-FFI functions that are direct equivalent of functions available in C.

use crate::ccx_s_options;
use lib_ccxr::util::log::*;
use std::convert::TryInto;

/// Initializes the logger at the rust side.
///
/// # Safety
///
/// `ccx_options` must not be null and must initialized properly before calling this function.
#[no_mangle]
pub unsafe extern "C" fn ccxr_init_basic_logger(ccx_options: *const ccx_s_options) {
    let debug_mask = DebugMessageFlag::from_bits(
        (*ccx_options)
            .debug_mask
            .try_into()
            .expect("Failed to convert debug_mask to an unsigned integer"),
    )
    .expect("Failed to convert debug_mask to a DebugMessageFlag");

    let debug_mask_on_debug = DebugMessageFlag::from_bits(
        (*ccx_options)
            .debug_mask_on_debug
            .try_into()
            .expect("Failed to convert debug_mask_on_debug to an unsigned integer"),
    )
    .expect("Failed to convert debug_mask_on_debug to a DebugMessageFlag");

    let mask = DebugMessageMask::new(debug_mask, debug_mask_on_debug);
    let gui_mode_reports = (*ccx_options).gui_mode_reports != 0;
    let messages_target = match (*ccx_options).messages_target {
        0 => OutputTarget::Stdout,
        1 => OutputTarget::Stderr,
        2 => OutputTarget::Quiet,
        _ => panic!("incorrect value for messages_target"),
    };

    set_logger(CCExtractorLogger::new(
        messages_target,
        mask,
        gui_mode_reports,
    ))
    .expect("Failed to initialize and setup the logger");
}
