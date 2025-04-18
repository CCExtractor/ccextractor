use crate::decoder_vbi::exit_codes::*;
use crate::decoder_vbi::functions_vbi_decode::*;
use crate::decoder_vbi::structs_isdb::*;
use crate::decoder_vbi::structs_vbi_decode::*;
use crate::decoder_vbi::structs_vbi_slice::*;
use crate::decoder_vbi::vbi_service_table::*;

use crate::util::log::error;

use std::alloc::{alloc_zeroed, dealloc, Layout};
use std::ptr::{self, null_mut};

pub unsafe fn vbi3_raw_decoder_delete(rd: *mut Vbi3RawDecoder) {
    if rd.is_null() {
        return;
    }
    vbi3_raw_decoder_destroy(&mut *rd);
}

pub fn vbi3_raw_decoder_debug(rd: &mut Vbi3RawDecoder, enable: bool) -> bool {
    assert!(!rd.pattern.is_null(), "Decoder cannot be null");

    let mut _sp_lines: *mut _Vbi3RawDecoderSpLine = null_mut(); // _sp_lines not used
    let mut n_lines: u32 = 0;
    let mut result = true;

    rd.debug = enable as i32;

    if enable {
        n_lines = (rd.sampling.count[0] + rd.sampling.count[1]) as u32;
    }

    match rd.sampling.sampling_format {
        VbiPixfmt::Yuv420 => {}
        _ => {
            // Not implemented
            n_lines = 0;
            result = false;
        }
    }

    if rd.n_sp_lines == n_lines {
        return result;
    }

    // Free existing lines
    if !rd.sp_lines.is_null() {
        unsafe {
            let layout = Layout::array::<_Vbi3RawDecoderSpLine>(rd.n_sp_lines as usize).unwrap();
            dealloc(rd.sp_lines as *mut u8, layout);
        }
        rd.sp_lines = null_mut();
    }
    rd.n_sp_lines = 0;

    if n_lines > 0 {
        unsafe {
            let layout = Layout::array::<_Vbi3RawDecoderSpLine>(n_lines as usize).unwrap();
            rd.sp_lines = alloc_zeroed(layout) as *mut _Vbi3RawDecoderSpLine;
            if rd.sp_lines.is_null() {
                return false;
            }
        }
        rd.n_sp_lines = n_lines;
    }

    result
}

use std::mem::MaybeUninit;

pub fn vbi3_raw_decoder_destroy(rd: &mut Vbi3RawDecoder) {
    vbi3_raw_decoder_reset(rd);
    vbi3_raw_decoder_debug(rd, false);

    // Make unusable
    *rd = unsafe { MaybeUninit::zeroed().assume_init() };
}

pub fn vbi3_raw_decoder_reset(rd: &mut Vbi3RawDecoder) {
    assert!(!rd.pattern.is_null(), "Decoder cannot be null");

    unsafe {
        if !rd.pattern.is_null() {
            dealloc(rd.pattern as *mut u8, Layout::new::<u8>());
            rd.pattern = null_mut();
        }
    }

    rd.services = 0;
    rd.n_jobs = 0;
    rd.readjust = 1;

    rd.jobs.fill(Default::default());
}

pub fn vbi3_raw_decoder_new(sp: Option<&VbiSamplingPar>) -> Option<Box<Vbi3RawDecoder>> {
    let mut rd = Box::new(Vbi3RawDecoder {
        sampling: Default::default(),
        services: 0,
        log: Default::default(),
        debug: 0,
        n_jobs: 0,
        n_sp_lines: 0,
        readjust: 0,
        pattern: std::ptr::null_mut(),
        jobs: [Default::default(); _VBI3_RAW_DECODER_MAX_JOBS],
        sp_lines: std::ptr::null_mut(),
    });

    if !_vbi3_raw_decoder_init(&mut rd, sp) {
        return None;
    }

    Some(rd)
}

pub fn _vbi3_raw_decoder_init(rd: &mut Vbi3RawDecoder, sp: Option<&VbiSamplingPar>) -> bool {
    vbi3_raw_decoder_reset(rd);

    if let Some(sp) = sp {
        if _vbi_sampling_par_valid_log(sp, &rd.log) == 0 {
            return false;
        }
        rd.sampling = *sp;
    }

    true
}

pub fn vbi3_raw_decoder_set_sampling_par(
    rd: &mut Vbi3RawDecoder,
    sp: &VbiSamplingPar,
    strict: i32,
) -> u32 {
    assert!(!rd.pattern.is_null());
    assert!(!sp.pattern.is_null());

    let services = rd.services;

    vbi3_raw_decoder_reset(rd);

    if _vbi_sampling_par_valid_log(sp, &rd.log) == 0 {
        rd.sampling = Default::default();
        return 0;
    }

    rd.sampling = *sp;

    vbi3_raw_decoder_debug(rd, rd.debug != 0);

    vbi3_raw_decoder_add_services(rd, services, strict)
}

pub fn _vbi3_bit_slicer_init(bs: &mut Vbi3BitSlicer) -> bool {
    assert!(!ptr::eq(bs, null_mut()));
    *bs = Vbi3BitSlicer::default();
    bs.func = Some(null_function);
    true
}

/// Helper function to get the green component based on pixel format.
pub fn get_green(raw: &[u8], bs: &Vbi3BitSlicer, pixfmt: VbiPixfmt) -> u32 {
    match pixfmt {
        VbiPixfmt::Yuv420 => raw[0] as u32,
        VbiPixfmt::Rgb24 | VbiPixfmt::Bgr24 => {
            // For RGB24, green is in different positions depending on format
            raw[1] as u32
        }
        VbiPixfmt::Rgba32Le | VbiPixfmt::Bgra32Le => {
            // For RGBA32, green is byte 1 (RGBA) or 1 (BGRA)
            raw[1] as u32
        }
        VbiPixfmt::Rgb16Le
        | VbiPixfmt::Bgr16Le
        | VbiPixfmt::Rgba15Le
        | VbiPixfmt::Bgra15Le
        | VbiPixfmt::Argb15Le
        | VbiPixfmt::Abgr15Le => {
            let word = u16::from_le_bytes([raw[0], raw[1]]);
            ((word & bs.green_mask as u16) >> bs.green_mask.trailing_zeros() as u16) as u32
        }
        VbiPixfmt::Rgb16Be
        | VbiPixfmt::Bgr16Be
        | VbiPixfmt::Rgba15Be
        | VbiPixfmt::Bgra15Be
        | VbiPixfmt::Argb15Be
        | VbiPixfmt::Abgr15Be => {
            let word = u16::from_be_bytes([raw[0], raw[1]]);
            ((word & bs.green_mask as u16) >> bs.green_mask.trailing_zeros() as u16) as u32
        }
        _ => raw[0] as u32, // Default case for YUV formats
    }
}

#[allow(clippy::too_many_arguments)]
pub fn sample<'a>(
    raw: &'a [u8],
    i: u32,
    bpp: usize,
    bs: &Vbi3BitSlicer,
    pixfmt: VbiPixfmt,
    kind: Vbi3BitSlicerBit,
    collect_points: bool,
    points: &mut Option<&mut [Vbi3BitSlicerPoint]>,
    raw_start: usize,
    tr: u32,
) -> (u32, &'a [u8]) {
    let r = &raw[(i >> 8) as usize * bpp..];
    let raw0 = get_green(r, bs, pixfmt);
    let raw1 = get_green(&r[bpp..], bs, pixfmt);
    let raw0 = ((raw1 as i32 - raw0 as i32) * (i & 255) as i32 + (raw0 << 8) as i32) >> 8;

    if collect_points {
        if let Some(points) = points {
            if let Some(first) = points.iter_mut().next() {
                *first = Vbi3BitSlicerPoint {
                    kind,
                    index: ((raw.as_ptr() as usize - raw_start) * 256 + i as usize) as u32,
                    level: raw0 as u32,
                    thresh: tr,
                };
            }
        }
    }

    (raw0 as u32, r)
}

