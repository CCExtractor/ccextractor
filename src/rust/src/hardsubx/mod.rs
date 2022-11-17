pub mod classifier;
pub mod decoder;
pub mod imgops;
pub mod main;
pub mod utility;

#[cfg(feature = "hardsubx_ocr")]
use ffmpeg_sys_next::*;

#[cfg(feature = "hardsubx_ocr")]
use tesseract_sys::*;

#[cfg(feature = "hardsubx_ocr")]
use leptonica_sys::*;

use crate::bindings;
use crate::bindings::{
    cc_subtitle, ccx_output_format, ccx_s_options, get_file_extension, probe_tessdata_location,
};
use crate::utils::string_to_c_char;
use std::boxed::Box;
use std::convert::TryInto;
use std::ffi;
use std::matches;
use std::os::raw::c_char;
use std::os::raw::c_void;
use std::process;
use std::ptr::null;
use std::vec::Vec;

// definitions taken from ccx_common_common.h
static EXIT_NOT_ENOUGH_MEMORY: i32 = 500;
static EXIT_READ_ERROR: i32 = 8;

static EXIT_MALFORMED_PARAMETER: i32 = 7;

extern "C" {
    pub static mut ccx_options: ccx_s_options;
}

pub enum hardsubx_color_type {
    HARDSUBX_COLOR_WHITE,
    HARDSUBX_COLOR_YELLOW,
    HARDSUBX_COLOR_GREEN,
    HARDSUBX_COLOR_CYAN,
    HARDSUBX_COLOR_BLUE,
    HARDSUBX_COLOR_MAGENTA,
    HARDSUBX_COLOR_RED,
    HARDSUBX_COLOR_CUSTOM,
}

pub enum hardsubx_ocr_mode {
    HARDSUBX_OCRMODE_FRAME,
    HARDSUBX_OCRMODE_WORD,
    HARDSUBX_OCRMODE_LETTER,
}

impl Default for cc_subtitle {
    fn default() -> Self {
        Self {
            data: null::<std::os::raw::c_void>() as *mut std::os::raw::c_void,
            datatype: 0,
            nb_data: 0,
            type_: 0,
            enc_type: 0,
            start_time: 0,
            end_time: 0,
            flags: 0,
            lang_index: 0,
            got_output: 0,
            mode: [0, 0, 0, 0, 0],
            info: [0, 0, 0, 0],
            time_out: 0,
            next: null::<cc_subtitle>() as *mut cc_subtitle,
            prev: null::<cc_subtitle>() as *mut cc_subtitle,
        }
    }
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

pub struct HardsubxContext {
    pub cc_to_stdout: i32,
    pub subs_delay: i64,
    pub last_displayed_subs_ms: i64,
    pub basefilename: String,
    pub extension: String,
    pub current_file: i32,
    pub inputfile: Vec<String>,
    pub num_input_files: i32,
    pub system_start_time: i64,
    pub write_format: ccx_output_format,
    pub format_ctx: *mut AVFormatContext,

    pub codec_par: *mut AVCodecParameters,
    // Codec context will be generated from the code parameters
    pub codec_ctx: *mut AVCodecContext,

    pub codec: *mut AVCodec,
    pub frame: *mut AVFrame,
    pub rgb_frame: *mut AVFrame,

    pub packet: AVPacket,

    pub options_dict: *mut AVDictionary,
    pub sws_ctx: *mut SwsContext,
    pub rgb_buffer: *mut u8,

    pub video_stream_id: i32,
    pub im: *mut Pix,
    pub tess_handle: *mut TessBaseAPI,

    pub cur_conf: f32,
    pub prev_conf: f32,

    pub tickertext: bool,
    pub hardsubx_and_common: bool,

    pub dec_sub: Box<cc_subtitle>,
    pub ocr_mode: hardsubx_ocr_mode,
    pub subcolor: hardsubx_color_type,

    pub min_sub_duration: f32,
    pub detect_italics: bool,

