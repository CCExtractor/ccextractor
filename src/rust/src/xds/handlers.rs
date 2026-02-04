//! XDS (Extended Data Services) handler functions for processing extended data packets.
//!
//! This module provides functions for handling XDS packets including content advisory,
//! copy generation management, program information, channel data, and miscellaneous metadata.
//!
//! # Conversion Guide
//!
//! | C (ccx_decoders_xds.c)                    | Rust (handlers.rs)                                    |
//! |-------------------------------------------|-------------------------------------------------------|
//! | `write_xds_string`                        | [`write_xds_string`]                                  |
//! | `xdsprint`                                | [`xdsprint`]                                          |
//! | `xds_do_copy_generation_management_system`| [`xds_do_copy_generation_management_system`]          |
//! | `xds_do_content_advisory`                 | [`xds_do_content_advisory`]                           |
//! | `xds_do_current_and_future`               | [`xds_do_current_and_future`]                         |
//! | `xds_do_channel`                          | [`xds_do_channel`]                                    |
//! | `xds_do_misc`                             | [`xds_do_misc`]                                       |
//! | `xds_do_private_data`                     | [`xds_do_private_data`]                               |
//! | `do_end_of_xds`                           | [`do_end_of_xds`]                                     |
//! | `xds_debug_test`                          | [`xds_debug_test`]                                    |
//! | `xds_cea608_test`                         | [`xds_cea608_test`]                                   |
//! | `XDS_TYPE_*` defines                      | [`XdsType`] enum or constants                         |
//! | `XDS_CLASS_*` defines                     | [`XdsClass`] enum                                     |
//! | `ts_start_of_xds` global                  | Parameter or context field                            |
//! | `last_c1`, `last_c2` statics              | Context fields or function-local state                |
//! | `cc_subtitle` struct                      | [`cc_subtitle`] (C binding)                           |
//! | `eia608_screen` struct                    | [`eia608_screen`] (C binding)                         |
//! | `SFORMAT_XDS`                             | [`SFORMAT_XDS`] constant                              |
//! | `get_fts(ctx->timing, 2)`                 | [`TimingContext::get_fts`]                            |

#[allow(clippy::all)]
pub mod bindings {
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}

use std::ffi::CString;
use std::os::raw::c_void;
use std::ptr::null_mut;
use std::sync::atomic::{AtomicI64, Ordering};
use std::sync::Mutex;

use crate::bindings::{cc_subtitle, eia608_screen, realloc};

use lib_ccxr::debug;
use lib_ccxr::info;
use lib_ccxr::time::c_functions::get_fts;
use lib_ccxr::time::CaptionField;
use lib_ccxr::util::log::{hex_dump, send_gui, DebugMessageFlag, GuiXdsMessage};

use libc::free;
use libc::malloc;

use crate::xds::types::*;

pub static TS_START_OF_XDS: AtomicI64 = AtomicI64::new(-1); // Time at which we switched to XDS mode, =-1 hasn't happened yet
                                                            // Usage example:
                                                            // TS_START_OF_XDS.store(new_value, Ordering::SeqCst);
                                                            // let value = TS_START_OF_XDS.load(Ordering::SeqCst);

/// Represents failures during XDS string writing or processing.
pub enum XDSError {
    Err,
}

/// Writes an XDS string to the subtitle output buffer.
///
/// # Safety
/// - `sub.data` must be a valid pointer previously allocated by C's malloc/realloc, or null.
/// - The caller must ensure `sub` and `ctx` remain valid for the duration of the call.
/// - The returned `xds_str` pointer in the screen data must be freed with `CString::from_raw`.
pub unsafe fn write_xds_string(
    sub: &mut cc_subtitle,
    ctx: &mut CcxDecodersXdsContext,
    p: String,
    ts_start_of_xds: i64,
) -> Result<(), XDSError> {
    let new_size = (sub.nb_data + 1) as usize * size_of::<eia608_screen>();
    let new_data =
        unsafe { realloc(sub.data as *mut c_void, new_size as u64) as *mut eia608_screen };
    if new_data.is_null() {
        freep(&mut sub.data);
        sub.nb_data = 0;
        info!("No Memory left");
        return Err(XDSError::Err);
    }
    sub.data = new_data as *mut c_void;
    sub.datatype = 0;
    let data_element = &mut *new_data.add(sub.nb_data as usize);
    let c_str = CString::new(p).map_err(|_| XDSError::Err)?;

    let len = c_str.as_bytes().len(); // length w/o null terminator - matching C's vsnprintf return
    let alloc_len = len + 1; // allocate space for null terminator
    let ptr = unsafe { malloc(alloc_len) as *mut i8 }; // fixing c/rust mem mismatch
    if ptr.is_null() {
        return Err(XDSError::Err);
    }
    unsafe {
        std::ptr::copy_nonoverlapping(c_str.as_ptr(), ptr, alloc_len); // copy including null terminator
    }

    data_element.format = 2;
    data_element.start_time = ts_start_of_xds;
    if let Some(timing) = ctx.timing.as_mut() {
        data_element.end_time = get_fts(timing, CaptionField::Field2).millis();
    }
    data_element.xds_str = ptr;
    data_element.xds_len = len;
    data_element.cur_xds_packet_class = ctx.cur_xds_packet_class;
    sub.nb_data += 1;
    sub.type_ = 1;
    sub.got_output = 1;
    Ok(())
}

/// Prints an XDS message to the subtitle output if XDS file writing is enabled.
///
/// # Safety
/// - Same safety requirements as `write_xds_string`.
/// - Relies on the global `TS_START_OF_XDS` atomic value being correctly set.
pub unsafe fn xdsprint(
    sub: &mut cc_subtitle,
    ctx: &mut CcxDecodersXdsContext,
    message: String,
) -> Result<(), XDSError> {
    if !ctx.xds_write_to_file {
        return Ok(());
    }

    write_xds_string(sub, ctx, message, TS_START_OF_XDS.load(Ordering::SeqCst))
}

/// Frees a pointer and sets it to null.
///
/// Converts the raw pointer back into a `Box` to deallocate the memory,
/// then sets the original pointer to null to prevent use-after-free.
///
/// # Safety
/// - The pointer must have been originally allocated by Rust's `Box::into_raw`
///   or equivalent. Using this with C-allocated memory will cause undefined behavior.
/// - The pointer must not be used after this call.
pub fn freep<T>(ptr: &mut *mut T) {
    unsafe {
        if !ptr.is_null() {
            free(*ptr as *mut libc::c_void);
            *ptr = null_mut();
        }
    }
}

