use lib_ccxr::common::{Decoder608Report, DecoderDtvccReport, OutputFormat};
use crate::demuxer::demuxer::{CcxDemuxer, DecodersCommonSettings, EITProgram, HList, PSI_buffer};

#[repr(C)]
pub struct FileReport {
    pub width: u32,
    pub height: u32,
    pub aspect_ratio: u32,
    pub frame_rate: u32,
    pub data_from_608: *mut Decoder608Report,  // Pointer to Decoder608Report
    pub data_from_708: *mut DecoderDtvccReport, // Pointer to DecoderDtvccReport
    pub mp4_cc_track_cnt: u32,
}

#[repr(C)]
pub struct LibCcxCtx {
    // Common data for both loops
    pub pesheaderbuf: *mut u8, // unsigned char* -> raw pointer
    pub inputsize: i64,        // LLONG -> i64
    pub total_inputsize: i64,
    pub total_past: i64,       // Only in binary concat mode

    pub last_reported_progress: i32,

    /* Stats */
    pub stat_numuserheaders: i32,
    pub stat_dvdccheaders: i32,
    pub stat_scte20ccheaders: i32,
    pub stat_replay5000headers: i32,
    pub stat_replay4000headers: i32,
    pub stat_dishheaders: i32,
    pub stat_hdtv: i32,
    pub stat_divicom: i32,
    pub false_pict_header: i32,

    pub dec_global_setting: *mut DecodersCommonSettings, // Pointer to global decoder settings
    pub dec_ctx_head: HList, // Linked list head

    pub rawmode: i32, // Broadcast or DVD mode
    pub cc_to_stdout: i32, // Output captions to stdout
    pub pes_header_to_stdout: i32, // Output PES Header data to console
    pub dvb_debug_traces_to_stdout: i32, // Output DVB subtitle debug traces
    pub ignore_pts_jumps: i32, // Ignore PTS jumps for DVB subtitles

    pub subs_delay: i64, // Delay (or advance) subtitles in ms

    pub startcredits_displayed: i32,
    pub end_credits_displayed: i32,
    pub last_displayed_subs_ms: i64, // Timestamp of last displayed subtitles
    pub screens_to_process: i64, // Number of screenfuls to process
    pub basefilename: *mut u8, // char* -> raw pointer for input filename without extension

    pub extension: *const u8, // const char* -> immutable raw pointer for output extension
    pub current_file: i32, // Tracks the current file being processed

    pub inputfile: Vec<String>, // char** -> double raw pointer for file list
    pub num_input_files: i32, // Number of input files

    pub teletext_warning_shown: u32, // Flag for PAL teletext warning

    pub epg_inited: i32,
    pub epg_buffers: *mut PSI_buffer, // Pointer to PSI buffers
    pub eit_programs: *mut EITProgram, // Pointer to EIT programs
    pub eit_current_events: *mut i32, // Pointer to current EIT events
    pub atsc_source_pg_map: *mut i16, // Pointer to ATSC source program map
    pub epg_last_output: i32,
    pub epg_last_live_output: i32,
    pub freport: FileReport, // File report struct

    pub hauppauge_mode: u32, // Special handling mode for Hauppauge
    pub live_stream: i32, // -1 = live stream without timeout, 0 = file, >0 = live stream with timeout
    pub binary_concat: i32, // Disabled by -ve or --videoedited
    pub multiprogram: i32, // Multi-program support
    pub write_format: OutputFormat, // Output format

    pub demux_ctx: *mut CcxDemuxer, // Pointer to demux context
    pub enc_ctx_head: HList, // Linked list for encoding contexts
    pub mp4_cfg: Mp4Cfg, // MP4 configuration struct
    pub out_interval: i32, // Output interval
    pub segment_on_key_frames_only: i32, // Segment only on keyframes
    pub segment_counter: i32, // Segment counter
    pub system_start_time: i64, // System start time
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct Mp4Cfg {
    pub mp4vidtrack: u32, // unsigned int :1 (bitfield) -> represented as a u32
}
