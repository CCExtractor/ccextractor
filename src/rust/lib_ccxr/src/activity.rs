use std::io;
use std::io::Write;

use crate::common::Options;
use std::sync::atomic::{AtomicUsize, Ordering};

pub static mut NET_ACTIVITY_GUI: AtomicUsize = AtomicUsize::new(0);

pub trait ActivityExt {
    fn activity_report_version(&mut self);
    fn activity_input_file_closed(&mut self);
    fn activity_input_file_open(&mut self, filename: &str);
    fn activity_report_data_read(&mut self);
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
    fn activity_input_file_closed(&mut self) {
        if self.gui_mode_reports {
            let mut stderr = io::stderr();
            writeln!(stderr, "###INPUTFILECLOSED").unwrap();
            stderr.flush().unwrap();
        }
    }

    fn activity_input_file_open(&mut self, filename: &str) {
        if self.gui_mode_reports {
            let mut stderr = io::stderr();
            writeln!(stderr, "###INPUTFILEOPEN#{}", filename).unwrap();
            stderr.flush().unwrap();
        }
    }

    fn activity_report_data_read(&mut self) {
        if self.gui_mode_reports {
            let mut stderr = io::stderr();
            writeln!(stderr, "###DATAREAD#{}", unsafe {
                NET_ACTIVITY_GUI.load(Ordering::SeqCst) / 1000
            })
            .unwrap();
            stderr.flush().unwrap();
        }
    }
}
pub fn update_net_activity_gui(value: usize) {
    unsafe {
        NET_ACTIVITY_GUI.store(value, Ordering::SeqCst);
    }
}
