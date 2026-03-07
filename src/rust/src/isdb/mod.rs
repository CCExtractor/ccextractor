//! ISDB (Integrated Services Digital Broadcasting) subtitle decoder module.
//!
//! This module provides Rust implementations for decoding ISDB caption subtitles,
//! following the ARIB STD-B24 standard for Brazilian digital TV.
//!
//! # Submodules
//! For the C-to-Rust function mapping, see individual submodule([`leaf`], [`mid`], [`high`], [`types`]) docs :
//! - [`types`] - ISDB-specific types, enums, and constants
//! - [`leaf`]  - Low-level helper functions
//! - [`mid`]   - Mid-level control processing
//! - [`high`]  - High-level parsers
//!
//! # Conversion Guide for FFI entry points for the ISDB subtitle decoder
//! Thin wrappers that bridge C callers to the Rust implementation.
//!
//! | C (ccx_decoders_isdb.c)        | Rust                             |
//! |--------------------------------|----------------------------------|
//! | `init_isdb_decoder`            | [`ccxr_init_isdb_decoder`]       |
//! | `delete_isdb_decoder`          | [`ccxr_delete_isdb_decoder`]     |
//! | `isdb_set_global_time`         | [`ccxr_isdb_set_global_time`]    |
//! | `isdbsub_decode`               | [`ccxr_isdbsub_decode`]          |

use crate::bindings::{cc_subtitle, ccx_encoding_type_CCX_ENC_UTF_8, lib_cc_decode};
use crate::isdb::high::isdb_parse_data_group;
use crate::isdb::types::IsdbSubContext;
use std::ffi::c_void;
use std::os::raw::c_char;
use std::ptr::null_mut;

mod high;
mod leaf;
mod mid;
mod types;

/// Initialize a new ISDB decoder context.
/// Returns an opaque pointer to the context, or null on failure.
///
/// # Safety
/// The returned pointer must be freed with `ccxr_delete_isdb_decoder`.
#[no_mangle]
pub extern "C" fn ccxr_init_isdb_decoder() -> *mut c_void {
    let ctx = Box::new(IsdbSubContext::new());
    Box::into_raw(ctx) as *mut c_void
}

/// Delete an ISDB decoder context.
///
/// # Safety
/// - `ctx` must be a valid pointer returned by `ccxr_init_isdb_decoder`,
///   or null (in which case this is a no-op).
/// - After this call, the pointer is invalid and must not be used.
#[no_mangle]
pub unsafe extern "C" fn ccxr_delete_isdb_decoder(ctx: *mut *mut c_void) {
    // if ctx points to null
    if ctx.is_null() {
        return;
    }

    // if what ctx points to is null
    let ptr = *ctx;
    if ptr.is_null() {
        return;
    }

    // reconstruct box and drop it automatically as box goes out of scope - the rust way
    let _ = Box::from_raw(ptr as *mut IsdbSubContext);
    // set ctx to null
    *ctx = null_mut();
}

/// Set the global timestamp for the ISDB decoder.
///
/// # Safety
/// - `ctx` must be a valid pointer returned by `ccxr_init_isdb_decoder`.
#[no_mangle]
pub unsafe extern "C" fn ccxr_isdb_set_global_time(ctx: *mut c_void, timestamp: u64) -> i32 {
    if ctx.is_null() {
        return -1;
    }
    let ctx = &mut *(ctx as *mut IsdbSubContext);
    ctx.timestamp = timestamp;

    0 // CCX_OK
}

// helper to ccxr_isdbsub_decode (function)
extern "C" {
    fn add_cc_sub_text(
        sub: *mut cc_subtitle,
        str: *mut c_char,
        start_time: i64,
        end_time: i64,
        info: *mut c_char,
        mode: *mut c_char,
        encoding: u32,
    ) -> i32;
}

/// Decode ISDB subtitles from a PES packet.
///
/// # Safety
/// - `dec_ctx` must be a valid pointer to a `lib_cc_decode` whose `private_data`
///   was returned by `ccxr_init_isdb_decoder`.
/// - `buf` must point to `buf_size` valid bytes.
/// - `sub` must be a valid pointer to a `cc_subtitle`.
#[allow(clippy::unnecessary_cast)]
#[no_mangle]
pub unsafe extern "C" fn ccxr_isdbsub_decode(
    dec_ctx: *mut lib_cc_decode,
    buf: *const u8,
    buf_size: usize,
    sub: *mut cc_subtitle,
) -> i32 {
    if dec_ctx.is_null() || buf.is_null() || sub.is_null() || buf_size == 0 {
        return -1;
    }

    let dec = &*dec_ctx;
    let data = std::slice::from_raw_parts(buf, buf_size);

    let mut pos: usize = 0;

    // check synced PES marker (0x80)
    if data[pos] != 0x80 {
        log::debug!("Not a Synchronised PES");
        return -1;
    }
    pos += 1;

    // skip pvt data stream byte (0xFF)
    pos += 1;

    if pos >= data.len() {
        return -1;
    }

    // parse header length; skip header
    let header_end = pos + 1 + (data[pos] as usize & 0x0f);
    pos += 1;

    while pos < header_end && pos < data.len() {
        pos += 1;
    }

    if pos >= data.len() {
        return -1;
    }

    // get Rust ISDB context from private_data
    let ctx = &mut *(dec.private_data as *mut IsdbSubContext);
    ctx.cfg_no_rollup = dec.no_rollup != 0;

    let (ret, subtitle_data) = isdb_parse_data_group(ctx, &data[pos..]);

    if ret < 0 {
        return -1;
    }

    if let Some(sub_data) = subtitle_data {
        let mut text_buf = sub_data.text;
        text_buf.push(0);

        let mut info = b"NA\0".to_vec();
        let mut mode = b"ISDB\0".to_vec();

        add_cc_sub_text(
            sub,
            text_buf.as_mut_ptr() as *mut c_char,
            sub_data.start_time as i64,
            sub_data.end_time as i64,
            info.as_mut_ptr() as *mut c_char,
            mode.as_mut_ptr() as *mut c_char,
            ccx_encoding_type_CCX_ENC_UTF_8 as u32,
        );

        if (*sub).start_time == (*sub).end_time {
            (*sub).end_time += 2; // if start_time == end_time, extend by 2ms (matching c code)
        }
    }

    1 // success!
}