/// Utility methods for XDS buffer management and process_xds_bytes (function).
impl CcxDecodersXdsContext<'_> {
    /// Count how many XDS buffers are currently in use
    fn how_many_used(&self) -> usize {
        self.xds_buffers
            .iter()
            .filter(|buf| buf.in_use != 0)
            .count()
    }
}

/// XDS byte processing and packet handling.
impl CcxDecodersXdsContext<'_> {
    pub(crate) fn process_xds_bytes(&mut self, hi: u8, lo: u8) {
        if (0x01..=0x0f).contains(&hi) {
            let xds_class = ((hi - 1) / 2) as i32; // Start codes 1 and 2 are "class type" 0, 3-4 are 2, and so on.
            let is_new = !hi.is_multiple_of(2); // Start codes are even

            log::debug!(
                "XDS Start: {}.{}  Is new: {}  | Class: {} ({}), Used buffers: {}",
                hi,
                lo,
                is_new,
                xds_class,
                XDS_CLASSES.get(xds_class as usize).unwrap_or(&"Unknown"),
                self.how_many_used()
            );

            let mut first_free_buf = -1i32;
            let mut matching_buf = -1i32;

            for i in 0..NUM_XDS_BUFFERS {
                if self.xds_buffers[i].in_use != 0
                    && self.xds_buffers[i]
                    .xds_class
                    .map(|c| c.to_c_int())
                    .unwrap_or(-1)
                    == xds_class
                    && self.xds_buffers[i]
                    .xds_type
                    .map(|t| t.to_c_int())
                    .unwrap_or(-1)
                    == lo as i32
                {
                    matching_buf = i as i32;
                    break;
                }
                if first_free_buf == -1 && self.xds_buffers[i].in_use == 0 {
                    first_free_buf = i as i32;
                }
            }

            /* Here, 3 possibilities:
              1) We already had a buffer for this class/type and matching_buf points to it
              2) We didn't have a buffer for this class/type and first_free_buf points to an unused one
              3) All buffers are full and we will have to skip this packet.
            */
            if matching_buf == -1 && first_free_buf == -1 {
                log::info!(
                    "Note: All XDS buffers full (bug or suicidal stream). Ignoring this one ({},{}).",
                    xds_class, lo
                );
                self.cur_xds_buffer_idx = -1;
                return;
            }

            self.cur_xds_buffer_idx = if matching_buf != -1 {
                matching_buf
            } else {
                first_free_buf
            };
            let idx = self.cur_xds_buffer_idx as usize;

            if is_new || self.xds_buffers[idx].in_use == 0 {
                // Whatever we had before we discard; must belong to an interrupted packet
                self.xds_buffers[idx].xds_class = XdsClass::from_c_int(xds_class);
                self.xds_buffers[idx].xds_type =
                    XdsType::from_c_int(self.xds_buffers[idx].xds_class, lo as i32);
                self.xds_buffers[idx].used_bytes = 0;
                self.xds_buffers[idx].in_use = 1;
                self.xds_buffers[idx].bytes = [0; NUM_BYTES_PER_PACKET];
            }

            if !is_new {
                // Continue codes aren't added to packet.
                return;
            }
        } else {
            // Informational: 00, or 0x20-0x7F, so 01-0x1f forbidden
            log::debug!(
                "XDS: {:02X}.{:02X} ({}, {})",
                hi,
                lo,
                hi as char,
                lo as char
            );

            if (hi > 0 && hi <= 0x1f) || (lo > 0 && lo <= 0x1f) {
                log::info!("\rNote: Illegal XDS data");
                return;
            }
        }

        if self.cur_xds_buffer_idx >= 0 {
            let idx = self.cur_xds_buffer_idx as usize;
            if idx < NUM_XDS_BUFFERS && (self.xds_buffers[idx].used_bytes as usize) <= 32 {
                let pos = self.xds_buffers[idx].used_bytes as usize;
                self.xds_buffers[idx].bytes[pos] = hi;
                self.xds_buffers[idx].bytes[pos + 1] = lo;
                self.xds_buffers[idx].used_bytes += 2;
                if (self.xds_buffers[idx].used_bytes as usize) < NUM_BYTES_PER_PACKET {
                    self.xds_buffers[idx].bytes[self.xds_buffers[idx].used_bytes as usize] = 0;
                }
            }
        }
    }
}

/// State for CGMS (Copy Generation Management System) caching
struct CgmsState {
    last_c1: u32,
    last_c2: u32,
    copy_permitted: String,
    aps: String,
    rcd: String,
}

impl Default for CgmsState {
    fn default() -> Self {
        CgmsState {
            last_c1: u32::MAX, // equivalent to -1 for unsigned in C
            last_c2: u32::MAX,
            copy_permitted: String::new(),
            aps: String::new(),
            rcd: String::new(),
        }
    }
}

/// Cached state for Copy Generation Management System (CGMS) to detect changes
static CGMS_STATE: Mutex<CgmsState> = Mutex::new(CgmsState {
    last_c1: u32::MAX,
    last_c2: u32::MAX,
    copy_permitted: String::new(),
    aps: String::new(),
    rcd: String::new(),
});

/// Copy Generation Management System text descriptions
const COPY_TEXT: [&str; 4] = [
    "Copy permitted (no restrictions)",
    "No more copies (one generation copy has been made)",
    "One generation of copies can be made",
    "No copying is permitted",
];

/// APS (Analog Protection System) mode descriptions
const APS_TEXT: [&str; 4] = [
    "No APS",
    "PSP On; Split Burst Off",
    "PSP On; 2 line Split Burst On",
    "PSP On; 4 line Split Burst On",
];

