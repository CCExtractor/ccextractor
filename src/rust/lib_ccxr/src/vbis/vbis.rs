use std::os::raw::{c_char, c_int, c_long, c_uchar, c_uint, c_ulonglong, c_void};
use std::ptr::*;

extern crate libc;
use crate::vbis::*;
use vbid::*;

pub unsafe fn freep(arg: *mut *mut c_void) {
    if !arg.is_null() && !(*arg).is_null() {
        free(*arg);
        *arg = std::ptr::null_mut();
    }
}

pub unsafe fn free<T>(ptr: *mut T) {
    if !ptr.is_null() {
        // Reconstruct the Box to take ownership and drop it, deallocating the memory.
        Box::from_raw(ptr);
    }
}

pub unsafe fn vbi_free<T>(ptr: *mut T) {
    if !ptr.is_null() {
        // Reconstruct the Box to take ownership and drop it, deallocating the memory.
        Box::from_raw(ptr);
    }
}

pub unsafe fn vbi_raw_decoder_destroy(rd: *mut VbiRawDecoder) {
    assert!(!rd.is_null());

    let rd3 = (*rd).pattern as *mut Vbi3RawDecoder;

    vbi3_raw_decoder_delete(rd3);

    std::ptr::write_bytes(rd, 0, 1);
}

pub unsafe fn vbi3_raw_decoder_delete(rd: *mut Vbi3RawDecoder) {
    if rd.is_null() {
        return;
    }

    _vbi3_raw_decoder_destroy(rd);

    vbi_free(rd as *mut c_void);
}

pub unsafe fn _vbi3_raw_decoder_destroy(rd: *mut Vbi3RawDecoder) {
    vbi3_raw_decoder_reset(rd);

    vbi3_raw_decoder_debug(rd, false);

    // Make unusable
    std::ptr::write_bytes(rd, 0, 1);
}

pub unsafe fn vbi3_raw_decoder_reset(rd: *mut Vbi3RawDecoder) {
    assert!(!rd.is_null());

    if !(*rd).pattern.is_null() {
        vbi_free((*rd).pattern as *mut c_void);
        (*rd).pattern = std::ptr::null_mut();
    }

    (*rd).services = 0;
    (*rd).n_jobs = 0;
    (*rd).readjust = 1;

    std::ptr::write_bytes(&mut (*rd).jobs, 0, std::mem::size_of_val(&(*rd).jobs));
}

pub unsafe fn vbi3_raw_decoder_debug(rd: *mut Vbi3RawDecoder, enable: bool) -> bool {
    assert!(!rd.is_null());

    let sp_lines: *mut _Vbi3RawDecoderSpLine = std::ptr::null_mut();
    let mut n_lines: c_uint = 0;
    let mut r: bool = true;

    (*rd).debug = enable as i32;

    if enable {
        n_lines = (*rd).sampling.count[0] as u32 + (*rd).sampling.count[1] as u32;
    }

    match (*rd).sampling.sampling_format {
        VbiPixfmt::Yuv420 => {}
        _ => {
            // Not implemented
            n_lines = 0;
            r = false;
        }
    }

    if (*rd).n_sp_lines == n_lines {
        return r;
    }

    vbi_free((*rd).sp_lines as *mut c_void);
    (*rd).sp_lines = std::ptr::null_mut();
    (*rd).n_sp_lines = 0;

    if n_lines > 0 {
        (*rd).sp_lines = libc::calloc(
            n_lines as usize,
            std::mem::size_of::<_Vbi3RawDecoderSpLine>(),
        ) as *mut _Vbi3RawDecoderSpLine;
        if (*rd).sp_lines.is_null() {
            return false;
        }

        (*rd).n_sp_lines = n_lines;
    }

    r
}

pub unsafe fn delete_decoder_vbi(arg: *mut *mut CcxDecoderVbiCtx) {
    unsafe {
        let ctx = *arg;
        vbi_raw_decoder_destroy(&mut (*ctx).zvbi_decoder);

        freep(arg as *mut *mut c_void);
    }
}

#[repr(C)]
pub struct CcxDecoderVbiCfg {
    pub debug_file_name: *mut c_char,
}

pub unsafe fn vbi_raw_decoder_init(rd: *mut VbiRawDecoder) {
    assert!(!rd.is_null());

    std::ptr::write_bytes(rd, 0, 1);

    let rd3 = vbi3_raw_decoder_new(std::ptr::null_mut());
    assert!(!rd3.is_null());

    (*rd).pattern = rd3 as *mut i8;
}

pub unsafe fn vbi3_raw_decoder_new(sp: *const VbiSamplingPar) -> *mut Vbi3RawDecoder {
    let rd = libc::malloc(std::mem::size_of::<Vbi3RawDecoder>()) as *mut Vbi3RawDecoder;
    if rd.is_null() {
        *errno() = libc::ENOMEM;
        return std::ptr::null_mut();
    }

    if _vbi3_raw_decoder_init(rd, sp) == CCX_FALSE {
        vbi_free(rd as *mut c_void);
        return std::ptr::null_mut();
    }

    rd
}

#[inline]
pub unsafe fn errno() -> *mut c_int {
    libc::__error()
}

pub fn init_decoder_vbi(cfg: Option<&CcxDecoderVbiCfg>) -> Option<*mut CcxDecoderVbiCtx> {
    let vbi = unsafe { libc::malloc(std::mem::size_of::<CcxDecoderVbiCtx>()) as *mut CcxDecoderVbiCtx };
    if vbi.is_null() {
        return None;
    }

    unsafe {
        (*vbi).vbi_debug_dump = libc::fopen(
            b"dump_720.vbi\0".as_ptr() as *const c_char,
            b"w\0".as_ptr() as *const c_char,
        );

        vbi_raw_decoder_init(&mut (*vbi).zvbi_decoder);

        if cfg.is_none() {
            (*vbi).zvbi_decoder.scanning = 525;
            (*vbi).zvbi_decoder.sampling_format = VbiPixfmt::Yuv420;
            (*vbi).zvbi_decoder.sampling_rate = 13.5e6 as c_int;
            (*vbi).zvbi_decoder.bytes_per_line = 720;
            (*vbi).zvbi_decoder.offset = (9.7e-6 * 13.5e6) as c_int;
            (*vbi).zvbi_decoder.start[0] = 21;
            (*vbi).zvbi_decoder.count[0] = 1;
            (*vbi).zvbi_decoder.start[1] = 284;
            (*vbi).zvbi_decoder.count[1] = 1;
            (*vbi).zvbi_decoder.interlaced = CCX_TRUE;
            (*vbi).zvbi_decoder.synchronous = CCX_TRUE;

            vbi_raw_decoder_add_services(&mut (*vbi).zvbi_decoder, VBI_SLICED_CAPTION_525, 0);
        }
    }

    Some(vbi)
}

extern "C" {
    pub static mut ccx_common_logging: CcxCommonLogging;
}

pub unsafe fn debug(log: &mut VbiLogHook, fmt: &str, args: std::fmt::Arguments) {
    if let Some(debug_fn) = ccx_common_logging.debug_ftn {
        debug_fn(
            CCX_DMT_PARSE as i64,
            format!("VBI:{}:{}: {}\0", std::module_path!(), line!(), fmt).as_ptr() as *const i8,
            args
        );
    }
}

pub unsafe fn log(hook: &mut VbiLogHook, fmt: &str, args: std::fmt::Arguments) {
    if let Some(log_fn) = ccx_common_logging.log_ftn {
        log_fn(
            format!("VBI:{}: {}\0", line!(), fmt).as_ptr() as *const i8,
            args
        );
    }
}

pub unsafe fn warning(hook: &mut VbiLogHook, fmt: &str, args: std::fmt::Arguments) {
    if let Some(log_fn) = ccx_common_logging.log_ftn {
        log_fn(
            format!("VBI:{}: {}\0", line!(), fmt).as_ptr() as *const i8,
            args
        );
    }
}

pub unsafe fn error(hook: &mut VbiLogHook, fmt: &str, args: std::fmt::Arguments) {
    log(hook, fmt, args);
}

pub unsafe fn info(log: &mut VbiLogHook, fmt: &str, args: std::fmt::Arguments) {
    debug(log, fmt, args);
}

pub fn vbi_pixfmt_bpp(fmt: VbiPixfmt) -> i32 {
    match fmt {
        VbiPixfmt::Yuv420 => 1,
        VbiPixfmt::Rgba32Le
        | VbiPixfmt::Rgba32Be
        | VbiPixfmt::Bgra32Le
        | VbiPixfmt::Bgra32Be => 4,
        VbiPixfmt::Rgb24 | VbiPixfmt::Bgr24 => 3,
        _ => 2,
    }
}

pub fn _vbi_videostd_set_from_scanning(scanning: i32) -> u64 {
    match scanning {
        525 => VBI_VIDEOSTD_SET_525_60,
        625 => VBI_VIDEOSTD_SET_625_50,
        _ => 0,
    }
}

#[inline]
pub fn range_check(start: i32, count: i32, min: i32, max: i32) -> bool {
    // Check bounds and overflow.
    start >= min && (start + count) <= max && (start + count) >= start
}

