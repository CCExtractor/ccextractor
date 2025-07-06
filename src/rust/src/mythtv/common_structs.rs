//! MythTV C<->Rust bindings

use std::os::raw::c_void;

/// Globals
pub static mut HEADER_STATE: u32 = 0;
pub static mut PSM_ES_TYPE: [u8; 256] = [0; 256];
pub static mut PROCESSED_CCBLOCKS: i64 = 0;

/// Error codes
pub const AVERROR_IO: i64 = -2;

/// MPEG2 sync/start codes & masks
pub const MAX_SYNC_SIZE: u32 = 100_000;
pub const PACK_START_CODE: u32 = 0x0000_01ba;
pub const SYSTEM_HEADER_START_CODE: u32 = 0x0000_01bb;
pub const SEQUENCE_END_CODE: u32 = 0x0000_01b7;
pub const ISO_11172_END_CODE: u32 = 0x0000_01b9;
pub const PACKET_START_CODE_MASK: u32 = 0xffff_ff00;
pub const PACKET_START_CODE_PREFIX: u32 = 0x0000_0100;
pub const AV_NOPTS_VALUE: i64 = 0x8000000000000000u64 as i64;

// Module‐level constant for AVS sequence header
pub const AVS_SEQH: [u8; 4] = [0, 0, 1, 0xb0];

/// VBI types (lines carrying teletext, CC, etc.)
#[repr(u32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum VBITypeMythTv {
    Teletext = 0x1,
    CC = 0x4,
    WSS = 0x5,
    VPS = 0x7,
}
impl VBITypeMythTv {
    pub fn from_c(v: u32) -> Option<Self> {
        match v {
            0x1 => Some(Self::Teletext),
            0x4 => Some(Self::CC),
            0x5 => Some(Self::WSS),
            0x7 => Some(Self::VPS),
            _ => None,
        }
    }
    pub fn to_c(self) -> u32 {
        self as u32
    }
}

/// Program stream map IDs & private streams
#[repr(u32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Mpeg2StreamTypeMythTv {
    ProgramStreamMap = 0x1bc,
    PrivateStream1 = 0x1bd,
    PaddingStream = 0x1be,
    PrivateStream2 = 0x1bf,
}
impl Mpeg2StreamTypeMythTv {
    pub fn from_c(v: u32) -> Option<Self> {
        match v {
            0x1bc => Some(Self::ProgramStreamMap),
            0x1bd => Some(Self::PrivateStream1),
            0x1be => Some(Self::PaddingStream),
            0x1bf => Some(Self::PrivateStream2),
            _ => None,
        }
    }
    pub fn to_c(self) -> u32 {
        self as u32
    }
}

/// PES packet types
#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum StreamTypeMythTv {
    VideoMpeg1 = 0x01,
    VideoMpeg2 = 0x02,
    AudioMpeg1 = 0x03,
    AudioMpeg2 = 0x04,
    PrivateSection = 0x05,
    PrivateData = 0x06,
    AudioAAC = 0x0f,
    VideoMpeg4 = 0x10,
    VideoH264 = 0x1b,
    AudioAC3 = 0x81,
    AudioDTS = 0x8a,
}
impl StreamTypeMythTv {
    pub fn from_c(v: u8) -> Option<Self> {
        use StreamTypeMythTv::*;
        match v {
            0x01 => Some(VideoMpeg1),
            0x02 => Some(VideoMpeg2),
            0x03 => Some(AudioMpeg1),
            0x04 => Some(AudioMpeg2),
            0x05 => Some(PrivateSection),
            0x06 => Some(PrivateData),
            0x0f => Some(AudioAAC),
            0x10 => Some(VideoMpeg4),
            0x1b => Some(VideoH264),
            0x81 => Some(AudioAC3),
            0x8a => Some(AudioDTS),
            _ => None,
        }
    }
    pub fn to_c(self) -> u8 {
        self as u8
    }
}

/// Audio/Video elementary stream IDs
#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum AudioVideoIdMythTv {
    Audio = 0xc0,
    Video = 0xe0,
    AC3 = 0x80,
    DTS = 0x8a,
    LPCM = 0xa0,
    Subtitle = 0x20,
}
impl AudioVideoIdMythTv {
    pub fn from_c(v: u8) -> Option<Self> {
        use AudioVideoIdMythTv::*;
        match v {
            0xc0 => Some(Audio),
            0xe0 => Some(Video),
            0x80 => Some(AC3),
            0x8a => Some(DTS),
            0xa0 => Some(LPCM),
            0x20 => Some(Subtitle),
            _ => None,
        }
    }
    pub fn to_c(self) -> u8 {
        self as u8
    }
}

