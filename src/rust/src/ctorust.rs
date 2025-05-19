use crate::bindings::{
    __BindgenBitfieldUnit, cap_info, ccx_boundary_time, ccx_code_type, ccx_common_timing_ctx,
    ccx_decoder_608_color_code, ccx_decoder_608_color_code_COL_MAX, ccx_decoder_608_report,
    ccx_decoder_608_settings, ccx_decoder_dtvcc_report, ccx_decoder_dtvcc_settings,
    ccx_demux_report, ccx_demuxer, ccx_encoders_transcript_format, ccx_encoding_type,
    ccx_frame_type_CCX_FRAME_TYPE_B_FRAME, ccx_frame_type_CCX_FRAME_TYPE_D_FRAME,
    ccx_frame_type_CCX_FRAME_TYPE_I_FRAME, ccx_frame_type_CCX_FRAME_TYPE_P_FRAME,
    ccx_output_date_format, ccx_output_format, ccx_stream_mode_enum, ccx_stream_type, demuxer_cfg,
    encoder_cfg, lib_ccx_ctx, list_head, program_info, PMT_entry, PSI_buffer, LLONG,
};
use crate::demuxer::demux::{
    CapInfo, CcxDemuxReport, CcxDemuxer, PMTEntry, PSIBuffer, ProgramInfo,
};
use lib_ccxr::common::{
    Codec, CommonTimingCtx, Decoder608ColorCode, Decoder608Report, Decoder608Settings,
    DecoderDtvccReport, DecoderDtvccSettings, DtvccServiceCharset, EncoderConfig, FrameType,
    SelectCodec, StreamMode, StreamType, DTVCC_MAX_SERVICES,
};
use lib_ccxr::time::Timestamp;
use lib_ccxr::util::encoding::Encoding;
use lib_ccxr::util::log::{DebugMessageFlag, DebugMessageMask, OutputTarget};
use std::convert::TryInto;
use std::ffi::CStr;
use std::os::raw::{c_int, c_uint};
use std::path::PathBuf;
use std::ptr::null_mut;
use std::slice;

pub fn from_ctype_Decoder608ColorCode(color: ccx_decoder_608_color_code) -> Decoder608ColorCode {
    match color {
        0 => Decoder608ColorCode::White,
        1 => Decoder608ColorCode::Green,
        2 => Decoder608ColorCode::Blue,
        3 => Decoder608ColorCode::Cyan,
        4 => Decoder608ColorCode::Red,
        5 => Decoder608ColorCode::Yellow,
        6 => Decoder608ColorCode::Magenta,
        7 => Decoder608ColorCode::Userdefined,
        8 => Decoder608ColorCode::Black,
        9 => Decoder608ColorCode::Transparent,
        _ => panic!("Invalid color code"),
    }
}

pub fn from_ctype_Decoder608Report(report: ccx_decoder_608_report) -> Decoder608Report {
    Decoder608Report {
        xds: report._bitfield_1.get_bit(0),
        cc_channels: report.cc_channels,
    }
}

