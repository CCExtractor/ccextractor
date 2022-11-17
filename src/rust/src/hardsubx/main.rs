#[cfg(feature = "hardsubx_ocr")]
use crate::bindings::{
    ccx_s_options, dinit_encoder, encoder_cfg, encoder_ctx, init_encoder, lib_ccx_ctx,
};
#[cfg(feature = "hardsubx_ocr")]
use crate::hardsubx::{lib_hardsubx_ctx, EXIT_NOT_ENOUGH_MEMORY, EXIT_READ_ERROR};
use crate::utils::string_to_c_char;
#[cfg(feature = "hardsubx_ocr")]
use ffmpeg_sys_next::AVMediaType::AVMEDIA_TYPE_VIDEO;
#[cfg(feature = "hardsubx_ocr")]
use ffmpeg_sys_next::AVPixelFormat::*;
#[cfg(feature = "hardsubx_ocr")]
use ffmpeg_sys_next::*;
use palette::encoding::pixel::RawPixel;
use std::convert::TryInto;
use std::ffi;
use std::format;
use std::os::raw::{c_void, c_int, c_uint};
use std::process;
use std::ptr;
use std::ptr::null;
#[cfg(feature = "hardsubx_ocr")]
use tesseract_sys::*;
use super::HardsubxContext;
use super::decoder::hardsubx_process_frames_linear;
extern "C" {
    pub static mut ccx_options: ccx_s_options;
}




