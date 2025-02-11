#![allow(non_camel_case_types)]
#![allow(unexpected_cfgs)]
#![allow(unused_doc_comments)]

use std::cmp::PartialEq;
use crate::decoder::common_structs::*;
use crate::demuxer::demuxer::{CcxDemuxer, DemuxerData};
use crate::file_functions::*;
use lib_ccxr::util::log::{debug, DebugMessageFlag};
use std::convert::TryFrom;
use std::ptr;
use std::slice;
use lib_ccxr::info;

const STR_LEN: u32 = 256;
const CLOSED_CAP_DID: u8 = 0x61;
const CLOSED_C708_SDID: u8 = 0x01;
const CLOSED_C608_SDID: u8 = 0x02;
pub const STARTBYTESLENGTH: usize = 1024 * 1024;
use lib_ccxr::common::{BufferdataType, Codec};
use libc::{ntohl, ntohs};

macro_rules! dbg {
    ($($args:expr),*) => {
        debug!(msg_type = DebugMessageFlag::PARSE; $($args),*)
    };
}

fn rl32(x: *const u8) -> u32 {
    unsafe { *(x as *const u32) }
}

fn rb32(x: &[u8; 16]) -> u32 {
    unsafe { ntohl(*(x.as_ptr() as *const u32)) }
}

fn rl16(x: *const u8) -> u16 {
    unsafe { *(x as *const u16) }
}

fn rb16(x: *const u8) -> u16 {
    unsafe { ntohs(*(x as *const u16)) }
}


// macro_rules! log {
//     ($fmt:expr, $($args:tt)*) => {
//         ccx_common_logging::log_ftn(
//             &format!("GXF:{}: {}", line!(), format!($fmt, $($args)*))
//         )
//     };
// }

// Equivalent enums
#[repr(u8)]
enum GXFPktType {
    PKT_MAP = 0xbc,
    PKT_MEDIA = 0xbf,
    PKT_EOS = 0xfb,
    PKT_FLT = 0xfc,
    PKT_UMF = 0xfd,
}

#[repr(u8)]
enum GXFMatTag {
    MAT_NAME = 0x40,
    MAT_FIRST_FIELD = 0x41,
    MAT_LAST_FIELD = 0x42,
    MAT_MARK_IN = 0x43,
    MAT_MARK_OUT = 0x44,
    MAT_SIZE = 0x45,
}

#[repr(u8)]
enum GXFTrackTag {
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
enum GXFTrackType {
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
enum GXFAncDataPresFormat {
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
enum MpegPictureCoding {
    CCX_MPC_NONE = 0,
    CCX_MPC_I_FRAME = 1,
    CCX_MPC_P_FRAME = 2,
    CCX_MPC_B_FRAME = 3,
}

#[repr(u8)]
#[derive(Debug)]
enum MpegPictureStruct {
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
struct CcxRational {
    num: i32,
    den: i32,
}

#[derive(Debug)]
struct CcxGxfVideoTrack {
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
struct CcxGxfAncillaryDataTrack {
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



impl CcxGxf {
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

pub unsafe fn parse_packet_header(
    ctx: *mut CcxDemuxer,
    pkt_type: &mut GXFPktType,
    length: &mut i32,
) -> i32 {
    if ctx.is_null() {
        return CCX_EINVAL;
    }

    let mut pkt_header = [0u8; 16];
    let result = buffered_read(
        ctx.as_mut().unwrap(),
        // pkt_header.as_mut_ptr(), inside Option<>
        Some(&mut pkt_header[..]),
        16);
    (*ctx).past += result as i64;

    if result != 16 {
        return CCX_EOF;
    }

    // Verify 5-byte packet leader (must be 0x00 0x00 0x00 0x00 0x00 0x01)
    if rb32(&pkt_header) != 0 {
        return CCX_EINVAL;
    }
    let mut index = 4;

    if pkt_header[index] != 1 {
        return CCX_EINVAL;
    }
    index += 1;

    *pkt_type = match pkt_header[index] {
        0xbc => GXFPktType::PKT_MAP,
        0xbf => GXFPktType::PKT_MEDIA,
        0xfb => GXFPktType::PKT_EOS,
        0xfc => GXFPktType::PKT_FLT,
        0xfd => GXFPktType::PKT_UMF,
        _ => return CCX_EINVAL,
    };
    index += 1;

    *length = rb32(<&[u8; 16]>::try_from(&pkt_header[index..]).unwrap()) as i32;
    index += 4;

    if (*length >> 24) != 0 || *length < 16 {
        return CCX_EINVAL;
    }
    *length -= 16;

    // Reserved as per Rdd-14-2007
    index += 4;

    // Verify packet trailer (must be 0xE1 0xE2)
    if pkt_header[index] != 0xe1 {
        return CCX_EINVAL;
    }
    index += 1;

    if pkt_header[index] != 0xe2 {
        return CCX_EINVAL;
    }

    CCX_OK
}
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
            break;
        }

