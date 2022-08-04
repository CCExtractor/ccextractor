#[cfg(feature = "hardsubx_ocr")]
use ffmpeg_sys_next::*;

#[cfg(feature = "hardsubx_ocr")]
use tesseract_sys::*;

#[cfg(feature = "hardsubx_ocr")]
use leptonica_sys::*;

use std::ffi;
use std::ptr::null;

pub type ccx_output_format = ::std::os::raw::c_uint;
pub type subdatatype = ::std::os::raw::c_uint;
pub type subtype = ::std::os::raw::c_uint;
pub type ccx_encoding_type = ::std::os::raw::c_uint;

use crate::utils::{mprint_mem_safe, string_to_c_char};

use std::os::raw::c_char;

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

pub unsafe extern "C" fn get_ocr_text_simple(
    ctx: *mut lib_hardsubx_ctx,
    image: *mut PIX,
) -> *mut ::std::os::raw::c_char {
    let text_out: *mut ::std::os::raw::c_char;

    TessBaseAPISetImage2((*ctx).tess_handle, image);

    if TessBaseAPIRecognize((*ctx).tess_handle, null::<ETEXT_DESC>() as *mut ETEXT_DESC) != 0 {
        mprint_mem_safe("Error in Tesseract recognition, skipping frame\n");
        null::<c_char>() as *mut c_char
    } else {
        text_out = TessBaseAPIGetUTF8Text((*ctx).tess_handle);

        if text_out == null::<c_char>() as *mut c_char {
            mprint_mem_safe("Error getting text, skipping frame\n");
        }
        text_out
    }
}

unsafe fn _tess_string_helper(it: *mut TessResultIterator, level: TessPageIteratorLevel) -> String {
    // Function extracts string from tess iterator object
    // frees memory associated with tesseract string
    // takes and gives ownership of the string

    let ts_word_ptr: *mut ::std::os::raw::c_char = TessResultIteratorGetUTF8Text(it, level);

    if ts_word_ptr == null::<c_char>() as *mut c_char {
        // this is required because trying to generate
        // CStr from null pointer will be a segmentation fault
        return String::new();
    }

    let ts_word = ffi::CStr::from_ptr(ts_word_ptr);
    let ts_word_arr = ffi::CStr::to_bytes_with_nul(&ts_word);

    let ts_word_string: String = match String::from_utf8(ts_word_arr.to_vec()) {
        Ok(string_rep) => string_rep,
        Err(error) => std::panic::panic_any(error),
    };

    TessDeleteText(ts_word_ptr);
    // clean up the memory

    ts_word_string
}

pub unsafe extern "C" fn get_ocr_text_wordwise(
    ctx: *mut lib_hardsubx_ctx,
    image: *mut PIX,
) -> *mut ::std::os::raw::c_char {
    let mut text_out = String::new();

    TessBaseAPISetImage2((*ctx).tess_handle, image);

    if TessBaseAPIRecognize((*ctx).tess_handle, null::<ETEXT_DESC>() as *mut ETEXT_DESC) != 0 {
        mprint_mem_safe("Error in Tesseract recognition, skipping word\n");
        return null::<c_char>() as *mut c_char;
    }

    let it: *mut TessResultIterator = TessBaseAPIGetIterator((*ctx).tess_handle);
    let level: TessPageIteratorLevel = TessPageIteratorLevel_RIL_WORD;

    let mut prev_ital: bool = false;

    if it != null::<TessResultIterator>() as *mut TessResultIterator {
        loop {
            let mut word = _tess_string_helper(it, level);

            if word.len() == 0 {
                continue;
            }

            if (*ctx).detect_italics != 0 {
                let mut italic: i32 = 0;
                let mut dummy: i32 = 0;

                TessResultIteratorWordFontAttributes(
                    it,
                    &mut dummy as *mut i32,
                    &mut italic as *mut i32,
                    &mut dummy as *mut i32,
                    &mut dummy as *mut i32,
                    &mut dummy as *mut i32,
                    &mut dummy as *mut i32,
                    &mut dummy as *mut i32,
                    &mut dummy as *mut i32,
                );
                if italic != 0 && !prev_ital {
                    word = format!("<i>{}", word);
                    prev_ital = true;
                } else if italic == 0 && prev_ital {
                    word = format!("{}</i>", word);
                    prev_ital = false;
                }
            }

            text_out = format!("{} {}", text_out, word);

            if TessPageIteratorNext(it as *mut TessPageIterator, level) == 0 {
                if (*ctx).detect_italics == 1 && prev_ital {
                    // if there are italics words at the end
                    text_out = format!("{}</i>", text_out);
                }
                break;
            }
        }
    }

    TessResultIteratorDelete(it);

    // TODO: this is a memory leak that can only be plugged when the caller functions have been ported
    return string_to_c_char(&text_out);
}
