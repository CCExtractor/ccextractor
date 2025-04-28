use crate::common::OutputFormat; // ccxoutputformat
use crate::time::TimingContext; // ccxcommontimingctx
use crate::util::log::{error, info}; // all kinds of logging

use crate::decoder_vbi::exit_codes::*;
use crate::decoder_vbi::functions_vbi_slice::*;
use crate::decoder_vbi::structs_ccdecode::*;
use crate::decoder_vbi::structs_isdb::*;
use crate::decoder_vbi::structs_vbi_decode::*;
use crate::decoder_vbi::structs_vbi_slice::*;
use crate::decoder_vbi::structs_xds::*;
use crate::decoder_vbi::vbi_service_table::*;

use std::os::raw::c_void;
use std::ptr::null_mut;

pub fn vbi_raw_decoder_destroy(rd: &mut VbiRawDecoder) {
    unsafe {
        if !rd.pattern.is_null() {
            let rd3 = rd.pattern as *mut Vbi3RawDecoder;
            vbi3_raw_decoder_delete(rd3);
        }
        rd.pattern = null_mut();
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_vbi_raw_decoder_destroy_null_pattern() {
        let mut decoder = VbiRawDecoder::default();

        vbi_raw_decoder_destroy(&mut decoder);

        assert!(decoder.pattern.is_null());
    }
}

pub fn vbi_raw_decoder_init(rd: &mut VbiRawDecoder) {
    assert!(!rd.pattern.is_null(), "Decoder cannot be null");

    let rd3 = vbi3_raw_decoder_new(None);
    assert!(rd3.is_some(), "Failed to initialize vbi3_raw_decoder");

    rd.pattern = Box::into_raw(rd3.unwrap()) as *mut i8;
}

pub fn vbi_raw_decoder_add_services(rd: &mut VbiRawDecoder, services: u32, strict: i32) -> u32 {
    assert!(!rd.pattern.is_null(), "Decoder pattern cannot be null");

    let rd3 = unsafe { &mut *(rd.pattern as *mut Vbi3RawDecoder) };
    let mut service_set = services;

    vbi3_raw_decoder_set_sampling_par(rd3, &*rd, strict);

    service_set = vbi3_raw_decoder_add_services(rd3, service_set, strict);

    service_set
}

type VbiVideostdSet = u64;

#[inline]
fn vbi_pixfmt_bytes_per_pixel(fmt: VbiPixfmt) -> u32 {
    match fmt {
        VbiPixfmt::Yuv420 => 1,
        VbiPixfmt::Rgba32Le | VbiPixfmt::Rgba32Be | VbiPixfmt::Bgra32Le | VbiPixfmt::Bgra32Be => 4,
        VbiPixfmt::Rgb24 | VbiPixfmt::Bgr24 => 3,
        _ => 2,
    }
}

#[inline]
pub fn range_check(start: u32, count: u32, min: u32, max: u32) -> bool {
    start >= min && (start + count) <= max && (start + count) >= start
}

pub fn _vbi_videostd_set_from_scanning(scanning: i32) -> VbiVideostdSet {
    match scanning {
        525 => VBI_VIDEOSTD_SET_525_60,
        625 => VBI_VIDEOSTD_SET_625_50,
        _ => 0,
    }
}

pub fn _vbi_sampling_par_valid_log(sp: &VbiSamplingPar, _log: &VbiLogHook) -> VbiBool {
    // log unused
    assert!(!sp.pattern.is_null(), "Sampling parameters cannot be null");

    match sp.sampling_format {
        VbiPixfmt::Yuv420 => {}
        _ => {
            let bpp = vbi_pixfmt_bytes_per_pixel(sp.sampling_format);
            if sp.bytes_per_line % bpp as i32 != 0 {
                info!(
                    "bytes_per_line value {} is no multiple of the sample size {}.",
                    sp.bytes_per_line, bpp
                );
                return 0;
            }
        }
    }

    if sp.bytes_per_line == 0 {
        info!("samples_per_line is zero.");
        return 0;
    }

    if sp.count[0] == 0 && sp.count[1] == 0 {
        info!(
            "Invalid VBI scan range {}-{} ({} lines), {}-{} ({} lines).",
            sp.start[0],
            sp.start[0] + sp.count[0] - 1,
            sp.count[0],
            sp.start[1],
            sp.start[1] + sp.count[1] - 1,
            sp.count[1]
        );
        return 0;
    }

    let videostd_set = _vbi_videostd_set_from_scanning(sp.scanning);

    if videostd_set & VBI_VIDEOSTD_SET_525_60 != 0 {
        if videostd_set & VBI_VIDEOSTD_SET_625_50 != 0 {
            info!("Ambiguous videostd_set 0x{:x}.", videostd_set);
            return 0;
        }

        if sp.start[0] != 0 && !range_check(sp.start[0] as u32, sp.count[0] as u32, 1, 262) {
            info!(
                "Invalid VBI scan range {}-{} ({} lines), {}-{} ({} lines).",
                sp.start[0],
                sp.start[0] + sp.count[0] - 1,
                sp.count[0],
                sp.start[1],
                sp.start[1] + sp.count[1] - 1,
                sp.count[1]
            );
            return 0;
        }

        if sp.start[1] != 0 && !range_check(sp.start[1] as u32, sp.count[1] as u32, 263, 525) {
            info!(
                "Invalid VBI scan range {}-{} ({} lines), {}-{} ({} lines).",
                sp.start[0],
                sp.start[0] + sp.count[0] - 1,
                sp.count[0],
                sp.start[1],
                sp.start[1] + sp.count[1] - 1,
                sp.count[1]
            );
            return 0;
        }
    } else if videostd_set & VBI_VIDEOSTD_SET_625_50 != 0 {
        if sp.start[0] != 0 && !range_check(sp.start[0] as u32, sp.count[0] as u32, 1, 311) {
            info!(
                "Invalid VBI scan range {}-{} ({} lines), {}-{} ({} lines).",
                sp.start[0],
                sp.start[0] + sp.count[0] - 1,
                sp.count[0],
                sp.start[1],
                sp.start[1] + sp.count[1] - 1,
                sp.count[1]
            );
            return 0;
        }

        if sp.start[1] != 0 && !range_check(sp.start[1] as u32, sp.count[1] as u32, 312, 625) {
            info!(
                "Invalid VBI scan range {}-{} ({} lines), {}-{} ({} lines).",
                sp.start[0],
                sp.start[0] + sp.count[0] - 1,
                sp.count[0],
                sp.start[1],
                sp.start[1] + sp.count[1] - 1,
                sp.count[1]
            );
            return 0;
        }
    } else {
        info!("Ambiguous videostd_set 0x{:x}.", videostd_set);
        return 0;
    }

    if sp.interlaced != 0 && (sp.count[0] != sp.count[1] || sp.count[0] == 0) {
        info!(
            "Line counts {}, {} must be equal and non-zero when raw VBI data is interlaced.",
            sp.count[0], sp.count[1]
        );
        return 0;
    }

    1 // true
}

pub extern "C" fn null_function(
    bs: *mut Vbi3BitSlicer,
    buffer: *mut u8,
    points: *mut Vbi3BitSlicerPoint,
    _n_points: *mut u32, // mark unused
    _raw: *const u8,     // mark unused
) -> i32 {
    let _ = (bs, buffer, points); // unused
    info!("vbi3_bit_slicer_set_params() not called.");

    0 // false
}

pub fn lines_containing_data(
    start: &mut [u32; 2],
    count: &mut [u32; 2],
    sp: &VbiRawDecoder,
    par: &VbiServicePar,
) {
    start[0] = 0;
    start[1] = sp.count[0] as u32;

    count[0] = sp.count[0] as u32;
    count[1] = sp.count[1] as u32;

    if sp.synchronous == 0 {
        return;
    }

    for field in 0..2 {
        if par.first[field] == 0 || par.last[field] == 0 {
            count[field] = 0;
            continue;
        }

        let mut first = sp.start[field] as u32;
        let mut last = first + sp.count[field] as u32 - 1;

        if first > 0 && sp.count[field] > 0 {
            assert!(par.first[field] <= par.last[field]);

            if par.first[field] > last || par.last[field] < first {
                continue;
            }

            first = first.max(par.first[field]);
            last = last.min(par.last[field]);

            start[field] += first - sp.start[field] as u32;
            count[field] = last + 1 - first;
        }
    }
}

pub fn add_job_to_pattern(
    rd: &mut VbiRawDecoder,
    job_num: i32,
    start: &[u32; 2],
    count: &[u32; 2],
) -> bool {
    let job_num = job_num + 1;
    let scan_lines = rd.count[0] as u32 + rd.count[1] as u32;

    let _pattern_end = unsafe {
        // unused variable
        rd.pattern
            .add(scan_lines as usize * _VBI3_RAW_DECODER_MAX_WAYS)
    };

    for field in 0..2 {
        let mut pattern = unsafe {
            rd.pattern
                .add(start[field] as usize * _VBI3_RAW_DECODER_MAX_WAYS)
        };

        for _ in 0..count[field] {
            let mut free = 0;
            let mut dst = pattern;
            let end = unsafe { pattern.add(_VBI3_RAW_DECODER_MAX_WAYS) };

            for src in unsafe { std::slice::from_raw_parts(pattern, _VBI3_RAW_DECODER_MAX_WAYS) } {
                if *src <= 0 {
                    free += 1;
                } else {
                    free += (*src == job_num as i8) as u32;
                    unsafe { *dst = *src };
                    dst = unsafe { dst.add(1) };
                }
            }

            while dst < end {
                unsafe {
                    *dst = 0;
                    dst = dst.add(1);
                }
            }

            if free <= 1 {
                return false;
            }

            pattern = unsafe { pattern.add(_VBI3_RAW_DECODER_MAX_WAYS) };
        }
    }

    for field in 0..2 {
        let mut pattern = unsafe {
            rd.pattern
                .add(start[field] as usize * _VBI3_RAW_DECODER_MAX_WAYS)
        };

        for _ in 0..count[field] {
            let mut way = 0;

            while unsafe { *pattern.add(way) } > 0 {
                if unsafe { *pattern.add(way) } == job_num as i8 {
                    break;
                }
                way += 1;
            }

            unsafe {
                *pattern.add(way) = job_num as i8;
                *pattern.add(_VBI3_RAW_DECODER_MAX_WAYS - 1) = -128;
                pattern = pattern.add(_VBI3_RAW_DECODER_MAX_WAYS);
            }
        }
    }

    true
}

fn _vbi_sampling_par_permit_service(
    sp: &VbiSamplingPar,
    par: &VbiServicePar,
    strict: u32,
    _log: &VbiLogHook, // unused variable
) -> VbiBool {
    const UNKNOWN: u32 = 0;
    let mut _field: usize; // unused variable
    let _samples_per_line: u32; // unused variable
    let videostd_set: VbiVideostdSet = _vbi_videostd_set_from_scanning(sp.scanning);

    assert!(!sp.pattern.is_null());
    assert!(!par.label.is_null());
    if par.videostd_set & videostd_set == 0 {
        info!(
            "Service 0x{:08x} ({:?}) requires videostd_set 0x{:x}, have 0x{:x}.",
            par.id,
            unsafe { std::ffi::CStr::from_ptr(par.label).to_string_lossy() },
            par.videostd_set,
            videostd_set
        );
        return CCX_FALSE as i32;
    }

    if par.flags as u32 & VbiServiceParFlag::LineNum as u32 != 0
        && ((par.first[0] > 0 && sp.start[0] as u32 == UNKNOWN)
            || (par.first[1] > 0 && sp.start[1] as u32 == UNKNOWN))
    {
        info!(
            "Service 0x{:08x} ({:?}) requires known line numbers.",
            par.id,
            unsafe { std::ffi::CStr::from_ptr(par.label).to_string_lossy() }
        );
        return CCX_FALSE as i32;
    }

    let mut rate = par.cri_rate.max(par.bit_rate);

    match par.id {
        VBI_SLICED_WSS_625 => {
            // effective bit rate is just 1/3 max_rate, so 1 * max_rate should suffice
        }
        _ => {
            rate = (rate * 3) >> 1;
        }
    }

    if rate > sp.sampling_rate as u32 {
        info!(
            "Sampling rate {:.2} MHz too low for service 0x{:08x} ({:?}).",
            sp.sampling_rate as f64 / 1e6,
            par.id,
            unsafe { std::ffi::CStr::from_ptr(par.label).to_string_lossy() }
        );
        return CCX_FALSE as i32;
    }

    let signal: f64 = par.cri_bits as f64 / par.cri_rate as f64
        + (par.frc_bits + par.payload) as f64 / par.bit_rate as f64;

    let samples_per_line: u32 =
        sp.bytes_per_line as u32 / vbi_pixfmt_bytes_per_pixel(sp.sampling_format);

    if sp.offset > 0 && strict > 0 {
        let sampling_rate = sp.sampling_rate as f64;
        let offset = sp.offset as f64 / sampling_rate;
        let end = ((sp.offset as u32 + samples_per_line) as f64) / sampling_rate;

        if offset > (par.offset as f64 / 1e3 - 0.5e-6) {
            info!(
                "Sampling starts at 0H + {:.2} us, too late for service 0x{:08x} ({:?}) at {:.2} us.",
                offset * 1e6,
                par.id,
                unsafe { std::ffi::CStr::from_ptr(par.label).to_string_lossy() },
                par.offset as f64 / 1e3
            );
            return CCX_FALSE as i32;
        }

        if end < (par.offset as f64 / 1e3 + signal + 0.5e-6) {
            info!(
                "Sampling ends too early at 0H + {:.2} us for service 0x{:08x} ({:?}) which ends at {:.2} us.",
                end * 1e6,
                par.id,
                unsafe { std::ffi::CStr::from_ptr(par.label).to_string_lossy() },
                par.offset as f64 / 1e3 + signal * 1e6 + 0.5
            );
            return CCX_FALSE as i32;
        }
    } else {
        let mut samples = samples_per_line as f64 / sp.sampling_rate as f64;

        if strict > 0 {
            samples -= 1e-6; // headroom
        }

        if samples < signal {
            info!(
                "Service 0x{:08x} ({:?}) signal length {:.2} us exceeds {:.2} us sampling length.",
                par.id,
                unsafe { std::ffi::CStr::from_ptr(par.label).to_string_lossy() },
                signal * 1e6,
                samples * 1e6
            );
            return CCX_FALSE as i32;
        }
    }

    if par.flags as u32 & VbiServiceParFlag::FieldNum as u32 != 0 {
        info!(
            "Service 0x{:08x} ({}) requires synchronous field order.",
            par.id,
            unsafe { std::ffi::CStr::from_ptr(par.label).to_string_lossy() }
        );
        return CCX_FALSE as i32;
    }

    for field in 0..2 {
        let start = sp.start[field] as u32;
        let end = start + sp.count[field] as u32 - 1;

        if par.first[field] == 0 || par.last[field] == 0 {
            // no data on this field
            continue;
        }

        if sp.count[field] == 0 {
            info!(
                "Service 0x{:08x} ({:?}) requires data from field {}.",
                par.id,
                unsafe { std::ffi::CStr::from_ptr(par.label).to_string_lossy() },
                field + 1
            );
            return CCX_FALSE as i32;
        }

        if strict as i32 <= 0 || sp.start[field] == 0 {
            continue;
        }

        if strict == 1 && par.first[field] > par.last[field] {
            // may succeed if not all scanning lines available for the service are actually used
            continue;
        }

        if start > par.first[field] || end < par.last[field] {
            info!(
                "Service 0x{:08x} ({:?}) requires lines {}-{}, have {}-{}.",
                par.id,
                unsafe { std::ffi::CStr::from_ptr(par.label).to_string_lossy() },
                par.first[field],
                par.last[field],
                start,
                end
            );
            return CCX_FALSE as i32;
        }
    }

    CCX_TRUE as i32
}

pub fn _vbi_sampling_par_check_services_log(
    sp: &VbiSamplingPar,
    services: VbiServiceSet,
    strict: u32,
    log: &VbiLogHook,
) -> VbiServiceSet {
    assert!(!sp.pattern.is_null());

    let mut rservices: VbiServiceSet = 0;

    for par in VBI_SERVICE_TABLE.iter() {
        if par.id == 0 {
            break;
        }

        if par.id & services == 0 {
            continue;
        }

        if _vbi_sampling_par_permit_service(sp, par, strict, log) != 0 {
            rservices |= par.id;
        }
    }

    rservices
}

pub fn vbi3_raw_decoder_add_services(
    rd: &mut Vbi3RawDecoder,
    mut services: VbiServiceSet,
    strict: i32,
) -> VbiServiceSet {
    assert!(!rd.pattern.is_null());

    services &= !(VBI_SLICED_VBI_525 | VBI_SLICED_VBI_625);

    if rd.services & services != 0 {
        info!(
            "Already decoding services 0x{:08x}.",
            rd.services & services
        );
        services &= !rd.services;
    }

    if services == 0 {
        info!("No services to add.");
        return rd.services;
    }

    if rd.pattern.is_null() {
        let scan_lines = rd.sampling.count[0] + rd.sampling.count[1];
        let scan_ways = scan_lines as usize * _VBI3_RAW_DECODER_MAX_WAYS;
        let size = scan_ways * std::mem::size_of::<i8>();

        rd.pattern = unsafe {
            std::alloc::alloc_zeroed(std::alloc::Layout::array::<i8>(size).unwrap()) as *mut i8
        };
        if rd.pattern.is_null() {
            error!("Out of memory.");
            return rd.services;
        }

        unsafe {
            std::ptr::write_bytes(rd.pattern, 0, scan_ways);
        }
    }

    let min_offset = if rd.sampling.scanning == 525 {
        7.9e-6
    } else {
        8.0e-6
    };

    for par in VBI_SERVICE_TABLE.iter() {
        if par.id == 0 {
            break;
        }

        if par.id & services == 0 {
            continue;
        }

        // Find suitable job index instead of getting a direct mutable reference
        let job_index = rd.jobs.iter().position(|job| {
            let id = job.id | par.id;
            id & !VBI_SLICED_TELETEXT_B == 0
                || id & !VBI_SLICED_CAPTION_525 == 0
                || id & !VBI_SLICED_CAPTION_625 == 0
                || id & !(VBI_SLICED_VPS | VBI_SLICED_VPS_F2) == 0
        });

        let job_index = if let Some(index) = job_index {
            index
        } else {
            if rd.n_jobs as usize >= _VBI3_RAW_DECODER_MAX_JOBS {
                error!(
                    "Set 0x{:08x} exceeds number of simultaneously decodable services ({}).",
                    services, _VBI3_RAW_DECODER_MAX_WAYS
                );
                break;
            }
            let index = rd.n_jobs as usize;
            rd.n_jobs += 1;
            index
        };

        // Reset job ID
        rd.jobs[job_index].id = 0;

        if _vbi_sampling_par_check_services_log(&rd.sampling, par.id, strict as u32, &rd.log) == 0 {
            continue;
        }

        let mut sample_offset = 0;

        if rd.sampling.offset > 0 && strict > 0 {
            let offset = rd.sampling.offset as f64 / rd.sampling.sampling_rate as f64;
            if offset < min_offset {
                sample_offset = (min_offset * rd.sampling.sampling_rate as f64) as u32;
            }
        }

        let cri_end = u32::MAX;

        if !_vbi3_bit_slicer_init(&mut rd.jobs[job_index].slicer) {
            panic!("bit_slicer_init failed");
        }

        if !vbi3_bit_slicer_set_params(
            &mut rd.jobs[job_index].slicer,
            rd.sampling.sampling_format,
            rd.sampling.sampling_rate as u32,
            sample_offset,
            rd.sampling.bytes_per_line as u32,
            par.cri_frc >> par.frc_bits,
            par.cri_frc_mask >> par.frc_bits,
            par.cri_bits,
            par.cri_rate,
            cri_end,
            par.cri_frc & ((1 << par.frc_bits) - 1),
            par.frc_bits,
            par.payload,
            par.bit_rate,
            par.modulation,
        ) {
            panic!("bit_slicer_set_params failed");
        }

        let mut start = [0; 2];
        let mut count = [0; 2];
        lines_containing_data(&mut start, &mut count, &rd.sampling, par);

        // Use job_index instead of pointer arithmetic
        if !add_job_to_pattern(&mut rd.sampling, job_index as i32, &start, &count) {
            error!(
                "Out of decoder pattern space for service 0x{:08x} ({}).",
                par.id,
                unsafe { std::ffi::CStr::from_ptr(par.label).to_string_lossy() }
            );
            continue;
        }

        // Update job ID using the index
        rd.jobs[job_index].id |= par.id;
        rd.services |= par.id;
    }

    rd.services
}

//----------------------

pub fn vbi_raw_decode(rd: &mut VbiRawDecoder, raw: &[u8], out: &mut [VbiSliced]) -> u32 {
    assert!(!rd.pattern.is_null(), "rd.pattern cannot be null");
    assert!(!raw.is_empty(), "raw cannot be empty");
    assert!(!out.is_empty(), "out cannot be empty");

    let rd3 = unsafe { &mut *(rd.pattern as *mut Vbi3RawDecoder) };
    let n_lines = (rd.count[0] + rd.count[1]) as u32;

    vbi3_raw_decoder_decode(rd3, out, n_lines, raw)
}

pub fn decode_pattern(
    rd: &mut Vbi3RawDecoder,
    sliced: *mut VbiSliced,
    pattern: *mut i8,
    i: u32,
    raw: &[u8],
) -> *mut VbiSliced {
    let sp = rd.sampling;
    let mut pat = pattern;

    loop {
        let j = unsafe { *pat };

        if j > 0 {
            let job = &mut rd.jobs[(j - 1) as usize];

            // Convert _Vbi3RawDecoderJob to VbiRawDecoderJob if necessary
            let job = unsafe { &mut *(job as *mut _Vbi3RawDecoderJob as *mut VbiRawDecoderJob) };

            if !slice(rd, sliced, job, i, raw) {
                pat = unsafe { pat.add(1) };
                continue;
            }

            unsafe {
                (*sliced).id = job.id;
                (*sliced).line = 0;

                if i >= sp.count[0] as u32 {
                    if sp.synchronous != 0 && sp.start[1] != 0 {
                        (*sliced).line = sp.start[1] as u32 + i - sp.count[0] as u32;
                    }
                } else if sp.synchronous != 0 && sp.start[0] != 0 {
                    (*sliced).line = sp.start[0] as u32 + i;
                }
            }

            unsafe {
                let sliced = sliced.add(1);
                *pattern.add(_VBI3_RAW_DECODER_MAX_WAYS - 1) = -128;
                return sliced;
            }
        } else if pat == pattern {
            if rd.readjust == 0 {
                let size = _VBI3_RAW_DECODER_MAX_WAYS - 1;
                unsafe {
                    let j = *pattern;
                    std::ptr::copy(pattern.add(1), pattern, size);
                    *pattern.add(_VBI3_RAW_DECODER_MAX_WAYS - 1) = j;
                }
            }
            break;
        } else if unsafe { *pattern.add(_VBI3_RAW_DECODER_MAX_WAYS - 1) } < 0 {
            break;
        }

        unsafe {
            std::ptr::swap(pat, pattern);
        }

        break;
    }

    sliced
}

pub fn do_cb(ctx: &mut LibCcDecode, cc_block: &[u8], sub: &mut CcSubtitle) -> i32 {
    let cc_valid = (cc_block[0] & 4) >> 2;
    let cc_type = cc_block[0] & 3;
    let mut timeok = true;

    if ctx.fix_padding != 0 && cc_valid == 0 && cc_type <= 1 && cc_block[1] == 0 && cc_block[2] == 0
    {
        // Padding
        let mut cc_block_mut = cc_block.to_vec();
        cc_block_mut[0] |= 4; // Set cc_valid
        cc_block_mut[1] = 0x80;
        cc_block_mut[2] = 0x80;
    }

    if ctx.write_format != OutputFormat::Raw
        && ctx.write_format != OutputFormat::DvdRaw
        && (cc_block[0] == 0xFA || cc_block[0] == 0xFC || cc_block[0] == 0xFD)
        && (cc_block[1] & 0x7F) == 0
        && (cc_block[2] & 0x7F) == 0
    {
        return 1;
    }

    if cc_valid != 0 || cc_type == 3 {
        ctx.cc_stats[cc_type as usize] += 1;

        match cc_type {
            0 => {
                ctx.current_field = 1;
                ctx.saw_caption_block = 1;

                if let Some(start) = ctx.extraction_start {
                    if get_fts(ctx.timing, ctx.current_field) < start.millis() {
                        // used millis in place of time_in_ms
                        timeok = false;
                    }
                }
                if let Some(end) = ctx.extraction_end {
                    if get_fts(ctx.timing, ctx.current_field) > end.millis() {
                        // used millis in place of time_in_ms
                        timeok = false;
                        ctx.processed_enough = 1;
                    }
                }
                if timeok {
                    if ctx.write_format != OutputFormat::Rcwt {
                        printdata(ctx, Some(&cc_block[1..3]), None, sub);
                    } else {
                        writercwtdata(ctx, Some(cc_block), sub);
                    }
                }
            }
            1 => {
                ctx.current_field = 2;
                ctx.saw_caption_block = 1;

                if let Some(start) = ctx.extraction_start {
                    if get_fts(ctx.timing, ctx.current_field) < start.millis() {
                        // used millis in place of time_in_ms
                        timeok = false;
                    }
                }
                if let Some(end) = ctx.extraction_end {
                    if get_fts(ctx.timing, ctx.current_field) > end.millis() {
                        // used millis in place of time_in_ms
                        timeok = false;
                        ctx.processed_enough = 1;
                    }
                }
                if timeok {
                    if ctx.write_format != OutputFormat::Rcwt {
                        printdata(ctx, Some(&cc_block[1..3]), None, sub);
                    } else {
                        writercwtdata(ctx, Some(cc_block), sub);
                    }
                }
            }
            2 | 3 => {
                ctx.current_field = 3;

                if let Some(start) = ctx.extraction_start {
                    if get_fts(ctx.timing, ctx.current_field) < start.millis() {
                        // used millis in place of time_in_ms
                        timeok = false;
                    }
                }
                if let Some(end) = ctx.extraction_end {
                    if get_fts(ctx.timing, ctx.current_field) > end.millis() {
                        // used millis in place of time_in_ms
                        timeok = false;
                        ctx.processed_enough = 1;
                    }
                }
                if timeok {
                    if ctx.write_format != OutputFormat::Rcwt {
                        // Do nothing - Rust 708 is used
                    } else {
                        writercwtdata(ctx, Some(cc_block), sub);
                    }
                }
            }
            _ => {
                panic!("Impossible value for cc_type. Please file a bug report.");
            }
        }
    } else {
        info!("Found !(cc_valid || cc_type == 3) - ignoring this block");
    }

    1
}

const DVD_HEADER: [u8; 8] = [0x00, 0x00, 0x01, 0xb2, 0x43, 0x43, 0x01, 0xf8];
const LC1: [u8; 1] = [0x8a];
const LC2: [u8; 1] = [0x8f];
const LC3: [u8; 2] = [0x16, 0xfe];
const LC4: [u8; 2] = [0x1e, 0xfe];
const LC5: [u8; 1] = [0xff];
const LC6: [u8; 1] = [0xfe];

pub fn write_dvd_raw(data1: &[u8], data2: &[u8], sub: &mut CcSubtitle) {
    static mut LOOP_COUNT: i32 = 1;
    static mut DATA_COUNT: i32 = 0;

    unsafe {
        if DATA_COUNT == 0 {
            writeraw(&DVD_HEADER, sub);
            if LOOP_COUNT == 1 {
                writeraw(&LC1, sub);
            }
            if LOOP_COUNT == 2 {
                writeraw(&LC2, sub);
            }
            if LOOP_COUNT == 3 {
                writeraw(&LC3, sub);
                if !data2.is_empty() {
                    writeraw(data2, sub);
                }
            }
            if LOOP_COUNT > 3 {
                writeraw(&LC4, sub);
                if !data2.is_empty() {
                    writeraw(data2, sub);
                }
            }
        }

        DATA_COUNT += 1;
        writeraw(&LC5, sub);
        if !data1.is_empty() {
            writeraw(data1, sub);
        }

        if (LOOP_COUNT == 1 && DATA_COUNT < 5)
            || (LOOP_COUNT == 2 && DATA_COUNT < 8)
            || (LOOP_COUNT == 3 && DATA_COUNT < 11)
            || (LOOP_COUNT > 3 && DATA_COUNT < 15)
        {
            writeraw(&LC6, sub);
            if !data2.is_empty() {
                writeraw(data2, sub);
            }
        } else {
            if LOOP_COUNT == 1 {
                writeraw(&LC6, sub);
                if !data2.is_empty() {
                    writeraw(data2, sub);
                }
            }
            LOOP_COUNT += 1;
            DATA_COUNT = 0;
        }
    }
}

pub fn writeraw(data: &[u8], sub: &mut CcSubtitle) -> i32 {
    // Don't do anything for empty data
    if data.is_empty() {
        return -1;
    }

    // Resize the subtitle data buffer
    let new_size = sub.nb_data as usize + data.len();
    let mut new_data = if sub.data.is_null() {
        Vec::with_capacity(new_size)
    } else {
        unsafe {
            let existing_data = Vec::from_raw_parts(
                sub.data as *mut u8,
                sub.nb_data as usize,
                sub.nb_data as usize,
            );
            let mut resized_data = existing_data;
            resized_data.reserve(data.len());
            resized_data
        }
    };

    // Append the new data
    new_data.extend_from_slice(data);

    // Update subtitle metadata
    sub.data = new_data.as_mut_ptr() as *mut std::ffi::c_void;
    sub.datatype = SubDataType::Generic;
    sub.got_output = true;
    sub.nb_data += data.len() as u32;
    sub.subtype = SubType::Raw;

    std::mem::forget(new_data); // Prevent Vec from being dropped

    0 // EXIT SUCCESS
}

pub unsafe extern "C" fn writeraw_wrapper(
    data: *const u8,
    len: i32,
    _ctx: *mut c_void, // ctx unused
    sub: *mut CcSubtitle,
) -> i32 {
    if data.is_null() || sub.is_null() {
        return -1; // Error: Null pointer
    }

    let data_slice = std::slice::from_raw_parts(data, len as usize);
    let sub_ref = &mut *sub;

    writeraw(data_slice, sub_ref)
}

pub fn printdata(
    ctx: &mut LibCcDecode,
    data1: Option<&[u8]>,
    data2: Option<&[u8]>,
    sub: &mut CcSubtitle,
) {
    if ctx.write_format == OutputFormat::DvdRaw {
        if let Some(data1) = data1 {
            write_dvd_raw(data1, data2.unwrap_or(&[]), sub);
        }
    } else {
        // Broadcast raw or any non-raw
        if let Some(data1) = data1 {
            if ctx.extract != 2 {
                ctx.current_field = 1;
                if let Some(writedata) = ctx.writedata {
                    writedata(
                        data1.as_ptr(),
                        data1.len() as i32,
                        ctx as *mut _ as *mut c_void,
                        sub,
                    );
                }
            }
        }
        if let Some(data2) = data2 {
            ctx.current_field = 2;
            if ctx.extract != 1 {
                if let Some(writedata) = ctx.writedata {
                    writedata(
                        data2.as_ptr(),
                        data2.len() as i32,
                        ctx as *mut _ as *mut c_void,
                        sub,
                    );
                }
            } else if ctx.writedata.map(|f| f as usize) != Some(writeraw_wrapper as usize) {
                if let Some(writedata) = ctx.writedata {
                    writedata(
                        data2.as_ptr(),
                        data2.len() as i32,
                        ctx as *mut _ as *mut c_void,
                        sub,
                    );
                }
            }
        }
    }
}

pub fn writercwtdata(ctx: &mut LibCcDecode, data: Option<&[u8]>, sub: &mut CcSubtitle) {
    static mut PREV_FTS: i64 = -1;
    static mut CBCOUNT: u16 = 0;
    static mut CBEMPTY: i32 = 0;
    static mut CBBUFFER: [u8; 0xFFFF * 3] = [0; 0xFFFF * 3];
    static mut CBHEADER: [u8; 10] = [0; 10];

    unsafe {
        let curr_fts = get_fts(ctx.timing, ctx.current_field);

        let prev_fts: *mut i64 = &raw mut PREV_FTS;
        let cbcount: *mut u16 = &raw mut CBCOUNT;
        let cbempty: *mut i32 = &raw mut CBEMPTY;
        let cbbuffer: *mut [u8; 0xFFFF * 3] = &raw mut CBBUFFER;
        let cbheader: *mut [u8; 10] = &raw mut CBHEADER;

        // Handle writing stored data when timestamp changes, end of data, or buffer full
        if (*prev_fts != curr_fts && *prev_fts != -1) || data.is_none() || *cbcount == 0xFFFF {
            if *cbcount != 0xFFFF {
                let store_cbcount = *cbcount;

                for cb in (0..(*cbcount) as i32).rev() {
                    let cc_valid = ((*cbbuffer)[3 * cb as usize] & 4) >> 2;
                    let cc_type = (*cbbuffer)[3 * cb as usize] & 3;

                    // Skip NTSC padding packets or unused packets
                    if (cc_valid != 0
                        && cc_type <= 1
                        && ctx.fullbin == 0
                        && (*cbbuffer)[3 * cb as usize + 1] == 0x80
                        && (*cbbuffer)[3 * cb as usize + 2] == 0x80)
                        || !(cc_valid != 0 || cc_type == 3)
                    {
                        *cbcount -= 1;
                    } else {
                        break;
                    }
                }

                info!(
                    "{:?} Write {} RCWT blocks - skipped {} padding / {} unused blocks.",
                    *prev_fts,
                    *cbcount,
                    store_cbcount - *cbcount,
                    *cbempty
                );
            }

            // Prepare and write data header
            (*cbheader)[0..8].copy_from_slice(&(*prev_fts).to_le_bytes());
            (*cbheader)[8..10].copy_from_slice(&(*cbcount).to_le_bytes());

            if *cbcount > 0 {
                if let Some(writedata) = ctx.writedata {
                    writedata((*cbheader).as_ptr(), 10, ctx.context_cc608_field_1, sub);
                    writedata(
                        (*cbbuffer).as_ptr(),
                        3 * (*cbcount) as i32,
                        ctx.context_cc608_field_1,
                        sub,
                    );
                }
            }

            *cbcount = 0;
            *cbempty = 0;
        }

        if let Some(data_slice) = data {
            // Store new data while FTS is unchanged
            let cc_valid = (data_slice[0] & 4) >> 2;
            let cc_type = data_slice[0] & 3;

            // Only store non-empty packets
            if cc_valid != 0 || cc_type == 3 {
                (*cbbuffer)[(*cbcount as usize) * 3..(*cbcount as usize) * 3 + 3]
                    .copy_from_slice(&data_slice[0..3]);
                *cbcount += 1;
            } else {
                *cbempty += 1;
            }
        } else {
            // Write final padding blocks for field 1 and 2
            let adjusted_fts = curr_fts - 1001 / 30;

            (*cbheader)[0..8].copy_from_slice(&adjusted_fts.to_le_bytes());
            *cbcount = 2;
            (*cbheader)[8..10].copy_from_slice(&(*cbcount).to_le_bytes());

            // Field 1 and 2 padding
            (*cbbuffer)[0..3].copy_from_slice(&[0x04, 0x80, 0x80]);
            (*cbbuffer)[3..6].copy_from_slice(&[0x05, 0x80, 0x80]);

            if let Some(writedata) = ctx.writedata {
                writedata((*cbheader).as_ptr(), 10, ctx.context_cc608_field_1, sub);
                writedata(
                    (*cbbuffer).as_ptr(),
                    3 * (*cbcount) as i32,
                    ctx.context_cc608_field_1,
                    sub,
                );
            }

            *cbcount = 0;
            *cbempty = 0;

            info!("{:?} Write final padding RCWT blocks.", adjusted_fts);
        }

        *prev_fts = curr_fts;
    }
}

pub fn get_fts(ctx: *mut TimingContext, current_field: i32) -> i64 {
    if ctx.is_null() {
        error!("get_fts: ctx is null");
        return 0;
    }

    let ctx = unsafe { &mut *ctx };

    let base_ts = ctx.fts_now + ctx.fts_global;

    let field_offset = match current_field {
        1 => 0,         // Field 1
        2 => 1001 / 60, // Field 2 (half frame later)
        3 => 0,         // 708 field
        _ => {
            error!(
                "get_fts: unhandled branch: current_field = {}",
                current_field
            );
            return 0;
        }
    };

    base_ts.millis() + field_offset
}
