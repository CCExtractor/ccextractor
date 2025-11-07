use crate::avc::common_types::AvcContextRust;
use crate::bindings::*;
use crate::ctorust::FromCType;
use crate::demuxer::common_types::{
    CapInfo, CcxDemuxReport, CcxRational, PMTEntry, PSIBuffer, ProgramInfo,
};
use crate::utils::null_pointer;
use crate::utils::string_to_c_char;
use crate::utils::string_to_c_chars;
use lib_ccxr::common::Decoder608Report;
use lib_ccxr::common::Decoder608Settings;
use lib_ccxr::common::DecoderDtvccReport;
use lib_ccxr::common::DecoderDtvccSettings;
use lib_ccxr::common::DemuxerConfig;
use lib_ccxr::common::DtvccServiceCharset;
use lib_ccxr::common::EncoderConfig;
use lib_ccxr::common::EncodersTranscriptFormat;
use lib_ccxr::common::Language;
use lib_ccxr::common::Options;
use lib_ccxr::common::OutputFormat;
use lib_ccxr::common::SelectCodec;
use lib_ccxr::common::StreamMode;
use lib_ccxr::common::StreamType;
use lib_ccxr::common::{BufferdataType, CommonTimingCtx};
use lib_ccxr::common::{Codec, DataSource};
use lib_ccxr::hardsubx::ColorHue;
use lib_ccxr::hardsubx::OcrMode;
use lib_ccxr::teletext::TeletextConfig;
use lib_ccxr::time::units::Timestamp;
use lib_ccxr::time::units::TimestampFormat;
use lib_ccxr::util::encoding::Encoding;
use lib_ccxr::util::log::{DebugMessageMask, OutputTarget};
use std::os::raw::{c_int, c_long};
use std::path::PathBuf;
use std::str::FromStr;

pub trait FromC<T> {
    fn from_c(value: T) -> Self;
}

pub trait CType<T> {
    /// # Safety
    /// This function is unsafe because it dereferences the pointer passed to it.
    unsafe fn to_ctype(&self) -> T;
}
pub trait CType2<T, U> {
    /// # Safety
    /// This function is unsafe because it dereferences the pointer passed to it.
    unsafe fn to_ctype(&self, value: U) -> T;
}
pub trait FromRust<T> {
    /// # Safety
    /// This function is unsafe because it dereferences the pointer passed to it.
    unsafe fn copy_from_rust(&mut self, options: T);
}

