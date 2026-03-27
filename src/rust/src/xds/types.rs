//! XDS (Extended Data Services) types and structures for decoding extended data.
//!
//! This module provides types for handling XDS packets which carry metadata about programs,
//! channels, and other broadcast information embedded in the vertical blanking interval.
//!
//! # Key Types
//!
//! - [`XdsBuffer`] - Buffer for accumulating XDS packet bytes
//! - [`CcxDecodersXdsContext`] - Main context structure for XDS decoding state
//!
//! # Conversion Guide
//!
//! | C (ccx_decoders_xds.c/.h)          | Rust (types.rs)                               |
//! |------------------------------------|-----------------------------------------------|
//! | `struct xds_buffer`                | [`XdsBuffer`]                                 |
//! | `ccx_decoders_xds_context`         | [`CcxDecodersXdsContext`]                     |
//! | `clear_xds_buffer`                 | [`CcxDecodersXdsContext::clear_xds_buffer`]   |
//! | `how_many_used`                    | [`CcxDecodersXdsContext::how_many_used`]      |
//! | `process_xds_bytes`                | [`CcxDecodersXdsContext::process_xds_bytes`]  |
//! | C struct -> Rust                   | [`CcxDecodersXdsContext::from_ctype`]         |
//! | Rust -> C struct                   | [`copy_xds_context_from_rust_to_c`]           |

use crate::bindings::*;
use crate::common::CType;
use crate::ctorust::FromCType;
use crate::libccxr_exports::time::write_back_to_common_timing_ctx;
pub use crate::xds::constants::*;
use lib_ccxr::time::TimingContext;
use std::os::raw::c_int;

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

#[cfg(test)]
mod tests {
    use super::*;

    // --- XdsClass ---

    #[test]
    fn test_xds_class_from_c_int_valid() {
        assert_eq!(XdsClass::from_c_int(0), Some(XdsClass::Current));
        assert_eq!(XdsClass::from_c_int(1), Some(XdsClass::Future));
        assert_eq!(XdsClass::from_c_int(2), Some(XdsClass::Channel));
        assert_eq!(XdsClass::from_c_int(3), Some(XdsClass::Misc));
        assert_eq!(XdsClass::from_c_int(4), Some(XdsClass::Public));
        assert_eq!(XdsClass::from_c_int(5), Some(XdsClass::Reserved));
        assert_eq!(XdsClass::from_c_int(6), Some(XdsClass::Private));
        assert_eq!(XdsClass::from_c_int(7), Some(XdsClass::End));
        assert_eq!(XdsClass::from_c_int(0x40), Some(XdsClass::OutOfBand));
    }

    #[test]
    fn test_xds_class_from_c_int_invalid() {
        assert_eq!(XdsClass::from_c_int(-1), None);
        assert_eq!(XdsClass::from_c_int(8), None);
        assert_eq!(XdsClass::from_c_int(100), None);
    }

    #[test]
    fn test_xds_class_roundtrip() {
        let classes = [
            XdsClass::Current,
            XdsClass::Future,
            XdsClass::Channel,
            XdsClass::Misc,
            XdsClass::Public,
            XdsClass::Reserved,
            XdsClass::Private,
            XdsClass::End,
            XdsClass::OutOfBand,
        ];
        for class in &classes {
            let c_int = class.to_c_int();
            let roundtripped = XdsClass::from_c_int(c_int).unwrap();
            assert_eq!(*class, roundtripped);
        }
    }

    // --- XdsType ---

