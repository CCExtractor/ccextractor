use url::Url;

use std::path::PathBuf;

use crate::common::{
    DataSource, Language, OutputFormat, SelectCodec, StreamMode, StreamType, DTVCC_MAX_SERVICES,
};
use crate::hardsubx::{ColorHue, OcrMode};
use crate::time::units::{Timestamp, TimestampFormat};
use crate::util::encoding::Encoding;
use crate::util::log::{DebugMessageFlag, DebugMessageMask, OutputTarget};
use crate::util::time::stringztoms;

#[derive(Debug, Clone)]
pub struct EncodersTranscriptFormat {
    pub show_start_time: bool,
    pub show_end_time: bool,
    pub show_mode: bool,
    pub show_cc: bool,
    pub relative_timestamp: bool,
    pub xds: bool,
    pub use_colors: bool,
    pub is_final: bool,
}

impl Default for EncodersTranscriptFormat {
    fn default() -> Self {
        Self {
            show_start_time: false,
            show_end_time: false,
            show_mode: false,
            show_cc: false,
            relative_timestamp: true,
            xds: false,
            use_colors: true,
            is_final: false,
        }
    }
}

#[derive(Debug, Default, Copy, Clone)]
pub enum FrameType {
    #[default]
    ResetOrUnknown = 0,
    IFrame = 1,
    PFrame = 2,
    BFrame = 3,
    DFrame = 4,
}

#[derive(Debug, Default, Clone, Copy)]
pub struct CommonTimingCtx {
    pub pts_set: i32,          // 0 = No, 1 = received, 2 = min_pts set
    pub min_pts_adjusted: i32, // 0 = No, 1=Yes (don't adjust again)
    pub current_pts: i64,
    pub current_picture_coding_type: FrameType,
    pub current_tref: i32, // Store temporal reference of current frame
    pub min_pts: i64,
    pub max_pts: i64,
    pub sync_pts: i64,
    pub minimum_fts: i64,      // No screen should start before this FTS
    pub fts_now: i64,          // Time stamp of current file (w/ fts_offset, w/o fts_global)
    pub fts_offset: i64,       // Time before first sync_pts
    pub fts_fc_offset: i64,    // Time before first GOP
    pub fts_max: i64,          // Remember the maximum fts that we saw in current file
    pub fts_global: i64,       // Duration of previous files (-ve mode)
    pub sync_pts2fts_set: i32, // 0 = No, 1 = Yes
    pub sync_pts2fts_fts: i64,
    pub sync_pts2fts_pts: i64,
    pub pts_reset: i32, // 0 = No, 1 = Yes. PTS resets when current_pts is lower than prev
}

impl Default for DecoderDtvccReport {
    fn default() -> Self {
        Self {
            reset_count: 0,
            services: [0; DTVCC_MAX_SERVICES],
        }
    }
}

#[derive(Debug, Copy, Clone)]

pub struct DecoderDtvccReport {
    pub reset_count: i32,
    pub services: [u32; DTVCC_MAX_SERVICES],
}

impl Default for DecoderDtvccSettings {
    fn default() -> Self {
        Self {
            enabled: false,
            print_file_reports: true,
            no_rollup: false,
            report: None,
            active_services_count: 0,
            services_enabled: [false; DTVCC_MAX_SERVICES],
            timing: CommonTimingCtx::default(),
        }
    }
}

#[derive(Debug, Clone, Copy)]
pub struct DecoderDtvccSettings {
    pub enabled: bool,
    pub print_file_reports: bool,
    pub no_rollup: bool,
    pub report: Option<DecoderDtvccReport>,
    pub active_services_count: i32,
    pub services_enabled: [bool; DTVCC_MAX_SERVICES],
    pub timing: CommonTimingCtx,
}