/// Convert the rust struct (CcxOptions) to C struct (ccx_s_options), so that it can be used by the C code.
///
/// Using the FromRust trait here requires a &mut self on the global variable ccx_options. This leads to
/// the warning: creating a mutable reference to mutable static is discouraged. So we instead pass a raw pointer
///
/// # Safety
///
/// This function is unsafe because we are modifying a global static mut variable
/// and we are dereferencing the pointer passed to it.
pub unsafe fn copy_from_rust(ccx_s_options: *mut ccx_s_options, options: Options) {
    (*ccx_s_options).extract = options.extract as _;
    (*ccx_s_options).no_rollup = options.no_rollup as _;
    (*ccx_s_options).noscte20 = options.noscte20 as _;
    (*ccx_s_options).webvtt_create_css = options.webvtt_create_css as _;
    (*ccx_s_options).cc_channel = options.cc_channel as _;
    (*ccx_s_options).buffer_input = options.buffer_input as _;
    (*ccx_s_options).nofontcolor = options.nofontcolor as _;
    (*ccx_s_options).write_format = options.write_format.to_ctype();
    (*ccx_s_options).send_to_srv = options.send_to_srv as _;
    (*ccx_s_options).nohtmlescape = options.nohtmlescape as _;
    (*ccx_s_options).notypesetting = options.notypesetting as _;
    (*ccx_s_options).extraction_start = options.extraction_start.to_ctype();
    (*ccx_s_options).extraction_end = options.extraction_end.to_ctype();
    (*ccx_s_options).print_file_reports = options.print_file_reports as _;
    (*ccx_s_options).settings_608 = options.settings_608.to_ctype();
    (*ccx_s_options).settings_dtvcc = options.settings_dtvcc.to_ctype();
    (*ccx_s_options).is_608_enabled = options.is_608_enabled as _;
    (*ccx_s_options).is_708_enabled = options.is_708_enabled as _;
    (*ccx_s_options).millis_separator = options.millis_separator() as _;
    (*ccx_s_options).binary_concat = options.binary_concat as _;
    (*ccx_s_options).use_gop_as_pts = if let Some(usegops) = options.use_gop_as_pts {
        if usegops {
            1
        } else {
            -1
        }
    } else {
        0
    };
    (*ccx_s_options).fix_padding = options.fix_padding as _;
    (*ccx_s_options).gui_mode_reports = options.gui_mode_reports as _;
    (*ccx_s_options).no_progress_bar = options.no_progress_bar as _;

    if options.sentence_cap_file.try_exists().unwrap_or_default() {
        (*ccx_s_options).sentence_cap_file = string_to_c_char(
            options
                .sentence_cap_file
                .clone()
                .to_str()
                .unwrap_or_default(),
        );
    }

    (*ccx_s_options).live_stream = if let Some(live_stream) = options.live_stream {
        live_stream.seconds() as _
    } else {
        -1
    };
    if options
        .filter_profanity_file
        .try_exists()
        .unwrap_or_default()
    {
        (*ccx_s_options).filter_profanity_file = string_to_c_char(
            options
                .filter_profanity_file
                .clone()
                .to_str()
                .unwrap_or_default(),
        );
    }
    (*ccx_s_options).messages_target = options.messages_target as _;
    (*ccx_s_options).timestamp_map = options.timestamp_map as _;
    (*ccx_s_options).dolevdist = options.dolevdist.into();
    (*ccx_s_options).levdistmincnt = options.levdistmincnt as _;
    (*ccx_s_options).levdistmaxpct = options.levdistmaxpct as _;
    (*ccx_s_options).investigate_packets = options.investigate_packets as _;
    (*ccx_s_options).fullbin = options.fullbin as _;
    (*ccx_s_options).nosync = options.nosync as _;
    (*ccx_s_options).hauppauge_mode = options.hauppauge_mode as _;
    (*ccx_s_options).wtvconvertfix = options.wtvconvertfix as _;
    (*ccx_s_options).wtvmpeg2 = options.wtvmpeg2 as _;
    (*ccx_s_options).auto_myth = if let Some(auto_myth) = options.auto_myth {
        auto_myth as _
    } else {
        2
    };
    (*ccx_s_options).mp4vidtrack = options.mp4vidtrack as _;
    (*ccx_s_options).extract_chapters = options.extract_chapters as _;
    (*ccx_s_options).usepicorder = options.usepicorder as _;
    (*ccx_s_options).xmltv = options.xmltv as _;
    (*ccx_s_options).xmltvliveinterval = options.xmltvliveinterval.seconds() as _;
    (*ccx_s_options).xmltvoutputinterval = options.xmltvoutputinterval.seconds() as _;
    (*ccx_s_options).xmltvonlycurrent = options.xmltvonlycurrent.into();
    (*ccx_s_options).keep_output_closed = options.keep_output_closed as _;
    (*ccx_s_options).force_flush = options.force_flush as _;
    (*ccx_s_options).append_mode = options.append_mode as _;
    (*ccx_s_options).ucla = options.ucla as _;
    (*ccx_s_options).tickertext = options.tickertext as _;
    (*ccx_s_options).hardsubx = options.hardsubx as _;
    (*ccx_s_options).hardsubx_and_common = options.hardsubx_and_common as _;
    if let Some(dvblang) = options.dvblang {
        (*ccx_s_options).dvblang = string_to_c_char(dvblang.to_ctype().as_str());
    }
    if let Some(ocrlang) = options.ocrlang {
        (*ccx_s_options).ocrlang = string_to_c_char(ocrlang.to_ctype().as_str());
    }
    (*ccx_s_options).ocr_oem = options.ocr_oem as _;
    (*ccx_s_options).psm = options.psm as _;
    (*ccx_s_options).ocr_quantmode = options.ocr_quantmode as _;
    if let Some(mkvlang) = options.mkvlang {
        (*ccx_s_options).mkvlang = string_to_c_char(mkvlang.to_ctype().as_str());
    }
    (*ccx_s_options).analyze_video_stream = options.analyze_video_stream as _;
    (*ccx_s_options).hardsubx_ocr_mode = options.hardsubx_ocr_mode.to_ctype();
    (*ccx_s_options).hardsubx_subcolor = options.hardsubx_hue.to_ctype();
    (*ccx_s_options).hardsubx_min_sub_duration = options.hardsubx_min_sub_duration.seconds() as _;
    (*ccx_s_options).hardsubx_detect_italics = options.hardsubx_detect_italics as _;
    (*ccx_s_options).hardsubx_conf_thresh = options.hardsubx_conf_thresh as _;
    (*ccx_s_options).hardsubx_hue = options.hardsubx_hue.get_hue() as _;
    (*ccx_s_options).hardsubx_lum_thresh = options.hardsubx_lum_thresh as _;
    (*ccx_s_options).transcript_settings = options.transcript_settings.to_ctype();
    (*ccx_s_options).date_format = options.date_format.to_ctype();
    (*ccx_s_options).write_format_rewritten = options.write_format_rewritten as _;
    (*ccx_s_options).use_ass_instead_of_ssa = options.use_ass_instead_of_ssa as _;
    (*ccx_s_options).use_webvtt_styling = options.use_webvtt_styling as _;
    (*ccx_s_options).debug_mask = options.debug_mask.normal_mask().bits() as _;
    (*ccx_s_options).debug_mask_on_debug = options.debug_mask.debug_mask().bits() as _;
    if options.udpsrc.is_some() {
        (*ccx_s_options).udpsrc = string_to_c_char(&options.udpsrc.clone().unwrap());
    }
    if options.udpaddr.is_some() {
        (*ccx_s_options).udpaddr = string_to_c_char(&options.udpaddr.clone().unwrap());
    }
    (*ccx_s_options).udpport = options.udpport as _;
    if options.tcpport.is_some() {
        (*ccx_s_options).tcpport = string_to_c_char(&options.tcpport.unwrap().to_string());
    }
    if options.tcp_password.is_some() {
        (*ccx_s_options).tcp_password = string_to_c_char(&options.tcp_password.clone().unwrap());
    }
    if options.tcp_desc.is_some() {
        (*ccx_s_options).tcp_desc = string_to_c_char(&options.tcp_desc.clone().unwrap());
    }
    if options.srv_addr.is_some() {
        (*ccx_s_options).srv_addr = string_to_c_char(&options.srv_addr.clone().unwrap());
    }
    if options.srv_port.is_some() {
        (*ccx_s_options).srv_port = string_to_c_char(&options.srv_port.unwrap().to_string());
    }
    (*ccx_s_options).noautotimeref = options.noautotimeref as _;
    (*ccx_s_options).input_source = options.input_source as _;
    if options.output_filename.is_some() {
        (*ccx_s_options).output_filename =
            string_to_c_char(&options.output_filename.clone().unwrap());
    }
    if options.inputfile.is_some() {
        (*ccx_s_options).inputfile = string_to_c_chars(options.inputfile.clone().unwrap());
        (*ccx_s_options).num_input_files =
            options.inputfile.iter().filter(|s| !s.is_empty()).count() as _;
    }
    (*ccx_s_options).demux_cfg = options.demux_cfg.to_ctype();
    (*ccx_s_options).enc_cfg = options.enc_cfg.to_ctype();
    (*ccx_s_options).subs_delay = options.subs_delay.millis();
    (*ccx_s_options).cc_to_stdout = options.cc_to_stdout as _;
    (*ccx_s_options).pes_header_to_stdout = options.pes_header_to_stdout as _;
    (*ccx_s_options).ignore_pts_jumps = options.ignore_pts_jumps as _;
    (*ccx_s_options).multiprogram = options.multiprogram as _;
    (*ccx_s_options).out_interval = options.out_interval;
    (*ccx_s_options).segment_on_key_frames_only = options.segment_on_key_frames_only as _;
    #[cfg(feature = "with_libcurl")]
    {
        if options.curlposturl.is_some() {
            (*ccx_s_options).curlposturl =
                string_to_c_char(&options.curlposturl.as_ref().unwrap_or_default().as_str());
        }
    }
}

