#![allow(non_camel_case_types)]
#![allow(unexpected_cfgs)]

use libc::{lseek, SEEK_CUR, SEEK_END, SEEK_SET};
use std::ffi::{CStr, CString};
use std::ptr::NonNull;

use crate::ccx_options;
use crate::decoder::common_structs::LibCcDecode;
use crate::demuxer::lib_ccx::{FileReport, LibCcxCtx};
use crate::demuxer::stream_functions::{detect_myth, detect_stream_type};
use crate::file_functions::FILEBUFFERSIZE;
use lib_ccxr::common::DataSource;
// use crate::common::{Codec, Decoder608Settings, DecoderDtvccSettings, OutputFormat, SelectCodec, StreamMode, StreamType};
use lib_ccxr::common::{Codec, Decoder608Settings, DecoderDtvccSettings, OutputFormat, SelectCodec, StreamMode, StreamType};
use lib_ccxr::time::Timestamp;
use lib_ccxr::util::log::ExitCause;
use lib_ccxr::{error, fatal, info};

const CCX_COMMON_EXIT_BUG_BUG: i32 = 1000;
// Constants
const SUB_STREAMS_CNT: usize = 10;
const MAX_PID: usize = 65536;
const MAX_NUM_OF_STREAMIDS: usize = 51;
const MAX_PSI_PID: usize = 8191;
const TS_PMT_MAP_SIZE: usize = 128;
const MAX_PROGRAM: usize = 128;
const MAX_PROGRAM_NAME_LEN: usize = 128;
pub const STARTBYTESLENGTH: usize = 1024 * 1024;

// STREAM_TYPE Enum
#[repr(u32)]
pub enum Stream_Type {
    PrivateStream1 = 0,
    Audio,
    Video,
    Count,
}

// ccx_demux_report Struct
pub struct CcxDemuxReport {
    pub program_cnt: u32,
    pub dvb_sub_pid: [u32; SUB_STREAMS_CNT],
    pub tlt_sub_pid: [u32; SUB_STREAMS_CNT],
    pub mp4_cc_track_cnt: u32,
}
pub struct CcxRational {
    pub(crate) num: i32,
    pub(crate) den: i32,
}
// program_info Struct
pub struct ProgramInfo {
    pub pid: i32,
    pub program_number: i32,
    pub initialized_ocr: i32, // Avoid initializing the OCR more than once
    pub analysed_PMT_once: u8, // 1-bit field
    pub version: u8,
    pub saved_section: [u8; 1021],
    pub crc: i32,
    pub valid_crc: u8, // 1-bit field
    pub name: [u8; MAX_PROGRAM_NAME_LEN],
    /**
     * -1 pid represent that pcr_pid is not available
     */
    pub pcr_pid: i16,
    pub got_important_streams_min_pts: [u64; Stream_Type::Count as usize],
    pub has_all_min_pts: i32,
}

// cap_info Struct
pub struct CapInfo {
    pub pid: i32,
    pub program_number: i32,
    pub stream: StreamType, // ccx_stream_type maps to StreamType
    pub codec: Codec,       // ccx_code_type maps to Codec
    pub capbufsize: i64,
    pub capbuf: *mut u8,
    pub capbuflen: i64, // Bytes read in capbuf
    pub saw_pesstart: i32,
    pub prev_counter: i32,
    pub codec_private_data: *mut std::ffi::c_void,
    pub ignore: i32,

    /**
     * List joining all streams in TS
     */
    pub all_stream: HList, // List head representing a hyperlinked list

    /**
     * List joining all sibling Stream in Program
     */
    pub sib_head: HList,
    pub sib_stream: HList,

    /**
     * List joining all sibling Stream in Program
     */
    pub pg_stream: HList,
}

// HList (Hyperlinked List)
pub struct HList {
    pub next: *mut HList,
    pub prev: *mut HList,
}

