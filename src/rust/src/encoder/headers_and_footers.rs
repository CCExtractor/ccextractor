#![allow(dead_code)]
use crate::bindings::{
    ccx_encoding_type_CCX_ENC_LATIN_1, ccx_encoding_type_CCX_ENC_UNICODE,
    ccx_encoding_type_CCX_ENC_UTF_8, ccx_output_format, ccx_s_write, encoder_ctx, net_send_header,
    write_spumux_footer, write_spumux_header,
};
use crate::ccx_options;
use lib_ccxr::common::{BROADCAST_HEADER, LITTLE_ENDIAN_BOM, UTF8_BOM};
use lib_ccxr::util::log::DebugMessageFlag;
use lib_ccxr::{debug, info};
use std::alloc::{alloc, dealloc, Layout};
use std::fs::File;
use std::io::Write;
#[cfg(unix)]
use std::os::fd::FromRawFd;
use std::os::raw::{c_int, c_uchar, c_uint, c_void};
#[cfg(windows)]
use std::os::windows::io::FromRawHandle;
use std::ptr;
const CCD_HEADER: &[u8] = b"SCC_disassembly V1.2";
const SCC_HEADER: &[u8] = b"Scenarist_SCC V1.0";

const SSA_HEADER: &str = "[Script Info]\n\
Title: Default file\n\
ScriptType: v4.00+\n\
\n\
[V4+ Styles]\n\
Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\n\
Style: Default,Arial,20,&H00FFFFFF,&H000000FF,&H00000000,&H00000000,0,0,0,0,100,100,0,0,1,1,1,2,10,10,10,0\n\
\n\
[Events]\n\
Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n\
\n";
const SAMI_HEADER: &str = "<SAMI>\n\
<HEAD>\n\
<STYLE TYPE=\"text/css\">\n\
<!--\n\
P {margin-left: 16pt; margin-right: 16pt; margin-bottom: 16pt; margin-top: 16pt;\n\
text-align: center; font-size: 18pt; font-family: arial; font-weight: bold; color: #f0f0f0;}\n\
.UNKNOWNCC {Name:Unknown; lang:en-US; SAMIType:CC;}\n\
-->\n\
</STYLE>\n\
</HEAD>\n\n\
<BODY>\n";
const SMPTETT_HEADER: &str = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n  <tt xmlns=\"http://www.w3.org/ns/ttml\" xmlns:ttp=\"http://www.w3.org/ns/ttml#parameter\" ttp:dropMode=\"dropNTSC\" ttp:frameRate=\"30\" ttp:frameRateMultiplier=\"1000 1001\" ttp:timeBase=\"smpte\" xmlns:m608=\"http://www.smpte-ra.org/schemas/2052-1/2010/smpte-tt#cea608\" xmlns:smpte=\"http://www.smpte-ra.org/schemas/2052-1/2010/smpte-tt\" xmlns:ttm=\"http://www.w3.org/ns/ttml#metadata\" xmlns:tts=\"http://www.w3.org/ns/ttml#styling\">\n  <head>\n    <styling>\n      <style tts:color=\"white\" tts:fontFamily=\"monospace\" tts:fontWeight=\"normal\" tts:textAlign=\"left\" xml:id=\"basic\"/>\n    </styling>\n    <layout>\n      <region tts:backgroundColor=\"transparent\" xml:id=\"pop1\"/>\n      <region tts:backgroundColor=\"transparent\" xml:id=\"paint\"/>\n      <region tts:backgroundColor=\"transparent\" xml:id=\"rollup2\"/>\n      <region tts:backgroundColor=\"transparent\" xml:id=\"rollup3\"/>\n      <region tts:backgroundColor=\"transparent\" xml:id=\"rollup4\"/>\n    </layout>\n    <metadata/>\n    <smpte:information m608:captionService=\"F1C1CC\" m608:channel=\"cc1\"/>\n  </head>\n  <body>\n    <div>\n";
const SIMPLE_XML_HEADER: &str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<captions>\n";

