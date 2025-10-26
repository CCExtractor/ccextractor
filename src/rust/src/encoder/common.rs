#![allow(dead_code)]
use crate::bindings::{
    ccx_s_write, net_send_header,
    write_spumux_footer,
};
use crate::libccxr_exports::encoder_ctx::ccxr_encoder_ctx;
use crate::ccx_options;
use crate::encoder::FromCType;
use lib_ccxr::common::{OutputFormat, BROADCAST_HEADER, LITTLE_ENDIAN_BOM, UTF8_BOM};
use lib_ccxr::util::encoding::Encoding;
use lib_ccxr::util::log::DebugMessageFlag;
use lib_ccxr::{debug, info};
use std::fs::File;
use std::io::Write;
#[cfg(unix)]
use std::os::fd::FromRawFd;
use std::os::raw::{c_int, c_uint, c_void};
#[cfg(windows)]
use std::os::windows::io::FromRawHandle;

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

/// Optimal encoding function that encodes text with the specified encoding.
/// This is the unified function replacing encode_line, encode_with_encoding, and encode_line_rust.
/// 
/// # Arguments
/// * `encoding` - The encoding type to use (extracted from context if needed)
/// * `buffer` - Output buffer to write encoded text
/// * `text` - Input text to encode (null-terminated or full slice)
/// 
/// # Returns
/// Number of bytes written to the buffer
pub fn encode_line(encoding: Encoding, buffer: &mut [u8], text: &[u8]) -> usize {
    if buffer.is_empty() {
        return 0;
    }

    let mut bytes: usize = 0;
    let mut buffer_pos = 0;
    let text_len = text.iter().position(|&b| b == 0).unwrap_or(text.len());
    
    for &current_byte in &text[..text_len] {
        match encoding {
            Encoding::UTF8 | Encoding::Latin1 | Encoding::Line21 => {
                if buffer_pos + 1 >= buffer.len() {
                    break;
                }
                buffer[buffer_pos] = current_byte;
                buffer_pos += 1;
                bytes += 1;
            }
            Encoding::UCS2 => {
                if buffer_pos + 2 >= buffer.len() {
                    break;
                }
                // Convert to UCS-2 (2 bytes, little-endian)
                let ucs2_char = current_byte as u16;
                let bytes_le = ucs2_char.to_le_bytes();
                buffer[buffer_pos] = bytes_le[0];
                buffer[buffer_pos + 1] = bytes_le[1];
                buffer_pos += 2;
                bytes += 2;
            }
        }
    }

    // Add null terminator if there's space
    if buffer_pos < buffer.len() {
        buffer[buffer_pos] = 0;
    }

    bytes
}

