#[cfg(windows)]
use crate::bindings::_get_osfhandle;
use crate::bindings::{lib_ccx_ctx, print_file_report};
use crate::demuxer::common_types::*;
use crate::libccxr_exports::demuxer::copy_demuxer_from_c_to_rust;
use cfg_if::cfg_if;
use lib_ccxr::activity::ActivityExt;
use lib_ccxr::common::{DataSource, Options};
use lib_ccxr::fatal;
use lib_ccxr::time::Timestamp;
use lib_ccxr::util::log::ExitCause;
use lib_ccxr::util::log::{debug, DebugMessageFlag};
use std::ffi::CStr;
use std::fs::File;
use std::io::{Read, Seek, SeekFrom};
#[cfg(unix)]
use std::os::fd::FromRawFd;
#[cfg(windows)]
use std::os::windows::io::FromRawHandle;
use std::ptr::{copy, copy_nonoverlapping};
use std::time::{SystemTime, UNIX_EPOCH};
use std::{mem, ptr, slice};

use crate::hlist::{is_decoder_processed_enough, list_empty};
#[cfg(feature = "sanity_check")]
use std::os::fd::IntoRawFd;
use std::os::raw::{c_char, c_void};

cfg_if! {
    if #[cfg(test)] {
        use crate::file_functions::file::tests::{terminate_asap, net_activity_gui, net_udp_read, net_tcp_read};
    }
    else {
        use crate::{terminate_asap, net_activity_gui, net_udp_read, net_tcp_read};
    }
}

pub const FILEBUFFERSIZE: usize = 1024 * 1024 * 16; // 16 Mbytes no less. Minimize the number of real read calls()

pub fn init_file_buffer(ctx: &mut CcxDemuxer) -> i32 {
    ctx.filebuffer_start = 0;
    ctx.filebuffer_pos = 0;
    if ctx.filebuffer.is_null() {
        let mut buf = vec![0u8; FILEBUFFERSIZE].into_boxed_slice();
        ctx.filebuffer = buf.as_mut_ptr();
        mem::forget(buf);
        ctx.bytesinbuffer = 0;
    }
    if ctx.filebuffer.is_null() {
        return -1;
    }
    0
}

#[cfg(windows)]
pub fn open_windows(infd: i32) -> File {
    // Convert raw fd to a File without taking ownership
    let handle = unsafe { _get_osfhandle(infd) };
    if handle == -1 {
        fatal!(cause = ExitCause::Bug; "Invalid file descriptor for Windows handle.");
    }
    unsafe { File::from_raw_handle(handle as *mut _) }
}

