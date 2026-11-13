use std::ffi::CStr;
use std::os::raw::{c_char, c_int, c_uint};

use lib_ccxr::common::OutputFormat;

/// Rust equivalent for `hex_to_int` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `high` and `low` must be valid ASCII characters. Invalid characters will
/// return -1 rather than causing undefined behavior.
#[no_mangle]
pub unsafe extern "C" fn ccxr_hex_to_int(high: c_char, low: c_char) -> c_int {
    lib_ccxr::util::hex::hex_to_int(high as u8 as char, low as u8 as char).unwrap_or(-1)
}

/// Rust equivalent for `hex_string_to_int` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `string` must be a valid null-terminated C string. If null, returns -1.
/// Invalid hex characters will return -1 rather than causing undefined behavior.
#[no_mangle]
pub unsafe extern "C" fn ccxr_hex_string_to_int(string: *const c_char, len: c_int) -> c_int {
    if string.is_null() {
        return -1;
    }
    let s = std::ffi::CStr::from_ptr(string).to_str().unwrap_or("");
    lib_ccxr::util::hex::hex_string_to_int(s, len as usize).unwrap_or(-1)
}

/// Rust equivalent for `get_file_extension` function in C. Uses C-native types as input and output.
///
/// Returns a pointer to a static null-terminated C string, or null if the format
/// has no extension or is unknown. The caller must NOT free the returned pointer.
#[no_mangle]
pub extern "C" fn ccxr_get_file_extension(write_format: c_uint) -> *const c_char {
    let format = match OutputFormat::from_repr(write_format) {
        Some(f) => f,
        None => return std::ptr::null(),
    };

    let ext: Option<&'static [u8]> = match format {
        OutputFormat::Ccd => Some(b".ccd\0"),
        OutputFormat::Raw => Some(b".raw\0"),
        OutputFormat::Scc => Some(b".scc\0"),
        OutputFormat::Srt => Some(b".srt\0"),
        OutputFormat::Ssa => Some(b".ass\0"),
        OutputFormat::WebVtt => Some(b".vtt\0"),
        OutputFormat::Sami => Some(b".smi\0"),
        OutputFormat::SmpteTt => Some(b".ttml\0"),
        OutputFormat::Transcript => Some(b".txt\0"),
        OutputFormat::Rcwt => Some(b".bin\0"),
        OutputFormat::SpuPng | OutputFormat::SimpleXml => Some(b".xml\0"),
        OutputFormat::DvdRaw => Some(b".dvdraw\0"),
        OutputFormat::G608 => Some(b".g608\0"),
        OutputFormat::Mcc => Some(b".mcc\0"),
        OutputFormat::Null | OutputFormat::Curl => None,
    };

    match ext {
        Some(bytes) => bytes.as_ptr() as *const c_char,
        None => std::ptr::null(),
    }
}

/// Rust equivalent for `get_basename` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `filename` must be null or point to a valid null-terminated C string.
/// The returned pointer is allocated and must be freed by the caller using `free()`.
#[no_mangle]
pub unsafe extern "C" fn ccxr_get_basename(filename: *const c_char) -> *mut c_char {
    if filename.is_null() {
        return std::ptr::null_mut();
    }
    let s = match CStr::from_ptr(filename).to_str() {
        Ok(s) => s,
        Err(_) => return std::ptr::null_mut(),
    };
    let base = lib_ccxr::util::get_basename(s);
    if base.is_empty() {
        match std::ffi::CString::new("") {
            Ok(cs) => cs.into_raw(),
            Err(_) => std::ptr::null_mut(),
        }
    } else {
        crate::utils::string_to_c_char(&base)
    }
}

