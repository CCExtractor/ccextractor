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

/// Run `args` (without the program name) the way `main()` does, and return the exit code and
/// copies of the C options and teletext config it produced.
fn parse_to_c(args: &[&str]) -> (i32, ccx_s_options, ccx_s_teletext_config) {
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
        // Start from the zeroed state the globals have in test builds, so a test can't see values
        // left by another one. The old strings are leaked on purpose: copies returned by earlier
        // calls may still point to them.
        ccx_options = std::mem::zeroed();
        tlt_config = std::mem::zeroed();
        let code = ccxr_parse_parameters(argv.len() as _, argv.as_mut_ptr());
        (
            code,
            std::ptr::read(&raw const ccx_options),
            std::ptr::read(&raw const tlt_config),
        )
    }
}

/// The C string `ptr` points to, or `None` if it is null.
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

/// One command line case: ways to write some options, and a check on the C options they give.
struct Case {
    /// Each entry is one way to write the same thing (for example the new and the old spelling).
    /// All of them must be accepted and pass `check`.
    spellings: &'static [&'static [&'static str]],
    /// Input sources like `--stdin` don't allow an input file. The other cases get one.
    with_input_file: bool,
    check: fn(&ccx_s_options, &ccx_s_teletext_config) -> bool,
}

impl Case {
    /// The arguments of one spelling, after the program name.
    fn args(&self, spelling: &[&'static str]) -> Vec<&'static str> {
        let input: &[&'static str] = if self.with_input_file {
            &["input.ts"]
        } else {
            &[]
        };
        input.iter().chain(spelling).copied().collect()
    }
}

/// A case whose arguments follow an input file.
fn case(
    spellings: &'static [&'static [&'static str]],
    check: fn(&ccx_s_options, &ccx_s_teletext_config) -> bool,
) -> Case {
    Case {
        spellings,
        with_input_file: true,
        check,
    }
}

/// Every command line case. `test_cases_reach_c` runs each one through `ccxr_parse_parameters`.
fn cases() -> Vec<Case> {
    vec![
        case(
            &[&["--delay", "-500"], &["--delay=-500"], &["-delay", "-500"]],
            |o, _| o.enc_cfg.subs_delay == -500,
        ),
        case(
            &[&["--out=webvtt"], &["-out=webvtt"], &["--webvtt"]],
            |o, _| {
                o.write_format == ccx_output_format::CCX_OF_WEBVTT
                    && o.enc_cfg.write_format == ccx_output_format::CCX_OF_WEBVTT
            },
        ),
        case(&[&["--ucla"], &["-ucla"], &["-UCLA"]], |o, _| o.ucla == 1),
        case(&[&["--608"], &["-608"]], |o, _| {
            o.debug_mask & CCX_DMT_DECODER_608 != 0
        }),
        case(&[&["--no-fontcolor"], &["-nofontcolor"]], |o, _| {
            o.nofontcolor == 1 && o.enc_cfg.no_font_color == 1
        }),
        case(
            &[&["--gui-mode-reports"], &["--gui_mode_reports"]],
            |o, _| o.gui_mode_reports == 1,
        ),
        case(
            &[&["--startat", "1:30"], &["-startat", "00:01:30"]],
            |o, _| o.extraction_start.set == 1 && o.extraction_start.time_in_ms == 90_000,
        ),
        case(&[&["--sentencecap"], &["-sc"]], |o, _| {
            o.enc_cfg.sentence_cap == 1
        }),
        case(&[&["--mkvlang", "eng"]], |o, _| {
            c_string(o.mkvlang).as_deref() == Some("eng")
        }),
        // The C config keeps the decimal page number; ccextractor.c converts it to BCD
        case(&[&["--tpage", "777"]], |_, t| t.page == 777),
    ]
}

#[test]
fn test_cases_reach_c() {
    for case in cases() {
        for spelling in case.spellings {
            let args = case.args(spelling);
            let (code, options, teletext) = parse_to_c(&args);
            assert_eq!(code, 0, "{:?} was rejected", args);
            assert!(
                (case.check)(&options, &teletext),
                "{:?} did not reach C as expected",
                args
            );
        }
    }
}

#[test]
fn test_each_parse_starts_fresh() {
    // copy_from_rust only copies enc_cfg while enc_cfg.output_filename is unset, so the second
    // parse would not reach C if it saw the output file name left by the first
    assert_eq!(parse_to_c(&["input.ts", "-o", "out.srt"]).0, 0);
    let (code, options, _) = parse_to_c(&["input.ts", "--sentencecap"]);
    assert_eq!(code, 0);
    assert_eq!(options.enc_cfg.sentence_cap, 1);
}

#[test]
fn test_unknown_option_is_rejected() {
    let (code, _, _) = parse_to_c(&["input.ts", "--no-such-option"]);
    assert_ne!(code, 0);
}
