use crate::bindings::{lib_ccx_ctx, net_send_epg, FILE};
use crate::common::CType;
use crate::ctorust::FromCType;
use crate::transportstream::common_structs::{EPGEventRust, EPGRatingRust};
use crate::transportstream::tables::TS_PMT_MAP_SIZE;
use crate::{fclose, fopen, fprintf, fwrite};
use chrono::{Datelike, NaiveDate, NaiveDateTime, NaiveTime, Timelike};
use encoding_rs::*;
use lib_ccxr::common::Options;
use lib_ccxr::debug;
use lib_ccxr::info;
use lib_ccxr::util::log::DebugMessageFlag;
use std::alloc::{alloc, dealloc, realloc, Layout};
use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_void};
use std::ptr;

/// Specific errors for EPG DVB string decoding.
#[derive(Debug)]
pub struct EpgDecodeError;

/// Decode an EPG DVB‐encoded byte slice into UTF‑8.
pub fn epg_dvb_decode_string(input: &[u8]) -> Result<String, EpgDecodeError> {
    // Handle empty input - return empty string like C version
    if input.is_empty() {
        return Ok(String::new());
    }

    let first = input[0];

    // If first byte >= 0x20, skip encoding conversion (like C version)
    if first >= 0x20 {
        // Use windows-1252 instead of iso-8859-9 as fallback since encoding_rs
        // treats iso-8859-1 as windows-1252, and iso-8859-9 may not be available
        if let Some(enc) = Encoding::for_label(b"windows-1252") {
            let (cow, _, had_errors) = enc.decode(input);
            if !had_errors {
                return Ok(cow.into_owned());
            }
        }
        // If encoding fails, fall back to ASCII filtering like C version
        let filtered: Vec<u8> = input.iter().filter(|&&b| b <= 127).cloned().collect();
        return Ok(String::from_utf8_lossy(&filtered).into_owned());
    }

    // Determine how many bytes to skip and what remains to decode
    let (_, to_decode) = if first == 0x10 {
        if input.len() < 3 {
            return Err(EpgDecodeError);
        }
        (3, &input[3..])
    } else {
        if input.is_empty() {
            return Err(EpgDecodeError);
        }
        (1, &input[1..])
    };

    // Map encoding byte to encoding label
    let encoding_label = match first {
        0x01 => "iso-8859-5",   // ISO8859-5 (Cyrillic)
        0x02 => "iso-8859-6",   // ISO8859-6 (Arabic)
        0x03 => "iso-8859-7",   // ISO8859-7 (Greek)
        0x04 => "iso-8859-8",   // ISO8859-8 (Hebrew)
        0x05 => "windows-1254", // ISO8859-9 -> windows-1254 (Turkish)
        0x06 => "iso-8859-10",  // ISO8859-10 (Nordic)
        0x07 => "iso-8859-11",  // ISO8859-11 (Thai)
        0x08 => "windows-1252", // ISO8859-12 doesn't exist, fallback
        0x09 => "iso-8859-13",  // ISO8859-13 (Baltic)
        0x0a => "iso-8859-14",  // ISO8859-14 (Celtic)
        0x0b => "iso-8859-15",  // ISO8859-15 (Latin-9 with Euro)
        0x10 => {
            // Dynamic ISO encoding based on next two bytes
            let cpn = ((input[1] as u16) << 8) | (input[2] as u16);
            // Handle common cases that might not be supported
            match cpn {
                1 => "windows-1252", // ISO-8859-1 -> windows-1252
                9 => "windows-1254", // ISO-8859-9 -> windows-1254
                _ => {
                    // Try to construct the label, but have fallback ready
                    let label = format!("iso-8859-{cpn}");
                    if Encoding::for_label(label.as_bytes()).is_some() {
                        // This is a bit of a hack - we'll handle this case separately below
                        return decode_with_dynamic_label(to_decode, &label);
                    } else {
                        "windows-1252" // fallback
                    }
                }
            }
        }
        0x11 => "utf-8",     // UTF-8
        0x12 => "euc-kr",    // KS_C_5601-1987 -> EUC-KR
        0x13 => "gb2312",    // GB2312
        0x14 => "big5",      // BIG-5
        0x15 => "utf-8",     // UTF-8
        _ => "windows-1252", // Default fallback like C version uses ISO8859-9
    };

    // Try to decode with the determined encoding
    if let Some(enc) = Encoding::for_label(encoding_label.as_bytes()) {
        let (cow, _, had_errors) = enc.decode(to_decode);
        if !had_errors {
            return Ok(cow.into_owned());
        }
        // If decoding had errors but we got some result, still use it
        return Ok(cow.into_owned());
    }

    // Fallback: ASCII filtering like the C version does
    let filtered: Vec<u8> = to_decode.iter().filter(|&&b| b <= 127).cloned().collect();
    Ok(String::from_utf8_lossy(&filtered).into_owned())
}
// Helper function for dynamic ISO encoding labels
fn decode_with_dynamic_label(data: &[u8], label: &str) -> Result<String, EpgDecodeError> {
    if let Some(enc) = Encoding::for_label(label.as_bytes()) {
        let (cow, _, _) = enc.decode(data);
        Ok(cow.into_owned())
    } else {
        // Fallback to ASCII filtering
        let filtered: Vec<u8> = data.iter().filter(|&&b| b <= 127).cloned().collect();
        Ok(String::from_utf8_lossy(&filtered).into_owned())
    }
}
pub fn epg_fprintxml(f: &mut FILE, string: &str) {
    let bytes = string.as_bytes();
    let mut start_idx = 0;

    for (i, &byte) in bytes.iter().enumerate() {
        let replacement: &[u8] = match byte {
            b'<' => b"&lt;",
            b'>' => b"&gt;",
            b'"' => b"&quot;",
            b'&' => b"&amp;",
            b'\'' => b"&apos;",
            _ => continue,
        };

        // Write the segment before the special character using fwrite (raw bytes)
        if i > start_idx {
            write_raw_bytes(f, &bytes[start_idx..i]);
        }

        // Write the XML entity using fwrite (raw bytes)
        write_raw_bytes(f, replacement);

        // Update start position for next segment
        start_idx = i + 1;
    }

    // Write any remaining segment using fwrite (raw bytes)
    if start_idx < bytes.len() {
        write_raw_bytes(f, &bytes[start_idx..]);
    }
}

// Helper function to write raw bytes using fwrite (like the C version)
fn write_raw_bytes(f: &mut FILE, bytes: &[u8]) {
    if bytes.is_empty() {
        return;
    }

    unsafe {
        fwrite(
            bytes.as_ptr() as *const c_void,
            1,
            bytes.len() as _,
            f as *mut FILE,
        );
    }
}

// Fills given string with given (event.*_time_string) ATSC time converted to XMLTV style time string
pub fn epg_atsc_calc_time(output: &mut String, time: u32) {
    // Start from January 6, 1980
    let base_date = NaiveDate::from_ymd_opt(1980, 1, 6).unwrap();
    let base_time = NaiveTime::from_hms_opt(0, 0, 0).unwrap();
    let base_datetime = NaiveDateTime::new(base_date, base_time);

    // Add the time in seconds
    let result_datetime = base_datetime + chrono::Duration::seconds(time as i64);

    *output = format!(
        "{:04}{:02}{:02}{:02}{:02}{:02} +0000",
        result_datetime.year(),
        result_datetime.month(),
        result_datetime.day(),
        result_datetime.hour(),
        result_datetime.minute(),
        result_datetime.second()
    );
}

