//! TV screen to be displayed
//!
//! TV screen contains the captions to be displayed.
//! Captions are added to TV screen from a window when any of DSW, HDW, TGW, DLW or CR commands are received  

use std::cmp::Ordering;
#[cfg(unix)]
use std::os::unix::prelude::IntoRawFd;
#[cfg(windows)]
use std::os::windows::io::IntoRawHandle;
use std::{ffi::CStr, fs::File};

use super::output::{color_to_hex, write_char, Writer};
use super::timing::{get_scc_time_str, get_time_str};
use super::{CCX_DTVCC_SCREENGRID_COLUMNS, CCX_DTVCC_SCREENGRID_ROWS};
use crate::{
    bindings::*,
    utils::{is_false, is_true},
};

use log::{debug, warn};

impl dtvcc_tv_screen {
    /// Clear all text from TV screen
    pub fn clear(&mut self) {
        for row in 0..CCX_DTVCC_SCREENGRID_ROWS as usize {
            self.chars[row].fill(dtvcc_symbol::default());
        }
        self.time_ms_hide = -1;
        self.time_ms_show = -1;
    }

    /// Update TV screen show time
    pub fn update_time_show(&mut self, time: LLONG) {
        let prev_time_str = get_time_str(self.time_ms_show);
        let curr_time_str = get_time_str(time);
        debug!("Screen show time: {} -> {}", prev_time_str, curr_time_str);
        if self.time_ms_show == -1 || self.time_ms_show > time {
            self.time_ms_show = time;
        }
    }

    /// Update TV screen hide time
    pub fn update_time_hide(&mut self, time: LLONG) {
        let prev_time_str = get_time_str(self.time_ms_hide);
        let curr_time_str = get_time_str(time);
        debug!("Screen hide time: {} -> {}", prev_time_str, curr_time_str);
        if self.time_ms_hide == -1 || self.time_ms_hide < time {
            self.time_ms_hide = time;
        }
    }

    /// Write captions to the output file
    ///
    /// Opens a new file when called for the first time.
    /// Uses the already open file on subsequent calls.
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
            writer.writer_ctx.fd = file.into_raw_fd();

