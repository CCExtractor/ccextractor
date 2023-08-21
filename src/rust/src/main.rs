pub mod args;
pub mod parser;

pub mod activity;

pub mod ccx_encoders_helpers;
pub mod common;

use args::Args;
use clap::Parser;
use common::{CcxOptions, CcxTeletextConfig};

fn main() {
    let args: Args = Args::parse();

    let mut options: CcxOptions = CcxOptions {
        ..Default::default()
    };
    let mut tlt_config: CcxTeletextConfig = CcxTeletextConfig {
        ..Default::default()
    };

    options.parse_parameters(&args, &mut tlt_config);
}
