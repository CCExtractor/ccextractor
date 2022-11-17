#[cfg(feature = "hardsubx_ocr")]
use ffmpeg_sys_next::*;
#[cfg(feature = "hardsubx_ocr")]
use leptonica_sys::*;

// #[cfg(feature = "hardsubx_ocr")]
// use ffmpeg_sys_next::*;

use std::convert::TryInto;
use std::eprintln;
use std::ffi;
use std::format;
use std::io::empty;
use std::os::raw::c_char;
use std::process::exit;
use std::ptr::null;

use crate::bindings::{activity_progress, add_cc_sub_text, cc_subtitle, encode_sub, encoder_ctx};
#[cfg(feature = "hardsubx_ocr")]
// use crate::bindings::{hardsubx_ocr_mode_HARDSUBX_OCRMODE_WORD};
use crate::hardsubx::classifier::*;
use crate::hardsubx::imgops::{rgb_to_hsv, rgb_to_lab};
use crate::hardsubx::lib_hardsubx_ctx;
use crate::utils::string_to_c_char;

use super::hardsubx_color_type;
use super::hardsubx_ocr_mode;
use super::utility::*;
use super::HardsubxContext;
use super::CCX_ENC_UTF_8;

use std::{cmp, num};

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
pub unsafe fn dispatch_classifier_functions(ctx: &mut HardsubxContext, im: *mut Pix) -> String {
    // function that calls the classifier functions
    match ctx.ocr_mode {
        hardsubx_ocr_mode::HARDSUBX_OCRMODE_FRAME => {
            get_ocr_text_wordwise_threshold(ctx, im, (*ctx).conf_thresh)
        }

        hardsubx_ocr_mode::HARDSUBX_OCRMODE_LETTER => {
            get_ocr_text_letterwise_threshold(ctx, im, (*ctx).conf_thresh)
        }

        hardsubx_ocr_mode::HARDSUBX_OCRMODE_LETTER => {
            get_ocr_text_simple_threshold(ctx, im, (*ctx).conf_thresh)
        }

        _ => {
            eprintln!("Invalid OCR Mode");
            exit(EXIT_MALFORMED_PARAMETER);
            // String::new()
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
    ctx: &mut HardsubxContext,
    frame: *mut AVFrame,
    width: ::std::os::raw::c_int,
    height: ::std::os::raw::c_int,
    _index: ::std::os::raw::c_int,
) -> String {
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

            if L > ctx.lum_thresh {
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

    if ctx.detect_italics {
        ctx.ocr_mode = hardsubx_ocr_mode::HARDSUBX_OCRMODE_WORD;
    }

    let subtitle_text = dispatch_classifier_functions(ctx, feat_im);

    pixDestroy(&mut im as *mut *mut Pix);
    pixDestroy(&mut gray_im as *mut *mut Pix);
    pixDestroy(&mut sobel_edge_im as *mut *mut Pix);
    pixDestroy(&mut dilate_gray_im as *mut *mut Pix);
    pixDestroy(&mut edge_im as *mut *mut Pix);
    pixDestroy(&mut lum_im as *mut *mut Pix);
    pixDestroy(&mut feat_im as *mut *mut Pix);

    subtitle_text
}

/// # Safety
/// The function dereferences a raw pointer
/// The function also calls other functions whose safety is not guaranteed
/// The function returns a raw pointer of a String created in Rust
/// This has to be deallocated at some point using from_raw() lest it be a memory leak
#[no_mangle]
pub unsafe extern "C" fn _process_frame_color_basic(
    ctx: &mut HardsubxContext,
    frame: *mut AVFrame,
    width: ::std::os::raw::c_int,
    height: ::std::os::raw::c_int,
    _index: ::std::os::raw::c_int,
) -> String {
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

            if ((H - ctx.hue).abs()) < 20.0 {
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

    if ctx.detect_italics {
        ctx.ocr_mode = hardsubx_ocr_mode::HARDSUBX_OCRMODE_WORD;
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
    subtitle_text
}
/// # Safety
/// The function accepts and dereferences a raw pointer
/// The function also makes calls to functions whose safety is not guaranteed
/// The function returns a raw pointer which is a string made in C
#[no_mangle]
pub unsafe extern "C" fn _process_frame_tickertext(
    ctx: &mut HardsubxContext,
    frame: *mut AVFrame,
    width: ::std::os::raw::c_int,
    height: ::std::os::raw::c_int,
    index: ::std::os::raw::c_int,
) -> String {
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

            if L > ctx.lum_thresh {
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

pub unsafe fn hardsubx_process_frames_linear(ctx: &mut HardsubxContext, enc_ctx: *mut encoder_ctx) {
    let mut prev_sub_encoded: bool = true;
    let mut got_frame = 0;
    let mut dist = 0;
    let mut cur_sec = 0;
    let mut total_sec = 0;
    let mut progress = 0;

    let mut frame_number = 0;

    let mut prev_begin_time: i64 = 0;
    let mut prev_end_time: i64 = 0;

    let mut prev_packet_pts: i64 = 0;

    let mut subtitle_text: String = String::new();
    let mut prev_subtitle_text: String = String::new();

    while av_read_frame(ctx.format_ctx, &mut ctx.packet as *mut AVPacket) >= 0 {
        if (ctx.packet.stream_index == ctx.video_stream_id) {
            frame_number += 1;

            let mut status = avcodec_send_packet(ctx.codec_ctx, &mut ctx.packet as *mut AVPacket);
            // status = avcodec_receive_frame(ctx.codec_ctx, ctx.frame);

            if status >= 0 || status == AVERROR(EAGAIN) || status == AVERROR_EOF {
                if status >= 0 {
                    ctx.packet.size = 0;
                }

                status = avcodec_receive_frame(ctx.codec_ctx, ctx.frame);

                if status == 0 {
                    got_frame = 1;
                }
            }

            if got_frame != 0 && frame_number % 25 == 0 {
                let diff = convert_pts_to_ms(
                    ctx.packet.pts - prev_packet_pts,
                    (**(*ctx.format_ctx)
                        .streams
                        .offset(ctx.video_stream_id.try_into().unwrap()))
                    .time_base,
                );

                if (diff.abs() as f32) < 1000.0 * ctx.min_sub_duration {
                    continue;
                }

                sws_scale(
                    ctx.sws_ctx,
                    (*ctx.frame).data.as_ptr() as *const *const u8,
                    (*ctx.frame).linesize.as_mut_ptr(),
                    0,
                    (*ctx.codec_ctx).height,
                    (*ctx.rgb_frame).data.as_mut_ptr(),
                    (*ctx.rgb_frame).linesize.as_mut_ptr(),
                );

                subtitle_text = match ctx.subcolor {
                    hardsubx_color_type::HARDSUBX_COLOR_WHITE => _process_frame_white_basic(
                        ctx,
                        ctx.rgb_frame,
                        (*ctx.codec_ctx).width,
                        (*ctx.codec_ctx).height,
                        frame_number,
                    ),
                    _ => _process_frame_color_basic(
                        ctx,
                        ctx.rgb_frame,
                        (*ctx.codec_ctx).width,
                        (*ctx.codec_ctx).height,
                        frame_number,
                    ),
                };

                cur_sec = convert_pts_to_s(
                    ctx.packet.pts,
                    (**(*ctx.format_ctx)
                        .streams
                        .offset(ctx.video_stream_id.try_into().unwrap()))
                    .time_base,
                );
                total_sec = convert_pts_to_s((*ctx.format_ctx).duration, AV_TIME_BASE_Q);

                progress = (cur_sec * 100) / total_sec;
                activity_progress(
                    progress.try_into().unwrap(),
                    (cur_sec / 60).try_into().unwrap(),
                    (cur_sec % 60).try_into().unwrap(),
                );

                if subtitle_text.is_empty() && prev_subtitle_text.is_empty() {
                    prev_end_time = convert_pts_to_ms(
                        ctx.packet.pts,
                        (**(*ctx.format_ctx)
                            .streams
                            .offset(ctx.video_stream_id.try_into().unwrap()))
                        .time_base,
                    );
                }

                if !subtitle_text.is_empty() {
                    let double_enter = subtitle_text.find("\n\n");
                    match double_enter {
                        Some(T) => {
                            subtitle_text = subtitle_text[0..T].to_string();
                        }
                        _ => {}
                    }
                }

                if !prev_sub_encoded && !prev_subtitle_text.is_empty() {
                    if !subtitle_text.is_empty() {
                        dist = edit_distance_string(&subtitle_text, &prev_subtitle_text);
                        if (dist as f32)
                            < 0.2
                                * (cmp::min(
                                    subtitle_text.chars().count(),
                                    prev_subtitle_text.chars().count(),
                                ) as f32)
                        {
                            dist = -1;
                            subtitle_text = String::new();
                            prev_end_time = convert_pts_to_ms(
                                ctx.packet.pts,
                                (**(*ctx.format_ctx)
                                    .streams
                                    .offset(ctx.video_stream_id.try_into().unwrap()))
                                .time_base,
                            );
                        }
                    }

                    if dist != -1 {
                        let sub_text_chr = string_to_c_char(&subtitle_text);
                        let prev_text_chr = string_to_c_char(&prev_subtitle_text);
                        let empty_chr = string_to_c_char("");
                        let mode_chr = string_to_c_char("BURN");
                        add_cc_sub_text(
                            &mut *ctx.dec_sub as *mut cc_subtitle,
                            prev_text_chr,
                            prev_begin_time,
                            prev_begin_time,
                            empty_chr,
                            mode_chr,
                            CCX_ENC_UTF_8,
                        );

                        encode_sub(enc_ctx, &mut *ctx.dec_sub);

                        // Deallocation
                        subtitle_text = ffi::CString::from_raw(sub_text_chr)
                            .to_string_lossy()
                            .into_owned();
                        prev_subtitle_text = ffi::CString::from_raw(prev_text_chr)
                            .to_string_lossy()
                            .into_owned();
                        ffi::CString::from_raw(empty_chr);
                        ffi::CString::from_raw(mode_chr);

                        prev_begin_time = prev_end_time + 1;
                        prev_subtitle_text = String::new();
                        prev_sub_encoded = true;
                        prev_end_time = convert_pts_to_ms(
                            ctx.packet.pts,
                            (**(*ctx.format_ctx)
                                .streams
                                .offset(ctx.video_stream_id.try_into().unwrap()))
                            .time_base,
                        );

                        if !subtitle_text.is_empty() {
                            prev_subtitle_text = subtitle_text.clone();
                            prev_sub_encoded = false;
                        }
                    }
                    dist = 0;
                }

                if prev_subtitle_text.is_empty() && !subtitle_text.is_empty() {
                    prev_begin_time = prev_end_time + 1;
                    prev_end_time = convert_pts_to_ms(
                        ctx.packet.pts,
                        (**(*ctx.format_ctx)
                            .streams
                            .offset(ctx.video_stream_id.try_into().unwrap()))
                        .time_base,
                    );
                    prev_subtitle_text = subtitle_text.clone();
                    prev_sub_encoded = false;
                }
                prev_packet_pts = ctx.packet.pts;
            }
        }
        av_packet_unref(&mut ctx.packet as *mut AVPacket);
    }

    if !prev_sub_encoded {
        let sub_text_chr = string_to_c_char(&subtitle_text);
        let prev_text_chr = string_to_c_char(&prev_subtitle_text);
        let empty_chr = string_to_c_char("");
        let mode_chr = string_to_c_char("BURN");
        add_cc_sub_text(
            &mut *ctx.dec_sub as *mut cc_subtitle,
            prev_text_chr,
            prev_begin_time,
            prev_begin_time,
            empty_chr,
            mode_chr,
            CCX_ENC_UTF_8,
        );

        // Deallocation
        subtitle_text = ffi::CString::from_raw(sub_text_chr)
            .to_string_lossy()
            .into_owned();
        prev_subtitle_text = ffi::CString::from_raw(prev_text_chr)
            .to_string_lossy()
            .into_owned();
        ffi::CString::from_raw(empty_chr);
        ffi::CString::from_raw(mode_chr);

        encode_sub(enc_ctx, &mut *ctx.dec_sub as *mut cc_subtitle);
        prev_sub_encoded = true;
    }

    activity_progress(
        100,
        (cur_sec / 60).try_into().unwrap(),
        (cur_sec % 60).try_into().unwrap(),
    );
}

pub unsafe fn hardsubx_process_frames_tickertext(
    ctx: &mut HardsubxContext,
    enc_ctx: *mut encoder_ctx,
) {
    let mut got_frame: bool = false;

    let mut cur_sec: i64 = 0;
    let mut total_sec: i64 = 0;
    let mut progress: i64 = 0;

    let mut frame_number = 0;

    let mut ticker_text = String::new();

    while (av_read_frame(ctx.format_ctx, &mut ctx.packet) >= 0) {
        if ctx.packet.stream_index == ctx.video_stream_id {
            frame_number += 1;

            let mut status = avcodec_send_packet(ctx.codec_ctx, &mut ctx.packet as *mut AVPacket);
            // status = avcodec_receive_frame(ctx.codec_ctx, ctx.frame);

            if status >= 0 || status == AVERROR(EAGAIN) || status == AVERROR_EOF {
                if status >= 0 {
                    ctx.packet.size = 0;
                }

                status = avcodec_receive_frame(ctx.codec_ctx, ctx.frame);

                if status == 0 {
                    got_frame = true;
                }
            }

            if got_frame && frame_number % 1000 == 0 {
                sws_scale(
                    ctx.sws_ctx,
                    (*ctx.frame).data.as_ptr() as *const *const u8,
                    (*ctx.frame).linesize.as_mut_ptr(),
                    0,
                    (*ctx.codec_ctx).height,
                    (*ctx.rgb_frame).data.as_ptr() as *const *mut u8,
                    (*ctx.rgb_frame).linesize.as_mut_ptr(),
                );

                ticker_text = _process_frame_tickertext(
                    ctx,
                    ctx.rgb_frame,
                    (*ctx.codec_ctx).width,
                    (*ctx.codec_ctx).height,
                    frame_number,
                );
                println!("frame_number: {}", frame_number);

                if !ticker_text.is_empty() {
                    println!("{}", ticker_text);
                }

                cur_sec = convert_pts_to_ms(
                    ctx.packet.pts,
                    (**(*ctx.format_ctx)
                        .streams
                        .offset(ctx.video_stream_id.try_into().unwrap()))
                    .time_base,
                );

                total_sec = convert_pts_to_s((*ctx.format_ctx).duration, AV_TIME_BASE_Q);
                progress = (cur_sec * 100) / total_sec;

                activity_progress(
                    progress.try_into().unwrap(),
                    (cur_sec / 60).try_into().unwrap(),
                    (cur_sec % 60).try_into().unwrap(),
                );
            }
        }

        av_packet_unref(&mut ctx.packet);
    }

    activity_progress(
        100,
        (cur_sec / 60).try_into().unwrap(),
        (cur_sec % 60).try_into().unwrap(),
    );
}
