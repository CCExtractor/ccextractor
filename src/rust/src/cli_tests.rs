//! Command line tests that go through the real entry point, `ccxr_parse_parameters`, and check
//! the C `ccx_options` it fills in. The tests in parser.rs stop at the Rust `Options`, so they
//! can't see problems in the legacy spelling conversion or in the copy to C.

use std::ffi::{CStr, CString};
use std::sync::Mutex;

use crate::bindings::*;
use crate::{ccx_options, ccxr_parse_parameters, tlt_config};

/// `ccxr_parse_parameters` writes to global C state, so these tests must not run concurrently.
static PARSE_LOCK: Mutex<()> = Mutex::new(());

/// Value of `CCX_DMT_DECODER_608` in ccx_common_constants.h
const CCX_DMT_DECODER_608: i64 = 0x10;

/// Run `args` (without the program name and input file) the way `main()` does, and return the
/// exit code and copies of the C options and teletext config it produced.
fn parse_to_c(args: &[&str]) -> (i32, ccx_s_options, ccx_s_teletext_config) {
    let with_input: Vec<&str> = std::iter::once("input.ts")
        .chain(args.iter().copied())
        .collect();
    parse_args_to_c(&with_input)
}

/// Like `parse_to_c`, but `args` are passed exactly as given, without an input file.
fn parse_args_to_c(args: &[&str]) -> (i32, ccx_s_options, ccx_s_teletext_config) {
    let _guard = PARSE_LOCK.lock().unwrap_or_else(|e| e.into_inner());
    let arguments: Vec<CString> = std::iter::once(&"ccextractor")
        .chain(args.iter())
        .map(|arg| CString::new(*arg).unwrap())
        .collect();
    let mut argv: Vec<*mut std::os::raw::c_char> = arguments
        .iter()
        .map(|arg| arg.as_ptr().cast_mut())
        .collect();
    // Each pointer refers to a CString that lives until the end of this function
    unsafe {
        let code = ccxr_parse_parameters(argv.len() as _, argv.as_mut_ptr());
        (
            code,
            std::ptr::read(&raw const ccx_options),
            std::ptr::read(&raw const tlt_config),
        )
    }
}

/// Like `parse_to_c`, but fails the test if the arguments were rejected.
fn parse_ok(args: &[&str]) -> (ccx_s_options, ccx_s_teletext_config) {
    let (code, options, teletext) = parse_to_c(args);
    assert_eq!(code, 0, "{:?} was rejected", args);
    (options, teletext)
}

/// Like `parse_ok`, but without an input file, for input sources that don't allow one.
fn parse_ok_without_input(args: &[&str]) -> ccx_s_options {
    let (code, options, _) = parse_args_to_c(args);
    assert_eq!(code, 0, "{:?} was rejected", args);
    options
}

/// Each spelling in `spellings` must be accepted and give the same C options per `check`.
fn check_spellings(spellings: &[&[&str]], check: impl Fn(&ccx_s_options) -> bool) {
    for args in spellings {
        assert!(
            check(&parse_ok(args).0),
            "{:?} did not reach C as expected",
            args
        );
    }
}

#[test]
fn test_negative_delay_reaches_c() {
    check_spellings(
        &[&["--delay", "-500"], &["--delay=-500"], &["-delay", "-500"]],
        |o| o.enc_cfg.subs_delay == -500,
    );
}

#[test]
fn test_output_format_reaches_c() {
    check_spellings(&[&["--out=webvtt"], &["-out=webvtt"], &["--webvtt"]], |o| {
        o.write_format == ccx_output_format::CCX_OF_WEBVTT
            && o.enc_cfg.write_format == ccx_output_format::CCX_OF_WEBVTT
    });
}

#[test]
fn test_ucla_reaches_c() {
    check_spellings(&[&["--ucla"], &["-ucla"], &["-UCLA"]], |o| o.ucla == 1);
}

#[test]
fn test_608_debug_reaches_c() {
    check_spellings(&[&["--608"], &["-608"]], |o| {
        o.debug_mask & CCX_DMT_DECODER_608 != 0
    });
}