pub unsafe fn _vbi_sampling_par_valid_log(
    sp: *const VbiSamplingPar,
    log: &mut VbiLogHook,
) -> VbiBool {
    assert!(!sp.is_null());

    let mut videostd_set: u64;
    let bpp: i32;

    match (*sp).sampling_format {
        VbiPixfmt::Yuv420 => {
            // This conflicts with the ivtv driver, which returns an
            // odd number of bytes per line. The driver format is
            // _GREY but libzvbi 0.2 has no VBI_PIXFMT_Y8.
        }
        _ => {
            bpp = vbi_pixfmt_bpp((*sp).sampling_format);
            if (*sp).bytes_per_line % bpp != 0 {
                goto_bad_samples(log, sp);
                return CCX_FALSE;
            }
        }
    }

    if (*sp).bytes_per_line == 0 {
        info(log, "samples_per_line is zero.", format_args!(""));
        return CCX_FALSE;
    }

    if (*sp).count[0] == 0 && (*sp).count[1] == 0 {
        goto_bad_range(log, sp);
        return CCX_FALSE;
    }

    videostd_set = _vbi_videostd_set_from_scanning((*sp).scanning);

    if (VBI_VIDEOSTD_SET_525_60 & videostd_set) != 0 {
        if (VBI_VIDEOSTD_SET_625_50 & videostd_set) != 0 {
            info(log, "Ambiguous videostd_set 0x{:x}.", format_args!("{:x}", videostd_set));
            return CCX_FALSE;
        }

        if (*sp).start[0] != 0
            && !range_check((*sp).start[0], (*sp).count[0], 1, 262)
        {
            goto_bad_range(log, sp);
            return CCX_FALSE;
        }

        if (*sp).start[1] != 0
            && !range_check((*sp).start[1], (*sp).count[1], 263, 525)
        {
            goto_bad_range(log, sp);
            return CCX_FALSE;
        }
    } else if (VBI_VIDEOSTD_SET_625_50 & videostd_set) != 0 {
        if (*sp).start[0] != 0
            && !range_check((*sp).start[0], (*sp).count[0], 1, 311)
        {
            goto_bad_range(log, sp);
            return CCX_FALSE;
        }

        if (*sp).start[1] != 0
            && !range_check((*sp).start[1], (*sp).count[1], 312, 625)
        {
            goto_bad_range(log, sp);
            return CCX_FALSE;
        }
    } else {
        info(log, "Ambiguous videostd_set 0x{:x}.", format_args!("{:x}", videostd_set));
        return CCX_FALSE;
    }

    if (*sp).interlaced != 0
        && ((*sp).count[0] != (*sp).count[1] || (*sp).count[0] == 0)
    {
        info(
            log,
            "Line counts {}, {} must be equal and non-zero when raw VBI data is interlaced.",
            format_args!("{}, {}", (*sp).count[0], (*sp).count[1]),
        );
        return CCX_FALSE;
    }

    CCX_TRUE
}

unsafe fn goto_bad_samples(log: &mut VbiLogHook, sp: *const VbiSamplingPar) {
    info(
        log,
        "bytes_per_line value {} is no multiple of the sample size {}.",
        format_args!(
            "{}, {}", 
            (*sp).bytes_per_line, 
            vbi_pixfmt_bpp((*sp).sampling_format)
        )
    );
}

unsafe fn goto_bad_range(log: &mut VbiLogHook, sp: *const VbiSamplingPar) {
    info(
        log,
        "Invalid VBI scan range {}-{} ({} lines), {}-{} ({} lines).",
        format_args!(
            "{}-{} ({} lines), {}-{} ({} lines)",
            (*sp).start[0],
            (*sp).start[0] + (*sp).count[0] - 1,
            (*sp).count[0],
            (*sp).start[1],
            (*sp).start[1] + (*sp).count[1] - 1,
            (*sp).count[1]
        )
    );
}

pub unsafe fn vbi_raw_decoder_add_services(
    rd: *mut VbiRawDecoder,
    services: u32,
    strict: c_int,
) -> u32 {
    assert!(!rd.is_null());

    let rd3 = (*rd).pattern as *mut Vbi3RawDecoder;
    let mut service_set = services;

    vbi3_raw_decoder_set_sampling_par(rd3, rd as *mut VbiSamplingPar, strict);

    service_set = vbi3_raw_decoder_add_services(rd3, service_set, strict);

    service_set
}

pub unsafe fn vbi3_raw_decoder_set_sampling_par(
    rd: *mut Vbi3RawDecoder,
    sp: *const VbiSamplingPar,
    strict: c_int,
) -> u32 {
    assert!(!rd.is_null());
    assert!(!sp.is_null());

    let mut services = (*rd).services;

    vbi3_raw_decoder_reset(rd);

    if _vbi_sampling_par_valid_log(sp, &mut (*rd).log) == CCX_FALSE {
        std::ptr::write_bytes(&mut (*rd).sampling, 0, 1);
        return 0;
    }

    (*rd).sampling = *sp;

    // Error ignored.
    vbi3_raw_decoder_debug(rd, (*rd).debug != 0);

    vbi3_raw_decoder_add_services(rd, services, strict)
}

pub unsafe fn _vbi3_raw_decoder_init(rd: *mut Vbi3RawDecoder, sp: *const VbiSamplingPar) -> VbiBool {
    std::ptr::write_bytes(rd, 0, 1);

    vbi3_raw_decoder_reset(rd);

    if !sp.is_null() {
        if _vbi_sampling_par_valid_log(sp, &mut (*rd).log) == CCX_FALSE {
            return CCX_FALSE;
        }

        (*rd).sampling = *sp;
    }

    CCX_TRUE
}

pub unsafe fn lines_containing_data(
    start: &mut [u32; 2],
    count: &mut [u32; 2],
    sp: *const VbiSamplingPar,
    par: *const VbiServicePar,
) {
    start[0] = 0;
    start[1] = (*sp).count[0] as u32;

    count[0] = (*sp).count[0] as u32;
    count[1] = (*sp).count[1] as u32;

    if (*sp).synchronous == 0 {
        // Scanning all lines isn't always necessary.
        return;
    }

    for field in 0..2 {
        let mut first: u32;
        let mut last: u32;

        if (*par).first[field] == 0 || (*par).last[field] == 0 {
            // No data on this field.
            count[field] = 0;
            continue;
        }

        first = (*sp).start[field] as u32;
        last = first + (*sp).count[field] as u32 - 1;

        if first > 0 && (*sp).count[field] > 0 {
            assert!((*par).first[field] <= (*par).last[field]);

            if ((*par).first[field] as u32) > last || ((*par).last[field] as u32) < first {
                continue;
            }

            first = first.max(((*par).first[field] as u32));
            last = last.min((*par).last[field] as u32);

            start[field] += first - (*sp).start[field] as u32;
            count[field] = last + 1 - first;
        }
    }
}

pub unsafe fn _vbi3_bit_slicer_init(bs: *mut Vbi3BitSlicer) -> VbiBool {
    assert!(!bs.is_null());

    std::ptr::write_bytes(bs, 0, 1);

    (*bs).func = None;

    CCX_TRUE
}

pub unsafe fn vbi3_bit_slicer_set_params(
    bs: *mut Vbi3BitSlicer,
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
) -> VbiBool {
    assert!(!bs.is_null());
    assert!(cri_bits <= 32);
    assert!(frc_bits <= 32);
    assert!(payload_bits <= 32767);
    assert!(samples_per_line <= 32767);

    if cri_rate > sampling_rate {
        warning(
            &mut (*bs).log,
            "cri_rate {} > sampling_rate {}.",
            format_args!("{}, {}", cri_rate, sampling_rate),
        );
        return CCX_FALSE;
    }

    if payload_rate > sampling_rate {
        warning(
            &mut (*bs).log,
            "payload_rate {} > sampling_rate {}.",
            format_args!("{}, {}", payload_rate, sampling_rate),
        );
        return CCX_FALSE;
    }

    let min_samples_per_bit = sampling_rate / cri_rate.max(payload_rate);

    (*bs).sample_format = sample_format;

    let c_mask = if cri_bits == 32 { !0 } else { (1 << cri_bits) - 1 };
    let f_mask = if frc_bits == 32 { !0 } else { (1 << frc_bits) - 1 };

    let mut oversampling = 4;
    let mut skip = 0;

    (*bs).thresh = 105 << DEF_THR_FRAC;
    (*bs).thresh_frac = DEF_THR_FRAC;

    match sample_format {
        VbiPixfmt::Yuv420 => {
            (*bs).bytes_per_sample = 1;
            (*bs).func = Some(bit_slicer_Y8);
            if min_samples_per_bit > (3 << (LP_AVG - 1)) {
                (*bs).func = Some(low_pass_bit_slicer_Y8);
                oversampling = 1;
                (*bs).thresh <<= LP_AVG - 2;
                (*bs).thresh_frac += LP_AVG - 2;
            }
        }
        VbiPixfmt::Yuyv | VbiPixfmt::Yvyu => {
            (*bs).bytes_per_sample = 2;
            (*bs).func = Some(bit_slicer_YUYV);
            if min_samples_per_bit > (3 << (LP_AVG - 1)) {
                (*bs).func = Some(low_pass_bit_slicer_Y8);
                oversampling = 1;
                (*bs).thresh <<= LP_AVG - 2;
                (*bs).thresh_frac += LP_AVG - 2;
            }
        }
        VbiPixfmt::Uyvy | VbiPixfmt::Vyuy => {
            skip = 1;
            (*bs).bytes_per_sample = 2;
            (*bs).func = Some(bit_slicer_YUYV);
            if min_samples_per_bit > (3 << (LP_AVG - 1)) {
                (*bs).func = Some(low_pass_bit_slicer_Y8);
                oversampling = 1;
                (*bs).thresh <<= LP_AVG - 2;
                (*bs).thresh_frac += LP_AVG - 2;
            }
        }
        _ => {
            warning(
                &mut (*bs).log,
                "Unknown sample_format 0x{:x}.",
                format_args!("{:x}", sample_format as u32),
            );
            return CCX_FALSE;
        }
    }

    (*bs).skip = sample_offset * (*bs).bytes_per_sample + skip;

    (*bs).cri_mask = cri_mask & c_mask;
    (*bs).cri = cri & (*bs).cri_mask;

    let cri_samples = (sampling_rate as u64 * cri_bits as u64) / cri_rate as u64;
    let data_bits = payload_bits + frc_bits;
    let data_samples = (sampling_rate as u64 * data_bits as u64) / payload_rate as u64;

    (*bs).total_bits = cri_bits + data_bits;

    if sample_offset > samples_per_line
        || (cri_samples + data_samples) > (samples_per_line - sample_offset) as u64
    {
        warning(
            &mut (*bs).log,
            "{} samples_per_line too small for sample_offset {} + {} cri_bits ({} samples) + {} frc_bits and {} payload_bits ({} samples).",
            format_args!(
                "{} samples_per_line too small for sample_offset {} + {} cri_bits ({} samples) + {} frc_bits and {} payload_bits ({} samples)",
                samples_per_line,
                sample_offset,
                cri_bits,
                cri_samples,
                frc_bits,
                payload_bits,
                data_samples
            )
        );
        return CCX_FALSE;
    }

    let cri_end = cri_end.min(samples_per_line - data_samples as u32);

    (*bs).cri_samples = cri_end - sample_offset;
    (*bs).cri_rate = cri_rate;

    (*bs).oversampling_rate = sampling_rate * oversampling;

    (*bs).frc = frc & f_mask;
    (*bs).frc_bits = frc_bits;

    (*bs).step = (sampling_rate as u32 * 256) / payload_rate as u32;

    if payload_bits & 7 != 0 {
        (*bs).payload = payload_bits;
        (*bs).endian = 3;
    } else {
        (*bs).payload = payload_bits >> 3;
        (*bs).endian = 1;
    }

    match modulation {
        VbiModulation::NrzMsb => {
            (*bs).endian -= 1;
            (*bs).phase_shift = (sampling_rate as f64 * 256.0 / cri_rate as f64 * 0.5
                + (*bs).step as f64 * 0.5
                + 128.0) as u32;
        }
        VbiModulation::NrzLsb => {
            (*bs).phase_shift = (sampling_rate as f64 * 256.0 / cri_rate as f64 * 0.5
                + (*bs).step as f64 * 0.5
                + 128.0) as u32;
        }
        VbiModulation::BiphaseMsb => {
            (*bs).endian -= 1;
            (*bs).phase_shift = (sampling_rate as f64 * 256.0 / cri_rate as f64 * 0.5
                + (*bs).step as f64 * 0.25
                + 128.0) as u32;
        }
        VbiModulation::BiphaseLsb => {
            (*bs).phase_shift = (sampling_rate as f64 * 256.0 / cri_rate as f64 * 0.5
                + (*bs).step as f64 * 0.25
                + 128.0) as u32;
        }
    }

    CCX_TRUE
}

