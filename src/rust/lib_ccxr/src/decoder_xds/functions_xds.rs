use crate::decoder_xds::structs_xds::*;

use crate::time::c_functions::*;
use crate::time::timing::*;

use crate::util::log::*;

use std::alloc::{realloc, Layout};
use std::ffi::CString;
use std::fmt::Write;
use std::ptr;

/// Rust equivalent for `ccx_decoders_xds_init_library` function in C. Initializes the XDS decoder context.
///
/// # Safety
///
/// The `timing` parameter must be valid and properly initialized. Ensure that all memory allocations succeed.
pub fn ccx_decoders_xds_init_library(
    timing: TimingContext,
    xds_write_to_file: i64,
) -> CcxDecodersXdsContext {
    let mut ctx = CcxDecodersXdsContext {
        current_xds_min: -1,
        current_xds_hour: -1,
        current_xds_date: -1,
        current_xds_month: -1,
        current_program_type_reported: 0,
        xds_start_time_shown: 0,
        xds_program_length_shown: 0,
        xds_program_description: [[0; 33]; 8],
        current_xds_network_name: [0; 33],
        current_xds_program_name: [0; 33],
        current_xds_call_letters: [0; 7],
        current_xds_program_type: [0; 33],
        xds_buffers: [XdsBuffer {
            in_use: 0,
            xds_class: -1,
            xds_type: -1,
            bytes: [0; NUM_BYTES_PER_PACKET as usize],
            used_bytes: 0,
        }; NUM_XDS_BUFFERS as usize],
        cur_xds_buffer_idx: -1,
        cur_xds_packet_class: -1,
        cur_xds_payload: ptr::null_mut(),
        cur_xds_payload_length: 0,
        cur_xds_packet_type: 0,
        timing,
        current_ar_start: 0,
        current_ar_end: 0,
        xds_write_to_file,
    };

    // Initialize xds_buffers
    for buffer in ctx.xds_buffers.iter_mut() {
        buffer.in_use = 0;
        buffer.xds_class = -1;
        buffer.xds_type = -1;
        buffer.used_bytes = 0;
        unsafe {
            ptr::write_bytes(buffer.bytes.as_mut_ptr(), 0, NUM_BYTES_PER_PACKET as usize);
        }
    }

    // Initialize xds_program_description
    for description in ctx.xds_program_description.iter_mut() {
        unsafe {
            ptr::write_bytes(description.as_mut_ptr(), 0, 33);
        }
    }

    // Initialize other fields
    unsafe {
        ptr::write_bytes(ctx.current_xds_network_name.as_mut_ptr(), 0, 33);
        ptr::write_bytes(ctx.current_xds_program_name.as_mut_ptr(), 0, 33);
        ptr::write_bytes(ctx.current_xds_call_letters.as_mut_ptr(), 0, 7);
        ptr::write_bytes(ctx.current_xds_program_type.as_mut_ptr(), 0, 33);
    }

    ctx
}

/// Rust equivalent for `write_xds_string` function in C. Writes XDS string data into the subtitle structure.
///
/// # Safety
///
/// The `sub` and `ctx` pointers must be valid and non-null. Ensure that memory allocations succeed and that `p` points to valid memory.
pub fn write_xds_string(
    sub: &mut CcSubtitle,
    ctx: &mut CcxDecodersXdsContext,
    p: *const u8,
    len: usize,
) -> i32 {
    // Allocate or reallocate memory for `sub.data`
    let new_size = (sub.nb_data as usize + 1) * std::mem::size_of::<Eia608Screen>();
    let layout = Layout::array::<Eia608Screen>(sub.nb_data as usize + 1).unwrap();

    let data = if sub.data.is_null() {
        unsafe { std::alloc::alloc(layout) as *mut Eia608Screen }
    } else {
        unsafe { realloc(sub.data as *mut u8, layout, new_size) as *mut Eia608Screen }
    };

    if data.is_null() {
        sub.data = ptr::null_mut();
        sub.nb_data = 0;
        info!("No Memory left");
        return -1;
    } else {
        sub.data = data as *mut std::ffi::c_void; // the as *mut std::ffi::c_void is necessary to convert from *mut Eia608Screen to *mut std::ffi::c_void
                                                  // couldn't find a way to do this without the as *mut std::ffi::c_void
        sub.datatype = SubDataType::Generic;
        unsafe {
            let data_ptr = (sub.data as *mut Eia608Screen).add(sub.nb_data as usize);
            (*data_ptr).format = CcxEia608Format::SformatXds;
            (*data_ptr).end_time = get_fts(&mut ctx.timing, CaptionField::Field2).millis(); // read millis from Timestamp
                                                                                            // (*data_ptr).end_time = get_fts(&ctx.timing, 2);
            (*data_ptr).xds_str = p;
            (*data_ptr).xds_len = len;
            (*data_ptr).cur_xds_packet_class = ctx.cur_xds_packet_class;
        }
        sub.nb_data += 1;
        sub.subtype = SubType::Cc608;
        sub.got_output = true;
    }

    0
}

/// Rust equivalent for `xdsprint` function in C. Formats and writes XDS data to the subtitle structure.
///
/// # Safety
///
/// The `sub` and `ctx` pointers must be valid and non-null. Ensure that memory allocations succeed and that the formatted string is valid.
pub fn xdsprint(
    sub: &mut CcSubtitle,
    ctx: &mut CcxDecodersXdsContext,
    _fmt: &str, // coz fmt is unused in the current implementation
    args: std::fmt::Arguments,
) {
    if ctx.xds_write_to_file == 0 {
        return;
    }

    // Initial buffer size
    let mut size: usize = 100;
    let mut buffer = String::with_capacity(size);

    loop {
        // Try to write the formatted string into the buffer
        match write!(&mut buffer, "{}", args) {
            Ok(_) => {
                // If successful, write the XDS string
                let c_string = CString::new(buffer.as_bytes()).unwrap();
                write_xds_string(sub, ctx, c_string.as_ptr() as *const u8, buffer.len());
                return;
            }
            Err(_) => {
                // If the buffer is too small, double its size
                size *= 2;
                buffer.reserve(size);
            }
        }
    }
}

/// Rust equivalent for `clear_xds_buffer` function in C. Clears the specified XDS buffer.
///
/// # Safety
///
/// The `num` parameter must be within the valid range of the `xds_buffers` array, and the `ctx` pointer must be valid and non-null.
pub fn clear_xds_buffer(ctx: &mut CcxDecodersXdsContext, num: i64) {
    ctx.xds_buffers[num as usize].in_use = 0;
    ctx.xds_buffers[num as usize].xds_class = -1;
    ctx.xds_buffers[num as usize].xds_type = -1;
    ctx.xds_buffers[num as usize].used_bytes = 0;

    unsafe {
        ptr::write_bytes(
            ctx.xds_buffers[num as usize].bytes.as_mut_ptr(),
            0,
            NUM_BYTES_PER_PACKET as usize,
        );
    }
}

/// Rust equivalent for `how_many_used` function in C. Counts the number of used XDS buffers.
///
/// # Safety
///
/// The `ctx` pointer must be valid and non-null.
pub fn how_many_used(ctx: &CcxDecodersXdsContext) -> i64 {
    let mut count: i64 = 0;
    for buffer in ctx.xds_buffers.iter() {
        if buffer.in_use != 0 {
            count += 1;
        }
    }
    count
}