#[test]
fn test_no_font_color_reaches_c() {
    check_spellings(&[&["--no-fontcolor"], &["-nofontcolor"]], |o| {
        o.nofontcolor == 1 && o.enc_cfg.no_font_color == 1
    });
}

#[test]
fn test_gui_mode_reports_reaches_c() {
    check_spellings(&[&["--gui-mode-reports"], &["--gui_mode_reports"]], |o| {
        o.gui_mode_reports == 1
    });
}

#[test]
fn test_extraction_start_reaches_c() {
    check_spellings(&[&["--startat", "1:30"], &["-startat", "00:01:30"]], |o| {
        o.extraction_start.set == 1 && o.extraction_start.time_in_ms == 90_000
    });
}

#[test]
fn test_sentence_cap_reaches_c() {
    check_spellings(&[&["--sentencecap"], &["-sc"]], |o| {
        o.enc_cfg.sentence_cap == 1
    });
}

#[test]
fn test_mkvlang_reaches_c() {
    let (options, _) = parse_ok(&["--mkvlang", "eng"]);
    assert!(!options.mkvlang.is_null());
    let mkvlang = unsafe { CStr::from_ptr(options.mkvlang) };
    assert_eq!(mkvlang.to_str(), Ok("eng"));
}

#[test]
fn test_tpage_reaches_teletext_config() {
    let (_, teletext) = parse_ok(&["--tpage", "777"]);
    // The C config keeps the decimal page number; ccextractor.c converts it to BCD
    assert_eq!(teletext.page, 777);
}

#[test]
fn test_input_format_options_reach_c() {
    for (long, legacy, mode) in [
        (
            "--es",
            "-es",
            ccx_stream_mode_enum_CCX_SM_ELEMENTARY_OR_NOT_FOUND,
        ),
        ("--ts", "-ts", ccx_stream_mode_enum_CCX_SM_TRANSPORT),
        ("--ps", "-ps", ccx_stream_mode_enum_CCX_SM_PROGRAM),
        ("--asf", "-asf", ccx_stream_mode_enum_CCX_SM_ASF),
        ("--dvr-ms", "-dvr-ms", ccx_stream_mode_enum_CCX_SM_ASF),
        ("--wtv", "-wtv", ccx_stream_mode_enum_CCX_SM_WTV),
        ("--mp4", "-mp4", ccx_stream_mode_enum_CCX_SM_MP4),
        ("--mkv", "-mkv", ccx_stream_mode_enum_CCX_SM_MKV),
    ] {
        check_spellings(&[&[long], &[legacy]], |o| {
            o.demux_cfg.auto_stream == mode as _
        });
    }
}

#[test]
fn test_output_format_options_reach_c() {
    for (long, legacy, format) in [
        ("--srt", "-srt", ccx_output_format::CCX_OF_SRT),
        ("--sami", "-sami", ccx_output_format::CCX_OF_SAMI),
        ("--dvdraw", "-dvdraw", ccx_output_format::CCX_OF_DVDRAW),
        ("--mcc", "-mcc", ccx_output_format::CCX_OF_MCC),
        ("--null", "-null", ccx_output_format::CCX_OF_NULL),
    ] {
        check_spellings(&[&[long], &[legacy]], |o| {
            o.write_format == format && o.enc_cfg.write_format == format
        });
    }
}

#[test]
fn test_null_terminated_reaches_c() {
    check_spellings(&[&["--null-terminated"]], |o| {
        o.enc_cfg.frame_terminator_0 == 1
    });
}

#[test]
fn test_report_format_reaches_c() {
    let (options, _) = parse_ok(&["--out=report", "--report-format", "JSON"]);
    assert!(!options.report_format.is_null());
    let format = unsafe { CStr::from_ptr(options.report_format) };
    assert_eq!(format.to_str(), Ok("json"));
}

fn c_string(ptr: *const std::os::raw::c_char) -> Option<String> {
    if ptr.is_null() {
        return None;
    }
    Some(
        unsafe { CStr::from_ptr(ptr) }
            .to_string_lossy()
            .into_owned(),
    )
}