pub unsafe extern "C" fn bit_slicer_Y8(
    bs: *mut Vbi3BitSlicer,
    buffer: *mut u8,
    mut points: *mut Vbi3BitSlicerPoint,
    n_points: *mut u32,
    raw: *const u8,
) -> VbiBool {
    let mut points_start = points;
    let mut raw_start = raw;
    let mut raw = raw.add((*bs).skip as usize);

    let bps = (*bs).bytes_per_sample as usize;
    let mut thresh0 = (*bs).thresh as i32;

    let mut c = -1i32;
    let mut cl = 0u32;
    let mut b1 = 0u8;

    let mut raw0sum = *raw;
    for m in (bps..(bps << LP_AVG)).step_by(bps) {
        raw0sum += *raw.add(m);
    }

    let mut i = (*bs).cri_samples;

    loop {
        let mut b: u8;

        let tr = (*bs).thresh >> (*bs).thresh_frac;
        let raw0 = raw0sum;
        raw0sum = raw0sum + *raw.add(bps << LP_AVG) - *raw;
        raw = raw.add(bps);
        (*bs).thresh = (*bs).thresh.wrapping_add(
            ((raw0 as i32 - tr as i32) * (raw0sum as i32 - raw0 as i32).abs()) as u32
        );

        b = if raw0 as u32 >= tr { 1 } else { 0 };

        if b != b1 {
            cl = (*bs).oversampling_rate >> 1;
        } else {
            cl += (*bs).cri_rate;

            if cl >= (*bs).oversampling_rate {
                if !points.is_null() {
                    (*points).kind = Vbi3BitSlicerBit::CRI_BIT;
                    (*points).index = (((raw as usize - raw_start as usize) * 256
                        / (*bs).bytes_per_sample as usize)
                        + ((1 << LP_AVG) * 128)) as u32;
                    (*points).level = (raw0 as u32) << (8 - LP_AVG);
                    (*points).thresh = tr << (8 - LP_AVG);
                    points = points.add(1);
                }

                cl -= (*bs).oversampling_rate;
                c = c * 2 + b as i32;
                if (c & (*bs).cri_mask as i32) == (*bs).cri as i32 {
                    break;
                }
            }
        }

        b1 = b;

        if i == 0 {
            (*bs).thresh = thresh0 as u32;

            if !points.is_null() {
                *n_points = points.offset_from(points_start) as u32;
            }

            return CCX_FALSE;
        }

        i -= 1;
    }

    CCX_TRUE
}

pub unsafe extern "C" fn low_pass_bit_slicer_Y8(
    bs: *mut Vbi3BitSlicer,
    mut buffer: *mut u8,
    mut points: *mut Vbi3BitSlicerPoint,
    n_points: *mut u32,
    raw: *const u8,
) -> VbiBool {
    let mut points_start = points;
    let mut raw_start = raw;
    let mut raw = raw.add((*bs).skip as usize);

    let bps = (*bs).bytes_per_sample as usize;
    let mut thresh0 = (*bs).thresh as i32;

    let mut c = -1i32;
    let mut cl = 0u32;
    let mut b1 = 0u8;

    let mut raw0sum = *raw;
    for m in (bps..(bps << LP_AVG)).step_by(bps) {
        raw0sum += *raw.add(m);
    }

    let mut i = (*bs).cri_samples;
    let mut raw0: u8 = 0;
    let tr = (*bs).thresh >> (*bs).thresh_frac;

    loop {
        let mut b: u8;

        let tr = (*bs).thresh >> (*bs).thresh_frac;
        let raw0 = raw0sum;
        raw0sum = raw0sum + *raw.add(bps << LP_AVG) - *raw;
        raw = raw.add(bps);
        (*bs).thresh = (*bs).thresh.wrapping_add(
            ((raw0 as i32 - tr as i32) * (raw0sum as i32 - raw0 as i32).abs()) as u32,
        );

        b = if raw0 as u32 >= tr { 1 } else { 0 };

        if b != b1 {
            cl = (*bs).oversampling_rate >> 1;
        } else {
            cl += (*bs).cri_rate;

            if cl >= (*bs).oversampling_rate {
                if !points.is_null() {
                    (*points).kind = Vbi3BitSlicerBit::CRI_BIT;
                    (*points).index = (((raw as usize - raw_start as usize) * 256
                        / (*bs).bytes_per_sample as usize)
                        + ((1 << LP_AVG) * 128)) as u32;
                    (*points).level = (raw0 as u32) << (8 - LP_AVG);
                    (*points).thresh = tr << (8 - LP_AVG);
                    points = points.add(1);
                }

                cl -= (*bs).oversampling_rate;
                c = c * 2 + b as i32;
                if (c & (*bs).cri_mask as i32) == (*bs).cri as i32 {
                    break;
                }
            }
        }

        b1 = b;

        if i == 0 {
            (*bs).thresh = thresh0 as u32;

            if !points.is_null() {
                *n_points = points.offset_from(points_start) as u32;
            }

            return CCX_FALSE;
        }

        i -= 1;
    }

    macro_rules! lp_sample {
        ($kind:expr) => {
            let ii = (i >> 8) as usize * bps;

            let mut raw0 = *raw.add(ii);
            for m in (bps..(bps << LP_AVG)).step_by(bps) {
                raw0 += *raw.add(ii + m);
            }

            if !points.is_null() {
                (*points).kind = $kind;
                (*points).index = (((raw as usize - raw_start as usize) * 256
                    / (*bs).bytes_per_sample as usize)
                    + ((1 << LP_AVG) * 128)
                    + ii * 256) as u32;
                (*points).level = (raw0 as u32) << (8 - LP_AVG);
                (*points).thresh = tr << (8 - LP_AVG);
                points = points.add(1);
            }
        };
    }

    i = (*bs).phase_shift;
    c = 0;

    for _ in 0..(*bs).frc_bits {
        lp_sample!(Vbi3BitSlicerBit::FRC_BIT);
        c = c * 2 + (raw0 >= tr as u8) as i32;
        i += (*bs).step;
    }

    if c != (*bs).frc as i32 {
        return CCX_FALSE;
    }

    c = 0;

    match (*bs).endian {
        3 => {
            for j in 0..(*bs).payload {
                lp_sample!(Vbi3BitSlicerBit::PAYLOAD_BIT);
                c = (c >> 1) + ((raw0 >= tr as u8) as i32 * 128);
                i += (*bs).step;
                if (j & 7) == 7 {
                    *buffer = c as u8;
                    buffer = buffer.add(1);
                }
            }
            *buffer = (c >> ((8 - (*bs).payload) & 7)) as u8;
        }
        2 => {
            for j in 0..(*bs).payload {
                lp_sample!(Vbi3BitSlicerBit::PAYLOAD_BIT);
                c = c * 2 + (raw0 >= tr as u8) as i32;
                i += (*bs).step;
                if (j & 7) == 7 {
                    *buffer = c as u8;
                    buffer = buffer.add(1);
                }
            }
            *buffer = (c & ((1 << ((*bs).payload & 7)) - 1)) as u8;
        }
        1 => {
            let mut j = (*bs).payload;
            while j > 0 {
                for _ in 0..8 {
                    lp_sample!(Vbi3BitSlicerBit::PAYLOAD_BIT);
                    c = (c >> 1) + ((raw0 >= tr as u8) as i32 * 128);
                    i += (*bs).step;
                }
                *buffer = c as u8;
                buffer = buffer.add(1);
                j -= 1;
            }
        }
        _ => {
            let mut j = (*bs).payload;
            while j > 0 {
                for _ in 0..8 {
                    lp_sample!(Vbi3BitSlicerBit::PAYLOAD_BIT);
                    c = c * 2 + (raw0 >= tr as u8) as i32;
                    i += (*bs).step;
                }
                *buffer = c as u8;
                buffer = buffer.add(1);
                j -= 1;
            }
        }
    }

    if !points.is_null() {
        *n_points = points.offset_from(points_start) as u32;
    }

    CCX_TRUE
}

