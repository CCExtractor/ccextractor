use log::info;

use crate::decode_vbi::exit_codes::*;
use crate::decode_vbi::structs_ccdecode::*;
use crate::decode_vbi::structs_isdb::*;
use crate::decode_vbi::structs_vbi::*;
use crate::decode_vbi::structs_xds::*;

use crate::common::OutputFormat; // ccxoutputformat

use crate::common::StreamMode; // //CcxStreamModeEnum

use crate::common::Codec; // ccxcodetype - 2 options{Codec, SelectCodec} - use appropriately

use crate::time::TimestampFormat; // ccxoutputdateformat

use crate::time::TimingContext; // ccxcommontimingctx

use crate::time::Timestamp; // ccx_boundary_time

use std::os::raw::{c_char, c_int, c_long, c_uchar, c_uint, c_void};

use crate::common::BufferdataType; // ccxbufferdatatype

use crate::common::FrameType; // ccxframetype

use crate::util::log::GuiXdsMessage; // CcxCommonLoggingGui

// | `fatal`, `ccx_common_logging.fatal_ftn`                                                                                                  | [`fatal!`]                  |
// | `mprint`, `ccx_common_logging.log_ftn`                                                                                                   | [`info!`]                   |
// | `dbg_print`, `ccx_common_logging.debug_ftn`                                                                                              | [`debug!`]                  |
// | `activity_library_process`, `ccx_common_logging.gui_ftn`                                                                                 | [`send_gui`]                |
// #[repr(C)]
// pub struct CcxCommonLogging {
//     pub debug_mask: i64, // Equivalent to LLONG in C
//     pub fatal_ftn: Option<unsafe extern "C" fn(exit_code: i32, fmt: *const c_char, ...)>,
//     pub debug_ftn: Option<unsafe extern "C" fn(mask: i64, fmt: *const c_char, ...)>,
//     pub log_ftn: Option<unsafe extern "C" fn(fmt: *const c_char, ...)>,
//     pub gui_ftn: Option<unsafe extern "C" fn(message_type: CcxCommonLoggingGui, ...)>,
// }

//-------------------vbi-functions-start-------------------------

pub fn vbi3_raw_decoder_debug(rd: &mut Vbi3RawDecoder, enable: bool) -> bool {
    let mut _sp_lines: Option<Vec<Vbi3RawDecoderSpLine>> = None; // Not used in the original code
    let mut result = true;

    rd.debug = enable;

    let mut n_lines = 0;
    if enable {
        n_lines = rd.sampling.count[0] + rd.sampling.count[1];
    }

    match rd.sampling.sampling_format {
        VbiPixfmt::Yuv420 => {}
        _ => {
            // Not implemented
            n_lines = 0;
            result = false;
        }
    }

    if rd.n_sp_lines == n_lines as i64 {
        return result;
    }

    rd.sp_lines.clear();
    rd.n_sp_lines = 0;

    if n_lines > 0 {
        rd.sp_lines = vec![Vbi3RawDecoderSpLine::default(); n_lines as usize];
        rd.n_sp_lines = n_lines as i64;
    }

    result
}

pub fn vbi3_raw_decoder_reset(rd: &mut Vbi3RawDecoder) {
    assert!(!rd.pattern.is_empty());

    rd.pattern.clear();
    rd.services = 0;
    rd.n_jobs = 0;
    rd.readjust = 1;

    rd.jobs
        .iter_mut()
        .for_each(|job| *job = Vbi3RawDecoderJob::default());
}

pub unsafe fn _vbi3_raw_decoder_destroy(rd: *mut Vbi3RawDecoder) {
    vbi3_raw_decoder_reset(&mut *rd);

    vbi3_raw_decoder_debug(&mut *rd, false);

    // Make unusable
    std::ptr::write_bytes(rd, 0, 1);
}

pub fn vbi3_raw_decoder_delete(rd: Option<Box<Vbi3RawDecoder>>) {
    if let Some(rd) = rd {
        unsafe {
            _vbi3_raw_decoder_destroy(Box::into_raw(rd));
        }
    }
}

pub unsafe fn vbi_raw_decoder_destroy(rd: *mut VbiRawDecoder) {
    assert!(!rd.is_null());

    let rd3 = (*rd).pattern as *mut Vbi3RawDecoder;

    vbi3_raw_decoder_delete(if rd3.is_null() {
        None
    } else {
        Some(Box::from_raw(rd3))
    });

    std::ptr::write_bytes(rd, 0, 1);
}

pub fn vbi_pixfmt_bpp(fmt: VbiPixfmt) -> i32 {
    match fmt {
        VbiPixfmt::Yuv420 => 1,
        VbiPixfmt::Rgba32Le | VbiPixfmt::Rgba32Be | VbiPixfmt::Bgra32Le | VbiPixfmt::Bgra32Be => 4,
        VbiPixfmt::Rgb24 | VbiPixfmt::Bgr24 => 3,
        _ => 2,
    }
}
