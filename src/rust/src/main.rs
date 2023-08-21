pub mod args;
pub mod parser;

pub mod activity;

pub mod ccx_encoders_helpers;
pub mod common;

use args::Args;
use clap::Parser;
use common::{CcxOptions, CcxTeletextConfig};
use parser::parse_parameters;

fn main() {
    let args: Args = Args::parse();

    let mut opt = CcxOptions {
        ..Default::default()
    };
    let mut tlt_config: CcxTeletextConfig = CcxTeletextConfig {
        ..Default::default()
    };

    parse_parameters(&mut opt, &args, &mut tlt_config);
}
