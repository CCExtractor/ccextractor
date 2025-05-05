use crate::decoder_isdb::exit_codes::*;
use crate::decoder_isdb::structs_xds::{CcxDecoder608ColorCode, CcxEncodingType};

use crate::common::OutputFormat; // ccxoutputformat

use crate::common::StreamMode; // //CcxStreamModeEnum

use crate::common::Codec; // ccxcodetype - 2 options{Codec, SelectCodec} - use appropriately

use crate::time::TimestampFormat; // ccxoutputdateformat

use crate::time::TimingContext; // ccxcommontimingctx

use crate::time::Timestamp; // ccx_boundary_time

use std::os::raw::{c_char, c_int, c_uchar, c_uint, c_ulonglong, c_void};

#[derive(Debug, Clone)]
#[repr(C)]
pub struct ListHead {
    pub next: *mut ListHead,
    pub prev: *mut ListHead,
}

#[repr(C)]
pub struct DtvccCtx {
    pub is_active: c_int,
    pub active_services_count: c_int,
    pub services_active: [c_int; CCX_DTVCC_MAX_SERVICES], // 0 - inactive, 1 - active
    pub report_enabled: c_int,
    pub report: *mut CcxDecoderDtvccReport,
    pub decoders: [DtvccServiceDecoder; CCX_DTVCC_MAX_SERVICES],
    pub current_packet: [c_uchar; CCX_DTVCC_MAX_PACKET_LENGTH],
    pub current_packet_length: c_int,
    pub is_current_packet_header_parsed: c_int,
    pub last_sequence: c_int,
    pub encoder: *mut c_void, // we can't include header, so keeping it this way
    pub no_rollup: c_int,
    pub timing: *mut TimingContext,
}

#[repr(C)]
pub struct CcxDecoderDtvccReport {
    pub reset_count: c_int,
    pub services: [c_uint; CCX_DTVCC_MAX_SERVICES],
}

#[repr(C)]
pub struct DtvccServiceDecoder {
    pub windows: [DtvccWindow; CCX_DTVCC_MAX_WINDOWS],
    pub current_window: c_int,
    pub tv: *mut DtvccTvScreen,
    pub cc_count: c_int,
}

#[repr(C)]
pub struct DtvccWindow {
    pub is_defined: c_int,
    pub number: c_int,
    pub priority: c_int,
    pub col_lock: c_int,
    pub row_lock: c_int,
    pub visible: c_int,
    pub anchor_vertical: c_int,
    pub relative_pos: c_int,
    pub anchor_horizontal: c_int,
    pub row_count: c_int,
    pub anchor_point: c_int,
    pub col_count: c_int,
    pub pen_style: c_int,
    pub win_style: c_int,
    pub commands: [c_uchar; 6], // Commands used to create this window
    pub attribs: DtvccWindowAttribs,
    pub pen_row: c_int,
    pub pen_column: c_int,
    pub rows: [*mut DtvccSymbol; CCX_DTVCC_MAX_ROWS],
    pub pen_colors: [[DtvccPenColor; CCX_DTVCC_SCREENGRID_COLUMNS]; CCX_DTVCC_MAX_ROWS],
    pub pen_attribs: [[DtvccPenAttribs; CCX_DTVCC_SCREENGRID_COLUMNS]; CCX_DTVCC_MAX_ROWS],
    pub pen_color_pattern: DtvccPenColor,
    pub pen_attribs_pattern: DtvccPenAttribs,
    pub memory_reserved: c_int,
    pub is_empty: c_int,
    pub time_ms_show: i64, // Assuming LLONG is equivalent to i64
    pub time_ms_hide: i64,
}

#[repr(C)]
pub struct DtvccWindowAttribs {
    pub justify: c_int,
    pub print_direction: c_int,
    pub scroll_direction: c_int,
    pub word_wrap: c_int,
    pub display_effect: c_int,
    pub effect_direction: c_int,
    pub effect_speed: c_int,
    pub fill_color: c_int,
    pub fill_opacity: c_int,
    pub border_type: c_int,
    pub border_color: c_int,
}