#[derive(Debug, Copy, Clone)]
pub struct Decoder608Settings {
    pub direct_rollup: i32,
    pub force_rollup: i32,
    pub no_rollup: bool,
    pub default_color: Decoder608ColorCode,
    pub screens_to_process: i32,
    pub report: Option<Decoder608Report>,
}
impl Default for Decoder608Settings {
    fn default() -> Self {
        Self {
            direct_rollup: 0,
            force_rollup: 0,
            no_rollup: false,
            default_color: Decoder608ColorCode::Transparent,
            screens_to_process: -1,
            report: None,
        }
    }
}

#[derive(Debug, Default, Copy, Clone)]
pub struct Decoder608Report {
    pub xds: bool,
    pub cc_channels: [u8; 4],
}

impl Default for Decoder608ColorCode {
    fn default() -> Self {
        Self::Userdefined
    }
}

#[derive(Debug, Copy, Clone)]
pub enum Decoder608ColorCode {
    White = 0,
    Green = 1,
    Blue = 2,
    Cyan = 3,
    Red = 4,
    Yellow = 5,
    Magenta = 6,
    Userdefined = 7,
    Black = 8,
    Transparent = 9,
    Max,
}

#[derive(Debug, Default, PartialEq, Eq, Clone)]
pub enum DtvccServiceCharset {
    #[default]
    None,
    Same(String),
    Unique(Box<[String; DTVCC_MAX_SERVICES]>),
}

#[allow(dead_code)]
#[derive(Debug, Clone)]
pub struct DemuxerConfig {
    /// Regular TS or M2TS
    pub m2ts: bool,
    pub auto_stream: StreamMode,

    /* subtitle codec type */
    pub codec: SelectCodec,
    pub nocodec: SelectCodec,

    /// Try to find a stream with captions automatically (no -pn needed)
    pub ts_autoprogram: bool,
    pub ts_allprogram: bool,
    /// PID for stream that holds caption information
    pub ts_cappids: Vec<u32>,
    /// If 1, never mess with the selected PID
    pub ts_forced_cappid: bool,
    /// Specific program to process in TS files, if a forced program is given
    pub ts_forced_program: Option<i32>,
    /// User WANTED stream type (i.e. use the stream that has this type)
    pub ts_datastreamtype: StreamType,
    /// User selected (forced) stream type
    pub ts_forced_streamtype: StreamType,
}

impl Default for DemuxerConfig {
    fn default() -> Self {
        Self {
            m2ts: false,
            auto_stream: StreamMode::Autodetect,
            codec: SelectCodec::Some(super::Codec::Any),
            nocodec: SelectCodec::None,
            ts_autoprogram: false,
            ts_allprogram: false,
            ts_cappids: Vec::new(),
            ts_forced_cappid: false,
            ts_forced_program: None,
            ts_datastreamtype: StreamType::Unknownstream,
            ts_forced_streamtype: StreamType::Unknownstream,
        }
    }
}

impl Default for EncoderConfig {
    fn default() -> Self {
        Self {
            extract: 1,
            dtvcc_extract: false,
            gui_mode_reports: false,
            output_filename: String::default(),
            write_format: OutputFormat::default(),
            keep_output_closed: false,
            force_flush: false,
            append_mode: false,
            ucla: false,
            encoding: Encoding::default(),
            date_format: TimestampFormat::default(),
            autodash: false,
            trim_subs: false,
            sentence_cap: false,
            splitbysentence: false,
            curlposturl: None,
            filter_profanity: false,
            with_semaphore: false,
            start_credits_text: String::default(),
            end_credits_text: String::default(),
            startcreditsnotbefore: stringztoms("0").unwrap_or_default(),
            startcreditsnotafter: stringztoms("5:00").unwrap_or_default(),
            startcreditsforatleast: stringztoms("2").unwrap_or_default(),
            startcreditsforatmost: stringztoms("5").unwrap_or_default(),
            endcreditsforatleast: stringztoms("2").unwrap_or_default(),
            endcreditsforatmost: stringztoms("5").unwrap_or_default(),
            transcript_settings: EncodersTranscriptFormat::default(),
            send_to_srv: false,
            no_bom: true,
            first_input_file: String::default(),
            multiple_files: false,
            no_font_color: false,
            no_type_setting: false,
            cc_to_stdout: false,
            line_terminator_lf: false,
            subs_delay: Timestamp::default(),
            program_number: 0,
            in_format: 1,
            nospupngocr: false,
            force_dropframe: false,
            render_font: PathBuf::default(),
            render_font_italics: PathBuf::default(),
            services_enabled: [false; DTVCC_MAX_SERVICES],
            services_charsets: DtvccServiceCharset::None,
            extract_only_708: false,
        }
    }
}

