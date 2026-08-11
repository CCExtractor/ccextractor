use std::os::raw::c_char;

const TEXT_FORMAT: &[u8] = b"%s\0";

pub(super) fn mprint_arguments(message: *const c_char) -> (*const c_char, *const c_char) {
    (TEXT_FORMAT.as_ptr() as *const c_char, message)
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::ffi::{CStr, CString};

    #[test]
    fn text_uses_a_literal_format() {
        let message = CString::new("Opening '%s%n.mp4' with FFmpeg").unwrap();
        let (format, argument) = mprint_arguments(message.as_ptr());

        assert_eq!(unsafe { CStr::from_ptr(format) }.to_bytes(), b"%s");
        assert_eq!(
            unsafe { CStr::from_ptr(argument) }.to_bytes(),
            message.as_bytes()
        );
    }
}