/// Rust equivalent for `create_outfilename` function in C. Uses C-native types as input and output.
///
/// # Safety
///
/// `basename`, `suffix`, and `extension` may each be null or point to a valid
/// null-terminated C string.
/// The returned pointer is allocated and must be freed by the caller using `free()`.
#[no_mangle]
pub unsafe extern "C" fn ccxr_create_outfilename(
    basename: *const c_char,
    suffix: *const c_char,
    extension: *const c_char,
) -> *mut c_char {
    let b = if basename.is_null() {
        None
    } else {
        match CStr::from_ptr(basename).to_str() {
            Ok(s) => Some(s),
            Err(_) => return std::ptr::null_mut(),
        }
    };
    let s = if suffix.is_null() {
        None
    } else {
        match CStr::from_ptr(suffix).to_str() {
            Ok(s) => Some(s),
            Err(_) => return std::ptr::null_mut(),
        }
    };
    let e = if extension.is_null() {
        None
    } else {
        match CStr::from_ptr(extension).to_str() {
            Ok(s) => Some(s),
            Err(_) => return std::ptr::null_mut(),
        }
    };

    let res = lib_ccxr::util::create_outfilename(b, s, e);
    if res.is_empty() {
        match std::ffi::CString::new("") {
            Ok(cs) => cs.into_raw(),
            Err(_) => std::ptr::null_mut(),
        }
    } else {
        crate::utils::string_to_c_char(&res)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::ffi::CString;

    #[test]
    fn test_ccxr_get_file_extension() {
        // Raw (0) -> ".raw"
        let ext_raw = ccxr_get_file_extension(0);
        assert!(!ext_raw.is_null());
        unsafe {
            assert_eq!(CStr::from_ptr(ext_raw).to_str().unwrap(), ".raw");
        }

        // Srt (1) -> ".srt"
        let ext_srt = ccxr_get_file_extension(1);
        assert!(!ext_srt.is_null());
        unsafe {
            assert_eq!(CStr::from_ptr(ext_srt).to_str().unwrap(), ".srt");
        }

        // Null (5) -> null pointer
        assert!(ccxr_get_file_extension(5).is_null());

        // Curl (12) -> null pointer
        assert!(ccxr_get_file_extension(12).is_null());

        // Invalid discriminant -> null pointer
        assert!(ccxr_get_file_extension(999).is_null());
    }

    #[test]
    fn test_ccxr_get_basename() {
        unsafe {
            // Null input
            assert!(ccxr_get_basename(std::ptr::null()).is_null());

            // Standard filename
            let input = CString::new("/dir/sub/sample.ts.vtt").unwrap();
            let ptr = ccxr_get_basename(input.as_ptr());
            assert!(!ptr.is_null());
            assert_eq!(CStr::from_ptr(ptr).to_str().unwrap(), "/dir/sub/sample.ts");
            drop(CString::from_raw(ptr));

            // Empty filename
            let empty_input = CString::new("").unwrap();
            let empty_ptr = ccxr_get_basename(empty_input.as_ptr());
            assert!(!empty_ptr.is_null());
            assert_eq!(CStr::from_ptr(empty_ptr).to_str().unwrap(), "");
            drop(CString::from_raw(empty_ptr));
        }
    }

    #[test]
    fn test_ccxr_create_outfilename() {
        unsafe {
            // All null inputs -> empty allocated string
            let null = std::ptr::null();
            let empty_ptr = ccxr_create_outfilename(null, null, null);
            assert!(!empty_ptr.is_null());
            assert_eq!(CStr::from_ptr(empty_ptr).to_str().unwrap(), "");
            drop(CString::from_raw(empty_ptr));

            // Valid combination
            let b = CString::new("video").unwrap();
            let s = CString::new("_track1").unwrap();
            let e = CString::new(".srt").unwrap();
            let ptr = ccxr_create_outfilename(b.as_ptr(), s.as_ptr(), e.as_ptr());
            assert!(!ptr.is_null());
            assert_eq!(CStr::from_ptr(ptr).to_str().unwrap(), "video_track1.srt");
            drop(CString::from_raw(ptr));

            // Partial combination (no suffix)
            let ptr2 = ccxr_create_outfilename(b.as_ptr(), std::ptr::null(), e.as_ptr());
            assert!(!ptr2.is_null());
            assert_eq!(CStr::from_ptr(ptr2).to_str().unwrap(), "video.srt");
            drop(CString::from_raw(ptr2));
        }
    }
}
