use args::{Args, OutFormat};
use num_integer::Integer;

use std::fs::File;
use std::io::{prelude::*, BufReader};
use std::string::String;

use cfg_if::cfg_if;

use common::CcxOptions;
use time::OffsetDateTime;

use crate::args::{self, InFormat, OutputField};
use crate::ccx_encoders_helpers::{
    CAPITALIZATION_LIST, CAPITALIZED_BUILTIN, PROFANE, PROFANE_BUILTIN,
};
use crate::common;
use crate::{
    args::{Codec, Ru},
    common::CcxDebugMessageTypes,
    common::*,
};

cfg_if! {
    if #[cfg(test)] {
        use crate::parser::tests::{set_binary_mode, MPEG_CLOCK_FREQ};
    } else {
        use crate::{set_binary_mode, MPEG_CLOCK_FREQ};
    }
}

cfg_if! {
    if #[cfg(windows)] {
        const DEFAULT_FONT_PATH: &str = "C:\\\\Windows\\\\Fonts\\\\calibri.ttf";
        const DEFAULT_FONT_PATH_ITALICS: &str = "C:\\\\Windows\\\\Fonts\\\\calibrii.ttf";
    } else if #[cfg(target_os = "macos")] {
        const DEFAULT_FONT_PATH: &str = "/System/Library/Fonts/Helvetica.ttc";
        const DEFAULT_FONT_PATH_ITALICS: &str = "/System/Library/Fonts/Helvetica-Oblique.ttf";
    } else {
        const DEFAULT_FONT_PATH: &str = "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf";
        const DEFAULT_FONT_PATH_ITALICS: &str = "/usr/share/fonts/truetype/noto/NotoSans-Italic.ttf";
    }
}

pub static mut FILEBUFFERSIZE: i64 = 1024 * 1024 * 16;
static mut USERCOLOR_RGB: String = String::new();
pub static mut UTC_REFVALUE: u64 = 0;
const CCX_DECODER_608_SCREEN_WIDTH: u16 = 32;

fn to_ms(value: &str) -> CcxBoundaryTime {
    let mut parts = value.rsplit(':');

    let seconds: i32 = parts
        .next()
        .unwrap_or_else(|| {
            println!("Malformed timecode: {}", value);
            std::process::exit(ExitCode::MalformedParameter as i32);
        })
        .parse()
        .unwrap();

    let mut minutes: i32 = 0;

    if let Some(mins) = parts.next() {
        minutes = mins.parse().unwrap();
    };
    let mut hours: i32 = 0;

    if let Some(hrs) = parts.next() {
        hours = hrs.parse().unwrap();
    };

    if seconds > 60 || minutes > 60 {
        println!("Malformed timecode: {}", value);
        std::process::exit(ExitCode::MalformedParameter as i32);
    }

    CcxBoundaryTime {
        hh: hours,
        mm: minutes,
        ss: seconds,
        time_in_ms: (hours + 60 * minutes + 3600 * seconds) as i64,
        set: true,
    }
}

fn get_vector_words(string_array: &[&str]) -> Vec<String> {
    let mut vector = Vec::new();
    for string in string_array {
        vector.push(String::from(*string));
    }
    vector
}

fn atol(bufsize: &str) -> i64 {
    let mut val = bufsize[0..bufsize.len() - 1].parse::<i64>().unwrap();
    let size = bufsize
        .to_string()
        .to_uppercase()
        .chars()
        .nth(bufsize.len() - 1)
        .unwrap();
    if size == 'M' {
        val *= 1024 * 1024;
    } else if size == 'K' {
        val *= 1024;
    }
    val
}

fn atoi_hex<T>(s: &str) -> Result<T, &str>
where
    T: Integer + std::str::FromStr,
{
    if s.len() > 2 && s.to_lowercase().starts_with("0x") {
        // Hexadecimal
        T::from_str_radix(&s[2..], 16).map_err(|_| "not a valid hexadecimal number")
    } else {
        // Decimal
        s.parse::<T>().map_err(|_| "not a valid decimal number")
    }
}

fn get_atoi_hex<T>(s: &str) -> T
where
    T: Integer + std::str::FromStr<Err = std::num::ParseIntError>,
{
    match atoi_hex(s) {
        Ok(val) => val,
        Err(_) => {
            println!("Malformed parameter: {}", s);
            std::process::exit(ExitCode::MalformedParameter as i32)
        }
    }
}

fn process_word_file(filename: &str, list: &mut Vec<String>) -> Result<(), std::io::Error> {
    let file = File::open(filename)?;
    let reader = BufReader::new(file);
    let mut num = 0;

    for line in reader.lines() {
        num += 1;
        let line = line.unwrap();
        if line.starts_with('#') {
            continue;
        }

        let new_len = line.trim().len();
        if new_len > CCX_DECODER_608_SCREEN_WIDTH as usize {
            println!(
                "Word in line {} too long, max = {} characters.",
                num, CCX_DECODER_608_SCREEN_WIDTH
            );
            continue;
        }

        if new_len > 0 {
            list.push(line.trim().to_string());
        }
    }
    Ok(())
}
fn mkvlang_params_check(lang: &str) {
    let mut initial = 0;
    let mut _present = 0;

    for (char_index, c) in lang.to_lowercase().chars().enumerate() {
        if c == ',' {
            _present = char_index;

            if _present - initial < 3 || _present - initial > 6 {
                panic!("language codes should be xxx,xxx,xxx,....\n");
            }

            if _present - initial == 6 {
                let sub_slice = &lang[initial.._present];
                if !sub_slice.contains('-') {
                    panic!("language code is not of the form xxx-xx\n");
                }
            }

            initial = _present + 1;
        }
    }

    // Steps to check for the last lang of multiple mkvlangs provided by the user.
    _present = lang.len() - 1;

    for char_index in (0.._present).rev() {
        if lang.chars().nth(char_index) == Some(',') {
            initial = char_index + 1;
            break;
        }
    }

    if _present - initial < 2 || _present - initial > 5 {
        panic!("last language code should be xxx.\n");
    }

    if _present - initial == 5 {
        let sub_slice = &lang[initial.._present];
        if !sub_slice.contains('-') {
            panic!("last language code is not of the form xxx-xx\n");
        }
    }
}

impl CcxOptions {
    fn set_output_format_type(&mut self, out_format: OutFormat) {
        match out_format {
            #[cfg(feature = "with_libcurl")]
            OutFormat::Curl => options.write_format = CcxOutputFormat::Curl,
            OutFormat::Ass => self.write_format = CcxOutputFormat::Ssa,
            OutFormat::Ccd => self.write_format = CcxOutputFormat::Ccd,
            OutFormat::Scc => self.write_format = CcxOutputFormat::Scc,
            OutFormat::Srt => self.write_format = CcxOutputFormat::Srt,
            OutFormat::Ssa => self.write_format = CcxOutputFormat::Ssa,
            OutFormat::Webvtt => self.write_format = CcxOutputFormat::Webvtt,
            OutFormat::WebvttFull => {
                self.write_format = CcxOutputFormat::Webvtt;
                self.use_webvtt_styling = true;
            }
            OutFormat::Sami => self.write_format = CcxOutputFormat::Sami,
            OutFormat::Txt => {
                self.write_format = CcxOutputFormat::Transcript;
                self.settings_dtvcc.no_rollup = true;
            }
            OutFormat::Ttxt => {
                self.write_format = CcxOutputFormat::Transcript;
                if self.date == CcxOutputDateFormat::None {
                    self.date = CcxOutputDateFormat::HhMmSsMs;
                }
                // Sets the right things so that timestamps and the mode are printed.
                if !self.transcript_settings.is_final {
                    self.transcript_settings.show_start_time = true;
                    self.transcript_settings.show_end_time = true;
                    self.transcript_settings.show_cc = false;
                    self.transcript_settings.show_mode = true;
                }
            }
            OutFormat::Report => {
                self.write_format = CcxOutputFormat::Null;
                self.messages_target = Some(0);
                self.print_file_reports = true;
                self.demux_cfg.ts_allprogram = true;
            }
            OutFormat::Raw => self.write_format = CcxOutputFormat::Raw,
            OutFormat::Smptett => self.write_format = CcxOutputFormat::Smptett,
            OutFormat::Bin => self.write_format = CcxOutputFormat::Rcwt,
            OutFormat::Null => self.write_format = CcxOutputFormat::Null,
            OutFormat::Dvdraw => self.write_format = CcxOutputFormat::Dvdraw,
            OutFormat::Spupng => self.write_format = CcxOutputFormat::Spupng,
            // OutFormat::SimpleXml => self.write_format = CcxOutputFormat::SimpleXml,
            OutFormat::G608 => self.write_format = CcxOutputFormat::G608,
            OutFormat::Mcc => self.write_format = CcxOutputFormat::Mcc,
        }
    }

