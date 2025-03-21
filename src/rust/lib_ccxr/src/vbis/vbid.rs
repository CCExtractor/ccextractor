use std::os::raw::{c_char, c_int, c_long, c_uchar, c_uint, c_ulonglong, c_void};
use std::ptr::null_mut;
use std::ptr::*;

use libc::FILE;
extern crate libc;

const CCX_DTVCC_MAX_SERVICES: usize = 63;
const CCX_DTVCC_MAX_WINDOWS: usize = 8;
const CCX_DTVCC_MAX_ROWS: usize = 15;
const CCX_DTVCC_SCREENGRID_COLUMNS: usize = 210;
const CCX_DTVCC_MAX_PACKET_LENGTH: usize = 128;
const CCX_DTVCC_SCREENGRID_ROWS: usize = 75;

pub const CCX_MESSAGES_QUIET: i32 = 0;
pub const CCX_MESSAGES_STDOUT: i32 = 1;
pub const CCX_MESSAGES_STDERR: i32 = 2;

const SORTBUF: usize = 2 * MAXBFRAMES + 1;
const MAXBFRAMES: usize = 50;

const NUM_BYTES_PER_PACKET: usize = 35;
const NUM_XDS_BUFFERS: usize = 9;

pub const _VBI3_RAW_DECODER_MAX_JOBS: usize = 8;

pub const CCX_FALSE: i32 = 0;
pub const CCX_TRUE: i32 = 1;

pub const VBI_SLICED_CAPTION_525_F1: u32 = 0x00000020;
pub const VBI_SLICED_CAPTION_525_F2: u32 = 0x00000040;
pub const VBI_SLICED_CAPTION_525: u32 = VBI_SLICED_CAPTION_525_F1 | VBI_SLICED_CAPTION_525_F2;


pub const VBI_SLICED_VBI_525: u32 = 0x40000000;
pub const VBI_SLICED_VBI_625: u32 = 0x20000000;
pub const _VBI3_RAW_DECODER_MAX_WAYS: usize = 8;

pub const VBI_SLICED_TELETEXT_B_L10_625: u32 = 0x00000001;
pub const VBI_SLICED_TELETEXT_B_L25_625: u32 = 0x00000002;
pub const VBI_SLICED_TELETEXT_B: u32 = VBI_SLICED_TELETEXT_B_L10_625 | VBI_SLICED_TELETEXT_B_L25_625;

pub const VBI_SLICED_CAPTION_625_F1: u32 = 0x00000008;
pub const VBI_SLICED_CAPTION_625_F2: u32 = 0x00000010;
pub const VBI_SLICED_CAPTION_625: u32 = VBI_SLICED_CAPTION_625_F1 | VBI_SLICED_CAPTION_625_F2;

pub const VBI_SLICED_VPS: u32 = 0x00000004;
pub const VBI_SLICED_VPS_F2: u32 = 0x00001000;

pub const VBI_SLICED_WSS_625: u32 = 0x00000400;

pub const CCX_DMT_PARSE: i32 = 1;

pub const VBI_VIDEOSTD_SET_EMPTY: u64 = 0;
pub const VBI_VIDEOSTD_SET_PAL_BG: u64 = 1;
pub const VBI_VIDEOSTD_SET_625_50: u64 = 1;
pub const VBI_VIDEOSTD_SET_525_60: u64 = 2;
pub const VBI_VIDEOSTD_SET_ALL: u64 = 3;
pub const VBI_SLICED_TELETEXT_A: u32 = 0x00002000;
pub const VBI_SLICED_TELETEXT_C_625: u32 = 0x00004000;
pub const VBI_SLICED_TELETEXT_D_625: u32 = 0x00008000;
pub const VBI_SLICED_TELETEXT_B_525: u32 = 0x00010000;
pub const VBI_SLICED_TELETEXT_C_525: u32 = 0x00000100;
pub const VBI_SLICED_TELETEXT_D_525: u32 = 0x00020000;
pub const VBI_SLICED_2xCAPTION_525: u32 = 0x00000080;

pub const DEF_THR_FRAC: u32 = 9;
pub const LP_AVG: u32 = 4;
const RAW_DECODER_PATTERN_DUMP: bool = false;
pub const VBI_SLICED_TELETEXT_BD_525: u32 = 0x00000200;

