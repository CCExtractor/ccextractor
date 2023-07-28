mod args;
use args::Args;
 
use clap::Parser;

fn main() {
    let _cli = Args::parse();
    println!("Issues? Open a ticket here\n https://github.com/CCExtractor/ccextractor/issues");
}