pub fn list_empty(head: &mut HList) -> bool {
    unsafe { (*head).next == head as *mut HList }
}
pub unsafe fn get_sib_stream_by_type(program: *mut CapInfo, codec_type: Codec) -> *mut CapInfo {
    if program.is_null() {
        return std::ptr::null_mut();
    }

    let mut iter = (*program).sib_head.next as *mut CapInfo;

    while !iter.is_null() && iter != program {
        if (*iter).codec == codec_type {
            return iter;
        }
        iter = (*iter).sib_stream.next as *mut CapInfo;
    }

    std::ptr::null_mut()
}
pub unsafe fn get_best_sib_stream(program: *mut CapInfo) -> *mut CapInfo {
    let mut info = get_sib_stream_by_type(program, Codec::Teletext);
    if !info.is_null() {
        return info;
    }

    info = get_sib_stream_by_type(program, Codec::Dvb);
    if !info.is_null() {
        return info;
    }

    info = get_sib_stream_by_type(program, Codec::AtscCc);
    if !info.is_null() {
        return info;
    }

    std::ptr::null_mut()
}

// Constants

// PSI_buffer Struct
pub struct PSI_buffer {
    pub prev_ccounter: u32,
    pub buffer: *mut u8,
    pub buffer_length: u32,
    pub ccounter: u32,
}


#[repr(C)]
pub struct DecodersCommonSettings {
    pub subs_delay: i64, // LLONG -> int64_t -> i64 in Rust

    pub output_format: OutputFormat, // ccx_output_format -> OutputFormat

    pub fix_padding: i32, // int -> i32
    pub extraction_start: Option<Timestamp>,
    pub extraction_end: Option<Timestamp>, // ccx_boundary_time -> Option<Timestamp>

    pub cc_to_stdout: i32,
    pub extract: i32,
    pub fullbin: i32,
    pub no_rollup: i32,
    pub noscte20: i32,

    pub settings_608: NonNull<Decoder608Settings>, // Pointer to struct
    pub settings_dtvcc: NonNull<DecoderDtvccSettings>, // Pointer to struct

    pub cc_channel: i32,
    pub send_to_srv: u32,
    pub hauppauge_mode: u32, // unsigned int -> u32
    pub program_number: i32,

    pub codec: SelectCodec, // ccx_code_type -> SelectCodec

    pub xds_write_to_file: i32,
    pub private_data: *mut std::ffi::c_void, // void* -> raw pointer

    pub ocr_quantmode: i32,
}

// PMT_entry Struct
pub struct PMT_entry {
    pub program_number: u32,
    pub elementary_PID: u32,
    pub stream_type: StreamType, // ccx_stream_type maps to StreamType
    pub printable_stream_type: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct EPGRating {
    pub country_code: [u8; 4], // char[4] -> fixed-size array of bytes
    pub age: u8, // uint8_t -> u8
}

#[repr(C)]
pub struct EPGEvent {
    pub id: u32, // uint32_t -> u32

    pub start_time_string: [u8; 21], // char[21] -> fixed-size array of bytes
    pub end_time_string: [u8; 21],

    pub running_status: u8, // uint8_t -> u8
    pub free_ca_mode: u8,

    pub iso_639_language_code: [u8; 4], // char[4] -> fixed-size array
    pub event_name: *mut u8, // char* -> raw pointer
    pub text: *mut u8,

    pub extended_iso_639_language_code: [u8; 4], // char[4] -> fixed-size array
    pub extended_text: *mut u8, // char* -> raw pointer

    pub has_simple: u8, // uint8_t -> u8

    pub ratings: *mut EPGRating, // struct EPG_rating* -> raw pointer
    pub num_ratings: u32, // uint32_t -> u32

    pub categories: *mut u8, // uint8_t* -> raw pointer
    pub num_categories: u32, // uint32_t -> u32

    pub service_id: u16, // uint16_t -> u16
    pub count: i64, // long long int -> i64

