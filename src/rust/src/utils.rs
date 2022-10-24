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

/// function to convert Rust literals to C strings to be passed into functions
/// # Safety
/// The pointer returned has to be deallocated using from_raw() at some point
pub unsafe fn string_to_c_char(a: &str) -> *mut ::std::os::raw::c_char {
    let s = ffi::CString::new(a).unwrap();

    s.into_raw()
}
