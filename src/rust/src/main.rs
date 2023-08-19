pub mod args;
pub mod params;

pub mod activity;
pub mod structs;

pub mod ccx_encoders_helpers;
pub mod enums;

use args::Args;
use clap::Parser;
use params::parse_parameters;
use structs::{CcxSOptions, CcxSTeletextConfig};

fn main() {
    let args: Args = Args::parse();

    let mut opt = CcxSOptions {
        ..Default::default()
    };
    let mut tlt_config: CcxSTeletextConfig = CcxSTeletextConfig {
        ..Default::default()
    };

    parse_parameters(&mut opt, &args, &mut tlt_config);
}