/// Decodes and outputs Copy Generation Management System (CGMS) data from XDS packets.
///
/// # Safety
/// - `sub` must be a valid, non-null pointer to a `cc_subtitle` struct
/// - `ctx` must be a valid, non-null pointer to a `ccx_decoders_xds_context` struct
/// - Both pointers must remain valid for the duration of the call
/// - The caller must ensure proper synchronization if accessed from multiple threads
pub unsafe fn xds_do_copy_generation_management_system(
    sub: &mut crate::bindings::cc_subtitle,
    ctx: &mut CcxDecodersXdsContext,
    c1: u32,
    c2: u32,
) {
    // Extract bit fields from c1
    let c1_6 = (c1 & 0x40) >> 6;
    // let _unused1 = (c1 & 0x20) >> 5;  // unused in original
    let cgms_a_b4 = (c1 & 0x10) >> 4;
    let cgms_a_b3 = (c1 & 0x08) >> 3;
    let aps_b2 = (c1 & 0x04) >> 2;
    let aps_b1 = (c1 & 0x02) >> 1;
    // let _asb_0 = c1 & 0x01;  // unused in original

    // Extract bit fields from c2
    let c2_6 = (c2 & 0x40) >> 6;
    // let _c2_5 = (c2 & 0x20) >> 5;  // unused
    // let _c2_4 = (c2 & 0x10) >> 4;  // unused
    // let _c2_3 = (c2 & 0x08) >> 3;  // unused
    // let _c2_2 = (c2 & 0x04) >> 2;  // unused
    // let _c2_1 = (c2 & 0x02) >> 1;  // unused
    let rcd0 = c2 & 0x01;

    // These bits must be high per specs
    if c1_6 == 0 || c2_6 == 0 {
        return;
    }

    let mut state = match CGMS_STATE.lock() {
        Ok(s) => s,
        Err(_) => return,
    };

    let changed = if state.last_c1 != c1 || state.last_c2 != c2 {
        state.last_c1 = c1;
        state.last_c2 = c2;

        // Decode CGMS-A copy protection
        let copy_idx = (cgms_a_b4 * 2 + cgms_a_b3) as usize;
        state.copy_permitted = format!("CGMS: {}", COPY_TEXT[copy_idx]);

        // Decode APS (Analog Protection System)
        let aps_idx = (aps_b2 * 2 + aps_b1) as usize;
        state.aps = format!("APS: {}", APS_TEXT[aps_idx]);

        // Decode RCD (Redistribution Control Descriptor)
        state.rcd = format!("Redistribution Control Descriptor: {}", rcd0);

        true
    } else {
        false
    };

    // Output via xdsprint
    let _ = xdsprint(sub, ctx, state.copy_permitted.clone());
    let _ = xdsprint(sub, ctx, state.aps.clone());
    let _ = xdsprint(sub, ctx, state.rcd.clone());

    // Log if changed
    if changed {
        info!("\rXDS: {}\n", state.copy_permitted);
        info!("\rXDS: {}\n", state.aps);
        info!("\rXDS: {}\n", state.rcd);
    }

    // Debug output (always, when debug mask matches)
    debug!(msg_type = DebugMessageFlag::DECODER_XDS; "\rXDS: {}\n", state. copy_permitted);
    debug!(msg_type = DebugMessageFlag::DECODER_XDS; "\rXDS: {}\n", state. aps);
    debug!(msg_type = DebugMessageFlag::DECODER_XDS; "\rXDS: {}\n", state.rcd);
}

/// State for Content Advisory caching
struct ContentAdvisoryState {
    last_c1: u32,
    last_c2: u32,
    age: String,
    content: String,
    rating: String,
}

impl Default for ContentAdvisoryState {
    fn default() -> Self {
        ContentAdvisoryState {
            last_c1: u32::MAX,
            last_c2: u32::MAX,
            age: String::new(),
            content: String::new(),
            rating: String::new(),
        }
    }
}

/// Cached state for Content Advisory to detect changes
static CONTENT_ADVISORY_STATE: Mutex<ContentAdvisoryState> = Mutex::new(ContentAdvisoryState {
    last_c1: u32::MAX,
    last_c2: u32::MAX,
    age: String::new(),
    content: String::new(),
    rating: String::new(),
});

/// US TV Parental Guidelines age ratings
const US_TV_AGE_TEXT: [&str; 8] = [
    "None",
    "TV-Y (All Children)",
    "TV-Y7 (Older Children)",
    "TV-G (General Audience)",
    "TV-PG (Parental Guidance Suggested)",
    "TV-14 (Parents Strongly Cautioned)",
    "TV-MA (Mature Audience Only)",
    "None",
];

/// MPA rating text
const MPA_RATING_TEXT: [&str; 8] = ["N/A", "G", "PG", "PG-13", "R", "NC-17", "X", "Not Rated"];

/// Canadian English Language Rating
const CANADIAN_ENGLISH_RATING_TEXT: [&str; 8] = [
    "Exempt",
    "Children",
    "Children eight years and older",
    "General programming suitable for all audiences",
    "Parental Guidance",
    "Viewers 14 years and older",
    "Adult Programming",
    "[undefined]",
];

/// Canadian French Language Rating
const CANADIAN_FRENCH_RATING_TEXT: [&str; 8] = [
    "Exempt?es",
    "G?n?ral",
    "G?n?ral - D?conseill? aux jeunes enfants",
    "Cette ?mission peut ne pas convenir aux enfants de moins de 13 ans",
    "Cette ?mission ne convient pas aux moins de 16 ans",
    "Cette ?mission est r?serv?e aux adultes",
    "[invalid]",
    "[invalid]",
];

