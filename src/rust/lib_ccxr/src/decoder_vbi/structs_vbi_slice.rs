use crate::decoder_vbi::exit_codes::*;
use crate::decoder_vbi::structs_isdb::*;
use crate::decoder_vbi::structs_vbi_decode::*;

use std::os::raw::{c_int, c_uchar};
use std::ptr::null_mut;

pub struct Vbi3RawDecoder {
    pub sampling: VbiSamplingPar,
    pub services: VbiServiceSet,
    pub log: VbiLogHook,
    pub debug: VbiBool,
    pub n_jobs: u32,
    pub n_sp_lines: u32,
    pub readjust: c_int,
    pub pattern: *mut i8,
    pub jobs: [_Vbi3RawDecoderJob; _VBI3_RAW_DECODER_MAX_JOBS],
    pub sp_lines: *mut _Vbi3RawDecoderSpLine,
}

impl Default for Vbi3RawDecoder {
    fn default() -> Self {
        Self {
            sampling: VbiRawDecoder::default(),
            services: 0,
            log: VbiLogHook::default(),
            debug: 0,
            n_jobs: 0,
            n_sp_lines: 0,
            readjust: 0,
            pattern: std::ptr::null_mut(),
            jobs: [_Vbi3RawDecoderJob::default(); _VBI3_RAW_DECODER_MAX_JOBS],
            sp_lines: std::ptr::null_mut(),
        }
    }
}

#[derive(Debug, Copy, Clone, Default)]
pub struct _Vbi3RawDecoderJob {
    pub id: VbiServiceSet,
    pub slicer: Vbi3BitSlicer,
}

#[derive(Debug, Clone)]
pub struct _Vbi3RawDecoderSpLine {
    pub points: [Vbi3BitSlicerPoint; 512],
    pub n_points: u32,
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
            log: VbiLogHook {
                fn_ptr: None,
                user_data: null_mut(),
                mask: 0,
            },
        }
    }
}

#[derive(Debug, Copy, Clone)]
pub struct Vbi3BitSlicer {
    pub func: Option<Vbi3BitSlicerFn>,
    pub sample_format: VbiPixfmt,
    pub cri: u32,
    pub cri_mask: u32,
    pub thresh: u32,
    pub thresh_frac: u32,
    pub cri_samples: u32,
    pub cri_rate: u32,
    pub oversampling_rate: u32,
    pub phase_shift: u32,
    pub step: u32,
    pub frc: u32,
    pub frc_bits: u32,
    pub total_bits: u32,
    pub payload: u32,
    pub endian: u32,
    pub bytes_per_sample: u32,
    pub skip: u32,
    pub green_mask: u32,
    pub log: VbiLogHook,
}

pub type Vbi3BitSlicerFn = unsafe extern "C" fn(
    bs: *mut Vbi3BitSlicer,
    buffer: *mut c_uchar,
    points: *mut Vbi3BitSlicerPoint,
    n_points: *mut u32,
    raw: *const c_uchar,
) -> VbiBool;

#[derive(Debug, Clone, Default)]
pub struct Vbi3BitSlicerPoint {
    pub kind: Vbi3BitSlicerBit,
    pub index: u32,
    pub level: u32,
    pub thresh: u32,
}

#[derive(Debug, Clone, Default)]
pub enum Vbi3BitSlicerBit {
    #[default]
    CriBit = 1,
    FrcBit,
    PayloadBit,
}
