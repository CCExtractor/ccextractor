use args::{Args, OutFormat};
use num_integer::Integer;

use std::fs::File;
use std::io::{prelude::*, BufReader};
use std::string::String;

use cfg_if::cfg_if;

use structs::CcxSOptions;
use time::OffsetDateTime;

cfg_if! {
    if #[cfg(windows)] {
        use std::os::windows::io::{AsRawHandle, FromRawHandle};
        use winapi::um::ioapiset::SetFileCompletionNotificationModes;
        use winapi::um::winbase::FILE_FLAG_OVERLAPPED;
        use winapi::um::winbase::{FILE_SKIP_COMPLETION_PORT_ON_SUCCESS, FILE_SKIP_SET_EVENT_ON_HANDLE};
        use winapi::um::winnt::{FILE_READ_ATTRIBUTES, FILE_SHARE_DELETE, FILE_SHARE_WRITE};
        use winapi::um::winnt::{FILE_SHARE_READ, GENERIC_READ, HANDLE};
    }
}
use crate::args::{self, InFormat, OutputField};
use crate::ccx_encoders_helpers::{
    CAPITALIZATION_LIST, CAPITALIZED_BUILTIN, PROFANE, PROFANE_BUILTIN,
};
use crate::structs;
use crate::{
    activity::activity_report_version,
    args::{Codec, Ru},
    enums::CcxDebugMessageTypes,
    structs::*,
};