    fn set_output_format(&mut self, args: &Args) {
        self.write_format_rewritten = true;

        if self.send_to_srv && args.out.unwrap_or(OutFormat::Null) != OutFormat::Bin {
            println!("Output format is changed to bin\n");
            self.set_output_format_type(OutFormat::Bin);
            return;
        }

        if let Some(out_format) = args.out {
            self.set_output_format_type(out_format);
        } else if args.sami {
            self.set_output_format_type(OutFormat::Sami);
        } else if args.webvtt {
            self.set_output_format_type(OutFormat::Webvtt);
        } else if args.srt {
            self.set_output_format_type(OutFormat::Srt);
        } else if args.null {
            self.set_output_format_type(OutFormat::Null);
        } else if args.dvdraw {
            self.set_output_format_type(OutFormat::Dvdraw);
        } else if args.txt {
            self.set_output_format_type(OutFormat::Txt);
        } else if args.ttxt {
            self.set_output_format_type(OutFormat::Ttxt);
        } else if args.mcc {
            self.set_output_format_type(OutFormat::Mcc);
        }
    }

    fn set_input_format_type(&mut self, input_format: InFormat) {
        match input_format {
            #[cfg(feature = "wtv_debug")]
            InFormat::Hex => self.demux_cfg.auto_stream = CcxStreamMode::HexDump,
            InFormat::Es => self.demux_cfg.auto_stream = CcxStreamMode::ElementaryOrNotFound,
            InFormat::Ts => self.demux_cfg.auto_stream = CcxStreamMode::Transport,
            InFormat::M2ts => self.demux_cfg.auto_stream = CcxStreamMode::Transport,
            InFormat::Ps => self.demux_cfg.auto_stream = CcxStreamMode::Program,
            InFormat::Asf => self.demux_cfg.auto_stream = CcxStreamMode::Asf,
            InFormat::Wtv => self.demux_cfg.auto_stream = CcxStreamMode::Wtv,
            InFormat::Raw => self.demux_cfg.auto_stream = CcxStreamMode::McpoodlesRaw,
            InFormat::Bin => self.demux_cfg.auto_stream = CcxStreamMode::Rcwt,
            InFormat::Mp4 => self.demux_cfg.auto_stream = CcxStreamMode::Mp4,
            InFormat::Mkv => self.demux_cfg.auto_stream = CcxStreamMode::Mkv,
            InFormat::Mxf => self.demux_cfg.auto_stream = CcxStreamMode::Mxf,
        }
    }

    fn set_input_format(&mut self, args: &Args) {
        if self.input_source == CcxDatasource::Tcp {
            println!("Input format is changed to bin\n");
            self.set_input_format_type(InFormat::Bin);
            return;
        }

        if let Some(input_format) = args.input {
            self.set_input_format_type(input_format);
        } else if args.es {
            self.set_input_format_type(InFormat::Es);
        } else if args.ts {
            self.set_input_format_type(InFormat::Ts);
        } else if args.ps {
            self.set_input_format_type(InFormat::Ps);
        } else if args.asf || args.dvr_ms {
            self.set_input_format_type(InFormat::Asf);
        } else if args.wtv {
            self.set_input_format_type(InFormat::Wtv);
        } else {
            println!("Unknown input file format: {}\n", args.input.unwrap());
            std::process::exit(ExitCode::MalformedParameter as i32);
        }
    }

    fn parse_708_services(&mut self, s: &str) {
        if s.starts_with("all") {
            let charset = if s.len() > 3 { &s[4..s.len() - 1] } else { "" };
            self.settings_dtvcc.enabled = true;
            self.enc_cfg.dtvcc_extract = true;
            self.enc_cfg.all_services_charset = charset.to_owned();
            self.enc_cfg.services_charsets = vec![String::new(); CCX_DTVCC_MAX_SERVICES];

            for i in 0..CCX_DTVCC_MAX_SERVICES {
                self.settings_dtvcc.services_enabled[i] = true;
                self.enc_cfg.services_enabled[i] = true;
            }

            self.settings_dtvcc.active_services_count = CCX_DTVCC_MAX_SERVICES as _;
            return;
        }

        let mut services = Vec::new();
        let mut charsets = Vec::new();
        for c in s.split(',') {
            let mut service = String::new();
            let mut charset = None;
            let mut inside_charset = false;

            for e in c.chars() {
                if e.is_ascii_digit() && !inside_charset {
                    service.push(e);
                } else if e == '[' {
                    inside_charset = true;
                } else if e == ']' {
                    inside_charset = false;
                } else if inside_charset {
                    if charset.is_none() {
                        charset = Some(String::new());
                    }
                    charset.as_mut().unwrap().push(e);
                }
            }
            if service.is_empty() {
                continue;
            }
            services.push(service);
            charsets.push(charset.clone());
        }

        self.enc_cfg.services_charsets = vec![String::new(); CCX_DTVCC_MAX_SERVICES];

        for (i, service) in services.iter().enumerate() {
            let svc = service.parse::<usize>().unwrap();
            if !(1..=CCX_DTVCC_MAX_SERVICES).contains(&svc) {
                panic!("[CEA-708] Malformed parameter: Invalid service number ({}), valid range is 1-{}.\n", svc, CCX_DTVCC_MAX_SERVICES);
            }
            self.settings_dtvcc.services_enabled[svc - 1] = true;
            self.enc_cfg.services_enabled[svc - 1] = true;
            self.settings_dtvcc.enabled = true;
            self.enc_cfg.dtvcc_extract = true;
            self.settings_dtvcc.active_services_count += 1;

            if charsets.len() > i && charsets[i].is_some() {
                self.enc_cfg.services_charsets[svc - 1] = charsets[i].as_ref().unwrap().to_owned();
            }
        }

        if self.settings_dtvcc.active_services_count == 0 {
            panic!("[CEA-708] Malformed parameter: no services\n");
        }
    }

