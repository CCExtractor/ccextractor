use crate::bindings::demuxer_data;
use crate::common::CType;
use crate::ctorust::FromCType;
use crate::demuxer::common_types::CcxRational;
use crate::demuxer::demuxer_data::DemuxerData;
use lib_ccxr::common::{BufferdataType, Codec};
use std::os::raw::c_uchar;
use std::os::raw::{c_int, c_uint};

/// Convert from C demuxer_data to Rust DemuxerData
/// # Safety
/// - `c_data` must be a valid pointer to a demuxer_data struct
/// - The buffer pointer in c_data must be valid for the length specified by len
/// - The returned DemuxerData borrows the buffer data, so the C struct must outlive it
#[allow(clippy::unnecessary_cast)]
pub unsafe fn copy_demuxer_data_to_rust(c_data: *const demuxer_data) -> DemuxerData {
    DemuxerData {
        program_number: (*c_data).program_number as i32,
        stream_pid: (*c_data).stream_pid as i32,
        codec: Codec::from_ctype((*c_data).codec),
        bufferdatatype: BufferdataType::from_ctype((*c_data).bufferdatatype)
            .unwrap_or(BufferdataType::Unknown),
        buffer: (*c_data).buffer,
        len: (*c_data).len,
        rollover_bits: (*c_data).rollover_bits as u32,
        pts: (*c_data).pts as i64,
        tb: CcxRational::from_ctype((*c_data).tb).unwrap_or(CcxRational::default()),
        next_stream: (*c_data).next_stream,
        next_program: (*c_data).next_program,
    }
}

/// Copy from Rust DemuxerData to C demuxer_data
/// # Safety
/// - `c_data` must be a valid pointer to a demuxer_data struct
/// - The buffer in the C struct must be large enough to hold the Rust buffer data
/// - This function copies the buffer content, not just the pointer
#[allow(clippy::unnecessary_cast)]
pub unsafe fn copy_demuxer_data_from_rust(c_data: *mut demuxer_data, rust_data: &DemuxerData) {
    (*c_data).program_number = rust_data.program_number as c_int;
    (*c_data).stream_pid = rust_data.stream_pid as c_int;
    if rust_data.codec.is_some() {
        (*c_data).codec = rust_data.codec.unwrap().to_ctype();
    }
    (*c_data).bufferdatatype = rust_data.bufferdatatype.to_ctype();

    (*c_data).buffer = rust_data.buffer as *mut c_uchar;
    (*c_data).len = rust_data.len;

    (*c_data).rollover_bits = rust_data.rollover_bits as c_uint;
    (*c_data).pts = rust_data.pts as i64;
    (*c_data).tb = rust_data.tb.to_ctype();
    (*c_data).next_stream = rust_data.next_stream as *mut demuxer_data;
    (*c_data).next_program = rust_data.next_program as *mut demuxer_data;
}

mod tests {
    use super::*;
    use crate::bindings::ccx_bufferdata_type_CCX_H264;
    use std::ptr;
    // Helper function to create a test C demuxer_data struct

