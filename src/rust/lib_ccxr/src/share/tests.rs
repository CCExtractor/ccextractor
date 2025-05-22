#![allow(unused_imports)]
#![allow(unused)]

#[cfg(feature = "enable_sharing")]
mod test {
    use crate::share::ccxr_sub_entry_message::*;
    use crate::share::functions::sharing::*;
    use crate::util::log::{
        set_logger, CCExtractorLogger, DebugMessageFlag, DebugMessageMask, OutputTarget,
    };
    use std::os::raw::c_char;
    use std::sync::Once;

    static INIT: Once = Once::new();

    fn initialize_logger() {
        INIT.call_once(|| {
            set_logger(CCExtractorLogger::new(
                OutputTarget::Stdout,
                DebugMessageMask::new(DebugMessageFlag::VERBOSE, DebugMessageFlag::VERBOSE),
                false,
            ))
            .ok();
        });
    }
    #[test]
    fn test_ffi_sub_entry_msg_cleanup() {
        let mut msg = CcxSubEntryMessage {
            eos: 0,
            stream_name: "test".to_string(),
            counter: 0,
            start_time: 0,
            end_time: 0,
            lines: vec!["test".to_string()],
        };

        unsafe {
            ccxr_sub_entry_msg_cleanup(&mut *(&mut msg as *mut CcxSubEntryMessage));
        }

        assert!(msg.lines.is_empty());
        assert_eq!(msg.stream_name, "");
    }

    #[test]
    fn test_ffi_sub_entry_msg_print() {
        let msg = CcxSubEntryMessage {
            eos: 0,
            stream_name: "test".to_string(),
            counter: 0,
            start_time: 0,
            end_time: 0,
            lines: vec!["test".to_string(), "test".to_string(), "test".to_string()],
        };

        unsafe {
            ccxr_sub_entry_msg_print(&*(&msg as *const CcxSubEntryMessage));
        }
    }
    #[test]
    fn test_ffi_sub_entries_init() {
        let mut entries = CcxSubEntries {
            messages: vec![CcxSubEntryMessage {
                eos: 0,
                counter: 1,
                stream_name: "Test".parse().unwrap(),
                start_time: 0,
                end_time: 0,
                lines: vec![],
            }],
        };

        unsafe {
            ccxr_sub_entries_cleanup(&mut *(&mut entries as *mut CcxSubEntries));
        }

        assert!(entries.messages.is_empty());
    }

    #[test]
    fn test_ffi_sub_entries_cleanup() {
        let mut entries = CcxSubEntries {
            messages: vec![CcxSubEntryMessage {
                eos: 0,
                counter: 1,
                stream_name: "Test".parse().unwrap(),
                start_time: 0,
                end_time: 0,
                lines: vec![],
            }],
        };

        unsafe {
            ccxr_sub_entries_cleanup(&mut *(&mut entries as *mut CcxSubEntries));
        }

        assert!(entries.messages.is_empty());
    }

    #[test]
    fn test_ffi_sub_entries_print() {
        let entries = CcxSubEntries {
            messages: vec![CcxSubEntryMessage {
                eos: 0,
                counter: 1,
                stream_name: "Test".parse().unwrap(),
                start_time: 0,
                end_time: 0,
                lines: vec![],
            }],
        };

        unsafe {
            ccxr_sub_entries_print(&*(&entries as *const CcxSubEntries));
        }
    }

    #[test]
    fn test_ccxr_share_send() {
        initialize_logger();

        let mut sub = CcSubtitle {
            data: std::ptr::null_mut(),
            datatype: 1,
            nb_data: 0,
            type_: 1,
            enc_type: 1,
            start_time: 0,
            end_time: 0,
            flags: 0,
            lang_index: 0,
            got_output: 0,
            mode: [0; 5],
            info: [0; 4],
            time_out: 0,
            next: std::ptr::null_mut(),
            prev: std::ptr::null_mut(),
        };
        let status = unsafe { ccxr_share_send(&mut sub as *mut CcSubtitle) };
        assert!(matches!(status, CcxShareStatus::Ok | CcxShareStatus::Fail));
    }