/// This function checks that the current file position matches the expected value.
#[allow(unused_variables)]
pub fn position_sanity_check(ctx: &mut CcxDemuxer) {
    #[cfg(feature = "sanity_check")]
    {
        if ctx.infd != -1 {
            let fd = ctx.infd;
            // Convert raw fd to a File without taking ownership
            #[cfg(unix)]
            let mut file = unsafe { File::from_raw_fd(fd) };
            #[cfg(windows)]
            let mut file = open_windows(fd);
            let realpos_result = file.seek(SeekFrom::Current(0));
            let realpos = match realpos_result {
                Ok(pos) => pos as i64, // Convert to i64 to match C's LLONG
                Err(_) => {
                    // Return the fd to avoid closing it
                    ctx.drop_fd(file);
                    return;
                }
            };
            // Return the fd to avoid closing it
            ctx.drop_fd(file);

            let expected_pos = ctx.past - ctx.filebuffer_pos as i64 + ctx.bytesinbuffer as i64;

            if realpos != expected_pos {
                fatal!(
                    cause = ExitCause::Bug;
                    "Position desync, THIS IS A BUG. Real pos = {}, past = {}.",
                    realpos,
                    ctx.past
                );
            }
        }
    }
}
pub fn sleep_secs(secs: u64) {
    #[cfg(target_os = "windows")]
    {
        std::thread::sleep(std::time::Duration::from_millis(secs * 1000));
    }
    #[cfg(not(target_os = "windows"))]
    {
        std::thread::sleep(std::time::Duration::from_secs(secs));
    }
}
pub fn sleepandchecktimeout(start: u64, ccx_options: &mut Options) {
    if ccx_options.input_source == DataSource::Stdin {
        // For stdin, just sleep for 1 second and reset live_stream
        sleep_secs(1);
        if ccx_options.live_stream.is_some() && ccx_options.live_stream.unwrap().seconds() != 0 {
            ccx_options.live_stream = Option::from(Timestamp::from_millis(0));
        }
        return;
    }
    if ccx_options.live_stream.is_none() || ccx_options.live_stream.unwrap().seconds() == -1 {
        // Sleep without timeout check
        sleep_secs(1);
        return;
    }
    // Get current time
    let current_time = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .expect("System time went backwards")
        .as_secs();

    if let Some(live_stream) = ccx_options.live_stream {
        if live_stream.seconds() != 0 {
            if current_time > start + live_stream.millis() as u64 {
                // Timeout elapsed
                ccx_options.live_stream = Option::from(Timestamp::from_millis(0));
            } else {
                sleep_secs(1);
            }
        } else {
            sleep_secs(1);
        }
    } else {
        sleep_secs(1);
    }
}
/// # Safety
///
/// This function has to copy the demuxer from C to Rust, and open a file with a raw file descriptor, thus it is unsafe.
pub unsafe fn switch_to_next_file(
    ctx: &mut lib_ccx_ctx,
    bytes_in_buffer: i64,
    ccx_options: &mut Options,
) -> i32 {
    let mut ret = 0;

    // 1. Initially reset condition
    let mut demux_ctx = copy_demuxer_from_c_to_rust(ctx.demux_ctx);
    if ctx.current_file == -1 || !ccx_options.binary_concat {
        demux_ctx.reset();
    }

    // 2. Handle special input sources
    #[allow(deref_nullptr)]
    match ccx_options.input_source {
        DataSource::Stdin | DataSource::Network | DataSource::Tcp => {
            demux_ctx.open(*ptr::null(), ccx_options);
            return match ret {
                r if r < 0 => 0,
                r if r > 0 => r,
                _ => 1,
            };
        }
        _ => {}
    }

    // 3. Close current file handling

    if demux_ctx.is_open() {
        debug!(
            msg_type = DebugMessageFlag::DECODER_708;
            "[CEA-708] The 708 decoder was reset [{}] times.\n",
            unsafe { (*ctx.freport.data_from_708).reset_count }
        );

        if ccx_options.print_file_reports {
            print_file_report(ctx);
        }

        // Premature end check
        // Only warn about premature ending if:
        // 1. File has known size
        // 2. User-defined limits were NOT reached (is_decoder_processed_enough == 0)
        // 3. Less data was processed than file size
        // 4. There are actually decoders active (decoder list not empty)
        // If the decoder list is empty, it means no captions were found, which is a
        // normal condition - don't warn about it.
        if ctx.inputsize > 0
            && is_decoder_processed_enough(ctx) == 0
            && (demux_ctx.past + bytes_in_buffer < ctx.inputsize)
            && !list_empty(&ctx.dec_ctx_head)
        {
            debug!(msg_type = DebugMessageFlag::DECODER_708; "\n\n\n\nATTENTION!!!!!!");
            debug!(
                msg_type = DebugMessageFlag::DECODER_708;
                "In Rust:switch_to_next_file(): Processing of {} {} ended prematurely {} < {}, please send bug report.\n\n",
                CStr::from_ptr((*ctx.inputfile).add(ctx.current_file as usize)).to_string_lossy(),
                ctx.current_file,
                demux_ctx.past,
                ctx.inputsize
            );
        }

        demux_ctx.close(&mut *ccx_options);
        if ccx_options.binary_concat {
            ctx.total_past += ctx.inputsize;
            demux_ctx.past = 0;
        }
    }
    // 4. File iteration loop
    loop {
        ctx.current_file += 1;
        if ctx.current_file >= ctx.num_input_files {
            break;
        }

        println!("\n\r-----------------------------------------------------------------");
        println!(
            "\rOpening file: {}",
            CStr::from_ptr(*ctx.inputfile.add(ctx.current_file as usize)).to_string_lossy(),
        );

        let filename =
            CStr::from_ptr(*ctx.inputfile.add(ctx.current_file as usize)).to_string_lossy();

        ret = demux_ctx.open(&filename, ccx_options);

        if ret < 0 {
            println!(
                "\rWarning: Unable to open input file [{}]",
                CStr::from_ptr(*ctx.inputfile.add(ctx.current_file as usize)).to_string_lossy(),
            );
        } else {
            // Activity reporting
            let mut c = Options::default();
            c.activity_input_file_open(
                &CStr::from_ptr(*ctx.inputfile.add(ctx.current_file as usize)).to_string_lossy(),
            );

            if ccx_options.live_stream.is_some() && ccx_options.live_stream.unwrap().millis() == 0 {
                ctx.inputsize = demux_ctx.get_filesize();
                if !ccx_options.binary_concat {
                    ctx.total_inputsize = ctx.inputsize;
                }
            }
            return 1;
        }
    }

    0
}
/// # Safety
/// This function is unsafe because we are using raw file descriptors and variables imported from C like terminate_asap.
pub unsafe fn buffered_read_opt(
    ctx: &mut CcxDemuxer,
    buffer: *mut u8,
    bytes: usize,
    ccx_options: &mut Options,
) -> usize {
    let origin_buffer_size = bytes;
    let mut copied = 0;
    let mut seconds = 0i64;
    let mut bytes = bytes;

    position_sanity_check(ctx);
    if let Some(live_stream) = &ccx_options.live_stream {
        if live_stream.millis() > 0 {
            seconds = SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .map(|d| d.as_secs() as i64)
                .unwrap_or(0);
        }
    }
    if ccx_options.buffer_input || ctx.filebuffer_pos < ctx.bytesinbuffer {
        // Needs to return data from filebuffer_start+pos to filebuffer_start+pos+bytes-1;
        let mut eof = ctx.infd == -1;
        let mut buffer_ptr = buffer;

        while (!eof
            || (ccx_options.live_stream.is_some()
                && ccx_options.live_stream.unwrap().millis() != 0))
            && bytes > 0
        {
            if terminate_asap != 0 {
                break;
            }
            if eof {
                // No more data available immediately, we sleep a while to give time
                // for the data to come up
                sleepandchecktimeout(seconds as u64, ccx_options);
            }
            let ready = ctx.bytesinbuffer.saturating_sub(ctx.filebuffer_pos);
            if ready == 0 {
                // We really need to read more
                if !ccx_options.buffer_input {
                    // We got in the buffering code because of the initial buffer for
                    // detection stuff. However we don't want more buffering so
                    // we do the rest directly on the final buffer.
                    let mut i;
                    loop {
                        // No code for network support here, because network is always
                        // buffered - if here, then it must be files.
                        if !buffer_ptr.is_null() {
                            // Read
                            #[cfg(unix)]
                            let mut file = File::from_raw_fd(ctx.infd);
                            #[cfg(windows)]
                            let mut file = open_windows(ctx.infd);
                            let slice = slice::from_raw_parts_mut(buffer_ptr, bytes);
                            match file.read(slice) {
                                Ok(n) => i = n as isize,
                                Err(_) => {
                                    mem::forget(file);
                                    fatal!(cause = ExitCause::ReadError; "Error reading input file!\n");
                                }
                            }
                            mem::forget(file);
                            if i == -1 {
                                fatal!(cause = ExitCause::ReadError; "Error reading input file!\n");
                            }
                            buffer_ptr = buffer_ptr.add(i as usize);
                        } else {
                            // Seek
                            #[cfg(unix)]
                            let mut file = File::from_raw_fd(ctx.infd);
                            #[cfg(windows)]
                            let mut file = open_windows(ctx.infd);
                            let mut op = file.stream_position().unwrap_or(i64::MAX as u64) as i64; // Get current pos
                            if op == i64::MAX {
                                op = -1;
                            }
                            if (op + bytes as i64) < 0 {
                                // Would mean moving beyond start of file: Not supported
                                mem::forget(file);
                                return 0;
                            }
                            let mut np = file
                                .seek(SeekFrom::Current(bytes as i64))
                                .unwrap_or(i64::MAX as u64)
                                as i64; // Pos after moving
                            if np == i64::MAX {
                                np = -1;
                            }

                            i = (np - op) as isize;
                            mem::forget(file);
                        }
                        // if both above lseek returned -1 (error); i would be 0 here and
                        // in case when its not live stream copied would decrease and bytes would...
                        if i == 0
                            && ccx_options.live_stream.is_some()
                            && ccx_options.live_stream.unwrap().millis() != 0
                        {
                            if ccx_options.input_source == DataSource::Stdin {
                                ccx_options.live_stream = Some(Timestamp::from_millis(0));
                                break;
                            } else {
                                sleepandchecktimeout(seconds as u64, ccx_options);
                            }
                        } else {
                            copied += i as usize;
                            bytes -= i as usize;
                        }
                        if !((i != 0
                            || (ccx_options.live_stream.is_some()
                                && ccx_options.live_stream.unwrap().millis() != 0)
                            || (ccx_options.binary_concat
                                && ctx.parent.is_some()
                                && switch_to_next_file(
                                    ctx.parent.as_mut().unwrap(),
                                    copied as i64,
                                    ccx_options,
                                ) != 0))
                            && bytes > 0)
                        {
                            break;
                        }
                    }
                    return copied;
                }
                // Keep the last 8 bytes, so we have a guaranteed
                // working seek (-8) - needed by mythtv.
                let keep = if ctx.bytesinbuffer > 8 {
                    8
                } else {
                    ctx.bytesinbuffer
                };
                copy(
                    ctx.filebuffer.add(FILEBUFFERSIZE - keep as usize),
                    ctx.filebuffer,
                    keep as usize,
                );
                let i = if ccx_options.input_source == DataSource::File
                    || ccx_options.input_source == DataSource::Stdin
                {
                    #[cfg(unix)]
                    let mut file = File::from_raw_fd(ctx.infd);
                    #[cfg(windows)]
                    let mut file = open_windows(ctx.infd);
                    let slice = slice::from_raw_parts_mut(
                        ctx.filebuffer.add(keep as usize),
                        FILEBUFFERSIZE - keep as usize,
                    );
                    let mut result = file.read(slice).unwrap_or(i64::MAX as usize) as isize;
                    if result == i64::MAX as usize as isize {
                        result = -1;
                    }
                    mem::forget(file);
                    result
                } else if ccx_options.input_source == DataSource::Tcp {
                    net_tcp_read(
                        ctx.infd,
                        ctx.filebuffer.add(keep as usize) as *mut c_void,
                        FILEBUFFERSIZE - keep as usize,
                    ) as isize
                } else {
                    net_udp_read(
                        ctx.infd,
                        ctx.filebuffer.add(keep as usize) as *mut c_void,
                        FILEBUFFERSIZE - keep as usize,
                        ccx_options
                            .udpsrc
                            .as_deref()
                            .map_or(ptr::null(), |s| s.as_ptr() as *const c_char),
                        ccx_options
                            .udpaddr
                            .as_deref()
                            .map_or(ptr::null(), |s| s.as_ptr() as *const c_char),
                    ) as isize
                };
                if terminate_asap != 0 {
                    // Looks like receiving a signal here will trigger a -1, so check that first
                    break;
                }
                if i == -1 {
                    fatal!(cause = ExitCause::ReadError; "Error reading input stream!\n");
                }
                if i == 0 {
                    // If live stream, don't try to switch - acknowledge eof here as it won't
                    // cause a loop end
                    if (ccx_options.live_stream.is_some()
                        && ccx_options.live_stream.unwrap().millis() != 0)
                        || (ctx.parent.is_some()
                            && ctx.parent.as_ref().unwrap().inputsize <= origin_buffer_size as i64)
                        || !(ccx_options.binary_concat
                            && switch_to_next_file(
                                ctx.parent.as_mut().unwrap(),
                                copied as i64,
                                ccx_options,
                            ) != 0)
                    {
                        eof = true;
                    }
                }
                ctx.filebuffer_pos = keep;
                ctx.bytesinbuffer = i as u32 + keep;
            }
            let copy = std::cmp::min(ready, bytes as u32) as usize;
            if copy > 0 {
                if !buffer_ptr.is_null() {
                    copy_nonoverlapping(
                        ctx.filebuffer.add(ctx.filebuffer_pos as usize),
                        buffer_ptr,
                        copy,
                    );
                    buffer_ptr = buffer_ptr.add(copy);
                }
                ctx.filebuffer_pos += copy as u32;
                bytes -= copy;
                copied += copy;
            }
        }
    } else {
        // Read without buffering
        if !buffer.is_null() {
            let mut buffer_ptr = buffer;
            let mut i;
            while bytes > 0 && ctx.infd != -1 {
                #[cfg(unix)]
                let mut file = File::from_raw_fd(ctx.infd);
                #[cfg(windows)]
                let mut file = open_windows(ctx.infd);
                let slice = slice::from_raw_parts_mut(buffer_ptr, bytes);
                i = file.read(slice).unwrap_or(i64::MAX as usize) as isize;
                if i == i64::MAX as usize as isize {
                    i = -1;
                }
                mem::forget(file);
                if !((i != 0
                    || (ccx_options.live_stream.is_some()
                        && ccx_options.live_stream.unwrap().millis() != 0)
                    || (ccx_options.binary_concat
                        && ctx.parent.as_mut().is_some()
                        && switch_to_next_file(
                            ctx.parent.as_mut().unwrap(),
                            copied as i64,
                            ccx_options,
                        ) != 0))
                    && bytes > 0)
                {
                    break;
                }
                if terminate_asap != 0 {
                    break;
                }
                if i == -1 {
                    fatal!(cause = ExitCause::ReadError; "Error reading input file!\n");
                } else if i == 0 {
                    sleepandchecktimeout(seconds as u64, ccx_options);
                } else {
                    copied += i as usize;
                    bytes -= i as usize;
                    buffer_ptr = buffer_ptr.add(i as usize);
                }
            }
            return copied;
        }
        while bytes != 0 && ctx.infd != -1 {
            if terminate_asap != 0 {
                break;
            }
            #[cfg(unix)]
            let mut file = File::from_raw_fd(ctx.infd);
            #[cfg(windows)]
            let mut file = open_windows(ctx.infd);

            // Try to get current position and seek
            let op_result = file.stream_position(); // Get current pos
            if let Ok(op) = op_result {
                if (op as i64 + bytes as i64) < 0 {
                    // Would mean moving beyond start of file: Not supported
                    mem::forget(file);
                    return 0;
                }
            }

            let np_result = file.seek(SeekFrom::Current(bytes as i64)); // Pos after moving

            let moved = match (op_result, np_result) {
                (Ok(op), Ok(np)) => {
                    // Both seeks succeeded - normal case
                    mem::forget(file);
                    (np - op) as usize
                }
                (Err(_), Err(_)) => {
                    // Both seeks failed - possibly a pipe that doesn't like "skipping"
                    let mut c: u8 = 0;
                    for _ in 0..bytes {
                        let slice = slice::from_raw_parts_mut(&mut c, 1);
                        match &file.read(slice) {
                            Ok(1) => { /* It is fine, we got a byte */ }
                            Ok(0) => { /* EOF, simply continue*/ }
                            Ok(n) if *n > 1 => {
                                // reading more than one byte is fine, but treat as “we read 1” anyway
                            }
                            _ => {
                                let _ = &file;
                                fatal!(cause = ExitCause::ReadError; "reading from file");
                            }
                        }
                    }
                    mem::forget(file);
                    bytes
                }
                (Ok(op), Err(_)) => {
                    // Second seek failed, reset to original position if possible
                    let _ = file.seek(SeekFrom::Start(op));
                    mem::forget(file);
                    let i = (-1_i64 - op as i64) as isize;
                    i as usize
                }
                (Err(_), Ok(np)) => {
                    // First seek failed but second succeeded - unusual case
                    mem::forget(file);
                    let i = (np as i64 - (-1_i64)) as isize;
                    i as usize
                }
            };
            let delta = moved;
            copied += delta;
            bytes -= copied;
            if copied == 0 {
                if ccx_options.live_stream.is_some()
                    && ccx_options.live_stream.unwrap().millis() != 0
                {
                    sleepandchecktimeout(seconds as u64, ccx_options);
                } else if ccx_options.binary_concat && ctx.parent.is_some() {
                    switch_to_next_file(ctx.parent.as_mut().unwrap(), 0, ccx_options);
                } else {
                    break;
                }
            }
        }
    }
    copied
}
/// The function moves the incoming bytes back into the demuxer's filebuffer.
/// It first checks if the requested bytes equal the current filebuffer_pos:
/// if so, it copies and resets filebuffer_pos. If filebuffer_pos is > 0,
/// it discards old data by shifting the valid bytes (filebuffer + filebuffer_pos)
/// to the beginning, resets bytesinbuffer and filebuffer_pos, and then copies
/// the new data into the front. It checks for buffer overflow and then shifts
/// the existing data right by `bytes` before copying new data in.
/// Finally, it increments bytesinbuffer by bytes.
pub fn return_to_buffer(ctx: &mut CcxDemuxer, buffer: &[u8], bytes: u32) {
    unsafe {
        if bytes == ctx.filebuffer_pos {
            // Usually we're just going back in the buffer and memcpy would be
            // unnecessary, but we do it in case we intentionally messed with the
            // buffer
            copy_nonoverlapping(buffer.as_ptr(), ctx.filebuffer, bytes as usize);
            ctx.filebuffer_pos = 0;
            return;
        }
        if ctx.filebuffer_pos > 0 {
            // Discard old bytes, because we may need the space.
            // Non optimal since data is moved later again but we don't care since
            // we're never here in ccextractor.
            let valid_len = ctx.bytesinbuffer - ctx.filebuffer_pos;
            copy(
                ctx.filebuffer.add(ctx.filebuffer_pos as usize),
                ctx.filebuffer,
                valid_len as usize,
            );
            ctx.bytesinbuffer = 0;
            ctx.filebuffer_pos = 0;
        }
        if ctx.bytesinbuffer + bytes > FILEBUFFERSIZE as u32 {
            fatal!(cause = ExitCause::Bug;
                   "Invalid return_to_buffer() - please submit a bug report.");
        }
        // Shift existing data to make room at the front.
        copy(
            ctx.filebuffer as *const u8,
            ctx.filebuffer.add(bytes as usize),
            ctx.bytesinbuffer as usize,
        );
        // Copy the incoming data to the front of the buffer.
        copy_nonoverlapping(buffer.as_ptr(), ctx.filebuffer, bytes as usize);
    }
    ctx.bytesinbuffer += bytes;
}
/// # Safety
/// This function is unsafe because it calls multiple unsafe functions like `from_raw_parts` and `buffered_read_opt`
pub unsafe fn buffered_read(
    ctx: &mut CcxDemuxer,
    buffer: Option<&mut [u8]>,
    bytes: usize,
    ccx_options: &mut Options,
) -> usize {
    let available = (ctx.bytesinbuffer - ctx.filebuffer_pos) as usize;

    if bytes <= available {
        if let Some(buf) = buffer {
            let src = slice::from_raw_parts(ctx.filebuffer.add(ctx.filebuffer_pos as usize), bytes);
            buf[..bytes].copy_from_slice(src);
        }
        ctx.filebuffer_pos += bytes as u32;
        bytes
    } else {
        let ptr = buffer
            .map(|b| b.as_mut_ptr())
            .unwrap_or(std::ptr::null_mut());

        let result = buffered_read_opt(ctx, ptr, bytes, ccx_options);

        if ccx_options.gui_mode_reports && ccx_options.input_source == DataSource::Network {
            net_activity_gui += 1;
            if net_activity_gui.is_multiple_of(1000) {
                #[allow(static_mut_refs)]
                ccx_options.activity_report_data_read(&mut net_activity_gui);
            }
        }

        result
    }
}

