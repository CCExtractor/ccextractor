//! Rust library for CCExtractor
//!
//! Currently we are in the process of porting the 708 decoder to rust. See [decoder]

// Allow C naming style
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

use env_logger::{builder, Target};
use log::LevelFilter;
use std::io::Write;

/// CCExtractor C bindings generated by bindgen
#[allow(clippy::all)]
pub mod bindings {
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}

pub mod decoder;

/// Initialize env logger
#[no_mangle]
pub extern "C" fn init_logger() {
    builder()
        .format(|buf, record| writeln!(buf, "[CEA-708] {}", record.args()))
        .filter_level(LevelFilter::Debug)
        .target(Target::Stdout)
        .init();
}
