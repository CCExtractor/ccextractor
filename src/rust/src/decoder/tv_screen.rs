use log::{debug, warn};

#[cfg(windows)]
use std::os::windows::io::IntoRawHandle;
use std::{
    ffi::CStr,
    fs::File,
    os::unix::prelude::{IntoRawFd},
};

use super::{
    output::{color_to_hex, write_char, write_to_file, Writer},
    timing::get_time_str,
    CCX_DTVCC_SCREENGRID_COLUMNS, CCX_DTVCC_SCREENGRID_ROWS,
};
use crate::{
    bindings::*,
    utils::{is_false, is_true},
};

impl dtvcc_tv_screen {
    /// Clear text from tv screen
    pub fn clear(&mut self) {
        for row in 0..CCX_DTVCC_SCREENGRID_ROWS as usize {
            self.chars[row].fill(dtvcc_symbol::default());
        }
        self.time_ms_hide = -1;
        self.time_ms_show = -1;
    }
    pub fn update_time_show(&mut self, time: LLONG) {
        let prev_time_str = get_time_str(self.time_ms_show);
        let curr_time_str = get_time_str(time);
        debug!("Screen show time: {} -> {}", prev_time_str, curr_time_str);
        if self.time_ms_show == -1 || self.time_ms_show > time {
            self.time_ms_show = time;
        }
    }
    pub fn update_time_hide(&mut self, time: LLONG) {
        let prev_time_str = get_time_str(self.time_ms_hide);
        let curr_time_str = get_time_str(time);
        debug!("Screen hide time: {} -> {}", prev_time_str, curr_time_str);
        if self.time_ms_hide == -1 || self.time_ms_hide < time {
            self.time_ms_hide = time;
        }
    }
    pub fn writer_output(&self, writer: &mut Writer) -> Result<(), String> {
        debug!(
            "dtvcc_writer_output: writing... [{:?}][{}]",
            unsafe { CStr::from_ptr(writer.writer_ctx.filename) },
            writer.writer_ctx.fd
        );

        #[cfg(unix)]
        if writer.writer_ctx.filename.is_null() && writer.writer_ctx.fd < 0 {
            return Err("Filename missing".to_owned());
        } else if writer.writer_ctx.fd < 0 {
            let filename = unsafe {
                CStr::from_ptr(writer.writer_ctx.filename)
                    .to_str()
                    .map_err(|err| err.to_string())
            }?;
            debug!("dtvcc_writer_output: creating {}", filename);
            let file = File::create(filename).map_err(|err| err.to_string())?;

            //TODO Check for no bom

            writer.writer_ctx.fd = file.into_raw_fd();
        }

        #[cfg(windows)]
        if writer.writer_ctx.filename.is_null() && writer.fhandle.is_null() {
            return Err("Filename missing".to_owned())?;
        } else if writer.fhandle.is_null() {
            let filename = unsafe {
                CStr::from_ptr(writer.writer_ctx.filename)
                    .to_str()
                    .map_err(|err| err.to_string())
            }?;
            debug!("dtvcc_writer_output: creating {}", filename);
            let file = File::create(filename).map_err(|err| err.to_string())?;

            //TODO Check for no bom

            writer.fhandle = file.into_raw_handle();
        }

        self.write(writer);

        Ok(())
    }
    pub fn get_write_interval(&self, row_index: usize) -> (usize, usize) {
        let mut first = 0;
        let mut last = CCX_DTVCC_SCREENGRID_COLUMNS as usize - 1;
        for col in 0..CCX_DTVCC_SCREENGRID_COLUMNS as usize {
            if self.chars[row_index][col].is_set() {
                first = col;
                break;
            }
        }
        for col in (0..(CCX_DTVCC_SCREENGRID_COLUMNS as usize - 1)).rev() {
            if self.chars[row_index][col].is_set() {
                last = col;
                break;
            }
        }
        (first, last)
    }
    pub fn write(&self, writer: &mut Writer) {
        if writer.write_format == ccx_output_format_CCX_OF_SRT {
            if let Err(e) = self.write_srt(writer) {
                warn!("{}", e);
            }
        }
    }
    pub fn write_row(
        &self,
        writer: &mut Writer,
        row_index: usize,
        use_colors: bool,
    ) -> std::io::Result<()> {
        let mut buf = Vec::new();
        let mut pen_color = dtvcc_pen_color::default();
        let mut pen_attribs = dtvcc_pen_attribs::default();
        let (first, last) = self.get_write_interval(row_index);
        debug!("First: {}, Last: {}", first, last);

        for i in 0..last + 1 {
            if use_colors {
                self.change_pen_color(
                    &pen_color,
                    writer.no_font_color,
                    row_index,
                    i,
                    false,
                    &mut buf,
                );
            }
            self.change_pen_attribs(
                &pen_attribs,
                writer.no_font_color,
                row_index,
                i,
                false,
                &mut buf,
            );
            self.change_pen_attribs(
                &pen_attribs,
                writer.no_font_color,
                row_index,
                i,
                true,
                &mut buf,
            );
            if use_colors {
                self.change_pen_color(
                    &pen_color,
                    writer.no_font_color,
                    row_index,
                    i,
                    true,
                    &mut buf,
                )
            }
            pen_color = self.pen_colors[row_index][i];
            pen_attribs = self.pen_attribs[row_index][i];
            if i < first {
                buf.push(b' ');
            } else {
                write_char(&self.chars[row_index][i], &mut buf)
            }
        }
        // there can be unclosed tags or colors after the last symbol in a row
        if use_colors {
            self.change_pen_color(
                &pen_color,
                writer.no_font_color,
                row_index,
                CCX_DTVCC_SCREENGRID_COLUMNS as usize,
                false,
                &mut buf,
            )
        }
        self.change_pen_attribs(
            &pen_attribs,
            writer.no_font_color,
            row_index,
            CCX_DTVCC_SCREENGRID_COLUMNS as usize,
            false,
            &mut buf,
        );
        // Tags can still be crossed e.g <f><i>text</f></i>, but testing HTML code has shown that they still are handled correctly.

        write_to_file(writer, &buf)?;
        Ok(())
    }
    pub fn write_srt(&self, writer: &mut Writer) -> std::io::Result<()> {
        if self.is_screen_empty(writer) {
            return Ok(());
        }
        if self.time_ms_show + writer.subs_delay < 0 {
            return Ok(());
        }

        let time_show = get_time_str(self.time_ms_show);
        let time_hide = get_time_str(self.time_ms_hide);

        let counter = *writer.cea_708_counter;
        let line = format!(
            "{}{}{} --> {}{}",
            counter, "\r\n", time_show, time_hide, "\r\n"
        );
        write_to_file(writer, line.as_bytes())?;

        for row_index in 0..CCX_DTVCC_SCREENGRID_ROWS as usize {
            if !self.is_row_empty(row_index) {
                self.write_row(writer, row_index, true)?;
                write_to_file(writer, b"\r\n")?;
            }
        }
        write_to_file(writer, b"\r\n")?;
        Ok(())
    }
    pub fn is_screen_empty(&self, writer: &mut Writer) -> bool {
        for index in 0..CCX_DTVCC_SCREENGRID_ROWS as usize {
            if !self.is_row_empty(index) {
                // we will write subtitle
                *writer.cea_708_counter += 1;
                return false;
            }
        }
        true
    }
    pub fn is_row_empty(&self, row_index: usize) -> bool {
        for col_index in 0..CCX_DTVCC_SCREENGRID_COLUMNS as usize {
            if self.chars[row_index][col_index].is_set() {
                return false;
            }
        }
        true
    }
    pub fn change_pen_attribs(
        &self,
        pen_attribs: &dtvcc_pen_attribs,
        no_font_color: bool,
        row_index: usize,
        col_index: usize,
        open: bool,
        buf: &mut Vec<u8>,
    ) {
        if no_font_color {
            return;
        }
        let new_pen_attribs = if col_index >= CCX_DTVCC_SCREENGRID_COLUMNS as usize {
            dtvcc_pen_attribs::default()
        } else {
            self.pen_attribs[row_index][col_index]
        };

        if pen_attribs.italic != new_pen_attribs.italic {
            if is_true(pen_attribs.italic) && !open {
                buf.extend_from_slice(b"</i>");
            } else if is_false(pen_attribs.italic) && open {
                buf.extend_from_slice(b"<i>");
            }
        }
        if pen_attribs.underline != new_pen_attribs.underline {
            if is_true(pen_attribs.underline) && !open {
                buf.extend_from_slice(b"</u>");
            } else if is_false(pen_attribs.underline) && open {
                buf.extend_from_slice(b"<u>");
            }
        }
    }
    pub fn change_pen_color(
        &self,
        pen_color: &dtvcc_pen_color,
        no_font_color: bool,
        row_index: usize,
        col_index: usize,
        open: bool,
        buf: &mut Vec<u8>,
    ) {
        if no_font_color {
            return;
        }
        let new_pen_color = if col_index >= CCX_DTVCC_SCREENGRID_COLUMNS as usize {
            dtvcc_pen_color::default()
        } else {
            self.pen_colors[row_index][col_index]
        };
        if pen_color.fg_color != new_pen_color.fg_color {
            if pen_color.fg_color != 0x3F && !open {
                // should close older non-white color
                buf.extend_from_slice(b"</font>");
            } else if new_pen_color.fg_color != 0x3F && open {
                debug!("Colors: {}", col_index);
                let (mut red, mut green, mut blue) = color_to_hex(new_pen_color.fg_color as u8);
                red *= 255 / 3;
                green *= 255 / 3;
                blue *= 255 / 3;
                let font_tag = format!("<font color=\"{:02x}{:02x}{:02x}\">", red, green, blue);
                buf.extend_from_slice(font_tag.as_bytes());
            }
        }
    }
}