            if is_false(writer.no_bom) {
                let BOM = [0xef, 0xbb, 0xbf];
                writer.write_to_file(&BOM)?;
            }
        }

        #[cfg(windows)]
        if writer.writer_ctx.filename.is_null() && writer.writer_ctx.fhandle.is_null() {
            return Err("Filename missing".to_owned())?;
        } else if writer.writer_ctx.fhandle.is_null() {
            let filename = unsafe {
                CStr::from_ptr(writer.writer_ctx.filename)
                    .to_str()
                    .map_err(|err| err.to_string())
            }?;
            debug!("dtvcc_writer_output: creating {}", filename);
            let file = File::create(filename).map_err(|err| err.to_string())?;
            writer.writer_ctx.fhandle = file.into_raw_handle();

            if is_false(writer.no_bom) {
                let BOM = [0xef, 0xbb, 0xbf];
                writer.write_to_file(&BOM)?;
            }
        }
        self.write(writer);
        Ok(())
    }

    /// Returns the bounds in which captions are present
    pub fn get_write_interval(&self, row_index: usize) -> (usize, usize) {
        let mut first = 0;
        let mut last = 0;

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

    /// Write captions according to the output file type
    ///
    /// Calls the respective function for the output file type
    pub fn write(&self, writer: &mut Writer) {
        let result = match writer.write_format {
            ccx_output_format::CCX_OF_SRT => self.write_srt(writer),
            ccx_output_format::CCX_OF_SAMI => self.write_sami(writer),
            ccx_output_format::CCX_OF_TRANSCRIPT => self.write_transcript(writer),
            ccx_output_format::CCX_OF_SCC => self.write_scc(writer),
            _ => {
                self.write_debug();
                Err("Unsupported write format".to_owned())
            }
        };
        if let Err(err) = result {
            warn!("{}", err);
        }
    }

    /// Write all captions from the row to the output file
    ///
    /// If use_colors is 'true' then <font color="xxx"></font> tags are added to the output
    pub fn write_row(
        &self,
        writer: &mut Writer,
        row_index: usize,
        use_colors: bool,
    ) -> Result<(), String> {
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
        if writer.writer_ctx.cd != (-1_isize) as iconv_t {
            if writer.writer_ctx.charset.is_null() {
                debug!("Charset: null");
            } else {
                let charset = unsafe {
                    CStr::from_ptr(writer.writer_ctx.charset)
                        .to_str()
                        .map_err(|err| err.to_string())?
                };
                debug!("Charset: {}", charset);

                let op = iconv::decode(&buf, charset).map_err(|err| err.to_string())?;
                writer.write_to_file(op.as_bytes())?;
            }
        } else {
            writer.write_to_file(&buf)?;
        }

        Ok(())
    }

    /// Write captions in SRT format
    pub fn write_srt(&self, writer: &mut Writer) -> Result<(), String> {
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
        writer.write_to_file(line.as_bytes())?;

        for row_index in 0..CCX_DTVCC_SCREENGRID_ROWS as usize {
            if !self.is_row_empty(row_index) {
                self.write_row(writer, row_index, true)?;
                writer.write_to_file(b"\r\n")?;
            }
        }
        writer.write_to_file(b"\r\n")?;
        Ok(())
    }

    /// Write captions in Transcripts format
    pub fn write_transcript(&self, writer: &mut Writer) -> Result<(), String> {
        if self.is_screen_empty(writer) {
            return Ok(());
        }
        if self.time_ms_show + writer.subs_delay < 0 {
            return Ok(());
        }

        let time_show = get_time_str(self.time_ms_show);
        let time_hide = get_time_str(self.time_ms_hide);

        for row_index in 0..CCX_DTVCC_SCREENGRID_ROWS as usize {
            if !self.is_row_empty(row_index) {
                let mut buf = String::new();

                if is_true(writer.transcript_settings.showStartTime) {
                    buf.push_str(&time_show);
                    buf.push('|');
                }
                if is_true(writer.transcript_settings.showEndTime) {
                    buf.push_str(&time_hide);
                    buf.push('|');
                }
                if is_true(writer.transcript_settings.showCC) {
                    buf.push_str("CC1|"); //always CC1 because CEA-708 is field-independent
                }
                if is_true(writer.transcript_settings.showMode) {
                    buf.push_str("POP|"); //TODO caption mode(pop, rollup, etc.)
                }
                writer.write_to_file(buf.as_bytes())?;
                self.write_row(writer, row_index, false)?;
                writer.write_to_file(b"\r\n")?;
            }
        }
        Ok(())
    }

    /// Write captions in SAMI format
    pub fn write_sami(&self, writer: &mut Writer) -> Result<(), String> {
        if self.is_screen_empty(writer) {
            return Err("Sami:- Screen is empty".to_owned());
        }
        if self.time_ms_show + writer.subs_delay < 0 {
            return Err(format!(
                "Sami:- timing is -ve, {}:{}",
                self.time_ms_show, writer.subs_delay
            ));
        }
        if self.cc_count == 1 {
            self.write_sami_header(writer)?;
        }
        let buf = format!(
            "<sync start={}><p class=\"unknowncc\">\r\n",
            self.time_ms_show + writer.subs_delay
        );
        writer.write_to_file(buf.as_bytes())?;

        for row_index in 0..CCX_DTVCC_SCREENGRID_ROWS as usize {
            if !self.is_row_empty(row_index) {
                self.write_row(writer, row_index, true)?;
                writer.write_to_file("<br>\r\n".as_bytes())?;
            }
        }
        let buf = format!(
            "<sync start={}><p class=\"unknowncc\">&nbsp;</p></sync>\r\n\r\n",
            self.time_ms_hide + writer.subs_delay
        );
        writer.write_to_file(buf.as_bytes())?;
        Ok(())
    }

    /// Writes the header according to the SAMI format
    pub fn write_sami_header(&self, writer: &mut Writer) -> Result<(), String> {
        let buf = b"<sami>\r\n\
                            <head>\r\n\
                            <style type=\"text/css\">\r\n\
                            <!--\r\n\
                            p {margin-left: 16pt; margin-right: 16pt; margin-bottom: 16pt; margin-top: 16pt;\r\n\
                            text-align: center; font-size: 18pt; font-family: arial; font-weight: bold; color: #f0f0f0;}\r\n\
                            .unknowncc {Name:Unknown; lang:en-US; SAMIType:CC;}\r\n\
                            -->\r\n\
                            </style>\r\n\
                            </head>\r\n\r\n\
                            <body>\r\n";

        writer.write_to_file(buf)?;
        Ok(())
    }

    fn count_captions_lines_scc(&self) -> usize {
        (0..CCX_DTVCC_SCREENGRID_ROWS)
            .filter(|&row_index| !self.is_row_empty(row_index as usize))
            .count()
    }

    /// Write captions in SCC format
    pub fn write_scc(&self, writer: &mut Writer) -> Result<(), String> {
        fn adjust_odd_parity(value: u8) -> u8 {
            let mut ones = 0;
            for i in 0..=7 {
                if value & (1 << i) != 0 {
                    ones += 1;
                }
            }
            if ones % 2 == 0 {
                0b10000000 | value
            } else {
                value
            }
        }
        // This function is designed to assign appropriate SSC labels for positioning subtitles based on their length.
        // In some scenarios where the video stream provides lengthy subtitles that cannot fit within a single line.
        // Single-line subtitle can be placed in 15th row(most bottom row)
        // 2 line length subtitles can be placed in 14th and 15th row
        // 3 line length subtitles can be placed in 13th, 14th and 15th row
        fn add_needed_scc_labels(
            buf: &mut String,
            total_subtitle_count: usize,
            current_subtitle_count: usize,
        ) {
            match total_subtitle_count {
                // row 15, column 00
                1 => buf.push_str(" 94e0 94e0"),
                2 => {
                    if current_subtitle_count == 1 {
                        // row 14, column 00
                        buf.push_str(" 9440 9440");
                    } else {
                        // row 15, column 00
                        buf.push_str(" 94e0 94e0")
                    }
                }
                _ => {
                    if current_subtitle_count == 1 {
                        // row 13, column 04
                        buf.push_str(" 13e0 13e0");
                    } else if current_subtitle_count == 2 {
                        // row 14, column 00
                        buf.push_str(" 9440 9440");
                    } else {
                        // row 15, column 00
                        buf.push_str(" 94e0 94e0")
                    }
                }
            }
        }
        if self.is_screen_empty(writer) {
            return Ok(());
        }

        if self.time_ms_show + writer.subs_delay < 0 {
            return Ok(());
        }

        if self.cc_count == 2 {
            writer.write_to_file(b"Scenarist_SCC V1.0\n\n")?;
        }

        if writer.old_cc_time_end == 0 {
            writer.old_cc_time_end = self.time_ms_show as i32;
        }

        let mut buf = String::new();
        let mut time_show = ccx_boundary_time::get_time(self.time_ms_show);
        let time_end = ccx_boundary_time::get_time(self.time_ms_hide);

        // Caption overlapping situation
        match writer.old_cc_time_end.cmp(&(time_show.time_in_ms as i32)) {
            Ordering::Greater => {
                // Correct the frame delay
                time_show.time_in_ms -= 1000 / 29.97 as i64;
                buf.push_str(&(get_scc_time_str(time_show) + "\t942c 942c ").to_owned());
                time_show.time_in_ms += 1000 / 29.97 as i64;
                // Clear the buffer and start pop on caption
                buf.push_str("94ae 94ae 9420 9420");
            }
            Ordering::Less => {
                // Clear the screen for new caption
                let time_to_display = ccx_boundary_time::get_time(writer.old_cc_time_end as i64);
                buf.push_str(&(get_scc_time_str(time_to_display) + "\t942c 942c \n\n").to_owned());
                // Correct the frame delay
                time_show.time_in_ms -= 1000 / 29.97 as i64;
                // Clear the buffer and start pop on caption in new time
                buf.push_str(&(get_scc_time_str(time_show) + "\t94ae 94ae 9420 9420").to_owned());
                time_show.time_in_ms += 1000 / 29.97 as i64;
            }
            Ordering::Equal => {
                time_show.time_in_ms -= 1000 / 29.97 as i64;
                buf.push_str(
                    &(get_scc_time_str(time_show) + "\t942c 942c 94ae 94ae 9420 9420").to_owned(),
                );
                time_show.time_in_ms += 1000 / 29.97 as i64;
            }
        }

        let total_subtitle_count = self.count_captions_lines_scc();
        let mut current_subtitle_count = 0;

        for row_index in 0..CCX_DTVCC_SCREENGRID_ROWS as usize {
            if !self.is_row_empty(row_index) {
                current_subtitle_count += 1;
                add_needed_scc_labels(&mut buf, total_subtitle_count, current_subtitle_count);

                let (first, last) = self.get_write_interval(row_index);
                debug!("First: {}, Last: {}", first, last);

                let mut bytes_written = 0;
                for i in 0..last + 1 {
                    if bytes_written % 2 == 0 {
                        buf.push(' ');
                    }
                    let adjusted_val = adjust_odd_parity(self.chars[row_index][i].sym as u8);
                    buf = format!("{}{:x}", buf, adjusted_val);
                    bytes_written += 1;
                }
                // add 0x80 padding and form byte pair if the last byte pair is not form
                if bytes_written % 2 == 1 {
                    buf.push_str("80 ");
                } else {
                    buf.push(' ');
                }
            }
        }

        // Display caption (942f 942f)
        buf.push_str("942f 942f \n\n");
        writer.write_to_file(buf.as_bytes())?;

        writer.old_cc_time_end = time_end.time_in_ms as i32;
        Ok(())
    }

    /// Write debug messages
    ///
    /// Write all characters,show and hide time as a debug log
    pub fn write_debug(&self) {
        let time_show = get_time_str(self.time_ms_show);
        let time_hide = get_time_str(self.time_ms_hide);
        debug!("{} --> {}", time_show, time_hide);

        for row_index in 0..CCX_DTVCC_SCREENGRID_ROWS as usize {
            if !self.is_row_empty(row_index) {
                let mut buf = String::new();
                let (first, last) = self.get_write_interval(row_index);
                for sym in self.chars[row_index][first..=last].iter() {
                    buf.push_str(&format!("{:04X},", sym.sym));
                }
                debug!("{}", buf);
            }
        }
    }

    /// Returns `true` if TV screen has no text
    ///
    /// If any text is found then 708 counter is incremented
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

    /// Returns `true` if row has no text
    pub fn is_row_empty(&self, row_index: usize) -> bool {
        for col_index in 0..CCX_DTVCC_SCREENGRID_COLUMNS as usize {
            if self.chars[row_index][col_index].is_set() {
                return false;
            }
        }
        true
    }

    /// Add underline(<u>) and italic(<i>) tags according to the pen attributes
    ///
    /// Open specifies if tag is an opening or closing tag
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

    /// Add font(<font color="xxx">) tag according to the pen color
    ///
    /// Open specifies if tag is an opening or closing tag
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
                let font_tag = format!("<font color=\"#{:02x}{:02x}{:02x}\">", red, green, blue);
                buf.extend_from_slice(font_tag.as_bytes());
            }
        }
    }
}

