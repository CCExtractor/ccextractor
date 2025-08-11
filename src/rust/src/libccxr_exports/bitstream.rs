use crate::bindings::bitstream;
use lib_ccxr::common::BitStreamRust;
use lib_ccxr::info;
use std::os::raw::{c_int, c_uchar};

/// Copies the state from a Rust `BitStreamRust` to a C `bitstream` struct.
///
/// # Safety
/// This function is unsafe because it:
/// - Dereferences a raw pointer (`bitstream_ptr`)
/// - Performs pointer arithmetic and assumes valid memory layout
unsafe fn copy_bitstream_from_rust_to_c(
    bitstream_ptr: *mut bitstream,
    rust_bitstream: &BitStreamRust,
) {
    // Handle empty slice case
    if rust_bitstream.data.is_empty() {
        (*bitstream_ptr).pos = std::ptr::null_mut();
        (*bitstream_ptr).bpos = 8;
        (*bitstream_ptr).end = std::ptr::null_mut();
        (*bitstream_ptr).bitsleft = 0;
        (*bitstream_ptr).error = c_int::from(rust_bitstream.error);
        (*bitstream_ptr)._i_pos = std::ptr::null_mut();
        (*bitstream_ptr)._i_bpos = 0;
        return;
    }

    // Get the original pos (which is the base of our slice)
    let base_ptr = rust_bitstream.data.as_ptr() as *mut c_uchar;
    let end_ptr = base_ptr.add(rust_bitstream.data.len());

    // Current position should be base + rust pos
    let current_pos_ptr = base_ptr.add(rust_bitstream.pos);

    // Internal position should be base + _i_pos
    let i_pos_ptr = if rust_bitstream._i_pos <= rust_bitstream.data.len() {
        base_ptr.add(rust_bitstream._i_pos)
    } else {
        base_ptr
    };

    (*bitstream_ptr).pos = current_pos_ptr;
    (*bitstream_ptr).bpos = rust_bitstream.bpos as i32;
    (*bitstream_ptr).end = end_ptr;
    (*bitstream_ptr).bitsleft = rust_bitstream.bits_left;
    (*bitstream_ptr).error = c_int::from(rust_bitstream.error);
    (*bitstream_ptr)._i_pos = i_pos_ptr;
    (*bitstream_ptr)._i_bpos = rust_bitstream._i_bpos as i32;
}

/// Converts a C `bitstream` struct to a Rust `BitStreamRust` with static lifetime.
///
/// # Safety
/// This function is unsafe because it Dereferences a raw pointer (`value`) without null checking and
/// Performs pointer arithmetic and offset calculations assuming valid pointer relationships
unsafe fn copy_bitstream_c_to_rust(value: *mut bitstream) -> BitStreamRust<'static> {
    // We need to work with the original buffer, not just from current position
    let mut slice: &[u8] = &[];
    let mut current_pos_in_slice = 0;
    let mut i_pos_in_slice = 0;

    if !(*value).pos.is_null() && !(*value).end.is_null() {
        // Calculate total buffer length from pos to end
        let total_len = (*value).end.offset_from((*value).pos) as usize;

        if total_len > 0 {
            // Create slice from current position to end
            slice = std::slice::from_raw_parts((*value).pos, total_len);
            // Current position in this slice is 0 (since slice starts from current pos)
            current_pos_in_slice = 0;

            // Calculate _i_pos relative to the slice start (which is current pos)
            if !(*value)._i_pos.is_null() {
                let i_offset = (*value)._i_pos.offset_from((*value).pos);
                i_pos_in_slice = i_offset.max(0) as usize;
            }
        } else if (*value).pos == (*value).end {
            slice = std::slice::from_raw_parts((*value).pos, 1);
        }
    }

    BitStreamRust {
        data: slice,
        pos: current_pos_in_slice,
        bpos: (*value).bpos as u8,
        bits_left: (*value).bitsleft,
        error: (*value).error != 0,
        _i_pos: i_pos_in_slice,
        _i_bpos: (*value)._i_bpos as u8,
    }
}
/// Updates only the internal state of a C bitstream without modifying current position pointers.
/// Used by functions like `ccxr_next_bits` that peek at data without advancing the main position.
///
/// # Safety
/// This function is unsafe because it:
/// This function is unsafe because it Dereferences a raw pointer (`value`) without null checking and
/// Performs pointer arithmetic and offset calculations assuming valid pointer relationships
unsafe fn copy_internal_state_from_rust_to_c(
    bitstream_ptr: *mut bitstream,
    rust_bitstream: &BitStreamRust,
) {
    // Only update internal positions and bits_left, NOT current position
    (*bitstream_ptr).bitsleft = rust_bitstream.bits_left;
    (*bitstream_ptr).error = c_int::from(rust_bitstream.error);
    (*bitstream_ptr)._i_bpos = rust_bitstream._i_bpos as i32;

    // Handle _i_pos
    if rust_bitstream.data.is_empty() {
        (*bitstream_ptr)._i_pos = std::ptr::null_mut();
    } else {
        let base_ptr = rust_bitstream.data.as_ptr() as *mut c_uchar;
        let i_pos_ptr = if rust_bitstream._i_pos <= rust_bitstream.data.len() {
            base_ptr.add(rust_bitstream._i_pos)
        } else {
            base_ptr // Fallback to start if out of bounds
        };
        (*bitstream_ptr)._i_pos = i_pos_ptr;
    }
}