/// Rust equivalent for `process_xds_bytes` function in C. Processes XDS bytes and updates the context.
///
/// # Safety
///
/// The `ctx` pointer must be valid and non-null. Ensure that `hi` and `lo` values are within valid ranges.
pub fn process_xds_bytes(ctx: &mut CcxDecodersXdsContext, hi: u8, lo: i64) {
    if (0x01..=0x0f).contains(&hi) {
        let xds_class = ((hi - 1) / 2) as i64; // Start codes 1 and 2 are "class type" 0, 3-4 are 2, and so on.
        let is_new = (hi % 2) != 0; // Start codes are even
        info!(
            "XDS Start: {}.{}  Is new: {}  | Class: {} ({:?}), Used buffers: {}",
            hi,
            lo,
            is_new,
            xds_class,
            XDS_CLASSES[xds_class as usize],
            how_many_used(ctx)
        );

        let mut first_free_buf: i64 = -1;
        let mut matching_buf: i64 = -1;

        for i in 0..NUM_XDS_BUFFERS {
            let buffer = &ctx.xds_buffers[i as usize];
            if buffer.in_use != 0 && buffer.xds_class == xds_class && buffer.xds_type == lo {
                matching_buf = i;
                break;
            }
            if first_free_buf == -1 && buffer.in_use == 0 {
                first_free_buf = i;
            }
        }

        // Handle the three possibilities
        if matching_buf == -1 && first_free_buf == -1 {
            info!(
                "Note: All XDS buffers full (bug or suicidal stream). Ignoring this one ({}, {}).",
                xds_class, lo
            );
            ctx.cur_xds_buffer_idx = -1;
            return;
        }

        ctx.cur_xds_buffer_idx = if matching_buf != -1 {
            matching_buf
        } else {
            first_free_buf
        };

        let cur_idx = ctx.cur_xds_buffer_idx as usize;
        let cur_buffer = &mut ctx.xds_buffers[cur_idx];

        if is_new || cur_buffer.in_use == 0 {
            // Discard previous data
            cur_buffer.xds_class = xds_class;
            cur_buffer.xds_type = lo;
            cur_buffer.used_bytes = 0;
            cur_buffer.in_use = 1;

            unsafe {
                ptr::write_bytes(
                    cur_buffer.bytes.as_mut_ptr(),
                    0,
                    NUM_BYTES_PER_PACKET as usize,
                );
            }
        }

        if !is_new {
            // Continue codes aren't added to the packet
            return;
        }
    } else {
        // Informational: 00, or 0x20-0x7F, so 01-0x1f forbidden
        info!(
            "XDS: {:02X}.{:02X} ({}, {})",
            hi, lo, hi as char, lo as u8 as char
        );
        if (hi > 0 && hi <= 0x1f) || (lo > 0 && lo <= 0x1f) {
            info!("Note: Illegal XDS data");
            return;
        }
    }

    let cur_idx = ctx.cur_xds_buffer_idx as usize;
    let cur_buffer = &mut ctx.xds_buffers[cur_idx];

    if cur_buffer.used_bytes <= 32 {
        // Should always happen
        cur_buffer.bytes[cur_buffer.used_bytes as usize] = hi;
        cur_buffer.used_bytes += 1;
        cur_buffer.bytes[cur_buffer.used_bytes as usize] = lo as u8;
        cur_buffer.used_bytes += 1;
        cur_buffer.bytes[cur_buffer.used_bytes as usize] = 0;
    }
}

/// Rust equivalent for `xds_do_copy_generation_management_system` function in C. Processes CGMS (Copy Generation Management System) data.
///
/// # Safety
///
/// The `ctx` pointer must be valid and non-null. Ensure that `c1` and `c2` values are within valid ranges.
pub fn xds_do_copy_generation_management_system(
    sub: &mut CcSubtitle,
    ctx: &mut CcxDecodersXdsContext,
    c1: u8,
    c2: u8,
) {
    static mut LAST_C1: i64 = -1;
    static mut LAST_C2: i64 = -1;

    let mut copy_permitted = String::new();
    let mut aps = String::new();
    let mut rcd = String::new();

    let mut changed = false;

    let c1_6 = (c1 & 0x40) >> 6;
    let cgms_a_b4 = (c1 & 0x10) >> 4;
    let cgms_a_b3 = (c1 & 0x08) >> 3;
    let aps_b2 = (c1 & 0x04) >> 2;
    let aps_b1 = (c1 & 0x02) >> 1;
    let c2_6 = (c2 & 0x40) >> 6;
    let rcd0 = c2 & 0x01;

    // User doesn't need XDS
    if ctx.xds_write_to_file == 0 {
        return;
    }

    // These must be high. If not, not following specs
    if c1_6 == 0 || c2_6 == 0 {
        return;
    }

    unsafe {
        if LAST_C1 != c1 as i64 || LAST_C2 != c2 as i64 {
            changed = true;
            LAST_C1 = c1 as i64;
            LAST_C2 = c2 as i64;

            // Changed since last time, decode
            let copytext = [
                "Copy permitted (no restrictions)",
                "No more copies (one generation copy has been made)",
                "One generation of copies can be made",
                "No copying is permitted",
            ];
            let apstext = [
                "No APS",
                "PSP On; Split Burst Off",
                "PSP On; 2 line Split Burst On",
                "PSP On; 4 line Split Burst On",
            ];

            copy_permitted = format!("CGMS: {}", copytext[(cgms_a_b4 * 2 + cgms_a_b3) as usize]);
            aps = format!("APS: {}", apstext[(aps_b2 * 2 + aps_b1) as usize]);
            rcd = format!("Redistribution Control Descriptor: {}", rcd0);
        }
    }

    xdsprint(sub, ctx, &copy_permitted, format_args!(""));
    xdsprint(sub, ctx, &aps, format_args!(""));
    xdsprint(sub, ctx, &rcd, format_args!(""));

    if changed {
        info!("XDS: {}", copy_permitted);
        info!("XDS: {}", aps);
        info!("XDS: {}", rcd);
    }

    info!("XDS: {}", copy_permitted);
    info!("XDS: {}", aps);
    info!("XDS: {}", rcd);
}

