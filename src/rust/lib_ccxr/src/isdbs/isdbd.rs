// command-line bindgen used

use std::os::raw::{c_char, c_int, c_long, c_uchar, c_uint, c_ulonglong, c_void};

#[allow(dead_code)]

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

// Define the necessary enums and structs
#[repr(C)]
pub enum CcxOutputFormat {
    Raw = 0,
    Srt = 1,
    Sami = 2,
    Transcript = 3,
    Rcwt = 4,
    Null = 5,
    Smptett = 6,
    Spupng = 7,
    DvdRaw = 8,
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
pub struct CcxBoundaryTime {
    pub hh: i32,
    pub mm: i32,
    pub ss: i32,
    pub time_in_ms: i64,
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

const NUM_BYTES_PER_PACKET: usize = 35;
const NUM_XDS_BUFFERS: usize = 9;

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
    // #ifdef VBI_DEBUG
    // pub vbi_debug_dump: *mut FILE,
    // #endif
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

#[derive(Debug, Clone)]
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

#[derive(Debug, Clone)]
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

#[derive(Debug, Clone)]
#[repr(C)]
pub struct Spacing {
    pub col: c_int,
    pub row: c_int,
}

#[derive(Debug, Clone)]
#[repr(C)]
pub struct ISDBPos {
    pub x: c_int,
    pub y: c_int,
}

#[derive(Debug, Clone)]
#[repr(C)]
pub struct OffsetTime {
    pub hour: c_int,
    pub min: c_int,
    pub sec: c_int,
    pub milli: c_int,
}

#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub enum WritingFormat {
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

#[derive(Debug, Clone)]
#[repr(C)]
pub enum IsdbCCComposition {
    None = 0,
    And = 2,
    Or = 3,
    Xor = 4,
}

#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub enum IsdbTmd {
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

#[repr(i32)]
pub enum CcxStreamMode {
    ElementaryOrNotFound = 0,
    Transport = 1,
    Program = 2,
    Asf = 3,
    McPoodlesRaw = 4,
    Rcwt = 5, // Raw Captions With Time, not used yet.
    Myth = 6, // Use the myth loop
    Mp4 = 7,  // MP4, ISO-
    Wtv = 9,
    Gxf = 11,
    Mkv = 12,
    Mxf = 13,
    Autodetect = 16,
}

#[repr(i32)]
pub enum CcxOutputDateFormat {
    None = 0,
    Hhmmss = 1,
    Seconds = 2,
    Date = 3,
    Hhmmssms = 4, // HH:MM:SS,MILIS (.srt style)
}

pub struct CcxEncodersTranscriptFormat {
    show_start_time: i32,    // Show start and/or end time.
    show_end_time: i32,      // Show start and/or end time.
    show_mode: i32,          // Show which mode if available (E.G.: POP, RU1, ...)
    show_cc: i32,            // Show which CC channel has been captured.
    relative_timestamp: i32, // Timestamps relative to start of sample or in UTC?
    xds: i32,                // Show XDS or not
    use_colors: i32,         // Add colors or no colors
    is_final: i32,           // Used to determine if these parameters should be changed afterwards.
}

#[repr(i32)]
pub enum CcxDataSource {
    File = 0,
    Stdin = 1,
    Network = 2,
    Tcp = 3,
}

#[repr(C)]
pub struct CcxSOptions {
    pub extract: i32,
    pub no_rollup: i32,
    pub noscte20: i32,
    pub webvtt_create_css: i32,
    pub cc_channel: i32,
    pub buffer_input: i32,
    pub nofontcolor: i32,
    pub nohtmlescape: i32,
    pub notypesetting: i32,
    pub extraction_start: CcxBoundaryTime,
    pub extraction_end: CcxBoundaryTime,
    pub print_file_reports: i32,
    pub settings_608: CcxDecoder608Settings,
    pub settings_dtvcc: CcxDecoderDtvccSettings,
    pub is_608_enabled: i32,
    pub is_708_enabled: i32,
    pub millis_separator: c_char,
    pub binary_concat: i32,
    pub use_gop_as_pts: i32,
    pub fix_padding: i32,
    pub gui_mode_reports: i32,
    pub no_progress_bar: i32,
    pub sentence_cap_file: *mut c_char,
    pub live_stream: i32,
    pub filter_profanity_file: *mut c_char,
    pub messages_target: i32,
    pub timestamp_map: i32,
    pub dolevdist: i32,
    pub levdistmincnt: i32,
    pub levdistmaxpct: i32,
    pub investigate_packets: i32,
    pub fullbin: i32,
    pub nosync: i32,
    pub hauppauge_mode: u32,
    pub wtvconvertfix: i32,
    pub wtvmpeg2: i32,
    pub auto_myth: i32,
    pub mp4vidtrack: u32,
    pub extract_chapters: i32,
    pub usepicorder: i32,
    pub xmltv: i32,
    pub xmltvliveinterval: i32,
    pub xmltvoutputinterval: i32,
    pub xmltvonlycurrent: i32,
    pub keep_output_closed: i32,
    pub force_flush: i32,
    pub append_mode: i32,
    pub ucla: i32,
    pub tickertext: i32,
    pub hardsubx: i32,
    pub hardsubx_and_common: i32,
    pub dvblang: *mut c_char,
    pub ocrlang: *const c_char,
    pub ocr_oem: i32,
    pub psm: i32,
    pub ocr_quantmode: i32,
    pub mkvlang: *mut c_char,
    pub analyze_video_stream: i32,
    pub hardsubx_ocr_mode: i32,
    pub hardsubx_subcolor: i32,
    pub hardsubx_min_sub_duration: f32,
    pub hardsubx_detect_italics: i32,
    pub hardsubx_conf_thresh: f32,
    pub hardsubx_hue: f32,
    pub hardsubx_lum_thresh: f32,
    pub transcript_settings: CcxEncodersTranscriptFormat,
    pub date_format: CcxOutputDateFormat,
    pub send_to_srv: u32,
    pub write_format: CcxOutputFormat,
    pub write_format_rewritten: i32,
    pub use_ass_instead_of_ssa: i32,
    pub use_webvtt_styling: i32,
    pub debug_mask: i64,
    pub debug_mask_on_debug: i64,
    pub udpsrc: *mut c_char,
    pub udpaddr: *mut c_char,
    pub udpport: u32,
    pub tcpport: *mut c_char,
    pub tcp_password: *mut c_char,
    pub tcp_desc: *mut c_char,
    pub srv_addr: *mut c_char,
    pub srv_port: *mut c_char,
    pub noautotimeref: i32,
    pub input_source: CcxDatasource,
    pub output_filename: *mut c_char,
    pub inputfile: *mut *mut c_char,
    pub num_input_files: i32,
    pub demux_cfg: DemuxerCfg,
    pub enc_cfg: EncoderCfg,
    pub subs_delay: i64,
    pub cc_to_stdout: i32,
    pub pes_header_to_stdout: i32,
    pub ignore_pts_jumps: i32,
    pub multiprogram: i32,
    pub out_interval: i32,
    pub segment_on_key_frames_only: i32,
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
pub enum CcxDecoder608ColorCode {
    ColWhite = 0,
    ColGreen = 1,
    ColBlue = 2,
    ColCyan = 3,
    ColRed = 4,
    ColYellow = 5,
    ColMagenta = 6,
    ColUserdefined = 7,
    ColBlack = 8,
    ColTransparent = 9,
    ColMax,
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
    pub timing: *mut CcxCommonTimingCtx,
}

#[repr(C)]
pub enum CcxDatasource {
    File = 0,
    Stdin = 1,
    Network = 2,
    Tcp = 3,
}

#[repr(C)]
pub struct DemuxerCfg {
    pub m2ts: i32,
    pub auto_stream: CcxStreamModeEnum,
    pub codec: CcxCodeType,
    pub nocodec: CcxCodeType,
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
    pub write_format: CcxOutputFormat,
    pub keep_output_closed: i32,
    pub force_flush: i32,
    pub append_mode: i32,
    pub ucla: i32,
    pub encoding: CcxEncodingType,
    pub date_format: CcxOutputDateFormat,
    pub millis_separator: c_char,
    pub autodash: i32,
    pub trim_subs: i32,
    pub sentence_cap: i32,
    pub splitbysentence: i32,
    pub filter_profanity: i32,
    pub with_semaphore: i32,
    pub start_credits_text: *mut c_char,
    pub end_credits_text: *mut c_char,
    pub startcreditsnotbefore: CcxBoundaryTime,
    pub startcreditsnotafter: CcxBoundaryTime,
    pub startcreditsforatleast: CcxBoundaryTime,
    pub startcreditsforatmost: CcxBoundaryTime,
    pub endcreditsforatleast: CcxBoundaryTime,
    pub endcreditsforatmost: CcxBoundaryTime,
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

#[repr(C)]
pub enum CcxStreamModeEnum {
    ElementaryOrNotFound = 0,
    Transport = 1,
    Program = 2,
    Asf = 3,
    McPoodlesRaw = 4,
    Rcwt = 5,
    Myth = 6,
    Mp4 = 7,
    Wtv = 9,
    Gxf = 11,
    Mkv = 12,
    Mxf = 13,
    Autodetect = 16,
}

pub struct CcxOptions {
    pub messages_target: i32,
}
