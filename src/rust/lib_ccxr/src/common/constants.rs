use std::ffi::OsStr;

use strum_macros::{EnumString, FromRepr};

pub const DTVCC_MAX_SERVICES: usize = 63;

/// An enum of all the available formats for the subtitle output.
#[derive(Default, Copy, Clone, Debug, PartialEq, Eq)]
pub enum OutputFormat {
    #[default]
    Raw,
    Srt,
    Sami,
    Transcript,
    Rcwt,
    Null,
    SmpteTt,
    SpuPng,
    DvdRaw, // See -d at http://www.theneitherworld.com/mcpoodle/SCC_TOOLS/DOCS/SCC_TOOLS.HTML#CCExtract
    WebVtt,
    SimpleXml,
    G608,
    Curl,
    Ssa,
    Mcc,
    Scc,
    Ccd,
}
// Provides common constant types throughout the codebase.
// Rust equivalent for `ccx_common_constants.c` file in C.
//
// # Conversion Guide
//
// | From                           | To                                         |
// |--------------------------------|--------------------------------------------|
// | `ccx_avc_nal_types`            | [`AvcNalType`]                             |
// | `ccx_stream_type`              | [`StreamType`]                             |
// | `ccx_mpeg_descriptor`          | [`MpegDescriptor`]                         |
// | `ccx_datasource`               | [`DataSource`]                             |
// | `ccx_output_format`            | [`OutputFormat`]                           |
// | `ccx_stream_mode_enum`         | [`StreamMode`]                             |
// | `ccx_bufferdata_type`          | [`BufferdataType`]                         |
// | `ccx_frame_type`               | [`FrameType`]                              |
// | `ccx_code_type`                | [`Codec`]                                  |
// | `cdp_section_type`             | [`CdpSectionType`]                         |
// | `cc_types[4]`                  | [`CCTypes`]                                |
// | `CCX_TXT_*` macros             | [`CcxTxt`]                                 |
// | `language[NB_LANGUAGE]`        | [`Language`]                               |
// | `DEF_VAL_*` macros             | [`CreditTiming`]                           |
// | `IS_FEASIBLE` macro            | [`Codec::is_feasible`]                     |
// | `IS_VALID_TELETEXT_DESC` macro | [`MpegDescriptor::is_valid_teletext_desc`] |

// RCWT header (11 bytes):
// byte(s)   value   description (All values below are hex numbers, not
//                  actual numbers or values
// 0-2       CCCCED  magic number, for Closed Caption CC Extractor Data
// 3         CC      Creating program.  Legal values: CC = CC Extractor
// 4-5       0050    Program version number
// 6-7       0001    File format version
// 8-10      000000  Padding, required  :-)
pub static mut RCWT_HEADER: [u8; 11] = [0xCC, 0xCC, 0xED, 0xCC, 0x00, 0x50, 0, 1, 0, 0, 0];

pub const BROADCAST_HEADER: [u8; 4] = [0xff, 0xff, 0xff, 0xff];
pub const LITTLE_ENDIAN_BOM: [u8; 2] = [0xff, 0xfe];
pub const UTF8_BOM: [u8; 3] = [0xef, 0xbb, 0xbf];
pub const DVD_HEADER: [u8; 8] = [0x00, 0x00, 0x01, 0xb2, 0x43, 0x43, 0x01, 0xf8];
pub const LC1: [u8; 1] = [0x8a];
pub const LC2: [u8; 1] = [0x8f];
pub const LC3: [u8; 2] = [0x16, 0xfe];
pub const LC4: [u8; 2] = [0x1e, 0xfe];
pub const LC5: [u8; 1] = [0xff];
pub const LC6: [u8; 1] = [0xfe];

pub const FRAMERATES_VALUES: [f64; 16] = [
    0.0,
    24000.0 / 1001.0, // 23.976
    24.0,
    25.0,
    30000.0 / 1001.0, // 29.97
    30.0,
    50.0,
    60000.0 / 1001.0, // 59.94
    60.0,
    0.0,
    0.0,
    0.0,
    0.0,
    0.0,
    0.0,
    0.0,
];