    #[test]
    fn test_ccxr_share_send_c() {
        initialize_logger();
        let msg = CcxSubEntryMessage {
            eos: 0,
            stream_name: "test_stream".to_string(),
            counter: 1,
            start_time: 0,
            end_time: 0,
            lines: vec!["Line 1".to_string(), "Line 2".to_string()],
        };

        let status = unsafe { _ccxr_share_send(&*(&msg as *const CcxSubEntryMessage)) };
        assert!(matches!(status, CcxShareStatus::Ok | CcxShareStatus::Fail));
    }

    const CCX_DECODER_608_SCREEN_WIDTH: usize = 32;
    const CCX_DECODER_608_SCREEN_ROWS: usize = 15;

    #[test]
    fn test_ccxr_share_sub_to_entries() {
        initialize_logger();
        const NUM_ROWS: usize = CCX_DECODER_608_SCREEN_ROWS;
        let mut screen = Eia608Screen::new();
        screen.row_used[0] = 1;
        screen.row_used[2] = 1;
        screen.row_used[4] = 1;

        let row_0_content = b"Hello, World!";
        let row_2_content = b"Subtitle line 2";
        let row_4_content = b"Subtitle line 3";

        for (i, &ch) in row_0_content.iter().enumerate() {
            screen.characters[0][i] = ch as c_char;
        }
        for (i, &ch) in row_2_content.iter().enumerate() {
            screen.characters[2][i] = ch as c_char;
        }
        for (i, &ch) in row_4_content.iter().enumerate() {
            screen.characters[4][i] = ch as c_char;
        }

        screen.start_time = 1000;
        screen.end_time = 2000;
        screen.mode = CcModes::ModePaintOn;
        screen.channel = 1;
        screen.my_field = 42;

        let sub = CcSubtitle {
            data: &screen as *const _ as *mut std::os::raw::c_void,
            datatype: 1,
            nb_data: 1,
            type_: SUBTYPE_CC_608,
            enc_type: 1,
            start_time: 0,
            end_time: 0,
            flags: 0,
            lang_index: 0,
            got_output: 1,
            mode: [
                b'M' as c_char,
                b'O' as c_char,
                b'D' as c_char,
                b'E' as c_char,
                0,
            ],
            info: [b'I' as c_char, b'N' as c_char, b'F' as c_char, 0],
            time_out: 0,
            next: std::ptr::null_mut(),
            prev: std::ptr::null_mut(),
        };
        let mut entries = CcxSubEntries {
            messages: Vec::new(),
        };
        let status = ccxr_share_sub_to_entries(&sub, &mut entries);
        assert_eq!(
            status,
            CcxShareStatus::Ok,
            "Function should return OK status"
        );
        assert_eq!(
            entries.messages.len(),
            1,
            "There should be one entry in messages"
        );

        let message = &entries.messages[0];
        assert_eq!(message.start_time, 1000, "Start time should match input");
        assert_eq!(message.end_time, 2000, "End time should match input");
        assert_eq!(message.lines.len(), 3, "There should be 3 lines of content");

        assert_eq!(
            message.lines[0], "Hello, World!",
            "First line content mismatch"
        );
        assert_eq!(
            message.lines[1], "Subtitle line 2",
            "Second line content mismatch"
        );
        assert_eq!(
            message.lines[2], "Subtitle line 3",
            "Third line content mismatch"
        );
    }

    #[test]
    fn test_ccxr_share_sub_to_entries_empty_rows() {
        let mut screen = Eia608Screen::new();

        screen.start_time = 1000;
        screen.end_time = 2000;

        let sub = CcSubtitle {
            data: &screen as *const _ as *mut std::os::raw::c_void,
            datatype: 1,
            nb_data: 1,
            type_: SUBTYPE_CC_608,
            enc_type: 1,
            start_time: 0,
            end_time: 0,
            flags: 0,
            lang_index: 0,
            got_output: 1,
            mode: [
                b'M' as c_char,
                b'O' as c_char,
                b'D' as c_char,
                b'E' as c_char,
                0,
            ],
            info: [b'I' as c_char, b'N' as c_char, b'F' as c_char, 0],
            time_out: 0,
            next: std::ptr::null_mut(),
            prev: std::ptr::null_mut(),
        };

        let mut entries = CcxSubEntries {
            messages: Vec::new(),
        };

        let status = ccxr_share_sub_to_entries(&sub, &mut entries);

        assert_eq!(
            status,
            CcxShareStatus::Ok,
            "Function should return OK status"
        );
        assert_eq!(
            entries.messages.len(),
            0,
            "There should be no messages for empty rows"
        );
    }
}
