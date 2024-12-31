use std::io;
use std::io::Write;

use crate::common::Options;

pub trait ActivityExt {
    fn activity_report_version(&mut self);
}
impl ActivityExt for Options {
    fn activity_report_version(&mut self) {
        if self.gui_mode_reports {
            let mut stderr = io::stderr();
            let version = env!("CARGO_PKG_VERSION");
            writeln!(stderr, "###VERSION#CCExtractor#{}", version).unwrap();
            stderr.flush().unwrap();
        }
    }
}