#[allow(dead_code)]
#[derive(Debug, Clone)]
pub struct EncoderConfig {
    /// Extract 1st (1), 2nd (2) or both fields (12)
    pub extract: u8,
    pub dtvcc_extract: bool,
    // If true, output in stderr progress updates so the GUI can grab them
    pub gui_mode_reports: bool,
    pub output_filename: String,
    pub write_format: OutputFormat,
    pub keep_output_closed: bool,
    /// Force flush on content write
    pub force_flush: bool,
    /// Append mode for output files
    pub append_mode: bool,
    /// true if -UCLA used, false if not
    pub ucla: bool,

    pub encoding: Encoding,
    pub date_format: TimestampFormat,
    /// Add dashes (-) before each speaker automatically?
    pub autodash: bool,
    /// "    Remove spaces at sides?    "
    pub trim_subs: bool,
    /// FIX CASE? = Fix case?
    pub sentence_cap: bool,
    /// Split text into complete sentences and prorate time?
    pub splitbysentence: bool,

    /// If out=curl, where do we send the data to?
    #[cfg(feature = "with_libcurl")]
    pub curlposturl: Option<Url>,

    /// Censors profane words from subtitles
    pub filter_profanity: bool,

    /// Write a .sem file on file open and delete it on close?
    pub with_semaphore: bool,
    /* Credit stuff */
    pub start_credits_text: String,
    pub end_credits_text: String,
    pub startcreditsnotbefore: Timestamp, // Where to insert start credits, if possible
    pub startcreditsnotafter: Timestamp,
    pub startcreditsforatleast: Timestamp, // How long to display them?
    pub startcreditsforatmost: Timestamp,
    pub endcreditsforatleast: Timestamp,
    pub endcreditsforatmost: Timestamp,

    /// Keeps the settings for generating transcript output files.
    pub transcript_settings: EncodersTranscriptFormat,
    pub send_to_srv: bool,
    /// Set to true when no BOM (Byte Order Mark) should be used for files.
    /// Note, this might make files unreadable in windows!
    pub no_bom: bool,
    pub first_input_file: String,
    pub multiple_files: bool,
    pub no_font_color: bool,
    pub no_type_setting: bool,
    /// If this is set to true, the stdout will be flushed when data was written to the screen during a process_608 call.
    pub cc_to_stdout: bool,
    /// false = CRLF, true = LF
    pub line_terminator_lf: bool,
    /// ms to delay (or advance) subs
    pub subs_delay: Timestamp,
    pub program_number: u32,
    pub in_format: u8,
    // true if we don't want to OCR bitmaps to add the text as comments in the XML file in spupng
    pub nospupngocr: bool,

    // MCC File
    /// true if dropframe frame count should be used. defaults to no drop frame.
    pub force_dropframe: bool,

    // text -> png (text render)
    /// The font used to render text if needed (e.g. teletext->spupng)
    pub render_font: PathBuf,
    pub render_font_italics: PathBuf,

    //CEA-708
    pub services_enabled: [bool; DTVCC_MAX_SERVICES],
    pub services_charsets: DtvccServiceCharset,
    // true if only 708 subs extraction is enabled
    pub extract_only_708: bool,
}

