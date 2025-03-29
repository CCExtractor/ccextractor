use crate::decode_vbi::exit_codes::*;
use crate::decode_vbi::structs_xds::*;

//----------------------vbi_structs-start-------------------------

#[derive(Debug)]
#[repr(C)]
pub struct Vbi3RawDecoder {
    sampling: VbiSamplingPar,
    services: VbiServiceSet,
    log: VbiLogHook,
    debug: bool,
    n_jobs: i64,
    n_sp_lines: i64,
    readjust: i64,
    pattern: Vec<i8>,             // n scan lines * MAX_WAYS
    jobs: [Vbi3RawDecoderJob; 8], // VBI3_RAW_DECODER_MAX_JOBS = 8
    sp_lines: Vec<Vbi3RawDecoderSpLine>,
}

pub type VbiServiceSet = i64;

pub type VbiBool = i64;

#[derive(Debug)]
#[repr(C)]
pub struct Vbi3RawDecoderJob {
    id: VbiServiceSet,
    slicer: Vbi3BitSlicer,
}

#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub enum Vbi3BitSlicerBit {
    CriBit = 1,
    FrcBit,
    PayloadBit,
}

#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub struct Vbi3BitSlicerPoint {
    /** Whether this struct refers to a CRI, FRC or payload bit. */
    kind: Vbi3BitSlicerBit,

    /** Number of the sample times 256. */
    index: i64,

    /** Signal amplitude at this sample, in range 0 to 65535. */
    level: i64,

    /** 0/1 threshold at this sample, in range 0 to 65535. */
    thresh: i64,
}

pub type Vbi3BitSlicerFn = fn(
    bs: &mut Vbi3BitSlicer,
    buffer: &mut [u8],
    points: &mut [Vbi3BitSlicerPoint],
    n_points: &mut i64,
    raw: &[u8],
) -> VbiBool;

#[derive(Debug)]
#[repr(C)]
pub struct Vbi3BitSlicer {
    func: Option<Vbi3BitSlicerFn>,
    sample_format: VbiPixfmt,
    cri: i64,
    cri_mask: i64,
    thresh: i64,
    thresh_frac: i64,
    cri_samples: i64,
    cri_rate: i64,
    oversampling_rate: i64,
    phase_shift: i64,
    step: i64,
    frc: i64,
    frc_bits: i64,
    total_bits: i64,
    payload: i64,
    endian: i64,
    bytes_per_sample: i64,
    skip: i64,
    green_mask: i64,
    log: VbiLogHook,
}

#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub enum VbiLogMask {
    /** External error causes, for example lack of memory. */
    Error = 1 << 3,

    /**
     * Invalid parameters and similar problems which suggest
     * a bug in the application using the library.
     */
    Warning = 1 << 4,

    /**
     * Causes of possibly undesired results, for example when a
     * data service cannot be decoded with the current video
     * standard setting.
     */
    Notice = 1 << 5,

    /** Progress messages. */
    Info = 1 << 6,

    /** Information useful to debug the library. */
    Debug = 1 << 7,

    /** Driver responses (strace). Not implemented yet. */
    Driver = 1 << 8,

    /** More detailed debugging information. */
    Debug2 = 1 << 9,
    Debug3 = 1 << 10,
}

#[derive(Debug, Clone, Copy)]
#[repr(C)]
pub struct Vbi3RawDecoderSpLine {
    points: [Vbi3BitSlicerPoint; 512],
    n_points: i64,
}

pub type VbiLogFn =
    fn(level: VbiLogMask, context: &str, message: &str, user_data: &mut dyn std::any::Any);

#[derive(Debug)]
#[repr(C)]
pub struct VbiLogHook {
    func: Option<VbiLogFn>,
    user_data: Option<Box<dyn std::any::Any>>,
    mask: VbiLogMask,
}

pub type VbiSamplingPar = VbiRawDecoder;

#[repr(C)]
pub struct CcxDecoderVbiCfg {
    pub debug_file_name: *mut c_char,
}

//-----------defaults-vbi-satart--------------
impl Default for Vbi3RawDecoderSpLine {
    fn default() -> Self {
        Self {
            points: [Vbi3BitSlicerPoint::default(); 512],
            n_points: 0,
        }
    }
}

impl Default for Vbi3BitSlicerPoint {
    fn default() -> Self {
        Self {
            kind: Vbi3BitSlicerBit::CriBit,
            index: 0,
            level: 0,
            thresh: 0,
        }
    }
}

impl Default for Vbi3BitSlicerBit {
    fn default() -> Self {
        Vbi3BitSlicerBit::CriBit
    }
}

impl Default for Vbi3RawDecoderJob {
    fn default() -> Self {
        Self {
            id: 0,
            slicer: Vbi3BitSlicer::default(),
        }
    }
}

impl Default for Vbi3BitSlicer {
    fn default() -> Self {
        Self {
            func: None,
            sample_format: VbiPixfmt::Yuv420,
            cri: 0,
            cri_mask: 0,
            thresh: 0,
            thresh_frac: 0,
            cri_samples: 0,
            cri_rate: 0,
            oversampling_rate: 0,
            phase_shift: 0,
            step: 0,
            frc: 0,
            frc_bits: 0,
            total_bits: 0,
            payload: 0,
            endian: 0,
            bytes_per_sample: 0,
            skip: 0,
            green_mask: 0,
            log: VbiLogHook::default(),
        }
    }
}

impl Default for VbiLogHook {
    fn default() -> Self {
        Self {
            func: None,
            user_data: None,
            mask: VbiLogMask::Error,
        }
    }
}

impl Default for VbiLogMask {
    fn default() -> Self {
        VbiLogMask::Error
    }
}

impl Default for VbiRawDecoderJob {
    fn default() -> Self {
        Self {
            id: 0,
            offset: 0,
            slicer: VbiBitSlicer::default(),
        }
    }
}

// impl Default for Vbi3RawDecoderJob {
//     fn default() -> Self {
//         Self {
//             id: 0,
//             slicer: Vbi3BitSlicer::default(),
//         }
//     }
// }

impl Default for VbiBitSlicer {
    fn default() -> Self {
        Self {
            func: None,
            cri: 0,
            cri_mask: 0,
            thresh: 0,
            cri_bytes: 0,
            cri_rate: 0,
            oversampling_rate: 0,
            phase_shift: 0,
            step: 0,
            frc: 0,
            frc_bits: 0,
            payload: 0,
            endian: 0,
            skip: 0,
        }
    }
}

//----------------defaults-vbi-end-------------------

//-------------------vbi-structs-end-----------------------------
