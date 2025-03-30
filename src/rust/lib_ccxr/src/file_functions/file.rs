#![allow(unexpected_cfgs)]
#![allow(unused_mut)]

use crate::activity::{update_net_activity_gui, ActivityExt, NET_ACTIVITY_GUI};
use crate::common::{DataSource, Options};
use crate::demuxer::demux::*;
use crate::demuxer::lib_ccx::*;
use crate::fatal;
use crate::time::Timestamp;
use crate::util::log::ExitCause;
use crate::util::log::{debug, DebugMessageFlag};
use std::ffi::CString;
use std::fs::File;
use std::io::{Read, Seek, SeekFrom};
use std::mem::ManuallyDrop;
#[cfg(unix)]
use std::os::fd::FromRawFd;
#[cfg(windows)]
use std::os::windows::io::FromRawHandle;
use std::ptr::{copy, copy_nonoverlapping};
use std::sync::atomic::Ordering;
use std::sync::{LazyLock, Mutex};
use std::time::{SystemTime, UNIX_EPOCH};
use std::{ptr, slice};

pub static mut TERMINATE_ASAP: bool = false; //TODO convert to Mutex
pub const FILEBUFFERSIZE: usize = 1024 * 1024 * 16; // 16 Mbytes no less. Minimize number of real read calls()
                                                    // lazy_static! {
                                                    //     pub static ref CcxOptions: Mutex<Options> = Mutex::new(Options::default());
                                                    // }

// pub static mut ccx_options:Options = Options::default();

pub static CCX_OPTIONS: LazyLock<Mutex<Options>> = LazyLock::new(|| Mutex::new(Options::default()));

