use lib_ccxr::share::ccxr_sub_entry_message::*;
use lib_ccxr::share::functions::sharing::*;
use lib_ccxr::util::log::{debug, DebugMessageFlag};
use std::ffi::CStr;
/// C-compatible function to clean up a `CcxSubEntryMessage`.
#[no_mangle]
pub extern "C" fn ccxr_sub_entry_msg_cleanup_c(msg: *mut CcxSubEntryMessage) {
    unsafe {
        if msg.is_null() {
            return;
        }
        let msg = &mut *msg;
        ccxr_sub_entry_msg_cleanup(msg);
    }
}

/// C-compatible function to print a `CcxSubEntryMessage`.
#[no_mangle]
pub unsafe extern "C" fn ccxr_sub_entry_msg_print_c(msg: *const CcxSubEntryMessage) {
    if msg.is_null() {
        debug!(msg_type = DebugMessageFlag::SHARE; "[share] print(!msg)\n");
        return;
    }

    let msg = &*msg;

    // Call the main Rust function
    ccxr_sub_entry_msg_print(msg);
}

#[no_mangle]
pub unsafe extern "C" fn ccxr_sub_entries_cleanup_c(entries: *mut CcxSubEntries) {
    if entries.is_null() {
        return;
    }
    let entries = &mut *entries;
    ccxr_sub_entries_cleanup(entries);
}

#[no_mangle]
pub unsafe extern "C" fn ccxr_sub_entries_print_c(entries: *const CcxSubEntries) {
    if entries.is_null() {
        debug!(msg_type = DebugMessageFlag::SHARE; "[share] ccxr_sub_entries_print (null entries)\n");
        return;
    }
    let entries = &*entries;
    ccxr_sub_entries_print(entries);
}

/// C-compatible function to start the sharing service.
#[no_mangle]
pub unsafe extern "C" fn ccxr_share_start_c(
    stream_name: *const std::os::raw::c_char,
) -> CcxShareStatus {
    if stream_name.is_null() {
        return ccxr_share_start(Option::from("unknown"));
    }

    let c_str = CStr::from_ptr(stream_name);
    let stream_name = match c_str.to_str() {
        Ok(name) => name,
        Err(_) => return CcxShareStatus::Fail,
    };

    ccxr_share_start(Option::from(stream_name))
}
#[no_mangle]
pub unsafe extern "C" fn ccxr_share_stop_c() -> CcxShareStatus {
    ccxr_share_stop()
}

#[no_mangle]
pub unsafe extern "C" fn _ccxr_share_send_c(msg: *const CcxSubEntryMessage) -> CcxShareStatus {
    if msg.is_null() {
        return CcxShareStatus::Fail;
    }
    _ccxr_share_send(&*msg)
}

#[no_mangle]
pub unsafe extern "C" fn ccxr_share_send_c(sub: *const CcSubtitle) -> CcxShareStatus {
    if sub.is_null() {
        return CcxShareStatus::Fail;
    }
    ccxr_share_send(sub as *mut CcSubtitle)
}

#[no_mangle]
pub unsafe extern "C" fn ccxr_share_stream_done_c(
    stream_name: *const std::os::raw::c_char,
) -> CcxShareStatus {
    if stream_name.is_null() {
        return CcxShareStatus::Fail;
    }

    let c_str = CStr::from_ptr(stream_name);
    match c_str.to_str() {
        Ok(name) => ccxr_share_stream_done(name),
        Err(_) => CcxShareStatus::Fail,
    }
}

/// C-compatible function to convert subtitles to sub-entry messages.
#[no_mangle]
pub unsafe extern "C" fn _ccxr_share_sub_to_entries_c(
    sub: *const CcSubtitle,
    entries: *mut CcxSubEntries,
) -> CcxShareStatus {
    if sub.is_null() || entries.is_null() {
        return CcxShareStatus::Fail;
    }

    // Dereference the pointers safely
    let sub_ref = &*sub;
    let entries_ref = &mut *entries;

    ccxr_share_sub_to_entries(sub_ref, entries_ref)
}