/// Free a bitstream created by Rust allocation functions.
///
/// # Safety
/// This function is unsafe because it Drops the `Box` created from a raw pointer (`bs`) without null checking.
#[no_mangle]
pub unsafe extern "C" fn ccxr_free_bitstream(bs: *mut BitStreamRust<'static>) {
    if !bs.is_null() {
        drop(Box::from_raw(bs));
    }
}

/// Read bits from a bitstream without advancing the position (peek operation).
///
/// # Safety
/// This function is unsafe because it calls unsafe functions `copy_bitstream_c_to_rust` and `copy_internal_state_from_rust_to_c`
#[no_mangle]
pub unsafe extern "C" fn ccxr_next_bits(bs: *mut bitstream, bnum: u32) -> u64 {
    let mut rust_bs = copy_bitstream_c_to_rust(bs);
    let val = match rust_bs.next_bits(bnum) {
        Ok(val) => val,
        Err(_) => return 0,
    };
    // Only copy back internal state, NOT current position
    copy_internal_state_from_rust_to_c(bs, &rust_bs);
    val
}

/// Read bits from a bitstream and advance the position.
///
/// # Safety
/// This function is unsafe because it calls unsafe functions `copy_bitstream_c_to_rust` and `copy_bitstream_from_rust_to_c`
#[no_mangle]
pub unsafe extern "C" fn ccxr_read_bits(bs: *mut bitstream, bnum: u32) -> u64 {
    let mut rust_bs = copy_bitstream_c_to_rust(bs);
    let val = match rust_bs.read_bits(bnum) {
        Ok(val) => val,
        Err(_) => return 0,
    };
    copy_bitstream_from_rust_to_c(bs, &rust_bs);
    val
}

/// Skip a number of bits in the bitstream.
///
/// # Safety
/// This function is unsafe because it calls unsafe functions `copy_bitstream_c_to_rust` and `copy_bitstream_from_rust_to_c`
#[no_mangle]
pub unsafe extern "C" fn ccxr_skip_bits(bs: *mut bitstream, bnum: u32) -> i32 {
    let mut rust_bs = copy_bitstream_c_to_rust(bs);
    let val = match rust_bs.skip_bits(bnum) {
        Ok(val) => val,
        Err(_) => return 0,
    };
    copy_bitstream_from_rust_to_c(bs, &rust_bs);
    if val {
        1
    } else {
        0
    }
}

