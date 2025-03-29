// codes for do_end_of_xds
pub const XDS_CLASS_FUTURE: i64 = 1;
pub const XDS_CLASS_CHANNEL: i64 = 2;
pub const XDS_CLASS_MISC: i64 = 3;
pub const XDS_CLASS_PRIVATE: i64 = 6;
pub const XDS_CLASS_OUT_OF_BAND: i64 = 0x40;

// codes for xds_do_misc
pub const XDS_TYPE_TIME_OF_DAY: i64 = 1;
pub const XDS_TYPE_LOCAL_TIME_ZONE: i64 = 4;

// codes for xds_do_channel
pub const XDS_TYPE_NETWORK_NAME: i64 = 1;
pub const XDS_TYPE_CALL_LETTERS_AND_CHANNEL: i64 = 2;
pub const XDS_TYPE_TSID: i64 = 4;

// codes for xds_do_current_and_future
pub const XDS_CLASS_CURRENT: i64 = 0;

pub const XDS_TYPE_PIN_START_TIME: i64 = 1;
pub const XDS_TYPE_LENGTH_AND_CURRENT_TIME: i64 = 2;
pub const XDS_TYPE_PROGRAM_NAME: i64 = 3;
pub const XDS_TYPE_PROGRAM_TYPE: i64 = 4;
pub const XDS_TYPE_CONTENT_ADVISORY: i64 = 5;
pub const XDS_TYPE_AUDIO_SERVICES: i64 = 6;
pub const XDS_TYPE_CGMS: i64 = 8;
pub const XDS_TYPE_ASPECT_RATIO_INFO: i64 = 9;

pub const XDS_TYPE_PROGRAM_DESC_1: i64 = 0x10;
pub const XDS_TYPE_PROGRAM_DESC_8: i64 = 0x17;

// codes for write_xds_string
pub const TS_START_OF_XDS: i64 = -1; // Time at which we switched to XDS mode, =-1 hasn't happened yet

// codes for Eia608Screen::default
pub const CCX_DECODER_608_SCREEN_ROWS: usize = 15;
pub const CCX_DECODER_608_SCREEN_WIDTH: usize = 32;

// codes for CcxDecodersXdsContext::default
pub const NUM_XDS_BUFFERS: usize = 9;

// codes for XdsBuffer::default
pub const NUM_BYTES_PER_PACKET: usize = 35;
