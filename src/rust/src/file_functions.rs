//allow unsafe
#![allow(unsafe_code)]
#![allow(unexpected_cfgs)]

use crate::ccx_options;
use crate::demuxer::demuxer::*;
use crate::demuxer::lib_ccx::*;
use lib_ccxr::activity::{update_net_activity_gui, NET_ACTIVITY_GUI};
use lib_ccxr::common::DataSource;
use lib_ccxr::util::log::ExitCause;
use lib_ccxr::util::log::{debug, DebugMessageFlag};
use libc::{c_void, memcpy, memmove};
use palette::encoding::pixel::RawPixel;
use std::ffi::{c_char, CStr};
use std::ptr;
use std::sync::atomic::Ordering;
use std::time::{SystemTime, UNIX_EPOCH};


pub static mut terminate_asap: bool = false;
pub const FILEBUFFERSIZE: usize = 1024 * 1024 * 16; // 16 Mbytes no less. Minimize number of real read calls()

pub fn position_sanity_check(ctx: &mut CcxDemuxer) {
    #[cfg(feature = "sanity_check")]
    {
        if ctx.infd != -1 {
            // Attempt to get the real position in the file
            unsafe {
                let realpos = unsafe { libc::lseek(ctx.infd, 0, libc::SEEK_CUR) };
                if realpos == -1 {
                    // If `lseek` fails (e.g., input is stdin), simply return
                    return;
                }

                // Calculate the expected position
                let expected_pos = ctx.past - ctx.filebuffer_pos as i64 + ctx.bytesinbuffer as i64;

                // Check for position desynchronization
                if realpos != expected_pos {
                    panic!(
                        "Position desync, THIS IS A BUG. Real pos = {}, past = {}.",
                        realpos, ctx.past
                    );
                }
            }
        }
    }
}
pub fn sleep_secs(secs: u64) {
    #[cfg(target_os = "windows")]
    {
        // Windows uses Sleep, which takes milliseconds
        unsafe { libc::Sleep(secs * 1000) };
    }
    #[cfg(not(target_os = "windows"))]
    {
        // Linux/macOS use sleep, which takes seconds
        unsafe { libc::sleep(secs as u32) };
    }
}


pub unsafe fn sleepandchecktimeout(start: u64) {
    if ccx_options.input_source == DataSource::Stdin as u32 {
        // For stdin, just sleep for 1 second and reset live_stream
        sleep_secs(1);
        if ccx_options.live_stream != 0 {
            ccx_options.live_stream = 0;
        }
        return;
    }

    if ccx_options.live_stream != 0 {
        if ccx_options.live_stream == -1 {
            // Sleep without timeout check
            sleep_secs(1);
            return;
        }
    }

    // Get the current time in seconds since the epoch
    let current_time = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .expect("System time went backwards")
        .as_secs();

    if ccx_options.live_stream != 0 {
        if current_time > start + ccx_options.live_stream as u64 {
            // Timeout elapsed, no more live streaming
            ccx_options.live_stream = 0;
        } else {
            // Otherwise, sleep for 1 second
            sleep_secs(1);
        }
    } else {
        sleep_secs(1);
    }
}

fn close_input_file(ctx: &mut LibCcxCtx) {
    unsafe {
        (*ctx.demux_ctx).close();
    }
}

