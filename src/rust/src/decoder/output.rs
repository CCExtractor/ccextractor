use std::{
    fs::File,
    io::Write,
    os::unix::prelude::{FromRawFd, IntoRawFd},
};

use log::debug;

#[cfg(windows)]
use std::os::windows::io::{FromRawHandle, IntoRawHandle, RawHandle};

use crate::{bindings::*, utils::is_true};

// Context for writing subtitles to file
pub struct Writer<'a> {
    pub cea_708_counter: &'a mut u32,
    pub subs_delay: LLONG,
    pub crlf: String,
    pub write_format: ccx_output_format,
    pub writer_ctx: &'a mut dtvcc_writer_ctx,
    pub no_font_color: bool,
    pub transcript_settings: &'a ccx_encoders_transcript_format,
    pub no_bom: i32,
}

impl<'a> Writer<'a> {
    pub fn new(
        cea_708_counter: &'a mut u32,
        subs_delay: LLONG,
        write_format: ccx_output_format,
        writer_ctx: &'a mut dtvcc_writer_ctx,
        no_font_color: i32,
        transcript_settings: &'a ccx_encoders_transcript_format,
        no_bom: i32,
    ) -> Self {
        Self {
            cea_708_counter,
            subs_delay,
            crlf: "\r\n".to_owned(),
            write_format,
            writer_ctx,
            no_font_color: is_true(no_font_color),
            transcript_settings,
            no_bom,
        }
    }
    pub fn write_to_file(&mut self, buf: &[u8]) -> Result<(), String> {
        #[cfg(unix)]
        {
            let mut file = unsafe { File::from_raw_fd(self.writer_ctx.fd) };
            file.write_all(buf).map_err(|err| err.to_string())?;
            self.writer_ctx.fd = file.into_raw_fd();
        }
        #[cfg(windows)]
        {
            let mut file = unsafe { File::from_raw_handle(self.writer_ctx.fhandle) };
            file.write_all(buf).map_err(|err| err.to_string())?;
            self.writer_ctx.fhandle = file.into_raw_handle();
        }
        Ok(())
    }
}

pub fn write_char(sym: &dtvcc_symbol, buf: &mut Vec<u8>) {
    if sym.sym >> 8 != 0 {
        buf.push((sym.sym >> 8) as u8);
        buf.push((sym.sym & 0xff) as u8);
    } else {
        buf.push(sym.sym as u8);
    }
}

pub fn color_to_hex(color: u8) -> (u8, u8, u8) {
    let red = color >> 4;
    let green = (color >> 2) & 0x3;
    let blue = color & 0x3;
    debug!(
        "Color: {} [{:06x}] {} {} {}",
        color, color, red, green, blue
    );
    (red, green, blue)
}
