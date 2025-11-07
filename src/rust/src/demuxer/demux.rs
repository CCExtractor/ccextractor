use crate::demuxer::stream_functions::{detect_myth, detect_stream_type};
use lib_ccxr::activity::ActivityExt;
use lib_ccxr::common::StreamMode;
use lib_ccxr::common::{DataSource, Options};
use lib_ccxr::time::Timestamp;
use lib_ccxr::util::log::ExitCause;
use lib_ccxr::{error, fatal, info};
use std::ffi::CString;
use std::fs::File;
use std::io::{Seek, SeekFrom};

use crate::demuxer::common_types::*;
use crate::file_functions::file::init_file_buffer;
#[cfg(windows)]
use crate::{bindings::_open_osfhandle, file_functions::file::open_windows};
use cfg_if::cfg_if;
#[cfg(unix)]
use std::os::fd::{FromRawFd, IntoRawFd};
use std::os::raw::{c_char, c_uint};
#[cfg(windows)]
use std::os::windows::io::IntoRawHandle;
use std::path::Path;
use std::ptr::{null, null_mut};

cfg_if! {
    if #[cfg(test)] {
        use crate::demuxer::demux::tests::{print_file_report, start_tcp_srv, start_upd_srv};
    } else {
        use crate::{print_file_report, start_tcp_srv, start_upd_srv};
        #[cfg(feature = "enable_ffmpeg")]
        use crate::init_ffmpeg;
    }
}