cfg_if! {
    if #[cfg(target_os = "windows")] {
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
pub static mut MPEG_CLOCK_FREQ: i64 = 0;
static mut USERCOLOR_RGB: String = String::new();
pub static mut UTC_REFVALUE: u64 = 0;
const CCX_DECODER_608_SCREEN_WIDTH: u16 = 32;

#[cfg(windows)]
unsafe fn set_binary_mode() {
    let stdin_handle: HANDLE = libc::get_osfhandle(libc::STDIN_FILENO);

    SetFileCompletionNotificationModes(
        stdin_handle,
        FILE_SKIP_COMPLETION_PORT_ON_SUCCESS | FILE_SKIP_SET_EVENT_ON_HANDLE,
    );

    let mut flags: u32 = 0;
    let handle = libc::get_osfhandle(0);
    flags |= FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    let access_mode = GENERIC_READ;
    if access_mode != FILE_READ_ATTRIBUTES {
        flags |= FILE_FLAG_OVERLAPPED;
    }
    SetFileCompletionNotificationModes(
        handle as HANDLE,
        FILE_SKIP_COMPLETION_PORT_ON_SUCCESS | FILE_SKIP_SET_EVENT_ON_HANDLE,
    );
}

fn to_ms(value: &str) -> CcxBoundaryTime {
    let mut parts = value.rsplit(':');

    let seconds: u32 = parts
        .next()
        .unwrap_or_else(|| {
            println!("Malformed timecode: {}", value);
            std::process::exit(ExitCode::MalformedParameter as i32);
        })
        .parse()
        .unwrap();

    let mut minutes: u32 = 0;

    if let Some(mins) = parts.next() {
        minutes = mins.parse().unwrap();
    };
    let mut hours: u32 = 0;

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
        time_in_ms: hours + 60 * minutes + 3600 * seconds,
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

fn set_output_format_type(opt: &mut CcxSOptions, out_format: OutFormat) {
    match out_format {
        #[cfg(feature = "with_libcurl")]
        OutFormat::Curl => opt.write_format = CcxOutputFormat::Curl,
        OutFormat::Ass => opt.write_format = CcxOutputFormat::Ssa,
        OutFormat::Ccd => opt.write_format = CcxOutputFormat::Ccd,
        OutFormat::Scc => opt.write_format = CcxOutputFormat::Scc,
        OutFormat::Srt => opt.write_format = CcxOutputFormat::Srt,
        OutFormat::Ssa => opt.write_format = CcxOutputFormat::Ssa,
        OutFormat::Webvtt => opt.write_format = CcxOutputFormat::Webvtt,
        OutFormat::WebvttFull => {
            opt.write_format = CcxOutputFormat::Webvtt;
            opt.use_webvtt_styling = true;
        }
        OutFormat::Sami => opt.write_format = CcxOutputFormat::Sami,
        OutFormat::Txt => {
            opt.write_format = CcxOutputFormat::Transcript;
            opt.settings_dtvcc.no_rollup = true;
        }
        OutFormat::Ttxt => {
            opt.write_format = CcxOutputFormat::Transcript;
            if opt.date == CcxOutputDateFormat::None {
                opt.date = CcxOutputDateFormat::HhMmSsMs;
            }
            // Sets the right things so that timestamps and the mode are printed.
            if !opt.transcript_settings.is_final {
                opt.transcript_settings.show_start_time = true;
                opt.transcript_settings.show_end_time = true;
                opt.transcript_settings.show_cc = false;
                opt.transcript_settings.show_mode = true;
            }
        }
        OutFormat::Report => {
            opt.write_format = CcxOutputFormat::Null;
            opt.messages_target = Some(0);
            opt.print_file_reports = true;
            opt.demux_cfg.ts_allprogram = true;
        }
        OutFormat::Raw => opt.write_format = CcxOutputFormat::Raw,
        OutFormat::Smptett => opt.write_format = CcxOutputFormat::Smptett,
        OutFormat::Bin => opt.write_format = CcxOutputFormat::Rcwt,
        OutFormat::Null => opt.write_format = CcxOutputFormat::Null,
        OutFormat::Dvdraw => opt.write_format = CcxOutputFormat::Dvdraw,
        OutFormat::Spupng => opt.write_format = CcxOutputFormat::Spupng,
        // OutFormat::SimpleXml => opt.write_format = CcxOutputFormat::SimpleXml,
        OutFormat::G608 => opt.write_format = CcxOutputFormat::G608,
        OutFormat::Mcc => opt.write_format = CcxOutputFormat::Mcc,
    }
}

fn set_output_format(opt: &mut CcxSOptions, args: &Args) {
    opt.write_format_rewritten = true;

    if opt.send_to_srv && args.out.unwrap_or(OutFormat::Null) != OutFormat::Bin {
        println!("Output format is changed to bin\n");
        set_output_format_type(opt, OutFormat::Bin);
        return;
    }

    if let Some(out_format) = args.out {
        set_output_format_type(opt, out_format);
    } else if args.sami {
        set_output_format_type(opt, OutFormat::Sami);
    } else if args.webvtt {
        set_output_format_type(opt, OutFormat::Webvtt);
    } else if args.srt {
        set_output_format_type(opt, OutFormat::Srt);
    } else if args.null {
        set_output_format_type(opt, OutFormat::Null);
    } else if args.dvdraw {
        set_output_format_type(opt, OutFormat::Dvdraw);
    } else if args.txt {
        set_output_format_type(opt, OutFormat::Txt);
    } else if args.ttxt {
        set_output_format_type(opt, OutFormat::Ttxt);
    } else if args.mcc {
        set_output_format_type(opt, OutFormat::Mcc);
    }
}

fn set_input_format_type(opt: &mut CcxSOptions, input_format: InFormat) {
    match input_format {
        #[cfg(feature = "wtv_debug")]
        InFormat::Hex => opt.demux_cfg.auto_stream = CcxStreamMode::HexDump,
        InFormat::Es => opt.demux_cfg.auto_stream = CcxStreamMode::ElementaryOrNotFound,
        InFormat::Ts => opt.demux_cfg.auto_stream = CcxStreamMode::Transport,
        InFormat::M2ts => opt.demux_cfg.auto_stream = CcxStreamMode::Transport,
        InFormat::Ps => opt.demux_cfg.auto_stream = CcxStreamMode::Program,
        InFormat::Asf => opt.demux_cfg.auto_stream = CcxStreamMode::Asf,
        InFormat::Wtv => opt.demux_cfg.auto_stream = CcxStreamMode::Wtv,
        InFormat::Raw => opt.demux_cfg.auto_stream = CcxStreamMode::McpoodlesRaw,
        InFormat::Bin => opt.demux_cfg.auto_stream = CcxStreamMode::Rcwt,
        InFormat::Mp4 => opt.demux_cfg.auto_stream = CcxStreamMode::Mp4,
        InFormat::Mkv => opt.demux_cfg.auto_stream = CcxStreamMode::Mkv,
        InFormat::Mxf => opt.demux_cfg.auto_stream = CcxStreamMode::Mxf,
    }
}

fn set_input_format(opt: &mut CcxSOptions, args: &Args) {
    if opt.input_source == CcxDatasource::Tcp {
        println!("Input format is changed to bin\n");
        set_input_format_type(opt, InFormat::Bin);
        return;
    }

    if let Some(input_format) = args.input {
        set_input_format_type(opt, input_format);
    } else if args.es {
        set_input_format_type(opt, InFormat::Es);
    } else if args.ts {
        set_input_format_type(opt, InFormat::Ts);
    } else if args.ps {
        set_input_format_type(opt, InFormat::Ps);
    } else if args.asf || args.dvr_ms {
        set_input_format_type(opt, InFormat::Asf);
    } else if args.wtv {
        set_input_format_type(opt, InFormat::Wtv);
    } else {
        println!("Unknown input file format: {}\n", args.input.unwrap());
        std::process::exit(ExitCode::MalformedParameter as i32);
    }
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

fn parse_708_services(opts: &mut CcxSOptions, s: &str) {
    if s.starts_with("all") {
        let charset = if s.len() > 3 { &s[4..s.len() - 1] } else { "" };
        opts.settings_dtvcc.enabled = true;
        opts.enc_cfg.dtvcc_extract = true;
        opts.enc_cfg.all_services_charset = charset.to_owned();
        opts.enc_cfg.services_charsets = vec![String::new(); CCX_DTVCC_MAX_SERVICES];

        for i in 0..CCX_DTVCC_MAX_SERVICES {
            opts.settings_dtvcc.services_enabled[i] = true;
            opts.enc_cfg.services_enabled[i] = true;
        }

        opts.settings_dtvcc.active_services_count = CCX_DTVCC_MAX_SERVICES;
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

    opts.enc_cfg.services_charsets = vec![String::new(); CCX_DTVCC_MAX_SERVICES];

    for (i, service) in services.iter().enumerate() {
        let svc = service.parse::<usize>().unwrap();
        if !(1..=CCX_DTVCC_MAX_SERVICES).contains(&svc) {
            panic!("[CEA-708] Malformed parameter: Invalid service number ({}), valid range is 1-{}.\n", svc, CCX_DTVCC_MAX_SERVICES);
        }
        opts.settings_dtvcc.services_enabled[svc - 1] = true;
        opts.enc_cfg.services_enabled[svc - 1] = true;
        opts.settings_dtvcc.enabled = true;
        opts.enc_cfg.dtvcc_extract = true;
        opts.settings_dtvcc.active_services_count += 1;

        if charsets.len() > i && charsets[i].is_some() {
            opts.enc_cfg.services_charsets[svc - 1] = charsets[i].as_ref().unwrap().to_owned();
        }
    }

    if opts.settings_dtvcc.active_services_count == 0 {
        panic!("[CEA-708] Malformed parameter: no services\n");
    }
}

pub fn parse_parameters(opt: &mut CcxSOptions, args: &Args, tlt_config: &mut CcxSTeletextConfig) {
    if args.stdin {
        #[cfg(feature = "windows")]
        {
            set_binary_mode();
        }

        opt.input_source = CcxDatasource::Stdin;
        opt.live_stream = Some(-1);
    }

    #[cfg(feature = "hardsubx_ocr")]
    {
        if args.hardsubx {
            opt.hardsubx = true;

            if args.hcc {
                opt.hardsubx_and_common = true;
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
                opt.hardsubx_ocr_mode = value;
            }

            if let Some(ref subcolor) = args.subcolor {
                match subcolor.as_str() {
                    "white" => {
                        opt.hardsubx_subcolor = Some(HardsubxColorType::White as i32);
                        opt.hardsubx_hue = Some(0.0);
                    }
                    "yellow" => {
                        opt.hardsubx_subcolor = Some(HardsubxColorType::Yellow as i32);
                        opt.hardsubx_hue = Some(60.0);
                    }
                    "green" => {
                        opt.hardsubx_subcolor = Some(HardsubxColorType::Green as i32);
                        opt.hardsubx_hue = Some(120.0);
                    }
                    "cyan" => {
                        opt.hardsubx_subcolor = Some(HardsubxColorType::Cyan as i32);
                        opt.hardsubx_hue = Some(180.0);
                    }
                    "blue" => {
                        opt.hardsubx_subcolor = Some(HardsubxColorType::Blue as i32);
                        opt.hardsubx_hue = Some(240.0);
                    }
                    "magenta" => {
                        opt.hardsubx_subcolor = Some(HardsubxColorType::Magenta as i32);
                        opt.hardsubx_hue = Some(300.0);
                    }
                    "red" => {
                        opt.hardsubx_subcolor = Some(HardsubxColorType::Red as i32);
                        opt.hardsubx_hue = Some(0.0);
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
                        opt.hardsubx_subcolor = Some(HardsubxColorType::Custom as i32);
                        opt.hardsubx_hue = Some(hue);
                    }
                }
            }

            if let Some(ref value) = args.min_sub_duration {
                if *value == 0.0 {
                    println!("Invalid minimum subtitle duration");
                    std::process::exit(ExitCode::MalformedParameter as i32);
                }
                opt.hardsubx_min_sub_duration = Some(*value);
            }

            if args.detect_italics {
                opt.hardsubx_detect_italics = true;
            }

            if let Some(ref value) = args.conf_thresh {
                if !(0.0..=100.0).contains(value) {
                    println!("Invalid confidence threshold, valid values are between 0 & 100");
                    std::process::exit(ExitCode::MalformedParameter as i32);
                }
                opt.hardsubx_conf_thresh = Some(*value);
            }

            if let Some(ref value) = args.whiteness_thresh {
                if !(0.0..=100.0).contains(value) {
                    println!("Invalid whiteness threshold, valid values are between 0 & 100");
                    std::process::exit(ExitCode::MalformedParameter as i32);
                }
                opt.hardsubx_lum_thresh = Some(*value);
            }
        }
    } // END OF HARDSUBX

    if args.chapters {
        opt.extract_chapters = true;
    }

    if args.bufferinput {
        opt.buffer_input = true;
    }

    if args.no_bufferinput {
        opt.buffer_input = false;
    }

    if args.koc {
        opt.keep_output_closed = true;
    }

    if args.forceflush {
        opt.force_flush = true;
    }

    if args.append {
        opt.append_mode = true;
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
        opt.settings_608.direct_rollup = 1;
    }

    if args.no_fontcolor {
        opt.nofontcolor = true;
    }

    if args.no_htmlescape {
        opt.nohtmlescape = true;
    }

    if args.bom {
        opt.enc_cfg.no_bom = false;
    }
    if args.no_bom {
        opt.enc_cfg.no_bom = true;
    }

    if args.sem {
        opt.enc_cfg.with_semaphore = true;
    }

    if args.no_typesetting {
        opt.notypesetting = true;
    }

    if args.timestamp_map {
        opt.timestamp_map = true;
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
        set_input_format(opt, args);
    }

    if let Some(ref codec) = args.codec {
        match codec {
            Codec::Teletext => {
                opt.demux_cfg.codec = CcxCodeType::Teletext;
            }
            Codec::Dvbsub => {
                opt.demux_cfg.codec = CcxCodeType::Dvb;
            }
        }
    }

    if let Some(ref codec) = args.no_codec {
        match codec {
            Codec::Dvbsub => {
                opt.demux_cfg.nocodec = CcxCodeType::Dvb;
            }
            Codec::Teletext => {
                opt.demux_cfg.nocodec = CcxCodeType::Teletext;
            }
        }
    }

    if let Some(ref lang) = args.dvblang {
        opt.dvblang = Some(lang.clone());
    }

    if let Some(ref lang) = args.ocrlang {
        opt.ocrlang = Some(lang.clone());
    }

    if let Some(ref quant) = args.quant {
        if !(0..=2).contains(quant) {
            println!("Invalid quant value");
            std::process::exit(ExitCode::MalformedParameter as i32);
        }
        opt.ocr_quantmode = Some(*quant);
    }

    if args.no_spupngocr {
        opt.enc_cfg.nospupngocr = true;
    }

    if let Some(ref oem) = args.oem {
        if !(0..=2).contains(oem) {
            println!("Invalid oem value");
            std::process::exit(ExitCode::MalformedParameter as i32);
        }
        opt.ocr_oem = Some(*oem);
    }

    if let Some(ref lang) = args.mkvlang {
        opt.mkvlang = Some(lang.to_string());
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
        set_output_format(opt, args);
    }

    if let Some(ref startcreditstext) = args.startcreditstext {
        opt.enc_cfg.start_credits_text = startcreditstext.clone();
    }

    if let Some(ref startcreditsnotbefore) = args.startcreditsnotbefore {
        opt.enc_cfg.startcreditsnotbefore = to_ms(startcreditsnotbefore.clone().as_str());
    }

    if let Some(ref startcreditsnotafter) = args.startcreditsnotafter {
        opt.enc_cfg.startcreditsnotafter = to_ms(startcreditsnotafter.clone().as_str());
    }

    if let Some(ref startcreditsforatleast) = args.startcreditsforatleast {
        opt.enc_cfg.startcreditsforatleast = to_ms(startcreditsforatleast.clone().as_str());
    }
    if let Some(ref startcreditsforatmost) = args.startcreditsforatmost {
        opt.enc_cfg.startcreditsforatmost = to_ms(startcreditsforatmost.clone().as_str());
    }

    if let Some(ref endcreditstext) = args.endcreditstext {
        opt.enc_cfg.end_credits_text = endcreditstext.clone();
    }

    if let Some(ref endcreditsforatleast) = args.endcreditsforatleast {
        opt.enc_cfg.endcreditsforatleast = to_ms(endcreditsforatleast.clone().as_str());
    }

    if let Some(ref endcreditsforatmost) = args.endcreditsforatmost {
        opt.enc_cfg.endcreditsforatmost = to_ms(endcreditsforatmost.clone().as_str());
    }

    /* More stuff */
    if args.videoedited {
        opt.binary_concat = false;
    }

    if args.goptime {
        opt.use_gop_as_pts = 1;
    }
    if args.no_goptime {
        opt.use_gop_as_pts = -1;
    }

    if args.fixpadding {
        opt.fix_padding = true;
    }

    if args.mpeg90090 {
        unsafe {
            MPEG_CLOCK_FREQ = 90090;
        }
    }
    if args.no_scte20 {
        opt.noscte20 = true;
    }

    if args.webvtt_create_css {
        opt.webvtt_create_css = true;
    }

    if args.no_rollup {
        opt.no_rollup = true;
        opt.settings_608.no_rollup = true;
        opt.settings_dtvcc.no_rollup = true;
    }

    if let Some(ref ru) = args.rollup {
        match ru {
            Ru::Ru1 => {
                opt.settings_608.force_rollup = 1;
            }
            Ru::Ru2 => {
                opt.settings_608.force_rollup = 2;
            }
            Ru::Ru3 => {
                opt.settings_608.force_rollup = 3;
            }
        }
    }

    if args.trim {
        opt.enc_cfg.trim_subs = true;
    }

    if let Some(ref outinterval) = args.outinterval {
        opt.out_interval = Some(*outinterval);
    }

    if args.segmentonkeyonly {
        opt.segment_on_key_frames_only = true;
        opt.analyze_video_stream = true;
    }

    if args.gui_mode_reports {
        opt.gui_mode_reports = true;
    }

    if args.no_progress_bar {
        opt.no_progress_bar = true;
    }

    if args.splitbysentence {
        opt.enc_cfg.splitbysentence = true;
    }

    if args.sentencecap {
        opt.enc_cfg.sentence_cap = true;
    }

    if let Some(ref capfile) = args.capfile {
        opt.enc_cfg.sentence_cap = true;
        opt.sentence_cap_file = Some(capfile.clone());
    }

    if args.kf {
        opt.enc_cfg.filter_profanity = true;
    }

    if let Some(ref profanity_file) = args.profanity_file {
        opt.enc_cfg.filter_profanity = true;
        opt.filter_profanity_file = Some(profanity_file.clone());
    }

    if let Some(ref program_number) = args.program_number {
        opt.demux_cfg.ts_forced_program = get_atoi_hex(program_number.as_str());
        opt.demux_cfg.ts_forced_program_selected = true;
    }

    if args.autoprogram {
        opt.demux_cfg.ts_autoprogram = true;
    }

    if args.multiprogram {
        opt.multiprogram = true;
        opt.demux_cfg.ts_allprogram = true;
    }

    if let Some(ref stream) = args.stream {
        opt.live_stream = Some(get_atoi_hex(stream.as_str()));
    }

    if let Some(ref defaultcolor) = args.defaultcolor {
        unsafe {
            if defaultcolor.len() != 7 || !defaultcolor.starts_with('#') {
                println!("Invalid default color");
                std::process::exit(ExitCode::MalformedParameter as i32);
            }
            USERCOLOR_RGB = defaultcolor.clone();
            opt.settings_608.default_color = CcxDecoder608ColorCode::Userdefined;
        }
    }

    if let Some(ref delay) = args.delay {
        opt.subs_delay = *delay;
    }

    if let Some(ref screenfuls) = args.screenfuls {
        opt.settings_608.screens_to_process = get_atoi_hex(screenfuls.as_str());
    }

    if let Some(ref startat) = args.startat {
        opt.extraction_start = to_ms(startat.clone().as_str());
    }
    if let Some(ref endat) = args.endat {
        opt.extraction_end = to_ms(endat.clone().as_str());
    }

    if args.cc2 {
        opt.cc_channel = Some(2);
    }

    if let Some(ref extract) = args.output_field {
        opt.extract = Some(*extract);
        opt.is_608_enabled = true;
    }

    if args.stdout {
        if let Some(messages_target) = opt.messages_target {
            if messages_target == 1 {
                opt.messages_target = Some(2);
            }
        }
        opt.cc_to_stdout = true;
    }

    if args.pesheader {
        opt.pes_header_to_stdout = true;
    }

    if args.debugdvdsub {
        opt.debug_mask = CcxDebugMessageTypes::Dvb;
    }

    if args.ignoreptsjumps {
        opt.ignore_pts_jumps = true;
    }

    if args.fixptsjumps {
        opt.ignore_pts_jumps = false;
    }

    if args.quiet {
        opt.messages_target = Some(0);
    }

    if args.debug {
        opt.debug_mask = CcxDebugMessageTypes::Verbose;
    }

    if args.eia608 {
        opt.debug_mask = CcxDebugMessageTypes::Decoder608;
    }

    if args.deblev {
        opt.debug_mask = CcxDebugMessageTypes::Levenshtein;
    }

    if args.no_levdist {
        opt.dolevdist = 0;
    }

    if let Some(ref levdistmincnt) = args.levdistmincnt {
        opt.levdistmincnt = Some(get_atoi_hex(levdistmincnt.as_str()));
    }
    if let Some(ref levdistmaxpct) = args.levdistmaxpct {
        opt.levdistmaxpct = Some(get_atoi_hex(levdistmaxpct.as_str()));
    }

    if args.eia708 {
        opt.debug_mask = CcxDebugMessageTypes::Parse;
    }

    if args.goppts {
        opt.debug_mask = CcxDebugMessageTypes::Time;
    }

    if args.vides {
        opt.debug_mask = CcxDebugMessageTypes::Vides;
        opt.analyze_video_stream = true;
    }

    if args.analyzevideo {
        opt.analyze_video_stream = true;
    }

    if args.xds {
        opt.transcript_settings.xds = true;
    }
    if args.xdsdebug {
        opt.transcript_settings.xds = true;
        opt.debug_mask = CcxDebugMessageTypes::DecoderXds;
    }

    if args.parsedebug {
        opt.debug_mask = CcxDebugMessageTypes::Parse;
    }

    if args.parse_pat {
        opt.debug_mask = CcxDebugMessageTypes::Pat;
    }

    if args.parse_pmt {
        opt.debug_mask = CcxDebugMessageTypes::Pmt;
    }

    if args.dumpdef {
        opt.debug_mask = CcxDebugMessageTypes::Dumpdef;
    }

    if args.investigate_packets {
        opt.investigate_packets = true;
    }

    if args.cbraw {
        opt.debug_mask = CcxDebugMessageTypes::Cbraw;
    }

    if args.tverbose {
        opt.debug_mask = CcxDebugMessageTypes::Teletext;
        tlt_config.verbose = true;
    }

    #[cfg(feature = "enable_sharing")]
    {
        if args.sharing_debug {
            opt.debug_mask = CcxDebugMessageTypes::Share;
            tlt_config.verbose = true;
        }
    }

    if args.fullbin {
        opt.fullbin = true;
    }

    if args.no_sync {
        opt.nosync = true;
    }

    if args.hauppauge {
        opt.hauppauge_mode = true;
    }

    if args.mp4vidtrack {
        opt.mp4vidtrack = true;
    }

    if args.unicode {
        opt.enc_cfg.encoding = CcxEncodingType::Unicode;
    }

    if args.utf8 {
        opt.enc_cfg.encoding = CcxEncodingType::Utf8;
    }

    if args.latin1 {
        opt.enc_cfg.encoding = CcxEncodingType::Latin1;
    }
    if args.usepicorder {
        opt.usepicorder = true;
    }

    if args.myth {
        opt.auto_myth = Some(1);
    }
    if args.no_myth {
        opt.auto_myth = Some(0);
    }

    if args.wtvconvertfix {
        opt.wtvconvertfix = true;
    }

    if args.wtvmpeg2 {
        opt.wtvmpeg2 = true;
    }

    if let Some(ref output) = args.output {
        opt.output_filename = Some(output.clone());
    }

    if let Some(ref service) = args.cea708services {
        opt.is_708_enabled = true;
        parse_708_services(opt, service);
    }

    if let Some(ref datapid) = args.datapid {
        opt.demux_cfg.ts_cappids[opt.demux_cfg.nb_ts_cappid] = get_atoi_hex(datapid.as_str());
        opt.demux_cfg.nb_ts_cappid += 1;
    }

    if let Some(ref datastreamtype) = args.datastreamtype {
        opt.demux_cfg.ts_datastreamtype = datastreamtype.clone().parse().unwrap();
    }

    if let Some(ref streamtype) = args.streamtype {
        opt.demux_cfg.ts_forced_streamtype = streamtype.clone().parse().unwrap();
    }

    if let Some(ref tpage) = args.tpage {
        tlt_config.page = get_atoi_hex(tpage.as_str());
        tlt_config.user_page = tlt_config.page;
    }

    // Red Hen/ UCLA Specific stuff
    if args.ucla {
        opt.ucla = true;
        opt.millis_separator = '.';
        opt.enc_cfg.no_bom = true;

        if !opt.transcript_settings.is_final {
            opt.transcript_settings.show_start_time = true;
            opt.transcript_settings.show_end_time = true;
            opt.transcript_settings.show_cc = true;
            opt.transcript_settings.show_mode = true;
            opt.transcript_settings.relative_timestamp = false;
            opt.transcript_settings.is_final = true;
        }
    }

    if args.latrusmap {
        tlt_config.latrusmap = true;
    }

    if args.tickertext {
        opt.tickertext = true;
    }

    if args.lf {
        opt.enc_cfg.line_terminator_lf = true;
    }

    if args.df {
        opt.enc_cfg.force_dropframe = true;
    }

    if args.no_autotimeref {
        opt.noautotimeref = true;
    }

    if args.autodash {
        opt.enc_cfg.autodash = true;
    }

    if let Some(ref xmltv) = args.xmltv {
        opt.xmltv = Some(get_atoi_hex(xmltv.as_str()));
    }

    if let Some(ref xmltvliveinterval) = args.xmltvliveinterval {
        opt.xmltvliveinterval = Some(get_atoi_hex(xmltvliveinterval.as_str()));
    }

    if let Some(ref xmltvoutputinterval) = args.xmltvoutputinterval {
        opt.xmltvoutputinterval = Some(get_atoi_hex(xmltvoutputinterval.as_str()));
    }
    if let Some(ref xmltvonlycurrent) = args.xmltvonlycurrent {
        opt.xmltvonlycurrent = Some(get_atoi_hex(xmltvonlycurrent.as_str()));
    }

    if let Some(ref unixts) = args.unixts {
        let mut t = get_atoi_hex(unixts.as_str());

        if t == 0 {
            t = OffsetDateTime::now_utc().unix_timestamp() as u64;
        }
        unsafe {
            UTC_REFVALUE = t;
        }
        opt.noautotimeref = true;
    }

    if args.sects {
        opt.date = CcxOutputDateFormat::Seconds;
    }

    if args.datets {
        opt.date = CcxOutputDateFormat::Date;
    }

    if args.teletext {
        opt.demux_cfg.codec = CcxCodeType::Teletext;
    }

    if args.no_teletext {
        opt.demux_cfg.nocodec = CcxCodeType::Teletext;
    }

    if let Some(ref customtxt) = args.customtxt {
        if customtxt.to_string().len() == 7 {
            if opt.date == CcxOutputDateFormat::None {
                opt.date = CcxOutputDateFormat::HhMmSsMs;
            }

            if !opt.transcript_settings.is_final {
                let chars = format!("{}", customtxt).chars().collect::<Vec<char>>();
                opt.transcript_settings.show_start_time = chars[0] == '1';
                opt.transcript_settings.show_end_time = chars[1] == '1';
                opt.transcript_settings.show_mode = chars[2] == '1';
                opt.transcript_settings.show_cc = chars[3] == '1';
                opt.transcript_settings.relative_timestamp = chars[4] == '1';
                opt.transcript_settings.xds = chars[5] == '1';
                opt.transcript_settings.use_colors = chars[6] == '1';
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

            opt.udpsrc = Some(udp.clone());
            opt.udpaddr = Some(addr.to_owned());
            opt.udpport = Some(port.parse().unwrap());
        } else if let Some(colon) = udp.find(':') {
            let addr = &udp[0..colon];
            let port = get_atoi_hex(&udp[colon + 1..]);

            opt.udpsrc = Some(udp.clone());
            opt.udpaddr = Some(addr.to_owned());
            opt.udpport = Some(port);
        } else {
            opt.udpaddr = None;
            opt.udpport = Some(udp.parse().unwrap());
        }

        opt.input_source = CcxDatasource::Network;
    }

    if let Some(ref addr) = args.sendto {
        opt.send_to_srv = true;
        set_output_format_type(opt, OutFormat::Bin);

        opt.xmltv = Some(2);
        opt.xmltvliveinterval = Some(2);
        let mut _addr: String = addr.to_string();

        if let Some(saddr) = addr.strip_prefix('[') {
            _addr = saddr.to_string();

            let mut br = _addr
                .find(']')
                .expect("Wrong address format, for IPv6 use [address]:port");
            _addr = _addr.replace(']', "");

            opt.srv_addr = Some(_addr.clone());

            br += 1;
            if !_addr[br..].is_empty() {
                opt.srv_port = Some(_addr[br..].parse().unwrap());
            }
        }

        opt.srv_addr = Some(_addr.clone());

        let colon = _addr.find(':').unwrap();
        _addr = _addr.replace(':', "");
        opt.srv_port = Some(_addr[(colon + 1)..].parse().unwrap());
    }

    if let Some(ref tcp) = args.tcp {
        opt.tcpport = Some(*tcp);
        opt.input_source = CcxDatasource::Tcp;
        set_input_format_type(opt, InFormat::Bin);
    }

    if let Some(ref tcppassworrd) = args.tcp_password {
        opt.tcp_password = Some(tcppassworrd.to_string());
    }

    if let Some(ref tcpdesc) = args.tcp_description {
        opt.tcp_desc = Some(tcpdesc.to_string());
    }

    if let Some(ref font) = args.font {
        opt.enc_cfg.render_font = font.to_string();
    }

    if let Some(ref italics) = args.italics {
        opt.enc_cfg.render_font_italics = italics.to_string();
    }

    #[cfg(feature = "with_libcurl")]
    if let Some(ref curlposturl) = args.curlposturl {
        opt.curlposturl = curlposturl.to_string();
    }

    #[cfg(feature = "enable_sharing")]
    {
        if args.enable_sharing {
            opt.sharing_enabled = true;
        }

        if let Some(ref sharingurl) = args.sharing_url {
            opt.sharing_url = sharingurl.to_string();
        }

        if let Some(ref translate) = args.translate {
            opt.translate_enabled = true;
            opt.sharing_enabled = true;
            opt.translate_langs = translate.to_string();
        }

        if let Some(ref translateauth) = args.translate_auth {
            opt.translate_key = translateauth.to_string();
        }
    }

    if opt.demux_cfg.auto_stream == CcxStreamMode::Mp4 && opt.input_source == CcxDatasource::Stdin {
        println!("MP4 requires an actual file, it's not possible to read from a stream, including stdin.");
        std::process::exit(ExitCode::IncompatibleParameters as i32);
    }

    if opt.extract_chapters {
        println!("Request to extract chapters recieved.");
        println!("Note that this must only be used with MP4 files,");
        println!("for other files it will simply generate subtitles file.\n");
    }

    if opt.gui_mode_reports {
        opt.no_progress_bar = true;
        // Do it as soon as possible, because it something fails we might not have a chance
        activity_report_version(opt);
    }

    if opt.enc_cfg.sentence_cap {
        unsafe {
            CAPITALIZATION_LIST = get_vector_words(&CAPITALIZED_BUILTIN);
            if let Some(ref sentence_cap_file) = opt.sentence_cap_file {
                process_word_file(sentence_cap_file, &mut CAPITALIZATION_LIST)
                    .expect("There was an error processing the capitalization file.\n");
            }
        }
    }
    if opt.enc_cfg.filter_profanity {
        unsafe {
            PROFANE = get_vector_words(&PROFANE_BUILTIN);
            if let Some(ref profanityfile) = opt.filter_profanity_file {
                process_word_file(profanityfile.as_str(), &mut PROFANE)
                    .expect("There was an error processing the profanity file.\n");
            }
        }
    }

    if opt.demux_cfg.ts_forced_program == -1 {
        opt.demux_cfg.ts_forced_program_selected = true;
    }

    // Init telexcc redundant options
    tlt_config.dolevdist = opt.dolevdist;
    tlt_config.levdistmincnt = opt.levdistmincnt.unwrap_or(0);
    tlt_config.levdistmaxpct = opt.levdistmaxpct.unwrap_or(0);
    tlt_config.extraction_start = opt.extraction_start.clone();
    tlt_config.extraction_end = opt.extraction_end.clone();
    tlt_config.write_format = opt.write_format.clone();
    tlt_config.gui_mode_reports = opt.gui_mode_reports;
    tlt_config.date_format = opt.date;
    tlt_config.noautotimeref = opt.noautotimeref;
    tlt_config.send_to_srv = opt.send_to_srv;
    tlt_config.nofontcolor = opt.nofontcolor;
    tlt_config.nohtmlescape = opt.nohtmlescape;
    tlt_config.millis_separator = opt.millis_separator;

    // teletext page number out of range
    if tlt_config.page != 0 && (tlt_config.page < 100 || tlt_config.page > 899) {
        println!("Teletext page number out of range (100-899)");
        std::process::exit(ExitCode::NotClassified as i32);
    }

    if opt.num_input_files.unwrap_or(0) == 0 && opt.input_source == CcxDatasource::File {
        std::process::exit(ExitCode::NoInputFiles as i32);
    }

    if opt.num_input_files.is_some() && opt.live_stream.is_some() {
        println!("Live stream mode only supports one input file");
        std::process::exit(ExitCode::TooManyInputFiles as i32);
    }

    if opt.num_input_files.is_some() && opt.input_source == CcxDatasource::Network {
        println!("UDP mode is not compatible with input files");
        std::process::exit(ExitCode::TooManyInputFiles as i32);
    }

    if opt.input_source == CcxDatasource::Network || opt.input_source == CcxDatasource::Tcp {
        // TODO(prateekmedia): Check why we use ccx_options instead of opts
        // currently using same]
        opt.buffer_input = true;
    }

    if opt.num_input_files.is_some() && opt.input_source == CcxDatasource::Tcp {
        println!("TCP mode is not compatible with input files");
        std::process::exit(ExitCode::TooManyInputFiles as i32);
    }

    if opt.demux_cfg.auto_stream == CcxStreamMode::McpoodlesRaw
        && opt.write_format == CcxOutputFormat::Raw
    {
        println!("-in=raw can only be used if the output is a subtitle file.");
        std::process::exit(ExitCode::IncompatibleParameters as i32);
    }

    if opt.demux_cfg.auto_stream == CcxStreamMode::Rcwt
        && opt.write_format == CcxOutputFormat::Rcwt
        && opt.output_filename.is_none()
    {
        println!("CCExtractor's binary format can only be used simultaneously for input and\noutput if the output file name is specified given with -o.\n");
        std::process::exit(ExitCode::IncompatibleParameters as i32);
    }

    if opt.write_format != CcxOutputFormat::Dvdraw
        && opt.cc_to_stdout
        && opt.extract.is_some()
        && opt.extract.unwrap() == OutputField::Both
    {
        println!("You can't extract both fields to stdout at the same time in broadcast mode.\n",);
        std::process::exit(ExitCode::IncompatibleParameters as i32);
    }

    if opt.write_format == CcxOutputFormat::Spupng && opt.cc_to_stdout {
        println!("You cannot use -out=spupng with -stdout.\n");
        std::process::exit(ExitCode::IncompatibleParameters as i32);
    }

    if opt.write_format == CcxOutputFormat::Webvtt && opt.enc_cfg.encoding != CcxEncodingType::Utf8
    {
        opt.enc_cfg.encoding = CcxEncodingType::Utf8;
        println!("Note: Output format is WebVTT, forcing UTF-8");
        std::process::exit(ExitCode::IncompatibleParameters as i32);
    }

    // Check WITH_LIBCURL
    #[cfg(feature = "with_libcurl")]
    {
        if opt.write_format == CCX_OF_CURL && opt.curlposturl.is_none() {
            Err("You must pass a URL (-curlposturl) if output format is curl\n")
        }
        if opt.write_format != CCX_OF_CURL && opt.curlposturl.is_some() {
            Err("-curlposturl requires that the format is curl\n")
        }
    }

    // Initialize some Encoder Configuration
    opt.enc_cfg.extract = opt.extract;
    if opt.num_input_files.unwrap_or(0) > 0 {
        opt.enc_cfg.multiple_files = true;
        opt.enc_cfg.first_input_file = opt.inputfile.as_ref().unwrap()[0].to_string();
    }
    opt.enc_cfg.cc_to_stdout = opt.cc_to_stdout;
    opt.enc_cfg.write_format = opt.write_format.clone();
    opt.enc_cfg.send_to_srv = opt.send_to_srv;
    opt.enc_cfg.date_format = opt.date;
    opt.enc_cfg.transcript_settings = opt.transcript_settings.clone();
    opt.enc_cfg.millis_separator = opt.millis_separator;
    opt.enc_cfg.no_font_color = opt.nofontcolor;
    opt.enc_cfg.force_flush = opt.force_flush;
    opt.enc_cfg.append_mode = opt.append_mode;
    opt.enc_cfg.ucla = opt.ucla;
    opt.enc_cfg.no_type_setting = opt.notypesetting;
    opt.enc_cfg.subs_delay = opt.subs_delay;
    opt.enc_cfg.gui_mode_reports = opt.gui_mode_reports;

    if opt.enc_cfg.render_font.is_empty() {
        opt.enc_cfg.render_font = DEFAULT_FONT_PATH.to_string();
    }

    if opt.enc_cfg.render_font_italics.is_empty() {
        opt.enc_cfg.render_font = DEFAULT_FONT_PATH_ITALICS.to_string();
    }

    if opt.output_filename.is_some() && opt.multiprogram {
        opt.enc_cfg.output_filename = opt.output_filename.clone().unwrap();
    }

    if !opt.is_608_enabled && !opt.is_708_enabled {
        // If nothing is selected then extract both 608 and 708 subs
        // 608 field 1 is enabled by default
        // Enable 708 subs extraction
        parse_708_services(opt, "all");
    } else if !opt.is_608_enabled && opt.is_708_enabled {
        // Extract only 708 subs
        // 608 field 1 is enabled by default, disable it
        opt.extract = None;
        opt.enc_cfg.extract_only_708 = true;
    }

    // Check WITH_LIBCURL
    #[cfg(feature = "with_libcurl")]
    {
        opt.enc_cfg.curlposturl = opt.curlposturl.clone();
    }

    println!("Issues? Open a ticket here\n https://github.com/CCExtractor/ccextractor/issues");
}

#[cfg(test)]
pub mod tests {
    use crate::{args::*, enums::*, params::*, structs::*};
    use clap::Parser;

    #[test]
    fn broken_1() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--autoprogram",
            "--out",
            "srt",
            "--latin1",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.demux_cfg.ts_autoprogram);
        assert_eq!(opt.write_format, CcxOutputFormat::Srt);
        assert_eq!(opt.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn broken_2() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--autoprogram",
            "--out",
            "sami",
            "--latin1",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.demux_cfg.ts_autoprogram);
        assert_eq!(opt.write_format, CcxOutputFormat::Sami);
        assert_eq!(opt.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn broken_3() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--autoprogram",
            "--out",
            "ttxt",
            "--latin1",
            "--ucla",
            "--xds",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.demux_cfg.ts_autoprogram);
        assert_eq!(opt.write_format, CcxOutputFormat::Transcript);
        assert_eq!(opt.enc_cfg.encoding, CcxEncodingType::Latin1);
        assert!(opt.ucla);
        assert!(opt.transcript_settings.xds);
    }

    #[test]
    fn broken_4() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--out",
            "ttxt",
            "--latin1",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.write_format, CcxOutputFormat::Transcript);
        assert_eq!(opt.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn broken_5() {
        let args: Args =
            Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--out", "srt", "--latin1"])
                .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.write_format, CcxOutputFormat::Srt);
        assert_eq!(opt.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn cea708_1() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--service",
            "1",
            "--out",
            "txt",
            "--no-bom",
            "--no-rollup",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.is_708_enabled);
        assert!(opt.enc_cfg.no_bom);
        assert!(opt.no_rollup);
        assert_eq!(opt.write_format, CcxOutputFormat::Transcript);
    }

    #[test]
    fn cea708_2() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--service",
            "1,2[UTF-8],3[EUC-KR],54",
            "--out",
            "txt",
            "--no-rollup",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        println!("{:?}", opt.enc_cfg.services_charsets);

        assert!(opt.is_708_enabled);
        assert!(opt.no_rollup);
        assert_eq!(opt.write_format, CcxOutputFormat::Transcript);
        assert_eq!(opt.enc_cfg.services_charsets[1], "UTF-8");
        assert_eq!(opt.enc_cfg.services_charsets[2], "EUC-KR");
    }

    #[test]
    fn cea708_3() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--service",
            "all[EUC-KR]",
            "--no-rollup",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.is_708_enabled);
        assert!(opt.no_rollup);
        assert_eq!(opt.enc_cfg.all_services_charset, "EUC-KR");
    }

    #[test]
    fn dvb_1() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--autoprogram",
            "--out",
            "srt",
            "--latin1",
            "--teletext",
            "--datapid",
            "5603",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.demux_cfg.ts_autoprogram);
        assert_eq!(opt.demux_cfg.ts_cappids[0], 5603);
        assert_eq!(opt.demux_cfg.nb_ts_cappid, 1);
        assert_eq!(opt.write_format, CcxOutputFormat::Srt);
        assert_eq!(opt.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn dvb_2() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--stdout",
            "--quiet",
            "--no-fontcolor",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.cc_to_stdout);
        assert_eq!(opt.messages_target.unwrap(), 0);
        assert_eq!(opt.nofontcolor, true);
    }

    #[test]
    fn dvd_1() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--autoprogram",
            "--out",
            "ttxt",
            "--latin1",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.demux_cfg.ts_autoprogram);
        assert_eq!(opt.write_format, CcxOutputFormat::Transcript);
        assert_eq!(opt.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn dvr_ms_1() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--wtvconvertfix",
            "--autoprogram",
            "--out",
            "srt",
            "--latin1",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.wtvconvertfix);
        assert!(opt.demux_cfg.ts_autoprogram);
        assert_eq!(opt.write_format, CcxOutputFormat::Srt);
        assert_eq!(opt.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn general_1() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--ucla",
            "--autoprogram",
            "--out",
            "ttxt",
            "--latin1",
            "--output-field",
            "field2",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.ucla);
        assert!(opt.demux_cfg.ts_autoprogram);
        assert!(opt.is_608_enabled);
        assert_eq!(opt.write_format, CcxOutputFormat::Transcript);
        assert_eq!(opt.extract.unwrap(), OutputField::Field2);
        assert_eq!(opt.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn general_2() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--autoprogram",
            "--out",
            "bin",
            "--latin1",
            "--sentencecap",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.demux_cfg.ts_autoprogram);
        assert!(opt.enc_cfg.sentence_cap);
        assert_eq!(opt.write_format, CcxOutputFormat::Rcwt);
        assert_eq!(opt.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn haup_1() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--hauppauge",
            "--ucla",
            "--autoprogram",
            "--out",
            "ttxt",
            "--latin1",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.ucla);
        assert!(opt.hauppauge_mode);
        assert!(opt.demux_cfg.ts_autoprogram);
        assert_eq!(opt.write_format, CcxOutputFormat::Transcript);
        assert_eq!(opt.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn mp4_1() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--input",
            "mp4",
            "--out",
            "srt",
            "--latin1",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.demux_cfg.auto_stream, CcxStreamMode::Mp4);
        assert_eq!(opt.write_format, CcxOutputFormat::Srt);
        assert_eq!(opt.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn mp4_2() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--autoprogram",
            "--out",
            "srt",
            "--bom",
            "--latin1",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.demux_cfg.ts_autoprogram);
        assert!(!opt.enc_cfg.no_bom);
        assert_eq!(opt.write_format, CcxOutputFormat::Srt);
        assert_eq!(opt.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn nocc_1() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--autoprogram",
            "--out",
            "ttxt",
            "--mp4vidtrack",
            "--latin1",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.demux_cfg.ts_autoprogram);
        assert!(opt.mp4vidtrack);
        assert_eq!(opt.write_format, CcxOutputFormat::Transcript);
        assert_eq!(opt.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn nocc_2() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--autoprogram",
            "--out",
            "ttxt",
            "--latin1",
            "--ucla",
            "--xds",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.demux_cfg.ts_autoprogram);
        assert!(opt.ucla);
        assert!(opt.transcript_settings.xds);
        assert_eq!(opt.write_format, CcxOutputFormat::Transcript);
        assert_eq!(opt.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn options_1() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--input", "ts"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.demux_cfg.auto_stream, CcxStreamMode::Transport);
    }

    #[test]
    fn options_2() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--out", "dvdraw"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.write_format, CcxOutputFormat::Dvdraw);
    }

    #[test]
    fn options_3() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--goptime"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.use_gop_as_pts, 1);
    }

    #[test]
    fn options_4() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--no-goptime"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.use_gop_as_pts, -1);
    }

    #[test]
    fn options_5() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--fixpadding"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.fix_padding);
    }

    #[test]
    fn options_6() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--90090"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        unsafe {
            assert_eq!(MPEG_CLOCK_FREQ, 90090);
        }
    }

    #[test]
    fn options_7() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--myth"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.auto_myth.unwrap(), 1);
    }

    #[test]
    fn options_8() {
        let args: Args =
            Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--program-number", "1"])
                .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.demux_cfg.ts_forced_program, 1);
    }

    #[test]
    fn options_9() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--datastreamtype",
            "2",
            "--streamtype",
            "2",
            "--no-autotimeref",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.noautotimeref);
        assert_eq!(opt.demux_cfg.ts_datastreamtype, 2);
        assert_eq!(opt.demux_cfg.ts_forced_streamtype, 2);
    }
    #[test]
    fn options_10() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--unicode",
            "--no-typesetting",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.notypesetting);
        assert_eq!(opt.enc_cfg.encoding, CcxEncodingType::Unicode);
    }

    #[test]
    fn options_11() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--utf8", "--trim"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.enc_cfg.trim_subs);
        assert_eq!(opt.enc_cfg.encoding, CcxEncodingType::Utf8);
    }

    #[test]
    fn options_12() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--capfile",
            "tests/resources/dictionary.txt",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.enc_cfg.sentence_cap);
        assert_eq!(
            opt.sentence_cap_file.unwrap(),
            "tests/resources/dictionary.txt"
        );
    }

    #[test]
    fn options_13() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--unixts",
            "5",
            "--out",
            "txt",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);
        unsafe {
            assert_eq!(UTC_REFVALUE, 5);
        }

        assert_eq!(opt.write_format, CcxOutputFormat::Transcript);
    }

    #[test]
    fn options_14() {
        let args: Args =
            Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--datets", "--out", "txt"])
                .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.date, CcxOutputDateFormat::Date);
        assert_eq!(opt.write_format, CcxOutputFormat::Transcript);
    }

    #[test]
    fn options_15() {
        let args: Args =
            Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--sects", "--out", "txt"])
                .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.date, CcxOutputDateFormat::Seconds);
        assert_eq!(opt.write_format, CcxOutputFormat::Transcript);
    }

    #[test]
    fn options_16() {
        let args: Args =
            Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--lf", "--out", "txt"])
                .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.enc_cfg.line_terminator_lf);
        assert_eq!(opt.write_format, CcxOutputFormat::Transcript);
    }

    #[test]
    fn options_17() {
        let args: Args =
            Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--autodash", "--trim"])
                .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.enc_cfg.autodash);
        assert!(opt.enc_cfg.trim_subs);
    }

    #[test]
    fn options_18() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--bufferinput"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.buffer_input);
    }

    #[test]
    fn options_19() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--no-bufferinput"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(!opt.buffer_input);
    }

    #[test]
    fn options_20() {
        let args: Args =
            Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--buffersize", "1M"])
                .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        unsafe {
            assert_eq!(FILEBUFFERSIZE, 1024 * 1024);
        }
    }

    #[test]
    fn options_21() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--dru"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.settings_608.direct_rollup, 1);
    }

    #[test]
    fn options_22() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--no-rollup"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.no_rollup);
    }

    #[test]
    fn options_23() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--rollup", "ru1"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.settings_608.force_rollup, 1);
    }

    #[test]
    fn options_24() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--delay", "200"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.subs_delay, 200);
    }

    #[test]
    fn options_25() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--startat",
            "4",
            "--endat",
            "7",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.extraction_start.ss, 4);
    }

    #[test]
    fn options_26() {
        let args: Args =
            Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--no-codec", "dvbsub"])
                .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.demux_cfg.nocodec, CcxCodeType::Dvb);
    }

    #[test]
    fn options_27() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--debug"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.debug_mask, CcxDebugMessageTypes::Verbose);
    }

    #[test]
    fn options_28() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--608"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.debug_mask, CcxDebugMessageTypes::Decoder608);
    }

    #[test]
    fn options_29() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--708"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.debug_mask, CcxDebugMessageTypes::Parse);
    }

    #[test]
    fn options_30() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--goppts"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.debug_mask, CcxDebugMessageTypes::Time);
    }

    #[test]
    fn options_31() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--xdsdebug"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.debug_mask, CcxDebugMessageTypes::DecoderXds);
    }

    #[test]
    fn options_32() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--vides"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.debug_mask, CcxDebugMessageTypes::Vides);
    }

    #[test]
    fn options_33() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--cbraw"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.debug_mask, CcxDebugMessageTypes::Cbraw);
    }

    #[test]
    fn options_34() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--no-sync"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.nosync);
    }

    #[test]
    fn options_35() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--fullbin"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.fullbin);
    }

    #[test]
    fn options_36() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--parsedebug"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.debug_mask, CcxDebugMessageTypes::Parse);
    }

    #[test]
    fn options_37() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--parsePAT"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.debug_mask, CcxDebugMessageTypes::Pat);
    }

    #[test]
    fn options_38() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--parsePMT"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.debug_mask, CcxDebugMessageTypes::Pmt);
    }

    #[test]
    fn options_39() {
        let args: Args =
            Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--investigate-packets"])
                .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.investigate_packets);
    }

    #[test]
    fn options_40() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--mp4vidtrack"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.mp4vidtrack);
    }

    #[test]
    fn options_41() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--wtvmpeg2"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.wtvmpeg2);
    }

    #[test]
    fn options_42() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--hauppauge"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.hauppauge_mode);
    }

    #[test]
    fn options_43() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--xmltv",
            "1",
            "--out",
            "null",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.xmltv.unwrap(), 1);
        assert_eq!(opt.write_format, CcxOutputFormat::Null);
    }

    #[test]
    fn options_44() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--codec",
            "dvbsub",
            "--out",
            "spupng",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.demux_cfg.codec, CcxCodeType::Dvb);
        assert_eq!(opt.write_format, CcxOutputFormat::Spupng);
    }

    #[test]
    fn options_45() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--startcreditsnotbefore",
            "1",
            "--startcreditstext",
            "CCextractor Start credit Testing",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.enc_cfg.startcreditsnotbefore.ss, 1);
        assert_eq!(
            opt.enc_cfg.start_credits_text,
            "CCextractor Start credit Testing"
        );
    }

    #[test]
    fn options_46() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--startcreditsnotafter",
            "2",
            "--startcreditstext",
            "CCextractor Start credit Testing",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.enc_cfg.startcreditsnotafter.ss, 2);
        assert_eq!(
            opt.enc_cfg.start_credits_text,
            "CCextractor Start credit Testing"
        );
    }

    #[test]
    fn options_47() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--startcreditsforatleast",
            "1",
            "--startcreditstext",
            "CCextractor Start credit Testing",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.enc_cfg.startcreditsforatleast.ss, 1);
        assert_eq!(
            opt.enc_cfg.start_credits_text,
            "CCextractor Start credit Testing"
        );
    }

    #[test]
    fn options_48() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--startcreditsforatmost",
            "2",
            "--startcreditstext",
            "CCextractor Start credit Testing",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.enc_cfg.startcreditsforatmost.ss, 2);
        assert_eq!(
            opt.enc_cfg.start_credits_text,
            "CCextractor Start credit Testing"
        );
    }

    #[test]
    fn options_49() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--endcreditsforatleast",
            "3",
            "--endcreditstext",
            "CCextractor Start credit Testing",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.enc_cfg.endcreditsforatleast.ss, 3);
        assert_eq!(
            opt.enc_cfg.end_credits_text,
            "CCextractor Start credit Testing"
        );
    }

    #[test]
    fn options_50() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--endcreditsforatmost",
            "2",
            "--endcreditstext",
            "CCextractor Start credit Testing",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert_eq!(opt.enc_cfg.endcreditsforatmost.ss, 2);
        assert_eq!(
            opt.enc_cfg.end_credits_text,
            "CCextractor Start credit Testing"
        );
    }

    #[test]
    fn options_51() {
        let args: Args = Args::try_parse_from(vec!["<PROGRAM NAME>", "myfile", "--tverbose"])
            .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };
        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(tlt_config.verbose);
        assert_eq!(opt.debug_mask, CcxDebugMessageTypes::Teletext);
    }

    #[test]
    fn teletext_1() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--autoprogram",
            "--out",
            "srt",
            "--latin1",
            "--datapid",
            "2310",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.demux_cfg.ts_autoprogram);
        assert_eq!(opt.demux_cfg.ts_cappids[0], 2310);
        assert_eq!(opt.demux_cfg.nb_ts_cappid, 1);
        assert_eq!(opt.write_format, CcxOutputFormat::Srt);
        assert_eq!(opt.enc_cfg.encoding, CcxEncodingType::Latin1);
    }

    #[test]
    fn teletext_2() {
        let args: Args = Args::try_parse_from(vec![
            "<PROGRAM NAME>",
            "myfile",
            "--autoprogram",
            "--out",
            "srt",
            "--latin1",
            "--teletext",
            "--tpage",
            "398",
        ])
        .expect("Failed to parse arguments");
        let mut opt: CcxSOptions = CcxSOptions {
            ..Default::default()
        };
        let mut tlt_config = CcxSTeletextConfig {
            ..Default::default()
        };

        parse_parameters(&mut opt, &args, &mut tlt_config);

        assert!(opt.demux_cfg.ts_autoprogram);
        assert_eq!(opt.demux_cfg.codec, CcxCodeType::Teletext);
        assert_eq!(tlt_config.page, 398);
        assert_eq!(opt.write_format, CcxOutputFormat::Srt);
        assert_eq!(opt.enc_cfg.encoding, CcxEncodingType::Latin1);
    }
}