#[test]
fn test_stdin_reaches_c() {
    for args in [&["--stdin"], &["-stdin"]] {
        let options = parse_ok_without_input(args);
        assert_eq!(
            options.input_source, ccx_datasource_CCX_DS_STDIN as _,
            "{:?}",
            args
        );
    }
}

#[test]
fn test_udp_reaches_c() {
    for args in [&["--udp", "1234"], &["-udp", "1234"]] {
        let options = parse_ok_without_input(args);
        assert_eq!(options.input_source, ccx_datasource_CCX_DS_NETWORK as _);
        assert_eq!(options.udpport, 1234);
    }

    let options = parse_ok_without_input(&["--udp", "127.0.0.1:5000"]);
    assert_eq!(c_string(options.udpaddr).as_deref(), Some("127.0.0.1"));
    assert_eq!(options.udpport, 5000);

    let options = parse_ok_without_input(&["--udp", "10.0.0.1@239.1.1.1:5001"]);
    assert_eq!(c_string(options.udpsrc).as_deref(), Some("10.0.0.1"));
    assert_eq!(c_string(options.udpaddr).as_deref(), Some("239.1.1.1"));
    assert_eq!(options.udpport, 5001);
}

#[test]
fn test_tcp_reaches_c() {
    for args in [&["--tcp", "7311"], &["-tcp", "7311"]] {
        let options = parse_ok_without_input(args);
        assert_eq!(options.input_source, ccx_datasource_CCX_DS_TCP as _);
        assert_eq!(c_string(options.tcpport).as_deref(), Some("7311"));
        // TCP input is always CCExtractor's binary format (RCWT)
        assert_eq!(
            options.demux_cfg.auto_stream,
            ccx_stream_mode_enum_CCX_SM_RCWT as _
        );
    }
}

#[test]
fn test_unknown_option_is_rejected() {
    let (code, _, _) = parse_to_c(&["--no-such-option"]);
    assert_ne!(code, 0);
}

/// Options that no test uses yet. When you add a test for one, remove it from this list.
const UNTESTED_OPTIONS: &[&str] = &[
    // Input sources
    "src",
    "sendto-port",
    "stream",
    // Output formats
    "smi",
    // Burned-in subtitles
    "hardsubx",
    "hcc",
    // Only in builds with the with_libcurl feature
    "curlposturl",
];

/// The test code searched for option names: the test modules of parser.rs and lib.rs, and this
/// file. A test anywhere else is not counted.
fn test_sources() -> [&'static str; 3] {
    let parser = include_str!("parser.rs");
    let lib = include_str!("lib.rs");
    [
        &parser[parser.find("mod tests {").unwrap()..],
        &lib[lib.find("mod test {").unwrap()..],
        include_str!("cli_tests.rs"),
    ]
}

/// Whether a test passes `option` on the command line, as `--name` or `-name`, alone or with `=`.
fn is_used_in_tests(option: &str) -> bool {
    test_sources().iter().any(|source| {
        ["--", "-"].iter().any(|dashes| {
            source.contains(&format!("\"{}{}\"", dashes, option))
                || source.contains(&format!("\"{}{}=", dashes, option))
        })
    })
}

#[test]
fn test_every_option_has_a_test() {
    use clap::CommandFactory;

    let options: Vec<String> = crate::args::Args::command()
        .get_arguments()
        .filter_map(|arg| arg.get_long().map(String::from))
        .collect();

    let untested: Vec<&String> = options
        .iter()
        .filter(|option| !is_used_in_tests(option) && !UNTESTED_OPTIONS.contains(&option.as_str()))
        .collect();
    assert!(
        untested.is_empty(),
        "These options have no test, please add one: {:?}",
        untested
    );

    let now_tested: Vec<&&str> = UNTESTED_OPTIONS
        .iter()
        .filter(|option| is_used_in_tests(option))
        .collect();
    assert!(
        now_tested.is_empty(),
        "These options have a test now, remove them from UNTESTED_OPTIONS: {:?}",
        now_tested
    );
}
