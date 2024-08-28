use lib_ccxr::common::Codec;
use lib_ccxr::common::CommonTimingCtx;
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
use lib_ccxr::hardsubx::ColorHue;
use lib_ccxr::hardsubx::OcrMode;
use lib_ccxr::teletext::TeletextConfig;
use lib_ccxr::time::units::Timestamp;
use lib_ccxr::time::units::TimestampFormat;
use lib_ccxr::util::encoding::Encoding;

use crate::bindings::*;
use crate::utils::null_pointer;
use crate::utils::string_to_c_char;
use crate::utils::string_to_c_chars;

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

impl FromRust<Options> for ccx_s_options {
    /// # Safety
    ///
    /// This function is unsafe because it dereferences the pointer passed to it.
    unsafe fn copy_from_rust(self: &mut ccx_s_options, options: Options) {
        self.extract = options.extract as _;
        self.no_rollup = options.no_rollup as _;
        self.noscte20 = options.noscte20 as _;
        self.webvtt_create_css = options.webvtt_create_css as _;
        self.cc_channel = options.cc_channel as _;
        self.buffer_input = options.buffer_input as _;
        self.nofontcolor = options.nofontcolor as _;
        self.write_format = options.write_format.to_ctype();
        self.send_to_srv = options.send_to_srv as _;
        self.nohtmlescape = options.nohtmlescape as _;
        self.notypesetting = options.notypesetting as _;
        self.extraction_start = options.extraction_start.to_ctype();
        self.extraction_end = options.extraction_end.to_ctype();
        self.print_file_reports = options.print_file_reports as _;
        self.settings_608 = options.settings_608.to_ctype();
        self.settings_dtvcc = options.settings_dtvcc.to_ctype();
        self.is_608_enabled = options.is_608_enabled as _;
        self.is_708_enabled = options.is_708_enabled as _;
        self.millis_separator = options.millis_separator() as _;
        self.binary_concat = options.binary_concat as _;
        self.use_gop_as_pts = if let Some(usegops) = options.use_gop_as_pts {
            if usegops {
                1
            } else {
                -1
            }
        } else {
            0
        };
        self.fix_padding = options.fix_padding as _;
        self.gui_mode_reports = options.gui_mode_reports as _;
        self.no_progress_bar = options.no_progress_bar as _;

        if options.sentence_cap_file.try_exists().unwrap_or_default() {
            self.sentence_cap_file = string_to_c_char(
                options
                    .sentence_cap_file
                    .clone()
                    .to_str()
                    .unwrap_or_default(),
            );
        }

        self.live_stream = if let Some(live_stream) = options.live_stream {
            live_stream.seconds() as _
        } else {
            -1
        };
        if options
            .filter_profanity_file
            .try_exists()
            .unwrap_or_default()
        {
            self.filter_profanity_file = string_to_c_char(
                options
                    .filter_profanity_file
                    .clone()
                    .to_str()
                    .unwrap_or_default(),
            );
        }
        self.messages_target = options.messages_target as _;
        self.timestamp_map = options.timestamp_map as _;
        self.dolevdist = options.dolevdist.into();
        self.levdistmincnt = options.levdistmincnt as _;
        self.levdistmaxpct = options.levdistmaxpct as _;
        self.investigate_packets = options.investigate_packets as _;
        self.fullbin = options.fullbin as _;
        self.nosync = options.nosync as _;
        self.hauppauge_mode = options.hauppauge_mode as _;
        self.wtvconvertfix = options.wtvconvertfix as _;
        self.wtvmpeg2 = options.wtvmpeg2 as _;
        self.auto_myth = if let Some(auto_myth) = options.auto_myth {
            auto_myth as _
        } else {
            2
        };
        self.mp4vidtrack = options.mp4vidtrack as _;
        self.extract_chapters = options.extract_chapters as _;
        self.usepicorder = options.usepicorder as _;
        self.xmltv = options.xmltv as _;
        self.xmltvliveinterval = options.xmltvliveinterval.seconds() as _;
        self.xmltvoutputinterval = options.xmltvoutputinterval.seconds() as _;
        self.xmltvonlycurrent = options.xmltvonlycurrent.into();
        self.keep_output_closed = options.keep_output_closed as _;
        self.force_flush = options.force_flush as _;
        self.append_mode = options.append_mode as _;
        self.ucla = options.ucla as _;
        self.tickertext = options.tickertext as _;
        self.hardsubx = options.hardsubx as _;
        self.hardsubx_and_common = options.hardsubx_and_common as _;
        if let Some(dvblang) = options.dvblang {
            self.dvblang = string_to_c_char(dvblang.to_ctype().as_str());
        }
        if options.ocrlang.try_exists().unwrap_or_default() {
            self.ocrlang = string_to_c_char(options.ocrlang.to_str().unwrap());
        }
        self.ocr_oem = options.ocr_oem as _;
        self.ocr_quantmode = options.ocr_quantmode as _;
        if let Some(mkvlang) = options.mkvlang {
            self.mkvlang = string_to_c_char(mkvlang.to_ctype().as_str());
        }
        self.analyze_video_stream = options.analyze_video_stream as _;
        self.hardsubx_ocr_mode = options.hardsubx_ocr_mode.to_ctype();
        self.hardsubx_subcolor = options.hardsubx_hue.to_ctype();
        self.hardsubx_min_sub_duration = options.hardsubx_min_sub_duration.seconds() as _;
        self.hardsubx_detect_italics = options.hardsubx_detect_italics as _;
        self.hardsubx_conf_thresh = options.hardsubx_conf_thresh as _;
        self.hardsubx_hue = options.hardsubx_hue.get_hue() as _;
        self.hardsubx_lum_thresh = options.hardsubx_lum_thresh as _;
        self.transcript_settings = options.transcript_settings.to_ctype();
        self.date_format = options.date_format.to_ctype();
        self.write_format_rewritten = options.write_format_rewritten as _;
        self.use_ass_instead_of_ssa = options.use_ass_instead_of_ssa as _;
        self.use_webvtt_styling = options.use_webvtt_styling as _;
        self.debug_mask = options.debug_mask.normal_mask().bits() as _;
        self.debug_mask_on_debug = options.debug_mask.debug_mask().bits() as _;
        if options.udpsrc.is_some() {
            self.udpsrc = string_to_c_char(&options.udpsrc.clone().unwrap());
        }
        if options.udpaddr.is_some() {
            self.udpaddr = string_to_c_char(&options.udpaddr.clone().unwrap());
        }
        self.udpport = options.udpport as _;
        if options.tcpport.is_some() {
            self.tcpport = string_to_c_char(&options.tcpport.unwrap().to_string());
        }
        if options.tcp_password.is_some() {
            self.tcp_password = string_to_c_char(&options.tcp_password.clone().unwrap());
        }
        if options.tcp_desc.is_some() {
            self.tcp_desc = string_to_c_char(&options.tcp_desc.clone().unwrap());
        }
        if options.srv_addr.is_some() {
            self.srv_addr = string_to_c_char(&options.srv_addr.clone().unwrap());
        }
        if options.srv_port.is_some() {
            self.srv_port = string_to_c_char(&options.srv_port.unwrap().to_string());
        }
        self.noautotimeref = options.noautotimeref as _;
        self.input_source = options.input_source as _;
        if options.output_filename.is_some() {
            self.output_filename = string_to_c_char(&options.output_filename.clone().unwrap());
        }
        if options.inputfile.is_some() {
            self.inputfile = string_to_c_chars(options.inputfile.clone().unwrap());
            self.num_input_files = options.inputfile.iter().filter(|s| !s.is_empty()).count() as _;
        }
        self.demux_cfg = options.demux_cfg.to_ctype();
        self.enc_cfg = options.enc_cfg.to_ctype();
        self.subs_delay = options.subs_delay.millis();
        self.cc_to_stdout = options.cc_to_stdout as _;
        self.pes_header_to_stdout = options.pes_header_to_stdout as _;
        self.ignore_pts_jumps = options.ignore_pts_jumps as _;
        self.multiprogram = options.multiprogram as _;
        self.out_interval = options.out_interval;
        self.segment_on_key_frames_only = options.segment_on_key_frames_only as _;
        #[cfg(feature = "with_libcurl")]
        {
            if options.curlposturl.is_some() {
                self.curlposturl =
                    string_to_c_char(&options.curlposturl.as_ref().unwrap_or_default().as_str());
            }
        }
        #[cfg(feature = "enable_sharing")]
        {
            self.sharing_enabled = options.sharing_enabled as _;
            if options.sharing_url.is_some() {
                self.sharing_url =
                    string_to_c_char(&options.sharing_url.as_ref().unwrap().as_str());
            }
            self.translate_enabled = options.translate_enabled as _;
            if options.translate_langs.is_some() {
                self.translate_langs = string_to_c_char(&options.translate_langs.unwrap());
            }
            if options.translate_key.is_some() {
                self.translate_key = string_to_c_char(&options.translate_key.unwrap());
            }
        }
    }
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
            Encoding::Utf8 => ccx_encoding_type_CCX_ENC_UTF_8 as _,
            Encoding::Ucs2 => ccx_encoding_type_CCX_ENC_UNICODE as _,
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
