use crate::decoder_isdb::structs_xds::{CcSubtitle, CcxEncodingType, SubDataType, SubType};

/// # Safety
///
/// - The caller must ensure that `sub` is a valid, mutable reference to a `CcSubtitle`.
/// - The `sub` parameter must point to a properly initialized `CcSubtitle` structure.
/// - The `info` and `mode` strings must not exceed 4 bytes in length; otherwise, they will be truncated.
/// - The function uses unsafe operations to manipulate pointers, so improper usage may result in undefined behavior.
pub fn add_cc_sub_text(
    sub: &mut CcSubtitle,
    str: &str,
    start_time: i64,
    end_time: i64,
    info: &str,
    mode: &str,
    e_type: CcxEncodingType,
) -> i32 {
    if str.is_empty() {
        return 0;
    }

    // Traverse to the last subtitle node if there are existing subtitles
    let mut current_sub = sub;
    while let Some(next_sub) = unsafe { current_sub.next.as_mut() } {
        current_sub = next_sub;
    }

    // Allocate a new subtitle node if needed
    if current_sub.nb_data > 0 {
        let new_sub = Box::new(CcSubtitle {
            prev: current_sub as *mut CcSubtitle,
            ..Default::default()
        });

        current_sub.next = Box::into_raw(new_sub);

        current_sub = unsafe { &mut *current_sub.next };
    }

    // Populate the subtitle fields
    current_sub.subtype = SubType::Text;
    current_sub.enc_type = e_type;
    current_sub.data = Box::into_raw(Box::new(str.to_string())) as *mut std::ffi::c_void;
    current_sub.datatype = SubDataType::Generic;
    current_sub.nb_data = str.len() as u32;
    current_sub.start_time = start_time;
    current_sub.end_time = end_time;

    if !info.is_empty() {
        current_sub.info[..info.len().min(4)]
            .copy_from_slice(&info.as_bytes()[..info.len().min(4)]);
    }

    if !mode.is_empty() {
        current_sub.mode[..mode.len().min(4)]
            .copy_from_slice(&mode.as_bytes()[..mode.len().min(4)]);
    }

    current_sub.got_output = true;
    current_sub.next = std::ptr::null_mut();

    0
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::decoder_isdb::structs_xds::{CcSubtitle, CcxEncodingType, SubDataType, SubType};

    #[test]
    fn test_add_cc_sub_text_basic() {
        let mut subtitle = CcSubtitle::default();
        let result = add_cc_sub_text(
            &mut subtitle,
            "Hello, world!",
            1000,
            2000,
            "info",
            "mode",
            CcxEncodingType::Utf8,
        );

        assert_eq!(result, 0);
        assert_eq!(subtitle.nb_data, 13); // "Hello, world!" has 13 characters
        assert_eq!(subtitle.start_time, 1000);
        assert_eq!(subtitle.end_time, 2000);
        assert_eq!(subtitle.subtype, SubType::Text);
        assert_eq!(subtitle.enc_type, CcxEncodingType::Utf8);
        assert_eq!(subtitle.datatype, SubDataType::Generic);
        assert!(subtitle.got_output);
    }

    #[test]
    fn test_add_cc_sub_text_empty_string() {
        let mut subtitle = CcSubtitle::default();
        let result = add_cc_sub_text(
            &mut subtitle,
            "",
            1000,
            2000,
            "info",
            "mode",
            CcxEncodingType::Utf8,
        );

        assert_eq!(result, 0);
        assert_eq!(subtitle.nb_data, 0);
        assert_eq!(subtitle.start_time, 0);
        assert_eq!(subtitle.end_time, 0);
        assert!(!subtitle.got_output);
    }

    #[test]
    fn test_add_cc_sub_text_truncated_info_and_mode() {
        let mut subtitle = CcSubtitle::default();
        let result = add_cc_sub_text(
            &mut subtitle,
            "Test",
            1000,
            2000,
            "longinfo",
            "longmode",
            CcxEncodingType::Utf8,
        );

        assert_eq!(result, 0);
        assert_eq!(subtitle.nb_data, 4); // "Test" has 4 characters
        assert_eq!(subtitle.start_time, 1000);
        assert_eq!(subtitle.end_time, 2000);
        assert_eq!(subtitle.info, *b"long"); // Only the first 4 bytes of "longinfo" are used
        assert_eq!(&subtitle.mode[..4], b"long"); // Only the first 4 bytes of "longmode" are used
        assert!(subtitle.got_output);
    }

    #[test]
    fn test_add_cc_sub_text_multiple_nodes() {
        let mut subtitle = CcSubtitle::default();
        add_cc_sub_text(
            &mut subtitle,
            "First",
            1000,
            2000,
            "info",
            "mode",
            CcxEncodingType::Utf8,
        );

        let result = add_cc_sub_text(
            &mut subtitle,
            "Second",
            3000,
            4000,
            "info2",
            "mode2",
            CcxEncodingType::Utf8,
        );

        assert_eq!(result, 0);

        // Verify the first node
        assert_eq!(subtitle.nb_data, 5); // "First" has 5 characters
        assert_eq!(subtitle.start_time, 1000);
        assert_eq!(subtitle.end_time, 2000);
        assert!(subtitle.next.is_null() == false);

        // Verify the second node
        let second_subtitle = unsafe { &*subtitle.next };
        assert_eq!(second_subtitle.nb_data, 6); // "Second" has 6 characters
        assert_eq!(second_subtitle.start_time, 3000);
        assert_eq!(second_subtitle.end_time, 4000);
        assert!(second_subtitle.next.is_null());
    }
}
