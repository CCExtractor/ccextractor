use lib_ccxr::demuxer::demux::{
    ccx_demuxer_get_file_size, ccx_demuxer_get_stream_mode, ccx_demuxer_print_cfg, dinit_cap,
    freep, CcxDemuxer,
};
use std::ffi::CStr;
use std::os::raw::{c_char, c_int, c_longlong};
/// # Safety
/// This function is unsafe because it dereferences a raw pointer.
#[no_mangle]
pub unsafe extern "C" fn ccxr_demuxer_reset(ctx: *mut CcxDemuxer) {
    // Check for a null pointer to avoid undefined behavior.
    if ctx.is_null() {
        // Depending on your error handling strategy,
        // you might want to log an error or panic here.
        return;
    }
    // Convert the raw pointer into a mutable reference and call the reset method.
    (*ctx).reset();
}

// Extern function for ccx_demuxer_close
/// # Safety
/// This function is unsafe because it dereferences a raw pointer.
#[no_mangle]
pub unsafe extern "C" fn ccxr_demuxer_close(ctx: *mut CcxDemuxer) {
    if ctx.is_null() {
        return;
    }
    // Call the Rust implementation of close()
    (*ctx).close();
}

// Extern function for ccx_demuxer_isopen
/// # Safety
/// This function is unsafe because it dereferences a raw pointer.
#[no_mangle]
pub unsafe extern "C" fn ccxr_demuxer_isopen(ctx: *const CcxDemuxer) -> c_int {
    if ctx.is_null() {
        return 0;
    }
    // Call the Rust implementation is_open()
    if (*ctx).is_open() {
        1
    } else {
        0
    }
}

// Extern function for ccx_demuxer_open
/// # Safety
/// This function is unsafe because it dereferences a raw pointer and calls unsafe function `open`
#[no_mangle]
pub unsafe extern "C" fn ccxr_demuxer_open(ctx: *mut CcxDemuxer, file: *const c_char) -> c_int {
    if ctx.is_null() || file.is_null() {
        return -1;
    }
    // Convert the C string to a Rust string slice.
    let c_str = CStr::from_ptr(file);
    let file_str = match c_str.to_str() {
        Ok(s) => s,
        Err(_) => return -1,
    };

    // Call the Rust open() implementation.
    // Note: This call is unsafe because the Rust method itself is marked unsafe.
    (*ctx).open(file_str)
}

// Extern function for ccx_demuxer_get_file_size
/// # Safety
/// This function is unsafe because it dereferences a raw pointer.
#[no_mangle]
pub unsafe extern "C" fn ccxr_demuxer_get_file_size(ctx: *mut CcxDemuxer) -> c_longlong {
    if ctx.is_null() {
        return -1;
    }
    // Call the Rust function that returns the file size.
    ccx_demuxer_get_file_size(&mut *ctx)
}

// Extern function for ccx_demuxer_get_stream_mode
/// # Safety
/// This function is unsafe because it dereferences a raw pointer.
#[no_mangle]
pub unsafe extern "C" fn ccxr_demuxer_get_stream_mode(ctx: *const CcxDemuxer) -> c_int {
    if ctx.is_null() {
        return -1;
    }
    // Call the Rust function that returns the stream mode.
    ccx_demuxer_get_stream_mode(&*ctx)
}

// Extern function for ccx_demuxer_print_cfg
/// # Safety
/// This function is unsafe because it dereferences a raw pointer.
#[no_mangle]
pub unsafe extern "C" fn ccxr_demuxer_print_cfg(ctx: *const CcxDemuxer) {
    if ctx.is_null() {
        return;
    }
    // Call the Rust function to print the configuration.
    ccx_demuxer_print_cfg(&*ctx)
}

/// # Safety
/// This function is unsafe because it dereferences a raw pointer and calls unsafe function `dinit_cap`
#[no_mangle]
pub unsafe extern "C" fn ccxr_demuxer_delete(ctx: *mut *mut CcxDemuxer) {
    // Check if the pointer itself or the pointed-to pointer is null.
    if ctx.is_null() || (*ctx).is_null() {
        return;
    }

    // Convert the double pointer to a mutable reference.
    let lctx = &mut **ctx;

    // Call cleanup functions to free internal resources.
    dinit_cap(lctx);
    freep(&mut lctx.last_pat_payload);

    // Iterate through pid_buffers and free each buffer and the container.
    for pid_buffer in lctx.pid_buffers.iter_mut() {
        if !pid_buffer.is_null() {
            // Free the inner buffer if present.
            freep(&mut (**pid_buffer).buffer);
            // Free the PID_buffer itself.
            freep(pid_buffer);
        }
    }

    // Iterate through pids_programs and free each entry.
    for pid_prog in lctx.pids_programs.iter_mut() {
        freep(pid_prog);
    }

    // Free the filebuffer.
    freep(&mut lctx.filebuffer);

    // Deallocate the demuxer structure.
    let _ = Box::from_raw(*ctx);

    // Set the pointer to null to avoid dangling pointer.
    *ctx = std::ptr::null_mut();
}
