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
    let mut val = bufsize[0..bufsize.len() - 1].parse::<i32>().unwrap();
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
        let line = line.unwrap();
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
fn mkvlang_params_check(lang: &str) {
    let mut initial = 0;
    let mut _present = 0;

    for (char_index, c) in lang.to_lowercase().chars().enumerate() {
        if c == ',' {
            _present = char_index;

            if _present - initial < 3 || _present - initial > 6 {
                fatal!(
                    cause = ExitCause::MalformedParameter;
                    "language codes should be xxx,xxx,xxx,....\n"
                );
            }

            if _present - initial == 6 {
                let sub_slice = &lang[initial.._present];
                if !sub_slice.contains('-') {
                    fatal!(
                        cause = ExitCause::MalformedParameter;
                        "language codes should be xxx,xxx,xxx,....\n"
                    );
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
        fatal!(
            cause = ExitCause::MalformedParameter;
            "last language code should be xxx.\n"
        );
    }

    if _present - initial == 5 {
        let sub_slice = &lang[initial.._present];
        if !sub_slice.contains('-') {
            fatal!(
                cause = ExitCause::MalformedParameter;
                "last language code is not of the form xxx-xx\n"
            );
        }
    }
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
                    self.date_format = TimestampFormat::HHMMSS;
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

    fn append_file_to_queue(&mut self, filename: &str, inputfile_capacity: &mut i32) -> i32 {
        if filename.is_empty() {
            return 0;
        }

        let num_input_files = if let Some(ref inputfile) = self.inputfile {
            inputfile.len()
        } else {
            0
        };
        if num_input_files >= *inputfile_capacity as usize {
            *inputfile_capacity += 10;
        }

        let new_size = (*inputfile_capacity).try_into().unwrap_or(0);

        if self.inputfile.is_none() {
            self.inputfile = Some(Vec::with_capacity(new_size));
        }

        if let Some(ref mut inputfile) = self.inputfile {
            inputfile.resize(new_size, String::new());

            let index = num_input_files;
            inputfile[index] = filename.to_string();
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
            self.dvblang = Some(Language::from_str(lang.as_str()).unwrap());
        }

        if let Some(ref ocrlang) = args.ocrlang {
            self.ocrlang = Some(Language::from_str(ocrlang.as_str()).unwrap());
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

        if let Some(ref lang) = args.mkvlang {
            self.mkvlang = Some(Language::from_str(lang.as_str()).unwrap());
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

        if let Some(ref tpage) = args.tpage {
            tlt_config.user_page = get_atoi_hex::<u16>(tpage.as_str()) as _;
            tlt_config.page = Cell::new(TeletextPageNumber::from(tlt_config.user_page));
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
        common::{OutputFormat, SelectCodec, StreamMode, StreamType},
        util::{encoding::Encoding, log::DebugMessageFlag},
    };

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

    #[test]
    fn broken_1() {
        let (options, _) = parse_args(&["--autoprogram", "--out", "srt", "--latin1"]);

        assert!(options.demux_cfg.ts_autoprogram);
        assert_eq!(options.write_format, OutputFormat::Srt);
        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
    }

    #[test]
    fn broken_2() {
        let (options, _) = parse_args(&["--autoprogram", "--out", "sami", "--latin1"]);

        assert!(options.demux_cfg.ts_autoprogram);
        assert_eq!(options.write_format, OutputFormat::Sami);
        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
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
        assert_eq!(options.write_format, OutputFormat::Transcript);
        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
        assert!(options.ucla);
        assert!(options.transcript_settings.xds);
    }

    #[test]
    fn broken_4() {
        let (options, _) = parse_args(&["--out", "ttxt", "--latin1"]);
        assert_eq!(options.write_format, OutputFormat::Transcript);

        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
    }

    #[test]
    fn broken_5() {
        let (options, _) = parse_args(&["--out", "srt", "--latin1"]);
        assert_eq!(options.write_format, OutputFormat::Srt);

        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
    }

    #[test]
    fn cea708_1() {
        let (options, _) =
            parse_args(&["--service", "1", "--out", "txt", "--no-bom", "--no-rollup"]);
        assert!(options.is_708_enabled);

        assert!(options.enc_cfg.no_bom);
        assert!(options.no_rollup);
        assert_eq!(options.write_format, OutputFormat::Transcript);
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
        assert_eq!(options.write_format, OutputFormat::Transcript);

        match options.enc_cfg.services_charsets {
            DtvccServiceCharset::None => {
                assert!(false);
            }
            DtvccServiceCharset::Same(_) => {
                assert!(false);
            }
            DtvccServiceCharset::Unique(charsets) => {
                assert_eq!(charsets[1], "UTF-8");
                assert_eq!(charsets[2], "EUC-KR");
            }
        }
    }

    #[test]
    fn cea708_3() {
        let (options, _) = parse_args(&["--service", "all[EUC-KR]", "--no-rollup"]);
        assert!(options.is_708_enabled);

        assert!(options.no_rollup);
        assert_eq!(
            options.enc_cfg.services_charsets,
            DtvccServiceCharset::Same("EUC-KR".to_string()),
        );
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
        assert_eq!(options.demux_cfg.ts_cappids.len(), 1);
        assert_eq!(options.write_format, OutputFormat::Srt);
        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
    }

    #[test]
    fn dvb_2() {
        let (options, _) = parse_args(&["--stdout", "--quiet", "--no-fontcolor"]);
        assert!(options.cc_to_stdout);

        assert_eq!(options.messages_target, OutputTarget::Quiet);
        assert!(options.nofontcolor);
    }

    #[test]
    fn dvd_1() {
        let (options, _) = parse_args(&["--autoprogram", "--out", "ttxt", "--latin1"]);
        assert!(options.demux_cfg.ts_autoprogram);

        assert_eq!(options.write_format, OutputFormat::Transcript);
        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
    }

    #[test]
    fn dvr_ms_1() {
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
    fn general_1() {
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
    fn general_2() {
        let (options, _) =
            parse_args(&["--autoprogram", "--out", "bin", "--latin1", "--sentencecap"]);
        assert!(options.demux_cfg.ts_autoprogram);

        assert!(options.enc_cfg.sentence_cap);
        assert_eq!(options.write_format, OutputFormat::Rcwt);
        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
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
        assert_eq!(options.write_format, OutputFormat::Transcript);
        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
    }

    #[test]
    fn mp4_1() {
        let (options, _) = parse_args(&["--input", "mp4", "--out", "srt", "--latin1"]);
        assert_eq!(options.demux_cfg.auto_stream, StreamMode::Mp4);

        assert_eq!(options.write_format, OutputFormat::Srt);
        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
    }

    #[test]
    fn mp4_2() {
        let (options, _) = parse_args(&["--autoprogram", "--out", "srt", "--bom", "--latin1"]);
        assert!(options.demux_cfg.ts_autoprogram);

        assert!(!options.enc_cfg.no_bom);
        assert_eq!(options.write_format, OutputFormat::Srt);
        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
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
        assert_eq!(options.write_format, OutputFormat::Transcript);
        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
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
        assert_eq!(options.write_format, OutputFormat::Transcript);
        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
    }

    #[test]
    fn options_1() {
        let (options, _) = parse_args(&["--input", "ts"]);

        assert_eq!(options.demux_cfg.auto_stream, StreamMode::Transport);
    }

    #[test]
    fn options_2() {
        let (options, _) = parse_args(&["--out", "dvdraw"]);
        assert_eq!(options.write_format, OutputFormat::DvdRaw);
    }

    #[test]
    fn options_3() {
        let (options, _) = parse_args(&["--goptime"]);
        assert_eq!(options.use_gop_as_pts, Some(true));
    }

    #[test]
    fn options_4() {
        let (options, _) = parse_args(&["--no-goptime"]);
        assert_eq!(options.use_gop_as_pts, Some(false));
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
        assert_eq!(options.auto_myth, Some(true));
    }

    #[test]
    fn options_8() {
        let (options, _) = parse_args(&["--program-number", "1"]);
        assert_eq!(options.demux_cfg.ts_forced_program, Some(1));
    }

    #[test]
    fn options_9() {
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
    fn options_10() {
        let (options, _) = parse_args(&["--unicode", "--no-typesetting"]);
        assert!(options.notypesetting);

        assert_eq!(options.enc_cfg.encoding, Encoding::UCS2);
    }

    #[test]
    fn options_11() {
        let (options, _) = parse_args(&["--utf8", "--trim"]);
        assert!(options.enc_cfg.trim_subs);

        assert_eq!(options.enc_cfg.encoding, Encoding::UTF8);
    }

    #[test]
    fn options_12() {
        let (options, _) = parse_args(&["--capfile", "Cargo.toml"]);

        assert!(options.enc_cfg.sentence_cap);
        assert_eq!(options.sentence_cap_file.to_str(), Some("Cargo.toml"));
    }

    #[test]
    fn options_13() {
        let (options, _) = parse_args(&["--unixts", "5", "--out", "txt"]);

        assert_eq!(*(UTC_REFVALUE.read().unwrap()), 5);

        assert_eq!(options.write_format, OutputFormat::Transcript);
    }

    #[test]
    fn options_14() {
        let (options, _) = parse_args(&["--datets", "--out", "txt"]);

        assert_eq!(
            options.date_format,
            TimestampFormat::Date {
                millis_separator: ','
            }
        );
        assert_eq!(options.write_format, OutputFormat::Transcript);
    }

    #[test]
    fn options_15() {
        let (options, _) = parse_args(&["--sects", "--out", "txt"]);

        assert_eq!(
            options.date_format,
            TimestampFormat::Seconds {
                millis_separator: ','
            },
        );
        assert_eq!(options.write_format, OutputFormat::Transcript);
    }

    #[test]
    fn options_16() {
        let (options, _) = parse_args(&["--lf", "--out", "txt"]);

        assert!(options.enc_cfg.line_terminator_lf);
        assert_eq!(options.write_format, OutputFormat::Transcript);
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

        assert_eq!(get_file_buffer_size(), 1024 * 1024);
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
        let (options, _) = parse_args(&["--ru1"]);

        assert_eq!(options.settings_608.force_rollup, 1);
    }

    #[test]
    fn options_24() {
        let (options, _) = parse_args(&["--delay", "200"]);

        assert_eq!(options.subs_delay, Timestamp::from_millis(200));
    }

    #[test]
    fn options_25() {
        let (options, _) = parse_args(&["--startat", "4", "--endat", "7"]);

        assert_eq!(options.extraction_start.unwrap_or_default().seconds(), 4);
    }

    #[test]
    fn options_26() {
        let (options, _) = parse_args(&["--no-codec", "dvbsub"]);

        assert_eq!(options.demux_cfg.nocodec, SelectCodec::Some(Codec::Dvb));
    }

    #[test]
    fn options_27() {
        let (options, _) = parse_args(&["--debug"]);

        assert_eq!(options.debug_mask.mask(), DebugMessageFlag::VERBOSE);
    }

    #[test]
    fn options_28() {
        let (options, _) = parse_args(&["--608"]);

        assert_eq!(options.debug_mask.mask(), DebugMessageFlag::DECODER_608);
    }

    #[test]
    fn options_29() {
        let (options, _) = parse_args(&["--708"]);

        assert_eq!(options.debug_mask.mask(), DebugMessageFlag::DECODER_708);
    }

    #[test]
    fn options_30() {
        let (options, _) = parse_args(&["--goppts"]);

        assert_eq!(options.debug_mask.mask(), DebugMessageFlag::TIME);
    }

    #[test]
    fn options_31() {
        let (options, _) = parse_args(&["--xdsdebug"]);

        assert_eq!(options.debug_mask.mask(), DebugMessageFlag::DECODER_XDS);
    }

    #[test]
    fn options_32() {
        let (options, _) = parse_args(&["--vides"]);

        assert_eq!(options.debug_mask.mask(), DebugMessageFlag::VIDEO_STREAM);
    }

    #[test]
    fn options_33() {
        let (options, _) = parse_args(&["--cbraw"]);

        assert_eq!(options.debug_mask.mask(), DebugMessageFlag::CB_RAW);
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

        assert_eq!(options.debug_mask.mask(), DebugMessageFlag::PARSE);
    }

    #[test]
    fn options_37() {
        let (options, _) = parse_args(&["--parsePAT"]);

        assert_eq!(options.debug_mask.mask(), DebugMessageFlag::PAT);
    }

    #[test]
    fn options_38() {
        let (options, _) = parse_args(&["--parsePMT"]);

        assert_eq!(options.debug_mask.mask(), DebugMessageFlag::PMT);
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

        assert_eq!(options.xmltv, 1);
        assert_eq!(options.write_format, OutputFormat::Null);
    }

    #[test]
    fn options_44() {
        let (options, _) = parse_args(&["--codec", "dvbsub", "--out", "spupng"]);

        assert_eq!(options.demux_cfg.codec, SelectCodec::Some(Codec::Dvb));
        assert_eq!(options.write_format, OutputFormat::SpuPng);
    }

    #[test]
    fn options_45() {
        let (options, _) = parse_args(&[
            "--startcreditsnotbefore",
            "1",
            "--startcreditstext",
            "CCextractor Start credit Testing",
        ]);

        assert_eq!(options.enc_cfg.startcreditsnotbefore.seconds(), 1);
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

        assert_eq!(options.enc_cfg.startcreditsnotafter.seconds(), 2);
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

        assert_eq!(options.enc_cfg.startcreditsforatleast.seconds(), 1);
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

        assert_eq!(options.enc_cfg.startcreditsforatmost.seconds(), 2);
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

        assert_eq!(options.enc_cfg.endcreditsforatleast.seconds(), 3);
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

        assert_eq!(options.enc_cfg.endcreditsforatmost.seconds(), 2);
        assert_eq!(
            options.enc_cfg.end_credits_text,
            "CCextractor Start credit Testing"
        );
    }

    #[test]
    fn options_51() {
        let (options, tlt_config) = parse_args(&["--tverbose"]);

        assert!(tlt_config.verbose);
        assert_eq!(options.debug_mask.mask(), DebugMessageFlag::TELETEXT);
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
        assert_eq!(options.demux_cfg.ts_cappids.len(), 1);
        assert_eq!(options.write_format, OutputFormat::Srt);
        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
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
        assert_eq!(options.demux_cfg.codec, SelectCodec::Some(Codec::Teletext));
        assert_eq!(tlt_config.user_page, 398);
        assert_eq!(options.write_format, OutputFormat::Srt);
        assert_eq!(options.enc_cfg.encoding, Encoding::Latin1);
    }
}
