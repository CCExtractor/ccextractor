use crate::demuxer::common_structs::CcxRational;
use std::convert::TryFrom;
use std::ptr;

pub const STR_LEN: u32 = 256;
pub const CLOSED_CAP_DID: u8 = 0x61;
pub const CLOSED_C708_SDID: u8 = 0x01;
pub const CLOSED_C608_SDID: u8 = 0x02;
pub const STARTBYTESLENGTH: usize = 1024 * 1024;
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum DemuxerError {
    Retry = -100,           // CCX_EAGAIN
    EOF = -101,             // CCX_EOF
    InvalidArgument = -102, // CCX_EINVAL
    Unsupported = -103,     // CCX_ENOSUPP
    OutOfMemory = -104,     // CCX_ENOMEM
}
// Equivalent enums
#[repr(u8)]
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum GXF_Pkt_Type {
    PKT_MAP = 0xbc,
    PKT_MEDIA = 0xbf,
    PKT_EOS = 0xfb,
    PKT_FLT = 0xfc,
    PKT_UMF = 0xfd,
}

#[repr(u8)]
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum GXF_Mat_Tag {
    MAT_NAME = 0x40,
    MAT_FIRST_FIELD = 0x41,
    MAT_LAST_FIELD = 0x42,
    MAT_MARK_IN = 0x43,
    MAT_MARK_OUT = 0x44,
    MAT_SIZE = 0x45,
}

#[repr(u8)]
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum GXF_Track_Tag {
    // Media file name
    TRACK_NAME = 0x4c,
    // Auxiliary Information. The exact meaning depends on the track type.
    TRACK_AUX = 0x4d,
    // Media file system version
    TRACK_VER = 0x4e,
    // MPEG auxiliary information
    TRACK_MPG_AUX = 0x4f,
    /**
     * Frame rate
     * 1 = 60 frames/sec
     * 2 = 59.94 frames/sec
     * 3 = 50 frames/sec
     * 4 = 30 frames/sec
     * 5 = 29.97 frames/sec
     * 6 = 25 frames/sec
     * 7 = 24 frames/sec
     * 8 = 23.98 frames/sec
     * -1 = Not applicable for this track type
     * -2 = Not available
     */
    TRACK_FPS = 0x50,
    /**
     * Lines per frame
     * 1 = 525
     * 2 = 625
     * 4 = 1080
     * 5 = Reserved
     * 6 = 720
     * -1 = Not applicable
     * -2 = Not available
     */
    TRACK_LINES = 0x51,
    /**
     * Fields per frame
     * 1 = Progressive
     * 2 = Interlaced
     * -1 = Not applicable
     * -2 = Not available
     */
    TRACK_FPF = 0x52,
}

#[repr(u8)]
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum GXF_Track_Type {
    // A video track encoded using JPEG (ITU-R T.81 or ISO/IEC 10918-1) for 525 line material.
    TRACK_TYPE_MOTION_JPEG_525 = 3,
    // A video track encoded using JPEG (ITU-R T.81 or ISO/IEC 10918-1) for 625 line material
    TRACK_TYPE_MOTION_JPEG_625 = 4,
    // SMPTE 12M time code tracks
    TRACK_TYPE_TIME_CODE_525 = 7,
    TRACK_TYPE_TIME_CODE_625 = 8,
    // A mono 24-bit PCM audio track
    TRACK_TYPE_AUDIO_PCM_24 = 9,
    // A mono 16-bit PCM audio track.
    TRACK_TYPE_AUDIO_PCM_16 = 10,
    // A video track encoded using ISO/IEC 13818-2 (MPEG-2).
    TRACK_TYPE_MPEG2_525 = 11,
    TRACK_TYPE_MPEG2_625 = 12,
    /**
     * A video track encoded using SMPTE 314M or ISO/IEC 61834-2 DV
     * encoded at 25 Mb/s for 525/60i
     */
    TRACK_TYPE_DV_BASED_25MB_525 = 13,
    /**
     * A video track encoded using SMPTE 314M or ISO/IEC 61834-2 DV encoding at 25 Mb/s
     * for 625/50i.
     */
    TRACK_TYPE_DV_BASED_25MB_625 = 14,
    /**
     * A video track encoded using SMPTE 314M DV encoding at 50Mb/s
     * for 525/50i.
     */
    TRACK_TYPE_DV_BASED_50MB_525 = 15,
    /**
     * A video track encoded using SMPTE 314M DV encoding at 50Mb/s for 625/50i
     */
    TRACK_TYPE_DV_BASED_50_MB_625 = 16,
    // An AC-3 audio track
    TRACK_TYPE_AC_3_16b_audio = 17,
    // A non-PCM AES data track
    TRACK_TYPE_COMPRESSED_24B_AUDIO = 18,
    // Ignore it as nice decoder
    TRACK_TYPE_RESERVED = 19,
    /**
     * A video track encoded using ISO/IEC 13818-2 (MPEG-2) main profile at main
     * level or high level, or 4:2:2 profile at main level or high level.
     */
    TRACK_TYPE_MPEG2_HD = 20,
    // SMPTE 291M 10-bit type 2 component ancillary data.
    TRACK_TYPE_ANCILLARY_DATA = 21,
    // A video track encoded using ISO/IEC 11172-2 (MPEG-1)
    TRACK_TYPE_MPEG1_525 = 22,
    // A video track encoded using ISO/IEC 11172-2 (MPEG-1).
    TRACK_TYPE_MPEG1_625 = 23,
    // SMPTE 12M time codes For HD material.
    TRACK_TYPE_TIME_CODE_HD = 24,
}
impl TryFrom<u8> for GXF_Track_Type {
    type Error = ();