    #[test]
    fn test_xds_type_current_future_from_c_int() {
        let class = Some(XdsClass::Current);
        assert_eq!(
            XdsType::from_c_int(class, 1),
            Some(XdsType::CurrentFuture(XdsCurrentFutureType::PinStartTime))
        );
        assert_eq!(
            XdsType::from_c_int(class, 2),
            Some(XdsType::CurrentFuture(
                XdsCurrentFutureType::LengthAndCurrentTime
            ))
        );
        assert_eq!(
            XdsType::from_c_int(class, 3),
            Some(XdsType::CurrentFuture(XdsCurrentFutureType::ProgramName))
        );
        assert_eq!(
            XdsType::from_c_int(class, 4),
            Some(XdsType::CurrentFuture(XdsCurrentFutureType::ProgramType))
        );
        assert_eq!(
            XdsType::from_c_int(class, 5),
            Some(XdsType::CurrentFuture(
                XdsCurrentFutureType::ContentAdvisory
            ))
        );
        assert_eq!(
            XdsType::from_c_int(class, 6),
            Some(XdsType::CurrentFuture(XdsCurrentFutureType::AudioServices))
        );
        assert_eq!(
            XdsType::from_c_int(class, 8),
            Some(XdsType::CurrentFuture(XdsCurrentFutureType::Cgms))
        );
        assert_eq!(
            XdsType::from_c_int(class, 9),
            Some(XdsType::CurrentFuture(
                XdsCurrentFutureType::AspectRatioInfo
            ))
        );
        for i in 0x10..=0x17 {
            assert!(XdsType::from_c_int(class, i).is_some());
        }
        assert_eq!(XdsType::from_c_int(class, 7), None);
        assert_eq!(XdsType::from_c_int(class, 0x18), None);
    }

    #[test]
    fn test_xds_type_future_class_same_as_current() {
        for type_val in [
            1, 2, 3, 4, 5, 6, 8, 9, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        ] {
            assert_eq!(
                XdsType::from_c_int(Some(XdsClass::Current), type_val),
                XdsType::from_c_int(Some(XdsClass::Future), type_val)
            );
        }
    }

    #[test]
    fn test_xds_type_channel_from_c_int() {
        let class = Some(XdsClass::Channel);
        assert_eq!(
            XdsType::from_c_int(class, 1),
            Some(XdsType::Channel(XdsChannelType::NetworkName))
        );
        assert_eq!(
            XdsType::from_c_int(class, 2),
            Some(XdsType::Channel(XdsChannelType::CallLettersAndChannel))
        );
        assert_eq!(
            XdsType::from_c_int(class, 4),
            Some(XdsType::Channel(XdsChannelType::Tsid))
        );
        assert_eq!(XdsType::from_c_int(class, 3), None);
        assert_eq!(XdsType::from_c_int(class, 5), None);
    }

    #[test]
    fn test_xds_type_misc_from_c_int() {
        let class = Some(XdsClass::Misc);
        assert_eq!(
            XdsType::from_c_int(class, 1),
            Some(XdsType::Misc(XdsMiscType::TimeOfDay))
        );
        assert_eq!(
            XdsType::from_c_int(class, 4),
            Some(XdsType::Misc(XdsMiscType::LocalTimeZone))
        );
        assert_eq!(
            XdsType::from_c_int(class, 0x40),
            Some(XdsType::Misc(XdsMiscType::OutOfBandChannelNumber))
        );
        assert_eq!(XdsType::from_c_int(class, 2), None);
    }

    #[test]
    fn test_xds_type_unsupported_classes() {
        for class in [
            XdsClass::Public,
            XdsClass::Reserved,
            XdsClass::Private,
            XdsClass::End,
            XdsClass::OutOfBand,
        ] {
            assert_eq!(XdsType::from_c_int(Some(class), 1), None);
        }
    }

    #[test]
    fn test_xds_type_none_class() {
        assert_eq!(XdsType::from_c_int(None, 1), None);
    }

    #[test]
    fn test_xds_type_roundtrip_current_future() {
        let class = Some(XdsClass::Current);
        for type_val in [
            1, 2, 3, 4, 5, 6, 8, 9, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        ] {
            let xds_type = XdsType::from_c_int(class, type_val).unwrap();
            assert_eq!(xds_type.to_c_int(), type_val);
        }
    }

    #[test]
    fn test_xds_type_roundtrip_channel() {
        let class = Some(XdsClass::Channel);
        for type_val in [1, 2, 4] {
            let xds_type = XdsType::from_c_int(class, type_val).unwrap();
            assert_eq!(xds_type.to_c_int(), type_val);
        }
    }

    #[test]
    fn test_xds_type_roundtrip_misc() {
        let class = Some(XdsClass::Misc);
        for type_val in [1, 4, 0x40] {
            let xds_type = XdsType::from_c_int(class, type_val).unwrap();
            assert_eq!(xds_type.to_c_int(), type_val);
        }
    }

    // --- XdsBuffer ---

