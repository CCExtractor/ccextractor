pub mod classifier;
pub mod decoder;
pub mod imgops;
pub mod utility;

#[cfg(feature = "hardsubx_ocr")]
use rsmpeg::avutil::*;
#[cfg(feature = "hardsubx_ocr")]
use rsmpeg::ffi::{AVCodec, AVCodecContext, AVFormatContext, SwsContext};

#[cfg(feature = "hardsubx_ocr")]
use tesseract_sys::*;

#[cfg(feature = "hardsubx_ocr")]
use leptonica_sys::*;

use crate::bindings;
use crate::bindings::{cc_subtitle, ccx_output_format};

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct lib_hardsubx_ctx {
    pub cc_to_stdout: ::std::os::raw::c_int,
    pub subs_delay: i64,
    pub last_displayed_subs_ms: i64,
    pub basefilename: *mut ::std::os::raw::c_char,
    pub extension: *const ::std::os::raw::c_char,
    pub current_file: ::std::os::raw::c_int,
    pub inputfile: *mut *mut ::std::os::raw::c_char,
    pub num_input_files: ::std::os::raw::c_int,
    pub system_start_time: i64,
    pub write_format: ccx_output_format,
    pub format_ctx: *mut AVFormatContext,
    pub codec_ctx: *mut AVCodecContext,
    pub codec: *mut AVCodec,
    pub frame: *mut AVFrame,
    pub rgb_frame: *mut AVFrame,
    pub packet: bindings::AVPacket,
    pub options_dict: *mut AVDictionary,
    pub sws_ctx: *mut SwsContext,
    pub rgb_buffer: *mut u8,
    pub video_stream_id: ::std::os::raw::c_int,
    pub im: *mut Pix,
    pub tess_handle: *mut TessBaseAPI,
    pub cur_conf: f32,
    pub prev_conf: f32,
    pub tickertext: ::std::os::raw::c_int,
    pub hardsubx_and_common: ::std::os::raw::c_int,
    pub dec_sub: *mut cc_subtitle,
    pub ocr_mode: ::std::os::raw::c_int,
    pub subcolor: ::std::os::raw::c_int,
    pub min_sub_duration: f32,
    pub detect_italics: ::std::os::raw::c_int,
    pub conf_thresh: f32,
    pub hue: f32,
    pub lum_thresh: f32,
}