// Fills event.start_time_string in XMLTV format with passed DVB time
pub fn epg_dvb_calc_start_time(event: &mut EPGEventRust, time: u64) {
    let mjd = time >> 24;
    event.start_time_string.clear();

    if mjd > 0 {
        let y = ((mjd as f64 - 15078.2) / 365.25) as i32;
        let m = ((mjd as f64 - 14956.1 - (y as f64 * 365.25)) / 30.6001) as i32;
        let d = mjd as i32 - 14956 - (y as f64 * 365.25) as i32 - (m as f64 * 30.6001) as i32;
        let k = if m == 14 || m == 15 { 1 } else { 0 };
        let year = y + k + 1900;
        let month = m - 1 - k * 12;

        event.start_time_string =
            format!("{:02}{:02}{:02}{:06}+0000", year, month, d, time & 0xffffff);
    }
}

// Fills event.end_time_string in XMLTV with passed DVB time + duration
pub fn epg_dvb_calc_end_time(event: &mut EPGEventRust, time: u64, duration: u32) {
    let mjd = time >> 24;
    event.end_time_string.clear();

    if mjd > 0 {
        let y = ((mjd as f64 - 15078.2) / 365.25) as i32;
        let m = ((mjd as f64 - 14956.1 - (y as f64 * 365.25)) / 30.6001) as i32;
        let d = mjd as i32 - 14956 - (y as f64 * 365.25) as i32 - (m as f64 * 30.6001) as i32;
        let k = if m == 14 || m == 15 { 1 } else { 0 };
        let year = y + k + 1900;
        let month = m - 1 - k * 12;

        let mut tm_year = year - 1900;
        let mut tm_mon = month - 1;
        let mut tm_mday = d;

        let mut tm_sec = ((time & 0x0f)
            + (10 * ((time & 0xf0) >> 4))
            + ((duration & 0x0f) as u64)
            + (10 * (((duration & 0xf0) >> 4) as u64))) as i32;
        let mut tm_min = (((time & 0x0f00) >> 8)
            + (10 * (((time & 0xf000) >> 4) >> 8))
            + (((duration & 0x0f00) >> 8) as u64)
            + (10 * ((((duration & 0xf000) >> 4) >> 8) as u64))) as i32;
        let mut tm_hour = (((time & 0x0f0000) >> 16)
            + (10 * (((time & 0xf00000) >> 4) >> 16))
            + (((duration & 0x0f0000) >> 16) as u64)
            + (10 * ((((duration & 0xf00000) >> 4) >> 16) as u64)))
            as i32;

        // Simulate mktime() normalization
        if tm_sec >= 60 {
            tm_min += tm_sec / 60;
            tm_sec %= 60;
        }
        if tm_min >= 60 {
            tm_hour += tm_min / 60;
            tm_min %= 60;
        }
        if tm_hour >= 24 {
            tm_mday += tm_hour / 24;
            tm_hour %= 24;
        }

        // Handle month/year overflow
        while tm_mday > days_in_month(tm_year + 1900, tm_mon + 1) {
            tm_mday -= days_in_month(tm_year + 1900, tm_mon + 1);
            tm_mon += 1;
            if tm_mon >= 12 {
                tm_mon = 0;
                tm_year += 1;
            }
        }

        event.end_time_string = format!(
            "{:04}{:02}{:02}{:02}{:02}{:02} +0000",
            tm_year + 1900,
            tm_mon + 1,
            tm_mday,
            tm_hour,
            tm_min,
            tm_sec
        );
    }
}

// Helper function to get days in month
fn days_in_month(year: i32, month: i32) -> i32 {
    match month {
        1 | 3 | 5 | 7 | 8 | 10 | 12 => 31,
        4 | 6 | 9 | 11 => 30,
        2 => {
            if (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0) {
                29
            } else {
                28
            }
        }
        _ => 30, // fallback
    }
}
// Returns english string description of the passed DVB category ID
pub fn epg_dvb_content_type_to_string(cat: u8) -> &'static str {
    match cat {
        0x00 => "reserved",
        0x10 => "movie/drama (general)",
        0x11 => "detective/thriller",
        0x12 => "adventure/western/war",
        0x13 => "science fiction/fantasy/horror",
        0x14 => "comedy",
        0x15 => "soap/melodram/folkloric",
        0x16 => "romance",
        0x17 => "serious/classical/religious/historical movie/drama",
        0x18 => "adult movie/drama",
        0x1E => "reserved",
        0x1F => "user defined",
        0x20 => "news/current affairs (general)",
        0x21 => "news/weather report",
        0x22 => "news magazine",
        0x23 => "documentary",
        0x24 => "discussion/interview/debate",
        0x2E => "reserved",
        0x2F => "user defined",
        0x30 => "show/game show (general)",
        0x31 => "game show/quiz/contest",
        0x32 => "variety show",
        0x33 => "talk show",
        0x3E => "reserved",
        0x3F => "user defined",
        0x40 => "sports (general)",
        0x41 => "special events",
        0x42 => "sports magazine",
        0x43 => "football/soccer",
        0x44 => "tennis/squash",
        0x45 => "team sports",
        0x46 => "athletics",
        0x47 => "motor sport",
        0x48 => "water sport",
        0x49 => "winter sport",
        0x4A => "equestrian",
        0x4B => "martial sports",
        0x4E => "reserved",
        0x4F => "user defined",
        0x50 => "childrens's/youth program (general)",
        0x51 => "pre-school children's program",
        0x52 => "entertainment (6-14 year old)",
        0x53 => "entertainment (10-16 year old)",
        0x54 => "information/education/school program",
        0x55 => "cartoon/puppets",
        0x5E => "reserved",
        0x5F => "user defined",
        0x60 => "music/ballet/dance (general)",
        0x61 => "rock/pop",
        0x62 => "serious music/classic music",
        0x63 => "folk/traditional music",
        0x64 => "jazz",
        0x65 => "musical/opera",
        0x66 => "ballet",
        0x6E => "reserved",
        0x6F => "user defined",
        0x70 => "arts/culture (without music, general)",
        0x71 => "performing arts",
        0x72 => "fine arts",
        0x73 => "religion",
        0x74 => "popular culture/traditional arts",
        0x75 => "literature",
        0x76 => "film/cinema",
        0x77 => "experimental film/video",
        0x78 => "broadcasting/press",
        0x79 => "new media",
        0x7A => "arts/culture magazine",
        0x7B => "fashion",
        0x7E => "reserved",
        0x7F => "user defined",
        0x80 => "social/political issues/economics (general)",
        0x81 => "magazines/reports/documentary",
        0x82 => "economics/social advisory",
        0x83 => "remarkable people",
        0x8E => "reserved",
        0x8F => "user defined",
        0x90 => "education/science/factual topics (general)",
        0x91 => "nature/animals/environment",
        0x92 => "technology/natural science",
        0x93 => "medicine/physiology/psychology",
        0x94 => "foreign countries/expeditions",
        0x95 => "social/spiritual science",
        0x96 => "further education",
        0x97 => "languages",
        0x9E => "reserved",
        0x9F => "user defined",
        0xA0 => "leisure hobbies (general)",
        0xA1 => "tourism/travel",
        0xA2 => "handicraft",
        0xA3 => "motoring",
        0xA4 => "fitness & health",
        0xA5 => "cooking",
        0xA6 => "advertisement/shopping",
        0xA7 => "gardening",
        0xAE => "reserved",
        0xAF => "user defined",
        0xB0 => "original language",
        0xB1 => "black & white",
        0xB2 => "unpublished",
        0xB3 => "live broadcast",
        0xBE => "reserved",
        0xBF => "user defined",
        0xEF => "reserved",
        0xFF => "user defined",
        _ => "undefined content",
    }
}
fn write_fprintf(f: &mut FILE, s: &str) {
    let format_str = "%s\0";
    let format_cstr = format_str.as_ptr() as *const c_char;
    let c_string = CString::new(s).unwrap();

    unsafe {
        fprintf(f as *mut FILE, format_cstr, c_string.as_ptr());
    }
}

