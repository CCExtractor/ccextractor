use std::io;
use std::io::Write;

use crate::structs::CcxSOptions;

pub fn activity_report_version(ccx_options: &mut CcxSOptions) {
    if ccx_options.gui_mode_reports {
        let mut stderr = io::stderr();
        let version = env!("CARGO_PKG_VERSION");
        writeln!(stderr, "###VERSION#CCExtractor#{}", version).unwrap();
        stderr.flush().unwrap();
    }
}
