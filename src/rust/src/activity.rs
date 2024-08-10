use std::io;
use std::io::Write;

use crate::common::CcxOptions;

impl CcxOptions {
    pub fn activity_report_version(&mut self) {
        if self.gui_mode_reports {
            let mut stderr = io::stderr();
            let version = env!("CARGO_PKG_VERSION");
            writeln!(stderr, "###VERSION#CCExtractor#{}", version).unwrap();
            stderr.flush().unwrap();
        }
    }
}
