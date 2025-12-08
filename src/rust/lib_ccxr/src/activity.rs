#![allow(static_mut_refs)] // Temporary fix for mutable static variable
use crate::common::Options;
use std::io;
use std::io::Write;
use std::os::raw::c_ulong;

pub trait ActivityExt {
    fn activity_report_version(&mut self);
    fn activity_input_file_closed(&mut self);
    fn activity_input_file_open(&mut self, filename: &str);
    fn activity_report_data_read(&mut self, net_activity_gui: &mut c_ulong);
    fn activity_video_info(
        &mut self,
        hor_size: u32,
        vert_size: u32,
        aspect_ratio: &str,
        framerate: &str,
    );
}
impl ActivityExt for Options {
    fn activity_report_version(&mut self) {
        if self.gui_mode_reports {
            let mut stderr = io::stderr();
            let version = env!("CARGO_PKG_VERSION");
            writeln!(stderr, "###VERSION#CCExtractor#{version}").unwrap();
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
            writeln!(stderr, "###INPUTFILEOPEN#{filename}").unwrap();
            stderr.flush().unwrap();
        }
    }

    fn activity_report_data_read(&mut self, net_activity_gui: &mut c_ulong) {
        if self.gui_mode_reports {
            let mut stderr = io::stderr();
            writeln!(stderr, "###DATAREAD#{}", (*net_activity_gui) / 1000).unwrap();
            stderr.flush().unwrap();
        }
    }
    fn activity_video_info(
        &mut self,
        hor_size: u32,
        vert_size: u32,
        aspect_ratio: &str,
        framerate: &str,
    ) {
        if self.gui_mode_reports {
            let mut stderr = io::stderr();
            writeln!(
                stderr,
                "###VIDEOINFO#{}#{}#{}#{}",
                hor_size, vert_size, aspect_ratio, framerate
            )
            .unwrap();
            stderr.flush().unwrap();
        }
    }
}
