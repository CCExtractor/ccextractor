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
pub const NUM_XDS_BUFFERS: i64 = 35; // CEA recommends no more than one level of interleaving. Play it safe

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

//isdb specific codes
pub const CCX_DTVCC_MAX_SERVICES: usize = 63;
pub const CCX_DTVCC_MAX_WINDOWS: usize = 8;
pub const CCX_DTVCC_MAX_ROWS: usize = 15;
pub const CCX_DTVCC_SCREENGRID_COLUMNS: usize = 210;
pub const CCX_DTVCC_MAX_PACKET_LENGTH: usize = 128;
pub const CCX_DTVCC_SCREENGRID_ROWS: usize = 75;

pub const CCX_MESSAGES_QUIET: i32 = 0;
pub const CCX_MESSAGES_STDOUT: i32 = 1;
pub const CCX_MESSAGES_STDERR: i32 = 2;
