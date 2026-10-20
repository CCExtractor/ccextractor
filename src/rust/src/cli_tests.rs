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
    let _guard = PARSE_LOCK.lock().unwrap_or_else(|e| e.into_inner());
    let arguments: Vec<CString> = ["ccextractor", "input.ts"]
        .iter()
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
fn test_unknown_option_is_rejected() {
    let (code, _, _) = parse_to_c(&["--no-such-option"]);
    assert_ne!(code, 0);
}