/// Rust equivalent for `xds_do_content_advisory` function in C. Processes content advisory data.
///
/// # Safety
///
/// The `ctx` pointer must be valid and non-null. Ensure that `sub` is properly initialized and points to valid memory.
pub fn xds_do_content_advisory(
    sub: &mut CcSubtitle,
    ctx: &mut CcxDecodersXdsContext,
    c1: u8,
    c2: u8,
) {
    static mut LAST_C1: i64 = -1;
    static mut LAST_C2: i64 = -1;

    let mut age = String::new();
    let mut content = String::new();
    let mut rating = String::new();
    let mut changed = false;

    let c1_6 = (c1 & 0x40) >> 6;
    let da2 = (c1 & 0x20) >> 5;
    let a1 = (c1 & 0x10) >> 4;
    let a0 = (c1 & 0x08) >> 3;
    let r2 = (c1 & 0x04) >> 2;
    let r1 = (c1 & 0x02) >> 1;
    let r0 = c1 & 0x01;
    let c2_6 = (c2 & 0x40) >> 6;
    let fv = (c2 & 0x20) >> 5;
    let s = (c2 & 0x10) >> 4;
    let la3 = (c2 & 0x08) >> 3;
    let g2 = (c2 & 0x04) >> 2;
    let g1 = (c2 & 0x02) >> 1;
    let g0 = c2 & 0x01;
    let mut supported = false;

    // User doesn't need XDS
    if ctx.xds_write_to_file == 0 {
        return;
    }

    // These must be high. If not, not following specs
    if c1_6 == 0 || c2_6 == 0 {
        return;
    }

    unsafe {
        if LAST_C1 != c1 as i64 || LAST_C2 != c2 as i64 {
            changed = true;
            LAST_C1 = c1 as i64;
            LAST_C2 = c2 as i64;

            // Changed since last time, decode
            if a1 == 0 && a0 == 1 {
                // US TV parental guidelines
                let agetext = [
                    "None",
                    "TV-Y (All Children)",
                    "TV-Y7 (Older Children)",
                    "TV-G (General Audience)",
                    "TV-PG (Parental Guidance Suggested)",
                    "TV-14 (Parents Strongly Cautioned)",
                    "TV-MA (Mature Audience Only)",
                    "None",
                ];
                age = format!(
                    "ContentAdvisory: US TV Parental Guidelines. Age Rating: {}",
                    agetext[(g2 * 4 + g1 * 2 + g0) as usize]
                );
                content.clear();
                if g2 == 0 && g1 == 1 && g0 == 0 {
                    // For TV-Y7 (Older children), the Violence bit is "fantasy violence"
                    if fv != 0 {
                        content.push_str("[Fantasy Violence] ");
                    }
                } else {
                    // For all others, is real
                    if fv != 0 {
                        content.push_str("[Violence] ");
                    }
                }
                if s != 0 {
                    content.push_str("[Sexual Situations] ");
                }
                if la3 != 0 {
                    content.push_str("[Adult Language] ");
                }
                if da2 != 0 {
                    content.push_str("[Sexually Suggestive Dialog] ");
                }
                supported = true;
            }
            if a0 == 0 {
                // MPA
                let ratingtext = ["N/A", "G", "PG", "PG-13", "R", "NC-17", "X", "Not Rated"];
                rating = format!(
                    "ContentAdvisory: MPA Rating: {}",
                    ratingtext[(r2 * 4 + r1 * 2 + r0) as usize]
                );
                supported = true;
            }
            if a0 == 1 && a1 == 1 && da2 == 0 && la3 == 0 {
                // Canadian English Language Rating
                let ratingtext = [
                    "Exempt",
                    "Children",
                    "Children eight years and older",
                    "General programming suitable for all audiences",
                    "Parental Guidance",
                    "Viewers 14 years and older",
                    "Adult Programming",
                    "[undefined]",
                ];
                rating = format!(
                    "ContentAdvisory: Canadian English Rating: {}",
                    ratingtext[(g2 * 4 + g1 * 2 + g0) as usize]
                );
                supported = true;
            }
            if a0 == 1 && a1 == 1 && da2 == 1 && la3 == 0 {
                // Canadian French Language Rating
                let ratingtext = [
                    "Exemptes",
                    "General",
                    "General - Not recommended for young children",
                    "This program may not be suitable for children under 13 years of age",
                    "This program is not suitable for children under 16 years of age",
                    "This program is reserved for adults",
                    "[invalid]",
                    "[invalid]",
                ];
                rating = format!(
                    "ContentAdvisory: Canadian French Rating: {}",
                    ratingtext[(g2 * 4 + g1 * 2 + g0) as usize]
                );
                supported = true;
            }
        }
    }

    if a1 == 0 && a0 == 1 {
        // US TV parental guidelines
        xdsprint(sub, ctx, &age, format_args!(""));
        xdsprint(sub, ctx, &content, format_args!(""));
        if changed {
            info!("XDS: {}", age);
            info!("XDS: {}", content);
        }
    }
    if a0 == 0
        || (a0 == 1 && a1 == 1 && da2 == 0 && la3 == 0)
        || (a0 == 1 && a1 == 1 && da2 == 1 && la3 == 0)
    {
        // MPA or Canadian ratings
        xdsprint(sub, ctx, &rating, format_args!(""));
        if changed {
            info!("XDS: {}", rating);
        }
    }

    if changed && !supported {
        info!("XDS: Unsupported ContentAdvisory encoding, please submit sample.");
    }
}

/// Rust equivalent for `xds_do_private_data` function in C. Processes private data in the XDS context.
///
/// # Safety
///
/// The `ctx` pointer must be valid and non-null. Ensure that `cur_xds_payload` is properly initialized and points to valid memory.
pub fn xds_do_private_data(sub: &mut CcSubtitle, ctx: &mut CcxDecodersXdsContext) -> i64 {
    if ctx.xds_write_to_file == 0 {
        return CCX_EINVAL;
    }

    // Allocate a string to hold the formatted private data
    let mut str = String::with_capacity((ctx.cur_xds_payload_length * 3) as usize + 1);

    // Only thing we can do with private data is dump it
    for i in 2..(ctx.cur_xds_payload_length - 1) as usize {
        let byte = unsafe { *ctx.cur_xds_payload.add(i) };
        write!(&mut str, "{:02X} ", byte).unwrap();
    }

    // Print the private data
    xdsprint(sub, ctx, &str, format_args!(""));

    1
}

/// Rust equivalent for `xds_do_misc` function in C. Processes miscellaneous XDS data.
///
/// # Safety
///
/// The `ctx` pointer must be valid and non-null. Ensure that `cur_xds_payload` is properly initialized and points to valid memory.
pub fn xds_do_misc(ctx: &CcxDecodersXdsContext) -> i64 {
    let was_proc: i64;

    if ctx.cur_xds_payload.is_null() {
        return CCX_EINVAL;
    }

    match ctx.cur_xds_packet_type {
        x if x as u8 == XDS_TYPE_TIME_OF_DAY => {
            was_proc = 1;
            if ctx.cur_xds_payload_length < 9 {
                // We need at least 6 data bytes
                return was_proc;
            }

            let min = unsafe { *ctx.cur_xds_payload.add(2) & 0x3f }; // 6 bits
            let hour = unsafe { *ctx.cur_xds_payload.add(3) & 0x1f }; // 5 bits
            let date = unsafe { *ctx.cur_xds_payload.add(4) & 0x1f }; // 5 bits
            let month = unsafe { *ctx.cur_xds_payload.add(5) & 0x0f }; // 4 bits
            let reset_seconds = unsafe { (*ctx.cur_xds_payload.add(5) & 0x20) != 0 }; // Reset seconds
            let day_of_week = unsafe { *ctx.cur_xds_payload.add(6) & 0x07 }; // 3 bits
            let year = unsafe { (*ctx.cur_xds_payload.add(7) & 0x3f) as i64 + 1990 }; // Year offset

            info!(
                "Time of day: (YYYY/MM/DD) {}/{:02}/{:02} (HH:MM) {:02}:{:02} DoW: {} Reset seconds: {}",
                year, month, date, hour, min, day_of_week, reset_seconds
            );
        }
        x if x as u8 == XDS_TYPE_LOCAL_TIME_ZONE => {
            was_proc = 1;
            if ctx.cur_xds_payload_length < 5 {
                // We need at least 2 data bytes
                return was_proc;
            }

            let dst = unsafe { (*ctx.cur_xds_payload.add(2) & 0x20) >> 5 }; // Daylight Saving Time
            let hour = unsafe { *ctx.cur_xds_payload.add(2) & 0x1f }; // 5 bits

            info!("Local Time Zone: {:02} DST: {}", hour, dst);
        }
        _ => {
            was_proc = 0;
        }
    }

    was_proc
}