#[repr(C)]
pub struct DtvccSymbol {
    pub sym: u16,      // symbol itself, at least 16 bit
    pub init: c_uchar, // initialized or not. could be 0 or 1
}

#[repr(C)]
pub struct DtvccPenColor {
    pub fg_color: c_int,
    pub fg_opacity: c_int,
    pub bg_color: c_int,
    pub bg_opacity: c_int,
    pub edge_color: c_int,
}

#[repr(C)]
pub struct DtvccPenAttribs {
    pub pen_size: c_int,
    pub offset: c_int,
    pub text_tag: c_int,
    pub font_tag: c_int,
    pub edge_type: c_int,
    pub underline: c_int,
    pub italic: c_int,
}

#[repr(C)]
pub struct DtvccTvScreen {
    pub chars: [[DtvccSymbol; CCX_DTVCC_SCREENGRID_COLUMNS]; CCX_DTVCC_SCREENGRID_ROWS],
    pub pen_colors: [[DtvccPenColor; CCX_DTVCC_SCREENGRID_COLUMNS]; CCX_DTVCC_SCREENGRID_ROWS],
    pub pen_attribs: [[DtvccPenAttribs; CCX_DTVCC_SCREENGRID_COLUMNS]; CCX_DTVCC_SCREENGRID_ROWS],
    pub time_ms_show: i64,
    pub time_ms_hide: i64,
    pub cc_count: c_uint,
    pub service_number: c_int,
    pub old_cc_time_end: c_int,
}

#[repr(C)]
pub struct CcxDecoderVbiCtx {
    pub vbi_decoder_inited: c_int,
    pub zvbi_decoder: VbiRawDecoder,
    // vbi3_raw_decoder zvbi_decoder;
    pub vbi_debug_dump: Option<Box<std::fs::File>>,
}

#[repr(C)]
pub struct VbiRawDecoder {
    /* Sampling parameters */
    /**
     * Either 525 (M/NTSC, M/PAL) or 625 (PAL, SECAM), describing the
     * scan line system all line numbers refer to.
     */
    pub scanning: c_int,
    /**
     * Format of the raw vbi data.
     */
    pub sampling_format: VbiPixfmt,
    /**
     * Sampling rate in Hz, the number of samples or pixels
     * captured per second.
     */
    pub sampling_rate: c_int, // Hz
    /**
     * Number of samples or pixels captured per scan line,
     * in bytes. This determines the raw vbi image width and you
     * want it large enough to cover all data transmitted in the line (with
     * headroom).
     */
    pub bytes_per_line: c_int,
    /**
     * The distance from 0H (leading edge hsync, half amplitude point)
     * to the first sample (pixel) captured, in samples (pixels). You want
     * an offset small enough not to miss the start of the data
     * transmitted.
     */
    pub offset: c_int, // 0H, samples
    /**
     * First scan line to be captured, first and second field
     * respectively, according to the ITU-R line numbering scheme
     * (see vbi_sliced). Set to zero if the exact line number isn't
     * known.
     */
    pub start: [c_int; 2], // ITU-R numbering
    /**
     * Number of scan lines captured, first and second
     * field respectively. This can be zero if only data from one
     * field is required. The sum @a count[0] + @a count[1] determines the
     * raw vbi image height.
     */
    pub count: [c_int; 2], // field lines
    /**
     * In the raw vbi image, normally all lines of the second
     * field are supposed to follow all lines of the first field. When
     * this flag is set, the scan lines of first and second field
     * will be interleaved in memory. This implies @a count[0] and @a count[1]
     * are equal.
     */
    pub interlaced: c_int,
    /**
     * Fields must be stored in temporal order, i. e. as the
     * lines have been captured. It is assumed that the first field is
     * also stored first in memory, however if the hardware cannot reliable
     * distinguish fields this flag shall be cleared, which disables
     * decoding of data services depending on the field number.
     */
    pub synchronous: c_int,