/// Converts the C struct (ccx_s_options) to Rust struct (CcxOptions/Options), retrieving data from C code.
///
/// # Safety
///
/// This function is unsafe because we are dereferencing the pointer passed to it.
#[allow(clippy::unnecessary_cast)]
pub unsafe fn copy_to_rust(ccx_s_options: *const ccx_s_options) -> Options {
    let mut options = Options {
        extract: (*ccx_s_options).extract as u8,
        no_rollup: (*ccx_s_options).no_rollup != 0,
        noscte20: (*ccx_s_options).noscte20 != 0,
        webvtt_create_css: (*ccx_s_options).webvtt_create_css != 0,
        cc_channel: (*ccx_s_options).cc_channel as u8,
        buffer_input: (*ccx_s_options).buffer_input != 0,
        nofontcolor: (*ccx_s_options).nofontcolor != 0,
        nohtmlescape: (*ccx_s_options).nohtmlescape != 0,
        notypesetting: (*ccx_s_options).notypesetting != 0,
        // Handle extraction_start and extraction_end
        extraction_start: Some(
            Timestamp::from_hms_millis(
                (*ccx_s_options).extraction_start.hh as u8,
                (*ccx_s_options).extraction_start.mm as u8,
                (*ccx_s_options).extraction_start.ss as u8,
                0,
            )
            .expect("Invalid extraction start time"),
        ),
        extraction_end: Some(
            Timestamp::from_hms_millis(
                (*ccx_s_options).extraction_end.hh as u8,
                (*ccx_s_options).extraction_end.mm as u8,
                (*ccx_s_options).extraction_end.ss as u8,
                0,
            )
            .expect("Invalid extraction end time"),
        ),
        print_file_reports: (*ccx_s_options).print_file_reports != 0,
        // Handle settings_608 and settings_dtvcc - assuming FromCType trait is implemented for these
        settings_608: Decoder608Settings::from_ctype((*ccx_s_options).settings_608)
            .unwrap_or(Decoder608Settings::default()),
        settings_dtvcc: DecoderDtvccSettings::from_ctype((*ccx_s_options).settings_dtvcc)
            .unwrap_or(DecoderDtvccSettings::default()),
        is_608_enabled: (*ccx_s_options).is_608_enabled != 0,
        is_708_enabled: (*ccx_s_options).is_708_enabled != 0,
        // Assuming a millis_separator conversion function exists or we can use chars directly
        binary_concat: (*ccx_s_options).binary_concat != 0,
        // Handle use_gop_as_pts special case
        use_gop_as_pts: match (*ccx_s_options).use_gop_as_pts {
            1 => Some(true),
            -1 => Some(false),
            _ => None,
        },
        fix_padding: (*ccx_s_options).fix_padding != 0,
        gui_mode_reports: (*ccx_s_options).gui_mode_reports != 0,
        no_progress_bar: (*ccx_s_options).no_progress_bar != 0,
        ..Default::default()
    };

    // Handle sentence_cap_file (C string to PathBuf)
    if !(*ccx_s_options).sentence_cap_file.is_null() {
        options.sentence_cap_file =
            PathBuf::from(c_char_to_string((*ccx_s_options).sentence_cap_file));
    }

    // Handle live_stream special case
    options.live_stream = if (*ccx_s_options).live_stream < 0 {
        None
    } else {
        Some(Timestamp::from_millis(
            ((*ccx_s_options).live_stream) as i64,
        ))
    };

    // Handle filter_profanity_file (C string to PathBuf)
    if !(*ccx_s_options).filter_profanity_file.is_null() {
        options.filter_profanity_file =
            PathBuf::from(c_char_to_string((*ccx_s_options).filter_profanity_file));
    }

    options.messages_target =
        OutputTarget::from_ctype((*ccx_s_options).messages_target).unwrap_or(OutputTarget::Stdout);
    options.timestamp_map = (*ccx_s_options).timestamp_map != 0;
    options.dolevdist = (*ccx_s_options).dolevdist != 0;
    options.levdistmincnt = (*ccx_s_options).levdistmincnt as u8;
    options.levdistmaxpct = (*ccx_s_options).levdistmaxpct as u8;
    options.investigate_packets = (*ccx_s_options).investigate_packets != 0;
    options.fullbin = (*ccx_s_options).fullbin != 0;
    options.nosync = (*ccx_s_options).nosync != 0;
    options.hauppauge_mode = (*ccx_s_options).hauppauge_mode != 0;
    options.wtvconvertfix = (*ccx_s_options).wtvconvertfix != 0;
    options.wtvmpeg2 = (*ccx_s_options).wtvmpeg2 != 0;

    // Handle auto_myth special case
    options.auto_myth = match (*ccx_s_options).auto_myth {
        0 => Some(false),
        1 => Some(true),
        _ => None,
    };

    options.mp4vidtrack = (*ccx_s_options).mp4vidtrack != 0;
    options.extract_chapters = (*ccx_s_options).extract_chapters != 0;
    options.usepicorder = (*ccx_s_options).usepicorder != 0;
    options.xmltv = (*ccx_s_options).xmltv as u8;
    options.xmltvliveinterval = Timestamp::from_millis((*ccx_s_options).xmltvliveinterval as i64);
    options.xmltvoutputinterval =
        Timestamp::from_millis((*ccx_s_options).xmltvoutputinterval as i64);
    options.xmltvonlycurrent = (*ccx_s_options).xmltvonlycurrent != 0;
    options.keep_output_closed = (*ccx_s_options).keep_output_closed != 0;
    options.force_flush = (*ccx_s_options).force_flush != 0;
    options.append_mode = (*ccx_s_options).append_mode != 0;
    options.ucla = (*ccx_s_options).ucla != 0;
    options.tickertext = (*ccx_s_options).tickertext != 0;
    options.hardsubx = (*ccx_s_options).hardsubx != 0;
    options.hardsubx_and_common = (*ccx_s_options).hardsubx_and_common != 0;

    // Handle dvblang (C string to Option<Language>)
    if !(*ccx_s_options).dvblang.is_null() {
        options.dvblang = Some(
            Language::from_str(&c_char_to_string((*ccx_s_options).dvblang))
                .expect("Invalid language"),
        );
    }
    // Handle ocrlang (C string to PathBuf)
    if !(*ccx_s_options).ocrlang.is_null() {
        options.ocrlang = Some(
            Language::from_str(&c_char_to_string((*ccx_s_options).ocrlang))
                .expect("Invalid language"),
        );
    }

    options.ocr_oem = (*ccx_s_options).ocr_oem as i8;
    options.psm = (*ccx_s_options).psm;
    options.ocr_quantmode = (*ccx_s_options).ocr_quantmode as u8;

    // Handle mkvlang (C string to Option<Language>)
    if !(*ccx_s_options).mkvlang.is_null() {
        options.mkvlang = Some(
            Language::from_str(&c_char_to_string((*ccx_s_options).mkvlang))
                .expect("Invalid language"),
        )
    }

    options.analyze_video_stream = (*ccx_s_options).analyze_video_stream != 0;
    options.hardsubx_ocr_mode =
        OcrMode::from_ctype((*ccx_s_options).hardsubx_ocr_mode).unwrap_or(OcrMode::Frame);
    options.hardsubx_min_sub_duration =
        Timestamp::from_millis((*ccx_s_options).hardsubx_min_sub_duration as i64);
    options.hardsubx_detect_italics = (*ccx_s_options).hardsubx_detect_italics != 0;
    options.hardsubx_conf_thresh = (*ccx_s_options).hardsubx_conf_thresh as f64;
    options.hardsubx_hue = ColorHue::from_ctype((*ccx_s_options).hardsubx_hue as f64 as c_int)
        .unwrap_or(ColorHue::White);
    options.hardsubx_lum_thresh = (*ccx_s_options).hardsubx_lum_thresh as f64;

    // Handle transcript_settings
    options.transcript_settings =
        EncodersTranscriptFormat::from_ctype((*ccx_s_options).transcript_settings)
            .unwrap_or(EncodersTranscriptFormat::default());

    options.date_format =
        TimestampFormat::from_ctype((*ccx_s_options).date_format).unwrap_or(TimestampFormat::None);
    options.send_to_srv = (*ccx_s_options).send_to_srv != 0;
    options.write_format =
        OutputFormat::from_ctype((*ccx_s_options).write_format).unwrap_or(OutputFormat::Raw);
    options.write_format_rewritten = (*ccx_s_options).write_format_rewritten != 0;
    options.use_ass_instead_of_ssa = (*ccx_s_options).use_ass_instead_of_ssa != 0;
    options.use_webvtt_styling = (*ccx_s_options).use_webvtt_styling != 0;
    // Handle debug_mask - assuming DebugMessageMask has a constructor or from method
    options.debug_mask = DebugMessageMask::from_ctype((
        (*ccx_s_options).debug_mask as u32,
        (*ccx_s_options).debug_mask_on_debug as u32,
    ))
    .unwrap_or(DebugMessageMask::default());

    // Handle string pointers
    if !(*ccx_s_options).udpsrc.is_null() {
        options.udpsrc = Some(c_char_to_string((*ccx_s_options).udpsrc));
    }

    if !(*ccx_s_options).udpaddr.is_null() {
        options.udpaddr = Some(c_char_to_string((*ccx_s_options).udpaddr));
    }

    options.udpport = (*ccx_s_options).udpport as u16;

    if !(*ccx_s_options).tcpport.is_null() {
        options.tcpport = Some(
            c_char_to_string((*ccx_s_options).tcpport)
                .parse()
                .unwrap_or_default(),
        );
    }

    if !(*ccx_s_options).tcp_password.is_null() {
        options.tcp_password = Some(c_char_to_string((*ccx_s_options).tcp_password));
    }

    if !(*ccx_s_options).tcp_desc.is_null() {
        options.tcp_desc = Some(c_char_to_string((*ccx_s_options).tcp_desc));
    }

    if !(*ccx_s_options).srv_addr.is_null() {
        options.srv_addr = Some(c_char_to_string((*ccx_s_options).srv_addr));
    }

    if !(*ccx_s_options).srv_port.is_null() {
        options.srv_port = Some(
            c_char_to_string((*ccx_s_options).srv_port)
                .parse()
                .unwrap_or_default(),
        );
    }

    options.noautotimeref = (*ccx_s_options).noautotimeref != 0;
    options.input_source = DataSource::from((*ccx_s_options).input_source as u32);

    if !(*ccx_s_options).output_filename.is_null() {
        options.output_filename = Some(c_char_to_string((*ccx_s_options).output_filename));
    }

    // Handle inputfile (array of C strings)
    if !(*ccx_s_options).inputfile.is_null() && (*ccx_s_options).num_input_files > 0 {
        let mut inputfiles = Vec::with_capacity((*ccx_s_options).num_input_files as usize);

        for i in 0..(*ccx_s_options).num_input_files {
            let ptr = *(*ccx_s_options).inputfile.offset(i as isize);
            if !ptr.is_null() {
                inputfiles.push(c_char_to_string(ptr));
            }
        }

        if !inputfiles.is_empty() {
            options.inputfile = Some(inputfiles);
        }
    }

    // Handle demux_cfg and enc_cfg
    options.demux_cfg =
        DemuxerConfig::from_ctype((*ccx_s_options).demux_cfg).unwrap_or(DemuxerConfig::default());
    options.enc_cfg =
        EncoderConfig::from_ctype((*ccx_s_options).enc_cfg).unwrap_or(EncoderConfig::default());

    options.subs_delay = Timestamp::from_millis((*ccx_s_options).subs_delay);
    options.cc_to_stdout = (*ccx_s_options).cc_to_stdout != 0;
    options.pes_header_to_stdout = (*ccx_s_options).pes_header_to_stdout != 0;
    options.ignore_pts_jumps = (*ccx_s_options).ignore_pts_jumps != 0;
    options.multiprogram = (*ccx_s_options).multiprogram != 0;
    options.out_interval = (*ccx_s_options).out_interval;
    options.segment_on_key_frames_only = (*ccx_s_options).segment_on_key_frames_only != 0;

    // Handle optional features with conditional compilation
    #[cfg(feature = "with_libcurl")]
    if !(*ccx_s_options).curlposturl.is_null() {
        let url_str = c_char_to_string((*ccx_s_options).curlposturl);
        options.curlposturl = url_str.parse::<Url>().ok();
    }

    options
}