/// # Safety
/// This function is unsafe because it dereferences a raw pointer and calls unsafe function `buffered_read_opt`
pub unsafe fn buffered_read_byte(
    ctx: &mut CcxDemuxer,
    buffer: Option<&mut u8>,
    ccx_options: &mut Options,
) -> usize {
    if ctx.bytesinbuffer > ctx.filebuffer_pos {
        if let Some(buf) = buffer {
            *buf = *ctx.filebuffer.add(ctx.filebuffer_pos as usize);
            ctx.filebuffer_pos += 1;
            return 1;
        }
    } else {
        let ptr = buffer.map(|b| b as *mut u8).unwrap_or(std::ptr::null_mut());
        return buffered_read_opt(ctx, ptr, 1, ccx_options);
    }
    0
}
/// # Safety
/// This function is unsafe because it dereferences a raw pointer and calls unsafe function `buffered_read_byte`
pub unsafe fn buffered_get_be16(ctx: &mut CcxDemuxer, ccx_options: &mut Options) -> u16 {
    let mut a: u8 = 0;
    let mut b: u8 = 0;

    buffered_read_byte(ctx, Some(&mut a), ccx_options);
    ctx.past += 1;

    buffered_read_byte(ctx, Some(&mut b), ccx_options);
    ctx.past += 1;

    ((a as u16) << 8) | (b as u16)
}
/// # Safety
/// This function is unsafe because it dereferences a raw pointer and calls unsafe function `buffered_read_byte`
pub unsafe fn buffered_get_byte(ctx: &mut CcxDemuxer, ccx_options: &mut Options) -> u8 {
    let mut b: u8 = 0;

    if buffered_read_byte(ctx, Some(&mut b), ccx_options) == 1 {
        ctx.past += 1;
        return b;
    }

    0
}
/// # Safety
/// This function is unsafe because it dereferences a raw pointer and calls unsafe function `buffered_get_be16`
pub unsafe fn buffered_get_be32(ctx: &mut CcxDemuxer, ccx_options: &mut Options) -> u32 {
    let high = (buffered_get_be16(ctx, ccx_options) as u32) << 16;
    let low = buffered_get_be16(ctx, ccx_options) as u32;

    high | low
}
/// # Safety
/// This function is unsafe because it dereferences a raw pointer and calls unsafe function `buffered_read_byte`
pub unsafe fn buffered_get_le16(ctx: &mut CcxDemuxer, ccx_options: &mut Options) -> u16 {
    let mut a: u8 = 0;
    let mut b: u8 = 0;

    buffered_read_byte(ctx, Some(&mut a), ccx_options);
    ctx.past += 1;

    buffered_read_byte(ctx, Some(&mut b), ccx_options);
    ctx.past += 1;

    ((b as u16) << 8) | (a as u16)
}
/// # Safety
/// This function is unsafe because it dereferences a raw pointer and calls unsafe function `buffered_get_le16`
pub unsafe fn buffered_get_le32(ctx: &mut CcxDemuxer, ccx_options: &mut Options) -> u32 {
    let low = buffered_get_le16(ctx, ccx_options) as u32;
    let high = (buffered_get_le16(ctx, ccx_options) as u32) << 16;

    low | high
}
/// # Safety
/// This function is unsafe because it dereferences a raw pointer and calls unsafe function `buffered_read_opt`
pub unsafe fn buffered_skip(ctx: &mut CcxDemuxer, bytes: u32, ccx_options: &mut Options) -> usize {
    let available = ctx.bytesinbuffer.saturating_sub(ctx.filebuffer_pos);

    if bytes <= available {
        ctx.filebuffer_pos += bytes;
        bytes as usize
    } else {
        buffered_read_opt(ctx, ptr::null_mut(), bytes as usize, ccx_options)
    }
}
#[cfg(test)]
#[allow(clippy::field_reassign_with_default)]
mod tests {
    use super::*;
    use crate::libccxr_exports::demuxer::copy_demuxer_from_rust_to_c;
    use lib_ccxr::common::{Codec, StreamMode, StreamType};
    use lib_ccxr::util::log::{set_logger, CCExtractorLogger, DebugMessageMask, OutputTarget};
    use serial_test::serial;
    use std::ffi::CString;
    #[cfg(feature = "sanity_check")]
    use std::io::Write;
    use std::os::raw::{c_char, c_int, c_ulong, c_void};
    #[cfg(unix)]
    use std::os::unix::io::IntoRawFd;
    #[cfg(windows)]
    use std::os::windows::io::IntoRawHandle;
    use std::ptr::null_mut;
    use std::slice;
    use std::sync::Once;
    #[cfg(feature = "sanity_check")]
    use tempfile::tempfile;