/// Rust equivalent for `xds_do_current_and_future` function in C. Processes XDS data for current and future programs.
///
/// # Safety
///
/// The `ctx` pointer must be valid and non-null. Ensure that `cur_xds_payload` is properly initialized and points to valid memory.
pub fn xds_do_current_and_future(sub: &mut CcSubtitle, ctx: &mut CcxDecodersXdsContext) -> i64 {
    let mut was_proc: i64 = 0;

    if ctx.cur_xds_payload.is_null() {
        return CCX_EINVAL;
    }

    match ctx.cur_xds_packet_type as u8 {
        XDS_TYPE_PIN_START_TIME => {
            was_proc = 1;
            if ctx.cur_xds_payload_length < 7 {
                return was_proc;
            }

            let min = unsafe { *ctx.cur_xds_payload.add(2) & 0x3f };
            let hour = unsafe { *ctx.cur_xds_payload.add(3) & 0x1f };
            let date = unsafe { *ctx.cur_xds_payload.add(4) & 0x1f };
            let month = unsafe { *ctx.cur_xds_payload.add(5) & 0x0f };

            if ctx.current_xds_min != min as i64
                || ctx.current_xds_hour != hour as i64
                || ctx.current_xds_date != date as i64
                || ctx.current_xds_month != month as i64
            {
                ctx.xds_start_time_shown = 0;
                ctx.current_xds_min = min as i64;
                ctx.current_xds_hour = hour as i64;
                ctx.current_xds_date = date as i64;
                ctx.current_xds_month = month as i64;
            }

            info!(
                "PIN (Start Time): {}  {:02}-{:02} {:02}:{:02}",
                if ctx.cur_xds_packet_class == XDS_CLASS_CURRENT as i64 {
                    "Current"
                } else {
                    "Future"
                },
                date,
                month,
                hour,
                min
            );

            xdsprint(
                sub,
                ctx,
                &format!(
                    "PIN (Start Time): {}  {:02}-{:02} {:02}:{:02}",
                    if ctx.cur_xds_packet_class == XDS_CLASS_CURRENT as i64 {
                        "Current"
                    } else {
                        "Future"
                    },
                    date,
                    month,
                    hour,
                    min
                ),
                format_args!(""),
            );

            if ctx.xds_start_time_shown == 0 && ctx.cur_xds_packet_class == XDS_CLASS_CURRENT as i64
            {
                info!("XDS: Program changed.");
                info!(
                    "XDS program start time (DD/MM HH:MM) {:02}-{:02} {:02}:{:02}",
                    date, month, hour, min
                );
                ctx.xds_start_time_shown = 1;
            }
        }
        XDS_TYPE_LENGTH_AND_CURRENT_TIME => {
            was_proc = 1;
            if ctx.cur_xds_payload_length < 5 {
                return was_proc;
            }

            let min = unsafe { *ctx.cur_xds_payload.add(2) & 0x3f };
            let hour = unsafe { *ctx.cur_xds_payload.add(3) & 0x1f };

            info!("XDS: Program length (HH:MM): {:02}:{:02}", hour, min);
            xdsprint(
                sub,
                ctx,
                &format!("Program length (HH:MM): {:02}:{:02}", hour, min),
                format_args!(""),
            );

            if ctx.cur_xds_payload_length > 6 {
                let el_min = unsafe { *ctx.cur_xds_payload.add(4) & 0x3f };
                let el_hour = unsafe { *ctx.cur_xds_payload.add(5) & 0x1f };
                info!("Elapsed (HH:MM): {:02}:{:02}", el_hour, el_min);
                xdsprint(
                    sub,
                    ctx,
                    &format!("Elapsed (HH:MM): {:02}:{:02}", el_hour, el_min),
                    format_args!(""),
                );
            }

            if ctx.cur_xds_payload_length > 8 {
                let el_sec = unsafe { *ctx.cur_xds_payload.add(6) & 0x3f };
                info!("Elapsed (SS): {:02}", el_sec);
                xdsprint(
                    sub,
                    ctx,
                    &format!("Elapsed (SS): {:02}", el_sec),
                    format_args!(""),
                );
            }
        }
        XDS_TYPE_PROGRAM_NAME => {
            was_proc = 1;
            let mut xds_program_name = String::new();
            for i in 2..(ctx.cur_xds_payload_length - 1) as usize {
                let byte = unsafe { *ctx.cur_xds_payload.add(i) };
                xds_program_name.push(byte as char);
            }

            info!("XDS Program name: {}", xds_program_name);
            xdsprint(
                sub,
                ctx,
                &format!("Program name: {}", xds_program_name),
                format_args!(""),
            );

            if ctx.cur_xds_packet_class == XDS_CLASS_CURRENT as i64
                && xds_program_name != String::from_utf8_lossy(&ctx.current_xds_program_name)
            {
                info!("XDS Notice: Program is now {}", xds_program_name);
                unsafe {
                    ptr::write_bytes(
                        ctx.current_xds_program_name.as_mut_ptr(),
                        0,
                        ctx.current_xds_program_name.len(),
                    );
                }
                ctx.current_xds_program_name[..xds_program_name.len()]
                    .copy_from_slice(xds_program_name.as_bytes());
            }
        }
        XDS_TYPE_PROGRAM_TYPE => {
            was_proc = 1;
            if ctx.cur_xds_payload_length < 5 {
                return was_proc;
            }

            if ctx.current_program_type_reported != 0 {
                for i in 0..ctx.cur_xds_payload_length as usize {
                    if unsafe { *ctx.cur_xds_payload.add(i) } != ctx.current_xds_program_type[i] {
                        ctx.current_program_type_reported = 0;
                        break;
                    }
                }
            }

            unsafe {
                ptr::write_bytes(
                    ctx.current_xds_program_type.as_mut_ptr(),
                    0,
                    ctx.current_xds_program_type.len(),
                );
            }
            for i in 0..ctx.cur_xds_payload_length as usize {
                ctx.current_xds_program_type[i] = unsafe { *ctx.cur_xds_payload.add(i) };
            }

            let mut program_type = String::new();
            for i in 2..(ctx.cur_xds_payload_length - 1) as usize {
                let byte = unsafe { *ctx.cur_xds_payload.add(i) };
                if (0x20..0x7f).contains(&byte) {
                    program_type.push_str(XDS_PROGRAM_TYPES[(byte - 0x20) as usize]);
                }
            }

            info!("XDS Program Type: {}", program_type);
            xdsprint(
                sub,
                ctx,
                &format!("Program type {}", program_type),
                format_args!(""),
            );

            ctx.current_program_type_reported = 1;
        }
        XDS_TYPE_CONTENT_ADVISORY => {
            was_proc = 1;
            if ctx.cur_xds_payload_length < 5 {
                return was_proc;
            }
            xds_do_content_advisory(sub, ctx, unsafe { *ctx.cur_xds_payload.add(2) }, unsafe {
                *ctx.cur_xds_payload.add(3)
            });
        }
        XDS_TYPE_CGMS => {
            was_proc = 1;
            xds_do_copy_generation_management_system(
                sub,
                ctx,
                unsafe { *ctx.cur_xds_payload.add(2) },
                unsafe { *ctx.cur_xds_payload.add(3) },
            );
        }
        XDS_TYPE_ASPECT_RATIO_INFO => {
            was_proc = 1;
            if ctx.cur_xds_payload_length < 5 {
                return was_proc;
            }

            let ar_start = (unsafe { *ctx.cur_xds_payload.add(2) } & 0x1f) as i64 + 22;
            let ar_end = 262 - (unsafe { *ctx.cur_xds_payload.add(3) } & 0x1f) as i64;
            let active_picture_height = ar_end - ar_start;
            let aspect_ratio = 320.0 / active_picture_height as f64;

            if ar_start != ctx.current_ar_start {
                ctx.current_ar_start = ar_start;
                ctx.current_ar_end = ar_end;
                info!(
                    "XDS Notice: Aspect ratio info, start line={}, end line={}",
                    ar_start, ar_end
                );
                info!(
                    "XDS Notice: Aspect ratio info, active picture height={}, ratio={}",
                    active_picture_height, aspect_ratio
                );
            }
        }
        XDS_TYPE_PROGRAM_DESC_1..=XDS_TYPE_PROGRAM_DESC_8 => {
            was_proc = 1;
            let mut xds_desc = String::new();
            for i in 2..(ctx.cur_xds_payload_length - 1) as usize {
                let byte = unsafe { *ctx.cur_xds_payload.add(i) };
                xds_desc.push(byte as char);
            }

            if !xds_desc.is_empty() {
                let line_num = ctx.cur_xds_packet_type as usize - XDS_TYPE_PROGRAM_DESC_1 as usize;
                if xds_desc != String::from_utf8_lossy(&ctx.xds_program_description[line_num]) {
                    info!("XDS description line {}: {}", line_num, xds_desc);
                    unsafe {
                        ptr::write_bytes(
                            ctx.xds_program_description[line_num].as_mut_ptr(),
                            0,
                            ctx.xds_program_description[line_num].len(),
                        );
                    }
                    ctx.xds_program_description[line_num][..xds_desc.len()]
                        .copy_from_slice(xds_desc.as_bytes());
                }
                xdsprint(
                    sub,
                    ctx,
                    &format!("XDS description line {}: {}", line_num, xds_desc),
                    format_args!(""),
                );
            }
        }
        _ => {}
    }

    was_proc
}

