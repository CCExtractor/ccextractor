//! XDS (Extended Data Services) types and structures for decoding extended data.
//!
//! This module provides types for handling XDS packets which carry metadata about programs,
//! channels, and other broadcast information embedded in the vertical blanking interval.
//!
//! # Key Types
//!
//! - [`XdsClass`] - Classification of XDS packet (Current, Future, Channel, Misc, Private, OutOfBand)
//! - [`XdsType`] - Specific type within each class (PIN, Program Name, Content Advisory, etc.)
//! - [`XdsBuffer`] - Buffer for accumulating XDS packet bytes
//! - [`CcxDecodersXdsContext`] - Main context structure for XDS decoding state
//!
//! # Constants
//!
//! - [`NUM_XDS_BUFFERS`] - Maximum number of concurrent XDS buffers (9)
//! - [`NUM_BYTES_PER_PACKET`] - Maximum bytes per XDS packet (35)
//! - [`XDS_CLASSES`] - Human-readable names for XDS classes
//! - [`XDS_PROGRAM_TYPES`] - Program type descriptions (Education, Entertainment, etc.)
//!
//! # Conversion Guide
//!
//! | C (ccx_decoders_xds.c/.h)             | Rust (types.rs)                                   |
//! |---------------------------------------|---------------------------------------------------|
//! | `XDS_CLASS_*` constants               | [`XdsClass`] enum                                 |
//! | `XDS_TYPE_*` constants                | [`XdsType`] enum variants                         |
//! | `struct xds_buffer`                   | [`XdsBuffer`]                                     |
//! | `ccx_decoders_xds_context`            | [`CcxDecodersXdsContext`]                         |
//! | `xds_class` (int)                     | [`XdsClass::from_c_int`], [`XdsClass::to_c_int`]  |
//! | `xds_type` (int)                      | [`XdsType::from_c_int`], [`XdsType::to_c_int`]    |
//! | `xds_program_type` array              | [`XDS_PROGRAM_TYPES`] constant array              |
//! | `xds_classes` array                   | [`XDS_CLASSES`] constant array                    |
//! | `clear_xds_buffer`                    | [`CcxDecodersXdsContext::clear_xds_buffer`]       |
//! | `how_many_used`                       | [`CcxDecodersXdsContext::how_many_used`]          |
//! | `process_xds_bytes`                   | [`CcxDecodersXdsContext::process_xds_bytes`]      |
//! | `CcxDecodersXdsContext::from_ctype`   | Convert from C `ccx_decoders_xds_context`         |
//! | `copy_xds_context_from_rust_to_c`     | Sync Rust context back to C struct                |

use crate::bindings::*;
use crate::common::CType;
use crate::ctorust::FromCType;
use crate::libccxr_exports::time::write_back_to_common_timing_ctx;
use lib_ccxr::time::TimingContext;
use std::os::raw::c_int;
use std::ptr::null_mut;

pub const NUM_BYTES_PER_PACKET: usize = 35; // Class + type (repeated for convenience) + data + zero
pub const NUM_XDS_BUFFERS: usize = 9; // CEA recommends no more than one level of interleaving. Play it safe

pub static XDS_CLASSES: [&str; 8] = [
    "Current",
    "Future",
    "Channel",
    "Miscellaneous",
    "Public service",
    "Reserved",
    "Private data",
    "End",
];