/// Helper function to convert C char pointer to Rust String
unsafe fn c_char_to_string(c_str: *const ::std::os::raw::c_char) -> String {
    if c_str.is_null() {
        return String::new();
    }

    std::ffi::CStr::from_ptr(c_str)
        .to_string_lossy()
        .into_owned()
}
impl CType2<ccx_s_teletext_config, &Options> for TeletextConfig {
    unsafe fn to_ctype(&self, value: &Options) -> ccx_s_teletext_config {
        let mut config = ccx_s_teletext_config {
            _bitfield_1: Default::default(),
            _bitfield_2: Default::default(),
            _bitfield_align_1: Default::default(),
            _bitfield_align_2: Default::default(),
            page: self.user_page,
            tid: 0,
            offset: 0.0,
            user_page: self.user_page,
            dolevdist: self.dolevdist.into(),
            levdistmincnt: self.levdistmincnt.into(),
            levdistmaxpct: self.levdistmaxpct.into(),
            extraction_start: self.extraction_start.to_ctype(),
            extraction_end: self.extraction_end.to_ctype(),
            write_format: self.write_format.to_ctype(),
            gui_mode_reports: value.gui_mode_reports as _,
            date_format: self.date_format.to_ctype(),
            noautotimeref: self.noautotimeref.into(),
            send_to_srv: value.send_to_srv.into(),
            encoding: value.enc_cfg.encoding.to_ctype() as _,
            nofontcolor: self.nofontcolor.into(),
            nohtmlescape: self.nohtmlescape.into(),
            millis_separator: value.millis_separator() as _,
            latrusmap: self.latrusmap.into(),
        };
        config.set_verbose(self.verbose.into());
        config.set_bom(1);
        config.set_nonempty(1);

        config
    }
}