/* ----------- */

pub unsafe extern "C" fn bit_slicer_YUYV(
    bs: *mut Vbi3BitSlicer,
    buffer: *mut u8,
    points: *mut Vbi3BitSlicerPoint,
    n_points: *mut u32,
    raw: *const u8,
) -> VbiBool {
    const PIXFMT: VbiPixfmt = VbiPixfmt::Yuyv;
    let bpp = vbi_pixfmt_bpp(PIXFMT) as usize;
    const OVERSAMPLING: u32 = 4;
    let thresh_frac = DEF_THR_FRAC;

    let mut thresh0 = (*bs).thresh;
    let mut raw_start = raw;
    let mut raw = raw.add((*bs).skip as usize);

    let mut cl = 0u32;
    let mut c = 0u32;
    let mut b1 = 0u8;

    for mut i in (0..(*bs).cri_samples).rev() {
        let tr = (*bs).thresh >> thresh_frac;
        let raw0 = *raw;
        let raw1 = *raw.add(bpp) - raw0;
        (*bs).thresh = (*bs).thresh.wrapping_add(((raw0 as i32 - tr as i32) * (raw1 as i32).abs()) as u32);
        let mut t = (raw0 as u32) * OVERSAMPLING;

        for _ in 0..OVERSAMPLING {
            // CRI logic placeholder
        }

        raw = raw.add(bpp);
    }

    (*bs).thresh = thresh0;

    if !points.is_null() {
        *n_points = points.offset_from(points) as u32;
    }

    CCX_FALSE
}

/* -------------- */

pub unsafe fn vbi3_raw_decoder_add_services(
    rd: *mut Vbi3RawDecoder,
    mut services: VbiServiceSet,
    strict: c_int,
) -> VbiServiceSet {
    assert!(!rd.is_null());

    services &= !(VBI_SLICED_VBI_525 | VBI_SLICED_VBI_625);

    if (*rd).services & services != 0 {
        info(
            &mut (*rd).log,
            "Already decoding services 0x{:08x}.",
            format_args!("{:08x}", (*rd).services & services),
        );
        services &= !(*rd).services;
    }

    if services == 0 {
        info(&mut (*rd).log, "No services to add.", format_args!(""));
        return (*rd).services;
    }

    if (*rd).pattern.is_null() {
        let scan_lines = (*rd).sampling.count[0] + (*rd).sampling.count[1];
        let scan_ways = scan_lines as usize * _VBI3_RAW_DECODER_MAX_WAYS;
        let size = scan_ways * std::mem::size_of::<i8>();

        (*rd).pattern = libc::malloc(size) as *mut i32;
        if (*rd).pattern.is_null() {
            error(&mut (*rd).log, "Out of memory.", format_args!(""));
            return (*rd).services;
        }

        std::ptr::write_bytes((*rd).pattern, 0, scan_ways);
    }

    let min_offset = if (*rd).sampling.scanning == 525 {
        7.9e-6
    } else {
        8.0e-6
    };

    let mut par = _VBI_SERVICE_TABLE.as_ptr();
    while (*par).id != 0 {
        if services & (*par).id == 0 {
            par = par.add(1);
            continue;
        }

        let mut job = (*rd).jobs.as_mut_ptr();
        let mut j = 0;

        while j < (*rd).n_jobs as usize {
            let id = (*job).id | (*par).id;

            if id & !VBI_SLICED_TELETEXT_B == 0
                || id & !VBI_SLICED_CAPTION_525 == 0
                || id & !VBI_SLICED_CAPTION_625 == 0
                || id & !(VBI_SLICED_VPS | VBI_SLICED_VPS_F2) == 0
            {
                break;
            }

            job = job.add(1);
            j += 1;
        }

        if j >= _VBI3_RAW_DECODER_MAX_JOBS {
            error(
                &mut (*rd).log,
                "Set 0x{:08x} exceeds number of simultaneously decodable services ({}).",
                format_args!("{:08x}, {}", services, _VBI3_RAW_DECODER_MAX_WAYS),
            );
            break;
        } else if j >= (*rd).n_jobs as usize {
            (*job).id = 0;
        }

        let sp = &(*rd).sampling;

        if _vbi_sampling_par_check_services_log(sp, (*par).id, strict as u32, &mut (*rd).log) == 0 {
            par = par.add(1);
            continue;
        }

        let mut sample_offset = 0;

        if sp.offset > 0 && strict > 0 {
            let offset = sp.offset as f64 / sp.sampling_rate as f64;
            if offset < min_offset {
                sample_offset = (min_offset * sp.sampling_rate as f64) as u32;
            }
        }

        let cri_end = if (*par).id & VBI_SLICED_WSS_625 != 0 {
            !0
        } else {
            !0
        };

        let samples_per_line = sp.bytes_per_line;

        if _vbi3_bit_slicer_init(&mut (*job).slicer) == CCX_FALSE {
            assert!(false, "bit_slicer_init failed");
        }

        if vbi3_bit_slicer_set_params(
            &mut (*job).slicer,
            sp.sampling_format,
            sp.sampling_rate as u32,
            sample_offset,
            samples_per_line as u32,
            (*par).cri_frc >> (*par).frc_bits,
            (*par).cri_frc_mask >> (*par).frc_bits,
            (*par).cri_bits,
            (*par).cri_rate,
            cri_end,
            (*par).cri_frc & ((1 << (*par).frc_bits) - 1),
            (*par).frc_bits,
            (*par).payload,
            (*par).bit_rate,
            (*par).modulation.clone(),
        ) == CCX_FALSE {
            assert!(false, "bit_slicer_set_params failed");
        }

        let mut start = [0; 2];
        let mut count = [0; 2];
        lines_containing_data(&mut start, &mut count, sp, par);

        if add_job_to_pattern(rd, job.offset_from((*rd).jobs.as_mut_ptr()) as i32, start.as_ptr(), count.as_ptr()) == CCX_FALSE
        {
            error(
                &mut (*rd).log,
                "Out of decoder pattern space for service 0x{:08x} ({}).",
                format_args!("{:08x}, {}", (*par).id, std::ffi::CStr::from_ptr((*par).label).to_string_lossy()),
            );
            par = par.add(1);
            continue;
        }

        (*job).id |= (*par).id;

        if job.offset_from((*rd).jobs.as_mut_ptr()) as usize >= (*rd).n_jobs as usize {
            (*rd).n_jobs += 1;
        }

        (*rd).services |= (*par).id;
        par = par.add(1);
    }

    (*rd).services
}

pub unsafe fn add_job_to_pattern(
    rd: *mut Vbi3RawDecoder,
    job_num: i32,
    start: *const u32,
    count: *const u32,
) -> VbiBool {
    let mut job_num = job_num + 1; // index into rd->jobs, 0 means no job

    let scan_lines = (*rd).sampling.count[0] + (*rd).sampling.count[1];
    let pattern_end = (*rd).pattern.add(scan_lines as usize * _VBI3_RAW_DECODER_MAX_WAYS);

    for field in 0..2 {
        let mut pattern = (*rd).pattern.add((*start.add(field) as usize) * _VBI3_RAW_DECODER_MAX_WAYS);

        // For each line where we may find the data.
        for _ in 0..*count.add(field) {
            let mut free = 0;
            let mut dst = pattern;
            let end = pattern.add(_VBI3_RAW_DECODER_MAX_WAYS);

            let mut src = pattern;
            while src < end {
                let num = *src;
                src = src.add(1);

                if num <= 0 {
                    free += 1;
                    continue;
                } else {
                    free += (num == job_num) as u32;
                    *dst = num;
                    dst = dst.add(1);
                }
            }

            while dst < end {
                *dst = 0;
                dst = dst.add(1);
            }

            if free <= 1 {
                // Reserve a NULL way
                return CCX_FALSE;
            }

            pattern = end;
        }
    }

    for field in 0..2 {
        let mut pattern = (*rd).pattern.add((*start.add(field) as usize) * _VBI3_RAW_DECODER_MAX_WAYS);

        // For each line where we may find the data.
        for _ in 0..*count.add(field) {
            let mut way = 0;

            while unsafe { *pattern.add(way) } > 0 {
                if unsafe { *pattern.add(way) } == job_num {
                    break;
                }
                way += 1;
            }

            *pattern.add(way) = job_num;
            *pattern.add(_VBI3_RAW_DECODER_MAX_WAYS - 1) = -128;

            pattern = pattern.add(_VBI3_RAW_DECODER_MAX_WAYS);
        }
    }

    CCX_TRUE
}

pub unsafe fn _vbi_sampling_par_check_services_log(
    sp: *const VbiSamplingPar,
    services: VbiServiceSet,
    strict: u32,
    log: *mut VbiLogHook,
) -> VbiServiceSet {
    assert!(!sp.is_null());

    let mut rservices: VbiServiceSet = 0;

    let mut par = _VBI_SERVICE_TABLE.as_ptr();
    while (*par).id != 0 {
        if (*par).id & services == 0 {
            par = par.add(1);
            continue;
        }

        if _vbi_sampling_par_permit_service(sp, par, strict, log) == CCX_TRUE {
            rservices |= (*par).id;
        }

        par = par.add(1);
    }

    rservices
}