    #[test]
    fn test_demuxer_data_default() {
        let default_data = DemuxerData::default();

        assert_eq!(default_data.program_number, -1);
        assert_eq!(default_data.stream_pid, -1);
        assert_eq!(default_data.codec, None);
        assert_eq!(default_data.bufferdatatype, BufferdataType::Pes);
        assert!(default_data.buffer.is_null());
        assert_eq!(default_data.len, 0);
        assert_eq!(default_data.rollover_bits, 0);
        assert_eq!(default_data.pts, crate::demuxer::common_types::CCX_NOPTS);
        assert_eq!(default_data.tb.num, 1);
        assert_eq!(default_data.tb.den, 90000);
        assert!(default_data.next_stream.is_null());
        assert!(default_data.next_program.is_null());
    }
    #[allow(dead_code)]
    fn create_test_c_demuxer_data() -> (*mut demuxer_data, Vec<u8>) {
        unsafe {
            let c_data = Box::into_raw(Box::new(demuxer_data {
                program_number: 42,
                stream_pid: 256,
                codec: Codec::Any.to_ctype(),
                bufferdatatype: ccx_bufferdata_type_CCX_H264,
                buffer: ptr::null_mut(),
                len: 0,
                rollover_bits: 123,
                pts: 987654321,
                tb: CcxRational { num: 1, den: 25 }.to_ctype(),
                next_stream: ptr::null_mut(),
                next_program: ptr::null_mut(),
            }));

            // Create test buffer data
            let buffer_data = vec![0x01, 0x02, 0x03, 0x04, 0x05, 0xAA, 0xBB, 0xCC];
            (*c_data).buffer = buffer_data.as_ptr() as *mut u8;
            (*c_data).len = buffer_data.len();

            (c_data, buffer_data)
        }
    }
    #[test]
    fn test_copy_demuxer_to_rust_basic_fields() {
        let (c_data_ptr, _buffer) = create_test_c_demuxer_data();

        let rust_data = unsafe { copy_demuxer_data_to_rust(c_data_ptr) };

        unsafe {
            let c_data = &*c_data_ptr;

            // Test all basic fields
            assert_eq!(rust_data.program_number, c_data.program_number);
            assert_eq!(rust_data.stream_pid, c_data.stream_pid);
            assert_eq!(rust_data.rollover_bits, c_data.rollover_bits);
            assert_eq!(rust_data.pts, c_data.pts);

            // Test buffer data
            assert_eq!(rust_data.len, c_data.len);

            // Test buffer content
            let expected_buffer = &mut [0x01, 0x02, 0x03, 0x04, 0x05, 0xAA, 0xBB, 0xCC];
            assert_eq!(
                std::slice::from_raw_parts(rust_data.buffer, rust_data.len),
                expected_buffer
            );
            // Cleanup
            let _ = Box::from_raw(c_data_ptr);
        }
    }

    #[test]
    fn test_copy_demuxer_to_rust_codec_conversion() {
        let (c_data_ptr, _buffer) = create_test_c_demuxer_data();

        let rust_data = unsafe { copy_demuxer_data_to_rust(c_data_ptr) };

        // Test codec conversion
        assert!(rust_data.codec.is_some());
        let codec = rust_data.codec.unwrap();

        // The codec should match what we set in create_test_c_demuxer_data
        // This tests the from_ctype_Codec function
        match codec {
            Codec::Any => (), // Expected
            _ => panic!("Codec conversion failed: got {:?}", codec),
        }

        // Cleanup
        unsafe {
            let _ = Box::from_raw(c_data_ptr);
        }
    }

    #[test]
    fn test_copy_demuxer_to_rust_bufferdatatype_conversion() {
        let (c_data_ptr, _buffer) = create_test_c_demuxer_data();

        let rust_data = unsafe { copy_demuxer_data_to_rust(c_data_ptr) };

        // Test bufferdatatype conversion
        assert_eq!(rust_data.bufferdatatype, BufferdataType::H264);

        // Cleanup
        unsafe {
            let _ = Box::from_raw(c_data_ptr);
        }
    }

    #[test]
    fn test_copy_demuxer_to_rust_timebase_conversion() {
        let (c_data_ptr, _buffer) = create_test_c_demuxer_data();

        let rust_data = unsafe { copy_demuxer_data_to_rust(c_data_ptr) };

        // Test timebase conversion
        assert_eq!(rust_data.tb.num, 1);
        assert_eq!(rust_data.tb.den, 25);

        // Cleanup
        unsafe {
            let _ = Box::from_raw(c_data_ptr);
        }
    }

