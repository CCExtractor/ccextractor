//! Provides basic utilities used throughout the program.
//!
//! # Conversion Guide
//!
//! | From                                       | To                             |
//! |--------------------------------------------|--------------------------------|
//! | `PARITY_8`                                 | [`parity`]                     |
//! | `REVERSE_8`                                | [`reverse`]                    |
//! | `UNHAM_8_4`                                | [`decode_hamming_8_4`]         |
//! | `unham_24_18`                              | [`decode_hamming_24_18`]       |
//! | `levenshtein_dist`, levenshtein_dist_char` | [`levenshtein`](levenshtein()) |

pub mod bits;
pub mod encoders_helper;
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

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_write_string_into_pointer() {
        let test_string = "CCExtractor is the best";
        let mut buffer = vec![0u8; test_string.len() + 1];
        let buffer_ptr = buffer.as_mut_ptr() as *mut c_char;

        write_string_into_pointer(buffer_ptr, test_string);

        assert_eq!(&buffer[..test_string.len()], test_string.as_bytes());
        assert_eq!(buffer[test_string.len()], 0); // Check null terminator
    }
}