/// Options from user parameters
#[derive(Debug, Clone)]
pub struct Options {
    /// Extract 1st, 2nd or both fields. Can be 1, 2 or 12 respectively.
    pub extract: u8,
    /// Disable roll-up emulation (no duplicate output in generated file)
    pub no_rollup: bool,
    pub noscte20: bool,
    pub webvtt_create_css: bool,
    /// Channel we want to dump in srt mode
    pub cc_channel: u8,
    pub buffer_input: bool,
    pub nofontcolor: bool,
    pub nohtmlescape: bool,
    pub notypesetting: bool,
    /// The start of the segment we actually process
    pub extraction_start: Option<Timestamp>,
    /// The end of the segment we actually process
    pub extraction_end: Option<Timestamp>,
    pub print_file_reports: bool,
    /// Contains the settings for the 608 decoder.
    pub settings_608: Decoder608Settings,
    /// Same for 708 decoder
    pub settings_dtvcc: DecoderDtvccSettings,
    /// Is 608 enabled by explicitly using flags(--output-field 1 / 2 / both)
    pub is_608_enabled: bool,
    /// Is 708 enabled by explicitly using flags(--svc)
    pub is_708_enabled: bool,

    /// Disabled by -ve or --videoedited
    pub binary_concat: bool,
    /// Use GOP instead of PTS timing (None=do as needed, true=always, false=never)
    pub use_gop_as_pts: Option<bool>,
    /// Replace 0000 with 8080 in HDTV (needed for some cards)
    pub fix_padding: bool,
    /// If true, output in stderr progress updates so the GUI can grab them
    pub gui_mode_reports: bool,
    /// If true, suppress the output of the progress to stdout
    pub no_progress_bar: bool,
    /// Extra capitalization word file
    pub sentence_cap_file: PathBuf,
    /// None -> Not a complete file but a live stream, without timeout
    ///
    /// 0 -> A regular file
    ///
    /// \> 0 -> Live stream with a timeout of this value in seconds
    pub live_stream: Option<Timestamp>,
    /// Extra profanity word file
    pub filter_profanity_file: PathBuf,
    pub messages_target: OutputTarget,
    /// If true, add WebVTT X-TIMESTAMP-MAP header
    pub timestamp_map: bool,
    /* Levenshtein's parameters, for string comparison */
    /// false => don't attempt to correct typos with this algorithm
    pub dolevdist: bool,
    /// Means 2 fails or less is "the same"
    pub levdistmincnt: u8,
    /// Means 10% or less is also "the same"
    pub levdistmaxpct: u8,
    /// Look for captions in all packets when everything else fails
    pub investigate_packets: bool,
    /// Disable pruning of padding cc blocks
    pub fullbin: bool,
    /// Disable syncing
    pub nosync: bool,
    /// If true, use PID=1003, process specially and so on
    pub hauppauge_mode: bool,
    /// Fix broken Windows 7 conversion
    pub wtvconvertfix: bool,
    pub wtvmpeg2: bool,
    /// Use myth-tv mpeg code? false=no, true=yes, None=auto
    pub auto_myth: Option<bool>,
    /* MP4 related stuff */
    /// Process the video track even if a CC dedicated track exists.
    pub mp4vidtrack: bool,
    /// If true, extracts chapters (if present), from MP4 files.
    pub extract_chapters: bool,
    /* General settings */
    /// Force the use of pic_order_cnt_lsb in AVC/H.264 data streams
    pub usepicorder: bool,
    /// 1 = full output. 2 = live output. 3 = both
    pub xmltv: u8,
    /// interval in seconds between writing xmltv output files in live mode
    pub xmltvliveinterval: Timestamp,
    /// interval in seconds between writing xmltv full file output
    pub xmltvoutputinterval: Timestamp,
    pub xmltvonlycurrent: bool,
    pub keep_output_closed: bool,
    /// Force flush on content write
    pub force_flush: bool,
    /// Append mode for output files
    pub append_mode: bool,
    /// true if UCLA used, false if not
    pub ucla: bool,
    /// true if ticker text style burned in subs, false if not
    pub tickertext: bool,
    /// true if burned-in subtitles to be extracted
    pub hardsubx: bool,
    /// true if both burned-in and not burned in need to be extracted
    pub hardsubx_and_common: bool,
    /// The name of the language stream for DVB
    pub dvblang: Option<Language>,
    /// The name of the .traineddata file to be loaded with tesseract
    pub ocrlang: PathBuf,
    /// The Tesseract OEM mode, could be 0 (default), 1 or 2
    pub ocr_oem: i8,
    /// The Tesseract PSM mode, could be between 0 and 13. 3 is tesseract default
    pub psm: i32,
    /// How to quantize the bitmap before passing to to tesseract
    /// (0 = no quantization at all, 1 = CCExtractor's internal,
    ///  2 = reduce distinct color count in image for faster results.)
    pub ocr_quantmode: u8,
    /// The name of the language stream for MKV
    pub mkvlang: Option<Language>,
    /// If true, the video stream will be processed even if we're using a different one for subtitles.
    pub analyze_video_stream: bool,