const WEBVTT_HEADER: &[&str] = &["WEBVTT\n", "\n", "\n"];

const RCWT_HEADER: &[u8] = &[0xCC, 0xCC, 0xED, 0xCC, 0x00, 0x50, 0, 1, 0, 0, 0]; // "RCWT" + version

pub fn encode_line(ctx: &mut encoder_ctx, buffer: &mut [c_uchar], text: &[u8]) -> c_uint {
    if buffer.is_empty() {
        return 0;
    }

    let mut bytes: c_uint = 0;
    let mut buffer_pos = 0;
    let mut text_pos = 0;
    let text_len = text.iter().position(|&b| b == 0).unwrap_or(text.len());
    while text_pos < text_len {
        let current_byte = text[text_pos];

        match ctx.encoding {
            ccx_encoding_type_CCX_ENC_UTF_8 | ccx_encoding_type_CCX_ENC_LATIN_1 => {
                if buffer_pos + 1 >= buffer.len() {
                    break;
                }

                buffer[buffer_pos] = current_byte;
                bytes += 1;
                buffer_pos += 1;
            }

            ccx_encoding_type_CCX_ENC_UNICODE => {
                if buffer_pos + 2 >= buffer.len() {
                    break;
                }

                buffer[buffer_pos] = current_byte;
                buffer[buffer_pos + 1] = 0;
                bytes += 2;
                buffer_pos += 2;
            }
            _ => {}
        }
        text_pos += 1;
    }

    // Add null terminator if there's space
    if buffer_pos < buffer.len() {
        buffer[buffer_pos] = 0;
    }

    bytes
}
pub fn write_subtitle_file_footer(ctx: &mut encoder_ctx, out: &mut ccx_s_write) -> c_int {
    let mut ret: c_int = 0;
    let mut str_buffer = [0u8; 1024];

    match ctx.write_format {
        ccx_output_format::CCX_OF_SAMI => {
            let footer = b"</BODY></SAMI>\n\0";

            // Bounds check for str_buffer
            if footer.len() > str_buffer.len() {
                return -1;
            }

            str_buffer[..footer.len()].copy_from_slice(footer);

            if ctx.encoding != ccx_encoding_type_CCX_ENC_UNICODE {
                debug!(msg_type = DebugMessageFlag::DECODER_608; "\r{}\n",
                    std::str::from_utf8(&str_buffer[..footer.len()-1]).unwrap_or(""));
            }

            // Create safe slice from buffer pointer and capacity
            let buffer_slice =
                unsafe { std::slice::from_raw_parts_mut(ctx.buffer, ctx.capacity as usize) };
            let text_slice = &str_buffer[..footer.len()];
            let used = encode_line(ctx, buffer_slice, text_slice);

            // Bounds check for buffer access
            if used > ctx.capacity {
                return -1;
            }

            ret = write_raw(out.fh, ctx.buffer as *const c_void, used as usize) as c_int;

            if ret != used as c_int {
                info!("WARNING: loss of data\n");
            }
        }

        ccx_output_format::CCX_OF_SMPTETT => {
            let footer = b"    </div>\n  </body>\n</tt>\n\0";

            // Bounds check for str_buffer
            if footer.len() > str_buffer.len() {
                return -1;
            }

            str_buffer[..footer.len()].copy_from_slice(footer);

            if ctx.encoding != ccx_encoding_type_CCX_ENC_UNICODE {
                debug!(msg_type = DebugMessageFlag::DECODER_608; "\r{}\n",
                    std::str::from_utf8(&str_buffer[..footer.len()-1]).unwrap_or(""));
            }

            // Create safe slice from buffer pointer and capacity
            let buffer_slice =
                unsafe { std::slice::from_raw_parts_mut(ctx.buffer, ctx.capacity as usize) };
            let text_slice = &str_buffer[..footer.len()];
            let used = encode_line(ctx, buffer_slice, text_slice);

            // Bounds check for buffer access
            if used > ctx.capacity {
                return -1;
            }

            ret = write_raw(out.fh, ctx.buffer as *const c_void, used as usize) as c_int;

            if ret != used as c_int {
                info!("WARNING: loss of data\n");
            }
        }

        ccx_output_format::CCX_OF_SPUPNG => unsafe {
            write_spumux_footer(out);
        },

        ccx_output_format::CCX_OF_SIMPLE_XML => {
            let footer = b"</captions>\n\0";

            // Bounds check for str_buffer
            if footer.len() > str_buffer.len() {
                return -1;
            }

            str_buffer[..footer.len()].copy_from_slice(footer);

            if ctx.encoding != ccx_encoding_type_CCX_ENC_UNICODE {
                debug!(msg_type = DebugMessageFlag::DECODER_608; "\r{}\n",
                    std::str::from_utf8(&str_buffer[..footer.len()-1]).unwrap_or(""));
            }

            // Create safe slice from buffer pointer and capacity
            let buffer_slice =
                unsafe { std::slice::from_raw_parts_mut(ctx.buffer, ctx.capacity as usize) };
            let text_slice = &str_buffer[..footer.len()];
            let used = encode_line(ctx, buffer_slice, text_slice);

            // Bounds check for buffer access
            if used > ctx.capacity {
                return -1;
            }

            ret = write_raw(out.fh, ctx.buffer as *const c_void, used as usize) as c_int;

            if ret != used as c_int {
                info!("WARNING: loss of data\n");
            }
        }

        ccx_output_format::CCX_OF_SCC | ccx_output_format::CCX_OF_CCD => {
            // Bounds check for encoded_crlf access
            if ctx.encoded_crlf_length as usize > ctx.encoded_crlf.len() {
                return -1;
            }

            ret = write_raw(
                out.fh,
                ctx.encoded_crlf.as_ptr() as *const c_void,
                ctx.encoded_crlf_length as usize,
            ) as c_int;
        }

        _ => {
            // Nothing to do, no footer on this format
        }
    }

    ret
}