#[allow(clippy::too_many_arguments)]
pub fn payload(
    bs: &Vbi3BitSlicer,
    buffer: &mut [u8],
    points: &mut Option<&mut [Vbi3BitSlicerPoint]>,
    raw: &[u8],
    raw_start: usize,
    pixfmt: VbiPixfmt,
    bpp: usize,
    tr: u32,
    collect_points: bool,
) -> bool {
    let mut i = bs.phase_shift;
    let tr = tr * 256;
    let mut c = 0;

    // Process FRC bits
    for _j in 0..bs.frc_bits {
        // _j is unused
        let (raw0, _) = sample(
            raw,
            i,
            bpp,
            bs,
            pixfmt,
            Vbi3BitSlicerBit::FrcBit,
            collect_points,
            points,
            raw_start,
            tr,
        );
        c = c * 2 + (raw0 >= tr) as u32;
        i += bs.step;
    }

    if c != bs.frc {
        return false;
    }

    let buffer_len = buffer.len();
    let mut buffer_ptr = 0;

    match bs.endian {
        3 => {
            // Bitwise, LSB first
            for j in 0..bs.payload {
                let (raw0, _) = sample(
                    raw,
                    i,
                    bpp,
                    bs,
                    pixfmt,
                    Vbi3BitSlicerBit::PayloadBit,
                    collect_points,
                    points,
                    raw_start,
                    tr,
                );
                c = ((c >> 1) + ((raw0 >= tr) as u32)) << 7;
                i += bs.step;
                if (j & 7) == 7 && buffer_ptr < buffer_len {
                    buffer[buffer_ptr] = c as u8;
                    buffer_ptr += 1;
                    c = 0;
                }
            }
            if buffer_ptr < buffer_len {
                buffer[buffer_ptr] = (c >> ((8 - bs.payload) & 7)) as u8;
            }
        }
        2 => {
            // Bitwise, MSB first
            for j in 0..bs.payload {
                let (raw0, _) = sample(
                    raw,
                    i,
                    bpp,
                    bs,
                    pixfmt,
                    Vbi3BitSlicerBit::PayloadBit,
                    collect_points,
                    points,
                    raw_start,
                    tr,
                );
                c = c * 2 + (raw0 >= tr) as u32;
                i += bs.step;
                if (j & 7) == 7 && buffer_ptr < buffer_len {
                    buffer[buffer_ptr] = c as u8;
                    buffer_ptr += 1;
                    c = 0;
                }
            }
            if buffer_ptr < buffer_len {
                buffer[buffer_ptr] = (c & ((1 << (bs.payload & 7)) - 1)) as u8;
            }
        }
        1 => {
            // Octets, LSB first
            for _ in 0..(bs.payload / 8) {
                let mut byte = 0;
                for k in 0..8 {
                    let (raw0, _) = sample(
                        raw,
                        i,
                        bpp,
                        bs,
                        pixfmt,
                        Vbi3BitSlicerBit::PayloadBit,
                        collect_points,
                        points,
                        raw_start,
                        tr,
                    );
                    byte += ((raw0 >= tr) as u8) << k;
                    i += bs.step;
                }
                if buffer_ptr < buffer_len {
                    buffer[buffer_ptr] = byte;
                    buffer_ptr += 1;
                }
            }
        }
        _ => {
            // Octets, MSB first
            for _ in 0..(bs.payload / 8) {
                let mut byte = 0;
                for _k in 0..8 {
                    // _k is unused
                    let (raw0, _) = sample(
                        raw,
                        i,
                        bpp,
                        bs,
                        pixfmt,
                        Vbi3BitSlicerBit::PayloadBit,
                        collect_points,
                        points,
                        raw_start,
                        tr,
                    );
                    byte = byte * 2 + (raw0 >= tr) as u8;
                    i += bs.step;
                }
                if buffer_ptr < buffer_len {
                    buffer[buffer_ptr] = byte;
                    buffer_ptr += 1;
                }
            }
        }
    }

    true
}

#[allow(clippy::too_many_arguments)]
pub fn core(
    bs: &mut Vbi3BitSlicer,
    buffer: &mut [u8],
    mut points: Option<&mut [Vbi3BitSlicerPoint]>,
    n_points: Option<&mut u32>,
    raw: &[u8],
    pixfmt: VbiPixfmt,
    bpp: usize,
    oversampling: u32,
    thresh_frac: u32,
    collect_points: bool,
) -> bool {
    let thresh0 = bs.thresh;
    let raw_start = raw.as_ptr() as usize;
    let mut raw = &raw[bs.skip as usize..];
    let mut cl = 0;
    let mut c = 0;
    let mut b1 = 0;
    let mut points_index = 0; // Track the current index in the points slice

    for _ in 0..bs.cri_samples {
        let tr = bs.thresh >> thresh_frac;
        let raw0 = get_green(raw, bs, pixfmt);
        let raw1 = get_green(&raw[bpp..], bs, pixfmt);
        let raw1_diff = (raw1 as i32 - raw0 as i32).unsigned_abs();
        bs.thresh = (bs.thresh as i32 + (raw0 as i32 - tr as i32) * raw1_diff as i32) as u32;
        let mut t = raw0 * oversampling;

        for _ in 0..oversampling {
            let tavg = (t + oversampling / 2) / oversampling;
            let b = (tavg >= tr) as u8;

            if b ^ b1 != 0 {
                cl = bs.oversampling_rate >> 1;
            } else {
                cl += bs.cri_rate;

                if cl >= bs.oversampling_rate {
                    if collect_points {
                        if let Some(points_ref) = points.as_mut() {
                            if points_index < points_ref.len() {
                                points_ref[points_index] = Vbi3BitSlicerPoint {
                                    kind: Vbi3BitSlicerBit::CriBit,
                                    index: ((raw.as_ptr() as usize - raw_start) << 8) as u32,
                                    level: tavg << 8,
                                    thresh: tr << 8,
                                };
                                points_index += 1; // Move to the next point
                            }
                        }
                    }

                    cl -= bs.oversampling_rate;
                    c = c * 2 + b as u32;
                    if (c & bs.cri_mask) == bs.cri {
                        bs.thresh = thresh0;
                        return payload(
                            bs,
                            buffer,
                            &mut points,
                            raw,
                            raw_start,
                            pixfmt,
                            bpp,
                            tr,
                            collect_points,
                        );
                    }
                }
            }

            b1 = b;
            if oversampling > 1 {
                t += raw1;
            }
        }

        raw = &raw[bpp..];
    }

    bs.thresh = thresh0;
    if collect_points {
        if let (Some(_points_ref), Some(n_points_ref)) = (points, n_points) {
            // points_ref is unused
            *n_points_ref = points_index as u32; // Use the points_index to calculate the number of points used
        }
    }
    false
}

pub fn bit_slicer_y8(
    bs: &mut Vbi3BitSlicer,
    buffer: &mut [u8],
    points: Option<&mut [Vbi3BitSlicerPoint]>,
    n_points: Option<&mut u32>,
    raw: &[u8],
) -> bool {
    let pixfmt = VbiPixfmt::Yuv420; // Pixel format for YUV420
    let bpp = 1; // Bytes per pixel
    let oversampling = 4; // Oversampling rate
    let thresh_frac = DEF_THR_FRAC; // Threshold fraction
    let collect_points = points.is_some(); // Determine if points collection is enabled

    core(
        bs,
        buffer,
        points,
        n_points,
        raw,
        pixfmt,
        bpp,
        oversampling,
        thresh_frac,
        collect_points,
    )
}