    static INIT: Once = Once::new();
    pub static mut terminate_asap: i32 = 0;
    pub static mut net_activity_gui: c_ulong = 0;
    pub fn net_udp_read(
        _socket: c_int,
        _buffer: *mut c_void,
        _length: usize,
        _src_str: *const c_char,
        _addr_str: *const c_char,
    ) -> c_int {
        0
    }
    pub fn net_tcp_read(_socket: c_int, _buffer: *mut c_void, _length: usize) -> c_int {
        0
    }

    fn initialize_logger() {
        INIT.call_once(|| {
            set_logger(CCExtractorLogger::new(
                OutputTarget::Stdout,
                DebugMessageMask::new(DebugMessageFlag::VERBOSE, DebugMessageFlag::VERBOSE),
                false,
            ))
            .ok();
        });
    }
    #[cfg(feature = "sanity_check")]
    #[test]
    fn test_position_sanity_check_valid() {
        // To run - type  RUST_MIN_STACK=16777216 cargo test --lib --features sanity_check
        // Create temp file
        let mut file = tempfile().unwrap();
        file.write_all(b"test data").unwrap();
        #[cfg(unix)]
        let fd = file.into_raw_fd(); // Now owns the fd
        #[cfg(windows)]
        let fd = file.into_raw_handle(); // Now owns the handle

        // SAFETY: Initialize directly on heap without stack intermediate
        let mut ctx = Box::new(CcxDemuxer::default());

        // Set only needed fields
        ctx.infd = fd;
        ctx.past = 100;
        ctx.filebuffer_pos = 20;
        ctx.bytesinbuffer = 50;

        // Initialize large arrays on HEAP
        // Example for one array - repeat for others:
        ctx.startbytes = vec![0u8; STARTBYTESLENGTH];
        // Set file position
        #[cfg(unix)]
        let mut file = unsafe { File::from_raw_fd(fd) };
        #[cfg(windows)]
        let mut file = unsafe { open_windows(fd) };
        file.seek(SeekFrom::Start(130)).unwrap();

        ctx.drop_fd(file);

        // Run test
        position_sanity_check(&mut ctx);
    }
    #[test]
    fn test_sleep_secs() {
        let start = SystemTime::now();
        sleep_secs(1);
        let duration = start.elapsed().unwrap().as_secs();
        assert!((1..=2).contains(&duration));
    }