    pub services: c_uint,
    pub num_jobs: c_int,

    pub pattern: *mut i8,
    pub jobs: [VbiRawDecoderJob; 8],
}

#[repr(C)]
pub struct VbiRawDecoderJob {
    pub id: c_uint,
    pub offset: c_int,
    pub slicer: VbiBitSlicer,
}

#[repr(C)]
pub enum VbiPixfmt {
    Yuv420 = 1,
    Yuyv,
    Yvyu,
    Uyvy,
    Vyuy,
    Pal8,
    Rgba32Le = 32,
    Rgba32Be,
    Bgra32Le,
    Bgra32Be,
    Abgr32Be, /* = 32, // synonyms */
    Abgr32Le,
    Argb32Be,
    Argb32Le,
    Rgb24,
    Bgr24,
    Rgb16Le,
    Rgb16Be,
    Bgr16Le,
    Bgr16Be,
    Rgba15Le,
    Rgba15Be,
    Bgra15Le,
    Bgra15Be,
    Argb15Le,
    Argb15Be,
    Abgr15Le,
    Abgr15Be,
}

#[repr(C)]
pub struct VbiBitSlicer {
    pub func: Option<
        extern "C" fn(slicer: *mut VbiBitSlicer, raw: *mut c_uchar, buf: *mut c_uchar) -> c_int,
    >,
    pub cri: c_uint,
    pub cri_mask: c_uint,
    pub thresh: c_int,
    pub cri_bytes: c_int,
    pub cri_rate: c_int,
    pub oversampling_rate: c_int,
    pub phase_shift: c_int,
    pub step: c_int,
    pub frc: c_uint,
    pub frc_bits: c_int,
    pub payload: c_int,
    pub endian: c_int,
    pub skip: c_int,
}

#[derive(Debug, Clone)]
#[repr(C)]
pub struct ISDBSubContext {
    pub nb_char: c_int,
    pub nb_line: c_int,
    pub timestamp: u64,
    pub prev_timestamp: u64,
    pub text_list_head: ListHead,
    pub buffered_text: ListHead,
    pub current_state: ISDBSubState,
    pub tmd: IsdbTmd,
    pub nb_lang: c_int,
    pub offset_time: OffsetTime,
    pub dmf: c_uchar,
    pub dc: c_uchar,
    pub cfg_no_rollup: c_int,
}

#[derive(Debug, Clone, Default)]
#[repr(C)]
pub struct ISDBSubState {
    pub auto_display: c_int, // bool
    pub rollup_mode: c_int,  // bool
    pub need_init: c_uchar,  // bool
    pub clut_high_idx: c_uchar,
    pub fg_color: c_uint,
    pub bg_color: c_uint,
    pub hfg_color: c_uint,
    pub hbg_color: c_uint,
    pub mat_color: c_uint,
    pub raster_color: c_uint,
    pub layout_state: ISDBSubLayout,
}

#[derive(Debug, Clone)]
#[repr(C)]
pub struct ISDBSubLayout {
    pub format: WritingFormat,
    pub display_area: DispArea,
    pub font_size: c_int,
    pub font_scale: FScale,
    pub cell_spacing: Spacing,
    pub cursor_pos: ISDBPos,
    pub ccc: IsdbCCComposition,
    pub acps: [c_int; 2],
}

#[derive(Debug, Clone, Default)]
#[repr(C)]
pub struct DispArea {
    pub x: c_int,
    pub y: c_int,
    pub w: c_int,
    pub h: c_int,
}

#[derive(Debug, Clone)]
#[repr(C)]
pub struct FScale {
    pub fscx: c_int,
    pub fscy: c_int,
}

#[derive(Debug, Clone, Default)]
#[repr(C)]
pub struct Spacing {
    pub col: c_int,
    pub row: c_int,
}

#[derive(Debug, Clone, Default)]
#[repr(C)]
pub struct ISDBPos {
    pub x: c_int,
    pub y: c_int,
}

