#[cfg(feature = "hardsubx_ocr")]
use leptonica_sys::*;

// #[cfg(feature = "hardsubx_ocr")]
// use ffmpeg_sys_next::*;

use std::convert::TryInto;
use std::eprintln;
use std::ffi;
use std::format;
use std::os::raw::c_char;
use std::process::exit;
use std::ptr::null;

#[cfg(feature = "hardsubx_ocr")]
// use crate::bindings::{hardsubx_ocr_mode_HARDSUBX_OCRMODE_WORD};
use crate::bindings::AVFrame;
use crate::hardsubx::classifier::*;
use crate::hardsubx::imgops::{rgb_to_hsv, rgb_to_lab};
use crate::hardsubx::lib_hardsubx_ctx;
use crate::utils::string_to_c_char;

static EXIT_MALFORMED_PARAMETER: i32 = 7;

// TODO: turn into an enum definition when the hardsubx context is rewritten
// static HARDSUBX_OCRMODE_FRAME: i32 = 0;
static HARDSUBX_OCRMODE_WORD: i32 = 1;
// static HARDSUBX_OCRMODE_LETTER: i32 = 2;

// enum hardsubx_ocr_mode {
//     HARDSUBX_OCRMODE_FRAME,
//     HARDSUBX_OCRMODE_WORD,
//     HARDSUBX_OCRMODE_LETTER
// };

/// # Safety
/// dereferences a raw pointer
/// calls functions that are not necessarily safe
pub unsafe fn dispatch_classifier_functions(ctx: *mut lib_hardsubx_ctx, im: *mut Pix) -> String {
    // function that calls the classifier functions
    match (*ctx).ocr_mode {
        0 => {
            let ret_char_arr = get_ocr_text_wordwise_threshold(ctx, im, (*ctx).conf_thresh);
            ffi::CStr::from_ptr(ret_char_arr)
                .to_string_lossy()
                .into_owned()
        }
        1 => {
            let ret_char_arr = get_ocr_text_letterwise_threshold(ctx, im, (*ctx).conf_thresh);
            let text_out_result = ffi::CString::from_raw(ret_char_arr).into_string();
            match text_out_result {
                Ok(T) => T,
                Err(_E) => "".to_string(),
            }
        }

        2 => {
            let ret_char_arr = get_ocr_text_simple_threshold(ctx, im, (*ctx).conf_thresh);
            let text_out_result = ffi::CString::from_raw(ret_char_arr).into_string();
            match text_out_result {
                Ok(T) => T,
                Err(_E) => "".to_string(),
            }
        }

        _ => {
            eprintln!("Invalid OCR Mode");
            exit(EXIT_MALFORMED_PARAMETER);
            // "".to_string()
        }
    }
}

/// # Safety
/// The function dereferences a raw pointer
/// The function also calls other functions whose safety is not guaranteed
/// The function returns a raw pointer of a String created in Rust
/// This has to be deallocated at some point using from_raw() lest it be a memory leak
#[no_mangle]
pub unsafe extern "C" fn _process_frame_white_basic(
    ctx: *mut lib_hardsubx_ctx,
    frame: *mut AVFrame,
    width: ::std::os::raw::c_int,
    height: ::std::os::raw::c_int,
    _index: ::std::os::raw::c_int,
) -> *mut ::std::os::raw::c_char {
    let mut im: *mut Pix = pixCreate(width, height, 32);
    let mut lum_im: *mut Pix = pixCreate(width, height, 32);
    let frame_deref = *frame;

    for i in (3 * height / 4)..height {
        for j in 0..width {
            let p: isize = (j * 3 + i * frame_deref.linesize[0]).try_into().unwrap();
            let r: i32 = (*(frame_deref.data[0]).offset(p)).into();
            let g: i32 = (*(frame_deref.data[0]).offset(p + 1)).into();
            let b: i32 = (*(frame_deref.data[0]).offset(p + 2)).into();
            pixSetRGBPixel(im, j, i, r, g, b);

            let mut L: f32 = 0.0;
            let mut A: f32 = 0.0;
            let mut B: f32 = 0.0;

            rgb_to_lab(r as f32, g as f32, b as f32, &mut L, &mut A, &mut B);

            if L > (*ctx).lum_thresh {
                pixSetRGBPixel(lum_im, j, i, 255, 255, 255);
            } else {
                pixSetRGBPixel(lum_im, j, i, 0, 0, 0);
            }
        }
    }

    let mut gray_im: *mut Pix = pixConvertRGBToGray(im, 0.0, 0.0, 0.0);
    let mut sobel_edge_im: *mut Pix =
        pixSobelEdgeFilter(gray_im, L_VERTICAL_EDGES.try_into().unwrap());
    let mut dilate_gray_im: *mut Pix = pixDilateGray(sobel_edge_im, 21, 1);
    let mut edge_im: *mut Pix = pixThresholdToBinary(dilate_gray_im, 50);

    let mut feat_im: *mut Pix = pixCreate(width, height, 32);

    for i in (3 * (height / 4))..height {
        for j in 0..width {
            let mut p1: u32 = 0;
            let mut p2: u32 = 0;

            pixGetPixel(edge_im, j, i, &mut p1);
            pixGetPixel(lum_im, j, i, &mut p2);

            if p1 == 0 && p2 > 0 {
                pixSetRGBPixel(feat_im, j, i, 255, 255, 255);
            } else {
                pixSetRGBPixel(feat_im, j, i, 0, 0, 0);
            }
        }
    }

    if (*ctx).detect_italics != 0 {
        (*ctx).ocr_mode = HARDSUBX_OCRMODE_WORD;
    }

    let subtitle_text = dispatch_classifier_functions(ctx, feat_im);

    pixDestroy(&mut im as *mut *mut Pix);
    pixDestroy(&mut gray_im as *mut *mut Pix);
    pixDestroy(&mut sobel_edge_im as *mut *mut Pix);
    pixDestroy(&mut dilate_gray_im as *mut *mut Pix);
    pixDestroy(&mut edge_im as *mut *mut Pix);
    pixDestroy(&mut lum_im as *mut *mut Pix);
    pixDestroy(&mut feat_im as *mut *mut Pix);

    string_to_c_char(&subtitle_text)
}