#[repr(C)]
#[derive(PartialEq, Eq, Debug, Copy, Clone)]
pub enum CcxOutputFormat {
    Raw = 0,
    Srt = 1,
    Sami = 2,
    Transcript = 3,
    Rcwt = 4,
    Null = 5,
    Smptett = 6,
    Spupng = 7,
    DvdRaw = 8, // See -d at http://www.theneitherworld.com/mcpoodle/SCC_TOOLS/DOCS/SCC_TOOLS.HTML#CCExtract
    Webvtt = 9,
    SimpleXml = 10,
    G608 = 11,
    Curl = 12,
    Ssa = 13,
    Mcc = 14,
    Scc = 15,
    Ccd = 16,
}


#[repr(C)]
pub struct LibCcDecode {
    pub cc_stats: [c_int; 4],
    pub saw_caption_block: c_int,
    pub processed_enough: c_int, // If 1, we have enough lines, time, etc.

    /* 608 contexts - note that this shouldn't be global, they should be
    per program */
    pub context_cc608_field_1: *mut c_void,
    pub context_cc608_field_2: *mut c_void,

    pub no_rollup: c_int, // If 1, write one line at a time
    pub noscte20: c_int,
    pub fix_padding: c_int, // Replace 0000 with 8080 in HDTV (needed for some cards)
    pub write_format: CcxOutputFormat, // 0 = Raw, 1 = srt, 2 = SMI
    pub extraction_start: CcxBoundaryTime,
    pub extraction_end: CcxBoundaryTime, // Segment we actually process
    pub subs_delay: i64,                 // ms to delay (or advance) subs
    pub extract: c_int,                  // Extract 1st, 2nd or both fields
    pub fullbin: c_int,                  // Disable pruning of padding cc blocks
    pub dec_sub: CcSubtitle,
    pub in_bufferdatatype: CcxBufferdataType,
    pub hauppauge_mode: c_uint, // If 1, use PID=1003, process specially and so on

    pub frames_since_last_gop: c_int,
    /* GOP-based timing */
    pub saw_gop_header: c_int,
    /* Time info for timed-transcript */
    pub max_gop_length: c_int,  // (Maximum) length of a group of pictures
    pub last_gop_length: c_int, // Length of the previous group of pictures
    pub total_pulldownfields: c_uint,
    pub total_pulldownframes: c_uint,
    pub program_number: c_int,
    pub list: ListHead,
    pub timing: *mut CcxCommonTimingCtx,
    pub codec: CcxCodeType,
    // Set to true if data is buffered
    pub has_ccdata_buffered: c_int,
    pub is_alloc: c_int,

    pub avc_ctx: *mut AvcCtx,
    pub private_data: *mut c_void,

    /* General video information */
    pub current_hor_size: c_uint,
    pub current_vert_size: c_uint,
    pub current_aspect_ratio: c_uint,
    pub current_frame_rate: c_uint, // Assume standard fps, 29.97

    /* Required in es_function.c */
    pub no_bitstream_error: c_int,
    pub saw_seqgoppic: c_int,
    pub in_pic_data: c_int,

    pub current_progressive_sequence: c_uint,
    pub current_pulldownfields: c_uint,

    pub temporal_reference: c_int,
    pub picture_coding_type: CcxFrameType,
    pub num_key_frames: c_uint,
    pub picture_structure: c_uint,
    pub repeat_first_field: c_uint,
    pub progressive_frame: c_uint,
    pub pulldownfields: c_uint,
    /* Required in es_function.c and es_userdata.c */
    pub top_field_first: c_uint, // Needs to be global

    /* Stats. Modified in es_userdata.c */
    pub stat_numuserheaders: c_int,
    pub stat_dvdccheaders: c_int,
    pub stat_scte20ccheaders: c_int,
    pub stat_replay5000headers: c_int,
    pub stat_replay4000headers: c_int,
    pub stat_dishheaders: c_int,
    pub stat_hdtv: c_int,
    pub stat_divicom: c_int,
    pub false_pict_header: c_int,

    pub dtvcc: *mut DtvccCtx,
    pub current_field: c_int,
    // Analyse/use the picture information
    pub maxtref: c_int, // Use to remember the temporal reference number

    pub cc_data_count: [c_int; SORTBUF],
    // Store fts;
    pub cc_fts: [i64; SORTBUF],
    // Store HD CC packets
    pub cc_data_pkts: [[u8; 10 * 31 * 3 + 1]; SORTBUF], // *10, because MP4 seems to have different limits

    // The sequence number of the current anchor frame.  All currently read
    // B-Frames belong to this I- or P-frame.
    pub anchor_seq_number: c_int,
    pub xds_ctx: *mut CcxDecodersXdsContext,
    pub vbi_decoder: *mut CcxDecoderVbiCtx,

