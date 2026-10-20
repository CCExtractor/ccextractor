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

/// Every command line case. An option counts as tested only if a case here uses it (see
/// `test_every_option_has_a_case`).
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
        // The C options keep the --stream timeout in seconds
        case(&[&["--stream", "5"], &["-s", "5"]], |o, _| {
            o.live_stream == 5
        }),
        // Without a number, as in C, there is no timeout
        case(&[&["--stream"], &["-s"]], |o, _| o.live_stream == -1),
        // Input formats set the C demuxer mode
        case(&[&["--es"], &["-es"]], |o, _| {
            o.demux_cfg.auto_stream == ccx_stream_mode_enum_CCX_SM_ELEMENTARY_OR_NOT_FOUND as _
        }),
        case(&[&["--ts"], &["-ts"]], |o, _| {
            o.demux_cfg.auto_stream == ccx_stream_mode_enum_CCX_SM_TRANSPORT as _
        }),
        case(&[&["--ps"], &["-ps"]], |o, _| {
            o.demux_cfg.auto_stream == ccx_stream_mode_enum_CCX_SM_PROGRAM as _
        }),
        case(
            &[&["--asf"], &["-asf"], &["--dvr-ms"], &["-dvr-ms"]],
            |o, _| o.demux_cfg.auto_stream == ccx_stream_mode_enum_CCX_SM_ASF as _,
        ),
        case(&[&["--wtv"], &["-wtv"]], |o, _| {
            o.demux_cfg.auto_stream == ccx_stream_mode_enum_CCX_SM_WTV as _
        }),
        case(&[&["--mp4"], &["-mp4"]], |o, _| {
            o.demux_cfg.auto_stream == ccx_stream_mode_enum_CCX_SM_MP4 as _
        }),
        case(&[&["--mkv"], &["-mkv"]], |o, _| {
            o.demux_cfg.auto_stream == ccx_stream_mode_enum_CCX_SM_MKV as _
        }),
        case(&[&["--null-terminated"]], |o, _| {
            o.enc_cfg.frame_terminator_0 == 1
        }),
        case(&[&["--out=report", "--report-format", "JSON"]], |o, _| {
            c_string(o.report_format).as_deref() == Some("json")
        }),
        // Output formats. SRT is also the default, so write_format_rewritten shows that the
        // option was used.
        case(&[&["--srt"], &["-srt"]], |o, _| {
            output_format_is(o, ccx_output_format::CCX_OF_SRT)
        }),
        case(&[&["--sami"], &["-sami"]], |o, _| {
            output_format_is(o, ccx_output_format::CCX_OF_SAMI)
        }),
        case(&[&["--dvdraw"], &["-dvdraw"]], |o, _| {
            output_format_is(o, ccx_output_format::CCX_OF_DVDRAW)
        }),
        case(&[&["--mcc"], &["-mcc"]], |o, _| {
            output_format_is(o, ccx_output_format::CCX_OF_MCC)
        }),
        case(&[&["--null"], &["-null"]], |o, _| {
            output_format_is(o, ccx_output_format::CCX_OF_NULL)
        }),
    ]
}

