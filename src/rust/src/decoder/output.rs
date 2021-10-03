//! Utilty functions to write captions

#[cfg(unix)]
use std::os::unix::prelude::{FromRawFd, IntoRawFd};
#[cfg(windows)]
use std::os::windows::io::{FromRawHandle, IntoRawHandle};
use std::{fs::File, io::Write};

use crate::{bindings::*, utils::is_true};

use log::{debug, warn};

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
    /// Create a new writer context
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
    /// Write subtitles to the file
    ///
    /// File must already exist
    /// On Unix Platforms, uses the raw fd from context to access the file
    /// On Windows Platforms, uses the raw handle from context to access the file
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
    /// Finish writing up any remaining parts
    pub fn write_done(&mut self) {
        if self.write_format == ccx_output_format::CCX_OF_SAMI {
            if let Err(err) = self.write_sami_footer() {
                warn!("{}", err);
            }
        } else {
            debug!("dtvcc_write_done: no handling required");
        }
    }
    /// Writes the footer according to the SAMI format
    pub fn write_sami_footer(&mut self) -> Result<(), String> {
        self.write_to_file(b"</body></sami>")?;
        Ok(())
    }
}

/// Write the symbol to the provided buffer
///
/// If symbol is 8-bit, then it's written to the buffer
/// If symbol is 16-bit, then two 8-bit symbols are written to the buffer
pub fn write_char(sym: &dtvcc_symbol, buf: &mut Vec<u8>) {
    if sym.sym >> 8 != 0 {
        buf.push((sym.sym >> 8) as u8);
        buf.push((sym.sym & 0xff) as u8);
    } else {
        buf.push(sym.sym as u8);
    }
}

/// Convert from CEA-708 color representation to hex code
///
/// Two bits are specified for each red, green, and blue color value which defines the
/// intensity of each individual color component.
///
/// Refer section 8.8 CEA-708-E
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