pub fn write_raw(fd: c_int, buf: *const c_void, count: usize) -> isize {
    if buf.is_null() || count == 0 {
        return 0;
    }
    #[cfg(unix)]
    let mut file = unsafe { File::from_raw_fd(fd) };
    #[cfg(windows)]
    let mut file = unsafe { File::from_raw_handle(fd as _) };
    let data = unsafe { std::slice::from_raw_parts(buf as *const u8, count) };
    let result = match file.write(data) {
        Ok(bytes_written) => bytes_written as isize,
        Err(_) => -1,
    };
    std::mem::forget(file);
    result
}

fn request_buffer_capacity(ctx: &mut encoder_ctx, length: c_uint) -> bool {
    if length > ctx.capacity {
        let old_capacity = ctx.capacity;
        ctx.capacity = length * 2;

        // Allocate new buffer
        let new_layout = Layout::from_size_align(ctx.capacity as usize, align_of::<u8>()).unwrap();

        let new_buffer = unsafe { alloc(new_layout) };

        if new_buffer.is_null() {
            // In C this would call: fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory for reallocating buffer, bailing out\n");
            return false;
        }

        // Copy old data if buffer existed
        if !ctx.buffer.is_null() && old_capacity > 0 {
            unsafe {
                ptr::copy_nonoverlapping(ctx.buffer, new_buffer, old_capacity as usize);
            }

            // Deallocate old buffer
            let old_layout =
                Layout::from_size_align(old_capacity as usize, std::mem::align_of::<u8>()).unwrap();

            unsafe {
                dealloc(ctx.buffer, old_layout);
            }
        }

        ctx.buffer = new_buffer;
    }

    true
}
pub fn write_bom(ctx: &mut encoder_ctx, out: &mut ccx_s_write) -> c_int {
    let mut ret: c_int = 0;

    if ctx.no_bom == 0 {
        match ctx.encoding {
            ccx_encoding_type_CCX_ENC_UTF_8 => {
                ret =
                    write_raw(out.fh, UTF8_BOM.as_ptr() as *const c_void, UTF8_BOM.len()) as c_int;
                if ret < UTF8_BOM.len() as c_int {
                    info!("WARNING: Unable to write UTF BOM\n");
                    return -1;
                }
            }
            ccx_encoding_type_CCX_ENC_UNICODE => {
                ret = write_raw(
                    out.fh,
                    LITTLE_ENDIAN_BOM.as_ptr() as *const c_void,
                    LITTLE_ENDIAN_BOM.len(),
                ) as c_int;
                if ret < LITTLE_ENDIAN_BOM.len() as c_int {
                    info!("WARNING: Unable to write LITTLE_ENDIAN_BOM\n");
                    return -1;
                }
            }
            _ => {}
        }
    }

    ret
}

