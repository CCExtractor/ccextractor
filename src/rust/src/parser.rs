use args::{Args, OutFormat};
use lib_ccxr::activity::ActivityExt;
use lib_ccxr::teletext::{TeletextConfig, TeletextPageNumber, UTC_REFVALUE};
use lib_ccxr::time::units::{Timestamp, TimestampFormat};
use lib_ccxr::util::encoders_helper::{add_builtin_capitalization, add_builtin_profane};
use lib_ccxr::util::encoding::Encoding;
use lib_ccxr::util::log::{DebugMessageFlag, DebugMessageMask, ExitCause, OutputTarget};
use lib_ccxr::util::time::stringztoms;
use num_integer::Integer;
use std::cell::Cell;
use std::convert::TryInto;
use std::fs::File;
use std::io::{prelude::*, BufReader};
use std::path::PathBuf;
use std::str::FromStr;
use std::string::String;

use lib_ccxr::{common::*, fatal};

use cfg_if::cfg_if;

use time::OffsetDateTime;

use crate::args::CCXCodec;
use crate::args::{self, InFormat};

use crate::{set_binary_mode, usercolor_rgb, FILEBUFFERSIZE, MPEG_CLOCK_FREQ};

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

fn set_usercolor_rgb(color: &str) {
    let mut rgb: [i32; 8] = [0; 8];
    for (i, item) in color.chars().enumerate() {
        rgb[i] = item as i32;
    }
    rgb[7] = 0;
    unsafe {
        usercolor_rgb = rgb;
    }
}

fn set_mpeg_clock_freq(freq: i32) {
    unsafe {
        MPEG_CLOCK_FREQ = freq as _;
    }
}

fn atol(bufsize: &str) -> i32 {
    let s = bufsize.trim();
    if s.is_empty() {
        return 0;
    }

    let last_char = s.chars().last().unwrap();
    if last_char.is_ascii_digit() {
        return s.parse::<i32>().unwrap_or(0);
    }

    let val_str = &s[..s.len() - 1];
    let mut val = val_str.parse::<i32>().unwrap_or(0);

    match last_char.to_ascii_uppercase() {
        'M' => val *= 1024 * 1024,
        'K' => val *= 1024,
        'G' => val *= 1024 * 1024 * 1024,
        _ => {}
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
            fatal!(
                cause = ExitCause::MalformedParameter;
                "Malformed parameter: {}",s
            );
        }
    }
}

fn process_word_file(filename: &str, list: &mut Vec<String>) -> Result<(), std::io::Error> {
    let file = File::open(filename)?;
    let reader = BufReader::new(file);
    let mut num = 0;

    for line in reader.lines() {
        num += 1;
        let line = line?;
        if line.starts_with('#') {
            continue;
        }

        let new_len = line.trim().len();
        if new_len > CCX_DECODER_608_SCREEN_WIDTH {
            println!(
                "Word in line {num} too long, max = {CCX_DECODER_608_SCREEN_WIDTH} characters."
            );
            continue;
        }

        if new_len > 0 {
            list.push(line.trim().to_string());
        }
    }
    Ok(())
}

fn get_file_buffer_size() -> i32 {
    unsafe { FILEBUFFERSIZE }
}

fn set_file_buffer_size(size: i32) {
    unsafe {
        FILEBUFFERSIZE = size;
    }
}

pub trait OptionsExt {
    fn set_output_format_type(&mut self, out_format: OutFormat);
    fn set_output_format(&mut self, args: &Args);
    fn set_input_format_type(&mut self, input_format: InFormat);
    fn set_input_format(&mut self, args: &Args);
    fn parse_708_services(&mut self, s: &str);
    fn append_file_to_queue(&mut self, filename: &str, inputfile_capacity: &mut i32) -> i32;
    fn add_file_sequence(&mut self, filename: &mut String, inputfile_capacity: &mut i32) -> i32;
    fn parse_parameters(
        &mut self,
        args: &Args,
        tlt_config: &mut TeletextConfig,
        capitalization_list: &mut Vec<String>,
        profane: &mut Vec<String>,
    );
    fn is_inputfile_empty(&self) -> bool;
}

impl OptionsExt for Options {
    fn is_inputfile_empty(&self) -> bool {
        self.inputfile.is_none() || self.inputfile.as_ref().is_some_and(|v| v.is_empty())
    }

