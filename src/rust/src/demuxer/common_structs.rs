use crate::bindings::{lib_ccx_ctx, list_head};
use lib_ccxr::common::{Codec, Decoder608Report, DecoderDtvccReport, StreamMode, StreamType};
use lib_ccxr::time::Timestamp;
use std::ptr::null_mut;

// Size of the Startbytes Array in CcxDemuxer - const 1MB
pub(crate) const ARRAY_SIZE: usize = 1024 * 1024;

// Constants for Report Information
pub const SUB_STREAMS_CNT: usize = 10;
pub const MAX_PID: usize = 65536;
pub const MAX_PSI_PID: usize = 8191;
pub const MAX_PROGRAM: usize = 128;
pub const MAX_PROGRAM_NAME_LEN: usize = 128;
pub const STARTBYTESLENGTH: usize = 1024 * 1024;

#[repr(u32)]
pub enum Stream_Type {
    PrivateStream1 = 0,
    Audio,
    Video,
    Count,
}
#[derive(Debug)]
pub struct CcxStreamMp4Box {
    pub box_type: [u8; 4],
    pub score: i32,
}
#[derive(Clone)]
pub struct CcxDemuxReport {
    pub program_cnt: u32,
    pub dvb_sub_pid: [u32; SUB_STREAMS_CNT],
    pub tlt_sub_pid: [u32; SUB_STREAMS_CNT],
    pub mp4_cc_track_cnt: u32,
}
#[derive(Debug)]
pub struct FileReport {
    pub width: u32,
    pub height: u32,
    pub aspect_ratio: u32,
    pub frame_rate: u32,
    pub data_from_608: Option<Decoder608Report>,
    pub data_from_708: Option<DecoderDtvccReport>,
    pub mp4_cc_track_cnt: u32,
}
// program_info Struct
#[derive(Copy, Clone)]
pub struct ProgramInfo {
    pub pid: i32,
    pub program_number: i32,
    pub initialized_ocr: bool, // Avoid initializing the OCR more than once
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
    pub has_all_min_pts: bool,
}

// cap_info Struct
#[derive(Clone)]
pub struct CapInfo {
    pub pid: i32,
    pub program_number: i32,
    pub stream: StreamType,
    pub codec: Codec,
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

pub struct PMTEntry {
    pub program_number: u32,
    pub elementary_pid: u32,
    pub stream_type: StreamType,
    pub printable_stream_type: u32,
}
impl Default for PMTEntry {
    fn default() -> Self {
        PMTEntry {
            program_number: 0,
            elementary_pid: 0,
            stream_type: StreamType::Unknownstream,
            printable_stream_type: 0,
        }
    }
}

pub struct CcxDemuxer<'a> {
    pub m2ts: i32,
    pub stream_mode: StreamMode,
    pub auto_stream: StreamMode,
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
    pub codec: Codec,
    pub nocodec: Codec,

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
    pub private_data: *mut std::ffi::c_void, // this could point at large variety of contexts, it's a raw pointer now but after all modules implemented we make it an Option<>
    #[cfg(feature = "enable_ffmpeg")]
    pub ffmpeg_ctx: *mut std::ffi::c_void,
}