/// # Safety
/// The function dereferences a raw pointer
/// The function also calls other functions whose safety is not guaranteed
/// The function returns a raw pointer of a String created in Rust
/// This has to be deallocated at some point using from_raw() lest it be a memory leak
#[no_mangle]
pub unsafe extern "C" fn _process_frame_color_basic(
    ctx: *mut lib_hardsubx_ctx,
    frame: *mut AVFrame,
    width: ::std::os::raw::c_int,
    height: ::std::os::raw::c_int,
    _index: ::std::os::raw::c_int,
) -> *mut ::std::os::raw::c_char {
    let mut im: *mut Pix = pixCreate(width, height, 32);
    let mut hue_im: *mut Pix = pixCreate(width, height, 32);
    let frame_deref = *frame;

    for i in 0..height {
        for j in 0..width {
            let p: isize = (j * 3 + i * frame_deref.linesize[0]).try_into().unwrap();
            let r: i32 = (*(frame_deref.data[0]).offset(p)).into();
            let g: i32 = (*(frame_deref.data[0]).offset(p + 1)).into();
            let b: i32 = (*(frame_deref.data[0]).offset(p + 2)).into();
            pixSetRGBPixel(im, j, i, r, g, b);

            let mut H: f32 = 0.0;
            let mut S: f32 = 0.0;
            let mut V: f32 = 0.0;

            rgb_to_hsv(r as f32, g as f32, b as f32, &mut H, &mut S, &mut V);

            if ((H - (*ctx).hue).abs()) < 20.0 {
                pixSetRGBPixel(hue_im, j, i, r, g, b);
            }
        }
    }

    let mut gray_im: *mut Pix = pixConvertRGBToGray(im, 0.0, 0.0, 0.0);
    let mut sobel_edge_im: *mut Pix =
        pixSobelEdgeFilter(gray_im, L_VERTICAL_EDGES.try_into().unwrap());
    let mut dilate_gray_im: *mut Pix = pixDilateGray(sobel_edge_im, 21, 1);
    let mut edge_im: *mut Pix = pixThresholdToBinary(dilate_gray_im, 50);

    let mut gray_im_2: *mut Pix = pixConvertRGBToGray(hue_im, 0.0, 0.0, 0.0);
    let mut edge_im_2: *mut Pix = pixDilateGray(gray_im_2, 5, 5);

    let mut pixd: *mut Pix = null::<Pix>() as *mut Pix;
    pixSauvolaBinarize(
        gray_im_2,
        15,
        0.3,
        1,
        null::<*mut Pix>() as *mut *mut Pix,
        null::<*mut Pix>() as *mut *mut Pix,
        null::<*mut Pix>() as *mut *mut Pix,
        &mut pixd,
    );

    let mut feat_im: *mut Pix = pixCreate(width, height, 32);

    for i in (3 * (height / 4))..height {
        for j in 0..width {
            let mut p1: u32 = 0;
            let mut p2: u32 = 0;
            let mut p3: u32 = 0;

            pixGetPixel(edge_im, j, i, &mut p1);
            pixGetPixel(pixd, j, i, &mut p2);
            pixGetPixel(edge_im_2, j, i, &mut p3);

            if p1 == 0 && p2 == 0 && p3 > 0 {
                pixSetRGBPixel(feat_im, j, i, 255, 255, 255);
            }
        }
    }

    if (*ctx).detect_italics != 0 {
        (*ctx).ocr_mode = HARDSUBX_OCRMODE_WORD;
    }

    let subtitle_text = dispatch_classifier_functions(ctx, feat_im);

    pixDestroy(&mut im as *mut *mut Pix);
    pixDestroy(&mut hue_im as *mut *mut Pix);
    pixDestroy(&mut gray_im as *mut *mut Pix);
    pixDestroy(&mut sobel_edge_im as *mut *mut Pix);
    pixDestroy(&mut dilate_gray_im as *mut *mut Pix);
    pixDestroy(&mut edge_im as *mut *mut Pix);
    pixDestroy(&mut gray_im_2 as *mut *mut Pix);
    pixDestroy(&mut edge_im_2 as *mut *mut Pix);
    pixDestroy(&mut pixd as *mut *mut Pix);
    pixDestroy(&mut feat_im as *mut *mut Pix);

    // This is a memory leak
    // the returned thing needs to be deallocated by caller
    string_to_c_char(&subtitle_text)
}
/// # Safety
/// The function accepts and dereferences a raw pointer
/// The function also makes calls to functions whose safety is not guaranteed
/// The function returns a raw pointer which is a string made in C
#[no_mangle]
pub unsafe extern "C" fn _process_frame_tickertext(
    ctx: *mut lib_hardsubx_ctx,
    frame: *mut AVFrame,
    width: ::std::os::raw::c_int,
    height: ::std::os::raw::c_int,
    index: ::std::os::raw::c_int,
) -> *mut ::std::os::raw::c_char {
    let mut im: *mut Pix = pixCreate(width, height, 32);
    let mut lum_im: *mut Pix = pixCreate(width, height, 32);
    let frame_deref = *frame;

    for i in ((92 * height) / 100)..height {
        for j in 0..width {
            let p: isize = (j * 3 + i * frame_deref.linesize[0]).try_into().unwrap();
            let r: i32 = (*(frame_deref.data[0]).offset(p)).into();
            let g: i32 = (*(frame_deref.data[0]).offset(p + 1)).into();
            let b: i32 = (*(frame_deref.data[0]).offset(p + 2)).into();
            pixSetRGBPixel(im, j, i, r, g, b);

            let mut L: f32 = 0.0;
            let mut A: f32 = 0.0;
            let mut B: f32 = 0.0;

            rgb_to_lab(r as f32, g as f32, b as f32, &mut L, &mut A, &mut B);

            if L > (*ctx).lum_thresh {
                pixSetRGBPixel(lum_im, j, i, 255, 255, 255);
            } else {
                pixSetRGBPixel(lum_im, j, i, 0, 0, 0);
            }
        }
    }

    let mut gray_im: *mut Pix = pixConvertRGBToGray(im, 0.0, 0.0, 0.0);
    let mut sobel_edge_im: *mut Pix =
        pixSobelEdgeFilter(gray_im, L_VERTICAL_EDGES.try_into().unwrap());
    let mut dilate_gray_im: *mut Pix = pixDilateGray(sobel_edge_im, 21, 11);
    let mut edge_im: *mut Pix = pixThresholdToBinary(dilate_gray_im, 50);

    let mut feat_im: *mut Pix = pixCreate(width, height, 32);

    for i in (92 * (height / 100))..height {
        for j in 0..width {
            let mut p1: u32 = 0;
            let mut p2: u32 = 0;

            pixGetPixel(edge_im, j, i, &mut p1);
            pixGetPixel(lum_im, j, i, &mut p2);

            if p1 == 0 && p2 > 0 {
                pixSetRGBPixel(feat_im, j, i, 255, 255, 255);
            } else {
                pixSetRGBPixel(feat_im, j, i, 0, 0, 0);
            }
        }
    }

    let subtitle_text = get_ocr_text_simple_threshold(ctx, lum_im, 0.0);

    let write_path: String = format!("./lum_im{}.jpg", index);
    let write_path_c: *mut c_char = string_to_c_char(&write_path);
    pixWrite(write_path_c, lum_im, IFF_JFIF_JPEG.try_into().unwrap());
    let _dealloc = std::ffi::CString::from_raw(write_path_c); // for memory reasons

    let write_path: String = format!("./im{}.jpg", index);
    let write_path_c: *mut c_char = string_to_c_char(&write_path);
    pixWrite(write_path_c, lum_im, IFF_JFIF_JPEG.try_into().unwrap());
    let _dealloc = std::ffi::CString::from_raw(write_path_c); // for memory reasons

    pixDestroy(&mut im as *mut *mut Pix);
    pixDestroy(&mut gray_im as *mut *mut Pix);
    pixDestroy(&mut sobel_edge_im as *mut *mut Pix);
    pixDestroy(&mut dilate_gray_im as *mut *mut Pix);
    pixDestroy(&mut edge_im as *mut *mut Pix);
    pixDestroy(&mut lum_im as *mut *mut Pix);
    pixDestroy(&mut feat_im as *mut *mut Pix);

    subtitle_text
}