/// Check if the bitstream is byte aligned.
///
/// # Safety
/// This function is unsafe because it calls unsafe functions `copy_bitstream_c_to_rust` and `copy_bitstream_from_rust_to_c`
#[no_mangle]
pub unsafe extern "C" fn ccxr_is_byte_aligned(bs: *mut bitstream) -> i32 {
    let rust_bs = copy_bitstream_c_to_rust(bs);
    match rust_bs.is_byte_aligned() {
        Ok(val) => {
            if val {
                1
            } else {
                0
            }
        }
        Err(_) => 0,
    }
}

/// Align the bitstream to the next byte boundary.
///
/// # Safety
/// This function is unsafe because it calls unsafe functions `copy_bitstream_c_to_rust` and `copy_bitstream_from_rust_to_c`
#[no_mangle]
pub unsafe extern "C" fn ccxr_make_byte_aligned(bs: *mut bitstream) {
    let mut rust_bs = copy_bitstream_c_to_rust(bs);
    if rust_bs.make_byte_aligned().is_ok() {
        copy_bitstream_from_rust_to_c(bs, &rust_bs);
    } else {
        info!("Bitstream : Failed to make bitstream byte aligned");
    }
}

/// Get a pointer to the next bytes in the bitstream without advancing the position.
///
/// # Safety
/// This function is unsafe because it calls unsafe functions `copy_bitstream_c_to_rust` and `copy_internal_state_from_rust_to_c`
#[no_mangle]
pub unsafe extern "C" fn ccxr_next_bytes(bs: *mut bitstream, bynum: usize) -> *const u8 {
    let mut rust_bs = copy_bitstream_c_to_rust(bs);
    match rust_bs.next_bytes(bynum) {
        Ok(slice) => {
            // Copy back internal state only (like next_bits does)
            copy_internal_state_from_rust_to_c(bs, &rust_bs);
            slice.as_ptr()
        }
        Err(_) => std::ptr::null(),
    }
}

/// Read bytes from the bitstream and advance the position.
///
/// # Safety
/// This function is unsafe because it calls unsafe functions `copy_bitstream_c_to_rust` and `copy_bitstream_from_rust_to_c`
#[no_mangle]
pub unsafe extern "C" fn ccxr_read_bytes(bs: *mut bitstream, bynum: usize) -> *const u8 {
    let mut rust_bs = copy_bitstream_c_to_rust(bs);
    match rust_bs.read_bytes(bynum) {
        Ok(slice) => {
            copy_bitstream_from_rust_to_c(bs, &rust_bs);
            slice.as_ptr()
        }
        Err(_) => std::ptr::null(),
    }
}
/// Read a multi-byte number from the bitstream with optional position advancement.
///
/// # Safety
/// This function is unsafe because it calls unsafe functions `copy_bitstream_c_to_rust` and `copy_bitstream_from_rust_to_c`
#[no_mangle]
pub unsafe extern "C" fn ccxr_bitstream_get_num(
    bs: *mut bitstream,
    bytes: usize,
    advance: i32,
) -> u64 {
    let mut rust_bs = copy_bitstream_c_to_rust(bs);
    let result = rust_bs.bitstream_get_num(bytes, advance != 0).unwrap_or(0);
    copy_bitstream_from_rust_to_c(bs, &rust_bs);
    result
}

/// Read an unsigned Exp-Golomb code from the bitstream.
///
/// # Safety
/// This function is unsafe because it calls unsafe functions `copy_bitstream_c_to_rust` and `copy_bitstream_from_rust_to_c`
#[no_mangle]
pub unsafe extern "C" fn ccxr_read_exp_golomb_unsigned(bs: *mut bitstream) -> u64 {
    let mut rust_bs = copy_bitstream_c_to_rust(bs);
    let result = rust_bs.read_exp_golomb_unsigned().unwrap_or(0);
    copy_bitstream_from_rust_to_c(bs, &rust_bs);
    result
}