impl Default for CcxDemuxer<'_> {
    fn default() -> Self {
        // 1) Initialize pinfo exactly as C’s init loop does
        let mut pinfo_vec = Vec::with_capacity(MAX_PROGRAM);
        for _ in 0..MAX_PROGRAM {
            let mut pi = ProgramInfo {
                has_all_min_pts: false,
                ..Default::default()
            };
            for j in 0..(Stream_Type::Count as usize) {
                pi.got_important_streams_min_pts[j] = u64::MAX;
            }
            pi.initialized_ocr = false;
            pi.version = 0xFF; // “not initialized” marker
                               // pid and program_number remain zero for now
            pinfo_vec.push(pi);
        }

        // 2) Build and return the full struct, matching C’s init_demuxer(cfg=zero, parent=NULL)
        CcxDemuxer {
            // (a) File handle fields
            infd: -1, // C: ctx->infd = -1
            past: 0,  // C does not set past here (init_ts might), so zero is fine

            // (b) TS‐specific fields from “cfg = zero” case
            m2ts: 0, // C: ctx->m2ts = cfg->m2ts (cfg->m2ts == 0)
            auto_stream: StreamMode::ElementaryOrNotFound,
            stream_mode: StreamMode::ElementaryOrNotFound,
            ts_autoprogram: false,    // C: cfg->ts_autoprogram == 0
            ts_allprogram: false,     // C: cfg->ts_allprogram == 0
            flag_ts_forced_pn: false, // C sets this only if cfg->ts_forced_program != -1
            ts_datastreamtype: StreamType::Unknownstream,

            // (c) Program info
            pinfo: pinfo_vec,
            nb_program: 0, // C: starts at 0 (no forced program)

            // (d) Codec fields
            codec: Codec::Any,            // C: cfg->codec == CCX_CODEC_ANY (zero)
            flag_ts_forced_cappid: false, // no forced CA‐PID if cfg->nb_ts_cappid == 0
            nocodec: Codec::Any,          // C: cfg->nocodec == CCX_CODEC_ANY

            // (e) Capability‐info tree
            cinfo_tree: CapInfo::default(), // C: INIT_LIST_HEAD; with no capids, the tree is empty

            // (f) Start‐bytes buffer
            startbytes: vec![0; STARTBYTESLENGTH],
            startbytes_pos: 0,
            startbytes_avail: 0,

            // (g) Global timestamps
            global_timestamp: Timestamp::from_millis(0),
            min_global_timestamp: Timestamp::from_millis(0),
            offset_global_timestamp: Timestamp::from_millis(0),
            last_global_timestamp: Timestamp::from_millis(0),
            global_timestamp_inited: Timestamp::from_millis(0),

            // (h) PID buffers
            pid_buffers: vec![null_mut(); MAX_PSI_PID],

            // (i) Arrays that init_ts would set:
            pids_seen: vec![0; MAX_PID],
            stream_id_of_each_pid: vec![0; MAX_PSI_PID + 1],
            min_pts: {
                let mut v = vec![0u64; MAX_PSI_PID + 1];
                for item in v.iter_mut().take(MAX_PSI_PID + 1) {
                    *item = u64::MAX;
                }
                v
            },
            have_pids: vec![-1; MAX_PSI_PID + 1],
            num_of_pids: 0,
            pids_programs: vec![null_mut(); MAX_PID],

            // (j) Report fields
            freport: CcxDemuxReport::default(),

            // (k) Hauppauge and multi‐stream flags
            hauppauge_warning_shown: false,
            multi_stream_per_prog: 0,

            // (l) PAT tracking
            last_pat_payload: null_mut(),
            last_pat_length: 0,

            // (m) Filebuffer (init_ts would set to NULL/0)
            filebuffer: null_mut(),
            filebuffer_start: 0,
            filebuffer_pos: 0,
            bytesinbuffer: 0,

            // (n) Warnings and headers
            warning_program_not_found_shown: false,
            strangeheader: 0,

            // (o) Parent & private data
            parent: None,
            private_data: null_mut(),

            #[cfg(feature = "enable_ffmpeg")]
            ffmpeg_ctx: null_mut(),
        }
    }
}
impl Default for ProgramInfo {
    fn default() -> Self {
        ProgramInfo {
            pid: -1,
            program_number: 0,
            initialized_ocr: false,
            analysed_pmt_once: 0,
            version: 0,
            saved_section: [0; 1021],
            crc: 0,
            valid_crc: 0,
            name: [0; MAX_PROGRAM_NAME_LEN],
            pcr_pid: -1,
            got_important_streams_min_pts: [0; Stream_Type::Count as usize],
            has_all_min_pts: false,
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

            all_stream: list_head::default(),
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
            dvb_sub_pid: [0; SUB_STREAMS_CNT],
            tlt_sub_pid: [0; SUB_STREAMS_CNT],
            mp4_cc_track_cnt: 0,
        }
    }
}

#[derive(Default, Debug, Clone, Copy, Eq, PartialEq)]
pub struct CcxRational {
    pub num: i32,
    pub den: i32,
}