/// Handles content advisory/rating information (US TV, MPA, Canadian ratings)
///
/// # Safety
/// - `sub` must be a valid, non-null pointer to a `cc_subtitle` struct
/// - `ctx` must be a valid, non-null pointer to a `ccx_decoders_xds_context` struct
/// - Both pointers must remain valid for the duration of the call
/// - The caller must ensure proper synchronization if accessed from multiple threads
pub unsafe fn xds_do_content_advisory(
    sub: &mut crate::bindings::cc_subtitle,
    ctx: &mut CcxDecodersXdsContext,
    c1: u32,
    c2: u32,
) {
    // Extract bit fields from c1 (insane encoding per original comment)
    let c1_6 = (c1 & 0x40) >> 6;
    let da2 = (c1 & 0x20) >> 5;
    let a1 = (c1 & 0x10) >> 4;
    let a0 = (c1 & 0x08) >> 3;
    let r2 = (c1 & 0x04) >> 2;
    let r1 = (c1 & 0x02) >> 1;
    let r0 = c1 & 0x01;

    // Extract bit fields from c2
    let c2_6 = (c2 & 0x40) >> 6;
    let fv = (c2 & 0x20) >> 5;
    let s = (c2 & 0x10) >> 4;
    let la3 = (c2 & 0x08) >> 3;
    let g2 = (c2 & 0x04) >> 2;
    let g1 = (c2 & 0x02) >> 1;
    let g0 = c2 & 0x01;

    // These bits must be high per specs
    if c1_6 == 0 || c2_6 == 0 {
        return;
    }

    let mut state = match CONTENT_ADVISORY_STATE.lock() {
        Ok(s) => s,
        Err(_) => return,
    };

    let mut changed = false;
    let mut supported = false;

    if state.last_c1 != c1 || state.last_c2 != c2 {
        changed = true;
        state.last_c1 = c1;
        state.last_c2 = c2;

        // Bits a1 and a0 determine the encoding
        if a1 == 0 && a0 != 0 {
            // US TV parental guidelines
            let age_idx = (g2 * 4 + g1 * 2 + g0) as usize;
            state.age = format!(
                "ContentAdvisory: US TV Parental Guidelines. Age Rating: {}",
                US_TV_AGE_TEXT[age_idx]
            );

            // Build content string
            let mut content_parts = Vec::new();

            if g2 == 0 && g1 != 0 && g0 == 0 {
                // For TV-Y7 (Older children), the Violence bit is "fantasy violence"
                if fv != 0 {
                    content_parts.push("[Fantasy Violence] ");
                }
            } else {
                // For all others, is real
                if fv != 0 {
                    content_parts.push("[Violence] ");
                }
            }

            if s != 0 {
                content_parts.push("[Sexual Situations] ");
            }
            if la3 != 0 {
                content_parts.push("[Adult Language] ");
            }
            if da2 != 0 {
                content_parts.push("[Sexually Suggestive Dialog] ");
            }

            state.content = content_parts.join(""); // "" used instead of " " : to keep xds terminal(stdout) output same as that of c code
            supported = true;
        }

        if a0 == 0 {
            // MPA
            let rating_idx = (r2 * 4 + r1 * 2 + r0) as usize;
            state.rating = format!(
                "ContentAdvisory: MPA Rating: {}",
                MPA_RATING_TEXT[rating_idx]
            );
            supported = true;
        }

        if a0 != 0 && a1 != 0 && da2 == 0 && la3 == 0 {
            // Canadian English Language Rating
            let rating_idx = (g2 * 4 + g1 * 2 + g0) as usize;
            state.rating = format!(
                "ContentAdvisory: Canadian English Rating: {}",
                CANADIAN_ENGLISH_RATING_TEXT[rating_idx]
            );
            supported = true;
        }

        if a0 != 0 && a1 != 0 && da2 != 0 && la3 == 0 {
            // Canadian French Language Rating
            let rating_idx = (g2 * 4 + g1 * 2 + g0) as usize;
            state.rating = format!(
                "ContentAdvisory: Canadian French Rating: {}",
                CANADIAN_FRENCH_RATING_TEXT[rating_idx]
            );
            supported = true;
        }
    }

    // Output based on encoding type
    // US TV parental guidelines
    if a1 == 0 && a0 != 0 {
        let _ = xdsprint(sub, ctx, state.age.clone());
        if !state.content.is_empty() {
            let _ = xdsprint(sub, ctx, state.content.clone());
        }

        if changed {
            info!("\rXDS: {}\n  ", state.age);
            info!("\rXDS: {}\n  ", state.content);
        }

        debug!(msg_type = DebugMessageFlag::DECODER_XDS; "\rXDS: {}\n", state.age);
        debug!(msg_type = DebugMessageFlag::DECODER_XDS; "\rXDS: {}\n", state.content);
    }

    // MPA, Canadian English, or Canadian French
    if a0 == 0 || (a1 != 0 && la3 == 0) {
        let _ = xdsprint(sub, ctx, state.rating.clone());

        if changed {
            info!("\rXDS: {}\n  ", state.rating);
        }

        debug!(msg_type = DebugMessageFlag::DECODER_XDS; "\rXDS: {}\n", state.rating);
    }

    if changed && !supported {
        info!("XDS: Unsupported ContentAdvisory encoding, please submit sample.\n");
    }
}