    pub live_output: u8, // uint8_t -> u8 (boolean flag)
}

const EPG_MAX_EVENTS: usize = 60 * 24 * 7; // Define the max event constant

#[repr(C)]
pub struct EITProgram {
    pub array_len: u32, // uint32_t -> u32
    pub epg_events: [EPGEvent; EPG_MAX_EVENTS], // struct EPG_event[EPG_MAX_EVENTS] -> fixed-size array
}


// ccx_demuxer Struct
pub struct CcxDemuxer {
    pub m2ts: i32,
    pub stream_mode: StreamMode,      // ccx_stream_mode_enum maps to StreamMode
    pub auto_stream: StreamMode,     // ccx_stream_mode_enum maps to StreamMode
    pub startbytes: [u8; STARTBYTESLENGTH],
    pub startbytes_pos: u32,
    pub startbytes_avail: i32,

    // User Specified Params
    pub ts_autoprogram: i32,
    pub ts_allprogram: i32,
    pub flag_ts_forced_pn: i32,
    pub flag_ts_forced_cappid: i32,
    pub ts_datastreamtype: i32,

    pub pinfo: [ProgramInfo; MAX_PROGRAM],
    pub nb_program: i32,

    // Subtitle codec type
    pub codec: Codec,   // ccx_code_type maps to Codec
    pub nocodec: Codec, // ccx_code_type maps to Codec

    pub cinfo_tree: CapInfo,

    // File Handles
    pub infd: i32,      // Descriptor number for input
    pub past: i64,      // Position in file, equivalent to ftell()

    // Global timestamps
    pub global_timestamp: i64,
    pub min_global_timestamp: i64,
    pub offset_global_timestamp: i64,
    pub last_global_timestamp: i64,
    pub global_timestamp_inited: i32,

    pub PID_buffers: [*mut PSI_buffer; MAX_PSI_PID],
    pub PIDs_seen: [i32; MAX_PID],

    pub stream_id_of_each_pid: [u8; MAX_PSI_PID + 1],
    pub min_pts: [u64; MAX_PSI_PID + 1],
    pub have_PIDs: [i32; MAX_PSI_PID + 1],
    pub num_of_PIDs: i32,

    pub PIDs_programs: [*mut PMT_entry; MAX_PID],
    pub freport: CcxDemuxReport,

    // Hauppauge support
    pub hauppauge_warning_shown: u32,

    pub multi_stream_per_prog: i32,

    pub last_pat_payload: *mut u8,
    pub last_pat_length: u32,

    pub filebuffer: *mut u8,
    pub filebuffer_start: i64,      // Position of buffer start relative to file
    pub filebuffer_pos: u32,       // Position of pointer relative to buffer start
    pub bytesinbuffer: u32,        // Number of bytes in buffer

    pub warning_program_not_found_shown: i32,

    pub strangeheader: i32, // Tracks if the last header was valid

    #[cfg(feature = "ffmpeg")]
    pub ffmpeg_ctx: *mut std::ffi::c_void,

    pub parent: *mut std::ffi::c_void,

