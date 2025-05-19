#![allow(non_camel_case_types)]
#![allow(unexpected_cfgs)]
#![allow(unused_mut)]
#![allow(clippy::needless_lifetimes)]

use lib_ccxr::activity::ActivityExt;
use lib_ccxr::common::{BufferdataType, Codec, Decoder608Report, Decoder608Settings, DecoderDtvccReport, DecoderDtvccSettings, OutputFormat, SelectCodec, StreamMode, StreamType};
use lib_ccxr::common::{DataSource, Options};
use crate::demuxer::stream_functions::{detect_myth, detect_stream_type};
use crate::file_functions::file::FILEBUFFERSIZE;
use lib_ccxr::time::Timestamp;
use lib_ccxr::util::log::ExitCause;
use lib_ccxr::{common, fatal, info};
use std::ffi::CStr;
use std::fs::File;
use std::io::{Seek, SeekFrom};

#[cfg(unix)]
use std::os::fd::{FromRawFd, IntoRawFd};
#[cfg(windows)]
use std::os::windows::io::{FromRawHandle, IntoRawHandle};
use std::path::Path;
use std::ptr::{null_mut, NonNull};
use std::sync::{LazyLock, Mutex};
use std::{mem, ptr};
use cfg_if::cfg_if;
use crate::bindings::{lib_cc_decode, lib_ccx_ctx, list_head};
use crate::hlist::{init_list_head, list_del, list_empty, list_entry};

cfg_if! {
    if #[cfg(test)] {
        use crate::demuxer::demux::tests::{print_file_report};
    } else {
        use crate::{print_file_report};
    }
}
pub static CCX_OPTIONS: LazyLock<Mutex<Options>> = LazyLock::new(|| Mutex::new(Options::default()));

// Constants
pub const SUB_STREAMS_CNT: usize = 10;
pub const MAX_PID: usize = 65536;
pub const MAX_NUM_OF_STREAMIDS: usize = 51;
pub const MAX_PSI_PID: usize = 8191;
pub const TS_PMT_MAP_SIZE: usize = 128;
pub const MAX_PROGRAM: usize = 128;
pub const MAX_PROGRAM_NAME_LEN: usize = 128;
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
#[repr(C)]
#[derive(Debug)]
pub struct FileReport {
    pub width: u32,
    pub height: u32,
    pub aspect_ratio: u32,
    pub frame_rate: u32,
    pub data_from_608: *mut Decoder608Report, // Pointer to Decoder608Report
    pub data_from_708: *mut DecoderDtvccReport, // Pointer to DecoderDtvccReport
    pub mp4_cc_track_cnt: u32,
}
pub struct CcxRational {
    pub(crate) num: i32,
    pub(crate) den: i32,
}
// program_info Struct
#[derive(Copy, Clone)]
pub struct ProgramInfo {
    pub pid: i32,
    pub program_number: i32,
    pub initialized_ocr: i32,  // Avoid initializing the OCR more than once
    pub analysed_pmt_once: u8, // 1-bit field
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
    pub all_stream: list_head, // List head representing a hyperlinked list

    /**
     * List joining all sibling Stream in Program
     */
    pub sib_head: list_head,
    pub sib_stream: list_head,

    /**
     * List joining all sibling Stream in Program
     */
    pub pg_stream: list_head,
}

/// # Safety
/// This function is unsafe because it dereferences a raw pointer.
pub unsafe extern "C" fn is_decoder_processed_enough(ctx: *mut lib_ccx_ctx) -> i32 {
    // Use core::mem::offset_of!() for safer offset calculation
    const LIST_OFFSET: usize = memoffset::offset_of!(lib_cc_decode, list);

    let head = &(*ctx).dec_ctx_head;
    let mut current = head.next;

    while current != &(*ctx).dec_ctx_head as *const list_head as *mut list_head {
        if current.is_null() {
            break;
        }

        // Convert list node to containing struct
        let dec_ctx = (current as *mut u8).sub(LIST_OFFSET) as *mut lib_cc_decode;

        // Check if current decoder meets the condition
        if (*dec_ctx).processed_enough == 1 && (*ctx).multiprogram == 0 {
            return 1;
        }

        // Move to next node with null check
        current = (*current).next;
        if current == head.next {
            // Cycle detected, break to prevent infinite loop
            break;
        }
    }

    0
}
/// # Safety
/// This function is unsafe because it dereferences a raw pointer.
pub unsafe fn get_sib_stream_by_type(program: *mut CapInfo, codec_type: Codec) -> *mut CapInfo {
    if program.is_null() {
        return null_mut();
    }
    // Compute the offset of the `sib_stream` field within CapInfo.
    let offset = {
        let dummy = mem::MaybeUninit::<CapInfo>::uninit();
        let base_ptr = dummy.as_ptr() as usize;
        let member_ptr = &(*dummy.as_ptr()).sib_stream as *const _ as usize;
        member_ptr - base_ptr
    };

    let head = &(*program).sib_head as *const list_head as *mut list_head;
    let mut current = (*head).next;
    while !current.is_null() && current != head {
        let cap_ptr = (current as *mut u8).sub(offset) as *mut CapInfo;
        if cap_ptr.is_null() {
            break;
        }
        if (*cap_ptr).codec == codec_type {
            return cap_ptr;
        }
        current = (*current).next;
    }
    null_mut()
}

/// # Safety
/// This function is unsafe because it dereferences a raw pointer.
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

    null_mut()
}

// Constants

// PSI_buffer Struct
pub struct PSIBuffer {
    pub prev_ccounter: u32,
    pub buffer: *mut u8,
    pub buffer_length: u32,
    pub ccounter: u32,
}
impl Default for PSIBuffer {
    fn default() -> Self {
        PSIBuffer {
            prev_ccounter: 0,
            buffer: Box::into_raw(Box::new(0u8)),
            buffer_length: 0,
            ccounter: 0,
        }
    }
}

