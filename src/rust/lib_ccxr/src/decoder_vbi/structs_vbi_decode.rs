use std::os::raw::{c_char, c_int, c_uchar, c_void};

use crate::decoder_vbi::structs_isdb::*;

pub struct CcxDecoderVbiCfg {
    // #ifdef VBI_DEBUG
    pub debug_file_name: *mut c_char, // Pointer to a debug file name
                                      // #endif
}

pub type VbiServiceSet = u32;

#[derive(Debug, Copy, Clone)]
pub struct VbiLogHook {
    pub fn_ptr: Option<VbiLogFn>,
    pub user_data: *mut c_void,
    pub mask: VbiLogMask,
}

pub type VbiLogFn = unsafe extern "C" fn(
    level: VbiLogMask,
    context: *const c_uchar,
    message: *const c_uchar,
    user_data: *mut c_void,
);

impl Default for VbiLogHook {
    fn default() -> Self {
        Self {
            fn_ptr: None,
            user_data: std::ptr::null_mut(),
            mask: 0,
        }
    }
}

pub type VbiLogMask = u32;

pub type VbiBool = c_int;

pub type VbiSamplingPar = VbiRawDecoder;

impl Default for VbiRawDecoder {
    fn default() -> Self {
        Self {
            scanning: 0,
            sampling_format: VbiPixfmt::Yuv420,
            sampling_rate: 0,
            bytes_per_line: 0,
            offset: 0,
            start: [0; 2],
            count: [0; 2],
            interlaced: 0,
            synchronous: 0,
            services: 0,
            num_jobs: 0,
            pattern: std::ptr::null_mut(),
            jobs: [VbiRawDecoderJob::default(); 8],
        }
    }
}

type VbiVideostdSet = u64;

pub struct VbiServicePar {
    pub id: VbiServiceSet,
    pub label: *const c_char,

    /// Video standard
    /// - 525 lines, FV = 59.94 Hz, FH = 15734 Hz
    /// - 625 lines, FV = 50 Hz, FH = 15625 Hz
    pub videostd_set: VbiVideostdSet,

    /// Most scan lines used by the data service, first and last
    /// line of first and second field. ITU-R numbering scheme.
    /// Zero if no data from this field, requires field sync.
    pub first: [u32; 2],
    pub last: [u32; 2],

    /// Leading edge hsync to leading edge first CRI one bit,
    /// half amplitude points, in nanoseconds.
    pub offset: u32,

    pub cri_rate: u32, // Hz
    pub bit_rate: u32, // Hz

    /// Clock Run In and FRaming Code, LSB last transmitted bit of FRC.
    pub cri_frc: u32,

    /// CRI and FRC bits significant for identification.
    pub cri_frc_mask: u32,

    /// Number of significant cri_bits (at cri_rate),
    /// frc_bits (at bit_rate).
    pub cri_bits: u32,
    pub frc_bits: u32,

    pub payload: u32, // bits
    pub modulation: VbiModulation,

    pub flags: VbiServiceParFlag,
}

#[derive(Debug, Copy, Clone, PartialEq, Eq)]
#[repr(u32)]
pub enum VbiModulation {
    /// The data is 'non-return to zero' coded, logical '1' bits
    /// are described by high sample values, logical '0' bits by
    /// low values. The data is last significant bit first transmitted.
    NrzLsb = 0,

    /// 'Non-return to zero' coded, most significant bit first
    /// transmitted.
    NrzMsb = 1,

    /// The data is 'bi-phase' coded. Each data bit is described
    /// by two complementary signaling elements, a logical '1'
    /// by a sequence of '10' elements, a logical '0' by a '01'
    /// sequence. The data is last significant bit first transmitted.
    BiphaseLsb = 2,

    /// 'Bi-phase' coded, most significant bit first transmitted.
    BiphaseMsb = 3,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub enum VbiServiceParFlag {
    LineNum = 1 << 0,
    FieldNum = 1 << 1,
    LineAndField = (1 << 0) | (1 << 1), // Bitwise OR of LineNum and FieldNum
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct VbiSliced {
    /// A VBI_SLICED_ symbol identifying the data service.
    /// Under certain circumstances (see VBI_SLICED_TELETEXT_B)
    /// this can be a set of VBI_SLICED_ symbols.
    pub id: u32,

    /// Source line number according to the ITU-R line numbering scheme,
    /// a value of 0 if the exact line number is unknown. Note that some
    /// data services cannot be reliably decoded without line number.
    ///
    /// ITU-R PAL/SECAM line numbering scheme and ITU-R NTSC line numbering
    /// scheme are illustrated in the original documentation.
    pub line: u32,

    /// The actual payload. See the documentation of VBI_SLICED_ symbols
    /// for details.
    pub data: [u8; 56],
}