fn write_fprintf_with_int(f: &mut FILE, format: &str, value: u32) {
    let formatted = format.replace("{}", &value.to_string());
    write_fprintf(f, &formatted);
}

fn write_fprintf_with_str(f: &mut FILE, format: &str, value: &str) {
    let formatted = format.replace("{}", value);
    write_fprintf(f, &formatted);
}

// Prints given event to already opened XMLTV file
pub fn epg_print_event(event: &EPGEventRust, channel: u32, f: &mut FILE) {
    // Write program opening tag with attributes
    write_fprintf(f, "  <program  ");
    write_fprintf(f, "start=\"");
    write_fprintf(f, &event.start_time_string);
    write_fprintf(f, "\" ");
    write_fprintf(f, "stop=\"");
    write_fprintf(f, &event.end_time_string);
    write_fprintf(f, "\" ");
    write_fprintf_with_int(f, "channel=\"{}\">\n", channel);

    // Write simple event information if available
    if event.has_simple {
        if let Some(ref event_name) = event.event_name {
            write_fprintf_with_str(f, "    <title lang=\"{}\">", &event.iso_639_language_code);
            epg_fprintxml(f, event_name);
            write_fprintf(f, "</title>\n");
        }

        if let Some(ref text) = event.text {
            write_fprintf_with_str(
                f,
                "    <sub-title lang=\"{}\">",
                &event.iso_639_language_code,
            );
            epg_fprintxml(f, text);
            write_fprintf(f, "</sub-title>\n");
        }
    }

    // Write extended description if available
    if let Some(ref extended_text) = event.extended_text {
        write_fprintf_with_str(
            f,
            "    <desc lang=\"{}\">",
            &event.extended_iso_639_language_code,
        );
        epg_fprintxml(f, extended_text);
        write_fprintf(f, "</desc>\n");
    }
    for rating in &event.ratings {
        if rating.age > 0 && rating.age < 0x10 {
            let rating_str = format!(
                "    <rating system=\"dvb:{}\">{}</rating>\n",
                rating.country_code,
                rating.age + 3
            );
            write_fprintf(f, &rating_str);
        }
    }

    // Write categories
    for &category in &event.categories {
        write_fprintf(f, "    <category lang=\"en\">");
        let category_str = epg_dvb_content_type_to_string(category);
        epg_fprintxml(f, category_str);
        write_fprintf(f, "</category>\n");
    }

    // Write metadata ID and closing tag
    write_fprintf_with_int(f, "    <ts-meta-id>{}</ts-meta-id>\n", event.id);
    write_fprintf(f, "  </program>\n");
}