#[cfg(test)]
mod test {
    use crate::utils::get_zero_allocated_obj;

    use super::*;

    #[test]
    fn test_clear() {
        let mut screen = get_zero_allocated_obj::<dtvcc_tv_screen>();
        screen.time_ms_show = 1000;
        screen.time_ms_hide = 2000;
        screen.chars = [[dtvcc_symbol { sym: 1, init: 2 }; CCX_DTVCC_SCREENGRID_COLUMNS as usize];
            CCX_DTVCC_SCREENGRID_ROWS as usize];

        // Clear the screen will clear the chars and timings
        screen.clear();

        assert_eq!(screen.time_ms_show, -1);
        assert_eq!(screen.time_ms_hide, -1);
        for row in 0..CCX_DTVCC_SCREENGRID_ROWS as usize {
            for col in 0..CCX_DTVCC_SCREENGRID_COLUMNS as usize {
                assert_eq!(screen.chars[row][col], dtvcc_symbol::default());
            }
        }
    }

    #[test]
    fn test_update_time_show() {
        let mut screen = get_zero_allocated_obj::<dtvcc_tv_screen>();
        screen.time_ms_show = -1;

        // Case 1: time_ms_show = -1     -> Update time show
        screen.update_time_show(2000);
        assert_eq!(screen.time_ms_show, 2000);

        // Case 2: time_ms_show > time   -> Update time show
        screen.update_time_show(1000);
        assert_eq!(screen.time_ms_show, 1000);

        // Case 3: time_ms_show < time   -> Do not update time show
        screen.update_time_show(2000);
        assert_eq!(screen.time_ms_show, 1000);
    }