/// Read a signed Exp-Golomb code from the bitstream.
///
/// # Safety
/// This function is unsafe because it calls unsafe functions `copy_bitstream_c_to_rust` and `copy_bitstream_from_rust_to_c`
#[no_mangle]
pub unsafe extern "C" fn ccxr_read_exp_golomb(bs: *mut bitstream) -> i64 {
    let mut rust_bs = copy_bitstream_c_to_rust(bs);
    let result = rust_bs.read_exp_golomb().unwrap_or(0);
    copy_bitstream_from_rust_to_c(bs, &rust_bs);
    result
}
/// Read a signed integer value from the specified number of bits.
///
/// # Safety
/// This function is unsafe because it calls unsafe functions `copy_bitstream_c_to_rust` and `copy_bitstream_from_rust_to_c`
#[no_mangle]
pub unsafe extern "C" fn ccxr_read_int(bs: *mut bitstream, bnum: u32) -> i64 {
    let mut rust_bs = copy_bitstream_c_to_rust(bs);
    let result = rust_bs.read_int(bnum).unwrap_or(0);
    copy_bitstream_from_rust_to_c(bs, &rust_bs);
    result
}

/// Reverse the bits in a byte (bit 0 becomes bit 7, etc.).
///
/// # Safety
/// This function is marked unsafe only because it's part of the FFI interface.
#[no_mangle]
pub unsafe extern "C" fn ccxr_reverse8(data: u8) -> u8 {
    BitStreamRust::reverse8(data)
}

mod tests {
    // FFI binding tests
    #[test]
    fn test_ffi_next_bits() {
        let data = vec![0b10101010];
        let mut c_bs = crate::bindings::bitstream {
            pos: data.as_ptr() as *mut u8,
            bpos: 8,
            end: unsafe { data.as_ptr().add(data.len()) } as *mut u8,
            bitsleft: (data.len() as i64) * 8,
            error: 0,
            _i_pos: data.as_ptr() as *mut u8,
            _i_bpos: 8,
        };

        assert_eq!(unsafe { super::ccxr_next_bits(&mut c_bs, 1) }, 1);
    }

    #[test]
    fn test_ffi_read_bits() {
        let data = vec![0b10101010];
        let mut c_bs = crate::bindings::bitstream {
            pos: data.as_ptr() as *mut u8,
            bpos: 8,
            end: unsafe { data.as_ptr().add(data.len()) } as *mut u8,
            bitsleft: (data.len() as i64) * 8,
            error: 0,
            _i_pos: data.as_ptr() as *mut u8,
            _i_bpos: 8,
        };

        assert_eq!(unsafe { super::ccxr_read_bits(&mut c_bs, 3) }, 0b101);
    }

    #[test]
    fn test_ffi_byte_alignment() {
        let data = vec![0xFF];
        let mut c_bs = crate::bindings::bitstream {
            pos: data.as_ptr() as *mut u8,
            bpos: 8,
            end: unsafe { data.as_ptr().add(data.len()) } as *mut u8,
            bitsleft: (data.len() as i64) * 8,
            error: 0,
            _i_pos: data.as_ptr() as *mut u8,
            _i_bpos: 8,
        };

        assert_eq!(
            unsafe { super::ccxr_is_byte_aligned(&mut c_bs as *mut crate::bindings::bitstream) },
            1
        );
        unsafe { super::ccxr_read_bits(&mut c_bs, 1) };
        assert_eq!(
            unsafe { super::ccxr_is_byte_aligned(&mut c_bs as *mut crate::bindings::bitstream) },
            0
        );
    }

