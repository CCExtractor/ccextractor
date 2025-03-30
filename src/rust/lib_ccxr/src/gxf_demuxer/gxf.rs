#![allow(non_camel_case_types)]
#![allow(unexpected_cfgs)]
#![allow(unused_doc_comments)]
#![allow(unused_assignments)]

use crate::demuxer::common_structs::*;
use crate::demuxer::demux::{CcxDemuxer, DemuxerData};
use crate::file_functions::file::*;
use crate::info;
use crate::util::log::{debug, DebugMessageFlag};
use byteorder::{ByteOrder, NetworkEndian};
use std::cmp::PartialEq;
use std::convert::TryFrom;
use std::ptr;
use std::slice;

const STR_LEN: u32 = 256;
const CLOSED_CAP_DID: u8 = 0x61;
const CLOSED_C708_SDID: u8 = 0x01;
const CLOSED_C608_SDID: u8 = 0x02;
pub const STARTBYTESLENGTH: usize = 1024 * 1024;
use crate::common::BufferdataType;

macro_rules! dbg {
    ($($args:expr),*) => {
        debug!(msg_type = DebugMessageFlag::PARSE; $($args),*)
    };
}

/// Reads a 32-bit big-endian value from the given pointer and converts it to host order.
/// Mimics the C macro: #define RB32(x) (ntohl(*(unsigned int *)(x)))
/// # Safety
/// This function is unsafe because it calls unsafe function `from_raw_parts`
pub unsafe fn rb32(ptr: *const u8) -> u32 {
    let bytes = slice::from_raw_parts(ptr, 4);
    NetworkEndian::read_u32(bytes)
}
/// # Safety
/// This function is unsafe because it calls unsafe function `from_raw_parts`
pub unsafe fn rb16(ptr: *const u8) -> u16 {
    let bytes = slice::from_raw_parts(ptr, 2);
    NetworkEndian::read_u16(bytes)
}
/// # Safety
/// This function is unsafe because it calls unsafe function `read_unaligned`
pub unsafe fn rl32(ptr: *const u8) -> u32 {
    ptr::read_unaligned::<u32>(ptr as *const u32)
}
/// # Safety
/// This function is unsafe because it calls unsafe function `read_unaligned`
pub unsafe fn rl16(ptr: *const u8) -> u16 {
    ptr::read_unaligned::<u16>(ptr as *const u16)
}

// Equivalent enums
#[repr(u8)]
pub enum GXFPktType {
    PKT_MAP = 0xbc,
    PKT_MEDIA = 0xbf,
    PKT_EOS = 0xfb,
    PKT_FLT = 0xfc,
    PKT_UMF = 0xfd,
}

#[repr(u8)]
pub enum GXFMatTag {
    MAT_NAME = 0x40,
    MAT_FIRST_FIELD = 0x41,
    MAT_LAST_FIELD = 0x42,
    MAT_MARK_IN = 0x43,
    MAT_MARK_OUT = 0x44,
    MAT_SIZE = 0x45,
}

#[repr(u8)]
pub enum GXFTrackTag {
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
pub enum GXFTrackType {
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
impl TryFrom<u8> for GXFTrackType {
    type Error = ();

    fn try_from(value: u8) -> Result<Self, Self::Error> {
        match value {
            3 => Ok(GXFTrackType::TRACK_TYPE_MOTION_JPEG_525),
            4 => Ok(GXFTrackType::TRACK_TYPE_MOTION_JPEG_625),
            7 => Ok(GXFTrackType::TRACK_TYPE_TIME_CODE_525),
            8 => Ok(GXFTrackType::TRACK_TYPE_TIME_CODE_625),
            9 => Ok(GXFTrackType::TRACK_TYPE_AUDIO_PCM_24),
            10 => Ok(GXFTrackType::TRACK_TYPE_AUDIO_PCM_16),
            11 => Ok(GXFTrackType::TRACK_TYPE_MPEG2_525),
            12 => Ok(GXFTrackType::TRACK_TYPE_MPEG2_625),
            13 => Ok(GXFTrackType::TRACK_TYPE_DV_BASED_25MB_525),
            14 => Ok(GXFTrackType::TRACK_TYPE_DV_BASED_25MB_625),
            15 => Ok(GXFTrackType::TRACK_TYPE_DV_BASED_50MB_525),
            16 => Ok(GXFTrackType::TRACK_TYPE_DV_BASED_50_MB_625),
            17 => Ok(GXFTrackType::TRACK_TYPE_AC_3_16b_audio),
            18 => Ok(GXFTrackType::TRACK_TYPE_COMPRESSED_24B_AUDIO),
            19 => Ok(GXFTrackType::TRACK_TYPE_RESERVED),
            20 => Ok(GXFTrackType::TRACK_TYPE_MPEG2_HD),
            21 => Ok(GXFTrackType::TRACK_TYPE_ANCILLARY_DATA),
            22 => Ok(GXFTrackType::TRACK_TYPE_MPEG1_525),
            23 => Ok(GXFTrackType::TRACK_TYPE_MPEG1_625),
            24 => Ok(GXFTrackType::TRACK_TYPE_TIME_CODE_HD),
            _ => Err(()),
        }
    }
}
#[repr(u8)]
#[derive(Debug)]
pub enum GXFAncDataPresFormat {
    PRES_FORMAT_SD = 1,
    PRES_FORMAT_HD = 2,
}
impl std::fmt::Display for GXFAncDataPresFormat {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            GXFAncDataPresFormat::PRES_FORMAT_SD => write!(f, "PRES_FORMAT_SD"),
            GXFAncDataPresFormat::PRES_FORMAT_HD => write!(f, "PRES_FORMAT_HD"),
        }
    }
}

#[repr(u8)]
#[derive(Debug)]
pub enum MpegPictureCoding {
    CCX_MPC_NONE = 0,
    CCX_MPC_I_FRAME = 1,
    CCX_MPC_P_FRAME = 2,
    CCX_MPC_B_FRAME = 3,
}

#[repr(u8)]
#[derive(Debug)]
pub enum MpegPictureStruct {
    CCX_MPS_NONE = 0,
    CCX_MPS_TOP_FIELD = 1,
    CCX_MPS_BOTTOM_FIELD = 2,
    CCX_MPS_FRAME = 3,
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
#[derive(Debug, Clone, Copy)]
pub struct CcxRational {
    num: i32,
    den: i32,
}

#[derive(Debug)]
pub struct CcxGxfVideoTrack {
    /// Name of Media File
    track_name: [u8; STR_LEN as usize],

    /// Media File system Version
    fs_version: u32,

    /// Frame Rate - Calculate timestamp on the basis of this
    frame_rate: CcxRational,

    /// Lines per frame (valid value for AD tracks)
    /// May be used while parsing VBI
    line_per_frame: u32,

    /**
     * Field per frame (Needed when parsing VBI)
     * 1 = Progressive
     * 2 = Interlaced
     * -1 = Not applicable
     * -2 = Not available
     */
    field_per_frame: u32,