/// Handles current and future program info (PIN, length, name, type, descriptions)
///
/// # Safety
/// - `sub` must be a valid, non-null pointer to a `cc_subtitle` struct
/// - `ctx` must be a valid, non-null pointer to a `ccx_decoders_xds_context` struct
/// - `ctx.cur_xds_payload` must point to valid memory with at least `ctx.cur_xds_payload_length` bytes
/// - Both pointers must remain valid for the duration of the call
pub unsafe fn xds_do_current_and_future(
    sub: &mut crate::bindings::cc_subtitle,
    ctx: &mut CcxDecodersXdsContext,
) -> i32 {
    let mut was_proc = 0;

    // Safety check for payload
    if ctx.cur_xds_payload.is_null() || ctx.cur_xds_payload_length < 3 {
        return -1; // CCX_EINVAL equivalent
    }

    let payload =
        std::slice::from_raw_parts(ctx.cur_xds_payload, ctx.cur_xds_payload_length as usize);

    match ctx.cur_xds_packet_type {
        // XDS_TYPE_PIN_START_TIME = 1
        1 => {
            was_proc = 1;
            if ctx.cur_xds_payload_length < 7 {
                // We need 4 data bytes
                return was_proc;
            }

            let min = (payload[2] & 0x3f) as i32; // 6 bits
            let hour = (payload[3] & 0x1f) as i32; // 5 bits
            let date = (payload[4] & 0x1f) as i32; // 5 bits
            let month = (payload[5] & 0x0f) as i32; // 4 bits

            if ctx.current_xds_min != min
                || ctx.current_xds_hour != hour
                || ctx.current_xds_date != date
                || ctx.current_xds_month != month
            {
                ctx.xds_start_time_shown = 0;
                ctx.current_xds_min = min;
                ctx.current_xds_hour = hour;
                ctx.current_xds_date = date;
                ctx.current_xds_month = month;
            }

            // XDS_CLASS_CURRENT = 0
            let class_str = if ctx.cur_xds_packet_class == 0 {
                "Current"
            } else {
                "Future"
            };

            debug!(
                msg_type = DebugMessageFlag::DECODER_XDS;
                "PIN (Start Time): {}  {:02}-{:02} {:02}:{:02}\n",
                class_str, date, month, hour, min
            );

            let pin_msg = format!(
                "PIN (Start Time): {}  {:02}-{:02} {:02}:{:02}\n",
                class_str, date, month, hour, min
            );
            let _ = xdsprint(sub, ctx, pin_msg);

            // XDS_CLASS_CURRENT = 0
            if ctx.xds_start_time_shown == 0 && ctx.cur_xds_packet_class == 0 {
                info!("\rXDS: Program changed.\n");
                info!(
                    "XDS program start time (DD/MM HH:MM) {:02}-{:02} {:02}:{:02}\n",
                    date, month, hour, min
                );
                send_gui(GuiXdsMessage::ProgramIdNr {
                    minute: ctx.current_xds_min as u8,
                    hour: ctx.current_xds_hour as u8,
                    date: ctx.current_xds_date as u8,
                    month: ctx.current_xds_month as u8,
                });
                ctx.xds_start_time_shown = 1;
            }
        }

        // XDS_TYPE_LENGH_AND_CURRENT_TIME = 2
        2 => {
            was_proc = 1;
            if ctx.cur_xds_payload_length < 5 {
                // We need 2 data bytes
                return was_proc;
            }

            let min = (payload[2] & 0x3f) as i32; // 6 bits
            let hour = (payload[3] & 0x1f) as i32; // 5 bits

            if ctx.xds_program_length_shown == 0 {
                info!("\rXDS: Program length (HH:MM): {:02}:{:02}  ", hour, min);
            } else {
                debug!(
                    msg_type = DebugMessageFlag::DECODER_XDS;
                    "\rXDS: Program length (HH:MM): {:02}:{:02}  ", hour, min
                );
            }

            let length_msg = format!("Program length (HH:MM): {:02}:{:02}  ", hour, min);
            let _ = xdsprint(sub, ctx, length_msg);

            if ctx.cur_xds_payload_length > 6 {
                // Next two bytes (optional) available
                let el_min = (payload[4] & 0x3f) as i32; // 6 bits
                let el_hour = (payload[5] & 0x1f) as i32; // 5 bits

                if ctx.xds_program_length_shown == 0 {
                    info!("Elapsed (HH:MM): {:02}:{:02}", el_hour, el_min);
                } else {
                    debug!(
                        msg_type = DebugMessageFlag::DECODER_XDS;
                        "Elapsed (HH:MM): {:02}:{:02}", el_hour, el_min
                    );
                }

                let elapsed_msg = format!("Elapsed (HH:MM): {:02}:{:02}", el_hour, el_min);
                let _ = xdsprint(sub, ctx, elapsed_msg);
            }

            if ctx.cur_xds_payload_length > 8 {
                // Next two bytes (optional) available
                let el_sec = (payload[6] & 0x3f) as i32; // 6 bits

                if ctx.xds_program_length_shown == 0 {
                    debug!(
                        msg_type = DebugMessageFlag::DECODER_XDS;
                        ":{:02}", el_sec
                    );
                }

                let elapsed_sec_msg = format!("Elapsed (SS) :{:02}", el_sec);
                let _ = xdsprint(sub, ctx, elapsed_sec_msg);
            }

            if ctx.xds_program_length_shown == 0 {
                info!("\n");
            } else {
                debug!(msg_type = DebugMessageFlag::DECODER_XDS; "\n");
            }

            ctx.xds_program_length_shown = 1;
        }

        // XDS_TYPE_PROGRAM_NAME = 3
        3 => {
            was_proc = 1;

            // Extract program name from payload (bytes 2 to payload_length - 2)
            let name_end = (ctx.cur_xds_payload_length - 1) as usize;
            let name_slice = &payload[2..name_end];
            let name_bytes = match name_slice.iter().position(|&b| b == 0) {
                Some(pos) => &name_slice[..pos],
                None => name_slice,
            };

            let xds_program_name = String::from_utf8_lossy(name_bytes).to_string();

            debug!(
                msg_type = DebugMessageFlag::DECODER_XDS;
                "\rXDS Program name: {}\n", xds_program_name
            );

            let name_msg = format!("Program name: {}", xds_program_name);
            let _ = xdsprint(sub, ctx, name_msg);

            // Convert current_xds_program_name from i8 array to String for comparison
            let current_name = i8_array_to_string(&ctx.current_xds_program_name);

            // XDS_CLASS_CURRENT = 0
            if ctx.cur_xds_packet_class == 0 && xds_program_name != current_name {
                info!("\rXDS Notice: Program is now {}\n", xds_program_name);
                string_to_i8_array(&xds_program_name, &mut ctx.current_xds_program_name);
                send_gui(GuiXdsMessage::ProgramName(&xds_program_name));
            }
        }

        // XDS_TYPE_PROGRAM_TYPE = 4
        4 => {
            was_proc = 1;
            if ctx.cur_xds_payload_length < 5 {
                // We need 2 data bytes
                return was_proc;
            }

            if ctx.current_program_type_reported != 0 {
                // Check if we should do it again

                let should_report = payload
                    .iter()
                    .zip(ctx.current_xds_program_type.iter())
                    .take(33.min(ctx.cur_xds_payload_length as usize))
                    .any(|(p, c)| *p != *c as u8);

                if should_report {
                    ctx.current_program_type_reported = 0;
                }
            }

            // Copy payload to current_xds_program_type
            for (dst, src) in ctx
                .current_xds_program_type
                .iter_mut()
                .zip(payload.iter())
                .take(std::cmp::min(ctx.cur_xds_payload_length as usize, 32))
            {
                *dst = *src as i8;
            }
            if (ctx.cur_xds_payload_length as usize) < 33 {
                ctx.current_xds_program_type[ctx.cur_xds_payload_length as usize] = 0;
            }

            if ctx.current_program_type_reported == 0 {
                info!("\rXDS Program Type: ");
            }

            let mut type_str = String::new();

            for &byte in payload
                .iter()
                .take((ctx.cur_xds_payload_length - 1) as usize)
                .skip(2)
            {
                if byte == 0 {
                    continue;
                }

                if ctx.current_program_type_reported == 0 {
                    info!("[{:02X}-", byte);
                }

                if (0x20..0x7F).contains(&byte) {
                    let type_idx = (byte - 0x20) as usize;
                    if type_idx < XDS_PROGRAM_TYPES.len() {
                        type_str.push_str(&format!("[{}] ", XDS_PROGRAM_TYPES[type_idx]));
                    }
                }

                if ctx.current_program_type_reported == 0 {
                    if (0x20..0x7F).contains(&byte) {
                        let type_idx = (byte - 0x20) as usize;
                        if type_idx < XDS_PROGRAM_TYPES.len() {
                            info!("{}", XDS_PROGRAM_TYPES[type_idx]);
                        }
                    } else {
                        info!("ILLEGAL VALUE");
                    }
                    info!("] ");
                }
            }

            let type_msg = format!("Program type {}", type_str);
            let _ = xdsprint(sub, ctx, type_msg);

            if ctx.current_program_type_reported == 0 {
                info!("\n");
            }

            ctx.current_program_type_reported = 1;
        }

        // XDS_TYPE_CONTENT_ADVISORY = 5
        5 => {
            was_proc = 1;
            if ctx.cur_xds_payload_length < 5 {
                // We need 2 data bytes
                return was_proc;
            }
            xds_do_content_advisory(sub, ctx, payload[2] as u32, payload[3] as u32);
        }

        // XDS_TYPE_AUDIO_SERVICES = 6
        6 => {
            was_proc = 1;
            // No sample available - nothing to do
        }

        // XDS_TYPE_CGMS = 8
        8 => {
            was_proc = 1;
            xds_do_copy_generation_management_system(
                sub,
                ctx,
                payload[2] as u32,
                payload[3] as u32,
            );
        }

        // XDS_TYPE_ASPECT_RATIO_INFO = 9
        9 => {
            was_proc = 1;
            if ctx.cur_xds_payload_length < 5 {
                // We need 2 data bytes
                return was_proc;
            }

            // Bit 6 must be 1 (note: C code checks bit 5, 0x20 = bit 5)
            if (payload[2] & 0x20) == 0 || (payload[3] & 0x20) == 0 {
                return was_proc;
            }

            // CEA-608-B: The starting line is computed by adding 22 to the decimal number
            // represented by bits S0 to S5.  The ending line is computing by subtracting
            // the decimal number represented by bits E0 to E5 from 262
            let ar_start = ((payload[2] & 0x1F) as u32) + 22;
            let ar_end = 262 - ((payload[3] & 0x1F) as u32);
            let active_picture_height = ar_end - ar_start;
            let aspect_ratio = 320.0 / active_picture_height as f32;

            if ar_start != ctx.current_ar_start {
                ctx.current_ar_start = ar_start;
                ctx.current_ar_end = ar_end;
                info!(
                    "\rXDS Notice: Aspect ratio info, start line={}, end line={}\n",
                    ar_start, ar_end
                );
                info!(
                    "\rXDS Notice: Aspect ratio info, active picture height={}, ratio={}\n",
                    active_picture_height, aspect_ratio
                );
            } else {
                debug!(
                    msg_type = DebugMessageFlag::DECODER_XDS;
                    "\rXDS Notice: Aspect ratio info, start line={}, end line={}\n",
                    ar_start, ar_end
                );
                debug!(
                    msg_type = DebugMessageFlag::DECODER_XDS;
                    "\rXDS Notice: Aspect ratio info, active picture height={}, ratio={}\n",
                    active_picture_height, aspect_ratio
                );
            }
        }

        // XDS_TYPE_PROGRAM_DESC_1 through XDS_TYPE_PROGRAM_DESC_8 = 0x10-0x17
        0x10..=0x17 => {
            was_proc = 1;

            // Extract description from payload
            let desc_end = (ctx.cur_xds_payload_length - 1) as usize;
            let desc_slice = &payload[2..desc_end];
            let desc_bytes = match desc_slice.iter().position(|&b| b == 0) {
                Some(pos) => &desc_slice[..pos],
                None => desc_slice,
            };

            let xds_desc = String::from_utf8_lossy(desc_bytes).to_string();

            if !xds_desc.is_empty() {
                let line_num = ctx.cur_xds_packet_type - 0x10; // XDS_TYPE_PROGRAM_DESC_1 = 0x10

                // Get current description for this line
                let current_desc = if (line_num as usize) < 8 {
                    i8_array_to_string(&ctx.xds_program_description[line_num as usize])
                } else {
                    String::new()
                };

                let changed = xds_desc != current_desc;

                if changed {
                    info!("\rXDS description line {}: {}\n", line_num, xds_desc);
                    if (line_num as usize) < 8 {
                        string_to_i8_array(
                            &xds_desc,
                            &mut ctx.xds_program_description[line_num as usize],
                        );
                    }
                } else {
                    debug!(
                        msg_type = DebugMessageFlag::DECODER_XDS;
                        "\rXDS description line {}: {}\n", line_num, xds_desc
                    );
                }

                let desc_msg = format!("XDS description line {}: {}", line_num, xds_desc);
                let _ = xdsprint(sub, ctx, desc_msg);

                send_gui(GuiXdsMessage::ProgramDescription {
                    line_num,
                    desc: &xds_desc,
                });
            }
        }

        _ => {
            // Unknown packet type
        }
    }

    was_proc
}