    #[test]
    fn test_ffi_read_bytes() {
        static DATA: [u8; 3] = [0xAA, 0xBB, 0xCC];
        let mut c_bs = crate::bindings::bitstream {
            pos: DATA.as_ptr() as *mut u8,
            bpos: 8,
            end: unsafe { DATA.as_ptr().add(DATA.len()) } as *mut u8,
            bitsleft: (DATA.len() as i64) * 8,
            error: 0,
            _i_pos: DATA.as_ptr() as *mut u8,
            _i_bpos: 8,
        };

        unsafe {
            let ptr = super::ccxr_read_bytes(&mut c_bs, 2);
            assert!(!ptr.is_null());
            let b1 = *ptr;
            let b2 = *ptr.add(1);
            assert_eq!([b1, b2], [0xAA, 0xBB]);
        }
    }

    #[test]
    fn test_ffi_exp_golomb() {
        let data = vec![0b10000000];
        let data_ptr = data.as_ptr();
        let mut c_bs = crate::bindings::bitstream {
            pos: data_ptr as *mut u8,
            bpos: 8,
            end: unsafe { data_ptr.add(data.len()) } as *mut u8,
            bitsleft: (data.len() as i64) * 8,
            error: 0,
            _i_pos: data_ptr as *mut u8,
            _i_bpos: 8,
        };

        assert_eq!(
            unsafe { super::ccxr_read_exp_golomb_unsigned(&mut c_bs) },
            0
        );
        drop(data);
    }

    #[test]
    fn test_ffi_reverse8() {
        assert_eq!(unsafe { super::ccxr_reverse8(0b10101010) }, 0b01010101);
    }

    #[test]
    fn test_ffi_state_updates() {
        let data = vec![0xAA, 0xBB];
        let mut c_bs = crate::bindings::bitstream {
            pos: data.as_ptr() as *mut u8,
            bpos: 8,
            end: unsafe { data.as_ptr().add(data.len()) } as *mut u8,
            bitsleft: (data.len() as i64) * 8,
            error: 0,
            _i_pos: data.as_ptr() as *mut u8,
            _i_bpos: 8,
        };

        unsafe { super::ccxr_read_bits(&mut c_bs, 4) };
        assert_eq!(c_bs.bpos, 4);
    }
}
#[cfg(test)]
mod bitstream_copying_tests {
    use super::*;
    use std::ptr;

    // Test helper function to create a test buffer
    fn create_test_buffer(size: usize) -> Vec<u8> {
        (0..size).map(|i| (i % 256) as u8).collect()
    }

    // Test helper to verify pointer arithmetic safety
    unsafe fn verify_pointer_bounds(c_stream: &bitstream) -> bool {
        !c_stream.pos.is_null() && !c_stream.end.is_null() && c_stream.pos <= c_stream.end
    }

    #[test]
    fn test_rust_to_c_basic_conversion() {
        let buffer = create_test_buffer(100);
        let rust_stream = BitStreamRust {
            data: &buffer,
            pos: 0,
            bpos: 3,
            bits_left: 789,
            error: false,
            _i_pos: 10,
            _i_bpos: 5,
        };

        unsafe {
            let c_s = Box::into_raw(Box::new(bitstream::default()));
            copy_bitstream_from_rust_to_c(c_s, &rust_stream);
            let c_stream = &*c_s;
            // Verify basic field conversions
            assert_eq!(c_stream.bpos, 3);
            assert_eq!(c_stream.bitsleft, 789);
            assert_eq!(c_stream.error, 0);
            assert_eq!(c_stream._i_bpos, 5);

            // Verify pointer arithmetic
            assert!(verify_pointer_bounds(&c_stream));
            assert_eq!(c_stream.end.offset_from(c_stream.pos), 100);
            assert_eq!(c_stream._i_pos.offset_from(c_stream.pos), 10);

            // Verify data integrity
            let reconstructed_slice = std::slice::from_raw_parts(c_stream.pos, 100);
            assert_eq!(reconstructed_slice, &buffer[..]);
        }
    }

