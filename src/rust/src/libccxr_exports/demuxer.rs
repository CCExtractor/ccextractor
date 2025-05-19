use crate::bindings::{ccx_demuxer, lib_ccx_ctx, PMT_entry, PSI_buffer};
use crate::ccx_options;
use crate::common::{copy_to_rust, CType};
use crate::demuxer::demux::{
    ccx_demuxer_get_file_size, ccx_demuxer_get_stream_mode, ccx_demuxer_print_cfg, dinit_cap,
    freep, CcxDemuxer,
};
use lib_ccxr::common::Options;
use std::ffi::CStr;
use std::os::raw::{c_char, c_int, c_longlong, c_uchar, c_uint, c_void};
use std::slice;
use lib_ccxr::time::Timestamp;
use crate::ctorust::{from_ctype_Codec, from_ctype_PMT_entry, from_ctype_PSI_buffer, from_ctype_StreamMode, from_ctype_StreamType, from_ctype_cap_info, from_ctype_demux_report, from_ctype_program_info};

pub unsafe fn copy_demuxer_from_rust(c_demuxer: *mut ccx_demuxer, rust_demuxer: CcxDemuxer) {
    let c = &mut *c_demuxer;

    // Copy simple fields
    c.m2ts = rust_demuxer.m2ts;
    c.stream_mode = rust_demuxer.stream_mode.to_ctype();
    c.auto_stream = rust_demuxer.auto_stream.to_ctype();

    // Copy startbytes array
    let startbytes_len = rust_demuxer.startbytes.len().min(c.startbytes.len());
    c.startbytes[..startbytes_len].copy_from_slice(&rust_demuxer.startbytes[..startbytes_len]);
    c.startbytes_pos = rust_demuxer.startbytes_pos;
    c.startbytes_avail = rust_demuxer.startbytes_avail as c_int;

    // User-specified parameters
    c.ts_autoprogram = rust_demuxer.ts_autoprogram as c_int;
    c.ts_allprogram = rust_demuxer.ts_allprogram as c_int;
    c.flag_ts_forced_pn = rust_demuxer.flag_ts_forced_pn as c_int;
    c.flag_ts_forced_cappid = rust_demuxer.flag_ts_forced_cappid as c_int;
    c.ts_datastreamtype = rust_demuxer.ts_datastreamtype.to_ctype() as c_int;

    // Program info array
    let nb_program = rust_demuxer.nb_program.min(128);
    c.nb_program = nb_program as c_int;
    for (i, pinfo) in rust_demuxer.pinfo.iter().take(nb_program).enumerate() {
        c.pinfo[i] = pinfo.to_ctype();
    }

    // Codec settings
    c.codec = rust_demuxer.codec.to_ctype();
    c.nocodec = rust_demuxer.nocodec.to_ctype();

    // Cap info tree
    c.cinfo_tree = rust_demuxer.cinfo_tree.to_ctype();

    // File handles and positions
    c.infd = rust_demuxer.infd;
    c.past = rust_demuxer.past;

    // Global timestamps
    c.global_timestamp = rust_demuxer.global_timestamp.millis();
    c.min_global_timestamp = rust_demuxer.min_global_timestamp.millis();
    c.offset_global_timestamp = rust_demuxer.offset_global_timestamp.millis();
    c.last_global_timestamp = rust_demuxer.last_global_timestamp.millis();
    c.global_timestamp_inited = rust_demuxer.global_timestamp_inited.millis() as c_int;

    // PID buffers
    for (i, &pid_buffer) in rust_demuxer.pid_buffers.iter().take(8191).enumerate() {
        if !pid_buffer.is_null() {
            let rust_psi = &*pid_buffer;
            let c_psi = rust_psi.to_ctype();
            let c_ptr = Box::into_raw(Box::new(c_psi)) as *mut PSI_buffer;
            c.PID_buffers[i] = c_ptr;
        } else {
            c.PID_buffers[i] = std::ptr::null_mut();
        }
    }

    // PIDs seen array
    for (i, &val) in rust_demuxer.pids_seen.iter().take(65536).enumerate() {
        c.PIDs_seen[i] = val as c_int;
    }

    // Stream ID of each PID
    let stream_id_len = rust_demuxer.stream_id_of_each_pid.len().min(8192);
    c.stream_id_of_each_pid[..stream_id_len]
        .copy_from_slice(&rust_demuxer.stream_id_of_each_pid[..stream_id_len]);

    // Min PTS array
    let min_pts_len = rust_demuxer.min_pts.len().min(8192);
    c.min_pts[..min_pts_len].copy_from_slice(&rust_demuxer.min_pts[..min_pts_len]);

    // Have PIDs array
    for (i, &val) in rust_demuxer.have_pids.iter().take(8192).enumerate() {
        c.have_PIDs[i] = val as c_int;
    }

    c.num_of_PIDs = rust_demuxer.num_of_pids as c_int;

    // PIDs programs
    for (i, &pmt_entry) in rust_demuxer.pids_programs.iter().take(65536).enumerate() {
        if !pmt_entry.is_null() {
            let rust_pmt = &*pmt_entry;
            let c_pmt = rust_pmt.to_ctype();
            let c_ptr = Box::into_raw(Box::new(c_pmt)) as *mut PMT_entry;
            c.PIDs_programs[i] = c_ptr;
        } else {
            c.PIDs_programs[i] = std::ptr::null_mut();
        }
    }

    // Demux report
    c.freport = rust_demuxer.freport.to_ctype();

    // Hauppauge warning
    c.hauppauge_warning_shown = rust_demuxer.hauppauge_warning_shown as c_uint;

    c.multi_stream_per_prog = rust_demuxer.multi_stream_per_prog;

    // PAT payload
    c.last_pat_payload = rust_demuxer.last_pat_payload as *mut c_uchar;
    c.last_pat_length = rust_demuxer.last_pat_length;

    // File buffer
    c.filebuffer = rust_demuxer.filebuffer as *mut c_uchar;
    c.filebuffer_start = rust_demuxer.filebuffer_start;
    c.filebuffer_pos = rust_demuxer.filebuffer_pos;
    c.bytesinbuffer = rust_demuxer.bytesinbuffer;

    // Warnings and flags
    c.warning_program_not_found_shown = rust_demuxer.warning_program_not_found_shown as c_int;
    c.strangeheader = rust_demuxer.strangeheader;

    // Parent context
    c.parent = rust_demuxer
        .parent
        .map_or(std::ptr::null_mut(), |p| p as *mut _ as *mut c_void);

    // Private data
    c.private_data = rust_demuxer.private_data;

    // Function pointers (print_cfg, reset, etc.) are not copied from Rust
}
pub unsafe fn copy_demuxer_to_rust(ccx: *const ccx_demuxer) -> CcxDemuxer<'static> {
    let c = &*ccx;

    // Copy fixed-size fields
    let m2ts = c.m2ts;
    let stream_mode = from_ctype_StreamMode(c.stream_mode);
    let auto_stream = from_ctype_StreamMode(c.auto_stream);

    // Copy startbytes buffer up to available length
    let startbytes =
        slice::from_raw_parts(c.startbytes.as_ptr(), c.startbytes_avail as usize).to_vec();
    let startbytes_pos = c.startbytes_pos;
    let startbytes_avail = c.startbytes_avail;

    // User-specified params
    let ts_autoprogram = c.ts_autoprogram != 0;
    let ts_allprogram = c.ts_allprogram != 0;
    let flag_ts_forced_pn = c.flag_ts_forced_pn != 0;
    let flag_ts_forced_cappid = c.flag_ts_forced_cappid != 0;
    let ts_datastreamtype = from_ctype_StreamType(c.ts_datastreamtype as c_uint);

    // Program info list
    let nb_program = c.nb_program as usize;
    let pinfo = c.pinfo[..nb_program]
        .iter()
        .map(|pi| from_ctype_program_info(*pi))
        .collect::<Vec<_>>();

    // Codec settings
    let codec = from_ctype_Codec(c.codec);
    let nocodec = from_ctype_Codec(c.nocodec);
    let cinfo_tree = from_ctype_cap_info(c.cinfo_tree);

    // File handles and positions
    let infd = c.infd;
    let past = c.past;

    // Global timestamps
    let global_timestamp = Timestamp::from_millis(c.global_timestamp);
    let min_global_timestamp = Timestamp::from_millis(c.min_global_timestamp);
    let offset_global_timestamp = Timestamp::from_millis(c.offset_global_timestamp);
    let last_global_timestamp = Timestamp::from_millis(c.last_global_timestamp);
    let global_timestamp_inited = Timestamp::from_millis(c.global_timestamp_inited as i64);

    // PID buffers and related arrays
    let pid_buffers = c
        .PID_buffers
        .iter()
        .map(|buffer| Box::into_raw(Box::new(from_ctype_PSI_buffer(**buffer))))
        .collect::<Vec<_>>();
    let pids_programs = c.PIDs_programs[..]
        .iter()
        .map(|entry| Box::into_raw(Box::new(from_ctype_PMT_entry(**entry))))
        .collect::<Vec<_>>();
    let pids_seen = Vec::from(&c.PIDs_seen[..]);
    let stream_id_of_each_pid = Vec::from(&c.stream_id_of_each_pid[..]);
    let min_pts = Vec::from(&c.min_pts[..]);
    let have_pids = Vec::from(&c.have_PIDs[..]);
    let num_of_pids = c.num_of_PIDs;
    // Reports and warnings
    let freport = from_ctype_demux_report(c.freport);
    let hauppauge_warning_shown = c.hauppauge_warning_shown != 0;
    let multi_stream_per_prog = c.multi_stream_per_prog;

    // PAT tracking
    let last_pat_payload = c.last_pat_payload;
    let last_pat_length = c.last_pat_length;

    // File buffer
    let filebuffer = c.filebuffer;
    let filebuffer_start = c.filebuffer_start;
    let filebuffer_pos = c.filebuffer_pos;
    let bytesinbuffer = c.bytesinbuffer;

    let warning_program_not_found_shown = c.warning_program_not_found_shown != 0;
    let strangeheader = c.strangeheader;

    // Context and private data
    let mut parent = None;

    if c.parent != std::ptr::null_mut() {
        // Cast the `*mut c_void` to `*mut lib_ccx_ctx` and then dereference it.
        let parent_ref: &mut lib_ccx_ctx = &mut *(c.parent as *mut lib_ccx_ctx);
        parent = Some(parent_ref);
    }

    let private_data = c.private_data;

    CcxDemuxer {
        m2ts,
        stream_mode,
        auto_stream,
        startbytes,
        startbytes_pos,
        startbytes_avail,
        ts_autoprogram,
        ts_allprogram,
        flag_ts_forced_pn,
        flag_ts_forced_cappid,
        ts_datastreamtype,
        pinfo,
        nb_program,
        codec,
        nocodec,
        cinfo_tree,
        infd,
        past,
        global_timestamp,
        min_global_timestamp,
        offset_global_timestamp,
        last_global_timestamp,
        global_timestamp_inited,
        pid_buffers,
        pids_seen,
        stream_id_of_each_pid,
        min_pts,
        have_pids,
        num_of_pids,
        pids_programs,
        freport,
        hauppauge_warning_shown,
        multi_stream_per_prog,
        last_pat_payload,
        last_pat_length,
        filebuffer,
        filebuffer_start,
        filebuffer_pos,
        bytesinbuffer,
        warning_program_not_found_shown,
        strangeheader,
        parent,
        private_data,
        #[cfg(feature = "enable_ffmpeg")]
        ffmpeg_ctx: (), //todo after ffmpeg
    }
}


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
    let mut CcxOptions: Options = copy_to_rust(&raw const ccx_options);
    (*ctx).close(&mut CcxOptions);
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