pub fn bit_slicer_yuyv(
    bs: &mut Vbi3BitSlicer,
    buffer: &mut [u8],
    points: Option<&mut [Vbi3BitSlicerPoint]>,
    n_points: Option<&mut u32>,
    raw: &[u8],
) -> bool {
    let pixfmt = VbiPixfmt::Yuyv; // Pixel format for YUYV
    let bpp = 2; // Bytes per pixel
    let oversampling = 4; // Oversampling rate
    let thresh_frac = DEF_THR_FRAC; // Threshold fraction
    let collect_points = points.is_some(); // Determine if points collection is enabled

    core(
        bs,
        buffer,
        points,
        n_points,
        raw,
        pixfmt,
        bpp,
        oversampling,
        thresh_frac,
        collect_points,
    )
}

pub fn bit_slicer_rgb24_le(
    bs: &mut Vbi3BitSlicer,
    buffer: &mut [u8],
    points: Option<&mut [Vbi3BitSlicerPoint]>,
    n_points: Option<&mut u32>,
    raw: &[u8],
) -> bool {
    let pixfmt = VbiPixfmt::Rgb24; // Pixel format for RGB24
    let bpp = 3; // Bytes per pixel
    let oversampling = 4; // Oversampling rate
    let thresh_frac = DEF_THR_FRAC; // Threshold fraction
    let collect_points = points.is_some(); // Determine if points collection is enabled

    core(
        bs,
        buffer,
        points,
        n_points,
        raw,
        pixfmt,
        bpp,
        oversampling,
        thresh_frac,
        collect_points,
    )
}

pub fn bit_slicer_rgba24_le(
    bs: &mut Vbi3BitSlicer,
    buffer: &mut [u8],
    points: Option<&mut [Vbi3BitSlicerPoint]>,
    n_points: Option<&mut u32>,
    raw: &[u8],
) -> bool {
    let pixfmt = VbiPixfmt::Rgba32Le; // Pixel format for RGBA32 (Little Endian)
    let bpp = 4; // Bytes per pixel
    let oversampling = 4; // Oversampling rate
    let thresh_frac = DEF_THR_FRAC; // Threshold fraction
    let collect_points = points.is_some(); // Determine if points collection is enabled

    core(
        bs,
        buffer,
        points,
        n_points,
        raw,
        pixfmt,
        bpp,
        oversampling,
        thresh_frac,
        collect_points,
    )
}

pub fn bit_slicer_rgb16_le(
    bs: &mut Vbi3BitSlicer,
    buffer: &mut [u8],
    points: Option<&mut [Vbi3BitSlicerPoint]>,
    n_points: Option<&mut u32>,
    raw: &[u8],
) -> bool {
    let pixfmt = VbiPixfmt::Rgb16Le; // Pixel format for RGB16 (Little Endian)
    let bpp = 2; // Bytes per pixel
    let oversampling = 4; // Oversampling rate
    let thresh_frac = bs.thresh_frac; // Use the threshold fraction from the bit slicer
    let collect_points = points.is_some(); // Determine if points collection is enabled

    core(
        bs,
        buffer,
        points,
        n_points,
        raw,
        pixfmt,
        bpp,
        oversampling,
        thresh_frac,
        collect_points,
    )
}

pub fn bit_slicer_rgb16_be(
    bs: &mut Vbi3BitSlicer,
    buffer: &mut [u8],
    points: Option<&mut [Vbi3BitSlicerPoint]>,
    n_points: Option<&mut u32>,
    raw: &[u8],
) -> bool {
    let pixfmt = VbiPixfmt::Rgb16Be; // Pixel format for RGB16 (Big Endian)
    let bpp = 2; // Bytes per pixel
    let oversampling = 4; // Oversampling rate
    let thresh_frac = bs.thresh_frac; // Use the threshold fraction from the bit slicer
    let collect_points = points.is_some(); // if points collection is enabled

    core(
        bs,
        buffer,
        points,
        n_points,
        raw,
        pixfmt,
        bpp,
        oversampling,
        thresh_frac,
        collect_points,
    )
}

