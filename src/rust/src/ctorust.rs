#![allow(clippy::unnecessary_cast)] // we have to do this as windows has different types for some C types
use crate::avc::common_types::AvcContextRust;
use crate::bindings::*;
use crate::demuxer::common_types::{
    CapInfo, CcxDemuxReport, CcxRational, PMTEntry, PSIBuffer, ProgramInfo,
};
use lib_ccxr::common::{
    AvcNalType, BufferdataType, Codec, CommonTimingCtx, Decoder608ColorCode, Decoder608Report,
    Decoder608Settings, DecoderDtvccReport, DecoderDtvccSettings, DtvccServiceCharset,
    EncoderConfig, FrameType, SelectCodec, StreamMode, StreamType, DTVCC_MAX_SERVICES,
};
use lib_ccxr::time::Timestamp;
use lib_ccxr::util::encoding::Encoding;
use lib_ccxr::util::log::{DebugMessageFlag, DebugMessageMask, OutputTarget};
use std::convert::TryInto;
use std::ffi::CStr;
use std::os::raw::{c_int, c_uint};
use std::path::PathBuf;

pub trait FromCType<T> {
    /// # Safety
    /// This function is unsafe because it uses raw pointers to get data from C types.
    unsafe fn from_ctype(c_value: T) -> Option<Self>
    where
        Self: Sized;
}

pub struct DtvccServiceCharsetArgs {
    pub services_charsets: *mut *mut ::std::os::raw::c_char,
    pub all_services_charset: *mut ::std::os::raw::c_char,
}

impl FromCType<ccx_decoder_608_color_code> for Decoder608ColorCode {
    unsafe fn from_ctype(color: ccx_decoder_608_color_code) -> Option<Self> {
        Some(match color {
            1 => Decoder608ColorCode::Green,
            2 => Decoder608ColorCode::Blue,
            3 => Decoder608ColorCode::Cyan,
            4 => Decoder608ColorCode::Red,
            5 => Decoder608ColorCode::Yellow,
            6 => Decoder608ColorCode::Magenta,
            7 => Decoder608ColorCode::Userdefined,
            8 => Decoder608ColorCode::Black,
            9 => Decoder608ColorCode::Transparent,
            _ => Decoder608ColorCode::White,
        })
    }
}

impl FromCType<ccx_decoder_608_report> for Decoder608Report {
    unsafe fn from_ctype(report: ccx_decoder_608_report) -> Option<Self> {
        Some(Decoder608Report {
            xds: report._bitfield_1.get_bit(0),
            cc_channels: report.cc_channels,
        })
    }
}

impl FromCType<ccx_decoder_608_settings> for Decoder608Settings {
    unsafe fn from_ctype(settings: ccx_decoder_608_settings) -> Option<Self> {
        Some(Decoder608Settings {
            direct_rollup: settings.direct_rollup,
            force_rollup: settings.force_rollup,
            no_rollup: settings.no_rollup != 0,
            default_color: Decoder608ColorCode::from_ctype(settings.default_color)?,
            screens_to_process: settings.screens_to_process,
            report: if !settings.report.is_null() {
                Some(Decoder608Report::from_ctype(*settings.report)?)
            } else {
                None
            },
        })
    }
}

impl FromCType<*const ccx_common_timing_ctx> for CommonTimingCtx {
    unsafe fn from_ctype(ctx: *const ccx_common_timing_ctx) -> Option<Self> {
        let ctx = ctx.as_ref()?;
        let current_picture_coding_type = match ctx.current_picture_coding_type {
            ccx_frame_type_CCX_FRAME_TYPE_I_FRAME => FrameType::IFrame,
            ccx_frame_type_CCX_FRAME_TYPE_P_FRAME => FrameType::PFrame,
            ccx_frame_type_CCX_FRAME_TYPE_B_FRAME => FrameType::BFrame,
            ccx_frame_type_CCX_FRAME_TYPE_D_FRAME => FrameType::DFrame,
            _ => FrameType::ResetOrUnknown,
        };

        Some(CommonTimingCtx {
            pts_set: ctx.pts_set,
            min_pts_adjusted: ctx.min_pts_adjusted,
            current_pts: ctx.current_pts,
            current_picture_coding_type,
            current_tref: ctx.current_tref,
            min_pts: ctx.min_pts,
            max_pts: ctx.max_pts,
            sync_pts: ctx.sync_pts,
            minimum_fts: ctx.minimum_fts,
            fts_now: ctx.fts_now,
            fts_offset: ctx.fts_offset,
            fts_fc_offset: ctx.fts_fc_offset,
            fts_max: ctx.fts_max,
            fts_global: ctx.fts_global,
            sync_pts2fts_set: ctx.sync_pts2fts_set,
            sync_pts2fts_fts: ctx.sync_pts2fts_fts,
            sync_pts2fts_pts: ctx.sync_pts2fts_pts,
            pts_reset: ctx.pts_reset,
        })
    }
}