/// High‑level codec classification
#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CodecTypeMythTv {
    Unknown = -1,
    Video = 0,
    Audio = 1,
    Data = 2,
    Subtitle = 3,
}
impl CodecTypeMythTv {
    pub fn from_c(v: i32) -> Option<Self> {
        match v {
            -1 => Some(CodecTypeMythTv::Unknown),
            0 => Some(CodecTypeMythTv::Video),
            1 => Some(CodecTypeMythTv::Audio),
            2 => Some(CodecTypeMythTv::Data),
            3 => Some(CodecTypeMythTv::Subtitle),
            _ => None,
        }
    }
    pub fn to_c(self) -> i32 {
        self as i32
    }
}

/// Detailed codec IDs
#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CodecIDMythTv {
    None = 0,
    Mpeg1Video,
    Mpeg2Video, // preferred ID for MPEG Video 1 or 2 decoding
    Mpeg2VideoXvmc,
    Mpeg2VideoXvmcVld,
    H261,
    H263,
    Rv10,
    Rv20,
    Mjpeg,
    Mjpegb,
    Ljpeg,
    Sp5x,
    Jpegls,
    Mpeg4,
    Rawvideo,
    Msmpeg4v1,
    Msmpeg4v2,
    Msmpeg4v3,
    Wmv1,
    Wmv2,
    H263p,
    H263i,
    Flv1,
    Svq1,
    Svq3,
    Dvvideo,
    Huffyuv,
    Cyuv,
    H264,
    Indeo3,
    Vp3,
    Theora,
    Asv1,
    Asv2,
    Ffv1,
    FourXm,
    Vcr1,
    Cljr,
    Mdec,
    Roq,
    InterplayVideo,
    XanWc3,
    XanWc4,
    Rpza,
    Cinepak,
    WsVqa,
    Msrle,
    Msvideo1,
    Idcin,
    EightBps,
    Smc,
    Flic,
    Truemotion1,
    Vmdvideo,
    Mszh,
    Zlib,
    Qtrle,
    Snow,
    Tscc,
    Ulti,
    Qdraw,
    Vixl,
    Qpeg,
    Xvid,
    Png,
    Ppm,
    Pbm,
    Pgm,
    Pgmyuv,
    Pam,
    Ffvhuff,
    Rv30,
    Rv40,
    Vc1,
    Wmv3,
    Loco,
    Wnv1,
    Aasc,
    Indeo2,
    Fraps,
    Truemotion2,
    Bmp,
    Cscd,
    Mmvideo,
    Zmbv,
    Avs,
    Smackvideo,
    Nuv,
    Kmvc,
    Flashsv,
    Cavs,

    // Various PCM codecs
    PcmS16le = 0x10000,
    PcmS16be,
    PcmU16le,
    PcmU16be,
    PcmS8,
    PcmU8,
    PcmMulaw,
    PcmAlaw,
    PcmS32le,
    PcmS32be,
    PcmU32le,
    PcmU32be,
    PcmS24le,
    PcmS24be,
    PcmU24le,
    PcmU24be,
    PcmS24daud,

    // Various ADPCM codecs
    AdpcmImaQt = 0x11000,
    AdpcmImaWav,
    AdpcmImaDk3,
    AdpcmImaDk4,
    AdpcmImaWs,
    AdpcmImaSmjpeg,
    AdpcmMs,
    AdpcmFourXm,
    AdpcmXa,
    AdpcmAdx,
    AdpcmEa,
    AdpcmG726,
    AdpcmCt,
    AdpcmSwf,
    AdpcmYamaha,
    AdpcmSbpro4,
    AdpcmSbpro3,
    AdpcmSbpro2,

    // AMR
    AmrNb = 0x12000,
    AmrWb,

    // RealAudio codecs
    Ra144 = 0x13000,
    Ra288,

    // Various DPCM codecs
    RoqDpcm = 0x14000,
    InterplayDpcm,
    XanDpcm,
    SolDpcm,

    Mp2 = 0x15000,
    Mp3, // preferred ID for MPEG Audio layer 1, 2 or 3 decoding
    Aac,
    Mpeg4aac,
    Ac3,
    Dts,
    Vorbis,
    Dvaudio,
    Wmav1,
    Wmav2,
    Mace3,
    Mace6,
    Vmdaudio,
    Sonic,
    SonicLs,
    Flac,
    Mp3adu,
    Mp3on4,
    Shorten,
    Alac,
    WestwoodSnd1,
    Gsm,
    Qdm2,
    Cook,
    Truespeech,
    Tta,
    Smackaudio,
    Qcelp,

    // Subtitle codecs
    DvdSubtitle = 0x17000,
    DvbSubtitle,

    // Teletext codecs
    Mpeg2vbi,
    DvbVbi,

    // DSMCC codec
    DsmccB,

    Mpeg2ts = 0x20000, // _FAKE_ codec to indicate a raw MPEG2 transport stream (only used by libavformat)
}
impl CodecIDMythTv {
    pub fn from_c(v: i32) -> Option<Self> {
        match v {
            0 => Some(CodecIDMythTv::None),
            1 => Some(CodecIDMythTv::Mpeg1Video),
            2 => Some(CodecIDMythTv::Mpeg2Video),
            3 => Some(CodecIDMythTv::Mpeg2VideoXvmc),
            4 => Some(CodecIDMythTv::Mpeg2VideoXvmcVld),
            5 => Some(CodecIDMythTv::H261),
            6 => Some(CodecIDMythTv::H263),
            7 => Some(CodecIDMythTv::Rv10),
            8 => Some(CodecIDMythTv::Rv20),
            9 => Some(CodecIDMythTv::Mjpeg),
            10 => Some(CodecIDMythTv::Mjpegb),
            11 => Some(CodecIDMythTv::Ljpeg),
            12 => Some(CodecIDMythTv::Sp5x),
            13 => Some(CodecIDMythTv::Jpegls),
            14 => Some(CodecIDMythTv::Mpeg4),
            15 => Some(CodecIDMythTv::Rawvideo),
            16 => Some(CodecIDMythTv::Msmpeg4v1),
            17 => Some(CodecIDMythTv::Msmpeg4v2),
            18 => Some(CodecIDMythTv::Msmpeg4v3),
            19 => Some(CodecIDMythTv::Wmv1),
            20 => Some(CodecIDMythTv::Wmv2),
            21 => Some(CodecIDMythTv::H263p),
            22 => Some(CodecIDMythTv::H263i),
            23 => Some(CodecIDMythTv::Flv1),
            24 => Some(CodecIDMythTv::Svq1),
            25 => Some(CodecIDMythTv::Svq3),
            26 => Some(CodecIDMythTv::Dvvideo),
            27 => Some(CodecIDMythTv::Huffyuv),
            28 => Some(CodecIDMythTv::Cyuv),
            29 => Some(CodecIDMythTv::H264),
            30 => Some(CodecIDMythTv::Indeo3),
            31 => Some(CodecIDMythTv::Vp3),
            32 => Some(CodecIDMythTv::Theora),
            33 => Some(CodecIDMythTv::Asv1),
            34 => Some(CodecIDMythTv::Asv2),
            35 => Some(CodecIDMythTv::Ffv1),
            36 => Some(CodecIDMythTv::FourXm),
            37 => Some(CodecIDMythTv::Vcr1),
            38 => Some(CodecIDMythTv::Cljr),
            39 => Some(CodecIDMythTv::Mdec),
            40 => Some(CodecIDMythTv::Roq),
            41 => Some(CodecIDMythTv::InterplayVideo),
            42 => Some(CodecIDMythTv::XanWc3),
            43 => Some(CodecIDMythTv::XanWc4),
            44 => Some(CodecIDMythTv::Rpza),
            45 => Some(CodecIDMythTv::Cinepak),
            46 => Some(CodecIDMythTv::WsVqa),
            47 => Some(CodecIDMythTv::Msrle),
            48 => Some(CodecIDMythTv::Msvideo1),
            49 => Some(CodecIDMythTv::Idcin),
            50 => Some(CodecIDMythTv::EightBps),
            51 => Some(CodecIDMythTv::Smc),
            52 => Some(CodecIDMythTv::Flic),
            53 => Some(CodecIDMythTv::Truemotion1),
            54 => Some(CodecIDMythTv::Vmdvideo),
            55 => Some(CodecIDMythTv::Mszh),
            56 => Some(CodecIDMythTv::Zlib),
            57 => Some(CodecIDMythTv::Qtrle),
            58 => Some(CodecIDMythTv::Snow),
            59 => Some(CodecIDMythTv::Tscc),
            60 => Some(CodecIDMythTv::Ulti),
            61 => Some(CodecIDMythTv::Qdraw),
            62 => Some(CodecIDMythTv::Vixl),
            63 => Some(CodecIDMythTv::Qpeg),
            64 => Some(CodecIDMythTv::Xvid),
            65 => Some(CodecIDMythTv::Png),
            66 => Some(CodecIDMythTv::Ppm),
            67 => Some(CodecIDMythTv::Pbm),
            68 => Some(CodecIDMythTv::Pgm),
            69 => Some(CodecIDMythTv::Pgmyuv),
            70 => Some(CodecIDMythTv::Pam),
            71 => Some(CodecIDMythTv::Ffvhuff),
            72 => Some(CodecIDMythTv::Rv30),
            73 => Some(CodecIDMythTv::Rv40),
            74 => Some(CodecIDMythTv::Vc1),
            75 => Some(CodecIDMythTv::Wmv3),
            76 => Some(CodecIDMythTv::Loco),
            77 => Some(CodecIDMythTv::Wnv1),
            78 => Some(CodecIDMythTv::Aasc),
            79 => Some(CodecIDMythTv::Indeo2),
            80 => Some(CodecIDMythTv::Fraps),
            81 => Some(CodecIDMythTv::Truemotion2),
            82 => Some(CodecIDMythTv::Bmp),
            83 => Some(CodecIDMythTv::Cscd),
            84 => Some(CodecIDMythTv::Mmvideo),
            85 => Some(CodecIDMythTv::Zmbv),
            86 => Some(CodecIDMythTv::Avs),
            87 => Some(CodecIDMythTv::Smackvideo),
            88 => Some(CodecIDMythTv::Nuv),
            89 => Some(CodecIDMythTv::Kmvc),
            90 => Some(CodecIDMythTv::Flashsv),
            91 => Some(CodecIDMythTv::Cavs),
            0x10000 => Some(CodecIDMythTv::PcmS16le),
            0x10001 => Some(CodecIDMythTv::PcmS16be),
            0x10002 => Some(CodecIDMythTv::PcmU16le),
            0x10003 => Some(CodecIDMythTv::PcmU16be),
            0x10004 => Some(CodecIDMythTv::PcmS8),
            0x10005 => Some(CodecIDMythTv::PcmU8),
            0x10006 => Some(CodecIDMythTv::PcmMulaw),
            0x10007 => Some(CodecIDMythTv::PcmAlaw),
            0x10008 => Some(CodecIDMythTv::PcmS32le),
            0x10009 => Some(CodecIDMythTv::PcmS32be),
            0x1000A => Some(CodecIDMythTv::PcmU32le),
            0x1000B => Some(CodecIDMythTv::PcmU32be),
            0x1000C => Some(CodecIDMythTv::PcmS24le),
            0x1000D => Some(CodecIDMythTv::PcmS24be),
            0x1000E => Some(CodecIDMythTv::PcmU24le),
            0x1000F => Some(CodecIDMythTv::PcmU24be),
            0x10010 => Some(CodecIDMythTv::PcmS24daud),
            0x11000 => Some(CodecIDMythTv::AdpcmImaQt),
            0x11001 => Some(CodecIDMythTv::AdpcmImaWav),
            0x11002 => Some(CodecIDMythTv::AdpcmImaDk3),
            0x11003 => Some(CodecIDMythTv::AdpcmImaDk4),
            0x11004 => Some(CodecIDMythTv::AdpcmImaWs),
            0x11005 => Some(CodecIDMythTv::AdpcmImaSmjpeg),
            0x11006 => Some(CodecIDMythTv::AdpcmMs),
            0x11007 => Some(CodecIDMythTv::AdpcmFourXm),
            0x11008 => Some(CodecIDMythTv::AdpcmXa),
            0x11009 => Some(CodecIDMythTv::AdpcmAdx),
            0x1100A => Some(CodecIDMythTv::AdpcmEa),
            0x1100B => Some(CodecIDMythTv::AdpcmG726),
            0x1100C => Some(CodecIDMythTv::AdpcmCt),
            0x1100D => Some(CodecIDMythTv::AdpcmSwf),
            0x1100E => Some(CodecIDMythTv::AdpcmYamaha),
            0x1100F => Some(CodecIDMythTv::AdpcmSbpro4),
            0x11010 => Some(CodecIDMythTv::AdpcmSbpro3),
            0x11011 => Some(CodecIDMythTv::AdpcmSbpro2),
            0x12000 => Some(CodecIDMythTv::AmrNb),
            0x12001 => Some(CodecIDMythTv::AmrWb),
            0x13000 => Some(CodecIDMythTv::Ra144),
            0x13001 => Some(CodecIDMythTv::Ra288),
            0x14000 => Some(CodecIDMythTv::RoqDpcm),
            0x14001 => Some(CodecIDMythTv::InterplayDpcm),
            0x14002 => Some(CodecIDMythTv::XanDpcm),
            0x14003 => Some(CodecIDMythTv::SolDpcm),
            0x15000 => Some(CodecIDMythTv::Mp2),
            0x15001 => Some(CodecIDMythTv::Mp3),
            0x15002 => Some(CodecIDMythTv::Aac),
            0x15003 => Some(CodecIDMythTv::Mpeg4aac),
            0x15004 => Some(CodecIDMythTv::Ac3),
            0x15005 => Some(CodecIDMythTv::Dts),
            0x15006 => Some(CodecIDMythTv::Vorbis),
            0x15007 => Some(CodecIDMythTv::Dvaudio),
            0x15008 => Some(CodecIDMythTv::Wmav1),
            0x15009 => Some(CodecIDMythTv::Wmav2),
            0x1500A => Some(CodecIDMythTv::Mace3),
            0x1500B => Some(CodecIDMythTv::Mace6),
            0x1500C => Some(CodecIDMythTv::Vmdaudio),
            0x1500D => Some(CodecIDMythTv::Sonic),
            0x1500E => Some(CodecIDMythTv::SonicLs),
            0x1500F => Some(CodecIDMythTv::Flac),
            0x15010 => Some(CodecIDMythTv::Mp3adu),
            0x15011 => Some(CodecIDMythTv::Mp3on4),
            0x15012 => Some(CodecIDMythTv::Shorten),
            0x15013 => Some(CodecIDMythTv::Alac),
            0x15014 => Some(CodecIDMythTv::WestwoodSnd1),
            0x15015 => Some(CodecIDMythTv::Gsm),
            0x15016 => Some(CodecIDMythTv::Qdm2),
            0x15017 => Some(CodecIDMythTv::Cook),
            0x15018 => Some(CodecIDMythTv::Truespeech),
            0x15019 => Some(CodecIDMythTv::Tta),
            0x1501A => Some(CodecIDMythTv::Smackaudio),
            0x1501B => Some(CodecIDMythTv::Qcelp),
            0x17000 => Some(CodecIDMythTv::DvdSubtitle),
            0x17001 => Some(CodecIDMythTv::DvbSubtitle),
            0x17002 => Some(CodecIDMythTv::Mpeg2vbi),
            0x17003 => Some(CodecIDMythTv::DvbVbi),
            0x17004 => Some(CodecIDMythTv::DsmccB),
            0x20000 => Some(CodecIDMythTv::Mpeg2ts),
            _ => None,
        }
    }
    pub fn to_c(self) -> i32 {
        self as i32
    }
}
/// Rust mirror of C's AVPacket
#[repr(C)]
#[derive(Debug, Clone)]
pub struct AVPacketMythTv {
    pub pts: i64,
    pub dts: i64,
    pub data: Vec<u8>,
    pub size: usize,
    pub stream_index: i32,
    pub flags: i32,
    pub duration: i32,
    pub priv_data: *mut c_void,
    pub pos: i64,
    pub codec_id: CodecIDMythTv,
    pub ty: CodecTypeMythTv,
}
impl Default for AVPacketMythTv {
    fn default() -> Self {
        AVPacketMythTv {
            pts: 0,
            dts: 0,
            data: Vec::new(),
            size: 0,
            stream_index: 0,
            flags: 0,
            duration: 0,
            priv_data: std::ptr::null_mut(),
            pos: -1,
            codec_id: CodecIDMythTv::None,
            ty: CodecTypeMythTv::Unknown,
        }
    }
}

mod tests {

    #[test]
    fn test_codec_id_from_c() {
        use crate::mythtv::common_structs::CodecIDMythTv;
        assert_eq!(CodecIDMythTv::from_c(0), Some(CodecIDMythTv::None));
        assert_eq!(CodecIDMythTv::from_c(1), Some(CodecIDMythTv::Mpeg1Video));
        assert_eq!(
            CodecIDMythTv::from_c(0x10000),
            Some(CodecIDMythTv::PcmS16le)
        );
        assert_eq!(CodecIDMythTv::from_c(0x20000), Some(CodecIDMythTv::Mpeg2ts));
        assert_eq!(CodecIDMythTv::to_c(CodecIDMythTv::AdpcmYamaha), 0x1100E);
        assert_eq!(CodecIDMythTv::from_c(-1), None);
    }
}