    fn make_c_buf(in_use: u32, xds_class: i32, xds_type: i32, used_bytes: u8) -> xds_buffer {
        let mut bytes = [0u8; NUM_BYTES_PER_PACKET];
        bytes[0] = 0x01;
        bytes[1] = 0x03;
        xds_buffer {
            in_use,
            xds_class,
            xds_type,
            bytes,
            used_bytes,
        }
    }

    #[test]
    fn test_xds_buffer_from_ctype_valid_current_program_name() {
        let c_buf = make_c_buf(1, 0, 3, 4);
        let rust_buf = unsafe { XdsBuffer::from_ctype(c_buf) }.unwrap();
        assert_eq!(rust_buf.in_use, 1);
        assert_eq!(rust_buf.xds_class, Some(XdsClass::Current));
        assert_eq!(
            rust_buf.xds_type,
            Some(XdsType::CurrentFuture(XdsCurrentFutureType::ProgramName))
        );
        assert_eq!(rust_buf.used_bytes, 4);
        assert_eq!(rust_buf.bytes[0], 0x01);
        assert_eq!(rust_buf.bytes[1], 0x03);
    }

    #[test]
    fn test_xds_buffer_from_ctype_class_minus1_gives_none() {
        let c_buf = make_c_buf(0, -1, -1, 0);
        let rust_buf = unsafe { XdsBuffer::from_ctype(c_buf) }.unwrap();
        assert_eq!(rust_buf.xds_class, None);
        assert_eq!(rust_buf.xds_type, None);
    }

    #[test]
    fn test_xds_buffer_from_ctype_type_minus1_gives_none() {
        let c_buf = make_c_buf(1, 0, -1, 2);
        let rust_buf = unsafe { XdsBuffer::from_ctype(c_buf) }.unwrap();
        assert_eq!(rust_buf.xds_class, Some(XdsClass::Current));
        assert_eq!(rust_buf.xds_type, None);
    }

    #[test]
    fn test_xds_buffer_from_ctype_invalid_class_gives_none() {
        let c_buf = make_c_buf(1, 99, 1, 2);
        let rust_buf = unsafe { XdsBuffer::from_ctype(c_buf) }.unwrap();
        assert_eq!(rust_buf.xds_class, None);
        assert_eq!(rust_buf.xds_type, None);
    }

    #[test]
    fn test_xds_buffer_from_ctype_invalid_type_for_class_gives_none() {
        let c_buf = make_c_buf(1, 2, 99, 2);
        let rust_buf = unsafe { XdsBuffer::from_ctype(c_buf) }.unwrap();
        assert_eq!(rust_buf.xds_class, Some(XdsClass::Channel));
        assert_eq!(rust_buf.xds_type, None);
    }

    #[test]
    fn test_xds_buffer_to_ctype_valid() {
        let c_buf = make_c_buf(1, 0, 3, 4);
        let rust_buf = unsafe { XdsBuffer::from_ctype(c_buf) }.unwrap();
        let back = unsafe { rust_buf.to_ctype() };
        assert_eq!(back.in_use, 1);
        assert_eq!(back.xds_class, 0);
        assert_eq!(back.xds_type, 3);
        assert_eq!(back.used_bytes, 4);
        assert_eq!(back.bytes[0], 0x01);
        assert_eq!(back.bytes[1], 0x03);
    }

    #[test]
    fn test_xds_buffer_to_ctype_none_becomes_minus1() {
        let c_buf = make_c_buf(0, -1, -1, 0);
        let rust_buf = unsafe { XdsBuffer::from_ctype(c_buf) }.unwrap();
        let back = unsafe { rust_buf.to_ctype() };
        assert_eq!(back.xds_class, -1);
        assert_eq!(back.xds_type, -1);
    }

    #[test]
    fn test_xds_buffer_roundtrip_all_classes() {
        let cases: &[(i32, i32)] = &[
            (0, 1),
            (0, 2),
            (0, 3),
            (0, 4),
            (0, 5),
            (0, 6),
            (0, 8),
            (0, 9),
            (0, 0x10),
            (0, 0x17),
            (1, 3), // future + programName
            (2, 1),
            (2, 2),
            (2, 4), // channel
            (3, 1),
            (3, 4),
            (3, 0x40), // mics
        ];
        for &(class, typ) in cases {
            let c_buf = make_c_buf(1, class, typ, 2);
            let rust_buf = unsafe { XdsBuffer::from_ctype(c_buf) }.unwrap();
            let back = unsafe { rust_buf.to_ctype() };
            assert_eq!(
                back.xds_class, class,
                "class mismatch for ({}, {})",
                class, typ
            );
            assert_eq!(back.xds_type, typ, "type mismatch for ({}, {})", class, typ);
        }
    }