impl FromCType<ccx_decoder_dtvcc_settings> for DecoderDtvccSettings {
    unsafe fn from_ctype(settings: ccx_decoder_dtvcc_settings) -> Option<Self> {
        let mut services_enabled = [false; DTVCC_MAX_SERVICES];
        for (i, &flag) in settings
            .services_enabled
            .iter()
            .enumerate()
            .take(DTVCC_MAX_SERVICES)
        {
            services_enabled[i] = flag != 0;
        }

        Some(DecoderDtvccSettings {
            enabled: settings.enabled != 0,
            print_file_reports: settings.print_file_reports != 0,
            no_rollup: settings.no_rollup != 0,
            report: if !settings.report.is_null() {
                Some(DecoderDtvccReport::from_ctype(settings.report)?)
            } else {
                None
            },
            active_services_count: settings.active_services_count,
            services_enabled,
            timing: CommonTimingCtx::from_ctype(settings.timing)?,
        })
    }
}

impl FromCType<*const ccx_decoder_dtvcc_report> for DecoderDtvccReport {
    unsafe fn from_ctype(report_ptr: *const ccx_decoder_dtvcc_report) -> Option<Self> {
        if report_ptr.is_null() {
            return None;
        }

        let report = &*report_ptr; // Get a reference instead of copying
        Some(DecoderDtvccReport {
            reset_count: report.reset_count,
            services: report.services,
        })
    }
}

impl FromCType<c_int> for OutputTarget {
    unsafe fn from_ctype(target: c_int) -> Option<Self> {
        Some(match target {
            1 => OutputTarget::Stdout,
            2 => OutputTarget::Stderr,
            _ => OutputTarget::Quiet,
        })
    }
}

impl FromCType<c_int> for lib_ccxr::hardsubx::OcrMode {
    unsafe fn from_ctype(mode: c_int) -> Option<Self> {
        Some(match mode {
            1 => lib_ccxr::hardsubx::OcrMode::Word,
            2 => lib_ccxr::hardsubx::OcrMode::Letter,
            _ => lib_ccxr::hardsubx::OcrMode::Frame,
        })
    }
}

impl FromCType<c_int> for lib_ccxr::hardsubx::ColorHue {
    unsafe fn from_ctype(hue: c_int) -> Option<Self> {
        Some(match hue {
            1 => lib_ccxr::hardsubx::ColorHue::Yellow,
            2 => lib_ccxr::hardsubx::ColorHue::Green,
            3 => lib_ccxr::hardsubx::ColorHue::Cyan,
            4 => lib_ccxr::hardsubx::ColorHue::Blue,
            5 => lib_ccxr::hardsubx::ColorHue::Magenta,
            6 => lib_ccxr::hardsubx::ColorHue::Red,
            _ => lib_ccxr::hardsubx::ColorHue::White,
        })
    }
}

impl FromCType<ccx_encoders_transcript_format> for lib_ccxr::common::EncodersTranscriptFormat {
    unsafe fn from_ctype(format: ccx_encoders_transcript_format) -> Option<Self> {
        Some(lib_ccxr::common::EncodersTranscriptFormat {
            show_start_time: format.showStartTime != 0,
            show_end_time: format.showEndTime != 0,
            show_mode: format.showMode != 0,
            show_cc: format.showCC != 0,
            relative_timestamp: format.relativeTimestamp != 0,
            xds: format.xds != 0,
            use_colors: format.useColors != 0,
            is_final: format.isFinal != 0,
        })
    }
}