pub fn from_ctype_Decoder608Settings(settings: ccx_decoder_608_settings) -> Decoder608Settings {
    Decoder608Settings {
        direct_rollup: settings.direct_rollup,
        force_rollup: settings.force_rollup,
        no_rollup: settings.no_rollup != 0,
        default_color: from_ctype_Decoder608ColorCode(settings.default_color),
        screens_to_process: settings.screens_to_process,
        report: if !settings.report.is_null() {
            // Safety: We've checked the pointer is not null
            unsafe { Some(from_ctype_Decoder608Report(*settings.report)) }
        } else {
            None
        },
    }
}
pub unsafe fn generate_common_timing_context(ctx: *const ccx_common_timing_ctx) -> CommonTimingCtx {
    let ctx = ctx.as_ref().unwrap();
    let pts_set = ctx.pts_set;
    let min_pts_adjusted = ctx.min_pts_adjusted;
    let current_pts = ctx.current_pts;
    let current_picture_coding_type = match ctx.current_picture_coding_type {
        ccx_frame_type_CCX_FRAME_TYPE_I_FRAME => FrameType::IFrame,
        ccx_frame_type_CCX_FRAME_TYPE_P_FRAME => FrameType::PFrame,
        ccx_frame_type_CCX_FRAME_TYPE_B_FRAME => FrameType::BFrame,
        ccx_frame_type_CCX_FRAME_TYPE_D_FRAME => FrameType::DFrame,
        _ => FrameType::ResetOrUnknown,
    };
    let current_tref = ctx.current_tref;
    let min_pts = ctx.min_pts;
    let max_pts = ctx.max_pts;
    let sync_pts = ctx.sync_pts;
    let minimum_fts = ctx.minimum_fts;
    let fts_now = ctx.fts_now;
    let fts_offset = ctx.fts_offset;
    let fts_fc_offset = ctx.fts_fc_offset;
    let fts_max = ctx.fts_max;
    let fts_global = ctx.fts_global;
    let sync_pts2fts_set = ctx.sync_pts2fts_set;
    let sync_pts2fts_fts = ctx.sync_pts2fts_fts;
    let sync_pts2fts_pts = ctx.sync_pts2fts_pts;
    let pts_reset = ctx.pts_reset;

    CommonTimingCtx {
        pts_set,
        min_pts_adjusted,
        current_pts,
        current_picture_coding_type,
        current_tref,
        min_pts,
        max_pts,
        sync_pts,
        minimum_fts,
        fts_now,
        fts_offset,
        fts_fc_offset,
        fts_max,
        fts_global,
        sync_pts2fts_set,
        sync_pts2fts_fts,
        sync_pts2fts_pts,
        pts_reset,
    }
}

pub fn from_ctype_DecoderDtvccSettings(
    settings: ccx_decoder_dtvcc_settings,
) -> DecoderDtvccSettings {
    // Convert the C-int array into a Rust bool array by hand.
    let mut services_enabled = [false; DTVCC_MAX_SERVICES];
    for (i, &flag) in settings
        .services_enabled
        .iter()
        .enumerate()
        .take(DTVCC_MAX_SERVICES)
    {
        services_enabled[i] = flag != 0;
    }

    DecoderDtvccSettings {
        enabled: settings.enabled != 0,
        print_file_reports: settings.print_file_reports != 0,
        no_rollup: settings.no_rollup != 0,
        report: if !settings.report.is_null() {
            unsafe { Some(from_ctype_DecoderDtvccReport(*settings.report)) }
        } else {
            None
        },
        active_services_count: settings.active_services_count,
        services_enabled,
        timing: unsafe { generate_common_timing_context(settings.timing) },
    }
}

pub fn from_ctype_DecoderDtvccReport(report: ccx_decoder_dtvcc_report) -> DecoderDtvccReport {
    DecoderDtvccReport {
        reset_count: report.reset_count as i32,
        services: report.services.map(|svc| svc as u32),
    }
}

pub fn from_ctype_OutputTarget(target: c_int) -> OutputTarget {
    match target {
        0 => OutputTarget::Quiet,
        1 => OutputTarget::Stdout,
        2 => OutputTarget::Stderr,
        _ => panic!("Invalid output target"),
    }
}

pub fn from_ctype_ocr_mode(mode: c_int) -> lib_ccxr::hardsubx::OcrMode {
    match mode {
        0 => lib_ccxr::hardsubx::OcrMode::Frame,
        1 => lib_ccxr::hardsubx::OcrMode::Word,
        2 => lib_ccxr::hardsubx::OcrMode::Letter,
        _ => panic!("Invalid OCR mode"),
    }
}

pub fn from_ctype_ColorHue(hue: c_int) -> lib_ccxr::hardsubx::ColorHue {
    match hue {
        0 => lib_ccxr::hardsubx::ColorHue::White,
        1 => lib_ccxr::hardsubx::ColorHue::Yellow,
        2 => lib_ccxr::hardsubx::ColorHue::Green,
        3 => lib_ccxr::hardsubx::ColorHue::Cyan,
        4 => lib_ccxr::hardsubx::ColorHue::Blue,
        5 => lib_ccxr::hardsubx::ColorHue::Magenta,
        6 => lib_ccxr::hardsubx::ColorHue::Red,
        _ => panic!("Invalid color hue"),
    }
}