    #[test]
    fn test_xds_buffer_to_ctype_unsupported_class_type_is_none() {
        for class in [4i32, 5, 6, 7, 0x40] {
            let c_buf = make_c_buf(1, class, 1, 2);
            let rust_buf = unsafe { XdsBuffer::from_ctype(c_buf) }.unwrap();
            assert!(
                rust_buf.xds_class.is_some(),
                "expected class Some for {}",
                class
            );
            assert_eq!(
                rust_buf.xds_type, None,
                "expected type None for class {}",
                class
            );
            let back = unsafe { rust_buf.to_ctype() };
            assert_eq!(back.xds_type, -1, "expected -1 in C for class {}", class);
        }
    }

    #[test]
    fn test_xds_buffer_bytes_preserved_through_roundtrip() {
        let mut c_buf = make_c_buf(1, 0, 3, 10);
        for (i, b) in c_buf.bytes.iter_mut().enumerate() {
            *b = (i as u8).wrapping_mul(7);
        }
        let rust_buf = unsafe { XdsBuffer::from_ctype(c_buf) }.unwrap();
        let back = unsafe { rust_buf.to_ctype() };
        assert_eq!(c_buf.bytes, back.bytes);
    }

    // --- CcxDecodersXdsContext ---

    fn make_c_xds_ctx_zeroed() -> ccx_decoders_xds_context {
        unsafe { std::mem::zeroed() }
    }

    #[test]
    fn test_xds_context_from_ctype_scalars() {
        let mut c_ctx = make_c_xds_ctx_zeroed();
        c_ctx.current_xds_min = 42;
        c_ctx.current_xds_hour = 13;
        c_ctx.current_xds_date = 15;
        c_ctx.current_xds_month = 7;
        c_ctx.current_program_type_reported = 3;
        c_ctx.xds_start_time_shown = 1;
        c_ctx.xds_program_length_shown = 2;
        c_ctx.cur_xds_buffer_idx = 5;
        c_ctx.cur_xds_packet_class = 0;
        c_ctx.cur_xds_payload_length = 10;
        c_ctx.cur_xds_packet_type = 3;
        c_ctx.current_ar_start = 100;
        c_ctx.current_ar_end = 200;

        let rust_ctx = unsafe { CcxDecodersXdsContext::from_ctype(c_ctx) }.unwrap();
        assert_eq!(rust_ctx.current_xds_min, 42);
        assert_eq!(rust_ctx.current_xds_hour, 13);
        assert_eq!(rust_ctx.current_xds_date, 15);
        assert_eq!(rust_ctx.current_xds_month, 7);
        assert_eq!(rust_ctx.current_program_type_reported, 3);
        assert_eq!(rust_ctx.xds_start_time_shown, 1);
        assert_eq!(rust_ctx.xds_program_length_shown, 2);
        assert_eq!(rust_ctx.cur_xds_buffer_idx, 5);
        assert_eq!(rust_ctx.cur_xds_packet_class, 0);
        assert_eq!(rust_ctx.cur_xds_payload_length, 10);
        assert_eq!(rust_ctx.cur_xds_packet_type, 3);
        assert_eq!(rust_ctx.current_ar_start, 100);
        assert_eq!(rust_ctx.current_ar_end, 200);
    }

    #[test]
    fn test_xds_context_from_ctype_write_to_file() {
        let mut c_ctx = make_c_xds_ctx_zeroed();

        c_ctx.xds_write_to_file = 0;
        let rust_ctx = unsafe { CcxDecodersXdsContext::from_ctype(c_ctx) }.unwrap();
        assert!(!rust_ctx.xds_write_to_file);

        c_ctx.xds_write_to_file = 1;
        let rust_ctx = unsafe { CcxDecodersXdsContext::from_ctype(c_ctx) }.unwrap();
        assert!(rust_ctx.xds_write_to_file);

        c_ctx.xds_write_to_file = 42;
        let rust_ctx = unsafe { CcxDecodersXdsContext::from_ctype(c_ctx) }.unwrap();
        assert!(rust_ctx.xds_write_to_file);
    }

