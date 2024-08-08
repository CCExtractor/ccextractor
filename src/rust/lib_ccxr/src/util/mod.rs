//! Provides basic utilities used throughout the program.

pub mod bits;
pub mod encoding;
pub mod levenshtein;
pub mod log;
pub mod time;

use std::os::raw::c_char;

/// Helper function that converts a Rust-String (`string`) to C-String (`buffer`).
///
/// # Safety
///
/// `buffer` must have enough allocated space for `string` to fit.
pub fn write_string_into_pointer(buffer: *mut c_char, string: &str) {
    let buffer = unsafe { std::slice::from_raw_parts_mut(buffer as *mut u8, string.len() + 1) };
    buffer[..string.len()].copy_from_slice(string.as_bytes());
    buffer[string.len()] = b'\0';
}
