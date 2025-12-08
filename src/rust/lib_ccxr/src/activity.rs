use std::io;
use std::io::Write;

use crate::common::Options;

pub trait ActivityExt {
    fn activity_report_version(&mut self);
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