pub const FRAMERATES_TYPES: [&str; 16] = [
    "00 - forbidden",
    "01 - 23.976",
    "02 - 24",
    "03 - 25",
    "04 - 29.97",
    "05 - 30",
    "06 - 50",
    "07 - 59.94",
    "08 - 60",
    "09 - reserved",
    "10 - reserved",
    "11 - reserved",
    "12 - reserved",
    "13 - reserved",
    "14 - reserved",
    "15 - reserved",
];

pub const ASPECT_RATIO_TYPES: [&str; 16] = [
    "00 - forbidden",
    "01 - 1:1",
    "02 - 4:3",
    "03 - 16:9",
    "04 - 2.21:1",
    "05 - reserved",
    "06 - reserved",
    "07 - reserved",
    "08 - reserved",
    "09 - reserved",
    "10 - reserved",
    "11 - reserved",
    "12 - reserved",
    "13 - reserved",
    "14 - reserved",
    "15 - reserved",
];

pub const PICT_TYPES: [&str; 8] = [
    "00 - illegal (0)",
    "01 - I",
    "02 - P",
    "03 - B",
    "04 - illegal (D)",
    "05 - illegal (5)",
    "06 - illegal (6)",
    "07 - illegal (7)",
];

pub const SLICE_TYPES: [&str; 10] = [
    "0 - P", "1 - B", "2 - I", "3 - SP", "4 - SI", "5 - P", "6 - B", "7 - I", "8 - SP", "9 - SI",
];

pub const CCX_DECODER_608_SCREEN_WIDTH: usize = 32;
pub const ONEPASS: usize = 120; // Bytes we can always look ahead without going out of limits
pub const BUFSIZE: usize = 2048 * 1024 + ONEPASS; // 2 Mb plus the safety pass
pub const MAX_CLOSED_CAPTION_DATA_PER_PICTURE: usize = 32;
pub const EIA_708_BUFFER_LENGTH: usize = 2048; // TODO: Find out what the real limit is
pub const TS_PACKET_PAYLOAD_LENGTH: usize = 184; // From specs
pub const SUBLINESIZE: usize = 2048; // Max. length of a .srt line - TODO: Get rid of this
pub const STARTBYTESLENGTH: usize = 1024 * 1024;
pub const UTF8_MAX_BYTES: usize = 6;
pub const XMLRPC_CHUNK_SIZE: usize = 64 * 1024; // 64 Kb per chunk, to avoid too many realloc()

// AVC NAL types
#[derive(Debug, PartialEq, Eq)]
pub enum AvcNalType {
    Unspecified0 = 0,
    CodedSliceNonIdrPicture1 = 1,
    CodedSlicePartitionA = 2,
    CodedSlicePartitionB = 3,
    CodedSlicePartitionC = 4,
    CodedSliceIdrPicture = 5,
    Sei = 6,
    SequenceParameterSet7 = 7,
    PictureParameterSet = 8,
    AccessUnitDelimiter9 = 9,
    EndOfSequence = 10,
    EndOfStream = 11,
    FillerData = 12,
    SequenceParameterSetExtension = 13,
    PrefixNalUnit = 14,
    SubsetSequenceParameterSet = 15,
    Reserved16 = 16,
    Reserved17 = 17,
    Reserved18 = 18,
    CodedSliceAuxiliaryPicture = 19,
    CodedSliceExtension = 20,
    Reserved21 = 21,
    Reserved22 = 22,
    Reserved23 = 23,
    Unspecified24 = 24,
    Unspecified25 = 25,
    Unspecified26 = 26,
    Unspecified27 = 27,
    Unspecified28 = 28,
    Unspecified29 = 29,
    Unspecified30 = 30,
    Unspecified31 = 31,
}

// MPEG-2 TS stream types
#[derive(Default, Debug, PartialEq, Eq, FromRepr, Clone, Copy)]
pub enum StreamType {
    #[default]
    Unknownstream = 0,
    /*
    The later constants are defined by MPEG-TS standard
    Explore at: https://exiftool.org/TagNames/M2TS.html
    */
    VideoMpeg1 = 0x01,
    VideoMpeg2 = 0x02,
    AudioMpeg1 = 0x03,
    AudioMpeg2 = 0x04,
    PrivateTableMpeg2 = 0x05,
    PrivateMpeg2 = 0x06,
    MhegPackets = 0x07,
    Mpeg2AnnexADsmCc = 0x08,
    ItuTH222_1 = 0x09,
    IsoIec13818_6TypeA = 0x0a,
    IsoIec13818_6TypeB = 0x0b,
    IsoIec13818_6TypeC = 0x0c,
    IsoIec13818_6TypeD = 0x0d,
    AudioAac = 0x0f,
    VideoMpeg4 = 0x10,
    VideoH264 = 0x1b,
    PrivateUserMpeg2 = 0x80,
    AudioAc3 = 0x81,
    AudioHdmvDts = 0x82,
    AudioDts = 0x8a,
}