pub unsafe fn switch_to_next_file(ctx: &mut LibCcxCtx, bytes_in_buffer: i64) -> i32 {
    let mut ret = 0;

    if ctx.current_file == -1 || !ccx_options.binary_concat != 0 {
        unsafe { (*ctx.demux_ctx).reset() };
    }

    match DataSource::from(ccx_options.input_source) {
        DataSource::Stdin | DataSource::Network | DataSource::Tcp => {
            ret = unsafe { (*ctx.demux_ctx).open(*ptr::null()) };
            return if ret < 0 { 0 } else if ret > 0 { ret } else { 1 };
        }
        _ => {}
    }

    if unsafe { (*ctx.demux_ctx).is_open() } {
        debug!(
            msg_type = DebugMessageFlag::DECODER_708;
            "[CEA-708] The 708 decoder was reset [{}] times.",
            unsafe { (*ctx.freport.data_from_708).reset_count }
        );

        if ccx_options.print_file_reports != 0 {
            print_file_report(ctx)
        }

        if ctx.inputsize > 0
            && ((unsafe { (*ctx.demux_ctx).past } + bytes_in_buffer) < ctx.inputsize)
        // && !is_decoder_processed_enough(ctx) //TODO
        {
            println!("\n\n\n\nATTENTION!!!!!!");
            println!(
                "In switch_to_next_file(): Processing of {} {} ended prematurely {} < {}, please send bug report.\n\n",
                unsafe { CStr::from_ptr(ctx.inputfile[ctx.current_file as usize].as_ptr() as *const c_char) }
                    .to_string_lossy(), ctx.current_file,
                unsafe { (*ctx.demux_ctx).past },
                ctx.inputsize
            );
        }
        close_input_file(ctx);

        if ccx_options.binary_concat != 0 {
            ctx.total_past += ctx.inputsize;
            unsafe { (*ctx.demux_ctx).past = 0 };
        }
    }

    loop {
        ctx.current_file += 1;
        if ctx.current_file >= ctx.num_input_files {
            break;
        }

        println!("\n\r-----------------------------------------------------------------");
        println!("\rOpening file: {}", ctx.inputfile[ctx.current_file as usize]);

        let ret = (*ctx.demux_ctx).open(
            CStr::from_ptr(ctx.inputfile[ctx.current_file as usize].as_ptr() as *const c_char).to_str().unwrap(),
        );

        if ret < 0 {
            println!("\rWarning: Unable to open input file [{}]", ctx.inputfile[ctx.current_file as usize]);
        } else {
            // ccx_options.activity_input_file_open() // TODO
            &ctx.inputfile[ctx.current_file as usize];
            if !(ccx_options.live_stream != 0) {
                ctx.inputsize = (*ctx.demux_ctx).get_filesize(ctx.demux_ctx);
                if !(ccx_options.binary_concat != 0) {
                    ctx.total_inputsize = ctx.inputsize;
                }
            }
            return 1; // Succeeded
        }
    }


    0
}