pub fn from_ctype_EncodersTranscriptFormat(
    format: ccx_encoders_transcript_format,
) -> lib_ccxr::common::EncodersTranscriptFormat {
    lib_ccxr::common::EncodersTranscriptFormat {
        show_start_time: format.showStartTime != 0,
        show_end_time: format.showEndTime != 0,
        show_mode: format.showMode != 0,
        show_cc: format.showCC != 0,
        relative_timestamp: format.relativeTimestamp != 0,
        xds: format.xds != 0,
        use_colors: format.useColors != 0,
        is_final: format.isFinal != 0,
    }
}

pub fn from_ctype_Output_Date_Format(
    format: ccx_output_date_format,
) -> lib_ccxr::time::TimestampFormat {
    match format {
        ccx_output_date_format::ODF_NONE => lib_ccxr::time::TimestampFormat::None,
        ccx_output_date_format::ODF_HHMMSS => lib_ccxr::time::TimestampFormat::HHMMSS,
        ccx_output_date_format::ODF_SECONDS => lib_ccxr::time::TimestampFormat::Seconds {
            millis_separator: ',',
        },
        ccx_output_date_format::ODF_DATE => lib_ccxr::time::TimestampFormat::Date {
            millis_separator: ',',
        },
        ccx_output_date_format::ODF_HHMMSSMS => lib_ccxr::time::TimestampFormat::HHMMSSFFF,
        _ => panic!("Invalid output date format"),
    }
}

pub fn from_ctype_Output_Format(format: ccx_output_format) -> lib_ccxr::common::OutputFormat {
    match format {
        ccx_output_format::CCX_OF_RAW => lib_ccxr::common::OutputFormat::Raw,
        ccx_output_format::CCX_OF_SRT => lib_ccxr::common::OutputFormat::Srt,
        ccx_output_format::CCX_OF_SAMI => lib_ccxr::common::OutputFormat::Sami,
        ccx_output_format::CCX_OF_TRANSCRIPT => lib_ccxr::common::OutputFormat::Transcript,
        ccx_output_format::CCX_OF_RCWT => lib_ccxr::common::OutputFormat::Rcwt,
        ccx_output_format::CCX_OF_NULL => lib_ccxr::common::OutputFormat::Null,
        ccx_output_format::CCX_OF_SMPTETT => lib_ccxr::common::OutputFormat::SmpteTt,
        ccx_output_format::CCX_OF_SPUPNG => lib_ccxr::common::OutputFormat::SpuPng,
        ccx_output_format::CCX_OF_DVDRAW => lib_ccxr::common::OutputFormat::DvdRaw,
        ccx_output_format::CCX_OF_WEBVTT => lib_ccxr::common::OutputFormat::WebVtt,
        ccx_output_format::CCX_OF_SIMPLE_XML => lib_ccxr::common::OutputFormat::SimpleXml,
        ccx_output_format::CCX_OF_G608 => lib_ccxr::common::OutputFormat::G608,
        ccx_output_format::CCX_OF_CURL => lib_ccxr::common::OutputFormat::Curl,
        ccx_output_format::CCX_OF_SSA => lib_ccxr::common::OutputFormat::Ssa,
        ccx_output_format::CCX_OF_MCC => lib_ccxr::common::OutputFormat::Mcc,
        ccx_output_format::CCX_OF_SCC => lib_ccxr::common::OutputFormat::Scc,
        ccx_output_format::CCX_OF_CCD => lib_ccxr::common::OutputFormat::Ccd,
        _ => panic!("Invalid output format"),
    }
}