pub enum MpegDescriptor {
    /*
    The later constants are defined by ETSI EN 300 468 standard
    Explore at: https://www.etsi.org/deliver/etsi_en/300400_300499/300468/01.11.01_60/en_300468v011101p.pdf
    */
    Registration = 0x05,
    DataStreamAlignment = 0x06,
    Iso639Language = 0x0a,
    VbiDataDescriptor = 0x45,
    VbiTeletextDescriptor = 0x46,
    TeletextDescriptor = 0x56,
    DvbSubtitle = 0x59,
    /* User defined */
    CaptionService = 0x86,
    DataComp = 0xfd, // Consider to change DESC to DSC
}

#[derive(Default, Debug, PartialEq, Eq, Clone, Copy)]
pub enum DataSource {
    #[default]
    File,
    Stdin,
    Network,
    Tcp,
}
impl From<u32> for DataSource {
    fn from(value: u32) -> Self {
        match value {
            0 => DataSource::File,
            1 => DataSource::Stdin,
            2 => DataSource::Network,
            3 => DataSource::Tcp,
            _ => DataSource::File, // Default or fallback case
        }
    }
}

#[derive(Default, Debug, PartialEq, Eq, Clone, Copy)]
pub enum StreamMode {
    #[default]
    ElementaryOrNotFound = 0,
    Transport = 1,
    Program = 2,
    Asf = 3,
    McpoodlesRaw = 4,
    Rcwt = 5, // Raw Captions With Time, not used yet.
    Myth = 6, // Use the myth loop
    Mp4 = 7,  // MP4, ISO-
    #[cfg(feature = "wtv_debug")]
    HexDump = 8, // Hexadecimal dump generated by wtvccdump
    Wtv = 9,
    #[cfg(feature = "enable_ffmpeg")]
    Ffmpeg = 10,
    Gxf = 11,
    Mkv = 12,
    Mxf = 13,
    Autodetect = 16,
}
#[derive(Debug, Eq, Clone, Copy)]
pub enum BufferdataType {
    Unknown,
    Pes,
    Raw,
    H264,
    Hauppage,
    Teletext,
    PrivateMpeg2Cc,
    DvbSubtitle,
    IsdbSubtitle,
    /* Buffer where cc data contain 3 byte cc_valid ccdata 1 ccdata 2 */
    RawType,
    DvdSubtitle,
}

#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub enum Codec {
    Any,
    Teletext,
    Dvb,
    IsdbCc,
    AtscCc,
}

#[derive(Default, Debug, PartialEq, Eq, Clone, Copy)]
pub enum SelectCodec {
    All,
    Some(Codec),
    #[default]
    None,
}

/// Caption Distribution Packet
pub enum CdpSectionType {
    /*
    The later constants are defined by SMPTE ST 334
    Purchase for 80$ at: https://ieeexplore.ieee.org/document/8255806
    */
    Data = 0x72,
    SvcInfo = 0x73,
    Footer = 0x74,
}

pub enum CCTypes {
    NtscCCF1,
    NtscCCF2,
    DtvccPacketData,
    DtvccPacketStart,
}

pub enum CcxTxt {
    Forbidden = 0, // Ignore teletext packets
    AutoNotYetFound = 1,
    InUse = 2, // Positive auto-detected, or forced, etc
}