impl FromCType<ccx_output_date_format> for lib_ccxr::time::TimestampFormat {
    unsafe fn from_ctype(format: ccx_output_date_format) -> Option<Self> {
        Some(match format {
            ccx_output_date_format::ODF_NONE => lib_ccxr::time::TimestampFormat::None,
            ccx_output_date_format::ODF_HHMMSS => lib_ccxr::time::TimestampFormat::HHMMSS,
            ccx_output_date_format::ODF_SECONDS => lib_ccxr::time::TimestampFormat::Seconds {
                millis_separator: ',',
            },
            ccx_output_date_format::ODF_DATE => lib_ccxr::time::TimestampFormat::Date {
                millis_separator: ',',
            },
            ccx_output_date_format::ODF_HHMMSSMS => lib_ccxr::time::TimestampFormat::HHMMSSFFF,
        })
    }
}

impl FromCType<ccx_output_format> for lib_ccxr::common::OutputFormat {
    unsafe fn from_ctype(format: ccx_output_format) -> Option<Self> {
        Some(match format {
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
        })
    }
}

impl FromCType<demuxer_cfg> for lib_ccxr::common::DemuxerConfig {
    unsafe fn from_ctype(cfg: demuxer_cfg) -> Option<Self> {
        Some(lib_ccxr::common::DemuxerConfig {
            m2ts: cfg.m2ts != 0,
            auto_stream: StreamMode::from_ctype(cfg.auto_stream)?,
            codec: SelectCodec::from_ctype(cfg.codec)?,
            nocodec: SelectCodec::from_ctype(cfg.nocodec)?,
            ts_autoprogram: cfg.ts_autoprogram != 0,
            ts_allprogram: cfg.ts_allprogram != 0,
            ts_cappids: cfg.ts_cappids.to_vec(),
            ts_forced_cappid: cfg.ts_forced_cappid != 0,
            ts_forced_program: if cfg.ts_forced_program != -1 {
                Some(cfg.ts_forced_program)
            } else {
                None
            },
            ts_datastreamtype: StreamType::from_ctype(cfg.ts_datastreamtype as c_uint)?,
            ts_forced_streamtype: StreamType::from_ctype(cfg.ts_forced_streamtype)?,
        })
    }
}

impl FromCType<ccx_stream_mode_enum> for StreamMode {
    unsafe fn from_ctype(mode: ccx_stream_mode_enum) -> Option<Self> {
        Some(match mode {
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
            _ => StreamMode::ElementaryOrNotFound,
        })
    }
}

impl FromCType<ccx_code_type> for SelectCodec {
    unsafe fn from_ctype(codec: ccx_code_type) -> Option<Self> {
        Some(match codec {
            1 => SelectCodec::Some(Codec::Teletext),
            2 => SelectCodec::Some(Codec::Dvb),
            3 => SelectCodec::Some(Codec::IsdbCc),
            4 => SelectCodec::Some(Codec::AtscCc),
            5 => SelectCodec::None,
            _ => SelectCodec::Some(Codec::Any),
        })
    }
}

impl FromCType<c_uint> for StreamType {
    unsafe fn from_ctype(stream_type: c_uint) -> Option<Self> {
        Some(match stream_type {
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
            _ => StreamType::Unknownstream,
        })
    }
}