pub fn from_ctype_DemuxerConfig(cfg: demuxer_cfg) -> lib_ccxr::common::DemuxerConfig {
    lib_ccxr::common::DemuxerConfig {
        m2ts: cfg.m2ts != 0,
        auto_stream: unsafe { from_ctype_StreamMode(cfg.auto_stream) },
        codec: unsafe { from_ctype_SelectCodec(cfg.codec) },
        nocodec: unsafe { from_ctype_SelectCodec(cfg.nocodec) },
        ts_autoprogram: cfg.ts_autoprogram != 0,
        ts_allprogram: cfg.ts_allprogram != 0,
        ts_cappids: unsafe { c_array_to_vec(&cfg.ts_cappids) },
        ts_forced_cappid: cfg.ts_forced_cappid != 0,
        ts_forced_program: if cfg.ts_forced_program != -1 {
            Some(cfg.ts_forced_program)
        } else {
            None
        },
        ts_datastreamtype: unsafe { from_ctype_StreamType(cfg.ts_datastreamtype as c_uint) },
        ts_forced_streamtype: unsafe { from_ctype_StreamType(cfg.ts_forced_streamtype) },
    }
}
fn c_array_to_vec(c_array: &[::std::os::raw::c_uint; 128usize]) -> Vec<u32> {
    c_array.iter().map(|&val| val as u32).collect()
}
pub fn from_ctype_StreamMode(mode: ccx_stream_mode_enum) -> StreamMode {
    match mode {
        0 => StreamMode::ElementaryOrNotFound,
        1 => StreamMode::Transport,
        2 => StreamMode::Program,
        3 => StreamMode::Asf,
        4 => StreamMode::McpoodlesRaw,
        5 => StreamMode::Rcwt,
        6 => StreamMode::Myth,
        7 => StreamMode::Mp4,
        #[cfg(feature = "wtv_debug")]
        8 => StreamMode::HexDump,
        9 => StreamMode::Wtv,
        #[cfg(feature = "enable_ffmpeg")]
        10 => StreamMode::Ffmpeg,
        11 => StreamMode::Gxf,
        12 => StreamMode::Mkv,
        13 => StreamMode::Mxf,
        16 => StreamMode::Autodetect,
        _ => panic!("Invalid stream mode"),
    }
}

pub fn from_ctype_SelectCodec(codec: ccx_code_type) -> SelectCodec {
    match codec {
        0 => SelectCodec::Some(Codec::Any),
        1 => SelectCodec::Some(Codec::Teletext),
        2 => SelectCodec::Some(Codec::Dvb),
        3 => SelectCodec::Some(Codec::IsdbCc),
        4 => SelectCodec::Some(Codec::AtscCc),
        5 => SelectCodec::None,
        _ => panic!("Invalid codec type"),
    }
}