pub fn low_pass_bit_slicer_y8(
    bs: &mut Vbi3BitSlicer,
    buffer: &mut [u8],
    mut points: Option<&mut [Vbi3BitSlicerPoint]>,
    n_points: Option<&mut u32>,
    raw: &[u8],
) -> bool {
    let bpp = bs.bytes_per_sample as usize;
    let raw_start = raw.as_ptr() as usize;
    let mut raw = &raw[bs.skip as usize..];
    let thresh0 = bs.thresh;
    let mut c = !0u32;
    let mut cl = 0;
    let mut b1 = 0;
    let mut points_idx = 0;

    let mut raw0sum = raw[0] as u32;
    for m in bpp..(bpp << LP_AVG) {
        if m < raw.len() {
            raw0sum += raw[m] as u32;
        }
    }

    // Process CRI bits
    for i in (0..bs.cri_samples).rev() {
        let tr = bs.thresh >> bs.thresh_frac;
        let raw0 = raw0sum;

        if raw.len() > (bpp << LP_AVG) {
            raw0sum += raw[bpp << LP_AVG] as u32;
        }
        if !raw.is_empty() {
            raw0sum -= raw[0] as u32;
        }

        raw = &raw[bpp..];

        let diff = (raw0sum as i32 - raw0 as i32).abs();
        bs.thresh = (bs.thresh as i32 + (raw0 as i32 - tr as i32) * diff) as u32;

        let b = (raw0 >= tr) as u8;

        if b ^ b1 != 0 {
            cl = bs.oversampling_rate >> 1;
        } else {
            cl += bs.cri_rate;

            if cl >= bs.oversampling_rate {
                if let Some(points_ref) = &mut points {
                    if points_idx < points_ref.len() {
                        points_ref[points_idx] = Vbi3BitSlicerPoint {
                            kind: Vbi3BitSlicerBit::CriBit,
                            index: ((raw.as_ptr() as usize - raw_start) * 256 / bpp
                                + (1 << LP_AVG) * 128) as u32,
                            level: raw0 << (8 - LP_AVG),
                            thresh: tr << (8 - LP_AVG),
                        };
                        points_idx += 1;
                    }
                }

                cl -= bs.oversampling_rate;
                c = c * 2 + b as u32;

                if (c & bs.cri_mask) == bs.cri {
                    break;
                }
            }
        }

        b1 = b;

        if i == 0 {
            bs.thresh = thresh0;
            if let Some(n_points_ref) = n_points {
                *n_points_ref = points_idx as u32;
            }
            return false;
        }
    }

    // Process FRC and payload bits
    let mut i = bs.phase_shift;
    let tr = bs.thresh >> bs.thresh_frac;
    let mut c = 0;

    for _j in 0..bs.frc_bits {
        // _j is unused
        let ii = (i >> 8) * bpp as u32;
        let mut raw0 = if (ii as usize) < raw.len() {
            raw[ii as usize] as u32
        } else {
            0
        };

        for m in bpp..(bpp << LP_AVG) {
            if (ii as usize + m) < raw.len() {
                raw0 += raw[ii as usize + m] as u32;
            }
        }

        if let Some(points_ref) = &mut points {
            if points_idx < points_ref.len() {
                points_ref[points_idx] = Vbi3BitSlicerPoint {
                    kind: Vbi3BitSlicerBit::FrcBit,
                    index: ((raw.as_ptr() as usize - raw_start) * 256 / bpp
                        + ((1 << LP_AVG) * 128) as usize
                        + (ii as usize * 256)) as u32,
                    level: raw0 << (8 - LP_AVG),
                    thresh: tr << (8 - LP_AVG),
                };
                points_idx += 1;
            }
        }

        c = c * 2 + (raw0 >= tr) as u32;
        i += bs.step;
    }

    if c != bs.frc {
        return false;
    }

    let mut buffer_ptr = 0;
    c = 0;

    match bs.endian {
        // Bitwise, LSB first
        3 => {
            for j in 0..bs.payload {
                let ii = (i >> 8) * bpp as u32;
                let mut raw0 = if (ii as usize) < raw.len() {
                    raw[ii as usize] as u32
                } else {
                    0
                };

                for m in bpp..(bpp << LP_AVG) {
                    if (ii as usize + m) < raw.len() {
                        raw0 += raw[ii as usize + m] as u32;
                    }
                }

                if let Some(points_ref) = &mut points {
                    if points_idx < points_ref.len() {
                        points_ref[points_idx] = Vbi3BitSlicerPoint {
                            kind: Vbi3BitSlicerBit::PayloadBit,
                            index: ((raw.as_ptr() as usize - raw_start) * 256 / bpp
                                + (1 << LP_AVG) as usize * 128
                                + (ii as usize) * 256) as u32,
                            level: raw0 << (8 - LP_AVG),
                            thresh: tr << (8 - LP_AVG),
                        };
                        points_idx += 1;
                    }
                }

                c = ((c >> 1) + ((raw0 >= tr) as u32)) << 7;
                i += bs.step;

                if (j & 7) == 7 && buffer_ptr < buffer.len() {
                    buffer[buffer_ptr] = c as u8;
                    buffer_ptr += 1;
                    c = 0;
                }
            }

            if buffer_ptr < buffer.len() {
                buffer[buffer_ptr] = (c >> ((8 - bs.payload) & 7)) as u8;
            }
        }

        // Bitwise, MSB first
        2 => {
            for j in 0..bs.payload {
                let ii = (i >> 8) * bpp as u32;
                let mut raw0 = if (ii as usize) < raw.len() {
                    raw[ii as usize] as u32
                } else {
                    0
                };

                for m in bpp..(bpp << LP_AVG) {
                    if (ii as usize + m) < raw.len() {
                        raw0 += raw[ii as usize + m] as u32;
                    }
                }

                if let Some(points_ref) = &mut points {
                    if points_idx < points_ref.len() {
                        points_ref[points_idx] = Vbi3BitSlicerPoint {
                            kind: Vbi3BitSlicerBit::PayloadBit,
                            index: ((raw.as_ptr() as usize - raw_start) * 256 / bpp
                                + (1 << LP_AVG) * 128
                                + (ii as usize) * 256) as u32,
                            level: raw0 << (8 - LP_AVG),
                            thresh: tr << (8 - LP_AVG),
                        };
                        points_idx += 1;
                    }
                }

                c = c * 2 + (raw0 >= tr) as u32;
                i += bs.step;

                if (j & 7) == 7 && buffer_ptr < buffer.len() {
                    buffer[buffer_ptr] = c as u8;
                    buffer_ptr += 1;
                    c = 0;
                }
            }

            if buffer_ptr < buffer.len() {
                buffer[buffer_ptr] = (c & ((1 << (bs.payload & 7)) - 1)) as u8;
            }
        }

        // Octets, LSB first
        1 => {
            for _j in 0..(bs.payload / 8) {
                // _j is unused
                let mut byte = 0;
                for k in 0..8 {
                    let ii = (i >> 8) * bpp as u32;
                    let mut raw0 = if (ii as usize) < raw.len() {
                        raw[ii as usize] as u32
                    } else {
                        0
                    };

                    for m in bpp..(bpp << LP_AVG) {
                        if (ii as usize + m) < raw.len() {
                            raw0 += raw[ii as usize + m] as u32;
                        }
                    }

                    if let Some(points_ref) = &mut points {
                        if points_idx < points_ref.len() {
                            points_ref[points_idx] = Vbi3BitSlicerPoint {
                                kind: Vbi3BitSlicerBit::PayloadBit,
                                index: (((raw.as_ptr() as usize - raw_start) * 256 / bpp) as u32)
                                    + ((1 << LP_AVG) * 128)
                                    + (ii * 256),
                                level: raw0 << (8 - LP_AVG),
                                thresh: tr << (8 - LP_AVG),
                            };
                            points_idx += 1;
                        }
                    }

                    byte += ((raw0 >= tr) as u8) << k;
                    i += bs.step;
                }

                if buffer_ptr < buffer.len() {
                    buffer[buffer_ptr] = byte;
                    buffer_ptr += 1;
                }
            }
        }

        // Octets, MSB first (default)
        _ => {
            for _j in 0..(bs.payload / 8) {
                // _j is unused
                let mut byte = 0;
                for _k in 0..8 {
                    // _k is unused
                    let ii = (i >> 8) * bpp as u32;
                    let mut raw0 = if (ii as usize) < raw.len() {
                        raw[ii as usize] as u32
                    } else {
                        0
                    };

                    for m in bpp..(bpp << LP_AVG) {
                        if (ii as usize + m) < raw.len() {
                            raw0 += raw[ii as usize + m] as u32;
                        }
                    }

                    if let Some(points_ref) = &mut points {
                        if points_idx < points_ref.len() {
                            points_ref[points_idx] = Vbi3BitSlicerPoint {
                                kind: Vbi3BitSlicerBit::PayloadBit,
                                index: ((raw.as_ptr() as usize - raw_start) * 256 / bpp
                                    + ((1 << LP_AVG) * 128) as usize
                                    + (ii as usize * 256))
                                    as u32,
                                level: raw0 << (8 - LP_AVG),
                                thresh: tr << (8 - LP_AVG),
                            };
                            points_idx += 1;
                        }
                    }

                    byte = byte * 2 + (raw0 >= tr) as u8;
                    i += bs.step;
                }

                if buffer_ptr < buffer.len() {
                    buffer[buffer_ptr] = byte;
                    buffer_ptr += 1;
                }
            }
        }
    }

    // Update points count if collecting
    if let Some(n_points_ref) = n_points {
        *n_points_ref = points_idx as u32;
    }

    true
}

