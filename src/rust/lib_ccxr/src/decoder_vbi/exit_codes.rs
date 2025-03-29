// Status codes
pub const CCX_OK: i64 = 0;
pub const CCX_FALSE: i64 = 0;
pub const CCX_TRUE: i64 = 1;
pub const CCX_EAGAIN: i64 = -100;
pub const CCX_EOF: i64 = -101;
pub const CCX_EINVAL: i64 = -102;
pub const CCX_ENOSUPP: i64 = -103;
pub const CCX_ENOMEM: i64 = -104;

pub const NUM_BYTES_PER_PACKET: i64 = 35; // Class + type (repeated for convenience) + data + zero
pub const NUM_XDS_BUFFERS: i64 = 9; // CEA recommends no more than one level of interleaving. Play it safe

pub const MAXBFRAMES: usize = 50;
pub const SORTBUF: usize = 2 * MAXBFRAMES + 1;

// Time at which we switched to XDS mode, -1 means it hasn't happened yet
pub const TS_START_OF_XDS: i64 = -1;

// Exit codes
pub const EXIT_OK: i64 = 0;
pub const EXIT_NO_INPUT_FILES: i64 = 2;
pub const EXIT_TOO_MANY_INPUT_FILES: i64 = 3;
pub const EXIT_INCOMPATIBLE_PARAMETERS: i64 = 4;
pub const EXIT_UNABLE_TO_DETERMINE_FILE_SIZE: i64 = 6;
pub const EXIT_MALFORMED_PARAMETER: i64 = 7;
pub const EXIT_READ_ERROR: i64 = 8;
pub const EXIT_NO_CAPTIONS: i64 = 10;
pub const EXIT_WITH_HELP: i64 = 11;
pub const EXIT_NOT_CLASSIFIED: i64 = 300;
pub const EXIT_ERROR_IN_CAPITALIZATION_FILE: i64 = 501;
pub const EXIT_BUFFER_FULL: i64 = 502;
pub const EXIT_MISSING_ASF_HEADER: i64 = 1001;
pub const EXIT_MISSING_RCWT_HEADER: i64 = 1002;

// Common exit codes
pub const CCX_COMMON_EXIT_FILE_CREATION_FAILED: i64 = 5;
pub const CCX_COMMON_EXIT_UNSUPPORTED: i64 = 9;
pub const EXIT_NOT_ENOUGH_MEMORY: i64 = 500;
pub const CCX_COMMON_EXIT_BUG_BUG: i64 = 1000;

// Define max width in characters/columns on the screen
pub const CCX_DECODER_608_SCREEN_ROWS: usize = 15;
pub const CCX_DECODER_608_SCREEN_WIDTH: usize = 32;

//isdb, vbi common codes
pub const CCX_DTVCC_MAX_SERVICES: usize = 63;
pub const CCX_DTVCC_MAX_WINDOWS: usize = 8;
pub const CCX_DTVCC_MAX_ROWS: usize = 15;
pub const CCX_DTVCC_SCREENGRID_COLUMNS: usize = 210;
pub const CCX_DTVCC_MAX_PACKET_LENGTH: usize = 128;
pub const CCX_DTVCC_SCREENGRID_ROWS: usize = 75;

pub const CCX_MESSAGES_QUIET: i32 = 0;
pub const CCX_MESSAGES_STDOUT: i32 = 1;
pub const CCX_MESSAGES_STDERR: i32 = 2;

// vbi specific codes

pub const _VBI3_RAW_DECODER_MAX_JOBS: usize = 8;

pub const VBI_SLICED_CAPTION_525_F1: u32 = 0x00000020;
pub const VBI_SLICED_CAPTION_525_F2: u32 = 0x00000040;
pub const VBI_SLICED_CAPTION_525: u32 = VBI_SLICED_CAPTION_525_F1 | VBI_SLICED_CAPTION_525_F2;

pub const VBI_SLICED_VBI_525: u32 = 0x40000000;
pub const VBI_SLICED_VBI_625: u32 = 0x20000000;
pub const _VBI3_RAW_DECODER_MAX_WAYS: usize = 8;

pub const VBI_SLICED_TELETEXT_B_L10_625: u32 = 0x00000001;
pub const VBI_SLICED_TELETEXT_B_L25_625: u32 = 0x00000002;
pub const VBI_SLICED_TELETEXT_B: u32 =
    VBI_SLICED_TELETEXT_B_L10_625 | VBI_SLICED_TELETEXT_B_L25_625;

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
pub const VBI_SLICED_2XCAPTION_525: u32 = 0x00000080; // VBI_SLICED_2xCAPTION_525

pub const DEF_THR_FRAC: u32 = 9;
pub const LP_AVG: u32 = 4;
pub const RAW_DECODER_PATTERN_DUMP: bool = false;
pub const VBI_SLICED_TELETEXT_BD_525: u32 = 0x00000200;