impl CType<ccx_boundary_time> for Option<Timestamp> {
    /// Convert to C variant of `ccx_boundary_time`.
    unsafe fn to_ctype(&self) -> ccx_boundary_time {
        if self.is_none() {
            return ccx_boundary_time {
                hh: 0,
                mm: 0,
                ss: 0,
                time_in_ms: 0,
                set: 0,
            };
        }
        self.unwrap().to_ctype()
    }
}

impl CType<ccx_boundary_time> for Timestamp {
    /// Convert to C variant of `ccx_boundary_time`.
    unsafe fn to_ctype(&self) -> ccx_boundary_time {
        let (hh, mm, ss, _) = self.as_hms_millis().unwrap();
        ccx_boundary_time {
            hh: hh as _,
            mm: mm as _,
            ss: ss as _,
            time_in_ms: self.millis(),
            set: 1,
        }
    }
}

impl CType<ccx_output_date_format> for TimestampFormat {
    /// Convert to C variant of `ccx_boundary_time`.
    unsafe fn to_ctype(&self) -> ccx_output_date_format {
        match self {
            TimestampFormat::None => ccx_output_date_format::ODF_NONE,
            TimestampFormat::HHMMSS => ccx_output_date_format::ODF_HHMMSS,
            TimestampFormat::HHMMSSFFF => ccx_output_date_format::ODF_HHMMSSMS,
            TimestampFormat::Seconds {
                millis_separator: _,
            } => ccx_output_date_format::ODF_SECONDS,
            TimestampFormat::Date {
                millis_separator: _,
            } => ccx_output_date_format::ODF_DATE,
        }
    }
}

impl CType<ccx_output_format> for OutputFormat {
    /// Convert to C variant of `ccx_output_format`.
    unsafe fn to_ctype(&self) -> ccx_output_format {
        match self {
            OutputFormat::Raw => ccx_output_format::CCX_OF_RAW,
            OutputFormat::Srt => ccx_output_format::CCX_OF_SRT,
            OutputFormat::Sami => ccx_output_format::CCX_OF_SAMI,
            OutputFormat::Transcript => ccx_output_format::CCX_OF_TRANSCRIPT,
            OutputFormat::Rcwt => ccx_output_format::CCX_OF_RCWT,
            OutputFormat::Null => ccx_output_format::CCX_OF_NULL,
            OutputFormat::SmpteTt => ccx_output_format::CCX_OF_SMPTETT,
            OutputFormat::SpuPng => ccx_output_format::CCX_OF_SPUPNG,
            OutputFormat::DvdRaw => ccx_output_format::CCX_OF_DVDRAW,
            OutputFormat::WebVtt => ccx_output_format::CCX_OF_WEBVTT,
            OutputFormat::SimpleXml => ccx_output_format::CCX_OF_SIMPLE_XML,
            OutputFormat::G608 => ccx_output_format::CCX_OF_G608,
            OutputFormat::Curl => ccx_output_format::CCX_OF_CURL,
            OutputFormat::Ssa => ccx_output_format::CCX_OF_SSA,
            OutputFormat::Mcc => ccx_output_format::CCX_OF_MCC,
            OutputFormat::Scc => ccx_output_format::CCX_OF_SCC,
            OutputFormat::Ccd => ccx_output_format::CCX_OF_CCD,
        }
    }
}

impl CType<u32> for Encoding {
    /// Convert to C variant of `u32`.
    unsafe fn to_ctype(&self) -> u32 {
        match self {
            Encoding::Line21 => ccx_encoding_type_CCX_ENC_ASCII as _,
            Encoding::Latin1 => ccx_encoding_type_CCX_ENC_LATIN_1 as _,
            Encoding::UTF8 => ccx_encoding_type_CCX_ENC_UTF_8 as _,
            Encoding::UCS2 => ccx_encoding_type_CCX_ENC_UNICODE as _,
        }
    }
}

impl CType<String> for Language {
    /// Convert to C variant of `String`.
    unsafe fn to_ctype(&self) -> String {
        self.to_str().to_lowercase()
    }
}

impl CType<i32> for OcrMode {
    /// Convert to C variant of `i32`.
    unsafe fn to_ctype(&self) -> i32 {
        *self as i32
    }
}

impl CType<i32> for ColorHue {
    /// Convert to C variant of `i32`.
    unsafe fn to_ctype(&self) -> i32 {
        match self {
            ColorHue::Custom(_) => 7,
            ColorHue::White => 0,
            ColorHue::Yellow => 1,
            ColorHue::Green => 2,
            ColorHue::Cyan => 3,
            ColorHue::Blue => 4,
            ColorHue::Magenta => 5,
            ColorHue::Red => 6,
        }
    }
}

impl CType<ccx_decoder_608_settings> for Decoder608Settings {
    unsafe fn to_ctype(&self) -> ccx_decoder_608_settings {
        ccx_decoder_608_settings {
            direct_rollup: self.direct_rollup,
            force_rollup: self.force_rollup,
            no_rollup: self.no_rollup as _,
            default_color: self.default_color as _,
            screens_to_process: self.screens_to_process,
            report: if let Some(value) = self.report {
                &mut value.to_ctype()
            } else {
                std::ptr::null::<ccx_decoder_608_report>() as *mut ccx_decoder_608_report
            },
        }
    }
}