pub unsafe fn hardsubx_process_data(ctx: &mut HardsubxContext, _ctx_normal: *mut lib_ccx_ctx) {
    let mut format_ctx_tmp: *mut AVFormatContext = null::<AVFormatContext>() as *mut AVFormatContext;
    let file_name = string_to_c_char(&ctx.inputfile[0]);
    if avformat_open_input(
        (&mut format_ctx_tmp as *mut *mut AVFormatContext),
        file_name,
        null::<AVInputFormat>() as *const AVInputFormat,
        null::<AVDictionary>() as *mut *mut AVDictionary,
    ) != 0
    {
        eprintln!("Error reading input file!\n");
        process::exit(EXIT_READ_ERROR);
    }

    ctx.format_ctx = format_ctx_tmp;

    let format_ctx_ref = &mut *format_ctx_tmp;
    if avformat_find_stream_info(
        format_ctx_ref as *mut AVFormatContext,
        null::<*mut AVDictionary>() as *mut *mut AVDictionary,
    ) < 0
    {
        eprintln!("Error reading input stream!\n");

        process::exit(EXIT_READ_ERROR);
    }

    av_dump_format(
        format_ctx_tmp as *mut AVFormatContext,
        0,
        file_name,
        0,
    );

    ffi::CString::from_raw(file_name); //deallocation

    ctx.video_stream_id = -1;

    for i in 0..format_ctx_ref.nb_streams {
        if (*(**format_ctx_ref
            .streams
            .offset(i.try_into().unwrap()))
        .codecpar)
            .codec_type
            == AVMEDIA_TYPE_VIDEO
        {
            ctx.video_stream_id = i as i32;
            break;
        }
    }

    if ctx.video_stream_id == -1 {
        eprintln!("Video Stream not found!\n");

        process::exit(EXIT_READ_ERROR);
    }

    let mut codec_ctx: *mut AVCodecContext = avcodec_alloc_context3(null::<AVCodec>());

    let codec_par_ptr: *mut AVCodecParameters = (*(*format_ctx_ref.streams)
        .offset(ctx.video_stream_id.try_into().unwrap()))
    .codecpar;
    let codec_par_ref: &mut AVCodecParameters = &mut (*codec_par_ptr);


    let status =
        avcodec_parameters_to_context(codec_ctx, codec_par_ref as *const AVCodecParameters);

    if status < 0 {
        eprintln!("Creation of AVCodecContext failed.");
        process::exit(status);
    } else {
        ctx.codec_ctx = codec_ctx;
    }

    let codec_ptr = avcodec_find_decoder((*codec_ctx).codec_id) as *mut AVCodec;

    if codec_ptr == null::<AVCodec>() as *mut AVCodec {
        eprintln!("Input codec is not supported!\n");

        process::exit(EXIT_READ_ERROR);
    } else {
        ctx.codec = codec_ptr;
    }

    if avcodec_open2(ctx.codec_ctx, ctx.codec, &mut ctx.options_dict) < 0
    {
        eprintln!("Error opening input codec!\n");
        process::exit(EXIT_READ_ERROR);
    }

    let mut frame_ptr = av_frame_alloc();
    let mut rgb_frame_ptr = av_frame_alloc();

    if frame_ptr == null::<AVFrame>() as *mut AVFrame
        || rgb_frame_ptr == null::<AVFrame>() as *mut AVFrame
    {
        eprintln!("Not enough memory to initialize frame!");

        process::exit(EXIT_NOT_ENOUGH_MEMORY);
    }

    ctx.frame = frame_ptr;
    ctx.rgb_frame = rgb_frame_ptr;

    let frame_bytes: i32 = av_image_get_buffer_size(
        AVPixelFormat::AV_PIX_FMT_RGB24,
        (*codec_ctx).width,
        (*codec_ctx).height,
        16,
    );

    let rgb_buffer_ptr = av_malloc((frame_bytes * 8).try_into().unwrap()) as *mut u8;
    ctx.rgb_buffer = rgb_buffer_ptr;

    ctx.sws_ctx = sws_getContext(
        (*codec_ctx).width,
        (*codec_ctx).height,
        (*codec_ctx).pix_fmt,
        (*codec_ctx).width,
        (*codec_ctx).height,
        AVPixelFormat::AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        null::<SwsFilter>() as *mut SwsFilter,
        null::<SwsFilter>() as *mut SwsFilter,
        null::<f64>(),
    );

    av_image_fill_arrays(
        (*rgb_frame_ptr).data.as_mut_ptr(),
        (*rgb_frame_ptr).linesize.as_mut_ptr(),
        rgb_buffer_ptr,
        AV_PIX_FMT_RGB24,
        (*codec_ctx).width,
        (*codec_ctx).height,
        1,
    );

    let mut enc_ctx_ptr = init_encoder((&mut ccx_options.enc_cfg) as *mut encoder_cfg);

    println!("Beginning burned-in subtitle detection...\n");

    if ctx.tickertext {
        // hardsubx_process_frames_tickertext(&mut ctx, enc_ctx_ptr);
    } else {
        hardsubx_process_frames_linear(ctx, enc_ctx_ptr);
    }

    dinit_encoder(&mut enc_ctx_ptr as *mut *mut encoder_ctx, 0);

    av_free(rgb_buffer_ptr as *mut u8 as *mut c_void);

    if ctx.frame != null::<AVFrame>() as *mut AVFrame {
        av_frame_free(&mut frame_ptr as *mut *mut AVFrame);
    }

    if ctx.rgb_frame != null::<AVFrame>() as *mut AVFrame {
        av_frame_free(&mut rgb_frame_ptr as *mut *mut AVFrame);
    }

    avcodec_close(codec_ctx);
    avformat_close_input(
        &mut (format_ctx_ref as *mut AVFormatContext) as *mut *mut AVFormatContext,
    );
    avcodec_free_context(&mut codec_ctx as *mut *mut AVCodecContext);
}


pub unsafe fn _dinit_hardsubx(ctx: &mut HardsubxContext)
{

    // Free OCR related stuff
    TessBaseAPIEnd(ctx.tess_handle);
    TessBaseAPIDelete(ctx.tess_handle);
}

#[no_mangle]
pub unsafe extern "C" fn hardsubx(options: *mut ccx_s_options, ctx_normal: *mut lib_ccx_ctx) {
    let mut ctx = HardsubxContext::new(options);
    hardsubx_process_data(&mut ctx, ctx_normal);
    _dinit_hardsubx(&mut ctx);
}