    #[test]
    fn test_c_to_rust_basic_conversion() {
        let mut buffer = create_test_buffer(50);
        let buffer_ptr = buffer.as_mut_ptr();

        unsafe {
            let mut c_stream = bitstream {
                pos: buffer_ptr,
                bpos: 7,
                end: buffer_ptr.add(50),
                bitsleft: 400,
                error: 1,
                _i_pos: buffer_ptr.add(15),
                _i_bpos: 2,
            };

            let rust_stream = copy_bitstream_c_to_rust(&mut c_stream as *mut bitstream);

            // Verify basic field conversions
            assert_eq!(rust_stream.bpos, 7);
            assert_eq!(rust_stream.bits_left, 400);
            assert_eq!(rust_stream.error, true);
            assert_eq!(rust_stream._i_pos, 15);
            assert_eq!(rust_stream._i_bpos, 2);

            // Verify slice reconstruction
            assert_eq!(rust_stream.data.len(), 50);
            assert_eq!(rust_stream.data, &buffer[..]);
        }
    }

    #[test]
    fn test_round_trip_conversion() {
        let buffer = create_test_buffer(75);
        let original = BitStreamRust {
            data: &buffer,
            pos: 0,
            bpos: 4,
            bits_left: 600,
            error: true,
            _i_pos: 25,
            _i_bpos: 1,
        };

        unsafe {
            let c_s = Box::into_raw(Box::new(bitstream::default()));
            copy_bitstream_from_rust_to_c(c_s, &original);
            let reconstructed = copy_bitstream_c_to_rust(c_s);

            // Verify all fields match
            assert_eq!(reconstructed.bpos, original.bpos);
            assert_eq!(reconstructed.bits_left, original.bits_left);
            assert_eq!(reconstructed.error, original.error);
            assert_eq!(reconstructed._i_pos, original._i_pos);
            assert_eq!(reconstructed._i_bpos, original._i_bpos);
            assert_eq!(reconstructed.data, original.data);
        }
    }

    #[test]
    fn test_empty_buffer() {
        let buffer: &[u8] = &[];
        let rust_stream = BitStreamRust {
            data: buffer,
            pos: 0,
            bpos: 0,
            bits_left: 0,
            error: false,
            _i_pos: 0,
            _i_bpos: 0,
        };

        unsafe {
            let c_s = Box::into_raw(Box::new(bitstream::default()));
            copy_bitstream_from_rust_to_c(c_s, &rust_stream);
            let c_stream = &mut *c_s;

            // Verify empty buffer handling
            assert_eq!(c_stream.end.offset_from(c_stream.pos), 0);
            assert_eq!(c_stream._i_pos.offset_from(c_stream.pos), 0);

            let reconstructed = copy_bitstream_c_to_rust(c_s);
            assert_eq!(reconstructed.data.len(), 0);
            assert_eq!(reconstructed._i_pos, 0);
        }
    }

    #[test]
    fn test_single_byte_buffer() {
        let buffer = vec![0x42u8];
        let rust_stream = BitStreamRust {
            data: &buffer,
            pos: 0,
            bpos: 7,
            bits_left: 1,
            error: false,
            _i_pos: 0,
            _i_bpos: 3,
        };

        unsafe {
            let c_s = Box::into_raw(Box::new(bitstream::default()));
            copy_bitstream_from_rust_to_c(c_s, &rust_stream);
            let c_stream = &mut *c_s;
            assert_eq!(c_stream.end.offset_from(c_stream.pos), 1);

            let reconstructed = copy_bitstream_c_to_rust(c_s);
            assert_eq!(reconstructed.data.len(), 1);
            assert_eq!(reconstructed.data[0], 0x42);
        }
    }

