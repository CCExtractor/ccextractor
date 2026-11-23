//! C-FFI exports for character encoding conversions.

use lib_ccxr::util::encoding::{
    cctolower, cctoupper, line21_to_latin1, line21_to_ucs2, line21_to_utf8,
};
use std::ffi::{c_int, c_uchar};

/// Rust equivalent for `get_char_in_latin_1` function in C.
///
/// # Safety
///
/// `buffer` must not be null and must point to a writable byte in memory.
#[no_mangle]
pub unsafe extern "C" fn ccxr_get_char_in_latin_1(buffer: *mut c_uchar, c: c_uchar) {
    if buffer.is_null() {
        return;
    }
    *buffer = line21_to_latin1(c);
}

/// Rust equivalent for `get_char_in_unicode` function in C.
/// Writes 2 bytes in little-endian order (low byte followed by high byte).
///
/// # Safety
///
/// `buffer` must not be null and must point to at least 2 writable bytes in memory.
#[no_mangle]
pub unsafe extern "C" fn ccxr_get_char_in_unicode(buffer: *mut c_uchar, c: c_uchar) {
    if buffer.is_null() {
        return;
    }
    let ucs2 = line21_to_ucs2(c);
    *buffer = (ucs2 & 0xff) as u8;
    *buffer.add(1) = (ucs2 >> 8) as u8;
}

/// Rust equivalent for `get_char_in_utf_8` function in C.
/// Writes UTF-8 bytes to `buffer` and returns the number of bytes used (1, 2, or 3).
///
/// # Safety
///
/// `buffer` must not be null and must point to at least 3 writable bytes in memory.
#[no_mangle]
pub unsafe extern "C" fn ccxr_get_char_in_utf_8(buffer: *mut c_uchar, c: c_uchar) -> c_int {
    if buffer.is_null() {
        return 0;
    }
    let utf8_packed = line21_to_utf8(c);
    if utf8_packed <= 0xff {
        *buffer = utf8_packed as u8;
        1
    } else if utf8_packed <= 0xffff {
        *buffer = (utf8_packed >> 8) as u8;
        *buffer.add(1) = (utf8_packed & 0xff) as u8;
        2
    } else if utf8_packed <= 0xffffff {
        *buffer = (utf8_packed >> 16) as u8;
        *buffer.add(1) = ((utf8_packed >> 8) & 0xff) as u8;
        *buffer.add(2) = (utf8_packed & 0xff) as u8;
        3
    } else {
        *buffer = b'?';
        1
    }
}

/// Rust equivalent for `cctolower` function in C.
#[no_mangle]
pub extern "C" fn ccxr_cctolower(c: c_uchar) -> c_uchar {
    cctolower(c)
}

/// Rust equivalent for `cctoupper` function in C.
#[no_mangle]
pub extern "C" fn ccxr_cctoupper(c: c_uchar) -> c_uchar {
    cctoupper(c)
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::ptr;

    #[test]
    fn test_null_pointers_do_not_crash() {
        unsafe {
            ccxr_get_char_in_latin_1(ptr::null_mut(), b'A');
            ccxr_get_char_in_unicode(ptr::null_mut(), b'A');
            assert_eq!(ccxr_get_char_in_utf_8(ptr::null_mut(), b'A'), 0);
        }
    }

    #[test]
    fn test_ccxr_get_char_in_latin_1() {
        let mut buf = [0u8; 1];
        unsafe {
            ccxr_get_char_in_latin_1(buf.as_mut_ptr(), b'A');
            assert_eq!(buf[0], b'A');

            ccxr_get_char_in_latin_1(buf.as_mut_ptr(), 0x2a); // a acute
            assert_eq!(buf[0], 0xe1);

            ccxr_get_char_in_latin_1(buf.as_mut_ptr(), 0x87); // music note
            assert_eq!(buf[0], 0xb6);
        }
    }

    #[test]
    fn test_ccxr_get_char_in_unicode() {
        let mut buf = [0u8; 2];
        unsafe {
            // ASCII 'A' -> 0x0041 in little-endian: [0x41, 0x00]
            ccxr_get_char_in_unicode(buf.as_mut_ptr(), b'A');
            assert_eq!(buf, [b'A', 0x00]);

            // Solid block (0x7F) -> U+25A0: [0xa0, 0x25]
            ccxr_get_char_in_unicode(buf.as_mut_ptr(), 0x7f);
            assert_eq!(buf, [0xa0, 0x25]);

            // Eighth Note (0x87) -> U+266A: [0x6a, 0x26]
            ccxr_get_char_in_unicode(buf.as_mut_ptr(), 0x87);
            assert_eq!(buf, [0x6a, 0x26]);

            // Trademark (0x84) -> U+2122: [0x22, 0x21]
            ccxr_get_char_in_unicode(buf.as_mut_ptr(), 0x84);
            assert_eq!(buf, [0x22, 0x21]);
        }
    }

    #[test]
    fn test_ccxr_get_char_in_utf_8() {
        let mut buf = [0u8; 4];
        unsafe {
            // 1-byte ASCII
            let n = ccxr_get_char_in_utf_8(buf.as_mut_ptr(), b'X');
            assert_eq!(n, 1);
            assert_eq!(buf[0], b'X');

            // 2-byte: a acute (0x2A) -> 0xC3 0xA1
            let n = ccxr_get_char_in_utf_8(buf.as_mut_ptr(), 0x2a);
            assert_eq!(n, 2);
            assert_eq!(&buf[..2], &[0xc3, 0xa1]);

            // 3-byte: Solid block (0x7F) -> 0xE2 0x96 0xA0
            let n = ccxr_get_char_in_utf_8(buf.as_mut_ptr(), 0x7f);
            assert_eq!(n, 3);
            assert_eq!(&buf[..3], &[0xe2, 0x96, 0xa0]);

            // 3-byte: Music note (0x87) -> 0xE2 0x99 0xAA
            let n = ccxr_get_char_in_utf_8(buf.as_mut_ptr(), 0x87);
            assert_eq!(n, 3);
            assert_eq!(&buf[..3], &[0xe2, 0x99, 0xaa]);
        }
    }

    #[test]
    fn test_ccxr_casing() {
        assert_eq!(ccxr_cctolower(b'A'), b'a');
        assert_eq!(ccxr_cctolower(0x90), 0x2a);
        assert_eq!(ccxr_cctoupper(b'a'), b'A');
        assert_eq!(ccxr_cctoupper(0x2a), 0x90);
    }
}