    #[test]
    fn test_sleepandchecktimeout_stdin() {
        {
            let mut ccx_options = Options::default();
            ccx_options.input_source = DataSource::Stdin;
            ccx_options.live_stream = Some(Timestamp::from_millis(1000));

            let start = SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .unwrap()
                .as_secs();

            sleepandchecktimeout(start, &mut ccx_options);

            // Now, re-lock to verify the changes.
            assert_eq!(ccx_options.live_stream.unwrap().millis(), 0);
            assert!((null_mut::<*mut u8>()).is_null());
        }
    }
    // #[test] // Uncomment to run, needs real file
    #[allow(unused)]
    fn test_switch_to_next_file_success() {
        unsafe {
            initialize_logger();
            // Create a demuxer and leak its pointer.
            let demuxer = (CcxDemuxer::default());
            // let demuxer_ptr = Box::into_raw(demuxer);
            let mut ctx = lib_ccx_ctx::default();

            ctx.current_file = -1;
            ctx.num_input_files = 2;
            let c_strings: Vec<CString> = vec![
                CString::new("/home/file1.ts").unwrap(),
                CString::new("/home/file2.ts").unwrap(),
            ];
            let c_pointers: Vec<*mut c_char> = c_strings
                .iter()
                .map(|s| s.as_ptr() as *mut c_char)
                .collect();
            ctx.inputfile = c_pointers.as_ptr() as *mut *mut c_char;
            copy_demuxer_from_rust_to_c(ctx.demux_ctx, &demuxer);
            ctx.inputsize = 0;
            ctx.total_inputsize = 0;
            ctx.total_past = 0;

            println!("{:?}", ctx.inputfile);
            // Reset global options.
            let mut ccx_options = Options::default();
            // Ensure we're not using stdin/network so we take the file iteration path.
            ccx_options.input_source = DataSource::File;

            // First call should open file1.ts.
            assert_eq!(switch_to_next_file(&mut ctx, 0, &mut ccx_options), 1);
            assert_eq!(ctx.current_file, 0);
            // Expect inputsize to be set (e.g., 1000 bytes as per the test expectation).
            assert_eq!(ctx.inputsize, 51);

            // Second call should open file2.ts.
            assert_eq!(switch_to_next_file(&mut ctx, 0, &mut ccx_options), 1);
            assert_eq!(ctx.current_file, 1);
        }
    }

    // #[test]
    #[allow(unused)]
    fn test_switch_to_next_file_failure() {
        unsafe {
            let demuxer = (CcxDemuxer::default());
            let mut ctx = lib_ccx_ctx::default();
            ctx.current_file = 0;
            ctx.num_input_files = 2;
            let c_strings: Vec<CString> = vec![
                CString::new("/home/file1.ts").unwrap(),
                CString::new("/home/file2.ts").unwrap(),
            ];
            let c_pointers: Vec<*mut c_char> = c_strings
                .iter()
                .map(|s| s.as_ptr() as *mut c_char)
                .collect();
            ctx.inputfile = c_pointers.as_ptr() as *mut *mut c_char;
            copy_demuxer_from_rust_to_c(ctx.demux_ctx, &demuxer);
            ctx.inputsize = 0;
            ctx.total_inputsize = 0;
            ctx.total_past = 0;

            // Reset global options.
            let mut ccx_options = Options::default();

            // Should try both files and fail
            assert_eq!(switch_to_next_file(&mut ctx, 0, &mut ccx_options), 0);
            assert_eq!(ctx.current_file, 2);
        }
    }

    // #[test]
    #[allow(unused)]
    fn test_binary_concat_mode() {
        unsafe {
            let mut demuxer0 = (CcxDemuxer::default());
            let mut demuxer = (CcxDemuxer::default());
            let mut ctx = lib_ccx_ctx::default();

            ctx.current_file = -1;
            ctx.num_input_files = 2;
            let c_strings: Vec<CString> = vec![
                CString::new("/home/file1.ts").unwrap(),
                CString::new("/home/file2.ts").unwrap(),
            ];
            let c_pointers: Vec<*mut c_char> = c_strings
                .iter()
                .map(|s| s.as_ptr() as *mut c_char)
                .collect();
            ctx.inputfile = c_pointers.as_ptr() as *mut *mut c_char;
            copy_demuxer_from_rust_to_c(ctx.demux_ctx, &demuxer0);
            (demuxer).infd = 3;
            ctx.inputsize = 500;
            ctx.total_past = 1000;
            // Reset global options.

            let mut ccx_options = &mut Options::default();

            ccx_options.binary_concat = true;
            ctx.binary_concat = 1;
            println!("binary_concat: {}", ctx.binary_concat);
            println!("ccx binary concat: {:?}", ccx_options.binary_concat);
            switch_to_next_file(&mut ctx, 0, ccx_options);
            assert_eq!(ctx.total_past, 1500); // 1000 + 500
            assert_eq!({ (*ctx.demux_ctx).past }, 0);

            ccx_options.binary_concat = false;
        }
    }
    // Start of testing buffered_read_opt
    fn create_temp_file_with_content(content: &[u8]) -> i32 {
        use std::io::{Seek, SeekFrom, Write};
        use tempfile::NamedTempFile;
        let mut tmp = NamedTempFile::new().expect("Unable to create temp file");
        tmp.write_all(content)
            .expect("Unable to write to temp file");
        // Rewind the file pointer to the start.
        tmp.as_file_mut()
            .seek(SeekFrom::Start(0))
            .expect("Unable to seek to start");
        // Get the file descriptor. Ensure the file stays open.
        let file = tmp.reopen().expect("Unable to reopen temp file");
        #[cfg(unix)]
        {
            file.into_raw_fd()
        }
        #[cfg(windows)]
        {
            file.into_raw_handle() as i32
        }
    }

    // Dummy allocation for filebuffer.
    fn allocate_filebuffer() -> *mut u8 {
        // For simplicity, we allocate FILEBUFFERSIZE bytes.
        const FILEBUFFERSIZE: usize = 1024 * 1024 * 16;
        let buf = vec![0u8; FILEBUFFERSIZE].into_boxed_slice();
        Box::into_raw(buf) as *mut u8
    }