pub static XDS_PROGRAM_TYPES: [&str; 96] = [
    "Education",
    "Entertainment",
    "Movie",
    "News",
    "Religious",
    "Sports",
    "Other",
    "Action",
    "Advertisement",
    "Animated",
    "Anthology",
    "Automobile",
    "Awards",
    "Baseball",
    "Basketball",
    "Bulletin",
    "Business",
    "Classical",
    "College",
    "Combat",
    "Comedy",
    "Commentary",
    "Concert",
    "Consumer",
    "Contemporary",
    "Crime",
    "Dance",
    "Documentary",
    "Drama",
    "Elementary",
    "Erotica",
    "Exercise",
    "Fantasy",
    "Farm",
    "Fashion",
    "Fiction",
    "Food",
    "Football",
    "Foreign",
    "Fund-Raiser",
    "Game/Quiz",
    "Garden",
    "Golf",
    "Government",
    "Health",
    "High_School",
    "History",
    "Hobby",
    "Hockey",
    "Home",
    "Horror",
    "Information",
    "Instruction",
    "International",
    "Interview",
    "Language",
    "Legal",
    "Live",
    "Local",
    "Math",
    "Medical",
    "Meeting",
    "Military",
    "Mini-Series",
    "Music",
    "Mystery",
    "National",
    "Nature",
    "Police",
    "Politics",
    "Premiere",
    "Pre-Recorded",
    "Product",
    "Professional",
    "Public",
    "Racing",
    "Reading",
    "Repair",
    "Repeat",
    "Review",
    "Romance",
    "Science",
    "Series",
    "Service",
    "Shopping",
    "Soap_Opera",
    "Special",
    "Suspense",
    "Talk",
    "Technical",
    "Tennis",
    "Travel",
    "Variety",
    "Video",
    "Weather",
    "Western",
];

#[derive(Debug, Clone, Copy, PartialEq)]
#[repr(u8)]
pub enum XdsClass {
    Current = 0,
    Future = 1,
    Channel = 2,
    Misc = 3,
    Public = 4,
    Reserved = 5,
    Private = 6,
    End = 7,
    OutOfBand = 0x40,
}

impl XdsClass {
    pub fn from_c_int(value: c_int) -> Option<Self> {
        match value {
            0 => Some(XdsClass::Current),
            1 => Some(XdsClass::Future),
            2 => Some(XdsClass::Channel),
            3 => Some(XdsClass::Misc),
            4 => Some(XdsClass::Public),
            5 => Some(XdsClass::Reserved),
            6 => Some(XdsClass::Private),
            7 => Some(XdsClass::End),
            0x40 => Some(XdsClass::OutOfBand),
            _ => None,
        }
    }

