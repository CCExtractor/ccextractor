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
//! | `get_basename`                             | [`get_basename`]               |
//! | `create_outfilename`                       | [`create_outfilename`]         |

pub mod bits;
pub mod encoders_helper;
pub mod encoding;
pub mod hex;
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

/// Returns the filename without its final extension, preserving any directory path.
///
/// If there is no extension after the last directory separator, the original string is returned.
///
/// Rust equivalent for `get_basename` function in C.
pub fn get_basename(filename: &str) -> String {
    let search_start = filename.rfind(['/', '\\']).map_or(0, |idx| idx + 1);
    if let Some(dot_offset) = filename[search_start..].rfind('.') {
        filename[..search_start + dot_offset].to_string()
    } else {
        filename.to_string()
    }
}

/// Creates an output filename by concatenating `basename`, `suffix`, and `extension`.
///
/// Returns an empty string if all three components are `None` or empty.
///
/// Rust equivalent for `create_outfilename` function in C.
pub fn create_outfilename(
    basename: Option<&str>,
    suffix: Option<&str>,
    extension: Option<&str>,
) -> String {
    let b = basename.unwrap_or_default();
    let s = suffix.unwrap_or_default();
    let e = extension.unwrap_or_default();

    let mut out = String::with_capacity(b.len() + s.len() + e.len());
    out.push_str(b);
    out.push_str(s);
    out.push_str(e);
    out
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

    #[test]
    fn test_get_basename() {
        // Standard full paths preserving parent directory
        assert_eq!(get_basename("/home/user/sample.ts"), "/home/user/sample");
        assert_eq!(get_basename("/dir/sub/video.mp4"), "/dir/sub/video");
        assert_eq!(
            get_basename("C:\\Users\\user\\video.mp4"),
            "C:\\Users\\user\\video"
        );

        // Relative paths
        assert_eq!(get_basename("./sample.ts"), "./sample");
        assert_eq!(
            get_basename("../folder.v1/sample.ts"),
            "../folder.v1/sample"
        );
        assert_eq!(get_basename("../dir.name/video"), "../dir.name/video");
        assert_eq!(get_basename("./video"), "./video");

        // Compound extensions (only the last extension is stripped)
        assert_eq!(get_basename("sample.ts.vtt"), "sample.ts");
        assert_eq!(get_basename("archive.part1.mp4"), "archive.part1");

        // Single filename with or without extension
        assert_eq!(get_basename("video.mp4"), "video");
        assert_eq!(get_basename("sample"), "sample");

        // Hidden files and edge cases
        assert_eq!(get_basename(".hidden"), "");
        assert_eq!(get_basename(""), "");
        assert_eq!(get_basename("/path/to/.hidden"), "/path/to/");
    }

    #[test]
    fn test_create_outfilename() {
        // All parts present
        assert_eq!(
            create_outfilename(Some("video"), Some("_en"), Some(".srt")),
            "video_en.srt"
        );

        // Missing suffix
        assert_eq!(
            create_outfilename(Some("video"), None, Some(".srt")),
            "video.srt"
        );

        // Missing extension
        assert_eq!(
            create_outfilename(Some("video"), Some("_track1"), None),
            "video_track1"
        );

        // Missing basename
        assert_eq!(
            create_outfilename(None, Some("out"), Some(".txt")),
            "out.txt"
        );
        assert_eq!(create_outfilename(None, None, Some(".srt")), ".srt");

        // Empty combinations
        assert_eq!(create_outfilename(None, None, None), "");
        assert_eq!(create_outfilename(Some(""), Some(""), Some("")), "");
        assert_eq!(create_outfilename(Some(""), None, Some("")), "");
    }
}