// Network output function
/// # Safety
/// This function is unsafe because it dereferences raw pointers and performs network operations.
pub unsafe fn epg_output_net(ctx: &mut lib_ccx_ctx, ccx_options: &mut Options) {
    // Check if demux context exists
    if ctx.demux_ctx.is_null() {
        return;
    }

    let demux_ctx = unsafe { &*ctx.demux_ctx };

    if demux_ctx.nb_program == 0 {
        return;
    }

    // Find the program matching ts_forced_program
    let mut program_index = None;
    for (i, pinfo) in demux_ctx.pinfo.iter().enumerate() {
        if pinfo.program_number == ccx_options.demux_cfg.ts_forced_program.unwrap_or(i32::MIN) {
            program_index = Some(i);
            break;
        }
    }

    let program_index = match program_index {
        Some(idx) => idx,
        None => return,
    };

    // Process events for the found program
    if !ctx.eit_programs.is_null() {
        let eit_program = unsafe { &mut *ctx.eit_programs.add(program_index) };

        for j in 0..eit_program.array_len {
            let event = &mut eit_program.epg_events[j as usize];

            if event.live_output != 0 {
                continue;
            }

            event.live_output = 1;

            let category = if event.num_categories > 0 {
                let category_val = unsafe { *event.categories.offset(0) };
                epg_dvb_content_type_to_string(category_val)
            } else {
                "undefined content"
            };

            // Convert C strings to Rust strings for the network call
            let start_time = unsafe {
                CStr::from_ptr(event.start_time_string.as_ptr())
                    .to_str()
                    .unwrap_or("")
            };
            let end_time = unsafe {
                CStr::from_ptr(event.end_time_string.as_ptr())
                    .to_str()
                    .unwrap_or("")
            };
            let event_name = if !event.event_name.is_null() {
                unsafe { CStr::from_ptr(event.event_name).to_str().unwrap_or("") }
            } else {
                ""
            };
            let extended_text = if !event.extended_text.is_null() {
                unsafe { CStr::from_ptr(event.extended_text).to_str().unwrap_or("") }
            } else {
                ""
            };
            let language_code = unsafe {
                CStr::from_ptr(event.ISO_639_language_code.as_ptr())
                    .to_str()
                    .unwrap_or("")
            };

            // Call the network send function with C strings
            let start_cstr = CString::new(start_time).unwrap();
            let end_cstr = CString::new(end_time).unwrap();
            let name_cstr = CString::new(event_name).unwrap();
            let text_cstr = CString::new(extended_text).unwrap();
            let lang_cstr = CString::new(language_code).unwrap();
            let cat_cstr = CString::new(category).unwrap();

            unsafe {
                net_send_epg(
                    start_cstr.as_ptr(),
                    end_cstr.as_ptr(),
                    name_cstr.as_ptr(),
                    text_cstr.as_ptr(),
                    lang_cstr.as_ptr(),
                    cat_cstr.as_ptr(),
                );
            }
        }
    }
}
/// Creates fills and closes a new XMLTV file for live mode output.
/// File should include only events not previously output.
/// # Safety
/// This function is unsafe because it dereferences raw pointers and performs file operations.
pub unsafe fn epg_output_live(ctx_ref: &mut lib_ccx_ctx) {
    let mut has_new_events = false;

    // Check if demux context exists
    if ctx_ref.demux_ctx.is_null() {
        return;
    }

    let demux_ctx = &*ctx_ref.demux_ctx;

    // Check if there are any events not previously output
    if !ctx_ref.eit_programs.is_null() {
        for i in 0..demux_ctx.nb_program {
            let eit_program = &*ctx_ref.eit_programs.offset(i as isize);
            for j in 0..eit_program.array_len {
                let epg_event = &eit_program.epg_events[j as usize];
                if epg_event.live_output == 0 {
                    has_new_events = true;
                    break;
                }
            }
            if has_new_events {
                break;
            }
        }
    }

    if !has_new_events {
        return;
    }

    // Create filename
    let basefilename = if !ctx_ref.basefilename.is_null() {
        CStr::from_ptr(ctx_ref.basefilename)
            .to_str()
            .unwrap_or("output")
    } else {
        "output"
    };

    let filename = format!("{}_{}.xml.part", basefilename, ctx_ref.epg_last_live_output);
    let filename_cstring =
        CString::new(filename.clone()).unwrap_or_else(|_| CString::new("").unwrap());

    // Open file for writing
    let mode = CString::new("w").unwrap();
    let file = fopen(filename_cstring.as_ptr(), mode.as_ptr());
    if file.is_null() {
        return;
    }

    let f = &mut *(file as *mut FILE);

    // Write XML header
    write_fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    write_fprintf(f, "<!DOCTYPE tv SYSTEM \"xmltv.dtd\">\n\n");
    write_fprintf(f, "<tv>\n");

    // Write channel information
    for i in 0..demux_ctx.nb_program {
        let pinfo = &demux_ctx.pinfo[i as usize];
        write_fprintf_with_int(f, "  <channel id=\"{}\">", pinfo.program_number as u32);
        write_fprintf(f, "\n");
        write_fprintf_with_int(
            f,
            "    <display-name>{}</display-name>",
            pinfo.program_number as u32,
        );
        write_fprintf(f, "\n");
        write_fprintf(f, "  </channel>\n");
    }

    // Write events that haven't been output yet
    for i in 0..demux_ctx.nb_program {
        let eit_program = &mut *ctx_ref.eit_programs.offset(i as isize);
        let pinfo = &demux_ctx.pinfo[i as usize];

        for j in 0..eit_program.array_len {
            let epg_event = &mut eit_program.epg_events[j as usize];
            if epg_event.live_output == 0 {
                epg_event.live_output = 1;
                epg_print_event(
                    &EPGEventRust::from_ctype(*epg_event).unwrap_or(EPGEventRust::default()),
                    pinfo.program_number as u32,
                    f,
                );
            }
        }
    }

    write_fprintf(f, "</tv>");
    fclose(file);

    // Rename file (remove .part extension) - using system rename
    let final_filename = filename.strip_suffix(".part").unwrap_or(&filename);

    // Use system call for rename since we can't use libc
    std::fs::rename(&filename, final_filename).unwrap_or(());
}
/// Creates fills and closes a new XMLTV file for full output mode.
/// File should include all events in memory.
/// # Safety
/// This function is unsafe because it dereferences raw pointers and performs file operations.
pub unsafe fn epg_output(ctx_ref: &mut lib_ccx_ctx, ccx_options: &Options) {
    // Check if demux context exists
    if ctx_ref.demux_ctx.is_null() {
        return;
    }

    let demux_ctx = &*ctx_ref.demux_ctx;

    // Create filename
    let basefilename = if !ctx_ref.basefilename.is_null() {
        CStr::from_ptr(ctx_ref.basefilename)
            .to_str()
            .unwrap_or("output")
    } else {
        "output"
    };

    let filename = format!("{basefilename}_epg.xml");
    let filename_cstring =
        CString::new(filename.clone()).unwrap_or_else(|_| CString::new("").unwrap());

    // Open file for writing
    let mode = CString::new("w").unwrap();
    let file = fopen(filename_cstring.as_ptr(), mode.as_ptr());
    if file.is_null() {
        info!("Unable to open {}\n", filename);
        return;
    }

    let f = &mut *(file as *mut FILE);

    // Write XML header
    write_fprintf(
        f,
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE tv SYSTEM \"xmltv.dtd\">\n\n<tv>\n",
    );

    // Write channel information
    for i in 0..demux_ctx.nb_program {
        let pinfo = &demux_ctx.pinfo[i as usize];
        write_fprintf_with_int(f, "  <channel id=\"{}\">\n", pinfo.program_number as u32);
        write_fprintf(f, "    <display-name>");

        // Check if name is available and not empty
        let name_str = if !pinfo.name.is_empty() {
            let name_cstr = CStr::from_ptr(pinfo.name.as_ptr());
            let name = name_cstr.to_str().unwrap_or("");
            if !name.is_empty() {
                Some(name)
            } else {
                None
            }
        } else {
            None
        };

        if let Some(name) = name_str {
            epg_fprintxml(f, name);
        } else {
            write_fprintf_with_int(f, "{}\n", pinfo.program_number as u32);
        }

        write_fprintf(f, "</display-name>\n");
        write_fprintf(f, "  </channel>\n");
    }

    // Check xmltvonlycurrent option and write events accordingly
    if !ccx_options.xmltvonlycurrent {
        // Print all events
        if !ctx_ref.eit_programs.is_null() {
            for i in 0..demux_ctx.nb_program {
                let eit_program = &*ctx_ref.eit_programs.offset(i as isize);
                let pinfo = &demux_ctx.pinfo[i as usize];

                for j in 0..eit_program.array_len {
                    let epg_event = &eit_program.epg_events[j as usize];
                    epg_print_event(
                        &EPGEventRust::from_ctype(*epg_event).unwrap_or(EPGEventRust::default()),
                        pinfo.program_number as u32,
                        f,
                    );
                }
            }

            // Stream has no PMT, fall back to unordered events
            if demux_ctx.nb_program == 0 {
                let fallback_program = &*ctx_ref.eit_programs.add(TS_PMT_MAP_SIZE);
                for j in 0..fallback_program.array_len {
                    let epg_event = &fallback_program.epg_events[j as usize];
                    epg_print_event(
                        &EPGEventRust::from_ctype(*epg_event).unwrap_or(EPGEventRust::default()),
                        epg_event.service_id as u32,
                        f,
                    );
                }
            }
        }
    } else {
        // Print current events only
        if !ctx_ref.eit_programs.is_null() && !ctx_ref.eit_current_events.is_null() {
            for i in 0..demux_ctx.nb_program {
                let eit_program = &*ctx_ref.eit_programs.offset(i as isize);
                let pinfo = &demux_ctx.pinfo[i as usize];
                let ce = *ctx_ref.eit_current_events.offset(i as isize);

                for j in 0..eit_program.array_len {
                    let epg_event = &eit_program.epg_events[j as usize];
                    if ce as u32 == epg_event.id {
                        epg_print_event(
                            &EPGEventRust::from_ctype(*epg_event)
                                .unwrap_or(EPGEventRust::default()),
                            pinfo.program_number as u32,
                            f,
                        );
                    }
                }
            }
        }
    }

    write_fprintf(f, "</tv>");
    fclose(file);
}

/// Compare 2 events. Return false if they are different.
pub fn epg_event_cmp(e1: &EPGEventRust, e2: &EPGEventRust) -> bool {
    if e1.id != e2.id
        || e1.start_time_string != e2.start_time_string
        || e1.end_time_string != e2.end_time_string
    {
        return false;
    }
    // Could add full checking of strings here if desired.
    true
}

