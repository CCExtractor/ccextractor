use crate::time::timing::*;

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

// Status codes
pub const CCX_OK: i64 = 0;
pub const CCX_FALSE: i64 = 0;
pub const CCX_TRUE: i64 = 1;
pub const CCX_EAGAIN: i64 = -100;
pub const CCX_EOF: i64 = -101;
pub const CCX_EINVAL: i64 = -102;
pub const CCX_ENOSUPP: i64 = -103;
pub const CCX_ENOMEM: i64 = -104;

// Define max width in characters/columns on the screen
pub const CCX_DECODER_608_SCREEN_ROWS: usize = 15;
pub const CCX_DECODER_608_SCREEN_WIDTH: usize = 32;

pub const NUM_BYTES_PER_PACKET: i64 = 35; // Class + type (repeated for convenience) + data + zero
pub const NUM_XDS_BUFFERS: i64 = 35; // CEA recommends no more than one level of interleaving. Play it safe

// Enums for format, color codes, font bits, and modes
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum CcxEia608Format {
    SformatCcScreen,
    SformatCcLine,
    SformatXds,
}

#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum CcxDecoder608ColorCode {
    ColWhite = 0,
    ColGreen = 1,
    ColBlue = 2,
    ColCyan = 3,
    ColRed = 4,
    ColYellow = 5,
    ColMagenta = 6,
    ColUserDefined = 7,
    ColBlack = 8,
    ColTransparent = 9,
}

#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum FontBits {
    FontRegular = 0,
    FontItalics = 1,
    FontUnderlined = 2,
    FontUnderlinedItalics = 3,
}

#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum CcModes {
    ModePopOn = 0,
    ModeRollUp2 = 1,
    ModeRollUp3 = 2,
    ModeRollUp4 = 3,
    ModeText = 4,
    ModePaintOn = 5,
    ModeFakeRollUp1 = 100, // Fake modes to emulate stuff
}

// The `Eia608Screen` structure
#[derive(Debug)]
pub struct Eia608Screen {
    pub format: CcxEia608Format, // Format of data inside this structure
    pub characters: [[u8; CCX_DECODER_608_SCREEN_WIDTH + 1]; CCX_DECODER_608_SCREEN_ROWS], // Characters
    pub colors:
        [[CcxDecoder608ColorCode; CCX_DECODER_608_SCREEN_WIDTH + 1]; CCX_DECODER_608_SCREEN_ROWS], // Colors
    pub fonts: [[FontBits; CCX_DECODER_608_SCREEN_WIDTH + 1]; CCX_DECODER_608_SCREEN_ROWS], // Fonts
    pub row_used: [i64; CCX_DECODER_608_SCREEN_ROWS], // Any data in row?
    pub empty: i64,                                   // Buffer completely empty?
    pub start_time: i64,                              // Start time of this CC buffer
    pub end_time: i64,                                // End time of this CC buffer
    pub mode: CcModes,                                // Mode
    pub channel: i64,                                 // Currently selected channel
    pub my_field: i64,                                // Used for sanity checks
    pub xds_str: *const u8,                           // Pointer to XDS string
    pub xds_len: usize,                               // Length of XDS string
    pub cur_xds_packet_class: i64,                    // Class of XDS string
}

#[derive(Debug, Copy, Clone, PartialEq, Eq, Default)]
pub enum SubDataType {
    #[default]
    Generic = 0,
    Dvb = 1,
}

#[derive(Debug, Copy, Clone, PartialEq, Eq, Default)]
pub enum SubType {
    #[default]
    Bitmap,
    Cc608,
    Text,
    Raw,
}

#[derive(Debug, Copy, Clone, PartialEq, Eq, Default)]
pub enum CcxEncodingType {
    #[default]
    Unicode = 0,
    Latin1 = 1,
    Utf8 = 2,
    Ascii = 3,
}

#[derive(Debug)]
pub struct CcSubtitle {
    /**
     * A generic data which contains data according to the decoder.
     * @warn Decoder can't output multiple types of data.
     */
    pub data: *mut std::ffi::c_void, // Pointer to generic data
    pub datatype: SubDataType, // Data type (e.g., Generic, DVB)

    /** Number of data */
    pub nb_data: u32,

    /** Type of subtitle */
    pub subtype: SubType,

    /** Encoding type of text, ignored for bitmap or cc_screen subtypes */
    pub enc_type: CcxEncodingType,

    /* Set only when all the data is to be displayed at the same time.
     * Unit of time is milliseconds.
     */
    pub start_time: i64,
    pub end_time: i64,

    /* Flags */
    pub flags: i32,

    /* Index of language table */
    pub lang_index: i32,