/// Helper function to convert i8 array to String
fn i8_array_to_string(arr: &[i8]) -> String {
    let bytes: Vec<u8> = arr
        .iter()
        .take_while(|&&b| b != 0)
        .map(|&b| b as u8)
        .collect();
    String::from_utf8_lossy(&bytes).to_string()
}

/// Helper function to copy String into i8 array
fn string_to_i8_array(s: &str, arr: &mut [i8]) {
    let bytes = s.as_bytes();
    let len = std::cmp::min(bytes.len(), arr.len() - 1);
    for i in 0..len {
        arr[i] = bytes[i] as i8;
    }
    if len < arr.len() {
        arr[len] = 0;
    }
}

/// Processes channel-related XDS data (network name, call letters, TSID)
///
/// # Safety
/// - `sub` must be a valid, non-null pointer to a `cc_subtitle` struct
/// - `ctx` must be a valid, non-null pointer to a `ccx_decoders_xds_context` struct
/// - `ctx.cur_xds_payload` must point to valid memory with at least `ctx.cur_xds_payload_length` bytes
/// - Both pointers must remain valid for the duration of the call
pub unsafe fn xds_do_channel(
    sub: &mut crate::bindings::cc_subtitle,
    ctx: &mut CcxDecodersXdsContext,
) -> i32 {
    let mut was_proc = 0;

    // Safety check for payload
    if ctx.cur_xds_payload.is_null() || ctx.cur_xds_payload_length < 3 {
        return -1; // CCX_EINVAL equivalent
    }

    let payload =
        std::slice::from_raw_parts(ctx.cur_xds_payload, ctx.cur_xds_payload_length as usize);

    match ctx.cur_xds_packet_type {
        // XDS_TYPE_NETWORK_NAME = 1
        1 => {
            was_proc = 1;

            // Extract network name from payload (bytes 2 to payload_length - 2)
            let name_end = (ctx.cur_xds_payload_length - 1) as usize;
            let name_slice = &payload[2..name_end];
            let name_bytes = match name_slice.iter().position(|&b| b == 0) {
                Some(pos) => &name_slice[..pos],
                None => name_slice,
            };

            let xds_network_name = String::from_utf8_lossy(name_bytes).to_string();

            debug!(
                msg_type = DebugMessageFlag::DECODER_XDS;
                "XDS Network name: {}\n", xds_network_name
            );

            let network_msg = format!("Network: {}", xds_network_name);
            let _ = xdsprint(sub, ctx, network_msg);

            // Convert current_xds_network_name from i8 array to String for comparison
            let current_name = i8_array_to_string(&ctx.current_xds_network_name);

            if xds_network_name != current_name {
                // Change of station
                info!("XDS Notice: Network is now {}\n", xds_network_name);
                string_to_i8_array(&xds_network_name, &mut ctx.current_xds_network_name);
            }
        }

        // XDS_TYPE_CALL_LETTERS_AND_CHANNEL = 2
        2 => {
            was_proc = 1;

            // We need 4-6 data bytes (payload_length 7 or 9)
            if ctx.cur_xds_payload_length != 7 && ctx.cur_xds_payload_length != 9 {
                return was_proc;
            }

            // Extract call letters from payload (bytes 2 to payload_length - 2)
            let letters_end = (ctx.cur_xds_payload_length - 1) as usize;
            let letters_slice = &payload[2..letters_end];
            let letters_bytes = match letters_slice.iter().position(|&b| b == 0) {
                Some(pos) => &letters_slice[..pos],
                None => letters_slice,
            };

            let xds_call_letters = String::from_utf8_lossy(letters_bytes).to_string();

            debug!(
                msg_type = DebugMessageFlag::DECODER_XDS;
                "XDS Network call letters: {}\n", xds_call_letters
            );

            let letters_msg = format!("Call Letters: {}", xds_call_letters);
            let _ = xdsprint(sub, ctx, letters_msg);

            // Convert current_xds_call_letters from i8 array to String for comparison
            let current_letters = i8_array_to_string(&ctx.current_xds_call_letters);

            if xds_call_letters != current_letters {
                // Change of station
                info!(
                    "XDS Notice: Network call letters now {}\n",
                    xds_call_letters
                );
                string_to_i8_array(&xds_call_letters, &mut ctx.current_xds_call_letters);
                send_gui(GuiXdsMessage::CallLetters(&xds_call_letters));
            }
        }

        // XDS_TYPE_TSID = 4
        4 => {
            // According to CEA-608, data here (4 bytes) are used to identify the
            // "originating analog licensee".  No interesting data for us.
            was_proc = 1;

            if ctx.cur_xds_payload_length < 7 {
                // We need 4 data bytes
                return was_proc;
            }

            // Only low 4 bits from each byte
            let b1 = (payload[2] & 0x10) as u32;
            let b2 = (payload[3] & 0x10) as u32;
            let b3 = (payload[4] & 0x10) as u32;
            let b4 = (payload[5] & 0x10) as u32;
            let tsid = (b4 << 12) | (b3 << 8) | (b2 << 4) | b1;

            if tsid != 0 {
                let tsid_msg = format!("TSID: {}", tsid);
                let _ = xdsprint(sub, ctx, tsid_msg);
            }
        }

        _ => {
            // Unknown packet type
        }
    }

    was_proc
}

