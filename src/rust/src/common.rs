use crate::bindings::*;
use crate::utils::string_to_c_char;
use crate::utils::string_to_c_chars;

#[derive(Debug)]
pub struct CcxTeletextConfig {
    pub verbose: bool,
    pub page: u16,
    pub tid: u16,
    pub offset: f64,
    pub bom: bool,
    pub nonempty: bool,
    pub user_page: u16,
    pub dolevdist: i32,
    pub levdistmincnt: i32,
    pub levdistmaxpct: i32,
    pub extraction_start: CcxBoundaryTime,
    pub extraction_end: CcxBoundaryTime,
    pub write_format: CcxOutputFormat,
    pub gui_mode_reports: bool,
    pub date_format: CcxOutputDateFormat,
    pub noautotimeref: bool,
    pub send_to_srv: bool,
    pub encoding: CcxEncodingType,
    pub nofontcolor: bool,
    pub nohtmlescape: bool,
    pub millis_separator: char,
    pub latrusmap: bool,
}

impl Default for CcxTeletextConfig {
    fn default() -> Self {
        Self {
            verbose: true,
            page: 0,
            tid: 0,
            offset: 0.0,
            bom: true,
            nonempty: true,
            user_page: 0,
            dolevdist: 0,
            levdistmincnt: 0,
            levdistmaxpct: 0,
            extraction_start: CcxBoundaryTime::default(),
            extraction_end: CcxBoundaryTime::default(),
            write_format: CcxOutputFormat::default(),
            gui_mode_reports: false,
            date_format: CcxOutputDateFormat::default(),
            noautotimeref: false,
            send_to_srv: false,
            encoding: CcxEncodingType::default(),
            nofontcolor: false,
            nohtmlescape: false,
            millis_separator: ',',
            latrusmap: false,
        }
    }
}

impl CcxTeletextConfig {
    pub fn to_ctype(&self) -> ccx_s_teletext_config {
        let mut config = ccx_s_teletext_config {
            _bitfield_1: Default::default(),
            _bitfield_2: Default::default(),
            _bitfield_align_1: Default::default(),
            _bitfield_align_2: Default::default(),
            page: self.page,
            tid: self.tid,
            offset: self.offset,
            user_page: self.user_page,
            dolevdist: self.dolevdist,
            levdistmincnt: self.levdistmincnt,
            levdistmaxpct: self.levdistmaxpct,
            extraction_start: self.extraction_start.to_ctype(),
            extraction_end: self.extraction_end.to_ctype(),
            write_format: self.write_format.into(),
            gui_mode_reports: self.gui_mode_reports as _,
            date_format: self.date_format as _,
            noautotimeref: self.noautotimeref as _,
            send_to_srv: self.send_to_srv as _,
            encoding: self.encoding as _,
            nofontcolor: self.nofontcolor as _,
            nohtmlescape: self.nohtmlescape as _,
            millis_separator: self.millis_separator as _,
            latrusmap: self.latrusmap as _,
        };
        config.set_verbose(if self.verbose { 1 } else { 0 });
        config.set_bom(if self.bom { 1 } else { 0 });
        config.set_nonempty(if self.nonempty { 1 } else { 0 });

        config
    }
}

#[derive(Debug, Default, Clone)]
pub struct CcxBoundaryTime {
    pub hh: i32,
    pub mm: i32,
    pub ss: i32,
    pub time_in_ms: i64,
    pub set: bool,
}

impl CcxBoundaryTime {
    pub fn start_credits_not_before() -> CcxBoundaryTime {
        CcxBoundaryTime {
            hh: 0,
            mm: 0,
            ss: 0,
            time_in_ms: 0,
            set: true,
        }
    }
    pub fn start_credits_not_after() -> CcxBoundaryTime {
        CcxBoundaryTime {
            hh: 0,
            mm: 5,
            ss: 0,
            time_in_ms: 300000,
            set: true,
        }
    }
    pub fn start_credits_for_atleast() -> CcxBoundaryTime {
        CcxBoundaryTime {
            hh: 0,
            mm: 0,
            ss: 2,
            time_in_ms: 2000,
            set: true,
        }
    }
    pub fn start_credits_for_atmost() -> CcxBoundaryTime {
        CcxBoundaryTime {
            hh: 0,
            mm: 0,
            ss: 5,
            time_in_ms: 5000,
            set: true,
        }
    }
    pub fn end_credits_for_atleast() -> CcxBoundaryTime {
        CcxBoundaryTime {
            hh: 0,
            mm: 0,
            ss: 2,
            time_in_ms: 2000,
            set: true,
        }
    }
    pub fn end_credits_for_atmost() -> CcxBoundaryTime {
        CcxBoundaryTime {
            hh: 0,
            mm: 0,
            ss: 5,
            time_in_ms: 5000,
            set: true,
        }
    }

    pub fn to_ctype(&self) -> ccx_boundary_time {
        ccx_boundary_time {
            hh: self.hh,
            mm: self.mm,
            ss: self.ss,
            time_in_ms: self.time_in_ms,
            set: self.set as _,
        }
    }
}

#[derive(Debug, Default, Copy, Clone)]
pub struct CcxDecoder608Report {
    pub xds: bool,
    pub cc_channels: [u8; 4],
}

impl CcxDecoder608Report {
    pub fn to_ctype(&self) -> ccx_decoder_608_report {
        let mut decoder = ccx_decoder_608_report {
            _bitfield_1: Default::default(),
            _bitfield_align_1: Default::default(),
            cc_channels: self.cc_channels,
        };
        decoder.set_xds(if self.xds { 1 } else { 0 });
        decoder
    }
}