#[derive(Debug, Default, EnumString, Clone, Copy, PartialEq, Eq)]
#[strum(serialize_all = "lowercase")]
pub enum Language {
    #[default]
    Und, // Undefined
    Eng,
    Afr,
    Amh,
    Ara,
    Asm,
    Aze,
    Bel,
    Ben,
    Bod,
    Bos,
    Bul,
    Cat,
    Ceb,
    Ces,
    Chs,
    Chi,
    Chr,
    Cym,
    Dan,
    Deu,
    Dzo,
    Ell,
    Enm,
    Epo,
    Equ,
    Est,
    Eus,
    Fas,
    Fin,
    Fra,
    Frk,
    Frm,
    Gle,
    Glg,
    Grc,
    Guj,
    Hat,
    Heb,
    Hin,
    Hrv,
    Hun,
    Iku,
    Ind,
    Isl,
    Ita,
    Jav,
    Jpn,
    Kan,
    Kat,
    Kaz,
    Khm,
    Kir,
    Kor,
    Kur,
    Lao,
    Lat,
    Lav,
    Lit,
    Mal,
    Mar,
    Mkd,
    Mlt,
    Msa,
    Mya,
    Nep,
    Nld,
    Nor,
    Ori,
    Osd,
    Pan,
    Pol,
    Por,
    Pus,
    Ron,
    Rus,
    San,
    Sin,
    Slk,
    Slv,
    Spa,
    Sqi,
    Srp,
    Swa,
    Swe,
    Syr,
    Tam,
    Tel,
    Tgk,
    Tgl,
    Tha,
    Tir,
    Tur,
    Uig,
    Ukr,
    Urd,
    Uzb,
    Vie,
    Yid,
}

pub enum CreditTiming {
    StartCreditsNotBefore,
    StartCreditsNotAfter,
    StartCreditsForAtLeast,
    StartCreditsForAtMost,
    EndCreditsForAtLeast,
    EndCreditsForAtMost,
}

impl OutputFormat {
    /// Returns the file extension for the output format if it is a file based format.
    pub fn file_extension(&self) -> Option<&OsStr> {
        match self {
            OutputFormat::Raw => Some(OsStr::new(".raw")),
            OutputFormat::Srt => Some(OsStr::new(".srt")),
            OutputFormat::Sami => Some(OsStr::new(".smi")),
            OutputFormat::Transcript => Some(OsStr::new(".txt")),
            OutputFormat::Rcwt => Some(OsStr::new(".bin")),
            OutputFormat::Null => None,
            OutputFormat::SmpteTt => Some(OsStr::new(".ttml")),
            OutputFormat::SpuPng => Some(OsStr::new(".xml")),
            OutputFormat::DvdRaw => Some(OsStr::new(".dvdraw")),
            OutputFormat::WebVtt => Some(OsStr::new(".vtt")),
            OutputFormat::SimpleXml => Some(OsStr::new(".xml")),
            OutputFormat::G608 => Some(OsStr::new(".g608")),
            OutputFormat::Curl => None,
            OutputFormat::Ssa => Some(OsStr::new(".ass")),
            OutputFormat::Mcc => Some(OsStr::new(".mcc")),
            OutputFormat::Scc => Some(OsStr::new(".scc")),
            OutputFormat::Ccd => Some(OsStr::new(".ccd")),
        }
    }
}

impl Codec {
    /// Determines whether a specific subtitle codec type should be parsed.
    ///
    /// # Arguments
    ///
    /// * `user_selected` - The codec selected by the user to be searched in all elementary streams.
    /// * `user_not_selected` - The codec selected by the user not to be parsed.
    /// * `feasible` - The codec being tested for feasibility to parse.
    ///
    /// # Returns
    ///
    /// Returns `true` if the codec should be parsed, `false` otherwise.
    ///
    /// # Description
    ///
    /// This function is used when you want to find out whether you should parse a specific
    /// subtitle codec type or not. We ignore the stream if it's not selected, as setting
    /// a stream as both selected and not selected doesn't make sense.
    pub fn is_feasible(user_selected: &Codec, user_not_selected: &Codec, feasible: &Codec) -> bool {
        (*user_selected == Codec::Any && user_not_selected != feasible) || user_selected == feasible
    }
}

impl MpegDescriptor {
    pub fn is_valid_teletext_desc(&self) -> bool {
        matches!(
            self,
            MpegDescriptor::VbiDataDescriptor
                | MpegDescriptor::VbiTeletextDescriptor
                | MpegDescriptor::TeletextDescriptor
        )
    }
}