    pub writedata: Option<
        extern "C" fn(
            data: *const u8,
            length: c_int,
            private_data: *mut c_void,
            sub: *mut CcSubtitle,
        ) -> c_int,
    >,

    // dvb subtitle related
    pub ocr_quantmode: c_int,
    pub prev: *mut LibCcDecode,
}

#[repr(C)]
pub struct CcxBoundaryTime {
    pub hh: i32,
    pub mm: i32,
    pub ss: i32,
    pub time_in_ms: i64, // Assuming LLONG is equivalent to i64
    pub set: i32,
}

#[repr(C)]
pub struct CcSubtitle {
    /**
     * A generic data which contain data according to decoder
     * @warn decoder cant output multiple types of data
     */
    pub data: *mut c_void,
    pub datatype: Subdatatype,

    /** number of data */
    pub nb_data: c_uint,

    /**  type of subtitle */
    pub type_: Subtype,

    /** Encoding type of Text, must be ignored in case of subtype as bitmap or cc_screen*/
    pub enc_type: CcxEncodingType,

    /* set only when all the data is to be displayed at same time
     * Unit is of time is ms
     */
    pub start_time: i64, // Assuming LLONG is equivalent to i64
    pub end_time: i64,

    /* flags */
    pub flags: c_int,

    /* index of language table */
    pub lang_index: c_int,

    /** flag to tell that decoder has given output */
    pub got_output: c_int,

    pub mode: [u8; 5],
    pub info: [u8; 4],

    /** Used for DVB end time in ms */
    pub time_out: c_int,

    pub next: *mut CcSubtitle,
    pub prev: *mut CcSubtitle,
}

#[repr(C)]
pub enum Subdatatype {
    Generic = 0,
    Dvb = 1,
}

#[repr(C)]
pub enum Subtype {
    Bitmap,
    Cc608,
    Text,
    Raw,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub enum CcxEncodingType {
    Unicode = 0,
    Latin1 = 1,
    Utf8 = 2,
    Ascii = 3,
}

#[repr(C)]
pub enum CcxBufferdataType {
    Unknown = 0,
    Pes = 1,
    Raw = 2,
    H264 = 3,
    Hauppage = 4,
    Teletext = 5,
    PrivateMpeg2Cc = 6,
    DvbSubtitle = 7,
    IsdbSubtitle = 8,
    RawType = 9, // Buffer where cc data contain 3 byte cc_valid ccdata 1 ccdata 2
    DvdSubtitle = 10,
}

#[repr(C)]
pub enum CcxFrameType {
    ResetOrUnknown = 0,
    IFrame = 1,
    PFrame = 2,
    BFrame = 3,
    DFrame = 4,
}

#[derive(Debug, Clone)]
#[repr(C)]
pub struct ListHead {
    pub next: *mut ListHead,
    pub prev: *mut ListHead,
}

#[repr(C)]
pub struct CcxCommonTimingCtx {
    pub pts_set: i32,          // 0 = No, 1 = received, 2 = min_pts set
    pub min_pts_adjusted: i32, // 0 = No, 1=Yes (don't adjust again)
    pub current_pts: i64,      // Assuming LLONG is equivalent to i64
    pub current_picture_coding_type: CcxFrameType,
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

#[repr(C)]
pub enum CcxCodeType {
    Any = 0,
    Teletext = 1,
    Dvb = 2,
    IsdbCc = 3,
    AtscCc = 4,
    None = 5,
}

#[repr(C)]
pub struct AvcCtx {
    pub cc_count: c_uchar,
    // buffer to hold cc data
    pub cc_data: *mut c_uchar,
    pub cc_databufsize: c_long,
    pub cc_buffer_saved: c_int, // Was the CC buffer saved after it was last updated?

    pub got_seq_para: c_int,
    pub nal_ref_idc: c_uint,
    pub seq_parameter_set_id: i64, // Assuming LLONG is equivalent to i64
    pub log2_max_frame_num: c_int,
    pub pic_order_cnt_type: c_int,
    pub log2_max_pic_order_cnt_lsb: c_int,
    pub frame_mbs_only_flag: c_int,

    // Use and throw stats for debug, remove this ugliness soon
    pub num_nal_unit_type_7: c_long,
    pub num_vcl_hrd: c_long,
    pub num_nal_hrd: c_long,
    pub num_jump_in_frames: c_long,
    pub num_unexpected_sei_length: c_long,

