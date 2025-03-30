use crate::common::{Decoder608Report, DecoderDtvccReport, OutputFormat};
use crate::demuxer::demux::{CcxDemuxer, DecodersCommonSettings, EITProgram, HList, PSI_buffer};

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
#[derive(Debug)]
#[repr(C)]
pub struct LibCcxCtx<'a> {
    // Common data for both loops
    pub pesheaderbuf: *mut u8, // unsigned char* -> raw pointer
    pub inputsize: i64,        // LLONG -> i64
    pub total_inputsize: i64,
    pub total_past: i64, // Only in binary concat mode

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
    pub dec_ctx_head: HList,                             // Linked list head

    pub rawmode: i32,                    // Broadcast or DVD mode
    pub cc_to_stdout: i32,               // Output captions to stdout
    pub pes_header_to_stdout: i32,       // Output PES Header data to console
    pub dvb_debug_traces_to_stdout: i32, // Output DVB subtitle debug traces
    pub ignore_pts_jumps: i32,           // Ignore PTS jumps for DVB subtitles

    pub subs_delay: i64, // Delay (or advance) subtitles in ms

    pub startcredits_displayed: i32,
    pub end_credits_displayed: i32,
    pub last_displayed_subs_ms: i64, // Timestamp of last displayed subtitles
    pub screens_to_process: i64,     // Number of screenfuls to process
    pub basefilename: *mut u8,       // char* -> raw pointer for input filename without extension

    pub extension: *const u8, // const char* -> immutable raw pointer for output extension
    pub current_file: i32,    // Tracks the current file being processed

    pub inputfile: Vec<String>, // char** -> double raw pointer for file list
    pub num_input_files: i32,   // Number of input files

    pub teletext_warning_shown: u32, // Flag for PAL teletext warning

    pub epg_inited: i32,
    pub epg_buffers: *mut PSI_buffer,  // Pointer to PSI buffers
    pub eit_programs: *mut EITProgram, // Pointer to EIT programs
    pub eit_current_events: *mut i32,  // Pointer to current EIT events
    pub atsc_source_pg_map: *mut i16,  // Pointer to ATSC source program map
    pub epg_last_output: i32,
    pub epg_last_live_output: i32,
    pub freport: FileReport, // File report struct

    pub hauppauge_mode: u32,        // Special handling mode for Hauppauge
    pub live_stream: i32, // -1 = live stream without timeout, 0 = file, >0 = live stream with timeout
    pub binary_concat: i32, // Disabled by -ve or --videoedited
    pub multiprogram: i32, // Multi-program support
    pub write_format: OutputFormat, // Output format

    pub demux_ctx: *mut CcxDemuxer<'a>, // Pointer to demux context
    pub enc_ctx_head: HList,            // Linked list for encoding contexts
    pub mp4_cfg: Mp4Cfg,                // MP4 configuration struct
    pub out_interval: i32,              // Output interval
    pub segment_on_key_frames_only: i32, // Segment only on keyframes
    pub segment_counter: i32,           // Segment counter
    pub system_start_time: i64,         // System start time
}
impl<'a> LibCcxCtx<'a> {
    pub(crate) fn default() -> Self {
        LibCcxCtx {
            pesheaderbuf: Box::into_raw(Box::new(0u8)),
            inputsize: 0,
            total_inputsize: 0,
            total_past: 0,
            last_reported_progress: 0,
            stat_numuserheaders: 0,
            stat_dvdccheaders: 0,
            stat_scte20ccheaders: 0,
            stat_replay5000headers: 0,
            stat_replay4000headers: 0,
            stat_dishheaders: 0,
            stat_hdtv: 0,
            stat_divicom: 0,
            false_pict_header: 0,
            dec_global_setting: Box::into_raw(Box::new(DecodersCommonSettings::default())),
            dec_ctx_head: HList::default(),
            rawmode: 0,
            cc_to_stdout: 0,
            pes_header_to_stdout: 0,
            dvb_debug_traces_to_stdout: 0,
            ignore_pts_jumps: 0,
            subs_delay: 0,
            startcredits_displayed: 0,
            end_credits_displayed: 0,
            last_displayed_subs_ms: 0,
            screens_to_process: 0,
            basefilename: Box::into_raw(Box::new(0u8)),
            extension: Box::into_raw(Box::new(0u8)),
            current_file: -1,
            inputfile: Vec::new(),
            num_input_files: 0,
            teletext_warning_shown: 0,
            epg_inited: 0,
            epg_buffers: Box::into_raw(Box::new(PSI_buffer::default())),
            eit_programs: std::ptr::null_mut(),
            // eit_programs: Box::into_raw(Box::new(EITProgram::default())),
            eit_current_events: Box::into_raw(Box::new(0)),
            atsc_source_pg_map: Box::into_raw(Box::new(0)),
            epg_last_output: 0,
            epg_last_live_output: 0,
            freport: FileReport {
                width: 0,
                height: 0,
                aspect_ratio: 0,
                frame_rate: 0,
                data_from_608: Box::into_raw(Box::new(Decoder608Report::default())),
                data_from_708: Box::into_raw(Box::new(DecoderDtvccReport::default())),
                mp4_cc_track_cnt: 0,
            },
            hauppauge_mode: 0,
            live_stream: 0,
            binary_concat: 0,
            multiprogram: 0,
            write_format: OutputFormat::default(),
            demux_ctx: Box::into_raw(Box::new(CcxDemuxer::default())),
            enc_ctx_head: HList::default(),
            mp4_cfg: Mp4Cfg { mp4vidtrack: 0 },
            out_interval: 0,
            segment_on_key_frames_only: 0,
            segment_counter: 0,
            system_start_time: 0,
        }
    }
}

impl<'a> Default for LibCcxCtx<'a> {
    fn default() -> Self {
        Self::default()
    }
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct Mp4Cfg {
    pub mp4vidtrack: u32, // unsigned int :1 (bitfield) -> represented as a u32
}
