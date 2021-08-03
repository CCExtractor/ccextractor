use std::{
    ffi::CStr,
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
    pub write_format: u32,
    pub writer_ctx: &'a mut dtvcc_writer_ctx,
    pub no_font_color: bool,
}

impl<'a> Writer<'a> {
    pub fn new(
        cea_708_counter: &'a mut u32,
        subs_delay: LLONG,
        write_format: u32,
        writer_ctx: &'a mut dtvcc_writer_ctx,
        no_font_color: i32,
    ) -> Self {
        Self {
            cea_708_counter,
            subs_delay,
            crlf: "\r\n".to_owned(),
            write_format,
            writer_ctx,
            no_font_color: is_true(no_font_color),
        }
    }
}

pub fn write_char(sym: &dtvcc_symbol, buf: &mut Vec<u8>) {
    if sym.sym >> 8 != 0 {
        buf.push((sym.sym >> 8) as u8);
        buf.push((sym.sym & 0xff) as u8);
        for x in buf.iter().rev().take(2) {
            println!("Chars:- {:02X}", x);
        }
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

pub fn write_to_file(writer: &mut Writer, buf: &[u8]) -> std::io::Result<()> {
    #[cfg(unix)]
    {
        let mut file = unsafe { File::from_raw_fd(writer.writer_ctx.fd) };
        file.write_all(buf)?;
        writer.writer_ctx.fd = file.into_raw_fd();
    }
    #[cfg(windows)]
    {
        let mut file = unsafe { File::from_raw_handle(writer.writer_ctx.fhandle) };
        file.write_all(buf)?;
        writer.writer_ctx.fhandle = file.into_raw_handle();
    }
    Ok(())
}