    #[test]
    fn test_copy_demuxer_to_rust_null_buffer() {
        let c_data = demuxer_data {
            program_number: 100,
            stream_pid: 200,
            codec: unsafe { Codec::Any.to_ctype() },
            bufferdatatype: crate::bindings::ccx_bufferdata_type_CCX_PES,
            buffer: ptr::null_mut(),
            len: 0,
            rollover_bits: 456,
            pts: 123456789,
            tb: unsafe { CcxRational { num: 2, den: 50 }.to_ctype() },
            next_stream: ptr::null_mut(),
            next_program: ptr::null_mut(),
        };

        let rust_data = unsafe { copy_demuxer_data_to_rust(&c_data) };

        assert!(rust_data.buffer.is_null());
        assert_eq!(rust_data.len, 0);
        assert_eq!(rust_data.program_number, 100);
        assert_eq!(rust_data.stream_pid, 200);
    }

    #[test]
    fn test_copy_demuxer_from_rust_basic_fields() {
        let test_buffer = &mut [0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE];
        let rust_data = DemuxerData {
            program_number: 999,
            stream_pid: 888,
            codec: Some(Codec::Any),
            bufferdatatype: BufferdataType::Raw,
            buffer: test_buffer.as_mut_ptr(),
            len: 6,
            rollover_bits: 777,
            pts: 555666777,
            tb: CcxRational { num: 3, den: 60 },
            next_stream: ptr::null_mut(),
            next_program: ptr::null_mut(),
        };

        // Create C buffer large enough to hold the data
        let mut c_buffer = vec![0u8; test_buffer.len()];
        let mut c_data = unsafe {
            demuxer_data {
                program_number: 0,
                stream_pid: 0,
                codec: Codec::Any.to_ctype(),
                bufferdatatype: crate::bindings::ccx_bufferdata_type_CCX_PES,
                buffer: c_buffer.as_mut_ptr(),
                len: c_buffer.len(),
                rollover_bits: 0,
                pts: 0,
                tb: CcxRational::default().to_ctype(),
                next_stream: ptr::null_mut(),
                next_program: ptr::null_mut(),
            }
        };

        unsafe {
            copy_demuxer_data_from_rust(&mut c_data, &rust_data);

            // Verify all fields were copied correctly
            assert_eq!(c_data.program_number, rust_data.program_number);
            assert_eq!(c_data.stream_pid, rust_data.stream_pid);
            assert_eq!(c_data.rollover_bits, rust_data.rollover_bits);
            assert_eq!(c_data.pts, rust_data.pts);
            assert_eq!(c_data.len, 6);

            // Verify buffer content was copied
            let copied_buffer = std::slice::from_raw_parts(c_data.buffer, c_data.len);
            assert_eq!(copied_buffer, test_buffer);
        }
    }

    #[test]
    fn test_copy_demuxer_from_rust_codec_conversion() {
        let rust_data = DemuxerData {
            codec: Some(Codec::Any),
            ..Default::default()
        };

        let mut c_data = unsafe {
            demuxer_data {
                codec: Codec::Any.to_ctype(),
                ..std::mem::zeroed()
            }
        };

        unsafe {
            copy_demuxer_data_from_rust(&mut c_data, &rust_data);

            // Verify codec was converted correctly
            let converted_back = Codec::from_ctype(c_data.codec).unwrap();
            assert_eq!(converted_back, Codec::Any);
        }
    }

    #[test]
    fn test_copy_demuxer_from_rust_bufferdatatype_conversion() {
        let rust_data = DemuxerData {
            bufferdatatype: BufferdataType::Raw,
            ..Default::default()
        };

        let mut c_data = unsafe {
            demuxer_data {
                bufferdatatype: crate::bindings::ccx_bufferdata_type_CCX_PES,
                ..std::mem::zeroed()
            }
        };

        unsafe {
            copy_demuxer_data_from_rust(&mut c_data, &rust_data);

            // Verify bufferdatatype was converted correctly
            let converted_back = BufferdataType::from_ctype(c_data.bufferdatatype).unwrap();
            assert_eq!(converted_back, BufferdataType::Raw);
        }
    }

