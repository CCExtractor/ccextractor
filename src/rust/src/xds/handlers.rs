#[allow(clippy::all)]
pub mod bindings {
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}

use std::ffi::CString;
use std::os::raw::{c_int, c_ulong, c_void};
use std::ptr::null_mut;
use std::sync::atomic::{AtomicI64, Ordering};
use std::sync::Mutex;

use crate::bindings::{cc_subtitle, ccx_decoders_xds_context, eia608_screen, realloc};

use lib_ccxr::debug;
use lib_ccxr::info;
use lib_ccxr::time::c_functions::get_fts;
use lib_ccxr::time::CaptionField;
use lib_ccxr::util::log::{hex_dump, send_gui, DebugMessageFlag, GuiXdsMessage};

use crate::xds::types::*;

static TS_START_OF_XDS: AtomicI64 = AtomicI64::new(-1); // Time at which we switched to XDS mode, =-1 hasn't happened yet
                                                        // Usage example:
                                                        // TS_START_OF_XDS.store(new_value, Ordering::SeqCst);
                                                        // let value = TS_START_OF_XDS.load(Ordering::SeqCst);

/// Represents failures during XDS string writing or processing.
pub enum XDSError {
    Err,
}

/// Writes an XDS string to the subtitle output buffer.
///
/// # Safety
/// - `sub.data` must be a valid pointer previously allocated by C's malloc/realloc, or null.
/// - The caller must ensure `sub` and `ctx` remain valid for the duration of the call.
/// - The returned `xds_str` pointer in the screen data must be freed with `CString::from_raw`.
pub unsafe fn write_xds_string(
    sub: &mut cc_subtitle,
    ctx: &mut CcxDecodersXdsContext,
    p: String,
    len: usize,
    ts_start_of_xds: i64,
) -> Result<(), XDSError> {
    let new_size = (sub.nb_data + 1) as usize * size_of::<eia608_screen>();
    let new_data =
        unsafe { realloc(sub.data as *mut c_void, new_size as c_ulong) as *mut eia608_screen };
    if new_data.is_null() {
        freep(&mut sub.data);
        sub.nb_data = 0;
        info!("No Memory left");
        return Err(XDSError::Err);
    }
    sub.data = new_data as *mut c_void;
    sub.datatype = 0;
    let data_element = &mut *new_data.add(sub.nb_data as usize);
    let c_str = CString::new(p).map_err(|_| XDSError::Err)?;
    let c_str_ptr = c_str.into_raw();
    data_element.format = 2;
    data_element.start_time = ts_start_of_xds;
    if let Some(timing) = ctx.timing.as_mut() {
        data_element.end_time = get_fts(timing, CaptionField::Cea708).millis();
    }
    data_element.xds_str = c_str_ptr;
    data_element.xds_len = len;
    data_element.cur_xds_packet_class = ctx.cur_xds_packet_class;
    sub.nb_data += 1;
    sub.type_ = 1;
    sub.got_output = 1;
    Ok(())
}

/// Prints an XDS message to the subtitle output if XDS file writing is enabled.
///
/// # Safety
/// - Same safety requirements as `write_xds_string`.
/// - Relies on the global `TS_START_OF_XDS` atomic value being correctly set.
pub unsafe fn xdsprint(
    sub: &mut cc_subtitle,
    ctx: &mut CcxDecodersXdsContext,
    message: String,
) -> Result<(), XDSError> {
    if !ctx.xds_write_to_file {
        return Ok(());
    }

    let len = message.len();
    write_xds_string(
        sub,
        ctx,
        message,
        len,
        TS_START_OF_XDS.load(Ordering::SeqCst),
    )
}

/// Frees a pointer and sets it to null.
///
/// Converts the raw pointer back into a `Box` to deallocate the memory,
/// then sets the original pointer to null to prevent use-after-free.
///
/// # Safety
/// - The pointer must have been originally allocated by Rust's `Box::into_raw`
///   or equivalent. Using this with C-allocated memory will cause undefined behavior.
/// - The pointer must not be used after this call.
pub fn freep<T>(ptr: &mut *mut T) {
    unsafe {
        if !ptr.is_null() {
            let _ = Box::from_raw(*ptr);
            *ptr = null_mut();
        }
    }
}
