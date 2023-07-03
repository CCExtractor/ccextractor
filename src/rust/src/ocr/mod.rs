use std::os::raw::{c_char, c_void};

use crate::bindings::{cc_bitmap, cc_subtitle, encoder_ctx};

use crate::bindings::ccx_s_options;

use tesseract_sys;

#[derive(Debug)]
struct OcrCtx {
    api: *mut tesseract_sys::TessBaseAPI,
}

// global variables
extern "C" {
    static ccx_options: ccx_s_options;
    static language: *mut *mut ::std::os::raw::c_char;
}

#[no_mangle]
pub unsafe extern "C" fn init_ocr(
    lang_index: ::std::os::raw::c_int,
) -> *mut ::std::os::raw::c_void {
    println!("ocr should be initialized here");
    // let (lang, tessdata_path);
    //
    // if ccx_options.ocrlang.is_null() {
    //     lang = ccx_options.ocrlang
    // }
    //
    let lang = ccx_options.ocrlang as i8;
    println!("cxx options ocr lang: {}", lang);
    let mut ctx = OcrCtx {
        api: tesseract_sys::TessBaseAPICreate(),
    };
    &mut ctx as *mut _ as *mut c_void
}

#[no_mangle]
pub unsafe extern "C" fn delete_ocr(arg: *mut *mut ::std::os::raw::c_void) {
    println!("here, ocr context is deleted");
}

#[no_mangle]
pub unsafe extern "C" fn ocr_rect(
    arg: *mut ::std::os::raw::c_void,
    rect: *mut cc_bitmap,
    str_: *mut *mut ::std::os::raw::c_char,
    bgcolor: ::std::os::raw::c_int,
    ocr_quantmode: ::std::os::raw::c_int,
) -> ::std::os::raw::c_int {
    0
}

#[no_mangle]
pub unsafe extern "C" fn paraof_ocrtext(
    sub: *mut cc_subtitle,
    context: *mut encoder_ctx,
) -> *mut ::std::os::raw::c_char {
    let mut c = 'c';
    &mut c as *mut _ as *mut c_char
}
