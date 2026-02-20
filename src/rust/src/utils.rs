//! Some utility functions to deal with values from C bindings
use std::ffi;

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

/// Free a C string that was allocated by Rust's `string_to_c_char` function.
/// # Safety
/// The pointer must have been allocated by `string_to_c_char` (i.e., `CString::into_raw`)
/// or be null. Passing a pointer allocated by C's malloc will cause undefined behavior.
#[no_mangle]
pub unsafe extern "C" fn free_rust_c_string(ptr: *mut ::std::os::raw::c_char) {
    if !ptr.is_null() {
        // Reclaim ownership and drop the CString, which frees the memory
        let _ = ffi::CString::from_raw(ptr);
    }
}

/// Replace a C string pointer with a new one, freeing the old string if it was
/// allocated by Rust. Returns the new pointer.
/// # Safety
/// The old pointer must have been allocated by `string_to_c_char` or be null.
pub unsafe fn replace_rust_c_string(
    old: *mut ::std::os::raw::c_char,
    new_str: &str,
) -> *mut ::std::os::raw::c_char {
    // Free the old string if it exists
    free_rust_c_string(old);
    // Allocate and return the new string
    string_to_c_char(new_str)
}

/// # Safety
/// The pointer returned has to be deallocated using from_raw() at some point
pub fn null_pointer<T>() -> *mut T {
    std::ptr::null_mut()
}

use std::os::raw::c_char;

pub fn string_to_c_chars(strs: Vec<String>) -> *mut *mut c_char {
    let mut c_strs: Vec<*mut c_char> = Vec::with_capacity(strs.len());
    for s in strs {
        c_strs.push(string_to_c_char(&s));
    }
    c_strs.shrink_to_fit();
    let ptr = c_strs.as_mut_ptr();
    std::mem::forget(c_strs);
    ptr
}

/// Free an array of C strings that was allocated by `string_to_c_chars`.
/// # Safety
/// The pointers must have been allocated by `string_to_c_chars` or be null.
/// `count` must be the number of strings in the array.
#[no_mangle]
pub unsafe extern "C" fn free_rust_c_string_array(arr: *mut *mut c_char, count: usize) {
    if arr.is_null() {
        return;
    }
    // Free each string in the array
    for i in 0..count {
        let str_ptr = *arr.add(i);
        free_rust_c_string(str_ptr);
    }
    // Free the array itself by reconstructing the Vec and dropping it
    // Note: We assume the Vec was created with exactly `count` elements
    if count > 0 {
        let _ = Vec::from_raw_parts(arr, count, count);
    }
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
