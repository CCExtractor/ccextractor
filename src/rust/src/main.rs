mod args;
use args::Args;

mod structs;
use clap::Parser;

use structs::CcxSOptions;

use crate::{args::Codec, structs::*};

static mut FILEBUFFERSIZE: i64 = 1024 * 1024 * 16;

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
    let mut present = 0;

    for (char_index, c) in lang.to_lowercase().chars().enumerate() {
        if c == ',' {
            present = char_index;

            if present - initial < 3 || present - initial > 6 {
                panic!("language codes should be xxx,xxx,xxx,....\n");
            }

            if present - initial == 6 {
                let sub_slice = &lang[initial..present];
                if !sub_slice.contains('-') {
                    panic!("language code is not of the form xxx-xx\n");
                }
            }

            initial = present + 1;
        }
    }

    // Steps to check for the last lang of multiple mkvlangs provided by the user.
    present = lang.len() - 1;

    for char_index in (0..present).rev() {
        if lang.chars().nth(char_index) == Some(',') {
            initial = char_index + 1;
            break;
        }
    }

    if present - initial < 2 || present - initial > 5 {
        panic!("last language code should be xxx.\n");
    }

    if present - initial == 5 {
        let sub_slice = &lang[initial..present];
        if !sub_slice.contains('-') {
            panic!("last language code is not of the form xxx-xx\n");
        }
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

    {
        if args.hardsubx {
            opt.hardsubx = Some(1);

            if args.hcc {
                opt.hardsubx_and_common = Some(1);
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

            if let Some(value) = args.min_sub_duration {
                if value == 0.0 {
                    println!("Invalid minimum subtitle duration");
                    std::process::exit(ExitCode::MalformedParameter as i32);
                }
                opt.hardsubx_min_sub_duration = Some(value);
            }

            if args.detect_italics {
                opt.hardsubx_detect_italics = Some(1);
            }

            if let Some(value) = args.conf_thresh {
                if !(0.0..=100.0).contains(&value) {
                    println!("Invalid confidence threshold, valid values are between 0 & 100");
                    std::process::exit(ExitCode::MalformedParameter as i32);
                }
                opt.hardsubx_conf_thresh = Some(value);
            }

            if let Some(value) = args.whiteness_thresh {
                if !(0.0..=100.0).contains(&value) {
                    println!("Invalid whiteness threshold, valid values are between 0 & 100");
                    std::process::exit(ExitCode::MalformedParameter as i32);
                }
                opt.hardsubx_lum_thresh = Some(value);
            }
        }
    } // END OF HARDSUBX

    if args.chapters {
        opt.extract_chapters = Some(1);
    }

    if args.bufferinput {
        opt.buffer_input = Some(1);
    }

    if args.no_bufferinput {
        opt.buffer_input = Some(0);
    }

    if args.koc {
        opt.keep_output_closed = Some(1);
    }

    if args.forceflush {
        opt.force_flush = Some(1);
    }

    if args.append {
        opt.append_mode = Some(1);
    }

    if let Some(buffersize) = args.buffersize {
        unsafe {
            let mut_ref = &mut FILEBUFFERSIZE;

            if buffersize < 8 {
                *mut_ref = 8; // Otherwise crashes are guaranteed at least in MythTV
            } else {
                *mut_ref = buffersize;
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
        opt.timestamp_map = Some(1);
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

    if let Some(codec) = args.codec {
        match codec {
            Codec::Teletext => {
                opt.demux_cfg.codec = CcxCodeType::Teletext;
            }
            Codec::Dvbsub => {
                opt.demux_cfg.codec = CcxCodeType::Dvb;
            }
        }
    }

    if let Some(codec) = args.no_codec {
        match codec {
            Codec::Dvbsub => {
                opt.demux_cfg.nocodec = CcxCodeType::Teletext;
            }
            Codec::Teletext => {
                opt.demux_cfg.nocodec = CcxCodeType::Dvb;
            }
        }
    }

    if let Some(lang) = args.dvblang {
        opt.dvblang = Some(lang);
    }

    if let Some(lang) = args.ocrlang {
        opt.ocrlang = Some(lang);
    }

    if let Some(quant) = args.quant {
        if !(0..=2).contains(&quant) {
            println!("Invalid quant value");
            std::process::exit(ExitCode::MalformedParameter as i32);
        }
        opt.ocr_quantmode = Some(quant);
    }

    if args.no_spupngocr {
        opt.enc_cfg.nospupngocr = true;
    }

    if let Some(oem) = args.oem {
        if !(0..=2).contains(&oem) {
            println!("Invalid oem value");
            std::process::exit(ExitCode::MalformedParameter as i32);
        }
        opt.ocr_oem = Some(oem);
    }

    if let Some(ref lang) = args.mkvlang {
        opt.mkvlang = Some(lang.to_string());
        let str = lang.as_str();
        mkvlang_params_check(str);
    }

    println!("Issues? Open a ticket here\n https://github.com/CCExtractor/ccextractor/issues");
}
