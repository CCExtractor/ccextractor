#[cfg(feature = "hardsubx_ocr")]
use crate::bindings::{
    ccx_s_options, dinit_encoder, encoder_cfg, encoder_ctx, get_basename, get_file_extension,
    lib_ccx_ctx
};
#[cfg(feature = "hardsubx_ocr")]
use crate::cc_subtitle;
#[cfg(feature = "hardsubx_ocr")]
use crate::hardsubx::lib_hardsubx_ctx;
#[cfg(feature = "hardsubx_ocr")]
use crate::hardsubx::{
    hardsubx_process_frames_linear, hardsubx_process_frames_tickertext, init_encoder,
    process_hardsubx_linear_frames_and_normal_subs, EXIT_NOT_ENOUGH_MEMORY, EXIT_READ_ERROR
};
#[cfg(feature = "hardsubx_ocr")]
use ffmpeg_sys_next::AVMediaType::AVMEDIA_TYPE_VIDEO;
#[cfg(feature = "hardsubx_ocr")]
use ffmpeg_sys_next::*;
use std::convert::TryInto;
use std::format;
use std::os::raw::c_void;
use std::process;
use std::ptr::null;

extern "C" {
    pub static mut ccx_options: ccx_s_options;
}

#[no_mangle]
pub unsafe extern "C" fn hardsubx_process_data(
    ctx: *mut lib_hardsubx_ctx,
    ctx_normal: *mut lib_ccx_ctx,
) -> ::std::os::raw::c_int {
    let mut ctx_deref: lib_hardsubx_ctx = *ctx;
    if avformat_open_input(
        (ctx_deref.format_ctx) as *mut *mut AVFormatContext,
        (ctx_deref.inputfile.offset(0)) as *const i8,
        null::<AVInputFormat>() as *const AVInputFormat,
        null::<AVDictionary>() as *mut *mut AVDictionary,
    ) != 0
    {
        eprintln!("Error reading input file!\n");

        process::exit(EXIT_READ_ERROR)
    }

    if avformat_find_stream_info(
        ctx_deref.format_ctx,
        null::<*mut AVDictionary>() as *mut *mut AVDictionary,
    ) < 0
    {
        eprintln!("Error reading input stream!\n");

        process::exit(EXIT_READ_ERROR);
    }

    av_dump_format(ctx_deref.format_ctx, 0, *ctx_deref.inputfile.offset(0), 0);

    ctx_deref.video_stream_id = -1;

    for i in 0..(*ctx_deref.format_ctx).nb_streams {
        if (*(*(*(*ctx_deref.format_ctx).streams.offset(i as isize))).codecpar).codec_type
            == AVMEDIA_TYPE_VIDEO
        {
            ctx_deref.video_stream_id = i as i32;
            break;
        }
    }

    if ctx_deref.video_stream_id == -1 {
        eprintln!("Video Stream not found!\n");

        process::exit(EXIT_READ_ERROR);
    }

    let mut codec_ctx: *mut AVCodecContext = avcodec_alloc_context3(null::<AVCodec>());

    let codec_par: *mut AVCodecParameters = (*(*(*ctx_deref.format_ctx)
        .streams
        .offset(ctx_deref.video_stream_id.try_into().unwrap())))
    .codecpar;

    let status = avcodec_parameters_to_context(codec_ctx, codec_par);

    if status < 0 {
        eprintln!("Creation of AVCodecContext failed.");
        process::exit(status);
    } else {
        ctx_deref.codec_ctx = codec_ctx;
    }

    ctx_deref.codec = avcodec_find_decoder((*ctx_deref.codec_ctx).codec_id) as *mut AVCodec;

    if ctx_deref.codec == null::<AVCodec>() as *mut AVCodec {
        eprintln!("Input codec is not supported!\n");

        process::exit(EXIT_READ_ERROR);
    }

    ctx_deref.frame = av_frame_alloc();
    ctx_deref.rgb_frame = av_frame_alloc();

    if ctx_deref.frame != null::<AVFrame>() as *mut AVFrame
        || ctx_deref.rgb_frame != null::<AVFrame>() as *mut AVFrame
    {
        eprintln!("Not enough memory to initialize frame!");

        process::exit(EXIT_NOT_ENOUGH_MEMORY);
    }

    let frame_bytes: i32 = av_image_get_buffer_size(
        AVPixelFormat::AV_PIX_FMT_RGB24,
        (*ctx_deref.codec_ctx).width,
        (*ctx_deref.codec_ctx).height,
        16,
    );

    ctx_deref.rgb_buffer = av_malloc((frame_bytes * 8).try_into().unwrap()) as *mut u8;

    ctx_deref.sws_ctx = sws_getContext(
        (*ctx_deref.codec_ctx).width,
        (*ctx_deref.codec_ctx).height,
        (*ctx_deref.codec_ctx).pix_fmt,
        (*ctx_deref.codec_ctx).width,
        (*ctx_deref.codec_ctx).height,
        AVPixelFormat::AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        null::<SwsFilter>() as *mut SwsFilter,
        null::<SwsFilter>() as *mut SwsFilter,
        null::<f64>(),
    );

    av_image_fill_arrays(
        (*ctx_deref.rgb_frame).data.as_mut_ptr(),
        (*ctx_deref.rgb_frame).linesize.as_mut_ptr(),
        ctx_deref.rgb_buffer,
        AVPixelFormat::AV_PIX_FMT_RGB24,
        (*ctx_deref.codec_ctx).width,
        (*ctx_deref.codec_ctx).height,
        1,
    );

    let mut enc_ctx: *mut encoder_ctx;
    enc_ctx = init_encoder(&mut ccx_options.enc_cfg);

    println!("Beginning burned-in subtitle detection...\n");

    if ctx_deref.tickertext != 0 {
        hardsubx_process_frames_tickertext(ctx, enc_ctx);
    } else if ctx_deref.hardsubx_and_common != 0 {
        process_hardsubx_linear_frames_and_normal_subs(ctx, enc_ctx, ctx_normal);
    } else {
        hardsubx_process_frames_linear(ctx, enc_ctx);
    }

    dinit_encoder(&mut enc_ctx as *mut *mut encoder_ctx, 0);

    av_free(ctx_deref.rgb_buffer as *mut c_void);

    if ctx_deref.frame != null::<AVFrame>() as *mut AVFrame {
        av_frame_free(&mut ctx_deref.frame as *mut *mut AVFrame);
    }

    if ctx_deref.rgb_frame != null::<AVFrame>() as *mut AVFrame {
        av_frame_free(&mut ctx_deref.rgb_frame as *mut *mut AVFrame);
    }

    avcodec_close(ctx_deref.codec_ctx);
    avformat_close_input(&mut ctx_deref.format_ctx as *mut *mut AVFormatContext);
    avcodec_free_context(&mut codec_ctx as *mut *mut AVCodecContext);
    // this only exists for backwards compatibility
    // TODO: remove this and change the function definition to void return type
    return 0;
}