impl FromCType<encoder_cfg> for EncoderConfig {
    unsafe fn from_ctype(cfg: encoder_cfg) -> Option<Self> {
        let output_filename = if !cfg.output_filename.is_null() {
            CStr::from_ptr(cfg.output_filename)
                .to_string_lossy()
                .into_owned()
        } else {
            String::new()
        };

        let start_credits_text = if !cfg.start_credits_text.is_null() {
            CStr::from_ptr(cfg.start_credits_text)
                .to_string_lossy()
                .into_owned()
        } else {
            String::new()
        };

        let end_credits_text = if !cfg.end_credits_text.is_null() {
            CStr::from_ptr(cfg.end_credits_text)
                .to_string_lossy()
                .into_owned()
        } else {
            String::new()
        };

        let first_input_file = if !cfg.first_input_file.is_null() {
            CStr::from_ptr(cfg.first_input_file)
                .to_string_lossy()
                .into_owned()
        } else {
            String::new()
        };

        let render_font = if !cfg.render_font.is_null() {
            PathBuf::from(
                CStr::from_ptr(cfg.render_font)
                    .to_string_lossy()
                    .to_string(),
            )
        } else {
            PathBuf::new()
        };

        let render_font_italics = if !cfg.render_font_italics.is_null() {
            PathBuf::from(
                CStr::from_ptr(cfg.render_font_italics)
                    .to_string_lossy()
                    .to_string(),
            )
        } else {
            PathBuf::new()
        };

        let mut services_enabled = [false; DTVCC_MAX_SERVICES];
        for (i, &val) in cfg
            .services_enabled
            .iter()
            .enumerate()
            .take(DTVCC_MAX_SERVICES)
        {
            services_enabled[i] = val != 0;
        }

        let services_charsets_args = DtvccServiceCharsetArgs {
            services_charsets: cfg.services_charsets,
            all_services_charset: cfg.all_services_charset,
        };

        Some(EncoderConfig {
            extract: cfg.extract as u8,
            dtvcc_extract: cfg.dtvcc_extract != 0,
            gui_mode_reports: cfg.gui_mode_reports != 0,
            output_filename,
            write_format: lib_ccxr::common::OutputFormat::from_ctype(cfg.write_format)?,
            keep_output_closed: cfg.keep_output_closed != 0,
            force_flush: cfg.force_flush != 0,
            append_mode: cfg.append_mode != 0,
            ucla: cfg.ucla != 0,
            encoding: Encoding::from_ctype(cfg.encoding)?,
            date_format: lib_ccxr::time::TimestampFormat::from_ctype(cfg.date_format)?,
            autodash: cfg.autodash != 0,
            trim_subs: cfg.trim_subs != 0,
            sentence_cap: cfg.sentence_cap != 0,
            splitbysentence: cfg.splitbysentence != 0,
            curlposturl: None,
            filter_profanity: cfg.filter_profanity != 0,
            with_semaphore: cfg.with_semaphore != 0,
            start_credits_text,
            end_credits_text,
            startcreditsnotbefore: Timestamp::from_ctype(cfg.startcreditsnotbefore)?,
            startcreditsnotafter: Timestamp::from_ctype(cfg.startcreditsnotafter)?,
            startcreditsforatleast: Timestamp::from_ctype(cfg.startcreditsforatleast)?,
            startcreditsforatmost: Timestamp::from_ctype(cfg.startcreditsforatmost)?,
            endcreditsforatleast: Timestamp::from_ctype(cfg.endcreditsforatleast)?,
            endcreditsforatmost: Timestamp::from_ctype(cfg.endcreditsforatmost)?,
            transcript_settings: lib_ccxr::common::EncodersTranscriptFormat::from_ctype(
                cfg.transcript_settings,
            )?,
            send_to_srv: cfg.send_to_srv != 0,
            no_bom: cfg.no_bom != 0,
            first_input_file,
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
            render_font,
            render_font_italics,
            services_enabled,
            services_charsets: DtvccServiceCharset::from_ctype(services_charsets_args)?,
            extract_only_708: cfg.extract_only_708 != 0,
        })
    }
}

impl FromCType<ccx_encoding_type> for Encoding {
    unsafe fn from_ctype(encoding: ccx_encoding_type) -> Option<Self> {
        Some(match encoding {
            0 => Encoding::UCS2,   // CCX_ENC_UNICODE
            1 => Encoding::Latin1, // CCX_ENC_LATIN_1
            2 => Encoding::UTF8,   // CCX_ENC_UTF_8
            3 => Encoding::Line21, // CCX_ENC_ASCII
            _ => Encoding::UTF8,   // Default to UTF-8 if unknown
        })
    }
}

impl FromCType<DtvccServiceCharsetArgs> for DtvccServiceCharset {
    unsafe fn from_ctype(args: DtvccServiceCharsetArgs) -> Option<Self> {
        // First check if all_services_charset is not null (Same variant)
        if !args.all_services_charset.is_null() {
            let charset_str = CStr::from_ptr(args.all_services_charset)
                .to_string_lossy()
                .into_owned();
            return Some(DtvccServiceCharset::Same(charset_str));
        }

        if !args.services_charsets.is_null() {
            let mut charsets = Vec::with_capacity(DTVCC_MAX_SERVICES);

            // Read each string pointer from the array
            for i in 0..DTVCC_MAX_SERVICES {
                let str_ptr = *args.services_charsets.add(i);

                let charset_str = if str_ptr.is_null() {
                    // If individual string pointer is null, use empty string as fallback
                    String::new()
                } else {
                    CStr::from_ptr(str_ptr).to_string_lossy().into_owned()
                };

                charsets.push(charset_str);
            }

            let boxed_array = match charsets.try_into() {
                Ok(array) => Box::new(array),
                Err(_) => {
                    // This should never happen since we allocated exactly DTVCC_MAX_SERVICES items
                    // But as a fallback, create array filled with empty strings
                    Box::new([const { String::new() }; DTVCC_MAX_SERVICES])
                }
            };

            return Some(DtvccServiceCharset::Unique(boxed_array));
        }

        // Both pointers are null, return None variant
        Some(DtvccServiceCharset::None)
    }
}