    #[test]
    #[serial]
    fn test_buffered_read_opt_buffered_mode() {
        initialize_logger();
        let mut ccx_options = Options::default();
        // Set options to use buffering.
        ccx_options.buffer_input = true;
        ccx_options.live_stream = Some(Timestamp::from_millis(0));
        ccx_options.input_source = DataSource::File;
        ccx_options.binary_concat = false;
        // TERMINATE_ASAP = false;
        // Create a temp file with known content.
        let content = b"Hello, Rust buffered read!";
        let fd = create_temp_file_with_content(content);

        // Allocate a filebuffer and set dummy values.
        let filebuffer = allocate_filebuffer();
        // For test, we simulate that filebuffer is empty.
        let mut ctx = CcxDemuxer::default();
        ctx.infd = fd;
        ctx.past = 0;
        ctx.filebuffer = filebuffer;
        ctx.filebuffer_start = 0;
        ctx.filebuffer_pos = 0;
        ctx.bytesinbuffer = 0;
        ctx.stream_mode = StreamMode::Asf;
        ctx.auto_stream = StreamMode::Asf;
        ctx.startbytes = Vec::new();
        ctx.startbytes_pos = 0;
        ctx.startbytes_avail = 0;
        ctx.ts_autoprogram = false;
        ctx.ts_allprogram = false;
        ctx.flag_ts_forced_pn = false;
        ctx.flag_ts_forced_cappid = false;
        ctx.ts_datastreamtype = StreamType::Unknownstream;
        ctx.pinfo = vec![];
        ctx.nb_program = 0;
        ctx.codec = Codec::Dvb;
        ctx.nocodec = Codec::Dvb;
        ctx.cinfo_tree = CapInfo::default();
        ctx.global_timestamp = Timestamp::from_millis(0);
        ctx.min_global_timestamp = Timestamp::from_millis(0);
        ctx.offset_global_timestamp = Timestamp::from_millis(0);
        ctx.last_global_timestamp = Timestamp::from_millis(0);
        ctx.global_timestamp_inited = Timestamp::from_millis(0);
        // unsafe { ctx.parent = *ptr::null_mut(); }
        // Prepare an output buffer.
        let mut out_buf1 = vec![0u8; content.len()];
        let out_buf2 = vec![0u8; content.len()];
        let read_bytes = unsafe {
            buffered_read_opt(
                &mut ctx,
                out_buf1.as_mut_ptr(),
                out_buf2.len(),
                &mut ccx_options,
            )
        };
        assert_eq!(read_bytes, content.len());
        assert_eq!(&out_buf1, content);

        // Free the allocated filebuffer.
        unsafe {
            let _ = Box::from_raw(filebuffer);
        };
    }

    #[test]
    #[serial]
    fn test_buffered_read_opt_direct_mode() {
        let ccx_options = &mut Options::default();
        // Set options to disable buffering.
        ccx_options.buffer_input = false;
        ccx_options.live_stream = Some(Timestamp::from_millis(0));
        ccx_options.input_source = DataSource::File;
        ccx_options.binary_concat = false;
        // TERMINATE_ASAP = false;
        let content = b"Direct read test.";
        let fd = create_temp_file_with_content(content);

        // In non-buffered mode, filebuffer is not used.
        let mut ctx = CcxDemuxer::default();
        ctx.infd = fd;
        ctx.past = 0;
        ctx.filebuffer = ptr::null_mut();
        ctx.filebuffer_start = 0;
        ctx.filebuffer_pos = 0;
        ctx.bytesinbuffer = 0;
        ctx.stream_mode = StreamMode::Asf;
        ctx.auto_stream = StreamMode::Asf;
        ctx.startbytes = Vec::new();
        ctx.startbytes_pos = 0;
        ctx.startbytes_avail = 0;
        ctx.ts_autoprogram = false;
        ctx.ts_allprogram = false;
        ctx.flag_ts_forced_pn = false;
        ctx.flag_ts_forced_cappid = false;
        ctx.ts_datastreamtype = StreamType::Unknownstream;
        ctx.pinfo = vec![];
        ctx.nb_program = 0;
        ctx.codec = Codec::Dvb;
        ctx.nocodec = Codec::Dvb;
        ctx.cinfo_tree = CapInfo::default();
        ctx.global_timestamp = Timestamp::from_millis(0);
        ctx.min_global_timestamp = Timestamp::from_millis(0);
        ctx.offset_global_timestamp = Timestamp::from_millis(0);
        ctx.last_global_timestamp = Timestamp::from_millis(0);
        ctx.global_timestamp_inited = Timestamp::from_millis(0);
        // unsafe { ctx.parent = *ptr::null_mut(); }

        let mut out_buf1 = vec![0u8; content.len()];
        let out_buf2 = vec![0u8; content.len()];
        let read_bytes = unsafe {
            buffered_read_opt(
                &mut ctx,
                out_buf1.as_mut_ptr(),
                out_buf2.len(),
                &mut *ccx_options,
            )
        };
        assert_eq!(read_bytes, content.len());
        assert_eq!(&out_buf1, content);
    }

    // Helper: create a dummy CcxDemuxer with a preallocated filebuffer.
    fn create_ccx_demuxer_with_buffer<'a>() -> CcxDemuxer<'a> {
        let mut demuxer = CcxDemuxer::default();
        demuxer.filebuffer = allocate_filebuffer();
        demuxer.bytesinbuffer = 0;
        demuxer.filebuffer_pos = 0;
        demuxer
    }

    // Test 1: When the incoming bytes equal filebuffer_pos.
    #[test]
    fn test_return_to_buffer_exact_position() {
        // Prepare a demuxer with a filebuffer.
        let mut ctx = create_ccx_demuxer_with_buffer();
        // Pre-set filebuffer_pos to some value.
        ctx.filebuffer_pos = 5;
        // Pre-fill the filebuffer with known data.
        unsafe {
            // Fill filebuffer[0..5] with pattern 0xAA.
            for i in 0..5 {
                *ctx.filebuffer.add(i) = 0xAA;
            }
        }
        // Create an input buffer of length 5 with different pattern.
        let input = [0x55u8; 5];
        // Call return_to_buffer with bytes equal to filebuffer_pos.
        return_to_buffer(&mut ctx, &input, 5);
        // Expect that filebuffer now has the input data, and filebuffer_pos is reset.
        unsafe {
            let out_slice = slice::from_raw_parts(ctx.filebuffer, 5);
            assert_eq!(out_slice, &input);
        }
        assert_eq!(ctx.filebuffer_pos, 0);
        // Clean up the filebuffer.
        unsafe {
            let _ = Box::from_raw(std::ptr::slice_from_raw_parts_mut(
                ctx.filebuffer,
                FILEBUFFERSIZE,
            ));
        };
    }

    // Test 2: When filebuffer_pos > 0 (discarding old bytes).
    #[test]
    fn test_return_to_buffer_discard_old_bytes() {
        initialize_logger();
        let mut ctx = create_ccx_demuxer_with_buffer();
        // Set filebuffer_pos = 3, bytesinbuffer = 8.
        ctx.filebuffer_pos = 3;
        ctx.bytesinbuffer = 8;
        // Fill filebuffer[0..8] with 0,1,2,3,4,5,6,7.
        unsafe {
            for i in 0..8 {
                *ctx.filebuffer.add(i) = i as u8;
            }
        }
        // Call return_to_buffer with input of 2 bytes.
        let input = [0xFF, 0xEE];
        return_to_buffer(&mut ctx, &input, 2);
        // According to the C code logic, when filebuffer_pos > 0 the old data is discarded.
        // Therefore, final filebuffer should contain only the new input.
        unsafe {
            let out = slice::from_raw_parts(ctx.filebuffer, ctx.bytesinbuffer as usize);
            // The new front (first 2 bytes) should equal input.
            assert_eq!(&out[0..2], &input);
            // There should be no additional data.
        }
        // Clean up.
        unsafe {
            let _ = Box::from_raw(std::ptr::slice_from_raw_parts_mut(
                ctx.filebuffer,
                FILEBUFFERSIZE,
            ));
        };
    }

    //buffered_read tests