/// Processes XDS Private Data class packets.
///
/// # Safety
/// - `sub` must be a valid, non-null pointer to a `cc_subtitle` struct
/// - `ctx` must be a valid, non-null pointer to a `ccx_decoders_xds_context` struct
/// - `ctx.cur_xds_payload` must point to valid memory with at least `ctx.cur_xds_payload_length` bytes
/// - Both pointers must remain valid for the duration of the call
pub unsafe fn xds_do_private_data(
    sub: &mut crate::bindings::cc_subtitle,
    ctx: &mut CcxDecodersXdsContext,
) -> i32 {
    // Safety check for payload
    if ctx.cur_xds_payload.is_null() || ctx.cur_xds_payload_length < 3 {
        return -1; // CCX_EINVAL equivalent
    }

    let payload =
        std::slice::from_raw_parts(ctx.cur_xds_payload, ctx.cur_xds_payload_length as usize);

    // Build hex string from payload bytes (bytes 2 to payload_length - 2)
    let hex_str: String = payload[2..(ctx.cur_xds_payload_length - 1) as usize]
        .iter()
        .map(|b| format!("{:02X} ", b))
        .collect();

    let _ = xdsprint(sub, ctx, hex_str);

    1
}

/// Processes XDS Miscellaneous class packets.
///
/// # Safety
/// - `ctx` must be a valid, non-null pointer to a `ccx_decoders_xds_context` struct
/// - `ctx.cur_xds_payload` must point to valid memory with at least `ctx.cur_xds_payload_length` bytes
/// - The pointer must remain valid for the duration of the call
pub unsafe fn xds_do_misc(ctx: &mut CcxDecodersXdsContext) -> i32 {
    let was_proc;

    // Safety check for payload
    if ctx.cur_xds_payload.is_null() || ctx.cur_xds_payload_length < 3 {
        return -1; // CCX_EINVAL equivalent
    }

    let payload =
        std::slice::from_raw_parts(ctx.cur_xds_payload, ctx.cur_xds_payload_length as usize);

    match ctx.cur_xds_packet_type {
        // XDS_TYPE_TIME_OF_DAY = 1
        1 => {
            was_proc = 1;
            if ctx.cur_xds_payload_length < 9 {
                // We need 6 data bytes
                return was_proc;
            }

            let min = (payload[2] & 0x3f) as i32; // 6 bits
            let hour = (payload[3] & 0x1f) as i32; // 5 bits
            let date = (payload[4] & 0x1f) as i32; // 5 bits
            let month = (payload[5] & 0x0f) as i32; // 4 bits
            let reset_seconds = (payload[5] & 0x20) != 0;
            let day_of_week = (payload[6] & 0x07) as i32;
            let year = ((payload[7] & 0x3f) as i32) + 1990;

            debug!(
                msg_type = DebugMessageFlag::DECODER_XDS;
                "Time of day: (YYYY/MM/DD) {:04}/{:02}/{:02} (HH:SS) {:02}:{:02} DoW: {}  Reset seconds: {}\n",
                year, month, date, hour, min, day_of_week, if reset_seconds { 1 } else { 0 }
            );
        }

        // XDS_TYPE_LOCAL_TIME_ZONE = 4
        4 => {
            was_proc = 1;
            if ctx.cur_xds_payload_length < 5 {
                // We need 2 data bytes
                return was_proc;
            }

            // let b6 = (payload[2] & 0x40) >> 6; // Bit 6 should always be 1
            let dst = (payload[2] & 0x20) >> 5; // Daylight Saving Time
            let hour = (payload[2] & 0x1f) as i32; // 5 bits

            debug!(
                msg_type = DebugMessageFlag::DECODER_XDS;
                "Local Time Zone: {:02} DST: {}\n",
                hour, dst
            );
        }

        _ => {
            was_proc = 0;
        }
    }

    was_proc
}