impl FromCType<ccx_boundary_time> for Timestamp {
    unsafe fn from_ctype(ts: ccx_boundary_time) -> Option<Self> {
        Some(Timestamp::from_millis(ts.time_in_ms))
    }
}

impl FromCType<ccx_code_type> for Codec {
    unsafe fn from_ctype(codec: ccx_code_type) -> Option<Self> {
        Some(match codec {
            1 => Codec::Teletext,
            2 => Codec::Dvb,
            3 => Codec::IsdbCc,
            4 => Codec::AtscCc,
            _ => Codec::Any,
        })
    }
}

impl FromCType<program_info> for ProgramInfo {
    unsafe fn from_ctype(info: program_info) -> Option<Self> {
        let mut name_bytes = [0u8; 128];
        for (i, &c) in info.name.iter().enumerate() {
            name_bytes[i] = c as u8;
        }

        Some(ProgramInfo {
            pid: info.pid,
            program_number: info.program_number,
            initialized_ocr: info.initialized_ocr != 0,
            analysed_pmt_once: info._bitfield_1.get_bit(0) as u8,
            version: info.version,
            saved_section: info.saved_section,
            crc: info.crc,
            valid_crc: info._bitfield_2.get_bit(0) as u8,
            name: name_bytes,
            pcr_pid: info.pcr_pid,
            got_important_streams_min_pts: info.got_important_streams_min_pts,
            has_all_min_pts: info.has_all_min_pts != 0,
        })
    }
}

impl FromCType<cap_info> for CapInfo {
    unsafe fn from_ctype(info: cap_info) -> Option<Self> {
        Some(CapInfo {
            pid: info.pid,
            program_number: info.program_number,
            stream: StreamType::from_ctype(info.stream as u32)?,
            codec: Codec::from_ctype(info.codec)?,
            capbufsize: info.capbufsize as i64,
            capbuf: info.capbuf,
            capbuflen: info.capbuflen as i64,
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
        })
    }
}

impl FromCType<*mut PSI_buffer> for *mut PSIBuffer {
    unsafe fn from_ctype(buffer_ptr: *mut PSI_buffer) -> Option<Self> {
        if buffer_ptr.is_null() {
            return None;
        }

        let buffer = &*buffer_ptr;
        let psi_buffer = PSIBuffer {
            prev_ccounter: buffer.prev_ccounter,
            buffer: buffer.buffer,
            buffer_length: buffer.buffer_length,
            ccounter: buffer.ccounter,
        };

        Some(Box::into_raw(Box::new(psi_buffer)))
    }
}
impl FromCType<ccx_demux_report> for CcxDemuxReport {
    unsafe fn from_ctype(report: ccx_demux_report) -> Option<Self> {
        Some(CcxDemuxReport {
            program_cnt: report.program_cnt,
            dvb_sub_pid: report.dvb_sub_pid,
            tlt_sub_pid: report.tlt_sub_pid,
            mp4_cc_track_cnt: report.mp4_cc_track_cnt,
        })
    }
}