/// Rust equivalent for `do_end_of_xds` function in C. Processes the end of an XDS packet and handles its contents.
///
/// # Safety
///
/// The `ctx` pointer must be valid and non-null. Ensure that `cur_xds_payload` is properly initialized and points to valid memory.
pub fn do_end_of_xds(
    sub: &mut CcSubtitle,
    ctx: &mut CcxDecodersXdsContext,
    expected_checksum: i64,
) {
    let mut cs: i64 = 0;

    if ctx.cur_xds_buffer_idx == -1 || ctx.xds_buffers[ctx.cur_xds_buffer_idx as usize].in_use == 0
    {
        return;
    }

    ctx.cur_xds_packet_class = ctx.xds_buffers[ctx.cur_xds_buffer_idx as usize].xds_class;
    ctx.cur_xds_payload = ctx.xds_buffers[ctx.cur_xds_buffer_idx as usize]
        .bytes
        .as_mut_ptr();
    ctx.cur_xds_payload_length = ctx.xds_buffers[ctx.cur_xds_buffer_idx as usize].used_bytes;
    ctx.cur_xds_packet_type = unsafe { *ctx.cur_xds_payload.add(1) as i64 };
    unsafe {
        *ctx.cur_xds_payload.add(ctx.cur_xds_payload_length as usize) = 0x0F;
    }
    ctx.cur_xds_payload_length += 1;

    for i in 0..ctx.cur_xds_payload_length as usize {
        let byte = unsafe { *ctx.cur_xds_payload.add(i) };
        cs = (cs + byte as i64) & 0x7F;
        let c = byte & 0x7F;
        info!(
            "{:02X} - {} cs: {:02X}",
            c,
            if c >= 0x20 { c as char } else { '?' },
            cs
        );
    }
    cs = (128 - cs) & 0x7F;

    info!(
        "End of XDS. Class={} ({}), size={}  Checksum OK: {}   Used buffers: {}",
        ctx.cur_xds_packet_class,
        XDS_CLASSES[ctx.cur_xds_packet_class as usize],
        ctx.cur_xds_payload_length,
        cs == expected_checksum,
        how_many_used(ctx)
    );

    if cs != expected_checksum || ctx.cur_xds_payload_length < 3 {
        info!(
            "Expected checksum: {:02X}  Calculated: {:02X}",
            expected_checksum, cs
        );
        clear_xds_buffer(ctx, ctx.cur_xds_buffer_idx);
        return;
    }

    let mut was_proc: i64 = 0;
    if ctx.cur_xds_packet_type & 0x40 != 0 {
        ctx.cur_xds_packet_class = XDS_CLASS_OUT_OF_BAND as i64;
    }

    match ctx.cur_xds_packet_class {
        x if x == XDS_CLASS_FUTURE as i64 => {
            was_proc = 1;
        }
        x if x == XDS_CLASS_CURRENT as i64 => {
            was_proc = xds_do_current_and_future(sub, ctx);
        }
        x if x == XDS_CLASS_CHANNEL as i64 => {
            was_proc = xds_do_channel(sub, ctx);
        }
        x if x == XDS_CLASS_MISC as i64 => {
            was_proc = xds_do_misc(ctx);
        }
        x if x == XDS_CLASS_PRIVATE as i64 => {
            was_proc = xds_do_private_data(sub, ctx);
        }
        x if x == XDS_CLASS_OUT_OF_BAND as i64 => {
            info!("Out-of-band data, ignored.");
            was_proc = 1;
        }
        _ => {}
    }

    if was_proc == 0 {
        info!("Note: We found a currently unsupported XDS packet.");
        hex_dump_with_start_idx(
            DebugMessageFlag::DECODER_XDS,
            unsafe {
                std::slice::from_raw_parts(ctx.cur_xds_payload, ctx.cur_xds_payload_length as usize)
            },
            false,
            0,
        );
    }

    clear_xds_buffer(ctx, ctx.cur_xds_buffer_idx);
}

/// Rust equivalent for `xds_do_channel` function in C. Processes XDS data for channel-related information.
///
/// # Safety
///
/// The `ctx` pointer must be valid and non-null. Ensure that `cur_xds_payload` is properly initialized and points to valid memory.
pub fn xds_do_channel(sub: &mut CcSubtitle, ctx: &mut CcxDecodersXdsContext) -> i64 {
    let mut was_proc: i64 = 0;

    if ctx.cur_xds_payload.is_null() {
        return CCX_EINVAL;
    }

    match ctx.cur_xds_packet_type as u8 {
        XDS_TYPE_NETWORK_NAME => {
            was_proc = 1;
            let mut xds_network_name = String::new();
            for i in 2..(ctx.cur_xds_payload_length - 1) as usize {
                let byte = unsafe { *ctx.cur_xds_payload.add(i) };
                xds_network_name.push(byte as char);
            }

            info!("XDS Network name: {}", xds_network_name);
            xdsprint(
                sub,
                ctx,
                &format!("Network: {}", xds_network_name),
                format_args!(""),
            );

            if xds_network_name != String::from_utf8_lossy(&ctx.current_xds_network_name) {
                info!("XDS Notice: Network is now {}", xds_network_name);
                unsafe {
                    ptr::write_bytes(
                        ctx.current_xds_network_name.as_mut_ptr(),
                        0,
                        ctx.current_xds_network_name.len(),
                    );
                }
                ctx.current_xds_network_name[..xds_network_name.len()]
                    .copy_from_slice(xds_network_name.as_bytes());
            }
        }
        XDS_TYPE_CALL_LETTERS_AND_CHANNEL => {
            was_proc = 1;
            if ctx.cur_xds_payload_length != 7 && ctx.cur_xds_payload_length != 9 {
                return was_proc;
            }

            let mut xds_call_letters = String::new();
            for i in 2..(ctx.cur_xds_payload_length - 1) as usize {
                let byte = unsafe { *ctx.cur_xds_payload.add(i) };
                xds_call_letters.push(byte as char);
            }

            info!("XDS Network call letters: {}", xds_call_letters);
            xdsprint(
                sub,
                ctx,
                &format!("Call Letters: {}", xds_call_letters),
                format_args!(""),
            );

            if xds_call_letters != String::from_utf8_lossy(&ctx.current_xds_call_letters) {
                info!("XDS Notice: Network call letters now {}", xds_call_letters);
                unsafe {
                    ptr::write_bytes(
                        ctx.current_xds_call_letters.as_mut_ptr(),
                        0,
                        ctx.current_xds_call_letters.len(),
                    );
                }
                ctx.current_xds_call_letters[..xds_call_letters.len()]
                    .copy_from_slice(xds_call_letters.as_bytes());
            }
        }
        XDS_TYPE_TSID => {
            was_proc = 1;
            if ctx.cur_xds_payload_length < 7 {
                return was_proc;
            }

            let b1 = (unsafe { *ctx.cur_xds_payload.add(2) } & 0x10) as i64;
            let b2 = (unsafe { *ctx.cur_xds_payload.add(3) } & 0x10) as i64;
            let b3 = (unsafe { *ctx.cur_xds_payload.add(4) } & 0x10) as i64;
            let b4 = (unsafe { *ctx.cur_xds_payload.add(5) } & 0x10) as i64;
            let tsid = (b4 << 12) | (b3 << 8) | (b2 << 4) | b1;

            if tsid != 0 {
                xdsprint(sub, ctx, &format!("TSID: {}", tsid), format_args!(""));
            }
        }
        _ => {}
    }

    was_proc
}