    pub fn parse_parameters(&mut self, args: &Args, tlt_config: &mut CcxTeletextConfig) {
        if let Some(ref inputfile) = args.inputfile {
            self.inputfile = Some(inputfile.to_owned());
            self.num_input_files = Some(inputfile.len() as _);
        } else {
            println!("No input file specified\n");
            std::process::exit(ExitCode::NoInputFiles as i32);
        }

        if args.stdin {
            unsafe {
                set_binary_mode();
            }
            self.input_source = CcxDatasource::Stdin;
            self.live_stream = Some(-1);
        }

        #[cfg(feature = "hardsubx_ocr")]
        {
            if args.hardsubx {
                self.hardsubx = true;

                if args.hcc {
                    self.hardsubx_and_common = true;
                }

                if let Some(ref ocr_mode) = args.ocr_mode {
                    let ocr_mode = match ocr_mode.as_str() {
                        "simple" | "frame" => Some(HardsubxOcrMode::Frame),
                        "word" => Some(HardsubxOcrMode::Word),
                        "letter" | "symbol" => Some(HardsubxOcrMode::Letter),
                        _ => None,
                    };

                    if ocr_mode.is_none() {
                        println!("Invalid OCR mode");
                        std::process::exit(ExitCode::MalformedParameter as i32);
                    }

                    let value = Some(ocr_mode.unwrap() as i32);
                    self.hardsubx_ocr_mode = value;
                }

                if let Some(ref subcolor) = args.subcolor {
                    match subcolor.as_str() {
                        "white" => {
                            self.hardsubx_subcolor = Some(HardsubxColorType::White as i32);
                            self.hardsubx_hue = Some(0.0);
                        }
                        "yellow" => {
                            self.hardsubx_subcolor = Some(HardsubxColorType::Yellow as i32);
                            self.hardsubx_hue = Some(60.0);
                        }
                        "green" => {
                            self.hardsubx_subcolor = Some(HardsubxColorType::Green as i32);
                            self.hardsubx_hue = Some(120.0);
                        }
                        "cyan" => {
                            self.hardsubx_subcolor = Some(HardsubxColorType::Cyan as i32);
                            self.hardsubx_hue = Some(180.0);
                        }
                        "blue" => {
                            self.hardsubx_subcolor = Some(HardsubxColorType::Blue as i32);
                            self.hardsubx_hue = Some(240.0);
                        }
                        "magenta" => {
                            self.hardsubx_subcolor = Some(HardsubxColorType::Magenta as i32);
                            self.hardsubx_hue = Some(300.0);
                        }
                        "red" => {
                            self.hardsubx_subcolor = Some(HardsubxColorType::Red as i32);
                            self.hardsubx_hue = Some(0.0);
                        }
                        _ => {
                            let result = subcolor.parse::<f32>();
                            if result.is_err() {
                                println!("Invalid Hue value");
                                std::process::exit(ExitCode::MalformedParameter as i32);
                            }

                            let hue: f32 = result.unwrap();

                            if hue <= 0.0 || hue > 360.0 {
                                println!("Invalid Hue value");
                                std::process::exit(ExitCode::MalformedParameter as i32);
                            }
                            self.hardsubx_subcolor = Some(HardsubxColorType::Custom as i32);
                            self.hardsubx_hue = Some(hue);
                        }
                    }
                }

                if let Some(ref value) = args.min_sub_duration {
                    if *value == 0.0 {
                        println!("Invalid minimum subtitle duration");
                        std::process::exit(ExitCode::MalformedParameter as i32);
                    }
                    self.hardsubx_min_sub_duration = Some(*value);
                }

                if args.detect_italics {
                    self.hardsubx_detect_italics = true;
                }

                if let Some(ref value) = args.conf_thresh {
                    if !(0.0..=100.0).contains(value) {
                        println!("Invalid confidence threshold, valid values are between 0 & 100");
                        std::process::exit(ExitCode::MalformedParameter as i32);
                    }
                    self.hardsubx_conf_thresh = Some(*value);
                }

                if let Some(ref value) = args.whiteness_thresh {
                    if !(0.0..=100.0).contains(value) {
                        println!("Invalid whiteness threshold, valid values are between 0 & 100");
                        std::process::exit(ExitCode::MalformedParameter as i32);
                    }
                    self.hardsubx_lum_thresh = Some(*value);
                }
            }
        } // END OF HARDSUBX

        if args.chapters {
            self.extract_chapters = true;
        }

        if args.bufferinput {
            self.buffer_input = true;
        }

        if args.no_bufferinput {
            self.buffer_input = false;
        }

        if args.koc {
            self.keep_output_closed = true;
        }

        if args.forceflush {
            self.force_flush = true;
        }

        if args.append {
            self.append_mode = true;
        }

        if let Some(ref buffersize) = args.buffersize {
            unsafe {
                FILEBUFFERSIZE = atol(buffersize);

                if FILEBUFFERSIZE < 8 {
                    FILEBUFFERSIZE = 8; // Otherwise crashes are guaranteed at least in MythTV
                }
            }
        }

        if args.dru {
            self.settings_608.direct_rollup = 1;
        }

        if args.no_fontcolor {
            self.nofontcolor = true;
        }

        if args.no_htmlescape {
            self.nohtmlescape = true;
        }

        if args.bom {
            self.enc_cfg.no_bom = false;
        }
        if args.no_bom {
            self.enc_cfg.no_bom = true;
        }

        if args.sem {
            self.enc_cfg.with_semaphore = true;
        }

        if args.no_typesetting {
            self.notypesetting = true;
        }

        if args.timestamp_map {
            self.timestamp_map = true;
        }

        if args.es
            || args.ts
            || args.ps
            || args.asf
            || args.wtv
            || args.mp4
            || args.mkv
            || args.dvr_ms
            || args.input.is_some()
        {
            self.set_input_format(args);
        }

        if let Some(ref codec) = args.codec {
            match codec {
                Codec::Teletext => {
                    self.demux_cfg.codec = CcxCodeType::Teletext;
                }
                Codec::Dvbsub => {
                    self.demux_cfg.codec = CcxCodeType::Dvb;
                }
            }
        }

        if let Some(ref codec) = args.no_codec {
            match codec {
                Codec::Dvbsub => {
                    self.demux_cfg.nocodec = CcxCodeType::Dvb;
                }
                Codec::Teletext => {
                    self.demux_cfg.nocodec = CcxCodeType::Teletext;
                }
            }
        }

        if let Some(ref lang) = args.dvblang {
            self.dvblang = Some(lang.clone());
        }

        if let Some(ref lang) = args.ocrlang {
            self.ocrlang = Some(lang.clone());
        }

        if let Some(ref quant) = args.quant {
            if !(0..=2).contains(quant) {
                println!("Invalid quant value");
                std::process::exit(ExitCode::MalformedParameter as i32);
            }
            self.ocr_quantmode = Some(*quant);
        }

        if args.no_spupngocr {
            self.enc_cfg.nospupngocr = true;
        }

        if let Some(ref oem) = args.oem {
            if !(0..=2).contains(oem) {
                println!("Invalid oem value");
                std::process::exit(ExitCode::MalformedParameter as i32);
            }
            self.ocr_oem = Some(*oem);
        }

        if let Some(ref lang) = args.mkvlang {
            self.mkvlang = Some(lang.to_string());
            let str = lang.as_str();
            mkvlang_params_check(str);
        }
        if args.srt
            || args.mcc
            || args.dvdraw
            || args.smi
            || args.sami
            || args.txt
            || args.ttxt
            || args.webvtt
            || args.null
            || args.out.is_some()
        {
            self.set_output_format(args);
        }

        if let Some(ref startcreditstext) = args.startcreditstext {
            self.enc_cfg.start_credits_text = startcreditstext.clone();
        }

        if let Some(ref startcreditsnotbefore) = args.startcreditsnotbefore {
            self.enc_cfg.startcreditsnotbefore = to_ms(startcreditsnotbefore.clone().as_str());
        }

        if let Some(ref startcreditsnotafter) = args.startcreditsnotafter {
            self.enc_cfg.startcreditsnotafter = to_ms(startcreditsnotafter.clone().as_str());
        }

        if let Some(ref startcreditsforatleast) = args.startcreditsforatleast {
            self.enc_cfg.startcreditsforatleast = to_ms(startcreditsforatleast.clone().as_str());
        }
        if let Some(ref startcreditsforatmost) = args.startcreditsforatmost {
            self.enc_cfg.startcreditsforatmost = to_ms(startcreditsforatmost.clone().as_str());
        }

        if let Some(ref endcreditstext) = args.endcreditstext {
            self.enc_cfg.end_credits_text = endcreditstext.clone();
        }

        if let Some(ref endcreditsforatleast) = args.endcreditsforatleast {
            self.enc_cfg.endcreditsforatleast = to_ms(endcreditsforatleast.clone().as_str());
        }

        if let Some(ref endcreditsforatmost) = args.endcreditsforatmost {
            self.enc_cfg.endcreditsforatmost = to_ms(endcreditsforatmost.clone().as_str());
        }

        /* More stuff */
        if args.videoedited {
            self.binary_concat = false;
        }

        if args.goptime {
            self.use_gop_as_pts = 1;
        }
        if args.no_goptime {
            self.use_gop_as_pts = -1;
        }

        if args.fixpadding {
            self.fix_padding = true;
        }

        if args.mpeg90090 {
            unsafe {
                MPEG_CLOCK_FREQ = 90090;
            }
        }
        if args.no_scte20 {
            self.noscte20 = true;
        }

        if args.webvtt_create_css {
            self.webvtt_create_css = true;
        }

        if args.no_rollup {
            self.no_rollup = true;
            self.settings_608.no_rollup = true;
            self.settings_dtvcc.no_rollup = true;
        }

        if let Some(ref ru) = args.rollup {
            match ru {
                Ru::Ru1 => {
                    self.settings_608.force_rollup = 1;
                }
                Ru::Ru2 => {
                    self.settings_608.force_rollup = 2;
                }
                Ru::Ru3 => {
                    self.settings_608.force_rollup = 3;
                }
            }
        }

        if args.trim {
            self.enc_cfg.trim_subs = true;
        }

        if let Some(ref outinterval) = args.outinterval {
            self.out_interval = Some(*outinterval);
        }

        if args.segmentonkeyonly {
            self.segment_on_key_frames_only = true;
            self.analyze_video_stream = true;
        }

        if args.gui_mode_reports {
            self.gui_mode_reports = true;
        }

        if args.no_progress_bar {
            self.no_progress_bar = true;
        }

        if args.splitbysentence {
            self.enc_cfg.splitbysentence = true;
        }

        if args.sentencecap {
            self.enc_cfg.sentence_cap = true;
        }

        if let Some(ref capfile) = args.capfile {
            self.enc_cfg.sentence_cap = true;
            self.sentence_cap_file = Some(capfile.clone());
        }

        if args.kf {
            self.enc_cfg.filter_profanity = true;
        }

        if let Some(ref profanity_file) = args.profanity_file {
            self.enc_cfg.filter_profanity = true;
            self.filter_profanity_file = Some(profanity_file.clone());
        }

        if let Some(ref program_number) = args.program_number {
            self.demux_cfg.ts_forced_program = get_atoi_hex(program_number.as_str());
            self.demux_cfg.ts_forced_program_selected = true;
        }

        if args.autoprogram {
            self.demux_cfg.ts_autoprogram = true;
        }

        if args.multiprogram {
            self.multiprogram = true;
            self.demux_cfg.ts_allprogram = true;
        }

        if let Some(ref stream) = args.stream {
            self.live_stream = Some(get_atoi_hex(stream.as_str()));
        }

        if let Some(ref defaultcolor) = args.defaultcolor {
            unsafe {
                if defaultcolor.len() != 7 || !defaultcolor.starts_with('#') {
                    println!("Invalid default color");
                    std::process::exit(ExitCode::MalformedParameter as i32);
                }
                USERCOLOR_RGB = defaultcolor.clone();
                self.settings_608.default_color = CcxDecoder608ColorCode::Userdefined;
            }
        }

        if let Some(ref delay) = args.delay {
            self.subs_delay = *delay;
        }

        if let Some(ref screenfuls) = args.screenfuls {
            self.settings_608.screens_to_process = get_atoi_hex(screenfuls.as_str());
        }

        if let Some(ref startat) = args.startat {
            self.extraction_start = to_ms(startat.clone().as_str());
        }
        if let Some(ref endat) = args.endat {
            self.extraction_end = to_ms(endat.clone().as_str());
        }

        if args.cc2 {
            self.cc_channel = Some(2);
        }

        if let Some(ref extract) = args.output_field {
            self.extract = Some(*extract);
            self.is_608_enabled = true;
        }

        if args.stdout {
            if let Some(messages_target) = self.messages_target {
                if messages_target == 1 {
                    self.messages_target = Some(2);
                }
            }
            self.cc_to_stdout = true;
        }

        if args.pesheader {
            self.pes_header_to_stdout = true;
        }

        if args.debugdvdsub {
            self.debug_mask = CcxDebugMessageTypes::Dvb;
        }

        if args.ignoreptsjumps {
            self.ignore_pts_jumps = true;
        }

        if args.fixptsjumps {
            self.ignore_pts_jumps = false;
        }

        if args.quiet {
            self.messages_target = Some(0);
        }

        if args.debug {
            self.debug_mask = CcxDebugMessageTypes::Verbose;
        }

        if args.eia608 {
            self.debug_mask = CcxDebugMessageTypes::Decoder608;
        }

        if args.deblev {
            self.debug_mask = CcxDebugMessageTypes::Levenshtein;
        }

        if args.no_levdist {
            self.dolevdist = 0;
        }

        if let Some(ref levdistmincnt) = args.levdistmincnt {
            self.levdistmincnt = Some(get_atoi_hex(levdistmincnt.as_str()));
        }
        if let Some(ref levdistmaxpct) = args.levdistmaxpct {
            self.levdistmaxpct = Some(get_atoi_hex(levdistmaxpct.as_str()));
        }

        if args.eia708 {
            self.debug_mask = CcxDebugMessageTypes::Parse;
        }

        if args.goppts {
            self.debug_mask = CcxDebugMessageTypes::Time;
        }

        if args.vides {
            self.debug_mask = CcxDebugMessageTypes::Vides;
            self.analyze_video_stream = true;
        }

        if args.analyzevideo {
            self.analyze_video_stream = true;
        }

        if args.xds {
            self.transcript_settings.xds = true;
        }
        if args.xdsdebug {
            self.transcript_settings.xds = true;
            self.debug_mask = CcxDebugMessageTypes::DecoderXds;
        }

        if args.parsedebug {
            self.debug_mask = CcxDebugMessageTypes::Parse;
        }

        if args.parse_pat {
            self.debug_mask = CcxDebugMessageTypes::Pat;
        }

        if args.parse_pmt {
            self.debug_mask = CcxDebugMessageTypes::Pmt;
        }

        if args.dumpdef {
            self.debug_mask = CcxDebugMessageTypes::Dumpdef;
        }

        if args.investigate_packets {
            self.investigate_packets = true;
        }

        if args.cbraw {
            self.debug_mask = CcxDebugMessageTypes::Cbraw;
        }

        if args.tverbose {
            self.debug_mask = CcxDebugMessageTypes::Teletext;
            tlt_config.verbose = true;
        }

        #[cfg(feature = "enable_sharing")]
        {
            if args.sharing_debug {
                self.debug_mask = CcxDebugMessageTypes::Share;
                tlt_config.verbose = true;
            }
        }

        if args.fullbin {
            self.fullbin = true;
        }

        if args.no_sync {
            self.nosync = true;
        }

        if args.hauppauge {
            self.hauppauge_mode = true;
        }

        if args.mp4vidtrack {
            self.mp4vidtrack = true;
        }

        if args.unicode {
            self.enc_cfg.encoding = CcxEncodingType::Unicode;
        }

        if args.utf8 {
            self.enc_cfg.encoding = CcxEncodingType::Utf8;
        }

        if args.latin1 {
            self.enc_cfg.encoding = CcxEncodingType::Latin1;
        }
        if args.usepicorder {
            self.usepicorder = true;
        }

        if args.myth {
            self.auto_myth = Some(1);
        }
        if args.no_myth {
            self.auto_myth = Some(0);
        }

        if args.wtvconvertfix {
            self.wtvconvertfix = true;
        }

        if args.wtvmpeg2 {
            self.wtvmpeg2 = true;
        }

        if let Some(ref output) = args.output {
            self.output_filename = Some(output.clone());
        }

        if let Some(ref service) = args.cea708services {
            self.is_708_enabled = true;
            self.parse_708_services(service);
        }

        if let Some(ref datapid) = args.datapid {
            self.demux_cfg.ts_cappids[self.demux_cfg.nb_ts_cappid as usize] =
                get_atoi_hex(datapid.as_str());
            self.demux_cfg.nb_ts_cappid += 1;
        }

        if let Some(ref datastreamtype) = args.datastreamtype {
            self.demux_cfg.ts_datastreamtype = datastreamtype.clone().parse().unwrap();
        }

        if let Some(ref streamtype) = args.streamtype {
            self.demux_cfg.ts_forced_streamtype = streamtype.clone().parse().unwrap();
        }

        if let Some(ref tpage) = args.tpage {
            tlt_config.page = get_atoi_hex(tpage.as_str());
            tlt_config.user_page = tlt_config.page;
        }

        // Red Hen/ UCLA Specific stuff
        if args.ucla {
            self.ucla = true;
            self.millis_separator = '.';
            self.enc_cfg.no_bom = true;

            if !self.transcript_settings.is_final {
                self.transcript_settings.show_start_time = true;
                self.transcript_settings.show_end_time = true;
                self.transcript_settings.show_cc = true;
                self.transcript_settings.show_mode = true;
                self.transcript_settings.relative_timestamp = false;
                self.transcript_settings.is_final = true;
            }
        }

        if args.latrusmap {
            tlt_config.latrusmap = true;
        }

        if args.tickertext {
            self.tickertext = true;
        }

        if args.lf {
            self.enc_cfg.line_terminator_lf = true;
        }

        if args.df {
            self.enc_cfg.force_dropframe = true;
        }

        if args.no_autotimeref {
            self.noautotimeref = true;
        }

        if args.autodash {
            self.enc_cfg.autodash = true;
        }

        if let Some(ref xmltv) = args.xmltv {
            self.xmltv = Some(get_atoi_hex(xmltv.as_str()));
        }

        if let Some(ref xmltvliveinterval) = args.xmltvliveinterval {
            self.xmltvliveinterval = Some(get_atoi_hex(xmltvliveinterval.as_str()));
        }

        if let Some(ref xmltvoutputinterval) = args.xmltvoutputinterval {
            self.xmltvoutputinterval = Some(get_atoi_hex(xmltvoutputinterval.as_str()));
        }
        if let Some(ref xmltvonlycurrent) = args.xmltvonlycurrent {
            self.xmltvonlycurrent = Some(get_atoi_hex(xmltvonlycurrent.as_str()));
        }

        if let Some(ref unixts) = args.unixts {
            let mut t = get_atoi_hex(unixts.as_str());

            if t == 0 {
                t = OffsetDateTime::now_utc().unix_timestamp() as u64;
            }
            unsafe {
                UTC_REFVALUE = t;
            }
            self.noautotimeref = true;
        }

        if args.sects {
            self.date = CcxOutputDateFormat::Seconds;
        }

        if args.datets {
            self.date = CcxOutputDateFormat::Date;
        }

        if args.teletext {
            self.demux_cfg.codec = CcxCodeType::Teletext;
        }

        if args.no_teletext {
            self.demux_cfg.nocodec = CcxCodeType::Teletext;
        }

        if let Some(ref customtxt) = args.customtxt {
            if customtxt.to_string().len() == 7 {
                if self.date == CcxOutputDateFormat::None {
                    self.date = CcxOutputDateFormat::HhMmSsMs;
                }

                if !self.transcript_settings.is_final {
                    let chars = format!("{}", customtxt).chars().collect::<Vec<char>>();
                    self.transcript_settings.show_start_time = chars[0] == '1';
                    self.transcript_settings.show_end_time = chars[1] == '1';
                    self.transcript_settings.show_mode = chars[2] == '1';
                    self.transcript_settings.show_cc = chars[3] == '1';
                    self.transcript_settings.relative_timestamp = chars[4] == '1';
                    self.transcript_settings.xds = chars[5] == '1';
                    self.transcript_settings.use_colors = chars[6] == '1';
                }
            } else {
                println!("Invalid customtxt value. It must be 7 digits long");
                std::process::exit(ExitCode::MalformedParameter as i32);
            }
        }

        // Network stuff
        if let Some(ref udp) = args.udp {
            if let Some(at) = udp.find('@') {
                let addr = &udp[0..at];
                let port = &udp[at + 1..];

                self.udpsrc = Some(udp.clone());
                self.udpaddr = Some(addr.to_owned());
                self.udpport = Some(port.parse().unwrap());
            } else if let Some(colon) = udp.find(':') {
                let addr = &udp[0..colon];
                let port = get_atoi_hex(&udp[colon + 1..]);

                self.udpsrc = Some(udp.clone());
                self.udpaddr = Some(addr.to_owned());
                self.udpport = Some(port);
            } else {
                self.udpaddr = None;
                self.udpport = Some(udp.parse().unwrap());
            }

            self.input_source = CcxDatasource::Network;
        }

        if let Some(ref addr) = args.sendto {
            self.send_to_srv = true;
            self.set_output_format_type(OutFormat::Bin);

            self.xmltv = Some(2);
            self.xmltvliveinterval = Some(2);
            let mut _addr: String = addr.to_string();

            if let Some(saddr) = addr.strip_prefix('[') {
                _addr = saddr.to_string();

                let mut br = _addr
                    .find(']')
                    .expect("Wrong address format, for IPv6 use [address]:port");
                _addr = _addr.replace(']', "");

                self.srv_addr = Some(_addr.clone());

                br += 1;
                if !_addr[br..].is_empty() {
                    self.srv_port = Some(_addr[br..].parse().unwrap());
                }
            }

            self.srv_addr = Some(_addr.clone());

            let colon = _addr.find(':').unwrap();
            _addr = _addr.replace(':', "");
            self.srv_port = Some(_addr[(colon + 1)..].parse().unwrap());
        }

        if let Some(ref tcp) = args.tcp {
            self.tcpport = Some(*tcp);
            self.input_source = CcxDatasource::Tcp;
            self.set_input_format_type(InFormat::Bin);
        }

        if let Some(ref tcppassworrd) = args.tcp_password {
            self.tcp_password = Some(tcppassworrd.to_string());
        }

        if let Some(ref tcpdesc) = args.tcp_description {
            self.tcp_desc = Some(tcpdesc.to_string());
        }

        if let Some(ref font) = args.font {
            self.enc_cfg.render_font = font.to_string();
        }

        if let Some(ref italics) = args.italics {
            self.enc_cfg.render_font_italics = italics.to_string();
        }

        #[cfg(feature = "with_libcurl")]
        if let Some(ref curlposturl) = args.curlposturl {
            self.curlposturl = curlposturl.to_string();
        }

        #[cfg(feature = "enable_sharing")]
        {
            if args.enable_sharing {
                self.sharing_enabled = true;
            }

            if let Some(ref sharingurl) = args.sharing_url {
                self.sharing_url = sharingurl.to_string();
            }

            if let Some(ref translate) = args.translate {
                self.translate_enabled = true;
                self.sharing_enabled = true;
                self.translate_langs = translate.to_string();
            }

            if let Some(ref translateauth) = args.translate_auth {
                self.translate_key = translateauth.to_string();
            }
        }

        if self.demux_cfg.auto_stream == CcxStreamMode::Mp4
            && self.input_source == CcxDatasource::Stdin
        {
            println!("MP4 requires an actual file, it's not possible to read from a stream, including stdin.");
            std::process::exit(ExitCode::IncompatibleParameters as i32);
        }

        if self.extract_chapters {
            println!("Request to extract chapters recieved.");
            println!("Note that this must only be used with MP4 files,");
            println!("for other files it will simply generate subtitles file.\n");
        }

        if self.gui_mode_reports {
            self.no_progress_bar = true;
            // Do it as soon as possible, because it something fails we might not have a chance
            self.activity_report_version();
        }

        if self.enc_cfg.sentence_cap {
            unsafe {
                CAPITALIZATION_LIST = get_vector_words(&CAPITALIZED_BUILTIN);
                if let Some(ref sentence_cap_file) = self.sentence_cap_file {
                    process_word_file(sentence_cap_file, &mut CAPITALIZATION_LIST)
                        .expect("There was an error processing the capitalization file.\n");
                }
            }
        }
        if self.enc_cfg.filter_profanity {
            unsafe {
                PROFANE = get_vector_words(&PROFANE_BUILTIN);
                if let Some(ref profanityfile) = self.filter_profanity_file {
                    process_word_file(profanityfile.as_str(), &mut PROFANE)
                        .expect("There was an error processing the profanity file.\n");
                }
            }
        }

        if self.demux_cfg.ts_forced_program == -1 {
            self.demux_cfg.ts_forced_program_selected = true;
        }

        // Init telexcc redundant options
        tlt_config.dolevdist = self.dolevdist;
        tlt_config.levdistmincnt = self.levdistmincnt.unwrap_or(0);
        tlt_config.levdistmaxpct = self.levdistmaxpct.unwrap_or(0);
        tlt_config.extraction_start = self.extraction_start.clone();
        tlt_config.extraction_end = self.extraction_end.clone();
        tlt_config.write_format = self.write_format;
        tlt_config.gui_mode_reports = self.gui_mode_reports;
        tlt_config.date_format = self.date;
        tlt_config.noautotimeref = self.noautotimeref;
        tlt_config.send_to_srv = self.send_to_srv;
        tlt_config.nofontcolor = self.nofontcolor;
        tlt_config.nohtmlescape = self.nohtmlescape;
        tlt_config.millis_separator = self.millis_separator;

        // teletext page number out of range
        if tlt_config.page != 0 && (tlt_config.page < 100 || tlt_config.page > 899) {
            println!("Teletext page number out of range (100-899)");
            std::process::exit(ExitCode::NotClassified as i32);
        }

        if self.num_input_files.unwrap_or(0) == 0 && self.input_source == CcxDatasource::File {
            std::process::exit(ExitCode::NoInputFiles as i32);
        }

        if self.num_input_files.is_some() && self.live_stream.is_some() {
            println!("Live stream mode only supports one input file");
            std::process::exit(ExitCode::TooManyInputFiles as i32);
        }

        if self.num_input_files.is_some() && self.input_source == CcxDatasource::Network {
            println!("UDP mode is not compatible with input files");
            std::process::exit(ExitCode::TooManyInputFiles as i32);
        }

        if self.input_source == CcxDatasource::Network || self.input_source == CcxDatasource::Tcp {
            // TODO(prateekmedia): Check why we use ccx_options instead of opts
            // currently using same]
            self.buffer_input = true;
        }

        if self.num_input_files.is_some() && self.input_source == CcxDatasource::Tcp {
            println!("TCP mode is not compatible with input files");
            std::process::exit(ExitCode::TooManyInputFiles as i32);
        }

        if self.demux_cfg.auto_stream == CcxStreamMode::McpoodlesRaw
            && self.write_format == CcxOutputFormat::Raw
        {
            println!("-in=raw can only be used if the output is a subtitle file.");
            std::process::exit(ExitCode::IncompatibleParameters as i32);
        }

        if self.demux_cfg.auto_stream == CcxStreamMode::Rcwt
            && self.write_format == CcxOutputFormat::Rcwt
            && self.output_filename.is_none()
        {
            println!("CCExtractor's binary format can only be used simultaneously for input and\noutput if the output file name is specified given with -o.\n");
            std::process::exit(ExitCode::IncompatibleParameters as i32);
        }

        if self.write_format != CcxOutputFormat::Dvdraw
            && self.cc_to_stdout
            && self.extract.is_some()
            && self.extract.unwrap() == OutputField::Both
        {
            println!(
                "You can't extract both fields to stdout at the same time in broadcast mode.\n",
            );
            std::process::exit(ExitCode::IncompatibleParameters as i32);
        }

        if self.write_format == CcxOutputFormat::Spupng && self.cc_to_stdout {
            println!("You cannot use -out=spupng with -stdout.\n");
            std::process::exit(ExitCode::IncompatibleParameters as i32);
        }

        if self.write_format == CcxOutputFormat::Webvtt
            && self.enc_cfg.encoding != CcxEncodingType::Utf8
        {
            self.enc_cfg.encoding = CcxEncodingType::Utf8;
            println!("Note: Output format is WebVTT, forcing UTF-8");
            std::process::exit(ExitCode::IncompatibleParameters as i32);
        }

        // Check WITH_LIBCURL
        #[cfg(feature = "with_libcurl")]
        {
            if self.write_format == CCX_OF_CURL && self.curlposturl.is_none() {
                Err("You must pass a URL (-curlposturl) if output format is curl\n")
            }
            if self.write_format != CCX_OF_CURL && self.curlposturl.is_some() {
                Err("-curlposturl requires that the format is curl\n")
            }
        }

        // Initialize some Encoder Configuration
        self.enc_cfg.extract = self.extract;
        if self.num_input_files.unwrap_or(0) > 0 {
            self.enc_cfg.multiple_files = true;
            self.enc_cfg.first_input_file = self.inputfile.as_ref().unwrap()[0].to_string();
        }
        self.enc_cfg.cc_to_stdout = self.cc_to_stdout;
        self.enc_cfg.write_format = self.write_format;
        self.enc_cfg.send_to_srv = self.send_to_srv;
        self.enc_cfg.date_format = self.date;
        self.enc_cfg.transcript_settings = self.transcript_settings.clone();
        self.enc_cfg.millis_separator = self.millis_separator;
        self.enc_cfg.no_font_color = self.nofontcolor;
        self.enc_cfg.force_flush = self.force_flush;
        self.enc_cfg.append_mode = self.append_mode;
        self.enc_cfg.ucla = self.ucla;
        self.enc_cfg.no_type_setting = self.notypesetting;
        self.enc_cfg.subs_delay = self.subs_delay;
        self.enc_cfg.gui_mode_reports = self.gui_mode_reports;

        if self.enc_cfg.render_font.is_empty() {
            self.enc_cfg.render_font = DEFAULT_FONT_PATH.to_string();
        }

        if self.enc_cfg.render_font_italics.is_empty() {
            self.enc_cfg.render_font = DEFAULT_FONT_PATH_ITALICS.to_string();
        }

        if self.output_filename.is_some() && self.multiprogram {
            self.enc_cfg.output_filename = self.output_filename.clone().unwrap();
        }

        if !self.is_608_enabled && !self.is_708_enabled {
            // If nothing is selected then extract both 608 and 708 subs
            // 608 field 1 is enabled by default
            // Enable 708 subs extraction
            self.parse_708_services("all");
        } else if !self.is_608_enabled && self.is_708_enabled {
            // Extract only 708 subs
            // 608 field 1 is enabled by default, disable it
            self.extract = None;
            self.enc_cfg.extract_only_708 = true;
        }

        // Check WITH_LIBCURL
        #[cfg(feature = "with_libcurl")]
        {
            self.enc_cfg.curlposturl = self.curlposturl.clone();
        }
    }
}
#[cfg(test)]
pub mod tests {
    use crate::{args::*, common::*, parser::*};
    use clap::Parser;