#[allow(clippy::too_many_arguments)]
pub fn vbi3_bit_slicer_set_params(
    bs: &mut Vbi3BitSlicer,
    sample_format: VbiPixfmt,
    sampling_rate: u32,
    sample_offset: u32,
    samples_per_line: u32,
    cri: u32,
    cri_mask: u32,
    cri_bits: u32,
    cri_rate: u32,
    cri_end: u32,
    frc: u32,
    frc_bits: u32,
    payload_bits: u32,
    payload_rate: u32,
    modulation: VbiModulation,
) -> bool {
    assert!(cri_bits <= 32);
    assert!(frc_bits <= 32);
    assert!(payload_bits <= 32767);
    assert!(samples_per_line <= 32767);

    if cri_rate > sampling_rate {
        error!("cri_rate {} > sampling_rate {}.", cri_rate, sampling_rate);
        return set_null_function(bs);
    }

    if payload_rate > sampling_rate {
        error!(
            "payload_rate {} > sampling_rate {}.",
            payload_rate, sampling_rate
        );
        return set_null_function(bs);
    }

    let min_samples_per_bit = sampling_rate / cri_rate.max(payload_rate);
    bs.sample_format = sample_format;

    let c_mask = if cri_bits == 32 {
        !0u32
    } else {
        (1u32 << cri_bits) - 1
    };
    let f_mask = if frc_bits == 32 {
        !0u32
    } else {
        (1u32 << frc_bits) - 1
    };

    // Set bytes_per_sample according to pixel format
    bs.bytes_per_sample = match sample_format {
        VbiPixfmt::Yuv420 => 1,
        VbiPixfmt::Yuyv | VbiPixfmt::Yvyu | VbiPixfmt::Uyvy | VbiPixfmt::Vyuy => 2,
        VbiPixfmt::Rgb24 | VbiPixfmt::Bgr24 => 3,
        VbiPixfmt::Rgba32Le | VbiPixfmt::Rgba32Be | VbiPixfmt::Bgra32Le | VbiPixfmt::Bgra32Be => 4,
        VbiPixfmt::Rgb16Le | VbiPixfmt::Rgb16Be | VbiPixfmt::Bgr16Le | VbiPixfmt::Bgr16Be => 2,
        VbiPixfmt::Rgba15Le | VbiPixfmt::Rgba15Be | VbiPixfmt::Bgra15Le | VbiPixfmt::Bgra15Be => 2,
        VbiPixfmt::Argb15Le | VbiPixfmt::Argb15Be | VbiPixfmt::Abgr15Le | VbiPixfmt::Abgr15Be => 2,
        _ => {
            error!("Unknown sample_format {:?}.", sample_format);
            return set_null_function(bs);
        }
    };

    let (oversampling, skip, _func) = match sample_format {
        // unused _func
        VbiPixfmt::Yuv420 => {
            let func = if min_samples_per_bit > (3 << (LP_AVG - 1)) {
                low_pass_bit_slicer_y8
            } else {
                bit_slicer_y8
            };
            (
                if min_samples_per_bit > (3 << (LP_AVG - 1)) {
                    1
                } else {
                    4
                },
                0,
                func,
            )
        }
        VbiPixfmt::Yuyv | VbiPixfmt::Yvyu => {
            let func = if min_samples_per_bit > (3 << (LP_AVG - 1)) {
                low_pass_bit_slicer_y8
            } else {
                bit_slicer_yuyv
            };
            (
                if min_samples_per_bit > (3 << (LP_AVG - 1)) {
                    1
                } else {
                    4
                },
                0,
                func,
            )
        }
        VbiPixfmt::Uyvy | VbiPixfmt::Vyuy => {
            let func = if min_samples_per_bit > (3 << (LP_AVG - 1)) {
                low_pass_bit_slicer_y8
            } else {
                bit_slicer_yuyv
            };
            (
                if min_samples_per_bit > (3 << (LP_AVG - 1)) {
                    1
                } else {
                    4
                },
                1,
                func,
            )
        }
        VbiPixfmt::Rgba32Le | VbiPixfmt::Bgra32Le => {
            let func = if min_samples_per_bit > (3 << (LP_AVG - 1)) {
                low_pass_bit_slicer_y8
            } else {
                bit_slicer_rgba24_le
            };
            (
                if min_samples_per_bit > (3 << (LP_AVG - 1)) {
                    1
                } else {
                    4
                },
                1,
                func,
            )
        }
        VbiPixfmt::Rgba32Be | VbiPixfmt::Bgra32Be => {
            let func = if min_samples_per_bit > (3 << (LP_AVG - 1)) {
                low_pass_bit_slicer_y8
            } else {
                bit_slicer_rgba24_le
            };
            (
                if min_samples_per_bit > (3 << (LP_AVG - 1)) {
                    1
                } else {
                    4
                },
                2,
                func,
            )
        }
        VbiPixfmt::Rgb24 | VbiPixfmt::Bgr24 => {
            let func = if min_samples_per_bit > (3 << (LP_AVG - 1)) {
                low_pass_bit_slicer_y8
            } else {
                bit_slicer_rgb24_le
            };
            (
                if min_samples_per_bit > (3 << (LP_AVG - 1)) {
                    1
                } else {
                    4
                },
                1,
                func,
            )
        }
        VbiPixfmt::Rgb16Le | VbiPixfmt::Bgr16Le => {
            bs.green_mask = 0x07E0;
            bs.thresh = 105 << (5 - 2 + 12);
            bs.thresh_frac = 12;
            (
                4,
                0,
                bit_slicer_rgb16_le
                    as for<'a, 'b, 'c, 'd, 'e> fn(
                        &'a mut Vbi3BitSlicer,
                        &'b mut _,
                        Option<&'c mut _>,
                        Option<&'d mut _>,
                        &'e _,
                    ) -> _,
            )
        }
        VbiPixfmt::Rgb16Be | VbiPixfmt::Bgr16Be => {
            bs.green_mask = 0x07E0;
            bs.thresh = 105 << (5 - 2 + 12);
            bs.thresh_frac = 12;
            (
                4,
                0,
                bit_slicer_rgb16_be
                    as for<'a, 'b, 'c, 'd, 'e> fn(
                        &'a mut Vbi3BitSlicer,
                        &'b mut _,
                        Option<&'c mut _>,
                        Option<&'d mut _>,
                        &'e _,
                    ) -> _,
            )
        }
        VbiPixfmt::Rgba15Le | VbiPixfmt::Bgra15Le => {
            bs.green_mask = 0x03E0;
            bs.thresh = 105 << (5 - 3 + 11);
            bs.thresh_frac = 11;
            (
                4,
                0,
                bit_slicer_rgb16_le
                    as for<'a, 'b, 'c, 'd, 'e> fn(
                        &'a mut Vbi3BitSlicer,
                        &'b mut _,
                        Option<&'c mut _>,
                        Option<&'d mut _>,
                        &'e _,
                    ) -> _,
            )
        }
        VbiPixfmt::Rgba15Be | VbiPixfmt::Bgra15Be => {
            bs.green_mask = 0x03E0;
            bs.thresh = 105 << (5 - 3 + 11);
            bs.thresh_frac = 11;
            (
                4,
                0,
                bit_slicer_rgb16_be
                    as for<'a, 'b, 'c, 'd, 'e> fn(
                        &'a mut Vbi3BitSlicer,
                        &'b mut _,
                        Option<&'c mut _>,
                        Option<&'d mut _>,
                        &'e _,
                    ) -> _,
            )
        }
        VbiPixfmt::Argb15Le | VbiPixfmt::Abgr15Le => {
            bs.green_mask = 0x07C0;
            bs.thresh = 105 << (6 - 3 + 12);
            bs.thresh_frac = 12;
            (
                4,
                0,
                bit_slicer_rgb16_le
                    as for<'a, 'b, 'c, 'd, 'e> fn(
                        &'a mut Vbi3BitSlicer,
                        &'b mut _,
                        Option<&'c mut _>,
                        Option<&'d mut _>,
                        &'e _,
                    ) -> _,
            )
        }
        VbiPixfmt::Argb15Be | VbiPixfmt::Abgr15Be => {
            bs.green_mask = 0x07C0;
            bs.thresh = 105 << (6 - 3 + 12);
            bs.thresh_frac = 12;
            (
                4,
                0,
                bit_slicer_rgb16_be
                    as for<'a, 'b, 'c, 'd, 'e> fn(
                        &'a mut Vbi3BitSlicer,
                        &'b mut _,
                        Option<&'c mut _>,
                        Option<&'d mut _>,
                        &'e _,
                    ) -> _,
            )
        }
        _ => {
            error!("Unknown sample_format {:?}.", sample_format);
            return set_null_function(bs);
        }
    };

    if min_samples_per_bit > (3 << (LP_AVG - 1)) {
        bs.thresh <<= LP_AVG - 2;
        bs.thresh_frac += LP_AVG - 2;
    } else {
        bs.thresh = 105 << DEF_THR_FRAC;
        bs.thresh_frac = DEF_THR_FRAC;
    }

    bs.skip = sample_offset * bs.bytes_per_sample + skip;
    bs.cri_mask = cri_mask & c_mask;
    bs.cri = cri & bs.cri_mask;

    let cri_samples = ((sampling_rate as u64) * (cri_bits as u64)) / (cri_rate as u64);
    let data_bits = payload_bits + frc_bits;
    let data_samples = ((sampling_rate as u64) * (data_bits as u64)) / (payload_rate as u64);

    bs.total_bits = cri_bits + data_bits;

    if sample_offset > samples_per_line
        || (cri_samples + data_samples) > (samples_per_line - sample_offset) as u64
    {
        error!(
            "{} samples_per_line too small for sample_offset {} + {} cri_bits ({} samples) + {} frc_bits and {} payload_bits ({} samples).",
            samples_per_line, sample_offset, cri_bits, cri_samples, frc_bits, payload_bits, data_samples
        );
        return set_null_function(bs);
    }

    let cri_end = cri_end.min(samples_per_line - data_samples as u32);
    bs.cri_samples = cri_end - sample_offset;
    bs.cri_rate = cri_rate;
    bs.oversampling_rate = sampling_rate * oversampling;
    bs.frc = frc & f_mask;
    bs.frc_bits = frc_bits;
    bs.step = sampling_rate * 256 / payload_rate;

    if payload_bits & 7 != 0 {
        bs.payload = payload_bits;
        bs.endian = 3;
    } else {
        bs.payload = payload_bits >> 3;
        bs.endian = 1;
    }

    match modulation {
        VbiModulation::NrzMsb => {
            bs.endian -= 1;
            bs.phase_shift = (sampling_rate as f64 * 256.0 / cri_rate as f64 * 0.5
                + bs.step as f64 * 0.5
                + 128.0) as u32;
        }
        VbiModulation::NrzLsb => {
            bs.phase_shift = (sampling_rate as f64 * 256.0 / cri_rate as f64 * 0.5
                + bs.step as f64 * 0.5
                + 128.0) as u32;
        }
        VbiModulation::BiphaseMsb => {
            bs.endian -= 1;
            bs.phase_shift = (sampling_rate as f64 * 256.0 / cri_rate as f64 * 0.5
                + bs.step as f64 * 0.25
                + 128.0) as u32;
        }
        VbiModulation::BiphaseLsb => {
            bs.phase_shift = (sampling_rate as f64 * 256.0 / cri_rate as f64 * 0.5
                + bs.step as f64 * 0.25
                + 128.0) as u32;
        }
    }

    // Set the appropriate function based on the sample format
    match sample_format {
        VbiPixfmt::Yuv420 => {
            if min_samples_per_bit > (3 << (LP_AVG - 1)) {
                bs.func = Some(wrapper_low_pass_bit_slicer_y8);
            } else {
                bs.func = Some(wrapper_bit_slicer_y8);
            }
        }
        VbiPixfmt::Yuyv | VbiPixfmt::Yvyu => {
            if min_samples_per_bit > (3 << (LP_AVG - 1)) {
                bs.func = Some(wrapper_low_pass_bit_slicer_y8);
            } else {
                bs.func = Some(wrapper_bit_slicer_yuyv);
            }
        }
        VbiPixfmt::Uyvy | VbiPixfmt::Vyuy => {
            if min_samples_per_bit > (3 << (LP_AVG - 1)) {
                bs.func = Some(wrapper_low_pass_bit_slicer_y8);
            } else {
                bs.func = Some(wrapper_bit_slicer_yuyv);
            }
        }
        VbiPixfmt::Rgba32Le | VbiPixfmt::Bgra32Le => {
            if min_samples_per_bit > (3 << (LP_AVG - 1)) {
                bs.func = Some(wrapper_low_pass_bit_slicer_y8);
            } else {
                bs.func = Some(wrapper_bit_slicer_rgba24_le);
            }
        }
        VbiPixfmt::Rgba32Be | VbiPixfmt::Bgra32Be => {
            if min_samples_per_bit > (3 << (LP_AVG - 1)) {
                bs.func = Some(wrapper_low_pass_bit_slicer_y8);
            } else {
                bs.func = Some(wrapper_bit_slicer_rgba24_le);
            }
        }
        VbiPixfmt::Rgb24 | VbiPixfmt::Bgr24 => {
            if min_samples_per_bit > (3 << (LP_AVG - 1)) {
                bs.func = Some(wrapper_low_pass_bit_slicer_y8);
            } else {
                bs.func = Some(wrapper_bit_slicer_rgb24_le);
            }
        }
        VbiPixfmt::Rgb16Le | VbiPixfmt::Bgr16Le => {
            bs.func = Some(wrapper_bit_slicer_rgb16_le);
        }
        VbiPixfmt::Rgb16Be | VbiPixfmt::Bgr16Be => {
            bs.func = Some(wrapper_bit_slicer_rgb16_be);
        }
        _ => {
            bs.func = Some(wrapper_bit_slicer_y8);
        }
    }

    unsafe extern "C" fn wrapper_bit_slicer_y8(
        bs: *mut Vbi3BitSlicer,
        buffer: *mut u8,
        points: *mut Vbi3BitSlicerPoint,
        n_points: *mut u32,
        raw: *const u8,
    ) -> i32 {
        let bs = unsafe { &mut *bs };
        let buffer = unsafe { std::slice::from_raw_parts_mut(buffer, bs.payload as usize) };
        let points = if points.is_null() {
            None
        } else {
            Some(unsafe { std::slice::from_raw_parts_mut(points, bs.total_bits as usize) })
        };
        let n_points = if n_points.is_null() {
            None
        } else {
            Some(unsafe { &mut *n_points })
        };
        let raw = unsafe {
            std::slice::from_raw_parts(raw, bs.bytes_per_sample as usize * bs.total_bits as usize)
        };

        if bit_slicer_y8(bs, buffer, points, n_points, raw) {
            1 // true
        } else {
            0 // false
        }
    }

    unsafe extern "C" fn wrapper_bit_slicer_yuyv(
        bs: *mut Vbi3BitSlicer,
        buffer: *mut u8,
        points: *mut Vbi3BitSlicerPoint,
        n_points: *mut u32,
        raw: *const u8,
    ) -> i32 {
        let bs = unsafe { &mut *bs };
        let buffer = unsafe { std::slice::from_raw_parts_mut(buffer, bs.payload as usize) };
        let points = if points.is_null() {
            None
        } else {
            Some(unsafe { std::slice::from_raw_parts_mut(points, bs.total_bits as usize) })
        };
        let n_points = if n_points.is_null() {
            None
        } else {
            Some(unsafe { &mut *n_points })
        };
        let raw = unsafe {
            std::slice::from_raw_parts(raw, bs.bytes_per_sample as usize * bs.total_bits as usize)
        };

        if bit_slicer_yuyv(bs, buffer, points, n_points, raw) {
            1 // true
        } else {
            0 // false
        }
    }

    unsafe extern "C" fn wrapper_bit_slicer_rgb24_le(
        bs: *mut Vbi3BitSlicer,
        buffer: *mut u8,
        points: *mut Vbi3BitSlicerPoint,
        n_points: *mut u32,
        raw: *const u8,
    ) -> i32 {
        let bs = unsafe { &mut *bs };
        let buffer = unsafe { std::slice::from_raw_parts_mut(buffer, bs.payload as usize) };
        let points = if points.is_null() {
            None
        } else {
            Some(unsafe { std::slice::from_raw_parts_mut(points, bs.total_bits as usize) })
        };
        let n_points = if n_points.is_null() {
            None
        } else {
            Some(unsafe { &mut *n_points })
        };
        let raw = unsafe {
            std::slice::from_raw_parts(raw, bs.bytes_per_sample as usize * bs.total_bits as usize)
        };

        if bit_slicer_rgb24_le(bs, buffer, points, n_points, raw) {
            1 // true
        } else {
            0 // false
        }
    }

    unsafe extern "C" fn wrapper_bit_slicer_rgba24_le(
        bs: *mut Vbi3BitSlicer,
        buffer: *mut u8,
        points: *mut Vbi3BitSlicerPoint,
        n_points: *mut u32,
        raw: *const u8,
    ) -> i32 {
        let bs = unsafe { &mut *bs };
        let buffer = unsafe { std::slice::from_raw_parts_mut(buffer, bs.payload as usize) };
        let points = if points.is_null() {
            None
        } else {
            Some(unsafe { std::slice::from_raw_parts_mut(points, bs.total_bits as usize) })
        };
        let n_points = if n_points.is_null() {
            None
        } else {
            Some(unsafe { &mut *n_points })
        };
        let raw = unsafe {
            std::slice::from_raw_parts(raw, bs.bytes_per_sample as usize * bs.total_bits as usize)
        };

        if bit_slicer_rgba24_le(bs, buffer, points, n_points, raw) {
            1 // true
        } else {
            0 // false
        }
    }

    unsafe extern "C" fn wrapper_bit_slicer_rgb16_le(
        bs: *mut Vbi3BitSlicer,
        buffer: *mut u8,
        points: *mut Vbi3BitSlicerPoint,
        n_points: *mut u32,
        raw: *const u8,
    ) -> i32 {
        let bs = unsafe { &mut *bs };
        let buffer = unsafe { std::slice::from_raw_parts_mut(buffer, bs.payload as usize) };
        let points = if points.is_null() {
            None
        } else {
            Some(unsafe { std::slice::from_raw_parts_mut(points, bs.total_bits as usize) })
        };
        let n_points = if n_points.is_null() {
            None
        } else {
            Some(unsafe { &mut *n_points })
        };
        let raw = unsafe {
            std::slice::from_raw_parts(raw, bs.bytes_per_sample as usize * bs.total_bits as usize)
        };

        if bit_slicer_rgb16_le(bs, buffer, points, n_points, raw) {
            1 // true
        } else {
            0 // false
        }
    }

    unsafe extern "C" fn wrapper_bit_slicer_rgb16_be(
        bs: *mut Vbi3BitSlicer,
        buffer: *mut u8,
        points: *mut Vbi3BitSlicerPoint,
        n_points: *mut u32,
        raw: *const u8,
    ) -> i32 {
        let bs = unsafe { &mut *bs };
        let buffer = unsafe { std::slice::from_raw_parts_mut(buffer, bs.payload as usize) };
        let points = if points.is_null() {
            None
        } else {
            Some(unsafe { std::slice::from_raw_parts_mut(points, bs.total_bits as usize) })
        };
        let n_points = if n_points.is_null() {
            None
        } else {
            Some(unsafe { &mut *n_points })
        };
        let raw = unsafe {
            std::slice::from_raw_parts(raw, bs.bytes_per_sample as usize * bs.total_bits as usize)
        };

        if bit_slicer_rgb16_be(bs, buffer, points, n_points, raw) {
            1 // true
        } else {
            0 // false
        }
    }

    unsafe extern "C" fn wrapper_low_pass_bit_slicer_y8(
        bs: *mut Vbi3BitSlicer,
        buffer: *mut u8,
        points: *mut Vbi3BitSlicerPoint,
        n_points: *mut u32,
        raw: *const u8,
    ) -> i32 {
        let bs = unsafe { &mut *bs };
        let buffer = unsafe { std::slice::from_raw_parts_mut(buffer, bs.payload as usize) };
        let points = if points.is_null() {
            None
        } else {
            Some(unsafe { std::slice::from_raw_parts_mut(points, bs.total_bits as usize) })
        };
        let n_points = if n_points.is_null() {
            None
        } else {
            Some(unsafe { &mut *n_points })
        };
        let raw = unsafe {
            std::slice::from_raw_parts(raw, bs.bytes_per_sample as usize * bs.total_bits as usize)
        };

        if low_pass_bit_slicer_y8(bs, buffer, points, n_points, raw) {
            1 // true
        } else {
            0 // false
        }
    }
    true
}