impl PSIBuffer {
    pub(crate) fn default() -> PSIBuffer {
        PSIBuffer {
            prev_ccounter: 0,
            buffer: Box::into_raw(Box::new(0u8)),
            buffer_length: 0,
            ccounter: 0,
        }
    }
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

impl DecodersCommonSettings {
    pub(crate) fn default() -> DecodersCommonSettings {
        DecodersCommonSettings {
            subs_delay: 0,
            output_format: OutputFormat::Srt,
            fix_padding: 0,
            extraction_start: None,
            extraction_end: None,
            cc_to_stdout: 0,
            extract: 0,
            fullbin: 0,
            no_rollup: 0,
            noscte20: 0,
            settings_608: NonNull::dangling(),
            settings_dtvcc: NonNull::dangling(),
            cc_channel: 0,
            send_to_srv: 0,
            hauppauge_mode: 0,
            program_number: 0,
            codec: SelectCodec::None,
            xds_write_to_file: 0,
            private_data: null_mut(),
            ocr_quantmode: 0,
        }
    }
}

// PMT_entry Struct
pub struct PMTEntry {
    pub program_number: u32,
    pub elementary_pid: u32,
    pub stream_type: StreamType, // ccx_stream_type maps to StreamType
    pub printable_stream_type: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct EPGRating {
    pub country_code: [u8; 4], // char[4] -> fixed-size array of bytes
    pub age: u8,               // uint8_t -> u8
}
#[derive(Copy, Clone)]
#[repr(C)]
pub struct EPGEvent {
    pub id: u32, // uint32_t -> u32

    pub start_time_string: [u8; 21], // char[21] -> fixed-size array of bytes
    pub end_time_string: [u8; 21],

    pub running_status: u8, // uint8_t -> u8
    pub free_ca_mode: u8,

    pub iso_639_language_code: [u8; 4], // char[4] -> fixed-size array
    pub event_name: *mut u8,            // char* -> raw pointer
    pub text: *mut u8,

    pub extended_iso_639_language_code: [u8; 4], // char[4] -> fixed-size array
    pub extended_text: *mut u8,                  // char* -> raw pointer

    pub has_simple: u8, // uint8_t -> u8

    pub ratings: *mut EPGRating, // struct EPG_rating* -> raw pointer
    pub num_ratings: u32,        // uint32_t -> u32

    pub categories: *mut u8, // uint8_t* -> raw pointer
    pub num_categories: u32, // uint32_t -> u32

    pub service_id: u16, // uint16_t -> u16
    pub count: i64,      // long long int -> i64

    pub live_output: u8, // uint8_t -> u8 (boolean flag)
}

const EPG_MAX_EVENTS: usize = 60 * 24 * 7; // Define the max event constant

#[repr(C)]
pub struct EITProgram {
    pub array_len: u32,                         // uint32_t -> u32
    pub epg_events: [EPGEvent; EPG_MAX_EVENTS], // struct EPG_event[EPG_MAX_EVENTS] -> fixed-size array
}

impl EITProgram {
    #[allow(dead_code)]
    pub(crate) fn default() -> EITProgram {
        EITProgram {
            array_len: 0,
            epg_events: [EPGEvent {
                id: 0,
                start_time_string: [0; 21],
                end_time_string: [0; 21],
                running_status: 0,
                free_ca_mode: 0,
                iso_639_language_code: [0; 4],
                event_name: Box::into_raw(Box::new(0u8)),
                text: Box::into_raw(Box::new(0u8)),
                extended_iso_639_language_code: [0; 4],
                extended_text: Box::into_raw(Box::new(0u8)),
                has_simple: 0,
                ratings: Box::into_raw(Box::new(EPGRating {
                    country_code: [0; 4],
                    age: 0,
                })),
                num_ratings: 0,
                categories: Box::into_raw(Box::new(0u8)),
                num_categories: 0,
                service_id: 0,
                count: 0,
                live_output: 0,
            }; EPG_MAX_EVENTS],
        }
    }
}

// ccx_demuxer Struct
pub struct CcxDemuxer<'a> {
    pub m2ts: i32,
    pub stream_mode: StreamMode, // ccx_stream_mode_enum maps to StreamMode
    pub auto_stream: StreamMode, // ccx_stream_mode_enum maps to StreamMode
    pub startbytes: Vec<u8>,
    pub startbytes_pos: u32,
    pub startbytes_avail: i32,

    // User Specified Params
    pub ts_autoprogram: bool,
    pub ts_allprogram: bool,
    pub flag_ts_forced_pn: bool,
    pub flag_ts_forced_cappid: bool,
    pub ts_datastreamtype: StreamType,

    pub pinfo: Vec<ProgramInfo>,
    pub nb_program: usize,
    // Subtitle codec type
    pub codec: Codec,   // ccx_code_type maps to Codec
    pub nocodec: Codec, // ccx_code_type maps to Codec

    pub cinfo_tree: CapInfo,

    // File Handles
    pub infd: i32, // Descriptor number for input
    pub past: i64, // Position in file, equivalent to ftell()

    // Global timestamps
    pub global_timestamp: Timestamp,
    pub min_global_timestamp: Timestamp,
    pub offset_global_timestamp: Timestamp,
    pub last_global_timestamp: Timestamp,
    pub global_timestamp_inited: Timestamp,

    pub pid_buffers: Vec<*mut PSIBuffer>,
    pub pids_seen: Vec<i32>,

    pub stream_id_of_each_pid: Vec<u8>,
    pub min_pts: Vec<u64>,
    pub have_pids: Vec<i32>,
    pub num_of_pids: i32,
    pub pids_programs: Vec<*mut PMTEntry>,
    pub freport: CcxDemuxReport,