    pub fn to_c_int(&self) -> c_int {
        match self {
            XdsClass::Current => 0,
            XdsClass::Future => 1,
            XdsClass::Channel => 2,
            XdsClass::Misc => 3,
            XdsClass::Public => 4,
            XdsClass::Reserved => 5,
            XdsClass::Private => 6,
            XdsClass::End => 7,
            XdsClass::OutOfBand => 0x40,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
#[repr(u8)]
pub enum XdsCurrentFutureType {
    PinStartTime = 1,
    LengthAndCurrentTime = 2,
    ProgramName = 3,
    ProgramType = 4,
    ContentAdvisory = 5,
    AudioServices = 6,
    Cgms = 8,
    AspectRatioInfo = 9,
    ProgramDesc1 = 0x10,
    ProgramDesc2 = 0x11,
    ProgramDesc3 = 0x12,
    ProgramDesc4 = 0x13,
    ProgramDesc5 = 0x14,
    ProgramDesc6 = 0x15,
    ProgramDesc7 = 0x16,
    ProgramDesc8 = 0x17,
}

#[derive(Debug, Clone, Copy, PartialEq)]
#[repr(u8)]
pub enum XdsChannelType {
    NetworkName = 1,
    CallLettersAndChannel = 2,
    Tsid = 4,
}

#[derive(Debug, Clone, Copy, PartialEq)]
#[repr(u8)]
pub enum XdsMiscType {
    TimeOfDay = 1,
    LocalTimeZone = 4,
    OutOfBandChannelNumber = 0x40,
}

// this is new - was not there in my original code
#[derive(Debug, Clone, Copy, PartialEq)]
#[repr(u8)]
pub enum XdsType {
    CurrentFuture(XdsCurrentFutureType),
    Misc(XdsMiscType),
    Channel(XdsChannelType),
}

impl XdsType {
    pub fn from_c_int(class: Option<XdsClass>, type_value: c_int) -> Option<Self> {
        match class? {
            XdsClass::Current | XdsClass::Future => match type_value {
                1 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::PinStartTime)),
                2 => Some(XdsType::CurrentFuture(
                    XdsCurrentFutureType::LengthAndCurrentTime,
                )),
                3 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::ProgramName)),
                4 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::ProgramType)),
                5 => Some(XdsType::CurrentFuture(
                    XdsCurrentFutureType::ContentAdvisory,
                )),
                6 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::AudioServices)),
                8 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::Cgms)),
                9 => Some(XdsType::CurrentFuture(
                    XdsCurrentFutureType::AspectRatioInfo,
                )),
                0x10 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::ProgramDesc1)),
                0x11 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::ProgramDesc2)),
                0x12 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::ProgramDesc3)),
                0x13 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::ProgramDesc4)),
                0x14 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::ProgramDesc5)),
                0x15 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::ProgramDesc6)),
                0x16 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::ProgramDesc7)),
                0x17 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::ProgramDesc8)),
                _ => None,
            },
            XdsClass::Channel => match type_value {
                1 => Some(XdsType::Channel(XdsChannelType::NetworkName)),
                2 => Some(XdsType::Channel(XdsChannelType::CallLettersAndChannel)),
                4 => Some(XdsType::Channel(XdsChannelType::Tsid)),
                _ => None,
            },
            XdsClass::Misc => match type_value {
                1 => Some(XdsType::Misc(XdsMiscType::TimeOfDay)),
                4 => Some(XdsType::Misc(XdsMiscType::LocalTimeZone)),
                0x40 => Some(XdsType::Misc(XdsMiscType::OutOfBandChannelNumber)),
                _ => None,
            },
            XdsClass::Public
            | XdsClass::Reserved
            | XdsClass::Private
            | XdsClass::End
            | XdsClass::OutOfBand => {
                // These classes don't have specific types defined yet
                // Return None for now
                None
            }
        }
    }

    pub fn to_c_int(&self) -> c_int {
        match self {
            XdsType::CurrentFuture(t) => match t {
                XdsCurrentFutureType::PinStartTime => 1,
                XdsCurrentFutureType::LengthAndCurrentTime => 2,
                XdsCurrentFutureType::ProgramName => 3,
                XdsCurrentFutureType::ProgramType => 4,
                XdsCurrentFutureType::ContentAdvisory => 5,
                XdsCurrentFutureType::AudioServices => 6,
                XdsCurrentFutureType::Cgms => 8,
                XdsCurrentFutureType::AspectRatioInfo => 9,
                XdsCurrentFutureType::ProgramDesc1 => 0x10,
                XdsCurrentFutureType::ProgramDesc2 => 0x11,
                XdsCurrentFutureType::ProgramDesc3 => 0x12,
                XdsCurrentFutureType::ProgramDesc4 => 0x13,
                XdsCurrentFutureType::ProgramDesc5 => 0x14,
                XdsCurrentFutureType::ProgramDesc6 => 0x15,
                XdsCurrentFutureType::ProgramDesc7 => 0x16,
                XdsCurrentFutureType::ProgramDesc8 => 0x17,
            },
            XdsType::Channel(t) => match t {
                XdsChannelType::NetworkName => 1,
                XdsChannelType::CallLettersAndChannel => 2,
                XdsChannelType::Tsid => 4,
            },
            XdsType::Misc(t) => match t {
                XdsMiscType::TimeOfDay => 1,
                XdsMiscType::LocalTimeZone => 4,
                XdsMiscType::OutOfBandChannelNumber => 0x40,
            },
        }
    }
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct XdsBuffer {
    pub in_use: u32,
    pub xds_class: Option<XdsClass>,
    pub xds_type: Option<XdsType>,
    pub bytes: [u8; NUM_BYTES_PER_PACKET], // Class + type (repeated for convenience) + data + zero
    pub used_bytes: u8,
}

impl Default for XdsBuffer {
    fn default() -> Self {
        XdsBuffer {
            in_use: 0,
            xds_class: None,
            xds_type: None,
            bytes: [0; NUM_BYTES_PER_PACKET],
            used_bytes: 0,
        }
    }
}

