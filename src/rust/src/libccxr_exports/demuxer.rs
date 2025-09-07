use crate::bindings::{ccx_demuxer, lib_ccx_ctx};
use crate::ccx_options;
use crate::common::{copy_from_rust, copy_to_rust, CType};
use crate::ctorust::FromCType;
use crate::demuxer::common_types::{
    CapInfo, CcxDemuxReport, CcxDemuxer, PMTEntry, PSIBuffer, ProgramInfo,
};
use lib_ccxr::common::{Codec, Options, StreamMode, StreamType};
use lib_ccxr::time::Timestamp;
use std::alloc::{alloc_zeroed, Layout};
use std::ffi::CStr;
use std::os::raw::{c_char, c_int, c_longlong, c_uchar, c_uint, c_void};

pub fn copy_c_array_to_rust_vec(
    c_bytes: &[u8; crate::demuxer::common_types::ARRAY_SIZE],
) -> Vec<u8> {
    c_bytes.to_vec()
}
/// # Safety
/// This function is unsafe because it performs a copy operation from a raw pointer
#[no_mangle]
pub unsafe extern "C" fn copy_rust_vec_to_c(rust_vec: &Vec<u8>, c_ptr: *mut u8) {
    let mut size = crate::demuxer::common_types::ARRAY_SIZE;
    if rust_vec.is_empty() || rust_vec.len() < size {
        // This shouldn't happen, just for the tests
        size = rust_vec.len();
    }
    let rust_ptr = rust_vec.as_ptr();
    // Copies exactly ARRAY_SIZE bytes from rust_ptr → c_ptr
    std::ptr::copy(rust_ptr, c_ptr, size);
}
/// # Safety
///
/// This function is unsafe because we are modifying a global static mut variable
/// and we are dereferencing the pointer passed to it.
pub unsafe fn copy_demuxer_from_rust_to_c(c_demuxer: *mut ccx_demuxer, rust_demuxer: &CcxDemuxer) {
    let c = &mut *c_demuxer;
    // File handles and positions
    c.infd = rust_demuxer.infd;
    c.past = rust_demuxer.past;

    // Copy simple fields
    c.m2ts = rust_demuxer.m2ts;
    #[cfg(windows)]
    {
        c.stream_mode = rust_demuxer.stream_mode.to_ctype() as c_int;
        c.auto_stream = rust_demuxer.auto_stream.to_ctype() as c_int;
    }
    #[cfg(unix)]
    {
        c.stream_mode = rust_demuxer.stream_mode.to_ctype() as c_uint;
        c.auto_stream = rust_demuxer.auto_stream.to_ctype() as c_uint;
    }
    // Copy startbytes array
    copy_rust_vec_to_c(&rust_demuxer.startbytes, c.startbytes.as_mut_ptr());
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

    // Global timestamps
    c.global_timestamp = rust_demuxer.global_timestamp.millis();
    c.min_global_timestamp = rust_demuxer.min_global_timestamp.millis();
    c.offset_global_timestamp = rust_demuxer.offset_global_timestamp.millis();
    c.last_global_timestamp = rust_demuxer.last_global_timestamp.millis();
    c.global_timestamp_inited = rust_demuxer.global_timestamp_inited.millis() as c_int;

    // PID buffers - extra defensive version
    let pid_buffers_len = rust_demuxer.pid_buffers.len().min(8191);
    for i in 0..pid_buffers_len {
        let pid_buffer = rust_demuxer.pid_buffers[i];
        if !pid_buffer.is_null() {
            // Try to safely access the pointer
            match std::panic::catch_unwind(|| unsafe { &*pid_buffer }) {
                Ok(rust_psi) => {
                    let c_psi = unsafe { rust_psi.to_ctype() };
                    let c_ptr = Box::into_raw(Box::new(c_psi));
                    c.PID_buffers[i] = c_ptr;
                }
                Err(_) => {
                    // Pointer was invalid, set to null
                    eprintln!("Warning: Invalid PID buffer pointer at index {i}");
                    c.PID_buffers[i] = std::ptr::null_mut();
                }
            }
        } else {
            c.PID_buffers[i] = std::ptr::null_mut();
        }
    }

    // Clear remaining slots if rust array is smaller than C array
    for i in pid_buffers_len..8191 {
        c.PID_buffers[i] = std::ptr::null_mut();
    }

    // PIDs programs - extra defensive version
    let pids_programs_len = rust_demuxer.pids_programs.len().min(65536);
    for i in 0..pids_programs_len {
        let pmt_entry = rust_demuxer.pids_programs[i];
        if !pmt_entry.is_null() {
            // Try to safely access the pointer
            match std::panic::catch_unwind(|| unsafe { &*pmt_entry }) {
                Ok(rust_pmt) => {
                    let c_pmt = unsafe { rust_pmt.to_ctype() };
                    let c_ptr = Box::into_raw(Box::new(c_pmt));
                    c.PIDs_programs[i] = c_ptr;
                }
                Err(_) => {
                    // Pointer was invalid, set to null
                    eprintln!("Warning: Invalid PMT entry pointer at index {i}");
                    c.PIDs_programs[i] = std::ptr::null_mut();
                }
            }
        } else {
            c.PIDs_programs[i] = std::ptr::null_mut();
        }
    }

    // Clear remaining slots if rust array is smaller than C array
    for i in pids_programs_len..65536 {
        c.PIDs_programs[i] = std::ptr::null_mut();
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
    if rust_demuxer.parent.is_some() {
        unsafe {
            let parent_option_ptr = &rust_demuxer.parent as *const Option<&mut lib_ccx_ctx>;
            if let Some(parent_ref) = &*parent_option_ptr {
                c.parent = *parent_ref as *const lib_ccx_ctx as *mut c_void;
            }
        }
    }
    // Private data
    c.private_data = rust_demuxer.private_data;
}
/// # Safety
///
/// This function is unsafe because we are deferencing a raw pointer and calling unsafe functions to convert structs from C
pub unsafe fn copy_demuxer_from_c_to_rust(ccx: *const ccx_demuxer) -> CcxDemuxer<'static> {
    let c = &*ccx;

    // Copy fixed-size fields
    let m2ts = c.m2ts;
    let stream_mode =
        StreamMode::from_ctype(c.stream_mode).unwrap_or(StreamMode::ElementaryOrNotFound);
    let auto_stream =
        StreamMode::from_ctype(c.auto_stream).unwrap_or(StreamMode::ElementaryOrNotFound);

    // Copy startbytes buffer up to available length
    let startbytes = copy_c_array_to_rust_vec(&c.startbytes);
    let startbytes_pos = c.startbytes_pos;
    let startbytes_avail = c.startbytes_avail;

    // User-specified params
    let ts_autoprogram = c.ts_autoprogram != 0;
    let ts_allprogram = c.ts_allprogram != 0;
    let flag_ts_forced_pn = c.flag_ts_forced_pn != 0;
    let flag_ts_forced_cappid = c.flag_ts_forced_cappid != 0;
    let ts_datastreamtype =
        StreamType::from_ctype(c.ts_datastreamtype as c_uint).unwrap_or(StreamType::Unknownstream);

    // Program info list
    let nb_program = c.nb_program as usize;
    let pinfo = c.pinfo[..nb_program]
        .iter()
        .map(|pi| ProgramInfo::from_ctype(*pi).unwrap_or(ProgramInfo::default()))
        .collect::<Vec<_>>();

    // Codec settings
    let codec = Codec::from_ctype(c.codec).unwrap_or(Codec::Any);
    let nocodec = Codec::from_ctype(c.nocodec).unwrap_or(Codec::Any);
    let cinfo_tree = CapInfo::from_ctype(c.cinfo_tree).unwrap_or(CapInfo::default());

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
        .filter_map(|&buffer_ptr| {
            if buffer_ptr.is_null() {
                None
            } else {
                Some(Box::into_raw(Box::new(PSIBuffer::from_ctype(*buffer_ptr)?)))
            }
        })
        .collect::<Vec<_>>();
    let pids_programs = c
        .PIDs_programs
        .iter()
        .filter_map(|&buffer_ptr| {
            if buffer_ptr.is_null() || buffer_ptr as usize == 0xcdcdcdcdcdcdcdcd {
                None
            } else {
                Some(Box::into_raw(Box::new(PMTEntry::from_ctype(*buffer_ptr)?)))
            }
        })
        .collect::<Vec<_>>();

    let pids_seen = Vec::from(&c.PIDs_seen[..]);
    let stream_id_of_each_pid = Vec::from(&c.stream_id_of_each_pid[..]);
    let min_pts = Vec::from(&c.min_pts[..]);
    let have_pids = Vec::from(&c.have_PIDs[..]);
    let num_of_pids = c.num_of_PIDs;
    // Reports and warnings
    let freport = CcxDemuxReport::from_ctype(c.freport).unwrap_or(CcxDemuxReport::default());
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

    if !c.parent.is_null() {
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
///
/// This function is unsafe because we are calling a C struct and using alloc_zeroed to initialize it.
pub unsafe fn alloc_new_demuxer() -> *mut ccx_demuxer {
    let layout = Layout::new::<ccx_demuxer>();
    let ptr = alloc_zeroed(layout) as *mut ccx_demuxer;

    if ptr.is_null() {
        panic!("Failed to allocate memory for ccx_demuxer");
    }

    ptr
}
/// Rust equivalent of `ccx_demuxer_reset`
/// # Safety
/// This function is unsafe because it dereferences a raw pointer.
#[no_mangle]
pub unsafe extern "C" fn ccxr_demuxer_reset(ctx: *mut ccx_demuxer) {
    // Check for a null pointer to avoid undefined behavior.
    if ctx.is_null() {
        return;
    }
    let mut demux_ctx = copy_demuxer_from_c_to_rust(ctx);
    demux_ctx.reset();
    copy_demuxer_from_rust_to_c(ctx, &demux_ctx);
}

/// Rust equivalent of `ccx_demuxer_close`
/// # Safety
/// This function is unsafe because it dereferences a raw pointer.
#[no_mangle]
pub unsafe extern "C" fn ccxr_demuxer_close(ctx: *mut ccx_demuxer) {
    if ctx.is_null() {
        return;
    }
    let mut demux_ctx = copy_demuxer_from_c_to_rust(ctx);
    let mut CcxOptions: Options = copy_to_rust(&raw const ccx_options);
    demux_ctx.close(&mut CcxOptions);
    copy_from_rust(&raw mut ccx_options, CcxOptions);
    copy_demuxer_from_rust_to_c(ctx, &demux_ctx);
}

// Extern function for ccx_demuxer_isopen
/// # Safety
/// This function is unsafe because it dereferences a raw pointer.
#[no_mangle]
pub unsafe extern "C" fn ccxr_demuxer_isopen(ctx: *mut ccx_demuxer) -> c_int {
    if ctx.is_null() {
        return 0;
    }
    let demux_ctx = copy_demuxer_from_c_to_rust(ctx);
    if demux_ctx.is_open() {
        1
    } else {
        0
    }
}

// Extern function for ccx_demuxer_open
/// # Safety
/// This function is unsafe because it dereferences a raw pointer and calls unsafe function `open`
#[no_mangle]
pub unsafe extern "C" fn ccxr_demuxer_open(ctx: *mut ccx_demuxer, file: *const c_char) -> c_int {
    if ctx.is_null() {
        return -1;
    }
    let c_str = CStr::from_ptr(file);
    let file_str = match c_str.to_str() {
        Ok(s) => s,
        Err(_) => return -1,
    };

    let mut demux_ctx = copy_demuxer_from_c_to_rust(ctx);
    let mut CcxOptions: Options = copy_to_rust(&raw const ccx_options);

    let ReturnValue = demux_ctx.open(file_str, &mut CcxOptions);

    copy_from_rust(&raw mut ccx_options, CcxOptions);
    copy_demuxer_from_rust_to_c(ctx, &demux_ctx);
    ReturnValue
}

// Extern function for ccx_demuxer_get_file_size
/// # Safety
/// This function is unsafe because it dereferences a raw pointer.
#[no_mangle]
pub unsafe extern "C" fn ccxr_demuxer_get_file_size(ctx: *mut ccx_demuxer) -> c_longlong {
    if ctx.is_null() {
        return -1;
    }
    let mut demux_ctx = copy_demuxer_from_c_to_rust(ctx);
    demux_ctx.get_filesize() as c_longlong
}

// Extern function for ccx_demuxer_print_cfg
/// # Safety
/// This function is unsafe because it dereferences a raw pointer.
#[no_mangle]
pub unsafe extern "C" fn ccxr_demuxer_print_cfg(ctx: *mut ccx_demuxer) {
    if ctx.is_null() {
        return;
    }
    let mut demux_ctx = copy_demuxer_from_c_to_rust(ctx);
    demux_ctx.print_cfg()
}

#[cfg(test)]
mod tests {
    use super::*;
    use lib_ccxr::common::{Codec, StreamMode, StreamType};
    use std::ptr;
    // Working helper function to create ccx_demuxer on heap

    #[test]
    fn test_from_rust_to_c_from_rust() {
        let demuxer = unsafe { alloc_new_demuxer() };
        let mut vector = vec![0; 1024 * 1024];
        vector[0] = 0xAA;
        vector[1] = 0xBB;
        vector[2] = 0xCC;
        vector[3] = 0xDD;
        vector[4] = 0xEE;
        let mut parent = lib_ccx_ctx::default();
        struct MyContext {
            foo: i32,
            bar: String,
        }
        let ctx = Box::new(MyContext {
            foo: 42,
            bar: "hello".into(),
        });
        let raw_ctx: *mut MyContext = Box::into_raw(ctx);
        // Create a comprehensive Rust demuxer with test data for all fields
        let rust_demuxer = CcxDemuxer {
            m2ts: 99,
            stream_mode: StreamMode::Asf,
            auto_stream: StreamMode::Asf,
            startbytes: vector,
            startbytes_pos: 42,
            startbytes_avail: 100,
            ts_autoprogram: true,
            ts_allprogram: false,
            flag_ts_forced_pn: false,
            flag_ts_forced_cappid: true,
            ts_datastreamtype: StreamType::AudioAac,
            nb_program: 5,
            pinfo: Vec::new(), // We'll test this separately if needed
            codec: Codec::AtscCc,
            nocodec: Codec::Any,
            cinfo_tree: Default::default(),
            infd: 123,
            past: 987654321,
            global_timestamp: Timestamp::from_millis(1111),
            min_global_timestamp: Timestamp::from_millis(2222),
            offset_global_timestamp: Timestamp::from_millis(3333),
            last_global_timestamp: Timestamp::from_millis(4444),
            global_timestamp_inited: Timestamp::from_millis(5555),

            // Test arrays with some data
            pids_seen: vec![1, 0, 1, 0, 1],
            stream_id_of_each_pid: vec![11, 22, 33, 44, 55],
            min_pts: vec![111, 222, 333, 444, 555],
            have_pids: vec![1, 1, 0, 0, 1],
            num_of_pids: 7,

            // Empty pointer arrays - testing null handling
            pid_buffers: Vec::new(),
            pids_programs: Vec::new(),

            freport: Default::default(),
            hauppauge_warning_shown: false,
            multi_stream_per_prog: 88,

            // Test pointer fields
            last_pat_payload: ptr::null_mut(),
            last_pat_length: 777,
            filebuffer: ptr::null_mut(),
            filebuffer_start: 888999,
            filebuffer_pos: 111,
            bytesinbuffer: 222,
            warning_program_not_found_shown: false,
            strangeheader: 333,
            parent: Some(&mut parent),
            private_data: raw_ctx as *mut c_void,
            #[cfg(feature = "enable_ffmpeg")]
            ffmpeg_ctx: (),
        };

        unsafe {
            copy_demuxer_from_rust_to_c(demuxer, &rust_demuxer);
        }
        let c_demuxer = unsafe { &*demuxer };

        // Test ALL fields systematically:

        // Basic fields
        assert_eq!(c_demuxer.m2ts, 99);
        #[cfg(windows)]
        {
            assert_eq!(c_demuxer.stream_mode, StreamMode::Asf as c_int);
            assert_eq!(c_demuxer.auto_stream, StreamMode::Asf as c_int);
        }
        #[cfg(unix)]
        {
            assert_eq!(c_demuxer.stream_mode, StreamMode::Asf as c_uint);
            assert_eq!(c_demuxer.auto_stream, StreamMode::Asf as c_uint);
        }
        // startbytes array - test first few bytes
        assert_eq!(c_demuxer.startbytes[0], 0xAA);
        assert_eq!(c_demuxer.startbytes[1], 0xBB);
        assert_eq!(c_demuxer.startbytes[2], 0xCC);
        assert_eq!(c_demuxer.startbytes[3], 0xDD);
        assert_eq!(c_demuxer.startbytes[4], 0xEE);

        assert_eq!(c_demuxer.startbytes_pos, 42);
        assert_eq!(c_demuxer.startbytes_avail, 100);

        // Boolean to int conversions
        assert_eq!(c_demuxer.ts_autoprogram, 1); // true
        assert_eq!(c_demuxer.ts_allprogram, 0); // false
        assert_eq!(c_demuxer.flag_ts_forced_pn, 0); // false
        assert_eq!(c_demuxer.flag_ts_forced_cappid, 1); // true

        // Enum conversion
        assert_eq!(c_demuxer.ts_datastreamtype, StreamType::AudioAac as i32);

        // Program info
        assert_eq!(c_demuxer.nb_program, 5);

        // Codec fields
        #[cfg(unix)]
        {
            assert_eq!(c_demuxer.codec, Codec::AtscCc as c_uint);
            assert_eq!(c_demuxer.nocodec, Codec::Any as c_uint);
        }
        #[cfg(windows)]
        {
            assert_eq!(c_demuxer.codec, Codec::AtscCc as c_int);
            assert_eq!(c_demuxer.nocodec, Codec::Any as c_int);
        }
        // Add specific field checks here if CapInfo has testable fields

        // File handle fields
        assert_eq!(c_demuxer.infd, 123);
        assert_eq!(c_demuxer.past, 987654321);

        // Timestamp fields
        assert_eq!(c_demuxer.global_timestamp, 1111);
        assert_eq!(c_demuxer.min_global_timestamp, 2222);
        assert_eq!(c_demuxer.offset_global_timestamp, 3333);
        assert_eq!(c_demuxer.last_global_timestamp, 4444);
        assert_eq!(c_demuxer.global_timestamp_inited, 5555);

        // Array fields
        assert_eq!(c_demuxer.PIDs_seen[0], 1);
        assert_eq!(c_demuxer.PIDs_seen[1], 0);
        assert_eq!(c_demuxer.PIDs_seen[2], 1);
        assert_eq!(c_demuxer.PIDs_seen[3], 0);
        assert_eq!(c_demuxer.PIDs_seen[4], 1);

        assert_eq!(c_demuxer.stream_id_of_each_pid[0], 11);
        assert_eq!(c_demuxer.stream_id_of_each_pid[1], 22);
        assert_eq!(c_demuxer.stream_id_of_each_pid[2], 33);
        assert_eq!(c_demuxer.stream_id_of_each_pid[3], 44);
        assert_eq!(c_demuxer.stream_id_of_each_pid[4], 55);

        assert_eq!(c_demuxer.min_pts[0], 111);
        assert_eq!(c_demuxer.min_pts[1], 222);
        assert_eq!(c_demuxer.min_pts[2], 333);
        assert_eq!(c_demuxer.min_pts[3], 444);
        assert_eq!(c_demuxer.min_pts[4], 555);

        assert_eq!(c_demuxer.have_PIDs[0], 1);
        assert_eq!(c_demuxer.have_PIDs[1], 1);
        assert_eq!(c_demuxer.have_PIDs[2], 0);
        assert_eq!(c_demuxer.have_PIDs[3], 0);
        assert_eq!(c_demuxer.have_PIDs[4], 1);

        assert_eq!(c_demuxer.num_of_PIDs, 7);

        // Pointer arrays - should be null since we passed empty vecs
        for i in 0..10 {
            assert!(c_demuxer.PID_buffers[i].is_null());
            assert!(c_demuxer.PIDs_programs[i].is_null());
        }

        // Add specific field checks here if CcxDemuxReport has testable fields

        // More boolean conversions
        assert_eq!(c_demuxer.hauppauge_warning_shown, 0); // false
        assert_eq!(c_demuxer.warning_program_not_found_shown, 0); // false

        // Numeric fields
        assert_eq!(c_demuxer.multi_stream_per_prog, 88);
        assert_eq!(c_demuxer.strangeheader, 333);

        // Pointer fields
        assert_eq!(c_demuxer.last_pat_payload, ptr::null_mut());
        assert_eq!(c_demuxer.last_pat_length, 777);
        assert_eq!(c_demuxer.filebuffer, ptr::null_mut());
        assert_eq!(c_demuxer.filebuffer_start, 888999);
        assert_eq!(c_demuxer.filebuffer_pos, 111);
        assert_eq!(c_demuxer.bytesinbuffer, 222);
        assert!(!c_demuxer.parent.is_null());
        assert_eq!(c_demuxer.private_data, raw_ctx as *mut c_void);
        let ctx_ptr: *mut MyContext = c_demuxer.private_data as *mut MyContext;
        assert!(!ctx_ptr.is_null());
        unsafe {
            assert_eq!((*ctx_ptr).bar, "hello");
            assert_eq!((*ctx_ptr).foo, 42);
        }
    }

    #[test]
    fn test_from_rust_to_c_from_rust_arrays() {
        // Create a mock C demuxer with safe heap allocation
        let demuxer = unsafe { alloc_new_demuxer() };

        // Create a Rust demuxer with populated arrays
        let mut rust_demuxer = CcxDemuxer::default();

        // Set up some test data for arrays
        rust_demuxer.pids_seen = vec![1, 2, 3];
        rust_demuxer.stream_id_of_each_pid = vec![10, 20, 30];
        rust_demuxer.min_pts = vec![100, 200, 300];
        rust_demuxer.have_pids = vec![1, 0, 1];

        // Call the function being tested
        unsafe {
            copy_demuxer_from_rust_to_c(demuxer, &rust_demuxer);
        }
        let c_demuxer = unsafe { &*demuxer };

        // Verify arrays were copied correctly
        assert_eq!(c_demuxer.PIDs_seen[0], 1);
        assert_eq!(c_demuxer.PIDs_seen[1], 2);
        assert_eq!(c_demuxer.PIDs_seen[2], 3);

        assert_eq!(c_demuxer.stream_id_of_each_pid[0], 10);
        assert_eq!(c_demuxer.stream_id_of_each_pid[1], 20);
        assert_eq!(c_demuxer.stream_id_of_each_pid[2], 30);

        assert_eq!(c_demuxer.min_pts[0], 100);
        assert_eq!(c_demuxer.min_pts[1], 200);
        assert_eq!(c_demuxer.min_pts[2], 300);

        assert_eq!(c_demuxer.have_PIDs[0], 1);
        assert_eq!(c_demuxer.have_PIDs[1], 0);
        assert_eq!(c_demuxer.have_PIDs[2], 1);
    }

    #[test]
    fn test_copy_demuxer_from_c_to_rust() {
        // Allocate a new C demuxer structure
        let c_demuxer = unsafe { alloc_new_demuxer() };
        let c_demuxer_ptr = unsafe { &mut *c_demuxer };

        // Set up comprehensive test data in the C structure
        // Basic fields
        c_demuxer_ptr.m2ts = 42;
        #[cfg(unix)]
        {
            c_demuxer_ptr.stream_mode = StreamMode::Asf as c_uint;
            c_demuxer_ptr.auto_stream = StreamMode::Mp4 as c_uint;
        }
        #[cfg(windows)]
        {
            c_demuxer_ptr.stream_mode = StreamMode::Asf as c_int;
            c_demuxer_ptr.auto_stream = StreamMode::Mp4 as c_int;
        }
        // startbytes array - set some test data
        c_demuxer_ptr.startbytes[0] = 0xDE;
        c_demuxer_ptr.startbytes[1] = 0xAD;
        c_demuxer_ptr.startbytes[2] = 0xBE;
        c_demuxer_ptr.startbytes[3] = 0xEF;
        c_demuxer_ptr.startbytes[4] = 0xCA;
        c_demuxer_ptr.startbytes_pos = 123;
        c_demuxer_ptr.startbytes_avail = 456;

        // Boolean fields (stored as int in C)
        c_demuxer_ptr.ts_autoprogram = 1; // true
        c_demuxer_ptr.ts_allprogram = 0; // false
        c_demuxer_ptr.flag_ts_forced_pn = 1; // true
        c_demuxer_ptr.flag_ts_forced_cappid = 0; // false

        // Enum field
        c_demuxer_ptr.ts_datastreamtype = StreamType::AudioAac as i32;

        // Program info
        c_demuxer_ptr.nb_program = 3;

        // Codec fields
        #[cfg(unix)]
        {
            c_demuxer_ptr.codec = Codec::AtscCc as c_uint;
            c_demuxer_ptr.nocodec = Codec::Any as c_uint;
        }
        #[cfg(windows)]
        {
            c_demuxer_ptr.codec = Codec::AtscCc as c_int;
            c_demuxer_ptr.nocodec = Codec::Any as c_int;
        }
        // File handle fields
        c_demuxer_ptr.infd = 789;
        c_demuxer_ptr.past = 9876543210;

        // Timestamp fields
        c_demuxer_ptr.global_timestamp = 1111;
        c_demuxer_ptr.min_global_timestamp = 2222;
        c_demuxer_ptr.offset_global_timestamp = 3333;
        c_demuxer_ptr.last_global_timestamp = 4444;
        c_demuxer_ptr.global_timestamp_inited = 5555;

        // Array fields - set some test data
        c_demuxer_ptr.PIDs_seen[0] = 1;
        c_demuxer_ptr.PIDs_seen[1] = 0;
        c_demuxer_ptr.PIDs_seen[2] = 1;
        c_demuxer_ptr.PIDs_seen[3] = 0;
        c_demuxer_ptr.PIDs_seen[4] = 1;

        c_demuxer_ptr.stream_id_of_each_pid[0] = 11;
        c_demuxer_ptr.stream_id_of_each_pid[1] = 22;
        c_demuxer_ptr.stream_id_of_each_pid[2] = 33;
        c_demuxer_ptr.stream_id_of_each_pid[3] = 44;
        c_demuxer_ptr.stream_id_of_each_pid[4] = 55;

        c_demuxer_ptr.min_pts[0] = 111;
        c_demuxer_ptr.min_pts[1] = 222;
        c_demuxer_ptr.min_pts[2] = 333;
        c_demuxer_ptr.min_pts[3] = 444;
        c_demuxer_ptr.min_pts[4] = 555;

        c_demuxer_ptr.have_PIDs[0] = 1;
        c_demuxer_ptr.have_PIDs[1] = 1;
        c_demuxer_ptr.have_PIDs[2] = 0;
        c_demuxer_ptr.have_PIDs[3] = 0;
        c_demuxer_ptr.have_PIDs[4] = 1;

        c_demuxer_ptr.num_of_PIDs = 12;

        // Pointer arrays - set most to null for this test
        for i in 0..c_demuxer_ptr.PID_buffers.len() {
            c_demuxer_ptr.PID_buffers[i] = ptr::null_mut();
        }
        for i in 0..c_demuxer_ptr.PIDs_programs.len() {
            c_demuxer_ptr.PIDs_programs[i] = ptr::null_mut();
        }

        // Boolean fields stored as uint/int
        c_demuxer_ptr.hauppauge_warning_shown = 0; // false
        c_demuxer_ptr.warning_program_not_found_shown = 1; // true

        // Numeric fields
        c_demuxer_ptr.multi_stream_per_prog = 88;
        c_demuxer_ptr.strangeheader = 99;

        // Pointer fields
        c_demuxer_ptr.last_pat_payload = ptr::null_mut();
        c_demuxer_ptr.last_pat_length = 777;
        c_demuxer_ptr.filebuffer = ptr::null_mut();
        c_demuxer_ptr.filebuffer_start = 888999;
        c_demuxer_ptr.filebuffer_pos = 333;
        c_demuxer_ptr.bytesinbuffer = 444;
        c_demuxer_ptr.parent = ptr::null_mut();
        c_demuxer_ptr.private_data = ptr::null_mut();

        // Call the function under test
        let rust_demuxer = unsafe { copy_demuxer_from_c_to_rust(c_demuxer_ptr) };
        // Assert ALL fields are correctly copied:

        // Basic fields
        assert_eq!(rust_demuxer.m2ts, 42);
        assert_eq!(rust_demuxer.stream_mode, StreamMode::Asf);
        assert_eq!(rust_demuxer.auto_stream, StreamMode::Mp4);

        // startbytes - should be converted to Vec
        assert_eq!(rust_demuxer.startbytes[0], 0xDE);
        assert_eq!(rust_demuxer.startbytes[1], 0xAD);
        assert_eq!(rust_demuxer.startbytes[2], 0xBE);
        assert_eq!(rust_demuxer.startbytes[3], 0xEF);
        assert_eq!(rust_demuxer.startbytes[4], 0xCA);
        // The rest should be zeros from the C array
        assert_eq!(rust_demuxer.startbytes_pos, 123);
        assert_eq!(rust_demuxer.startbytes_avail, 456);

        // Boolean conversions (C int to Rust bool)
        assert!(rust_demuxer.ts_autoprogram);
        assert!(!rust_demuxer.ts_allprogram);
        assert!(rust_demuxer.flag_ts_forced_pn);
        assert!(!rust_demuxer.flag_ts_forced_cappid);

        // Enum conversion
        assert_eq!(rust_demuxer.ts_datastreamtype, StreamType::AudioAac);

        // Program info
        assert_eq!(rust_demuxer.nb_program, 3);
        assert_eq!(rust_demuxer.pinfo.len(), 3); // Should match nb_program

        // Codec fields
        assert_eq!(rust_demuxer.codec, Codec::AtscCc);
        assert_eq!(rust_demuxer.nocodec, Codec::Any);

        // File handle fields
        assert_eq!(rust_demuxer.infd, 789);
        assert_eq!(rust_demuxer.past, 9876543210);

        // Timestamp fields - should be converted from millis
        assert_eq!(rust_demuxer.global_timestamp, Timestamp::from_millis(1111));
        assert_eq!(
            rust_demuxer.min_global_timestamp,
            Timestamp::from_millis(2222)
        );
        assert_eq!(
            rust_demuxer.offset_global_timestamp,
            Timestamp::from_millis(3333)
        );
        assert_eq!(
            rust_demuxer.last_global_timestamp,
            Timestamp::from_millis(4444)
        );
        assert_eq!(
            rust_demuxer.global_timestamp_inited,
            Timestamp::from_millis(5555)
        );

        // Array fields - converted to Vec
        assert_eq!(rust_demuxer.pids_seen[0], 1);
        assert_eq!(rust_demuxer.pids_seen[1], 0);
        assert_eq!(rust_demuxer.pids_seen[2], 1);
        assert_eq!(rust_demuxer.pids_seen[3], 0);
        assert_eq!(rust_demuxer.pids_seen[4], 1);

        assert_eq!(rust_demuxer.stream_id_of_each_pid[0], 11);
        assert_eq!(rust_demuxer.stream_id_of_each_pid[1], 22);
        assert_eq!(rust_demuxer.stream_id_of_each_pid[2], 33);
        assert_eq!(rust_demuxer.stream_id_of_each_pid[3], 44);
        assert_eq!(rust_demuxer.stream_id_of_each_pid[4], 55);

        assert_eq!(rust_demuxer.min_pts[0], 111);
        assert_eq!(rust_demuxer.min_pts[1], 222);
        assert_eq!(rust_demuxer.min_pts[2], 333);
        assert_eq!(rust_demuxer.min_pts[3], 444);
        assert_eq!(rust_demuxer.min_pts[4], 555);

        assert_eq!(rust_demuxer.have_pids[0], 1);
        assert_eq!(rust_demuxer.have_pids[1], 1);
        assert_eq!(rust_demuxer.have_pids[2], 0);
        assert_eq!(rust_demuxer.have_pids[3], 0);
        assert_eq!(rust_demuxer.have_pids[4], 1);

        assert_eq!(rust_demuxer.num_of_pids, 12);

        // Pointer arrays - should be empty Vec since all were null
        assert!(rust_demuxer.pid_buffers.is_empty());
        assert!(rust_demuxer.pids_programs.is_empty());

        // Boolean conversions
        assert!(!rust_demuxer.hauppauge_warning_shown);
        assert!(rust_demuxer.warning_program_not_found_shown);

        // Numeric fields
        assert_eq!(rust_demuxer.multi_stream_per_prog, 88);
        assert_eq!(rust_demuxer.strangeheader, 99);

        // Pointer fields
        assert_eq!(rust_demuxer.last_pat_payload, ptr::null_mut());
        assert_eq!(rust_demuxer.last_pat_length, 777);
        assert_eq!(rust_demuxer.filebuffer, ptr::null_mut());
        assert_eq!(rust_demuxer.filebuffer_start, 888999);
        assert_eq!(rust_demuxer.filebuffer_pos, 333);
        assert_eq!(rust_demuxer.bytesinbuffer, 444);

        // Parent should be None since we set it to null_mut
        assert!(rust_demuxer.parent.is_none());
        assert_eq!(rust_demuxer.private_data, ptr::null_mut());
    }

    #[test]
    fn test_ccx_demuxer_other() {
        use super::*;
        use crate::demuxer::common_types::{CapInfo, CcxDemuxReport, ProgramInfo};
        use lib_ccxr::common::{Codec, StreamMode, StreamType};
        use std::ptr;

        // ── 1) Build a "full" Rust CcxDemuxer with nonempty pinfo, cinfo_tree, and freport ──

        let mut rust_demuxer = CcxDemuxer::default();

        // a) pinfo & nb_program
        rust_demuxer.pinfo = vec![
            ProgramInfo::default(),
            ProgramInfo::default(),
            ProgramInfo::default(),
        ];
        rust_demuxer.nb_program = rust_demuxer.pinfo.len();

        // b) cinfo_tree (use Default; assuming CapInfo: Default + PartialEq)
        let cap_defaults = CapInfo::default();
        rust_demuxer.cinfo_tree = cap_defaults.clone();

        // c) freport (use Default; assuming CcxDemuxReport: Default + PartialEq)
        let report_defaults = CcxDemuxReport::default();
        rust_demuxer.freport = report_defaults.clone();

        // We also need to satisfy the required scalar fields so that copy → C doesn’t panic.
        // Fill in at least the ones that have no defaults in `Default::default()`:
        rust_demuxer.m2ts = 7;
        rust_demuxer.stream_mode = StreamMode::Mp4;
        rust_demuxer.auto_stream = StreamMode::Asf;
        rust_demuxer.startbytes = vec![0u8; 16]; // small placeholder (C side will zero‐extend)
        rust_demuxer.startbytes_pos = 0;
        rust_demuxer.startbytes_avail = 0;
        rust_demuxer.ts_autoprogram = false;
        rust_demuxer.ts_allprogram = false;
        rust_demuxer.flag_ts_forced_pn = false;
        rust_demuxer.flag_ts_forced_cappid = false;
        rust_demuxer.ts_datastreamtype = StreamType::AudioAac;
        rust_demuxer.codec = Codec::AtscCc;
        rust_demuxer.nocodec = Codec::Any;
        rust_demuxer.infd = 0;
        rust_demuxer.past = 0;
        rust_demuxer.global_timestamp = Timestamp::from_millis(0);
        rust_demuxer.min_global_timestamp = Timestamp::from_millis(0);
        rust_demuxer.offset_global_timestamp = Timestamp::from_millis(0);
        rust_demuxer.last_global_timestamp = Timestamp::from_millis(0);
        rust_demuxer.global_timestamp_inited = Timestamp::from_millis(0);
        rust_demuxer.pid_buffers = Vec::new();
        rust_demuxer.pids_seen = Vec::new();
        rust_demuxer.stream_id_of_each_pid = Vec::new();
        rust_demuxer.min_pts = Vec::new();
        rust_demuxer.have_pids = Vec::new();
        rust_demuxer.num_of_pids = 0;
        rust_demuxer.pids_programs = Vec::new();
        rust_demuxer.hauppauge_warning_shown = false;
        rust_demuxer.multi_stream_per_prog = 0;
        rust_demuxer.last_pat_payload = ptr::null_mut();
        rust_demuxer.last_pat_length = 0;
        rust_demuxer.filebuffer = ptr::null_mut();
        rust_demuxer.filebuffer_start = 0;
        rust_demuxer.filebuffer_pos = 0;
        rust_demuxer.bytesinbuffer = 0;
        rust_demuxer.warning_program_not_found_shown = false;
        rust_demuxer.strangeheader = 0;
        rust_demuxer.parent = None;
        rust_demuxer.private_data = ptr::null_mut();

        // ── 2) Copy from Rust → C, then back to Rust, and assert on the "other" fields ──

        unsafe {
            // Allocate a fresh C-side ccx_demuxer on the heap
            let c_demuxer_ptr = alloc_new_demuxer();

            // Copy Rust struct into the C struct
            copy_demuxer_from_rust_to_c(c_demuxer_ptr, &rust_demuxer);

            // Immediately convert the C struct back into a fresh Rust CcxDemuxer
            let recovered: CcxDemuxer<'_> = copy_demuxer_from_c_to_rust(&*c_demuxer_ptr);

            // — nb_program/pinfo length roundtrip —
            assert_eq!(recovered.nb_program, rust_demuxer.nb_program);
            assert_eq!(recovered.pinfo.len(), rust_demuxer.pinfo.len());

            // Because we only used Default::default() inside each ProgramInfo,
            // the entries themselves should be identical to the originals:
            assert_eq!(recovered.pinfo[0].pcr_pid, rust_demuxer.pinfo[0].pcr_pid);

            // — cinfo_tree roundtrip —
            assert_eq!(recovered.cinfo_tree.codec, rust_demuxer.cinfo_tree.codec);

            // — freport roundtrip —
            assert_eq!(
                recovered.freport.mp4_cc_track_cnt,
                rust_demuxer.freport.mp4_cc_track_cnt
            );
        }

        // ── 3) Now test C→Rust side when C has its own “nonzero” nb_program and pinfo is zeroed array ──

        unsafe {
            // Allocate and zero a brand‐new C ccx_demuxer
            let c2_ptr = alloc_new_demuxer();
            let c2_ref = &mut *c2_ptr;

            // Manually set nb_program on the C side:
            c2_ref.nb_program = 2;

            // Now copy to Rust:
            let rust_from_zeroed: CcxDemuxer<'_> = copy_demuxer_from_c_to_rust(c2_ref);

            // The Rust‐side Vec<u ProgramInfo> should have length == 2,
            // and each ProgramInfo should be Default::default().
            assert_eq!(rust_from_zeroed.nb_program, 2);
            assert_eq!(rust_from_zeroed.pinfo.len(), 2);
            assert_eq!(rust_from_zeroed.pinfo[0].pcr_pid, 0);

            // cinfo_tree was zeroed in C; that should become default CapInfo in Rust
            assert_eq!(rust_from_zeroed.cinfo_tree.codec, Codec::Any);

            // freport was zeroed in C; that should become default CcxDemuxReport in Rust
            assert_eq!(
                rust_from_zeroed.freport.mp4_cc_track_cnt,
                CcxDemuxReport::default().mp4_cc_track_cnt
            );
        }
    }
}