    /*HardsubX related stuff*/
    pub hardsubx_ocr_mode: OcrMode,
    pub hardsubx_min_sub_duration: Timestamp,
    pub hardsubx_detect_italics: bool,
    pub hardsubx_conf_thresh: f64,
    pub hardsubx_hue: ColorHue,
    pub hardsubx_lum_thresh: f64,

    /// Keeps the settings for generating transcript output files.
    pub transcript_settings: EncodersTranscriptFormat,
    pub date_format: TimestampFormat,
    pub send_to_srv: bool,
    pub write_format: OutputFormat,
    pub write_format_rewritten: bool,
    pub use_ass_instead_of_ssa: bool,
    pub use_webvtt_styling: bool,

    /* Networking */
    pub udpsrc: Option<String>,
    pub udpaddr: Option<String>,
    /// Non-zero => Listen for UDP packets on this port, no files.
    pub udpport: u16,
    pub tcpport: Option<u16>,
    pub tcp_password: Option<String>,
    pub tcp_desc: Option<String>,
    pub srv_addr: Option<String>,
    pub srv_port: Option<u16>,
    /// Do NOT set time automatically?
    pub noautotimeref: bool,
    /// Files, stdin or network
    pub input_source: DataSource,

    pub output_filename: Option<String>,

    /// List of files to process
    pub inputfile: Option<Vec<String>>,
    pub demux_cfg: DemuxerConfig,
    pub enc_cfg: EncoderConfig,
    /// ms to delay (or advance) subs
    pub subs_delay: Timestamp,
    /// If true, the stdout will be flushed when data was written to the screen during a process_608 call.
    pub cc_to_stdout: bool,
    /// If true, the PES Header will be printed to console (debugging purposes)
    pub pes_header_to_stdout: bool,
    /// If true, the program will ignore PTS jumps.
    /// Sometimes this parameter is required for DVB subs with > 30s pause time
    pub ignore_pts_jumps: bool,
    pub multiprogram: bool,
    pub out_interval: i32,
    pub segment_on_key_frames_only: bool,
    pub debug_mask: DebugMessageMask,

    #[cfg(feature = "with_libcurl")]
    pub curlposturl: Option<Url>,

    //CC sharing
    #[cfg(feature = "enable_sharing")]
    pub sharing_enabled: bool,
    #[cfg(feature = "enable_sharing")]
    pub sharing_url: Option<Url>,
    #[cfg(feature = "enable_sharing")]
    //Translating
    pub translate_enabled: bool,
    #[cfg(feature = "enable_sharing")]
    pub translate_langs: Option<String>,
    #[cfg(feature = "enable_sharing")]
    pub translate_key: Option<String>,
}