impl FromCType<xds_buffer> for XdsBuffer {
    unsafe fn from_ctype(c_value: xds_buffer) -> Option<Self> {
        let xds_class = if c_value.xds_class == -1 {
            None
        } else {
            XdsClass::from_c_int(c_value.xds_class)
        };

        let xds_type = if c_value.xds_type == -1 {
            None
        } else {
            XdsType::from_c_int(xds_class, c_value.xds_type)
        };

        Some(XdsBuffer {
            in_use: c_value.in_use,
            xds_class,
            xds_type,
            bytes: c_value.bytes,
            used_bytes: c_value.used_bytes,
        })
    }
}

impl CType<xds_buffer> for XdsBuffer {
    unsafe fn to_ctype(&self) -> xds_buffer {
        xds_buffer {
            in_use: self.in_use,
            xds_class: self.xds_class.map(|c| c.to_c_int()).unwrap_or(-1),
            xds_type: self.xds_type.map(|t| t.to_c_int()).unwrap_or(-1),
            bytes: self.bytes,
            used_bytes: self.used_bytes,
        }
    }
}

#[repr(C)]
pub struct CcxDecodersXdsContext<'a> {
    // Program Identification Number (Start Time) for current program
    pub current_xds_min: i32,
    pub current_xds_hour: i32,
    pub current_xds_date: i32,
    pub current_xds_month: i32,
    pub current_program_type_reported: i32, // No.
    pub xds_start_time_shown: i32,
    pub xds_program_length_shown: i32,
    pub xds_program_description: [[i8; 33]; 8],
    pub current_xds_network_name: [i8; 33],
    pub current_xds_program_name: [i8; 33],
    pub current_xds_call_letters: [i8; 7],
    pub current_xds_program_type: [i8; 33],

    pub xds_buffers: [XdsBuffer; NUM_XDS_BUFFERS],
    pub cur_xds_buffer_idx: i32,
    pub cur_xds_packet_class: i32,
    pub cur_xds_payload: *mut u8,
    pub cur_xds_payload_length: i32,
    pub cur_xds_packet_type: i32,
    pub timing: Option<&'a mut TimingContext>,
    pub current_ar_start: u32,
    pub current_ar_end: u32,
    pub xds_write_to_file: bool,
}

impl Default for CcxDecodersXdsContext<'_> {
    fn default() -> Self {
        CcxDecodersXdsContext {
            current_xds_min: 0,
            current_xds_hour: 0,
            current_xds_date: 0,
            current_xds_month: 0,
            current_program_type_reported: 0,
            xds_start_time_shown: 0,
            xds_program_length_shown: 0,
            xds_program_description: [[0; 33]; 8],
            current_xds_network_name: [0; 33],
            current_xds_program_name: [0; 33],
            current_xds_call_letters: [0; 7],
            current_xds_program_type: [0; 33],
            xds_buffers: [XdsBuffer::default(); NUM_XDS_BUFFERS],
            cur_xds_buffer_idx: 0,
            cur_xds_packet_class: 0,
            cur_xds_payload: std::ptr::null_mut(),
            cur_xds_payload_length: 0,
            cur_xds_packet_type: 0,
            timing: None,
            current_ar_start: 0,
            current_ar_end: 0,
            xds_write_to_file: false,
        }
    }
}