    pub ccblocks_in_avc_total: c_int,
    pub ccblocks_in_avc_lost: c_int,

    pub frame_num: i64,
    pub lastframe_num: i64,
    pub currref: c_int,
    pub maxidx: c_int,
    pub lastmaxidx: c_int,

    // Used to find tref zero in PTS mode
    pub minidx: c_int,
    pub lastminidx: c_int,

    // Used to remember the max temporal reference number (poc mode)
    pub maxtref: c_int,
    pub last_gop_maxtref: c_int,

    // Used for PTS ordering of CC blocks
    pub currefpts: i64,
    pub last_pic_order_cnt_lsb: i64,
    pub last_slice_pts: i64,
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
    pub timing: *mut CcxCommonTimingCtx,
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
pub struct CcxDecodersXdsContext {
    // Program Identification Number (Start Time) for current program
    pub current_xds_min: c_int,
    pub current_xds_hour: c_int,
    pub current_xds_date: c_int,
    pub current_xds_month: c_int,
    pub current_program_type_reported: c_int, // No.
    pub xds_start_time_shown: c_int,
    pub xds_program_length_shown: c_int,
    pub xds_program_description: [[c_uchar; 33]; 8],

    pub current_xds_network_name: [c_uchar; 33],
    pub current_xds_program_name: [c_uchar; 33],
    pub current_xds_call_letters: [c_uchar; 7],
    pub current_xds_program_type: [c_uchar; 33],

    pub xds_buffers: [XdsBuffer; NUM_XDS_BUFFERS],
    pub cur_xds_buffer_idx: c_int,
    pub cur_xds_packet_class: c_int,
    pub cur_xds_payload: *mut c_uchar,
    pub cur_xds_payload_length: c_int,
    pub cur_xds_packet_type: c_int,
    pub timing: *mut CcxCommonTimingCtx,

    pub current_ar_start: c_uint,
    pub current_ar_end: c_uint,

    pub xds_write_to_file: c_int, // Set to 1 if XDS data is to be written to file
}

#[repr(C)]
pub struct XdsBuffer {
    pub in_use: c_uint,
    pub xds_class: c_int,
    pub xds_type: c_int,
    pub bytes: [c_uchar; NUM_BYTES_PER_PACKET], // Class + type (repeated for convenience) + data + zero
    pub used_bytes: c_uchar,
}

#[repr(C)]
pub struct CcxDecoderVbiCtx {
    pub vbi_decoder_inited: c_int,
    pub zvbi_decoder: VbiRawDecoder,
    // vbi3_raw_decoder zvbi_decoder;
    pub vbi_debug_dump: *mut FILE,
}

#[derive(Copy, Clone)]
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

#[derive(Copy, Clone)]
#[repr(C)]
pub struct VbiRawDecoderJob {
    pub id: c_uint,
    pub offset: c_int,
    pub slicer: VbiBitSlicer,
}

#[derive(Debug, Clone, Copy)]
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

#[derive(Copy, Clone)]
#[repr(C)]
pub struct VbiBitSlicer {
    /// Function pointer for processing the bit slicer
    pub func:
        Option<unsafe extern "C" fn(slicer: *mut VbiBitSlicer, raw: *mut u8, buf: *mut u8) -> i32>,
    pub cri: u32,
    pub cri_mask: u32,
    pub thresh: i32,
    pub cri_bytes: i32,
    pub cri_rate: i32,
    pub oversampling_rate: i32,
    pub phase_shift: i32,
    pub step: i32,
    pub frc: u32,
    pub frc_bits: i32,
    pub payload: i32,
    pub endian: i32,
    pub skip: i32,
}

pub type VbiSamplingPar = VbiRawDecoder;
pub type VbiServiceSet = c_uint;
pub type VbiBool = c_int;
pub type VbiLogMask = c_uint;

// #[derive(Copy)]
#[repr(C)]
pub struct Vbi3RawDecoder {
    sampling: VbiSamplingPar,
    services: VbiServiceSet,
    log: VbiLogHook,
    debug: VbiBool,
    n_jobs: c_uint,
    n_sp_lines: c_uint,
    readjust: c_int,
    pattern: *mut c_int,
    jobs: [_Vbi3RawDecoderJob; _VBI3_RAW_DECODER_MAX_JOBS],
    sp_lines: *mut _Vbi3RawDecoderSpLine,
}


#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub struct VbiLogHook {
    fn_: Option<fn(*mut c_void, VbiLogMask, &str)>,
    user_data: *mut c_void,
    mask: VbiLogMask,
}

#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub struct _Vbi3RawDecoderJob {
    id: VbiServiceSet,
    slicer: Vbi3BitSlicer,
}

#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub struct Vbi3BitSlicer {
    pub func: Option<
        unsafe extern "C" fn(
            bs: *mut Vbi3BitSlicer,
            buffer: *mut u8,
            points: *mut Vbi3BitSlicerPoint,
            n_points: *mut u32,
            raw: *const u8,
        ) -> VbiBool,
    >,
    pub sample_format: VbiPixfmt,
    pub cri: u32,
    pub cri_mask: u32,
    pub thresh: u32,
    pub thresh_frac: u32,
    pub cri_samples: u32,
    pub cri_rate: u32,
    pub oversampling_rate: u32,
    pub phase_shift: u32,
    pub step: u32,
    pub frc: u32,
    pub frc_bits: u32,
    pub total_bits: u32,
    pub payload: u32,
    pub endian: u32,
    pub bytes_per_sample: u32,
    pub skip: u32,
    pub green_mask: u32,
    pub log: VbiLogHook,
}