pub fn set_null_function(bs: &mut Vbi3BitSlicer) -> bool {
    bs.func = Some(null_function);
    false
}

pub fn vbi3_raw_decoder_decode(
    rd: &mut Vbi3RawDecoder,
    sliced: &mut [VbiSliced],
    max_lines: u32,
    mut raw: &[u8],
) -> u32 {
    if rd.services == 0 {
        return 0;
    }

    let scan_lines = (rd.sampling.count[0] + rd.sampling.count[1]) as usize;
    let pitch = (rd.sampling.bytes_per_line as usize) << rd.sampling.interlaced;
    let mut pattern = rd.pattern;
    let raw1 = raw;
    let mut sliced_begin = sliced.as_mut_ptr();
    let sliced_end = unsafe { sliced_begin.add(max_lines as usize) };

    if RAW_DECODER_PATTERN_DUMP {
        let mut output = Vec::new();
        _vbi3_raw_decoder_dump(rd, &mut output);
    }

    for i in 0..scan_lines {
        if sliced_begin >= sliced_end {
            break;
        }

        let interlaced = rd.sampling.interlaced;
        let count0 = rd.sampling.count[0];
        let bytes_per_line = rd.sampling.bytes_per_line;

        if interlaced != 0 && i == count0 as usize {
            raw = &raw1[bytes_per_line as usize..];
        }

        sliced_begin = decode_pattern(rd, sliced_begin, pattern, i as u32, raw);

        unsafe {
            pattern = pattern.add(_VBI3_RAW_DECODER_MAX_WAYS);
        }
        raw = &raw[pitch..];
    }

    rd.readjust = (rd.readjust + 1) & 15;

    unsafe { sliced_begin.offset_from(sliced.as_mut_ptr()) as u32 }
}