    fn set_output_format_type(&mut self, out_format: OutFormat) {
        match out_format {
            #[cfg(feature = "with_libcurl")]
            OutFormat::Curl => self.write_format = OutputFormat::Curl,
            OutFormat::Ass => self.write_format = OutputFormat::Ssa,
            OutFormat::Ccd => self.write_format = OutputFormat::Ccd,
            OutFormat::Scc => self.write_format = OutputFormat::Scc,
            OutFormat::Srt => self.write_format = OutputFormat::Srt,
            OutFormat::Ssa => self.write_format = OutputFormat::Ssa,
            OutFormat::Webvtt => self.write_format = OutputFormat::WebVtt,
            OutFormat::WebvttFull => {
                self.write_format = OutputFormat::WebVtt;
                self.use_webvtt_styling = true;
            }
            OutFormat::Sami => self.write_format = OutputFormat::Sami,
            OutFormat::Txt => {
                self.write_format = OutputFormat::Transcript;
                self.settings_dtvcc.no_rollup = true;
            }
            OutFormat::Ttxt => {
                self.write_format = OutputFormat::Transcript;
                if self.date_format == TimestampFormat::None {
                    self.date_format = TimestampFormat::HHMMSSFFF;
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
                self.write_format = OutputFormat::Null;
                self.messages_target = OutputTarget::Quiet;
                self.print_file_reports = true;
                self.demux_cfg.ts_allprogram = true;
            }
            OutFormat::Raw => self.write_format = OutputFormat::Raw,
            OutFormat::Smptett => self.write_format = OutputFormat::SmpteTt,
            OutFormat::Bin => self.write_format = OutputFormat::Rcwt,
            OutFormat::Null => self.write_format = OutputFormat::Null,
            OutFormat::Dvdraw => self.write_format = OutputFormat::DvdRaw,
            OutFormat::Spupng => self.write_format = OutputFormat::SpuPng,
            OutFormat::SimpleXml => self.write_format = OutputFormat::SimpleXml,
            OutFormat::G608 => self.write_format = OutputFormat::G608,
            OutFormat::Mcc => self.write_format = OutputFormat::Mcc,
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
            InFormat::Hex => self.demux_cfg.auto_stream = StreamMode::HexDump,
            InFormat::Es => self.demux_cfg.auto_stream = StreamMode::ElementaryOrNotFound,
            InFormat::Ts => self.demux_cfg.auto_stream = StreamMode::Transport,
            InFormat::M2ts => self.demux_cfg.auto_stream = StreamMode::Transport,
            InFormat::Ps => self.demux_cfg.auto_stream = StreamMode::Program,
            InFormat::Asf => self.demux_cfg.auto_stream = StreamMode::Asf,
            InFormat::Wtv => self.demux_cfg.auto_stream = StreamMode::Wtv,
            InFormat::Raw => self.demux_cfg.auto_stream = StreamMode::McpoodlesRaw,
            InFormat::Bin => self.demux_cfg.auto_stream = StreamMode::Rcwt,
            InFormat::Mp4 => self.demux_cfg.auto_stream = StreamMode::Mp4,
            InFormat::Mkv => self.demux_cfg.auto_stream = StreamMode::Mkv,
            InFormat::Mxf => self.demux_cfg.auto_stream = StreamMode::Mxf,
            InFormat::Scc => self.demux_cfg.auto_stream = StreamMode::Scc,
        }
    }

    fn set_input_format(&mut self, args: &Args) {
        if self.input_source == DataSource::Tcp {
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
            fatal!(
                cause = ExitCause::MalformedParameter;
               "Unknown input file format: {}\n", args.input.unwrap()
            );
        }
    }

    fn parse_708_services(&mut self, s: &str) {
        if s.starts_with("all") {
            let charset = if s.len() > 3 { &s[4..s.len() - 1] } else { "" };
            self.settings_dtvcc.enabled = true;
            self.enc_cfg.dtvcc_extract = true;

            if charset.is_empty() {
                self.enc_cfg.services_charsets = DtvccServiceCharset::Unique(
                    vec![String::new(); DTVCC_MAX_SERVICES]
                        .into_boxed_slice()
                        .try_into()
                        .unwrap(),
                );
            } else {
                self.enc_cfg.services_charsets = DtvccServiceCharset::Same(charset.to_string());
            }
            for i in 0..DTVCC_MAX_SERVICES {
                self.settings_dtvcc.services_enabled[i] = true;
                self.enc_cfg.services_enabled[i] = true;
            }

            self.settings_dtvcc.active_services_count = DTVCC_MAX_SERVICES as _;
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

        const ARRAY_REPEAT_VALUE: std::string::String = String::new();
        self.enc_cfg.services_charsets =
            DtvccServiceCharset::Unique(Box::new([ARRAY_REPEAT_VALUE; DTVCC_MAX_SERVICES]));

        for (i, service) in services.iter().enumerate() {
            let svc = service.parse::<usize>().unwrap();
            if !(1..=DTVCC_MAX_SERVICES).contains(&svc) {
                fatal!(
                    cause = ExitCause::MalformedParameter;
                  "[CEA-708] Malformed parameter: Invalid service number ({}), valid range is 1-{}.\n", svc, DTVCC_MAX_SERVICES
                );
            }
            self.settings_dtvcc.services_enabled[svc - 1] = true;
            self.enc_cfg.services_enabled[svc - 1] = true;
            self.settings_dtvcc.enabled = true;
            self.enc_cfg.dtvcc_extract = true;
            self.settings_dtvcc.active_services_count += 1;

            if charsets.len() > i && charsets[i].is_some() {
                if let DtvccServiceCharset::Unique(unique) = &mut self.enc_cfg.services_charsets {
                    unique[svc - 1].clone_from(charsets[i].as_ref().unwrap());
                }
            }
        }

        if self.settings_dtvcc.active_services_count == 0 {
            fatal!(
                cause = ExitCause::MalformedParameter;
               "[CEA-708] Malformed parameter: no services\n"
            );
        }
    }

    fn append_file_to_queue(&mut self, filename: &str, _inputfile_capacity: &mut i32) -> i32 {
        if filename.is_empty() {
            return 0;
        }

        if self.inputfile.is_none() {
            self.inputfile = Some(Vec::new());
        }

        if let Some(ref mut inputfile) = self.inputfile {
            inputfile.push(filename.to_string());
        }

        0
    }

    // Used for adding a sequence of files that are numbered
    // Ex: filename: video1.mp4 will search for video2.mp4, video3.mp4, ...
    fn add_file_sequence(&mut self, filename: &mut String, inputfile_capacity: &mut i32) -> i32 {
        filename.pop();
        let mut n: i32 = filename.len() as i32 - 1;
        let bytes = filename.as_bytes();

        // Look for the last digit in filename
        while n >= 0 && !bytes[n as usize].is_ascii_digit() {
            n -= 1;
        }
        if n == -1 {
            // None. No expansion needed
            return self.append_file_to_queue(filename, inputfile_capacity);
        }

        let mut m: i32 = n;
        while m >= 0 && bytes[m as usize].is_ascii_digit() {
            m -= 1;
        }
        m += 1;

        // Here: Significant digits go from filename[m] to filename[n]
        let num = &filename[(m as usize)..=(n as usize)];
        let mut i = num.parse::<i32>().unwrap();

        let mut temp;
        let mut filename = filename.to_string();

        loop {
            if std::path::Path::new(&filename).exists() {
                if self.append_file_to_queue(filename.as_str(), inputfile_capacity) != 0 {
                    return -1;
                }
                temp = format!("{}", i + 1);
                let temp_len = temp.len();
                let num_len = num.len();
                if temp_len > num_len {
                    break;
                }
                filename.replace_range(
                    (m as usize + num_len - temp_len)..(m as usize + num_len),
                    &temp,
                );
                filename.replace_range(
                    (m as usize)..(m as usize + num_len - temp_len),
                    &"0".repeat(num_len - temp_len),
                );
            } else {
                break;
            }
            i += 1;
        }

        0
    }

    fn parse_parameters(
        &mut self,
        args: &Args,
        tlt_config: &mut TeletextConfig,
        capitalization_list: &mut Vec<String>,
        profane: &mut Vec<String>,
    ) {
        if args.stdin {
            unsafe {
                set_binary_mode();
            }
            self.input_source = DataSource::Stdin;
            self.live_stream = None;
        }
        let mut inputfile_capacity = 0;
        if let Some(ref files) = args.inputfile {
            for inputfile in files {
                let plus_sign = '+';

                let rc: i32 = if !inputfile.ends_with(plus_sign) {
                    self.append_file_to_queue(inputfile, &mut inputfile_capacity)
                } else {
                    self.add_file_sequence(&mut inputfile.clone(), &mut inputfile_capacity)
                };

                if rc < 0 {
                    fatal!(
                        cause = ExitCause::NotEnoughMemory;
                       "Fatal: Not enough memory to parse parameters.\n"
                    );
                }
            }
        }

        #[cfg(feature = "hardsubx_ocr")]
        {
            use lib_ccxr::hardsubx::*;
            if args.hardsubx {
                self.hardsubx = true;

                if args.hcc {
                    self.hardsubx_and_common = true;
                }

                if let Some(ref ocr_mode) = args.ocr_mode {
                    let ocr_mode = match ocr_mode.as_str() {
                        "simple" | "frame" => Some(OcrMode::Frame),
                        "word" => Some(OcrMode::Word),
                        "letter" | "symbol" => Some(OcrMode::Letter),
                        _ => None,
                    };

                    if ocr_mode.is_none() {
                        fatal!(
                            cause = ExitCause::MalformedParameter;
                           "Invalid OCR mode"
                        );
                    }

                    self.hardsubx_ocr_mode = ocr_mode.unwrap_or_default();
                }

                if let Some(ref subcolor) = args.subcolor {
                    match subcolor.as_str() {
                        "white" => {
                            self.hardsubx_hue = ColorHue::White;
                        }
                        "yellow" => {
                            self.hardsubx_hue = ColorHue::Yellow;
                        }
                        "green" => {
                            self.hardsubx_hue = ColorHue::Green;
                        }
                        "cyan" => {
                            self.hardsubx_hue = ColorHue::Cyan;
                        }
                        "blue" => {
                            self.hardsubx_hue = ColorHue::Blue;
                        }
                        "magenta" => {
                            self.hardsubx_hue = ColorHue::Magenta;
                        }
                        "red" => {
                            self.hardsubx_hue = ColorHue::Red;
                        }
                        _ => {
                            let result = subcolor.parse::<f64>();
                            if result.is_err() {
                                fatal!(
                                    cause = ExitCause::MalformedParameter;
                                   "Invalid Hue value"
                                );
                            }

                            let hue: f64 = result.unwrap();

                            if hue <= 0.0 || hue > 360.0 {
                                fatal!(
                                    cause = ExitCause::MalformedParameter;
                                   "Invalid Hue value"
                                );
                            }
                            self.hardsubx_hue = ColorHue::Custom(hue);
                        }
                    }
                }

                if let Some(ref value) = args.min_sub_duration {
                    if *value == 0.0 {
                        fatal!(
                            cause = ExitCause::MalformedParameter;
                           "Invalid minimum subtitle duration"
                        );
                    }
                    self.hardsubx_min_sub_duration = Timestamp::from_millis((1000.0 * *value) as _);
                }

                if args.detect_italics {
                    self.hardsubx_detect_italics = true;
                }

                if let Some(ref value) = args.conf_thresh {
                    if !(0.0..=100.0).contains(value) {
                        fatal!(
                            cause = ExitCause::MalformedParameter;
                           "Invalid confidence threshold, valid values are between 0 & 100"
                        );
                    }
                    self.hardsubx_conf_thresh = *value as _;
                }

                if let Some(ref value) = args.whiteness_thresh {
                    if !(0.0..=100.0).contains(value) {
                        fatal!(
                            cause = ExitCause::MalformedParameter;
                           "Invalid whiteness threshold, valid values are between 0 & 100"
                        );
                    }
                    self.hardsubx_lum_thresh = *value as _;
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
            set_file_buffer_size(atol(buffersize));

            if get_file_buffer_size() < 8 {
                set_file_buffer_size(8); // Otherwise crashes are guaranteed at least in MythTV
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
                CCXCodec::Teletext => {
                    self.demux_cfg.codec = SelectCodec::Some(Codec::Teletext);
                }
                CCXCodec::Dvbsub => {
                    self.demux_cfg.codec = SelectCodec::Some(Codec::Dvb);
                }
            }
        }

        if let Some(ref codec) = args.no_codec {
            match codec {
                CCXCodec::Dvbsub => {
                    self.demux_cfg.nocodec = SelectCodec::Some(Codec::Dvb);
                }
                CCXCodec::Teletext => {
                    self.demux_cfg.nocodec = SelectCodec::Some(Codec::Teletext);
                }
            }
        }

        if let Some(ref lang) = args.dvblang {
            self.dvblang = Some(Language::from_str(lang.as_str()).unwrap_or_else(|_| {
                fatal!(
                    cause = ExitCause::MalformedParameter;
                    "Invalid dvblang value '{}'. Use a 3-letter ISO 639-2 language code (e.g., 'chi', 'eng', 'chs').",
                    lang
                );
            }));
        }

        if let Some(ref ocrlang) = args.ocrlang {
            // Accept Tesseract language names directly (e.g., "chi_tra", "chi_sim", "eng")
            self.ocrlang = Some(ocrlang.clone());
        }

        // Handle --split-dvb-subs flag
        if args.split_dvb_subs {
            self.split_dvb_subs = true;
        }

        // Handle --no-dvb-dedup flag
        if args.no_dvb_dedup {
            self.no_dvb_dedup = true;
        }

        if let Some(ref quant) = args.quant {
            if !(0..=2).contains(quant) {
                fatal!(
                    cause = ExitCause::MalformedParameter;
                   "Invalid quant value"
                );
            }
            self.ocr_quantmode = *quant;
        }

        if args.no_spupngocr {
            self.enc_cfg.nospupngocr = true;
        }

        if let Some(ref oem) = args.oem {
            if !(0..=2).contains(oem) {
                fatal!(
                    cause = ExitCause::MalformedParameter;
                   "oem value should be between 0 and 2"
                );
            }
            self.ocr_oem = *oem as _;
        }

        if let Some(ref psm) = args.psm {
            if !(0..=13).contains(psm) {
                fatal!(
                    cause = ExitCause::MalformedParameter;
                   "--psm must be between 0 and 13"
                );
            }
            self.psm = *psm as _;
        }

        if args.ocr_line_split {
            self.ocr_line_split = true;
        }

        if args.no_ocr_blacklist {
            self.ocr_blacklist = false;
        }

        if let Some(ref lang) = args.mkvlang {
            match MkvLangFilter::new(lang.as_str()) {
                Ok(filter) => self.mkvlang = Some(filter),
                Err(e) => fatal!(
                    cause = ExitCause::MalformedParameter;
                    "{}\n", e
                ),
            }
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
            self.enc_cfg.start_credits_text.clone_from(startcreditstext);
        }

        if let Some(ref startcreditsnotbefore) = args.startcreditsnotbefore {
            self.enc_cfg.startcreditsnotbefore =
                stringztoms(startcreditsnotbefore.clone().as_str()).unwrap();
        }

        if let Some(ref startcreditsnotafter) = args.startcreditsnotafter {
            self.enc_cfg.startcreditsnotafter =
                stringztoms(startcreditsnotafter.clone().as_str()).unwrap();
        }

        if let Some(ref startcreditsforatleast) = args.startcreditsforatleast {
            self.enc_cfg.startcreditsforatleast =
                stringztoms(startcreditsforatleast.clone().as_str()).unwrap();
        }
        if let Some(ref startcreditsforatmost) = args.startcreditsforatmost {
            self.enc_cfg.startcreditsforatmost =
                stringztoms(startcreditsforatmost.clone().as_str()).unwrap();
        }

        if let Some(ref endcreditstext) = args.endcreditstext {
            self.enc_cfg.end_credits_text.clone_from(endcreditstext);
        }

        if let Some(ref endcreditsforatleast) = args.endcreditsforatleast {
            self.enc_cfg.endcreditsforatleast =
                stringztoms(endcreditsforatleast.clone().as_str()).unwrap();
        }

        if let Some(ref endcreditsforatmost) = args.endcreditsforatmost {
            self.enc_cfg.endcreditsforatmost =
                stringztoms(endcreditsforatmost.clone().as_str()).unwrap();
        }

        /* More stuff */
        if args.videoedited {
            self.binary_concat = false;
        }

        if args.goptime {
            self.use_gop_as_pts = Some(true);
        }
        if args.no_goptime {
            self.use_gop_as_pts = Some(false);
        }

        if args.fixpadding {
            self.fix_padding = true;
        }

        if args.mpeg90090 {
            set_mpeg_clock_freq(90090);
        }

        // Handle SCC framerate option
        if let Some(ref fps_str) = args.scc_framerate {
            self.scc_framerate = match fps_str.as_str() {
                "29.97" | "29" => 0,
                "24" => 1,
                "25" => 2,
                "30" => 3,
                _ => {
                    eprintln!(
                        "Invalid SCC framerate '{}'. Using default 29.97fps",
                        fps_str
                    );
                    0
                }
            };
        }

        // Handle SCC accurate timing option (issue #1120)
        if args.scc_accurate_timing {
            self.scc_accurate_timing = true;
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
        } else if args.rollup1 {
            self.settings_608.force_rollup = 1;
        } else if args.rollup2 {
            self.settings_608.force_rollup = 2;
        } else if args.rollup3 {
            self.settings_608.force_rollup = 3;
        }

        if args.trim {
            self.enc_cfg.trim_subs = true;
        }

        if let Some(ref outinterval) = args.outinterval {
            self.out_interval = *outinterval;
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
            self.sentence_cap_file = PathBuf::from_str(capfile.as_str()).unwrap_or_default();
        }

        if args.kf {
            self.enc_cfg.filter_profanity = true;
        }

        if let Some(ref profanity_file) = args.profanity_file {
            self.enc_cfg.filter_profanity = true;
            self.filter_profanity_file =
                PathBuf::from_str(profanity_file.as_str()).unwrap_or_default();
        }

        if let Some(ref program_number) = args.program_number {
            self.demux_cfg.ts_forced_program = Some(get_atoi_hex(program_number.as_str()));
        }

        if args.autoprogram {
            self.demux_cfg.ts_autoprogram = true;
        }

        if args.multiprogram {
            self.multiprogram = true;
            self.demux_cfg.ts_allprogram = true;
        }

        if args.list_tracks {
            self.list_tracks_only = true;
        }

        if let Some(ref stream) = args.stream {
            self.live_stream = Some(Timestamp::from_millis(
                1000 * get_atoi_hex::<i64>(stream.as_str()),
            ));
        }

        if let Some(ref defaultcolor) = args.defaultcolor {
            if defaultcolor.len() != 7 || !defaultcolor.starts_with('#') {
                fatal!(
                    cause = ExitCause::MalformedParameter;
                   "Invalid default color"
                );
            }
            set_usercolor_rgb(defaultcolor);
            self.settings_608.default_color = Decoder608ColorCode::Userdefined;
        }

        if let Some(ref delay) = args.delay {
            self.subs_delay = Timestamp::from_millis(*delay);
        }

        if let Some(ref screenfuls) = args.screenfuls {
            self.settings_608.screens_to_process = get_atoi_hex(screenfuls.as_str());
        }

        if let Some(ref startat) = args.startat {
            self.extraction_start = Some(stringztoms(startat.clone().as_str()).unwrap());
        }
        if let Some(ref endat) = args.endat {
            self.extraction_end = Some(stringztoms(endat.clone().as_str()).unwrap());
        }

        if args.cc2 {
            self.cc_channel = 2;
        }

        if let Some(ref extract) = args.output_field {
            if *extract == "1" || *extract == "2" || *extract == "12" {
                self.extract = get_atoi_hex(extract);
            } else if *extract == "both" {
                self.extract = 12;
            } else {
                fatal!(
                    cause = ExitCause::MalformedParameter;
                   "Invalid output field"
                );
            }
            self.is_608_enabled = true;
        }

        if args.stdout {
            if self.messages_target == OutputTarget::Stdout {
                self.messages_target = OutputTarget::Stderr;
            }
            self.cc_to_stdout = true;
        }

        if args.pesheader {
            self.pes_header_to_stdout = true;
        }

        if args.debugdvbsub {
            self.debug_mask =
                DebugMessageMask::new(DebugMessageFlag::DVB, DebugMessageFlag::VERBOSE);
        }

        if args.ignoreptsjumps {
            self.ignore_pts_jumps = true;
        }

        if args.fixptsjumps {
            self.ignore_pts_jumps = false;
        }

        if args.quiet {
            self.messages_target = OutputTarget::Quiet;
        }

        if args.debug {
            self.debug_mask =
                DebugMessageMask::new(DebugMessageFlag::VERBOSE, DebugMessageFlag::VERBOSE);
        }

        if args.eia608 {
            self.debug_mask =
                DebugMessageMask::new(DebugMessageFlag::DECODER_608, DebugMessageFlag::VERBOSE);
        }

        if args.deblev {
            self.debug_mask =
                DebugMessageMask::new(DebugMessageFlag::LEVENSHTEIN, DebugMessageFlag::VERBOSE);
        }

        if args.no_levdist {
            self.dolevdist = false;
        }

        if let Some(ref levdistmincnt) = args.levdistmincnt {
            self.levdistmincnt = get_atoi_hex(levdistmincnt.as_str());
        }
        if let Some(ref levdistmaxpct) = args.levdistmaxpct {
            self.levdistmaxpct = get_atoi_hex(levdistmaxpct.as_str());
        }

        if args.eia708 {
            self.debug_mask =
                DebugMessageMask::new(DebugMessageFlag::DECODER_708, DebugMessageFlag::VERBOSE);
        }

        if args.goppts {
            self.debug_mask =
                DebugMessageMask::new(DebugMessageFlag::TIME, DebugMessageFlag::VERBOSE);
        }

        if args.vides {
            self.debug_mask =
                DebugMessageMask::new(DebugMessageFlag::VIDEO_STREAM, DebugMessageFlag::VERBOSE);
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
            self.debug_mask =
                DebugMessageMask::new(DebugMessageFlag::DECODER_XDS, DebugMessageFlag::VERBOSE);
        }

        if args.parsedebug {
            self.debug_mask =
                DebugMessageMask::new(DebugMessageFlag::PARSE, DebugMessageFlag::VERBOSE);
        }

        if args.parse_pat {
            self.debug_mask =
                DebugMessageMask::new(DebugMessageFlag::PAT, DebugMessageFlag::VERBOSE);
        }

        if args.parse_pmt {
            self.debug_mask =
                DebugMessageMask::new(DebugMessageFlag::PMT, DebugMessageFlag::VERBOSE);
        }

        if args.dumpdef {
            self.debug_mask =
                DebugMessageMask::new(DebugMessageFlag::DUMP_DEF, DebugMessageFlag::VERBOSE);
        }

        if args.investigate_packets {
            self.investigate_packets = true;
        }

        if args.cbraw {
            self.debug_mask =
                DebugMessageMask::new(DebugMessageFlag::CB_RAW, DebugMessageFlag::VERBOSE);
        }

        if args.tverbose {
            self.debug_mask =
                DebugMessageMask::new(DebugMessageFlag::TELETEXT, DebugMessageFlag::VERBOSE);
            tlt_config.verbose = true;
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
            self.enc_cfg.encoding = Encoding::UCS2;
        }

        if args.utf8 {
            self.enc_cfg.encoding = Encoding::UTF8;
        }

        if args.latin1 {
            self.enc_cfg.encoding = Encoding::Latin1;
        }
        if args.usepicorder {
            self.usepicorder = true;
        }

        if args.myth {
            self.auto_myth = Some(true);
        }
        if args.no_myth {
            self.auto_myth = Some(false);
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
            self.demux_cfg
                .ts_cappids
                .push(get_atoi_hex(datapid.as_str()));
        }

        if let Some(ref datastreamtype) = args.datastreamtype {
            if let Some(streamType) =
                StreamType::from_repr(get_atoi_hex::<usize>(&datastreamtype.to_string()))
            {
                self.demux_cfg.ts_datastreamtype = streamType;
            } else {
                fatal!(
                    cause = ExitCause::MalformedParameter;
                   "Invalid data stream type"
                );
            }
        }

        if let Some(ref streamtype) = args.streamtype {
            if let Some(streamType) =
                StreamType::from_repr(get_atoi_hex::<usize>(&streamtype.to_string()))
            {
                self.demux_cfg.ts_forced_streamtype = streamType;
            } else {
                fatal!(
                    cause = ExitCause::MalformedParameter;
                   "Invalid stream type"
                );
            }
        }

        if let Some(ref tpages) = args.tpage {
            // Support multiple --tpage arguments (issue #665)
            if tpages.len() == 1 {
                // Single page - legacy mode
                tlt_config.user_page = tpages[0];
                tlt_config.page = Cell::new(TeletextPageNumber::from(tlt_config.user_page));
            } else {
                // Multiple pages - each gets a separate output file
                for &page_num in tpages {
                    if (100..=899).contains(&page_num) {
                        tlt_config.user_pages.push(page_num);
                    }
                }
                // Set first page as legacy value for backward compatibility
                if !tlt_config.user_pages.is_empty() {
                    tlt_config.user_page = tlt_config.user_pages[0];
                    tlt_config.page = Cell::new(TeletextPageNumber::from(tlt_config.user_page));
                }
            }
        }

        if args.tpages_all {
            tlt_config.extract_all_pages = true;
        }

        // Red Hen/ UCLA Specific stuff
        if args.ucla {
            self.ucla = true;
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

        if args.ttxtforcelatin {
            tlt_config.forceg0latin = true;
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
            self.xmltv = get_atoi_hex(xmltv.as_str());
        }

        if let Some(ref xmltvliveinterval) = args.xmltvliveinterval {
            self.xmltvliveinterval =
                Timestamp::from_millis(1000 * get_atoi_hex::<i64>(xmltvliveinterval.as_str()));
        }

        if let Some(ref xmltvoutputinterval) = args.xmltvoutputinterval {
            self.xmltvoutputinterval =
                Timestamp::from_millis(1000 * get_atoi_hex::<i64>(xmltvoutputinterval.as_str()));
        }
        if args.xmltvonlycurrent {
            self.xmltvonlycurrent = true;
        }

        if let Some(ref unixts) = args.unixts {
            let mut t = get_atoi_hex(unixts.as_str());

            if t == 0 {
                t = OffsetDateTime::now_utc().unix_timestamp() as u64;
            }
            *UTC_REFVALUE.write().unwrap() = t as u64;
            self.noautotimeref = true;
        }

        if args.sects {
            self.date_format = TimestampFormat::Seconds {
                millis_separator: ',',
            };
        }

        if args.datets {
            self.date_format = TimestampFormat::Date {
                millis_separator: ',',
            };
        }

        if args.teletext {
            self.demux_cfg.codec = SelectCodec::Some(Codec::Teletext);
        }

        if args.no_teletext {
            self.demux_cfg.nocodec = SelectCodec::Some(Codec::Teletext);
        }

        if let Some(ref customtxt) = args.customtxt {
            if customtxt.to_string().len() == 7 {
                if self.date_format == TimestampFormat::None {
                    self.date_format = TimestampFormat::HHMMSSFFF;
                }

                if !self.transcript_settings.is_final {
                    let chars = format!("{customtxt}").chars().collect::<Vec<char>>();
                    self.transcript_settings.show_start_time = chars[0] == '1';
                    self.transcript_settings.show_end_time = chars[1] == '1';
                    self.transcript_settings.show_mode = chars[2] == '1';
                    self.transcript_settings.show_cc = chars[3] == '1';
                    self.transcript_settings.relative_timestamp = chars[4] == '1';
                    self.transcript_settings.xds = chars[5] == '1';
                    self.transcript_settings.use_colors = chars[6] == '1';
                }
            } else {
                fatal!(
                    cause = ExitCause::MalformedParameter;
                   "Invalid customtxt value. It must be 7 digits long"
                );
            }
        }

        // Network stuff
        if let Some(ref udp) = args.udp {
            let mut remaining_udp = udp.as_str();

            if let Some(at) = remaining_udp.find('@') {
                let src = &remaining_udp[0..at];
                self.udpsrc = Some(src.to_owned());

                remaining_udp = &remaining_udp[at + 1..];
            }

            if let Some(colon) = remaining_udp.find(':') {
                let addr = &remaining_udp[0..colon];
                let port = get_atoi_hex(&remaining_udp[colon + 1..]);

                self.udpaddr = Some(addr.to_owned());
                self.udpport = port;
            } else {
                match remaining_udp.parse() {
                    Ok(port) => {
                        self.udpport = port;
                    }
                    Err(_) => {
                        fatal!(
                            cause = ExitCause::MalformedParameter;
                           "Invalid UDP parameter\n"
                        );
                    }
                }
            }

            self.input_source = DataSource::Network;
        }

        if let Some(ref addr) = args.sendto {
            self.send_to_srv = true;
            self.set_output_format_type(OutFormat::Bin);

            self.xmltv = 2;
            self.xmltvliveinterval = Timestamp::from_millis(2000);
            let _addr: String = addr.to_string();

            // Handle IPv6 addresses in [addr]:port format
            if let Some(saddr) = addr.strip_prefix('[') {
                if let Some(end_bracket) = saddr.find(']') {
                    let addr_part = &saddr[..end_bracket];
                    let port_part = &saddr[end_bracket + 1..];
                    self.srv_addr = Some(addr_part.to_string());
                    if let Some(port) = port_part.strip_prefix(':') {
                        self.srv_port = Some(port.parse().unwrap());
                    }
                } else {
                    fatal!(
                        cause = ExitCause::IncompatibleParameters;
                        "Wrong address format, for IPv6 use [address]:port\n"
                    );
                }
            } else if let Some(colon) = _addr.rfind(':') {
                // Handle IPv4 or hostname:port
                let (host, port) = _addr.split_at(colon);
                self.srv_addr = Some(host.to_string());
                self.srv_port = Some(port[1..].parse().unwrap());
            } else {
                // No port specified, treat as address only
                self.srv_addr = Some(_addr.clone());
                self.srv_port = None;
            }
        }
        if let Some(ref tcp) = args.tcp {
            self.tcpport = Some(*tcp);
            self.input_source = DataSource::Tcp;
            self.set_input_format_type(InFormat::Bin);
        }

        if let Some(ref tcppassworrd) = args.tcp_password {
            self.tcp_password = Some(tcppassworrd.to_string());
        }

        if let Some(ref tcpdesc) = args.tcp_description {
            self.tcp_desc = Some(tcpdesc.to_string());
        }

        if let Some(ref font) = args.font {
            self.enc_cfg.render_font = PathBuf::from_str(font).unwrap_or_default();
        }

        if let Some(ref italics) = args.italics {
            self.enc_cfg.render_font_italics = PathBuf::from_str(italics).unwrap_or_default();
        }

        #[cfg(feature = "with_libcurl")]
        {
            use url::Url;
            if let Some(ref curlposturl) = args.curlposturl {
                self.curlposturl = Url::from_str(curlposturl).ok();
            }
        }

        if self.demux_cfg.auto_stream == StreamMode::Mp4 && self.input_source == DataSource::Stdin {
            fatal!(
                cause = ExitCause::IncompatibleParameters;
               "MP4 requires an actual file, it's not possible to read from a stream, including stdin."
            );
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
            add_builtin_capitalization(capitalization_list);

            if self.sentence_cap_file.exists() {
                if let Some(sentence_cap_file) = self.sentence_cap_file.to_str() {
                    if process_word_file(sentence_cap_file, capitalization_list).is_err() {
                        fatal!(
                            cause = ExitCause::ErrorInCapitalizationFile;
                           "There was an error processing the capitalization file.\n"
                        );
                    }
                }
            }
        }
        if self.enc_cfg.filter_profanity {
            add_builtin_profane(profane);
            if self.filter_profanity_file.exists() {
                if let Some(filter_profanity_file) = self.filter_profanity_file.to_str() {
                    if process_word_file(filter_profanity_file, profane).is_err() {
                        fatal!(
                            cause = ExitCause::ErrorInCapitalizationFile;
                           "There was an error processing the profanity file.\n"
                        );
                    }
                }
            }
        }

        // Init telexcc redundant options
        tlt_config.dolevdist = self.dolevdist;
        tlt_config.levdistmincnt = self.levdistmincnt;
        tlt_config.levdistmaxpct = self.levdistmaxpct;
        tlt_config.extraction_start = self.extraction_start;
        tlt_config.extraction_end = self.extraction_end;
        tlt_config.write_format = self.write_format;
        tlt_config.date_format = self.date_format;
        tlt_config.noautotimeref = self.noautotimeref;
        tlt_config.nofontcolor = self.nofontcolor;
        tlt_config.nohtmlescape = self.nohtmlescape;

        // teletext page number out of range
        if tlt_config.user_page != 0 && (tlt_config.user_page < 100 || tlt_config.user_page > 899) {
            fatal!(
                cause = ExitCause::NotClassified;
               "Teletext page number out of range (100-899)"
            );
        }
        // Validate multiple pages if specified (issue #665)
        for page in &tlt_config.user_pages {
            if *page < 100 || *page > 899 {
                fatal!(
                    cause = ExitCause::NotClassified;
                   "Teletext page number {} out of range (100-899)", page
                );
            }
        }

        if self.is_inputfile_empty() && self.input_source == DataSource::File {
            fatal!(
                cause = ExitCause::NoInputFiles;
                "No input file specified\n"
            );
        }

        if !self.is_inputfile_empty() && self.live_stream.unwrap_or_default().millis() != 0 {
            fatal!(
                cause = ExitCause::TooManyInputFiles;
               "Live stream mode only supports one input file"
            );
        }

        if !self.is_inputfile_empty() && self.input_source == DataSource::Network {
            fatal!(
                cause = ExitCause::TooManyInputFiles;
               "UDP mode is not compatible with input files"
            );
        }

        if self.input_source == DataSource::Network || self.input_source == DataSource::Tcp {
            self.buffer_input = true;
        }

        if !self.is_inputfile_empty() && self.input_source == DataSource::Tcp {
            fatal!(
                cause = ExitCause::TooManyInputFiles;
                "TCP mode is not compatible with input files"
            );
        }

        if self.demux_cfg.auto_stream == StreamMode::McpoodlesRaw
            && self.write_format == OutputFormat::Raw
        {
            fatal!(
                cause = ExitCause::IncompatibleParameters;
                "-in=raw can only be used if the output is a subtitle file."
            );
        }

        if self.demux_cfg.auto_stream == StreamMode::Rcwt
            && self.write_format == OutputFormat::Rcwt
            && self.output_filename.is_none()
        {
            fatal!(
                cause = ExitCause::IncompatibleParameters;
                "CCExtractor's binary format can only be used simultaneously for input and\noutput if the output file name is specified given with -o.\n"
            );
        }

        if self.write_format != OutputFormat::DvdRaw
            && self.cc_to_stdout
            && self.extract != 0
            && self.extract == 12
        {
            fatal!(
                cause = ExitCause::IncompatibleParameters;
                "You can't extract both fields to stdout at the same time in broadcast mode.\n"
            );
        }

        if self.write_format == OutputFormat::SpuPng && self.cc_to_stdout {
            fatal!(
                cause = ExitCause::IncompatibleParameters;
                "You cannot use --out=spupng with -stdout.\n"
            );
        }

        if self.split_dvb_subs && self.cc_to_stdout {
            fatal!(
                cause = ExitCause::IncompatibleParameters;
                "--split-dvb-subs cannot be used with stdout output.\n                     Multiple output files cannot be written to stdout."
            );
        }

        if self.write_format == OutputFormat::WebVtt && self.enc_cfg.encoding != Encoding::UTF8 {
            self.enc_cfg.encoding = Encoding::UTF8;
            println!("Note: Output format is WebVTT, forcing UTF-8");
        }

        // Check WITH_LIBCURL
        #[cfg(feature = "with_libcurl")]
        {
            if self.write_format == OutputFormat::Curl && self.curlposturl.is_none() {
                fatal!(
                    cause = ExitCause::MalformedParameter;
                   "You must pass a URL (--curlposturl) if output format is curl"
                );
            }
            if self.write_format != OutputFormat::Curl && self.curlposturl.is_some() {
                fatal!(
                    cause = ExitCause::MalformedParameter;
                  "--curlposturl requires that the format is curl"
                );
            }
        }

        // Initialize some Encoder Configuration
        self.enc_cfg.extract = self.extract;
        if !self.is_inputfile_empty() {
            self.enc_cfg.multiple_files = true;
            self.enc_cfg.first_input_file = self.inputfile.as_ref().unwrap()[0].to_string();
        }
        self.enc_cfg.cc_to_stdout = self.cc_to_stdout;
        self.enc_cfg.write_format = self.write_format;
        self.enc_cfg.send_to_srv = self.send_to_srv;
        self.enc_cfg.date_format = self.date_format;
        self.enc_cfg.transcript_settings = self.transcript_settings.clone();
        self.enc_cfg.no_font_color = self.nofontcolor;
        self.enc_cfg.force_flush = self.force_flush;
        self.enc_cfg.append_mode = self.append_mode;
        self.enc_cfg.ucla = self.ucla;
        self.enc_cfg.no_type_setting = self.notypesetting;
        self.enc_cfg.subs_delay = self.subs_delay;
        self.enc_cfg.gui_mode_reports = self.gui_mode_reports;

        if self
            .enc_cfg
            .render_font
            .to_str()
            .unwrap_or_default()
            .is_empty()
        {
            self.enc_cfg.render_font = PathBuf::from_str(DEFAULT_FONT_PATH).unwrap_or_default();
        }

        if self
            .enc_cfg
            .render_font_italics
            .to_str()
            .unwrap_or_default()
            .is_empty()
        {
            self.enc_cfg.render_font_italics =
                PathBuf::from_str(DEFAULT_FONT_PATH_ITALICS).unwrap_or_default();
        }

        if self.output_filename.is_some() && !self.multiprogram {
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
            self.extract = 0;
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
    use crate::{args::*, parser::*};
    use clap::Parser;
    use lib_ccxr::{
        common::{MkvLangFilter, OutputFormat, SelectCodec, StreamMode, StreamType},
        util::{encoding::Encoding, log::DebugMessageFlag},
    };

    /// # Safety
    ///
    /// This function is a no-op stub and is always safe to call.
    #[no_mangle]
    pub unsafe extern "C" fn set_binary_mode() {}

    fn parse_args(args: &[&str]) -> (Options, TeletextConfig) {
        let mut common_args = vec!["./ccextractor", "input_file"];
        common_args.extend_from_slice(args);
        let args = Args::try_parse_from(common_args).expect("Failed to parse arguments");
        let mut options = Options {
            ..Default::default()
        };
        let mut tlt_config = TeletextConfig {
            ..Default::default()
        };

        let mut capitalization_list: Vec<String> = vec![];
        let mut profane: Vec<String> = vec![];

        options.parse_parameters(
            &args,
            &mut tlt_config,
            &mut capitalization_list,
            &mut profane,
        );
        (options, tlt_config)
    }

    // =========================================================================
    // INPUT FORMAT TESTS
    // =========================================================================

    #[test]
    fn test_input_ts_sets_transport_stream_mode() {
        let (options, _) = parse_args(&["--input", "ts"]);
        assert_eq!(options.demux_cfg.auto_stream, StreamMode::Transport);
    }

    #[test]
    fn test_input_ps_sets_program_stream_mode() {
        let (options, _) = parse_args(&["--input", "ps"]);
        assert_eq!(options.demux_cfg.auto_stream, StreamMode::Program);
    }

    #[test]
    fn test_input_es_sets_elementary_stream_mode() {
        let (options, _) = parse_args(&["--input", "es"]);
        assert_eq!(
            options.demux_cfg.auto_stream,
            StreamMode::ElementaryOrNotFound
        );
    }

    #[test]
    fn test_input_asf_sets_asf_mode() {
        let (options, _) = parse_args(&["--input", "asf"]);
        assert_eq!(options.demux_cfg.auto_stream, StreamMode::Asf);
    }

    #[test]
    fn test_input_wtv_sets_wtv_mode() {
        let (options, _) = parse_args(&["--input", "wtv"]);
        assert_eq!(options.demux_cfg.auto_stream, StreamMode::Wtv);
    }

    #[test]
    fn test_input_mp4_sets_mp4_mode() {
        let (options, _) = parse_args(&["--input", "mp4"]);
        assert_eq!(options.demux_cfg.auto_stream, StreamMode::Mp4);
    }

    #[test]
    fn test_input_mkv_sets_mkv_mode() {
        let (options, _) = parse_args(&["--input", "mkv"]);
        assert_eq!(options.demux_cfg.auto_stream, StreamMode::Mkv);
    }

    #[test]
    fn test_input_mxf_sets_mxf_mode() {
        let (options, _) = parse_args(&["--input", "mxf"]);
        assert_eq!(options.demux_cfg.auto_stream, StreamMode::Mxf);
    }

    #[test]
    fn test_input_bin_sets_rcwt_mode() {
        let (options, _) = parse_args(&["--input", "bin"]);
        assert_eq!(options.demux_cfg.auto_stream, StreamMode::Rcwt);
    }

    #[test]
    fn test_input_raw_sets_mcpoodles_mode() {
        let (options, _) = parse_args(&["--input", "raw"]);
        assert_eq!(options.demux_cfg.auto_stream, StreamMode::McpoodlesRaw);
    }

    #[test]
    fn test_input_m2ts_sets_transport_mode() {
        let (options, _) = parse_args(&["--input", "m2ts"]);
        assert_eq!(options.demux_cfg.auto_stream, StreamMode::Transport);
    }

    #[test]
    fn test_input_scc_sets_scc_mode() {
        let (options, _) = parse_args(&["--input", "scc"]);
        assert_eq!(options.demux_cfg.auto_stream, StreamMode::Scc);
    }

    // =========================================================================
    // OUTPUT FORMAT TESTS
    // =========================================================================

    #[test]
    fn test_out_srt_sets_srt_format() {
        let (options, _) = parse_args(&["--out", "srt"]);
        assert_eq!(options.write_format, OutputFormat::Srt);
    }

    #[test]
    fn test_out_ass_sets_ssa_format() {
        let (options, _) = parse_args(&["--out", "ass"]);
        assert_eq!(options.write_format, OutputFormat::Ssa);
    }

    #[test]
    fn test_out_ssa_sets_ssa_format() {
        let (options, _) = parse_args(&["--out", "ssa"]);
        assert_eq!(options.write_format, OutputFormat::Ssa);
    }

    #[test]
    fn test_out_webvtt_sets_webvtt_format() {
        let (options, _) = parse_args(&["--out", "webvtt"]);
        assert_eq!(options.write_format, OutputFormat::WebVtt);
    }

    #[test]
    fn test_out_webvtt_full_sets_webvtt_with_styling() {
        let (options, _) = parse_args(&["--out", "webvtt-full"]);
        assert_eq!(options.write_format, OutputFormat::WebVtt);
        assert!(options.use_webvtt_styling);
    }

    #[test]
    fn test_out_sami_sets_sami_format() {
        let (options, _) = parse_args(&["--out", "sami"]);
        assert_eq!(options.write_format, OutputFormat::Sami);
    }

    #[test]
    fn test_out_dvdraw_sets_dvdraw_format() {
        let (options, _) = parse_args(&["--out", "dvdraw"]);
        assert_eq!(options.write_format, OutputFormat::DvdRaw);
    }

    #[test]
    fn test_out_raw_sets_raw_format() {
        let (options, _) = parse_args(&["--out", "raw"]);
        assert_eq!(options.write_format, OutputFormat::Raw);
    }

    #[test]
    fn test_out_bin_sets_rcwt_format() {
        let (options, _) = parse_args(&["--out", "bin"]);
        assert_eq!(options.write_format, OutputFormat::Rcwt);
    }

    #[test]
    fn test_out_txt_sets_transcript_format_with_no_rollup() {
        let (options, _) = parse_args(&["--out", "txt"]);
        assert_eq!(options.write_format, OutputFormat::Transcript);
        assert!(options.settings_dtvcc.no_rollup);
    }

    #[test]
    fn test_out_ttxt_sets_transcript_with_timestamps() {
        let (options, _) = parse_args(&["--out", "ttxt"]);
        assert_eq!(options.write_format, OutputFormat::Transcript);
        assert!(options.transcript_settings.show_start_time);
        assert!(options.transcript_settings.show_end_time);
    }

    #[test]
    fn test_out_mcc_sets_mcc_format() {
        let (options, _) = parse_args(&["--out", "mcc"]);
        assert_eq!(options.write_format, OutputFormat::Mcc);
    }

    #[test]
    fn test_out_scc_sets_scc_format() {
        let (options, _) = parse_args(&["--out", "scc"]);
        assert_eq!(options.write_format, OutputFormat::Scc);
    }

    #[test]
    fn test_out_ccd_sets_ccd_format() {
        let (options, _) = parse_args(&["--out", "ccd"]);
        assert_eq!(options.write_format, OutputFormat::Ccd);
    }

    #[test]
    fn test_out_g608_sets_g608_format() {
        let (options, _) = parse_args(&["--out", "g608"]);
        assert_eq!(options.write_format, OutputFormat::G608);
    }

    #[test]
    fn test_out_spupng_sets_spupng_format() {
        let (options, _) = parse_args(&["--out", "spupng"]);
        assert_eq!(options.write_format, OutputFormat::SpuPng);
    }

    #[test]
    fn test_out_smptett_sets_smptett_format() {
        let (options, _) = parse_args(&["--out", "smptett"]);
        assert_eq!(options.write_format, OutputFormat::SmpteTt);
    }

    #[test]
    fn test_out_simple_xml_sets_simple_xml_format() {
        let (options, _) = parse_args(&["--out", "simple-xml"]);
        assert_eq!(options.write_format, OutputFormat::SimpleXml);
    }

    #[test]
    fn test_out_null_sets_null_format() {
        let (options, _) = parse_args(&["--out", "null"]);
        assert_eq!(options.write_format, OutputFormat::Null);
    }

    #[test]
    fn test_out_report_enables_file_reports() {
        let (options, _) = parse_args(&["--out", "report"]);
        assert_eq!(options.write_format, OutputFormat::Null);
        assert!(options.print_file_reports);
        assert!(options.demux_cfg.ts_allprogram);
    }

    // =========================================================================
    // ENCODING TESTS
    // =========================================================================

    #[test]
    fn test_utf8_sets_utf8_encoding() {
        let (options, _) = parse_args(&["--utf8"]);
        assert_eq!(options.enc_cfg.encoding, Encoding::UTF8);
    }

    #[test]
    fn test_latin1_sets_latin1_encoding() {
        let (options, _) = parse_args(&["--latin1"]);
        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
    }

    #[test]
    fn test_unicode_sets_ucs2_encoding() {
        let (options, _) = parse_args(&["--unicode"]);
        assert_eq!(options.enc_cfg.encoding, Encoding::UCS2);
    }

    #[test]
    fn test_bom_disables_no_bom() {
        let (options, _) = parse_args(&["--bom"]);
        assert!(!options.enc_cfg.no_bom);
    }

    #[test]
    fn test_no_bom_enables_no_bom() {
        let (options, _) = parse_args(&["--no-bom"]);
        assert!(options.enc_cfg.no_bom);
    }

    #[test]
    fn test_lf_sets_unix_line_terminator() {
        let (options, _) = parse_args(&["--lf"]);
        assert!(options.enc_cfg.line_terminator_lf);
    }

    // =========================================================================
    // STREAM/PROGRAM SELECTION TESTS
    // =========================================================================

    #[test]
    fn test_program_number_sets_forced_program() {
        let (options, _) = parse_args(&["--program-number", "5"]);
        assert_eq!(options.demux_cfg.ts_forced_program, Some(5));
    }

    #[test]
    fn test_program_number_hex_format() {
        let (options, _) = parse_args(&["--program-number", "0x10"]);
        assert_eq!(options.demux_cfg.ts_forced_program, Some(16));
    }

    #[test]
    fn test_autoprogram_enables_auto_program_selection() {
        let (options, _) = parse_args(&["--autoprogram"]);
        assert!(options.demux_cfg.ts_autoprogram);
    }

    #[test]
    fn test_multiprogram_enables_all_programs() {
        let (options, _) = parse_args(&["--multiprogram"]);
        assert!(options.multiprogram);
        assert!(options.demux_cfg.ts_allprogram);
    }

    #[test]
    fn test_datapid_sets_caption_pid() {
        let (options, _) = parse_args(&["--datapid", "1234"]);
        assert_eq!(options.demux_cfg.ts_cappids[0], 1234);
    }

    #[test]
    fn test_datastreamtype_sets_data_stream_type() {
        let (options, _) = parse_args(&["--datastreamtype", "2"]);
        assert_eq!(options.demux_cfg.ts_datastreamtype, StreamType::VideoMpeg2);
    }

    #[test]
    fn test_streamtype_sets_forced_stream_type() {
        let (options, _) = parse_args(&["--streamtype", "6"]);
        assert_eq!(
            options.demux_cfg.ts_forced_streamtype,
            StreamType::PrivateMpeg2
        );
    }

    #[test]
    fn test_cc2_selects_channel_2() {
        let (options, _) = parse_args(&["--cc2"]);
        assert_eq!(options.cc_channel, 2);
    }

    #[test]
    fn test_output_field_1_extracts_field_1() {
        let (options, _) = parse_args(&["--output-field", "1"]);
        assert_eq!(options.extract, 1);
        assert!(options.is_608_enabled);
    }

    #[test]
    fn test_output_field_2_extracts_field_2() {
        let (options, _) = parse_args(&["--output-field", "2"]);
        assert_eq!(options.extract, 2);
        assert!(options.is_608_enabled);
    }

    #[test]
    fn test_output_field_both_extracts_both() {
        let (options, _) = parse_args(&["--output-field", "both"]);
        assert_eq!(options.extract, 12);
        assert!(options.is_608_enabled);
    }

    // =========================================================================
    // CEA-708 SERVICE TESTS
    // =========================================================================

    #[test]
    fn test_service_enables_708_with_single_service() {
        let (options, _) = parse_args(&["--service", "1"]);
        assert!(options.is_708_enabled);
        assert!(options.settings_dtvcc.services_enabled[0]);
        assert_eq!(options.settings_dtvcc.active_services_count, 1);
    }

    #[test]
    fn test_service_enables_multiple_services() {
        let (options, _) = parse_args(&["--service", "1,2,3"]);
        assert!(options.is_708_enabled);
        assert!(options.settings_dtvcc.services_enabled[0]);
        assert!(options.settings_dtvcc.services_enabled[1]);
        assert!(options.settings_dtvcc.services_enabled[2]);
        assert_eq!(options.settings_dtvcc.active_services_count, 3);
    }

    #[test]
    fn test_service_all_enables_all_services() {
        let (options, _) = parse_args(&["--service", "all"]);
        assert!(options.is_708_enabled);
        assert_eq!(
            options.settings_dtvcc.active_services_count,
            DTVCC_MAX_SERVICES as i32
        );
    }

    #[test]
    fn test_service_with_charset_sets_unique_charsets() {
        let (options, _) = parse_args(&["--service", "1[UTF-8],2[EUC-KR]"]);
        assert!(options.is_708_enabled);
        match options.enc_cfg.services_charsets {
            DtvccServiceCharset::Unique(charsets) => {
                assert_eq!(charsets[0], "UTF-8");
                assert_eq!(charsets[1], "EUC-KR");
            }
            _ => panic!("Expected DtvccServiceCharset::Unique"),
        }
    }

    #[test]
    fn test_service_all_with_charset_sets_same_charset() {
        let (options, _) = parse_args(&["--service", "all[EUC-KR]"]);
        assert!(options.is_708_enabled);
        assert_eq!(
            options.enc_cfg.services_charsets,
            DtvccServiceCharset::Same("EUC-KR".to_string())
        );
    }

    #[test]
    fn test_svc_alias_works() {
        let (options, _) = parse_args(&["--svc", "1"]);
        assert!(options.is_708_enabled);
    }

    // =========================================================================
    // CODEC SELECTION TESTS
    // =========================================================================

    #[test]
    fn test_codec_dvbsub_selects_dvb() {
        let (options, _) = parse_args(&["--codec", "dvbsub"]);
        assert_eq!(options.demux_cfg.codec, SelectCodec::Some(Codec::Dvb));
    }

    #[test]
    fn test_codec_teletext_selects_teletext() {
        let (options, _) = parse_args(&["--codec", "teletext"]);
        assert_eq!(options.demux_cfg.codec, SelectCodec::Some(Codec::Teletext));
    }

    #[test]
    fn test_no_codec_dvbsub_excludes_dvb() {
        let (options, _) = parse_args(&["--no-codec", "dvbsub"]);
        assert_eq!(options.demux_cfg.nocodec, SelectCodec::Some(Codec::Dvb));
    }

    #[test]
    fn test_no_codec_teletext_excludes_teletext() {
        let (options, _) = parse_args(&["--no-codec", "teletext"]);
        assert_eq!(
            options.demux_cfg.nocodec,
            SelectCodec::Some(Codec::Teletext)
        );
    }

    // =========================================================================
    // TIMING OPTION TESTS
    // =========================================================================

    #[test]
    fn test_goptime_enables_gop_as_pts() {
        let (options, _) = parse_args(&["--goptime"]);
        assert_eq!(options.use_gop_as_pts, Some(true));
    }

    #[test]
    fn test_no_goptime_disables_gop_as_pts() {
        let (options, _) = parse_args(&["--no-goptime"]);
        assert_eq!(options.use_gop_as_pts, Some(false));
    }

    #[test]
    fn test_fixpadding_enables_padding_fix() {
        let (options, _) = parse_args(&["--fixpadding"]);
        assert!(options.fix_padding);
    }

    #[test]
    fn test_90090_sets_mpeg_clock_frequency() {
        let (_, _) = parse_args(&["--90090"]);
        unsafe {
            assert_eq!(MPEG_CLOCK_FREQ as i64, 90090);
        }
    }

    #[test]
    fn test_delay_sets_subtitle_delay_positive() {
        let (options, _) = parse_args(&["--delay", "500"]);
        assert_eq!(options.subs_delay, Timestamp::from_millis(500));
    }

    #[test]
    fn test_delay_sets_subtitle_delay_negative() {
        let (options, _) = parse_args(&["--delay=-200"]);
        assert_eq!(options.subs_delay, Timestamp::from_millis(-200));
    }

    #[test]
    fn test_startat_sets_extraction_start() {
        let (options, _) = parse_args(&["--startat", "4"]);
        assert_eq!(options.extraction_start.unwrap_or_default().seconds(), 4);
    }

    #[test]
    fn test_endat_sets_extraction_end() {
        let (options, _) = parse_args(&["--endat", "7"]);
        assert_eq!(options.extraction_end.unwrap_or_default().seconds(), 7);
    }

    #[test]
    fn test_startat_and_endat_together() {
        let (options, _) = parse_args(&["--startat", "4", "--endat", "7"]);
        assert_eq!(options.extraction_start.unwrap_or_default().seconds(), 4);
        assert_eq!(options.extraction_end.unwrap_or_default().seconds(), 7);
    }

    #[test]
    fn test_no_autotimeref_disables_auto_time_reference() {
        let (options, _) = parse_args(&["--no-autotimeref"]);
        assert!(options.noautotimeref);
    }

    #[test]
    fn test_unixts_sets_utc_reference() {
        let (options, _) = parse_args(&["--unixts", "1234567890"]);
        assert_eq!(*UTC_REFVALUE.read().unwrap(), 1234567890);
        assert!(options.noautotimeref);
    }

    #[test]
    fn test_datets_sets_date_timestamp_format() {
        let (options, _) = parse_args(&["--datets"]);
        assert_eq!(
            options.date_format,
            TimestampFormat::Date {
                millis_separator: ','
            }
        );
    }

    #[test]
    fn test_sects_sets_seconds_timestamp_format() {
        let (options, _) = parse_args(&["--sects"]);
        assert_eq!(
            options.date_format,
            TimestampFormat::Seconds {
                millis_separator: ','
            }
        );
    }

    // =========================================================================
    // MYTHTV OPTION TESTS
    // =========================================================================

    #[test]
    fn test_myth_enables_mythtv_mode() {
        let (options, _) = parse_args(&["--myth"]);
        assert_eq!(options.auto_myth, Some(true));
    }

    #[test]
    fn test_no_myth_disables_mythtv_mode() {
        let (options, _) = parse_args(&["--no-myth"]);
        assert_eq!(options.auto_myth, Some(false));
    }

    // =========================================================================
    // WTV OPTION TESTS
    // =========================================================================

    #[test]
    fn test_wtvconvertfix_enables_wtv_fix() {
        let (options, _) = parse_args(&["--wtvconvertfix"]);
        assert!(options.wtvconvertfix);
    }

    #[test]
    fn test_wtvmpeg2_enables_wtv_mpeg2_processing() {
        let (options, _) = parse_args(&["--wtvmpeg2"]);
        assert!(options.wtvmpeg2);
    }

    // =========================================================================
    // MP4 OPTION TESTS
    // =========================================================================

    #[test]
    fn test_mp4vidtrack_forces_video_track_processing() {
        let (options, _) = parse_args(&["--mp4vidtrack"]);
        assert!(options.mp4vidtrack);
    }

    #[test]
    fn test_chapters_enables_chapter_extraction() {
        let (options, _) = parse_args(&["--chapters"]);
        assert!(options.extract_chapters);
    }

    // =========================================================================
    // HAUPPAUGE OPTION TESTS
    // =========================================================================

    #[test]
    fn test_hauppauge_enables_hauppauge_mode() {
        let (options, _) = parse_args(&["--hauppauge"]);
        assert!(options.hauppauge_mode);
    }

    // =========================================================================
    // ROLLUP OPTION TESTS
    // =========================================================================

    #[test]
    fn test_no_rollup_disables_rollup() {
        let (options, _) = parse_args(&["--no-rollup"]);
        assert!(options.no_rollup);
        assert!(options.settings_608.no_rollup);
        assert!(options.settings_dtvcc.no_rollup);
    }

    #[test]
    fn test_ru1_forces_1_line_rollup() {
        let (options, _) = parse_args(&["--ru1"]);
        assert_eq!(options.settings_608.force_rollup, 1);
    }

    #[test]
    fn test_ru2_forces_2_line_rollup() {
        let (options, _) = parse_args(&["--ru2"]);
        assert_eq!(options.settings_608.force_rollup, 2);
    }

    #[test]
    fn test_ru3_forces_3_line_rollup() {
        let (options, _) = parse_args(&["--ru3"]);
        assert_eq!(options.settings_608.force_rollup, 3);
    }

    #[test]
    fn test_dru_enables_direct_rollup() {
        let (options, _) = parse_args(&["--dru"]);
        assert_eq!(options.settings_608.direct_rollup, 1);
    }

    // =========================================================================
    // BUFFERING OPTION TESTS
    // =========================================================================

    #[test]
    fn test_bufferinput_enables_input_buffering() {
        let (options, _) = parse_args(&["--bufferinput"]);
        assert!(options.buffer_input);
    }

    #[test]
    fn test_no_bufferinput_disables_input_buffering() {
        let (options, _) = parse_args(&["--no-bufferinput"]);
        assert!(!options.buffer_input);
    }

    #[test]
    fn test_buffersize_with_k_suffix() {
        let (_, _) = parse_args(&["--buffersize", "64K"]);
        assert_eq!(get_file_buffer_size(), 64 * 1024);
    }

    #[test]
    fn test_buffersize_with_m_suffix() {
        let (_, _) = parse_args(&["--buffersize", "2M"]);
        assert_eq!(get_file_buffer_size(), 2 * 1024 * 1024);
    }

    #[test]
    fn test_buffersize_with_numeric_value() {
        let (_, _) = parse_args(&["--buffersize", "8192"]);
        assert_eq!(get_file_buffer_size(), 8192);
    }

    #[test]
    fn test_koc_enables_keep_output_closed() {
        let (options, _) = parse_args(&["--koc"]);
        assert!(options.keep_output_closed);
    }

    #[test]
    fn test_forceflush_enables_force_flush() {
        let (options, _) = parse_args(&["--forceflush"]);
        assert!(options.force_flush);
    }

    #[test]
    fn test_append_enables_append_mode() {
        let (options, _) = parse_args(&["--append"]);
        assert!(options.append_mode);
    }

    // =========================================================================
    // OUTPUT OPTION TESTS
    // =========================================================================

    #[test]
    fn test_stdout_redirects_to_stdout() {
        let (options, _) = parse_args(&["--stdout"]);
        assert!(options.cc_to_stdout);
        assert_eq!(options.messages_target, OutputTarget::Stderr);
    }

    #[test]
    fn test_quiet_suppresses_output() {
        let (options, _) = parse_args(&["--quiet"]);
        assert_eq!(options.messages_target, OutputTarget::Quiet);
    }

    #[test]
    fn test_no_fontcolor_disables_font_colors() {
        let (options, _) = parse_args(&["--no-fontcolor"]);
        assert!(options.nofontcolor);
    }

    #[test]
    fn test_no_htmlescape_disables_html_escaping() {
        let (options, _) = parse_args(&["--no-htmlescape"]);
        assert!(options.nohtmlescape);
    }

    #[test]
    fn test_no_typesetting_disables_typesetting() {
        let (options, _) = parse_args(&["--no-typesetting"]);
        assert!(options.notypesetting);
    }

    #[test]
    fn test_trim_enables_subtitle_trimming() {
        let (options, _) = parse_args(&["--trim"]);
        assert!(options.enc_cfg.trim_subs);
    }

    #[test]
    fn test_sentencecap_enables_sentence_capitalization() {
        let (options, _) = parse_args(&["--sentencecap"]);
        assert!(options.enc_cfg.sentence_cap);
    }

    #[test]
    fn test_capfile_sets_capitalization_file() {
        let (options, _) = parse_args(&["--capfile", "words.txt"]);
        assert!(options.enc_cfg.sentence_cap);
        assert_eq!(options.sentence_cap_file.to_str(), Some("words.txt"));
    }

    #[test]
    fn test_kf_enables_profanity_filter() {
        let (options, _) = parse_args(&["--kf"]);
        assert!(options.enc_cfg.filter_profanity);
    }

    #[test]
    fn test_profanity_file_sets_filter_file() {
        let (options, _) = parse_args(&["--profanity-file", "badwords.txt"]);
        assert!(options.enc_cfg.filter_profanity);
        assert_eq!(options.filter_profanity_file.to_str(), Some("badwords.txt"));
    }

    #[test]
    fn test_splitbysentence_enables_sentence_splitting() {
        let (options, _) = parse_args(&["--splitbysentence"]);
        assert!(options.enc_cfg.splitbysentence);
    }

    #[test]
    fn test_autodash_enables_speaker_identification() {
        let (options, _) = parse_args(&["--autodash"]);
        assert!(options.enc_cfg.autodash);
    }

    #[test]
    fn test_sem_enables_semaphore_files() {
        let (options, _) = parse_args(&["--sem"]);
        assert!(options.enc_cfg.with_semaphore);
    }

    #[test]
    fn test_df_enables_dropframe() {
        let (options, _) = parse_args(&["--df"]);
        assert!(options.enc_cfg.force_dropframe);
    }

    // =========================================================================
    // DEBUG FLAG TESTS
    // =========================================================================

    #[test]
    fn test_debug_enables_verbose_debug() {
        let (options, _) = parse_args(&["--debug"]);
        assert_eq!(options.debug_mask.mask(), DebugMessageFlag::VERBOSE);
    }

    #[test]
    fn test_608_enables_decoder_608_debug() {
        let (options, _) = parse_args(&["--608"]);
        assert_eq!(options.debug_mask.mask(), DebugMessageFlag::DECODER_608);
    }

    #[test]
    fn test_708_enables_decoder_708_debug() {
        let (options, _) = parse_args(&["--708"]);
        assert_eq!(options.debug_mask.mask(), DebugMessageFlag::DECODER_708);
    }

    #[test]
    fn test_goppts_enables_time_debug() {
        let (options, _) = parse_args(&["--goppts"]);
        assert_eq!(options.debug_mask.mask(), DebugMessageFlag::TIME);
    }

    #[test]
    fn test_xdsdebug_enables_xds_debug() {
        let (options, _) = parse_args(&["--xdsdebug"]);
        assert!(options.transcript_settings.xds);
        assert_eq!(options.debug_mask.mask(), DebugMessageFlag::DECODER_XDS);
    }

    #[test]
    fn test_vides_enables_video_stream_debug() {
        let (options, _) = parse_args(&["--vides"]);
        assert_eq!(options.debug_mask.mask(), DebugMessageFlag::VIDEO_STREAM);
        assert!(options.analyze_video_stream);
    }

    #[test]
    fn test_cbraw_enables_raw_cc_debug() {
        let (options, _) = parse_args(&["--cbraw"]);
        assert_eq!(options.debug_mask.mask(), DebugMessageFlag::CB_RAW);
    }

    #[test]
    fn test_parsedebug_enables_parse_debug() {
        let (options, _) = parse_args(&["--parsedebug"]);
        assert_eq!(options.debug_mask.mask(), DebugMessageFlag::PARSE);
    }

    #[test]
    fn test_parse_pat_enables_pat_debug() {
        let (options, _) = parse_args(&["--parsePAT"]);
        assert_eq!(options.debug_mask.mask(), DebugMessageFlag::PAT);
    }

    #[test]
    fn test_parse_pmt_enables_pmt_debug() {
        let (options, _) = parse_args(&["--parsePMT"]);
        assert_eq!(options.debug_mask.mask(), DebugMessageFlag::PMT);
    }

    #[test]
    fn test_dumpdef_enables_dump_def_debug() {
        let (options, _) = parse_args(&["--dumpdef"]);
        assert_eq!(options.debug_mask.mask(), DebugMessageFlag::DUMP_DEF);
    }

    #[test]
    fn test_investigate_packets_enables_packet_investigation() {
        let (options, _) = parse_args(&["--investigate-packets"]);
        assert!(options.investigate_packets);
    }

    #[test]
    fn test_no_sync_disables_syncing() {
        let (options, _) = parse_args(&["--no-sync"]);
        assert!(options.nosync);
    }

    #[test]
    fn test_fullbin_enables_full_binary_output() {
        let (options, _) = parse_args(&["--fullbin"]);
        assert!(options.fullbin);
    }

    #[test]
    fn test_deblev_enables_levenshtein_debug() {
        let (options, _) = parse_args(&["--deblev"]);
        assert_eq!(options.debug_mask.mask(), DebugMessageFlag::LEVENSHTEIN);
    }

    #[test]
    fn test_debugdvbsub_enables_dvb_debug() {
        let (options, _) = parse_args(&["--debugdvbsub"]);
        assert_eq!(options.debug_mask.mask(), DebugMessageFlag::DVB);
    }

    #[test]
    fn test_pesheader_enables_pes_header_output() {
        let (options, _) = parse_args(&["--pesheader"]);
        assert!(options.pes_header_to_stdout);
    }

    // =========================================================================
    // LEVENSHTEIN DISTANCE TESTS
    // =========================================================================

    #[test]
    fn test_no_levdist_disables_levenshtein() {
        let (options, _) = parse_args(&["--no-levdist"]);
        assert!(!options.dolevdist);
    }

    #[test]
    fn test_levdistmincnt_sets_minimum_count() {
        let (options, _) = parse_args(&["--levdistmincnt", "5"]);
        assert_eq!(options.levdistmincnt, 5);
    }

    #[test]
    fn test_levdistmaxpct_sets_maximum_percentage() {
        let (options, _) = parse_args(&["--levdistmaxpct", "20"]);
        assert_eq!(options.levdistmaxpct, 20);
    }

    // =========================================================================
    // TELETEXT OPTION TESTS
    // =========================================================================

    #[test]
    fn test_teletext_enables_teletext_codec() {
        let (options, _) = parse_args(&["--teletext"]);
        assert_eq!(options.demux_cfg.codec, SelectCodec::Some(Codec::Teletext));
    }

    #[test]
    fn test_no_teletext_disables_teletext_codec() {
        let (options, _) = parse_args(&["--no-teletext"]);
        assert_eq!(
            options.demux_cfg.nocodec,
            SelectCodec::Some(Codec::Teletext)
        );
    }

    #[test]
    fn test_tpage_sets_teletext_page() {
        let (_, tlt_config) = parse_args(&["--tpage", "888"]);
        assert_eq!(tlt_config.user_page, 888);
    }

    #[test]
    fn test_tpage_multiple_pages() {
        let (_, tlt_config) = parse_args(&["--tpage", "888", "--tpage", "889"]);
        assert_eq!(tlt_config.user_pages.len(), 2);
        assert!(tlt_config.user_pages.contains(&888));
        assert!(tlt_config.user_pages.contains(&889));
    }

    #[test]
    fn test_tpages_all_extracts_all_pages() {
        let (_, tlt_config) = parse_args(&["--tpages-all"]);
        assert!(tlt_config.extract_all_pages);
    }

    #[test]
    fn test_tverbose_enables_teletext_verbose() {
        let (options, tlt_config) = parse_args(&["--tverbose"]);
        assert!(tlt_config.verbose);
        assert_eq!(options.debug_mask.mask(), DebugMessageFlag::TELETEXT);
    }

    #[test]
    fn test_latrusmap_enables_russian_mapping() {
        let (_, tlt_config) = parse_args(&["--latrusmap"]);
        assert!(tlt_config.latrusmap);
    }

    #[test]
    fn test_ttxtforcelatin_forces_latin_charset() {
        let (_, tlt_config) = parse_args(&["--ttxtforcelatin"]);
        assert!(tlt_config.forceg0latin);
    }

    // =========================================================================
    // XMLTV OPTION TESTS
    // =========================================================================

    #[test]
    fn test_xmltv_sets_xmltv_mode() {
        let (options, _) = parse_args(&["--xmltv", "1"]);
        assert_eq!(options.xmltv, 1);
    }

    #[test]
    fn test_xmltv_live_mode() {
        let (options, _) = parse_args(&["--xmltv", "2"]);
        assert_eq!(options.xmltv, 2);
    }

    #[test]
    fn test_xmltv_both_modes() {
        let (options, _) = parse_args(&["--xmltv", "3"]);
        assert_eq!(options.xmltv, 3);
    }

    #[test]
    fn test_xmltvliveinterval_sets_interval() {
        let (options, _) = parse_args(&["--xmltvliveinterval", "10"]);
        assert_eq!(options.xmltvliveinterval.millis(), 10000);
    }

    #[test]
    fn test_xmltvoutputinterval_sets_interval() {
        let (options, _) = parse_args(&["--xmltvoutputinterval", "30"]);
        assert_eq!(options.xmltvoutputinterval.millis(), 30000);
    }

    #[test]
    fn test_xmltvonlycurrent_enables_current_only() {
        let (options, _) = parse_args(&["--xmltvonlycurrent"]);
        assert!(options.xmltvonlycurrent);
    }

    // =========================================================================
    // CREDITS OPTION TESTS
    // =========================================================================

    #[test]
    fn test_startcreditstext_sets_start_credits() {
        let (options, _) = parse_args(&["--startcreditstext", "Opening Credits"]);
        assert_eq!(options.enc_cfg.start_credits_text, "Opening Credits");
    }

    #[test]
    fn test_startcreditsnotbefore_sets_time() {
        let (options, _) = parse_args(&["--startcreditsnotbefore", "5"]);
        assert_eq!(options.enc_cfg.startcreditsnotbefore.seconds(), 5);
    }

    #[test]
    fn test_startcreditsnotafter_sets_time() {
        let (options, _) = parse_args(&["--startcreditsnotafter", "30"]);
        assert_eq!(options.enc_cfg.startcreditsnotafter.seconds(), 30);
    }

    #[test]
    fn test_startcreditsforatleast_sets_duration() {
        let (options, _) = parse_args(&["--startcreditsforatleast", "3"]);
        assert_eq!(options.enc_cfg.startcreditsforatleast.seconds(), 3);
    }

    #[test]
    fn test_startcreditsforatmost_sets_duration() {
        let (options, _) = parse_args(&["--startcreditsforatmost", "10"]);
        assert_eq!(options.enc_cfg.startcreditsforatmost.seconds(), 10);
    }

    #[test]
    fn test_endcreditstext_sets_end_credits() {
        let (options, _) = parse_args(&["--endcreditstext", "Closing Credits"]);
        assert_eq!(options.enc_cfg.end_credits_text, "Closing Credits");
    }

    #[test]
    fn test_endcreditsforatleast_sets_duration() {
        let (options, _) = parse_args(&["--endcreditsforatleast", "5"]);
        assert_eq!(options.enc_cfg.endcreditsforatleast.seconds(), 5);
    }

    #[test]
    fn test_endcreditsforatmost_sets_duration() {
        let (options, _) = parse_args(&["--endcreditsforatmost", "15"]);
        assert_eq!(options.enc_cfg.endcreditsforatmost.seconds(), 15);
    }

    // =========================================================================
    // LANGUAGE OPTION TESTS
    // =========================================================================

    #[test]
    fn test_dvblang_sets_dvb_language() {
        let (options, _) = parse_args(&["--dvblang", "eng"]);
        assert_eq!(options.dvblang.unwrap(), Language::Eng);
    }

    #[test]
    fn test_mkvlang_sets_mkv_language() {
        let (options, _) = parse_args(&["--mkvlang", "eng"]);
        assert_eq!(
            options.mkvlang.unwrap(),
            "eng".parse::<MkvLangFilter>().unwrap()
        );
    }

    #[test]
    fn test_ocrlang_sets_ocr_language() {
        let (options, _) = parse_args(&["--ocrlang", "chi_tra"]);
        assert_eq!(options.ocrlang.as_ref().unwrap(), "chi_tra");
    }

    // =========================================================================
    // OCR OPTION TESTS
    // =========================================================================

    #[test]
    fn test_quant_sets_quantization_mode() {
        let (options, _) = parse_args(&["--quant", "2"]);
        assert_eq!(options.ocr_quantmode, 2);
    }

    #[test]
    fn test_oem_sets_ocr_engine_mode() {
        let (options, _) = parse_args(&["--oem", "1"]);
        assert_eq!(options.ocr_oem, 1);
    }

    #[test]
    fn test_psm_sets_page_segmentation_mode() {
        let (options, _) = parse_args(&["--psm", "7"]);
        assert_eq!(options.psm, 7);
    }

    #[test]
    fn test_ocr_line_split_enables_line_splitting() {
        let (options, _) = parse_args(&["--ocr-line-split"]);
        assert!(options.ocr_line_split);
    }

    #[test]
    fn test_no_ocr_blacklist_disables_blacklist() {
        let (options, _) = parse_args(&["--no-ocr-blacklist"]);
        assert!(!options.ocr_blacklist);
    }

    #[test]
    fn test_no_spupngocr_disables_spupng_ocr() {
        let (options, _) = parse_args(&["--no-spupngocr"]);
        assert!(options.enc_cfg.nospupngocr);
    }

    // =========================================================================
    // FONT OPTION TESTS
    // =========================================================================

    #[test]
    fn test_font_sets_font_path() {
        let (options, _) = parse_args(&["--font", "/path/to/font.ttf"]);
        assert_eq!(
            options.enc_cfg.render_font.to_str(),
            Some("/path/to/font.ttf")
        );
    }

    #[test]
    fn test_italics_sets_italics_font_path() {
        let (options, _) = parse_args(&["--italics", "/path/to/font-italic.ttf"]);
        assert_eq!(
            options.enc_cfg.render_font_italics.to_str(),
            Some("/path/to/font-italic.ttf")
        );
    }

    // =========================================================================
    // SCC INPUT OPTION TESTS
    // =========================================================================

    #[test]
    fn test_scc_framerate_29_97() {
        let (options, _) = parse_args(&["--scc-framerate", "29.97"]);
        assert_eq!(options.scc_framerate, 0);
    }

    #[test]
    fn test_scc_framerate_24() {
        let (options, _) = parse_args(&["--scc-framerate", "24"]);
        assert_eq!(options.scc_framerate, 1);
    }

    #[test]
    fn test_scc_framerate_25() {
        let (options, _) = parse_args(&["--scc-framerate", "25"]);
        assert_eq!(options.scc_framerate, 2);
    }

    #[test]
    fn test_scc_framerate_30() {
        let (options, _) = parse_args(&["--scc-framerate", "30"]);
        assert_eq!(options.scc_framerate, 3);
    }

    #[test]
    fn test_scc_accurate_timing_enables_accurate_mode() {
        let (options, _) = parse_args(&["--scc-accurate-timing"]);
        assert!(options.scc_accurate_timing);
    }

    // =========================================================================
    // MISCELLANEOUS OPTION TESTS
    // =========================================================================

    #[test]
    fn test_usepicorder_enables_pic_order() {
        let (options, _) = parse_args(&["--usepicorder"]);
        assert!(options.usepicorder);
    }

    #[test]
    fn test_videoedited_disables_binary_concat() {
        let (options, _) = parse_args(&["--videoedited"]);
        assert!(!options.binary_concat);
    }

    #[test]
    fn test_no_scte20_disables_scte20() {
        let (options, _) = parse_args(&["--no-scte20"]);
        assert!(options.noscte20);
    }

    #[test]
    fn test_webvtt_create_css_enables_css_file() {
        let (options, _) = parse_args(&["--webvtt-create-css"]);
        assert!(options.webvtt_create_css);
    }

    #[test]
    fn test_timestamp_map_enables_hls_header() {
        let (options, _) = parse_args(&["--timestamp-map"]);
        assert!(options.timestamp_map);
    }

    #[test]
    fn test_analyzevideo_enables_video_analysis() {
        let (options, _) = parse_args(&["--analyzevideo"]);
        assert!(options.analyze_video_stream);
    }

    #[test]
    fn test_screenfuls_sets_screens_to_process() {
        let (options, _) = parse_args(&["--screenfuls", "10"]);
        assert_eq!(options.settings_608.screens_to_process, 10);
    }

    #[test]
    fn test_xds_enables_xds_in_transcripts() {
        let (options, _) = parse_args(&["--xds"]);
        assert!(options.transcript_settings.xds);
    }

    #[test]
    fn test_ucla_enables_ucla_format() {
        let (options, _) = parse_args(&["--ucla"]);
        assert!(options.ucla);
        assert!(options.enc_cfg.no_bom);
        assert!(options.transcript_settings.show_start_time);
        assert!(options.transcript_settings.show_end_time);
    }

    #[test]
    fn test_tickertext_enables_ticker_search() {
        let (options, _) = parse_args(&["--tickertext"]);
        assert!(options.tickertext);
    }

    #[test]
    fn test_gui_mode_reports_enables_gui_mode() {
        let (options, _) = parse_args(&["--gui-mode-reports"]);
        assert!(options.gui_mode_reports);
        assert!(options.no_progress_bar);
    }

    #[test]
    fn test_no_progress_bar_disables_progress() {
        let (options, _) = parse_args(&["--no-progress-bar"]);
        assert!(options.no_progress_bar);
    }

    #[test]
    fn test_segmentonkeyonly_enables_keyframe_segmentation() {
        let (options, _) = parse_args(&["--segmentonkeyonly"]);
        assert!(options.segment_on_key_frames_only);
        assert!(options.analyze_video_stream);
    }

    #[test]
    fn test_outinterval_sets_output_interval() {
        let (options, _) = parse_args(&["--outinterval", "60"]);
        assert_eq!(options.out_interval, 60);
    }

    #[test]
    fn test_list_tracks_enables_track_listing() {
        let (options, _) = parse_args(&["--list-tracks"]);
        assert!(options.list_tracks_only);
    }

    #[test]
    fn test_ignoreptsjumps_enables_pts_jump_ignore() {
        let (options, _) = parse_args(&["--ignoreptsjumps"]);
        assert!(options.ignore_pts_jumps);
    }

    #[test]
    fn test_fixptsjumps_disables_pts_jump_ignore() {
        let (options, _) = parse_args(&["--fixptsjumps"]);
        assert!(!options.ignore_pts_jumps);
    }

    #[test]
    fn test_customtxt_sets_transcript_format() {
        let (options, _) = parse_args(&["--customtxt", "1100100"]);
        assert!(options.transcript_settings.show_start_time);
        assert!(options.transcript_settings.show_end_time);
        assert!(!options.transcript_settings.show_mode);
        assert!(!options.transcript_settings.show_cc);
        assert!(options.transcript_settings.relative_timestamp);
        assert!(!options.transcript_settings.xds);
        assert!(!options.transcript_settings.use_colors);
    }

    // =========================================================================
    // COMBINATION/INTEGRATION TESTS
    // =========================================================================

    #[test]
    fn test_autoprogram_with_srt_and_latin1() {
        let (options, _) = parse_args(&["--autoprogram", "--out", "srt", "--latin1"]);
        assert!(options.demux_cfg.ts_autoprogram);
        assert_eq!(options.write_format, OutputFormat::Srt);
        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
    }

    #[test]
    fn test_autoprogram_with_sami_and_latin1() {
        let (options, _) = parse_args(&["--autoprogram", "--out", "sami", "--latin1"]);
        assert!(options.demux_cfg.ts_autoprogram);
        assert_eq!(options.write_format, OutputFormat::Sami);
        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
    }

    #[test]
    fn test_autoprogram_with_ttxt_and_ucla_xds() {
        let (options, _) = parse_args(&[
            "--autoprogram",
            "--out",
            "ttxt",
            "--latin1",
            "--ucla",
            "--xds",
        ]);
        assert!(options.demux_cfg.ts_autoprogram);
        assert_eq!(options.write_format, OutputFormat::Transcript);
        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
        assert!(options.ucla);
        assert!(options.transcript_settings.xds);
    }

    #[test]
    fn test_service_with_txt_no_bom_no_rollup() {
        let (options, _) =
            parse_args(&["--service", "1", "--out", "txt", "--no-bom", "--no-rollup"]);
        assert!(options.is_708_enabled);
        assert!(options.enc_cfg.no_bom);
        assert!(options.no_rollup);
        assert_eq!(options.write_format, OutputFormat::Transcript);
    }

    #[test]
    fn test_autoprogram_with_teletext_and_datapid() {
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
        assert_eq!(options.write_format, OutputFormat::Srt);
        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
    }

    #[test]
    fn test_stdout_with_quiet_and_no_fontcolor() {
        let (options, _) = parse_args(&["--stdout", "--quiet", "--no-fontcolor"]);
        assert!(options.cc_to_stdout);
        assert_eq!(options.messages_target, OutputTarget::Quiet);
        assert!(options.nofontcolor);
    }

    #[test]
    fn test_wtvconvertfix_with_autoprogram_and_dvblang() {
        let (options, _) = parse_args(&[
            "--wtvconvertfix",
            "--autoprogram",
            "--out",
            "srt",
            "--latin1",
            "--dvblang",
            "eng",
        ]);
        assert!(options.wtvconvertfix);
        assert!(options.demux_cfg.ts_autoprogram);
        assert_eq!(options.write_format, OutputFormat::Srt);
        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
        assert_eq!(options.dvblang.unwrap(), Language::Eng);
    }

    #[test]
    fn test_ucla_with_autoprogram_and_output_field() {
        let (options, _) = parse_args(&[
            "--ucla",
            "--autoprogram",
            "--out",
            "ttxt",
            "--latin1",
            "--output-field",
            "2",
        ]);
        assert!(options.ucla);
        assert!(options.demux_cfg.ts_autoprogram);
        assert!(options.is_608_enabled);
        assert_eq!(options.write_format, OutputFormat::Transcript);
        assert_eq!(options.extract, 2);
        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
    }

    #[test]
    fn test_hauppauge_with_ucla_and_autoprogram() {
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
        assert_eq!(options.write_format, OutputFormat::Transcript);
        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
    }

    #[test]
    fn test_mp4_input_with_srt_and_latin1() {
        let (options, _) = parse_args(&["--input", "mp4", "--out", "srt", "--latin1"]);
        assert_eq!(options.demux_cfg.auto_stream, StreamMode::Mp4);
        assert_eq!(options.write_format, OutputFormat::Srt);
        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
    }

    #[test]
    fn test_autoprogram_with_bom_and_latin1() {
        let (options, _) = parse_args(&["--autoprogram", "--out", "srt", "--bom", "--latin1"]);
        assert!(options.demux_cfg.ts_autoprogram);
        assert!(!options.enc_cfg.no_bom);
        assert_eq!(options.write_format, OutputFormat::Srt);
        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
    }

    #[test]
    fn test_autoprogram_with_mp4vidtrack_and_ttxt() {
        let (options, _) = parse_args(&[
            "--autoprogram",
            "--out",
            "ttxt",
            "--mp4vidtrack",
            "--latin1",
        ]);
        assert!(options.demux_cfg.ts_autoprogram);
        assert!(options.mp4vidtrack);
        assert_eq!(options.write_format, OutputFormat::Transcript);
        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
    }

    #[test]
    fn test_codec_dvbsub_with_spupng_output() {
        let (options, _) = parse_args(&["--codec", "dvbsub", "--out", "spupng"]);
        assert_eq!(options.demux_cfg.codec, SelectCodec::Some(Codec::Dvb));
        assert_eq!(options.write_format, OutputFormat::SpuPng);
    }

    #[test]
    fn test_teletext_with_tpage_and_autoprogram() {
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
        assert_eq!(options.demux_cfg.codec, SelectCodec::Some(Codec::Teletext));
        assert_eq!(tlt_config.user_page, 398);
        assert_eq!(options.write_format, OutputFormat::Srt);
        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
    }

    #[test]
    fn test_datastreamtype_with_streamtype_and_no_autotimeref() {
        let (options, _) = parse_args(&[
            "--datastreamtype",
            "0x2",
            "--streamtype",
            "2",
            "--no-autotimeref",
        ]);
        assert!(options.noautotimeref);
        assert_eq!(options.demux_cfg.ts_datastreamtype, StreamType::VideoMpeg2);
        assert_eq!(
            options.demux_cfg.ts_forced_streamtype,
            StreamType::VideoMpeg2
        );
    }

    #[test]
    fn test_unicode_with_no_typesetting() {
        let (options, _) = parse_args(&["--unicode", "--no-typesetting"]);
        assert!(options.notypesetting);
        assert_eq!(options.enc_cfg.encoding, Encoding::UCS2);
    }

    #[test]
    fn test_utf8_with_trim() {
        let (options, _) = parse_args(&["--utf8", "--trim"]);
        assert!(options.enc_cfg.trim_subs);
        assert_eq!(options.enc_cfg.encoding, Encoding::UTF8);
    }

    #[test]
    fn test_autodash_with_trim() {
        let (options, _) = parse_args(&["--autodash", "--trim"]);
        assert!(options.enc_cfg.autodash);
        assert!(options.enc_cfg.trim_subs);
    }

    #[test]
    fn test_xmltv_with_null_output() {
        let (options, _) = parse_args(&["--xmltv", "1", "--out", "null"]);
        assert_eq!(options.xmltv, 1);
        assert_eq!(options.write_format, OutputFormat::Null);
    }
}
