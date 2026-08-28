#[cfg(not(test))]
mod implementation {
    use crate::bindings::*;
    use crate::encoder::common::encode_line;
    use crate::encoder::g608::write_wrapped;
    use crate::libccxr_exports::time::ccxr_millis_to_time;
    use lib_ccxr::common::CCX_DECODER_608_SCREEN_WIDTH;
    use lib_ccxr::debug;
    use lib_ccxr::util::log::DebugMessageFlag;
    use std::os::raw::c_int;

    /// Wrapper around encoder_ctx for writing SRT subtitles.
    ///
    /// Follows the same pattern as decoder/output.rs Writer —
    /// wraps the C context into a Rust struct with safe methods.
    pub struct SrtWriter<'a> {
        pub ctx: &'a mut encoder_ctx,
        pub out_fh: c_int,
        pub srt_counter: &'a mut u32,
        pub crlf: Vec<u8>,
    }

    impl<'a> SrtWriter<'a> {
        /// Create a new SrtWriter from an encoder context.
        pub fn from_context(ctx: &'a mut encoder_ctx) -> Self {
            let out_fh = unsafe { (*ctx.out).fh };
            let crlf_len = ctx.encoded_crlf_length as usize;
            let crlf =
                unsafe { std::slice::from_raw_parts(ctx.encoded_crlf.as_ptr(), crlf_len) }.to_vec();
            let counter = &mut ctx.srt_counter;
            // We need raw pointer to avoid double borrow — counter lives inside ctx
            let counter_ptr = counter as *mut u32;
            SrtWriter {
                ctx,
                out_fh,
                srt_counter: unsafe { &mut *counter_ptr },
                crlf,
            }
        }

        /// Create a new SrtWriter with a specific output file and counter.
        /// Used for teletext multi-page output (issue #665).
        pub fn with_output(
            ctx: &'a mut encoder_ctx,
            out_fh: c_int,
            srt_counter: &'a mut u32,
        ) -> Self {
            let crlf_len = ctx.encoded_crlf_length as usize;
            let crlf =
                unsafe { std::slice::from_raw_parts(ctx.encoded_crlf.as_ptr(), crlf_len) }.to_vec();
            SrtWriter {
                ctx,
                out_fh,
                srt_counter,
                crlf,
            }
        }

        /// Get CRLF as string (for format! macros).
        fn crlf_str(&self) -> &str {
            std::str::from_utf8(&self.crlf).unwrap_or("\r\n")
        }

        /// Encode text through the encoder context buffer and return the encoded bytes.
        fn encode(&mut self, text: &[u8]) -> Vec<u8> {
            let cap = self.ctx.capacity as usize;
            let buf = unsafe { std::slice::from_raw_parts_mut(self.ctx.buffer, cap) };
            let used = encode_line(self.ctx, buf, text) as usize;
            buf[..used].to_vec()
        }

        /// Write bytes to the output file.
        fn write(&self, data: &[u8]) -> Result<(), String> {
            write_wrapped(self.out_fh, data).map_err(|e| e.to_string())
        }

        /// Write a string subtitle as SRT.
        ///
        /// Handles counter increment, timestamp formatting with -1ms overlap
        /// prevention, and \\n unescape to actual line breaks.
        pub fn write_stringz(
            &mut self,
            string: &str,
            ms_start: i64,
            ms_end: i64,
        ) -> Result<(), String> {
            if string.is_empty() {
                return Ok(());
            }

            let (h1, m1, s1, ms1) = millis_to_time(ms_start);
            let (h2, m2, s2, ms2) = millis_to_time(ms_end - 1); // -1 overlap prevention

            // Write counter
            *self.srt_counter += 1;
            let crlf = self.crlf_str().to_owned();
            let counter_line = format!("{}{}", *self.srt_counter, crlf);
            let encoded = self.encode(counter_line.as_bytes());
            self.write(&encoded)?;

            // Write timestamp
            let timeline = format!(
                "{:02}:{:02}:{:02},{:03} --> {:02}:{:02}:{:02},{:03}{}",
                h1, m1, s1, ms1, h2, m2, s2, ms2, crlf
            );
            let encoded = self.encode(timeline.as_bytes());

            debug!(msg_type = DebugMessageFlag::DECODER_608; "\n- - - SRT caption - - -\n");
            debug!(msg_type = DebugMessageFlag::DECODER_608; "{}", timeline);

            self.write(&encoded)?;

            // Unescape \\n and write each line
            for part in string.split("\\n") {
                if part.is_empty() {
                    continue;
                }
                let mut el = vec![0u8; part.len() * 3 + 1];
                let u = encode_line(self.ctx, &mut el, part.as_bytes()) as usize;

                if self.ctx.encoding != ccx_encoding_type_CCX_ENC_UNICODE {
                    debug!(msg_type = DebugMessageFlag::DECODER_608; "\r");
                    if let Ok(s) = std::str::from_utf8(&el[..u]) {
                        debug!(msg_type = DebugMessageFlag::DECODER_608; "{}\n", s);
                    }
                }

                self.write(&el[..u])?;
                self.write(&self.crlf.clone())?;
            }

            debug!(msg_type = DebugMessageFlag::DECODER_608; "- - - - - - - - - - - -\r\n");

            self.write(&self.crlf.clone())?;
            Ok(())
        }

        /// Write a CEA-608 screen buffer as SRT.
        ///
        /// Handles 15-row screen with autodash detection for speaker changes.
        pub fn write_cc_buffer(&mut self, screen: &eia608_screen) -> Result<bool, String> {
            let mut wrote_something = false;
            let mut prev_line_start: i32 = -1;
            let mut prev_line_end: i32 = -1;
            let mut prev_line_center1: i32 = -1;
            let mut prev_line_center2: i32 = -1;

            // Skip empty screens
            if (0..15).all(|i| screen.row_used[i] == 0) {
                return Ok(false);
            }

            let (h1, m1, s1, ms1) = millis_to_time(screen.start_time);
            let (h2, m2, s2, ms2) = millis_to_time(screen.end_time - 1);

            // Write counter
            *self.srt_counter += 1;
            let crlf = self.crlf_str().to_owned();
            let counter_line = format!("{}{}", *self.srt_counter, crlf);
            let encoded = self.encode(counter_line.as_bytes());
            self.write(&encoded)?;

            // Write timestamp
            let timeline = format!(
                "{:02}:{:02}:{:02},{:03} --> {:02}:{:02}:{:02},{:03}{}",
                h1, m1, s1, ms1, h2, m2, s2, ms2, crlf
            );
            let encoded = self.encode(timeline.as_bytes());
            self.write(&encoded)?;

            debug!(msg_type = DebugMessageFlag::DECODER_608; "\n- - - SRT caption ( {}) - - -\n", *self.srt_counter);
            debug!(msg_type = DebugMessageFlag::DECODER_608; "{}", timeline);

            for i in 0..15usize {
                if screen.row_used[i] == 0 {
                    continue;
                }

                // Autodash logic
                if self.ctx.autodash != 0 && self.ctx.trim_subs != 0 {
                    let line = &screen.characters[i];
                    let width = CCX_DECODER_608_SCREEN_WIDTH;
                    let mut first: i32 = -1;
                    let mut last: i32 = -1;

                    for (j, &ch) in line.iter().enumerate().take(width) {
                        if ch != b' ' && ch != 0 {
                            if first == -1 {
                                first = j as i32;
                            }
                            last = j as i32;
                        }
                    }

                    if first != -1 && last != -1 {
                        let mut do_dash = true;
                        let mut colon_pos: i32 = -1;

                        for j in first..=last {
                            let ch = line[j as usize];
                            if ch == b':' {
                                colon_pos = j;
                                break;
                            }
                            if !(ch as char).is_ascii_uppercase() {
                                break;
                            }
                        }

                        if prev_line_start == -1 {
                            do_dash = false;
                        }
                        if first == prev_line_start {
                            do_dash = false;
                        }
                        if last == prev_line_end {
                            do_dash = false;
                        }
                        if first > prev_line_start && last < prev_line_end {
                            do_dash = false;
                        }
                        if (first > prev_line_start && first < prev_line_end)
                            || (last > prev_line_start && last < prev_line_end)
                        {
                            do_dash = false;
                        }

                        let center1 = (first + last) / 2;
                        let center2 = if colon_pos != -1 {
                            let mut cp = colon_pos;
                            while (cp as usize) < width {
                                let ch = line[cp as usize];
                                if ch != b':' && ch != b' ' && ch != 0x89 {
                                    break;
                                }
                                cp += 1;
                            }
                            (cp + last) / 2
                        } else {
                            center1
                        };

                        if center1 >= prev_line_center1 - 1
                            && center1 <= prev_line_center1 + 1
                            && center1 != -1
                        {
                            do_dash = false;
                        }
                        if center2 >= prev_line_center2 - 2
                            && center2 <= prev_line_center2 + 2
                            && center2 != -1
                        {
                            do_dash = false;
                        }

                        if do_dash {
                            let _ = self.write(b"- ");
                        }

                        prev_line_start = first;
                        prev_line_end = last;
                        prev_line_center1 = center1;
                        prev_line_center2 = center2;
                    }
                }

                // Encode and write the line
                let ctx_ptr = self.ctx as *mut encoder_ctx;
                let length = unsafe {
                    crate::get_decoder_line_encoded(
                        ctx_ptr,
                        self.ctx.subline,
                        i as c_int,
                        screen as *const eia608_screen,
                    )
                };
                let sub_slice =
                    unsafe { std::slice::from_raw_parts(self.ctx.subline, length as usize) };

                if self.ctx.encoding != ccx_encoding_type_CCX_ENC_UNICODE {
                    debug!(msg_type = DebugMessageFlag::DECODER_608; "\r");
                    if let Ok(s) = std::str::from_utf8(sub_slice) {
                        debug!(msg_type = DebugMessageFlag::DECODER_608; "{}\n", s);
                    }
                }

                if self.write(sub_slice).is_ok() {
                    wrote_something = true;
                }
                let _ = self.write(&self.crlf.clone());
            }

            debug!(msg_type = DebugMessageFlag::DECODER_608; "- - - - - - - - - - - -\r\n");

            let _ = self.write(&self.crlf.clone());
            Ok(wrote_something)
        }
    }

    /// Convert milliseconds to (hours, minutes, seconds, milliseconds).
    fn millis_to_time(ms: i64) -> (u32, u32, u32, u32) {
        let mut h: u32 = 0;
        let mut m: u32 = 0;
        let mut s: u32 = 0;
        let mut ms_out: u32 = 0;
        unsafe {
            ccxr_millis_to_time(ms, &mut h, &mut m, &mut s, &mut ms_out);
        }
        (h, m, s, ms_out)
    }

    // ═══════════════════════════════════════════════════════════════
    // FFI exports — thin wrappers for C callers
    // ═══════════════════════════════════════════════════════════════

    #[cfg(not(test))]
    #[cfg(not(test))]
    /// FFI: Write a string as SRT to the default output.
    ///
    /// # Safety
    /// Caller must ensure context and string are valid pointers.
    #[no_mangle]
    pub unsafe extern "C" fn ccxr_write_stringz_as_srt(
        string: *const i8,
        context: *mut encoder_ctx,
        ms_start: i64,
        ms_end: i64,
    ) -> c_int {
        if context.is_null() || string.is_null() {
            return -1;
        }
        let ctx = &mut *context;
        let c_str = match std::ffi::CStr::from_ptr(string).to_str() {
            Ok(s) => s,
            Err(_) => return -1,
        };
        let mut writer = SrtWriter::from_context(ctx);
        match writer.write_stringz(c_str, ms_start, ms_end) {
            Ok(()) => 0,
            Err(_) => -1,
        }
    }

    #[cfg(not(test))]
    #[cfg(not(test))]
    /// FFI: Write a CEA-608 screen buffer as SRT.
    ///
    /// # Safety
    /// Caller must ensure data and context are valid pointers.
    #[no_mangle]
    pub unsafe extern "C" fn ccxr_write_cc_buffer_as_srt(
        data: *const eia608_screen,
        context: *mut encoder_ctx,
    ) -> c_int {
        if context.is_null() || data.is_null() {
            return 0;
        }
        let ctx = &mut *context;
        let screen = &*data;
        let mut writer = SrtWriter::from_context(ctx);
        match writer.write_cc_buffer(screen) {
            Ok(true) => 1,
            Ok(false) => 0,
            Err(_) => 0,
        }
    }

    #[cfg(not(test))]
    #[cfg(not(test))]
    /// FFI: Write a cc_subtitle linked list as SRT.
    ///
    /// # Safety
    /// Caller must ensure sub and context are valid pointers. Frees subtitle data after writing.
    #[no_mangle]
    pub unsafe extern "C" fn ccxr_write_cc_subtitle_as_srt(
        sub: *mut cc_subtitle,
        context: *mut encoder_ctx,
    ) -> c_int {
        if context.is_null() || sub.is_null() {
            return 0;
        }

        let ctx = &mut *context;
        let mut ret: c_int = 0;
        let osub = sub;
        let mut current = sub;
        let mut lsub = sub;

        while !current.is_null() {
            let s = &mut *current;
            if s.type_ == subtype_CC_TEXT {
                let out = crate::get_teletext_output(context, s.teletext_page);
                let counter = crate::get_teletext_srt_counter(context, s.teletext_page);

                let c_str = if !s.data.is_null() {
                    std::ffi::CStr::from_ptr(s.data as *const i8)
                        .to_str()
                        .unwrap_or("")
                } else {
                    ""
                };

                if !out.is_null() && !counter.is_null() {
                    let mut writer = SrtWriter::with_output(ctx, (*out).fh, &mut *counter);
                    let _ = writer.write_stringz(c_str, s.start_time, s.end_time);
                } else {
                    let mut writer = SrtWriter::from_context(ctx);
                    let _ = writer.write_stringz(c_str, s.start_time, s.end_time);
                }

                if !s.data.is_null() {
                    crate::ffi_alloc::c_free(s.data);
                    s.data = std::ptr::null_mut();
                }
                s.nb_data = 0;
                ret = 1;
            }
            lsub = current;
            current = s.next;
        }

        // Free the linked list (except the original node)
        while lsub != osub {
            let prev = (*lsub).prev;
            crate::ffi_alloc::c_free(lsub);
            lsub = prev;
        }

        ret
    }

    /// FFI: Write OCR bitmap subtitle as SRT (behind hardsubx_ocr feature).
    #[cfg(feature = "hardsubx_ocr")]
    extern "C" {
        fn paraof_ocrtext(sub: *mut cc_subtitle, context: *mut encoder_ctx) -> *mut i8;
    }

    #[cfg(feature = "hardsubx_ocr")]
    const SUB_EOD_MARKER: c_int = 1 << 0;

    #[cfg(feature = "hardsubx_ocr")]
    #[no_mangle]
    pub unsafe extern "C" fn ccxr_write_cc_bitmap_as_srt(
        sub: *mut cc_subtitle,
        context: *mut encoder_ctx,
    ) -> c_int {
        if context.is_null() || sub.is_null() {
            return 0;
        }
        let ctx = &mut *context;
        let s = &mut *sub;

        if s.nb_data == 0 {
            return 0;
        }

        // prev_start is initialized to -1 in encoder_init (ccx_encoders_common.c:791)
        if s.flags & SUB_EOD_MARKER != 0 {
            ctx.prev_start = s.start_time;
        }

        let str_ptr = paraof_ocrtext(sub, context);
        if !str_ptr.is_null() {
            if ctx.is_mkv == 1 {
                ctx.last_string = str_ptr;
            } else {
                if ctx.prev_start != -1 || (s.flags & SUB_EOD_MARKER) == 0 {
                    let c_str = std::ffi::CStr::from_ptr(str_ptr).to_str().unwrap_or("");
                    let mut writer = SrtWriter::from_context(ctx);

                    let (h1, m1, s1_t, ms1) = millis_to_time(s.start_time);
                    let (h2, m2, s2_t, ms2) = millis_to_time(s.end_time - 1);

                    *writer.srt_counter += 1;
                    let crlf = writer.crlf_str().to_owned();
                    let counter_line = format!("{}{}", *writer.srt_counter, crlf);
                    let encoded = writer.encode(counter_line.as_bytes());
                    let _ = writer.write(&encoded);

                    let timeline = format!(
                        "{:02}:{:02}:{:02},{:03} --> {:02}:{:02}:{:02},{:03}{}",
                        h1, m1, s1_t, ms1, h2, m2, s2_t, ms2, crlf
                    );
                    let encoded = writer.encode(timeline.as_bytes());
                    let _ = writer.write(&encoded);

                    let text_len = std::ffi::CStr::from_ptr(str_ptr).to_bytes().len();
                    let text_slice = std::slice::from_raw_parts(str_ptr as *const u8, text_len);
                    let _ = writer.write(text_slice);
                    let crlf_clone = writer.crlf.clone();
                    let _ = writer.write(&crlf_clone);
                }
                crate::ffi_alloc::c_free(str_ptr);
            }
        }

        // Free bitmap data
        // TODO: add cc_bitmap to bindgen allowlist in build.rs instead of manual repr
        #[repr(C)]
        struct CcBitmap {
            x: c_int,
            y: c_int,
            w: c_int,
            h: c_int,
            nb_colors: c_int,
            data0: *mut std::os::raw::c_uchar,
            data1: *mut std::os::raw::c_uchar,
            linesize0: c_int,
            linesize1: c_int,
        }
        let data_ptr = s.data as *mut CcBitmap;
        if !data_ptr.is_null() {
            for i in 0..s.nb_data as isize {
                let rect = &mut *data_ptr.offset(i);
                if !rect.data0.is_null() {
                    crate::ffi_alloc::c_free(rect.data0);
                    rect.data0 = std::ptr::null_mut();
                }
                if !rect.data1.is_null() {
                    crate::ffi_alloc::c_free(rect.data1);
                    rect.data1 = std::ptr::null_mut();
                }
            }
        }

        s.nb_data = 0;
        if !s.data.is_null() {
            crate::ffi_alloc::c_free(s.data);
            s.data = std::ptr::null_mut();
        }

        0
    }
}
#[cfg(not(test))]
pub use implementation::*;