    // Demuxer Context
    pub private_data: *mut std::ffi::c_void,
    pub print_cfg: Option<extern "C" fn(ctx: *mut CcxDemuxer)>,
    pub reset: Option<extern "C" fn(ctx: *mut CcxDemuxer)>,
    pub close: Option<extern "C" fn(ctx: *mut CcxDemuxer)>,
    pub open: Option<extern "C" fn(ctx: *mut CcxDemuxer, file_name: *const u8) -> i32>,
    pub is_open: Option<extern "C" fn(ctx: *mut CcxDemuxer) -> i32>,
    pub get_stream_mode: Option<extern "C" fn(ctx: *mut CcxDemuxer) -> i32>,
    pub get_filesize: Option<extern "C" fn(ctx: *mut CcxDemuxer) -> i64>,
}

impl CcxDemuxer {
    pub(crate) fn get_filesize(&self, p0: *mut CcxDemuxer) -> i64 {
        ccx_demuxer_get_file_size(p0)
    }
}

pub struct DemuxerData {
    pub program_number: i32,
    pub stream_pid: i32,
    pub codec: Codec,                // ccx_code_type maps to Codec
    pub bufferdatatype: lib_ccxr::common::BufferdataType, // ccx_bufferdata_type maps to BufferDataType
    pub buffer: *mut u8,
    pub len: usize,
    pub rollover_bits: u32,          // Tracks PTS rollover
    pub pts: i64,
    pub tb: CcxRational,             // Corresponds to ccx_rational
    pub next_stream: *mut DemuxerData,
    pub next_program: *mut DemuxerData,
}

impl CcxDemuxer {
    pub fn reset(&mut self) {
        unsafe {
            self.startbytes_pos = 0;
            self.startbytes_avail = 0;
            self.num_of_PIDs = 0;
            libc::memset(
                self.have_PIDs.as_mut_ptr() as *mut std::ffi::c_void,
                -1,
                (MAX_PSI_PID + 1) * size_of::<i32>(),
            );
            libc::memset(
                self.PIDs_seen.as_mut_ptr() as *mut std::ffi::c_void,
                0,
                MAX_PID * size_of::<i32>(),
            );
            for i in 0..=MAX_PSI_PID {
                self.min_pts[i] = u64::MAX;
            }
            libc::memset(
                self.stream_id_of_each_pid.as_mut_ptr() as *mut std::ffi::c_void,
                0,
                (MAX_PSI_PID + 1) * size_of::<u8>(),
            );
            libc::memset(
                self.PIDs_programs.as_mut_ptr() as *mut std::ffi::c_void,
                0,
                MAX_PID * size_of::<*mut PMT_entry>(),
            );
        }
    }
}

pub fn init_file_buffer(ctx: &mut CcxDemuxer) -> i32 {
    ctx.filebuffer_start = 0;
    ctx.filebuffer_pos = 0;
    if ctx.filebuffer.is_null() {
        ctx.filebuffer = unsafe {
            libc::malloc(FILEBUFFERSIZE) as *mut u8
        };
        ctx.bytesinbuffer = 0;
    }
    if ctx.filebuffer.is_null() {
        return -1;
    }
    0
}
impl CcxDemuxer {
    pub unsafe fn close(&mut self) {
        self.past = 0;
        if self.infd != -1 && ccx_options.input_source == DataSource::File as u32 {
            unsafe {
                libc::close(self.infd);
            }
            self.infd = -1;
            // ccx_options.activity_input_file_closed(); // TODO
        }
    }
}

impl CcxDemuxer {
    pub fn is_open(&self) -> bool {
        self.infd != -1
    }
}


impl CcxDemuxer {
    pub unsafe fn open(&mut self, file_name: &str) -> i32 {
        // Initialize some fields
        self.past = 0;
        self.min_global_timestamp = 0;
        self.global_timestamp_inited = 0;
        self.last_global_timestamp = 0;
        self.offset_global_timestamp = 0;

        // Conditional FFmpeg init
        //TODO Uncommnet after implement ffmpeg module
        // #[cfg(feature = "ffmpeg")]
        // {
        //     if let Some(ff_ctx) = init_ffmpeg_ctx(file_name) {
        //         self.ffmpeg_ctx = ff_ctx;
        //         self.stream_mode = StreamMode::Ffmpeg;
        //         self.auto_stream = StreamMode::Ffmpeg;
        //         return 0;
        //     } else {
        //         info!("Failed to initialize ffmpeg, falling back to legacy\n");
        //     }
        // }

        // Fallback: file-mode usage
        init_file_buffer(self);

        // Handle input source (stdin, network, tcp, or file open)
        if ccx_options.input_source == DataSource::Stdin as u32 {
            if self.infd != -1 {
                if ccx_options.print_file_reports != 0 {
                    print_file_report(&mut *(self.parent as *mut LibCcxCtx));
                }
                return -1;
            }
            self.infd = 0;
            println!("Reading from standard input\n");
        } else if ccx_options.input_source == DataSource::Network as u32 {
            if self.infd != -1 {
                if ccx_options.print_file_reports != 0 {
                    print_file_report(&mut *(self.parent as *mut LibCcxCtx));
                }
                return -1;
            }
            // self.infd = start_upd_srv(ccx_options.udpsrc, ccx_options.udpaddr, ccx_options.udpport); //TODO when networking module is implemented
            if self.infd < 0 {
                error!("{} {}",ccx_options.gui_mode_reports as u32, "socket() failed.");
                return CCX_COMMON_EXIT_BUG_BUG;
            }
        } else if ccx_options.input_source == DataSource::Tcp as u32 {
            if self.infd != -1 {
                if ccx_options.print_file_reports != 0 {
                    print_file_report(&mut *(self.parent as *mut LibCcxCtx));
                }
                return -1;
            }
            // self.infd = start_tcp_srv(ccx_options.tcpport, ccx_options.tcp_password); // Todo when networking module is implemented
        } else {
            // Open file
            let c_file_name = CString::new(file_name).expect("CString::new failed");
            self.infd = libc::open(c_file_name.as_ptr(), libc::O_RDONLY);

            if self.infd < 0 {
                return -1;
            }
        }

        // Auto-detection of stream type
        if self.auto_stream == StreamMode::Autodetect {
            detect_stream_type(self);
            match self.stream_mode {
                StreamMode::ElementaryOrNotFound => info!("File seems to be an elementary stream\n"),
                StreamMode::Transport => info!("Transport stream\n"),
                StreamMode::Program => info!("Program stream\n"),
                StreamMode::Asf => info!("ASF / DVR-MS\n"),
                StreamMode::Wtv => info!("WTV detected\n"),
                StreamMode::McpoodlesRaw => info!("McPoodle raw data\n"),
                StreamMode::Rcwt => info!("Raw caption with time data\n"),
                StreamMode::Mp4 => info!("MP4\n"),
                StreamMode::Gxf => info!("GXF\n"),
                StreamMode::Mkv => info!("Matroska/WebM\n"),
                #[cfg(feature = "wtv_debug")]
                StreamMode::HexDump => info!("Hex dump mode\n"),
                StreamMode::Mxf => info!("MXF\n"),
                StreamMode::Myth | StreamMode::Autodetect => {
                    fatal!(cause = ExitCause::Bug; "Impossible value in stream_mode.");
                }
                _ => (),
            }
        } else {
            self.stream_mode = self.auto_stream;
        }

        // Myth detection if requested
        match ccx_options.auto_myth {
            1 => { self.stream_mode = StreamMode::Myth; }
            2 => {
                if matches!(self.stream_mode, StreamMode::ElementaryOrNotFound | StreamMode::Program) {
                    if detect_myth(self) != 0 {
                        self.stream_mode = StreamMode::Myth;
                    }
                }
            }
            _ => {}
        }

        0
    }
}


/// This function returns the file size for a given demuxer.
/// Translated from the C function `ccx_demuxer_get_file_size`.
/// LLONG is `int64_t`, so we use `i64` in Rust.
pub fn ccx_demuxer_get_file_size(ctx: *mut CcxDemuxer) -> i64 {
    // LLONG ret = 0;
    let mut ret: i64;
    // int in = ctx->infd;
    let in_fd = unsafe { (*ctx).infd };

    // LLONG current = LSEEK(in, 0, SEEK_CUR);
    let current = unsafe { lseek(in_fd, 0, SEEK_CUR) };
    // LLONG length = LSEEK(in, 0, SEEK_END);
    let length = unsafe { lseek(in_fd, 0, SEEK_END) };
    if current < 0 || length < 0 {
        return -1;
    }

    // ret = LSEEK(in, current, SEEK_SET);
    ret = unsafe { lseek(in_fd, current, SEEK_SET) };
    if ret < 0 {
        return -1;
    }

    // return length;
    length
}

/// Translated from the C function `ccx_demuxer_get_stream_mode`.
/// Returns the current stream mode.
pub fn ccx_demuxer_get_stream_mode(ctx: &CcxDemuxer) -> i32 {
    // return ctx->stream_mode;
    ctx.stream_mode as i32
}

/// Translated from the C function `ccx_demuxer_print_cfg`.
/// Prints the current `auto_stream` mode for the demuxer.
// Note: `#ifdef WTV_DEBUG` becomes `#[cfg(feature = "wtv_debug")]` in Rust.
pub fn ccx_demuxer_print_cfg(ctx: &CcxDemuxer) {
    // switch (ctx->auto_stream)
    match ctx.auto_stream {
        StreamMode::ElementaryOrNotFound => {
            // mprint("Elementary");
            info!("Elementary");
        }
        StreamMode::Transport => {
            // mprint("Transport");
            info!("Transport");
        }
        StreamMode::Program => {
            // mprint("Program");
            info!("Program");
        }
        StreamMode::Asf => {
            // mprint("DVR-MS");
            info!("DVR-MS");
        }
        StreamMode::Wtv => {
            // mprint("Windows Television (WTV)");
            info!("Windows Television (WTV)");
        }
        StreamMode::McpoodlesRaw => {
            // mprint("McPoodle's raw");
            info!("McPoodle's raw");
        }
        StreamMode::Autodetect => {
            // mprint("Autodetect");
            info!("Autodetect");
        }
        StreamMode::Rcwt => {
            // mprint("BIN");
            info!("BIN");
        }
        StreamMode::Mp4 => {
            // mprint("MP4");
            info!("MP4");
        }
        StreamMode::Mkv => {
            // mprint("MKV");
            info!("MKV");
        }
        StreamMode::Mxf => {
            // mprint("MXF");
            info!("MXF");
        }
        #[cfg(feature = "wtv_debug")]
        StreamMode::HexDump => {
            // mprint("Hex");
            info!("Hex");
        }
        _ => {
            // fatal(CCX_COMMON_EXIT_BUG_BUG, "BUG: Unknown stream mode. Please file a bug report on Github.\n");
            fatal!(
                cause = ExitCause::Bug;
                "BUG: Unknown stream mode. Please file a bug report on Github.\n"
            );
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////
fn y_n(count: i32) -> &'static str {
    if count != 0 { "YES" } else { "NO" }
}

/// Translated version of the C `print_file_report` function, preserving structure
/// and replacing `#ifdef` with `#[cfg(feature = "wtv_debug")]`.
/// Uses `printf` from libc.
pub fn print_file_report(ctx: &mut LibCcxCtx) {
    unsafe {
        let mut dec_ctx: *mut LibCcDecode = std::ptr::null_mut();
        let demux_ctx = &mut *ctx.demux_ctx;

        println!("File:");

        match DataSource::from(ccx_options.input_source) {
            DataSource::File => {
                if ctx.current_file < 0 {
                    println!("file is not opened yet");
                    return;
                }
                if ctx.current_file >= ctx.num_input_files {
                    return;
                }
                let file_name = CString::new(ctx.inputfile[ctx.current_file as usize].clone())
                    .unwrap_or_else(|_| CString::new("???").unwrap());
                println!("{}", file_name.to_str().unwrap_or("???"));
            }
            DataSource::Stdin => {
                println!("stdin");
            }
            DataSource::Tcp | DataSource::Network => {
                println!("network");
            }
        }

        println!("Stream Mode:");

        match demux_ctx.stream_mode {
            StreamMode::Transport => {
                println!("Transport Stream");
                println!("Program Count: {}", demux_ctx.freport.program_cnt);
                println!("Program Numbers:");
                for i in 0..demux_ctx.nb_program {
                    println!("{}", demux_ctx.pinfo[i as usize].program_number);
                }
                println!();

                for i in 0..65536 {
                    let pid_program = demux_ctx.PIDs_programs[i as usize];
                    if pid_program.is_null() {
                        continue;
                    }
                    let prog_num = (*pid_program).program_number;
                    println!("PID: {}, Program: {}", i, prog_num);
                    let mut j = 0;
                    while j < SUB_STREAMS_CNT {
                        if demux_ctx.freport.dvb_sub_pid[j] == i as u32 {
                            println!("DVB Subtitles");
                            break;
                        }
                        if demux_ctx.freport.tlt_sub_pid[j] == i as u32 {
                            println!("Teletext Subtitles");
                            break;
                        }
                        j += 1;
                    }
                    if j == SUB_STREAMS_CNT {
                        let idx = (*pid_program).printable_stream_type as usize;
                        // println!("{}", get_desc_placeholder(idx));
                        let desc = unsafe { CStr::from_ptr(get_desc_placeholder(idx)) }.to_str().unwrap_or("Unknown");
                        println!("{}", desc);
                    }
                }
            }
            StreamMode::Program => println!("Program Stream"),
            StreamMode::Asf => println!("ASF"),
            StreamMode::Wtv => println!("WTV"),
            StreamMode::ElementaryOrNotFound => println!("Not Found"),
            StreamMode::Mp4 => println!("MP4"),
            StreamMode::McpoodlesRaw => println!("McPoodle's raw"),
            StreamMode::Rcwt => println!("BIN"),
            #[cfg(feature = "wtv_debug")]
            StreamMode::HexDump => println!("Hex"),
            _ => {}
        }

        if list_empty(&mut demux_ctx.cinfo_tree.all_stream) {
            // print_cc_report(ctx, std::ptr::null_mut()); // TODO
        }

        let mut program = demux_ctx.cinfo_tree.pg_stream.next;
        while program != &demux_ctx.cinfo_tree.pg_stream as *const _ as *mut _ {
            let program_ptr = program as *mut CapInfo;
            println!("//////// Program #{}: ////////", (*program_ptr).program_number);

            println!("DVB Subtitles:");
            let mut info = get_sib_stream_by_type(program_ptr, Codec::Dvb);
            println!("{}", if !info.is_null() { "Yes" } else { "No" });

            println!("Teletext:");
            info = get_sib_stream_by_type(program_ptr, Codec::Teletext);
            println!("{}", if !info.is_null() { "Yes" } else { "No" });

            println!("ATSC Closed Caption:");
            info = get_sib_stream_by_type(program_ptr, Codec::AtscCc);
            println!("{}", if !info.is_null() { "Yes" } else { "No" });

            let best_info = get_best_sib_stream(program_ptr);
            if !best_info.is_null() {
                if (*dec_ctx).in_bufferdatatype == lib_ccxr::common::BufferdataType::Pes
                    && (demux_ctx.stream_mode == StreamMode::Transport
                    || demux_ctx.stream_mode == StreamMode::Program
                    || demux_ctx.stream_mode == StreamMode::Asf
                    || demux_ctx.stream_mode == StreamMode::Wtv)
                {
                    println!("Width: {}", (*dec_ctx).current_hor_size);
                    println!("Height: {}", (*dec_ctx).current_vert_size);
                    let ar_idx = (*dec_ctx).current_aspect_ratio as usize;
                    println!("Aspect Ratio: {}", lib_ccxr::common::ASPECT_RATIO_TYPES[ar_idx]);
                    let fr_idx = (*dec_ctx).current_frame_rate as usize;
                    println!("Frame Rate: {}", lib_ccxr::common::FRAMERATES_TYPES[fr_idx]);
                }
                println!();
            }

            program = (*program).next;
        }

        println!("MPEG-4 Timed Text: {}", y_n(ctx.freport.mp4_cc_track_cnt as i32));
        if ctx.freport.mp4_cc_track_cnt != 0 {
            println!("MPEG-4 Timed Text tracks count: {}", ctx.freport.mp4_cc_track_cnt);
        }

        libc::memset(
            &mut ctx.freport as *mut FileReport as *mut libc::c_void,
            0,
            size_of::<FileReport>(),
        );
    }
}

/// Dummy placeholder for `desc[...]`.
/// In real code, you'd have something like `&desc[index]`.
unsafe fn get_desc_placeholder(_index: usize) -> *const i8 {
    b"Unknown\0".as_ptr() as *const i8
}