        len -= tag_len as i32;

        match tag {
            x if x == GXFMatTag::MAT_NAME as u8 => {
                let result = buffered_read(
                    demux.as_mut().unwrap(),
                    // (*ctx).media_name.as_mut_ptr(),
                    Some(&mut (*ctx).media_name[..]),
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
        -1 => {
            /* Not applicable for this track type */
        }
        -2 => {
            /* Not available */
        }
        _ => {
            /* Do nothing in case of no frame rate */
        }
    }
}
pub unsafe fn parse_mpeg525_track_desc(demux: &mut CcxDemuxer, mut len: i32) -> i32 {
    let ctx = unsafe { &mut *(demux.private_data as *mut CcxGxf) };
    let vid_track = &mut ctx.vid_track.as_mut().unwrap();
    let mut ret = CCX_OK;

    /* Auxiliary Information */
    // let auxi_info: [u8; 8];  // Not used, keeping comment for reference

    dbg!("Mpeg 525 {}", len);

    while len > 2 {
        let tag = buffered_get_byte(demux);
        let tag_len = buffered_get_byte(demux) as i32;
        len -= 2;

        if len < tag_len {
            ret = CCX_EINVAL;
            break;
        }

        len -= tag_len;

        match tag {
            x if x == GXFTrackTag::TRACK_NAME as u8 => {
                let result = buffered_read(
                    demux,
                    // &mut vid_track.track_name,
                    Some(&mut vid_track.track_name),
                    tag_len as usize,
                );
                demux.past += tag_len as i64;
                if result != tag_len as usize {
                    ret = CCX_EOF;
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
                /* Not Supported */
                let result = buffered_skip(demux, tag_len as u32);
                demux.past += result as i64;
            }
            x if x == GXFTrackTag::TRACK_MPG_AUX as u8 => {
                /* Not Supported */
                let result = buffered_skip(demux, tag_len as u32);
                demux.past += result as i64;
            }
            _ => {
                /* Not Supported */
                let result = buffered_skip(demux, tag_len as u32);
                demux.past += result as i64;
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
pub unsafe fn parse_ad_track_desc(demux: &mut CcxDemuxer, mut len: i32) -> i32 {
    let ctx = unsafe { &mut *(demux.private_data as *mut CcxGxf) };
    let ad_track = match ctx.ad_track.as_mut() {
        Some(track) => track,
        None => return CCX_EINVAL, // If no ad_track, return error
    };

    let mut auxi_info = [0u8; 8];
    let mut ret = CCX_OK;

    dbg!("Ancillary Data {}", len);

    while len > 2 {
        let tag = buffered_get_byte(demux);
        let tag_len = buffered_get_byte(demux) as i32;
        len -= 2;

        if len < tag_len {
            ret = CCX_EINVAL;
            break;
        }

        len -= tag_len;

        match tag {
            x if x == GXFTrackTag::TRACK_NAME as u8 => {
                let result = buffered_read(
                    demux,
                    // &mut ad_track.track_name,
                    Some(&mut ad_track.track_name),
                    tag_len as usize);
                demux.past += tag_len as i64;
                if result != tag_len as usize {
                    ret = CCX_EOF;
                    break;
                }
            }
            x if x == GXFTrackTag::TRACK_AUX as u8 => {
                let result = buffered_read(
                    demux,
                    // &mut auxi_info,
                    Some(&mut auxi_info),
                    8,
                );
                demux.past += 8;
                if result != 8 {
                    ret = CCX_EOF;
                    break;
                }
                if tag_len != 8 {
                    ret = CCX_EINVAL;
                    break;
                }

                ad_track.ad_format = match auxi_info[2] {
                    1 => GXFAncDataPresFormat::PRES_FORMAT_SD,
                    2 => GXFAncDataPresFormat::PRES_FORMAT_HD,
                    _ => GXFAncDataPresFormat::PRES_FORMAT_SD,
                };
                ad_track.nb_field = auxi_info[3] as i32;
                ad_track.field_size = i16::from_be_bytes([auxi_info[4], auxi_info[5]]) as i32;
                ad_track.packet_size = i16::from_be_bytes([auxi_info[6], auxi_info[7]]) as i32 * 256;

                dbg!(
                    "ad_format {} nb_field {} field_size {} packet_size {} track id {}",
                    ad_track.ad_format, ad_track.nb_field, ad_track.field_size, ad_track.packet_size, ad_track.id
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
                /* Not Supported */
                let result = buffered_skip(demux, tag_len as u32);
                demux.past += result as i64;
            }
            _ => {
                /* Not Supported */
                let result = buffered_skip(demux, tag_len as u32);
                demux.past += result as i64;
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

pub unsafe fn parse_track_sec(demux: &mut CcxDemuxer, mut len: i32, data: &mut DemuxerData) -> i32 {
    let ctx = match unsafe { (demux.private_data as *mut CcxGxf).as_mut() } {
        Some(ctx) => ctx,
        None => return CCX_EINVAL, // If context is null, return error
    };

    let mut ret = CCX_OK;

    while len > 4 {
        let mut track_type = buffered_get_byte(demux);
        let mut track_id = buffered_get_byte(demux);
        let track_len = buffered_get_be16(demux) as i32;
        len -= 4;

        if len < track_len {
            ret = CCX_EINVAL;
            break;
        }

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
                if ctx.ad_track.is_none() {
                    ctx.ad_track = Some(Box::new(CcxGxfAncillaryDataTrack::default()));
                }

                if let Some(ad_track) = ctx.ad_track.as_mut() {
                    ad_track.id = track_id;
                    parse_ad_track_desc(demux, track_len);
                    /* Ancillary data track has raw Closed Caption with cctype */
                    data.bufferdatatype = BufferdataType::Raw;
                }
                len -= track_len;
            }
            x if x == GXFTrackType::TRACK_TYPE_MPEG2_525 as u8 => {
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
    if cdp.len() < 11 {
        info!("Short packet can't accommodate header and footer");
        return Err("Invalid packet length");
    }

    if cdp[0] != 0x96 || cdp[1] != 0x69 {
        info!("Could not find CDP identifier of 0x96 0x69");
        return Err("Invalid CDP identifier");
    }

    let mut cdp = &cdp[2..];
    let cdp_length = cdp[0] as usize;
    cdp = &cdp[1..];

    if cdp_length != cdp.len() + 2 {
        info!("CDP length is not valid");
        return Err("Mismatched CDP length");
    }

    let cdp_framerate = (cdp[0] & 0xF0) >> 4;
    let cc_data_present = (cdp[0] & 0x40) >> 6;
    let caption_service_active = (cdp[1] & 0x02) >> 1;
    let cdp_header_sequence_counter = u16::from_be_bytes([cdp[2], cdp[3]]);
    cdp = &cdp[4..];

    dbg!("CDP length: {} words", cdp_length);
    dbg!("CDP frame rate: 0x{:x}", cdp_framerate);
    dbg!("CC data present: {}", cc_data_present);
    dbg!("Caption service active: {}", caption_service_active);
    dbg!("Header sequence counter: {} (0x{:x})", cdp_header_sequence_counter, cdp_header_sequence_counter);

    match cdp[0] {
        0x71 => {
            info!("Ignore Time code section");
            return Err("Time code section ignored");
        }
        0x72 => {
            let cc_count = cdp[1] & 0x1F;
            dbg!("cc_count: {}", cc_count);

            let copy_size = cc_count as usize * 3;
            if copy_size > cdp.len() - 2 {
                return Err("Insufficient data for CC section");
            }

            let buffer_slice = unsafe { slice::from_raw_parts_mut(data.buffer, data.len + copy_size) };
            buffer_slice[data.len..data.len + copy_size].copy_from_slice(&cdp[2..2 + copy_size]);
            data.len += copy_size;
            cdp = &cdp[2 + copy_size..];
        }
        0x73 => {
            info!("Ignore service information section");
            return Err("Service information section ignored");
        }
        0x75..=0xEF => {
            info!("Newer version of SMPTE-334 specification detected. New section id 0x{:x}", cdp[0]);
            return Err("Unhandled new section");
        }
        _ => {}
    }

    if cdp[0] == 0x74 {
        let footer_sequence_counter = u16::from_be_bytes([cdp[1], cdp[2]]);
        if cdp_header_sequence_counter != footer_sequence_counter {
            info!("Incomplete CDP packet");
            return Err("CDP footer sequence mismatch");
        }
    }

    Ok(())
}
/**
 * parse ancillary data payload
 */
pub unsafe fn parse_ad_pyld(
    demux: &mut CcxDemuxer,
    len: i32,
    data: &mut DemuxerData,
) -> i32 {
    let mut result;
    let mut ret = CCX_OK;

    #[cfg(not(feature = "CCX_GXF_ENABLE_AD_VBI"))]
    {
        let mut i: usize;

        // Read 16-bit values from buffered input
        let d_id = buffered_get_le16(demux);
        let sd_id = buffered_get_le16(demux);
        let mut dc = buffered_get_le16(demux) & 0xFF;

        let ctx = unsafe { &mut *(demux.private_data as *mut CcxGxf) };

        // Adjust length
        let len = len - 6;

        // If `ctx.cdp` buffer is too small, resize it
        if ctx.cdp_len < (len / 2) as usize {
            ctx.cdp = Some(vec![0; (len / 2) as usize]);
            if (ctx.cdp.is_none()) {
                info!("Could not allocate buffer {}\n", len / 2);
                return CCX_ENOMEM;
            }
            ctx.cdp_len = ((len - 2) / 2) as usize; // Exclude DID and SDID bytes
        }

        // Check if the data corresponds to CEA-708 captions
        if ((d_id & 0xFF) == CLOSED_CAP_DID as u16) && ((sd_id & 0xFF) == CLOSED_C708_SDID as u16) {
            if let Some(ref mut cdp) = ctx.cdp {
                i = 0;
                let mut remaining_len = len;

                while remaining_len > 2 {
                    let dat = buffered_get_le16(demux);

                    // Check parity for 0xFE and 0x01 (possibly converted from 0xFF and 0x00 by GXF)
                    // Ignoring the first 2 bits or bytes from a 10-bit code in a 16-bit variable
                    cdp[i] = match dat {
                        0x2FE => 0xFF,
                        0x201 => 0x01,
                        _ => (dat & 0xFF) as u8,
                    };

                    i += 1;
                    remaining_len -= 2;
                }

                parse_ad_cdp(cdp, data);
                // TODO: Check checksum
            }
        }
        // Check if the data corresponds to CEA-608 captions
        else if ((d_id & 0xFF) == CLOSED_CAP_DID as u16) && ((sd_id & 0xFF) == CLOSED_C608_SDID as u16) {
            info!("Need Sample\n");
        }
        // Ignore other services like:
        // Program description (DTV) with DID = 62h(162h) and SDID 1 (101h)
        // Data broadcast (DTV) with DID = 62h(162h) and SDID 2 (102h)
        // VBI data with DID = 62h(162h) and SDID 3 (103h)
    }

    // Skip the remaining bytes in the buffer
    result = buffered_skip(demux, len as u32);
    demux.past += result as i64;
    if result != len as usize {
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
fn parse_ad_vbi(demux: &mut CcxDemuxer, mut len: i32, data: &mut DemuxerData) -> i32 {
    let mut ret = CCX_OK;
    let result;

    #[cfg(feature = "ccx_gxf_enable_ad_vbi")]
    unsafe {
        // Increase buffer length
        data.len += len as usize;
        // Read data into buffer
        result = buffered_read(
            demux,
            // (data.buffer),
            Some(&mut data.buffer[data.len..]),
            len as usize,
        );
    }
    #[cfg(not(feature = "ccx_gxf_enable_ad_vbi"))]
    unsafe {
        // Skip bytes if VBI support is not enabled
        result = buffered_skip(demux, len as u32);
    }
    // Update file position
    demux.past += result as i64;
    if result != len as usize {
        ret = CCX_EOF;
    }
    ret
}

unsafe fn parse_ad_field(demux: &mut CcxDemuxer, mut len: i32, data: &mut DemuxerData) -> i32 {
    let mut ret = CCX_OK;
    let mut result;
    let mut tag = [0u8; 5]; // Null-terminated string buffer
    let field_identifier;

    tag[4] = 0;

    // Read 'finf' tag
    len -= 4;
    result = buffered_read(demux, Option::from(&mut tag[..4]), 4);
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
    field_identifier = buffered_get_le32(demux);
    dbg!("LOG: field identifier {}\n", field_identifier);

    // Read 'LIST' tag
    len -= 4;
    result = buffered_read(demux, Option::from(&mut tag[..4]), 4);
    demux.past += result as i64;
    if &tag[..4] != b"LIST" {
        info!("Warning: No List tag\n");
    }

    // Read ancillary data field section size
    len -= 4;
    if buffered_get_le32(demux) != len as u32 {
        info!("Warning: Unexpected sample size (!={})\n", len);
    }

    // Read 'anc ' tag
    len -= 4;
    result = buffered_read(demux, Option::from(&mut tag[..4]), 4);
    demux.past += result as i64;
    if &tag[..4] != b"anc " {
        info!("Warning: No anc tag\n");
    }

    while len > 28 {
        let mut line_nb;
        let mut luma_flag;
        let mut hanc_vanc_flag;
        let mut hdr_len;
        let mut pyld_len;

        // Read next tag
        len -= 4;
        result = buffered_read(demux, Option::from(&mut tag[..4]), 4);
        demux.past += result as i64;

        // Read header length
        len -= 4;
        hdr_len = buffered_get_le32(demux);

        // Check for 'pad ' tag
        if &tag[..4] == b"pad " {
            if hdr_len != len as u32 {
                info!("Warning: expected {} got {}\n", len, hdr_len);
            }
            len -= hdr_len as i32;
            result = buffered_skip(demux, hdr_len as u32);
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

        // Read line number
        len -= 4;
        line_nb = buffered_get_le32(demux);
        dbg!("Line nb: {}\n", line_nb);

        // Read luma flag
        len -= 4;
        luma_flag = buffered_get_le32(demux);
        dbg!("luma color diff flag: {}\n", luma_flag);

        // Read HANC/VANC flag
        len -= 4;
        hanc_vanc_flag = buffered_get_le32(demux);
        dbg!("hanc/vanc flag: {}\n", hanc_vanc_flag);

        // Read next tag
        len -= 4;
        result = buffered_read(demux, Option::from(&mut tag[..4]), 4);
        demux.past += result as i64;

        // Read payload length
        len -= 4;
        pyld_len = buffered_get_le32(demux);
        dbg!("pyld len: {}\n", pyld_len);

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

    // Skip remaining bytes
    result = buffered_skip(demux, len as u32);
    demux.past += result as i64;
    if result != len as usize {
        ret = CCX_EOF;
    }
    ret
}
/**
 * @param vid_format following format are supported to set valid timebase
 * 	in demuxer data
 * value   |  Meaning
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

fn set_data_timebase(vid_format: i32, data: &mut DemuxerData) {
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

unsafe fn parse_mpeg_packet(demux: &mut CcxDemuxer, len: usize, data: &mut DemuxerData) -> i32 {
    let result = buffered_read(
        demux,
        // unsafe { data.buffer.add(data.len) },
        // Some(&mut *data.buffer[data.len..]),
        Some(std::slice::from_raw_parts_mut(data.buffer.add(data.len), len)),
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
unsafe fn parse_ad_packet(demux: &mut CcxDemuxer, len: i32, data: &mut DemuxerData) -> i32 {
    let mut ret = CCX_OK;
    let mut result;
    let mut remaining_len = len;
    let mut tag = [0u8; 4];

    let ctx = unsafe { &mut *(demux.private_data as *mut CcxGxf) };

    let ad_track = &ctx.ad_track.as_mut().unwrap();

    // Read "RIFF" header
    remaining_len -= 4;
    result = buffered_read(
        demux,
        // &mut tag,
        Some(&mut tag),
        4,
    );
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
    result = buffered_read(
        demux,
        // &mut tag,
        Some(&mut tag),
        4,
    );
    demux.past += result as i64;
    if &tag != b"rcrd" {
        info!("Warning: No rcrd tag");
    }

    // Read "desc" tag
    remaining_len -= 4;
    result = buffered_read(
        demux,
        // &mut tag,
        Some(&mut tag),
        4,
    );
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
    result = buffered_read(
        demux,
        // &mut tag,
        Some(&mut tag),
        4);
    demux.past += result as i64;
    if &tag != b"LIST" {
        info!("Warning: No LIST tag");
    }

    // Validate field section size
    remaining_len -= 4;
    if buffered_get_le32(demux) != remaining_len as u32 {
        info!("Warning: Unexpected field section size (!={})", remaining_len);
    }

    // Read "fld " tag
    remaining_len -= 4;
    result = buffered_read(
        demux,
        // &mut tag,
        Some(&mut tag),
        4,
    );
    demux.past += result as i64;
    if &tag != b"fld " {
        info!("Warning: No fld tag");
    }

    // Parse each field
    for _ in 0..ad_track.nb_field {
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
/// Translated version of the C `set_mpeg_frame_desc` function.

fn set_mpeg_frame_desc(vid_track: &mut CcxGxfVideoTrack, mpeg_frame_desc_flag: u8) {
    // vid_track.p_code = MpegPictureCoding::from(mpeg_frame_desc_flag & 0x03);
    // vid_track.p_struct = MpegPictureStruct::from((mpeg_frame_desc_flag >> 2) & 0x03);
    vid_track.p_code = MpegPictureCoding::try_from(mpeg_frame_desc_flag & 0x03).unwrap();
    vid_track.p_struct = MpegPictureStruct::try_from((mpeg_frame_desc_flag >> 2) & 0x03).unwrap();
}

impl PartialEq for GXFTrackType {
    fn eq(&self, other: &Self) -> bool {
        std::mem::discriminant(self) == std::mem::discriminant(other)
    }
}


/// Translated version of the C `parse_media` function.
unsafe fn parse_media(demux: &mut CcxDemuxer, mut len: i32, data: &mut DemuxerData) -> i32 {
    let mut ret = CCX_OK;
    let mut result: i32;
    let media_type: GXFTrackType;
    let ctx = unsafe { &mut *(demux.private_data as *mut CcxGxf) };
    #[allow(unused)]
    let mut ad_track: &mut CcxGxfAncillaryDataTrack;
    // let mut vid_track: Option<&mut CcxGxfVideoTrack>;
    // struct ccx_gxf_video_track *vid_track;
    /**
     * The track number is a reference into the track description section of the map packets. It identifies the media
     * file to which the current media packet belongs. Track descriptions shall be considered as consecutive
     * elements of a vector and track numbers are the index into that vector.
     * Clips shall have 1 to 48 tracks in any combination of types. Tracks shall be numbered from 0 to n–1. Media
     * packets shall be inserted in the stream starting with track n-1 and proceeding to track 0. Time code packets
     * shall have the greatest track numbers. Ancillary data packets shall have track numbers less than time code
     * packets. Audio packets shall have track numbers less than ancillary data packets. Video packet track
     * numbers shall be less than audio track numbers.
     */

    let track_nb: u8;
    let media_field_nb: u32;
    /**
     * For ancillary data, the field information contains the first valid ancillary data field number (inclusive) and the
     * last valid ancillary data field number (exclusive). The first and last valid ancillary data field numbers apply to
     * the current packet only. If the entire ancillary data packet is valid, the first and last valid ancillary data field
     * numbers shall be 0 and 10 for high definition ancillary data, and 0 and 14 for standard definition ancillary data.
     * These values shall be sent starting with the ancillary data field from the lowest number video line continuing to
     * higher video line numbers. Within each line the ancillary data fields shall not be reordered.
     */

    let mut first_field_nb: u16 = 0;
    let mut last_field_nb: u16 = 0;
    /**
     *  see description of set_mpeg_frame_desc for details
     */

    let mut mpeg_pic_size: u32 = 0;
    let mut mpeg_frame_desc_flag: u8 = 0;
    /**
     * Observation 1: media_field_nb comes out equal to time_field number
     * for ancillary data
     *
     * The 32-bit unsigned field number relative to the start of the
     * material (time line position).
     * Time line field numbers shall be assigned consecutively from the
     * start of the material. The first time line field number shall be 0.
     */

    let time_field: u32;
    let valid_time_field: u8;

    if ctx.is_null() {
        return ret;
    }

    len -= 1;
    media_type = GXFTrackType::try_from(buffered_get_byte(demux)).unwrap();
    track_nb = buffered_get_byte(demux);
    len -= 1;
    media_field_nb = buffered_get_be32(demux);
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
            mpeg_pic_size &= 0xFFFFFF;
            len -= 4;
        }
        _ => {
            result = buffered_skip(demux, 4) as i32;
            demux.past += result as i64;
            len -= 4;
        }
    }

    time_field = buffered_get_be32(demux);
    len -= 4;

    valid_time_field = buffered_get_byte(demux) & 0x01;
    len -= 1;

    result = buffered_skip(demux, 1) as i32;
    demux.past += result as i64;
    len -= 1;

    dbg!("track number {}\n", track_nb);
    dbg!("field number {}\n", media_field_nb);
    dbg!("first field number {}\n", first_field_nb);
    dbg!("last field number {}\n", last_field_nb);
    dbg!("Pyld len {}\n", len);

    if media_type == GXFTrackType::TRACK_TYPE_ANCILLARY_DATA {
        if ctx.ad_track.is_none() {
            return ret;
        }
        let ad_track = &mut ctx.ad_track.as_mut().unwrap();
        if valid_time_field != 0 {
            data.pts = time_field as i64 - ctx.first_field_nb as i64;
        } else {
            data.pts = media_field_nb as i64 - ctx.first_field_nb as i64;
        }
        if len < ad_track.packet_size as i32 {
            return ret;
        }

        data.pts /= 2;

        parse_ad_packet(demux, ad_track.packet_size as i32, data);
        len -= ad_track.packet_size as i32;
    } else if media_type == GXFTrackType::TRACK_TYPE_MPEG2_525 && ctx.ad_track.is_none() {
        if ctx.vid_track.is_none() {
            return ret;
        }
        // vid_track = ctx.vid_track.as_mut();
        if valid_time_field != 0 {
            data.pts = time_field as i64 - ctx.first_field_nb as i64;
        } else {
            data.pts = media_field_nb as i64 - ctx.first_field_nb as i64;
        }
        data.tb.num = ctx.vid_track.as_mut().unwrap().frame_rate.den;
        data.tb.den = ctx.vid_track.as_mut().unwrap().frame_rate.num;
        data.pts /= 2;

        set_mpeg_frame_desc(ctx.vid_track.as_mut().unwrap(), mpeg_frame_desc_flag);
        parse_mpeg_packet(demux, mpeg_pic_size as usize, data);
        len -= mpeg_pic_size as i32;
    } else if media_type == GXFTrackType::TRACK_TYPE_TIME_CODE_525 {
        // Need SMPTE 12M to follow parse time code
    }

    result = buffered_skip(demux, len as u32) as i32;
    demux.past += result as i64;
    if result != len {
        ret = CCX_EOF;
    }
    ret
}
/**
 * Dummy function that ignore field locator table packet
 */
unsafe fn parse_flt(demux: &mut CcxDemuxer, len: i32) -> i32 {
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

unsafe fn parse_umf(demux: &mut CcxDemuxer, len: i32) -> i32 {
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


unsafe fn parse_map(demux: &mut CcxDemuxer, mut len: i32, data: &mut DemuxerData) -> i32 {
    let mut result: i32;
    let mut material_sec_len: i32 = 0;
    let mut track_sec_len: i32 = 0;
    let mut ret = CCX_OK;

    len -= 2;
    if buffered_get_be16(demux) != 0xe0ff {
        return CCX_EINVAL;
    }

    len -= 2;
    material_sec_len = buffered_get_be16(demux) as i32;
    if material_sec_len > len {
        return CCX_EINVAL;
    }

    len -= material_sec_len;
    parse_material_sec(demux, material_sec_len);

    len -= 2;
    track_sec_len = buffered_get_be16(demux) as i32;
    if track_sec_len > len {
        return CCX_EINVAL;
    }

    len -= track_sec_len;
    parse_track_sec(demux, track_sec_len, data);

    result = buffered_skip(demux, len as u32) as i32;
    demux.past += result as i64;
    if result != len {
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
unsafe fn read_packet(demux: &mut CcxDemuxer, data: &mut DemuxerData) -> i32 {
    let mut len = 0;
    #[allow(unused)]
    let mut result = 0;
    let mut ret = CCX_OK;
    let mut gxftype: GXFPktType = GXFPktType::PKT_EOS;

    ret = parse_packet_header(demux, &mut gxftype, &mut len);

    match gxftype {
        GXFPktType::PKT_MAP => {
            dbg!("pkt type Map {}\n", len);
            ret = parse_map(demux, len, data);
        }
        GXFPktType::PKT_MEDIA => {
            ret = parse_media(demux, len, data);
        }
        GXFPktType::PKT_EOS => {
            ret = CCX_EOF;
        }
        GXFPktType::PKT_FLT => {
            dbg!("pkt type FLT {}\n", len);
            ret = parse_flt(demux, len);
        }
        GXFPktType::PKT_UMF => {
            dbg!("pkt type umf {}\n\n", len);
            ret = parse_umf(demux, len);
        }
        // _ => {
        //     debug!("pkt type unknown or bad {}\n", len);
        //     result = buffered_skip(demux, len as u32) as i32;
        //     demux.past += result as i64;
        //     if result != len || len == 0 {
        //         ret = CCX_EOF;
        //     }
        // }
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
pub unsafe fn ccx_gxf_probe(buf: &[u8]) -> bool {
    let startcode = [0, 0, 0, 0, 1, 0xbc];
    if buf.len() < startcode.len() {
        return false;
    }
    //use libc
    if libc::memcmp(
        buf.as_ptr() as *const libc::c_void,
        startcode.as_ptr() as *const libc::c_void,
        startcode.len()) == 0 {
        return true;
    }
    false
}

use crate::demuxer::lib_ccx::LibCcxCtx;
use libc::malloc;

unsafe fn ccx_gxf_get_more_data(ctx: &mut LibCcxCtx, ppdata: &mut Option<Box<DemuxerData>>) -> i32 {
    if ppdata.is_none() {
        // *ppdata = Some(alloc_demuxer_data()); //TODO
        if ppdata.is_none() {
            return -1;
        }
        let data = ppdata.as_mut().unwrap();
        // TODO: Set to dummy, find and set actual value
        // Complex GXF does have multiple programs
        data.program_number = 1;
        data.stream_pid = 1;
        data.codec = Codec::AtscCc;
    }
    let data = ppdata.as_mut().unwrap();
    read_packet(&mut *ctx.demux_ctx, data)
}

pub fn ccx_gxf_init(_arg: &mut CcxDemuxer) -> Option<*mut CcxGxf> {
    unsafe {
        let ctx = malloc(std::mem::size_of::<CcxGxf>()) as *mut CcxGxf;
        if ctx.is_null() {
            eprintln!("Unable to allocate Gxf context");
            return None;
        }
        ptr::write_bytes(ctx, 0, 1); // memset to zero
        Some(ctx)
    }
}

pub fn ccx_gxf_delete(arg: &mut CcxDemuxer) {
    unsafe {
        if !arg.private_data.is_null() {
            // freep((*arg.private_data).cdp as *mut libc::c_void); //TODO
            // freep(arg.private_data as *mut libc::c_void); //TODO
            arg.private_data = ptr::null_mut();
        }
    }
}