#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub struct Vbi3BitSlicerPoint {
    kind: Vbi3BitSlicerBit,
    index: c_uint,
    level: c_uint,
    thresh: c_uint,
}

#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub enum Vbi3BitSlicerBit {
    CRI_BIT = 1,
    FRC_BIT,
    PAYLOAD_BIT,
}

#[repr(C)]
pub struct _Vbi3RawDecoderSpLine {
    points: [Vbi3BitSlicerPoint; 512],
    n_points: c_uint,
}

#[repr(C)]
pub enum CcxCommonLoggingGui {
    XdsProgramName,        // Called with xds_program_name
    XdsProgramIdNr,        // Called with current_xds_min, current_xds_hour, current_xds_date, current_xds_month
    XdsProgramDescription, // Called with line_num, xds_desc
    XdsCallLetters,        // Called with current_xds_call_letters
}

#[repr(C)]
pub struct CcxCommonLogging {
    pub debug_mask: i64, // Equivalent to LLONG in C
    pub fatal_ftn: Option<unsafe extern "C" fn(exit_code: i32, fmt: *const c_char, ...)>,
    pub debug_ftn: Option<unsafe extern "C" fn(mask: i64, fmt: *const c_char, ...)>,
    pub log_ftn: Option<unsafe extern "C" fn(fmt: *const c_char, ...)>,
    pub gui_ftn: Option<unsafe extern "C" fn(message_type: CcxCommonLoggingGui, ...)>,
}

#[repr(C)]
pub struct VbiServicePar {
    pub id: u32,
    pub label: *const c_char,

    /**
     * Video standard
     * - 525 lines, FV = 59.94 Hz, FH = 15734 Hz
     * - 625 lines, FV = 50 Hz, FH = 15625 Hz
     */
    pub videostd_set: u64,

    /**
     * Most scan lines used by the data service, first and last
     * line of first and second field. ITU-R numbering scheme.
     * Zero if no data from this field, requires field sync.
     */
    pub first: [u32; 2],
    pub last: [u32; 2],

    /**
     * Leading edge hsync to leading edge first CRI one bit,
     * half amplitude points, in nanoseconds.
     */
    pub offset: u32,

    pub cri_rate: u32, /**< Hz */
    pub bit_rate: u32, /**< Hz */

    /** Clock Run In and FRaming Code, LSB last txed bit of FRC. */
    pub cri_frc: u32,

    /** CRI and FRC bits significant for identification. */
    pub cri_frc_mask: u32,

    /**
     * Number of significant cri_bits (at cri_rate),
     * frc_bits (at bit_rate).
     */
    pub cri_bits: u32,
    pub frc_bits: u32,

    pub payload: u32, /**< bits */
    pub modulation: VbiModulation,

    pub flags: VbiServiceParFlag,
}

#[repr(C)]
#[derive(Clone)]
pub enum VbiModulation {
    NrzLsb,
    NrzMsb,
    BiphaseLsb,
    BiphaseMsb,
}

#[repr(C)]
#[derive(Copy, Clone)]

pub enum VbiServiceParFlag {
    LineNum = 1 << 0,
    FieldNum = 1 << 1,
    LineAndField = (1 << 0) | (1 << 1), // Bitwise OR of LineNum and FieldNum
}

#[repr(C)]
pub struct VbiSliced {
    pub id: u32,
    pub line: u32,
    pub data: [u8; 56],
}