pub unsafe fn _vbi_sampling_par_permit_service(
    sp: *const VbiSamplingPar,
    par: *const VbiServicePar,
    strict: u32,
    log: *mut VbiLogHook,
) -> VbiBool {
    assert!(!sp.is_null());
    assert!(!par.is_null());

    let videostd_set = _vbi_videostd_set_from_scanning((*sp).scanning);
    if (*par).videostd_set & videostd_set == 0 {
        info(
            &mut *log,
            "Service 0x{:08x} ({}) requires videostd_set 0x{:x}, have 0x{:x}.",
            format_args!(
                "{:08x}, {}, {:x}, {:x}",
                (*par).id,
                std::ffi::CStr::from_ptr((*par).label).to_string_lossy(),
                (*par).videostd_set,
                videostd_set
            ),
        );
        return CCX_FALSE;
    }

    if (*par).flags as u32 & VbiServiceParFlag::LineNum as u32 != 0 {
        if ((*par).first[0] > 0 && (*sp).start[0] as u32 == 0)
            || ((*par).first[1] > 0 && (*sp).start[1] as u32 == 0)
        {
            info(
                &mut *log,
                "Service 0x{:08x} ({}) requires known line numbers.",
                format_args!(
                    "{:08x}, {}",
                    (*par).id,
                    std::ffi::CStr::from_ptr((*par).label).to_string_lossy()
                ),
            );
            return CCX_FALSE;
        }
    }

    let rate = (*par).cri_rate.max((*par).bit_rate);
    let adjusted_rate = if (*par).id == VBI_SLICED_WSS_625 {
        rate
    } else {
        (rate * 3) >> 1
    };

    if adjusted_rate > (*sp).sampling_rate as u32 {
        info(
            &mut *log,
            "Sampling rate {:.2} MHz too low for service 0x{:08x} ({}).",
            format_args!(
                "{:.2}, {:08x}, {}",
                (*sp).sampling_rate as f64 / 1e6,
                (*par).id,
                std::ffi::CStr::from_ptr((*par).label).to_string_lossy()
            ),
        );
        return CCX_FALSE;
    }

    let signal = (*par).cri_bits as f64 / (*par).cri_rate as f64
        + ((*par).frc_bits + (*par).payload) as f64 / (*par).bit_rate as f64;

    let samples_per_line = (*sp).bytes_per_line / vbi_pixfmt_bpp((*sp).sampling_format) as i32;

    if (*sp).offset > 0 && strict > 0 {
        let sampling_rate = (*sp).sampling_rate as f64;
        let offset = (*sp).offset as f64 / sampling_rate;
        let end = ((*sp).offset + samples_per_line) as f64 / sampling_rate;

        if offset > ((*par).offset as f64 / 1e3 - 0.5e-6) {
            info(
                &mut *log,
                "Sampling starts at 0H + {:.2} us, too late for service 0x{:08x} ({}) at {:.2} us.",
                format_args!(
                    "{:.2}, {:08x}, {}, {:.2}",
                    offset * 1e6,
                    (*par).id,
                    std::ffi::CStr::from_ptr((*par).label).to_string_lossy(),
                    (*par).offset as f64 / 1e3
                ),
            );
            return CCX_FALSE;
        }

        if end < ((*par).offset as f64 / 1e9 + signal + 0.5e-6) {
            info(
                &mut *log,
                "Sampling ends too early at 0H + {:.2} us for service 0x{:08x} ({}) which ends at {:.2} us.",
                format_args!(
                    "{:.2}, {:08x}, {}, {:.2}",
                    end * 1e6,
                    (*par).id,
                    std::ffi::CStr::from_ptr((*par).label).to_string_lossy(),
                    (*par).offset as f64 / 1e3 + signal * 1e6 + 0.5
                ),
            );
            return CCX_FALSE;
        }
    } else {
        let mut samples = samples_per_line as f64 / (*sp).sampling_rate as f64;

        if strict > 0 {
            samples -= 1e-6; // headroom
        }

        if samples < signal {
            info(
                &mut *log,
                "Service 0x{:08x} ({}) signal length {:.2} us exceeds {:.2} us sampling length.",
                format_args!(
                    "{:08x}, {}, {:.2}, {:.2}",
                    (*par).id,
                    std::ffi::CStr::from_ptr((*par).label).to_string_lossy(),
                    signal * 1e6,
                    samples * 1e6
                ),
            );
            return CCX_FALSE;
        }
    }

    if (*par).flags as u32 & VbiServiceParFlag::FieldNum as u32 != 0 && (*sp).synchronous == 0 {
        info(
            &mut *log,
            "Service 0x{:08x} ({}) requires synchronous field order.",
            format_args!(
                "{:08x}, {}",
                (*par).id,
                std::ffi::CStr::from_ptr((*par).label).to_string_lossy()
            ),
        );
        return CCX_FALSE;
    }

    for field in 0..2 {
        let start = (*sp).start[field] as u32;
        let end = start + (*sp).count[field] as u32 - 1;

        if (*par).first[field] == 0 || (*par).last[field] == 0 {
            // No data on this field.
            continue;
        }

        if (*sp).count[field] == 0 {
            info(
                &mut *log,
                "Service 0x{:08x} ({}) requires data from field {}.",
                format_args!(
                    "{:08x}, {}, {}",
                    (*par).id,
                    std::ffi::CStr::from_ptr((*par).label).to_string_lossy(),
                    field + 1
                ),
            );
            return CCX_FALSE;
        }

        if strict <= 0 || (*sp).start[field] == 0 {
            continue;
        }

        if strict == 1 && (*par).first[field] > (*par).last[field] {
            // May succeed if not all scanning lines available for the service are actually used.
            continue;
        }

        if start > (*par).first[field] || end < (*par).last[field] {
            info(
                &mut *log,
                "Service 0x{:08x} ({}) requires lines {}-{}, have {}-{}.",
                format_args!(
                    "{:08x}, {}, {}, {}, {}, {}",
                    (*par).id,
                    std::ffi::CStr::from_ptr((*par).label).to_string_lossy(),
                    (*par).first[field],
                    (*par).last[field],
                    start,
                    end
                ),
            );
            return CCX_FALSE;
        }
    }

    CCX_TRUE
}


// Implement Sync for VbiServicePar since it contains primitive types and pointers
// that don't mutate
unsafe impl Sync for VbiServicePar {}