/// Add given event to array of events.
/// Return false if nothing changed, true if this is a new or updated event.
/// # Safety
/// This function is unsafe because it dereferences raw pointers and performs memory operations.
pub unsafe fn epg_add_event(ctx_ref: &mut lib_ccx_ctx, pmt_map: i32, event: &EPGEventRust) -> bool {
    // Get the EIT program array from the C struct
    let eit_programs_ptr = ctx_ref.eit_programs;
    if eit_programs_ptr.is_null() {
        return false;
    }

    // Validate pmt_map index - you need to know the actual size of the eit_programs array
    // This is a guess - you should replace with the actual maximum size
    if !(0..1000).contains(&pmt_map) {
        // Replace 1000 with actual max size
        return false;
    }

    // Access the specific EIT program at pmt_map index
    let eit_program_ptr = eit_programs_ptr.offset(pmt_map as isize);

    // Search for existing event with same ID
    for j in 0..(*eit_program_ptr).array_len {
        let existing_event = &mut (*eit_program_ptr).epg_events[j as usize];

        if existing_event.id == event.id {
            // Convert existing C event to Rust for comparison
            if let Some(existing_rust_event) = EPGEventRust::from_ctype(*existing_event) {
                // Compare events using the existing comparison function
                if epg_event_cmp(event, &existing_rust_event) {
                    // Events are identical, nothing to do
                    return false;
                } else {
                    // Event with this ID exists but has changed - update it
                    // First, preserve the count from the existing event
                    let mut updated_event = event.clone();
                    updated_event.count = existing_rust_event.count;

                    // Free the existing event (equivalent to EPG_free_event)
                    EPGEventRust::cleanup_c_event(*existing_event);

                    // Convert the updated Rust event back to C and copy it
                    let new_c_event = updated_event.to_ctype();
                    *existing_event = new_c_event;

                    return true;
                }
            }
        }
    }

    // ID not found in array - add new event
    // Create new event with count = 0
    let mut new_event = event.clone();
    new_event.count = 0;

    // Convert to C type and add to array
    let c_event = new_event.to_ctype();
    let array_len = (*eit_program_ptr).array_len as usize;

    // Check bounds to avoid buffer overflow
    if array_len >= 10080 {
        return false;
    }

    (*eit_program_ptr).epg_events[array_len] = c_event;

    // Increment array length
    (*eit_program_ptr).array_len += 1;

    true
}

/// EN 300 468 V1.3.1 (1998-02)
/// 6.2.4 Content descriptor
pub fn epg_decode_content_descriptor(
    offset: &[u8],
    descriptor_length: u32,
    event: &mut EPGEventRust,
) {
    let num_items = descriptor_length as usize / 2;

    if num_items == 0 {
        debug!(
            msg_type = DebugMessageFlag::GENERIC_NOTICE;
            "Warning: Invalid EIT content_descriptor length detected."
        );
        return;
    }

    // Clear existing categories and reserve space
    event.categories.clear();
    event.categories.reserve(num_items);

    // Extract categories from the descriptor
    for i in 0..num_items {
        if i * 2 < offset.len() {
            event.categories.push(offset[i * 2]);
        }
    }
}

/// EN 300 468 V1.3.1 (1998-02)
/// 6.2.20 Parental rating description
pub fn epg_decode_parental_rating_descriptor(
    offset: &[u8],
    descriptor_length: u32,
    event: &mut EPGEventRust,
) {
    let num_items = descriptor_length as usize / 4;

    if num_items == 0 {
        debug!(
            msg_type = DebugMessageFlag::GENERIC_NOTICE;
            "Warning: Invalid EIT parental_rating length detected."
        );
        return;
    }

    // Clear existing ratings and reserve space
    event.ratings.clear();
    event.ratings.reserve(num_items);

    // Extract ratings from the descriptor
    for i in 0..num_items {
        let base_idx = i * 4;

        // Ensure we have enough bytes for this rating entry
        if base_idx + 3 < offset.len() {
            // Extract country code (3 bytes + null terminator equivalent)
            let country_code = String::from_utf8_lossy(&offset[base_idx..base_idx + 3]).to_string();

            // Extract and validate age rating
            let age = if offset[base_idx + 3] == 0x00 || offset[base_idx + 3] >= 0x10 {
                0
            } else {
                offset[base_idx + 3]
            };

            // Create and add the rating
            let rating = EPGRatingRust { country_code, age };

            event.ratings.push(rating);
        }
    }
}
/// EN 300 468 V1.3.1 (1998-02)
/// 6.2.27 Short event descriptor
pub fn epg_decode_short_event_descriptor(
    offset: &[u8],
    descriptor_length: u32,
    event: &mut EPGEventRust,
) {
    if offset.len() < 4 {
        debug!(
            msg_type = DebugMessageFlag::GENERIC_NOTICE;
            "Warning: Invalid short_event_descriptor size detected."
        );
        return;
    }

    event.has_simple = true;
    event.iso_639_language_code = format!(
        "{}{}{}",
        offset[0] as char, offset[1] as char, offset[2] as char
    );

    let event_name_length = offset[3] as usize;
    if event_name_length + 4 > descriptor_length as usize {
        debug!(
            msg_type = DebugMessageFlag::GENERIC_NOTICE;
            "Warning: Invalid short_event_descriptor size detected."
        );
        return;
    }

    if offset.len() < 4 + event_name_length {
        debug!(
            msg_type = DebugMessageFlag::GENERIC_NOTICE;
            "Warning: Invalid short_event_descriptor size detected."
        );
        return;
    }

    // Decode event name
    match epg_dvb_decode_string(&offset[4..4 + event_name_length]) {
        Ok(decoded_name) => event.event_name = Some(decoded_name),
        Err(_) => {
            debug!(
                msg_type = DebugMessageFlag::GENERIC_NOTICE;
                "Warning: Failed to decode event name."
            );
        }
    }

    if offset.len() < 5 + event_name_length {
        debug!(
            msg_type = DebugMessageFlag::GENERIC_NOTICE;
            "Warning: Invalid short_event_descriptor size detected."
        );
        return;
    }

    let text_length = offset[4 + event_name_length] as usize;
    if text_length + event_name_length + 5 > descriptor_length as usize {
        debug!(
            msg_type = DebugMessageFlag::GENERIC_NOTICE;
            "Warning: Invalid short_event_descriptor size detected."
        );
        return;
    }

    if offset.len() < 5 + event_name_length + text_length {
        debug!(
            msg_type = DebugMessageFlag::GENERIC_NOTICE;
            "Warning: Invalid short_event_descriptor size detected."
        );
        return;
    }

    // Decode text
    match epg_dvb_decode_string(&offset[5 + event_name_length..5 + event_name_length + text_length])
    {
        Ok(decoded_text) => event.text = Some(decoded_text),
        Err(_) => {
            debug!(
                msg_type = DebugMessageFlag::GENERIC_NOTICE;
                "Warning: Failed to decode event text."
            );
        }
    }
}

