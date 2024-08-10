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
    if a.is_empty() {
        return string_null();
    }
    let s = ffi::CString::new(a).unwrap();

    s.into_raw()
}

/// # Safety
/// The pointer returned has to be deallocated using from_raw() at some point
pub fn string_null() -> *mut c_char {
    std::ptr::null_mut()
}

use std::ffi::CString;
use std::os::raw::c_char;

pub fn string_to_c_chars(strs: Vec<String>) -> *mut *mut c_char {
    let mut cstr_vec: Vec<CString> = vec![];
    for s in strs {
        let cstr = CString::new(s.as_str()).unwrap();
        cstr_vec.push(cstr);
    }
    cstr_vec.shrink_to_fit();

    let mut c_char_vec: Vec<*const c_char> = vec![];
    for s in &cstr_vec {
        if s.as_bytes().is_empty() {
            c_char_vec.push(string_null());
            continue;
        }
        c_char_vec.push(s.as_ptr());
    }
    let ptr = c_char_vec.as_ptr();

    std::mem::forget(cstr_vec);
    std::mem::forget(c_char_vec);

    ptr as *mut *mut c_char
}