    /** Flag to tell that the decoder has given output */
    pub got_output: bool,

    pub mode: [u8; 5], // Mode as a fixed-size array of 5 bytes
    pub info: [u8; 4], // Info as a fixed-size array of 4 bytes

    /** Used for DVB end time in milliseconds */
    pub time_out: i32,

    pub next: *mut CcSubtitle, // Pointer to the next subtitle
    pub prev: *mut CcSubtitle, // Pointer to the previous subtitle
}

// XDS classes
pub const XDS_CLASSES: [&str; 8] = [
    "Current",
    "Future",
    "Channel",
    "Miscellaneous",
    "Public service",
    "Reserved",
    "Private data",
    "End",
];

// XDS program types
pub const XDS_PROGRAM_TYPES: [&str; 96] = [
    "Education",
    "Entertainment",
    "Movie",
    "News",
    "Religious",
    "Sports",
    "Other",
    "Action",
    "Advertisement",
    "Animated",
    "Anthology",
    "Automobile",
    "Awards",
    "Baseball",
    "Basketball",
    "Bulletin",
    "Business",
    "Classical",
    "College",
    "Combat",
    "Comedy",
    "Commentary",
    "Concert",
    "Consumer",
    "Contemporary",
    "Crime",
    "Dance",
    "Documentary",
    "Drama",
    "Elementary",
    "Erotica",
    "Exercise",
    "Fantasy",
    "Farm",
    "Fashion",
    "Fiction",
    "Food",
    "Football",
    "Foreign",
    "Fund-Raiser",
    "Game/Quiz",
    "Garden",
    "Golf",
    "Government",
    "Health",
    "High_School",
    "History",
    "Hobby",
    "Hockey",
    "Home",
    "Horror",
    "Information",
    "Instruction",
    "International",
    "Interview",
    "Language",
    "Legal",
    "Live",
    "Local",
    "Math",
    "Medical",
    "Meeting",
    "Military",
    "Mini-Series",
    "Music",
    "Mystery",
    "National",
    "Nature",
    "Police",
    "Politics",
    "Premiere",
    "Pre-Recorded",
    "Product",
    "Professional",
    "Public",
    "Racing",
    "Reading",
    "Repair",
    "Repeat",
    "Review",
    "Romance",
    "Science",
    "Series",
    "Service",
    "Shopping",
    "Soap_Opera",
    "Special",
    "Suspense",
    "Talk",
    "Technical",
    "Tennis",
    "Travel",
    "Variety",
    "Video",
    "Weather",
    "Western",
];

// XDS class constants
pub const XDS_CLASS_CURRENT: u8 = 0;
pub const XDS_CLASS_FUTURE: u8 = 1;
pub const XDS_CLASS_CHANNEL: u8 = 2;
pub const XDS_CLASS_MISC: u8 = 3;
pub const XDS_CLASS_PUBLIC: u8 = 4;
pub const XDS_CLASS_RESERVED: u8 = 5;
pub const XDS_CLASS_PRIVATE: u8 = 6;
pub const XDS_CLASS_END: u8 = 7;
pub const XDS_CLASS_OUT_OF_BAND: u8 = 0x40; // Not a real class, a marker for packets for out-of-band data

// Types for the classes current and future
pub const XDS_TYPE_PIN_START_TIME: u8 = 1;
pub const XDS_TYPE_LENGTH_AND_CURRENT_TIME: u8 = 2;
pub const XDS_TYPE_PROGRAM_NAME: u8 = 3;
pub const XDS_TYPE_PROGRAM_TYPE: u8 = 4;
pub const XDS_TYPE_CONTENT_ADVISORY: u8 = 5;
pub const XDS_TYPE_AUDIO_SERVICES: u8 = 6;
pub const XDS_TYPE_CGMS: u8 = 8; // Copy Generation Management System
pub const XDS_TYPE_ASPECT_RATIO_INFO: u8 = 9; // Appears in CEA-608-B but in E it's been removed as is "reserved"
pub const XDS_TYPE_PROGRAM_DESC_1: u8 = 0x10;
pub const XDS_TYPE_PROGRAM_DESC_2: u8 = 0x11;
pub const XDS_TYPE_PROGRAM_DESC_3: u8 = 0x12;
pub const XDS_TYPE_PROGRAM_DESC_4: u8 = 0x13;
pub const XDS_TYPE_PROGRAM_DESC_5: u8 = 0x14;
pub const XDS_TYPE_PROGRAM_DESC_6: u8 = 0x15;
pub const XDS_TYPE_PROGRAM_DESC_7: u8 = 0x16;
pub const XDS_TYPE_PROGRAM_DESC_8: u8 = 0x17;

