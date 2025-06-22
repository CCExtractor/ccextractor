//! Some utility functions to deal with values from C bindings
use byteorder::{ByteOrder, NetworkEndian};
use std::{ffi, ptr, slice};

/// Check if the value is true (Set to 1). Use only for values from C bindings
pub fn is_true<T: Into<i32>>(val: T) -> bool {
    val.into() == 1
}

/// Check if the value is false (Set to 0). Use only for values from C bindings
pub fn is_false<T: Into<i32>>(val: T) -> bool {
    val.into() == 0
}

/// Convert a C string to a Rust string
/// # Safety
/// The pointer passed in has to be a valid C string
pub unsafe fn c_char_to_string(c: *const ::std::os::raw::c_char) -> Option<String> {
    if c.is_null() {
        return None;
    }
    let c_str = ffi::CStr::from_ptr(c);
    Some(c_str.to_string_lossy().into_owned())
}

/// function to convert Rust literals to C strings to be passed into functions
/// # Safety
/// The pointer returned has to be deallocated using from_raw() at some point
pub fn string_to_c_char(a: &str) -> *mut ::std::os::raw::c_char {
    if a.is_empty() {
        return null_pointer();
    }
    let s = ffi::CString::new(a).unwrap();

    s.into_raw()
}

/// # Safety
/// The pointer returned has to be deallocated using from_raw() at some point
pub fn null_pointer<T>() -> *mut T {
    std::ptr::null_mut()
}

use std::os::raw::c_char;

pub fn string_to_c_chars(strs: Vec<String>) -> *mut *mut c_char {
    let mut c_strs: Vec<*mut c_char> = Vec::new();
    for s in strs {
        c_strs.push(string_to_c_char(&s));
    }
    let ptr = c_strs.as_mut_ptr();
    std::mem::forget(c_strs);
    ptr
}

/// This function creates a new object of type `T` and fills it with zeros.
///
/// This function uses the `std::alloc::alloc_zeroed` function to allocate
/// memory for new object of type `T`
/// The allocated memory is then wrapped in a `Box<T>` and returned.
///
/// # Safety
/// This function is unsafe because it directly interacts with low-level
/// memory allocation and deallocation functions. Misusing this function
/// can lead to memory leaks, undefined behavior, and other memory-related
/// issues. It is the caller's responsibility to ensure that the returned
/// `Box<T>` is used and dropped correctly.
pub fn get_zero_allocated_obj<T>() -> Box<T> {
    use std::alloc::{alloc_zeroed, Layout};

    unsafe {
        let layout = Layout::new::<T>();
        let allocation = alloc_zeroed(layout) as *mut T;

        Box::from_raw(allocation)
    }
}

/// Reads a 32-bit big-endian value from the given pointer and converts it to host order.
/// # Safety
/// This function is unsafe because it calls unsafe function `from_raw_parts`
pub unsafe fn rb32(ptr: *const u8) -> u32 {
    let bytes = slice::from_raw_parts(ptr, 4);
    NetworkEndian::read_u32(bytes)
}
/// Reads a 16-bit big-endian value from the given pointer and converts it to host order.
/// # Safety
/// This function is unsafe because it calls unsafe function `from_raw_parts`
pub unsafe fn rb16(ptr: *const u8) -> u16 {
    let bytes = slice::from_raw_parts(ptr, 2);
    NetworkEndian::read_u16(bytes)
}
/// # Safety
/// This function is unsafe because it calls unsafe function `read_unaligned`
pub unsafe fn rl32(ptr: *const u8) -> u32 {
    ptr::read_unaligned::<u32>(ptr as *const u32)
}
/// # Safety
/// This function is unsafe because it calls unsafe function `read_unaligned`
pub unsafe fn rl16(ptr: *const u8) -> u16 {
    ptr::read_unaligned::<u16>(ptr as *const u16)
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn test_rl32() {
        // Prepare a little-endian 32-bit value.
        let bytes: [u8; 4] = [0x78, 0x56, 0x34, 0x12]; // expected value: 0x12345678 on little-endian systems
        let value = unsafe { rl32(bytes.as_ptr()) };
        // Since our test system is most likely little-endian, the value should be as stored.
        assert_eq!(value, 0x12345678);
    }

    #[test]
    fn test_rb32() {
        // Prepare a big-endian 32-bit value.
        let bytes: [u8; 4] = [0x01, 0x02, 0x03, 0x04]; // big-endian representation for 0x01020304
        let value = unsafe { rb32(bytes.as_ptr()) };
        // After conversion, we expect the same numerical value.
        assert_eq!(value, 0x01020304);
    }

    #[test]
    fn test_rl16() {
        // Prepare a little-endian 16-bit value.
        let bytes: [u8; 2] = [0xCD, 0xAB]; // expected value: 0xABCD on little-endian systems
        let value = unsafe { rl16(bytes.as_ptr()) };
        // Since our test system is most likely little-endian, the value should be as stored.
        assert_eq!(value, 0xABCD);
    }

    #[test]
    fn test_rb16() {
        // Prepare a big-endian 16-bit value.
        let bytes: [u8; 2] = [0x12, 0x34]; // big-endian representation for 0x1234
        let value = unsafe { rb16(bytes.as_ptr()) };
        // After conversion, we expect the same numerical value.
        assert_eq!(value, 0x1234);
    }

    // Additional tests to ensure functionality with varying data
    #[test]
    fn test_rb32_with_different_value() {
        // Another big-endian value test.
        let bytes: [u8; 4] = [0xFF, 0x00, 0xAA, 0x55];
        let value = unsafe { rb32(bytes.as_ptr()) };
        // On conversion, the expected value is 0xFF00AA55.
        assert_eq!(value, 0xFF00AA55);
    }

    #[test]
    fn test_rb16_with_different_value() {
        // Another big-endian value test.
        let bytes: [u8; 2] = [0xFE, 0xDC];
        let value = unsafe { rb16(bytes.as_ptr()) };
        // On conversion, the expected value is 0xFEDC.
        assert_eq!(value, 0xFEDC);
    }
}