#[derive(Debug, Clone, Default)]
#[repr(C)]
pub struct OffsetTime {
    pub hour: c_int,
    pub min: c_int,
    pub sec: c_int,
    pub milli: c_int,
}

#[repr(C)]
#[derive(Copy, Clone, Debug, Default, PartialEq)]
pub enum WritingFormat {
    #[default]
    HorizontalStdDensity = 0,
    VerticalStdDensity = 1,
    HorizontalHighDensity = 2,
    VerticalHighDensity = 3,
    HorizontalWesternLang = 4,
    Horizontal1920x1080 = 5,
    Vertical1920x1080 = 6,
    Horizontal960x540 = 7,
    Vertical960x540 = 8,
    Horizontal720x480 = 9,
    Vertical720x480 = 10,
    Horizontal1280x720 = 11,
    Vertical1280x720 = 12,
    HorizontalCustom = 100,
    None,
}

#[derive(Debug, Clone, Default)]
#[repr(C)]
pub enum IsdbCCComposition {
    #[default]
    None = 0,
    And = 2,
    Or = 3,
    Xor = 4,
}

use std::convert::TryFrom;

impl TryFrom<u8> for IsdbCCComposition {
    type Error = ();

    fn try_from(value: u8) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(IsdbCCComposition::None),
            2 => Ok(IsdbCCComposition::And),
            3 => Ok(IsdbCCComposition::Or),
            4 => Ok(IsdbCCComposition::Xor),
            _ => Err(()),
        }
    }
}

#[derive(Debug, Clone, Copy, Default, PartialEq)]
#[repr(C)]
pub enum IsdbTmd {
    #[default]
    Free = 0,
    RealTime = 0x1,
    OffsetTime = 0x2,
}

#[repr(C)]
pub enum FontSize {
    SmallFontSize,
    MiddleFontSize,
    StandardFontSize,
}

#[repr(C)]
pub enum CsiCommand {
    Gsm = 0x42,
    Swf = 0x53,
    Ccc = 0x54,
    Sdf = 0x56,
    Ssm = 0x57,
    Shs = 0x58,
    Svs = 0x59,
    Pld = 0x5B,
    Plu = 0x5C,
    Gaa = 0x5D,
    Src = 0x5E,
    Sdp = 0x5F,
    Acps = 0x61,
    Tcc = 0x62,
    Orn = 0x63,
    Mdf = 0x64,
    Cfs = 0x65,
    Xcs = 0x66,
    Pra = 0x68,
    Acs = 0x69,
    Rcs = 0x6E,
    Scs = 0x6F,
}

#[repr(C)]
pub enum Color {
    CcxIsdbBlack,
    FiRed,
    FiGreen,
    FiYellow,
    FiBlue,
    FiMagenta,
    FiCyan,
    FiWhite,
    CcxIsdbTransparent,
    HiRed,
    HiGreen,
    HiYellow,
    HiBlue,
    HiMagenta,
    HiCyan,
    HiWhite,
}

#[no_mangle]
pub const fn rgba(r: u8, g: u8, b: u8, a: u8) -> u32 {
    ((255 - a as u32) << 24) | ((b as u32) << 16) | ((g as u32) << 8) | (r as u32)
}

