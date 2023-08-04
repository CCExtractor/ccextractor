mod args;
use args::{Args, OutFormat};

mod structs;
use clap::Parser;

use structs::CcxSOptions;

use crate::{
    args::{Codec, Ru},
    structs::*,
};

// TODO(prateekmedia): Check atoi_hex everywhere

static mut FILEBUFFERSIZE: i64 = 1024 * 1024 * 16;
static mut MPEG_CLOCK_FREQ: i64 = 0;
static mut usercolor_rgb: String = String::new();

pub trait FromStr {
    #[allow(dead_code)]
    fn to_ms(&self) -> CcxBoundaryTime;
}

impl FromStr for &str {
    fn to_ms(&self) -> CcxBoundaryTime {
        let mut parts = self.rsplit(":");

        let mut seconds: u32 = 0;

        match parts.next() {
            Some(sec) => seconds = sec.parse().unwrap(),
            None => {
                println!("Malformed timecode: {}", self);
                std::process::exit(ExitCode::MalformedParameter as i32);
            }
        };
        let mut minutes: u32 = 0;

        match parts.next() {
            Some(mins) => minutes = mins.parse().unwrap(),
            None => {}
        };
        let mut hours: u32 = 0;

        match parts.next() {
            Some(hrs) => hours = hrs.parse().unwrap(),
            None => {}
        };

        if seconds > 60 || minutes > 60 {
            println!("Malformed timecode: {}", self);
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
}

fn set_output_format(opt: &mut CcxSOptions, args: &Args) {
    opt.write_format_rewritten = true;

    let can_unwrap = args.out.is_some();
    // TODO(prateemedia): there was no reference of this in help
    // #[cfg(feature = "with_libcurl")]
    // {
    //     if can_unwrap && args.out.unwrap() == OutFormat::Curl {
    //         opt.write_format = CcxOutputFormat::Curl;
    //         return;
    //     }
    // }

    if opt.send_to_srv && can_unwrap && args.out.unwrap() == OutFormat::Bin {
        println!("Output format is changed to bin\n");
        opt.write_format = CcxOutputFormat::Rcwt;
        return;
    }

    if can_unwrap && args.out.unwrap() == OutFormat::Ass {
        opt.write_format = CcxOutputFormat::Ssa;
        opt.use_ass_instead_of_ssa = true;
    } else if can_unwrap && args.out.unwrap() == OutFormat::Ccd {
        opt.write_format = CcxOutputFormat::Ccd;
    } else if can_unwrap && args.out.unwrap() == OutFormat::Scc {
        opt.write_format = CcxOutputFormat::Scc;
    } else if can_unwrap && args.out.unwrap() == OutFormat::Srt {
        opt.write_format = CcxOutputFormat::Srt;
    } else if args.srt || can_unwrap && args.out.unwrap() == OutFormat::Ssa {
        opt.write_format = CcxOutputFormat::Ssa;
    } else if args.webvtt || can_unwrap && args.out.unwrap() == OutFormat::Webvtt {
        opt.write_format = CcxOutputFormat::Webvtt;
    } else if can_unwrap && args.out.unwrap() == OutFormat::WebvttFull {
        opt.write_format = CcxOutputFormat::Webvtt;
        opt.use_webvtt_styling = true;
    } else if args.sami || can_unwrap && args.out.unwrap() == OutFormat::Sami {
        opt.write_format = CcxOutputFormat::Sami;
    } else if args.txt || can_unwrap && args.out.unwrap() == OutFormat::Txt {
        opt.write_format = CcxOutputFormat::Transcript;
        opt.settings_dtvcc.no_rollup = true;
    } else if args.ttxt || can_unwrap && args.out.unwrap() == OutFormat::Ttxt {
        opt.write_format = CcxOutputFormat::Transcript;
        if opt.date == CcxOutputDateFormat::None {
            opt.date = CcxOutputDateFormat::HHMMSSMS;
        }
        // Sets the right things so that timestamps and the mode are printed.
        if !opt.transcript_settings.is_final {
            opt.transcript_settings.show_start_time = true;
            opt.transcript_settings.show_end_time = true;
            opt.transcript_settings.show_cc = false;
            opt.transcript_settings.show_mode = true;
        }
    } else if can_unwrap && args.out.unwrap() == OutFormat::Report {
        opt.write_format = CcxOutputFormat::Null;
        opt.messages_target = Some(0);
        opt.print_file_reports = true;
        opt.demux_cfg.ts_allprogram = true;
    } else if can_unwrap && args.out.unwrap() == OutFormat::Raw {
        opt.write_format = CcxOutputFormat::Raw;
    } else if can_unwrap && args.out.unwrap() == OutFormat::Smptett {
        opt.write_format = CcxOutputFormat::Smptett;
    } else if can_unwrap && args.out.unwrap() == OutFormat::Bin {
        opt.write_format = CcxOutputFormat::Rcwt;
    } else if args.null || can_unwrap && args.out.unwrap() == OutFormat::Null {
        opt.write_format = CcxOutputFormat::Null;
    } else if args.dvdraw || can_unwrap && args.out.unwrap() == OutFormat::Dvdraw {
        opt.write_format = CcxOutputFormat::Dvdraw;
    } else if can_unwrap && args.out.unwrap() == OutFormat::Spupng {
        opt.write_format = CcxOutputFormat::Spupng;
    }
    // else if can_unwrap && args.out.unwrap() == OutFormat:: {
    // opt.write_format = CcxOutputFormat::SimpleXml;}
    else if can_unwrap && args.out.unwrap() == OutFormat::G608 {
        opt.write_format = CcxOutputFormat::G608;
    } else if args.mcc || can_unwrap && args.out.unwrap() == OutFormat::Mcc {
        opt.write_format = CcxOutputFormat::Mcc;
    } else {
        println!(
            "Unknown input file format: {}\n",
            args.input.unwrap().to_string()
        );
        std::process::exit(ExitCode::MalformedParameter as i32);
    }
}

fn set_input_format(opt: &mut CcxSOptions, args: &Args) {
    if opt.input_source == CcxDatasource::Tcp {
        println!("Input format is changed to bin\n");
        opt.demux_cfg.auto_stream = CcxStreamMode::Rcwt;
        return;
    }

    let can_unwrap = args.input.is_some();
    #[cfg(feature = "wtv_debug")]
    if can_unwrap && args.input.unwrap().to_string() == "hex" {
        opt.demux_cfg.auto_stream = CcxStreamMode::HexDump;
        return;
    }

    if args.es || can_unwrap && args.input.unwrap().to_string() == "es" {
        opt.demux_cfg.auto_stream = CcxStreamMode::ElementaryOrNotFound;
    } else if args.ts || can_unwrap && args.input.unwrap().to_string() == "ts" {
        opt.demux_cfg.auto_stream = CcxStreamMode::Transport;
        opt.demux_cfg.m2ts = false;
    } else if can_unwrap && args.input.unwrap().to_string() == "m2ts" {
        opt.demux_cfg.auto_stream = CcxStreamMode::Transport;
        opt.demux_cfg.m2ts = true;
    } else if args.ps || can_unwrap && args.input.unwrap().to_string() == "ps" {
        opt.demux_cfg.auto_stream = CcxStreamMode::Program;
    } else if args.asf
        || args.dvr_ms
        || can_unwrap
            && (args.input.unwrap().to_string() == "asf"
                || args.input.unwrap().to_string() == "dvr-ms")
    {
        opt.demux_cfg.auto_stream = CcxStreamMode::Asf;
    } else if args.wtv || can_unwrap && args.input.unwrap().to_string() == "wtv" {
        opt.demux_cfg.auto_stream = CcxStreamMode::Wtv;
    } else if can_unwrap && args.input.unwrap().to_string() == "raw" {
        opt.demux_cfg.auto_stream = CcxStreamMode::McpoodlesRaw;
    } else if can_unwrap && args.input.unwrap().to_string() == "bin" {
        opt.demux_cfg.auto_stream = CcxStreamMode::Rcwt;
    } else if can_unwrap && args.input.unwrap().to_string() == "mp4" {
        opt.demux_cfg.auto_stream = CcxStreamMode::Mp4;
    } else if can_unwrap && args.input.unwrap().to_string() == "mkv" {
        opt.demux_cfg.auto_stream = CcxStreamMode::Mkv;
    } else if can_unwrap && args.input.unwrap().to_string() == "bin" {
        opt.demux_cfg.auto_stream = CcxStreamMode::Mxf;
    } else {
        println!(
            "Unknown input file format: {}\n",
            args.input.unwrap().to_string()
        );
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
        let charset = &s[4..];
        opts.settings_dtvcc.enabled = true;
        opts.enc_cfg.dtvcc_extract = true;
        opts.enc_cfg.all_services_charset = charset.to_owned();
        opts.enc_cfg.services_charsets =
            vec![Box::new(charset.to_owned())].into_boxed_slice();
        opts.settings_dtvcc.active_services_count = CCX_DTVCC_MAX_SERVICES;
        return;
    }

    let mut services = Vec::new();
    let mut charsets = Vec::new();
    for c in s.split(',') {
        let mut service = String::new();
        let mut charset = None;
        for e in c.chars() {
            if e.is_digit(10) {
                service.push(e);
            } else if e == '[' {
                charset = Some(e.to_string());
            } else if e == ']' {
                if let Some(c) = charset {
                    charsets.push(c);
                }
            }
        }
        if service.is_empty() {
            continue;
        }
        services.push(service);
    }

    for (i, service) in services.iter().enumerate() {
        let svc = service.parse::<i32>().unwrap();
        if svc < 1 || svc > CCX_DTVCC_MAX_SERVICES {
            panic!("[CEA-708] Malformed parameter: Invalid service number ({}), valid range is 1-{}.\n", svc, CCX_DTVCC_MAX_SERVICES);
        }
        opts.settings_dtvcc.services_enabled[svc - 1] = true;
        opts.enc_cfg.services_enabled[svc - 1] = true;
        opts.settings_dtvcc.active_services_count += 1;
        if charsets.len() > i {
            opts.enc_cfg.services_charsets[svc - 1] = charsets[i].to_owned();
        }
    }

    if opts.settings_dtvcc.active_services_count <= 0 {
        panic!("[CEA-708] Malformed parameter: no services\n");
    }
}


fn main() {
    let args: Args = Args::parse();
    let mut opt = CcxSOptions {
        ..Default::default()
    };

    if args.stdin {
        if cfg!(windows) {
            // TODO(prateekmedia): check this
            // setmode(fileno(stdin), O_BINARY);
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
                if value == 0.0 {
                    println!("Invalid minimum subtitle duration");
                    std::process::exit(ExitCode::MalformedParameter as i32);
                }
                opt.hardsubx_min_sub_duration = Some(value);
            }

            if args.detect_italics {
                opt.hardsubx_detect_italics = true;
            }

            if let Some(ref value) = args.conf_thresh {
                if !(0.0..=100.0).contains(&value) {
                    println!("Invalid confidence threshold, valid values are between 0 & 100");
                    std::process::exit(ExitCode::MalformedParameter as i32);
                }
                opt.hardsubx_conf_thresh = Some(value);
            }

            if let Some(ref value) = args.whiteness_thresh {
                if !(0.0..=100.0).contains(&value) {
                    println!("Invalid whiteness threshold, valid values are between 0 & 100");
                    std::process::exit(ExitCode::MalformedParameter as i32);
                }
                opt.hardsubx_lum_thresh = Some(value);
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
            let mut_ref = &mut FILEBUFFERSIZE;

            if *buffersize < 8 {
                *mut_ref = 8; // Otherwise crashes are guaranteed at least in MythTV
            } else {
                *mut_ref = *buffersize;
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
        set_input_format(&mut opt, &args);
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
                opt.demux_cfg.codec = CcxCodeType::Teletext;
            }
            Codec::Teletext => {
                opt.demux_cfg.codec = CcxCodeType::Dvb;
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
        if !(0..=2).contains(&*quant) {
            println!("Invalid quant value");
            std::process::exit(ExitCode::MalformedParameter as i32);
        }
        opt.ocr_quantmode = Some(*quant);
    }

    if args.no_spupngocr {
        opt.enc_cfg.nospupngocr = true;
    }

    if let Some(ref oem) = args.oem {
        if !(0..=2).contains(&*oem) {
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
        set_output_format(&mut opt, &args);
    }

    if let Some(ref startcreditstext) = args.startcreditstext {
        opt.enc_cfg.start_credits_text = startcreditstext.clone();
    }

    if let Some(ref startcreditsnotbefore) = args.startcreditsnotbefore {
        opt.enc_cfg.startcreditsnotbefore = startcreditsnotbefore.clone().as_str().to_ms();
    }

    if let Some(ref startcreditsnotafter) = args.startcreditsnotafter {
        opt.enc_cfg.startcreditsnotafter = startcreditsnotafter.clone().as_str().to_ms();
    }

    if let Some(ref startcreditsforatleast) = args.startcreditsforatleast {
        opt.enc_cfg.startcreditsforatleast = startcreditsforatleast.clone().as_str().to_ms();
    }
    if let Some(ref startcreditsforatmost) = args.startcreditsforatmost {
        opt.enc_cfg.startcreditsforatmost = startcreditsforatmost.clone().as_str().to_ms();
    }

    if let Some(ref endcreditstext) = args.endcreditstext {
        opt.enc_cfg.end_credits_text = endcreditstext.clone();
    }

    if let Some(ref endcreditsforatleast) = args.endcreditsforatleast {
        opt.enc_cfg.endcreditsforatleast = endcreditsforatleast.clone().as_str().to_ms();
    }

    if let Some(ref endcreditsforatmost) = args.endcreditsforatmost {
        opt.enc_cfg.endcreditsforatmost = endcreditsforatmost.clone().as_str().to_ms();
    }

    /* More stuff */
    if args.videoedited {
        opt.binary_concat = false;
    }

    // TODO(prateekmedia): -12

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

    if let Some(ref ru) = args.ru {
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
        opt.demux_cfg.ts_forced_program = *program_number;
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
        opt.live_stream = Some(*stream);
    }

    if let Some(ref defaultcolor) = args.defaultcolor {
        unsafe {
            if defaultcolor.len() != 7 || !defaultcolor.starts_with("#") {
                println!("Invalid default color");
                std::process::exit(ExitCode::MalformedParameter as i32);
            }
            usercolor_rgb = defaultcolor.clone();
            opt.settings_608.default_color = CcxDecoder608ColorCode::Userdefined;
        }
    }

    if let Some(ref delay) = args.delay {
        opt.subs_delay = delay.clone();
    }

    if let Some(ref screenfuls) = args.screenfuls {
        opt.settings_608.screens_to_process = screenfuls.clone();
    }

    if let Some(ref startat) = args.startat {
        opt.extraction_start = startat.clone().as_str().to_ms();
    }
    if let Some(ref endat) = args.endat {
        opt.extraction_end = endat.clone().as_str().to_ms();
    }

    if args.cc2 {
        opt.cc_channel = Some(2);
    }

    if args.stdout {
        if let Some(message_target) = opt.messages_target {
            opt.messages_target = Some(2);
        }
        opt.cc_to_stdout = true;
    }

    if args.pesheader {
        opt.pes_header_to_stdout = true;
    }

    if args.debugdvdsub {
        opt.debug_mask = CCxDmt
    }

    if args.ignoreptsjumps {
        opt.ignore_pts_jumps = true;
    }

    if args.fixptsjumps {
        opt.ignore_pts_jumps = false;
    }

    if args.quiet {
        opt.messages_target = false;
    }

    if args.debug {
        opt.debug_mask =;
    }

    if args.eia608 {
        opt.debug_mask = ;
    }

    if args.deblev {
        opt.debug_mask = ;
    }

    if args.no_levdist {
        opt.dolevdist = Some(0);
    }

    if let Some(ref levdistmincnt) = args.levdistmincnt {
        opt.levdistmincnt = Some(*levdistmincnt);
    }
    if let Some(ref levdistmaxpct) = args.levdistmaxpct {
        opt.levdistmaxpct = Some(*levdistmaxpct);
    }

    if args.eia708 {
        opt.debug_mask = ;
    }

    if args.goppts {
        opt.debug_mask =;
    }

    if args.vides {
        opt.debug_mask = ;
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
        opt.debug_mask = ;
    }

    if args.parsedebug {
        opt.debug_mask = ;
    }

    if args.parse_pat {
        opt.debug_mask =;
    }

    if args.parse_pmt {
        opt.debug_mask = ;
    }

    if args.dumpdef {
        opt.debug_mask = ;
    }

    if args.investigate_packets {
        opt.investigate_packets = true;
    }

    if args.cbraw {
        opt.debug_mask = ;
    }

    if args.tverbose {
        opt.debug_mask = ;
        tlt_config.verbose = true;
    }

    // TODO: Check below configuration if exists
    // #[cfg(feature = "enable_sharing")]
    // {
            // opt.debug_mask = ;
    // }

    if  args.fullbin {
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

    if let Some(ref output) =  args.output {
        opt.output_filename = Some(output.clone());
    }

    if let Some(ref service) =  args.cea708services {
        opt.is_708_enabled = true;
        parse_708_services(opt, service);
    }

    if let Some(ref datapid) = args.datapid {
        opt.demux_cfg.ts_cappids[opt.demux_cfg.nb_ts_cappid] = *datapid;
        opt.demux_cfg.nb_ts_cappid += 1;
    }

    if let Some(ref datastreamtype) = args.datastreamtype {
        opt.demux_cfg.ts_datastreamtype = datastreamtype.clone().parse().unwrap();
    }

    println!("Issues? Open a ticket here\n https://github.com/CCExtractor/ccextractor/issues");
}
