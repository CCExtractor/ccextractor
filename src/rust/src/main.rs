use args::Args;
use params::parse_parameters;
use structs::{CcxSOptions, CcxSTeletextConfig};

mod args;
mod params;

mod activity;
mod structs;

mod ccx_encoders_helpers;

mod enums;
use clap::Parser;

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