// pub extern "C" fn _init_hardsubx(options: *mut ccx_s_options) -> lib_hardsubx_ctx
// {

//     let initial_codec_context: *mut AVCodecContext = avcodec_alloc_context3(null::<AVCodec>() as *mut AVCodec);
//     let tess_handle = TessBaseAPICreate();
//     let pars_vec: *mut ::std::os::raw::c_char = CString::new("debug_file").unwrap().into_raw();
//     let pars_values: *mut ::std::os::raw::c_char = CString::new("/dev/null").unwrap().into_raw();

//     let lang: *mut ::std::os::raw::c_char = (*options).ocrlang;

//     if(lang == null::<::std::os::raw::c_char>())
//     {
//         // default language is english
//         lang = CString::new("eng").unwrap().into_raw();
//     }

//     let tessdata_path: *mut ::std::os::raw::c_char = probe_tessdata_location(lang);

//     let lang_cstr = CStr::from_ptr(lang).to_string_lossy().into_owned();

//     if tessdata_path == null::<::std::os::raw::c_char>() && lang_cstr != "eng"
//     {
//         warn!("{}.traineddata not found! Switching to English", "eng");
//         tessdata_path = probe_tessdata_location("eng");
//     } else if(tessdata_path == null::<::std::os::raw::c_char>())
//     {
//         eprintln!("eng.traineddata not found! No Switching Possible\n");
//         process::exit();
//     }

//     let ret: i32 = {
//         if ccx_options.ocr_oem < 0
//         {
//             ccx_options.ocr_oem = 1;
//         }

//         TessBaseAPIInit4(tess_handle, tessdata_path, lang, ccx_options.ocr_mode, null(), 0, &*pars_vec, &*pars_values, 1, false)
//     };

//     // deallocation
//     CStr::from_raw(pars_vec);
//     CStr::from_raw(pars_values);

//     if ret != 0
//     {
//         eprintln!("Not enough memory to initialize Tesseract");
//         process::exit(EXIT_NOT_ENOUGH_MEMORY);
//     }
//     let dec_sub: cc_subtitle = cc_subtitle::new();
//     let ctx = lib_hardsubx_ctx{
//         tess_handle: tess_handle,
//         basefilename: get_basename((*options).output_filename),
//         current_file: -1,
//         inputfile: (*options).inputfile,
//         num_input_files: (*options).num_input_files,
//         extention: get_file_extension((*options).write_format),
//         write_format: (*options).write_format,
//         subs_delay: (*options).subs_delay,
//         cc_to_stdout: (*options).cc_to_stdout,

//         // initialize subtitle text parameters
//         tickertext: (*options).tickertext,
//         cur_conf: 0.0,
//         ocr_mode: (*options).hardsubx_ocr_mode,
//         subcolor: (*options).hardsubx_subcolor,
//         min_sub_duration: (*options).hardsubx_min_sub_duration,
//         conf_thresh: (*options).conf_thresh,
//         hue: (*options).hardsubx_hue,
//         lum_thresh: (*options).hardsubx_lum_thresh,
//         hardsubx_and_common: (*options).hardsubx_and_common,
//         dec_sub: &mut dec_sub

//     };

//     return ctx;
// }

// pub extern "C" fn hardsubx(options: *mut ccx_s_options, ctx_normal: *mut lib_ccx_ctx)
// {

// }