    #[no_mangle]
    pub extern "C" fn set_binary_mode() {}
    pub static mut MPEG_CLOCK_FREQ: u64 = 0;

    fn parse_args(args: &[&str]) -> (CcxOptions, CcxTeletextConfig) {
        let mut common_args = vec!["./ccextractor", "input_file"];
        common_args.extend_from_slice(args);
        let args = Args::try_parse_from(common_args).expect("Failed to parse arguments");
        let mut options = CcxOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxTeletextConfig {
            ..Default::default()
        };

        options.parse_parameters(&args, &mut tlt_config);
        (options, tlt_config)
    }

    #[test]
    fn broken_1() {
        let (options, _) = parse_args(&["--autoprogram", "--out", "srt", "--latin1"]);

        assert!(options.demux_cfg.ts_autoprogram);
        assert_eq!(options.write_format, CcxOutputFormat::Srt);
        assert_eq!(options.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn broken_2() {
        let (options, _) = parse_args(&["--autoprogram", "--out", "sami", "--latin1"]);

        assert!(options.demux_cfg.ts_autoprogram);
        assert_eq!(options.write_format, CcxOutputFormat::Sami);
        assert_eq!(options.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn broken_3() {
        let (options, _) = parse_args(&[
            "--autoprogram",
            "--out",
            "ttxt",
            "--latin1",
            "--ucla",
            "--xds",
        ]);

        assert!(options.demux_cfg.ts_autoprogram);
        assert_eq!(options.write_format, CcxOutputFormat::Transcript);
        assert_eq!(options.enc_cfg.encoding, CcxEncodingType::Latin1);
        assert!(options.ucla);
        assert!(options.transcript_settings.xds);
    }

    #[test]
    fn broken_4() {
        let (options, _) = parse_args(&["--out", "ttxt", "--latin1"]);
        assert_eq!(options.write_format, CcxOutputFormat::Transcript);

        assert_eq!(options.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn broken_5() {
        let (options, _) = parse_args(&["--out", "srt", "--latin1"]);
        assert_eq!(options.write_format, CcxOutputFormat::Srt);

        assert_eq!(options.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn cea708_1() {
        let (options, _) =
            parse_args(&["--service", "1", "--out", "txt", "--no-bom", "--no-rollup"]);
        assert!(options.is_708_enabled);

        assert!(options.enc_cfg.no_bom);
        assert!(options.no_rollup);
        assert_eq!(options.write_format, CcxOutputFormat::Transcript);
    }

    #[test]
    fn cea708_2() {
        let (options, _) = parse_args(&[
            "--service",
            "1,2[UTF-8],3[EUC-KR],54",
            "--out",
            "txt",
            "--no-rollup",
        ]);

        assert!(options.is_708_enabled);
        assert!(options.no_rollup);
        assert_eq!(options.write_format, CcxOutputFormat::Transcript);
        assert_eq!(options.enc_cfg.services_charsets[1], "UTF-8");
        assert_eq!(options.enc_cfg.services_charsets[2], "EUC-KR");
    }

    #[test]
    fn cea708_3() {
        let (options, _) = parse_args(&["--service", "all[EUC-KR]", "--no-rollup"]);
        assert!(options.is_708_enabled);

        assert!(options.no_rollup);
        assert_eq!(options.enc_cfg.all_services_charset, "EUC-KR");
    }

    #[test]
    fn dvb_1() {
        let (options, _) = parse_args(&[
            "--autoprogram",
            "--out",
            "srt",
            "--latin1",
            "--teletext",
            "--datapid",
            "5603",
        ]);

        assert!(options.demux_cfg.ts_autoprogram);
        assert_eq!(options.demux_cfg.ts_cappids[0], 5603);
        assert_eq!(options.demux_cfg.nb_ts_cappid, 1);
        assert_eq!(options.write_format, CcxOutputFormat::Srt);
        assert_eq!(options.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn dvb_2() {
        let (options, _) = parse_args(&["--stdout", "--quiet", "--no-fontcolor"]);
        assert!(options.cc_to_stdout);

        assert_eq!(options.messages_target.unwrap(), 0);
        assert_eq!(options.nofontcolor, true);
    }

    #[test]
    fn dvd_1() {
        let (options, _) = parse_args(&["--autoprogram", "--out", "ttxt", "--latin1"]);
        assert!(options.demux_cfg.ts_autoprogram);

        assert_eq!(options.write_format, CcxOutputFormat::Transcript);
        assert_eq!(options.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn dvr_ms_1() {
        let (options, _) = parse_args(&[
            "--wtvconvertfix",
            "--autoprogram",
            "--out",
            "srt",
            "--latin1",
        ]);

        assert!(options.wtvconvertfix);
        assert!(options.demux_cfg.ts_autoprogram);
        assert_eq!(options.write_format, CcxOutputFormat::Srt);
        assert_eq!(options.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn general_1() {
        let (options, _) = parse_args(&[
            "--ucla",
            "--autoprogram",
            "--out",
            "ttxt",
            "--latin1",
            "--output-field",
            "field2",
        ]);

        assert!(options.ucla);
        assert!(options.demux_cfg.ts_autoprogram);
        assert!(options.is_608_enabled);
        assert_eq!(options.write_format, CcxOutputFormat::Transcript);
        assert_eq!(options.extract.unwrap(), OutputField::Field2);
        assert_eq!(options.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn general_2() {
        let (options, _) =
            parse_args(&["--autoprogram", "--out", "bin", "--latin1", "--sentencecap"]);
        assert!(options.demux_cfg.ts_autoprogram);

        assert!(options.enc_cfg.sentence_cap);
        assert_eq!(options.write_format, CcxOutputFormat::Rcwt);
        assert_eq!(options.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn haup_1() {
        let (options, _) = parse_args(&[
            "--hauppauge",
            "--ucla",
            "--autoprogram",
            "--out",
            "ttxt",
            "--latin1",
        ]);

        assert!(options.ucla);
        assert!(options.hauppauge_mode);
        assert!(options.demux_cfg.ts_autoprogram);
        assert_eq!(options.write_format, CcxOutputFormat::Transcript);
        assert_eq!(options.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn mp4_1() {
        let (options, _) = parse_args(&["--input", "mp4", "--out", "srt", "--latin1"]);
        assert_eq!(options.demux_cfg.auto_stream, CcxStreamMode::Mp4);

        assert_eq!(options.write_format, CcxOutputFormat::Srt);
        assert_eq!(options.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn mp4_2() {
        let (options, _) = parse_args(&["--autoprogram", "--out", "srt", "--bom", "--latin1"]);
        assert!(options.demux_cfg.ts_autoprogram);

        assert!(!options.enc_cfg.no_bom);
        assert_eq!(options.write_format, CcxOutputFormat::Srt);
        assert_eq!(options.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn nocc_1() {
        let (options, _) = parse_args(&[
            "--autoprogram",
            "--out",
            "ttxt",
            "--mp4vidtrack",
            "--latin1",
        ]);

        assert!(options.demux_cfg.ts_autoprogram);
        assert!(options.mp4vidtrack);
        assert_eq!(options.write_format, CcxOutputFormat::Transcript);
        assert_eq!(options.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn nocc_2() {
        let (options, _) = parse_args(&[
            "--autoprogram",
            "--out",
            "ttxt",
            "--latin1",
            "--ucla",
            "--xds",
        ]);

        assert!(options.demux_cfg.ts_autoprogram);
        assert!(options.ucla);
        assert!(options.transcript_settings.xds);
        assert_eq!(options.write_format, CcxOutputFormat::Transcript);
        assert_eq!(options.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn options_1() {
        let (options, _) = parse_args(&["--input", "ts"]);

        assert_eq!(options.demux_cfg.auto_stream, CcxStreamMode::Transport);
    }

    #[test]
    fn options_2() {
        let (options, _) = parse_args(&["--out", "dvdraw"]);
        assert_eq!(options.write_format, CcxOutputFormat::Dvdraw);
    }

    #[test]
    fn options_3() {
        let (options, _) = parse_args(&["--goptime"]);
        assert_eq!(options.use_gop_as_pts, 1);
    }

    #[test]
    fn options_4() {
        let (options, _) = parse_args(&["--no-goptime"]);
        assert_eq!(options.use_gop_as_pts, -1);
    }

    #[test]
    fn options_5() {
        let (options, _) = parse_args(&["--fixpadding"]);
        assert!(options.fix_padding);
    }

    #[test]
    fn options_6() {
        let (_, _) = parse_args(&["--90090"]);

        unsafe {
            assert_eq!(MPEG_CLOCK_FREQ as i64, 90090);
        }
    }

    #[test]
    fn options_7() {
        let (options, _) = parse_args(&["--myth"]);
        assert_eq!(options.auto_myth.unwrap(), 1);
    }

    #[test]
    fn options_8() {
        let (options, _) = parse_args(&["--program-number", "1"]);
        assert_eq!(options.demux_cfg.ts_forced_program, 1);
    }

    #[test]
    fn options_9() {
        let (options, _) = parse_args(&[
            "--datastreamtype",
            "2",
            "--streamtype",
            "2",
            "--no-autotimeref",
        ]);

        assert!(options.noautotimeref);
        assert_eq!(options.demux_cfg.ts_datastreamtype, 2);
        assert_eq!(options.demux_cfg.ts_forced_streamtype, 2);
    }
    #[test]
    fn options_10() {
        let (options, _) = parse_args(&["--unicode", "--no-typesetting"]);
        assert!(options.notypesetting);

        assert_eq!(options.enc_cfg.encoding, CcxEncodingType::Unicode);
    }

    #[test]
    fn options_11() {
        let (options, _) = parse_args(&["--utf8", "--trim"]);
        assert!(options.enc_cfg.trim_subs);

        assert_eq!(options.enc_cfg.encoding, CcxEncodingType::Utf8);
    }

    #[test]
    fn options_12() {
        let (options, _) = parse_args(&["--capfile", "Cargo.toml"]);

        assert!(options.enc_cfg.sentence_cap);
        assert_eq!(options.sentence_cap_file.unwrap(), "Cargo.toml");
    }

    #[test]
    fn options_13() {
        let (options, _) = parse_args(&["--unixts", "5", "--out", "txt"]);

        unsafe {
            assert_eq!(UTC_REFVALUE, 5);
        }

        assert_eq!(options.write_format, CcxOutputFormat::Transcript);
    }

    #[test]
    fn options_14() {
        let (options, _) = parse_args(&["--datets", "--out", "txt"]);

        assert_eq!(options.date, CcxOutputDateFormat::Date);
        assert_eq!(options.write_format, CcxOutputFormat::Transcript);
    }

    #[test]
    fn options_15() {
        let (options, _) = parse_args(&["--sects", "--out", "txt"]);

        assert_eq!(options.date, CcxOutputDateFormat::Seconds);
        assert_eq!(options.write_format, CcxOutputFormat::Transcript);
    }

    #[test]
    fn options_16() {
        let (options, _) = parse_args(&["--lf", "--out", "txt"]);

        assert!(options.enc_cfg.line_terminator_lf);
        assert_eq!(options.write_format, CcxOutputFormat::Transcript);
    }

    #[test]
    fn options_17() {
        let (options, _) = parse_args(&["--autodash", "--trim"]);

        assert!(options.enc_cfg.autodash);
        assert!(options.enc_cfg.trim_subs);
    }

    #[test]
    fn options_18() {
        let (options, _) = parse_args(&["--bufferinput"]);

        assert!(options.buffer_input);
    }

    #[test]
    fn options_19() {
        let (options, _) = parse_args(&["--no-bufferinput"]);

        assert!(!options.buffer_input);
    }

    #[test]
    fn options_20() {
        let (_, _) = parse_args(&["--buffersize", "1M"]);

        unsafe {
            assert_eq!(FILEBUFFERSIZE, 1024 * 1024);
        }
    }

    #[test]
    fn options_21() {
        let (options, _) = parse_args(&["--dru"]);

        assert_eq!(options.settings_608.direct_rollup, 1);
    }

    #[test]
    fn options_22() {
        let (options, _) = parse_args(&["--no-rollup"]);

        assert!(options.no_rollup);
    }

    #[test]
    fn options_23() {
        let (options, _) = parse_args(&["--rollup", "ru1"]);

        assert_eq!(options.settings_608.force_rollup, 1);
    }

    #[test]
    fn options_24() {
        let (options, _) = parse_args(&["--delay", "200"]);

        assert_eq!(options.subs_delay, 200);
    }

    #[test]
    fn options_25() {
        let (options, _) = parse_args(&["--startat", "4", "--endat", "7"]);

        assert_eq!(options.extraction_start.ss, 4);
    }

    #[test]
    fn options_26() {
        let (options, _) = parse_args(&["--no-codec", "dvbsub"]);

        assert_eq!(options.demux_cfg.nocodec, CcxCodeType::Dvb);
    }

    #[test]
    fn options_27() {
        let (options, _) = parse_args(&["--debug"]);

        assert_eq!(options.debug_mask, CcxDebugMessageTypes::Verbose);
    }

    #[test]
    fn options_28() {
        let (options, _) = parse_args(&["--608"]);

        assert_eq!(options.debug_mask, CcxDebugMessageTypes::Decoder608);
    }

    #[test]
    fn options_29() {
        let (options, _) = parse_args(&["--708"]);

        assert_eq!(options.debug_mask, CcxDebugMessageTypes::Parse);
    }

    #[test]
    fn options_30() {
        let (options, _) = parse_args(&["--goppts"]);

        assert_eq!(options.debug_mask, CcxDebugMessageTypes::Time);
    }

    #[test]
    fn options_31() {
        let (options, _) = parse_args(&["--xdsdebug"]);

        assert_eq!(options.debug_mask, CcxDebugMessageTypes::DecoderXds);
    }

    #[test]
    fn options_32() {
        let (options, _) = parse_args(&["--vides"]);

        assert_eq!(options.debug_mask, CcxDebugMessageTypes::Vides);
    }

    #[test]
    fn options_33() {
        let (options, _) = parse_args(&["--cbraw"]);

        assert_eq!(options.debug_mask, CcxDebugMessageTypes::Cbraw);
    }

    #[test]
    fn options_34() {
        let (options, _) = parse_args(&["--no-sync"]);

        assert!(options.nosync);
    }

    #[test]
    fn options_35() {
        let (options, _) = parse_args(&["--fullbin"]);

        assert!(options.fullbin);
    }

    #[test]
    fn options_36() {
        let (options, _) = parse_args(&["--parsedebug"]);

        assert_eq!(options.debug_mask, CcxDebugMessageTypes::Parse);
    }

    #[test]
    fn options_37() {
        let (options, _) = parse_args(&["--parsePAT"]);

        assert_eq!(options.debug_mask, CcxDebugMessageTypes::Pat);
    }

    #[test]
    fn options_38() {
        let (options, _) = parse_args(&["--parsePMT"]);

        assert_eq!(options.debug_mask, CcxDebugMessageTypes::Pmt);
    }

    #[test]
    fn options_39() {
        let (options, _) = parse_args(&["--investigate-packets"]);

        assert!(options.investigate_packets);
    }

    #[test]
    fn options_40() {
        let (options, _) = parse_args(&["--mp4vidtrack"]);

        assert!(options.mp4vidtrack);
    }

    #[test]
    fn options_41() {
        let (options, _) = parse_args(&["--wtvmpeg2"]);

        assert!(options.wtvmpeg2);
    }

    #[test]
    fn options_42() {
        let (options, _) = parse_args(&["--hauppauge"]);

        assert!(options.hauppauge_mode);
    }

    #[test]
    fn options_43() {
        let (options, _) = parse_args(&["--xmltv", "1", "--out", "null"]);

        assert_eq!(options.xmltv.unwrap(), 1);
        assert_eq!(options.write_format, CcxOutputFormat::Null);
    }

    #[test]
    fn options_44() {
        let (options, _) = parse_args(&["--codec", "dvbsub", "--out", "spupng"]);

        assert_eq!(options.demux_cfg.codec, CcxCodeType::Dvb);
        assert_eq!(options.write_format, CcxOutputFormat::Spupng);
    }

    #[test]
    fn options_45() {
        let (options, _) = parse_args(&[
            "--startcreditsnotbefore",
            "1",
            "--startcreditstext",
            "CCextractor Start credit Testing",
        ]);

        assert_eq!(options.enc_cfg.startcreditsnotbefore.ss, 1);
        assert_eq!(
            options.enc_cfg.start_credits_text,
            "CCextractor Start credit Testing"
        );
    }

    #[test]
    fn options_46() {
        let (options, _) = parse_args(&[
            "--startcreditsnotafter",
            "2",
            "--startcreditstext",
            "CCextractor Start credit Testing",
        ]);

        assert_eq!(options.enc_cfg.startcreditsnotafter.ss, 2);
        assert_eq!(
            options.enc_cfg.start_credits_text,
            "CCextractor Start credit Testing"
        );
    }

    #[test]
    fn options_47() {
        let (options, _) = parse_args(&[
            "--startcreditsforatleast",
            "1",
            "--startcreditstext",
            "CCextractor Start credit Testing",
        ]);

        assert_eq!(options.enc_cfg.startcreditsforatleast.ss, 1);
        assert_eq!(
            options.enc_cfg.start_credits_text,
            "CCextractor Start credit Testing"
        );
    }

    #[test]
    fn options_48() {
        let (options, _) = parse_args(&[
            "--startcreditsforatmost",
            "2",
            "--startcreditstext",
            "CCextractor Start credit Testing",
        ]);

        assert_eq!(options.enc_cfg.startcreditsforatmost.ss, 2);
        assert_eq!(
            options.enc_cfg.start_credits_text,
            "CCextractor Start credit Testing"
        );
    }

    #[test]
    fn options_49() {
        let (options, _) = parse_args(&[
            "--endcreditsforatleast",
            "3",
            "--endcreditstext",
            "CCextractor Start credit Testing",
        ]);

        assert_eq!(options.enc_cfg.endcreditsforatleast.ss, 3);
        assert_eq!(
            options.enc_cfg.end_credits_text,
            "CCextractor Start credit Testing"
        );
    }

    #[test]
    fn options_50() {
        let (options, _) = parse_args(&[
            "--endcreditsforatmost",
            "2",
            "--endcreditstext",
            "CCextractor Start credit Testing",
        ]);

        assert_eq!(options.enc_cfg.endcreditsforatmost.ss, 2);
        assert_eq!(
            options.enc_cfg.end_credits_text,
            "CCextractor Start credit Testing"
        );
    }

    #[test]
    fn options_51() {
        let (options, tlt_config) = parse_args(&["--tverbose"]);

        assert!(tlt_config.verbose);
        assert_eq!(options.debug_mask, CcxDebugMessageTypes::Teletext);
    }

    #[test]
    fn teletext_1() {
        let (options, _) = parse_args(&[
            "--autoprogram",
            "--out",
            "srt",
            "--latin1",
            "--datapid",
            "2310",
        ]);

        assert!(options.demux_cfg.ts_autoprogram);
        assert_eq!(options.demux_cfg.ts_cappids[0], 2310);
        assert_eq!(options.demux_cfg.nb_ts_cappid, 1);
        assert_eq!(options.write_format, CcxOutputFormat::Srt);
        assert_eq!(options.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn teletext_2() {
        let (options, tlt_config) = parse_args(&[
            "--autoprogram",
            "--out",
            "srt",
            "--latin1",
            "--teletext",
            "--tpage",
            "398",
        ]);

        assert!(options.demux_cfg.ts_autoprogram);
        assert_eq!(options.demux_cfg.codec, CcxCodeType::Teletext);
        assert_eq!(tlt_config.page, 398);
        assert_eq!(options.write_format, CcxOutputFormat::Srt);
        assert_eq!(options.enc_cfg.encoding, CcxEncodingType::Latin1);
    }
}