pub static _VBI_SERVICE_TABLE: [VbiServicePar; 19+1] = [
    VbiServicePar {
        id: VBI_SLICED_TELETEXT_A,
        label: b"Teletext System A\0".as_ptr() as *const c_char,
        videostd_set: VBI_VIDEOSTD_SET_625_50,
        first: [6, 318],
        last: [22, 335],
        offset: 10500,
        cri_rate: 6203125,
        bit_rate: 6203125,
        cri_frc: 0x00AAAAE7,
        cri_frc_mask: 0xFFFF,
        cri_bits: 18,
        frc_bits: 6,
        payload: 37 * 8,
        modulation: VbiModulation::NrzLsb,
        flags: VbiServiceParFlag::FieldNum,
    },
    VbiServicePar {
        id: VBI_SLICED_TELETEXT_B_L10_625,
        label: b"Teletext System B 625 Level 1.5\0".as_ptr() as *const c_char,
        videostd_set: VBI_VIDEOSTD_SET_625_50,
        first: [7, 320],
        last: [22, 335],
        offset: 10300,
        cri_rate: 6937500,
        bit_rate: 6937500,
        cri_frc: 0x00AAAAE4,
        cri_frc_mask: 0xFFFF,
        cri_bits: 18,
        frc_bits: 6,
        payload: 42 * 8,
        modulation: VbiModulation::NrzLsb,
        flags: VbiServiceParFlag::FieldNum,
    },
    VbiServicePar {
        id: VBI_SLICED_TELETEXT_B,
        label: b"Teletext System B, 625\0".as_ptr() as *const c_char,
        videostd_set: VBI_VIDEOSTD_SET_625_50,
        first: [6, 318],
        last: [22, 335],
        offset: 10300,
        cri_rate: 6937500,
        bit_rate: 6937500,
        cri_frc: 0x00AAAAE4,
        cri_frc_mask: 0xFFFF,
        cri_bits: 18,
        frc_bits: 6,
        payload: 42 * 8,
        modulation: VbiModulation::NrzLsb,
        flags: VbiServiceParFlag::FieldNum,
    },
    VbiServicePar {
        id: VBI_SLICED_TELETEXT_C_625,
        label: b"Teletext System C 625\0".as_ptr() as *const c_char,
        videostd_set: VBI_VIDEOSTD_SET_625_50,
        first: [6, 318],
        last: [22, 335],
        offset: 10480,
        cri_rate: 5734375,
        bit_rate: 5734375,
        cri_frc: 0x00AAAAE7,
        cri_frc_mask: 0xFFFF,
        cri_bits: 18,
        frc_bits: 6,
        payload: 33 * 8,
        modulation: VbiModulation::NrzLsb,
        flags: VbiServiceParFlag::FieldNum,
    },
    VbiServicePar {
        id: VBI_SLICED_TELETEXT_D_625,
        label: b"Teletext System D 625\0".as_ptr() as *const c_char,
        videostd_set: VBI_VIDEOSTD_SET_625_50,
        first: [6, 318],
        last: [22, 335],
        offset: 10500,
        cri_rate: 5642787,
        bit_rate: 5642787,
        cri_frc: 0x00AAAAE5,
        cri_frc_mask: 0xFFFF,
        cri_bits: 18,
        frc_bits: 6,
        payload: 34 * 8,
        modulation: VbiModulation::NrzLsb,
        flags: VbiServiceParFlag::FieldNum,
    },
    VbiServicePar {
        id: VBI_SLICED_VPS,
        label: b"Video Program System\0".as_ptr() as *const c_char,
        videostd_set: VBI_VIDEOSTD_SET_PAL_BG,
        first: [16, 0],
        last: [16, 0],
        offset: 12500,
        cri_rate: 5000000,
        bit_rate: 2500000,
        cri_frc: 0xAAAA8A99,
        cri_frc_mask: 0xFFFFFF,
        cri_bits: 32,
        frc_bits: 0,
        payload: 13 * 8,
        modulation: VbiModulation::BiphaseMsb,
        flags: VbiServiceParFlag::FieldNum,
    },
    VbiServicePar {
        id: VBI_SLICED_VPS_F2,
        label: b"Pseudo-VPS on field 2\0".as_ptr() as *const c_char,
        videostd_set: VBI_VIDEOSTD_SET_PAL_BG,
        first: [0, 329],
        last: [0, 329],
        offset: 12500,
        cri_rate: 5000000,
        bit_rate: 2500000,
        cri_frc: 0xAAAA8A99,
        cri_frc_mask: 0xFFFFFF,
        cri_bits: 32,
        frc_bits: 0,
        payload: 13 * 8,
        modulation: VbiModulation::BiphaseMsb,
        flags: VbiServiceParFlag::FieldNum,
    },
    VbiServicePar {
        id: VBI_SLICED_WSS_625,
        label: b"Wide Screen Signalling 625\0".as_ptr() as *const c_char,
        videostd_set: VBI_VIDEOSTD_SET_625_50,
        first: [23, 0],
        last: [23, 0],
        offset: 11000,
        cri_rate: 5000000,
        bit_rate: 833333,
        cri_frc: 0x8E3C783E,
        cri_frc_mask: 0x2499339C,
        cri_bits: 32,
        frc_bits: 0,
        payload: 14 * 1,
        modulation: VbiModulation::BiphaseLsb,
        flags: VbiServiceParFlag::LineAndField,
    },
    VbiServicePar {
        id: VBI_SLICED_CAPTION_625_F1,
        label: b"Closed Caption 625, field 1\0".as_ptr() as *const c_char,
        videostd_set: VBI_VIDEOSTD_SET_625_50,
        first: [22, 0],
        last: [22, 0],
        offset: 10500,
        cri_rate: 1000000,
        bit_rate: 500000,
        cri_frc: 0x00005551,
        cri_frc_mask: 0x7FF,
        cri_bits: 14,
        frc_bits: 2,
        payload: 2 * 8,
        modulation: VbiModulation::NrzLsb,
        flags: VbiServiceParFlag::FieldNum,
    },
    VbiServicePar {
        id: 0,
        label: std::ptr::null(),
        videostd_set: VBI_VIDEOSTD_SET_EMPTY,
        first: [0, 0],
        last: [0, 0],
        offset: 0,
        cri_rate: 0,
        bit_rate: 0,
        cri_frc: 0,
        cri_frc_mask: 0,
        cri_bits: 0,
        frc_bits: 0,
        payload: 0,
        modulation: VbiModulation::NrzLsb,
        flags: VbiServiceParFlag::FieldNum,
    },
    VbiServicePar {
        id: VBI_SLICED_CAPTION_625_F2,
        label: b"Closed Caption 625, field 2\0".as_ptr() as *const c_char,
        videostd_set: VBI_VIDEOSTD_SET_625_50,
        first: [0, 335],
        last: [0, 335],
        offset: 10500,
        cri_rate: 1000000,
        bit_rate: 500000,
        cri_frc: 0x00005551,
        cri_frc_mask: 0x7FF,
        cri_bits: 14,
        frc_bits: 2,
        payload: 2 * 8,
        modulation: VbiModulation::NrzLsb,
        flags: VbiServiceParFlag::FieldNum,
    },
    VbiServicePar {
        id: VBI_SLICED_VBI_625,
        label: b"VBI 625\0".as_ptr() as *const c_char,
        videostd_set: VBI_VIDEOSTD_SET_625_50,
        first: [6, 318],
        last: [22, 335],
        offset: 10000,
        cri_rate: 1510000,
        bit_rate: 1510000,
        cri_frc: 0,
        cri_frc_mask: 0,
        cri_bits: 0,
        frc_bits: 0,
        payload: 10 * 8,
        modulation: VbiModulation::NrzLsb,
        flags: VbiServiceParFlag::FieldNum,
    },
    VbiServicePar {
        id: VBI_SLICED_TELETEXT_B_525,
        label: b"Teletext System B 525\0".as_ptr() as *const c_char,
        videostd_set: VBI_VIDEOSTD_SET_525_60,
        first: [10, 272],
        last: [21, 284],
        offset: 10500,
        cri_rate: 5727272,
        bit_rate: 5727272,
        cri_frc: 0x00AAAAE4,
        cri_frc_mask: 0xFFFF,
        cri_bits: 18,
        frc_bits: 6,
        payload: 34 * 8,
        modulation: VbiModulation::NrzLsb,
        flags: VbiServiceParFlag::FieldNum,
    },
    VbiServicePar {
        id: VBI_SLICED_TELETEXT_C_525,
        label: b"Teletext System C 525\0".as_ptr() as *const c_char,
        videostd_set: VBI_VIDEOSTD_SET_525_60,
        first: [10, 272],
        last: [21, 284],
        offset: 10480,
        cri_rate: 5727272,
        bit_rate: 5727272,
        cri_frc: 0x00AAAAE7,
        cri_frc_mask: 0xFFFF,
        cri_bits: 18,
        frc_bits: 6,
        payload: 33 * 8,
        modulation: VbiModulation::NrzLsb,
        flags: VbiServiceParFlag::FieldNum,
    },
    VbiServicePar {
        id: VBI_SLICED_TELETEXT_D_525,
        label: b"Teletext System D 525\0".as_ptr() as *const c_char,
        videostd_set: VBI_VIDEOSTD_SET_525_60,
        first: [10, 272],
        last: [21, 284],
        offset: 9780,
        cri_rate: 5727272,
        bit_rate: 5727272,
        cri_frc: 0x00AAAAE5,
        cri_frc_mask: 0xFFFF,
        cri_bits: 18,
        frc_bits: 6,
        payload: 34 * 8,
        modulation: VbiModulation::NrzLsb,
        flags: VbiServiceParFlag::FieldNum,
    },
    VbiServicePar {
        id: VBI_SLICED_CAPTION_525_F1,
        label: b"Closed Caption 525, field 1\0".as_ptr() as *const c_char,
        videostd_set: VBI_VIDEOSTD_SET_525_60,
        first: [21, 0],
        last: [21, 0],
        offset: 10500,
        cri_rate: 1006976,
        bit_rate: 503488,
        cri_frc: 0x03,
        cri_frc_mask: 0x0F,
        cri_bits: 4,
        frc_bits: 0,
        payload: 2 * 8,
        modulation: VbiModulation::NrzLsb,
        flags: VbiServiceParFlag::LineAndField,
    },
    VbiServicePar {
        id: VBI_SLICED_CAPTION_525_F2,
        label: b"Closed Caption 525, field 2\0".as_ptr() as *const c_char,
        videostd_set: VBI_VIDEOSTD_SET_525_60,
        first: [0, 284],
        last: [0, 284],
        offset: 10500,
        cri_rate: 1006976,
        bit_rate: 503488,
        cri_frc: 0x03,
        cri_frc_mask: 0x0F,
        cri_bits: 4,
        frc_bits: 0,
        payload: 2 * 8,
        modulation: VbiModulation::NrzLsb,
        flags: VbiServiceParFlag::LineAndField,
    },
    VbiServicePar {
        id: VBI_SLICED_2xCAPTION_525,
        label: b"2xCaption 525\0".as_ptr() as *const c_char,
        videostd_set: VBI_VIDEOSTD_SET_525_60,
        first: [10, 0],
        last: [21, 0],
        offset: 10500,
        cri_rate: 1006976,
        bit_rate: 1006976,
        cri_frc: 0x000554ED,
        cri_frc_mask: 0xFFFF,
        cri_bits: 12,
        frc_bits: 8,
        payload: 4 * 8,
        modulation: VbiModulation::NrzLsb,
        flags: VbiServiceParFlag::FieldNum,
    },
    VbiServicePar {
        id: VBI_SLICED_VBI_525,
        label: b"VBI 525\0".as_ptr() as *const c_char,
        videostd_set: VBI_VIDEOSTD_SET_525_60,
        first: [10, 272],
        last: [21, 284],
        offset: 9500,
        cri_rate: 1510000,
        bit_rate: 1510000,
        cri_frc: 0,
        cri_frc_mask: 0,
        cri_bits: 0,
        frc_bits: 0,
        payload: 10 * 8,
        modulation: VbiModulation::NrzLsb,
        flags: VbiServiceParFlag::FieldNum,
    },
    VbiServicePar {
        id: 0,
        label: std::ptr::null(),
        videostd_set: VBI_VIDEOSTD_SET_EMPTY,
        first: [0, 0],
        last: [0, 0],
        offset: 0,
        cri_rate: 0,
        bit_rate: 0,
        cri_frc: 0,
        cri_frc_mask: 0,
        cri_bits: 0,
        frc_bits: 0,
        payload: 0,
        modulation: VbiModulation::NrzLsb,
        flags: VbiServiceParFlag::FieldNum,
    },
];

pub fn decode_vbi(
    dec_ctx: *mut LibCcDecode,
    field: u8,
    buffer: *mut u8,
    len: usize,
    sub: *mut CcSubtitle,
) -> i32 {
    unsafe {
        let mut sliced: [VbiSliced; 52] = std::mem::zeroed();

        if (*dec_ctx).vbi_decoder.is_null() {
            (*dec_ctx).vbi_decoder = init_decoder_vbi(None).unwrap();
        }

        let len = len - 720;

        let n_lines = vbi_raw_decode(
            &mut (*(*dec_ctx).vbi_decoder).zvbi_decoder,
            buffer,
            sliced.as_mut_ptr(),
        );

        if n_lines > 0 {
            for i in 0..n_lines as usize {
                let mut data: [u8; 3] = [0; 3];
                data[0] = if field == 1 { 0x04 } else { 0x05 };
                data[1] = sliced[i].data[0];
                data[2] = sliced[i].data[1];
                do_cb(dec_ctx, data.as_ptr(), sub);
            }
        }

        0
    }
}


pub unsafe fn vbi_raw_decode(
    rd: *mut VbiRawDecoder,
    raw: *mut u8,
    out: *mut VbiSliced,
) -> u32 {
    assert!(!rd.is_null());
    assert!(!raw.is_null());
    assert!(!out.is_null());

    let rd3 = (*rd).pattern as *mut Vbi3RawDecoder;
    let mut n_lines = ((*rd).count[0] + (*rd).count[1]) as u32;

    n_lines = vbi3_raw_decoder_decode(rd3, out, n_lines, raw);

    n_lines
}

