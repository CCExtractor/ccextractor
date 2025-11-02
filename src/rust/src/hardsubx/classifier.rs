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

use crate::hardsubx::lib_hardsubx_ctx;
use crate::utils::string_to_c_char;

use std::os::raw::c_char;

use log::warn;

/// # Safety
/// The function accepts and dereferences a raw pointer
/// The function also makes calls to functions whose safety is not guaranteed
/// The function returns a raw pointer which is a string made in C
/// ctx should be not null
#[no_mangle]
pub unsafe extern "C" fn get_ocr_text_simple_threshold(
    ctx: *mut lib_hardsubx_ctx,
    image: *mut Pix,
    threshold: std::os::raw::c_float,
) -> *mut ::std::os::raw::c_char {
    let mut text_out: *mut ::std::os::raw::c_char;

    TessBaseAPISetImage2((*ctx).tess_handle, image);

    if TessBaseAPIRecognize((*ctx).tess_handle, null::<ETEXT_DESC>() as *mut ETEXT_DESC) != 0 {
        warn!("Error in Tesseract recognition, skipping frame\n");
        null::<c_char>() as *mut c_char
    } else {
        text_out = TessBaseAPIGetUTF8Text((*ctx).tess_handle);

        if text_out == null::<c_char>() as *mut c_char {
            warn!("Error getting text, skipping frame\n");
        }

        if threshold > 0.0 {
            // non-zero conf, only then we'll make the call to check for confidence
            let conf = TessBaseAPIMeanTextConf((*ctx).tess_handle);

            if (conf as std::os::raw::c_float) < threshold {
                text_out = null::<c_char>() as *mut c_char;
            } else {
                (*ctx).cur_conf = conf as std::os::raw::c_float;
            }
        }
        text_out
    }
}

/// basically the get_oct_text_simple function without threshold
/// This function is being kept only for backwards compatibility reasons
/// # Safety
/// The function accepts and dereferences a raw pointer
/// The function also makes calls to functions whose safety is not guaranteed
/// The function returns a raw pointer which is a string made in C
/// ctx should be not null
#[no_mangle]
pub unsafe extern "C" fn get_ocr_text_simple(
    ctx: *mut lib_hardsubx_ctx,
    image: *mut Pix,
) -> *mut ::std::os::raw::c_char {
    get_ocr_text_simple_threshold(ctx, image, 0.0)
}

/// Function extracts string from tess iterator object
/// frees memory associated with tesseract string
/// takes and gives ownership of the string
/// # Safety
/// Function dereferences a raw pointer and makes calls to functions whose safety is not guaranteed
unsafe fn _tess_string_helper(it: *mut TessResultIterator, level: TessPageIteratorLevel) -> String {
    let ts_ret_ptr: *mut ::std::os::raw::c_char = TessResultIteratorGetUTF8Text(it, level);

    if ts_ret_ptr == null::<c_char>() as *mut c_char {
        // this is required because trying to generate
        // CStr from null pointer will be a segmentation fault
        return String::new();
    }

    let ts_ret = ffi::CStr::from_ptr(ts_ret_ptr)
        .to_string_lossy()
        .into_owned();

    TessDeleteText(ts_ret_ptr);
    // clean up the memory

    ts_ret
}