impl CType<ccx_decoder_608_report> for Decoder608Report {
    unsafe fn to_ctype(&self) -> ccx_decoder_608_report {
        let mut decoder = ccx_decoder_608_report {
            _bitfield_1: Default::default(),
            _bitfield_align_1: Default::default(),
            cc_channels: self.cc_channels,
        };
        decoder.set_xds(if self.xds { 1 } else { 0 });
        decoder
    }
}

impl CType<ccx_decoder_dtvcc_settings> for DecoderDtvccSettings {
    unsafe fn to_ctype(&self) -> ccx_decoder_dtvcc_settings {
        ccx_decoder_dtvcc_settings {
            enabled: self.enabled as _,
            print_file_reports: self.print_file_reports as _,
            no_rollup: self.no_rollup as _,
            report: if let Some(value) = self.report {
                &mut value.to_ctype()
            } else {
                std::ptr::null::<ccx_decoder_dtvcc_report>() as *mut ccx_decoder_dtvcc_report
            },
            active_services_count: self.active_services_count,
            services_enabled: self.services_enabled.map(|b| if b { 1 } else { 0 }),
            timing: &mut self.timing.to_ctype(),
        }
    }
}

impl CType<ccx_common_timing_ctx> for CommonTimingCtx {
    unsafe fn to_ctype(&self) -> ccx_common_timing_ctx {
        ccx_common_timing_ctx {
            pts_set: self.pts_set,
            min_pts_adjusted: self.min_pts_adjusted,
            current_pts: self.current_pts,
            current_picture_coding_type: self.current_picture_coding_type as _,
            current_tref: self.current_tref,
            min_pts: self.min_pts,
            max_pts: self.max_pts,
            sync_pts: self.sync_pts,
            minimum_fts: self.minimum_fts,
            fts_now: self.fts_now,
            fts_offset: self.fts_offset,
            fts_fc_offset: self.fts_fc_offset,
            fts_max: self.fts_max,
            fts_global: self.fts_global,
            sync_pts2fts_set: self.sync_pts2fts_set,
            sync_pts2fts_fts: self.sync_pts2fts_fts,
            sync_pts2fts_pts: self.sync_pts2fts_pts,
            pts_reset: self.pts_reset,
        }
    }
}

impl CType<ccx_decoder_dtvcc_report> for DecoderDtvccReport {
    unsafe fn to_ctype(&self) -> ccx_decoder_dtvcc_report {
        ccx_decoder_dtvcc_report {
            reset_count: self.reset_count,
            services: self.services,
        }
    }
}

impl CType<ccx_encoders_transcript_format> for EncodersTranscriptFormat {
    unsafe fn to_ctype(&self) -> ccx_encoders_transcript_format {
        ccx_encoders_transcript_format {
            showStartTime: self.show_start_time as _,
            showEndTime: self.show_end_time as _,
            showMode: self.show_mode as _,
            showCC: self.show_cc as _,
            relativeTimestamp: self.relative_timestamp as _,
            xds: self.xds as _,
            useColors: self.use_colors as _,
            isFinal: self.is_final as _,
        }
    }
}

impl CType<demuxer_cfg> for DemuxerConfig {
    unsafe fn to_ctype(&self) -> demuxer_cfg {
        demuxer_cfg {
            m2ts: self.m2ts as _,
            auto_stream: self.auto_stream.to_ctype() as _,
            codec: self.codec.to_ctype() as _,
            nocodec: self.nocodec.to_ctype() as _,
            ts_autoprogram: self.ts_autoprogram as _,
            ts_allprogram: self.ts_allprogram as _,
            ts_cappids: self.ts_cappids.to_ctype(),
            nb_ts_cappid: self.ts_cappids.len() as _,
            ts_forced_cappid: self.ts_forced_cappid as _,
            ts_forced_program: self.ts_forced_program.unwrap_or(-1) as _,
            ts_forced_program_selected: self.ts_forced_program.is_some() as _,
            ts_datastreamtype: self.ts_datastreamtype.to_ctype() as _,
            ts_forced_streamtype: self.ts_forced_streamtype.to_ctype() as _,
        }
    }
}

impl CType<u32> for SelectCodec {
    unsafe fn to_ctype(&self) -> u32 {
        match self {
            SelectCodec::Some(codec) => match codec {
                Codec::Teletext => ccx_code_type_CCX_CODEC_TELETEXT as _,
                Codec::Dvb => ccx_code_type_CCX_CODEC_DVB as _,
                Codec::IsdbCc => ccx_code_type_CCX_CODEC_ISDB_CC as _,
                Codec::AtscCc => ccx_code_type_CCX_CODEC_ATSC_CC as _,
                Codec::Any => ccx_code_type_CCX_CODEC_ANY as _,
            },
            SelectCodec::None => ccx_code_type_CCX_CODEC_NONE as _,
            SelectCodec::All => ccx_code_type_CCX_CODEC_ANY as _,
        }
    }
}

impl CType<i32> for StreamType {
    unsafe fn to_ctype(&self) -> i32 {
        *self as i32
    }
}

impl CType<u32> for StreamMode {
    unsafe fn to_ctype(&self) -> u32 {
        match self {
            StreamMode::ElementaryOrNotFound => {
                ccx_stream_mode_enum_CCX_SM_ELEMENTARY_OR_NOT_FOUND as _
            }
            StreamMode::Transport => ccx_stream_mode_enum_CCX_SM_TRANSPORT as _,
            StreamMode::Program => ccx_stream_mode_enum_CCX_SM_PROGRAM as _,
            StreamMode::Asf => ccx_stream_mode_enum_CCX_SM_ASF as _,
            StreamMode::McpoodlesRaw => ccx_stream_mode_enum_CCX_SM_MCPOODLESRAW as _,
            StreamMode::Rcwt => ccx_stream_mode_enum_CCX_SM_RCWT as _,
            StreamMode::Myth => ccx_stream_mode_enum_CCX_SM_MYTH as _,
            StreamMode::Mp4 => ccx_stream_mode_enum_CCX_SM_MP4 as _,
            #[cfg(feature = "wtv_debug")]
            StreamMode::HexDump => ccx_stream_mode_enum_CCX_SM_HEX_DUMP as _,
            StreamMode::Wtv => ccx_stream_mode_enum_CCX_SM_WTV as _,
            #[cfg(feature = "enable_ffmpeg")]
            StreamMode::Ffmpeg => ccx_stream_mode_enum_CCX_SM_FFMPEG as _,
            StreamMode::Gxf => ccx_stream_mode_enum_CCX_SM_GXF as _,
            StreamMode::Mkv => ccx_stream_mode_enum_CCX_SM_MKV as _,
            StreamMode::Mxf => ccx_stream_mode_enum_CCX_SM_MXF as _,
            StreamMode::Autodetect => ccx_stream_mode_enum_CCX_SM_AUTODETECT as _,
            _ => ccx_stream_mode_enum_CCX_SM_ELEMENTARY_OR_NOT_FOUND as _,
        }
    }
}