/// Rust equivalent for `xds_debug_test` function in C. Tests the XDS processing logic with sample data.
///
/// # Safety
///
/// The `ctx` pointer must be valid and non-null. Ensure that `sub` is properly initialized and points to valid memory
pub fn xds_debug_test(ctx: &mut CcxDecodersXdsContext, sub: &mut CcSubtitle) {
    process_xds_bytes(ctx, 0x05, 0x02);
    process_xds_bytes(ctx, 0x20, 0x20);
    do_end_of_xds(sub, ctx, 0x2a);
}

/// Rust equivalent for `xds_cea608_test` function in C. Tests the XDS processing logic with CEA-608 sample data.
///
/// # Safety
///
/// The `ctx` pointer must be valid and non-null. Ensure that `sub` is properly initialized and points to valid memory.
pub fn xds_cea608_test(ctx: &mut CcxDecodersXdsContext, sub: &mut CcSubtitle) {
    // This test is the sample data that comes in CEA-608. It sets the program name
    // to be "Star Trek". The checksum is 0x1d and the validation must succeed.
    process_xds_bytes(ctx, 0x01, 0x03);
    process_xds_bytes(ctx, 0x53, 0x74);
    process_xds_bytes(ctx, 0x61, 0x72);
    process_xds_bytes(ctx, 0x20, 0x54);
    process_xds_bytes(ctx, 0x72, 0x65);
    process_xds_bytes(ctx, 0x02, 0x03);
    process_xds_bytes(ctx, 0x02, 0x03);
    process_xds_bytes(ctx, 0x6b, 0x00);
    do_end_of_xds(sub, ctx, 0x1d);
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::ffi::CStr;

    #[test]
    fn test_ccx_decoders_xds_init_library_initialization() {
        let timing = TimingContext::new(); // Assuming `TimingContext::new()` initializes a valid context
        let xds_write_to_file = 1;

        let ctx = ccx_decoders_xds_init_library(timing, xds_write_to_file);

        // Check initial values
        assert_eq!(ctx.current_xds_min, -1);
        assert_eq!(ctx.current_xds_hour, -1);
        assert_eq!(ctx.current_xds_date, -1);
        assert_eq!(ctx.current_xds_month, -1);
        assert_eq!(ctx.current_program_type_reported, 0);
        assert_eq!(ctx.xds_start_time_shown, 0);
        assert_eq!(ctx.xds_program_length_shown, 0);
        assert_eq!(ctx.cur_xds_buffer_idx, -1);
        assert_eq!(ctx.cur_xds_packet_class, -1);
        assert_eq!(ctx.cur_xds_payload, std::ptr::null_mut());
        assert_eq!(ctx.cur_xds_payload_length, 0);
        assert_eq!(ctx.cur_xds_packet_type, 0);
        assert_eq!(ctx.xds_write_to_file, xds_write_to_file);

        // Check that all buffers are initialized correctly
        for buffer in ctx.xds_buffers.iter() {
            assert_eq!(buffer.in_use, 0);
            assert_eq!(buffer.xds_class, -1);
            assert_eq!(buffer.xds_type, -1);
            assert_eq!(buffer.used_bytes, 0);
            assert!(buffer.bytes.iter().all(|&b| b == 0));
        }

        // Check that all program descriptions are initialized to zero
        for description in ctx.xds_program_description.iter() {
            assert!(description.iter().all(|&b| b == 0));
        }

        // Check that network name, program name, call letters, and program type are initialized to zero
        assert!(ctx.current_xds_network_name.iter().all(|&b| b == 0));
        assert!(ctx.current_xds_program_name.iter().all(|&b| b == 0));
        assert!(ctx.current_xds_call_letters.iter().all(|&b| b == 0));
        assert!(ctx.current_xds_program_type.iter().all(|&b| b == 0));
    }

    #[test]
    fn test_ccx_decoders_xds_init_library_with_different_write_flag() {
        let timing = TimingContext::new(); // Assuming `TimingContext::new()` initializes a valid context
        let xds_write_to_file = 0;

        let ctx = ccx_decoders_xds_init_library(timing, xds_write_to_file);

        // Check that the `xds_write_to_file` flag is set correctly
        assert_eq!(ctx.xds_write_to_file, xds_write_to_file);
    }

    #[test]
    fn test_ccx_decoders_xds_init_library_timing_context() {
        let timing = TimingContext::new(); // Assuming `TimingContext::new()` initializes a valid context
        let xds_write_to_file = 1;

        let ctx = ccx_decoders_xds_init_library(timing.clone(), xds_write_to_file);

        // Check that the timing context is correctly assigned
        assert_eq!(ctx.timing, timing);
    }

    #[test]
    fn test_write_xds_string_success() {
        let mut sub = CcSubtitle {
            data: std::ptr::null_mut(),
            datatype: SubDataType::Generic,
            nb_data: 0,
            subtype: SubType::Cc608,
            enc_type: CcxEncodingType::Utf8,
            start_time: 0,
            end_time: 0,
            flags: 0,
            lang_index: 0,
            got_output: false,
            mode: [0; 5],
            info: [0; 4],
            time_out: 0,
            next: std::ptr::null_mut(),
            prev: std::ptr::null_mut(),
        };

        let mut ctx = CcxDecodersXdsContext {
            current_xds_min: -1,
            current_xds_hour: -1,
            current_xds_date: -1,
            current_xds_month: -1,
            current_program_type_reported: 0,
            xds_start_time_shown: 0,
            xds_program_length_shown: 0,
            xds_program_description: [[0; 33]; 8],
            current_xds_network_name: [0; 33],
            current_xds_program_name: [0; 33],
            current_xds_call_letters: [0; 7],
            current_xds_program_type: [0; 33],
            xds_buffers: [XdsBuffer {
                in_use: 0,
                xds_class: -1,
                xds_type: -1,
                bytes: [0; NUM_BYTES_PER_PACKET as usize],
                used_bytes: 0,
            }; NUM_XDS_BUFFERS as usize],
            cur_xds_buffer_idx: -1,
            cur_xds_packet_class: -1,
            cur_xds_payload: std::ptr::null_mut(),
            cur_xds_payload_length: 0,
            cur_xds_packet_type: 0,
            timing: TimingContext::new(),
            current_ar_start: 0,
            current_ar_end: 0,
            xds_write_to_file: 1,
        };

        let xds_string = b"Test XDS String";
        let result = write_xds_string(&mut sub, &mut ctx, xds_string.as_ptr(), xds_string.len());

        assert_eq!(result, 0);
        assert_eq!(sub.nb_data, 1);
        assert!(sub.got_output);

        unsafe {
            let data_ptr = sub.data as *mut Eia608Screen;
            let first_screen = &*data_ptr;
            assert_eq!(first_screen.xds_len, xds_string.len());
            assert_eq!(
                std::slice::from_raw_parts(first_screen.xds_str, xds_string.len()),
                xds_string
            );
        }
    }

    #[test]
    fn test_xdsprint_no_write_flag() {
        let mut sub = CcSubtitle {
            data: std::ptr::null_mut(),
            datatype: SubDataType::Generic,
            nb_data: 0,
            subtype: SubType::Cc608,
            enc_type: CcxEncodingType::Utf8,
            start_time: 0,
            end_time: 0,
            flags: 0,
            lang_index: 0,
            got_output: false,
            mode: [0; 5],
            info: [0; 4],
            time_out: 0,
            next: std::ptr::null_mut(),
            prev: std::ptr::null_mut(),
        };

        let mut ctx = CcxDecodersXdsContext {
            current_xds_min: -1,
            current_xds_hour: -1,
            current_xds_date: -1,
            current_xds_month: -1,
            current_program_type_reported: 0,
            xds_start_time_shown: 0,
            xds_program_length_shown: 0,
            xds_program_description: [[0; 33]; 8],
            current_xds_network_name: [0; 33],
            current_xds_program_name: [0; 33],
            current_xds_call_letters: [0; 7],
            current_xds_program_type: [0; 33],
            xds_buffers: [XdsBuffer {
                in_use: 0,
                xds_class: -1,
                xds_type: -1,
                bytes: [0; NUM_BYTES_PER_PACKET as usize],
                used_bytes: 0,
            }; NUM_XDS_BUFFERS as usize],
            cur_xds_buffer_idx: -1,
            cur_xds_packet_class: -1,
            cur_xds_payload: std::ptr::null_mut(),
            cur_xds_payload_length: 0,
            cur_xds_packet_type: 0,
            timing: TimingContext::new(),
            current_ar_start: 0,
            current_ar_end: 0,
            xds_write_to_file: 0, // Writing is disabled
        };

        let message = "This message should not be written";
        xdsprint(&mut sub, &mut ctx, "", format_args!("{}", message));

        // Verify that the subtitle structure was not updated
        assert_eq!(sub.nb_data, 0);
        assert!(!sub.got_output);
    }

    #[test]
    fn test_clear_xds_buffer_success() {
        let mut ctx = CcxDecodersXdsContext {
            current_xds_min: -1,
            current_xds_hour: -1,
            current_xds_date: -1,
            current_xds_month: -1,
            current_program_type_reported: 0,
            xds_start_time_shown: 0,
            xds_program_length_shown: 0,
            xds_program_description: [[0; 33]; 8],
            current_xds_network_name: [0; 33],
            current_xds_program_name: [0; 33],
            current_xds_call_letters: [0; 7],
            current_xds_program_type: [0; 33],
            xds_buffers: [XdsBuffer {
                in_use: 1,
                xds_class: 2,
                xds_type: 3,
                bytes: [0xFF; NUM_BYTES_PER_PACKET as usize],
                used_bytes: 10,
            }; NUM_XDS_BUFFERS as usize],
            cur_xds_buffer_idx: -1,
            cur_xds_packet_class: -1,
            cur_xds_payload: std::ptr::null_mut(),
            cur_xds_payload_length: 0,
            cur_xds_packet_type: 0,
            timing: TimingContext::new(),
            current_ar_start: 0,
            current_ar_end: 0,
            xds_write_to_file: 1,
        };

        let buffer_index = 0;

        // Call the function to clear the buffer
        clear_xds_buffer(&mut ctx, buffer_index);

        // Verify that the buffer was cleared
        let buffer = &ctx.xds_buffers[buffer_index as usize];
        assert_eq!(buffer.in_use, 0);
        assert_eq!(buffer.xds_class, -1);
        assert_eq!(buffer.xds_type, -1);
        assert_eq!(buffer.used_bytes, 0);
        assert!(buffer.bytes.iter().all(|&b| b == 0));
    }

    #[test]
    fn test_clear_xds_buffer_partial_clear() {
        let mut ctx = CcxDecodersXdsContext {
            current_xds_min: -1,
            current_xds_hour: -1,
            current_xds_date: -1,
            current_xds_month: -1,
            current_program_type_reported: 0,
            xds_start_time_shown: 0,
            xds_program_length_shown: 0,
            xds_program_description: [[0; 33]; 8],
            current_xds_network_name: [0; 33],
            current_xds_program_name: [0; 33],
            current_xds_call_letters: [0; 7],
            current_xds_program_type: [0; 33],
            xds_buffers: [XdsBuffer {
                in_use: 1,
                xds_class: 2,
                xds_type: 3,
                bytes: [0xFF; NUM_BYTES_PER_PACKET as usize],
                used_bytes: 10,
            }; NUM_XDS_BUFFERS as usize],
            cur_xds_buffer_idx: -1,
            cur_xds_packet_class: -1,
            cur_xds_payload: std::ptr::null_mut(),
            cur_xds_payload_length: 0,
            cur_xds_packet_type: 0,
            timing: TimingContext::new(),
            current_ar_start: 0,
            current_ar_end: 0,
            xds_write_to_file: 1,
        };

        let buffer_index = 1;

        // Call the function to clear the buffer
        clear_xds_buffer(&mut ctx, buffer_index);

        // Verify that only the specified buffer was cleared
        let cleared_buffer = &ctx.xds_buffers[buffer_index as usize];
        assert_eq!(cleared_buffer.in_use, 0);
        assert_eq!(cleared_buffer.xds_class, -1);
        assert_eq!(cleared_buffer.xds_type, -1);
        assert_eq!(cleared_buffer.used_bytes, 0);
        assert!(cleared_buffer.bytes.iter().all(|&b| b == 0));

        // Verify that other buffers remain unchanged
        let untouched_buffer = &ctx.xds_buffers[0];
        assert_eq!(untouched_buffer.in_use, 1);
        assert_eq!(untouched_buffer.xds_class, 2);
        assert_eq!(untouched_buffer.xds_type, 3);
        assert_eq!(untouched_buffer.used_bytes, 10);
        assert!(untouched_buffer.bytes.iter().all(|&b| b == 0xFF));
    }

    #[test]
    fn test_how_many_used_all_unused() {
        let ctx = CcxDecodersXdsContext {
            xds_buffers: [XdsBuffer {
                in_use: 0,
                xds_class: -1,
                xds_type: -1,
                bytes: [0; NUM_BYTES_PER_PACKET as usize],
                used_bytes: 0,
            }; NUM_XDS_BUFFERS as usize],
            ..Default::default()
        };

        let used_count = how_many_used(&ctx);
        assert_eq!(used_count, 0, "Expected no buffers to be in use");
    }

    #[test]
    fn test_how_many_used_some_used() {
        let mut ctx = CcxDecodersXdsContext {
            xds_buffers: [XdsBuffer {
                in_use: 0,
                xds_class: -1,
                xds_type: -1,
                bytes: [0; NUM_BYTES_PER_PACKET as usize],
                used_bytes: 0,
            }; NUM_XDS_BUFFERS as usize],
            ..Default::default()
        };

        // Mark some buffers as in use
        ctx.xds_buffers[0].in_use = 1;
        ctx.xds_buffers[2].in_use = 1;
        ctx.xds_buffers[4].in_use = 1;

        let used_count = how_many_used(&ctx);
        assert_eq!(used_count, 3, "Expected 3 buffers to be in use");
    }

    #[test]
    fn test_how_many_used_all_used() {
        let ctx = CcxDecodersXdsContext {
            xds_buffers: [XdsBuffer {
                in_use: 1,
                xds_class: -1,
                xds_type: -1,
                bytes: [0; NUM_BYTES_PER_PACKET as usize],
                used_bytes: 0,
            }; NUM_XDS_BUFFERS as usize],
            ..Default::default()
        };

        let used_count = how_many_used(&ctx);
        assert_eq!(
            used_count, NUM_XDS_BUFFERS as i64,
            "Expected all buffers to be in use"
        );
    }

    #[test]
    fn test_xds_do_copy_generation_management_system_no_write_flag() {
        let mut sub = CcSubtitle::default();
        let mut ctx = CcxDecodersXdsContext::default();
        ctx.xds_write_to_file = 0; // Disable writing XDS data

        let c1 = 0x50; // CGMS: Copy permitted (no restrictions)
        let c2 = 0x40; // APS: No APS, RCD: 0

        xds_do_copy_generation_management_system(&mut sub, &mut ctx, c1, c2);

        // Verify that the subtitle structure was not updated
        assert_eq!(sub.nb_data, 0);
        assert!(!sub.got_output);
    }

    #[test]
    fn test_xds_do_copy_generation_management_system_invalid_data() {
        let mut sub = CcSubtitle::default();
        let mut ctx = CcxDecodersXdsContext::default();
        ctx.xds_write_to_file = 1; // Enable writing XDS data

        let c1 = 0x10; // Invalid CGMS data (c1_6 is 0)
        let c2 = 0x40; // APS: No APS, RCD: 0

        xds_do_copy_generation_management_system(&mut sub, &mut ctx, c1, c2);

        // Verify that the subtitle structure was not updated
        assert_eq!(sub.nb_data, 0);
        assert!(!sub.got_output);
    }

    #[test]
    fn test_xds_do_content_advisory_no_write_flag() {
        let mut sub = CcSubtitle::default();
        let mut ctx = CcxDecodersXdsContext::default();
        ctx.xds_write_to_file = 0; // Disable writing XDS data

        let c1 = 0x58; // US TV parental guidelines: TV-14 (Parents Strongly Cautioned)
        let c2 = 0x10; // Content: Violence

        xds_do_content_advisory(&mut sub, &mut ctx, c1, c2);

        // Verify that the subtitle structure was not updated
        assert_eq!(sub.nb_data, 0);
        assert!(!sub.got_output);
    }

    #[test]
    fn test_xds_do_content_advisory_invalid_data() {
        let mut sub = CcSubtitle::default();
        let mut ctx = CcxDecodersXdsContext::default();
        ctx.xds_write_to_file = 1; // Enable writing XDS data

        let c1 = 0x10; // Invalid data (c1_6 is 0)
        let c2 = 0x40; // No additional content

        xds_do_content_advisory(&mut sub, &mut ctx, c1, c2);

        // Verify that the subtitle structure was not updated
        assert_eq!(sub.nb_data, 0);
        assert!(!sub.got_output);
    }

    #[test]
    fn test_xds_do_misc_invalid_payload() {
        let mut ctx = CcxDecodersXdsContext::default();
        ctx.cur_xds_packet_type = XDS_TYPE_TIME_OF_DAY as i64;
        ctx.cur_xds_payload_length = 4; // Invalid length (less than required)

        // Simulate an invalid payload
        let payload: [u8; 4] = [0x00, 0x00, 0x3C, 0x12];
        ctx.cur_xds_payload = payload.as_ptr() as *mut u8;

        let result = xds_do_misc(&ctx);

        // Verify the function did not process the payload
        assert_eq!(result, 1); // Still returns 1 but does not process
    }

    #[test]
    fn test_xds_do_misc_null_payload() {
        let mut ctx = CcxDecodersXdsContext::default();
        ctx.cur_xds_packet_type = XDS_TYPE_TIME_OF_DAY as i64;
        ctx.cur_xds_payload = std::ptr::null_mut(); // Null payload

        let result = xds_do_misc(&ctx);

        // Verify the function returned an error
        assert_eq!(result, CCX_EINVAL);
    }

    #[test]
    fn test_xds_do_misc_unsupported_packet_type() {
        let mut ctx = CcxDecodersXdsContext::default();
        ctx.cur_xds_packet_type = 0x99; // Unsupported packet type
        ctx.cur_xds_payload_length = 5;

        // Simulate a valid payload
        let payload: [u8; 5] = [0x00, 0x00, 0x25, 0x00, 0x00];
        ctx.cur_xds_payload = payload.as_ptr() as *mut u8;

        let result = xds_do_misc(&ctx);

        // Verify the function did not process the payload
        assert_eq!(result, 0);
    }

    #[test]
    fn test_xds_do_current_and_future_invalid_payload() {
        let mut sub = CcSubtitle::default();
        let mut ctx = CcxDecodersXdsContext::default();
        ctx.cur_xds_packet_type = XDS_TYPE_PIN_START_TIME as i64;
        ctx.cur_xds_payload_length = 4; // Invalid length (less than required)

        // Simulate an invalid payload
        let payload: [u8; 4] = [0x00, 0x00, 0x15, 0x10];
        ctx.cur_xds_payload = payload.as_ptr() as *mut u8;

        let result = xds_do_current_and_future(&mut sub, &mut ctx);

        // Verify the function did not process the payload
        assert_eq!(result, 1); // Still returns 1 but does not process
        assert_eq!(sub.nb_data, 0);
        assert!(!sub.got_output);
    }

    #[test]
    fn test_do_end_of_xds_empty_buffer() {
        let mut sub = CcSubtitle::default();
        let mut ctx = CcxDecodersXdsContext::default();

        // Simulate an empty XDS buffer
        ctx.cur_xds_buffer_idx = 0;
        ctx.xds_buffers[0].in_use = 0;

        let expected_checksum = 0x1D; // Example checksum
        do_end_of_xds(&mut sub, &mut ctx, expected_checksum);

        // Verify that nothing was processed
        assert_eq!(ctx.xds_buffers[0].in_use, 0);
        assert_eq!(sub.nb_data, 0);
        assert!(!sub.got_output);
    }

    #[test]
    fn test_xds_do_channel_invalid_payload() {
        let mut sub = CcSubtitle::default();
        let mut ctx = CcxDecodersXdsContext::default();
        ctx.cur_xds_packet_type = XDS_TYPE_CALL_LETTERS_AND_CHANNEL as i64;
        ctx.cur_xds_payload_length = 4; // Invalid length (less than required)

        // Simulate an invalid payload
        let payload: [u8; 4] = [0x00, 0x00, b'W', b'A'];
        ctx.cur_xds_payload = payload.as_ptr() as *mut u8;

        let result = xds_do_channel(&mut sub, &mut ctx);

        // Verify the function did not process the payload
        assert_eq!(result, 1); // Still returns 1 but does not process
        assert_eq!(sub.nb_data, 0);
        assert!(!sub.got_output);
    }

    #[test]
    fn test_xds_debug_test_empty_context() {
        let mut ctx = CcxDecodersXdsContext::default();
        let mut sub = CcSubtitle::default();

        // Call the debug test function without any valid data
        do_end_of_xds(&mut sub, &mut ctx, 0x2a);

        // Verify that the subtitle structure was not updated
        assert_eq!(sub.nb_data, 0);
        assert!(!sub.got_output);
    }

    #[test]
    fn test_xds_cea608_test_empty_context() {
        let mut ctx = CcxDecodersXdsContext::default();
        let mut sub = CcSubtitle::default();

        // Call the CEA-608 test function without any valid data
        do_end_of_xds(&mut sub, &mut ctx, 0x1d);

        // Verify that the subtitle structure was not updated
        assert_eq!(sub.nb_data, 0);
        assert!(!sub.got_output);
    }
}