#[derive(Debug, Copy, Clone)]
pub struct CcxDecoder608Settings {
    pub direct_rollup: i32,
    pub force_rollup: i32,
    pub no_rollup: bool,
    pub default_color: CcxDecoder608ColorCode,
    pub screens_to_process: i32,
    pub report: Option<CcxDecoder608Report>,
}

impl Default for CcxDecoder608Settings {
    fn default() -> Self {
        Self {
            direct_rollup: 0,
            force_rollup: 0,
            no_rollup: false,
            default_color: CcxDecoder608ColorCode::Transparent,
            screens_to_process: -1,
            report: None,
        }
    }
}

impl CcxDecoder608Settings {
    pub fn to_ctype(&self) -> ccx_decoder_608_settings {
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

pub const CCX_DTVCC_MAX_SERVICES: usize = 63;

impl Default for CcxFrameType {
    fn default() -> Self {
        Self::ResetOrUnknown
    }
}

#[derive(Debug, Copy, Clone)]
pub enum CcxFrameType {
    ResetOrUnknown = 0,
    // IFrame = 1,
    // PFrame = 2,
    // BFrame = 3,
    // DFrame = 4,
}

#[derive(Debug, Default)]
pub struct CcxCommonTimingCtx {
    pub pts_set: i32,          // 0 = No, 1 = received, 2 = min_pts set
    pub min_pts_adjusted: i32, // 0 = No, 1=Yes (don't adjust again)
    pub current_pts: i64,
    pub current_picture_coding_type: CcxFrameType,
    pub current_tref: i32, // Store temporal reference of current frame
    pub min_pts: i64,
    pub max_pts: i64,
    pub sync_pts: i64,
    pub minimum_fts: i64,      // No screen should start before this FTS
    pub fts_now: i64,          // Time stamp of current file (w/ fts_offset, w/o fts_global)
    pub fts_offset: i64,       // Time before first sync_pts
    pub fts_fc_offset: i64,    // Time before first GOP
    pub fts_max: i64,          // Remember the maximum fts that we saw in current file
    pub fts_global: i64,       // Duration of previous files (-ve mode)
    pub sync_pts2fts_set: i32, // 0 = No, 1 = Yes
    pub sync_pts2fts_fts: i64,
    pub sync_pts2fts_pts: i64,
    pub pts_reset: i32, // 0 = No, 1 = Yes. PTS resets when current_pts is lower than prev
}

impl CcxCommonTimingCtx {
    pub fn to_ctype(&self) -> ccx_common_timing_ctx {
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

impl Default for CcxDecoderDtvccSettings {
    fn default() -> Self {
        Self {
            enabled: false,
            print_file_reports: true,
            no_rollup: false,
            report: None,
            active_services_count: 0,
            services_enabled: [false; CCX_DTVCC_MAX_SERVICES],
            timing: CcxCommonTimingCtx::default(),
        }
    }
}

#[derive(Debug)]
pub struct CcxDecoderDtvccSettings {
    pub enabled: bool,
    pub print_file_reports: bool,
    pub no_rollup: bool,
    pub report: Option<CcxDecoderDtvccReport>,
    pub active_services_count: i32,
    pub services_enabled: [bool; CCX_DTVCC_MAX_SERVICES],
    pub timing: CcxCommonTimingCtx,
}

impl CcxDecoderDtvccSettings {
    pub fn to_ctype(&self) -> ccx_decoder_dtvcc_settings {
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

impl Default for CcxDecoderDtvccReport {
    fn default() -> Self {
        Self {
            reset_count: 0,
            services: [0; CCX_DTVCC_MAX_SERVICES],
        }
    }
}

#[derive(Debug, Copy, Clone)]

pub struct CcxDecoderDtvccReport {
    pub reset_count: i32,
    pub services: [u32; CCX_DTVCC_MAX_SERVICES],
}

impl CcxDecoderDtvccReport {
    pub fn to_ctype(&self) -> ccx_decoder_dtvcc_report {
        ccx_decoder_dtvcc_report {
            reset_count: self.reset_count,
            services: self.services,
        }
    }
}

#[derive(Debug, Clone)]
pub struct CcxEncodersTranscriptFormat {
    pub show_start_time: bool,
    pub show_end_time: bool,
    pub show_mode: bool,
    pub show_cc: bool,
    pub relative_timestamp: bool,
    pub xds: bool,
    pub use_colors: bool,
    pub is_final: bool,
}

impl Default for CcxEncodersTranscriptFormat {
    fn default() -> Self {
        Self {
            show_start_time: false,
            show_end_time: false,
            show_mode: false,
            show_cc: false,
            relative_timestamp: true,
            xds: false,
            use_colors: true,
            is_final: false,
        }
    }
}

impl CcxEncodersTranscriptFormat {
    fn to_ctype(&self) -> ccx_encoders_transcript_format {
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

impl Default for CcxOutputDateFormat {
    fn default() -> Self {
        Self::None
    }
}

#[derive(Debug, PartialEq, Clone, Copy)]
pub enum CcxOutputDateFormat {
    None = 0,
    // HHMMSS = 1,
    Seconds = 2,
    Date = 3,
    HhMmSsMs = 4, // HH:MM:SS,MILIS (.srt style)
}

impl Default for CcxStreamMode {
    fn default() -> Self {
        Self::Autodetect
    }
}

#[derive(PartialEq, Debug, Copy, Clone)]
pub enum CcxStreamMode {
    ElementaryOrNotFound = 0,
    Transport = 1,
    Program = 2,
    Asf = 3,
    McpoodlesRaw = 4,
    Rcwt = 5, // Raw Captions With Time, not used yet.
    // Myth = 6, // Use the myth loop
    Mp4 = 7, // MP4, ISO-
    #[cfg(feature = "wtv_debug")]
    HexDump = 8, // Hexadecimal dump generated by wtvccdump
    Wtv = 9,
    #[cfg(feature = "enable_ffmpeg")]
    Ffmpeg = 10,
    // Gxf = 11,
    Mkv = 12,
    Mxf = 13,
    Autodetect = 16,
}

impl CcxStreamMode {
    pub fn to_ctype(&self) -> ccx_stream_mode_enum {
        match self {
            CcxStreamMode::ElementaryOrNotFound => {
                ccx_stream_mode_enum_CCX_SM_ELEMENTARY_OR_NOT_FOUND
            }
            CcxStreamMode::Transport => ccx_stream_mode_enum_CCX_SM_TRANSPORT,
            CcxStreamMode::Program => ccx_stream_mode_enum_CCX_SM_PROGRAM,
            CcxStreamMode::Asf => ccx_stream_mode_enum_CCX_SM_ASF,
            CcxStreamMode::McpoodlesRaw => ccx_stream_mode_enum_CCX_SM_MCPOODLESRAW,
            CcxStreamMode::Rcwt => ccx_stream_mode_enum_CCX_SM_RCWT,
            // CcxStreamMode::Myth => ccx_stream_mode_enum_CCX_SM_MYTH,
            CcxStreamMode::Mp4 => ccx_stream_mode_enum_CCX_SM_MP4,
            #[cfg(feature = "wtv_debug")]
            CcxStreamMode::HexDump => ccx_stream_mode_enum_CCX_SM_HEX_DUMP,
            CcxStreamMode::Wtv => ccx_stream_mode_enum_CCX_SM_WTV,
            #[cfg(feature = "enable_ffmpeg")]
            CcxStreamMode::Ffmpeg => ccx_stream_mode_enum_CCX_SM_FFMPEG,
            // CcxStreamMode::Gxf => ccx_stream_mode_enum_CCX_SM_GXF,
            CcxStreamMode::Mkv => ccx_stream_mode_enum_CCX_SM_MKV,
            CcxStreamMode::Mxf => ccx_stream_mode_enum_CCX_SM_MXF,
            CcxStreamMode::Autodetect => ccx_stream_mode_enum_CCX_SM_AUTODETECT,
        }
    }
}

impl Default for CcxCodeType {
    fn default() -> Self {
        Self::None
    }
}

#[derive(Debug, PartialEq, Copy, Clone)]
pub enum CcxCodeType {
    Any = 0,
    Teletext = 1,
    Dvb = 2,
    // IsdbCc = 3,
    // AtscCc = 4,
    None = 5,
}

impl CcxCodeType {
    pub fn to_ctype(&self) -> ccx_code_type {
        match self {
            CcxCodeType::Any => ccx_code_type_CCX_CODEC_ANY,
            CcxCodeType::Teletext => ccx_code_type_CCX_CODEC_TELETEXT,
            CcxCodeType::Dvb => ccx_code_type_CCX_CODEC_DVB,
            // CcxCodeType::IsdbCc => ccx_code_type_CCX_CODEC_ISDB_CC,
            // CcxCodeType::AtscCc => ccx_code_type_CCX_CODEC_ATSC_CC,
            CcxCodeType::None => ccx_code_type_CCX_CODEC_NONE,
        }
    }
}

impl Default for CcxDemuxerCfg {
    fn default() -> Self {
        Self {
            m2ts: false,
            auto_stream: CcxStreamMode::default(),
            codec: CcxCodeType::Any,
            nocodec: CcxCodeType::None,
            ts_autoprogram: false,
            ts_allprogram: false,
            ts_cappids: [0; 128],
            nb_ts_cappid: 0,
            ts_forced_cappid: 0,
            ts_forced_program: -1,
            ts_forced_program_selected: false,
            ts_datastreamtype: 0,
            ts_forced_streamtype: 0,
        }
    }
}

#[derive(Debug)]
pub struct CcxDemuxerCfg {
    pub m2ts: bool,
    pub auto_stream: CcxStreamMode,
    pub codec: CcxCodeType,
    pub nocodec: CcxCodeType,
    pub ts_autoprogram: bool,
    pub ts_allprogram: bool,
    pub ts_cappids: [u32; 128],
    pub nb_ts_cappid: i32,
    pub ts_forced_cappid: u32,
    pub ts_forced_program: i32,
    pub ts_forced_program_selected: bool,
    pub ts_datastreamtype: i32,
    pub ts_forced_streamtype: u32,
}

impl CcxDemuxerCfg {
    pub fn to_ctype(&self) -> demuxer_cfg {
        demuxer_cfg {
            m2ts: if self.m2ts { 1 } else { 0 },
            auto_stream: self.auto_stream.to_ctype(),
            codec: self.codec.to_ctype(),
            nocodec: self.nocodec.to_ctype(),
            ts_autoprogram: if self.ts_autoprogram { 1 } else { 0 },
            ts_allprogram: if self.ts_allprogram as _ { 1 } else { 0 },
            ts_cappids: self.ts_cappids,
            nb_ts_cappid: self.nb_ts_cappid,
            ts_forced_cappid: self.ts_forced_cappid,
            ts_forced_program: self.ts_forced_program,
            ts_forced_program_selected: if self.ts_forced_program_selected {
                1
            } else {
                0
            },
            ts_datastreamtype: self.ts_datastreamtype,
            ts_forced_streamtype: self.ts_forced_streamtype,
        }
    }
}

impl Default for CcxOutputFormat {
    fn default() -> Self {
        Self::Raw
    }
}

#[derive(Debug, PartialEq, Clone, Copy)]
pub enum CcxOutputFormat {
    Raw = 0,
    Srt = 1,
    Sami = 2,
    Transcript = 3,
    Rcwt = 4,
    Null = 5,
    Smptett = 6,
    Spupng = 7,
    Dvdraw = 8, // See -d at http://www.theneitherworld.com/mcpoodle/SCC_TOOLS/DOCS/SCC_TOOLS.HTML#CCExtract
    Webvtt = 9,
    // SimpleXml = 10,
    G608 = 11,
    Curl = 12,
    Ssa = 13,
    Mcc = 14,
    Scc = 15,
    Ccd = 16,
}

impl From<CcxOutputFormat> for ccx_output_format {
    fn from(value: CcxOutputFormat) -> ccx_output_format {
        match value {
            CcxOutputFormat::Raw => ccx_output_format::CCX_OF_RAW,
            CcxOutputFormat::Srt => ccx_output_format::CCX_OF_SRT,
            CcxOutputFormat::Sami => ccx_output_format::CCX_OF_SAMI,
            CcxOutputFormat::Transcript => ccx_output_format::CCX_OF_TRANSCRIPT,
            CcxOutputFormat::Rcwt => ccx_output_format::CCX_OF_RCWT,
            CcxOutputFormat::Null => ccx_output_format::CCX_OF_NULL,
            CcxOutputFormat::Smptett => ccx_output_format::CCX_OF_SMPTETT,
            CcxOutputFormat::Spupng => ccx_output_format::CCX_OF_SPUPNG,
            CcxOutputFormat::Dvdraw => ccx_output_format::CCX_OF_DVDRAW,
            CcxOutputFormat::Webvtt => ccx_output_format::CCX_OF_WEBVTT,
            // CcxOutputFormat::SimpleXml => ccx_output_format::CCX_OF_SIMPLE_XML,
            CcxOutputFormat::G608 => ccx_output_format::CCX_OF_G608,
            CcxOutputFormat::Curl => ccx_output_format::CCX_OF_CURL,
            CcxOutputFormat::Ssa => ccx_output_format::CCX_OF_SSA,
            CcxOutputFormat::Mcc => ccx_output_format::CCX_OF_MCC,
            CcxOutputFormat::Scc => ccx_output_format::CCX_OF_SCC,
            CcxOutputFormat::Ccd => ccx_output_format::CCX_OF_CCD,
        }
    }
}

impl Default for CcxEncodingType {
    fn default() -> Self {
        Self::Utf8
    }
}

#[derive(Debug, PartialEq, Clone, Copy)]
pub enum CcxEncodingType {
    Unicode = 0,
    Latin1 = 1,
    Utf8 = 2,
    // Ascii = 3,
}

impl Default for CcxEncoderCfg {
    fn default() -> Self {
        Self {
            extract: 1,
            dtvcc_extract: false,
            gui_mode_reports: false,
            output_filename: String::default(),
            write_format: CcxOutputFormat::default(),
            keep_output_closed: false,
            force_flush: false,
            append_mode: false,
            ucla: false,
            encoding: CcxEncodingType::default(),
            date_format: CcxOutputDateFormat::default(),
            millis_separator: ',',
            autodash: false,
            trim_subs: false,
            sentence_cap: false,
            splitbysentence: false,
            curlposturl: None,
            filter_profanity: false,
            with_semaphore: false,
            start_credits_text: String::default(),
            end_credits_text: String::default(),
            startcreditsnotbefore: CcxBoundaryTime::start_credits_not_before(),
            startcreditsnotafter: CcxBoundaryTime::start_credits_not_after(),
            startcreditsforatleast: CcxBoundaryTime::start_credits_for_atleast(),
            startcreditsforatmost: CcxBoundaryTime::start_credits_for_atmost(),
            endcreditsforatleast: CcxBoundaryTime::end_credits_for_atleast(),
            endcreditsforatmost: CcxBoundaryTime::end_credits_for_atmost(),
            transcript_settings: CcxEncodersTranscriptFormat::default(),
            send_to_srv: false,
            no_bom: true,
            first_input_file: String::default(),
            multiple_files: false,
            no_font_color: false,
            no_type_setting: false,
            cc_to_stdout: false,
            line_terminator_lf: false,
            subs_delay: 0,
            program_number: 0,
            in_format: 1,
            nospupngocr: false,
            force_dropframe: false,
            render_font: String::default(),
            render_font_italics: String::default(),
            services_enabled: [false; CCX_DTVCC_MAX_SERVICES],
            services_charsets: vec![],
            all_services_charset: String::default(),
            extract_only_708: false,
        }
    }
}

#[derive(Debug)]
pub struct CcxEncoderCfg {
    pub extract: i32,
    pub dtvcc_extract: bool,
    pub gui_mode_reports: bool,
    pub output_filename: String,
    pub write_format: CcxOutputFormat,
    pub keep_output_closed: bool,
    pub force_flush: bool,
    pub append_mode: bool,
    pub ucla: bool,
    pub encoding: CcxEncodingType,
    pub date_format: CcxOutputDateFormat,
    pub millis_separator: char,
    pub autodash: bool,
    pub trim_subs: bool,
    pub sentence_cap: bool,
    pub splitbysentence: bool,
    pub curlposturl: Option<String>,
    pub filter_profanity: bool,
    pub with_semaphore: bool,
    pub start_credits_text: String,
    pub end_credits_text: String,
    pub startcreditsnotbefore: CcxBoundaryTime,
    pub startcreditsnotafter: CcxBoundaryTime,
    pub startcreditsforatleast: CcxBoundaryTime,
    pub startcreditsforatmost: CcxBoundaryTime,
    pub endcreditsforatleast: CcxBoundaryTime,
    pub endcreditsforatmost: CcxBoundaryTime,
    pub transcript_settings: CcxEncodersTranscriptFormat,
    pub send_to_srv: bool,
    pub no_bom: bool,
    pub first_input_file: String,
    pub multiple_files: bool,
    pub no_font_color: bool,
    pub no_type_setting: bool,
    pub cc_to_stdout: bool,
    pub line_terminator_lf: bool,
    pub subs_delay: i64,
    pub program_number: i32,
    pub in_format: u8,
    pub nospupngocr: bool,
    pub force_dropframe: bool,
    pub render_font: String,
    pub render_font_italics: String,
    pub services_enabled: [bool; CCX_DTVCC_MAX_SERVICES],
    pub services_charsets: Vec<String>,
    pub all_services_charset: String,
    pub extract_only_708: bool,
}

impl CcxEncoderCfg {
    pub fn to_ctype(&self) -> encoder_cfg {
        unsafe {
            encoder_cfg {
                extract: self.extract,
                dtvcc_extract: self.dtvcc_extract as _,
                gui_mode_reports: self.gui_mode_reports as _,
                output_filename: string_to_c_char(&self.output_filename),
                write_format: self.write_format.into(),
                keep_output_closed: self.keep_output_closed as _,
                force_flush: self.force_flush as _,
                append_mode: self.append_mode as _,
                ucla: self.ucla as _,
                encoding: self.encoding as _,
                date_format: self.date_format as _,
                millis_separator: self.millis_separator as _,
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
                subs_delay: self.subs_delay,
                program_number: self.program_number,
                in_format: self.in_format,
                nospupngocr: self.nospupngocr as _,
                force_dropframe: self.force_dropframe as _,
                render_font: string_to_c_char(&self.render_font),
                render_font_italics: string_to_c_char(&self.render_font_italics),
                services_enabled: self.services_enabled.map(|b| if b { 1 } else { 0 }),
                services_charsets: string_to_c_chars(self.services_charsets.clone()),
                all_services_charset: string_to_c_char(&self.all_services_charset),
                extract_only_708: self.extract_only_708 as _,
            }
        }
    }
}

#[derive(Debug)]
pub struct CcxOptions {
    pub extract: i32,
    pub no_rollup: bool,
    pub noscte20: bool,
    pub webvtt_create_css: bool,
    pub cc_channel: i32,
    pub buffer_input: bool,
    pub nofontcolor: bool,
    pub write_format: CcxOutputFormat,
    pub send_to_srv: bool,
    pub nohtmlescape: bool,
    pub notypesetting: bool,
    pub extraction_start: CcxBoundaryTime,
    pub extraction_end: CcxBoundaryTime,
    pub print_file_reports: bool,
    pub settings_608: CcxDecoder608Settings,
    pub settings_dtvcc: CcxDecoderDtvccSettings,
    pub is_608_enabled: bool,
    pub is_708_enabled: bool,
    pub millis_separator: char,
    pub binary_concat: bool,
    pub use_gop_as_pts: i32,
    pub fix_padding: bool,
    pub gui_mode_reports: bool,
    pub no_progress_bar: bool,
    pub sentence_cap_file: Option<String>,
    pub live_stream: i32,
    pub filter_profanity_file: Option<String>,
    pub messages_target: i32,
    pub timestamp_map: bool,
    pub dolevdist: i32,
    pub levdistmincnt: i32,
    pub levdistmaxpct: i32,
    pub investigate_packets: bool,
    pub fullbin: bool,
    pub nosync: bool,
    pub hauppauge_mode: bool,
    pub wtvconvertfix: bool,
    pub wtvmpeg2: bool,
    pub auto_myth: i32,
    pub mp4vidtrack: bool,
    pub extract_chapters: bool,
    pub usepicorder: bool,
    pub xmltv: i32,
    pub xmltvliveinterval: i32,
    pub xmltvoutputinterval: i32,
    pub xmltvonlycurrent: i32,
    pub keep_output_closed: bool,
    pub force_flush: bool,
    pub append_mode: bool,
    pub ucla: bool,
    pub tickertext: bool,
    pub hardsubx: bool,
    pub hardsubx_and_common: bool,
    pub dvblang: Option<String>,
    pub ocrlang: Option<String>,
    pub ocr_oem: i32,
    pub psm: i32,
    pub ocr_quantmode: i32,
    pub mkvlang: Option<String>,
    pub analyze_video_stream: bool,
    pub hardsubx_ocr_mode: i32,
    pub hardsubx_subcolor: i32,
    pub hardsubx_min_sub_duration: f32,
    pub hardsubx_detect_italics: bool,
    pub hardsubx_conf_thresh: f32,
    pub hardsubx_hue: f32,
    pub hardsubx_lum_thresh: f32,
    pub transcript_settings: CcxEncodersTranscriptFormat,
    pub date: CcxOutputDateFormat,
    pub write_format_rewritten: bool,
    pub use_ass_instead_of_ssa: bool,
    pub use_webvtt_styling: bool,
    pub debug_mask: CcxDebugMessageTypes,
    pub debug_mask_on_debug: i64,
    pub udpsrc: Option<String>,
    pub udpaddr: Option<String>,
    pub udpport: u32,
    pub tcpport: Option<u16>,
    pub tcp_password: Option<String>,
    pub tcp_desc: Option<String>,
    pub srv_addr: Option<String>,
    pub srv_port: Option<u32>,
    pub noautotimeref: bool,
    pub input_source: CcxDatasource,
    pub output_filename: Option<String>,
    pub inputfile: Option<Vec<String>>,
    pub num_input_files: i32,
    pub demux_cfg: CcxDemuxerCfg,
    pub enc_cfg: CcxEncoderCfg,
    pub subs_delay: i64,
    pub cc_to_stdout: bool,
    pub pes_header_to_stdout: bool,
    pub ignore_pts_jumps: bool,
    pub multiprogram: bool,
    pub out_interval: i32,
    pub segment_on_key_frames_only: bool,
    pub curlposturl: Option<String>,
    pub sharing_enabled: bool,
    pub sharing_url: Option<String>,
    pub translate_enabled: bool,
    pub translate_langs: Option<String>,
    pub translate_key: Option<String>,
}

impl Default for CcxOptions {
    fn default() -> Self {
        Self {
            extract: 1,
            no_rollup: false,
            noscte20: false,
            webvtt_create_css: false,
            cc_channel: 1,
            buffer_input: false,
            nofontcolor: false,
            write_format: CcxOutputFormat::Srt,
            send_to_srv: false,
            nohtmlescape: false,
            notypesetting: false,
            extraction_start: CcxBoundaryTime::default(),
            extraction_end: CcxBoundaryTime::default(),
            print_file_reports: false,
            settings_608: CcxDecoder608Settings::default(),
            settings_dtvcc: CcxDecoderDtvccSettings::default(),
            is_608_enabled: false,
            is_708_enabled: false,
            millis_separator: ',',
            binary_concat: true,
            use_gop_as_pts: 0,
            fix_padding: false,
            gui_mode_reports: false,
            no_progress_bar: false,
            sentence_cap_file: None,
            live_stream: 0,
            filter_profanity_file: None,
            messages_target: 1,
            timestamp_map: false,
            dolevdist: 1,
            levdistmincnt: 2,
            levdistmaxpct: 10,
            investigate_packets: false,
            fullbin: false,
            nosync: false,
            hauppauge_mode: false,
            wtvconvertfix: false,
            wtvmpeg2: false,
            auto_myth: 2,
            mp4vidtrack: false,
            extract_chapters: false,
            usepicorder: false,
            xmltv: 0,
            xmltvliveinterval: 10,
            xmltvoutputinterval: 0,
            xmltvonlycurrent: 0,
            keep_output_closed: false,
            force_flush: false,
            append_mode: false,
            ucla: false,
            tickertext: false,
            hardsubx: false,
            hardsubx_and_common: false,
            dvblang: None,
            ocrlang: None,
            ocr_oem: -1,
            ocr_quantmode: 1,
            psm: 3,
            mkvlang: None,
            analyze_video_stream: false,
            hardsubx_ocr_mode: 0,
            hardsubx_subcolor: 0,
            hardsubx_min_sub_duration: 0.5,
            hardsubx_detect_italics: false,
            hardsubx_conf_thresh: 0.0,
            hardsubx_hue: 0.0,
            hardsubx_lum_thresh: 95.0,
            transcript_settings: CcxEncodersTranscriptFormat::default(),
            date: CcxOutputDateFormat::default(),
            write_format_rewritten: false,
            use_ass_instead_of_ssa: false,
            use_webvtt_styling: false,
            debug_mask: CcxDebugMessageTypes::GenericNotices,
            debug_mask_on_debug: 8,
            udpsrc: None,
            udpaddr: None,
            udpport: 0,
            tcpport: None,
            tcp_password: None,
            tcp_desc: None,
            srv_addr: None,
            srv_port: None,
            noautotimeref: false,
            input_source: CcxDatasource::File,
            output_filename: None,
            inputfile: None,
            num_input_files: 0,
            demux_cfg: CcxDemuxerCfg::default(),
            enc_cfg: CcxEncoderCfg::default(),
            subs_delay: 0,
            cc_to_stdout: false,
            pes_header_to_stdout: false,
            ignore_pts_jumps: true,
            multiprogram: false,
            out_interval: -1,
            segment_on_key_frames_only: false,
            curlposturl: None,
            sharing_enabled: false,
            sharing_url: None,
            translate_enabled: false,
            translate_langs: None,
            translate_key: None,
        }
    }
}

impl CcxOptions {
    /// # Safety
    ///
    /// This function is unsafe because it dereferences the pointer passed to it.
    pub unsafe fn to_ctype(&self, options: *mut ccx_s_options) {
        (*options).extract = self.extract;
        (*options).no_rollup = self.no_rollup as _;
        (*options).noscte20 = self.noscte20 as _;
        (*options).webvtt_create_css = self.webvtt_create_css as _;
        (*options).cc_channel = self.cc_channel;
        (*options).buffer_input = self.buffer_input as _;
        (*options).nofontcolor = self.nofontcolor as _;
        (*options).write_format = self.write_format.into();
        (*options).send_to_srv = self.send_to_srv as _;
        (*options).nohtmlescape = self.nohtmlescape as _;
        (*options).notypesetting = self.notypesetting as _;
        (*options).extraction_start = self.extraction_start.to_ctype();
        (*options).extraction_end = self.extraction_end.to_ctype();
        (*options).print_file_reports = self.print_file_reports as _;
        (*options).settings_608 = self.settings_608.to_ctype();
        (*options).settings_dtvcc = self.settings_dtvcc.to_ctype();
        (*options).is_608_enabled = self.is_608_enabled as _;
        (*options).is_708_enabled = self.is_708_enabled as _;
        (*options).millis_separator = self.millis_separator as _;
        (*options).binary_concat = self.binary_concat as _;
        (*options).use_gop_as_pts = self.use_gop_as_pts;
        (*options).fix_padding = self.fix_padding as _;
        (*options).gui_mode_reports = self.gui_mode_reports as _;
        (*options).no_progress_bar = self.no_progress_bar as _;

        if self.sentence_cap_file.is_some() {
            (*options).sentence_cap_file =
                string_to_c_char(&self.sentence_cap_file.clone().unwrap());
        }

        (*options).live_stream = self.live_stream;
        if self.filter_profanity_file.is_some() {
            (*options).filter_profanity_file =
                string_to_c_char(&self.filter_profanity_file.clone().unwrap());
        }
        (*options).messages_target = self.messages_target;
        (*options).timestamp_map = self.timestamp_map as _;
        (*options).dolevdist = self.dolevdist;
        (*options).levdistmincnt = self.levdistmincnt;
        (*options).levdistmaxpct = self.levdistmaxpct;
        (*options).investigate_packets = self.investigate_packets as _;
        (*options).fullbin = self.fullbin as _;
        (*options).nosync = self.nosync as _;
        (*options).hauppauge_mode = self.hauppauge_mode as _;
        (*options).wtvconvertfix = self.wtvconvertfix as _;
        (*options).wtvmpeg2 = self.wtvmpeg2 as _;
        (*options).auto_myth = self.auto_myth;
        (*options).mp4vidtrack = self.mp4vidtrack as _;
        (*options).extract_chapters = self.extract_chapters as _;
        (*options).usepicorder = self.usepicorder as _;
        (*options).xmltv = self.xmltv;
        (*options).xmltvliveinterval = self.xmltvliveinterval;
        (*options).xmltvoutputinterval = self.xmltvoutputinterval;
        (*options).xmltvonlycurrent = self.xmltvonlycurrent;
        (*options).keep_output_closed = self.keep_output_closed as _;
        (*options).force_flush = self.force_flush as _;
        (*options).append_mode = self.append_mode as _;
        (*options).ucla = self.ucla as _;
        (*options).tickertext = self.tickertext as _;
        (*options).hardsubx = self.hardsubx as _;
        (*options).hardsubx_and_common = self.hardsubx_and_common as _;
        if self.dvblang.is_some() {
            (*options).dvblang = string_to_c_char(&self.dvblang.clone().unwrap());
        }
        if self.ocrlang.is_some() {
            (*options).ocrlang = string_to_c_char(&self.ocrlang.clone().unwrap());
        }
        (*options).ocr_oem = self.ocr_oem;
        (*options).ocr_quantmode = self.ocr_quantmode;
        if self.mkvlang.is_some() {
            (*options).mkvlang = string_to_c_char(&self.mkvlang.clone().unwrap());
        }
        (*options).analyze_video_stream = self.analyze_video_stream as _;
        (*options).hardsubx_ocr_mode = self.hardsubx_ocr_mode;
        (*options).hardsubx_subcolor = self.hardsubx_subcolor;
        (*options).hardsubx_min_sub_duration = self.hardsubx_min_sub_duration;
        (*options).hardsubx_detect_italics = self.hardsubx_detect_italics as _;
        (*options).hardsubx_conf_thresh = self.hardsubx_conf_thresh;
        (*options).hardsubx_hue = self.hardsubx_hue;
        (*options).hardsubx_lum_thresh = self.hardsubx_lum_thresh;
        (*options).transcript_settings = self.transcript_settings.to_ctype();
        (*options).date_format = self.date as _;
        (*options).write_format_rewritten = self.write_format_rewritten as _;
        (*options).use_ass_instead_of_ssa = self.use_ass_instead_of_ssa as _;
        (*options).use_webvtt_styling = self.use_webvtt_styling as _;
        (*options).debug_mask = self.debug_mask as _;
        (*options).debug_mask_on_debug = self.debug_mask_on_debug;
        if self.udpsrc.is_some() {
            (*options).udpsrc = string_to_c_char(&self.udpsrc.clone().unwrap());
        }
        if self.udpaddr.is_some() {
            (*options).udpaddr = string_to_c_char(&self.udpaddr.clone().unwrap());
        }
        (*options).udpport = self.udpport;
        if self.tcpport.is_some() {
            (*options).tcpport = string_to_c_char(&self.tcpport.unwrap().to_string());
        }
        if self.tcp_password.is_some() {
            (*options).tcp_password = string_to_c_char(&self.tcp_password.clone().unwrap());
        }
        if self.tcp_desc.is_some() {
            (*options).tcp_desc = string_to_c_char(&self.tcp_desc.clone().unwrap());
        }
        if self.srv_addr.is_some() {
            (*options).srv_addr = string_to_c_char(&self.srv_addr.clone().unwrap());
        }
        if self.srv_port.is_some() {
            (*options).srv_port = string_to_c_char(&self.srv_port.unwrap().to_string());
        }
        (*options).noautotimeref = self.noautotimeref as _;
        (*options).input_source = self.input_source as _;
        if self.output_filename.is_some() {
            (*options).output_filename = string_to_c_char(&self.output_filename.clone().unwrap());
        }
        if self.inputfile.is_some() {
            (*options).inputfile = string_to_c_chars(self.inputfile.clone().unwrap());
        }
        (*options).num_input_files = self.num_input_files;
        (*options).demux_cfg = self.demux_cfg.to_ctype();
        (*options).enc_cfg = self.enc_cfg.to_ctype();
        (*options).subs_delay = self.subs_delay;
        (*options).cc_to_stdout = self.cc_to_stdout as _;
        (*options).pes_header_to_stdout = self.pes_header_to_stdout as _;
        (*options).ignore_pts_jumps = self.ignore_pts_jumps as _;
        (*options).multiprogram = self.multiprogram as _;
        (*options).out_interval = self.out_interval;
        (*options).segment_on_key_frames_only = self.segment_on_key_frames_only as _;
        #[cfg(feature = "with_libcurl")]
        {
            if self.curlposturl.is_some() {
                (*options).curlposturl = string_to_c_char(&self.curlposturl.unwrap());
            }
        }
        #[cfg(feature = "enable_sharing")]
        {
            (*options).sharing_enabled = self.sharing_enabled as _;
            if self.sharing_url.is_some() {
                (*options).sharing_url = string_to_c_char(&self.sharing_url.unwrap());
            }
            (*options).translate_enabled = self.translate_enabled as _;
            if self.translate_langs.is_some() {
                (*options).translate_langs = string_to_c_char(&self.translate_langs.unwrap());
            }
            if self.translate_key.is_some() {
                (*options).translate_key = string_to_c_char(&self.translate_key.unwrap());
            }
        }
    }
}

impl Default for HardsubxOcrMode {
    fn default() -> Self {
        Self::Frame
    }
}
#[allow(dead_code)]
pub enum HardsubxOcrMode {
    Frame = 0,
    Word = 1,
    Letter = 2,
}

impl Default for CcxDatasource {
    fn default() -> Self {
        Self::None
    }
}

#[derive(Debug, PartialEq, Clone, Copy)]
pub enum CcxDatasource {
    None = -1,
    File = 0,
    Stdin = 1,
    Network = 2,
    Tcp = 3,
}

impl Default for CcxEia608Format {
    fn default() -> Self {
        Self::CcScreen
    }
}
pub enum CcxEia608Format {
    CcScreen,
    // CcLine,
    // Xds,
}

impl Default for CcModes {
    fn default() -> Self {
        Self::Text
    }
}

#[derive(Debug)]
pub enum CcModes {
    // Popon = 0,
    // Rollup2 = 1,
    // Rollup3 = 2,
    // Rollup4 = 3,
    Text = 4,
    // Painton = 5,
    // FakeRollup1 = 100,
}

impl Default for FontBits {
    fn default() -> Self {
        Self::Regular
    }
}

#[derive(Debug)]
pub enum FontBits {
    Regular = 0,
    // Italics = 1,
    // Underlined = 2,
    // UnderlinedItalics = 3,
}

impl Default for CcxDecoder608ColorCode {
    fn default() -> Self {
        Self::Userdefined
    }
}

#[derive(Debug, Copy, Clone)]
pub enum CcxDecoder608ColorCode {
    White = 0,
    Green = 1,
    Blue = 2,
    Cyan = 3,
    Red = 4,
    Yellow = 5,
    Magenta = 6,
    Userdefined = 7,
    Black = 8,
    Transparent = 9,
    Max,
}

#[derive(Copy, Clone, Debug)]
pub enum ExitCode {
    NoInputFiles = 2,
    TooManyInputFiles = 3,
    IncompatibleParameters = 4,
    UnableToDetermineFileSize = 6,
    MalformedParameter = 7,
    ReadError = 8,
    NoCaptions = 10,
    WithHelp = 11,
    NotClassified = 300,
    ErrorInCapitalizationFile = 501,
    BufferFull = 502,
    MissingASFHeader = 1001,
    MissingRCWTHeader = 1002,
    FileCreationFailed = 5,
    Unsupported = 9,
    NotEnoughMemory = 500,
    BugBug = 1000,
}

// #[derive(Copy, Clone, Debug)]
// pub enum CCXResult {
// Ok = 0,
// EAGAIN = -100,
// EOF = -101,
// EINVAL = -102,
// ENOSUPP = -103,
// ENOMEM = -104,
// }

#[derive(Copy, Clone, Debug)]
#[allow(dead_code)]
pub enum HardsubxColorType {
    White = 0,
    Yellow = 1,
    Green = 2,
    Cyan = 3,
    Blue = 4,
    Magenta = 5,
    Red = 6,
    Custom = 7,
}

impl Default for CcxDebugMessageTypes {
    fn default() -> Self {
        Self::None
    }
}

#[derive(Copy, Clone, Debug, PartialEq)]
pub enum CcxDebugMessageTypes {
    None = 0,
    Parse = 1,
    Vides = 2,
    Time = 4,
    Verbose = 8,
    Decoder608 = 0x10,
    Decoder708 = 0x20,
    DecoderXds = 0x40,
    Cbraw = 0x80,
    GenericNotices = 0x100,
    Teletext = 0x200,
    Pat = 0x400,
    Pmt = 0x800,
    Levenshtein = 0x1000,
    Dvb = 0x2000,
    Dumpdef = 0x4000,
    #[cfg(feature = "enable_sharing")]
    Share = 0x8000,
}