/// EN 300 468 V1.3.1 (1998-02)
/// 6.2.9 Extended event descriptor
pub fn epg_decode_extended_event_descriptor(
    offset: &[u8],
    descriptor_length: u32,
    event: &mut EPGEventRust,
) {
    if offset.len() < 5 {
        debug!(
            msg_type = DebugMessageFlag::GENERIC_NOTICE;
            "Warning: Invalid extended_event_descriptor size detected."
        );
        return;
    }

    let descriptor_number = (offset[0] >> 4) as usize;
    let last_descriptor_number = (offset[0] & 0x0f) as usize;

    event.extended_iso_639_language_code = format!(
        "{}{}{}",
        offset[1] as char, offset[2] as char, offset[3] as char
    );

    let length_of_items = offset[4] as usize;
    if length_of_items > descriptor_length as usize - 5 {
        debug!(
            msg_type = DebugMessageFlag::GENERIC_NOTICE;
            "Warning: Invalid extended_event_descriptor size detected."
        );
        return;
    }

    if offset.len() < length_of_items + 6 {
        debug!(
            msg_type = DebugMessageFlag::GENERIC_NOTICE;
            "Warning: Invalid extended_event_descriptor size detected."
        );
        return;
    }

    let text_offset = length_of_items + 5;
    let mut text_length = offset[text_offset] as usize;

    if text_length > descriptor_length as usize - 5 - length_of_items - 1 {
        debug!(
            msg_type = DebugMessageFlag::GENERIC_NOTICE;
            "Warning: Invalid extended_event_text_length size detected."
        );
        return;
    }

    let mut text_start_offset = text_offset + 1;
    #[allow(unused_variables)] // TODO check this
    let mut oldlen = 0;

    // Handle continuation descriptors
    if descriptor_number > 0 {
        if offset.len() > text_start_offset && offset[text_start_offset] < 0x20 {
            text_start_offset += 1;
            text_length -= 1;
        }

        // Get existing extended text length
        #[allow(unused_assignments)] // TODO check this
        if let Some(ref existing_text) = event.extended_text {
            oldlen = existing_text.len();
        }
    }

    if offset.len() < text_start_offset + text_length {
        debug!(
            msg_type = DebugMessageFlag::GENERIC_NOTICE;
            "Warning: Invalid extended_event_descriptor size detected."
        );
        return;
    }

    // Extract new text segment
    let text_segment = &offset[text_start_offset..text_start_offset + text_length];

    // Append to existing extended text or create new
    let combined_text = if descriptor_number > 0 {
        if let Some(ref existing_text) = event.extended_text {
            let mut combined = existing_text.as_bytes().to_vec();
            combined.extend_from_slice(text_segment);
            combined
        } else {
            text_segment.to_vec()
        }
    } else {
        text_segment.to_vec()
    };

    // If this is the last descriptor, decode the complete text
    if descriptor_number == last_descriptor_number {
        match epg_dvb_decode_string(&combined_text) {
            Ok(decoded_text) => event.extended_text = Some(decoded_text),
            Err(_) => {
                debug!(
                    msg_type = DebugMessageFlag::GENERIC_NOTICE;
                    "Warning: Failed to decode extended event text."
                );
                // Fallback to raw text as UTF-8 lossy
                event.extended_text = Some(String::from_utf8_lossy(&combined_text).to_string());
            }
        }
    } else {
        // Store raw bytes as string for continuation
        event.extended_text = Some(String::from_utf8_lossy(&combined_text).to_string());
    }
}
/// Decode an ATSC multiple_string
/// Extremely basic implementation
/// Only handles single segment, single language ANSI string!
pub fn epg_atsc_decode_multiple_string(offset: &[u8], length: u32, event: &mut EPGEventRust) {
    if length == 0 {
        return;
    }

    let mut pos = 0usize;
    let length = length as usize;

    macro_rules! check_offset {
        ($val:expr) => {
            if pos + $val >= length || pos + $val >= offset.len() {
                return;
            }
        };
    }

    check_offset!(1);
    let number_strings = offset[pos];
    pos += 1;

    for _i in 0..number_strings {
        check_offset!(4);
        let number_segments = offset[pos + 3];
        let iso_639_language_code = [
            offset[pos] as char,
            offset[pos + 1] as char,
            offset[pos + 2] as char,
        ];
        pos += 4;

        for j in 0..number_segments {
            check_offset!(3);
            let compression_type = offset[pos];
            let mode = offset[pos + 1];
            let number_bytes = offset[pos + 2] as usize;
            pos += 3;

            if mode == 0 && compression_type == 0 && j == 0 {
                check_offset!(number_bytes);
                event.has_simple = true;
                event.iso_639_language_code = format!(
                    "{}{}{}",
                    iso_639_language_code[0], iso_639_language_code[1], iso_639_language_code[2]
                );

                let text_slice = &offset[pos..pos + number_bytes];
                let text = String::from_utf8_lossy(text_slice).to_string();

                event.event_name = Some(text.clone());
                event.text = Some(text);
            } else {
                // Warning: Unsupported ATSC multiple_string encoding detected!
                info!("Warning: Unsupported ATSC multiple_string encoding detected!");
            }
            pos += number_bytes;
        }
    }
}

/// Decode ATSC EIT table
/// # Safety
/// This function is unsafe because it dereferences raw pointers and performs memory operations.
pub unsafe fn epg_atsc_decode_eit(
    ctx_ref: &mut lib_ccx_ctx,
    payload_start: *const u8,
    size: u32,
    ccx_options: &mut Options,
) {
    if payload_start.is_null() || size < 11 {
        return;
    }
    let payload_slice = std::slice::from_raw_parts(payload_start, size as usize);

    let source_id = ((payload_slice[3] as u16) << 8) | payload_slice[4] as u16;

    let mut event = EPGEventRust {
        has_simple: false,
        extended_text: None,
        ratings: Vec::new(),
        categories: Vec::new(),
        live_output: false,
        ..Default::default()
    };

    let mut pmt_map = -1i32;

    if !ctx_ref.demux_ctx.is_null() {
        let demux_ctx = &*ctx_ref.demux_ctx;
        for i in 0..demux_ctx.nb_program {
            let pinfo = &demux_ctx.pinfo[i as usize];
            // This is a placeholder for the mapping logic
            if pinfo.program_number == source_id as i32 {
                pmt_map = i;
                break;
            }
        }
    }

    // Don't know how to store EPG until we know the programs. Ignore it.
    if pmt_map == -1 {
        pmt_map = TS_PMT_MAP_SIZE as i32;
    }

    let num_events_in_section = payload_slice[9];
    let mut pos = 10usize;
    let mut has_new = false;

    macro_rules! check_offset {
        ($val:expr) => {
            if pos + $val >= size as usize {
                return;
            }
        };
    }

    for _j in 0..num_events_in_section {
        if pos >= size as usize {
            break;
        }

        check_offset!(10);

        let event_id = ((payload_slice[pos] & 0x3F) as u16) << 8 | payload_slice[pos + 1] as u16;
        let full_id = ((source_id as u32) << 16) | event_id as u32;
        event.id = full_id;
        event.service_id = source_id;

        let start_time = ((payload_slice[pos + 2] as u32) << 24)
            | ((payload_slice[pos + 3] as u32) << 16)
            | ((payload_slice[pos + 4] as u32) << 8)
            | (payload_slice[pos + 5] as u32);

        epg_atsc_calc_time(&mut event.start_time_string, start_time);

        let length_in_seconds = (((payload_slice[pos + 6] & 0x0F) as u32) << 16)
            | ((payload_slice[pos + 7] as u32) << 8)
            | (payload_slice[pos + 8] as u32);

        epg_atsc_calc_time(&mut event.end_time_string, start_time + length_in_seconds);

        let title_length = payload_slice[pos + 9] as u32;

        check_offset!(11 + title_length as usize);

        epg_atsc_decode_multiple_string(&payload_slice[pos + 10..], title_length, &mut event);

        let descriptors_loop_length =
            (((payload_slice[pos + 10 + title_length as usize] & 0x0f) as u16) << 8)
                | payload_slice[pos + 10 + title_length as usize + 1] as u16;

        has_new |= epg_add_event(ctx_ref, pmt_map, &event);
        pos += 12 + descriptors_loop_length as usize + title_length as usize;
    }

    if (ccx_options.xmltv == 1 || ccx_options.xmltv == 3)
        && ccx_options.xmltvoutputinterval.millis() == 0
        && has_new
    {
        epg_output(ctx_ref, ccx_options);
    }
}
/// Decode ATSC VCT table
/// # Safety
/// This function is unsafe because it dereferences raw pointers and performs memory operations.
pub unsafe fn epg_atsc_decode_vct(ctx: &mut lib_ccx_ctx, payload_start: *const u8, size: u32) {
    if payload_start.is_null() || size <= 10 {
        return;
    }

    let payload_slice = std::slice::from_raw_parts(payload_start, size as usize);
    let num_channels_in_section = payload_slice[9];
    let mut offset_pos = 10usize;

    for _i in 0..num_channels_in_section {
        if offset_pos + 31 >= size as usize {
            break;
        }

        let program_number =
            ((payload_slice[offset_pos + 24] as u16) << 8) | payload_slice[offset_pos + 25] as u16;
        let source_id =
            ((payload_slice[offset_pos + 28] as u16) << 8) | payload_slice[offset_pos + 29] as u16;
        let descriptors_loop_length = (((payload_slice[offset_pos + 30] & 0x03) as u16) << 8)
            | payload_slice[offset_pos + 31] as u16;

        // Copy short_name (7 * 2 = 14 bytes)
        let mut short_name = [0u8; 14];
        short_name.copy_from_slice(&payload_slice[offset_pos..offset_pos + 14]);

        offset_pos += 32 + descriptors_loop_length as usize;

        // Store in ATSC source program map
        if !ctx.ATSC_source_pg_map.is_null() {
            *ctx.ATSC_source_pg_map.offset(source_id as isize) = program_number as i16;
        }
    }
}