pub unsafe fn buffered_read_opt(ctx: &mut CcxDemuxer, mut buffer: &mut [u8], mut bytes: usize) -> usize {
    let mut copied = 0;
    #[allow(unused)]
    let origin_buffer_size = bytes;
    let mut seconds = SystemTime::now();

    position_sanity_check(ctx);

    if ccx_options.live_stream > 0 {
        seconds = SystemTime::now();
    }

    if ccx_options.buffer_input != 0 || ctx.filebuffer_pos < ctx.bytesinbuffer {
        let mut eof = ctx.infd == -1;
        while (!eof || ccx_options.live_stream > 0) && bytes > 0 {
            if terminate_asap {
                break;
            }
            if eof {
                sleepandchecktimeout(seconds.duration_since(UNIX_EPOCH).unwrap().as_secs());
            }

            let ready = ctx.bytesinbuffer - ctx.filebuffer_pos;

            if ready == 0 {
                if !(ccx_options.buffer_input != 0) {
                    let mut i: isize;
                    loop {
                        // Use raw pointers to avoid borrowing issues
                        let buf_ptr = buffer.as_mut_ptr();
                        let len = buffer.len();

                        if len > 0 {
                            i = libc::read(ctx.infd, buf_ptr as *mut c_void, bytes) as isize;
                            buffer = std::slice::from_raw_parts_mut(buf_ptr.add(i as usize), len - i as usize);
                            if i == -1 {
                                panic!("Error reading input file!");
                            }
                        } else {
                            let op = libc::lseek(ctx.infd, 0, libc::SEEK_CUR);
                            let np = libc::lseek(ctx.infd, bytes as libc::off_t, libc::SEEK_CUR);
                            i = (np - op) as isize;
                            buffer = std::slice::from_raw_parts_mut(buf_ptr.add(i as usize), len - i as usize);
                            if (op + bytes as i64) < 0 {
                                return 0;
                            }
                        }

                        if i == 0 && (ccx_options.live_stream != 0) {
                            if ccx_options.input_source == DataSource::Stdin as u32 {
                                ccx_options.live_stream = 0;
                                break;
                            } else {
                                sleepandchecktimeout(seconds.duration_since(UNIX_EPOCH).unwrap().as_secs());
                            }
                        } else {
                            copied += i as usize;
                            bytes -= i as usize;
                        }

                        if (i != 0 || ccx_options.live_stream != 0 ||
                            (ccx_options.binary_concat != 0 && (switch_to_next_file(&mut *(ctx.parent as *mut LibCcxCtx), copied as i64) as i64 != 0)))
                            && bytes > 0
                        {
                            continue;
                        } else {
                            break;
                        }
                    }

                    return copied;
                }

                let keep: usize = ctx.bytesinbuffer.min(8) as usize;
                ptr::copy(ctx.filebuffer.add(FILEBUFFERSIZE - keep), ctx.filebuffer, keep);
                let i = if ccx_options.input_source == DataSource::File as u32 || ccx_options.input_source == DataSource::Stdin as u32 {
                    libc::read(ctx.infd, ctx.filebuffer.add(keep) as *mut c_void, FILEBUFFERSIZE - keep)
                } else if ccx_options.input_source == DataSource::Network as u32 {
                    0 // Placeholder for net_tcp_read once the net module has been built
                } else {
                    0 // Placeholder for net_udp_read once the net module has been built
                };
                if terminate_asap {
                    break;
                }
                if i == -1 {
                    panic!("Error reading input stream!");
                }
                if i == 0 {
                    if ccx_options.live_stream > 0 || ctx.parent.is_null() || !(ccx_options.binary_concat != 0) {
                        eof = true;
                    }
                }
                ctx.filebuffer_pos = keep as u32;
                ctx.bytesinbuffer = (i as usize + keep) as u32;
            }
            let copy = ready.min(bytes as u32) as usize;
            if copy > 0 {
                // Use raw pointers to avoid borrowing issues
                let buf_ptr = buffer.as_mut_ptr();
                ptr::copy(ctx.filebuffer.add(ctx.filebuffer_pos as usize), buf_ptr, copy);
                ctx.filebuffer_pos += copy as u32;
                bytes -= copy;
                copied += copy;
                buffer = std::slice::from_raw_parts_mut(buf_ptr.add(copy), buffer.len() - copy);
            }
        }
    } else {
        if !buffer.is_empty() {
            let mut i: isize = -1;
            while bytes > 0 && ctx.infd != -1
                && ({
                i = libc::read(ctx.infd, buffer.as_mut_ptr() as *mut c_void, bytes) as i32 as isize;
                i != 0
            }
                || ccx_options.live_stream != 0
                || (ccx_options.binary_concat != 0 && switch_to_next_file(&mut *(ctx.parent as *mut LibCcxCtx), copied as i64) != 0))
            {
                if terminate_asap {
                    break;
                }
                if i == -1 {
                    panic!("Error reading input file!");
                } else if i == 0 {
                    sleepandchecktimeout(seconds.duration_since(UNIX_EPOCH).unwrap().as_secs());
                } else {
                    copied += i as usize;
                    bytes -= i as usize;
                    buffer = std::slice::from_raw_parts_mut(buffer.as_mut_ptr().add(i as usize), buffer.len() - i as usize);
                }
            }
            return copied;
        }

        while bytes > 0 && ctx.infd != -1 {
            if terminate_asap {
                break;
            }

            let op = libc::lseek(ctx.infd, 0, libc::SEEK_CUR);
            if (op + bytes as i64) < 0 {
                return 0;
            }

            let np = libc::lseek(ctx.infd, bytes as libc::off_t, libc::SEEK_CUR);
            if op == -1 && np == -1 {
                let mut c = [0u8; 1];
                for _ in 0..bytes {
                    if libc::read(ctx.infd, c.as_mut_ptr() as *mut c_void, 1) == -1 {
                        panic!("Error reading from file!");
                    }
                }
                copied += bytes;
            } else {
                copied += (np - op) as usize;
            }

            bytes -= copied;

            if copied == 0 {
                if ccx_options.live_stream != 0 {
                    sleepandchecktimeout(seconds.duration_since(UNIX_EPOCH).unwrap().as_secs());
                } else if ccx_options.binary_concat != 0 {
                    switch_to_next_file(&mut *(ctx.parent as *mut LibCcxCtx), copied as i64);
                } else {
                    break;
                }
            }
        }
    }
    copied
}