extern "C" {
    pub static mut stderr: *mut libc::FILE;
}

pub unsafe fn vbi3_raw_decoder_decode(
    rd: *mut Vbi3RawDecoder,
    mut sliced: *mut VbiSliced,
    max_lines: u32,
    mut raw: *const u8,
) -> u32 {
    if (*rd).services == 0 {
        return 0;
    }

    let sp = &(*rd).sampling;
    let scan_lines = (sp.count[0] + sp.count[1]) as u32;
    let pitch = (sp.bytes_per_line << sp.interlaced) as usize;

    let mut pattern = (*rd).pattern;
    let raw1 = raw;

    let mut sliced_begin = sliced;
    let sliced_end = sliced.add(max_lines as usize);

    if RAW_DECODER_PATTERN_DUMP {
        _vbi3_raw_decoder_dump(rd, stderr);
    }

    for i in 0..scan_lines {
        if sliced >= sliced_end {
            break;
        }

        if sp.interlaced != 0 && i == sp.count[0] as u32 {
            raw = raw1.add(sp.bytes_per_line as usize);
        }

        sliced = decode_pattern(rd, sliced, pattern as *mut i8, i, raw);

        pattern = pattern.add(_VBI3_RAW_DECODER_MAX_WAYS);
        raw = raw.add(pitch);
    }

    (*rd).readjust = ((*rd).readjust + 1) & 15;

    sliced.offset_from(sliced_begin) as u32
}

pub unsafe fn _vbi3_raw_decoder_dump(rd: *const Vbi3RawDecoder, fp: *mut libc::FILE) {
    assert!(!fp.is_null());

    libc::fprintf(fp, b"vbi3_raw_decoder %p\n\0".as_ptr() as *const i8, rd);

    if rd.is_null() {
        return;
    }

    libc::fprintf(
        fp,
        b"  services 0x%08x\n\0".as_ptr() as *const i8,
        (*rd).services,
    );

    for i in 0..(*rd).n_jobs as usize {
        libc::fprintf(
            fp,
            b"  job %u: 0x%08x (%s)\n\0".as_ptr() as *const i8,
            i + 1,
            (*rd).jobs[i].id,
            vbi_sliced_name((*rd).jobs[i].id),
        );
    }

    if (*rd).pattern.is_null() {
        libc::fprintf(fp, b"  no pattern\n\0".as_ptr() as *const i8);
        return;
    }

    let sp = &(*rd).sampling;

    for i in 0..((sp.count[0] + sp.count[1]) as usize) {
        libc::fputs(b"  \0".as_ptr() as *const i8, fp);
        dump_pattern_line(rd, i as u32, fp);
    }
}

pub unsafe fn vbi_sliced_name(service: VbiServiceSet) -> *const c_char {
    // Handle ambiguous cases
    if service == VBI_SLICED_CAPTION_525 {
        return b"Closed Caption 525\0".as_ptr() as *const c_char;
    }
    if service == VBI_SLICED_CAPTION_625 {
        return b"Closed Caption 625\0".as_ptr() as *const c_char;
    }
    if service == (VBI_SLICED_VPS | VBI_SLICED_VPS_F2) {
        return b"Video Program System\0".as_ptr() as *const c_char;
    }
    if service == VBI_SLICED_TELETEXT_B_L25_625 {
        return b"Teletext System B 625 Level 2.5\0".as_ptr() as *const c_char;
    }

    // Incorrect, no longer in table
    if service == VBI_SLICED_TELETEXT_BD_525 {
        return b"Teletext System B/D\0".as_ptr() as *const c_char;
    }

    // Lookup in the service table
    if let Some(par) = find_service_par(service) {
        return par.label;
    }

    std::ptr::null()
}

pub unsafe fn dump_pattern_line(rd: *const Vbi3RawDecoder, row: u32, fp: *mut libc::FILE) {
    assert!(!rd.is_null());
    assert!(!fp.is_null());

    let sp = &(*rd).sampling;
    let mut line: u32;

    if sp.interlaced != 0 {
        let field = row & 1;

        if sp.start[field as usize] == 0 {
            line = 0;
        } else {
            line = sp.start[field as usize] as u32 + (row >> 1);
        }
    } else {
        if row >= sp.count[0] as u32 {
            if sp.start[1] == 0 {
                line = 0;
            } else {
                line = sp.start[1] as u32 + row as u32 - sp.count[0] as u32;
            }
        } else {
            if sp.start[0] == 0 {
                line = 0;
            } else {
                line = sp.start[0] as u32 + row;
            }
        }
    }

    libc::fprintf(fp, b"scan line %3u: \0".as_ptr() as *const i8, line);

    for i in 0.._VBI3_RAW_DECODER_MAX_WAYS {
        let pos = row as usize * _VBI3_RAW_DECODER_MAX_WAYS + i;
        libc::fprintf(fp, b"%02x \0".as_ptr() as *const i8, *((*rd).pattern.add(pos) as *const u8) as libc::c_int);
    }

    libc::fputc(b'\n' as i32, fp);
}

pub unsafe fn find_service_par(service: VbiServiceSet) -> Option<&'static VbiServicePar> {
    for par in _VBI_SERVICE_TABLE.iter() {
        if par.id == 0 {
            break; // End of table
        }
        if par.id == service {
            return Some(par);
        }
    }
    None
}

pub unsafe fn decode_pattern(
    rd: *mut Vbi3RawDecoder,
    mut sliced: *mut VbiSliced,
    pattern: *mut i8,
    i: u32,
    raw: *const u8,
) -> *mut VbiSliced {
    let sp = &(*rd).sampling;
    let mut pat = pattern;

    loop {
        let mut j = *pat;

        if j > 0 {
            let job = &(*rd).jobs[(j - 1) as usize];

            if !slice(rd, sliced, job as *const _Vbi3RawDecoderJob as *mut _Vbi3RawDecoderJob, i, raw) {
                pat = pat.add(1);
                continue; // No match, try next data service
            }

            // Positive match, output decoded line
            (*sliced).id = job.id;
            (*sliced).line = 0;

            if i >= sp.count[0] as u32 {
                if sp.synchronous != 0 && sp.start[1] != 0 {
                    (*sliced).line = sp.start[1] as u32 + i - sp.count[0] as u32;
                }
            } else {
                if sp.synchronous != 0 && sp.start[0] != 0 {
                    (*sliced).line = sp.start[0] as u32 + i;
                }
            }

            sliced = sliced.add(1);

            // Predict line as non-blank, force testing for all data services in the next 128 frames
            *pattern.add(_VBI3_RAW_DECODER_MAX_WAYS - 1) = -128;
        } else if pat == pattern {
            // Line was predicted as blank, once in 16 frames look for data services
            if (*rd).readjust == 0 {
                let size = std::mem::size_of::<i8>() * (_VBI3_RAW_DECODER_MAX_WAYS - 1);

                j = *pattern;
                std::ptr::copy(pattern.add(1), pattern, size);
                *pattern.add(_VBI3_RAW_DECODER_MAX_WAYS - 1) = j;
            }

            break;
        } else if *pattern.add(_VBI3_RAW_DECODER_MAX_WAYS - 1) < 0 {
            // Increment counter, when zero predict line as blank and stop looking for data services
            break;
        }

        // Try the found data service first next time
        *pat = *pattern;
        *pattern = j;

        break; // Line done
    }

    sliced
}

// should call already defined rust function do_cb from c code
extern "C" {
    pub fn do_cb(
        ctx: *mut LibCcDecode,
        cc_block: *const u8,
        sub: *mut CcSubtitle,
    ) -> i32;
}

pub unsafe fn slice(
    rd: *mut Vbi3RawDecoder,
    sliced: *mut VbiSliced,
    job: *mut _Vbi3RawDecoderJob,
    i: u32,
    raw: *const u8,
) -> bool {
    if (*rd).debug != 0 && !(*rd).sp_lines.is_null() {
        return vbi3_bit_slicer_slice_with_points(
            &mut (*job).slicer,
            (*sliced).data.as_mut_ptr(),
            std::mem::size_of_val(&(*sliced).data) as u32,
            (*rd).sp_lines.add(i as usize).as_mut().unwrap().points.as_mut_ptr(),
            &mut (*rd).sp_lines.add(i as usize).as_mut().unwrap().n_points,
            (*rd).sp_lines.add(i as usize).as_ref().unwrap().points.len() as u32,
            raw,
        ) != 0;
    } else {
        return vbi3_bit_slicer_slice(
            &mut (*job).slicer,
            (*sliced).data.as_mut_ptr(),
            std::mem::size_of_val(&(*sliced).data) as u32,
            raw,
        ) != 0;
    }
}

pub unsafe fn vbi3_bit_slicer_slice_with_points(
    bs: *mut Vbi3BitSlicer,
    buffer: *mut u8,
    buffer_size: u32,
    points: *mut Vbi3BitSlicerPoint,
    n_points: *mut u32,
    max_points: u32,
    raw: *const u8,
) -> VbiBool {
    const PIXFMT: VbiPixfmt = VbiPixfmt::Yuv420;
    const BPP: u32 = 1;
    const OVERSAMPLING: u32 = 4;
    const THRESH_FRAC: u32 = DEF_THR_FRAC;
    const COLLECT_POINTS: bool = true;

    assert!(!bs.is_null());
    assert!(!buffer.is_null());
    assert!(!points.is_null());
    assert!(!n_points.is_null());
    assert!(!raw.is_null());

    let points_start = points;
    *n_points = 0;

    if (*bs).payload > buffer_size * 8 {
        warning(
            &mut (*bs).log,
            "buffer_size {} < {} bits of payload.",
            format_args!("{}, {}", buffer_size * 8, (*bs).payload),
        );
        return CCX_FALSE;
    }

    if (*bs).total_bits > max_points {
        warning(
            &mut (*bs).log,
            "max_points {} < {} CRI, FRC and payload bits.",
            format_args!("{}, {}", max_points, (*bs).total_bits),
        );
        return CCX_FALSE;
    }

    if (*bs).func == Some(low_pass_bit_slicer_Y8) {
        return (*bs).func.unwrap()(bs, buffer, points, n_points, raw);
    } else if (*bs).func != Some(bit_slicer_Y8) {
        warning(
            &mut (*bs).log,
            "Function not implemented for pixfmt {}.",
            format_args!("{}", (*bs).sample_format as u32),
        );
        return (*bs).func.unwrap()(bs, buffer, std::ptr::null_mut(), std::ptr::null_mut(), raw);
    }

    core(bs, buffer, points, n_points, raw);

    CCX_TRUE
}

