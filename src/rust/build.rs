use std::env;
use std::path::PathBuf;

fn main() {
    let mut builder = bindgen::Builder::default()
        // The input header we would like to generate
        // bindings for.
        .header("wrapper.h")
        // Tell cargo to invalidate the built crate whenever any of the
        // included header files changed.
        .clang_arg("-DENABLE_HARDSUBX")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks));

    for type_name in ALLOWLIST_TYPES {
        builder = builder.allowlist_type(type_name);
    }

    for fn_name in ALLOWLIST_FUNCTIONS {
        builder = builder.allowlist_function(fn_name);
    }

    for rust_enum in RUSTIFIED_ENUMS {
        builder = builder.rustified_enum(rust_enum);
    }

    let bindings = builder
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

const ALLOWLIST_FUNCTIONS: &[&str] = &[
    ".*(?i)_?dtvcc_.*",
    "get_visible_.*",
    "get_fts",
    "printdata",
    "writercwtdata",
    "edit_distance",
    "convert_pts_to_.*",
    "av_rescale_q"
];
const ALLOWLIST_TYPES: &[&str] = &[".*(?i)_?dtvcc_.*", "encoder_ctx", "lib_cc_decode", "AVRational"];
const RUSTIFIED_ENUMS: &[&str] = &["dtvcc_(window|pen)_.*", "ccx_output_format"];
