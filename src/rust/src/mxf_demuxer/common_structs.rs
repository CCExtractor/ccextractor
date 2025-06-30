use crate::demuxer::common_structs::{CcxDemuxer, CcxRational};
use crate::gxf_demuxer::common_structs::DemuxerError;
use crate::mxf_demuxer::mxf::{
    mxf_read_header_partition_pack, mxf_read_timeline_track_metadata, mxf_read_vanc_vbi_desc,
};
use lib_ccxr::common::Options;
use std::convert::TryFrom;

// Type alias for UID - 16 bytes
pub type UidRust = [u8; 16];

// Enum for MXF Caption Types
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(C)]
pub enum MXFCaptionTypeRust {
    Vbi,
    Anc,
}

// Enum for MXF Local Tags
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u16)]
pub enum MXFLocalTagRust {
    LTrackId = 0x3006,
    TrackId = 0x4801,
    TrackNumber = 0x4804,
    EditRate = 0x4b01,
}
impl TryFrom<u16> for MXFLocalTagRust {
    type Error = &'static str;

    fn try_from(value: u16) -> Result<Self, Self::Error> {
        match value {
            0x3006 => Ok(MXFLocalTagRust::LTrackId),
            0x4801 => Ok(MXFLocalTagRust::TrackId),
            0x4804 => Ok(MXFLocalTagRust::TrackNumber),
            0x4b01 => Ok(MXFLocalTagRust::EditRate),
            _ => Err("Invalid MXF Local Tag"),
        }
    }
}

// KLV Packet structure
#[derive(Debug, Clone)]
#[repr(C)]
pub struct KLVPacketRust {
    pub key: UidRust,
    pub length: u64,
}

impl KLVPacketRust {
    pub(crate) fn default() -> Self {
        Self {
            key: [0; 16],
            length: 0,
        }
    }
}

// MXF Codec UL structure
#[derive(Debug, Clone)]
#[repr(C)]
pub struct MXFCodecULRust {
    pub uid: UidRust,
    pub caption_type: MXFCaptionTypeRust,
}

// Function pointer type for read functions
pub type ReadFuncRust = unsafe fn(&mut CcxDemuxer, u64, &mut Options) -> Result<i32, DemuxerError>;

// MXF Read Table Entry
#[derive(Clone)]
#[repr(C)]
pub struct MXFReadTableEntryRust {
    pub key: UidRust,
    pub read: Option<ReadFuncRust>,
}

// MXF Track structure
#[derive(Debug, Clone)]
#[repr(C)]
#[derive(Copy, Default)]
pub struct MXFTrackRust {
    pub track_id: i32,
    pub track_number: [u8; 4],
}

// Main MXF Context structure
#[derive(Debug, Clone)]
#[repr(C)]
pub struct MXFContextRust {
    pub caption_type: MXFCaptionTypeRust,
    pub cap_track_id: i32,
    pub cap_essence_key: UidRust,
    pub tracks: [MXFTrackRust; 32],
    pub nb_tracks: i32,
    pub cap_count: i32,
    pub edit_rate: CcxRational,
}

// Constants converted to Rust
pub const MXF_HEADER_PARTITION_PACK_KEY_RUST: [u8; 14] = [
    0x06, 0x0e, 0x2b, 0x34, 0x02, 0x05, 0x01, 0x01, 0x0d, 0x01, 0x02, 0x01, 0x01, 0x02,
];

pub const MXF_ESSENCE_ELEMENT_KEY_RUST: [u8; 12] = [
    0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01,
];

pub const MXF_KLV_KEY_RUST: [u8; 4] = [0x06, 0x0e, 0x2b, 0x34];

// Caption essence container constants
pub const MXF_CAPTION_ESSENCE_CONTAINER_RUST: [MXFCodecULRust; 2] = [
    MXFCodecULRust {
        uid: [
            0x6, 0xE, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x09, 0xD, 0x1, 0x3, 0x1, 0x2, 0xD, 0x0, 0x0,
        ],
        caption_type: MXFCaptionTypeRust::Vbi,
    },
    MXFCodecULRust {
        uid: [
            0x6, 0xE, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x09, 0xD, 0x1, 0x3, 0x1, 0x2, 0xE, 0x0, 0x0,
        ],
        caption_type: MXFCaptionTypeRust::Anc,
    },
];

// Framerate rationals constants
pub const FRAMERATE_RATIONALS_RUST: [CcxRational; 10] = [
    CcxRational { num: 0, den: 0 },
    CcxRational {
        num: 1001,
        den: 24000,
    },
    CcxRational { num: 1, den: 24 },
    CcxRational { num: 1, den: 25 },
    CcxRational {
        num: 1001,
        den: 30000,
    },
    CcxRational { num: 1, den: 30 },
    CcxRational { num: 1, den: 50 },
    CcxRational {
        num: 1001,
        den: 60000,
    },
    CcxRational { num: 1, den: 60 },
    CcxRational { num: 0, den: 0 },
];

// Implementations for default values and convenience methods

impl Default for MXFContextRust {
    fn default() -> Self {
        Self {
            caption_type: MXFCaptionTypeRust::Vbi,
            cap_track_id: 0,
            cap_essence_key: [0; 16],
            tracks: [MXFTrackRust::default(); 32],
            nb_tracks: 0,
            cap_count: 0,
            edit_rate: CcxRational { num: 0, den: 0 },
        }
    }
}

// MXF Read Table constant
pub const MXF_READ_TABLE_RUST: [MXFReadTableEntryRust; 4] = [
    // According to section 7.1 of S377 partition key byte 14 and 15 have variable values
    MXFReadTableEntryRust {
        key: [
            0x06, 0x0e, 0x2b, 0x34, 0x02, 0x05, 0x01, 0x01, 0x0d, 0x01, 0x02, 0x01, 0x01, 0x02,
            0x04, 0x00,
        ],
        read: Some(mxf_read_header_partition_pack),
    },
    // Structural Metadata Sets
    MXFReadTableEntryRust {
        key: [
            0x06, 0x0e, 0x2b, 0x34, 0x02, 0x53, 0x01, 0x01, 0x0d, 0x01, 0x01, 0x01, 0x01, 0x01,
            0x3B, 0x00,
        ],
        read: Some(mxf_read_timeline_track_metadata),
    },
    MXFReadTableEntryRust {
        key: [
            0x06, 0x0e, 0x2b, 0x34, 0x02, 0x53, 0x01, 0x01, 0x0d, 0x01, 0x01, 0x01, 0x01, 0x01,
            0x5c, 0x00,
        ],
        read: Some(mxf_read_vanc_vbi_desc),
    },
    // Generic Track (terminator with null key and function)
    MXFReadTableEntryRust {
        key: [
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00,
        ],
        read: None,
    },
];