    fn try_from(value: u8) -> Result<Self, Self::Error> {
        match value {
            3 => Ok(GXF_Track_Type::TRACK_TYPE_MOTION_JPEG_525),
            4 => Ok(GXF_Track_Type::TRACK_TYPE_MOTION_JPEG_625),
            7 => Ok(GXF_Track_Type::TRACK_TYPE_TIME_CODE_525),
            8 => Ok(GXF_Track_Type::TRACK_TYPE_TIME_CODE_625),
            9 => Ok(GXF_Track_Type::TRACK_TYPE_AUDIO_PCM_24),
            10 => Ok(GXF_Track_Type::TRACK_TYPE_AUDIO_PCM_16),
            11 => Ok(GXF_Track_Type::TRACK_TYPE_MPEG2_525),
            12 => Ok(GXF_Track_Type::TRACK_TYPE_MPEG2_625),
            13 => Ok(GXF_Track_Type::TRACK_TYPE_DV_BASED_25MB_525),
            14 => Ok(GXF_Track_Type::TRACK_TYPE_DV_BASED_25MB_625),
            15 => Ok(GXF_Track_Type::TRACK_TYPE_DV_BASED_50MB_525),
            16 => Ok(GXF_Track_Type::TRACK_TYPE_DV_BASED_50_MB_625),
            17 => Ok(GXF_Track_Type::TRACK_TYPE_AC_3_16b_audio),
            18 => Ok(GXF_Track_Type::TRACK_TYPE_COMPRESSED_24B_AUDIO),
            19 => Ok(GXF_Track_Type::TRACK_TYPE_RESERVED),
            20 => Ok(GXF_Track_Type::TRACK_TYPE_MPEG2_HD),
            21 => Ok(GXF_Track_Type::TRACK_TYPE_ANCILLARY_DATA),
            22 => Ok(GXF_Track_Type::TRACK_TYPE_MPEG1_525),
            23 => Ok(GXF_Track_Type::TRACK_TYPE_MPEG1_625),
            24 => Ok(GXF_Track_Type::TRACK_TYPE_TIME_CODE_HD),
            _ => Err(()),
        }
    }
}
#[repr(u8)]
#[derive(Debug, Clone, Eq, PartialEq)]
pub enum GXF_Anc_Data_Pres_Format {
    PRES_FORMAT_SD = 1,
    PRES_FORMAT_HD = 2,
}

impl std::fmt::Display for GXF_Anc_Data_Pres_Format {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            GXF_Anc_Data_Pres_Format::PRES_FORMAT_SD => write!(f, "PRES_FORMAT_SD"),
            GXF_Anc_Data_Pres_Format::PRES_FORMAT_HD => write!(f, "PRES_FORMAT_HD"),
        }
    }
}

#[repr(u8)]
#[derive(Debug, Clone, Eq, PartialEq)]
pub enum MpegPictureCoding {
    CCX_MPC_NONE = 0,
    CCX_MPC_I_FRAME = 1,
    CCX_MPC_P_FRAME = 2,
    CCX_MPC_B_FRAME = 3,
}

impl TryFrom<u8> for MpegPictureCoding {
    type Error = ();

    fn try_from(value: u8) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(MpegPictureCoding::CCX_MPC_NONE),
            1 => Ok(MpegPictureCoding::CCX_MPC_I_FRAME),
            2 => Ok(MpegPictureCoding::CCX_MPC_P_FRAME),
            3 => Ok(MpegPictureCoding::CCX_MPC_B_FRAME),
            _ => Err(()),
        }
    }
}
#[repr(u8)]
#[derive(Debug, Clone, Eq, PartialEq)]
pub enum MpegPictureStruct {
    CCX_MPS_NONE = 0,
    CCX_MPS_TOP_FIELD = 1,
    CCX_MPS_BOTTOM_FIELD = 2,
    CCX_MPS_FRAME = 3,
}
impl TryFrom<u8> for MpegPictureStruct {
    type Error = ();