/// Decode DVB EIT table
/// # Safety
/// This function is unsafe because it dereferences raw pointers, performs memory operations and calls unsafe functions like epg_add_event.
pub unsafe fn epg_dvb_decode_eit(
    ctx_ref: &mut lib_ccx_ctx,
    payload_start: *const u8,
    size: u32,
    ccx_options: &mut Options,
) {
    if payload_start.is_null() || size < 13 {
        return;
    }

    let payload_slice = std::slice::from_raw_parts(payload_start, size as usize);

    let table_id = payload_slice[0];
    let section_length = (((payload_slice[1] & 0x0F) as u16) << 8) | payload_slice[2] as u16;
    let service_id = ((payload_slice[3] as u16) << 8) | payload_slice[4] as u16;
    let section_number = payload_slice[6];
    let events_length = section_length - 11;

    let mut pmt_map = -1i32;
    let mut has_new = false;

    // Find PMT map
    if !ctx_ref.demux_ctx.is_null() {
        let demux_ctx = &*ctx_ref.demux_ctx;
        for i in 0..demux_ctx.nb_program {
            let pinfo = &demux_ctx.pinfo[i as usize];
            if pinfo.program_number == service_id as i32 {
                pmt_map = i;
                break;
            }
        }
    }

    // For any service we don't have a PMT for (yet), store it in the special last array pos
    if pmt_map == -1 {
        pmt_map = TS_PMT_MAP_SIZE as i32;
    }

    if events_length as u32 > size - 14 {
        info!("Warning: Invalid EIT packet size detected.");
        return;
    }

    let mut offset_pos = 14usize;
    let mut remaining = events_length as usize;

    while remaining > 4 && offset_pos + 12 <= size as usize {
        let mut event = EPGEventRust {
            id: ((payload_slice[offset_pos] as u32) << 8) | payload_slice[offset_pos + 1] as u32,
            has_simple: false,
            extended_text: None,
            ratings: Vec::new(),
            categories: Vec::new(),
            live_output: false,
            service_id,
            ..Default::default()
        };

        // Extract start time (40 bits)
        let start_time = ((payload_slice[offset_pos + 2] as u64) << 32)
            | ((payload_slice[offset_pos + 3] as u64) << 24)
            | ((payload_slice[offset_pos + 4] as u64) << 16)
            | ((payload_slice[offset_pos + 5] as u64) << 8)
            | (payload_slice[offset_pos + 6] as u64);

        epg_dvb_calc_start_time(&mut event, start_time);

        // Extract duration (24 bits)
        let duration = ((payload_slice[offset_pos + 7] as u32) << 16)
            | ((payload_slice[offset_pos + 8] as u32) << 8)
            | (payload_slice[offset_pos + 9] as u32);

        epg_dvb_calc_end_time(&mut event, start_time, duration);

        event.running_status = (payload_slice[offset_pos + 10] & 0xE0) >> 5;
        event.free_ca_mode = (payload_slice[offset_pos + 10] & 0x10) >> 4;

        // Extract descriptors loop length (12 bits)
        let descriptors_loop_length = (((payload_slice[offset_pos + 10] & 0x0f) as u16) << 8)
            | payload_slice[offset_pos + 11] as u16;

        if descriptors_loop_length as usize > remaining - 12 {
            info!("Warning: Invalid EIT descriptors_loop_length detected.");
            return;
        }

        let mut desc_pos = offset_pos + 12;
        let desc_end = desc_pos + descriptors_loop_length as usize;

        // Process descriptors
        while desc_pos < desc_end && desc_pos + 1 < size as usize {
            if desc_pos + payload_slice[desc_pos + 1] as usize + 2 > size as usize {
                info!("Warning: Invalid EIT descriptor_loop_length detected.");
                return;
            }

            let descriptor_tag = payload_slice[desc_pos];
            let descriptor_length = payload_slice[desc_pos + 1];

            if descriptor_length + 2 == 0 {
                info!("Warning: Invalid EIT descriptor_length detected.");
                return;
            }

            match descriptor_tag {
                0x4d => {
                    // Short event descriptor
                    epg_decode_short_event_descriptor(
                        &payload_slice[desc_pos + 2..],
                        descriptor_length as u32,
                        &mut event,
                    );
                }
                0x4e => {
                    // Extended event descriptor
                    epg_decode_extended_event_descriptor(
                        &payload_slice[desc_pos + 2..],
                        descriptor_length as u32,
                        &mut event,
                    );
                }
                0x54 => {
                    // Content descriptor
                    epg_decode_content_descriptor(
                        &payload_slice[desc_pos + 2..],
                        descriptor_length as u32,
                        &mut event,
                    );
                }
                0x55 => {
                    // Parental rating descriptor
                    epg_decode_parental_rating_descriptor(
                        &payload_slice[desc_pos + 2..],
                        descriptor_length as u32,
                        &mut event,
                    );
                }
                _ => {}
            }

            desc_pos += descriptor_length as usize + 2;
        }

        remaining -= descriptors_loop_length as usize + 12;
        offset_pos += descriptors_loop_length as usize + 12;

        has_new |= epg_add_event(ctx_ref, pmt_map, &event);

        if has_new
            && section_number == 0
            && table_id == 0x4e
            && !ctx_ref.eit_current_events.is_null()
        {
            *ctx_ref.eit_current_events.offset(pmt_map as isize) = event.id as i32;
        }
    }

    // Output EPG if conditions are met
    if has_new
        && (ccx_options.xmltv == 1 || ccx_options.xmltv == 3)
        && ccx_options.xmltvoutputinterval.millis() == 0
    {
        epg_output(ctx_ref, ccx_options);
    }
}
/// Handle outputting to XML files
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe functions like epg_output_net.
pub unsafe fn epg_handle_output(ctx_ref: &mut lib_ccx_ctx, ccx_options: &mut Options) {
    if ctx_ref.demux_ctx.is_null() {
        return;
    }

    let demux_ctx = &*ctx_ref.demux_ctx;
    let cur_sec = ((demux_ctx.global_timestamp - demux_ctx.min_global_timestamp) / 1000) as i32;

    // Full output
    if (ccx_options.xmltv == 1 || ccx_options.xmltv == 3)
        && ccx_options.xmltvoutputinterval.millis() != 0
        && cur_sec > ctx_ref.epg_last_output + ccx_options.xmltvliveinterval.millis() as i32
    {
        ctx_ref.epg_last_output = cur_sec;
        epg_output(ctx_ref, ccx_options);
    }

    // Live output
    if (ccx_options.xmltv == 2 || ccx_options.xmltv == 3 || ccx_options.send_to_srv)
        && cur_sec > ctx_ref.epg_last_live_output + ccx_options.xmltvliveinterval.millis() as i32
    {
        ctx_ref.epg_last_live_output = cur_sec;
        if ccx_options.send_to_srv {
            epg_output_net(ctx_ref, ccx_options);
        } else {
            epg_output_live(ctx_ref);
        }
    }
}