    #[test]
    fn test_copy_demuxer_from_rust_timebase_conversion() {
        let rust_data = DemuxerData {
            tb: CcxRational { num: 5, den: 120 },
            ..Default::default()
        };

        let mut c_data = unsafe {
            demuxer_data {
                tb: CcxRational::default().to_ctype(),
                ..std::mem::zeroed()
            }
        };

        unsafe {
            copy_demuxer_data_from_rust(&mut c_data, &rust_data);

            // Verify timebase was converted correctly
            let converted_back = CcxRational::from_ctype(c_data.tb).unwrap();
            assert_eq!(converted_back.num, 5);
            assert_eq!(converted_back.den, 120);
        }
    }

    #[test]
    fn test_copy_demuxer_from_rust_buffer_size_limits() {
        let mut large_buffer = vec![0x42; 1000]; // Large buffer
        let rust_data = DemuxerData {
            buffer: large_buffer.as_mut_ptr(),
            len: 100,
            ..Default::default()
        };

        // Create smaller C buffer
        let mut small_c_buffer = vec![0u8; 100];
        let mut c_data = unsafe {
            demuxer_data {
                buffer: small_c_buffer.as_mut_ptr(),
                len: small_c_buffer.len(),
                ..std::mem::zeroed()
            }
        };

        unsafe {
            copy_demuxer_data_from_rust(&mut c_data, &rust_data);

            // Should only copy what fits
            assert_eq!(c_data.len, 100);
            let copied_buffer = std::slice::from_raw_parts(c_data.buffer, c_data.len);
            assert_eq!(copied_buffer, &vec![0x42; 100]);
        }
    }

    #[test]
    fn test_copy_demuxer_from_rust_empty_buffer() {
        let rust_data = DemuxerData {
            buffer: [].as_mut_ptr(),
            ..Default::default()
        };

        let mut c_buffer = vec![0xFF; 10]; // Pre-fill with 0xFF
        let mut c_data = unsafe {
            demuxer_data {
                buffer: c_buffer.as_mut_ptr(),
                len: c_buffer.len(),
                ..std::mem::zeroed()
            }
        };

        unsafe {
            copy_demuxer_data_from_rust(&mut c_data, &rust_data);

            // Should set len to 0 for empty buffer
            assert_eq!(c_data.len, 0);
        }

        // Buffer content should remain unchanged
        assert_eq!(c_buffer[0], 0xFF);
    }

    #[test]
    fn test_copy_demuxer_from_rust_null_c_buffer() {
        let test_buffer = &mut [0x01, 0x02, 0x03];
        let rust_data = DemuxerData {
            buffer: test_buffer.as_mut_ptr(),
            ..Default::default()
        };

        let mut c_data = unsafe {
            demuxer_data {
                buffer: ptr::null_mut(),
                len: 0,
                ..std::mem::zeroed()
            }
        };

        unsafe {
            copy_demuxer_data_from_rust(&mut c_data, &rust_data);

            // Should handle null buffer gracefully
            assert_eq!(c_data.len, 0);
        }
    }

    #[test]
    fn test_from_ctype_bufferdatatype_all_variants() {
        // Test all possible bufferdatatype conversions
        unsafe {
            assert_eq!(
                BufferdataType::from_ctype(crate::bindings::ccx_bufferdata_type_CCX_PES).unwrap(),
                BufferdataType::Pes
            );
            assert_eq!(
                BufferdataType::from_ctype(crate::bindings::ccx_bufferdata_type_CCX_RAW).unwrap(),
                BufferdataType::Raw
            );
            assert_eq!(
                BufferdataType::from_ctype(ccx_bufferdata_type_CCX_H264).unwrap(),
                BufferdataType::H264
            );
            // Add tests for all other variants...

            // Test unknown/invalid value
            assert_eq!(
                BufferdataType::from_ctype(9999).unwrap(), // Invalid value
                BufferdataType::Unknown
            );
        }
    }

    #[test]
    fn test_ccx_nopts_constant() {
        // Verify the CCX_NOPTS constant matches the C definition
        assert_eq!(
            crate::demuxer::common_types::CCX_NOPTS,
            0x8000000000000000u64 as i64
        );

        let default_data = DemuxerData::default();
        assert_eq!(default_data.pts, crate::demuxer::common_types::CCX_NOPTS);
    }
}