    // Test 1: Direct branch - when requested bytes <= available in filebuffer.
    #[test]
    fn test_buffered_read_direct_branch() {
        let ccx_options = &mut Options::default();
        let mut ctx = create_ccx_demuxer_with_buffer();
        // Set filebuffer with known data.
        let data = b"Hello, direct read!";
        let data_len = data.len() as u32;
        unsafe {
            // Copy data into filebuffer.
            let dest = slice::from_raw_parts_mut(ctx.filebuffer, data.len());
            dest.copy_from_slice(data);
        }
        ctx.bytesinbuffer = data_len;
        ctx.filebuffer_pos = 0;
        // Prepare an output buffer.
        let mut out_buf = vec![0u8; data.len()];
        let read_bytes =
            unsafe { buffered_read(&mut ctx, Some(&mut out_buf), data.len(), &mut *ccx_options) };
        assert_eq!(read_bytes, data.len());
        assert_eq!(&out_buf, data);
        // filebuffer_pos should be advanced.
        assert_eq!(ctx.filebuffer_pos, data_len);
        // Clean up.
        unsafe {
            let _ = Box::from_raw(std::ptr::slice_from_raw_parts_mut(
                ctx.filebuffer,
                FILEBUFFERSIZE,
            ));
        };
    }

    #[test]
    #[serial]
    fn test_buffered_read_buffered_opt_branch() {
        let ccx_options = &mut Options::default();
        // Create a temporary file with known content.
        let content = b"Hello, Rust buffered read!";
        let fd = create_temp_file_with_content(content);
        let mut ctx = create_ccx_demuxer_with_buffer();
        ctx.infd = fd;
        ctx.past = 0;
        ctx.filebuffer_pos = 0;
        // Set bytesinbuffer to 0 to force the else branch.
        ctx.bytesinbuffer = 0;
        // Prepare an output buffer with size equal to the content length.
        let req = content.len();
        let mut out_buf = vec![0u8; req];
        let read_bytes =
            unsafe { buffered_read(&mut ctx, Some(&mut out_buf), req, &mut *ccx_options) };
        // Expect that the file content is read.
        assert_eq!(read_bytes, content.len());
        assert_eq!(&out_buf, content);
        unsafe {
            let _ = Box::from_raw(std::ptr::slice_from_raw_parts_mut(
                ctx.filebuffer,
                FILEBUFFERSIZE,
            ));
        };
    }

    // Test C: When gui_mode_reports is enabled and input_source is Network.
    #[test]
    #[serial]
    fn test_buffered_read_network_gui_branch() {
        // Create a temporary file with known content.
        let content = b"Network buffered read test!";
        let fd = create_temp_file_with_content(content);
        let mut ctx = create_ccx_demuxer_with_buffer();
        ctx.infd = fd;
        ctx.past = 0;
        ctx.filebuffer_pos = 0;
        // Force the else branch.
        ctx.bytesinbuffer = 0;
        let req = content.len();
        let mut out_buf = vec![0u8; req];
        let ccx_options = &mut Options::default();
        ccx_options.gui_mode_reports = true;
        ccx_options.input_source = DataSource::Network;
        let read_bytes =
            unsafe { buffered_read(&mut ctx, Some(&mut out_buf), req, &mut *ccx_options) };
        // Expect that the file content is read.
        assert_eq!(read_bytes, content.len());
        assert_eq!(&out_buf, content);
        // Check that NET_ACTIVITY_GUI has been incremented.
        unsafe {
            let _ = Box::from_raw(std::ptr::slice_from_raw_parts_mut(
                ctx.filebuffer,
                FILEBUFFERSIZE,
            ));
        };
    }
    // Tests for buffered_read_byte

    // Test 1: When available data exists in the filebuffer and a valid buffer is provided.
    #[test]
    fn test_buffered_read_byte_available() {
        let ccx_options = &mut Options::default();
        let mut ctx = create_ccx_demuxer_with_buffer();
        // Pre-fill filebuffer with known data.
        let data = b"\xAA\xBB\xCC";
        unsafe {
            let fb = slice::from_raw_parts_mut(ctx.filebuffer, data.len());
            fb.copy_from_slice(data);
        }
        ctx.bytesinbuffer = data.len() as u32;
        ctx.filebuffer_pos = 1; // Assume one byte has already been read.
        let mut out_byte: u8 = 0;
        let read = unsafe { buffered_read_byte(&mut ctx, Some(&mut out_byte), &mut *ccx_options) };
        // Expect to read 1 byte, which should be data[1] = 0xBB.
        assert_eq!(read, 1);
        assert_eq!(out_byte, 0xBB);
        // filebuffer_pos should have advanced.
        assert_eq!(ctx.filebuffer_pos, 2);
    }

    // Test 2: When available data exists but buffer is None.
    #[test]
    fn test_buffered_read_byte_available_none() {
        let ccx_options = &mut Options::default();
        let mut ctx = create_ccx_demuxer_with_buffer();
        let data = b"\x11\x22";
        unsafe {
            let fb = slice::from_raw_parts_mut(ctx.filebuffer, data.len());
            fb.copy_from_slice(data);
        }
        ctx.bytesinbuffer = data.len() as u32;
        ctx.filebuffer_pos = 0;
        // Call with None; expect it returns 0 since nothing is copied.
        let read = unsafe { buffered_read_byte(&mut ctx, None, &mut *ccx_options) };
        assert_eq!(read, 0);
        // filebuffer_pos remains unchanged.
        assert_eq!(ctx.filebuffer_pos, 0);
    }

    // Tests for buffered_get_be16