pub fn vbi_sliced_name(service: VbiServiceSet) -> Option<&'static str> {
    // These are ambiguous
    if service == VBI_SLICED_CAPTION_525 {
        return Some("Closed Caption 525");
    }
    if service == VBI_SLICED_CAPTION_625 {
        return Some("Closed Caption 625");
    }
    if service == (VBI_SLICED_VPS | VBI_SLICED_VPS_F2) {
        return Some("Video Program System");
    }
    if service == VBI_SLICED_TELETEXT_B_L25_625 {
        return Some("Teletext System B 625 Level 2.5");
    }

    // Incorrect, no longer in table
    if service == VBI_SLICED_TELETEXT_BD_525 {
        return Some("Teletext System B/D");
    }

    // Find the service in the table
    if let Some(par) = find_service_par(service) {
        return Some(unsafe {
            std::ffi::CStr::from_ptr(par.label)
                .to_str()
                .unwrap_or_default()
        });
    }

    None
}

pub fn find_service_par(service: VbiServiceSet) -> Option<&'static VbiServicePar> {
    VBI_SERVICE_TABLE.iter().find(|par| par.id == service)
}

pub fn slice(
    rd: &mut Vbi3RawDecoder,
    sliced: *mut VbiSliced,
    job: &mut VbiRawDecoderJob,
    i: u32,
    raw: &[u8],
) -> bool {
    // VbiBitSlicer to Vbi3BitSlicer adapter function
    fn vbi_to_vbi3_bit_slicer(bs: &mut VbiBitSlicer) -> &mut Vbi3BitSlicer {
        unsafe { &mut *(bs as *mut VbiBitSlicer as *mut Vbi3BitSlicer) }
    }

    if rd.debug != 0 && !rd.sp_lines.is_null() {
        let sp_line = unsafe { &mut *rd.sp_lines.add(i as usize) };

        let points_len = sp_line.points.len();
        vbi3_bit_slicer_slice_with_points(
            vbi_to_vbi3_bit_slicer(&mut job.slicer),
            unsafe { &mut (*sliced).data },
            std::mem::size_of_val(&(unsafe { *sliced }).data),
            &mut sp_line.points,
            &mut sp_line.n_points,
            points_len,
            raw,
        )
    } else {
        vbi3_bit_slicer_slice(
            vbi_to_vbi3_bit_slicer(&mut job.slicer),
            unsafe { &mut (*sliced).data },
            std::mem::size_of_val(&(unsafe { *sliced }).data),
            raw,
        )
    }
}