/// Processes the end of an XDS packet, validates checksum, and dispatches to appropriate handler.
///
/// # Safety
/// This function is marked unsafe because it dereferences raw pointers from
/// the C bindings in `cc_subtitle`. The caller must ensure:
/// * `sub` points to a valid, properly initialized `cc_subtitle` structure
/// * The `data` field in `sub` is either null or points to valid memory
/// * No other code is concurrently modifying the subtitle structure
pub unsafe fn do_end_of_xds(
    sub: &mut crate::bindings::cc_subtitle,
    ctx: &mut CcxDecodersXdsContext,
    expected_checksum: u8,
) {
    // Check if buffer index is valid and in use
    if ctx.cur_xds_buffer_idx == -1 {
        return;
    }

    let idx = ctx.cur_xds_buffer_idx as usize;
    if idx >= ctx.xds_buffers.len() || ctx.xds_buffers[idx].in_use == 0 {
        return;
    }

    // Set up context from buffer
    ctx.cur_xds_packet_class = ctx.xds_buffers[idx]
        .xds_class
        .map(|c| c as i32)
        .unwrap_or(-1);
    ctx.cur_xds_payload = ctx.xds_buffers[idx].bytes.as_mut_ptr();
    ctx.cur_xds_payload_length = ctx.xds_buffers[idx].used_bytes as i32;

    // Get packet type from payload[1]
    if ctx.cur_xds_payload_length >= 2 {
        ctx.cur_xds_packet_type = ctx.xds_buffers[idx].bytes[1] as i32;
    }

    // Add the end byte (0x0F) to the packet
    if (ctx.cur_xds_payload_length as usize) < NUM_BYTES_PER_PACKET {
        ctx.xds_buffers[idx].bytes[ctx.cur_xds_payload_length as usize] = 0x0F;
        ctx.cur_xds_payload_length += 1;
        ctx.xds_buffers[idx].used_bytes = ctx.cur_xds_payload_length as u8;
    }

    // Calculate checksum
    let mut cs: i32 = 0;
    for i in 0..ctx.cur_xds_payload_length as usize {
        let byte = ctx.xds_buffers[idx].bytes[i];
        cs = (cs + byte as i32) & 0x7f; // Keep 7 bits only
        let c = byte & 0x7F;
        let display_char = if c >= 0x20 { c as char } else { '?' };
        debug!(
            msg_type = DebugMessageFlag::DECODER_XDS;
            "{:02X} - {} cs: {:02X}\n", c, display_char, cs
        );
    }
    cs = (128 - cs) & 0x7F; // Convert to 2's complement & discard high-order bit

    // Get class name for debug output
    let class_name = if (ctx.cur_xds_packet_class as usize) < XDS_CLASSES.len() {
        XDS_CLASSES[ctx.cur_xds_packet_class as usize]
    } else {
        "Unknown"
    };

    debug!(
        msg_type = DebugMessageFlag::DECODER_XDS;
        "End of XDS.  Class={} ({}), size={}  Checksum OK: {}   Used buffers: {}\n",
        ctx.cur_xds_packet_class,
        class_name,
        ctx. cur_xds_payload_length,
        cs == expected_checksum as i32,
        ctx.xds_buffers. iter().filter(|b| b.in_use != 0).count()
    );

    // Validate checksum and minimum length
    if cs != expected_checksum as i32 || ctx.cur_xds_payload_length < 3 {
        debug!(
            msg_type = DebugMessageFlag::DECODER_XDS;
            "Expected checksum: {:02X}  Calculated: {:02X}\n", expected_checksum, cs
        );
        ctx.xds_buffers[idx] = XdsBuffer::default(); // clear_xds_buffer
        return; // Bad packets ignored as per specs
    }

    let mut was_proc = 0;

    // Check if bit 6 is set for out-of-band data
    if (ctx.cur_xds_packet_type & 0x40) != 0 {
        ctx.cur_xds_packet_class = XdsClass::OutOfBand as i32;
    }

    // Convert cur_xds_packet_class to XdsClass for matching
    let xds_class = match ctx.cur_xds_packet_class {
        x if x == XdsClass::Current as i32 => Some(XdsClass::Current),
        x if x == XdsClass::Future as i32 => Some(XdsClass::Future),
        x if x == XdsClass::Channel as i32 => Some(XdsClass::Channel),
        x if x == XdsClass::Misc as i32 => Some(XdsClass::Misc),
        x if x == XdsClass::Private as i32 => Some(XdsClass::Private),
        x if x == XdsClass::OutOfBand as i32 => Some(XdsClass::OutOfBand),
        _ => None,
    };

    match xds_class {
        Some(XdsClass::Future) => {
            // Info on future program
            // Check if debug mask includes DECODER_XDS
            if let Some(logger) = lib_ccxr::util::log::logger() {
                if !logger.is_debug_mode() {
                    // Don't bother processing something we don't need
                    was_proc = 1;
                } else {
                    // Fall through to current processing
                    was_proc = xds_do_current_and_future(sub, ctx);
                }
            } else {
                was_proc = 1;
            }
        }

        Some(XdsClass::Current) => {
            // Info on current program
            was_proc = xds_do_current_and_future(sub, ctx);
        }

        Some(XdsClass::Channel) => {
            was_proc = xds_do_channel(sub, ctx);
        }

        Some(XdsClass::Misc) => {
            was_proc = xds_do_misc(ctx);
        }

        Some(XdsClass::Private) => {
            // CEA-608: The Private Data Class is for use in any closed system
            // for whatever that system wishes.  It shall not be defined by this
            // standard now or in the future.
            was_proc = xds_do_private_data(sub, ctx);
        }

        Some(XdsClass::OutOfBand) => {
            debug!(
                msg_type = DebugMessageFlag::DECODER_XDS;
                "Out-of-band data, ignored."
            );
            was_proc = 1;
        }

        _ => {
            // Unknown class
        }
    }

    if was_proc == 0 {
        info!("Note: We found a currently unsupported XDS packet.\n");
        // Dump the payload for debugging
        let payload_slice = &ctx.xds_buffers[idx].bytes[0..ctx.cur_xds_payload_length as usize];
        hex_dump(DebugMessageFlag::DECODER_XDS, payload_slice, false);
    }

    // Clear the buffer
    ctx.xds_buffers[idx] = XdsBuffer::default();
}