/// # Safety
/// This function is unsafe because it takes a raw pointer to a C struct.
impl FromCType<*mut PMT_entry> for *mut PMTEntry {
    unsafe fn from_ctype(buffer_ptr: *mut PMT_entry) -> Option<Self> {
        if buffer_ptr.is_null() {
            return None;
        }

        let buffer = unsafe { &*buffer_ptr };

        let program_number = if buffer.program_number != 0 {
            buffer.program_number
        } else {
            0
        };

        let elementary_pid = if buffer.elementary_PID != 0 {
            buffer.elementary_PID
        } else {
            0
        };

        let stream_type = if buffer.stream_type != 0 {
            StreamType::from_ctype(buffer.stream_type as u32).unwrap_or(StreamType::Unknownstream)
        } else {
            StreamType::Unknownstream
        };

        let printable_stream_type = if buffer.printable_stream_type != 0 {
            buffer.printable_stream_type
        } else {
            0
        };

        let mut pmt_entry = PMTEntry {
            program_number,
            elementary_pid,
            stream_type,
            printable_stream_type,
        };

        Some(&mut pmt_entry as *mut PMTEntry)
    }
}
impl FromCType<ccx_bufferdata_type> for BufferdataType {
    unsafe fn from_ctype(c_value: ccx_bufferdata_type) -> Option<Self> {
        let rust_value = match c_value {
            ccx_bufferdata_type_CCX_UNKNOWN => BufferdataType::Unknown,
            ccx_bufferdata_type_CCX_PES => BufferdataType::Pes,
            ccx_bufferdata_type_CCX_RAW => BufferdataType::Raw,
            ccx_bufferdata_type_CCX_H264 => BufferdataType::H264,
            ccx_bufferdata_type_CCX_HAUPPAGE => BufferdataType::Hauppage,
            ccx_bufferdata_type_CCX_TELETEXT => BufferdataType::Teletext,
            ccx_bufferdata_type_CCX_PRIVATE_MPEG2_CC => BufferdataType::PrivateMpeg2Cc,
            ccx_bufferdata_type_CCX_DVB_SUBTITLE => BufferdataType::DvbSubtitle,
            ccx_bufferdata_type_CCX_ISDB_SUBTITLE => BufferdataType::IsdbSubtitle,
            ccx_bufferdata_type_CCX_RAW_TYPE => BufferdataType::RawType,
            ccx_bufferdata_type_CCX_DVD_SUBTITLE => BufferdataType::DvdSubtitle,
            _ => BufferdataType::Unknown,
        };
        Some(rust_value)
    }
}

impl FromCType<ccx_rational> for CcxRational {
    unsafe fn from_ctype(c_value: ccx_rational) -> Option<Self> {
        Some(CcxRational {
            num: c_value.num,
            den: c_value.den,
        })
    }
}
/// # Safety
/// This function is unsafe because it takes a raw pointer to a C struct.
impl FromCType<PSI_buffer> for PSIBuffer {
    unsafe fn from_ctype(buffer: PSI_buffer) -> Option<Self> {
        // Create a new PSIBuffer
        let psi_buffer = PSIBuffer {
            prev_ccounter: buffer.prev_ccounter,
            buffer: buffer.buffer,
            buffer_length: buffer.buffer_length,
            ccounter: buffer.ccounter,
        };

        // Box it and convert to raw pointer
        Some(psi_buffer)
    }
}
/// # Safety
/// This function is unsafe because it takes a raw pointer to a C struct.
impl FromCType<PMT_entry> for PMTEntry {
    unsafe fn from_ctype(buffer: PMT_entry) -> Option<Self> {
        let program_number = if buffer.program_number != 0 {
            buffer.program_number
        } else {
            0
        };

        let elementary_pid = if buffer.elementary_PID != 0 {
            buffer.elementary_PID
        } else {
            0
        };

        let stream_type = if buffer.stream_type != 0 {
            StreamType::from_ctype(buffer.stream_type as u32).unwrap_or(StreamType::Unknownstream)
        } else {
            StreamType::Unknownstream
        };

        let printable_stream_type = if buffer.printable_stream_type != 0 {
            buffer.printable_stream_type
        } else {
            0
        };

        Some(PMTEntry {
            program_number,
            elementary_pid,
            stream_type,
            printable_stream_type,
        })
    }
}
impl FromCType<(u32, u32)> for DebugMessageMask {
    unsafe fn from_ctype((mask_on_normal, mask_on_debug): (u32, u32)) -> Option<Self> {
        Some(DebugMessageMask::new(
            DebugMessageFlag::from_bits_truncate(mask_on_normal as u16),
            DebugMessageFlag::from_bits_truncate(mask_on_debug as u16),
        ))
    }
}

