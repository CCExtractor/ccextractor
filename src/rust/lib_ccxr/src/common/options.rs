use url::Url;

use std::path::PathBuf;

use crate::common::{
    DataSource, Language, OutputFormat, SelectCodec, StreamMode, StreamType, DTVCC_MAX_SERVICES,
};
use crate::hardsubx::{ColorHue, OcrMode};
use crate::util::encoding::Encoding;
use crate::util::log::OutputTarget;
use crate::util::time::{Timestamp, TimestampFormat};

pub enum DtvccServiceCharset {
    Same(String),
    Unique(Box<[String; DTVCC_MAX_SERVICES]>),
}

#[allow(dead_code)]
pub struct DemuxerConfig {
    /// Regular TS or M2TS
    m2ts: bool,
    auto_stream: StreamMode,

    /* subtitle codec type */
    codec: SelectCodec,
    nocodec: SelectCodec,

    /// Try to find a stream with captions automatically (no -pn needed)
    ts_autoprogram: bool,
    ts_allprogram: bool,
    /// PID for stream that holds caption information
    ts_cappids: Vec<u32>,
    /// If 1, never mess with the selected PID
    ts_forced_cappid: bool,
    /// Specific program to process in TS files, if ts_forced_program_selected==1
    ts_forced_program: Option<u32>,
    /// User WANTED stream type (i.e. use the stream that has this type)
    ts_datastreamtype: StreamType,
    /// User selected (forced) stream type
    ts_forced_streamtype: StreamType,
}

#[allow(dead_code)]
pub struct EncoderConfig {
    /// Extract 1st (1), 2nd (2) or both fields (12)
    extract: u8,
    dtvcc_extract: bool,
    // If true, output in stderr progress updates so the GUI can grab them
    gui_mode_reports: bool,
    output_filename: String,
    write_format: OutputFormat,
    keep_output_closed: bool,
    /// Force flush on content write
    force_flush: bool,
    /// Append mode for output files
    append_mode: bool,
    /// true if -UCLA used, false if not
    ucla: bool,

    encoding: Encoding,
    date_format: TimestampFormat,
    /// Add dashes (-) before each speaker automatically?
    autodash: bool,
    /// "    Remove spaces at sides?    "
    trim_subs: bool,
    /// FIX CASE? = Fix case?
    sentence_cap: bool,
    /// Split text into complete sentences and prorate time?
    splitbysentence: bool,

    /// If out=curl, where do we send the data to?
    #[cfg(feature = "with_libcurl")]
    curlposturl: Option<Url>,

    /// Censors profane words from subtitles
    filter_profanity: bool,

    /// Write a .sem file on file open and delete it on close?
    with_semaphore: bool,
    /* Credit stuff */
    start_credits_text: String,
    end_credits_text: String,
    startcreditsnotbefore: Timestamp, // Where to insert start credits, if possible
    startcreditsnotafter: Timestamp,
    startcreditsforatleast: Timestamp, // How long to display them?
    startcreditsforatmost: Timestamp,
    endcreditsforatleast: Timestamp,
    endcreditsforatmost: Timestamp,

    /// Keeps the settings for generating transcript output files.
    /* ccx_encoders_transcript_format transcript_settings; */
    send_to_srv: bool,
    /// Set to true when no BOM (Byte Order Mark) should be used for files.
    /// Note, this might make files unreadable in windows!
    no_bom: bool,
    first_input_file: String,
    multiple_files: bool,
    no_font_color: bool,
    no_type_setting: bool,
    /// If this is set to true, the stdout will be flushed when data was written to the screen during a process_608 call.
    cc_to_stdout: bool,
    /// false = CRLF, true = LF
    line_terminator_lf: bool,
    /// ms to delay (or advance) subs
    subs_delay: Timestamp,
    program_number: u32,
    in_format: u8,
    // true if we don't want to OCR bitmaps to add the text as comments in the XML file in spupng
    nospupngocr: bool,

    // MCC File
    /// true if dropframe frame count should be used. defaults to no drop frame.
    force_dropframe: bool,

    // text -> png (text render)
    /// The font used to render text if needed (e.g. teletext->spupng)
    render_font: PathBuf,
    render_font_italics: PathBuf,

    //CEA-708
    services_enabled: [bool; DTVCC_MAX_SERVICES],
    services_charsets: DtvccServiceCharset,
    // true if only 708 subs extraction is enabled
    extract_only_708: bool,
}

/// Options from user parameters
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
    pub extraction_start: Timestamp,
    /// The end of the segment we actually process
    pub extraction_end: Timestamp,
    pub print_file_reports: bool,
    /// Contains the settings for the 608 decoder.
    /* ccx_decoder_608_settings settings_608, */
    /// Same for 708 decoder
    /* ccx_decoder_dtvcc_settings settings_dtvcc, */
    /// Is 608 enabled by explicitly using flags(-1,-2,-12)
    pub is_608_enabled: bool,
    /// Is 708 enabled by explicitly using flags(-svc)
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
    /// 0 -> Not a complete file but a live stream, without timeout
    ///
    /// None -> A regular file
    ///
    /// \>0 -> Live stream with a timeout of this value in seconds
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
    pub ocr_oem: u8,
    /// How to quantize the bitmap before passing to to tesseract
    /// (false = no quantization at all, true = CCExtractor's internal)
    pub ocr_quantmode: bool,
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
    /* ccx_encoders_transcript_format transcript_settings; */
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
