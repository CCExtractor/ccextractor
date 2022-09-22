#[cfg(feature = "hardsubx_ocr")]
use ffmpeg_sys_next::*;

#[cfg(feature = "hardsubx_ocr")]
use tesseract_sys::*;

#[cfg(feature = "hardsubx_ocr")]
use leptonica_sys::*;

use crate::bindings::{
    ccx_s_options, encoder_ctx, lib_ccx_ctx, cc_subtitle, ccx_output_format, encoder_cfg
};

pub mod imgops;
pub mod main;
pub mod utility;


// definitions taken from ccx_common_common.h
static EXIT_NOT_ENOUGH_MEMORY: i32 = 500;
static EXIT_READ_ERROR: i32 = 8;

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


extern "C" {
    #[doc = " Inialize encoder context with output context"]
    #[doc = " allocate initial memory to buffer of context"]
    #[doc = " write subtitle header to file refrenced by"]
    #[doc = " output context"]
    #[doc = ""]
    #[doc = " @param cfg Option to initilaize encoder cfg params"]
    #[doc = ""]
    #[doc = " @return Allocated and properly initilaized Encoder Context, NULL on failure"]
    pub fn init_encoder(opt: *mut encoder_cfg) -> *mut encoder_ctx;
}

// #[link(name = "ccx", kind = "static")]
extern "C" {
    pub fn hardsubx_process_frames_linear(ctx: *mut lib_hardsubx_ctx, enc_ctx: *mut encoder_ctx);
}
extern "C" {
    pub fn hardsubx_process_frames_tickertext(
        ctx: *mut lib_hardsubx_ctx,
        enc_ctx: *mut encoder_ctx,
    ) -> ::std::os::raw::c_int;
}

extern "C" {
    pub fn process_hardsubx_linear_frames_and_normal_subs(
        hard_ctx: *mut lib_hardsubx_ctx,
        enc_ctx: *mut encoder_ctx,
        ctx: *mut lib_ccx_ctx,
    );
}


// pub struct HardsubxContext<'a>{
//     pub cc_to_stdout: i32,
//     pub subs_delay: i64,
//     pub last_displayed_subs_ms: i64,
//     pub basefilename: mut String,
//     pub extension: String,
//     pub current_file: i32,
//     pub inputfile: &String,
//     pub num_input_files, i32,
//     pub system_start_time: i64,
//     pub write_format: ccx_output_format,
//     pub format_ctx: &'a mut AVFormatContext,

//     pub codec_par: &'a mut AVCodecParameters,
//     // Codec context will be generated from the code parameters
//     pub codec_ctx: &'a mut AVCodecContext,

//     pub codec: &'a mut AVCodec,
//     pub frame: &'a mut AVFrame,
//     pub rgb_frame: &'a mut AVFrame,

//     pub video_stream_id: i32,
//     pub im: *mut PIX,
//     pub tess_handle: &'a mut TessBaseAPI,

//     pub cur_conf: f32,
//     pub prev_conf: f32,

//     pub tickertext: i32,
//     pub hardsubx_and_common: i32,

//     pub dec_sub: &'a mut cc_subtitle,
//     pub ocr_mode: i32,
//     pub subcolor: i32,

//     pub min_sub_duration: f32,
//     pub detect_italics: i32,

//     pub conf_thresh: f32,
//     pub hue: f32,
//     pub lum_thresh: f32

// }

// impl <'a> HardsubxContext <'a> {
//     pub fn new(ctx: &'a mut lib_hardsubx_ctx) -> Self {

//         // function to be used for only converting the C struct to rust

//     }
// }
// impl cc_subtitle {
//     fn new() -> Self {
//         cc_subtitle {
//             data: 0,
//             datatype: 0,
//             nb_ata: 0,
//             enc_type: 0,
//             start_time: 0,
//             end_time: 0,
//             flags: 0,
//             lang_index: 0,
//             got_output: 0,
//             mode: [0, 0, 0, 0, 0],
//             info: [0, 0, 0, 0],
//             time_out: 0,
//             next: 0,
//             prev: 0,
//         }
//     }
// }