    // Test 1: When filebuffer has at least 2 available bytes.
    #[test]
    fn test_buffered_get_be16_from_buffer() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let ccx_options = &mut Options::default();
        // Fill filebuffer with two bytes: 0x12, 0x34.
        let data = [0x12u8, 0x34u8];
        unsafe {
            let fb = slice::from_raw_parts_mut(ctx.filebuffer, 2);
            fb.copy_from_slice(&data);
        }
        ctx.bytesinbuffer = 2;
        ctx.filebuffer_pos = 0;
        ctx.past = 0;
        let value = unsafe { buffered_get_be16(&mut ctx, &mut *ccx_options) };
        // Expect 0x1234.
        assert_eq!(value, 0x1234);
        // past should have been incremented twice.
        assert_eq!(ctx.past, 2);
        // filebuffer_pos should have advanced by 2.
        assert_eq!(ctx.filebuffer_pos, 2);
    }

    // Test 2: When filebuffer is empty, forcing buffered_read_opt for each byte.
    #[test]
    #[serial]
    fn test_buffered_get_be16_from_opt() {
        initialize_logger();
        let ccx_options = &mut Options::default();
        #[allow(unused_variables)]
        let ctx = create_ccx_demuxer_with_buffer();
        let content = b"Network buffered read test ABCD!";
        let fd = create_temp_file_with_content(content);
        let mut ctx = create_ccx_demuxer_with_buffer();
        ctx.infd = fd;
        // Force no available data.
        ctx.bytesinbuffer = 0;
        ctx.filebuffer_pos = 0;
        ctx.past = 0;
        let value = unsafe { buffered_get_be16(&mut ctx, &mut *ccx_options) };
        // Expect the two bytes to be 0xAA each, so 0xAAAA.
        assert_eq!(value, 0x4E65);
        // past should have been incremented by 2.
        assert_eq!(ctx.past, 2);
    }
    //Tests for buffered_get_byte
    #[test]
    fn test_buffered_get_byte_available() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let ccx_options = &mut Options::default();
        // Fill filebuffer with one byte: 0x12.
        let data = [0x12u8];
        unsafe {
            let fb = slice::from_raw_parts_mut(ctx.filebuffer, 1);
            fb.copy_from_slice(&data);
        }
        ctx.bytesinbuffer = 1;
        ctx.filebuffer_pos = 0;
        ctx.past = 0;
        let value = unsafe { buffered_get_byte(&mut ctx, &mut *ccx_options) };
        // Expect 0x12.
        assert_eq!(value, 0x12);
        // past should have been incremented.
        assert_eq!(ctx.past, 1);
        // filebuffer_pos should have advanced by 1.
        assert_eq!(ctx.filebuffer_pos, 1);
    }

    #[test]
    fn test_buffered_get_be32_from_buffer() {
        let ccx_options = &mut Options::default();
        let mut ctx = create_ccx_demuxer_with_buffer();
        // Fill filebuffer with four bytes: 0x12, 0x34, 0x56, 0x78.
        let data = [0x12u8, 0x34u8, 0x56u8, 0x78u8];
        unsafe {
            let fb = slice::from_raw_parts_mut(ctx.filebuffer, 4);
            fb.copy_from_slice(&data);
        }
        ctx.bytesinbuffer = 4;
        ctx.filebuffer_pos = 0;
        ctx.past = 0;
        let value = unsafe { buffered_get_be32(&mut ctx, &mut *ccx_options) };
        // Expect 0x12345678.
        assert_eq!(value, 0x12345678);
        // past should have been incremented by 4.
        assert_eq!(ctx.past, 4);
        // filebuffer_pos should have advanced by 4.
        assert_eq!(ctx.filebuffer_pos, 4);
    }
    #[test]
    #[serial]
    fn test_buffered_get_be32_from_opt() {
        initialize_logger();
        let ccx_options = &mut Options::default();
        #[allow(unused_variables)]
        let ctx = create_ccx_demuxer_with_buffer();
        let content = b"Network buffered read test!";
        let fd = create_temp_file_with_content(content);
        let mut ctx = create_ccx_demuxer_with_buffer();
        ctx.infd = fd;
        // Force no available data.
        ctx.bytesinbuffer = 0;
        ctx.filebuffer_pos = 0;
        ctx.past = 0;
        // In this case, buffered_read_opt (our dummy version) will supply 0xAA for each byte.
        let value = unsafe { buffered_get_be32(&mut ctx, &mut *ccx_options) };
        // Expect the four bytes to be 0xAAAAAAAA.
        assert_eq!(value, 0x4E657477);
        // past should have been incremented by 4.
        assert_eq!(ctx.past, 4);
    }

    //Tests for buffered_get_le16
    #[test]
    fn test_buffered_get_le16_from_buffer() {
        let ccx_options = &mut Options::default();
        let mut ctx = create_ccx_demuxer_with_buffer();
        // Fill filebuffer with two bytes: 0x12, 0x34.
        let data = [0x12u8, 0x34u8];
        unsafe {
            let fb = slice::from_raw_parts_mut(ctx.filebuffer, 2);
            fb.copy_from_slice(&data);
        }
        ctx.bytesinbuffer = 2;
        ctx.filebuffer_pos = 0;
        ctx.past = 0;
        let value = unsafe { buffered_get_le16(&mut ctx, &mut *ccx_options) };
        // Expect 0x3412.
        assert_eq!(value, 0x3412);
        // past should have been incremented by 2.
        assert_eq!(ctx.past, 2);
        // filebuffer_pos should have advanced by 2.
        assert_eq!(ctx.filebuffer_pos, 2);
    }
    #[test]
    #[serial]
    fn test_buffered_get_le16_from_opt() {
        initialize_logger();
        let ccx_options = &mut Options::default();
        #[allow(unused_variables)]
        let ctx = create_ccx_demuxer_with_buffer();
        let content = b"Network buffered read test!";
        let mut ctx = create_ccx_demuxer_with_buffer();
        let fd = create_temp_file_with_content(content);
        ctx.infd = fd;
        // Force no available data.
        ctx.bytesinbuffer = 0;
        ctx.filebuffer_pos = 0;
        ctx.past = 0;
        // In this case, buffered_read_opt (our version) will supply 0xAA for each byte.
        let value = unsafe { buffered_get_le16(&mut ctx, &mut *ccx_options) };
        // Expect the two bytes to be 0xAAAA.
        assert_eq!(value, 0x654E);
        // past should have been incremented by 2.
        assert_eq!(ctx.past, 2);
    }

    //Tests for buffered_get_le32
    #[test]
    fn test_buffered_get_le32_from_buffer() {
        let ccx_options = &mut Options::default();
        let mut ctx = create_ccx_demuxer_with_buffer();
        // Fill filebuffer with four bytes: 0x12, 0x34, 0x56, 0x78.
        let data = [0x12u8, 0x34u8, 0x56u8, 0x78u8];
        unsafe {
            let fb = slice::from_raw_parts_mut(ctx.filebuffer, 4);
            fb.copy_from_slice(&data);
        }
        ctx.bytesinbuffer = 4;
        ctx.filebuffer_pos = 0;
        ctx.past = 0;
        let value = unsafe { buffered_get_le32(&mut ctx, &mut *ccx_options) };
        // Expect 0x78563412.
        assert_eq!(value, 0x78563412);
        // past should have been incremented by 4.
        assert_eq!(ctx.past, 4);
        // filebuffer_pos should have advanced by 4.
        assert_eq!(ctx.filebuffer_pos, 4);
    }

    #[test]
    #[serial]
    fn test_buffered_get_le32_from_opt() {
        initialize_logger();
        let ccx_options = &mut Options::default();
        #[allow(unused_variables)]
        let ctx = create_ccx_demuxer_with_buffer();
        let content = b"Network buffered read test!";
        let fd = create_temp_file_with_content(content);
        let mut ctx = create_ccx_demuxer_with_buffer();
        ctx.infd = fd;
        // Force no available data.
        ctx.bytesinbuffer = 0;
        ctx.filebuffer_pos = 0;
        ctx.past = 0;
        // In this case, buffered_read_opt (our dummy version) will supply 0xAA for each byte.
        let value = unsafe { buffered_get_le32(&mut ctx, &mut *ccx_options) };
        // Expect the four bytes to be 0xAAAAAAAA.
        assert_eq!(value, 0x7774654E);
        // past should have been incremented by 4.
        assert_eq!(ctx.past, 4);
    }
    #[test]
    fn test_buffered_skip_available() {
        let ccx_options = &mut Options::default();
        let mut ctx = create_ccx_demuxer_with_buffer();
        // Prepopulate filebuffer with dummy data (contents don't matter).
        ctx.bytesinbuffer = 50;
        ctx.filebuffer_pos = 10;
        let skip = 20u32;
        let result = unsafe { buffered_skip(&mut ctx, skip, &mut *ccx_options) };
        assert_eq!(result, 20);
        assert_eq!(ctx.filebuffer_pos, 30);
    }

    // Test 5: When requested bytes > available.
    #[test]
    #[serial]
    fn test_buffered_skip_not_available() {
        let ccx_options = &mut Options::default();
        let mut ctx = create_ccx_demuxer_with_buffer();
        // Set available bytes = 10.
        ctx.bytesinbuffer = 10;
        ctx.filebuffer_pos = 10;
        let skip = 15u32;
        let content = b"Network buffered read test!";
        let fd = create_temp_file_with_content(content);
        let mut ctx = create_ccx_demuxer_with_buffer();
        ctx.infd = fd;

        // In this case, buffered_skip will call buffered_read_opt with an empty buffer.
        // Our dummy buffered_read_opt returns the requested number of bytes when the buffer is empty.
        let result = unsafe { buffered_skip(&mut ctx, skip, &mut *ccx_options) };
        assert_eq!(result, 15);
    }
}
