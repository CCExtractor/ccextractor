extern crate pkg_config;
use std::env;
use std::path::PathBuf;

fn main() {
    let mut allowlist_functions = Vec::new();
    allowlist_functions.extend_from_slice(&[
        ".*(?i)_?dtvcc_.*",
        "get_visible_.*",
        "get_fts",
        "printdata",
        "writercwtdata",
        "version",
        "set_binary_mode",
        "print_file_report",
        "ccx_probe_mxf", // shall be removed after mxf
        "ccx_mxf_init",  // shall be removed after mxf
        "ccx_gxf_probe", // shall be removed after gxf
        "ccx_gxf_init",  // shall be removed after gxf
        #[cfg(windows)]
        "_open_osfhandle",
        #[cfg(windows)]
        "_get_osfhandle",
        #[cfg(feature = "enable_ffmpeg")]
        "init_ffmpeg",
        "net_send_header", // shall be removed after NET
        "process_hdcc",
        "anchor_hdcc",
        "store_hdcc",
        "do_cb",
        "decode_vbi",
        "realloc",
        "write_spumux_footer",
        "write_spumux_header",
    ]);

    #[cfg(feature = "hardsubx_ocr")]
    allowlist_functions.extend_from_slice(&[
        "edit_distance",
        "convert_pts_to_.*",
        "av_rescale_q",
        "mprint",
    ]);

    let mut allowlist_types = Vec::new();
    allowlist_types.extend_from_slice(&[
        ".*(?i)_?dtvcc_.*",
        "encoder_ctx",
        "lib_cc_decode",
        "ccx_demuxer",
        "lib_ccx_ctx",
        "bitstream",
        "cc_subtitle",
        "ccx_output_format",
        "ccx_boundary_time",
        "gop_time_code",
        "ccx_common_timing_settings_t",
        "ccx_s_options",
        "ccx_s_teletext_config",
        "ccx_output_date_format",
        "ccx_encoding_type",
        "ccx_decoder_608_settings",
        "ccx_decoder_608_report",
        "ccx_gxf",
        "MXFContext",
        "demuxer_data",
        "eia608_screen",
        "uint8_t",
        "word_list",
    ]);

    #[cfg(feature = "hardsubx_ocr")]
    allowlist_types.extend_from_slice(&["AVRational", "AVPacket", "AVFrame"]);

    let mut builder = bindgen::Builder::default()
        // The input header we would like to generate
        // bindings for.
        .header("wrapper.h");

    // enable hardsubx if and only if the feature is on
    #[cfg(feature = "hardsubx_ocr")]
    {
        builder = builder.clang_arg("-DENABLE_HARDSUBX");

        // Add FFmpeg include paths for Mac
        if cfg!(target_os = "macos") {
            // Try common Homebrew paths
            if std::path::Path::new("/opt/homebrew/include").exists() {
                builder = builder.clang_arg("-I/opt/homebrew/include");
            } else if std::path::Path::new("/usr/local/include").exists() {
                builder = builder.clang_arg("-I/usr/local/include");
            }

            // Check Homebrew Cellar for FFmpeg
            let cellar_ffmpeg = "/opt/homebrew/Cellar/ffmpeg";
            if std::path::Path::new(cellar_ffmpeg).exists() {
                // Find the FFmpeg version directory
                if let Ok(entries) = std::fs::read_dir(cellar_ffmpeg) {
                    for entry in entries {
                        if let Ok(entry) = entry {
                            let include_path = entry.path().join("include");
                            if include_path.exists() {
                                builder =
                                    builder.clang_arg(format!("-I{}", include_path.display()));
                                break;
                            }
                        }
                    }
                }
            }

            // Also check environment variable
            if let Ok(ffmpeg_include) = env::var("FFMPEG_INCLUDE_DIR") {
                builder = builder.clang_arg(format!("-I{}", ffmpeg_include));
            }
        }
    }

    // Tell cargo to invalidate the built crate whenever any of the
    // included header files changed.
    builder = builder.parse_callbacks(Box::new(bindgen::CargoCallbacks));

    for type_name in allowlist_types {
        builder = builder.allowlist_type(type_name);
    }

    for fn_name in allowlist_functions {
        builder = builder.allowlist_function(fn_name);
    }

    for rust_enum in RUSTIFIED_ENUMS {
        builder = builder.rustified_enum(rust_enum);
    }

    let bindings = builder
        .derive_default(true)
        .no_default("dtvcc_pen_attribs|dtvcc_pen_color|dtvcc_symbol")
        // Finish the builder and generate the bindings.
        .generate()
        // Unwrap the Result and panic on failure.
        .expect("Unable to generate bindings");

    // Write the bindings to the $OUT_DIR/bindings.rs file.
    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("bindings.rs"))
        .expect("Couldn't write bindings!");
}

const RUSTIFIED_ENUMS: &[&str] = &[
    "dtvcc_(window|pen)_.*",
    "ccx_output_format",
    "ccx_output_date_format",
];