impl CCTypes {
    pub fn to_str(&self) -> &'static str {
        match self {
            CCTypes::NtscCCF1 => "NTSC line 21 field 1 closed captions",
            CCTypes::NtscCCF2 => "NTSC line 21 field 2 closed captions",
            CCTypes::DtvccPacketData => "DTVCC Channel Packet Data",
            CCTypes::DtvccPacketStart => "DTVCC Channel Packet Start",
        }
    }
}

impl CreditTiming {
    pub fn value(&self) -> &str {
        match self {
            CreditTiming::StartCreditsNotBefore => "0",
            CreditTiming::StartCreditsNotAfter => "5:00",
            CreditTiming::StartCreditsForAtLeast => "2",
            CreditTiming::StartCreditsForAtMost => "5",
            CreditTiming::EndCreditsForAtLeast => "2",
            CreditTiming::EndCreditsForAtMost => "5",
        }
    }
}

impl Language {
    pub fn to_str(&self) -> &'static str {
        match self {
            Language::Und => "und", // Undefined
            Language::Eng => "eng",
            Language::Afr => "afr",
            Language::Amh => "amh",
            Language::Ara => "ara",
            Language::Asm => "asm",
            Language::Aze => "aze",
            Language::Bel => "bel",
            Language::Ben => "ben",
            Language::Bod => "bod",
            Language::Bos => "bos",
            Language::Bul => "bul",
            Language::Cat => "cat",
            Language::Ceb => "ceb",
            Language::Ces => "ces",
            Language::Chs => "chs",
            Language::Chi => "chi",
            Language::Chr => "chr",
            Language::Cym => "cym",
            Language::Dan => "dan",
            Language::Deu => "deu",
            Language::Dzo => "dzo",
            Language::Ell => "ell",
            Language::Enm => "enm",
            Language::Epo => "epo",
            Language::Equ => "equ",
            Language::Est => "est",
            Language::Eus => "eus",
            Language::Fas => "fas",
            Language::Fin => "fin",
            Language::Fra => "fra",
            Language::Frk => "frk",
            Language::Frm => "frm",
            Language::Gle => "gle",
            Language::Glg => "glg",
            Language::Grc => "grc",
            Language::Guj => "guj",
            Language::Hat => "hat",
            Language::Heb => "heb",
            Language::Hin => "hin",
            Language::Hrv => "hrv",
            Language::Hun => "hun",
            Language::Iku => "iku",
            Language::Ind => "ind",
            Language::Isl => "isl",
            Language::Ita => "ita",
            Language::Jav => "jav",
            Language::Jpn => "jpn",
            Language::Kan => "kan",
            Language::Kat => "kat",
            Language::Kaz => "kaz",
            Language::Khm => "khm",
            Language::Kir => "kir",
            Language::Kor => "kor",
            Language::Kur => "kur",
            Language::Lao => "lao",
            Language::Lat => "lat",
            Language::Lav => "lav",
            Language::Lit => "lit",
            Language::Mal => "mal",
            Language::Mar => "mar",
            Language::Mkd => "mkd",
            Language::Mlt => "mlt",
            Language::Msa => "msa",
            Language::Mya => "mya",
            Language::Nep => "nep",
            Language::Nld => "nld",
            Language::Nor => "nor",
            Language::Ori => "ori",
            Language::Osd => "osd",
            Language::Pan => "pan",
            Language::Pol => "pol",
            Language::Por => "por",
            Language::Pus => "pus",
            Language::Ron => "ron",
            Language::Rus => "rus",
            Language::San => "san",
            Language::Sin => "sin",
            Language::Slk => "slk",
            Language::Slv => "slv",
            Language::Spa => "spa",
            Language::Sqi => "sqi",
            Language::Srp => "srp",
            Language::Swa => "swa",
            Language::Swe => "swe",
            Language::Syr => "syr",
            Language::Tam => "tam",
            Language::Tel => "tel",
            Language::Tgk => "tgk",
            Language::Tgl => "tgl",
            Language::Tha => "tha",
            Language::Tir => "tir",
            Language::Tur => "tur",
            Language::Uig => "uig",
            Language::Ukr => "ukr",
            Language::Urd => "urd",
            Language::Uzb => "uzb",
            Language::Vie => "vie",
            Language::Yid => "yid",
        }
    }
}
impl PartialEq for BufferdataType {
    fn eq(&self, other: &Self) -> bool {
        std::mem::discriminant(self) == std::mem::discriminant(other)
    }
}