impl CType<[u32; 128]> for Vec<u32> {
    unsafe fn to_ctype(&self) -> [u32; 128] {
        let mut array = [0; 128];
        for (i, value) in self.iter().enumerate() {
            array[i] = *value;
        }
        array
    }
}

impl CType<encoder_cfg> for EncoderConfig {
    unsafe fn to_ctype(&self) -> encoder_cfg {
        encoder_cfg {
            extract: self.extract as _,
            dtvcc_extract: self.dtvcc_extract as _,
            gui_mode_reports: self.gui_mode_reports as _,
            output_filename: string_to_c_char(&self.output_filename),
            write_format: self.write_format.to_ctype(),
            keep_output_closed: self.keep_output_closed as _,
            force_flush: self.force_flush as _,
            append_mode: self.append_mode as _,
            ucla: self.ucla as _,
            encoding: self.encoding as _,
            date_format: self.date_format.to_ctype(),
            millis_separator: self.millis_separator() as _,
            autodash: self.autodash as _,
            trim_subs: self.trim_subs as _,
            sentence_cap: self.sentence_cap as _,
            splitbysentence: self.splitbysentence as _,
            #[cfg(feature = "with_libcurl")]
            curlposturl: string_to_c_char(&self.curlposturl.clone().unwrap()),
            filter_profanity: self.filter_profanity as _,
            with_semaphore: self.with_semaphore as _,
            start_credits_text: string_to_c_char(&self.start_credits_text),
            end_credits_text: string_to_c_char(&self.end_credits_text),
            startcreditsnotbefore: self.startcreditsnotbefore.to_ctype(),
            startcreditsnotafter: self.startcreditsnotafter.to_ctype(),
            startcreditsforatleast: self.startcreditsforatleast.to_ctype(),
            startcreditsforatmost: self.startcreditsforatmost.to_ctype(),
            endcreditsforatleast: self.endcreditsforatleast.to_ctype(),
            endcreditsforatmost: self.endcreditsforatmost.to_ctype(),
            transcript_settings: self.transcript_settings.to_ctype(),
            send_to_srv: self.send_to_srv as _,
            no_bom: self.no_bom as _,
            first_input_file: string_to_c_char(&self.first_input_file),
            multiple_files: self.multiple_files as _,
            no_font_color: self.no_font_color as _,
            no_type_setting: self.no_type_setting as _,
            cc_to_stdout: self.cc_to_stdout as _,
            line_terminator_lf: self.line_terminator_lf as _,
            subs_delay: self.subs_delay.millis(),
            program_number: self.program_number as _,
            in_format: self.in_format,
            nospupngocr: self.nospupngocr as _,
            force_dropframe: self.force_dropframe as _,
            render_font: string_to_c_char(self.render_font.to_str().unwrap_or_default()),
            render_font_italics: string_to_c_char(
                self.render_font_italics.to_str().unwrap_or_default(),
            ),
            services_enabled: self.services_enabled.map(|b| if b { 1 } else { 0 }),
            services_charsets: if let DtvccServiceCharset::Unique(vbox) =
                self.services_charsets.clone()
            {
                string_to_c_chars(vbox.to_vec())
            } else {
                null_pointer()
            },
            all_services_charset: if let DtvccServiceCharset::Same(string) =
                self.services_charsets.clone()
            {
                string_to_c_char(string.as_str())
            } else {
                null_pointer()
            },
            extract_only_708: self.extract_only_708 as _,
        }
    }
}