    #[test]
    fn test_large_buffer() {
        let buffer = create_test_buffer(10000);
        let rust_stream = BitStreamRust {
            data: &buffer,
            pos: 0,
            bpos: 2,
            bits_left: 80000,
            error: false,
            _i_pos: 5000,
            _i_bpos: 6,
        };

        unsafe {
            let c_s = Box::into_raw(Box::new(bitstream::default()));
            copy_bitstream_from_rust_to_c(c_s, &rust_stream);
            let c_stream = &mut *c_s;
            assert_eq!(c_stream.end.offset_from(c_stream.pos), 10000);
            assert_eq!(c_stream._i_pos.offset_from(c_stream.pos), 5000);

            let reconstructed = copy_bitstream_c_to_rust(c_s);
            assert_eq!(reconstructed.data.len(), 10000);
            assert_eq!(reconstructed._i_pos, 5000);
        }
    }

    #[test]
    fn test_boundary_values() {
        let buffer = create_test_buffer(256);

        // Test with maximum values
        let rust_stream = BitStreamRust {
            data: &buffer,
            pos: 0,
            bpos: 7, // Max value for 3 bits
            bits_left: i64::MAX,
            error: true,
            _i_pos: 255, // Last valid index
            _i_bpos: 7,
        };

        unsafe {
            let c_s = Box::into_raw(Box::new(bitstream::default()));
            copy_bitstream_from_rust_to_c(c_s, &rust_stream);
            let reconstructed = copy_bitstream_c_to_rust(c_s);

            assert_eq!(reconstructed.bpos, 7);
            assert_eq!(reconstructed.bits_left, i64::MAX);
            assert_eq!(reconstructed.error, true);
            assert_eq!(reconstructed._i_pos, 255);
            assert_eq!(reconstructed._i_bpos, 7);
        }
    }

    #[test]
    fn test_negative_values() {
        let buffer = create_test_buffer(50);
        let rust_stream = BitStreamRust {
            data: &buffer,
            pos: 0,
            bpos: 0,
            bits_left: -100, // Negative bits_left
            error: false,    // Negative error code
            _i_pos: 0,
            _i_bpos: 0,
        };

        unsafe {
            let c_s = Box::into_raw(Box::new(bitstream::default()));
            copy_bitstream_from_rust_to_c(c_s, &rust_stream);
            let reconstructed = copy_bitstream_c_to_rust(c_s);

            assert_eq!(reconstructed.bits_left, -100);
            assert_eq!(reconstructed.error, false);
        }
    }

    #[test]
    fn test_null_i_pos_handling() {
        let mut buffer = create_test_buffer(30);
        let buffer_ptr = buffer.as_mut_ptr();

        unsafe {
            let mut c_stream = bitstream {
                pos: buffer_ptr,
                bpos: 0,
                end: buffer_ptr.add(30),
                bitsleft: 240,
                error: 0,
                _i_pos: ptr::null_mut(), // Null _i_pos
                _i_bpos: 0,
            };

            let rust_stream = copy_bitstream_c_to_rust(&mut c_stream as *mut bitstream);

            // Should default to 0 when _i_pos is null
            assert_eq!(rust_stream._i_pos, 0);
        }
    }

    #[test]
    fn test_different_buffer_positions() {
        let buffer = create_test_buffer(100);

        // Test different _i_pos values
        for i_pos in [0, 1, 50, 99] {
            let rust_stream = BitStreamRust {
                data: &buffer,
                pos: 0,
                bpos: 0,
                bits_left: 800,
                error: false,
                _i_pos: i_pos,
                _i_bpos: 0,
            };

            unsafe {
                let c_s = Box::into_raw(Box::new(bitstream::default()));
                copy_bitstream_from_rust_to_c(c_s, &rust_stream);
                let c_stream = &mut *c_s;
                assert_eq!(c_stream._i_pos.offset_from(c_stream.pos), i_pos as isize);

                let reconstructed = copy_bitstream_c_to_rust(c_s);
                assert_eq!(reconstructed._i_pos, i_pos);
            }
        }
    }

