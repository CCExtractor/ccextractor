pub const ZEROBYTES_SHORTSTARTCODE: i32 = 2;
pub const AVC_CC_DATA_INITIAL_SIZE: usize = 1024;
pub const MAXBFRAMES: i32 = 50;

#[derive(Debug, Clone)]
pub struct AvcContextRust {
    pub cc_count: u8,
    pub cc_data: Vec<u8>,
    pub cc_databufsize: usize,
    pub cc_buffer_saved: bool,

    pub got_seq_para: bool,
    pub nal_ref_idc: u32,
    pub seq_parameter_set_id: i64,
    pub log2_max_frame_num: i32,
    pub pic_order_cnt_type: i32,
    pub log2_max_pic_order_cnt_lsb: i32,
    pub frame_mbs_only_flag: bool,

    // Statistics for debugging
    pub num_nal_unit_type_7: i64,
    pub num_vcl_hrd: i64,
    pub num_nal_hrd: i64,
    pub num_jump_in_frames: i64,
    pub num_unexpected_sei_length: i64,

    pub ccblocks_in_avc_total: i32,
    pub ccblocks_in_avc_lost: i32,

    pub frame_num: i64,
    pub lastframe_num: i64,
    pub currref: i32,
    pub maxidx: i32,
    pub lastmaxidx: i32,

    // Used to find tref zero in PTS mode
    pub minidx: i32,
    pub lastminidx: i32,

    // Used to remember the max temporal reference number (poc mode)
    pub maxtref: i32,
    pub last_gop_maxtref: i32,

    // Used for PTS ordering of CC blocks
    pub currefpts: i64,
    pub last_pic_order_cnt_lsb: i64,
    pub last_slice_pts: i64,
}

impl Default for AvcContextRust {
    fn default() -> Self {
        AvcContextRust {
            cc_count: 0,
            cc_data: Vec::with_capacity(1024),
            cc_databufsize: 1024,
            cc_buffer_saved: true,

            got_seq_para: false,
            nal_ref_idc: 0,
            seq_parameter_set_id: 0,
            log2_max_frame_num: 0,
            pic_order_cnt_type: 0,
            log2_max_pic_order_cnt_lsb: 0,
            frame_mbs_only_flag: false,

            num_nal_unit_type_7: 0,
            num_vcl_hrd: 0,
            num_nal_hrd: 0,
            num_jump_in_frames: 0,
            num_unexpected_sei_length: 0,

            ccblocks_in_avc_total: 0,
            ccblocks_in_avc_lost: 0,

            frame_num: -1,
            lastframe_num: -1,
            currref: 0,
            maxidx: -1,
            lastmaxidx: -1,

            minidx: 10000,
            lastminidx: 10000,

            maxtref: 0,
            last_gop_maxtref: 0,

            currefpts: 0,
            last_pic_order_cnt_lsb: -1,
            last_slice_pts: -1,
        }
    }
}

// SEI payload types
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u32)]
pub enum SeiPayloadType {
    BufferingPeriod = 0,
    PicTiming = 1,
    PanScanRect = 2,
    FillerPayload = 3,
    UserDataRegisteredItuTT35 = 4,
    UserDataUnregistered = 5,
    RecoveryPoint = 6,
    DecRefPicMarkingRepetition = 7,
    SparePic = 8,
    SceneInfo = 9,
    SubSeqInfo = 10,
    SubSeqLayerCharacteristics = 11,
    SubSeqCharacteristics = 12,
    FullFrameFreeze = 13,
    FullFrameFreezeRelease = 14,
    FullFrameSnapshot = 15,
    ProgressiveRefinementSegmentStart = 16,
    ProgressiveRefinementSegmentEnd = 17,
    MotionConstrainedSliceGroupSet = 18,
    FilmGrainCharacteristics = 19,
    DeblockingFilterDisplayPreference = 20,
    StereoVideoInfo = 21,
}

impl From<u32> for SeiPayloadType {
    fn from(value: u32) -> Self {
        match value {
            0 => SeiPayloadType::BufferingPeriod,
            1 => SeiPayloadType::PicTiming,
            2 => SeiPayloadType::PanScanRect,
            3 => SeiPayloadType::FillerPayload,
            4 => SeiPayloadType::UserDataRegisteredItuTT35,
            5 => SeiPayloadType::UserDataUnregistered,
            6 => SeiPayloadType::RecoveryPoint,
            7 => SeiPayloadType::DecRefPicMarkingRepetition,
            8 => SeiPayloadType::SparePic,
            9 => SeiPayloadType::SceneInfo,
            10 => SeiPayloadType::SubSeqInfo,
            11 => SeiPayloadType::SubSeqLayerCharacteristics,
            12 => SeiPayloadType::SubSeqCharacteristics,
            13 => SeiPayloadType::FullFrameFreeze,
            14 => SeiPayloadType::FullFrameFreezeRelease,
            15 => SeiPayloadType::FullFrameSnapshot,
            16 => SeiPayloadType::ProgressiveRefinementSegmentStart,
            17 => SeiPayloadType::ProgressiveRefinementSegmentEnd,
            18 => SeiPayloadType::MotionConstrainedSliceGroupSet,
            19 => SeiPayloadType::FilmGrainCharacteristics,
            20 => SeiPayloadType::DeblockingFilterDisplayPreference,
            21 => SeiPayloadType::StereoVideoInfo,
            _ => SeiPayloadType::UserDataUnregistered, // Default fallback
        }
    }
}
// NAL unit types from H.264 standard
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum NalUnitType {
    Unspecified = 0,
    CodedSliceNonIdr = 1,
    CodedSliceDataPartitionA = 2,
    CodedSliceDataPartitionB = 3,
    CodedSliceDataPartitionC = 4,
    CodedSliceIdr = 5,
    Sei = 6,
    SequenceParameterSet = 7,
    PictureParameterSet = 8,
    AccessUnitDelimiter = 9,
    EndOfSequence = 10,
    EndOfStream = 11,
    FillerData = 12,
    SequenceParameterSetExtension = 13,
    PrefixNalUnit = 14,
    SubsetSequenceParameterSet = 15,
    DepthParameterSet = 16,
    // 17-18 reserved
    CodedSliceAuxiliary = 19,
    CodedSliceExtension = 20,
    // 21-23 reserved
    // 24-31 unspecified
}

impl From<u8> for NalUnitType {
    fn from(value: u8) -> Self {
        match value {
            0 => NalUnitType::Unspecified,
            1 => NalUnitType::CodedSliceNonIdr,
            2 => NalUnitType::CodedSliceDataPartitionA,
            3 => NalUnitType::CodedSliceDataPartitionB,
            4 => NalUnitType::CodedSliceDataPartitionC,
            5 => NalUnitType::CodedSliceIdr,
            6 => NalUnitType::Sei,
            7 => NalUnitType::SequenceParameterSet,
            8 => NalUnitType::PictureParameterSet,
            9 => NalUnitType::AccessUnitDelimiter,
            10 => NalUnitType::EndOfSequence,
            11 => NalUnitType::EndOfStream,
            12 => NalUnitType::FillerData,
            13 => NalUnitType::SequenceParameterSetExtension,
            14 => NalUnitType::PrefixNalUnit,
            15 => NalUnitType::SubsetSequenceParameterSet,
            16 => NalUnitType::DepthParameterSet,
            19 => NalUnitType::CodedSliceAuxiliary,
            20 => NalUnitType::CodedSliceExtension,
            _ => NalUnitType::Unspecified,
        }
    }
}
