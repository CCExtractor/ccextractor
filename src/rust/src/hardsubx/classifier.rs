#[cfg(feature = "hardsubx_ocr")]
use ffmpeg_sys_next::*;

#[cfg(feature = "hardsubx_ocr")]
use tesseract_sys::*;

#[cfg(feature = "hardsubx_ocr")]
use leptonica_sys::*;

pub type ccx_output_format = ::std::os::raw::c_uint;
pub type subdatatype = ::std::os::raw::c_uint;
pub type subtype = ::std::os::raw::c_uint;
pub type ccx_encoding_type = ::std::os::raw::c_uint;

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct cc_subtitle {
    #[doc = " A generic data which contain data according to decoder"]
    #[doc = " @warn decoder cant output multiple types of data"]
    pub data: *mut ::std::os::raw::c_void,
    pub datatype: subdatatype,
    #[doc = " number of data"]
    pub nb_data: ::std::os::raw::c_uint,
    #[doc = "  type of subtitle"]
    pub type_: subtype,
    #[doc = " Encoding type of Text, must be ignored in case of subtype as bitmap or cc_screen"]
    pub enc_type: ccx_encoding_type,
    pub start_time: i64,
    pub end_time: i64,
    pub flags: ::std::os::raw::c_int,
    pub lang_index: ::std::os::raw::c_int,
    #[doc = " flag to tell that decoder has given output"]
    pub got_output: ::std::os::raw::c_int,
    pub mode: [::std::os::raw::c_char; 5usize],
    pub info: [::std::os::raw::c_char; 4usize],
    #[doc = " Used for DVB end time in ms"]
    pub time_out: ::std::os::raw::c_int,
    pub next: *mut cc_subtitle,
    pub prev: *mut cc_subtitle,
}

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
    pub packet: AVPacket,
    pub options_dict: *mut AVDictionary,
    pub sws_ctx: *mut SwsContext,
    pub rgb_buffer: *mut u8,
    pub video_stream_id: ::std::os::raw::c_int,
    pub im: *mut PIX,
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