impl FromCType<ccx_decoders_xds_context> for CcxDecodersXdsContext<'_> {
    unsafe fn from_ctype(c_value: ccx_decoders_xds_context) -> Option<Self> {
        let mut xds_buffers = [XdsBuffer::default(); NUM_XDS_BUFFERS];

        // Convert each xds_buffer from C to Rust
        for (i, c_buffer) in c_value.xds_buffers.iter().enumerate() {
            if let Some(rust_buffer) = XdsBuffer::from_ctype(*c_buffer) {
                xds_buffers[i] = rust_buffer;
            }
        }

        Some(CcxDecodersXdsContext {
            current_xds_min: c_value.current_xds_min,
            current_xds_hour: c_value.current_xds_hour,
            current_xds_date: c_value.current_xds_date,
            current_xds_month: c_value.current_xds_month,
            current_program_type_reported: c_value.current_program_type_reported,
            xds_start_time_shown: c_value.xds_start_time_shown,
            xds_program_length_shown: c_value.xds_program_length_shown,
            xds_program_description: c_value.xds_program_description,
            current_xds_network_name: c_value.current_xds_network_name,
            current_xds_program_name: c_value.current_xds_program_name,
            current_xds_call_letters: c_value.current_xds_call_letters,
            current_xds_program_type: c_value.current_xds_program_type,
            xds_buffers,
            cur_xds_buffer_idx: c_value.cur_xds_buffer_idx,
            cur_xds_packet_class: c_value.cur_xds_packet_class,
            cur_xds_payload: c_value.cur_xds_payload,
            cur_xds_payload_length: c_value.cur_xds_payload_length,
            cur_xds_packet_type: c_value.cur_xds_packet_type,
            timing: None, // Cannot directly convert raw pointer to reference - needs to be handled separately
            current_ar_start: c_value.current_ar_start,
            current_ar_end: c_value.current_ar_end,
            xds_write_to_file: c_value.xds_write_to_file != 0,
        })
    }
}

/// # Safety
///
/// - `bitstream_ptr` must be non-null and point to uniquely writable memory for a
///   `ccx_decoders_xds_context` for the duration of the call.
/// - `rust_ctx` must be valid and C-layout compatible for all fields copied.
/// - If `rust_ctx.cur_xds_payload` is non-null it must be valid for
///   `rust_ctx.cur_xds_payload_length` bytes.
/// - If `rust_ctx.timing` is `Some`, the C-side `timing` pointer in `bitstream_ptr`
///   must be a valid destination for `write_back_to_common_timing_ctx`.
/// - Violating these invariants is undefined behaviour; call only from `unsafe`.
pub unsafe fn copy_xds_context_from_rust_to_c(
    bitstream_ptr: *mut ccx_decoders_xds_context,
    rust_ctx: &CcxDecodersXdsContext<'_>,
) {
    if bitstream_ptr.is_null() {
        return;
    }
    let original_timing = (*bitstream_ptr).timing;
    let output = ccx_decoders_xds_context {
        current_xds_min: rust_ctx.current_xds_min,
        current_xds_hour: rust_ctx.current_xds_hour,
        current_xds_date: rust_ctx.current_xds_date,
        current_xds_month: rust_ctx.current_xds_month,
        current_program_type_reported: rust_ctx.current_program_type_reported,
        xds_start_time_shown: rust_ctx.xds_start_time_shown,
        xds_program_length_shown: rust_ctx.xds_program_length_shown,
        xds_program_description: rust_ctx.xds_program_description,
        current_xds_network_name: rust_ctx.current_xds_network_name,
        current_xds_program_name: rust_ctx.current_xds_program_name,
        current_xds_call_letters: rust_ctx.current_xds_call_letters,
        current_xds_program_type: rust_ctx.current_xds_program_type,
        xds_buffers: rust_ctx.xds_buffers.map(|buf| buf.to_ctype()),
        cur_xds_buffer_idx: rust_ctx.cur_xds_buffer_idx,
        cur_xds_packet_class: rust_ctx.cur_xds_packet_class,
        cur_xds_payload: rust_ctx.cur_xds_payload,
        cur_xds_payload_length: rust_ctx.cur_xds_payload_length,
        cur_xds_packet_type: rust_ctx.cur_xds_packet_type,
        timing: original_timing,
        current_ar_start: rust_ctx.current_ar_start,
        current_ar_end: rust_ctx.current_ar_end,
        xds_write_to_file: if rust_ctx.xds_write_to_file { 1 } else { 0 },
    };
    std::ptr::write(bitstream_ptr, output);
    if let Some(ref timing_ctx) = rust_ctx.timing {
        write_back_to_common_timing_ctx((*bitstream_ptr).timing, timing_ctx);
    }
}