type Rgba = u32;
pub const ZATA: u32 = 0;
pub const DEFAULT_CLUT: [Rgba; 128] = [
    // 0-7
    rgba(0, 0, 0, 255),
    rgba(255, 0, 0, 255),
    rgba(0, 255, 0, 255),
    rgba(255, 255, 0, 255),
    rgba(0, 0, 255, 255),
    rgba(255, 0, 255, 255),
    rgba(0, 255, 255, 255),
    rgba(255, 255, 255, 255),
    // 8-15
    rgba(0, 0, 0, 0),
    rgba(170, 0, 0, 255),
    rgba(0, 170, 0, 255),
    rgba(170, 170, 0, 255),
    rgba(0, 0, 170, 255),
    rgba(170, 0, 170, 255),
    rgba(0, 170, 170, 255),
    rgba(170, 170, 170, 255),
    // 16-23
    rgba(0, 0, 85, 255),
    rgba(0, 85, 0, 255),
    rgba(0, 85, 85, 255),
    rgba(0, 85, 170, 255),
    rgba(0, 85, 255, 255),
    rgba(0, 170, 85, 255),
    rgba(0, 170, 255, 255),
    rgba(0, 255, 85, 255),
    // 24-31
    rgba(0, 255, 170, 255),
    rgba(85, 0, 0, 255),
    rgba(85, 0, 85, 255),
    rgba(85, 0, 170, 255),
    rgba(85, 0, 255, 255),
    rgba(85, 85, 0, 255),
    rgba(85, 85, 85, 255),
    rgba(85, 85, 170, 255),
    // 32-39
    rgba(85, 85, 255, 255),
    rgba(85, 170, 0, 255),
    rgba(85, 170, 85, 255),
    rgba(85, 170, 170, 255),
    rgba(85, 170, 255, 255),
    rgba(85, 255, 0, 255),
    rgba(85, 255, 85, 255),
    rgba(85, 255, 170, 255),
    // 40-47
    rgba(85, 255, 255, 255),
    rgba(170, 0, 85, 255),
    rgba(170, 0, 255, 255),
    rgba(170, 85, 0, 255),
    rgba(170, 85, 85, 255),
    rgba(170, 85, 170, 255),
    rgba(170, 85, 255, 255),
    rgba(170, 170, 85, 255),
    // 48-55
    rgba(170, 170, 255, 255),
    rgba(170, 255, 0, 255),
    rgba(170, 255, 85, 255),
    rgba(170, 255, 170, 255),
    rgba(170, 255, 255, 255),
    rgba(255, 0, 85, 255),
    rgba(255, 0, 170, 255),
    rgba(255, 85, 0, 255),
    // 56-63
    rgba(255, 85, 85, 255),
    rgba(255, 85, 170, 255),
    rgba(255, 85, 255, 255),
    rgba(255, 170, 0, 255),
    rgba(255, 170, 85, 255),
    rgba(255, 170, 170, 255),
    rgba(255, 170, 255, 255),
    rgba(255, 255, 85, 255),
    // 64
    rgba(255, 255, 170, 255),
    // 65-127 are calculated later.
    // Initialize remaining elements to 0
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
    ZATA,
];

#[repr(C)]
pub struct ISDBText {
    pub buf: *mut c_char,
    pub len: usize,
    pub used: usize,
    pub pos: ISDBPos,
    pub txt_tail: usize, // tail of the text, excluding trailing control sequences.
    pub timestamp: c_ulonglong, // Time Stamp when first string is received
    pub refcount: c_int,
    pub list: ListHead,
}

pub struct CcxEncodersTranscriptFormat {
    pub show_start_time: i32,    // Show start and/or end time.
    pub show_end_time: i32,      // Show start and/or end time.
    pub show_mode: i32,          // Show which mode if available (E.G.: POP, RU1, ...)
    pub show_cc: i32,            // Show which CC channel has been captured.
    pub relative_timestamp: i32, // Timestamps relative to start of sample or in UTC?
    pub xds: i32,                // Show XDS or not
    pub use_colors: i32,         // Add colors or no colors
    pub is_final: i32, // Used to determine if these parameters should be changed afterwards.
}

#[repr(i32)]
pub enum CcxDataSource {
    File = 0,
    Stdin = 1,
    Network = 2,
    Tcp = 3,
}

#[repr(C)]
pub struct CcxDecoder608Settings {
    pub direct_rollup: i32,
    pub force_rollup: i32,
    pub no_rollup: i32,
    pub default_color: CcxDecoder608ColorCode,
    pub screens_to_process: i32,
    pub report: *mut CcxDecoder608Report,
}

#[repr(C)]
pub struct CcxDecoder608Report {
    pub xds: u8,
    pub cc_channels: [u8; 4],
}