impl FromCType<u8> for AvcNalType {
    unsafe fn from_ctype(value: u8) -> Option<AvcNalType> {
        let returnvalue = match value {
            0 => AvcNalType::Unspecified0,
            1 => AvcNalType::CodedSliceNonIdrPicture1,
            2 => AvcNalType::CodedSlicePartitionA,
            3 => AvcNalType::CodedSlicePartitionB,
            4 => AvcNalType::CodedSlicePartitionC,
            5 => AvcNalType::CodedSliceIdrPicture,
            6 => AvcNalType::Sei,
            7 => AvcNalType::SequenceParameterSet7,
            8 => AvcNalType::PictureParameterSet,
            9 => AvcNalType::AccessUnitDelimiter9,
            10 => AvcNalType::EndOfSequence,
            11 => AvcNalType::EndOfStream,
            12 => AvcNalType::FillerData,
            13 => AvcNalType::SequenceParameterSetExtension,
            14 => AvcNalType::PrefixNalUnit,
            15 => AvcNalType::SubsetSequenceParameterSet,
            16 => AvcNalType::Reserved16,
            17 => AvcNalType::Reserved17,
            18 => AvcNalType::Reserved18,
            19 => AvcNalType::CodedSliceAuxiliaryPicture,
            20 => AvcNalType::CodedSliceExtension,
            21 => AvcNalType::Reserved21,
            22 => AvcNalType::Reserved22,
            23 => AvcNalType::Reserved23,
            24 => AvcNalType::Unspecified24,
            25 => AvcNalType::Unspecified25,
            26 => AvcNalType::Unspecified26,
            27 => AvcNalType::Unspecified27,
            28 => AvcNalType::Unspecified28,
            29 => AvcNalType::Unspecified29,
            30 => AvcNalType::Unspecified30,
            31 => AvcNalType::Unspecified31,
            _ => AvcNalType::Unspecified0, // Default fallback
        };
        Some(returnvalue)
    }
}
impl FromCType<ccx_frame_type> for FrameType {
    // TODO move to ctorust.rs when demuxer is merged
    unsafe fn from_ctype(c_value: ccx_frame_type) -> Option<Self> {
        match c_value {
            ccx_frame_type_CCX_FRAME_TYPE_RESET_OR_UNKNOWN => Some(FrameType::ResetOrUnknown),
            ccx_frame_type_CCX_FRAME_TYPE_I_FRAME => Some(FrameType::IFrame),
            ccx_frame_type_CCX_FRAME_TYPE_P_FRAME => Some(FrameType::PFrame),
            ccx_frame_type_CCX_FRAME_TYPE_B_FRAME => Some(FrameType::BFrame),
            ccx_frame_type_CCX_FRAME_TYPE_D_FRAME => Some(FrameType::DFrame),
            _ => None,
        }
    }
}

impl FromCType<avc_ctx> for AvcContextRust {
    unsafe fn from_ctype(ctx: avc_ctx) -> Option<Self> {
        // Convert cc_data from C pointer to Vec
        let cc_data = if !ctx.cc_data.is_null() && ctx.cc_databufsize > 0 {
            std::slice::from_raw_parts(ctx.cc_data, ctx.cc_databufsize as usize).to_vec()
        } else {
            Vec::with_capacity(1024)
        };

        Some(AvcContextRust {
            cc_count: ctx.cc_count,
            cc_data,
            cc_databufsize: ctx.cc_databufsize as usize,
            cc_buffer_saved: ctx.cc_buffer_saved != 0,

            got_seq_para: ctx.got_seq_para != 0,
            nal_ref_idc: ctx.nal_ref_idc,
            seq_parameter_set_id: ctx.seq_parameter_set_id,
            log2_max_frame_num: ctx.log2_max_frame_num,
            pic_order_cnt_type: ctx.pic_order_cnt_type,
            log2_max_pic_order_cnt_lsb: ctx.log2_max_pic_order_cnt_lsb,
            frame_mbs_only_flag: ctx.frame_mbs_only_flag != 0,

            num_nal_unit_type_7: ctx.num_nal_unit_type_7 as _,
            num_vcl_hrd: ctx.num_vcl_hrd as _,
            num_nal_hrd: ctx.num_nal_hrd as _,
            num_jump_in_frames: ctx.num_jump_in_frames as _,
            num_unexpected_sei_length: ctx.num_unexpected_sei_length as _,

            ccblocks_in_avc_total: ctx.ccblocks_in_avc_total,
            ccblocks_in_avc_lost: ctx.ccblocks_in_avc_lost,

            frame_num: ctx.frame_num,
            lastframe_num: ctx.lastframe_num,
            currref: ctx.currref,
            maxidx: ctx.maxidx,
            lastmaxidx: ctx.lastmaxidx,

            minidx: ctx.minidx,
            lastminidx: ctx.lastminidx,

            maxtref: ctx.maxtref,
            last_gop_maxtref: ctx.last_gop_maxtref,

            currefpts: ctx.currefpts,
            last_pic_order_cnt_lsb: ctx.last_pic_order_cnt_lsb,
            last_slice_pts: ctx.last_slice_pts,
        })
    }
}