// Types for the class channel
pub const XDS_TYPE_NETWORK_NAME: u8 = 1;
pub const XDS_TYPE_CALL_LETTERS_AND_CHANNEL: u8 = 2;
pub const XDS_TYPE_TSID: u8 = 4; // Transmission Signal Identifier

// Types for miscellaneous packets
pub const XDS_TYPE_TIME_OF_DAY: u8 = 1;
pub const XDS_TYPE_LOCAL_TIME_ZONE: u8 = 4;
pub const XDS_TYPE_OUT_OF_BAND_CHANNEL_NUMBER: u8 = 0x40;

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct XdsBuffer {
    pub in_use: i64,                                // Whether the buffer is in use
    pub xds_class: i64,                             // XDS class (e.g., XDS_CLASS_CURRENT, etc.)
    pub xds_type: i64,                              // XDS type (e.g., XDS_TYPE_PROGRAM_NAME, etc.)
    pub bytes: [u8; NUM_BYTES_PER_PACKET as usize], // Data bytes (size defined by NUM_BYTES_PER_PACKET)
    pub used_bytes: i64,                            // Number of bytes used in the buffer
}

#[repr(C)]
pub struct CcxDecodersXdsContext {
    // Program Identification Number (Start Time) for current program
    pub current_xds_min: i64,
    pub current_xds_hour: i64,
    pub current_xds_date: i64,
    pub current_xds_month: i64,
    pub current_program_type_reported: i64, // No.
    pub xds_start_time_shown: i64,
    pub xds_program_length_shown: i64,
    pub xds_program_description: [[u8; 33]; 8], // 8 strings of 33 bytes each

    pub current_xds_network_name: [u8; 33], // String of 33 bytes
    pub current_xds_program_name: [u8; 33], // String of 33 bytes
    pub current_xds_call_letters: [u8; 7],  // String of 7 bytes
    pub current_xds_program_type: [u8; 33], // String of 33 bytes

    pub xds_buffers: [XdsBuffer; NUM_XDS_BUFFERS as usize], // Array of XdsBuffer
    pub cur_xds_buffer_idx: i64,
    pub cur_xds_packet_class: i64,
    pub cur_xds_payload: *mut u8, // Pointer to payload
    pub cur_xds_payload_length: i64,
    pub cur_xds_packet_type: i64,
    pub timing: TimingContext, // Replacing ccx_common_timing_ctx with TimingContext

    pub current_ar_start: i64,
    pub current_ar_end: i64,

    pub xds_write_to_file: i64, // Set to 1 if XDS data is to be written to file
}

impl Default for CcxDecodersXdsContext {
    fn default() -> Self {
        Self {
            current_xds_min: -1,
            current_xds_hour: -1,
            current_xds_date: -1,
            current_xds_month: -1,
            current_program_type_reported: 0,
            xds_start_time_shown: 0,
            xds_program_length_shown: 0,
            xds_program_description: [[0; 33]; 8],
            current_xds_network_name: [0; 33],
            current_xds_program_name: [0; 33],
            current_xds_call_letters: [0; 7],
            current_xds_program_type: [0; 33],
            xds_buffers: [XdsBuffer {
                in_use: 0,
                xds_class: -1,
                xds_type: -1,
                bytes: [0; NUM_BYTES_PER_PACKET as usize],
                used_bytes: 0,
            }; NUM_XDS_BUFFERS as usize],
            cur_xds_buffer_idx: -1,
            cur_xds_packet_class: -1,
            cur_xds_payload: std::ptr::null_mut(),
            cur_xds_payload_length: 0,
            cur_xds_packet_type: 0,
            timing: TimingContext::default(),
            current_ar_start: 0,
            current_ar_end: 0,
            xds_write_to_file: 0,
        }
    }
}

impl Default for XdsBuffer {
    fn default() -> Self {
        Self {
            in_use: 0,
            xds_class: -1,
            xds_type: -1,
            bytes: [0; NUM_BYTES_PER_PACKET as usize],
            used_bytes: 0,
        }
    }
}

impl Default for CcSubtitle {
    fn default() -> Self {
        Self {
            data: std::ptr::null_mut(),
            datatype: SubDataType::Generic,
            nb_data: 0,
            subtype: SubType::Cc608,
            enc_type: CcxEncodingType::Utf8,
            start_time: 0,
            end_time: 0,
            flags: 0,
            lang_index: 0,
            got_output: false,
            mode: [0; 5],
            info: [0; 4],
            time_out: 0,
            next: std::ptr::null_mut(),
            prev: std::ptr::null_mut(),
        }
    }
}