pub fn from_ctype_StreamType(stream_type: ::std::os::raw::c_uint) -> StreamType {
    match stream_type {
        0x00 => StreamType::Unknownstream,
        0x01 => StreamType::VideoMpeg1,
        0x02 => StreamType::VideoMpeg2,
        0x03 => StreamType::AudioMpeg1,
        0x04 => StreamType::AudioMpeg2,
        0x05 => StreamType::PrivateTableMpeg2,
        0x06 => StreamType::PrivateMpeg2,
        0x07 => StreamType::MhegPackets,
        0x08 => StreamType::Mpeg2AnnexADsmCc,
        0x09 => StreamType::ItuTH222_1,
        0x0a => StreamType::IsoIec13818_6TypeA,
        0x0b => StreamType::IsoIec13818_6TypeB,
        0x0c => StreamType::IsoIec13818_6TypeC,
        0x0d => StreamType::IsoIec13818_6TypeD,
        0x0f => StreamType::AudioAac,
        0x10 => StreamType::VideoMpeg4,
        0x1b => StreamType::VideoH264,
        0x80 => StreamType::PrivateUserMpeg2,
        0x81 => StreamType::AudioAc3,
        0x82 => StreamType::AudioHdmvDts,
        0x8a => StreamType::AudioDts,
        _ => panic!("Invalid stream type"),
    }
}
pub fn from_ctype_EncoderConfig(cfg: encoder_cfg) -> EncoderConfig {
    EncoderConfig {
        extract: cfg.extract as u8,
        dtvcc_extract: cfg.dtvcc_extract != 0,
        gui_mode_reports: cfg.gui_mode_reports != 0,
        output_filename: unsafe {
            if !cfg.output_filename.is_null() {
                CStr::from_ptr(cfg.output_filename)
                    .to_string_lossy()
                    .into_owned()
            } else {
                String::new()
            }
        },
        write_format: unsafe { from_ctype_Output_Format(cfg.write_format) },
        keep_output_closed: cfg.keep_output_closed != 0,
        force_flush: cfg.force_flush != 0,
        append_mode: cfg.append_mode != 0,
        ucla: cfg.ucla != 0,
        encoding: unsafe { from_ctype_Encoding(cfg.encoding) },
        date_format: unsafe { from_ctype_Output_Date_Format(cfg.date_format) },
        autodash: cfg.autodash != 0,
        trim_subs: cfg.trim_subs != 0,
        sentence_cap: cfg.sentence_cap != 0,
        splitbysentence: cfg.splitbysentence != 0,

        curlposturl: None, // TODO: Handle this

        filter_profanity: cfg.filter_profanity != 0,
        with_semaphore: cfg.with_semaphore != 0,

        start_credits_text: unsafe {
            if !cfg.start_credits_text.is_null() {
                CStr::from_ptr(cfg.start_credits_text)
                    .to_string_lossy()
                    .into_owned()
            } else {
                String::new()
            }
        },
        end_credits_text: unsafe {
            if !cfg.end_credits_text.is_null() {
                CStr::from_ptr(cfg.end_credits_text)
                    .to_string_lossy()
                    .into_owned()
            } else {
                String::new()
            }
        },

        startcreditsnotbefore: unsafe { from_ctype_Timestamp(cfg.startcreditsnotbefore) },
        startcreditsnotafter: unsafe { from_ctype_Timestamp(cfg.startcreditsnotafter) },
        startcreditsforatleast: unsafe { from_ctype_Timestamp(cfg.startcreditsforatleast) },
        startcreditsforatmost: unsafe { from_ctype_Timestamp(cfg.startcreditsforatmost) },
        endcreditsforatleast: unsafe { from_ctype_Timestamp(cfg.endcreditsforatleast) },
        endcreditsforatmost: unsafe { from_ctype_Timestamp(cfg.endcreditsforatmost) },

        transcript_settings: unsafe {
            from_ctype_EncodersTranscriptFormat(cfg.transcript_settings)
        },

        send_to_srv: cfg.send_to_srv != 0,
        no_bom: cfg.no_bom != 0,

        first_input_file: unsafe {
            if !cfg.first_input_file.is_null() {
                CStr::from_ptr(cfg.first_input_file)
                    .to_string_lossy()
                    .into_owned()
            } else {
                String::new()
            }
        },

        multiple_files: cfg.multiple_files != 0,
        no_font_color: cfg.no_font_color != 0,
        no_type_setting: cfg.no_type_setting != 0,
        cc_to_stdout: cfg.cc_to_stdout != 0,
        line_terminator_lf: cfg.line_terminator_lf != 0,
        subs_delay: Timestamp::from_millis(cfg.subs_delay),
        program_number: cfg.program_number as u32,
        in_format: cfg.in_format,
        nospupngocr: cfg.nospupngocr != 0,
        force_dropframe: cfg.force_dropframe != 0,

        render_font: unsafe {
            if !cfg.render_font.is_null() {
                PathBuf::from(
                    CStr::from_ptr(cfg.render_font)
                        .to_string_lossy()
                        .to_string(),
                )
            } else {
                PathBuf::new()
            }
        },
        render_font_italics: unsafe {
            if !cfg.render_font_italics.is_null() {
                PathBuf::from(
                    CStr::from_ptr(cfg.render_font_italics)
                        .to_string_lossy()
                        .to_string(),
                )
            } else {
                PathBuf::new()
            }
        },

        services_enabled: {
            let mut services = [false; DTVCC_MAX_SERVICES];
            for (i, &val) in cfg
                .services_enabled
                .iter()
                .take(DTVCC_MAX_SERVICES)
                .enumerate()
            {
                services[i] = val != 0;
            }
            services
        },

        services_charsets: from_ctype_DtvccServiceCharset(
            unsafe { cfg.services_charsets },
            unsafe { cfg.all_services_charset },
        ),
        extract_only_708: cfg.extract_only_708 != 0,
    }
}

pub fn from_ctype_Encoding(encoding: ccx_encoding_type) -> Encoding {
    match encoding {
        0 => Encoding::Line21,
        1 => Encoding::Latin1,
        3 => Encoding::Ucs2,
        2 => Encoding::Utf8,
        _ => panic!("Invalid encoding type"),
    }
}

