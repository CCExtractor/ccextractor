//! Some utility functions to deal with values from C bindings
use crate::bindings::mprint;
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
pub unsafe fn string_to_c_char(a: &str) -> *mut ::std::os::raw::c_char {
    let s = ffi::CString::new(a).unwrap();

    s.into_raw()
}

/// function to call mprint without memory leak
pub unsafe fn mprint_mem_safe(a: &str) {
    let mprint_str = string_to_c_char(a);

    mprint(mprint_str);

    // to deallocate memory properly
    let _S = ffi::CString::from_raw(mprint_str);
}
