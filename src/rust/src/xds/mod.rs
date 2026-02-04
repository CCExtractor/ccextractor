//! XDS (Extended Data Services) decoder module for processing CEA-608 extended data.
//!
//! This module provides Rust implementations for decoding XDS packets embedded in
//! closed caption streams, including program information, content ratings, network
//! identification, and time-of-day data.
//!
//! # Submodules
//!
//! - [`handlers`] - XDS packet processing and dispatch functions
//! - [`types`] - XDS-specific types, enums, and constants
//!
//! For detailed function-level mappings, see the [`handlers`] module documentation
//! For type definitions, see the [`types`] module

pub mod handlers;
pub mod types;

use crate::bindings::*;
use crate::ctorust::FromCType;
use crate::libccxr_exports::time::apply_timing_info;
use crate::libccxr_exports::time::generate_timing_context;
use crate::xds::handlers::{do_end_of_xds, TS_START_OF_XDS};
use crate::xds::types::{copy_xds_context_from_rust_to_c, CcxDecodersXdsContext};
use std::os::raw::c_int;
use std::sync::atomic::Ordering;

/// FFI wrapper for `do_end_of_xds`.
///
/// Finalizes XDS packet processing, validates checksum, and dispatches to appropriate handler.
///
/// # Safety
/// - `sub` must be a valid, non-null pointer to a `cc_subtitle` struct.
/// - `ctx` must be a valid, non-null pointer to a `ccx_decoders_xds_context` struct.
/// - The caller must ensure both pointers remain valid for the duration of the call.
#[no_mangle]
pub unsafe extern "C" fn ccxr_do_end_of_xds(
    sub: *mut cc_subtitle,
    ctx: *mut ccx_decoders_xds_context,
    expected_checksum: u8,
) {
    // Null pointer checks
    if sub.is_null() || ctx.is_null() {
        return;
    }

    // trying to sync cb_field2 etc globals from C to GLOBAL_TIMING_INFO. hope this works
    apply_timing_info();

    // Convert C context to Rust
    let mut rust_ctx = match CcxDecodersXdsContext::from_ctype(*ctx) {
        Some(c) => c,
        None => return,
    };

    // populate timing from the C ctx's timing ptr
    let c_timing_ptr = (*ctx).timing;
    let mut timing_ctx = if !c_timing_ptr.is_null() {
        Some(generate_timing_context(c_timing_ptr))
    } else {
        None
    };
    rust_ctx.timing = timing_ctx.as_mut();

    // Call the Rust implementation
    do_end_of_xds(&mut *sub, &mut rust_ctx, expected_checksum);

    // Write changes back to C context
    copy_xds_context_from_rust_to_c(ctx, &rust_ctx);
}

/// FFI wrapper for `process_xds_bytes`.
///
/// Processes a pair of XDS data bytes, managing buffer allocation and packet state.
///
/// # Safety
/// - `ctx` must be a valid, non-null pointer to a `ccx_decoders_xds_context` struct.
/// - The caller must ensure the pointer remains valid for the duration of the call.
#[no_mangle]
pub unsafe extern "C" fn ccxr_process_xds_bytes(
    ctx: *mut ccx_decoders_xds_context,
    hi: u8,
    lo: c_int,
) {
    if ctx.is_null() {
        return;
    }

    // Convert C context to Rust
    let mut rust_ctx = match CcxDecodersXdsContext::from_ctype(*ctx) {
        Some(c) => c,
        None => return,
    };

    // Process in Rust
    rust_ctx.process_xds_bytes(hi, lo as u8);

    // Write changes back to C
    copy_xds_context_from_rust_to_c(ctx, &rust_ctx);
}

/// setter function for TS_START_OF_XDS from C code
#[no_mangle]
pub extern "C" fn ccxr_set_ts_start_of_xds(value: i64) {
    TS_START_OF_XDS.store(value, Ordering::SeqCst);
}