/// This function checks that the current file position matches the expected value.
#[allow(unused_variables)]
pub fn position_sanity_check(ctx: &mut CcxDemuxer) {
    #[cfg(feature = "sanity_check")]
    {
        if ctx.infd != -1 {
            use std::fs::File;
            use std::io::Seek;
            use std::os::unix::io::{FromRawFd, IntoRawFd};

            let fd = ctx.infd;
            // Convert raw fd to a File without taking ownership
            let mut file = unsafe { File::from_raw_fd(fd) };
            let realpos_result = file.seek(SeekFrom::Current(0));
            let realpos = match realpos_result {
                Ok(pos) => pos as i64, // Convert to i64 to match C's LLONG
                Err(_) => {
                    // Return the fd to avoid closing it
                    let _ = file.into_raw_fd();
                    return;
                }
            };
            // Return the fd to avoid closing it
            let _ = file.into_raw_fd();

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
///  # Safety
/// This function is unsafe because it dereferences a raw pointer.
pub unsafe fn sleepandchecktimeout(start: u64) {
    let mut ccx_options = CCX_OPTIONS.lock().unwrap();

    use std::time::{SystemTime, UNIX_EPOCH};

    if ccx_options.input_source == DataSource::Stdin {
        // For stdin, just sleep for 1 second and reset live_stream
        sleep_secs(1);
        if ccx_options.live_stream.unwrap().seconds() != 0 {
            ccx_options.live_stream = Option::from(Timestamp::from_millis(0));
        }
        return;
    }
    if ccx_options.live_stream.unwrap().seconds() != 0
        && ccx_options.live_stream.unwrap().seconds() == -1
    {
        // Sleep without timeout check
        sleep_secs(1);
        return;
    }
    // Get current time
    let current_time = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .expect("System time went backwards")
        .as_secs();

    if ccx_options.live_stream.unwrap().seconds() != 0 {
        if current_time > start + ccx_options.live_stream.unwrap().millis() as u64 {
            // Timeout elapsed
            ccx_options.live_stream = Option::from(Timestamp::from_millis(0));
        } else {
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

/* Close current file and open next one in list -if any- */
/* bytesinbuffer is the number of bytes read (in some buffer) that haven't been added
to 'past' yet. We provide this number to switch_to_next_file() so a final sanity check
can be done */
/// # Safety
/// This function is unsafe because it dereferences a raw pointer and calls multiple unsafe functions like `is_decoder_processed_enough` and `demuxer.open`
pub unsafe fn switch_to_next_file(ctx: &mut LibCcxCtx, bytes_in_buffer: i64) -> i32 {
    let mut ccx_options = CCX_OPTIONS.lock().unwrap();

    let mut ret;

    // 1. Initial reset condition (matching C logic exactly)
    if ctx.current_file == -1 || !ccx_options.binary_concat {
        (*ctx.demux_ctx).reset();
    }

    // 2. Handle special input sources
    #[allow(deref_nullptr)]
    match ccx_options.input_source {
        DataSource::Stdin | DataSource::Network | DataSource::Tcp => {
            ret = (*ctx.demux_ctx).open(*ptr::null());
            return match ret {
                r if r < 0 => 0,
                r if r > 0 => r,
                _ => 1,
            };
        }
        _ => {}
    }

    // 3. Close current file handling

    if let Some(demuxer_ref) = unsafe { ctx.demux_ctx.as_ref() } {
        if demuxer_ref.is_open() {
            // Debug output matching C version
            debug!(
                msg_type = DebugMessageFlag::DECODER_708;
                "[CEA-708] The 708 decoder was reset [{}] times.\n",
                unsafe { (*ctx.freport.data_from_708).reset_count }
            );

            if ccx_options.print_file_reports {
                print_file_report(ctx);
            }

            // Premature end check
            if ctx.inputsize > 0
                && is_decoder_processed_enough(ctx) == 0
                && (unsafe { (*ctx.demux_ctx).past } + bytes_in_buffer < ctx.inputsize)
            {
                println!("\n\n\n\nATTENTION!!!!!!");
                println!(
                    "In switch_to_next_file(): Processing of {} {} ended prematurely {} < {}, please send bug report.\n\n",
                    ctx.inputfile[ctx.current_file as usize],
                    ctx.current_file,
                    unsafe { (*ctx.demux_ctx).past },
                    ctx.inputsize
                );
            }

            close_input_file(ctx);
            if ccx_options.binary_concat {
                ctx.total_past += ctx.inputsize;
                unsafe { (*ctx.demux_ctx).past = 0 };
            }
        }
    }
    // 4. File iteration loop
    loop {
        ctx.current_file += 1;
        if ctx.current_file >= ctx.num_input_files {
            break;
        }

        // Formatting matches C version exactly
        println!("\n\r-----------------------------------------------------------------");
        println!(
            "\rOpening file: {}",
            ctx.inputfile[ctx.current_file as usize]
        );
        #[allow(unused)]
        let c_filename = CString::new(ctx.inputfile[ctx.current_file as usize].as_bytes())
            .expect("Invalid filename");

        let filename = &ctx.inputfile[ctx.current_file as usize];
        ret = (*ctx.demux_ctx).open(filename);

        if ret < 0 {
            println!(
                "\rWarning: Unable to open input file [{}]",
                ctx.inputfile[ctx.current_file as usize]
            );
        } else {
            // Activity reporting
            let mut c = Options::default();
            c.activity_input_file_open(&ctx.inputfile[ctx.current_file as usize]);

            if ccx_options.live_stream.unwrap().millis() == 0 {
                if let Some(get_filesize_fn) = ctx.demux_ctx.as_ref() {
                    ctx.inputsize = get_filesize_fn.get_filesize(&mut *ctx.demux_ctx);
                }
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
/// This function is unsafe because it calls multiple unsafe functions like `switch_to_next_file` and `sleepandchecktimeout`
pub unsafe fn buffered_read_opt(
    ctx: &mut CcxDemuxer,
    mut buffer: &mut [u8],
    mut bytes: usize,
) -> usize {
    let mut ccx_options = CCX_OPTIONS.lock().unwrap();

    // Save original requested bytes.
    let origin_buffer_size = bytes;
    let mut copied = 0;
    let mut seconds = SystemTime::now();

    position_sanity_check(ctx);

    // If live stream is active, update seconds.
    if let Some(ts) = ccx_options.live_stream {
        if ts.millis() > 0 {
            seconds = SystemTime::now();
        }
    }

    // Create one File instance from the raw file descriptor.
    // This instance will be used throughout the function.
    if ctx.infd == -1 {
        return 0;
    }
    #[cfg(unix)]
    let mut file = ManuallyDrop::new(File::from_raw_fd(ctx.infd));
    #[cfg(windows)]
    let mut file = ManuallyDrop::new(File::from_raw_handle(ctx.infd as *mut _));
    // If buffering is enabled or there is data in filebuffer.
    if ccx_options.buffer_input || (ctx.filebuffer_pos < ctx.bytesinbuffer) {
        let mut eof = ctx.infd == -1;

        while (!eof || ccx_options.live_stream.unwrap().millis() > 0) && bytes > 0 {
            if unsafe { TERMINATE_ASAP } {
                break;
            }
            if eof {
                sleepandchecktimeout(seconds.duration_since(UNIX_EPOCH).unwrap().as_secs());
            }
            // ready bytes in buffer.
            let ready = ctx.bytesinbuffer.saturating_sub(ctx.filebuffer_pos);
            if ready == 0 {
                // We really need to read more.
                if !ccx_options.buffer_input {
                    // No buffering desired: do direct I/O.
                    #[allow(unused)]
                    let mut i: isize = 0;
                    loop {
                        if !buffer.is_empty() {
                            match file.read(buffer) {
                                Ok(n) => i = n as isize,
                                Err(_) => {
                                    fatal!(cause = ExitCause::NoInputFiles; "Error reading input file!\n")
                                }
                            }
                            if i < 0 {
                                fatal!(cause = ExitCause::NoInputFiles; "Error reading input file!\n");
                            }
                            let advance = i as usize;
                            buffer = &mut buffer[advance..];
                        } else {
                            let op = file.seek(SeekFrom::Current(0)).unwrap() as i64;
                            if (op + bytes as i64) < 0 {
                                return 0;
                            }
                            let np = file.seek(SeekFrom::Current(bytes as i64)).unwrap() as i64;
                            i = (np - op) as isize;
                        }
                        if i == 0 && ccx_options.live_stream.unwrap().millis() != 0 {
                            if ccx_options.input_source == DataSource::Stdin {
                                ccx_options.live_stream = Some(Timestamp::from_millis(0));

                                break;
                            } else {
                                sleepandchecktimeout(
                                    seconds.duration_since(UNIX_EPOCH).unwrap().as_secs(),
                                );
                            }
                        } else {
                            copied += i as usize;
                            bytes = bytes.saturating_sub(i as usize);
                        }
                        if (i != 0
                            || ccx_options.live_stream.unwrap().millis() != 0
                            || (ccx_options.binary_concat
                                && switch_to_next_file(
                                    ctx.parent.as_mut().unwrap(),
                                    copied as i64,
                                ) != 0))
                            && bytes > 0
                        {
                            continue;
                        } else {
                            break;
                        }
                    }
                    return copied;
                }
                // Buffering branch: read into filebuffer.
                const FILEBUFFERSIZE: usize = 8192;
                // Keep the last 8 bytes, so we have a guaranteed working seek (-8)
                let keep = if ctx.bytesinbuffer > 8 {
                    8
                } else {
                    ctx.bytesinbuffer as usize
                };
                // memmove equivalent: copy the last 'keep' bytes to beginning.
                copy(
                    ctx.filebuffer.add(FILEBUFFERSIZE - keep),
                    ctx.filebuffer,
                    keep,
                );
                // Read more data into filebuffer after the kept bytes.
                let mut read_buf =
                    slice::from_raw_parts_mut(ctx.filebuffer.add(keep), FILEBUFFERSIZE - keep);
                let i = match file.read(read_buf) {
                    Ok(n) => n as isize,
                    Err(_) => {
                        fatal!(cause = ExitCause::NoInputFiles; "Error reading input stream!\n")
                    }
                };
                if unsafe { TERMINATE_ASAP } {
                    break;
                }
                //TODO logic here is a bit diffrent and needs to be updated after net_tcp_read and net_udp_read is implemented in the net module
                if i == -1 {
                    fatal!(cause = ExitCause::NoInputFiles; "Error reading input stream!\n");
                }
                if i == 0
                    && (ccx_options.live_stream.unwrap().millis() > 0
                        || ctx.parent.as_mut().unwrap().inputsize <= origin_buffer_size as i64
                        || !(ccx_options.binary_concat
                            && switch_to_next_file(ctx.parent.as_mut().unwrap(), copied as i64)
                                != 0))
                {
                    eof = true;
                }
                ctx.filebuffer_pos = keep as u32;
                ctx.bytesinbuffer = (i as usize + keep) as u32;
            }
            // Determine number of bytes to copy from filebuffer.
            let copy = std::cmp::min(
                ctx.bytesinbuffer.saturating_sub(ctx.filebuffer_pos),
                bytes as u32,
            ) as usize;
            if copy > 0 {
                ptr::copy(
                    ctx.filebuffer.add(ctx.filebuffer_pos as usize),
                    buffer.as_mut_ptr(),
                    copy,
                );
                ctx.filebuffer_pos += copy as u32;
                bytes = bytes.saturating_sub(copy);
                copied += copy;
                buffer = slice::from_raw_parts_mut(
                    buffer.as_mut_ptr().add(copy),
                    buffer.len().saturating_sub(copy),
                );
            }
        }
    } else {
        // Read without buffering.
        if !buffer.is_empty() {
            let mut i: isize = -1;
            while bytes > 0
                && ctx.infd != -1
                && ({
                    match file.read(buffer) {
                        Ok(n) => {
                            i = n as isize;
                        }
                        Err(_) => {
                            i = -1;
                        }
                    }
                    i != 0
                        || ccx_options.live_stream.unwrap().millis() != 0
                        || (ccx_options.binary_concat
                            && ctx.parent.is_some()
                            && switch_to_next_file(ctx.parent.as_mut().unwrap(), copied as i64)
                                != 0)
                })
            {
                if unsafe { TERMINATE_ASAP } {
                    break;
                }
                if i == -1 {
                    fatal!(cause = ExitCause::NoInputFiles; "Error reading input file!\n");
                } else if i == 0 {
                    sleepandchecktimeout(seconds.duration_since(UNIX_EPOCH).unwrap().as_secs());
                } else {
                    copied += i as usize;
                    bytes = bytes.saturating_sub(i as usize);
                    buffer = slice::from_raw_parts_mut(
                        buffer.as_mut_ptr().add(i as usize),
                        buffer.len().saturating_sub(i as usize),
                    );
                }
            }
            return copied;
        }
        // Seek without a buffer.
        while bytes > 0 && ctx.infd != -1 {
            if unsafe { TERMINATE_ASAP } {
                break;
            }
            let op = file.seek(SeekFrom::Current(0)).unwrap() as i64;
            if (op + bytes as i64) < 0 {
                return 0;
            }
            let np = file.seek(SeekFrom::Current(bytes as i64)).unwrap() as i64;
            if op == -1 && np == -1 {
                let mut c = [0u8; 1];
                for _ in 0..bytes {
                    let n = file.read(&mut c).unwrap_or(0);
                    if n == 0 {
                        fatal!(cause = ExitCause::NoInputFiles; "reading from file");
                    }
                }
                copied = bytes;
            } else {
                copied += (np - op) as usize;
            }
            bytes = bytes.saturating_sub(copied);
            if copied == 0 {
                if ccx_options.live_stream.unwrap().millis() != 0 {
                    sleepandchecktimeout(seconds.duration_since(UNIX_EPOCH).unwrap().as_secs());
                } else if ccx_options.binary_concat {
                    switch_to_next_file(ctx.parent.as_mut().unwrap(), 0);
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
        let result = unsafe { buffered_read_opt(ctx, buffer.unwrap(), bytes) };
        let mut ccx_options = CCX_OPTIONS.lock().unwrap();

        if ccx_options.gui_mode_reports && ccx_options.input_source == DataSource::Network {
            update_net_activity_gui(NET_ACTIVITY_GUI.load(Ordering::SeqCst) + 1);
            if NET_ACTIVITY_GUI.load(Ordering::SeqCst) % 1000 == 0 {
                ccx_options.activity_report_data_read(); // Uncomment or implement as needed.
            }
        }

        result
    }
}
/// # Safety
/// This function is unsafe because it dereferences a raw pointer and calls unsafe function `buffered_read_opt`
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
/// # Safety
/// This function is unsafe because it dereferences a raw pointer and calls unsafe function `buffered_read_byte`
pub unsafe fn buffered_get_be16(ctx: *mut CcxDemuxer) -> u16 {
    if ctx.is_null() {
        return 0;
    }
    let ctx = &mut *ctx; // Dereference the raw pointer safely

    let mut a: u8 = 0;
    let mut b: u8 = 0;

    buffered_read_byte(ctx as *mut CcxDemuxer, Some(&mut a));
    ctx.past += 1;

    buffered_read_byte(ctx as *mut CcxDemuxer, Some(&mut b));
    ctx.past += 1;

    ((a as u16) << 8) | (b as u16)
}
/// # Safety
/// This function is unsafe because it dereferences a raw pointer and calls unsafe function `buffered_read_byte`
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
/// # Safety
/// This function is unsafe because it dereferences a raw pointer and calls unsafe function `buffered_get_be16`
pub unsafe fn buffered_get_be32(ctx: *mut CcxDemuxer) -> u32 {
    if ctx.is_null() {
        return 0;
    }

    let high = (buffered_get_be16(ctx) as u32) << 16;
    let low = buffered_get_be16(ctx) as u32;

    high | low
}
/// # Safety
/// This function is unsafe because it dereferences a raw pointer and calls unsafe function `buffered_read_byte`
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
/// # Safety
/// This function is unsafe because it dereferences a raw pointer and calls unsafe function `buffered_get_le16`
pub unsafe fn buffered_get_le32(ctx: *mut CcxDemuxer) -> u32 {
    if ctx.is_null() {
        return 0;
    }

    let low = buffered_get_le16(ctx) as u32;
    let high = (buffered_get_le16(ctx) as u32) << 16;

    low | high
}
/// # Safety
/// This function is unsafe because it dereferences a raw pointer and calls unsafe function `buffered_read_opt`
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
#[cfg(test)]
mod tests {
    use super::*;
    use crate::common::{Codec, StreamMode};
    use crate::util::log::{set_logger, CCExtractorLogger, DebugMessageMask, OutputTarget};
    // use std::io::{Seek, SeekFrom, Write};
    use serial_test::serial;
    use std::os::unix::io::IntoRawFd;
    use std::slice;
    use std::sync::Once;

    static INIT: Once = Once::new();

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
    #[test]
    #[cfg(feature = "sanity_check")]
    #[test]
    fn test_position_sanity_check_valid() {
        // To run - type  RUST_MIN_STACK=16777216 cargo test --lib --features sanity_check
        // Create temp file
        let mut file = tempfile().unwrap();
        file.write_all(b"test data").unwrap();
        let fd = file.into_raw_fd(); // Now owns the fd

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
        let mut file = unsafe { File::from_raw_fd(fd) };
        file.seek(SeekFrom::Start(130)).unwrap();

        // Prevent double-closing when 'file' drops
        let _ = file.into_raw_fd();

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
    fn test_ccx_options_default() {
        // let mut ccx_options = CcxOptions.lock().unwrap();
        {
            let ccx_options = CCX_OPTIONS.lock().unwrap();

            {
                println!("{:?}", ccx_options);
            }
        }
    }
    #[test]
    fn test_sleepandchecktimeout_stdin() {
        {
            {
                let mut ccx_options = CCX_OPTIONS.lock().unwrap();
                ccx_options.input_source = DataSource::Stdin;
                ccx_options.live_stream = Some(Timestamp::from_millis(1000));
            } // The lock is dropped here.

            let start = SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .unwrap()
                .as_secs();
            unsafe {
                sleepandchecktimeout(start);
            }

            // Now, re-lock to verify the changes.
            let ccx_options = CCX_OPTIONS.lock().unwrap();
            assert_eq!(ccx_options.live_stream.unwrap().millis(), 0);
        }
    }
    // #[test] // Uncomment to run
    #[allow(unused)]
    fn test_switch_to_next_file_success() {
        unsafe {
            initialize_logger();
            // Create a demuxer and leak its pointer.
            let demuxer = Box::from(CcxDemuxer::default());
            let demuxer_ptr = Box::into_raw(demuxer);
            let mut ctx = LibCcxCtx::default();

            ctx.current_file = -1;
            ctx.num_input_files = 2;
            ctx.inputfile = vec!["/home/file1.ts".to_string(), "/home/file2.ts".to_string()];
            ctx.demux_ctx = demuxer_ptr;
            ctx.inputsize = 0;
            ctx.total_inputsize = 0;
            ctx.total_past = 0;

            println!("{:?}", ctx.inputfile);
            // Reset global options.
            {
                let mut opts = CCX_OPTIONS.lock().unwrap();
                *opts = Options::default();
                // Ensure we're not using stdin/network so we take the file iteration path.
                opts.input_source = DataSource::File;
            }

            // First call should open file1.ts.
            assert_eq!(switch_to_next_file(&mut ctx, 0), 1);
            assert_eq!(ctx.current_file, 0);
            // Expect inputsize to be set (e.g., 1000 bytes as per your test expectation).
            assert_eq!(ctx.inputsize, 51);

            // Second call should open file2.ts.
            assert_eq!(switch_to_next_file(&mut ctx, 0), 1);
            assert_eq!(ctx.current_file, 1);

            // Cleanup.
            let _ = Box::from_raw(demuxer_ptr);
        }
    }

    // #[test]
    #[allow(unused)]
    fn test_switch_to_next_file_failure() {
        unsafe {
            let demuxer = Box::from(CcxDemuxer::default());
            let demuxer_ptr = Box::into_raw(demuxer);
            let mut ctx = LibCcxCtx::default();
            ctx.current_file = 0;
            ctx.num_input_files = 2;
            ctx.inputfile = vec!["badfile1.ts".to_string(), "badfile2.ts".to_string()];
            ctx.demux_ctx = demuxer_ptr;
            ctx.inputsize = 0;
            ctx.total_inputsize = 0;
            ctx.total_past = 0;

            // Reset global options.
            {
                let mut opts = CCX_OPTIONS.lock().unwrap();
                *opts = Options::default();
            }

            // Should try both files and fail
            assert_eq!(switch_to_next_file(&mut ctx, 0), 0);
            assert_eq!(ctx.current_file, 2);

            // Cleanup
            let _ = Box::from_raw(demuxer_ptr);
        }
    }

    // #[test]
    #[allow(unused)]
    fn test_binary_concat_mode() {
        unsafe {
            let demuxer = Box::from(CcxDemuxer::default());
            let demuxer_ptr = Box::into_raw(demuxer);
            let mut ctx = LibCcxCtx::default();

            ctx.current_file = -1;
            ctx.num_input_files = 2;
            ctx.inputfile = vec!["/home/file1.ts".to_string(), "/home/file2.ts".to_string()]; // replace with actual paths
            ctx.demux_ctx = demuxer_ptr;
            {
                (*demuxer_ptr).infd = 3;
            } // Mark the demuxer as "open"
            ctx.inputsize = 500;
            ctx.total_past = 1000;
            // Reset global options.

            {
                let mut opts = CCX_OPTIONS.lock().unwrap();
                *opts = Options::default();
            }
            {
                let mut ccx_options = CCX_OPTIONS.lock().unwrap();
                ccx_options.binary_concat = true;
                ctx.binary_concat = 1;
            }
            println!("binary_concat: {}", ctx.binary_concat);
            println!(
                "ccx binary concat: {:?}",
                CCX_OPTIONS.lock().unwrap().binary_concat
            );
            switch_to_next_file(&mut ctx, 0);
            assert_eq!(ctx.total_past, 1500); // 1000 + 500
            assert_eq!({ (*ctx.demux_ctx).past }, 0);

            // Cleanup
            let _ = Box::from_raw(demuxer_ptr);
            let mut ccx_options = CCX_OPTIONS.lock().unwrap();
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
        file.into_raw_fd()
    }

    // Dummy allocation for filebuffer.
    fn allocate_filebuffer() -> *mut u8 {
        // For simplicity, we allocate FILEBUFFERSIZE bytes.
        const FILEBUFFERSIZE: usize = 8192;
        let buf = vec![0u8; FILEBUFFERSIZE].into_boxed_slice();
        Box::into_raw(buf) as *mut u8
    }

    #[test]
    #[serial]
    fn test_buffered_read_opt_buffered_mode() {
        initialize_logger();
        {
            let mut ccx_options = CCX_OPTIONS.lock().unwrap();
            // Set options to use buffering.
            ccx_options.buffer_input = true;
            ccx_options.live_stream = Some(Timestamp::from_millis(0));
            ccx_options.input_source = DataSource::File;
            ccx_options.binary_concat = false;
            // TERMINATE_ASAP = false;
        }
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
        ctx.ts_autoprogram = 0;
        ctx.ts_allprogram = 0;
        ctx.flag_ts_forced_pn = 0;
        ctx.flag_ts_forced_cappid = 0;
        ctx.ts_datastreamtype = 0;
        ctx.pinfo = vec![];
        ctx.nb_program = 0;
        ctx.codec = Codec::Dvb;
        ctx.nocodec = Codec::Dvb;
        ctx.cinfo_tree = CapInfo::default();
        ctx.global_timestamp = 0;
        ctx.min_global_timestamp = 0;
        ctx.offset_global_timestamp = 0;
        ctx.last_global_timestamp = 0;
        ctx.global_timestamp_inited = 0;
        // unsafe { ctx.parent = *ptr::null_mut(); }
        // Prepare an output buffer.
        let mut out_buf1 = vec![0u8; content.len()];
        let mut out_buf2 = vec![0u8; content.len()];
        let read_bytes = unsafe { buffered_read_opt(&mut ctx, &mut out_buf1, out_buf2.len()) };
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
        {
            let mut ccx_options = CCX_OPTIONS.lock().unwrap();
            // Set options to disable buffering.
            ccx_options.buffer_input = false;
            ccx_options.live_stream = Some(Timestamp::from_millis(0));
            ccx_options.input_source = DataSource::File;
            ccx_options.binary_concat = false;
            // TERMINATE_ASAP = false;
        }
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
        ctx.ts_autoprogram = 0;
        ctx.ts_allprogram = 0;
        ctx.flag_ts_forced_pn = 0;
        ctx.flag_ts_forced_cappid = 0;
        ctx.ts_datastreamtype = 0;
        ctx.pinfo = vec![];
        ctx.nb_program = 0;
        ctx.codec = Codec::Dvb;
        ctx.nocodec = Codec::Dvb;
        ctx.cinfo_tree = CapInfo::default();
        ctx.global_timestamp = 0;
        ctx.min_global_timestamp = 0;
        ctx.offset_global_timestamp = 0;
        ctx.last_global_timestamp = 0;
        ctx.global_timestamp_inited = 0;
        // unsafe { ctx.parent = *ptr::null_mut(); }

        let mut out_buf1 = vec![0u8; content.len()];
        let mut out_buf2 = vec![0u8; content.len()];
        let read_bytes = unsafe { buffered_read_opt(&mut ctx, &mut out_buf1, out_buf2.len()) };
        assert_eq!(read_bytes, content.len());
        assert_eq!(&out_buf1, content);
    }
    #[test]
    #[serial]
    fn test_buffered_read_opt_empty_file() {
        initialize_logger();
        {
            let mut opts = CCX_OPTIONS.lock().unwrap();
            // Use buffering.
            opts.buffer_input = true;
            opts.live_stream = Some(Timestamp::from_millis(0));
            opts.input_source = DataSource::File;
            opts.binary_concat = false;
        }
        // Create an empty temporary file.
        let content: &[u8] = b"";
        let fd = create_temp_file_with_content(content); // file pointer will be at beginning
                                                         // Allocate a filebuffer.
        let filebuffer = allocate_filebuffer();
        let mut ctx = CcxDemuxer::default();
        ctx.infd = fd;
        ctx.past = 0;
        ctx.filebuffer = filebuffer;
        ctx.filebuffer_start = 0;
        ctx.filebuffer_pos = 0;
        ctx.bytesinbuffer = 0;
        // (Other fields can remain default.)

        // Prepare an output buffer with the same length as content (i.e. zero length).
        let mut out_buf1 = vec![0u8; content.len()];
        let mut out_buf2 = vec![0u8; content.len()];
        let read_bytes = unsafe { buffered_read_opt(&mut ctx, &mut out_buf1, out_buf2.len()) };
        assert_eq!(read_bytes, 0);
        assert_eq!(&out_buf1, content);

        // Clean up allocated filebuffer.
        unsafe {
            let _ = Box::from_raw(filebuffer);
        };
    }

    #[test]
    #[serial]
    fn test_buffered_read_opt_seek_without_buffer() {
        initialize_logger();
        {
            let mut opts = CCX_OPTIONS.lock().unwrap();
            // Disable buffering.
            opts.buffer_input = false;
            opts.live_stream = Some(Timestamp::from_millis(0));
            opts.input_source = DataSource::File;
            opts.binary_concat = false;
        }
        // Create a file with some content.
        let content = b"Content for seek branch";
        let fd = create_temp_file_with_content(content);
        // For this test we simulate the "seek without a buffer" branch by passing an empty output slice.
        let mut ctx = CcxDemuxer::default();
        ctx.infd = fd;
        ctx.past = 0;
        // In this branch, the filebuffer is not used.
        ctx.filebuffer = ptr::null_mut();
        ctx.filebuffer_start = 0;
        ctx.filebuffer_pos = 0;
        ctx.bytesinbuffer = 0;

        // Pass an empty buffer so that the branch that checks `if !buffer.is_empty()` fails.
        let mut out_buf1 = vec![0u8; 0];
        let mut out_buf2 = [0u8; 0];
        let read_bytes = unsafe { buffered_read_opt(&mut ctx, &mut out_buf1, out_buf2.len()) };
        // Expect that no bytes can be read into an empty slice.
        assert_eq!(read_bytes, 0);
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
            let _ = Box::from_raw(slice::from_raw_parts_mut(ctx.filebuffer, FILEBUFFERSIZE));
        };
    }

    // Test 2: When filebuffer_pos > 0 (discarding old bytes).
    #[test]
    fn test_return_to_buffer_discard_old_bytes() {
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
            assert_eq!(&out[2..], &[]);
        }
        // Clean up.
        unsafe {
            let _ = Box::from_raw(slice::from_raw_parts_mut(ctx.filebuffer, FILEBUFFERSIZE));
        };
    }

    // Test 3: Normal case: no filebuffer_pos; simply copy incoming data.
    #[test]
    fn test_return_to_buffer_normal() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        // Set filebuffer_pos to 0 and bytesinbuffer to some existing data.
        ctx.filebuffer_pos = 0;
        ctx.bytesinbuffer = 4;
        // Pre-fill the filebuffer with some data.
        unsafe {
            for i in 0..4 {
                *ctx.filebuffer.add(i) = (i + 10) as u8; // 10,11,12,13
            }
        }
        // Now call return_to_buffer with an input of 3 bytes.
        let input = [0x77, 0x88, 0x99];
        return_to_buffer(&mut ctx, &input, 3);
        // Expected behavior:
        // - Since filebuffer_pos == 0, it skips the first if blocks.
        // - It checks that (bytesinbuffer + bytes) does not exceed FILEBUFFERSIZE.
        // - It then shifts the existing data right by 3 bytes.
        // - It copies the new input to the front.
        // - It increments bytesinbuffer by 3 (so now 7 bytes).
        unsafe {
            let out = slice::from_raw_parts(ctx.filebuffer, ctx.bytesinbuffer as usize);
            // First 3 bytes should equal input.
            assert_eq!(&out[0..3], &input);
            // Next 4 bytes should be the old data.
            let expected = &[10u8, 11, 12, 13];
            assert_eq!(&out[3..7], expected);
        }
        // Clean up.
        unsafe {
            let _ = Box::from_raw(slice::from_raw_parts_mut(ctx.filebuffer, FILEBUFFERSIZE));
        };
    }
    //buffered_read tests
    // Helper: create a dummy CcxDemuxer with a preallocated filebuffer.

    // Test 1: Direct branch - when requested bytes <= available in filebuffer.
    #[test]
    fn test_buffered_read_direct_branch() {
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
        let read_bytes = unsafe { buffered_read(&mut ctx, Some(&mut out_buf), data.len()) };
        assert_eq!(read_bytes, data.len());
        assert_eq!(&out_buf, data);
        // filebuffer_pos should be advanced.
        assert_eq!(ctx.filebuffer_pos, data_len);
        // Clean up.
        unsafe {
            let _ = Box::from_raw(slice::from_raw_parts_mut(ctx.filebuffer, FILEBUFFERSIZE));
        };
    }

    #[test]
    #[serial]
    fn test_buffered_read_buffered_opt_branch() {
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
        let read_bytes = unsafe { buffered_read(&mut ctx, Some(&mut out_buf), req) };
        // Expect that the file content is read.
        assert_eq!(read_bytes, content.len());
        assert_eq!(&out_buf, content);
        unsafe {
            let _ = Box::from_raw(slice::from_raw_parts_mut(ctx.filebuffer, FILEBUFFERSIZE));
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
        {
            let mut opts = CCX_OPTIONS.lock().unwrap();
            opts.gui_mode_reports = true;
            opts.input_source = DataSource::Network;
        }
        unsafe {
            NET_ACTIVITY_GUI.store(0, Ordering::SeqCst);
        }
        let read_bytes = unsafe { buffered_read(&mut ctx, Some(&mut out_buf), req) };
        // Expect that the file content is read.
        assert_eq!(read_bytes, content.len());
        assert_eq!(&out_buf, content);
        // Check that NET_ACTIVITY_GUI has been incremented.
        unsafe {
            let _ = Box::from_raw(slice::from_raw_parts_mut(ctx.filebuffer, FILEBUFFERSIZE));
        };
    }
    // Tests for buffered_read_byte

    // Test 1: When available data exists in the filebuffer and a valid buffer is provided.
    #[test]
    fn test_buffered_read_byte_available() {
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
        let read = unsafe { buffered_read_byte(&mut ctx as *mut CcxDemuxer, Some(&mut out_byte)) };
        // Expect to read 1 byte, which should be data[1] = 0xBB.
        assert_eq!(read, 1);
        assert_eq!(out_byte, 0xBB);
        // filebuffer_pos should have advanced.
        assert_eq!(ctx.filebuffer_pos, 2);
    }

    // Test 2: When available data exists but buffer is None.
    #[test]
    fn test_buffered_read_byte_available_none() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        let data = b"\x11\x22";
        unsafe {
            let fb = slice::from_raw_parts_mut(ctx.filebuffer, data.len());
            fb.copy_from_slice(data);
        }
        ctx.bytesinbuffer = data.len() as u32;
        ctx.filebuffer_pos = 0;
        // Call with None; expect it returns 0 since nothing is copied.
        let read = unsafe { buffered_read_byte(&mut ctx as *mut CcxDemuxer, None) };
        assert_eq!(read, 0);
        // filebuffer_pos remains unchanged.
        assert_eq!(ctx.filebuffer_pos, 0);
    }

    // Test 3: When no available data in filebuffer, forcing call to buffered_read_opt.
    #[test]
    #[serial]
    fn test_buffered_read_byte_no_available() {
        #[allow(unused_variables)]
        let ctx = create_ccx_demuxer_with_buffer();
        let content = b"a";
        let fd = create_temp_file_with_content(content);
        let mut ctx = create_ccx_demuxer_with_buffer();
        ctx.infd = fd;
        ctx.past = 0;
        // Set bytesinbuffer to 0 to force the else branch.

        // Set bytesinbuffer to equal filebuffer_pos so that no data is available.
        ctx.bytesinbuffer = 10;
        ctx.filebuffer_pos = 10;
        let mut out_byte: u8 = 0;
        // Our dummy buffered_read_opt returns 1 and writes 0xAA.
        let read = unsafe { buffered_read_byte(&mut ctx as *mut CcxDemuxer, Some(&mut out_byte)) };
        assert_eq!(read, 1);
        assert_eq!(out_byte, 0);
    }

    // Tests for buffered_get_be16

    // Test 4: When filebuffer has at least 2 available bytes.
    #[test]
    fn test_buffered_get_be16_from_buffer() {
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
        let value = unsafe { buffered_get_be16(&mut ctx as *mut CcxDemuxer) };
        // Expect 0x1234.
        assert_eq!(value, 0x1234);
        // past should have been incremented twice.
        assert_eq!(ctx.past, 2);
        // filebuffer_pos should have advanced by 2.
        assert_eq!(ctx.filebuffer_pos, 2);
    }

    // Test 5: When filebuffer is empty, forcing buffered_read_opt for each byte.
    #[test]
    #[serial]
    fn test_buffered_get_be16_from_opt() {
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
        let value = unsafe { buffered_get_be16(&mut ctx as *mut CcxDemuxer) };
        // Expect the two bytes to be 0xAA each, so 0xAAAA.
        assert_eq!(value, 0);
        // past should have been incremented by 2.
        assert_eq!(ctx.past, 2);
    }
    //Tests for buffered_get_byte
    #[test]
    fn test_buffered_get_byte_available() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        // Fill filebuffer with one byte: 0x12.
        let data = [0x12u8];
        unsafe {
            let fb = slice::from_raw_parts_mut(ctx.filebuffer, 1);
            fb.copy_from_slice(&data);
        }
        ctx.bytesinbuffer = 1;
        ctx.filebuffer_pos = 0;
        ctx.past = 0;
        let value = unsafe { buffered_get_byte(&mut ctx as *mut CcxDemuxer) };
        // Expect 0x12.
        assert_eq!(value, 0x12);
        // past should have been incremented.
        assert_eq!(ctx.past, 1);
        // filebuffer_pos should have advanced by 1.
        assert_eq!(ctx.filebuffer_pos, 1);
    }
    #[test]
    #[serial]
    fn test_buffered_get_byte_no_available() {
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
        let value = unsafe { buffered_get_byte(&mut ctx as *mut CcxDemuxer) };
        // Expect the byte to be 0xAA.
        assert_eq!(value, 0);
        // past should have been incremented.
        assert_eq!(ctx.past, 0);
    }

    #[test]
    fn test_buffered_get_be32_from_buffer() {
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
        let value = unsafe { buffered_get_be32(&mut ctx as *mut CcxDemuxer) };
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
        let value = unsafe { buffered_get_be32(&mut ctx as *mut CcxDemuxer) };
        // Expect the four bytes to be 0xAAAAAAAA.
        assert_eq!(value, 0);
        // past should have been incremented by 4.
        assert_eq!(ctx.past, 4);
    }

    //Tests for buffered_get_le16
    #[test]
    fn test_buffered_get_le16_from_buffer() {
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
        let value = unsafe { buffered_get_le16(&mut ctx as *mut CcxDemuxer) };
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
        // In this case, buffered_read_opt (our dummy version) will supply 0xAA for each byte.
        let value = unsafe { buffered_get_le16(&mut ctx as *mut CcxDemuxer) };
        // Expect the two bytes to be 0xAAAA.
        assert_eq!(value, 0);
        // past should have been incremented by 2.
        assert_eq!(ctx.past, 2);
    }

    //Tests for buffered_get_le32
    #[test]
    fn test_buffered_get_le32_from_buffer() {
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
        let value = unsafe { buffered_get_le32(&mut ctx as *mut CcxDemuxer) };
        // Expect 0x78563412.
        assert_eq!(value, 0x78563412);
        // past should have been incremented by 4.
        assert_eq!(ctx.past, 4);
        // filebuffer_pos should have advanced by 4.
        assert_eq!(ctx.filebuffer_pos, 4);
    }
    #[test]
    fn test_buffered_get_le16_null_ctx() {
        let value = unsafe { buffered_get_le16(ptr::null_mut()) };
        assert_eq!(value, 0);
    }

    #[test]
    #[serial]
    fn test_buffered_get_le32_from_opt() {
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
        let value = unsafe { buffered_get_le32(&mut ctx as *mut CcxDemuxer) };
        // Expect the four bytes to be 0xAAAAAAAA.
        assert_eq!(value, 0);
        // past should have been incremented by 4.
        assert_eq!(ctx.past, 4);
    }
    #[test]
    fn test_buffered_skip_available() {
        let mut ctx = create_ccx_demuxer_with_buffer();
        // Prepopulate filebuffer with dummy data (contents don't matter).
        ctx.bytesinbuffer = 50;
        ctx.filebuffer_pos = 10;
        let skip = 20u32;
        let result = unsafe { buffered_skip(&mut ctx as *mut CcxDemuxer, skip) };
        // Since available = 50 - 10 = 40, and 20 <= 40, buffered_skip should just increment filebuffer_pos.
        assert_eq!(result, 20);
        assert_eq!(ctx.filebuffer_pos, 30);
    }

    // Test 5: When requested bytes > available.
    #[test]
    #[serial]
    fn test_buffered_skip_not_available() {
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
        let result = unsafe { buffered_skip(&mut ctx as *mut CcxDemuxer, skip) };
        assert_eq!(result, 15);
    }

    // Test 6: When ctx is null.
    #[test]
    fn test_buffered_skip_null_ctx() {
        let result = unsafe { buffered_skip(ptr::null_mut(), 10) };
        assert_eq!(result, 0);
    }
}