/// Determine table type and call the correct function to handle it
/// # Safety
/// This function is unsafe because it dereferences raw pointers and performs memory operations.
pub unsafe fn epg_parse_table(
    ctx: &mut lib_ccx_ctx,
    b: *const u8,
    size: u32,
    ccx_options: &mut Options,
) {
    if b.is_null() || size < 2 {
        return;
    }

    let buffer_slice = std::slice::from_raw_parts(b, size as usize);
    let pointer_field = buffer_slice[0];

    // XXX hack, should accumulate data
    if pointer_field as u32 + 2 > size {
        return;
    }

    let payload_start_offset = (pointer_field + 1) as usize;
    if payload_start_offset >= size as usize {
        return;
    }

    let payload_start = b.add(payload_start_offset);
    let remaining_size = size - payload_start_offset as u32;

    if remaining_size == 0 {
        return;
    }

    let table_id = buffer_slice[payload_start_offset];

    match table_id {
        0x0cb => {
            epg_atsc_decode_eit(ctx, payload_start, remaining_size, ccx_options);
        }
        0xc8 => {
            epg_atsc_decode_vct(ctx, payload_start, remaining_size);
        }
        _ => {
            if (0x4e..=0x6f).contains(&table_id) {
                epg_dvb_decode_eit(ctx, payload_start, remaining_size, ccx_options);
            }
        }
    }

    epg_handle_output(ctx, ccx_options);
}
/// Reconstructs DVB EIT and ATSC tables
/// # Safety
/// This function is unsafe because it dereferences raw pointers and performs memory operations.
pub unsafe fn parse_epg_packet(ctx: &mut lib_ccx_ctx, tspacket: &[u8; 188], options: &mut Options) {
    let mut payload_start = tspacket.as_ptr().offset(4);
    let mut payload_length = 188 - 4;
    let payload_start_indicator = (tspacket[1] & 0x40) >> 6;
    // let transport_priority = (tspacket[1] & 0x20) >> 5;
    let pid = (((tspacket[1] & 0x1F) as u16) << 8 | tspacket[2] as u16) & 0x1FFF;
    // let transport_scrambling_control = (tspacket[3] & 0xC0) >> 6;
    let adaptation_field_control = (tspacket[3] & 0x30) >> 4;
    let ccounter = tspacket[3] & 0xF;
    let adaptation_field_length;
    let mut buffer_map = 0xfff;
    if adaptation_field_control & 2 != 0 {
        adaptation_field_length = tspacket[4];
        payload_start = payload_start.offset(adaptation_field_length as isize + 1);
        payload_length = (tspacket.as_ptr().offset(188) as usize - payload_start as usize) as u32;
    }

    if (pid != 0x12 && pid != 0x1ffb && pid < 0x1000) || pid == 0x1fff {
        return;
    }

    if pid != 0x12 {
        buffer_map = (pid - 0x1000) as usize;
    }

    // Get reference to the EPG buffer
    let epg_buffer = &mut *ctx.epg_buffers.add(buffer_map);

    if payload_start_indicator != 0 {
        if epg_buffer.ccounter > 0 {
            epg_buffer.ccounter = 0;
            epg_parse_table(ctx, epg_buffer.buffer, epg_buffer.buffer_length, options);
        }

        epg_buffer.prev_ccounter = ccounter as u32;

        if !epg_buffer.buffer.is_null() {
            // Free using Rust's allocator
            let layout = Layout::from_size_align_unchecked(epg_buffer.buffer_length as usize, 1);
            dealloc(epg_buffer.buffer, layout);
        }

        // Allocate using Rust's allocator
        let layout = Layout::from_size_align_unchecked(payload_length as usize, 1);
        epg_buffer.buffer = alloc(layout);

        // CHECK FOR ALLOCATION FAILURE
        if epg_buffer.buffer.is_null() {
            debug!(
                msg_type = DebugMessageFlag::GENERIC_NOTICE;
                "\rError: Failed to allocate memory for EPG buffer.\n\0"
            );
            return;
        }

        ptr::copy_nonoverlapping(payload_start, epg_buffer.buffer, payload_length as usize);
        epg_buffer.buffer_length = payload_length;
        epg_buffer.ccounter += 1;
    } else if ccounter as u32 == epg_buffer.prev_ccounter + 1
        || (epg_buffer.prev_ccounter == 0x0f && ccounter == 0)
    {
        epg_buffer.prev_ccounter = ccounter as u32;

        // Reallocate using Rust's allocator
        let old_layout = Layout::from_size_align_unchecked(epg_buffer.buffer_length as usize, 1);
        let new_size = epg_buffer.buffer_length + payload_length;
        let new_buffer = realloc(epg_buffer.buffer, old_layout, new_size as usize);

        // CHECK FOR REALLOCATION FAILURE
        if new_buffer.is_null() {
            debug!(
                msg_type = DebugMessageFlag::GENERIC_NOTICE;
                "\rError: Failed to reallocate memory for EPG buffer.\n\0"
            );
            return;
        }

        epg_buffer.buffer = new_buffer;

        ptr::copy_nonoverlapping(
            payload_start,
            epg_buffer.buffer.offset(epg_buffer.buffer_length as isize),
            payload_length as usize,
        );
        epg_buffer.ccounter += 1;
        epg_buffer.buffer_length += payload_length;
    } else {
        debug!(
            msg_type = DebugMessageFlag::GENERIC_NOTICE;
            "\rWarning: Out of order EPG packets detected.\n\0"
        );
    }
}