pub unsafe fn core(
    bs: *mut Vbi3BitSlicer,
    buffer: *mut u8,
    points: *mut Vbi3BitSlicerPoint,
    n_points: *mut u32,
    raw: *const u8,
) -> VbiBool {
    let mut raw_start = raw;
    let mut raw = raw.add((*bs).skip as usize);

    let mut thresh0 = (*bs).thresh;
    let mut cl = 0u32; // clock
    let mut c = 0u32; // current byte
    let mut b1 = 0u8; // previous bit

    for mut i in (0..(*bs).cri_samples).rev() {
        let tr = (*bs).thresh >> DEF_THR_FRAC;
        let raw0 = green(raw, (*bs).sample_format as u32, &*bs);
        let mut raw1 = green(raw.add((*bs).bytes_per_sample as usize), (*bs).sample_format as u32, &*bs);
        raw1 = raw1.wrapping_sub(raw0);
        (*bs).thresh = (*bs).thresh.wrapping_add(
            ((raw0 as i32 - tr as i32) * (raw1 as i32).abs()) as u32,
        );
        let mut t = raw0 * 4; // oversampling

        for _ in 0..4 {
            let oversampling = 4; // Define oversampling with an appropriate value
            let collect_points = true; // Define collect_points with an appropriate value
            cri(bs, buffer, points, n_points, t, raw, raw_start, &mut cl, &mut c, &mut b1, tr, oversampling, collect_points);
        }

        raw = raw.add((*bs).bytes_per_sample as usize);
    }

    (*bs).thresh = thresh0;

    if true {
        // collect_points
        *n_points = points.offset_from(points) as u32;
    }

    CCX_FALSE
}

pub unsafe fn green2(raw: *const u8, endian: usize, bs: &Vbi3BitSlicer) -> u32 {
    ((*raw.add(0 + endian) as u32) + ((*raw.add(1 - endian) as u32) << 8)) & bs.green_mask
}

pub unsafe fn green(raw: *const u8, pixfmt: u32, bs: &Vbi3BitSlicer) -> u32 {
    match pixfmt {
        VBI_PIXFMT_RGB8 => (*raw as u32) & bs.green_mask,
        VBI_PIXFMT_RGB16_LE => (*(raw as *const u16) as u32) & bs.green_mask,
        VBI_PIXFMT_RGB16_BE => green2(raw, 1, bs),
        _ => *raw as u32,
    }
}

pub const VBI_PIXFMT_RGB8: u32 = 101;

pub unsafe fn cri(
    bs: *mut Vbi3BitSlicer,
    buffer: *mut u8,
    mut points: *mut Vbi3BitSlicerPoint,
    n_points: *mut u32,
    mut t: u32,
    raw: *const u8,
    raw_start: *const u8,
    mut cl: &mut u32,
    mut c: &mut u32,
    mut b1: &mut u8,
    tr: u32,
    oversampling: u32,
    collect_points: bool,
) -> VbiBool {
    let tavg = (t + (oversampling / 2)) / oversampling;
    let b = (tavg >= tr) as u8;

    if b != *b1 {
        *cl = (*bs).oversampling_rate >> 1;
    } else {
        *cl += (*bs).cri_rate;

        if *cl >= (*bs).oversampling_rate {
            if collect_points {
                (*points).kind = Vbi3BitSlicerBit::CRI_BIT;
                (*points).index = ((raw as usize - raw_start as usize) << 8) as u32;
                (*points).level = tavg << 8;
                (*points).thresh = tr << 8;
                points = points.add(1);
            }

            *cl -= (*bs).oversampling_rate;
            *c = *c * 2 + b as u32;

            if (*c & (*bs).cri_mask) == (*bs).cri {
                payload(bs, buffer, points, n_points, (*bs).phase_shift, *c, tr, raw);
                if collect_points {
                    *n_points = points.offset_from(points) as u32;
                }
                return CCX_TRUE;
            }
        }
    }

    *b1 = b;

    if oversampling > 1 {
        t += green(raw.add((*bs).bytes_per_sample as usize), (*bs).sample_format as u32, &*bs);
    }

    CCX_FALSE
}

pub unsafe fn payload(
    bs: *mut Vbi3BitSlicer,
    mut buffer: *mut u8,
    points: *mut Vbi3BitSlicerPoint,
    n_points: *mut u32,
    mut i: u32,
    mut c: u32,
    mut tr: u32,
    raw: *const u8,
) -> VbiBool {
    tr *= 256;

    // Process FRC bits
    for _ in 0..(*bs).frc_bits {
        sample(bs, points, n_points, Vbi3BitSlicerBit::FRC_BIT, i, raw, tr);
        c = c * 2 + ((*raw >= tr as u8) as u32);
        i += (*bs).step;
    }

    if c != (*bs).frc {
        return CCX_FALSE;
    }

    // Process payload bits based on endianness
    match (*bs).endian {
        3 => {
            // Bitwise, LSB first
            for j in 0..(*bs).payload {
                sample(bs, points, n_points, Vbi3BitSlicerBit::PAYLOAD_BIT, i, raw, tr);
                c = (c >> 1) + ((*raw >= tr as u8) as u32 * 128);
                i += (*bs).step;
                if (j & 7) == 7 {
                    *buffer = c as u8;
                    buffer = buffer.add(1);
                }
            }
            *buffer = (c >> ((8 - (*bs).payload) & 7)) as u8;
        }
        2 => {
            // Bitwise, MSB first
            for j in 0..(*bs).payload {
                sample(bs, points, n_points, Vbi3BitSlicerBit::PAYLOAD_BIT, i, raw, tr);
                c = c * 2 + ((*raw >= tr as u8) as u32);
                i += (*bs).step;
                if (j & 7) == 7 {
                    *buffer = c as u8;
                    buffer = buffer.add(1);
                }
            }
            *buffer = (c & ((1 << ((*bs).payload & 7)) - 1)) as u8;
        }
        1 => {
            // Octets, LSB first
            for _ in 0..(*bs).payload {
                for k in 0..8 {
                    sample(bs, points, n_points, Vbi3BitSlicerBit::PAYLOAD_BIT, i, raw, tr);
                    c += ((*raw >= tr as u8) as u32) << k;
                    i += (*bs).step;
                }
                *buffer = c as u8;
                buffer = buffer.add(1);
            }
        }
        _ => {
            // Octets, MSB first
            for _ in 0..(*bs).payload {
                for _ in 0..8 {
                    sample(bs, points, n_points, Vbi3BitSlicerBit::PAYLOAD_BIT, i, raw, tr);
                    c = c * 2 + ((*raw >= tr as u8) as u32);
                    i += (*bs).step;
                }
                *buffer = c as u8;
                buffer = buffer.add(1);
            }
        }
    }

    CCX_TRUE
}

pub unsafe fn sample(
    bs: *mut Vbi3BitSlicer,
    mut points: *mut Vbi3BitSlicerPoint,
    n_points: *mut u32,
    kind: Vbi3BitSlicerBit,
    i: u32,
    raw: *const u8,
    tr: u32,
) {
    let bpp = (*bs).bytes_per_sample as usize;
    let r = raw.add((i >> 8) as usize * bpp);

    let mut raw0 = green(r, (*bs).sample_format as u32, &*bs);
    let raw1 = green(r.add(bpp), (*bs).sample_format as u32, &*bs);

    raw0 = ((raw1 as i32 - raw0 as i32) * (i & 255) as i32 + (raw0 << 8) as i32) as u32;

    if !points.is_null() {
        (*points).kind = kind;
        (*points).index = (((raw as usize - raw as usize) as u32) * 256 + i) as u32;
        (*points).level = raw0;
        (*points).thresh = tr;
        points = points.add(1);
    }
}

pub unsafe fn vbi3_bit_slicer_slice(
    bs: *mut Vbi3BitSlicer,
    buffer: *mut u8,
    buffer_size: u32,
    raw: *const u8,
) -> VbiBool {
    assert!(!bs.is_null());
    assert!(!buffer.is_null());
    assert!(!raw.is_null());

    if (*bs).payload > buffer_size * 8 {
        warning(
            &mut (*bs).log,
            "buffer_size {} < {} bits of payload.",
            format_args!("{}, {}", buffer_size * 8, (*bs).payload),
        );
        return CCX_FALSE;
    }

    if let Some(func) = (*bs).func {
        func(bs, buffer, std::ptr::null_mut(), std::ptr::null_mut(), raw)
    } else {
        CCX_FALSE
    }
}


pub static mut CB_FIELD1: i64 = 0;
pub static mut CB_FIELD2: i64 = 0;
pub static mut CB_708: i64 = 0;

pub unsafe fn get_fts(ctx: *mut CcxCommonTimingCtx, current_field: i32) -> i64 {
    match current_field {
        1 => (*ctx).fts_now + (*ctx).fts_global + (CB_FIELD1 * 1001 / 30),
        2 => (*ctx).fts_now + (*ctx).fts_global + (CB_FIELD2 * 1001 / 30),
        3 => (*ctx).fts_now + (*ctx).fts_global + (CB_708 * 1001 / 30),
        _ => {
            panic!("get_fts: unhandled branch");
        }
    }
}

