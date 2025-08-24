use crate::bindings::{ccx_encoding_type, ccx_output_format};
use lib_ccxr::common::OutputFormat;
use lib_ccxr::encoder::txt_helpers::get_str_basic;
use lib_ccxr::util::encoding::Encoding;
use std::os::raw::{c_int, c_uchar};

pub mod common;
pub mod g608;
pub mod simplexml;
/// # Safety
/// This function is unsafe because it deferences to raw pointers and performs operations on pointer slices.
#[no_mangle]
pub unsafe fn ccxr_get_str_basic(
    out_buffer: *mut c_uchar,
    in_buffer: *mut c_uchar,
    trim_subs: c_int,
    in_enc: ccx_encoding_type,
    out_enc: ccx_encoding_type,
    max_len: c_int,
) -> c_int {
    let trim_subs_bool = trim_subs != 0;
    let in_encoding = match Encoding::from_ctype(in_enc) {
        Some(enc) => enc,
        None => return 0,
    };
    let out_encoding = match Encoding::from_ctype(out_enc) {
        Some(enc) => enc,
        None => return 0,
    };
    let in_buffer_slice = if in_buffer.is_null() || max_len <= 0 {
        return 0;
    } else {
        std::slice::from_raw_parts(in_buffer, max_len as usize)
    };
    let mut out_buffer_vec = Vec::with_capacity(max_len as usize * 4); // UTF-8 can be up to 4 bytes per character
    let result = get_str_basic(
        &mut out_buffer_vec,
        in_buffer_slice,
        trim_subs_bool,
        in_encoding,
        out_encoding,
        max_len,
    );
    if result > 0 && !out_buffer.is_null() {
        let copy_len = std::cmp::min(result as usize, out_buffer_vec.len());
        if copy_len > 0 {
            std::ptr::copy_nonoverlapping(out_buffer_vec.as_ptr(), out_buffer, copy_len);
        }
        if copy_len < max_len as usize {
            *out_buffer.add(copy_len) = 0;
        }
    } else if !out_buffer.is_null() {
        *out_buffer = 0;
    }
    result
}
pub trait FromCType<T> {
    // Remove after demuxer, just import from ctorust
    /// # Safety
    /// This function is unsafe because it uses raw pointers to get data from C types.
    unsafe fn from_ctype(c_value: T) -> Option<Self>
    where
        Self: Sized;
}
impl FromCType<ccx_encoding_type> for Encoding {
    // Remove after demuxer, just import from ctorust
    unsafe fn from_ctype(encoding: ccx_encoding_type) -> Option<Self> {
        Some(match encoding {
            0 => Encoding::UCS2,   // CCX_ENC_UNICODE
            1 => Encoding::Latin1, // CCX_ENC_LATIN_1
            2 => Encoding::UTF8,   // CCX_ENC_UTF_8
            3 => Encoding::Line21, // CCX_ENC_ASCII
            _ => Encoding::UTF8,   // Default to UTF-8 if unknown
        })
    }
}
impl FromCType<ccx_output_format> for OutputFormat {
    unsafe fn from_ctype(format: ccx_output_format) -> Option<Self> {
        Some(match format {
            ccx_output_format::CCX_OF_RAW => OutputFormat::Raw,
            ccx_output_format::CCX_OF_SRT => OutputFormat::Srt,
            ccx_output_format::CCX_OF_SAMI => OutputFormat::Sami,
            ccx_output_format::CCX_OF_TRANSCRIPT => OutputFormat::Transcript,
            ccx_output_format::CCX_OF_RCWT => OutputFormat::Rcwt,
            ccx_output_format::CCX_OF_NULL => OutputFormat::Null,
            ccx_output_format::CCX_OF_SMPTETT => OutputFormat::SmpteTt,
            ccx_output_format::CCX_OF_SPUPNG => OutputFormat::SpuPng,
            ccx_output_format::CCX_OF_DVDRAW => OutputFormat::DvdRaw,
            ccx_output_format::CCX_OF_WEBVTT => OutputFormat::WebVtt,
            ccx_output_format::CCX_OF_SIMPLE_XML => OutputFormat::SimpleXml,
            ccx_output_format::CCX_OF_G608 => OutputFormat::G608,
            ccx_output_format::CCX_OF_CURL => OutputFormat::Curl,
            ccx_output_format::CCX_OF_SSA => OutputFormat::Ssa,
            ccx_output_format::CCX_OF_MCC => OutputFormat::Mcc,
            ccx_output_format::CCX_OF_SCC => OutputFormat::Scc,
            ccx_output_format::CCX_OF_CCD => OutputFormat::Ccd,
        })
    }
}