/// # Safety
/// The function dereferences a raw pointer
/// The function also calls other functions whose safety is not guaranteed
/// The function returns a raw pointer of a String created in Rust
/// This has to be deallocated at some point using from_raw() lest it be a memory leak
/// ctx should be not null
#[no_mangle]
pub unsafe extern "C" fn get_ocr_text_wordwise_threshold(
    ctx: *mut lib_hardsubx_ctx,
    image: *mut Pix,
    threshold: std::os::raw::c_float,
) -> *mut ::std::os::raw::c_char {
    let mut text_out = String::new();

    TessBaseAPISetImage2((*ctx).tess_handle, image);

    if TessBaseAPIRecognize((*ctx).tess_handle, null::<ETEXT_DESC>() as *mut ETEXT_DESC) != 0 {
        warn!("Error in Tesseract recognition, skipping word\n");
        return null::<c_char>() as *mut c_char;
    }

    let it: *mut TessResultIterator = TessBaseAPIGetIterator((*ctx).tess_handle);
    let level: TessPageIteratorLevel = TessPageIteratorLevel_RIL_WORD;

    let mut prev_ital: bool = false;
    let mut total_conf: std::os::raw::c_float = 0.0;
    let mut num_words: std::os::raw::c_int = 0;

    let mut first_iter: bool = true;

    if it != null::<TessResultIterator>() as *mut TessResultIterator {
        loop {
            if first_iter {
                first_iter = false;
            } else if TessPageIteratorNext(it as *mut TessPageIterator, level) == 0 {
                if (*ctx).detect_italics == 1 && prev_ital {
                    // if there are italics words at the end
                    text_out = format!("{}</i>", text_out);
                }
                break;
            }

            let mut word = _tess_string_helper(it, level);
            if word.is_empty() {
                continue;
            }

            if threshold > 0.0 {
                // we don't even want to check for confidence if threshold is 0 or less
                let conf = TessResultIteratorConfidence(it, level);
                if conf < threshold {
                    continue;
                }

                total_conf += conf;
                num_words += 1;
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
        }
    }

    if threshold > 0.0 {
        // any confidence calculation has happened if and only if threshold > 0
        (*ctx).cur_conf = total_conf / (num_words as std::os::raw::c_float);
    }

    TessResultIteratorDelete(it);

    string_to_c_char(&text_out)
}

/// # Safety
/// The function dereferences a raw pointer
/// The function also calls other functions whose safety is not guaranteed
/// The function returns a raw pointer of a String created in Rust
/// This has to be deallocated at some point using from_raw() lest it be a memory leak
/// ctx should be not null
#[no_mangle]
pub unsafe extern "C" fn get_ocr_text_wordwise(
    ctx: *mut lib_hardsubx_ctx,
    image: *mut Pix,
) -> *mut ::std::os::raw::c_char {
    get_ocr_text_wordwise_threshold(ctx, image, 0.0)
}

/// # Safety
/// The function dereferences a raw pointer
/// The function also calls other functions whose safety is not guaranteed
/// The function returns a raw pointer of a String created in Rust
/// This has to be deallocated at some point using from_raw() lest it be a memory leak
/// ctx should be not null
#[no_mangle]
pub unsafe extern "C" fn get_ocr_text_letterwise_threshold(
    ctx: *mut lib_hardsubx_ctx,
    image: *mut Pix,
    threshold: std::os::raw::c_float,
) -> *mut ::std::os::raw::c_char {
    let mut text_out: String = String::new();

    TessBaseAPISetImage2((*ctx).tess_handle, image);

    if TessBaseAPIRecognize((*ctx).tess_handle, null::<ETEXT_DESC>() as *mut ETEXT_DESC) != 0 {
        warn!("Error in Tesseract recognition, skipping symbol\n");
        return null::<c_char>() as *mut c_char;
    }

    let it: *mut TessResultIterator = TessBaseAPIGetIterator((*ctx).tess_handle);
    let level: TessPageIteratorLevel = TessPageIteratorLevel_RIL_SYMBOL;

    let mut total_conf: std::os::raw::c_float = 0.0;
    let mut num_characters: std::os::raw::c_int = 0;

    let mut first_iter: bool = false;

    if it != null::<TessResultIterator>() as *mut TessResultIterator {
        loop {
            if first_iter {
                first_iter = false;
            } else if TessPageIteratorNext(it as *mut TessPageIterator, level) == 0 {
                break;
            }

            let letter = _tess_string_helper(it, level);
            text_out = format!("{}{}", text_out, letter);

            if threshold > 0.0 {
                // we don't even want to bother with this call if threshold is 0 or less
                let conf: std::os::raw::c_float = TessResultIteratorConfidence(it, level);
                if conf < threshold {
                    continue;
                }

                total_conf += conf;
                num_characters += 1;
            }
        }
    }

    if threshold > 0.0 {
        // No confidence calculation has been done if threshold is 0 or less
        (*ctx).cur_conf = total_conf / (num_characters as std::os::raw::c_float);
    }

    TessResultIteratorDelete(it);

    string_to_c_char(&text_out)
}

/// # Safety
/// The function dereferences a raw pointer
/// The function also calls other functions whose safety is not guaranteed
/// The function returns a raw pointer of a String created in Rust
/// This has to be deallocated at some point using from_raw() lest it be a memory leak
/// ctx should be not null
#[no_mangle]
pub unsafe extern "C" fn get_ocr_text_letterwise(
    ctx: *mut lib_hardsubx_ctx,
    image: *mut Pix,
) -> *mut ::std::os::raw::c_char {
    get_ocr_text_letterwise_threshold(ctx, image, 0.0)
}