/// Whether an output format option set `format` in both C places and marked it as chosen.
fn output_format_is(o: &ccx_s_options, format: ccx_output_format) -> bool {
    o.write_format == format && o.enc_cfg.write_format == format && o.write_format_rewritten == 1
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

/// Long names of the options that `args` sets, as clap sees them after the legacy spellings are
/// converted.
fn options_set_by(args: &[&str]) -> Vec<String> {
    use clap::parser::ValueSource;
    use clap::CommandFactory;

    let command = crate::args::Args::command();
    let argv = std::iter::once("ccextractor")
        .chain(args.iter().copied())
        .map(String::from)
        .collect();
    let matches = command
        .clone()
        .try_get_matches_from(crate::normalize_legacy_args(argv))
        .unwrap_or_else(|e| panic!("{:?} was rejected: {}", args, e));
    command
        .get_arguments()
        .filter(|arg| matches.value_source(arg.get_id().as_str()) == Some(ValueSource::CommandLine))
        .filter_map(|arg| arg.get_long().map(String::from))
        .collect()
}

/// Options that no case in `cases()` uses yet. When you add a case for one, remove it from this
/// list.
const UNTESTED_OPTIONS: &[&str] = &[
    // File name related options
    "stdout",
    "pesheader",
    "debugdvbsub",
    "ignoreptsjumps",
    "fixptsjumps",
    "stdin",
    // Output File Segmentation
    "outinterval",
    "segmentonkeyonly",
    // Network support
    "udp",
    "src",
    "sendto",
    "sendto-port",
    "tcp",
    "tcp-password",
    "tcp-description",
    // Options that affect what will be processed
    "output-field",
    "append",
    "cc2",
    "service",
    // Input Formats
    "input",
    // Output formats
    "smi",
    "txt",
    "ttxt",
    // Options that affect how input files will be processed
    "goptime",
    "no-goptime",
    "fixpadding",
    "90090",
    "scc-framerate",
    "scc-accurate-timing",
    "videoedited",
    "usepicorder",
    "myth",
    "no-myth",
    "wtvconvertfix",
    "wtvmpeg2",
    "program-number",
    "autoprogram",
    "multiprogram",
    "list-tracks",
    "datapid",
    "datastreamtype",
    "streamtype",
    "hauppauge",
    "mp4vidtrack",
    "no-autotimeref",
    "no-scte20",
    "webvtt-create-css",
    "deblev",
    "analyzevideo",
    "timestamp-map",
    // Output formats
    "no-timestamp-map",
    // Levenshtein distance
    "no-levdist",
    "levdistmincnt",
    "levdistmaxpct",
    // Options that affect what kind of output will be produced
    "chapters",
    "bom",
    "no-bom",
    "unicode",
    "utf8",
    "latin1",
    "no-htmlescape",
    "no-typesetting",
    "trim",
    "defaultcolor",
    "capfile",
    "kf",
    "profanity-file",
    "splitbysentence",
    "unixts",
    "datets",
    "sects",
    "latrusmap",
    "ttxtforcelatin",
    "xds",
    "lf",
    "df",
    "autodash",
    "xmltv",
    "xmltvliveinterval",
    "xmltvoutputinterval",
    "xmltvonlycurrent",
    "sem",
    "dvblang",
    "ocrlang",
    "quant",
    "oem",
    "psm",
    "ocr-line-split",
    "no-ocr-blacklist",
    "no-spupngocr",
    "font",
    "italics",
    // Options that affect how ccextractor reads and writes (buffering)
    "bufferinput",
    "no-bufferinput",
    "buffersize",
    "koc",
    "forceflush",
    "dru",
    "no-rollup",
    "ru1",
    "ru2",
    "ru3",
    // Options that affect what segment of the input file(s) to process
    "endat",
    "screenfuls",
    // Options that affect which codec is to be used have to be searched in input
    "codec",
    "no-codec",
    // Adding start and end credits
    "startcreditstext",
    "startcreditsnotbefore",
    "startcreditsnotafter",
    "startcreditsforatleast",
    "startcreditsforatmost",
    "endcreditstext",
    "endcreditsforatleast",
    "endcreditsforatmost",
    // Options that affect debug data
    "debug",
    "708",
    "goppts",
    "xdsdebug",
    "vides",
    "cbraw",
    "no-sync",
    "fullbin",
    "parsedebug",
    "parsePAT",
    "parsePMT",
    "dumpdef",
    "investigate-packets",
    // Teletext related options
    "tpages-all",
    "tverbose",
    "teletext",
    "no-teletext",
    // Transcript customizing options
    "customtxt",
    // Communication with other programs and console output
    "no-progress-bar",
    "quiet",
    // Burned-in subtitle extraction
    "hardsubx",
    "tickertext",
    "ocr-mode",
    "subcolor",
    "min-sub-duration",
    "detect-italics",
    "conf-thresh",
    "whiteness-thresh",
    "hcc",
    // Only in builds with the with_libcurl feature
    #[cfg(feature = "with_libcurl")]
    "curlposturl",
];

#[test]
fn test_every_option_has_a_case() {
    use clap::CommandFactory;

    let options: Vec<String> = crate::args::Args::command()
        .get_arguments()
        .filter_map(|arg| arg.get_long().map(String::from))
        .collect();
    let mut tested = std::collections::HashSet::new();
    for case in cases() {
        for spelling in case.spellings {
            tested.extend(options_set_by(&case.args(spelling)));
        }
    }

    let untested: Vec<&String> = options
        .iter()
        .filter(|option| !tested.contains(*option) && !UNTESTED_OPTIONS.contains(&option.as_str()))
        .collect();
    assert!(
        untested.is_empty(),
        "These options have no case in cases(), please add one: {:?}",
        untested
    );

    let now_tested: Vec<&&str> = UNTESTED_OPTIONS
        .iter()
        .filter(|option| tested.contains(**option))
        .collect();
    assert!(
        now_tested.is_empty(),
        "These options have a case now, remove them from UNTESTED_OPTIONS: {:?}",
        now_tested
    );

    let not_options: Vec<&&str> = UNTESTED_OPTIONS
        .iter()
        .filter(|option| !options.iter().any(|known| known == *option))
        .collect();
    assert!(
        not_options.is_empty(),
        "These are not options, remove them from UNTESTED_OPTIONS: {:?}",
        not_options
    );
}