    #[test]
    fn test_update_time_hide() {
        let mut screen = get_zero_allocated_obj::<dtvcc_tv_screen>();
        screen.time_ms_hide = -1;

        // Case 1: time_ms_show = -1     -> Update time hide
        screen.update_time_hide(2000);
        assert_eq!(screen.time_ms_hide, 2000);

        // Case 2: time_ms_show < time   -> Update time hide
        screen.update_time_hide(3000);
        assert_eq!(screen.time_ms_hide, 3000);

        // Case 3: time_ms_show > time   -> Do not update time hide
        screen.update_time_hide(2000);
        assert_eq!(screen.time_ms_hide, 3000);
    }

    #[test]
    fn test_get_write_interval() {
        let mut screen = get_zero_allocated_obj::<dtvcc_tv_screen>();
        screen.chars[0][2] = dtvcc_symbol::new(0x41);
        screen.chars[0][3] = dtvcc_symbol::new(0x42);
        screen.chars[0][4] = dtvcc_symbol::new(0x43);
        screen.chars[1][4] = dtvcc_symbol::new(0x43);

        // Mulitple row filed
        assert_eq!(screen.get_write_interval(0), (2, 4));
        // Single row filed
        assert_eq!(screen.get_write_interval(1), (4, 4));
        // Empty rows
        assert_eq!(screen.get_write_interval(2), (0, 0));
    }

    #[test]
    fn test_count_captions_lines_scc() {
        let mut screen = get_zero_allocated_obj::<dtvcc_tv_screen>();

        // No captions
        assert_eq!(screen.count_captions_lines_scc(), 0);

        // Set some non-default values
        screen.chars[0][2] = dtvcc_symbol::new(0x41);
        screen.chars[1][2] = dtvcc_symbol::new(0x42);
        screen.chars[2][2] = dtvcc_symbol::new(0x43);

        assert_eq!(screen.count_captions_lines_scc(), 3);
    }

    #[test]
    fn test_is_row_empty() {
        let mut screen = get_zero_allocated_obj::<dtvcc_tv_screen>();

        // Default emty row check
        assert!(screen.is_row_empty(0));
        assert!(screen.is_row_empty(1));

        // Non-default emty row check
        screen.chars[0][0] = dtvcc_symbol::new(0x51);
        assert!(!screen.is_row_empty(0));
        assert!(screen.is_row_empty(1));
    }
}