/// Translated version of the C `return_to_buffer` function with comments preserved.
/// This version takes a `CcxDemuxer` object instead of using an `impl`.

/// Translated C `return_to_buffer` function using `libc` for memcpy and memmove.
/// This takes a `CcxDemuxer` object and a byte buffer, returning data to the internal
/// file buffer.
pub fn return_to_buffer(ctx: &mut CcxDemuxer, buffer: &[u8], bytes: u32) {
    unsafe {
        // If the requested bytes match the current buffer position,
        // simply copy and reset the position.
        if bytes == ctx.filebuffer_pos {
            // memcpy(ctx->filebuffer, buffer, bytes);
            // Even if it might be a no-op, do it in case we intentionally messed with the buffer.
            memcpy(
                ctx.filebuffer as *mut c_void,
                buffer.as_ptr() as *const c_void,
                bytes as usize,
            );
            ctx.filebuffer_pos = 0;
            return;
        }

        // If there's a nonzero filebuffer_pos, discard old bytes to make space.
        if ctx.filebuffer_pos > 0 {
            // memmove(ctx->filebuffer, ctx->filebuffer + ctx->filebuffer_pos, ctx->bytesinbuffer - ctx->filebuffer_pos);
            let valid_len = ctx.bytesinbuffer - ctx.filebuffer_pos;
            memmove(
                ctx.filebuffer as *mut c_void,
                ctx.filebuffer.add(ctx.filebuffer_pos as usize) as *const c_void,
                valid_len as usize,
            );
            ctx.bytesinbuffer = 0;
            ctx.filebuffer_pos = 0;
        }

        // Check if we have enough room.
        if ctx.bytesinbuffer + bytes > FILEBUFFERSIZE as u32 {
            lib_ccxr::fatal!(
                cause = ExitCause::Bug;
                "Invalid return_to_buffer() - please submit a bug report."
            );
        }

        // memmove(ctx->filebuffer + bytes, ctx->filebuffer, ctx->bytesinbuffer);
        // Shift existing data to make room at the front.
        memmove(
            ctx.filebuffer.add(bytes as usize) as *mut c_void,
            ctx.filebuffer as *const c_void,
            ctx.bytesinbuffer as usize,
        );

        // memcpy(ctx->filebuffer, buffer, bytes);
        // Copy the incoming data to the front of the buffer.
        memcpy(
            ctx.filebuffer as *mut c_void,
            buffer.as_ptr() as *const c_void,
            bytes as usize,
        );
    }
    ctx.bytesinbuffer += bytes;
}
pub unsafe fn buffered_read(mut ctx: &mut CcxDemuxer
                            , buffer: Option<&mut [u8]>, bytes: usize) -> usize {
    let available = (ctx.bytesinbuffer - ctx.filebuffer_pos) as usize;

    if bytes <= available {
        if let Some(buf) = buffer {
            unsafe {
                memcpy(
                    buf.as_mut_ptr() as *mut c_void,
                    ctx.filebuffer.add(ctx.filebuffer_pos as usize) as *const c_void,
                    bytes,
                );
            }
        }
        ctx.filebuffer_pos += bytes as u32;
        bytes
    } else {
        let result = buffered_read_opt(ctx, buffer.unwrap(), bytes);

        unsafe {
            if ccx_options.gui_mode_reports != 0 && ccx_options.input_source == DataSource::Network as u32 {
                // net_activity_gui += 1;
                update_net_activity_gui(unsafe { NET_ACTIVITY_GUI.load(Ordering::SeqCst) + 1 });
                if NET_ACTIVITY_GUI.load(Ordering::SeqCst) % 1000 == 0 {
                    // ccx_options.activity_report_data_read(); // TODO
                }
            }
        }

        result
    }
}
pub unsafe fn buffered_read_byte(ctx: *mut CcxDemuxer, buffer: Option<&mut u8>) -> usize {
    if ctx.is_null() {
        return 0;
    }

    let ctx = &mut *ctx; // Dereference the raw pointer safely

    if ctx.bytesinbuffer > ctx.filebuffer_pos {
        if let Some(buf) = buffer {
            *buf = *ctx.filebuffer.add(ctx.filebuffer_pos as usize);
            ctx.filebuffer_pos += 1;
            return 1;
        }
    } else {
        let mut buf = [0u8; 1048576];
        if let Some(b) = buffer {
            buf[0] = *b;
        }
        return buffered_read_opt(ctx, &mut buf, 1);
    }
    0
}
pub unsafe fn buffered_get_be16(ctx: *mut CcxDemuxer) -> u16 {
    if ctx.is_null() {
        return 0;
    }

    let ctx = &mut *ctx; // Dereference the raw pointer safely

    let mut a: u8 = 0;
    let mut b: u8 = 0;

    buffered_read_byte(ctx, Some(&mut a));
    ctx.past += 1;

    buffered_read_byte(ctx, Some(&mut b));
    ctx.past += 1;

    ((a as u16) << 8) | (b as u16)
}
pub unsafe fn buffered_get_byte(ctx: *mut CcxDemuxer) -> u8 {
    if ctx.is_null() {
        return 0;
    }

    let ctx = &mut *ctx;
    let mut b: u8 = 0;

    if buffered_read_byte(ctx, Some(&mut b)) == 1 {
        ctx.past += 1;
        return b;
    }

    0
}
pub unsafe fn buffered_get_be32(ctx: *mut CcxDemuxer) -> u32 {
    if ctx.is_null() {
        return 0;
    }

    let high = (buffered_get_be16(ctx) as u32) << 16;
    let low = buffered_get_be16(ctx) as u32;

    high | low
}
pub unsafe fn buffered_get_le16(ctx: *mut CcxDemuxer) -> u16 {
    if ctx.is_null() {
        return 0;
    }

    let ctx = &mut *ctx;
    let mut a: u8 = 0;
    let mut b: u8 = 0;

    buffered_read_byte(ctx, Some(&mut a));
    ctx.past += 1;

    buffered_read_byte(ctx, Some(&mut b));
    ctx.past += 1;

    ((b as u16) << 8) | (a as u16)
}
pub unsafe fn buffered_get_le32(ctx: *mut CcxDemuxer) -> u32 {
    if ctx.is_null() {
        return 0;
    }

    let low = buffered_get_le16(ctx) as u32;
    let high = (buffered_get_le16(ctx) as u32) << 16;

    low | high
}
pub unsafe fn buffered_skip(ctx: *mut CcxDemuxer, bytes: u32) -> usize {
    if ctx.is_null() {
        return 0;
    }

    let ctx_ref = &mut *ctx;

    if bytes <= ctx_ref.bytesinbuffer - ctx_ref.filebuffer_pos {
        ctx_ref.filebuffer_pos += bytes;
        bytes as usize
    } else {
        // buffered_read_opt(ctx.as_mut().unwrap(), *ptr::null_mut(), bytes as usize)
        buffered_read_opt(ctx.as_mut().unwrap(), &mut [], bytes as usize)
    }
}