pub fn from_ctype_DtvccServiceCharset(
    services_charsets: *mut *mut ::std::os::raw::c_char,
    all_services_charset: *mut ::std::os::raw::c_char,
) -> DtvccServiceCharset {
    if unsafe { *all_services_charset } < ccx_decoder_608_color_code_COL_MAX as i8 {
        // Convert `all_services_charset` to `DtvccServiceCharset::Same`
        let charset = format!("Charset_{}", unsafe { *all_services_charset });
        DtvccServiceCharset::Same(charset)
    } else {
        // Convert `services_charsets` to `DtvccServiceCharset::Unique`
        let charsets_slice =
            unsafe { std::slice::from_raw_parts(services_charsets, DTVCC_MAX_SERVICES) };
        let mut charsets = Vec::new();
        for &code in charsets_slice {
            if unsafe { *code } < ccx_decoder_608_color_code_COL_MAX as i8 {
                charsets.push(format!("Charset_{:?}", code));
            } else {
                charsets.push("Invalid".to_string());
            }
        }
        if let Ok(array) = charsets.try_into() {
            DtvccServiceCharset::Unique(Box::new(array))
        } else {
            DtvccServiceCharset::None
        }
    }
}
pub fn from_ctype_Timestamp(ts: ccx_boundary_time) -> Timestamp {
    Timestamp::from_millis(ts.time_in_ms)
}
pub fn from_ctype_DebugMessageMask(mask_on_normal: u32, mask_on_debug: u32) -> DebugMessageMask {
    DebugMessageMask::new(
        DebugMessageFlag::from_bits_truncate(mask_on_normal as u16),
        DebugMessageFlag::from_bits_truncate(mask_on_debug as u16),
    )
}
pub fn from_ctype_Codec(codec: ccx_code_type) -> Codec {
    match codec {
        0 => Codec::Any,
        1 => Codec::Teletext,
        2 => Codec::Dvb,
        3 => Codec::IsdbCc,
        4 => Codec::AtscCc,
        _ => panic!("No codec type"),
    }
}
pub fn from_ctype_program_info(info: program_info) -> ProgramInfo {
    ProgramInfo {
        pid: info.pid,
        program_number: info.program_number,
        initialized_ocr: info.initialized_ocr,
        analysed_pmt_once: info._bitfield_1.get_bit(0) as u8,
        version: info.version,
        saved_section: info.saved_section,
        crc: info.crc,
        valid_crc: info._bitfield_2.get_bit(0) as u8,
        name: {
            let mut name_bytes = [0u8; 128];
            for (i, &c) in info.name.iter().enumerate() {
                name_bytes[i] = c as u8;
            }
            name_bytes
        },
        pcr_pid: info.pcr_pid,
        got_important_streams_min_pts: info.got_important_streams_min_pts,
        has_all_min_pts: info.has_all_min_pts,
    }
}
pub fn from_ctype_cap_info(info: cap_info) -> CapInfo {
    CapInfo {
        pid: info.pid,
        program_number: info.program_number,
        stream: from_ctype_StreamType(info.stream),
        codec: from_ctype_Codec(info.codec),
        capbufsize: info.capbufsize,
        capbuf: info.capbuf,
        capbuflen: info.capbuflen,
        saw_pesstart: info.saw_pesstart,
        prev_counter: info.prev_counter,
        codec_private_data: info.codec_private_data,
        ignore: info.ignore,
        all_stream: list_head {
            next: info.all_stream.next,
            prev: info.all_stream.prev,
        },
        sib_head: list_head {
            next: info.sib_head.next,
            prev: info.sib_head.prev,
        },
        sib_stream: list_head {
            next: info.sib_stream.next,
            prev: info.sib_stream.prev,
        },
        pg_stream: list_head {
            next: info.pg_stream.next,
            prev: info.pg_stream.prev,
        },
    }
}
pub fn from_ctype_PSI_buffer(buffer: PSI_buffer) -> PSIBuffer {
    PSIBuffer {
        prev_ccounter: buffer.prev_ccounter,
        buffer: buffer.buffer,
        buffer_length: buffer.buffer_length,
        ccounter: buffer.ccounter,
    }
}
pub fn from_ctype_demux_report(report: ccx_demux_report) -> CcxDemuxReport {
    CcxDemuxReport {
        program_cnt: report.program_cnt,
        dvb_sub_pid: report.dvb_sub_pid,
        tlt_sub_pid: report.tlt_sub_pid,
        mp4_cc_track_cnt: report.mp4_cc_track_cnt,
    }
}

pub fn from_ctype_PMT_entry(entry: PMT_entry) -> PMTEntry {
    PMTEntry {
        program_number: entry.program_number,
        elementary_pid: entry.elementary_PID,
        stream_type: from_ctype_StreamType(entry.stream_type),
        printable_stream_type: entry.printable_stream_type,
    }
}