impl CcxDemuxer<'_> {
    pub fn get_filesize(&mut self) -> i64 {
        // Get the file descriptor from ctx.
        let in_fd = self.infd;
        if in_fd < 0 {
            return -1;
        }

        // SAFETY: We are creating a File from an existing raw fd.
        // To prevent the File from closing the descriptor on drop,
        // we call into_raw_fd() after using it.
        #[cfg(unix)]
        let mut file = unsafe { File::from_raw_fd(in_fd) };
        #[cfg(windows)]
        let mut file = open_windows(in_fd);

        // Get current position: equivalent to LSEEK(in, 0, SEEK_CUR);
        let current = match file.stream_position() {
            Ok(pos) => pos,
            Err(_) => {
                // Return the fd back and then -1.
                self.drop_fd(file);
                return -1;
            }
        };

        // Get file length: equivalent to LSEEK(in, 0, SEEK_END);
        let length = match file.seek(SeekFrom::End(0)) {
            Ok(pos) => pos,
            Err(_) => {
                self.drop_fd(file);
                return -1;
            }
        };

        #[allow(unused_variables)]
        let ret = match file.seek(SeekFrom::Start(current)) {
            Ok(pos) => pos,
            Err(_) => {
                self.drop_fd(file);
                return -1;
            }
        };

        // Return the fd back to its original owner.
        self.drop_fd(file);
        length as i64
    }

    pub fn reset(&mut self) {
        {
            self.startbytes_pos = 0;
            self.startbytes_avail = 0;
            self.num_of_pids = 0;
            // Fill have_pids with -1 for (MAX_PSI_PID+1) elements.
            let len_have = MAX_PSI_PID + 1;
            if self.have_pids.len() < len_have {
                self.have_pids.resize(len_have, -1);
            } else {
                self.have_pids[..len_have].fill(-1);
            }
            // Fill pids_seen with 0 for MAX_PID elements.
            if self.pids_seen.len() < MAX_PID {
                self.pids_seen.resize(MAX_PID, 0);
            } else {
                self.pids_seen[..MAX_PID].fill(0);
            }
            if self.min_pts.len() != MAX_PSI_PID + 1 {
                self.min_pts.resize(MAX_PSI_PID + 1, u64::MAX);
            } else {
                // Set each min_pts[i] to u64::MAX for i in 0..=MAX_PSI_PID.
                for i in 0..=MAX_PSI_PID {
                    self.min_pts[i] = u64::MAX;
                }
            }
            // Fill stream_id_of_each_pid with 0 for (MAX_PSI_PID+1) elements.
            if self.stream_id_of_each_pid.len() < len_have {
                self.stream_id_of_each_pid.resize(len_have, 0);
            } else {
                self.stream_id_of_each_pid[..len_have].fill(0);
            }
            // Fill pids_programs with null for MAX_PID elements.
            if self.pids_programs.len() < MAX_PID {
                self.pids_programs.resize(MAX_PID, null_mut());
            } else {
                self.pids_programs[..MAX_PID].fill(null_mut());
            }
        }
    }

    pub fn close(&mut self, ccx_options: &mut Options) {
        self.past = 0;
        if self.infd != -1 && ccx_options.input_source == DataSource::File {
            // Convert raw fd to Rust File to handle closing
            let file;
            #[cfg(unix)]
            {
                file = unsafe { File::from_raw_fd(self.infd) };
            }
            #[cfg(windows)]
            {
                file = open_windows(self.infd);
            }
            drop(file); // This closes the file descriptor
            self.infd = -1;
            ccx_options.activity_input_file_closed();
        }
    }

    pub fn is_open(&self) -> bool {
        self.infd != -1
    }

    /// # Safety
    /// detect_stream_type is an unsafe function
    pub unsafe fn open(&mut self, file_name: &str, ccx_options: &mut Options) -> i32 {
        // Initialize timestamp fields
        self.past = 0;
        self.min_global_timestamp = Timestamp::from_millis(0);
        self.last_global_timestamp = Timestamp::from_millis(0);
        self.global_timestamp_inited = Timestamp::from_millis(0);
        self.offset_global_timestamp = Timestamp::from_millis(0);

        #[cfg(feature = "enable_ffmpeg")]
        {
            self.ffmpeg_ctx = init_ffmpeg(file_name);
            if !self.ffmpeg_ctx.is_null() {
                self.stream_mode = StreamMode::Ffmpeg;
                self.auto_stream = StreamMode::Ffmpeg;
                return 0;
            } else {
                info!("Failed to initialize ffmpeg, falling back to legacy");
            }
        }

        init_file_buffer(self);

        let mut handle_existing_infd = || {
            if self.infd != -1 {
                if ccx_options.print_file_reports {
                    print_file_report(*self.parent.as_mut().unwrap());
                }
                return Some(-1);
            }
            None
        };

        match ccx_options.input_source {
            DataSource::Stdin => {
                if let Some(result) = handle_existing_infd() {
                    return result;
                }
                self.infd = 0;
                info!("\n\r-----------------------------------------------------------------\n");
                info!("\rReading from standard input\n");
            }
            DataSource::Network => {
                if let Some(result) = handle_existing_infd() {
                    return result;
                }
                self.infd = start_upd_srv(
                    ccx_options
                        .udpsrc
                        .as_deref()
                        .map_or(null(), |s| s.as_ptr() as *const c_char),
                    ccx_options
                        .udpaddr
                        .as_deref()
                        .map_or(null(), |s| s.as_ptr() as *const c_char),
                    ccx_options.udpport as c_uint,
                );
                if self.infd < 0 {
                    error!("socket() failed.");
                    return ExitCause::Bug as i32;
                }
            }
            DataSource::Tcp => {
                if let Some(result) = handle_existing_infd() {
                    return result;
                }
                let port_cstring = ccx_options
                    .tcpport
                    .map(|port| CString::new(port.to_string()).unwrap());

                self.infd = start_tcp_srv(
                    port_cstring.as_deref().map_or(null(), |cs| cs.as_ptr()),
                    ccx_options
                        .tcp_password
                        .as_deref()
                        .map_or(null(), |s| s.as_ptr() as *const c_char),
                );
            }
            _ => {
                let file_result = File::open(Path::new(file_name));
                match file_result {
                    Ok(file) => {
                        #[cfg(unix)]
                        {
                            self.infd = file.into_raw_fd();
                        }
                        #[cfg(windows)]
                        {
                            let handle = file.into_raw_handle();
                            let fd = unsafe { _open_osfhandle(handle as isize, 0) };
                            if fd == -1 {
                                return -1;
                            }
                            self.infd = fd;
                        }
                    }
                    Err(_) => return -1,
                }
            }
        }
        // Stream mode detection
        if self.auto_stream == StreamMode::Autodetect {
            detect_stream_type(self, ccx_options);
            match self.stream_mode {
                StreamMode::ElementaryOrNotFound => {
                    info!("\rFile seems to be an elementary stream")
                }
                StreamMode::Transport => info!("\rFile seems to be a transport stream"),
                StreamMode::Program => info!("\rFile seems to be a program stream"),
                StreamMode::Asf => info!("\rFile seems to be an ASF"),
                StreamMode::Wtv => info!("\rFile seems to be a WTV"),
                StreamMode::McpoodlesRaw => info!("\rFile seems to be McPoodle raw data"),
                StreamMode::Rcwt => info!("\rFile seems to be a raw caption with time data"),
                StreamMode::Mp4 => info!("\rFile seems to be a MP4"),
                StreamMode::Gxf => info!("\rFile seems to be a GXF"),
                StreamMode::Mkv => info!("\rFile seems to be a Matroska/WebM container"),
                #[cfg(feature = "wtv_debug")]
                StreamMode::HexDump => info!("\rFile seems to be an hexadecimal dump"),
                StreamMode::Mxf => info!("\rFile seems to be an MXF"),
                StreamMode::Myth | StreamMode::Autodetect => {
                    fatal!(cause = ExitCause::Bug; "Impossible stream_mode value");
                }
                _ => {}
            }
        } else {
            self.stream_mode = self.auto_stream;
        }
        // MythTV detection logic
        match ccx_options.auto_myth {
            Some(true) => {
                // Force stream mode to myth
                self.stream_mode = StreamMode::Myth;
            }
            Some(false) => {
                if matches!(
                    self.stream_mode,
                    StreamMode::ElementaryOrNotFound | StreamMode::Program
                ) && detect_myth(self) != 0
                {
                    self.stream_mode = StreamMode::Myth;
                }
            }
            _ => {}
        }
        0
    }
    pub fn print_cfg(&mut self) {
        match self.auto_stream {
            StreamMode::ElementaryOrNotFound => {
                info!("Elementary");
            }
            StreamMode::Transport => {
                info!("Transport");
            }
            StreamMode::Program => {
                info!("Program");
            }
            StreamMode::Asf => {
                info!("DVR-MS");
            }
            StreamMode::Wtv => {
                info!("Windows Television (WTV)");
            }
            StreamMode::McpoodlesRaw => {
                info!("McPoodle's raw");
            }
            StreamMode::Autodetect => {
                info!("Autodetect");
            }
            StreamMode::Rcwt => {
                info!("BIN");
            }
            StreamMode::Mp4 => {
                info!("MP4");
            }
            StreamMode::Mkv => {
                info!("MKV");
            }
            StreamMode::Mxf => {
                info!("MXF");
            }
            #[cfg(feature = "wtv_debug")]
            StreamMode::HexDump => {
                info!("Hex");
            }
            _ => {
                fatal!(
                    cause = ExitCause::Bug;
                    "BUG: Unknown stream mode. Please file a bug report on Github.\n"
                );
            }
        }
    }
    pub fn drop_fd(&mut self, file: File) {
        #[cfg(unix)]
        let _ = file.into_raw_fd();
        #[cfg(windows)]
        let _ = file.into_raw_handle();
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::bindings::{lib_ccx_ctx, list_head};
    use crate::hlist::list_empty;
    use lib_ccxr::common::{Codec, StreamType};
    use lib_ccxr::util::log::{
        set_logger, CCExtractorLogger, DebugMessageFlag, DebugMessageMask, OutputTarget,
    };
    use serial_test::serial;
    use std::fs::OpenOptions;
    use std::io::Write;
    #[cfg(unix)]
    use std::os::fd::AsRawFd;
    use std::os::raw::{c_char, c_int, c_uint};
    #[cfg(windows)]
    use std::os::windows::io::AsRawHandle;
    #[cfg(windows)]
    use std::os::windows::io::RawHandle;
    use std::slice;
    use std::sync::Once;
    use tempfile::NamedTempFile;

    #[no_mangle]
    #[allow(unused_variables)]
    pub unsafe extern "C" fn print_file_report(ctx: *mut lib_ccx_ctx) {}
    pub static mut FILEBUFFERSIZE: i32 = 0;

    static INIT: Once = Once::new();
    pub fn start_tcp_srv(_port: *const c_char, _pwd: *const c_char) -> c_int {
        0
    }
    pub fn start_upd_srv(_src: *const c_char, _addr: *const c_char, _port: c_uint) -> c_int {
        0
    }

    fn initialize_logger() {
        INIT.call_once(|| {
            set_logger(CCExtractorLogger::new(
                OutputTarget::Stdout,
                DebugMessageMask::new(DebugMessageFlag::VERBOSE, DebugMessageFlag::VERBOSE),
                false,
            ))
            .ok();
        });
    }

    #[test]
    fn test_default_ccx_demuxer() {
        let demuxer = CcxDemuxer::default();
        assert_eq!(demuxer.m2ts, 0);
        assert_eq!(demuxer.stream_mode, StreamMode::default());
        assert_eq!(demuxer.auto_stream, StreamMode::default());
        assert_eq!(demuxer.startbytes_pos, 0);
        assert_eq!(demuxer.startbytes_avail, 0);
        assert!(!demuxer.ts_autoprogram);
        assert!(!demuxer.ts_allprogram);
        assert!(!demuxer.flag_ts_forced_pn);
        assert!(!demuxer.flag_ts_forced_cappid);
        assert_eq!(demuxer.ts_datastreamtype, StreamType::Unknownstream);
        assert_eq!(demuxer.nb_program, 0);
        assert_eq!(demuxer.codec, Codec::Any);
        assert_eq!(demuxer.nocodec, Codec::Any);
        assert_eq!(demuxer.infd, -1);
        assert_eq!(demuxer.past, 0);
        assert_eq!(demuxer.global_timestamp, Timestamp::from_millis(0));
        assert_eq!(demuxer.min_global_timestamp, Timestamp::from_millis(0));
        assert_eq!(demuxer.offset_global_timestamp, Timestamp::from_millis(0));
        assert_eq!(demuxer.last_global_timestamp, Timestamp::from_millis(0));
        assert_eq!(demuxer.global_timestamp_inited, Timestamp::from_millis(0));
        assert_eq!(demuxer.pid_buffers.len(), MAX_PSI_PID);
        assert_eq!(demuxer.pids_seen.len(), MAX_PID);
        assert_eq!(demuxer.stream_id_of_each_pid.len(), MAX_PSI_PID + 1);
        assert_eq!(demuxer.min_pts.len(), MAX_PSI_PID + 1);
        assert_eq!(demuxer.have_pids.len(), MAX_PSI_PID + 1);
        assert_eq!(demuxer.num_of_pids, 0);
        assert_eq!(demuxer.pids_programs.len(), MAX_PID);
        assert!(!demuxer.hauppauge_warning_shown);
        assert_eq!(demuxer.multi_stream_per_prog, 0);
        assert_eq!(demuxer.last_pat_payload, null_mut());
        assert_eq!(demuxer.last_pat_length, 0);
        assert_eq!(demuxer.filebuffer, null_mut());
        assert_eq!(demuxer.filebuffer_start, 0);
        assert_eq!(demuxer.filebuffer_pos, 0);
        assert_eq!(demuxer.bytesinbuffer, 0);
        assert!(!demuxer.warning_program_not_found_shown);
        assert_eq!(demuxer.strangeheader, 0);
        #[cfg(feature = "enable_ffmpeg")]
        assert_eq!(demuxer.ffmpeg_ctx, null_mut());
        assert_eq!(demuxer.private_data, null_mut());
    }

    #[allow(unused)]

    fn new_cap_info(codec: Codec) -> Box<CapInfo> {
        Box::new(CapInfo {
            codec,
            sib_head: {
                let mut hl = list_head::default();
                let ptr = &mut hl as *mut list_head;
                hl.next = ptr;
                hl.prev = ptr;
                hl
            },
            sib_stream: {
                let mut hl = list_head::default();
                let ptr = &mut hl as *mut list_head;
                hl.next = ptr;
                hl.prev = ptr;
                hl
            },
            ..Default::default()
        })
    }

    //Tests for list_empty
    #[test]
    fn test_list_empty_not_empty() {
        let mut head = list_head::default();
        let mut node = list_head::default();
        head.next = &mut node;
        head.prev = &mut node;
        let result = list_empty(&mut head);
        assert!(!result);
    }

    fn dummy_demuxer<'a>() -> CcxDemuxer<'a> {
        CcxDemuxer {
            filebuffer: null_mut(),
            filebuffer_start: 999,
            filebuffer_pos: 999,
            bytesinbuffer: 999,
            have_pids: vec![],
            pids_seen: vec![],
            min_pts: vec![0; MAX_PSI_PID + 1],
            stream_id_of_each_pid: vec![],
            pids_programs: vec![],
            ..Default::default()
        }
    }

    // --------- Tests for init_file_buffer ---------

    #[test]
    fn test_init_file_buffer_allocates_if_null() {
        let mut ctx = dummy_demuxer();
        ctx.filebuffer = null_mut();
        let res = init_file_buffer(&mut ctx);
        assert_eq!(res, 0);
        assert!(!ctx.filebuffer.is_null());
        assert_eq!(ctx.filebuffer_start, 0);
        assert_eq!(ctx.filebuffer_pos, 0);
        assert_eq!(ctx.bytesinbuffer, 0);
    }

    #[test]
    fn test_init_file_buffer_does_not_reallocate_if_nonnull() {
        let mut ctx = dummy_demuxer();
        let buf = vec![1u8; unsafe { FILEBUFFERSIZE } as usize].into_boxed_slice();
        ctx.filebuffer = Box::into_raw(buf) as *mut u8;
        ctx.bytesinbuffer = 123;
        let res = init_file_buffer(&mut ctx);
        assert_eq!(res, 0);
        assert!(!ctx.filebuffer.is_null());
        // bytesinbuffer remains unchanged.
        assert_eq!(ctx.bytesinbuffer, 123);
        // Clean up.
        unsafe {
            let _ = Box::from_raw(slice::from_raw_parts_mut(
                ctx.filebuffer,
                FILEBUFFERSIZE as usize,
            ));
        }
    }

    // --------- Tests for ccx_demuxer_reset ---------

    #[test]
    fn test_reset_sets_fields_correctly() {
        let mut ctx = dummy_demuxer();
        ctx.startbytes_pos = 42;
        ctx.startbytes_avail = 99;
        ctx.num_of_pids = 123;
        ctx.have_pids = vec![0; MAX_PSI_PID + 1];
        ctx.pids_seen = vec![1; MAX_PID];
        ctx.min_pts = vec![0; MAX_PSI_PID + 1];
        ctx.stream_id_of_each_pid = vec![5; MAX_PSI_PID + 1];
        ctx.pids_programs = vec![null_mut(); MAX_PID];
        ctx.reset();
        assert_eq!(ctx.startbytes_pos, 0);
        assert_eq!(ctx.startbytes_avail, 0);
        assert_eq!(ctx.num_of_pids, 0);
        assert!(ctx.have_pids.iter().all(|&x| x == -1));
        assert!(ctx.pids_seen.iter().all(|&x| x == 0));
        for &val in ctx.min_pts.iter() {
            assert_eq!(val, u64::MAX);
        }
        assert!(ctx.stream_id_of_each_pid.iter().all(|&x| x == 0));
        assert!(ctx.pids_programs.iter().all(|&p| p.is_null()));
    }
    // #[test]
    // #[serial]
    #[allow(unused)]
    fn test_open_close_file() {
        let mut demuxer = CcxDemuxer::default();
        let test_file = NamedTempFile::new().unwrap();
        let file_path = test_file.path().to_str().unwrap();

        unsafe {
            assert_eq!(demuxer.open(file_path, &mut Options::default()), 0);
            assert!(demuxer.is_open());

            demuxer.close(&mut Options::default());
            assert!(!demuxer.is_open());
        }
    }

    // #[test]
    // #[serial]
    #[allow(unused)]
    fn test_open_invalid_file() {
        let mut demuxer = CcxDemuxer::default();
        unsafe {
            assert_eq!(
                demuxer.open("non_existent_file.txt", &mut Options::default()),
                -1
            );
            assert!(!demuxer.is_open());
        }
    }

    // #[test]
    // #[serial]
    #[allow(unused)]
    fn test_reopen_after_close() {
        let mut demuxer = CcxDemuxer::default();
        let test_file = NamedTempFile::new().unwrap();
        let file_path = test_file.path().to_str().unwrap();

        unsafe {
            assert_eq!(demuxer.open(file_path, &mut Options::default()), 0);
            demuxer.close(&mut Options::default());
            assert_eq!(demuxer.open(file_path, &mut Options::default()), 0);
            demuxer.close(&mut Options::default());
        }
    }

    #[test]
    #[serial]
    fn test_stream_mode_detection() {
        initialize_logger();
        let mut demuxer = CcxDemuxer::default();
        demuxer.auto_stream = StreamMode::Autodetect;

        let test_file = NamedTempFile::new().unwrap();
        let file_path = test_file.path().to_str().unwrap();

        unsafe {
            assert_eq!(demuxer.open(file_path, &mut Options::default()), 0);
            // Verify stream mode was detected
            assert_ne!(demuxer.stream_mode, StreamMode::Autodetect);
            demuxer.close(&mut Options::default());
        }
    }
    // #[serial]
    // #[test]
    #[allow(unused)]
    fn test_open_ccx_demuxer() {
        let mut demuxer = CcxDemuxer::default();
        let file_name = ""; // Replace with actual file name
        let result = unsafe { demuxer.open(file_name, &mut Options::default()) };
        assert_eq!(demuxer.infd, 3);
        assert_eq!(result, 0);
    }

    /// Helper: Create a temporary file with the given content and return its file descriptor.
    #[cfg(unix)]
    fn create_temp_file_with_content(content: &[u8]) -> (NamedTempFile, i32, u64) {
        let mut tmpfile = NamedTempFile::new().expect("Failed to create temp file");
        tmpfile.write_all(content).expect("Failed to write content");
        let metadata = tmpfile
            .as_file()
            .metadata()
            .expect("Failed to get metadata");
        let size = metadata.len();

        // Get the raw file descriptor.
        #[cfg(unix)]
        let fd = tmpfile.as_file().as_raw_fd();
        #[cfg(windows)]
        let fd = tmpfile.as_file().as_raw_handle();
        (tmpfile, fd, size)
    }
    #[cfg(windows)]
    fn create_temp_file_with_content(content: &[u8]) -> (NamedTempFile, RawHandle, u64) {
        let mut tmpfile = NamedTempFile::new().expect("Failed to create temp file");
        tmpfile.write_all(content).expect("Failed to write content");
        let metadata = tmpfile
            .as_file()
            .metadata()
            .expect("Failed to get metadata");
        let size = metadata.len();

        let handle: RawHandle = tmpfile.as_file().as_raw_handle();
        (tmpfile, handle, size)
    }

    /// Test that ccx_demuxer_get_file_size returns the correct file size for a valid file.
    #[test]
    #[serial]
    fn test_get_file_size_valid() {
        let content = b"Hello, world!";
        let (_tmpfile, fd, size) = create_temp_file_with_content(content);

        // Create a default demuxer and modify the infd field.
        let mut demuxer = CcxDemuxer::default();
        demuxer.infd = fd as _;

        // Call the file-size function.
        let ret = demuxer.get_filesize();
        assert_eq!(ret, size as i64, "File size should match the actual size");
    }

    /// Test that ccx_demuxer_get_file_size returns -1 when an invalid file descriptor is provided.
    #[test]
    fn test_get_file_size_invalid_fd() {
        let mut demuxer = CcxDemuxer::default();
        demuxer.infd = -1;
        let ret = demuxer.get_filesize();
        assert_eq!(
            ret, -1,
            "File size should be -1 for an invalid file descriptor"
        );
    }
    /// Test that the file position is restored after calling ccx_demuxer_get_file_size.
    #[test]
    #[serial]
    fn test_file_position_restored() {
        let content = b"Testing file position restoration.";
        let (tmpfile, fd, _size) = create_temp_file_with_content(content);
        let mut file = OpenOptions::new()
            .read(true)
            .write(true)
            .open(tmpfile.path())
            .expect("Failed to open file");
        // Move the file cursor to a nonzero position.
        file.seek(SeekFrom::Start(5)).expect("Failed to seek");
        let pos_before = file
            .stream_position()
            .expect("Failed to get current position");

        // Create a demuxer with the same file descriptor.
        let mut demuxer = CcxDemuxer::default();
        demuxer.infd = fd as _;

        // Call the file-size function.
        let _ = demuxer.get_filesize();

        // After calling the function, the file position should be restored.
        let pos_after = file
            .stream_position()
            .expect("Failed to get current position");
        assert_eq!(
            pos_before, pos_after,
            "File position should be restored after calling get_file_size"
        );
    }
}