    fn try_from(value: u8) -> Result<Self, Self::Error> {
        match value {
            0 => Ok(MpegPictureStruct::CCX_MPS_NONE),
            1 => Ok(MpegPictureStruct::CCX_MPS_TOP_FIELD),
            2 => Ok(MpegPictureStruct::CCX_MPS_BOTTOM_FIELD),
            3 => Ok(MpegPictureStruct::CCX_MPS_FRAME),
            _ => Err(()),
        }
    }
}

#[derive(Debug, Clone)]
pub struct CcxGxfVideoTrack {
    /// Name of Media File
    pub(crate) track_name: String,

    /// Media File system Version
    pub fs_version: u32,

    /// Frame Rate - Calculate timestamp on the basis of this
    pub frame_rate: CcxRational,

    /// Lines per frame (valid value for AD tracks)
    /// May be used while parsing VBI
    pub line_per_frame: u32,

    /**
     * Field per frame (Needed when parsing VBI)
     * 1 = Progressive
     * 2 = Interlaced
     * -1 = Not applicable
     * -2 = Not available
     */
    pub field_per_frame: u32,

    pub p_code: MpegPictureCoding,
    pub p_struct: MpegPictureStruct,
}
impl Default for CcxGxfVideoTrack {
    fn default() -> Self {
        CcxGxfVideoTrack {
            track_name: String::new(),
            fs_version: 0,
            frame_rate: CcxRational { num: 0, den: 0 },
            line_per_frame: 0,
            field_per_frame: 0,
            p_code: MpegPictureCoding::CCX_MPC_NONE,
            p_struct: MpegPictureStruct::CCX_MPS_NONE,
        }
    }
}

#[derive(Debug)]
pub struct CcxGxfAncillaryDataTrack {
    /// Name of Media File
    pub track_name: String,

    /// ID of track
    pub id: u8,

    /// Presentation Format
    pub ad_format: GXF_Anc_Data_Pres_Format,

    /// Number of ancillary data fields per ancillary data media packet
    pub nb_field: i32,

    /// Byte size of each ancillary data field
    pub field_size: i32,

    /**
     * Byte size of the ancillary data media packet in 256-byte units:
     * This value shall be 256, indicating an ancillary data media packet size
     * of 65536 bytes
     */
    pub packet_size: i32,

    /// Media File system Version
    pub fs_version: u32,

    /**
     * Frame Rate XXX AD track does have valid but this field may
     * be ignored since related to only video
     */
    pub frame_rate: u32,

    /**
     * Lines per frame (valid value for AD tracks)
     * XXX may be ignored since related to raw video frame
     */
    pub line_per_frame: u32,

    /// Field per frame Might need if parsed VBI
    pub field_per_frame: u32,
}
impl Default for CcxGxfAncillaryDataTrack {
    fn default() -> Self {
        CcxGxfAncillaryDataTrack {
            track_name: String::new(),
            id: 0,
            ad_format: GXF_Anc_Data_Pres_Format::PRES_FORMAT_SD,
            nb_field: 0,
            field_size: 0,
            packet_size: 0,
            fs_version: 0,
            frame_rate: 0,
            line_per_frame: 0,
            field_per_frame: 0,
        }
    }
}

#[allow(dead_code)]
#[derive(Debug)]
pub struct CcxGxf {
    pub nb_streams: i32,

    /// Name of Media File
    pub media_name: String,

    /**
     * The first field number shall represent the position on a playout
     * timeline of the first recorded field on a track
     */
    pub first_field_nb: i32,

    /**
     * The last field number shall represent the position on a playout
     * timeline of the last recorded field plus one
     */
    pub last_field_nb: i32,

    /**
     * The mark-in field number shall represent the position on a playout
     * timeline of the first field to be played from a track
     */
    pub mark_in: i32,

    /**
     * The mark-out field number shall represent the position on a playout
     * timeline of the last field to be played plus one
     */
    pub mark_out: i32,

    /**
     * Estimated size in KB; for bytes, multiply by 1024
     */
    pub stream_size: i32,

    pub ad_track: Option<CcxGxfAncillaryDataTrack>,

    pub vid_track: Option<CcxGxfVideoTrack>,

    /// CDP data buffer
    pub cdp: Option<Vec<u8>>,
    pub cdp_len: usize,
}
impl Default for CcxGxf {
    fn default() -> Self {
        let mut ctx = CcxGxf {
            nb_streams: 0,
            media_name: String::new(),
            first_field_nb: 0,
            last_field_nb: 0,
            mark_in: 0,
            mark_out: 0,
            stream_size: 0,
            ad_track: None,
            vid_track: None,
            cdp: None,
            cdp_len: 0,
        };
        // Initialize the context with zeroed memory
        unsafe {
            ptr::write_bytes(&mut ctx as *mut _ as *mut u8, 0, size_of::<CcxGxf>());
        }
        ctx
    }
}