pub fn write_subtitle_file_footer_rust(ctx: &mut ccxr_encoder_ctx, out: &mut ccx_s_write) -> c_int {
    let mut ret: c_int = 0;
    let mut str_buffer = [0u8; 1024];
    let write_format = unsafe { OutputFormat::from_ctype(ctx.write_format).unwrap_or(OutputFormat::Raw) };

    match write_format {
        OutputFormat::Sami | OutputFormat::SmpteTt | OutputFormat::SimpleXml => {
            let footer: &[u8] = match write_format {
                OutputFormat::Sami => b"</BODY></SAMI>\n\0",
                OutputFormat::SmpteTt => b"    </div>\n  </body>\n</tt>\n\0",
                OutputFormat::SimpleXml => b"</captions>\n\0",
                _ => unreachable!(),
            };

            // Bounds check for str_buffer
            if footer.len() > str_buffer.len() {
                return -1;
            }

            str_buffer[..footer.len()].copy_from_slice(footer);

            if ctx.encoding != Encoding::UTF8 {
                debug!(msg_type = DebugMessageFlag::DECODER_608; "\r{}\n",
                    std::str::from_utf8(&str_buffer[..footer.len()-1]).unwrap_or(""));
            }

            // Use Rust buffer directly
            let text_slice = &str_buffer[..footer.len()];
            let encoding = ctx.encoding; // Extract encoding first to avoid borrowing conflicts
            let used = if let Some(ref mut buffer) = ctx.buffer {
                encode_line(encoding, buffer, text_slice)
            } else {
                return -1;
            };

            // Bounds check for buffer access
            if used > ctx.buffer.as_ref().unwrap().len() {
                return -1;
            }

            ret = write_raw(out.fh, ctx.buffer.as_ref().unwrap().as_ptr() as *const c_void, used) as c_int;

            if ret != used as c_int {
                info!("WARNING: loss of data\n");
            }
        }

        OutputFormat::SpuPng => unsafe {
            write_spumux_footer(out);
        },

        OutputFormat::Scc | OutputFormat::Ccd => {
            // Use Rust encoded_crlf directly
            if let Some(ref crlf) = ctx.encoded_crlf {
                ret = write_raw(
                    out.fh,
                    crlf.as_ptr() as *const c_void,
                    crlf.len(),
                ) as c_int;
            }
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

fn request_buffer_capacity(ctx: &mut ccxr_encoder_ctx, length: u32) -> bool {
    if length > ctx.capacity {
        ctx.capacity = length * 2;

        // Resize the Vec buffer if it exists, or create a new one
        match &mut ctx.buffer {
            Some(buffer) => {
                // Reserve additional capacity
                buffer.reserve((ctx.capacity as usize).saturating_sub(buffer.len()));
            }
            None => {
                // Create new buffer with requested capacity
                ctx.buffer = Some(Vec::with_capacity(ctx.capacity as usize));
            }
        }
    }

    true
}
pub fn write_bom_rust(ctx: &mut ccxr_encoder_ctx, out: &mut ccx_s_write) -> c_int {
    let mut ret: c_int = 0;

    if ctx.no_bom == 0 {
        let enc = ctx.encoding;
        match enc {
            Encoding::UTF8 => {
                ret =
                    write_raw(out.fh, UTF8_BOM.as_ptr() as *const c_void, UTF8_BOM.len()) as c_int;
                if ret < UTF8_BOM.len() as c_int {
                    info!("WARNING: Unable to write UTF BOM\n");
                    return -1;
                }
            }
            Encoding::UCS2 => {
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

pub fn write_subtitle_file_header_rust(ctx: &mut ccxr_encoder_ctx, out: &mut ccx_s_write) -> c_int {
    let mut used: c_uint;
    let mut header_size: usize = 0;
    let write_format = unsafe { OutputFormat::from_ctype(ctx.write_format).unwrap_or(OutputFormat::Raw) };

    match write_format {
        OutputFormat::Ccd => {
            if write_raw(
                out.fh,
                CCD_HEADER.as_ptr() as *const c_void,
                CCD_HEADER.len() - 1,
            ) == -1
                || (if let Some(ref crlf) = ctx.encoded_crlf {
                    write_raw(out.fh, crlf.as_ptr() as *const c_void, crlf.len()) == -1
                } else {
                    true
                })
            {
                info!("Unable to write CCD header to file\n");
                return -1;
            }
        }

        OutputFormat::Scc => {
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

        OutputFormat::Srt
        | OutputFormat::G608
        | OutputFormat::SpuPng
        | OutputFormat::Transcript => {
            if write_bom_rust(ctx, out) < 0 {
                return -1;
            }
            if write_format == OutputFormat::SpuPng {
                // Note: write_spumux_header requires C encoder_ctx, so we skip it for now
                // This would need to be implemented as a Rust version
            }
        }

        OutputFormat::Ssa
        | OutputFormat::Sami
        | OutputFormat::SmpteTt
        | OutputFormat::SimpleXml => {
            if write_bom_rust(ctx, out) < 0 {
                return -1;
            }

            let header_data = match write_format {
                OutputFormat::Ssa => SSA_HEADER,
                OutputFormat::Sami => SAMI_HEADER,
                OutputFormat::SmpteTt => SMPTETT_HEADER,
                OutputFormat::SimpleXml => SIMPLE_XML_HEADER,
                _ => unreachable!(),
            };

            // Ensure buffer has enough capacity
            let required_capacity = header_data.len() * 3;
            if ctx.buffer.as_ref().map_or(true, |b| b.len() < required_capacity) {
                ctx.buffer = Some(vec![0u8; required_capacity]);
            }

            // Use Rust buffer directly
            let text_slice = header_data;
            let encoding = ctx.encoding; // Extract encoding first to avoid borrowing conflicts
            used = if let Some(ref mut buffer) = ctx.buffer {
                encode_line(encoding, buffer, text_slice.as_ref()) as c_uint
            } else {
                return -1;
            };

            if used > ctx.buffer.as_ref().unwrap().len() as c_uint {
                return -1;
            }

            if write_raw(out.fh, ctx.buffer.as_ref().unwrap().as_ptr() as *const c_void, used as usize) < used as isize {
                info!("WARNING: Unable to write complete Buffer\n");
                return -1;
            }
        }

        OutputFormat::WebVtt => {
            if write_bom_rust(ctx, out) < 0 {
                return -1;
            }

            // Calculate total header size
            for header_line in WEBVTT_HEADER {
                header_size += header_line.len();
            }

            // Ensure buffer has enough capacity
            let required_capacity = header_size * 3;
            if ctx.buffer.as_ref().map_or(true, |b| b.len() < required_capacity) {
                ctx.buffer = Some(vec![0u8; required_capacity]);
            }

            for header_line in WEBVTT_HEADER {
                let line_to_write = unsafe {
                    if ccx_options.enc_cfg.line_terminator_lf == 1 && *header_line == "\r\n" {
                        // If -lf parameter passed, write LF instead of CRLF
                        "\n"
                    } else {
                        header_line
                    }
                };

                // Use Rust buffer directly
                let text_slice = line_to_write.as_bytes();
                let encoding = ctx.encoding; // Extract encoding first to avoid borrowing conflicts
                used = if let Some(ref mut buffer) = ctx.buffer {
                    encode_line(encoding, buffer, text_slice) as c_uint
                } else {
                    return -1;
                };

                if used > ctx.buffer.as_ref().unwrap().len() as c_uint {
                    return -1;
                }

                if write_raw(out.fh, ctx.buffer.as_ref().unwrap().as_ptr() as *const c_void, used as usize) < used as isize {
                    info!("WARNING: Unable to write complete Buffer\n");
                    return -1;
                }
            }
        }

        OutputFormat::Rcwt => {
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

        OutputFormat::Raw => {
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

        OutputFormat::Mcc => {
            ctx.header_printed_flag = 0;
        }

        _ => {
            // Default case - do nothing
        }
    }

    0
}