pub fn vbi3_bit_slicer_slice(
    bs: &mut Vbi3BitSlicer,
    buffer: &mut [u8],
    buffer_size: usize,
    raw: &[u8],
) -> bool {
    assert!(bs.func.is_some(), "Bit slicer function is not set.");
    assert!(!buffer.is_empty(), "Buffer cannot be null.");
    assert!(!raw.is_empty(), "Raw data cannot be null.");

    if bs.payload > (buffer_size * 8) as u32 {
        error!(
            "buffer_size {} < {} bits of payload.",
            buffer_size * 8,
            bs.payload
        );
        return false;
    }

    if let Some(func) = bs.func {
        unsafe {
            func(
                bs,
                buffer.as_mut_ptr(),
                std::ptr::null_mut(),
                std::ptr::null_mut(),
                raw.as_ptr(),
            ) != 0
        }
    } else {
        false
    }
}

pub fn vbi3_bit_slicer_slice_with_points(
    bs: &mut Vbi3BitSlicer,
    buffer: &mut [u8],
    buffer_size: usize,
    points: &mut [Vbi3BitSlicerPoint],
    n_points: &mut u32,
    max_points: usize,
    raw: &[u8],
) -> bool {
    const PIXFMT: VbiPixfmt = VbiPixfmt::Yuv420; // Equivalent to VBI_PIXFMT_Y8
    const BPP: usize = 1; // Bytes per pixel
    const OVERSAMPLING: u32 = 4; // Oversampling rate
    const THRESH_FRAC: u32 = DEF_THR_FRAC; // Threshold fraction
    const COLLECT_POINTS: bool = true; // Collect points

    assert!(!buffer.is_empty(), "Buffer cannot be null.");
    assert!(!points.is_empty(), "Points cannot be null.");
    // assert!(n_points.is_some(), "n_points cannot be null."); // not needed
    assert!(!raw.is_empty(), "Raw data cannot be null.");

    *n_points = 0;

    if bs.payload > (buffer_size * 8) as u32 {
        error!(
            "buffer_size {} < {} bits of payload.",
            buffer_size * 8,
            bs.payload
        );
        return false;
    }

    if bs.total_bits as usize > max_points {
        error!(
            "max_points {} < {} CRI, FRC and payload bits.",
            max_points, bs.total_bits
        );
        return false;
    }

    if let Some(func) = bs.func {
        if std::ptr::fn_addr_eq(
            func,
            wrapper_low_pass_bit_slicer_y8
                as unsafe extern "C" fn(
                    *mut Vbi3BitSlicer,
                    *mut u8,
                    *mut Vbi3BitSlicerPoint,
                    *mut u32,
                    *const u8,
                ) -> i32,
        ) {
            return unsafe {
                func(
                    bs,
                    buffer.as_mut_ptr(),
                    points.as_mut_ptr(),
                    n_points,
                    raw.as_ptr(),
                ) != 0
            };
        } else if !std::ptr::fn_addr_eq(
            func,
            wrapper_bit_slicer_y8
                as unsafe extern "C" fn(
                    *mut Vbi3BitSlicer,
                    *mut u8,
                    *mut Vbi3BitSlicerPoint,
                    *mut u32,
                    *const u8,
                ) -> i32,
        ) {
            error!(
                "Function not implemented for pixfmt {:?}.",
                bs.sample_format
            );
            return unsafe {
                func(
                    bs,
                    buffer.as_mut_ptr(),
                    std::ptr::null_mut(),
                    std::ptr::null_mut(),
                    raw.as_ptr(),
                ) != 0
            };
        }
    }

    core(
        bs,
        buffer,
        Some(points),
        Some(n_points),
        raw,
        PIXFMT,
        BPP,
        OVERSAMPLING,
        THRESH_FRAC,
        COLLECT_POINTS,
    )
}

pub unsafe extern "C" fn wrapper_low_pass_bit_slicer_y8(
    bs: *mut Vbi3BitSlicer,
    buffer: *mut u8,
    points: *mut Vbi3BitSlicerPoint,
    n_points: *mut u32,
    raw: *const u8,
) -> i32 {
    let bs = &mut *bs;
    let buffer = std::slice::from_raw_parts_mut(buffer, bs.payload as usize);
    let points = if points.is_null() {
        None
    } else {
        Some(std::slice::from_raw_parts_mut(
            points,
            bs.total_bits as usize,
        ))
    };
    let n_points = if n_points.is_null() {
        None
    } else {
        Some(&mut *n_points)
    };
    let raw =
        std::slice::from_raw_parts(raw, bs.bytes_per_sample as usize * bs.total_bits as usize);

    if low_pass_bit_slicer_y8(bs, buffer, points, n_points, raw) {
        1 // true
    } else {
        0 // false
    }
}

pub unsafe extern "C" fn wrapper_bit_slicer_y8(
    bs: *mut Vbi3BitSlicer,
    buffer: *mut u8,
    points: *mut Vbi3BitSlicerPoint,
    n_points: *mut u32,
    raw: *const u8,
) -> i32 {
    let bs = &mut *bs;
    let buffer = std::slice::from_raw_parts_mut(buffer, bs.payload as usize);
    let points = if points.is_null() {
        None
    } else {
        Some(std::slice::from_raw_parts_mut(
            points,
            bs.total_bits as usize,
        ))
    };
    let n_points = if n_points.is_null() {
        None
    } else {
        Some(&mut *n_points)
    };
    let raw =
        std::slice::from_raw_parts(raw, bs.bytes_per_sample as usize * bs.total_bits as usize);

    if bit_slicer_y8(bs, buffer, points, n_points, raw) {
        1 // true
    } else {
        0 // false
    }
}

pub fn _vbi3_raw_decoder_dump(rd: &Vbi3RawDecoder, fp: &mut dyn std::io::Write) {
    writeln!(fp, "vbi3_raw_decoder {:p}", rd).unwrap();

    if rd.services == 0 {
        writeln!(fp, "  services 0x00000000").unwrap();
        return;
    }

    writeln!(fp, "  services 0x{:08x}", rd.services).unwrap();

    for (i, job) in rd.jobs.iter().enumerate().take(rd.n_jobs as usize) {
        writeln!(
            fp,
            "  job {}: 0x{:08x} ({})",
            i + 1,
            job.id,
            vbi_sliced_name(job.id).unwrap_or("Unknown")
        )
        .unwrap();
    }

    if rd.pattern.is_null() {
        writeln!(fp, "  no pattern").unwrap();
        return;
    }

    let sp = &rd.sampling;

    for i in 0..(sp.count[0] + sp.count[1]) as usize {
        write!(fp, "  ").unwrap();
        dump_pattern_line(rd, i, fp);
    }
}

pub fn dump_pattern_line(rd: &Vbi3RawDecoder, row: usize, fp: &mut dyn std::io::Write) {
    let sp = &rd.sampling;

    let line;
    if sp.interlaced != 0 {
        let field = row & 1;

        if sp.start[field] == 0 {
            line = 0;
        } else {
            line = sp.start[field] as usize + (row >> 1);
        }
    } else if row >= sp.count[0] as usize {
        if sp.start[1] == 0 {
            line = 0;
        } else {
            line = sp.start[1] as usize + row - sp.count[0] as usize;
        }
    } else if sp.start[0] == 0 {
        line = 0;
    } else {
        line = sp.start[0] as usize + row;
    }

    writeln!(fp, "scan line {:3}: ", line).unwrap();

    for i in 0.._VBI3_RAW_DECODER_MAX_WAYS {
        let pos = row * _VBI3_RAW_DECODER_MAX_WAYS;
        write!(fp, "{:02x} ", unsafe { *rd.pattern.add(pos + i) as u8 }).unwrap();
    }

    writeln!(fp).unwrap();
}