    pub conf_thresh: f32,
    pub hue: f32,
    pub lum_thresh: f32,
}

impl Default for HardsubxContext {
    fn default() -> Self {
        Self {
            cc_to_stdout: 0,
            subs_delay: 0,
            last_displayed_subs_ms: 0,
            basefilename: String::new(),
            extension: String::new(),
            current_file: 0,
            inputfile: Vec::<String>::new(),
            num_input_files: 0,
            system_start_time: -1,
            write_format: ccx_output_format::CCX_OF_RAW,
            format_ctx: null::<AVFormatContext>() as *mut AVFormatContext,
            codec_par: null::<AVCodecParameters>() as *mut AVCodecParameters,
            codec_ctx: null::<AVCodecContext>() as *mut AVCodecContext,
            codec: null::<AVCodec>() as *mut AVCodec,
            frame: null::<AVFrame>() as *mut AVFrame,
            rgb_frame: null::<AVFrame>() as *mut AVFrame,
            packet: AVPacket {
                buf: null::<AVBufferRef>() as *mut AVBufferRef,
                pts: 0,
                dts: 0,
                data: null::<u8>() as *mut u8,
                size: 0,
                stream_index: 0,
                flags: 0,
                side_data: null::<AVPacketSideData>() as *mut AVPacketSideData,
                side_data_elems: 0,
                duration: 0,
                pos: 0,
                opaque: null::<c_void>() as *mut c_void,
                opaque_ref: null::<AVBufferRef>() as *mut AVBufferRef,
                time_base: AV_TIME_BASE_Q,
                // convergence_duration: 0
            },
            options_dict: null::<AVDictionary>() as *mut AVDictionary,
            sws_ctx: null::<SwsContext>() as *mut SwsContext,
            rgb_buffer: null::<u8>() as *mut u8,
            video_stream_id: 0,
            im: null::<Pix>() as *mut Pix,
            tess_handle: null::<TessBaseAPI>() as *mut TessBaseAPI,
            cur_conf: 0.0,
            prev_conf: 0.0,

            tickertext: false,
            hardsubx_and_common: false,

            dec_sub: {
                let tmp = cc_subtitle::default();
                Box::new(tmp)
            },
            ocr_mode: hardsubx_ocr_mode::HARDSUBX_OCRMODE_FRAME,
            subcolor: hardsubx_color_type::HARDSUBX_COLOR_WHITE,

            min_sub_duration: 0.0,
            detect_italics: false,

            conf_thresh: 0.0,
            hue: 0.0,
            lum_thresh: 0.0,
        }
    }
}

impl HardsubxContext {
    /// # Safety
    /// dereferences a raw pointer
    /// Calls C functions and sends them raw pointers, their safety is not guaranteed
    pub unsafe fn new(options: *mut ccx_s_options) -> Self {
        let tess_handle = &mut (*TessBaseAPICreate()) as &mut TessBaseAPI;

        let lang: String = {
            if (*options).ocrlang != null::<char>() as *mut c_char {
                ffi::CStr::from_ptr((*options).ocrlang)
                    .to_string_lossy()
                    .into_owned()
            } else {
                "eng".to_string()
            }
        };

        let lang_cstr = string_to_c_char(&lang);

        let tessdata_path = {
            let path = probe_tessdata_location(lang_cstr);

            if path == (null::<c_char>() as *mut c_char) && lang != *"eng" {
                let eng_cstr = string_to_c_char("eng");
                let tmp = probe_tessdata_location(eng_cstr);
                ffi::CString::from_raw(eng_cstr); // deallocation
                if tmp != null::<c_char>() as *mut c_char {
                    println!("{}.traineddata not found! Switching to English\n", lang);
                    tmp
                } else {
                    eprintln!("eng.traineddata not found! No Switching Possible\n");
                    process::exit(0);
                }
            } else if path == null::<c_char>() as *mut c_char {
                eprintln!("eng.traineddata not found! No Switching Possible\n");
                process::exit(0);
            } else {
                path
            }
        };

        let mut tess_data_str: String = ffi::CStr::from_ptr(tessdata_path)
            .to_string_lossy()
            .into_owned();

        let mut pars_vec: *mut c_char = string_to_c_char("debug_file");
        let mut pars_values: *mut c_char = string_to_c_char("/dev/null");

        let ret = {
            let tess_version = ffi::CStr::from_ptr(TessVersion())
                .to_string_lossy()
                .into_owned();

            if tess_version[..2].eq(&("4.".to_string())) {
                tess_data_str = format!("{}/tessdata", tess_data_str);
                if ccx_options.ocr_oem < 0 {
                    ccx_options.ocr_oem = 1;
                }
            } else if ccx_options.ocr_oem < 0 {
                ccx_options.ocr_oem = 0;
            }

            let tessdata_path_cstr = string_to_c_char(&tess_data_str);
            let ret = TessBaseAPIInit4(
                tess_handle,
                tessdata_path_cstr,
                lang_cstr,
                ccx_options.ocr_oem.try_into().unwrap(),
                null::<*mut u8>() as *mut *mut i8,
                0,
                &mut pars_vec,
                &mut pars_values,
                1,
                0,
            );

            ffi::CString::from_raw(tessdata_path_cstr); // deallocation

            ret
        };

        // deallocation
        ffi::CString::from_raw(pars_vec);
        ffi::CString::from_raw(pars_values);

        if ret != 0 {
            eprintln!("Not enough memory to initialize");
            process::exit(EXIT_NOT_ENOUGH_MEMORY);
        }

        ffi::CString::from_raw(lang_cstr); //deallocate
                                           // function to be used for only converting the C struct to rust
        Self {
            tess_handle,
            basefilename: {
                if (*options).output_filename == null::<c_char>() as *mut c_char {
                    "".to_string()
                } else {
                    ffi::CStr::from_ptr((*options).output_filename)
                        .to_string_lossy()
                        .into_owned()
                }
            },
            current_file: -1,
            inputfile: {
                let mut vec_inputfile: Vec<String> = Vec::new();
                for i in 0..(*options).num_input_files {
                    vec_inputfile.push(
                        ffi::CStr::from_ptr(*(*options).inputfile.offset(i.try_into().unwrap()))
                            .to_string_lossy()
                            .into_owned(),
                    )
                }
                vec_inputfile
            },
            num_input_files: (*options).num_input_files,
            extension: ffi::CStr::from_ptr(get_file_extension((*options).write_format))
                .to_string_lossy()
                .into_owned(),
            write_format: (*options).write_format,
            subs_delay: (*options).subs_delay,
            cc_to_stdout: (*options).cc_to_stdout,
            tickertext: !matches!((*options).tickertext, 0),
            cur_conf: 0.0,
            prev_conf: 0.0,
            ocr_mode: {
                if (*options).hardsubx_ocr_mode == 0 {
                    hardsubx_ocr_mode::HARDSUBX_OCRMODE_FRAME
                } else if (*options).hardsubx_ocr_mode == 1 {
                    hardsubx_ocr_mode::HARDSUBX_OCRMODE_WORD
                } else if (*options).hardsubx_ocr_mode == 2 {
                    hardsubx_ocr_mode::HARDSUBX_OCRMODE_LETTER
                } else {
                    eprintln!("Invalid OCR Mode");
                    process::exit(EXIT_MALFORMED_PARAMETER);
                }
            },

            subcolor: match (*options).hardsubx_subcolor {
                0 => hardsubx_color_type::HARDSUBX_COLOR_WHITE,
                1 => hardsubx_color_type::HARDSUBX_COLOR_YELLOW,
                2 => hardsubx_color_type::HARDSUBX_COLOR_GREEN,
                3 => hardsubx_color_type::HARDSUBX_COLOR_CYAN,
                4 => hardsubx_color_type::HARDSUBX_COLOR_BLUE,
                5 => hardsubx_color_type::HARDSUBX_COLOR_MAGENTA,
                6 => hardsubx_color_type::HARDSUBX_COLOR_RED,
                7 => hardsubx_color_type::HARDSUBX_COLOR_CUSTOM,
                _ => hardsubx_color_type::HARDSUBX_COLOR_WHITE, // white is default
            },

            min_sub_duration: (*options).hardsubx_min_sub_duration,
            detect_italics: !matches!((*options).hardsubx_detect_italics, 0),

            conf_thresh: (*options).hardsubx_conf_thresh,
            hue: (*options).hardsubx_hue,

            lum_thresh: (*options).hardsubx_lum_thresh,
            hardsubx_and_common: !matches!((*options).hardsubx_and_common, 0),
            ..Default::default()
        }
    }
}