impl Default for Options {
    fn default() -> Self {
        Self {
            extract: 1,
            no_rollup: Default::default(),
            noscte20: Default::default(),
            webvtt_create_css: Default::default(),
            cc_channel: 1,
            buffer_input: Default::default(),
            nofontcolor: Default::default(),
            nohtmlescape: Default::default(),
            notypesetting: Default::default(),
            extraction_start: Default::default(),
            extraction_end: Default::default(),
            print_file_reports: Default::default(),
            settings_608: Default::default(),
            settings_dtvcc: Default::default(),
            is_608_enabled: Default::default(),
            is_708_enabled: Default::default(),
            binary_concat: true,
            use_gop_as_pts: Default::default(),
            fix_padding: Default::default(),
            gui_mode_reports: Default::default(),
            no_progress_bar: Default::default(),
            sentence_cap_file: Default::default(),
            live_stream: Some(Timestamp::default()),
            filter_profanity_file: Default::default(),
            messages_target: Default::default(),
            timestamp_map: Default::default(),
            dolevdist: true,
            levdistmincnt: 2,
            levdistmaxpct: 10,
            investigate_packets: Default::default(),
            fullbin: Default::default(),
            nosync: Default::default(),
            hauppauge_mode: Default::default(),
            wtvconvertfix: Default::default(),
            wtvmpeg2: Default::default(),
            auto_myth: None,
            mp4vidtrack: Default::default(),
            extract_chapters: Default::default(),
            usepicorder: Default::default(),
            xmltv: Default::default(),
            xmltvliveinterval: Timestamp::from_millis(10000),
            xmltvoutputinterval: Default::default(),
            xmltvonlycurrent: Default::default(),
            keep_output_closed: Default::default(),
            force_flush: Default::default(),
            append_mode: Default::default(),
            ucla: Default::default(),
            tickertext: Default::default(),
            hardsubx: Default::default(),
            hardsubx_and_common: Default::default(),
            dvblang: Default::default(),
            ocrlang: Default::default(),
            ocr_oem: -1,
            psm: 3,
            ocr_quantmode: 1,
            mkvlang: Default::default(),
            analyze_video_stream: Default::default(),
            hardsubx_ocr_mode: Default::default(),
            hardsubx_min_sub_duration: Timestamp::from_millis(500),
            hardsubx_detect_italics: Default::default(),
            hardsubx_conf_thresh: Default::default(),
            hardsubx_hue: Default::default(),
            hardsubx_lum_thresh: 95.0,
            transcript_settings: Default::default(),
            date_format: Default::default(),
            send_to_srv: Default::default(),
            write_format: OutputFormat::Srt,
            write_format_rewritten: Default::default(),
            use_ass_instead_of_ssa: Default::default(),
            use_webvtt_styling: Default::default(),
            udpsrc: Default::default(),
            udpaddr: Default::default(),
            udpport: Default::default(),
            tcpport: Default::default(),
            tcp_password: Default::default(),
            tcp_desc: Default::default(),
            srv_addr: Default::default(),
            srv_port: Default::default(),
            noautotimeref: Default::default(),
            input_source: DataSource::default(),
            output_filename: Default::default(),
            inputfile: Default::default(),
            demux_cfg: Default::default(),
            enc_cfg: Default::default(),
            subs_delay: Default::default(),
            cc_to_stdout: Default::default(),
            pes_header_to_stdout: Default::default(),
            ignore_pts_jumps: Default::default(),
            multiprogram: Default::default(),
            out_interval: -1,
            segment_on_key_frames_only: Default::default(),
            debug_mask: DebugMessageMask::new(
                DebugMessageFlag::GENERIC_NOTICE,
                DebugMessageFlag::VERBOSE,
            ),
            curlposturl: Default::default(),
            sharing_enabled: Default::default(),
            sharing_url: Default::default(),
            translate_enabled: Default::default(),
            translate_langs: Default::default(),
            translate_key: Default::default(),
        }
    }
}

impl Options {
    pub fn millis_separator(&self) -> char {
        if self.ucla {
            '.'
        } else {
            self.date_format.millis_separator()
        }
    }
}

impl EncoderConfig {
    pub fn millis_separator(&self) -> char {
        if self.ucla {
            '.'
        } else {
            self.date_format.millis_separator()
        }
    }
}