#[repr(C)]
pub struct CcxDecoderDtvccSettings {
    pub enabled: i32,
    pub print_file_reports: i32,
    pub no_rollup: i32,
    pub report: *mut CcxDecoderDtvccReport,
    pub active_services_count: i32,
    pub services_enabled: [i32; CCX_DTVCC_MAX_SERVICES],
    pub timing: *mut TimingContext,
}

#[repr(C)]
pub struct DemuxerCfg {
    pub m2ts: i32,
    pub auto_stream: StreamMode,
    pub codec: Codec,
    pub nocodec: Codec,
    pub ts_autoprogram: u32,
    pub ts_allprogram: u32,
    pub ts_cappids: [u32; 128],
    pub nb_ts_cappid: i32,
    pub ts_forced_cappid: u32,
    pub ts_forced_program: i32,
    pub ts_forced_program_selected: u32,
    pub ts_datastreamtype: i32,
    pub ts_forced_streamtype: u32,
}

#[repr(C)]
pub struct EncoderCfg {
    pub extract: i32,
    pub dtvcc_extract: i32,
    pub gui_mode_reports: i32,
    pub output_filename: *mut c_char,
    pub write_format: OutputFormat,
    pub keep_output_closed: i32,
    pub force_flush: i32,
    pub append_mode: i32,
    pub ucla: i32,
    pub encoding: CcxEncodingType,
    pub date_format: TimestampFormat,
    pub millis_separator: c_char,
    pub autodash: i32,
    pub trim_subs: i32,
    pub sentence_cap: i32,
    pub splitbysentence: i32,
    pub filter_profanity: i32,
    pub with_semaphore: i32,
    pub start_credits_text: *mut c_char,
    pub end_credits_text: *mut c_char,
    pub startcreditsnotbefore: Timestamp,
    pub startcreditsnotafter: Timestamp,
    pub startcreditsforatleast: Timestamp,
    pub startcreditsforatmost: Timestamp,
    pub endcreditsforatleast: Timestamp,
    pub endcreditsforatmost: Timestamp,
    pub transcript_settings: CcxEncodersTranscriptFormat,
    pub send_to_srv: u32,
    pub no_bom: i32,
    pub first_input_file: *mut c_char,
    pub multiple_files: i32,
    pub no_font_color: i32,
    pub no_type_setting: i32,
    pub cc_to_stdout: i32,
    pub line_terminator_lf: i32,
    pub subs_delay: i64,
    pub program_number: i32,
    pub in_format: u8,
    pub nospupngocr: i32,
    pub force_dropframe: i32,
    pub render_font: *mut c_char,
    pub render_font_italics: *mut c_char,
    pub services_enabled: [i32; CCX_DTVCC_MAX_SERVICES],
    pub services_charsets: *mut *mut c_char,
    pub all_services_charset: *mut c_char,
    pub extract_only_708: i32,
}

impl Default for ISDBSubLayout {
    fn default() -> Self {
        Self {
            format: WritingFormat::None,
            display_area: Default::default(),
            font_size: 0,
            font_scale: Default::default(),
            cell_spacing: Default::default(),
            cursor_pos: Default::default(),
            ccc: IsdbCCComposition::None,
            acps: [0; 2],
        }
    }
}

impl Default for FScale {
    fn default() -> Self {
        Self {
            fscx: 100,
            fscy: 100,
        }
    }
}

impl Default for ISDBSubContext {
    fn default() -> Self {
        Self {
            nb_char: 0,
            nb_line: 0,
            timestamp: 0,
            prev_timestamp: 0,
            text_list_head: Default::default(),
            buffered_text: Default::default(),
            current_state: Default::default(),
            tmd: IsdbTmd::Free,
            nb_lang: 0,
            offset_time: Default::default(),
            dmf: 0,
            dc: 0,
            cfg_no_rollup: 0,
        }
    }
}

impl Default for ListHead {
    fn default() -> Self {
        Self {
            next: std::ptr::null_mut(),
            prev: std::ptr::null_mut(),
        }
    }
}