    #[test]
    fn test_xds_context_from_ctype_timing_always_none() {
        let c_ctx = make_c_xds_ctx_zeroed(); // timing pointer is null
        let rust_ctx = unsafe { CcxDecodersXdsContext::from_ctype(c_ctx) }.unwrap();
        assert!(rust_ctx.timing.is_none());
    }

    #[test]
    fn test_xds_context_from_ctype_buffers_converted() {
        let mut c_ctx = make_c_xds_ctx_zeroed();
        c_ctx.xds_buffers[0].in_use = 1;
        c_ctx.xds_buffers[0].xds_class = 0; // curr
        c_ctx.xds_buffers[0].xds_type = 3; // programName
        c_ctx.xds_buffers[0].used_bytes = 5;

        let rust_ctx = unsafe { CcxDecodersXdsContext::from_ctype(c_ctx) }.unwrap();
        assert_eq!(rust_ctx.xds_buffers[0].in_use, 1);
        assert_eq!(rust_ctx.xds_buffers[0].xds_class, Some(XdsClass::Current));
        assert_eq!(
            rust_ctx.xds_buffers[0].xds_type,
            Some(XdsType::CurrentFuture(XdsCurrentFutureType::ProgramName))
        );
        assert_eq!(rust_ctx.xds_buffers[0].used_bytes, 5);
    }

    #[test]
    fn test_copy_xds_context_scalars() {
        let mut rust_ctx = CcxDecodersXdsContext::default();
        rust_ctx.current_xds_min = 30;
        rust_ctx.current_xds_hour = 9;
        rust_ctx.current_xds_date = 25;
        rust_ctx.current_xds_month = 12;
        rust_ctx.cur_xds_buffer_idx = 3;
        rust_ctx.current_ar_start = 50;
        rust_ctx.current_ar_end = 150;

        let mut c_ctx = make_c_xds_ctx_zeroed();
        unsafe { copy_xds_context_from_rust_to_c(&mut c_ctx as *mut _, &rust_ctx) };

        assert_eq!(c_ctx.current_xds_min, 30);
        assert_eq!(c_ctx.current_xds_hour, 9);
        assert_eq!(c_ctx.current_xds_date, 25);
        assert_eq!(c_ctx.current_xds_month, 12);
        assert_eq!(c_ctx.cur_xds_buffer_idx, 3);
        assert_eq!(c_ctx.current_ar_start, 50);
        assert_eq!(c_ctx.current_ar_end, 150);
    }

    #[test]
    fn test_copy_xds_context_write_to_file() {
        let mut c_ctx = make_c_xds_ctx_zeroed();

        let mut rust_ctx = CcxDecodersXdsContext::default();
        rust_ctx.xds_write_to_file = false;
        unsafe { copy_xds_context_from_rust_to_c(&mut c_ctx as *mut _, &rust_ctx) };
        assert_eq!(c_ctx.xds_write_to_file, 0);

        rust_ctx.xds_write_to_file = true;
        unsafe { copy_xds_context_from_rust_to_c(&mut c_ctx as *mut _, &rust_ctx) };
        assert_eq!(c_ctx.xds_write_to_file, 1);
    }

    #[test]
    fn test_copy_xds_context_timing_preserved() {
        let mut c_ctx = make_c_xds_ctx_zeroed();
        let sentinel: usize = 0xDEAD_BEEF;
        c_ctx.timing = sentinel as *mut _;

        let rust_ctx = CcxDecodersXdsContext::default(); // timing = None
        unsafe { copy_xds_context_from_rust_to_c(&mut c_ctx as *mut _, &rust_ctx) };

        assert_eq!(c_ctx.timing as usize, sentinel);
    }

    #[test]
    fn test_copy_xds_context_null_ptr_noop() {
        let rust_ctx = CcxDecodersXdsContext::default();
        unsafe { copy_xds_context_from_rust_to_c(std::ptr::null_mut(), &rust_ctx) };
    }
}