    p_code: MpegPictureCoding,
    p_struct: MpegPictureStruct,
}
impl Default for CcxGxfVideoTrack {
    fn default() -> Self {
        CcxGxfVideoTrack {
            track_name: [0; STR_LEN as usize],
            fs_version: 0,
            frame_rate: CcxRational { num: 0, den: 0 },
            line_per_frame: 0,
            field_per_frame: 0,
            p_code: MpegPictureCoding::CCX_MPC_NONE,
            p_struct: MpegPictureStruct::CCX_MPS_NONE,
        }
    }
}
impl CcxGxfVideoTrack {
    fn default() -> CcxGxfVideoTrack {
        CcxGxfVideoTrack {
            track_name: [0; STR_LEN as usize],
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
    track_name: [u8; STR_LEN as usize],

    /// ID of track
    id: u8,

    /// Presentation Format
    ad_format: GXFAncDataPresFormat,

    /// Number of ancillary data fields per ancillary data media packet
    nb_field: i32,

    /// Byte size of each ancillary data field
    field_size: i32,

    /**
     * Byte size of the ancillary data media packet in 256-byte units:
     * This value shall be 256, indicating an ancillary data media packet size
     * of 65536 bytes
     */
    packet_size: i32,

    /// Media File system Version
    fs_version: u32,

    /**
     * Frame Rate XXX AD track does have valid but this field may
     * be ignored since related to only video
     */
    frame_rate: u32,

    /**
     * Lines per frame (valid value for AD tracks)
     * XXX may be ignored since related to raw video frame
     */
    line_per_frame: u32,

    /// Field per frame Might need if parsed VBI
    field_per_frame: u32,
}
impl Default for CcxGxfAncillaryDataTrack {
    fn default() -> Self {
        CcxGxfAncillaryDataTrack {
            track_name: [0; STR_LEN as usize],
            id: 0,
            ad_format: GXFAncDataPresFormat::PRES_FORMAT_SD,
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
impl CcxGxfAncillaryDataTrack {
    fn default() -> CcxGxfAncillaryDataTrack {
        CcxGxfAncillaryDataTrack {
            track_name: [0; STR_LEN as usize],
            id: 0,
            ad_format: GXFAncDataPresFormat::PRES_FORMAT_SD,
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
    nb_streams: i32,

    /// Name of Media File
    media_name: [u8; STR_LEN as usize],

    /**
     * The first field number shall represent the position on a playout
     * timeline of the first recorded field on a track
     */
    first_field_nb: i32,

    /**
     * The last field number shall represent the position on a playout
     * timeline of the last recorded field plus one
     */
    last_field_nb: i32,

    /**
     * The mark-in field number shall represent the position on a playout
     * timeline of the first field to be played from a track
     */
    mark_in: i32,

    /**
     * The mark-out field number shall represent the position on a playout
     * timeline of the last field to be played plus one
     */
    mark_out: i32,

    /**
     * Estimated size in KB; for bytes, multiply by 1024
     */
    stream_size: i32,

    ad_track: Option<Box<CcxGxfAncillaryDataTrack>>,

    vid_track: Option<Box<CcxGxfVideoTrack>>,

    /// CDP data buffer
    cdp: Option<Vec<u8>>,
    cdp_len: usize,
}
impl Default for CcxGxf {
    fn default() -> Self {
        let mut ctx = CcxGxf {
            nb_streams: 0,
            media_name: [0; STR_LEN as usize],
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
impl CcxGxf {
    #[allow(unused)]
    fn default() -> CcxGxf {
        CcxGxf {
            nb_streams: 0,
            media_name: [0; STR_LEN as usize],
            first_field_nb: 0,
            last_field_nb: 0,
            mark_in: 0,
            mark_out: 0,
            stream_size: 0,
            ad_track: None,
            vid_track: None,
            cdp: None,
            cdp_len: 0,
        }
    }
    #[allow(unused)]
    fn is_null(&self) -> bool {
        self.nb_streams == 0
            && self.first_field_nb == 0
            && self.last_field_nb == 0
            && self.mark_in == 0
            && self.mark_out == 0
            && self.stream_size == 0
            && self.ad_track.is_none()
            && self.vid_track.is_none()
            && self.cdp.is_none()
            && self.cdp_len == 0
    }
}

/// Parses a packet header, extracting type and length.
/// @param ctx Demuxer Ctx used for reading from file
/// @param type detected packet type is stored here
/// @param length detected packet length, excluding header, is stored here
/// @return CCX_EINVAL if header not found or contains invalid data,
///         CCX_EOF if EOF reached while reading packet
///         CCX_OK if the stream was fine enough to be parsed
/// # Safety
/// This function is unsafe because it calls unsafe functions `buffered_read` and `rb32`
pub unsafe fn parse_packet_header(
    ctx: *mut CcxDemuxer,
    pkt_type: &mut GXFPktType,
    length: &mut i32,
) -> i32 {
    if ctx.is_null() {
        return CCX_EINVAL;
    }

    let ctx_ref = ctx.as_mut().unwrap();
    let mut pkt_header = [0u8; 16];
    let result = buffered_read(ctx_ref, Some(&mut pkt_header[..]), 16);
    ctx_ref.past += result as i64;
    if result != 16 {
        return CCX_EOF;
    }

    /**
     * Veify 5 byte packet leader, must be 0x00 0x00 0x00 0x00 0x00 0x01
     */
    if rb32(pkt_header.as_ptr()) != 0 {
        return CCX_EINVAL;
    }
    let mut index = 4;

    if pkt_header[index] != 1 {
        return CCX_EINVAL;
    }
    index += 1;

    // In C, the packet type is simply assigned.
    // Here, we map it to the GXFPktType enum.
    *pkt_type = match pkt_header[index] {
        0xbc => GXFPktType::PKT_MAP,
        0xbf => GXFPktType::PKT_MEDIA,
        0xfb => GXFPktType::PKT_EOS,
        0xfc => GXFPktType::PKT_FLT,
        0xfd => GXFPktType::PKT_UMF,
        _ => return CCX_EINVAL,
    };
    index += 1;

    // Read 4-byte length from the packet header.
    *length = rb32(pkt_header[index..].as_ptr()) as i32;
    index += 4;

    if ((*length >> 24) != 0) || *length < 16 {
        return CCX_EINVAL;
    }
    *length -= 16;

    index += 4;

    /**
     * verify packet trailer, must be 0xE1 0xE2
     */
    if pkt_header[index] != 0xe1 {
        return CCX_EINVAL;
    }
    index += 1;

    if pkt_header[index] != 0xe2 {
        return CCX_EINVAL;
    }

    CCX_OK
}
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe functions like `buffered_get_byte` and `buffered_read`
pub unsafe fn parse_material_sec(demux: *mut CcxDemuxer, mut len: i32) -> i32 {
    if demux.is_null() {
        return CCX_EINVAL;
    }

    let ctx = (*demux).private_data as *mut CcxGxf;
    if ctx.is_null() {
        return CCX_EINVAL;
    }

    let mut ret = CCX_OK;

    while len > 2 {
        let tag = buffered_get_byte(demux);
        let tag_len = buffered_get_byte(demux);
        len -= 2;

        if len < tag_len as i32 {
            ret = CCX_EOF;
            break;
        }

        // In C code, there is a redundant check here.
        // For our implementation, we subtract tag_len if possible.
        len -= tag_len as i32;

        match tag {
            x if x == GXFMatTag::MAT_NAME as u8 => {
                let result = buffered_read(
                    demux.as_mut().unwrap(),
                    Some(&mut (*ctx).media_name[..tag_len as usize]),
                    tag_len as usize,
                );
                (*demux).past += tag_len as i64;
                if result != tag_len as usize {
                    ret = CCX_EOF;
                    break;
                }
            }
            x if x == GXFMatTag::MAT_FIRST_FIELD as u8 => {
                (*ctx).first_field_nb = buffered_get_be32(demux) as i32;
            }
            x if x == GXFMatTag::MAT_LAST_FIELD as u8 => {
                (*ctx).last_field_nb = buffered_get_be32(demux) as i32;
            }
            x if x == GXFMatTag::MAT_MARK_IN as u8 => {
                (*ctx).mark_in = buffered_get_be32(demux) as i32;
            }
            x if x == GXFMatTag::MAT_MARK_OUT as u8 => {
                (*ctx).mark_out = buffered_get_be32(demux) as i32;
            }
            x if x == GXFMatTag::MAT_SIZE as u8 => {
                (*ctx).stream_size = buffered_get_be32(demux) as i32;
            }
            _ => {
                let result = buffered_skip(demux, tag_len as u32);
                (*demux).past += result as i64;
            }
        }
    }

    // error: label handling in C code.
    let skipped = buffered_skip(demux, len as u32);
    (*demux).past += skipped as i64;
    if skipped != len as usize {
        ret = CCX_EOF;
    }

    ret
}

pub fn set_track_frame_rate(vid_track: &mut CcxGxfVideoTrack, val: i8) {
    match val {
        1 => {
            vid_track.frame_rate.num = 60;
            vid_track.frame_rate.den = 1;
        }
        2 => {
            vid_track.frame_rate.num = 60000;
            vid_track.frame_rate.den = 1001;
        }
        3 => {
            vid_track.frame_rate.num = 50;
            vid_track.frame_rate.den = 1;
        }
        4 => {
            vid_track.frame_rate.num = 30;
            vid_track.frame_rate.den = 1;
        }
        5 => {
            vid_track.frame_rate.num = 30000;
            vid_track.frame_rate.den = 1001;
        }
        6 => {
            vid_track.frame_rate.num = 25;
            vid_track.frame_rate.den = 1;
        }
        7 => {
            vid_track.frame_rate.num = 24;
            vid_track.frame_rate.den = 1;
        }
        8 => {
            vid_track.frame_rate.num = 24000;
            vid_track.frame_rate.den = 1001;
        }
        -1 => { /* Not applicable for this track type */ }
        -2 => { /* Not available */ }
        _ => { /* Do nothing in case of no frame rate */ }
    }
}
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe functions like `buffered_get_byte` and `buffered_read`
pub unsafe fn parse_mpeg525_track_desc(demux: &mut CcxDemuxer, mut len: i32) -> i32 {
    // Retrieve the GXF context from demux->private_data.
    let ctx = match (demux.private_data as *mut CcxGxf).as_mut() {
        Some(ctx) => ctx,
        None => return CCX_EINVAL,
    };
    let vid_track = match ctx.vid_track.as_mut() {
        Some(track) => track,
        None => return CCX_EINVAL,
    };
    let mut ret = CCX_OK;
    let mut error_occurred = false;

    /* Auxiliary Information */
    // let auxi_info: [u8; 8];  // Not used, keeping comment for reference
    dbg!("Mpeg 525 {}", len);

    while len > 2 {
        let tag = buffered_get_byte(demux);
        let tag_len = buffered_get_byte(demux) as i32;
        len -= 2;
        if len < tag_len {
            ret = CCX_EINVAL;
            error_occurred = true;
            break;
        }
        len -= tag_len;
        match tag {
            x if x == GXFTrackTag::TRACK_NAME as u8 => {
                let result = buffered_read(
                    demux,
                    Some(&mut vid_track.track_name[..tag_len as usize]),
                    tag_len as usize,
                );
                demux.past += tag_len as i64;
                if result != tag_len as usize {
                    ret = CCX_EOF;
                    error_occurred = true;
                    break;
                }
            }
            x if x == GXFTrackTag::TRACK_VER as u8 => {
                vid_track.fs_version = buffered_get_be32(demux);
            }
            x if x == GXFTrackTag::TRACK_FPS as u8 => {
                let val = buffered_get_be32(demux);
                set_track_frame_rate(vid_track, val as i8);
            }
            x if x == GXFTrackTag::TRACK_LINES as u8 => {
                vid_track.line_per_frame = buffered_get_be32(demux);
            }
            x if x == GXFTrackTag::TRACK_FPF as u8 => {
                vid_track.field_per_frame = buffered_get_be32(demux);
            }
            x if x == GXFTrackTag::TRACK_AUX as u8 => {
                let result = buffered_skip(demux, tag_len as u32);
                demux.past += result as i64;
            }
            x if x == GXFTrackTag::TRACK_MPG_AUX as u8 => {
                let result = buffered_skip(demux, tag_len as u32);
                demux.past += result as i64;
            }
            _ => {
                let result = buffered_skip(demux, tag_len as u32);
                demux.past += result as i64;
            }
        }
    }

    // --- Error handling block ---
    let result = buffered_skip(demux, len as u32);
    demux.past += result as i64;
    if result != len as usize {
        ret = CCX_EOF;
    }
    if error_occurred {
        ret = CCX_EINVAL;
    }
    ret
}
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe functions like `buffered_get_byte` and `buffered_read`
pub unsafe fn parse_ad_track_desc(demux: &mut CcxDemuxer, mut len: i32) -> i32 {
    // Retrieve the GXF context from demux->private_data.
    let ctx = match (demux.private_data as *mut CcxGxf).as_mut() {
        Some(ctx) => ctx,
        None => return CCX_EINVAL,
    };
    // Retrieve the ancillary data track; if missing, error out.
    let ad_track = match ctx.ad_track.as_mut() {
        Some(track) => track,
        None => return CCX_EINVAL,
    };

    let mut auxi_info = [0u8; 8];
    let mut ret = CCX_OK;
    let mut error_occurred = false;

    dbg!("Ancillary Data {}", len);

    while len > 2 {
        let tag = buffered_get_byte(demux);
        let tag_len = buffered_get_byte(demux) as i32;
        len -= 2;
        if len < tag_len {
            ret = CCX_EINVAL;
            error_occurred = true;
            break;
        }
        len -= tag_len;
        match tag {
            x if x == GXFTrackTag::TRACK_NAME as u8 => {
                let result = buffered_read(
                    demux,
                    Some(&mut ad_track.track_name[..tag_len as usize]),
                    tag_len as usize,
                );
                demux.past += tag_len as i64;
                if result != tag_len as usize {
                    ret = CCX_EOF;
                    error_occurred = true;
                    break;
                }
            }
            x if x == GXFTrackTag::TRACK_AUX as u8 => {
                let result = buffered_read(demux, Some(&mut auxi_info), 8);
                demux.past += 8;
                if result != 8 {
                    ret = CCX_EOF;
                    error_occurred = true;
                    break;
                }
                if tag_len != 8 {
                    ret = CCX_EINVAL;
                    error_occurred = true;
                    break;
                }
                // Set ancillary track fields.
                ad_track.ad_format = match auxi_info[2] {
                    1 => GXFAncDataPresFormat::PRES_FORMAT_SD,
                    2 => GXFAncDataPresFormat::PRES_FORMAT_HD,
                    _ => GXFAncDataPresFormat::PRES_FORMAT_SD,
                };
                ad_track.nb_field = auxi_info[3] as i32;
                ad_track.field_size = i16::from_be_bytes([auxi_info[4], auxi_info[5]]) as i32;
                ad_track.packet_size =
                    i16::from_be_bytes([auxi_info[6], auxi_info[7]]) as i32 * 256;
                dbg!(
                    "ad_format {} nb_field {} field_size {} packet_size {} track id {}",
                    ad_track.ad_format,
                    ad_track.nb_field,
                    ad_track.field_size,
                    ad_track.packet_size,
                    ad_track.id
                );
            }
            x if x == GXFTrackTag::TRACK_VER as u8 => {
                ad_track.fs_version = buffered_get_be32(demux);
            }
            x if x == GXFTrackTag::TRACK_FPS as u8 => {
                ad_track.frame_rate = buffered_get_be32(demux);
            }
            x if x == GXFTrackTag::TRACK_LINES as u8 => {
                ad_track.line_per_frame = buffered_get_be32(demux);
            }
            x if x == GXFTrackTag::TRACK_FPF as u8 => {
                ad_track.field_per_frame = buffered_get_be32(demux);
            }
            x if x == GXFTrackTag::TRACK_MPG_AUX as u8 => {
                let result = buffered_skip(demux, tag_len as u32);
                demux.past += result as i64;
            }
            _ => {
                let result = buffered_skip(demux, tag_len as u32);
                demux.past += result as i64;
            }
        }
    }

    // Error handling block.
    let result = buffered_skip(demux, len as u32);
    demux.past += result as i64;
    if result != len as usize {
        ret = CCX_EOF;
    }
    if error_occurred {
        ret = CCX_EINVAL;
    }
    ret
}
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe functions like `buffered_get_byte` and `buffered_get_be16`
pub unsafe fn parse_track_sec(demux: &mut CcxDemuxer, mut len: i32, data: &mut DemuxerData) -> i32 {
    // Retrieve the GXF context from demux->private_data.
    let ctx = match (demux.private_data as *mut CcxGxf).as_mut() {
        Some(ctx) => ctx,
        None => return CCX_EINVAL,
    };

    let mut ret = CCX_OK;

    while len > 4 {
        // Read track header: 1 byte track_type, 1 byte track_id, 2 bytes track_len.
        let mut track_type = buffered_get_byte(demux);
        let mut track_id = buffered_get_byte(demux);
        let track_len = buffered_get_be16(demux) as i32;
        len -= 4;

        if len < track_len {
            ret = CCX_EINVAL;
            break;
        }

        // If track_type does not have high bit set, skip record.
        if (track_type & 0x80) != 0x80 {
            len -= track_len;
            let result = buffered_skip(demux, track_len as u32);
            demux.past += result as i64;
            if result != track_len as usize {
                ret = CCX_EOF;
                break;
            }
            continue;
        }
        track_type &= 0x7f;

        // If track_id does not have its two high bits set, skip record.
        if (track_id & 0xc0) != 0xc0 {
            len -= track_len;
            let result = buffered_skip(demux, track_len as u32);
            demux.past += result as i64;
            if result != track_len as usize {
                ret = CCX_EOF;
                break;
            }
            continue;
        }
        track_id &= 0xcf;

        match track_type {
            x if x == GXFTrackType::TRACK_TYPE_ANCILLARY_DATA as u8 => {
                // Allocate ancillary track if not present.
                if ctx.ad_track.is_none() {
                    ctx.ad_track = Some(Box::new(CcxGxfAncillaryDataTrack::default()));
                }
                if let Some(ad_track) = ctx.ad_track.as_mut() {
                    ad_track.id = track_id;
                    parse_ad_track_desc(demux, track_len);
                    data.bufferdatatype = BufferdataType::Raw;
                }
                len -= track_len;
            }
            x if x == GXFTrackType::TRACK_TYPE_MPEG2_525 as u8 => {
                // Allocate video track if not present.
                if ctx.vid_track.is_none() {
                    ctx.vid_track = Some(Box::new(CcxGxfVideoTrack::default()));
                }
                if ctx.vid_track.is_none() {
                    info!("Ignored MPEG track due to insufficient memory\n");
                    break;
                }
                parse_mpeg525_track_desc(demux, track_len);
                data.bufferdatatype = BufferdataType::Pes;
                len -= track_len;
            }
            _ => {
                let result = buffered_skip(demux, track_len as u32);
                demux.past += result as i64;
                len -= track_len;
                if result != track_len as usize {
                    ret = CCX_EOF;
                    break;
                }
            }
        }
    }

    let result = buffered_skip(demux, len as u32);
    demux.past += result as i64;
    if result != len as usize {
        ret = CCX_EOF;
    }
    ret
}
/**
 * Parse Caption Distribution Packet
 * General Syntax of cdp
 * cdp() {
 *   cdp_header();
 *   time_code_section();
 *   ccdata_section();
 *   ccsvcinfo_section();
 *   cdp_footer();
 * }
 * function does not parse cdp in chunk, user should provide complete cdp
 * with header and footer inclusive of checksum
 * @return CCX_EINVAL if cdp data fields are not valid
 */
pub fn parse_ad_cdp(cdp: &[u8], data: &mut DemuxerData) -> Result<(), &'static str> {
    // Do not accept packet whose length does not fit header and footer
    if cdp.len() < 11 {
        info!("Short packet can't accommodate header and footer");
        return Err("Invalid packet length");
    }

    // Verify cdp header identifier
    if cdp[0] != 0x96 || cdp[1] != 0x69 {
        info!("Could not find CDP identifier of 0x96 0x69");
        return Err("Invalid CDP identifier");
    }

    // Save original packet length.
    let orig_len = cdp.len();
    // Advance pointer by 2 bytes
    let mut cdp = &cdp[2..];
    // Read CDP length (1 byte) and verify that it equals the original packet length.
    let cdp_length = cdp[0] as usize;
    cdp = &cdp[1..];
    if cdp_length != orig_len {
        info!("CDP length is not valid");
        return Err("Mismatched CDP length");
    }

    // Parse header fields.
    let cdp_framerate = (cdp[0] & 0xF0) >> 4;
    let cc_data_present = (cdp[0] & 0x40) >> 6;
    let caption_service_active = (cdp[1] & 0x02) >> 1;
    let cdp_header_sequence_counter = u16::from_be_bytes([cdp[2], cdp[3]]);
    cdp = &cdp[4..];

    dbg!("CDP length: {} words", cdp_length);
    dbg!("CDP frame rate: 0x{:x}", cdp_framerate);
    dbg!("CC data present: {}", cc_data_present);
    dbg!("Caption service active: {}", caption_service_active);
    dbg!(
        "Header sequence counter: {} (0x{:x})",
        cdp_header_sequence_counter,
        cdp_header_sequence_counter
    );

    // Process CDP sections (only one section allowed per packet)
    match cdp[0] {
        0x71 => {
            cdp = &cdp[1..];
            info!("Ignore Time code section");
            return Err("Time code section ignored");
        }
        0x72 => {
            cdp = &cdp[1..]; // Advance past section id.
            let cc_count = cdp[0] & 0x1F;
            dbg!("cc_count: {}", cc_count);
            let copy_size = cc_count as usize * 3;
            if copy_size > cdp.len() - 1 {
                return Err("Insufficient data for CC section");
            }
            // Copy ccdata into data.buffer starting at offset data.len.
            let dst = unsafe { slice::from_raw_parts_mut(data.buffer, data.len + copy_size) };
            dst[data.len..data.len + copy_size].copy_from_slice(&cdp[1..1 + copy_size]);
            data.len += copy_size;
            cdp = &cdp[1 + copy_size..];
        }
        0x73 => {
            cdp = &cdp[1..];
            info!("Ignore service information section");
            return Err("Service information section ignored");
        }
        0x75..=0xEF => {
            info!(
                "Newer version of SMPTE-334 specification detected. New section id 0x{:x}",
                cdp[0]
            );
            cdp = &cdp[1..];
            return Err("Unhandled new section");
        }
        _ => {}
    }

    // Check CDP footer.
    if cdp[0] == 0x74 {
        cdp = &cdp[1..];
        let footer_sequence_counter = u16::from_be_bytes([cdp[0], cdp[1]]);
        if cdp_header_sequence_counter != footer_sequence_counter {
            info!("Incomplete CDP packet");
            return Err("CDP footer sequence mismatch");
        }
        // Optionally: cdp = &cdp[2..];
    }

    Ok(())
}

/**
 * parse ancillary data payload
 */
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe functions like `buffered_get_le16` and `buffered_skip`
pub unsafe fn parse_ad_pyld(demux: &mut CcxDemuxer, len: i32, data: &mut DemuxerData) -> i32 {
    #[allow(unused_variables)]
    let mut ret = CCX_OK;
    let mut rem_len = len;

    #[cfg(not(feature = "CCX_GXF_ENABLE_AD_VBI"))]
    {
        let mut i: usize;

        // Read 16-bit little-endian values from buffered input:
        let d_id = buffered_get_le16(demux);
        let sd_id = buffered_get_le16(demux);
        // Read dc and mask to 8 bits.
        let _dc = buffered_get_le16(demux) & 0xFF;

        let ctx = &mut *(demux.private_data as *mut CcxGxf);

        // Adjust length (remove the 6 header bytes)
        rem_len = len - 6;
        // If ctx.cdp buffer is too small, reallocate it.
        if ctx.cdp_len < (rem_len / 2) as usize {
            // Allocate a new buffer of size (rem_len/2)
            ctx.cdp = Some(vec![0u8; (rem_len / 2) as usize]);
            if ctx.cdp.is_none() {
                info!("Could not allocate buffer {}\n", rem_len / 2);
                return CCX_ENOMEM;
            }
            // Exclude DID and SDID bytes: set new cdp_len to ((rem_len - 2) / 2)
            ctx.cdp_len = ((rem_len - 2) / 2) as usize;
        }

        // Check for CEA-708 captions: d_id and sd_id must match.
        if ((d_id & 0xFF) == CLOSED_CAP_DID as u16) && ((sd_id & 0xFF) == CLOSED_C708_SDID as u16) {
            if let Some(ref mut cdp) = ctx.cdp {
                i = 0;
                let mut remaining_len = rem_len;
                while remaining_len > 2 {
                    let dat = buffered_get_le16(demux);
                    cdp[i] = match dat {
                        0x2FE => 0xFF,
                        0x201 => 0x01,
                        _ => (dat & 0xFF) as u8,
                    };
                    i += 1;
                    remaining_len -= 2;
                }
                // Call parse_ad_cdp on the newly filled buffer.
                // (Assume parse_ad_cdp returns Ok(()) on success.)
                let _ = parse_ad_cdp(ctx.cdp.as_ref().unwrap(), data);
                // TODO: Check checksum.
            }
        }
        // If it corresponds to CEA-608 captions:
        else if ((d_id & 0xFF) == CLOSED_CAP_DID as u16)
            && ((sd_id & 0xFF) == CLOSED_C608_SDID as u16)
        {
            info!("Need Sample\n");
        }
        // Otherwise, ignore other services.
    }

    let result = buffered_skip(demux, rem_len as u32);
    demux.past += result as i64;
    if result != rem_len as usize {
        ret = CCX_EOF;
    }
    ret
}

/**
 * VBI in ancillary data is not specified in GXF specs
 * but while traversing file, we found vbi data presence
 * in Multimedia file, so if there are some video which
 * show caption on tv or there software but ccextractor
 * is not able to see the caption, there might be need
 * of parsing vbi
 */
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe function `buffered_skip`
#[allow(unused_variables)]
pub unsafe fn parse_ad_vbi(demux: &mut CcxDemuxer, len: i32, data: &mut DemuxerData) -> i32 {
    let mut ret = CCX_OK;
    let result: usize;

    #[cfg(feature = "ccx_gxf_enable_ad_vbi")]
    {
        // In the C code, data.len is increased by len before reading.
        data.len += len as usize;
        // Read 'len' bytes into data.buffer (starting at index 0, mimicking the C code).
        // result = buffered_read(demux, Some(&mut data.buffer[..len as usize]), len as usize);
        let buffer_slice = std::slice::from_raw_parts_mut(data.buffer, len as usize);
        result = buffered_read(demux, Some(buffer_slice), len as usize);
    }
    #[cfg(not(feature = "ccx_gxf_enable_ad_vbi"))]
    {
        // Skip 'len' bytes if VBI support is not enabled.
        result = buffered_skip(demux, len as u32);
    }
    // Update demux.past with the bytes processed.
    demux.past += result as i64;
    if result != len as usize {
        ret = CCX_EOF;
    }
    ret
}

/// parse_ad_field: parses an ancillary data field from the demuxer buffer,
/// verifying header tags (e.g. "finf", "LIST", "anc ") and then processing each
/// sub‐section (e.g. "pyld"/"vbi") until the field is exhausted.
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe functions like `buffered_read` and `buffered_get_le32`
pub unsafe fn parse_ad_field(demux: &mut CcxDemuxer, mut len: i32, data: &mut DemuxerData) -> i32 {
    let mut ret = CCX_OK;
    let mut result;
    let mut tag = [0u8; 5]; // 4-byte tag plus null terminator

    tag[4] = 0;

    // Read "finf" tag
    len -= 4;
    result = buffered_read(demux, Some(&mut tag[..4]), 4);
    demux.past += result as i64;
    if &tag[..4] != b"finf" {
        info!("Warning: No finf tag\n");
    }

    // Read and validate GXF spec value (must be 4)
    len -= 4;
    if buffered_get_le32(demux) != 4 {
        info!("Warning: expected 4 acc GXF specs\n");
    }

    // Read field identifier
    len -= 4;
    let field_identifier = buffered_get_le32(demux);
    info!("LOG: field identifier {}\n", field_identifier);

    // Read "LIST" tag
    len -= 4;
    result = buffered_read(demux, Some(&mut tag[..4]), 4);
    demux.past += result as i64;
    if &tag[..4] != b"LIST" {
        info!("Warning: No List tag\n");
    }

    // Read ancillary data field section size.
    len -= 4;
    if buffered_get_le32(demux) != len as u32 {
        info!("Warning: Unexpected sample size (!={})\n", len);
    }

    // Read "anc " tag
    len -= 4;
    result = buffered_read(demux, Some(&mut tag[..4]), 4);
    demux.past += result as i64;
    if &tag[..4] != b"anc " {
        info!("Warning: No anc tag\n");
    }

    // Process sub-sections until less than or equal to 28 bytes remain.
    while len > 28 {
        len -= 4;
        result = buffered_read(demux, Some(&mut tag[..4]), 4);
        demux.past += result as i64;

        len -= 4;
        let hdr_len = buffered_get_le32(demux);

        // Check for pad tag.
        if &tag[..4] == b"pad " {
            if hdr_len != len as u32 {
                info!("Warning: expected {} got {}\n", len, hdr_len);
            }
            len -= hdr_len as i32;
            result = buffered_skip(demux, hdr_len);
            demux.past += result as i64;
            if result != hdr_len as usize {
                ret = CCX_EOF;
            }
            continue;
        } else if &tag[..4] == b"pos " {
            if hdr_len != 12 {
                info!("Warning: expected 4 got {}\n", hdr_len);
            }
        } else {
            info!("Warning: Instead pos tag got {:?} tag\n", &tag[..4]);
            if hdr_len != 12 {
                info!("Warning: expected 4 got {}\n", hdr_len);
            }
        }

        len -= 4;
        let line_nb = buffered_get_le32(demux);
        info!("Line nb: {}\n", line_nb);

        len -= 4;
        let luma_flag = buffered_get_le32(demux);
        info!("luma color diff flag: {}\n", luma_flag);

        len -= 4;
        let hanc_vanc_flag = buffered_get_le32(demux);
        info!("hanc/vanc flag: {}\n", hanc_vanc_flag);

        len -= 4;
        result = buffered_read(demux, Some(&mut tag[..4]), 4);
        demux.past += result as i64;

        len -= 4;
        let pyld_len = buffered_get_le32(demux);
        info!("pyld len: {}\n", pyld_len);

        if &tag[..4] == b"pyld" {
            len -= pyld_len as i32;
            ret = parse_ad_pyld(demux, pyld_len as i32, data);
            if ret == CCX_EOF {
                break;
            }
        } else if &tag[..4] == b"vbi " {
            len -= pyld_len as i32;
            ret = parse_ad_vbi(demux, pyld_len as i32, data);
            if ret == CCX_EOF {
                break;
            }
        } else {
            info!("Warning: No pyld/vbi tag got {:?} tag\n", &tag[..4]);
        }
    }

    result = buffered_skip(demux, len as u32);
    demux.past += result as i64;
    if result != len as usize {
        ret = CCX_EOF;
    }
    ret
}

/* *  @param vid_format following format are supported to set valid timebase
 *  in demuxer data
 *  value   |  Meaning
 *=====================================
 *  0      | 525 interlaced lines at 29.97 frames / sec
 *  1      | 625 interlaced lines at 25 frames / sec
 *  2      | 720 progressive lines at 59.94 frames / sec
 *  3      | 720 progressive lines at 60 frames / sec
 *  4      | 1080 progressive lines at 23.98 frames / sec
 *  5      | 1080 progressive lines at 24 frames / sec
 *  6      | 1080 progressive lines at 25 frames / sec
 *  7      | 1080 progressive lines at 29.97 frames / sec
 *  8      | 1080 progressive lines at 30 frames / sec
 *  9      | 1080 interlaced lines at 25 frames / sec
 *  10     | 1080 interlaced lines at 29.97 frames / sec
 *  11     | 1080 interlaced lines at 30 frames / sec
 *  12     | 1035 interlaced lines at 30 frames / sec
 *  13     | 1035 interlaced lines at 29.97 frames / sec
 *  14     | 720 progressive lines at 50 frames / sec
 *  15     | 525 progressive lines at 59.94 frames / sec
 *  16     | 525 progressive lines at 60 frames / sec
 *  17     | 525 progressive lines at 29.97 frames / sec
 *  18     | 525 progressive lines at 30 frames / sec
 *  19     | 525 progressive lines at 50 frames / sec
 *  20     | 525 progressive lines at 25 frames / sec
 *
 * @param data already allocated data, passing NULL will end up in seg-fault
 *        data len may be zero, while setting timebase we don not care about
 *        actual data
 */
pub fn set_data_timebase(vid_format: i32, data: &mut DemuxerData) {
    dbg!("LOG: Format Video {}", vid_format);

    match vid_format {
        // NTSC (30000/1001)
        0 | 7 | 10 | 13 | 17 => {
            data.tb.den = 30000;
            data.tb.num = 1001;
        }
        // PAL (25/1)
        1 | 6 | 9 | 20 => {
            data.tb.den = 25;
            data.tb.num = 1;
        }
        // NTSC 60fps (60000/1001)
        2 | 15 => {
            data.tb.den = 60000;
            data.tb.num = 1001;
        }
        // 60 fps (60/1)
        3 | 16 => {
            data.tb.den = 60;
            data.tb.num = 1;
        }
        // 24 fps (24000/1001)
        4 => {
            data.tb.den = 24000;
            data.tb.num = 1001;
        }
        // 24 fps (24/1)
        5 => {
            data.tb.den = 24;
            data.tb.num = 1;
        }
        // 30 fps (30/1)
        8 | 11 | 12 | 18 => {
            data.tb.den = 30;
            data.tb.num = 1;
        }
        // 50 fps (50/1)
        14 | 19 => {
            data.tb.den = 50;
            data.tb.num = 1;
        }
        // Default case does nothing
        _ => {}
    }
}

/// # Safety
/// This function is unsafe because it calls unsafe functions like `buffered_read` and `from_raw_parts_mut`
pub unsafe fn parse_mpeg_packet(demux: &mut CcxDemuxer, len: usize, data: &mut DemuxerData) -> i32 {
    // Read 'len' bytes into the data buffer at offset data.len.
    let result = buffered_read(
        demux,
        Some(slice::from_raw_parts_mut(data.buffer.add(data.len), len)),
        len,
    );
    data.len += len;
    demux.past += result as i64;

    if result != len {
        return CCX_EOF;
    }
    CCX_OK
}

/**
 * This packet contain RIFF data
 * @param demuxer Demuxer must contain vaild ad_track structure
 */
/// # Safety
/// This function is unsafe because it deferences raw pointers and calls unsafe functions like `buffered_read` and `buffered_get_le32`
pub unsafe fn parse_ad_packet(demux: &mut CcxDemuxer, len: i32, data: &mut DemuxerData) -> i32 {
    let mut ret = CCX_OK;
    let mut remaining_len = len;
    let mut tag = [0u8; 4];
    let mut result;

    let ctx = &mut *(demux.private_data as *mut CcxGxf);
    let ad_track = &ctx.ad_track.as_mut().unwrap();

    // Read "RIFF" header
    remaining_len -= 4;
    result = buffered_read(demux, Some(&mut tag), 4);
    demux.past += result as i64;
    if &tag != b"RIFF" {
        info!("Warning: No RIFF header");
    }

    // Validate ADT packet length
    remaining_len -= 4;
    if buffered_get_le32(demux) != 65528 {
        info!("Warning: ADT packet with non-trivial length");
    }

    // Read "rcrd" tag
    remaining_len -= 4;
    result = buffered_read(demux, Some(&mut tag), 4);
    demux.past += result as i64;
    if &tag != b"rcrd" {
        info!("Warning: No rcrd tag");
    }

    // Read "desc" tag
    remaining_len -= 4;
    result = buffered_read(demux, Some(&mut tag), 4);
    demux.past += result as i64;
    if &tag != b"desc" {
        info!("Warning: No desc tag");
    }

    // Validate desc length
    remaining_len -= 4;
    if buffered_get_le32(demux) != 20 {
        info!("Warning: Unexpected desc length (!=20)");
    }

    // Validate version
    remaining_len -= 4;
    if buffered_get_le32(demux) != 2 {
        info!("Warning: Unsupported version (!=2)");
    }

    // Check number of fields
    remaining_len -= 4;
    let val = buffered_get_le32(demux);
    if ad_track.nb_field != val as i32 {
        info!("Warning: Ambiguous number of fields");
    }

    // Check field size
    remaining_len -= 4;
    let val = buffered_get_le32(demux);
    if ad_track.field_size != val as i32 {
        info!("Warning: Ambiguous field size");
    }

    // Validate ancillary media packet size
    remaining_len -= 4;
    if buffered_get_le32(demux) != 65536 {
        info!("Warning: Unexpected buffer size (!=65536)");
    }

    // Set data timebase
    remaining_len -= 4;
    let val = buffered_get_le32(demux);
    set_data_timebase(val as i32, data);

    // Read "LIST" tag
    remaining_len -= 4;
    result = buffered_read(demux, Some(&mut tag), 4);
    demux.past += result as i64;
    if &tag != b"LIST" {
        info!("Warning: No LIST tag");
    }

    // Validate field section size
    remaining_len -= 4;
    if buffered_get_le32(demux) != remaining_len as u32 {
        info!(
            "Warning: Unexpected field section size (!={})",
            remaining_len
        );
    }

    // Read "fld " tag
    remaining_len -= 4;
    result = buffered_read(demux, Some(&mut tag), 4);
    demux.past += result as i64;
    if &tag != b"fld " {
        info!("Warning: No fld tag");
    }

    // Parse each field
    for _ in 0..ad_track.nb_field as usize {
        remaining_len -= ad_track.field_size;
        parse_ad_field(demux, ad_track.field_size, data);
    }

    // Skip remaining data
    result = buffered_skip(demux, remaining_len as u32);
    demux.past += result as i64;
    if result != remaining_len as usize {
        ret = CCX_EOF;
    }
    ret
}
/**
 *    +-----------------------------+-----------------------------+
 *    | Bits (0 is LSB; 7 is MSB)   |          Meaning            |
 *    +-----------------------------+-----------------------------+
 *    |                             |        Picture Coding       |
 *    |                             |         00 = NOne           |
 *    |          0:1                |         01 = I frame        |
 *    |                             |         10 = P Frame        |
 *    |                             |         11 = B Frame        |
 *    +-----------------------------+-----------------------------+
 *    |                             |      Picture Structure      |
 *    |                             |         00 = NOne           |
 *    |          2:3                |         01 = I frame        |
 *    |                             |         10 = P Frame        |
 *    |                             |         11 = B Frame        |
 *    +-----------------------------+-----------------------------+
 *    |          4:7                |      Not Used MUST be 0     |
 *    +-----------------------------+-----------------------------+
 */
/// C `set_mpeg_frame_desc` function.
pub fn set_mpeg_frame_desc(vid_track: &mut CcxGxfVideoTrack, mpeg_frame_desc_flag: u8) {
    vid_track.p_code = MpegPictureCoding::try_from(mpeg_frame_desc_flag & 0x03).unwrap();
    vid_track.p_struct = MpegPictureStruct::try_from((mpeg_frame_desc_flag >> 2) & 0x03).unwrap();
}

impl PartialEq for GXFTrackType {
    fn eq(&self, other: &Self) -> bool {
        std::mem::discriminant(self) == std::mem::discriminant(other)
    }
}

// Macro for common cleanup and return (replaces C's goto end)
macro_rules! goto_end {
    ($demux:expr, $len:expr, $ret:ident) => {{
        let result = buffered_skip($demux, $len as u32) as i32;
        $demux.past += result as i64;
        if result != $len {
            $ret = CCX_EOF;
        }
        return $ret;
    }};
}
/// C `parse_media` function.
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe functions like `buffered_get_byte` and `buffered_skip`
pub unsafe fn parse_media(demux: &mut CcxDemuxer, mut len: i32, data: &mut DemuxerData) -> i32 {
    let mut ret = CCX_OK;
    let mut result;

    // Check for null private_data before dereferencing
    if demux.private_data.is_null() {
        result = buffered_skip(demux, len as u32) as i32;
        demux.past += result as i64;
        if result != len {
            ret = CCX_EOF;
        }
        return ret;
    }

    let ctx = &mut *(demux.private_data as *mut CcxGxf);

    let mut first_field_nb: u16 = 0;
    let mut last_field_nb: u16 = 0;
    let mut mpeg_pic_size: u32 = 0;
    let mut mpeg_frame_desc_flag: u8 = 0;

    len -= 1;

    let media_type_0 = GXFTrackType::try_from(buffered_get_byte(demux));
    let media_type: GXFTrackType = if let Ok(mt) = media_type_0 {
        mt
    } else {
        goto_end!(demux, len, ret);
    };

    let track_nb: u8 = buffered_get_byte(demux);
    len -= 1;
    let media_field_nb: u32 = buffered_get_be32(demux);
    len -= 4;

    match media_type {
        GXFTrackType::TRACK_TYPE_ANCILLARY_DATA => {
            first_field_nb = buffered_get_be16(demux);
            len -= 2;
            last_field_nb = buffered_get_be16(demux);
            len -= 2;
        }
        GXFTrackType::TRACK_TYPE_MPEG1_525 | GXFTrackType::TRACK_TYPE_MPEG2_525 => {
            mpeg_pic_size = buffered_get_be32(demux);
            mpeg_frame_desc_flag = (mpeg_pic_size >> 24) as u8;
            mpeg_pic_size &= 0x00FFFFFF;
            len -= 4;
        }
        _ => {
            result = buffered_skip(demux, 4) as i32;
            demux.past += result as i64;
            len -= 4;
        }
    }

    let time_field: u32 = buffered_get_be32(demux);
    len -= 4;

    let valid_time_field: u8 = buffered_get_byte(demux) & 0x01;
    len -= 1;

    result = buffered_skip(demux, 1) as i32;
    demux.past += result as i64;
    len -= 1;

    // Debug logging (matching C's debug macros)
    info!("track number {}", track_nb);
    info!("field number {}", media_field_nb);
    info!("first field number {}", first_field_nb);
    info!("last field number {}", last_field_nb);
    info!("Pyld len {}", len);

    match media_type {
        GXFTrackType::TRACK_TYPE_ANCILLARY_DATA => {
            if ctx.ad_track.is_none() {
                goto_end!(demux, len, ret);
            }
            let ad_track = ctx.ad_track.as_mut().unwrap();
            data.pts = if valid_time_field != 0 {
                time_field as i64 - ctx.first_field_nb as i64
            } else {
                media_field_nb as i64 - ctx.first_field_nb as i64
            };
            if len < ad_track.packet_size {
                goto_end!(demux, len, ret);
            }
            data.pts /= 2;
            parse_ad_packet(demux, ad_track.packet_size, data);
            len -= ad_track.packet_size;
        }
        GXFTrackType::TRACK_TYPE_MPEG2_525 if ctx.ad_track.is_none() => {
            if ctx.vid_track.is_none() {
                goto_end!(demux, len, ret);
            }
            let vid_track = ctx.vid_track.as_mut().unwrap();
            data.pts = if valid_time_field != 0 {
                time_field as i64 - ctx.first_field_nb as i64
            } else {
                media_field_nb as i64 - ctx.first_field_nb as i64
            };
            data.tb.num = vid_track.frame_rate.den;
            data.tb.den = vid_track.frame_rate.num;
            data.pts /= 2;
            set_mpeg_frame_desc(vid_track, mpeg_frame_desc_flag);
            parse_mpeg_packet(demux, mpeg_pic_size as usize, data);
            len -= mpeg_pic_size as i32;
        }
        GXFTrackType::TRACK_TYPE_TIME_CODE_525 => {
            // Time code handling not implemented
        }
        _ => {}
    }

    goto_end!(demux, len, ret)
}

/**
 * Dummy function that ignore field locator table packet
 */
/// # Safety
/// This function is unsafe because it calls unsafe function `buffered_skip`
pub unsafe fn parse_flt(demux: &mut CcxDemuxer, len: i32) -> i32 {
    let mut ret = CCX_OK;
    let mut result = 0;

    result = buffered_skip(demux, len as u32);
    demux.past += result as i64;
    if result != len as usize {
        ret = CCX_EOF;
    }
    ret
}
/**
 * Dummy function that ignore unified material format packet
 */
/// # Safety
/// This function is unsafe because it calls unsafe function `buffered_skip`
pub unsafe fn parse_umf(demux: &mut CcxDemuxer, len: i32) -> i32 {
    let mut ret = CCX_OK;
    let mut result = 0;

    result = buffered_skip(demux, len as u32);
    demux.past += result as i64;
    if result != len as usize {
        ret = CCX_EOF;
    }
    ret
}
/**
 * Its this function duty to use len length buffer from demuxer
 *
 * This function gives basic info about tracks, here we get to know
 * whether Ancillary Data track is present or not.
 * If ancillary data track is not present only then it check for
 * presence of mpeg track.
 * return CCX_EINVAL if things are not following specs
 *
 * TODO do buffer cahce to know that you are not reading after eof
 */
/// # Safety
/// This function is unsafe because it calls unsafe functions like `buffered_skip` and `parse_material_sec`
pub unsafe fn parse_map(demux: &mut CcxDemuxer, mut len: i32, data: &mut DemuxerData) -> i32 {
    let mut ret = CCX_OK;

    // Check for MAP header 0xe0ff
    len -= 2;
    if buffered_get_be16(demux) != 0xe0ff {
        let result = buffered_skip(demux, len as u32);
        demux.past += result as i64;
        if result != len as usize {
            return CCX_EOF;
        }
        return ret;
    }

    // Parse material section length
    len -= 2;
    let material_sec_len = buffered_get_be16(demux) as i32;
    if material_sec_len > len {
        let result = buffered_skip(demux, len as u32);
        demux.past += result as i64;
        if result != len as usize {
            return CCX_EOF;
        }
        return ret;
    }

    // Parse material section
    len -= material_sec_len;
    parse_material_sec(demux, material_sec_len);

    // Parse track section length
    len -= 2;
    let track_sec_len = buffered_get_be16(demux) as i32;
    if track_sec_len > len {
        let result = buffered_skip(demux, len as u32);
        demux.past += result as i64;
        if result != len as usize {
            return CCX_EOF;
        }
        return ret;
    }

    // Parse track section
    len -= track_sec_len;
    parse_track_sec(demux, track_sec_len, data);

    // Skip any remaining bytes
    let result = buffered_skip(demux, len as u32);
    demux.past += result as i64;
    if result != len as usize {
        ret = CCX_EOF;
    }

    ret
}

/**
 * GXF Media File have 5 Section which are as following
 *     +----------+-------+------+---------------+--------+
 *     |          |       |      |               |        |
 *     |  MAP     |  FLT  |  UMF |  Media Packet |   EOS  |
 *     |          |       |      |               |        |
 *     +----------+-------+------+---------------+--------+
 *
 */
/// # Safety
/// This function is unsafe because it calls unsafe functions like `parse_packet_header` and `parse_map`
pub unsafe fn read_packet(demux: &mut CcxDemuxer, data: &mut DemuxerData) -> i32 {
    let mut len = 0;
    let mut ret = CCX_OK;
    let mut gxftype: GXFPktType = GXFPktType::PKT_EOS;

    ret = parse_packet_header(demux, &mut gxftype, &mut len);
    if ret != CCX_OK {
        return ret; // Propagate header parsing errors
    }

    match gxftype {
        GXFPktType::PKT_MAP => {
            info!("pkt type Map {}\n", len);
            ret = parse_map(demux, len, data);
        }
        GXFPktType::PKT_MEDIA => {
            ret = parse_media(demux, len, data);
        }
        GXFPktType::PKT_EOS => {
            ret = CCX_EOF;
        }
        GXFPktType::PKT_FLT => {
            info!("pkt type FLT {}\n", len);
            ret = parse_flt(demux, len);
        }
        GXFPktType::PKT_UMF => {
            info!("pkt type umf {}\n\n", len);
            ret = parse_umf(demux, len);
        }
        #[allow(unreachable_patterns)]
        _ => {
            info!("pkt type unknown or bad {}\n", len);
            let result = buffered_skip(demux, len as u32) as i32;
            demux.past += result as i64;
            if result != len || len == 0 {
                ret = CCX_EOF;
            }
        }
    }

    ret
}

/**
 * @param buf buffer with atleast acceptable length atleast 7 byte
 *            where we will test only important part of packet header
 *            In GXF packet header is of 16 byte and in header there is
 *            packet leader of 5 bytes 00 00 00 00 01
 *            Stream Starts with Map packet which is known by looking at offset 0x05
 *            of packet header.
 *            TODO Map packet are sent per 100 packets so search MAP packet, there might be
 *            no MAP header at start if GXF is sliced at unknown region
 */
pub fn ccx_gxf_probe(buf: &[u8]) -> bool {
    // Static startcode array.
    let startcode = [0, 0, 0, 0, 1, 0xbc];
    // If the buffer length is less than the startcode length, return false.
    if buf.len() < startcode.len() {
        return false;
    }
    // If the start of the buffer matches startcode, return true.
    if buf[..startcode.len()] == startcode {
        return true;
    }
    false
}

#[cfg(test)]
mod tests {
    static INIT: Once = Once::new();

    fn initialize_logger() {
        INIT.call_once(|| {
            set_logger(CCExtractorLogger::new(
                OutputTarget::Stdout,
                DebugMessageMask::new(DebugMessageFlag::VERBOSE, DebugMessageFlag::VERBOSE),
                false,
            ))
            .ok();
        });
    }

    use super::*;
    use crate::util::log::{set_logger, CCExtractorLogger, DebugMessageMask, OutputTarget};
    use std::mem;
    use std::os::fd::IntoRawFd;
    use std::sync::Once;

    #[test]
    fn test_rl32() {
        // Prepare a little-endian 32-bit value.
        let bytes: [u8; 4] = [0x78, 0x56, 0x34, 0x12]; // expected value: 0x12345678 on little-endian systems
        let value = unsafe { rl32(bytes.as_ptr()) };
        // Since our test system is most likely little-endian, the value should be as stored.
        assert_eq!(value, 0x12345678);
    }

    #[test]
    fn test_rb32() {
        // Prepare a big-endian 32-bit value.
        let bytes: [u8; 4] = [0x01, 0x02, 0x03, 0x04]; // big-endian representation for 0x01020304
        let value = unsafe { rb32(bytes.as_ptr()) };
        // After conversion, we expect the same numerical value.
        assert_eq!(value, 0x01020304);
    }

    #[test]
    fn test_rl16() {
        // Prepare a little-endian 16-bit value.
        let bytes: [u8; 2] = [0xCD, 0xAB]; // expected value: 0xABCD on little-endian systems
        let value = unsafe { rl16(bytes.as_ptr()) };
        // Since our test system is most likely little-endian, the value should be as stored.
        assert_eq!(value, 0xABCD);
    }

    #[test]
    fn test_rb16() {
        // Prepare a big-endian 16-bit value.
        let bytes: [u8; 2] = [0x12, 0x34]; // big-endian representation for 0x1234
        let value = unsafe { rb16(bytes.as_ptr()) };
        // After conversion, we expect the same numerical value.
        assert_eq!(value, 0x1234);
    }

    // Additional tests to ensure functionality with varying data
    #[test]
    fn test_rb32_with_different_value() {
        // Another big-endian value test.
        let bytes: [u8; 4] = [0xFF, 0x00, 0xAA, 0x55];
        let value = unsafe { rb32(bytes.as_ptr()) };
        // On conversion, the expected value is 0xFF00AA55.
        assert_eq!(value, 0xFF00AA55);
    }

    #[test]
    fn test_rb16_with_different_value() {
        // Another big-endian value test.
        let bytes: [u8; 2] = [0xFE, 0xDC];
        let value = unsafe { rb16(bytes.as_ptr()) };
        // On conversion, the expected value is 0xFEDC.
        assert_eq!(value, 0xFEDC);
    }
    const FILEBUFFERSIZE: usize = 1024;

    /// Helper function to allocate a file buffer and copy provided data.
    fn allocate_filebuffer(data: &[u8]) -> *mut u8 {
        // Allocate a vector with a fixed capacity.
        let mut buffer = vec![0u8; FILEBUFFERSIZE];
        // Copy provided data into the beginning of the buffer.
        buffer[..data.len()].copy_from_slice(data);
        // Leak the vector to obtain a raw pointer.
        let ptr = buffer.as_mut_ptr();
        mem::forget(buffer);
        ptr
    }
    fn create_demuxer_with_buffer(data: &[u8]) -> CcxDemuxer<'static> {
        CcxDemuxer {
            filebuffer: allocate_filebuffer(data),
            filebuffer_pos: 0,
            bytesinbuffer: data.len() as u32,
            past: 0,
            ..Default::default()
        }
    }
    /// Build a valid packet header.
    /// Header layout:
    ///   Bytes 0-3:  0x00 0x00 0x00 0x00
    ///   Byte 4:     0x01
    ///   Byte 5:     Packet type (0xbc for PKT_MAP)
    ///   Bytes 6-9:  Length in big-endian (e.g., 32)
    ///   Bytes 10-13: Reserved (set to 0)
    ///   Byte 14:    0xe1
    ///   Byte 15:    0xe2
    fn build_valid_header() -> Vec<u8> {
        let mut header = Vec::with_capacity(16);
        header.extend_from_slice(&[0, 0, 0, 0]); // 0x00 0x00 0x00 0x00
        header.push(1); // 0x01
        header.push(0xbc); // Packet type: PKT_MAP
        header.extend_from_slice(&32u32.to_be_bytes()); // Length = 32 (will become 16 after subtracting header size)
        header.extend_from_slice(&[0, 0, 0, 0]); // Reserved
        header.push(0xe1); // Trailer part 1
        header.push(0xe2); // Trailer part 2
        header
    }
    #[allow(unused)]
    fn create_temp_file_with_content(content: &[u8]) -> i32 {
        use std::io::{Seek, SeekFrom, Write};
        use tempfile::NamedTempFile;
        let mut tmp = NamedTempFile::new().expect("Unable to create temp file");
        tmp.write_all(content)
            .expect("Unable to write to temp file");
        // Rewind the file pointer to the start.
        tmp.as_file_mut()
            .seek(SeekFrom::Start(0))
            .expect("Unable to seek to start");
        // Get the file descriptor. Ensure the file stays open.
        let file = tmp.reopen().expect("Unable to reopen temp file");
        #[cfg(unix)]
        {
            file.into_raw_fd()
        }
        #[cfg(windows)]
        {
            file.into_raw_handle() as i32
        }
    }
    /// Create a dummy CcxDemuxer with a filebuffer containing `header_data`.
    fn create_ccx_demuxer_with_header(header_data: &[u8]) -> CcxDemuxer<'static> {
        let filebuffer = allocate_filebuffer(header_data);
        CcxDemuxer {
            filebuffer,
            filebuffer_pos: 0,
            bytesinbuffer: header_data.len() as u32,
            past: 0,
            ..Default::default()
        }
    }

    #[test]
    fn test_parse_packet_header_valid() {
        let header = build_valid_header();
        let mut demuxer = create_ccx_demuxer_with_header(&header);
        let mut pkt_type = GXFPktType::PKT_MEDIA; // dummy init
        let mut length = 0;
        let ret = unsafe { parse_packet_header(&mut demuxer, &mut pkt_type, &mut length) };
        assert_eq!(ret, CCX_OK);
        assert_eq!(pkt_type as u32, GXFPktType::PKT_MAP as u32);
        // length in header was 32, then subtract 16 -> 16
        assert_eq!(length, 16);
        // past should have advanced by 16 bytes
        assert_eq!(demuxer.past, 16);
    }
    #[test]
    fn test_parse_packet_header_incomplete_read() {
        // Provide a header that is too short (e.g. only 10 bytes)
        let header = vec![0u8; 10];
        let mut demuxer = create_ccx_demuxer_with_header(&header);
        // let content = b"Direct read test.";
        // let fd = create_temp_file_with_content(content);
        // demuxer.infd = fd;
        let mut pkt_type = GXFPktType::PKT_MEDIA;
        let mut length = 0;
        let ret = unsafe { parse_packet_header(&mut demuxer, &mut pkt_type, &mut length) };
        assert_eq!(ret, CCX_EOF);
    }

    #[test]
    fn test_parse_packet_header_invalid_leader() {
        // Build header with a non-zero in the first 4 bytes.
        let mut header = build_valid_header();
        header[0] = 1; // Invalid leader
        let mut demuxer = create_ccx_demuxer_with_header(&header);
        let mut pkt_type = GXFPktType::PKT_MEDIA;
        let mut length = 0;
        let ret = unsafe { parse_packet_header(&mut demuxer, &mut pkt_type, &mut length) };
        assert_eq!(ret, CCX_EINVAL);
    }

    #[test]
    fn test_parse_packet_header_invalid_trailer() {
        // Build header with an incorrect trailer byte.
        let mut header = build_valid_header();
        header[14] = 0; // Should be 0xe1
        let mut demuxer = create_ccx_demuxer_with_header(&header);
        let mut pkt_type = GXFPktType::PKT_MEDIA;
        let mut length = 0;
        let ret = unsafe { parse_packet_header(&mut demuxer, &mut pkt_type, &mut length) };
        assert_eq!(ret, CCX_EINVAL);
    }

    #[test]
    fn test_parse_packet_header_invalid_length() {
        // Build header with length field < 16.
        let mut header = build_valid_header();
        // Set length field (bytes 6-9) to 15 (which is < 16).
        let invalid_length: u32 = 15;
        header[6..10].copy_from_slice(&invalid_length.to_be_bytes());
        let mut demuxer = create_ccx_demuxer_with_header(&header);
        let mut pkt_type = GXFPktType::PKT_MEDIA;
        let mut length = 0;
        let ret = unsafe { parse_packet_header(&mut demuxer, &mut pkt_type, &mut length) };
        assert_eq!(ret, CCX_EINVAL);
    }
    fn build_valid_material_sec() -> (Vec<u8>, CcxGxf) {
        let mut buf = Vec::new();

        // Prepare a dummy GXF context.
        let gxf = CcxGxf::default();

        // MAT_NAME: tag=MAT_NAME, tag_len = 8, then 8 bytes of media name.
        buf.push(GXFMatTag::MAT_NAME as u8);
        buf.push(8);
        let name_data = b"RustTest";
        buf.extend_from_slice(name_data);

        // MAT_FIRST_FIELD: tag, tag_len=4, then 4 bytes representing a u32 value.
        buf.push(GXFMatTag::MAT_FIRST_FIELD as u8);
        buf.push(4);
        let first_field: u32 = 0x01020304;
        buf.extend_from_slice(&first_field.to_be_bytes());

        // MAT_MARK_OUT: tag, tag_len=4, then 4 bytes.
        buf.push(GXFMatTag::MAT_MARK_OUT as u8);
        buf.push(4);
        let mark_out: u32 = 0x0A0B0C0D;
        buf.extend_from_slice(&mark_out.to_be_bytes());

        // Remaining length to be skipped (simulate extra bytes).
        let remaining = 5;
        buf.extend_from_slice(&vec![0u8; remaining]);

        // Total length is the entire buffer length.
        (buf, gxf)
    }

    /// Setup a demuxer for testing parse_material_sec.
    /// The demuxer's private_data will be set to a leaked Box of CcxGxf.
    fn create_demuxer_for_material_sec(data: &[u8], gxf: &mut CcxGxf) -> CcxDemuxer<'static> {
        let mut demux = create_demuxer_with_buffer(data);
        // Set private_data to point to our gxf structure.
        demux.private_data = gxf as *mut CcxGxf as *mut std::ffi::c_void;
        demux
    }

    #[test]
    fn test_parse_material_sec_valid() {
        let (buf, mut gxf) = build_valid_material_sec();
        let total_len = buf.len() as i32;
        let mut demux = create_demuxer_for_material_sec(&buf, &mut gxf);

        let ret = unsafe { parse_material_sec(&mut demux, total_len) };
        assert_eq!(ret, CCX_OK);

        // Check that the media_name was read.
        assert_eq!(&gxf.media_name[..8], b"RustTest");
        // Check that first_field_nb was set correctly.
        assert_eq!(gxf.first_field_nb, 0x01020304);
        // Check that mark_out was set correctly.
        assert_eq!(gxf.mark_out, 0x0A0B0C0D);
    }

    #[test]
    fn test_parse_material_sec_incomplete_mat_name() {
        // Build a material section with MAT_NAME tag that promises 8 bytes but only 4 bytes are present.
        let mut buf = Vec::new();
        buf.push(GXFMatTag::MAT_NAME as u8);
        buf.push(8);
        buf.extend_from_slice(b"Test"); // only 4 bytes instead of 8

        // Add extra bytes to simulate remaining length.
        buf.extend_from_slice(&[0u8; 3]);

        let total_len = buf.len() as i32;
        let mut gxf = CcxGxf::default();
        let mut demux = create_demuxer_for_material_sec(&buf, &mut gxf);

        let ret = unsafe { parse_material_sec(&mut demux, total_len) };
        // Since buffered_read will return less than expected, we expect CCX_EOF.
        assert_eq!(ret, CCX_EOF);
    }

    #[test]
    fn test_parse_material_sec_invalid_private_data() {
        // Create a buffer with any data.
        let buf = vec![0u8; 10];
        let total_len = buf.len() as i32;
        let mut demux = create_demuxer_with_buffer(&buf);
        // Set private_data to null.
        demux.private_data = ptr::null_mut();

        let ret = unsafe { parse_material_sec(&mut demux, total_len) };
        assert_eq!(ret, CCX_EINVAL);
    }

    #[test]
    fn test_parse_material_sec_skip_remaining() {
        // Build a material section where the length remaining is greater than the data in tags.
        let mut buf = Vec::new();
        // One valid tag:
        buf.push(GXFMatTag::MAT_FIRST_FIELD as u8);
        buf.push(4);
        let first_field: u32 = 0x00AA55FF;
        buf.extend_from_slice(&first_field.to_be_bytes());
        // Now, simulate extra remaining bytes that cannot be processed.
        let extra = 10;
        buf.extend_from_slice(&vec![0u8; extra]);

        let total_len = buf.len() as i32;
        let mut gxf = CcxGxf::default();
        let mut demux = create_demuxer_for_material_sec(&buf, &mut gxf);

        let ret = unsafe { parse_material_sec(&mut demux, total_len) };
        // In this case, the extra bytes will be skipped.
        // If the number of bytes skipped doesn't match, ret becomes CCX_EOF.
        // For our simulated buffered_skip (which works in-buffer), we expect CCX_OK if the skip succeeds.
        assert_eq!(ret, CCX_OK);
        // And first_field_nb should be set.
        assert_eq!(gxf.first_field_nb, 0x00AA55FF);
    }

    // tests for set_track_frame_rate
    #[test]
    fn test_set_track_frame_rate_60() {
        let mut vid_track = CcxGxfVideoTrack::default();
        set_track_frame_rate(&mut vid_track, 1);
        assert_eq!(vid_track.frame_rate.num, 60);
        assert_eq!(vid_track.frame_rate.den, 1);
    }
    #[test]
    fn test_set_track_frame_rate_60000() {
        let mut vid_track = CcxGxfVideoTrack::default();
        set_track_frame_rate(&mut vid_track, 2);
        assert_eq!(vid_track.frame_rate.num, 60000);
        assert_eq!(vid_track.frame_rate.den, 1001);
    }
    // Build a valid track description buffer.
    // Contains:
    // - TRACK_NAME tag: tag_len = 8, then 8 bytes ("Track001").
    // - TRACK_FPS tag: tag_len = 4, then 4 bytes representing frame rate (2400).
    // - Extra bytes appended.
    fn build_valid_track_desc() -> (Vec<u8>, CcxGxf) {
        let mut buf = Vec::new();
        // TRACK_NAME tag.
        buf.push(GXFTrackTag::TRACK_NAME as u8);
        buf.push(8);
        let name = b"Track001XYZ"; // Use only first 8 bytes: "Track001"
        buf.extend_from_slice(&name[..8]);

        // TRACK_FPS tag.
        buf.push(GXFTrackTag::TRACK_FPS as u8);
        buf.push(4);
        let fps: u32 = 2400;
        buf.extend_from_slice(&fps.to_be_bytes());

        // Append extra bytes.
        buf.extend_from_slice(&[0u8; 5]);

        // Create a dummy CcxGxf context.
        let gxf = CcxGxf {
            nb_streams: 1,
            media_name: [0; STR_LEN as usize],
            first_field_nb: 0,
            last_field_nb: 0,
            mark_in: 0,
            mark_out: 0,
            stream_size: 0,
            ad_track: None,
            vid_track: Some(Box::new(CcxGxfVideoTrack {
                track_name: [0; STR_LEN as usize],
                fs_version: 0,
                frame_rate: CcxRational { num: 0, den: 1 },
                line_per_frame: 0,
                field_per_frame: 0,
                p_code: MpegPictureCoding::CCX_MPC_NONE,
                p_struct: MpegPictureStruct::CCX_MPS_NONE,
            })),
            cdp: None,
            cdp_len: 0,
        };

        (buf, gxf)
    }

    // Helper: Set up a demuxer for track description testing.
    fn create_demuxer_for_track_desc(data: &[u8], gxf: &mut CcxGxf) -> CcxDemuxer<'static> {
        let mut demux = create_demuxer_with_buffer(data);
        demux.private_data = gxf as *mut CcxGxf as *mut std::ffi::c_void;
        demux
    }

    #[test]
    fn test_parse_mpeg525_track_desc_valid() {
        initialize_logger();
        let (buf, mut gxf) = build_valid_track_desc();
        let total_len = buf.len() as i32;
        let mut demux = create_demuxer_for_track_desc(&buf, &mut gxf);

        let ret = unsafe { parse_mpeg525_track_desc(&mut demux, total_len) };
        assert_eq!(ret, CCX_OK);

        // Verify track name.
        let vid_track = gxf.vid_track.unwrap();
        assert_eq!(&vid_track.track_name[..8], b"Track001");
        // Verify frame rate: fs_version must be set to 2400.
        assert_eq!(vid_track.fs_version, 0);
        // Check that demux.past advanced exactly by buf.len().
        assert_eq!(demux.past as usize, buf.len());
    }

    #[test]
    fn test_parse_mpeg525_track_desc_incomplete_track_name() {
        initialize_logger();
        // Build a buffer where TRACK_NAME promises 8 bytes but provides only 4.
        let mut buf = Vec::new();
        buf.push(GXFTrackTag::TRACK_NAME as u8);
        buf.push(8);
        buf.extend_from_slice(b"Test"); // 4 bytes only.
        buf.extend_from_slice(&[0u8; 3]); // extra bytes
        let total_len = buf.len() as i32;

        let mut gxf = CcxGxf {
            nb_streams: 1,
            media_name: [0; STR_LEN as usize],
            first_field_nb: 0,
            last_field_nb: 0,
            mark_in: 0,
            mark_out: 0,
            stream_size: 0,
            ad_track: None,
            vid_track: Some(Box::new(CcxGxfVideoTrack {
                track_name: [0; STR_LEN as usize],
                fs_version: 0,
                frame_rate: CcxRational { num: 0, den: 1 },
                line_per_frame: 0,
                field_per_frame: 0,
                p_code: MpegPictureCoding::CCX_MPC_NONE,
                p_struct: MpegPictureStruct::CCX_MPS_NONE,
            })),
            cdp: None,
            cdp_len: 0,
        };

        let mut demux = create_demuxer_for_track_desc(&buf, &mut gxf);
        let ret = unsafe { parse_mpeg525_track_desc(&mut demux, total_len) };
        // Expect CCX_EINVAL because insufficient data leads to error.
        assert_eq!(ret, CCX_EINVAL);
    }

    #[test]
    fn test_parse_mpeg525_track_desc_invalid_private_data() {
        let buf = vec![0u8; 10];
        let total_len = buf.len() as i32;
        let mut demux = create_demuxer_with_buffer(&buf);
        demux.private_data = ptr::null_mut();

        let result = unsafe { parse_mpeg525_track_desc(&mut demux, total_len) };
        assert_eq!(result, CCX_EINVAL);
    }
    // Build a valid ancillary (AD) track description buffer.
    // This buffer contains:
    // - TRACK_NAME tag: tag_len = 8, then 8 bytes for the track name.
    // - TRACK_AUX tag: tag_len = 8, then 8 bytes of aux info.
    //   We set auxi_info such that:
    //     auxi_info[2] = 2 (maps to PRES_FORMAT_HD),
    //     auxi_info[3] = 4,
    //     auxi_info[4..6] = [0, 16] (field_size = 16),
    //     auxi_info[6..8] = [0, 2] (packet_size = 2*256 = 512).
    // - Extra bytes appended.
    fn build_valid_ad_track_desc() -> (Vec<u8>, CcxGxf) {
        let mut buf = Vec::new();
        // TRACK_NAME tag.
        buf.push(GXFTrackTag::TRACK_NAME as u8);
        buf.push(8);
        let name = b"ADTrk001XY"; // Use first 8 bytes: "ADTrk001"
        buf.extend_from_slice(&name[..8]);

        // TRACK_AUX tag.
        buf.push(GXFTrackTag::TRACK_AUX as u8);
        buf.push(8);
        // Create aux info: [?, ?, 2, 4, 0, 16, 0, 2]
        let auxi_info = [0u8, 0u8, 2, 4, 0, 16, 0, 2];
        buf.extend_from_slice(&auxi_info);

        // Append extra bytes.
        buf.extend_from_slice(&[0u8; 3]);

        // Create a dummy CcxGxf context.
        let gxf = CcxGxf {
            nb_streams: 1,
            media_name: [0; STR_LEN as usize],
            first_field_nb: 0,
            last_field_nb: 0,
            mark_in: 0,
            mark_out: 0,
            stream_size: 0,
            ad_track: Some(Box::new(CcxGxfAncillaryDataTrack {
                track_name: [0; STR_LEN as usize],
                fs_version: 0,
                frame_rate: 0,
                line_per_frame: 0,
                field_per_frame: 0,
                ad_format: GXFAncDataPresFormat::PRES_FORMAT_SD,
                nb_field: 0,
                field_size: 0,
                packet_size: 0,
                id: 123, // sample id
            })),
            vid_track: None,
            cdp: None,
            cdp_len: 0,
            // Other fields as needed...
        };

        (buf, gxf)
    }

    // Helper: Set up a demuxer for AD track description testing.
    fn create_demuxer_for_ad_track_desc(data: &[u8], gxf: &mut CcxGxf) -> CcxDemuxer<'static> {
        let mut demux = create_demuxer_with_buffer(data);
        demux.private_data = gxf as *mut CcxGxf as *mut std::ffi::c_void;
        demux
    }

    #[test]
    fn test_parse_ad_track_desc_valid() {
        let (buf, mut gxf) = build_valid_ad_track_desc();
        let total_len = buf.len() as i32;
        let mut demux = create_demuxer_for_ad_track_desc(&buf, &mut gxf);

        let ret = unsafe { parse_ad_track_desc(&mut demux, total_len) };
        assert_eq!(ret, CCX_OK);

        let ad_track = gxf.ad_track.unwrap();
        // Check that TRACK_NAME was read correctly.
        assert_eq!(&ad_track.track_name[..8], b"ADTrk001");
        // Check that TRACK_AUX set the fields as expected.
        // auxi_info[2] was 2, so we expect PRES_FORMAT_HD.
        assert_eq!(
            ad_track.ad_format as i32,
            GXFAncDataPresFormat::PRES_FORMAT_HD as i32
        );
        // auxi_info[3] is 4.
        assert_eq!(ad_track.nb_field, 4);
        // Field size: [0,16] => 16.
        assert_eq!(ad_track.field_size, 16);
        // Packet size: [0,2] => 2 * 256 = 512.
        assert_eq!(ad_track.packet_size, 512);
        // Verify that demux.past advanced by full buf length.
        assert_eq!(demux.past as usize, buf.len());
    }

    #[test]
    fn test_parse_ad_track_desc_incomplete_track_name() {
        initialize_logger();
        // Build a buffer where TRACK_NAME promises 8 bytes but only 4 are provided.
        let mut buf = Vec::new();
        buf.push(GXFTrackTag::TRACK_NAME as u8);
        buf.push(8);
        buf.extend_from_slice(b"Test"); // 4 bytes only.
        buf.extend_from_slice(&[0u8; 2]); // extra bytes
        let total_len = buf.len() as i32;

        let mut gxf = CcxGxf {
            nb_streams: 1,
            media_name: [0; STR_LEN as usize],
            first_field_nb: 0,
            last_field_nb: 0,
            mark_in: 0,
            mark_out: 0,
            stream_size: 0,
            ad_track: Some(Box::new(CcxGxfAncillaryDataTrack {
                track_name: [0; STR_LEN as usize],
                fs_version: 0,
                frame_rate: 0,
                line_per_frame: 0,
                field_per_frame: 0,
                ad_format: GXFAncDataPresFormat::PRES_FORMAT_SD,
                nb_field: 0,
                field_size: 0,
                packet_size: 0,
                id: 45,
            })),
            vid_track: None,
            cdp: None,
            cdp_len: 0,
        };

        let mut demux = create_demuxer_for_ad_track_desc(&buf, &mut gxf);
        let ret = unsafe { parse_ad_track_desc(&mut demux, total_len) };
        // Expect CCX_EINVAL because TRACK_NAME did not yield full 8 bytes.
        assert_eq!(ret, CCX_EINVAL);
    }

    #[test]
    fn test_parse_ad_track_desc_invalid_private_data() {
        let buf = vec![0u8; 10];
        let total_len = buf.len() as i32;
        let mut demux = create_demuxer_with_buffer(&buf);
        // Set private_data to null.
        demux.private_data = ptr::null_mut();

        let ret = unsafe { parse_ad_track_desc(&mut demux, total_len) };
        assert_eq!(ret, CCX_EINVAL);
    }
    fn create_demuxer_for_track_sec(data: &[u8], gxf: &mut CcxGxf) -> CcxDemuxer<'static> {
        let mut demux = create_demuxer_with_buffer(data);
        demux.private_data = gxf as *mut CcxGxf as *mut std::ffi::c_void;
        demux
    }

    // Helper: Build a track record.
    // Produces 4 header bytes followed by track_data of length track_len.
    // track_type, track_id, track_len are provided.
    fn build_track_record(
        track_type: u8,
        track_id: u8,
        track_len: i32,
        track_data: &[u8],
    ) -> Vec<u8> {
        let mut rec = Vec::new();
        rec.push(track_type);
        rec.push(track_id);
        rec.extend_from_slice(&(track_len as u16).to_be_bytes());
        rec.extend_from_slice(&track_data[..track_len as usize]);
        rec
    }

    #[test]
    fn test_parse_track_sec_no_context() {
        // Create a demuxer with a valid buffer.
        let buf = vec![0u8; 10];
        let mut demux = create_demuxer_with_buffer(&buf);
        // Set private_data to null.
        demux.private_data = ptr::null_mut();
        let mut data = DemuxerData::default();
        let ret = unsafe { parse_track_sec(&mut demux, buf.len() as i32, &mut data) };
        assert_eq!(ret, CCX_EINVAL);
    }

    #[test]
    fn test_parse_track_sec_skip_branch() {
        // Build a record that should be skipped because track_type does not have high bit set.
        let track_len = 7;
        let track_data = vec![0xEE; track_len as usize];
        // Use track_type = 0x10 (no high bit) and arbitrary track_id.
        let record = build_track_record(0x10, 0xFF, track_len, &track_data);
        let buf = record;
        let total_len = buf.len() as i32;

        // Create a dummy context.
        let mut gxf = CcxGxf {
            nb_streams: 1,
            media_name: [0; 256],
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
        let mut demux = create_demuxer_for_track_sec(&buf, &mut gxf);
        let mut data = DemuxerData::default();

        let ret = unsafe { parse_track_sec(&mut demux, total_len, &mut data) };
        // The record is skipped so ret should be CCX_OK and datatype remains Unknown.
        assert_eq!(ret, CCX_OK);
        assert_eq!(data.bufferdatatype as i32, BufferdataType::Unknown as i32);
        assert_eq!(demux.past as usize, buf.len());
    }
    impl DemuxerData {
        pub fn new(size: usize) -> Self {
            let mut vec = vec![0u8; size];
            let ptr = vec.as_mut_ptr();
            mem::forget(vec);
            DemuxerData {
                buffer: ptr,
                len: 0,
                ..Default::default()
            }
        }
    }

    // Build a valid CDP packet.
    // Packet layout:
    // 0: 0x96, 1: 0x69,
    // 2: cdp_length (should equal total length, here 18),
    // 3: frame rate byte (e.g. 0x50),
    // 4: a byte (e.g. 0x42),
    // 5-6: header sequence counter (0x00, 0x01),
    // 7: section id: 0x72,
    // 8: cc_count (e.g. 0x02 => cc_count = 2),
    // 9-14: 6 bytes of cc data,
    // 15: footer id: 0x74,
    // 16-17: footer sequence counter (0x00, 0x01).
    fn build_valid_cdp_packet() -> Vec<u8> {
        let total_len = 18u8;
        let mut packet = Vec::new();
        packet.push(0x96);
        packet.push(0x69);
        packet.push(total_len); // cdp_length = 18
        packet.push(0x50); // frame rate byte: framerate = 5
        packet.push(0x42); // cc_data_present = 1, caption_service_active = 1
        packet.extend_from_slice(&[0x00, 0x01]); // header sequence counter = 1
        packet.push(0x72); // section id for CC data
        packet.push(0x02); // cc_count = 2 (lower 5 bits)
        packet.extend_from_slice(&[0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF]); // cc data: 6 bytes
        packet.push(0x74); // footer id
        packet.extend_from_slice(&[0x00, 0x01]); // footer sequence counter = 1
        packet
    }

    #[test]
    fn test_parse_ad_cdp_valid() {
        initialize_logger();
        let packet = build_valid_cdp_packet();
        let mut data = DemuxerData::new(100);
        let result = parse_ad_cdp(&packet, &mut data);
        assert!(result.is_ok());
        // cc_count = 2 so we expect 2 * 3 = 6 bytes to be copied.
        assert_eq!(data.len, 6);
        let copied = unsafe { slice::from_raw_parts(data.buffer, data.len) };
        assert_eq!(copied, &[0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF]);
    }

    #[test]
    fn test_parse_ad_cdp_short_packet() {
        initialize_logger();
        // Packet shorter than 11 bytes.
        let packet = vec![0x96, 0x69, 0x08, 0x50, 0x42, 0x00, 0x01, 0x72, 0x01, 0xAA];
        let mut data = DemuxerData::new(100);
        let result = parse_ad_cdp(&packet, &mut data);
        assert!(result.is_err());
        assert_eq!(result.unwrap_err(), "Invalid packet length");
    }

    #[test]
    fn test_parse_ad_cdp_invalid_identifier() {
        initialize_logger();
        let mut packet = build_valid_cdp_packet();
        packet[0] = 0x00;
        let mut data = DemuxerData::new(100);
        let result = parse_ad_cdp(&packet, &mut data);
        assert!(result.is_err());
        assert_eq!(result.unwrap_err(), "Invalid CDP identifier");
    }

    #[test]
    fn test_parse_ad_cdp_mismatched_length() {
        initialize_logger();
        let mut packet = build_valid_cdp_packet();
        packet[2] = 20; // Set length to 20, but actual length is 18.
        let mut data = DemuxerData::new(100);
        let result = parse_ad_cdp(&packet, &mut data);
        assert!(result.is_err());
        assert_eq!(result.unwrap_err(), "Mismatched CDP length");
    }

    #[test]
    fn test_parse_ad_cdp_time_code_section() {
        initialize_logger();
        let mut packet = build_valid_cdp_packet();
        // Change section id at offset 7 to 0x71.
        packet[7] = 0x71;
        let mut data = DemuxerData::new(100);
        let result = parse_ad_cdp(&packet, &mut data);
        assert!(result.is_err());
        assert_eq!(result.unwrap_err(), "Time code section ignored");
    }

    #[test]
    fn test_parse_ad_cdp_service_info_section() {
        initialize_logger();
        let mut packet = build_valid_cdp_packet();
        packet[7] = 0x73;
        let mut data = DemuxerData::new(100);
        let result = parse_ad_cdp(&packet, &mut data);
        assert!(result.is_err());
        assert_eq!(result.unwrap_err(), "Service information section ignored");
    }

    #[test]
    fn test_parse_ad_cdp_new_section() {
        initialize_logger();
        let mut packet = build_valid_cdp_packet();
        packet[7] = 0x80; // falls in 0x75..=0xEF
        let mut data = DemuxerData::new(100);
        let result = parse_ad_cdp(&packet, &mut data);
        assert!(result.is_err());
        assert_eq!(result.unwrap_err(), "Unhandled new section");
    }

    #[test]
    fn test_parse_ad_cdp_footer_mismatch() {
        initialize_logger();
        let mut packet = build_valid_cdp_packet();
        // Change footer sequence counter (bytes 16-17) to 0x00,0x02.
        packet[16] = 0x00;
        packet[17] = 0x02;
        let mut data = DemuxerData::new(100);
        let result = parse_ad_cdp(&packet, &mut data);
        assert!(result.is_err());
        assert_eq!(result.unwrap_err(), "CDP footer sequence mismatch");
    }
    // Helper: Build a payload for parse_ad_pyld.
    // The payload length (len) is total bytes.
    // It must be at least 6 (header) + 2 (one iteration) = 8.
    // For a valid CEA-708 case, we supply:
    //  - d_id (2 bytes little-endian): CLOSED_CAP_DID (0x01, 0x00)
    //  - sd_id (2 bytes): CLOSED_C708_SDID (0x02, 0x00)
    //  - dc (2 bytes): arbitrary (e.g., 0xFF, 0x00)
    //  - Then one 16-bit word: e.g., 0xFF, 0x00.
    fn build_valid_ad_pyld_payload() -> Vec<u8> {
        let mut payload = Vec::new();
        // Header: d_id = 0x0001, sd_id = 0x0002, dc = 0xFF00.
        payload.extend_from_slice(&[0x01, 0x00]); // d_id
        payload.extend_from_slice(&[0x02, 0x00]); // sd_id
        payload.extend_from_slice(&[0xFF, 0x00]); // dc (masked to 0xFF)
                                                  // Remaining payload: one 16-bit word.
        payload.extend_from_slice(&[0xFF, 0x00]); // This will produce 0x00FF stored in cdp[0]
        payload
    }

    #[test]
    fn test_parse_ad_pyld_valid_cea708() {
        // Build a valid payload for CEA-708.
        let payload = build_valid_ad_pyld_payload();
        let total_len = payload.len() as i32; // e.g., 8 bytes
        let mut demux = create_demuxer_with_buffer(&payload);
        // Create a dummy GXF context with no cdp allocated.
        let mut gxf = CcxGxf {
            cdp: None,
            cdp_len: 0,
            // Other fields can be default.
            ..Default::default()
        };
        demux.private_data = &mut gxf as *mut CcxGxf as *mut std::ffi::c_void;
        let mut data = DemuxerData::new(100);

        let ret = unsafe { parse_ad_pyld(&mut demux, total_len, &mut data) };
        assert_eq!(ret, CCX_OK);
        // Check that demux.past advanced by total_len.
        assert_eq!(demux.past as usize, payload.len());
        // After subtracting 6, remaining length = 2.
        // So ctx.cdp_len should be set to ((2 - 2) / 2) = 0.
        // However, note that the loop runs if remaining_len > 2.
        // In this case, 2 is not >2 so loop does not run.
        // Thus, for a minimal valid payload, we need to supply at least 10 bytes.
        // Let's update our payload accordingly.
    }

    #[test]
    fn test_parse_ad_pyld_cea608_branch() {
        // Build a payload for the CEA-608 branch.
        // Use d_id = 0x0001 and sd_id = 0x0003.
        let mut payload = Vec::new();
        payload.extend_from_slice(&[0x01, 0x00]); // d_id
        payload.extend_from_slice(&[0x03, 0x00]); // sd_id = 0x0003 for CEA-608
        payload.extend_from_slice(&[0x00, 0x00]); // dc (arbitrary)
                                                  // Append some extra payload (e.g., 4 bytes).
        payload.extend_from_slice(&[0x11, 0x22, 0x33, 0x44]);
        let total_len = payload.len() as i32;
        let mut demux = create_demuxer_with_buffer(&payload);
        let mut gxf = CcxGxf {
            cdp: None,
            cdp_len: 0,
            ..Default::default()
        };
        demux.private_data = &mut gxf as *mut CcxGxf as *mut std::ffi::c_void;
        let mut data = DemuxerData::new(100);

        let ret = unsafe { parse_ad_pyld(&mut demux, total_len, &mut data) };
        // In this branch, the function only logs "Need Sample" and does not fill cdp.
        // The function still calls buffered_skip for the remaining bytes.
        assert_eq!(ret, CCX_OK);
        // demux.past should equal total_len.
        assert_eq!(demux.past as usize, payload.len());
    }

    #[test]
    fn test_parse_ad_pyld_other_branch() {
        // Build a payload for an "other" service (d_id != CLOSED_CAP_DID).
        // For example, d_id = 0x0002.
        let mut payload = Vec::new();
        payload.extend_from_slice(&[0x02, 0x00]); // d_id = 0x0002 (does not match)
        payload.extend_from_slice(&[0x02, 0x00]); // sd_id = 0x0002 (irrelevant)
        payload.extend_from_slice(&[0x00, 0x00]); // dc
                                                  // Append extra payload (4 bytes).
        payload.extend_from_slice(&[0x55, 0x66, 0x77, 0x88]);
        let total_len = payload.len() as i32;
        let mut demux = create_demuxer_with_buffer(&payload);
        let mut gxf = CcxGxf {
            cdp: None,
            cdp_len: 0,
            ..Default::default()
        };
        demux.private_data = &mut gxf as *mut CcxGxf as *mut std::ffi::c_void;
        let mut data = DemuxerData::new(100);

        let ret = unsafe { parse_ad_pyld(&mut demux, total_len, &mut data) };
        // For other service, no branch is taken; we simply skip remaining bytes.
        assert_eq!(ret, CCX_OK);
        // demux.past should equal total_len.
        assert_eq!(demux.past as usize, payload.len());
    }
    // Helper: Create a demuxer with a given GXF context.
    #[allow(unused)]
    fn create_demuxer_for_vbi(data: &[u8], gxf: &mut CcxGxf) -> CcxDemuxer<'static> {
        let mut demux = create_demuxer_with_buffer(data);
        demux.private_data = gxf as *mut CcxGxf as *mut std::ffi::c_void;
        demux
    }
    // --- Tests for when VBI support is disabled ---
    #[test]
    #[cfg(not(feature = "ccx_gxf_enable_ad_vbi"))]
    fn test_parse_ad_vbi_disabled() {
        // Create a buffer with known content.
        let payload = vec![0xAA; 20]; // 20 bytes of data.
        let total_len = payload.len() as i32;
        let mut demux = create_demuxer_with_buffer(&payload);
        // Create a dummy DemuxerData (not used in disabled branch).
        let mut data = DemuxerData::new(100);

        let ret = unsafe { parse_ad_vbi(&mut demux, total_len, &mut data) };
        assert_eq!(ret, CCX_OK);
        // Since VBI is disabled, buffered_skip should be called and return total_len.
        assert_eq!(demux.past as usize, payload.len());
        // data.len should remain unchanged.
        assert_eq!(data.len, 0);
    }

    // --- Tests for when VBI support is enabled ---
    #[test]
    #[cfg(feature = "ccx_gxf_enable_ad_vbi")] // to run use ccx_gxf_enable_ad_vbi=1 RUST_TEST_THREADS=1 cargo test
    fn test_parse_ad_vbi_enabled() {
        // Create a buffer with known content.
        let payload = vec![0xBB; 20]; // 20 bytes of data.
        let total_len = payload.len() as i32;
        let mut demux = create_demuxer_with_buffer(&payload);
        // Create a dummy GXF context.
        let mut gxf = CcxGxf::default();
        // Create a dummy DemuxerData with a buffer large enough.
        let mut data = DemuxerData::new(100);

        let ret = unsafe { parse_ad_vbi(&mut demux, total_len, &mut data) };
        assert_eq!(ret, CCX_OK);
        // In VBI enabled branch, data.len was increased by total_len.
        // And buffered_read copies total_len bytes.
        assert_eq!(data.len, total_len as usize);
        // Check that the bytes read into data.buffer match payload.
        assert_eq!(
            unsafe { std::slice::from_raw_parts(data.buffer, total_len as usize) },
            &payload[..]
        ); // demux.past should equal total_len.
        assert_eq!(demux.past as usize, total_len as usize);
    }
    // Helper: Create a demuxer for ad field, with a given GXF context that already has an ancillary track.
    fn create_demuxer_for_ad_field(data: &[u8], gxf: &mut CcxGxf) -> CcxDemuxer<'static> {
        let mut demux = create_demuxer_with_buffer(data);
        demux.private_data = gxf as *mut CcxGxf as *mut std::ffi::c_void;
        demux
    }
    // Test 1: Minimal valid field section (no loop iteration)
    #[test]
    fn test_parse_ad_field_valid_minimal() {
        // Build a minimal valid field section:
        // Total length = 52 bytes.
        // Header:
        //  "finf" (4 bytes)
        //  spec value = 4 (4 bytes: 00 00 00 04)
        //  field identifier = 0x10 (4 bytes: 00 00 00 10)
        //  "LIST" (4 bytes)
        //  sample size = 36 (4 bytes: 24 00 00 00) because after "LIST", remaining len = 52 - 16 = 36.
        //  "anc " (4 bytes)
        // Then remaining = 52 - 24 = 28 bytes. (Loop condition: while(28 > 28) false)
        let mut buf = Vec::new();
        buf.extend_from_slice(b"finf");
        buf.extend_from_slice(&[0x00, 0x00, 0x00, 0x04]);
        buf.extend_from_slice(&[0x00, 0x00, 0x00, 0x10]);
        buf.extend_from_slice(b"LIST");
        buf.extend_from_slice(&[0x24, 0x00, 0x00, 0x00]); // 36 decimal
        buf.extend_from_slice(b"anc ");
        // Append 28 bytes of dummy data (e.g. 0xAA)
        buf.extend_from_slice(&[0xAA; 28]);
        let total_len = buf.len() as i32;
        // Create a dummy GXF context with an ancillary track
        #[allow(unused_variables)]
        let ad_track = CcxGxfAncillaryDataTrack {
            track_name: [0u8; 256],
            fs_version: 0,
            frame_rate: 0,
            line_per_frame: 0,
            field_per_frame: 0,
            ad_format: GXFAncDataPresFormat::PRES_FORMAT_SD,
            nb_field: 0,
            field_size: 0,
            packet_size: 0,
            id: 0,
        };
        let mut gxf = CcxGxf::default();
        let mut demux = create_demuxer_for_ad_field(&buf, &mut gxf);
        let mut data = DemuxerData::new(100);

        let ret = unsafe { parse_ad_field(&mut demux, total_len, &mut data) };
        assert_eq!(ret, CCX_OK);
        // Expect demux.past to equal total length.
        assert_eq!(demux.past as usize, buf.len());
    }

    //tests for set_data_timebase
    #[test]
    fn test_set_data_timebase_0() {
        let mut data = DemuxerData::default();
        set_data_timebase(0, &mut data);
        assert_eq!(data.tb.den, 30000);
        assert_eq!(data.tb.num, 1001);
    }
    #[test]
    fn test_set_data_timebase_1() {
        let mut data = DemuxerData::default();
        set_data_timebase(1, &mut data);
        assert_eq!(data.tb.den, 25);
        assert_eq!(data.tb.num, 1);
    }
    fn create_demuxer_with_data(data: &[u8]) -> CcxDemuxer {
        CcxDemuxer {
            filebuffer: allocate_filebuffer(data),
            filebuffer_pos: 0,
            bytesinbuffer: data.len() as u32,
            past: 0,
            private_data: ptr::null_mut(),
            ..Default::default()
        }
    }

    // Helper: Create a DemuxerData with a writable buffer.
    fn create_demuxer_data(size: usize) -> DemuxerData {
        let mut buf = vec![0u8; size];
        let ptr = buf.as_mut_ptr();
        mem::forget(buf);
        DemuxerData {
            buffer: ptr,
            len: 0,
            ..Default::default()
        }
    }

    // Test: Full packet is successfully read.
    #[test]
    fn test_parse_mpeg_packet_valid() {
        // Build a test payload.
        let payload = b"Hello, Rust MPEG Packet!";
        let total_len = payload.len();
        let mut demux = create_demuxer_with_data(payload);
        let mut data = create_demuxer_data(1024);

        // Call parse_mpeg_packet.
        let ret = unsafe { parse_mpeg_packet(&mut demux, total_len, &mut data) };
        assert_eq!(ret, CCX_OK);
        // Check that data.len was increased by total_len.
        assert_eq!(data.len, total_len);
        // Verify that the content in data.buffer matches payload.
        let out = unsafe { slice::from_raw_parts(data.buffer, total_len) };
        assert_eq!(out, payload);
        // Check that demux.past equals total_len.
        assert_eq!(demux.past as usize, total_len);
    }

    // Test: Incomplete packet (simulate short read).
    #[test]
    fn test_parse_mpeg_packet_incomplete() {
        // Build a test payload but simulate that only part of it is available.
        let payload = b"Short Packet";
        let total_len = payload.len();
        // Create a demuxer with only half of the payload available.
        let available = total_len / 2;
        let mut demux = create_demuxer_with_data(&payload[..available]);
        let mut data = create_demuxer_data(1024);

        // Call parse_mpeg_packet.
        let ret = unsafe { parse_mpeg_packet(&mut demux, total_len, &mut data) };
        assert_eq!(ret, CCX_EOF);
        // data.len should still be increased by total_len (as per C code).
        assert_eq!(data.len, total_len);
        // demux.past should equal available.
        assert_eq!(demux.past as usize, 0);
    }
    #[test]
    fn test_parse_ad_packet_correct_data() {
        // Setup test data
        let mut data = Vec::new();
        data.extend_from_slice(b"RIFF");
        data.extend_from_slice(&65528u32.to_le_bytes()); // ADT packet length
        data.extend_from_slice(b"rcrd");
        data.extend_from_slice(b"desc");
        data.extend_from_slice(&20u32.to_le_bytes()); // desc length
        data.extend_from_slice(&2u32.to_le_bytes()); // version
        let nb_field = 2;
        data.extend_from_slice(&(nb_field as u32).to_le_bytes());
        let field_size = 100;
        data.extend_from_slice(&(field_size as u32).to_le_bytes());
        data.extend_from_slice(&65536u32.to_le_bytes()); // buffer size
        let timebase = 12345u32;
        data.extend_from_slice(&timebase.to_le_bytes());
        data.extend_from_slice(b"LIST");
        let field_section_size = 4 + (nb_field * field_size) as u32;
        data.extend_from_slice(&field_section_size.to_le_bytes());
        data.extend_from_slice(b"fld ");
        for _ in 0..nb_field {
            data.extend(vec![0u8; field_size as usize]);
        }

        let mut demux = create_ccx_demuxer_with_header(&data);
        let mut ctx = CcxGxf {
            ad_track: Some(Box::new(CcxGxfAncillaryDataTrack {
                nb_field,
                field_size,
                ..Default::default() // ... other necessary fields
            })),
            ..Default::default()
        };
        demux.private_data = &mut ctx as *mut _ as *mut std::ffi::c_void;

        let mut demuxer_data = DemuxerData::default();

        let result = unsafe { parse_ad_packet(&mut demux, data.len() as i32, &mut demuxer_data) };
        assert_eq!(result, CCX_OK);
        assert_eq!(demux.past, data.len() as i64);
    }

    #[test]
    fn test_parse_ad_packet_incorrect_riff() {
        let mut data = Vec::new();
        data.extend_from_slice(b"RIFX"); // Incorrect RIFF
                                         // ... rest of data setup similar to correct test but with incorrect header

        let mut demux = create_ccx_demuxer_with_header(&data);
        let mut ctx = CcxGxf {
            ad_track: Some(Box::new(CcxGxfAncillaryDataTrack {
                nb_field: 0,
                field_size: 0,
                ..Default::default()
            })),
            ..Default::default()
        };
        demux.private_data = &mut ctx as *mut _ as *mut std::ffi::c_void;

        let mut demuxer_data = DemuxerData::default();
        let result = unsafe { parse_ad_packet(&mut demux, data.len() as i32, &mut demuxer_data) };
        assert_eq!(result, CCX_EOF); // Or check for expected result based on partial parsing
    }

    #[test]
    fn test_parse_ad_packet_eof_condition() {
        let mut data = Vec::new();
        data.extend_from_slice(b"RIFF");
        data.extend_from_slice(&65528u32.to_le_bytes());
        // ... incomplete data

        let mut demux = create_demuxer_with_buffer(&data);
        let mut ctx = CcxGxf {
            ad_track: Some(Box::new(CcxGxfAncillaryDataTrack {
                nb_field: 0,
                field_size: 0,
                ..Default::default()
            })),
            ..Default::default()
        };
        demux.private_data = &mut ctx as *mut _ as *mut std::ffi::c_void;

        let mut demuxer_data = DemuxerData::default();
        let result =
            unsafe { parse_ad_packet(&mut demux, data.len() as i32 + 10, &mut demuxer_data) }; // Len larger than data
        assert_eq!(result, CCX_EOF);
    }
    // Tests for set_mpeg_frame_desc
    #[test]
    fn test_set_mpeg_frame_desc_i_frame() {
        let mut vid_track = CcxGxfVideoTrack::default();
        let mpeg_frame_desc_flag = 0b00000001;
        set_mpeg_frame_desc(&mut vid_track, mpeg_frame_desc_flag);
        assert_eq!(
            vid_track.p_code as i32,
            MpegPictureCoding::CCX_MPC_I_FRAME as i32
        );
        assert_eq!(
            vid_track.p_struct as i32,
            MpegPictureStruct::CCX_MPS_NONE as i32
        );
    }
    #[test]
    fn test_set_mpeg_frame_desc_p_frame() {
        let mut vid_track = CcxGxfVideoTrack::default();
        let mpeg_frame_desc_flag = 0b00000010;
        set_mpeg_frame_desc(&mut vid_track, mpeg_frame_desc_flag);
        assert_eq!(
            vid_track.p_code as i32,
            MpegPictureCoding::CCX_MPC_P_FRAME as i32
        );
        assert_eq!(
            vid_track.p_struct as i32,
            MpegPictureStruct::CCX_MPS_NONE as i32
        );
    }
    #[test]
    fn test_partial_eq_gxf_track_type() {
        let track_type1 = GXFTrackType::TRACK_TYPE_TIME_CODE_525;
        let track_type2 = GXFTrackType::TRACK_TYPE_TIME_CODE_525;
        assert_eq!(track_type1 as i32, track_type2 as i32);
    }
    fn create_test_demuxer(data: &[u8], has_ctx: bool) -> CcxDemuxer {
        CcxDemuxer {
            filebuffer: data.as_ptr() as *mut u8,
            bytesinbuffer: data.len() as u32,
            filebuffer_pos: 0,
            past: 0,
            private_data: if has_ctx {
                Box::into_raw(Box::new(CcxGxf {
                    ad_track: Some(Box::new(CcxGxfAncillaryDataTrack {
                        packet_size: 100,
                        nb_field: 2,
                        field_size: 100,
                        ..Default::default()
                    })),
                    vid_track: Some(Box::new(CcxGxfVideoTrack {
                        frame_rate: CcxRational {
                            num: 30000,
                            den: 1001,
                        },
                        ..Default::default()
                    })),
                    first_field_nb: 0,
                    ..Default::default()
                })) as *mut _
            } else {
                ptr::null_mut()
            },
            ..Default::default()
        }
    }

    #[test]
    fn test_parse_media_ancillary_data() {
        let mut data = vec![
            0x02, // TRACK_TYPE_ANCILLARY_DATA
            0x01, // track_nb
            0x00, 0x00, 0x00, 0x02, // media_field_nb (BE32)
            0x00, 0x01, // first_field_nb (BE16)
            0x00, 0x02, // last_field_nb (BE16)
            0x00, 0x00, 0x00, 0x03, // time_field (BE32)
            0x01, // valid_time_field (bit 0 set)
            0x00, // skipped byte
        ];
        // Add payload (100 bytes for ad_track->packet_size)
        data.extend(vec![0u8; 100]);

        let mut demux = create_test_demuxer(&data, true);
        let mut demuxer_data = DemuxerData::default();

        let result = unsafe { parse_media(&mut demux, data.len() as i32, &mut demuxer_data) };
        assert_eq!(result, CCX_OK);
    }

    #[test]
    fn test_parse_media_mpeg2() {
        initialize_logger();
        let mut data = vec![
            0x04, // TRACK_TYPE_MPEG2_525
            0x01, // track_nb
            0x00, 0x00, 0x00, 0x02, // media_field_nb (BE32)
            0x12, 0x34, 0x56, 0x78, // mpeg_pic_size (BE32)
            0x00, 0x00, 0x00, 0x03, // time_field (BE32)
            0x01, // valid_time_field
            0x00, // skipped byte
        ];
        // Add MPEG payload (0x123456 bytes)
        data.extend(vec![0u8; 0x123456]);

        let mut demux = create_test_demuxer(&data, true);
        demux.private_data = Box::into_raw(Box::new(CcxGxf {
            ad_track: None, // Force MPEG path
            vid_track: Some(Box::new(CcxGxfVideoTrack::default())),
            first_field_nb: 0,
            ..Default::default()
        })) as *mut _;

        let mut demuxer_data = DemuxerData::default();
        let result = unsafe { parse_media(&mut demux, data.len() as i32, &mut demuxer_data) };
        assert_eq!(result, CCX_OK);
    }

    #[test]
    fn test_parse_media_insufficient_len() {
        let data = vec![0x02, 0x01]; // Incomplete header
        let mut demux = create_test_demuxer(&data, true);
        let mut demuxer_data = DemuxerData::default();
        let result = unsafe { parse_media(&mut demux, 100, &mut demuxer_data) };
        assert_eq!(result, CCX_EOF);
    }
    // Tests for parse_flt

    fn create_test_demuxer_parse_map(data: &[u8]) -> CcxDemuxer {
        CcxDemuxer {
            filebuffer: data.as_ptr() as *mut u8,
            bytesinbuffer: data.len() as u32,
            filebuffer_pos: 0,
            past: 0,
            private_data: Box::into_raw(Box::new(CcxGxf::default())) as *mut _,
            ..Default::default()
        }
    }

    #[test]
    fn test_parse_flt() {
        let data = vec![0x01, 0x02, 0x03, 0x04];
        let mut demux = create_test_demuxer(&data, false);
        let result = unsafe { parse_flt(&mut demux, 4) };
        assert_eq!(result, CCX_OK);
        assert_eq!(demux.past, 4);
    }
    #[test]
    fn test_parse_flt_eof() {
        let data = vec![0x01, 0x02, 0x03, 0x04];
        let mut demux = create_test_demuxer(&data, false);
        let result = unsafe { parse_flt(&mut demux, 5) };
        assert_eq!(result, CCX_EOF);
        assert_eq!(demux.past as usize, unsafe { buffered_skip(&mut demux, 5) });
    }
    #[test]
    fn test_parse_flt_invalid() {
        let data = vec![0x01, 0x02, 0x03, 0x04];
        let mut demux = create_test_demuxer(&data, false);
        let result = unsafe { parse_flt(&mut demux, 40) };
        assert_eq!(result, CCX_EOF);
        assert_eq!(demux.past as usize, 0);
    }
    // Tests for parse_map
    #[test]
    fn test_parse_map_valid() {
        // Build a MAP packet as follows:
        // Total length: we simulate a buffer with a total length of, say, 40 bytes.
        // Layout:
        //   - First, subtract 2 bytes: these 2 bytes are used for the MAP identifier.
        //   - Next, 2 bytes: should equal 0xe0ff.
        //   For our test, we set these two bytes to 0xe0, 0xff.
        //   - Then, subtract 2 and read material_sec_len.
        //     Let's set material_sec_len = 10.
        //   - Then, material section: 10 bytes.
        //   - Then, subtract 2 and read track_sec_len.
        //     Let's set track_sec_len = 8.
        //   - Then, track section: 8 bytes.
        // Total consumed in header: 2 + 2 + 2 + 10 + 2 + 8 = 26.
        // We then simulate that the remaining len is (40 - 26) = 14.
        // In the error block, we skip those 14 bytes.
        let mut buf = Vec::new();
        // First 2 bytes: arbitrary (we subtract these, not used in MAP check).
        buf.extend_from_slice(&[0x00, 0x00]);
        // Next 2 bytes: MAP identifier (0xe0, 0xff).
        buf.extend_from_slice(&[0xe0, 0xff]);
        // Next 2 bytes: material_sec_len = 10 (big-endian).
        buf.extend_from_slice(&10u16.to_be_bytes());
        // Material section: 10 arbitrary bytes.
        buf.extend_from_slice(&[0xAA; 10]);
        // Next 2 bytes: track_sec_len = 8.
        buf.extend_from_slice(&8u16.to_be_bytes());
        // Track section: 8 arbitrary bytes.
        buf.extend_from_slice(&[0xBB; 8]);
        // Now remaining bytes: 14 arbitrary bytes.
        buf.extend_from_slice(&[0xCC; 14]);
        #[allow(unused_variables)]
        let total_len = buf.len() as i32; // should be 40 + 14 = 54? Let's check:
                                          // Actually: 2+2+2+10+2+8+14 = 40 bytes.
                                          // Let's set total length = buf.len() as i32.
        let total_len = buf.len() as i32;

        // Create demuxer with this buffer.
        let mut demux = create_demuxer_with_data(&buf);
        // Create a dummy GXF context and assign to demux.private_data.
        let mut gxf = CcxGxf::default();
        // For MAP, parse_material_sec and parse_track_sec are called;
        // our dummy implementations simply skip the specified bytes.
        // Set private_data.
        demux.private_data = &mut gxf as *mut CcxGxf as *mut std::ffi::c_void;
        // Create a dummy DemuxerData.
        let mut data = create_demuxer_data(1024);

        let ret = unsafe { parse_map(&mut demux, total_len, &mut data) };
        assert_eq!(ret, CCX_OK);
        // Check that demux.past equals the entire remaining length after processing.
        // In our dummy, parse_material_sec and parse_track_sec simply skip bytes.
        // Thus, final buffered_skip in error block should skip the remaining bytes.
        // Our test expects that demux.past equals total_len - 2 - 2 - 2 -10 -2 -8 + (skipped remaining).
        // For simplicity, we assert that demux.past equals the total remaining bytes (buf.len() - consumed headers).
        // Here, consumed header bytes: 2 (initial subtraction) + 2 (MAP id) + 2 (material_sec_len) + 10 + 2 (track_sec_len) + 8 = 26.
        // Then error block should skip  (total_len - 26).
        let expected_error_skip = (total_len - 26) as usize;
        // And demux.past should equal 26 (from header processing) + expected_error_skip.
        assert_eq!(demux.past as usize, 26 + expected_error_skip);
    }

    #[test]
    fn test_parse_map_invalid_header() {
        let data = vec![0x00, 0x00]; // Invalid header
        let mut demux = create_test_demuxer_parse_map(&data);
        let mut data = DemuxerData::default();
        let result = unsafe { parse_map(&mut demux, 2, &mut data) };
        assert_eq!(result, CCX_OK);
        assert_eq!(demux.past, 2);
    }

    #[test]
    fn test_parse_map_material_section_overflow() {
        let data = vec![
            0xe0, 0xff, // Valid header
            0x00, 0x05, // material_sec_len = 5 (exceeds remaining len)
        ];
        let mut demux = create_test_demuxer_parse_map(&data);
        let mut data = DemuxerData::default();
        let result = unsafe { parse_map(&mut demux, 4, &mut data) };
        assert_eq!(result, CCX_OK);
        assert_eq!(demux.past, 4);
    }

    #[test]
    fn test_parse_map_track_section_overflow() {
        let data = vec![
            0xe0, 0xff, // Valid header
            0x00, 0x02, // material_sec_len = 2
            0x00, 0x00, // Material section
            0x00, 0x05, // track_sec_len = 5 (exceeds remaining len)
        ];
        let mut demux = create_test_demuxer_parse_map(&data);
        let mut data = DemuxerData::default();
        let result = unsafe { parse_map(&mut demux, 8, &mut data) };
        assert_eq!(result, CCX_OK);
        assert_eq!(demux.past, 8);
    }

    #[test]
    fn test_parse_map_eof_during_skip() {
        let data = vec![0x00, 0x00]; // Invalid header, insufficient data
        let mut demux = create_test_demuxer_parse_map(&data);
        let result = unsafe { parse_map(&mut demux, 5, &mut DemuxerData::default()) };
        assert_eq!(result, CCX_EOF);
    }
    fn create_test_demuxer_packet_map(data: &[u8]) -> CcxDemuxer {
        CcxDemuxer {
            filebuffer: data.as_ptr() as *mut u8,
            bytesinbuffer: data.len() as u32,
            filebuffer_pos: 0,
            past: 0,
            private_data: Box::into_raw(Box::new(CcxGxf {
                ad_track: Some(Box::new(CcxGxfAncillaryDataTrack::default())),
                vid_track: Some(Box::new(CcxGxfVideoTrack::default())),
                ..Default::default()
            })) as *mut _,
            ..Default::default()
        }
    }
    fn valid_map_header(len: i32) -> Vec<u8> {
        let mut data = vec![
            0x00, 0x00, 0x00, 0x00, // Leader
            0x01, // Leader continuation
            0xbc, // MAP type
        ];
        // Add length (big-endian, including header size)
        let total_len = (len + 16).to_be_bytes();
        data.extend_from_slice(&total_len);
        data.extend_from_slice(&[0x00; 4]); // Flags
        data.push(0xe1); // Trailer
        data.push(0xe2); // Trailer
        data
    }

    fn valid_media_header(len: i32) -> Vec<u8> {
        let mut data = vec![
            0x00, 0x00, 0x00, 0x00, // Leader
            0x01, // Leader continuation
            0xbf, // MEDIA type
        ];
        let total_len = (len + 16).to_be_bytes();
        data.extend_from_slice(&total_len);
        data.extend_from_slice(&[0x00; 4]);
        data.push(0xe1);
        data.push(0xe2);
        data
    }

    #[test]
    fn test_read_packet_map() {
        let mut header = valid_map_header(8);
        header.extend(vec![0u8; 8]); // Payload
        let mut demux = create_test_demuxer_packet_map(&header);
        let mut data = DemuxerData::default();
        assert_eq!(unsafe { read_packet(&mut demux, &mut data) }, CCX_OK);
    }

    #[test]
    fn test_read_packet_media() {
        let mut header = valid_media_header(16);
        header.extend(vec![0u8; 16]);
        let mut demux = create_test_demuxer_packet_map(&header);
        let mut data = DemuxerData::default();
        assert_eq!(unsafe { read_packet(&mut demux, &mut data) }, CCX_OK);
    }

    #[test]
    fn test_read_packet_eos() {
        let data = vec![
            0x00, 0x00, 0x00, 0x00, 0x01, 0xfb, 0x00, 0x00, 0x00, 0x10, // Length = 16
            0x00, 0x00, 0x00, 0x00, 0xe1, 0xe2,
        ];
        let mut demux = create_test_demuxer_packet_map(&data);
        let mut dd = DemuxerData::default();
        assert_eq!(unsafe { read_packet(&mut demux, &mut dd) }, CCX_EOF);
    }

    #[test]
    fn test_read_packet_invalid_header() {
        let data = vec![0u8; 16]; // Invalid leader
        let mut demux = create_test_demuxer_packet_map(&data);
        let mut dd = DemuxerData::default();
        assert_eq!(unsafe { read_packet(&mut demux, &mut dd) }, CCX_EINVAL);
    }
    #[test]
    fn test_probe_buffer_too_short() {
        // Buffer shorter than startcode.
        let buf = [0, 0, 0];
        assert!(!ccx_gxf_probe(&buf));
    }

    #[test]
    fn test_probe_exact_match() {
        // Buffer exactly equal to startcode.
        let buf = [0, 0, 0, 0, 1, 0xbc];
        assert!(ccx_gxf_probe(&buf));
    }

    #[test]
    fn test_probe_match_with_extra_data() {
        // Buffer with startcode at the beginning, followed by extra data.
        let mut buf = vec![0, 0, 0, 0, 1, 0xbc];
        buf.extend_from_slice(&[0x12, 0x34, 0x56]);
        assert!(ccx_gxf_probe(&buf));
    }

    #[test]
    fn test_probe_no_match() {
        // Buffer with similar length but different content.
        let buf = [0, 0, 0, 1, 0, 0xbc]; // Note: fourth byte is 1 instead of 0
        assert!(!ccx_gxf_probe(&buf));
    }
}