pub fn write_subtitle_file_header(ctx: &mut encoder_ctx, out: &mut ccx_s_write) -> c_int {
    let mut used: c_uint;
    let mut header_size: usize = 0;

    match ctx.write_format {
        ccx_output_format::CCX_OF_CCD => {
            if write_raw(
                out.fh,
                CCD_HEADER.as_ptr() as *const c_void,
                CCD_HEADER.len() - 1,
            ) == -1
                || write_raw(
                    out.fh,
                    ctx.encoded_crlf.as_ptr() as *const c_void,
                    ctx.encoded_crlf_length as usize,
                ) == -1
            {
                info!("Unable to write CCD header to file\n");
                return -1;
            }
        }

        ccx_output_format::CCX_OF_SCC => {
            if write_raw(
                out.fh,
                SCC_HEADER.as_ptr() as *const c_void,
                SCC_HEADER.len() - 1,
            ) == -1
            {
                info!("Unable to write SCC header to file\n");
                return -1;
            }
        }

        ccx_output_format::CCX_OF_SRT | ccx_output_format::CCX_OF_G608 => {
            if write_bom(ctx, out) < 0 {
                return -1;
            }
        }

        ccx_output_format::CCX_OF_SSA => {
            if write_bom(ctx, out) < 0 {
                return -1;
            }

            if !request_buffer_capacity(ctx, (SSA_HEADER.len() * 3) as c_uint) {
                return -1;
            }

            // Create safe slice from buffer pointer and capacity
            let buffer_slice =
                unsafe { std::slice::from_raw_parts_mut(ctx.buffer, ctx.capacity as usize) };
            let text_slice = SSA_HEADER;
            used = encode_line(ctx, buffer_slice, text_slice.as_ref());

            if used > ctx.capacity {
                return -1;
            }

            if write_raw(out.fh, ctx.buffer as *const c_void, used as usize) < used as isize {
                info!("WARNING: Unable to write complete Buffer\n");
                return -1;
            }
        }

        ccx_output_format::CCX_OF_WEBVTT => {
            if write_bom(ctx, out) < 0 {
                return -1;
            }

            // Calculate total header size
            for header_line in WEBVTT_HEADER {
                header_size += header_line.len();
            }

            if !request_buffer_capacity(ctx, (header_size * 3) as c_uint) {
                return -1;
            }

            for header_line in WEBVTT_HEADER {
                let line_to_write = unsafe {
                    if ccx_options.enc_cfg.line_terminator_lf == 1 && *header_line == "\r\n" {
                        "\n"
                    } else {
                        header_line
                    }
                };

                // Create safe slice from buffer pointer and capacity
                let buffer_slice =
                    unsafe { std::slice::from_raw_parts_mut(ctx.buffer, ctx.capacity as usize) };
                let text_slice = line_to_write.as_bytes();
                used = encode_line(ctx, buffer_slice, text_slice);

                if used > ctx.capacity {
                    return -1;
                }

                if write_raw(out.fh, ctx.buffer as *const c_void, used as usize) < used as isize {
                    info!("WARNING: Unable to write complete Buffer\n");
                    return -1;
                }
            }
        }

        ccx_output_format::CCX_OF_SAMI => {
            if write_bom(ctx, out) < 0 {
                return -1;
            }

            if !request_buffer_capacity(ctx, (SAMI_HEADER.len() * 3) as c_uint) {
                return -1;
            }

            // Create safe slice from buffer pointer and capacity
            let buffer_slice =
                unsafe { std::slice::from_raw_parts_mut(ctx.buffer, ctx.capacity as usize) };
            let text_slice = SAMI_HEADER;
            used = encode_line(ctx, buffer_slice, text_slice.as_ref());

            if used > ctx.capacity {
                return -1;
            }

            if write_raw(out.fh, ctx.buffer as *const c_void, used as usize) < used as isize {
                info!("WARNING: Unable to write complete Buffer\n");
                return -1;
            }
        }

        ccx_output_format::CCX_OF_SMPTETT => {
            if write_bom(ctx, out) < 0 {
                return -1;
            }

            if !request_buffer_capacity(ctx, (SMPTETT_HEADER.len() * 3) as c_uint) {
                return -1;
            }

            // Create safe slice from buffer pointer and capacity
            let buffer_slice =
                unsafe { std::slice::from_raw_parts_mut(ctx.buffer, ctx.capacity as usize) };
            let text_slice = SMPTETT_HEADER;
            used = encode_line(ctx, buffer_slice, text_slice.as_ref());

            if used > ctx.capacity {
                return -1;
            }

            if write_raw(out.fh, ctx.buffer as *const c_void, used as usize) < used as isize {
                info!("WARNING: Unable to write complete Buffer\n");
                return -1;
            }
        }

        ccx_output_format::CCX_OF_RCWT => {
            let mut rcwt_header = RCWT_HEADER.to_vec();
            rcwt_header[7] = ctx.in_fileformat as u8; // sets file format version

            if ctx.send_to_srv != 0 {
                unsafe {
                    net_send_header(rcwt_header.as_ptr(), rcwt_header.len());
                }
            } else if write_raw(
                out.fh,
                rcwt_header.as_ptr() as *const c_void,
                rcwt_header.len(),
            ) < 0
            {
                info!("Unable to write rcwt header\n");
                return -1;
            }
        }

        ccx_output_format::CCX_OF_RAW => {
            if write_raw(
                out.fh,
                BROADCAST_HEADER.as_ptr() as *const c_void,
                BROADCAST_HEADER.len(),
            ) < BROADCAST_HEADER.len() as isize
            {
                info!("Unable to write Raw header\n");
                return -1;
            }
        }

        ccx_output_format::CCX_OF_SPUPNG => {
            if write_bom(ctx, out) < 0 {
                return -1;
            }
            unsafe {
                write_spumux_header(ctx, out);
            }
        }

        ccx_output_format::CCX_OF_TRANSCRIPT => {
            if write_bom(ctx, out) < 0 {
                return -1;
            }
        }

        ccx_output_format::CCX_OF_SIMPLE_XML => {
            if write_bom(ctx, out) < 0 {
                return -1;
            }

            if !request_buffer_capacity(ctx, (SIMPLE_XML_HEADER.len() * 3) as c_uint) {
                return -1;
            }

            // Create safe slice from buffer pointer and capacity
            let buffer_slice =
                unsafe { std::slice::from_raw_parts_mut(ctx.buffer, ctx.capacity as usize) };
            let text_slice = SIMPLE_XML_HEADER;
            used = encode_line(ctx, buffer_slice, text_slice.as_ref());

            if used > ctx.capacity {
                return -1;
            }

            if write_raw(out.fh, ctx.buffer as *const c_void, used as usize) < used as isize {
                info!("WARNING: Unable to write complete Buffer\n");
                return -1;
            }
        }

        ccx_output_format::CCX_OF_MCC => {
            ctx.header_printed_flag = 0;
        }

        _ => {
            // Default case - do nothing
        }
    }

    0
}