impl CType<word_list> for Vec<String> {
    unsafe fn to_ctype(&self) -> word_list {
        word_list {
            words: string_to_c_chars(self.clone()),
            len: self.len(),
            capacity: self.capacity(),
        }
    }
}
impl CType<ccx_code_type> for Codec {
    /// Convert to C variant of `ccx_code_type`.
    unsafe fn to_ctype(&self) -> ccx_code_type {
        match self {
            Codec::Any => ccx_code_type_CCX_CODEC_ANY,
            Codec::Teletext => ccx_code_type_CCX_CODEC_TELETEXT,
            Codec::Dvb => ccx_code_type_CCX_CODEC_DVB,
            Codec::IsdbCc => ccx_code_type_CCX_CODEC_ISDB_CC,
            Codec::AtscCc => ccx_code_type_CCX_CODEC_ATSC_CC,
        }
    }
}
impl CType<cap_info> for CapInfo {
    /// Convert to C variant of `cap_info`.
    unsafe fn to_ctype(&self) -> cap_info {
        cap_info {
            pid: self.pid,
            program_number: self.program_number,
            stream: self.stream.to_ctype() as ccx_stream_type, // CType<ccx_stream_type> for StreamType
            codec: self.codec.to_ctype(),                      // CType<ccx_code_type> for Codec
            capbufsize: self.capbufsize as c_long,
            capbuf: self.capbuf,
            capbuflen: self.capbuflen as c_long,
            saw_pesstart: self.saw_pesstart,
            prev_counter: self.prev_counter,
            codec_private_data: self.codec_private_data,
            ignore: self.ignore,
            all_stream: self.all_stream,
            sib_head: self.sib_head,
            sib_stream: self.sib_stream,
            pg_stream: self.pg_stream,
        }
    }
}
impl CType<ccx_demux_report> for CcxDemuxReport {
    /// Convert to C variant of `ccx_demux_report`.
    unsafe fn to_ctype(&self) -> ccx_demux_report {
        ccx_demux_report {
            program_cnt: self.program_cnt,
            dvb_sub_pid: self.dvb_sub_pid,
            tlt_sub_pid: self.tlt_sub_pid,
            mp4_cc_track_cnt: self.mp4_cc_track_cnt,
        }
    }
}
impl CType<program_info> for ProgramInfo {
    unsafe fn to_ctype(&self) -> program_info {
        // Set `analysed_pmt_once` in the first bitfield
        let mut bf1 = __BindgenBitfieldUnit::new([0u8; 1]);
        bf1.set(0, 1, self.analysed_pmt_once as u64); // 1-bit at offset 0

        // Set `valid_crc` in the second bitfield
        let mut bf2 = __BindgenBitfieldUnit::new([0u8; 1]);
        bf2.set(0, 1, self.valid_crc as u64); // 1-bit at offset 0

        // Convert `name` to C char array
        let mut name_c: [::std::os::raw::c_char; 128] = [0; 128];
        for (i, &byte) in self.name.iter().take(128).enumerate() {
            name_c[i] = byte as ::std::os::raw::c_char;
        }

        // Copy saved_section
        let mut saved_section_c = [0u8; 1021];
        saved_section_c.copy_from_slice(&self.saved_section);

        // Copy got_important_streams_min_pts (up to 3 entries only)
        let mut min_pts_c: [u64; 3] = [0; 3];
        for (i, &val) in self
            .got_important_streams_min_pts
            .iter()
            .take(3)
            .enumerate()
        {
            min_pts_c[i] = val;
        }

        program_info {
            pid: self.pid,
            program_number: self.program_number,
            initialized_ocr: self.initialized_ocr as c_int,
            _bitfield_align_1: [],
            _bitfield_1: bf1,
            version: self.version,
            saved_section: saved_section_c,
            crc: self.crc,
            _bitfield_align_2: [],
            _bitfield_2: bf2,
            name: name_c,
            pcr_pid: self.pcr_pid,
            got_important_streams_min_pts: min_pts_c,
            has_all_min_pts: self.has_all_min_pts as c_int,
        }
    }
}
impl CType<PSI_buffer> for PSIBuffer {
    /// Convert to C variant of `PSI_buffer`.
    unsafe fn to_ctype(&self) -> PSI_buffer {
        PSI_buffer {
            prev_ccounter: self.prev_ccounter,
            buffer: self.buffer,
            buffer_length: self.buffer_length,
            ccounter: self.ccounter,
        }
    }
}
impl CType<PMT_entry> for PMTEntry {
    /// Convert to C variant of `PMT_entry`.
    unsafe fn to_ctype(&self) -> PMT_entry {
        PMT_entry {
            program_number: self.program_number,
            elementary_PID: self.elementary_pid,
            stream_type: self.stream_type.to_ctype() as ccx_stream_type, // CType<ccx_stream_type> for StreamType
            printable_stream_type: self.printable_stream_type,
        }
    }
}
impl CType<ccx_bufferdata_type> for BufferdataType {
    unsafe fn to_ctype(&self) -> ccx_bufferdata_type {
        match self {
            BufferdataType::Unknown => ccx_bufferdata_type_CCX_UNKNOWN,
            BufferdataType::Pes => ccx_bufferdata_type_CCX_PES,
            BufferdataType::Raw => ccx_bufferdata_type_CCX_RAW,
            BufferdataType::H264 => ccx_bufferdata_type_CCX_H264,
            BufferdataType::Hauppage => ccx_bufferdata_type_CCX_HAUPPAGE,
            BufferdataType::Teletext => ccx_bufferdata_type_CCX_TELETEXT,
            BufferdataType::PrivateMpeg2Cc => ccx_bufferdata_type_CCX_PRIVATE_MPEG2_CC,
            BufferdataType::DvbSubtitle => ccx_bufferdata_type_CCX_DVB_SUBTITLE,
            BufferdataType::IsdbSubtitle => ccx_bufferdata_type_CCX_ISDB_SUBTITLE,
            BufferdataType::RawType => ccx_bufferdata_type_CCX_RAW_TYPE,
            BufferdataType::DvdSubtitle => ccx_bufferdata_type_CCX_DVD_SUBTITLE,
        }
    }
}

impl CType<ccx_rational> for CcxRational {
    unsafe fn to_ctype(&self) -> ccx_rational {
        ccx_rational {
            num: self.num,
            den: self.den,
        }
    }
}

impl CType<avc_ctx> for AvcContextRust {
    unsafe fn to_ctype(&self) -> avc_ctx {
        // Allocate cc_data buffer
        let cc_data_ptr = if !self.cc_data.is_empty() {
            let data_box = self.cc_data.clone().into_boxed_slice();
            Box::into_raw(data_box) as *mut u8
        } else {
            std::ptr::null_mut()
        };

        avc_ctx {
            cc_count: self.cc_count,
            cc_data: cc_data_ptr,
            cc_databufsize: self.cc_databufsize as _,
            cc_buffer_saved: if self.cc_buffer_saved { 1 } else { 0 },

            got_seq_para: if self.got_seq_para { 1 } else { 0 },
            nal_ref_idc: self.nal_ref_idc,
            seq_parameter_set_id: self.seq_parameter_set_id,
            log2_max_frame_num: self.log2_max_frame_num,
            pic_order_cnt_type: self.pic_order_cnt_type,
            log2_max_pic_order_cnt_lsb: self.log2_max_pic_order_cnt_lsb,
            frame_mbs_only_flag: if self.frame_mbs_only_flag { 1 } else { 0 },

            num_nal_unit_type_7: self.num_nal_unit_type_7 as _,
            num_vcl_hrd: self.num_vcl_hrd as _,
            num_nal_hrd: self.num_nal_hrd as _,
            num_jump_in_frames: self.num_jump_in_frames as _,
            num_unexpected_sei_length: self.num_unexpected_sei_length as _,

            ccblocks_in_avc_total: self.ccblocks_in_avc_total,
            ccblocks_in_avc_lost: self.ccblocks_in_avc_lost,

            frame_num: self.frame_num,
            lastframe_num: self.lastframe_num,
            currref: self.currref,
            maxidx: self.maxidx,
            lastmaxidx: self.lastmaxidx,

            minidx: self.minidx,
            lastminidx: self.lastminidx,

            maxtref: self.maxtref,
            last_gop_maxtref: self.last_gop_maxtref,

            currefpts: self.currefpts,
            last_pic_order_cnt_lsb: self.last_pic_order_cnt_lsb,
            last_slice_pts: self.last_slice_pts,
        }
    }
}