    #[test]
    fn test_data_integrity_after_conversion() {
        // Create a buffer with specific pattern
        let mut buffer = Vec::new();
        for i in 0..256 {
            buffer.push(i as u8);
        }

        let rust_stream = BitStreamRust {
            data: &buffer,
            pos: 0,
            bpos: 3,
            bits_left: 2048,
            error: false,
            _i_pos: 128,
            _i_bpos: 4,
        };

        unsafe {
            let c_s = Box::into_raw(Box::new(bitstream::default()));
            copy_bitstream_from_rust_to_c(c_s, &rust_stream);
            let c_stream = &mut *c_s;

            // Verify we can read the data correctly through C pointers
            for i in 0..256 {
                let byte_val = *c_stream.pos.add(i);
                assert_eq!(byte_val, i as u8);
            }

            // Verify internal position pointer
            let internal_byte = *c_stream._i_pos;
            assert_eq!(internal_byte, 128);

            let reconstructed = copy_bitstream_c_to_rust(c_s);
            assert_eq!(reconstructed.data.len(), 256);
            for (i, &byte) in reconstructed.data.iter().enumerate() {
                assert_eq!(byte, i as u8);
            }
        }
    }

    #[test]
    fn test_multiple_conversions() {
        let buffer = create_test_buffer(64);
        let mut rust_stream = BitStreamRust {
            data: &buffer,
            pos: 0,
            bpos: 1,
            bits_left: 512,
            error: false,
            _i_pos: 32,
            _i_bpos: 2,
        };

        // Test multiple round-trip conversions
        for _ in 0..5 {
            rust_stream.error = true;

            unsafe {
                let c_s = Box::into_raw(Box::new(bitstream::default()));
                copy_bitstream_from_rust_to_c(c_s, &rust_stream);
                let new_rust_stream = copy_bitstream_c_to_rust(c_s);

                assert_eq!(new_rust_stream.error, true);
                assert_eq!(new_rust_stream.data.len(), 64);
                assert_eq!(new_rust_stream._i_pos, 32);

                rust_stream = new_rust_stream;
            }
        }
    }

    #[test]
    fn test_bit_position_values() {
        let buffer = create_test_buffer(10);

        // Test all valid bit positions (0-7)
        for bpos in 0..8 {
            let rust_stream = BitStreamRust {
                data: &buffer,
                pos: 0,
                bpos: bpos as u8,
                bits_left: 80,
                error: false,
                _i_pos: 5,
                _i_bpos: (7 - bpos) as u8,
            };

            unsafe {
                let c_s = Box::into_raw(Box::new(bitstream::default()));
                copy_bitstream_from_rust_to_c(c_s, &rust_stream);
                let c_stream = &mut *c_s;
                assert_eq!(c_stream.bpos, bpos);
                assert_eq!(c_stream._i_bpos, 7 - bpos);

                let reconstructed = copy_bitstream_c_to_rust(c_s);
                assert_eq!(reconstructed.bpos, bpos as u8);
                assert_eq!(reconstructed._i_bpos, (7 - bpos) as u8);
            }
        }
    }

    #[test]
    fn test_memory_safety() {
        let buffer = create_test_buffer(1000);
        let rust_stream = BitStreamRust {
            data: &buffer,
            pos: 0,
            bpos: 0,
            bits_left: 8000,
            error: false,
            _i_pos: 500,
            _i_bpos: 0,
        };

        unsafe {
            let c_s = Box::into_raw(Box::new(bitstream::default()));
            copy_bitstream_from_rust_to_c(c_s, &rust_stream);
            let c_stream = &mut *c_s;

            // Verify all pointers are within bounds
            assert!(verify_pointer_bounds(&c_stream));

            // Verify we can safely access the boundaries
            let first_byte = *c_stream.pos;
            let last_byte = *c_stream.end.sub(1);
            let internal_byte = *c_stream._i_pos;

            // These should not panic and should match our buffer
            assert_eq!(first_byte, 0);
            assert_eq!(last_byte, (999 % 256) as u8);
            assert_eq!(internal_byte, (500 % 256) as u8);
        }
    }
}