    // Hauppauge support
    pub hauppauge_warning_shown: bool,

    pub multi_stream_per_prog: i32,

    pub last_pat_payload: *mut u8,
    // pub last_pat_payload: Option<NonNull<u8>>,
    pub last_pat_length: u32,

    pub filebuffer: *mut u8,
    pub filebuffer_start: i64, // Position of buffer start relative to file
    pub filebuffer_pos: u32,   // Position of pointer relative to buffer start
    pub bytesinbuffer: u32,    // Number of bytes in buffer

    pub warning_program_not_found_shown: bool,

    pub strangeheader: i32, // Tracks if the last header was valid


    pub parent: Option<&'a mut lib_ccx_ctx>,
    pub private_data: *mut std::ffi::c_void,// TODO - this struct contains a large variety of contexts
    #[cfg(feature = "enable_ffmpeg")]
    pub ffmpeg_ctx: *mut std::ffi::c_void,
}

impl<'a> Default for CcxDemuxer<'a> {
    fn default() -> Self {
        {
            CcxDemuxer {
                m2ts: 0,
                stream_mode: StreamMode::default(), // Assuming StreamMode has a Default implementation
                auto_stream: StreamMode::default(), // Assuming StreamMode has a Default implementation
                startbytes: vec![0; STARTBYTESLENGTH],
                startbytes_pos: 0,
                startbytes_avail: 0,

                // User Specified Params
                ts_autoprogram: false,
                ts_allprogram: false,
                flag_ts_forced_pn: false,
                flag_ts_forced_cappid: false,
                ts_datastreamtype: StreamType::Unknownstream,

                pinfo: vec![ProgramInfo::default(); MAX_PROGRAM],
                nb_program: 0,

                // Subtitle codec type
                codec: Codec::Dvb,
                nocodec: Codec::Dvb, // Assuming Codec has a Default implementation

                cinfo_tree: CapInfo::default(),

                // File Handles
                infd: -1,
                past: 0,
                global_timestamp: Timestamp::from_millis(0),
                min_global_timestamp: Timestamp::from_millis(0),
                offset_global_timestamp: Timestamp::from_millis(0),
                last_global_timestamp: Timestamp::from_millis(0),
                global_timestamp_inited: Timestamp::from_millis(0),

                pid_buffers: vec![null_mut(); MAX_PSI_PID],
                pids_seen: vec![0; MAX_PID],

                stream_id_of_each_pid: vec![0; MAX_PSI_PID + 1],
                min_pts: vec![0; MAX_PSI_PID + 1],
                have_pids: vec![0; MAX_PSI_PID + 1],
                num_of_pids: 0,
                pids_programs: vec![null_mut(); MAX_PID],
                freport: CcxDemuxReport::default(), // Assuming CcxDemuxReport has a Default implementation
                // Hauppauge support
                hauppauge_warning_shown: false,

                multi_stream_per_prog: 0,

                last_pat_payload: null_mut(),
                last_pat_length: 0,

                filebuffer: null_mut(),
                filebuffer_start: 0,
                filebuffer_pos: 0,
                bytesinbuffer: 0,

                warning_program_not_found_shown: false,

                strangeheader: 0,

                #[cfg(feature = "enable_ffmpeg")]
                ffmpeg_ctx: null_mut(),

                parent: None,
                private_data: null_mut(),

            }
        }
    }
}
impl Default for ProgramInfo {
    fn default() -> Self {
        ProgramInfo {
            pid: -1,
            program_number: 0,
            initialized_ocr: 0,
            analysed_pmt_once: 0,
            version: 0,
            saved_section: [0; 1021], // Initialize saved_section to zeroes
            crc: 0,
            valid_crc: 0,
            name: [0; MAX_PROGRAM_NAME_LEN], // Initialize name to zeroes
            pcr_pid: -1,                     // -1 indicates pcr_pid is not available
            got_important_streams_min_pts: [0; Stream_Type::Count as usize], // Initialize to zeroes
            has_all_min_pts: 0,
        }
    }
}
impl Default for CapInfo {
    fn default() -> Self {
        CapInfo {
            pid: -1,
            program_number: 0,
            stream: StreamType::default(),
            codec: Codec::Dvb,
            capbufsize: 0,
            capbuf: null_mut(),
            capbuflen: 0,
            saw_pesstart: 0,
            prev_counter: 0,
            codec_private_data: null_mut(),
            ignore: 0,

            // Initialize lists to empty or default states
            all_stream: list_head::default(), // Assuming HList has a Default impl
            sib_head: list_head::default(),
            sib_stream: list_head::default(),
            pg_stream: list_head::default(),
        }
    }
}
impl Default for CcxDemuxReport {
    fn default() -> Self {
        CcxDemuxReport {
            program_cnt: 0,
            dvb_sub_pid: [0; SUB_STREAMS_CNT], // Initialize array to zeroes
            tlt_sub_pid: [0; SUB_STREAMS_CNT], // Initialize array to zeroes
            mp4_cc_track_cnt: 0,
        }
    }
}

impl<'a> CcxDemuxer<'a> {
    pub fn get_filesize(&mut self) -> i64 {
        ccx_demuxer_get_file_size(self)
    }
}

pub struct DemuxerData {
    pub program_number: i32,
    pub stream_pid: i32,
    pub codec: Codec,                   // ccx_code_type maps to Codec
    pub bufferdatatype: BufferdataType, // ccx_bufferdata_type maps to BufferDataType
    pub buffer: *mut u8,
    pub len: usize,
    pub rollover_bits: u32, // Tracks PTS rollover
    pub pts: i64,
    pub tb: CcxRational, // Corresponds to ccx_rational
    pub next_stream: *mut DemuxerData,
    pub next_program: *mut DemuxerData,
}
impl Default for DemuxerData {
    fn default() -> Self {
        DemuxerData {
            program_number: 0,
            stream_pid: 0,
            codec: Codec::Dvb,
            bufferdatatype: BufferdataType::Unknown,
            buffer: null_mut(),
            len: 0,
            rollover_bits: 0,
            pts: 0,
            tb: CcxRational { num: 0, den: 0 },
            next_stream: null_mut(),
            next_program: null_mut(),
        }
    }
}

pub fn init_file_buffer(ctx: &mut CcxDemuxer) -> i32 {
    ctx.filebuffer_start = 0;
    ctx.filebuffer_pos = 0;
    if ctx.filebuffer.is_null() {
        // Allocate FILEBUFFERSIZE bytes using a Vec and leak it.
        let mut buf = vec![0u8; FILEBUFFERSIZE].into_boxed_slice();
        ctx.filebuffer = buf.as_mut_ptr();
        mem::forget(buf);
        ctx.bytesinbuffer = 0;
    }
    if ctx.filebuffer.is_null() {
        return -1;
    }
    0
}
impl<'a> CcxDemuxer<'a> {
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
            // Set each min_pts[i] to u64::MAX for i in 0..=MAX_PSI_PID.
            for i in 0..=MAX_PSI_PID {
                if !self.min_pts.is_empty() {
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
}

impl<'a> CcxDemuxer<'a> {
    pub fn close(&mut self, ccx_options : &mut Options) {
        self.past = 0;
        if self.infd != -1 && ccx_options.input_source == DataSource::File {
            // Convert raw fd to Rust File to handle closing
            #[cfg(unix)]
            {
                let file = unsafe { File::from_raw_fd(self.infd) };
                drop(file); // This closes the file descriptor
                self.infd = -1;
                ccx_options.activity_input_file_closed();
            }
            #[cfg(windows)]
            {
                let file = unsafe { File::from_raw_handle(self.infd as _) };
                drop(file); // This closes the file descriptor
                self.infd = -1;
                ccx_options.activity_input_file_closed();
            }
        }
    }
}

impl<'a> CcxDemuxer<'a> {
    pub fn is_open(&self) -> bool {
        self.infd != -1
    }
}

impl<'a> CcxDemuxer<'a> {
    /// # Safety
    /// detect_stream_type is an unsafe function
    pub unsafe fn open(&mut self, file_name: &str) -> i32 {
        let ccx_options = CCX_OPTIONS.lock().unwrap();

        // Initialize timestamp fields
        self.past = 0;
        self.min_global_timestamp = Timestamp::from_millis(0);
        self.global_timestamp_inited = Timestamp::from_millis(0);
        self.last_global_timestamp = Timestamp::from_millis(0);
        self.offset_global_timestamp = Timestamp::from_millis(0);

        // FFmpeg initialization (commented out until implemented)
        // #[cfg(feature = "enable_ffmpeg")]
        // {
        //     self.ffmpeg_ctx = init_ffmpeg(file_name);
        //     if !self.ffmpeg_ctx.is_null() {
        //         self.stream_mode = StreamMode::Ffmpeg;
        //         self.auto_stream = StreamMode::Ffmpeg;
        //         return 0;
        //     } else {
        //         info!("Failed to initialize ffmpeg, falling back to legacy");
        //     }
        // }

        init_file_buffer(self);

        match ccx_options.input_source {
            DataSource::Stdin => {
                if self.infd != -1 {
                    if ccx_options.print_file_reports {
                        {
                            print_file_report(*self.parent.as_mut().unwrap());
                        }
                    }
                    return -1;
                }
                self.infd = 0;
                print!("\n\r-----------------------------------------------------------------\n");
                print!("\rReading from standard input\n");
            }
            DataSource::Network => {
                if self.infd != -1 {
                    if ccx_options.print_file_reports {
                        {
                            print_file_report(*self.parent.as_mut().unwrap());
                        }
                    }
                    return -1;
                }
                // start_upd_srv implementation pending
                self.infd = -1; // Placeholder
                if self.infd < 0 {
                    // print_error(ccx_options.gui_mode_reports, "socket() failed.");
                    return ExitCause::Bug as i32;
                }
            }
            DataSource::Tcp => {
                if self.infd != -1 {
                    if ccx_options.print_file_reports {
                        {
                            print_file_report(*self.parent.as_mut().unwrap());
                        }
                    }
                    return -1;
                }
                // start_tcp_srv implementation pending
                self.infd = -1; // Placeholder
            }
            DataSource::File => {
                let file_result = File::open(Path::new(file_name));
                match file_result {
                    Ok(file) => {
                        #[cfg(unix)]
                        {
                            self.infd = file.into_raw_fd();
                        }
                        #[cfg(windows)]
                        {
                            self.infd = file.into_raw_handle() as _;
                        }
                    }
                    Err(_) => return -1,
                }
            }
        }

        // Stream mode detection
        if self.auto_stream == StreamMode::Autodetect {
            detect_stream_type(self);
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
            Some(false) => {
                self.stream_mode = StreamMode::Myth;
            }
            Some(true) => {
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
}

/// This function returns the file size for a given demuxer.
/// C function `ccx_demuxer_get_file_size`.
/// LLONG is `int64_t`, so we use `i64` in Rust.
pub fn ccx_demuxer_get_file_size(ctx: &mut CcxDemuxer) -> i64 {
    // Get the file descriptor from ctx.
    let in_fd = ctx.infd;
    if in_fd < 0 {
        return -1;
    }

    // SAFETY: We are creating a File from an existing raw fd.
    // To prevent the File from closing the descriptor on drop,
    // we call into_raw_fd() after using it.
    #[cfg(unix)]
    let mut file = unsafe { File::from_raw_fd(in_fd) };
    #[cfg(windows)]
    let mut file = unsafe { File::from_raw_handle(in_fd as _) };

    // Get current position: equivalent to LSEEK(in, 0, SEEK_CUR);
    let current = match file.stream_position() {
        Ok(pos) => pos,
        Err(_) => {
            // Return the fd back and then -1.
            #[cfg(unix)]
            let _ = file.into_raw_fd();
            #[cfg(windows)]
            let _ = file.into_raw_handle();
            return -1;
        }
    };

    // Get file length: equivalent to LSEEK(in, 0, SEEK_END);
    let length = match file.seek(SeekFrom::End(0)) {
        Ok(pos) => pos,
        Err(_) => {
            #[cfg(unix)]
            let _ = file.into_raw_fd();
            #[cfg(windows)]
            let _ = file.into_raw_handle();
            return -1;
        }
    };

    // If current or length is negative, return -1.
    // (This check is somewhat redundant because seek returns Result<u64, _>,
    //  but we keep it for exact logic parity with the C code.)

    // Restore the file position: equivalent to LSEEK(in, current, SEEK_SET);
    #[allow(unused_variables)]
    let ret = match file.seek(SeekFrom::Start(current)) {
        Ok(pos) => pos,
        Err(_) => {
            #[cfg(unix)]
            let _ = file.into_raw_fd();
            #[cfg(windows)]
            let _ = file.into_raw_handle();
            return -1;
        }
    };

    // Return the fd back to its original owner.
    #[cfg(unix)]
    let _ = file.into_raw_fd();
    #[cfg(windows)]
    let _ = file.into_raw_handle();
    length as i64
}

/// C function `ccx_demuxer_get_stream_mode`.
/// Returns the current stream mode.
pub fn ccx_demuxer_get_stream_mode(ctx: &CcxDemuxer) -> i32 {
    // return ctx->stream_mode;
    ctx.stream_mode as i32
}

/// C function `ccx_demuxer_print_cfg`.
/// Prints the current `auto_stream` mode for the demuxer.
// Note: `#ifdef WTV_DEBUG` becomes `#[cfg(feature = "wtv_debug")]` in Rust.
pub fn ccx_demuxer_print_cfg(ctx: &CcxDemuxer) {
    match ctx.auto_stream {
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

//////////////////////////////////////////////////////////////////////////////////
fn y_n(count: i32) -> &'static str {
    if count != 0 {
        "YES"
    } else {
        "NO"
    }
}

#[allow(clippy::manual_c_str_literals)]
pub fn get_desc_placeholder(_index: usize) -> *const i8 {
    b"Unknown\0".as_ptr() as *const i8
}
pub fn freep<T>(ptr: &mut *mut T) {
    unsafe {
        if !ptr.is_null() {
            let _ = Box::from_raw(*ptr);
            *ptr = null_mut();
        }
    }
}


// Tests for ccx_demuxer_delete
pub fn create_capinfo() -> *mut CapInfo {
    Box::into_raw(Box::new(CapInfo {
        all_stream: list_head {
            next: null_mut(),
            prev: null_mut(),
        },
        capbuf: Box::into_raw(Box::new(0u8)),
        ..Default::default()
    }))
}

/// # Safety
/// This function is unsafe because it calls list_del.
pub unsafe fn dinit_cap(ctx: &mut CcxDemuxer) {
    // Calculate offset of all_stream within CapInfo
    let offset = {
        let mut dummy = CapInfo {
            pid: 0,
            all_stream: list_head::default(),
            sib_head: list_head::default(),
            sib_stream: list_head::default(),
            pg_stream: list_head::default(),
            ..Default::default()
        };
        &dummy.all_stream as *const list_head as usize - &dummy as *const CapInfo as usize
    };

    // Process all_stream list
    while !list_empty(&ctx.cinfo_tree.all_stream) {
        let current = ctx.cinfo_tree.all_stream.next;
        let entry = list_entry::<CapInfo>(current, offset);

        // Remove from list before processing
        if let Some(current) = current.as_mut() {
            list_del(current);
        }
        // Free resources
        freep(&mut (*entry).capbuf);
        freep(&mut (*entry).codec_private_data);
        let _ = Box::from_raw(entry);
    }

    // Reinitialize all relevant list heads
    init_list_head(&mut ctx.cinfo_tree.all_stream);
    init_list_head(&mut ctx.cinfo_tree.sib_stream);
    init_list_head(&mut ctx.cinfo_tree.pg_stream);
}

// ccx_demuxer_delete remains similar
#[allow(unused)]
unsafe fn ccx_demuxer_delete(ctx: &mut *mut CcxDemuxer) {
    if ctx.is_null() || (*ctx).is_null() {
        return;
    }

    let lctx = &mut **ctx;
    dinit_cap(lctx);
    freep(&mut lctx.last_pat_payload);

    for pid_buffer in lctx.pid_buffers.iter_mut() {
        if !pid_buffer.is_null() {
            freep(&mut (**pid_buffer).buffer);
            freep(pid_buffer);
        }
    }

    for pid_prog in lctx.pids_programs.iter_mut() {
        freep(pid_prog);
    }

    freep(&mut lctx.filebuffer);
    let _ = Box::from_raw(*ctx);
    *ctx = null_mut();
}

#[cfg(test)]
mod tests {
    use super::*;
    use lib_ccxr::util::log::{
        set_logger, CCExtractorLogger, DebugMessageFlag, DebugMessageMask, OutputTarget,
    };
    use serial_test::serial;
    use std::fs::OpenOptions;
    use std::io::Write;
    use std::os::fd::AsRawFd;
    use std::slice;
    use std::sync::Once;
    use tempfile::NamedTempFile;
    use lib_ccxr::util::write_string_into_pointer;
    use crate::common::CType;
    use crate::hlist::list_add;
    #[no_mangle]
    pub unsafe extern "C" fn print_file_report(ctx: *mut lib_ccx_ctx) {}


    static INIT: Once = Once::new();

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
    fn test_y_n() {
        assert_eq!(y_n(0), "NO");
        assert_eq!(y_n(1), "YES");
    }

    #[test]
    fn test_get_desc_placeholder() {
        unsafe {
            let desc = get_desc_placeholder(0);
            let desc_str = CStr::from_ptr(desc).to_str().unwrap();
            assert_eq!(desc_str, "Unknown");
        }
    }
    #[test]
    fn test_default_ccx_demuxer() {
        let demuxer = CcxDemuxer::default();
        assert_eq!(demuxer.m2ts, 0);
        assert_eq!(demuxer.stream_mode, StreamMode::default());
        assert_eq!(demuxer.auto_stream, StreamMode::default());
        assert_eq!(demuxer.startbytes_pos, 0);
        assert_eq!(demuxer.startbytes_avail, 0);
        assert_eq!(demuxer.ts_autoprogram, false);
        assert_eq!(demuxer.ts_allprogram, false);
        assert_eq!(demuxer.flag_ts_forced_pn, false);
        assert_eq!(demuxer.flag_ts_forced_cappid, false);
        assert_eq!(demuxer.ts_datastreamtype, StreamType::Unknownstream);
        assert_eq!(demuxer.pinfo.len(), MAX_PROGRAM);
        assert_eq!(demuxer.nb_program, 0);
        assert_eq!(demuxer.codec, Codec::Dvb);
        assert_eq!(demuxer.nocodec, Codec::Dvb);
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
        assert_eq!(demuxer.hauppauge_warning_shown, false);
        assert_eq!(demuxer.multi_stream_per_prog, 0);
        assert_eq!(demuxer.last_pat_payload, null_mut());
        assert_eq!(demuxer.last_pat_length, 0);
        assert_eq!(demuxer.filebuffer, null_mut());
        assert_eq!(demuxer.filebuffer_start, 0);
        assert_eq!(demuxer.filebuffer_pos, 0);
        assert_eq!(demuxer.bytesinbuffer, 0);
        assert_eq!(demuxer.warning_program_not_found_shown, false);
        assert_eq!(demuxer.strangeheader, 0);
        #[cfg(feature = "enable_ffmpeg")]
        assert_eq!(demuxer.ffmpeg_ctx, null_mut());
        assert_eq!(demuxer.private_data, null_mut());
    }

    // Tests for is_decoder_processed_enough
    fn new_lib_cc_decode(processed_enough: i32) -> Box<lib_cc_decode> {
        Box::new(lib_cc_decode {
            processed_enough,
            list: list_head {
                next: null_mut(),
                prev: null_mut(),
            },
            ..Default::default()
        })
    }

    // Helper to build a circular linked list from a vector of LibCcDecode nodes.
    // Returns a Vec of Box<LibCcDecode> (to keep ownership) and sets up the links.
    fn build_decoder_list(nodes: &mut [Box<lib_cc_decode>]) -> *mut list_head {
        if nodes.is_empty() {
            return null_mut();
        }
        // Let head be the address of dec_ctx_head in LibCcxCtx.
        // For simplicity, we simulate this by using the list field of the first node as head.
        let head = &mut nodes[0].list as *mut list_head;
        // Link all nodes in a circular doubly linked list.
        for i in 0..nodes.len() {
            let next_i = (i + 1) % nodes.len();
            let prev_i = if i == 0 { nodes.len() - 1 } else { i - 1 };
            nodes[i].list.next = &mut nodes[next_i].list;
            nodes[i].list.prev = &mut nodes[prev_i].list;
        }
        head
    }

    // --- Tests for is_decoder_processed_enough ---

    #[test]
    fn test_is_decoder_processed_enough_true() {
        // multiprogram == false, and one of the decoders has processed_enough true.
        let mut decoders: Vec<Box<lib_cc_decode>> = vec![
            new_lib_cc_decode(0),
            new_lib_cc_decode(1),
            new_lib_cc_decode(0),
        ];
        let head = build_decoder_list(&mut decoders);
        let mut ctx = lib_ccx_ctx::default();
        ctx.dec_ctx_head.next = head;
        ctx.dec_ctx_head.prev = head;
        ctx.multiprogram = 0;
        // Manually set the list pointers in ctx to our head.
        // Now call the function.
        let result = unsafe { is_decoder_processed_enough(&mut ctx as *mut lib_ccx_ctx) };
        assert_ne!(result, 0);
    }

    #[test]
    fn test_is_decoder_processed_enough_false_no_decoder() {
        // multiprogram == false, but no decoder has processed_enough true.
        let mut decoders: Vec<Box<lib_cc_decode>> = vec![new_lib_cc_decode(0), new_lib_cc_decode(0)];
        let head = build_decoder_list(&mut decoders);
        let mut ctx = lib_ccx_ctx::default();
        ctx.dec_ctx_head.next = head;
        ctx.dec_ctx_head.prev = head;
        ctx.multiprogram = 0;
        let result = unsafe { is_decoder_processed_enough(&mut ctx as *mut lib_ccx_ctx) };
        assert_eq!(result, 0);
    }

    #[test]
    fn test_is_decoder_processed_enough_false_multiprogram() {
        // Even if a decoder is processed enough, if multiprogram is true, should return false.
        let mut decoders: Vec<Box<lib_cc_decode>> = vec![new_lib_cc_decode(1)];
        let head = build_decoder_list(&mut decoders);
        let mut ctx = lib_ccx_ctx::default();
        ctx.dec_ctx_head.next = head;
        ctx.dec_ctx_head.prev = head;
        ctx.multiprogram = 1;
        let result = unsafe { is_decoder_processed_enough(&mut ctx as *mut lib_ccx_ctx) };
        assert_eq!(result, 0);
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

    // Helper: build a circular linked list for CapInfo nodes using the sib_stream field.
    // The program's sib_head acts as the dummy head.
    fn build_capinfo_list(program: &mut CapInfo, nodes: &mut [Box<CapInfo>]) {
        if nodes.is_empty() {
            let head_ptr = program as *mut CapInfo as *mut list_head;
            program.sib_head.next = head_ptr;
            program.sib_head.prev = head_ptr;
            return;
        }
        let head_ptr = program as *mut CapInfo as *mut list_head;
        program.sib_head.next = &mut nodes[0].sib_stream as *mut list_head;
        program.sib_head.prev = &mut nodes[nodes.len() - 1].sib_stream as *mut list_head;
        for i in 0..nodes.len() {
            let next_ptr = if i + 1 < nodes.len() {
                &mut nodes[i + 1].sib_stream as *mut list_head
            } else {
                head_ptr
            };
            let prev_ptr = if i == 0 {
                head_ptr
            } else {
                &mut nodes[i - 1].sib_stream as *mut list_head
            };
            nodes[i].sib_stream.next = next_ptr;
            nodes[i].sib_stream.prev = prev_ptr;
        }
    }

    #[test]
    fn test_get_sib_stream_by_type_found() {
        let mut program = CapInfo::default();
        let sib1 = Box::new(CapInfo {
            codec: Codec::Dvb,
            ..Default::default()
        });
        let sib2 = Box::new(CapInfo {
            codec: Codec::Teletext,
            ..Default::default()
        });
        let mut siblings = vec![sib1, sib2];
        build_capinfo_list(&mut program, &mut siblings);
        let result =
            unsafe { get_sib_stream_by_type(&mut program as *mut CapInfo, Codec::Teletext) };
        assert!(!result.is_null());
        unsafe {
            assert_eq!((*result).codec, Codec::Teletext);
        }
    }

    #[test]
    fn test_get_sib_stream_by_type_not_found() {
        let mut program = CapInfo::default();
        let sib1 = Box::new(CapInfo {
            codec: Codec::Dvb,
            ..Default::default()
        });
        let mut siblings = vec![sib1];
        build_capinfo_list(&mut program, &mut siblings);
        let result =
            unsafe { get_sib_stream_by_type(&mut program as *mut CapInfo, Codec::Teletext) };
        assert!(result.is_null());
    }

    #[test]
    fn test_get_sib_stream_by_type_null_program() {
        let result = unsafe { get_sib_stream_by_type(null_mut(), Codec::Dvb) };
        assert!(result.is_null());
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

    //Tests for get_best_sib_stream
    #[test]
    fn test_get_best_sib_stream_teletext() {
        let mut program = CapInfo::default();
        let sib1 = Box::new(CapInfo {
            codec: Codec::Teletext,
            ..Default::default()
        });
        let mut siblings = vec![sib1];
        build_capinfo_list(&mut program, &mut siblings);
        let result = unsafe { get_best_sib_stream(&mut program as *mut CapInfo) };
        assert!(!result.is_null());
        unsafe {
            assert_eq!((*result).codec, Codec::Teletext);
        }
    }
    #[test]
    fn test_get_best_sib_stream_dvb() {
        let mut program = CapInfo::default();
        let sib1 = Box::new(CapInfo {
            codec: Codec::Dvb,
            ..Default::default()
        });
        let mut siblings = vec![sib1];
        build_capinfo_list(&mut program, &mut siblings);
        let result = unsafe { get_best_sib_stream(&mut program as *mut CapInfo) };
        assert!(!result.is_null());
        unsafe {
            assert_eq!((*result).codec, Codec::Dvb);
        }
    }
    #[test]
    fn test_get_best_sib_stream_atsc() {
        let mut program = CapInfo::default();
        let sib1 = Box::new(CapInfo {
            codec: Codec::AtscCc,
            ..Default::default()
        });
        let mut siblings = vec![sib1];
        build_capinfo_list(&mut program, &mut siblings);
        let result = unsafe { get_best_sib_stream(&mut program as *mut CapInfo) };
        assert!(!result.is_null());
        unsafe {
            assert_eq!((*result).codec, Codec::AtscCc);
        }
    }
    #[test]
    fn test_get_best_sib_stream_null() {
        let result = unsafe { get_best_sib_stream(null_mut()) };
        assert!(result.is_null());
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
            // Other fields are default.
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
        let buf = vec![1u8; FILEBUFFERSIZE].into_boxed_slice();
        ctx.filebuffer = Box::into_raw(buf) as *mut u8;
        ctx.bytesinbuffer = 123;
        let res = init_file_buffer(&mut ctx);
        assert_eq!(res, 0);
        assert!(!ctx.filebuffer.is_null());
        // bytesinbuffer remains unchanged.
        assert_eq!(ctx.bytesinbuffer, 123);
        // Clean up.
        unsafe {
            let _ = Box::from_raw(slice::from_raw_parts_mut(ctx.filebuffer, FILEBUFFERSIZE));
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
            assert_eq!(demuxer.open(file_path), 0);
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
            assert_eq!(demuxer.open("non_existent_file.txt"), -1);
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
            assert_eq!(demuxer.open(file_path), 0);
            demuxer.close(&mut Options::default());
            assert_eq!(demuxer.open(file_path), 0);
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
            assert_eq!(demuxer.open(file_path), 0);
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
        let result = unsafe { demuxer.open(file_name) };
        assert_eq!(result, 0);
        assert_eq!(demuxer.infd, 3);
    }

    /// Helper: Create a temporary file with the given content and return its file descriptor.
    fn create_temp_file_with_content(content: &[u8]) -> (NamedTempFile, i32, u64) {
        let mut tmpfile = NamedTempFile::new().expect("Failed to create temp file");
        tmpfile.write_all(content).expect("Failed to write content");
        let metadata = tmpfile
            .as_file()
            .metadata()
            .expect("Failed to get metadata");
        let size = metadata.len();

        // Get the raw file descriptor.
        let fd = tmpfile.as_file().as_raw_fd();
        (tmpfile, fd, size)
    }

    /// Test that ccx_demuxer_get_file_size returns the correct file size for a valid file.
    #[test]
    #[serial]
    fn test_get_file_size_valid() {
        let content = b"Hello, world!";
        let (_tmpfile, fd, size) = create_temp_file_with_content(content);

        // Create a default demuxer and modify the infd field.
        let mut demuxer = CcxDemuxer::default();
        demuxer.infd = fd;

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

        // Open the file (using OpenOptions to allow seeking).
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
        demuxer.infd = fd;

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
    // Tests for ccx_demuxer_get_stream_mode
    #[test]
    fn test_ccx_demuxer_get_stream_mode() {
        let mut demuxer = CcxDemuxer::default();
        demuxer.stream_mode = StreamMode::ElementaryOrNotFound;
        let result = ccx_demuxer_get_stream_mode(&demuxer);
        assert_eq!(result, StreamMode::ElementaryOrNotFound as i32);
    }

    // Tests for ccx_demuxer_print_cfg
    #[test]
    fn test_ccx_demuxer_print_cfg() {
        initialize_logger();
        let mut demuxer = CcxDemuxer::default();
        demuxer.auto_stream = StreamMode::ElementaryOrNotFound;
        ccx_demuxer_print_cfg(&demuxer);
    }
    #[test]
    fn test_ccx_demuxer_print_cfg_transport() {
        initialize_logger();
        let mut demuxer = CcxDemuxer::default();
        demuxer.auto_stream = StreamMode::Transport;
        ccx_demuxer_print_cfg(&demuxer);
    }

    // Tests for print_file_report
    /// Helper function to create a LibCcxCtx with a dummy demuxer.
    unsafe fn create_dummy_ctx() -> lib_ccx_ctx{
        let mut ctx = lib_ccx_ctx::default();
        // For testing file input, set current_file and inputfile.
        ctx.current_file = 0;
        ctx.num_input_files = 1;
        write_string_into_pointer(*ctx.inputfile, "dummy_file.txt");
        // Set a dummy program in the demuxer.
        unsafe {
            (*ctx.demux_ctx).nb_program = 1;
            let mut pinfo = ProgramInfo::default();
            pinfo.program_number = 101;
            let pinfo_c = pinfo.to_ctype();
            (*ctx.demux_ctx).pinfo = [pinfo_c; 128];
            (*ctx.demux_ctx).freport.program_cnt = 1;
            // For testing, leave pids_programs as all null.
        }
        ctx
    }

    /// Helper to set global options.
    fn set_global_options(ds: DataSource) {
        let mut opts = CCX_OPTIONS.lock().unwrap();
        opts.input_source = ds;
    }



    fn create_test_capinfo() -> *mut CapInfo {
        Box::into_raw(Box::new(CapInfo {
            pid: 123,
            program_number: 1,
            stream: StreamType::default(),
            codec: Codec::AtscCc,
            capbuf: Box::into_raw(Box::new(0u8)),
            capbufsize: 1024,
            capbuflen: 0,
            saw_pesstart: 0,
            prev_counter: 0,
            codec_private_data: null_mut(),
            ignore: 0,
            all_stream: list_head {
                next: null_mut(),
                prev: null_mut(),
            },
            sib_head: list_head {
                next: null_mut(),
                prev: null_mut(),
            },
            sib_stream: list_head {
                next: null_mut(),
                prev: null_mut(),
            },
            pg_stream: list_head {
                next: null_mut(),
                prev: null_mut(),
            },
        }))
    }

    #[test]
    fn test_dinit_cap_safety() {
        let mut ctx = Box::new(CcxDemuxer {
            cinfo_tree: CapInfo {
                pid: 0,
                program_number: 0,
                stream: StreamType::default(),
                codec: Codec::Dvb,
                capbufsize: 0,
                capbuf: null_mut(),
                capbuflen: 0,
                saw_pesstart: 0,
                prev_counter: 0,
                codec_private_data: null_mut(),
                ignore: 0,
                all_stream: list_head {
                    next: null_mut(),
                    prev: null_mut(),
                },
                sib_head: list_head {
                    next: null_mut(),
                    prev: null_mut(),
                },
                sib_stream: list_head {
                    next: null_mut(),
                    prev: null_mut(),
                },
                pg_stream: list_head {
                    next: null_mut(),
                    prev: null_mut(),
                },
            },
            ..Default::default()
        });

        // Properly initialize list heads
        init_list_head(&mut ctx.cinfo_tree.all_stream);
        init_list_head(&mut ctx.cinfo_tree.sib_stream);
        init_list_head(&mut ctx.cinfo_tree.pg_stream);

        unsafe {
            // Add test entries
            let cap1 = create_test_capinfo();
            list_add(&mut (*cap1).all_stream, &mut ctx.cinfo_tree.all_stream);

            let cap2 = create_test_capinfo();
            list_add(&mut (*cap2).all_stream, &mut ctx.cinfo_tree.all_stream);

            // Convert to raw pointer for demuxer
            let mut ctx_ptr = Box::into_raw(ctx);

            dinit_cap(&mut *ctx_ptr);

            // Verify cleanup
            assert!(list_empty(&(*ctx_ptr).cinfo_tree.all_stream));
            assert_eq!(
                (*ctx_ptr).cinfo_tree.all_stream.next,
                &mut (*ctx_ptr).cinfo_tree.all_stream as *mut list_head
            );

            // Cleanup context
            let _ = Box::from_raw(ctx_ptr);
        }
    }
}
