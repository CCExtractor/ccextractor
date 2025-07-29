use crate::bindings::{EPG_event, EPG_rating};
use crate::common::CType;
use crate::ctorust::FromCType;
use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_longlong};
use std::ptr;

// Rustified version of the EPG_event struct
#[derive(Debug, Clone, PartialEq, Default)]
pub struct EPGEventRust {
    pub id: u32,
    pub start_time_string: String,
    pub end_time_string: String,
    pub running_status: u8,
    pub free_ca_mode: u8,
    pub iso_639_language_code: String,
    pub event_name: Option<String>,
    pub text: Option<String>,
    pub extended_iso_639_language_code: String,
    pub extended_text: Option<String>,
    pub has_simple: bool,
    pub ratings: Vec<EPGRatingRust>,
    pub categories: Vec<u8>,
    pub service_id: u16,
    pub count: i64,
    pub live_output: bool,
}

impl EPGEventRust {
    pub(crate) fn default() -> EPGEventRust {
        EPGEventRust {
            id: 0,
            start_time_string: String::new(),
            end_time_string: String::new(),
            running_status: 0,
            free_ca_mode: 0,
            iso_639_language_code: String::new(),
            event_name: None,
            text: None,
            extended_iso_639_language_code: String::new(),
            extended_text: None,
            has_simple: false,
            ratings: Vec::new(),
            categories: Vec::new(),
            service_id: 0,
            count: 0,
            live_output: false,
        }
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct EPGRatingRust {
    pub country_code: String,
    pub age: u8,
}

// Helper function to convert C char array to String
unsafe fn c_char_array_to_string<const N: usize>(arr: &[c_char; N]) -> String {
    let cstr = CStr::from_ptr(arr.as_ptr());
    cstr.to_string_lossy().into_owned()
}

// Helper function to convert String to C char array
fn string_to_c_char_array<const N: usize>(s: &str) -> [c_char; N] {
    let mut arr = [0; N];
    let bytes = s.as_bytes();
    let len = std::cmp::min(bytes.len(), N - 1); // Leave space for null terminator

    for i in 0..len {
        arr[i] = bytes[i] as c_char;
    }

    // arr[len] is already 0 (null terminator)
    arr
}

// Helper function to convert C string pointer to Option<String>
unsafe fn c_string_ptr_to_option_string(ptr: *mut c_char) -> Option<String> {
    if ptr.is_null() {
        None
    } else {
        let cstr = CStr::from_ptr(ptr);
        Some(cstr.to_string_lossy().into_owned())
    }
}

// Helper function to convert Option<String> to C string pointer
fn option_string_to_c_string_ptr(opt_str: &Option<String>) -> *mut c_char {
    match opt_str {
        Some(s) => {
            if let Ok(cstring) = CString::new(s.as_str()) {
                cstring.into_raw()
            } else {
                ptr::null_mut()
            }
        }
        None => ptr::null_mut(),
    }
}

impl FromCType<EPG_rating> for EPGRatingRust {
    unsafe fn from_ctype(rating: EPG_rating) -> Option<Self> {
        Some(EPGRatingRust {
            country_code: c_char_array_to_string(&rating.country_code),
            age: rating.age,
        })
    }
}

impl CType<EPG_rating> for EPGRatingRust {
    unsafe fn to_ctype(&self) -> EPG_rating {
        EPG_rating {
            country_code: string_to_c_char_array(&self.country_code),
            age: self.age,
        }
    }
}

impl FromCType<EPG_event> for EPGEventRust {
    unsafe fn from_ctype(event: EPG_event) -> Option<Self> {
        // Convert ratings array
        let ratings = if event.ratings.is_null() || event.num_ratings == 0 {
            Vec::new()
        } else {
            let ratings_slice =
                std::slice::from_raw_parts(event.ratings, event.num_ratings as usize);
            ratings_slice
                .iter()
                .filter_map(|r| EPGRatingRust::from_ctype(*r))
                .collect()
        };

        // Convert categories array
        let categories = if event.categories.is_null() || event.num_categories == 0 {
            Vec::new()
        } else {
            let categories_slice =
                std::slice::from_raw_parts(event.categories, event.num_categories as usize);
            categories_slice.to_vec()
        };

        Some(EPGEventRust {
            id: event.id,
            start_time_string: c_char_array_to_string(&event.start_time_string),
            end_time_string: c_char_array_to_string(&event.end_time_string),
            running_status: event.running_status,
            free_ca_mode: event.free_ca_mode,
            iso_639_language_code: c_char_array_to_string(&event.ISO_639_language_code),
            event_name: c_string_ptr_to_option_string(event.event_name),
            text: c_string_ptr_to_option_string(event.text),
            extended_iso_639_language_code: c_char_array_to_string(
                &event.extended_ISO_639_language_code,
            ),
            extended_text: c_string_ptr_to_option_string(event.extended_text),
            has_simple: event.has_simple != 0,
            ratings,
            categories,
            service_id: event.service_id,
            count: event.count,
            live_output: event.live_output != 0,
        })
    }
}

impl CType<EPG_event> for EPGEventRust {
    unsafe fn to_ctype(&self) -> EPG_event {
        // Convert ratings to C array
        let (ratings_ptr, num_ratings) = if self.ratings.is_empty() {
            (ptr::null_mut(), 0)
        } else {
            let ratings_vec: Vec<EPG_rating> = self.ratings.iter().map(|r| r.to_ctype()).collect();
            let ratings_boxed = ratings_vec.into_boxed_slice();
            let ptr = Box::into_raw(ratings_boxed) as *mut EPG_rating;
            (ptr, self.ratings.len() as u32)
        };

        // Convert categories to C array
        let (categories_ptr, num_categories) = if self.categories.is_empty() {
            (ptr::null_mut(), 0)
        } else {
            let categories_boxed = self.categories.clone().into_boxed_slice();
            let ptr = Box::into_raw(categories_boxed) as *mut u8;
            (ptr, self.categories.len() as u32)
        };

        EPG_event {
            id: self.id,
            start_time_string: string_to_c_char_array(&self.start_time_string),
            end_time_string: string_to_c_char_array(&self.end_time_string),
            running_status: self.running_status,
            free_ca_mode: self.free_ca_mode,
            ISO_639_language_code: string_to_c_char_array(&self.iso_639_language_code),
            event_name: option_string_to_c_string_ptr(&self.event_name),
            text: option_string_to_c_string_ptr(&self.text),
            extended_ISO_639_language_code: string_to_c_char_array(
                &self.extended_iso_639_language_code,
            ),
            extended_text: option_string_to_c_string_ptr(&self.extended_text),
            has_simple: if self.has_simple { 1 } else { 0 },
            ratings: ratings_ptr,
            num_ratings,
            categories: categories_ptr,
            num_categories,
            service_id: self.service_id,
            count: self.count as c_longlong,
            live_output: if self.live_output { 1 } else { 0 },
        }
    }
}

// Cleanup function to properly free allocated memory
impl EPGEventRust {
    /// # Safety
    /// This function must be called with a valid `EPG_event` pointer that was created by the `to_ctype` method.
    pub unsafe fn cleanup_c_event(event: EPG_event) {
        // Free string pointers
        if !event.event_name.is_null() {
            let _ = CString::from_raw(event.event_name);
        }
        if !event.text.is_null() {
            let _ = CString::from_raw(event.text);
        }
        if !event.extended_text.is_null() {
            let _ = CString::from_raw(event.extended_text);
        }

        if !event.ratings.is_null() && event.num_ratings > 0 {
            let ratings_slice = Box::from_raw(std::slice::from_raw_parts_mut(
                event.ratings,
                event.num_ratings as usize,
            ));
            drop(ratings_slice);
        }

        if !event.categories.is_null() && event.num_categories > 0 {
            let categories_slice = Box::from_raw(std::slice::from_raw_parts_mut(
                event.categories,
                event.num_categories as usize,
            ));
            drop(categories_slice);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::ffi::CString;

    fn create_test_rust_event() -> EPGEventRust {
        EPGEventRust {
            id: 12345,
            start_time_string: "20231201120000 +0000".to_string(),
            end_time_string: "20231201130000 +0000".to_string(),
            running_status: 4,
            free_ca_mode: 0,
            iso_639_language_code: "eng".to_string(),
            event_name: Some("Test Event Name".to_string()),
            text: Some("Test event description".to_string()),
            extended_iso_639_language_code: "eng".to_string(),
            extended_text: Some("Extended description".to_string()),
            has_simple: true,
            ratings: vec![
                EPGRatingRust {
                    country_code: "US".to_string(),
                    age: 18,
                },
                EPGRatingRust {
                    country_code: "GB".to_string(),
                    age: 12,
                },
            ],
            categories: vec![1, 2, 3, 4],
            service_id: 256,
            count: 42,
            live_output: false,
        }
    }

    unsafe fn create_test_c_event() -> EPG_event {
        let event_name = CString::new("Test Event Name").unwrap().into_raw();
        let text = CString::new("Test event description").unwrap().into_raw();
        let extended_text = CString::new("Extended description").unwrap().into_raw();

        let ratings = vec![
            EPG_rating {
                country_code: string_to_c_char_array("US"),
                age: 18,
            },
            EPG_rating {
                country_code: string_to_c_char_array("GB"),
                age: 12,
            },
        ];
        let ratings_boxed = ratings.into_boxed_slice();
        let ratings_ptr = Box::into_raw(ratings_boxed) as *mut EPG_rating;

        let categories = vec![1u8, 2, 3, 4];
        let categories_boxed = categories.into_boxed_slice();
        let categories_ptr = Box::into_raw(categories_boxed) as *mut u8;

        EPG_event {
            id: 12345,
            start_time_string: string_to_c_char_array("20231201120000 +0000"),
            end_time_string: string_to_c_char_array("20231201130000 +0000"),
            running_status: 4,
            free_ca_mode: 0,
            ISO_639_language_code: string_to_c_char_array("eng"),
            event_name,
            text,
            extended_ISO_639_language_code: string_to_c_char_array("eng"),
            extended_text,
            has_simple: 1,
            ratings: ratings_ptr,
            num_ratings: 2,
            categories: categories_ptr,
            num_categories: 4,
            service_id: 256,
            count: 42,
            live_output: 0,
        }
    }

    #[test]
    fn test_rust_to_c_conversion() {
        let rust_event = create_test_rust_event();

        unsafe {
            let c_event = rust_event.to_ctype();

            // Test basic fields
            assert_eq!(c_event.id, 12345);
            assert_eq!(c_event.running_status, 4);
            assert_eq!(c_event.free_ca_mode, 0);
            assert_eq!(c_event.service_id, 256);
            assert_eq!(c_event.count, 42);
            assert_eq!(c_event.has_simple, 1);
            assert_eq!(c_event.live_output, 0);

            // Test string arrays
            let start_time = c_char_array_to_string(&c_event.start_time_string);
            assert_eq!(start_time, "20231201120000 +0000");

            let end_time = c_char_array_to_string(&c_event.end_time_string);
            assert_eq!(end_time, "20231201130000 +0000");

            let lang_code = c_char_array_to_string(&c_event.ISO_639_language_code);
            assert_eq!(lang_code, "eng");

            let ext_lang_code = c_char_array_to_string(&c_event.extended_ISO_639_language_code);
            assert_eq!(ext_lang_code, "eng");

            // Test string pointers
            assert!(!c_event.event_name.is_null());
            let event_name = CStr::from_ptr(c_event.event_name).to_string_lossy();
            assert_eq!(event_name, "Test Event Name");

            assert!(!c_event.text.is_null());
            let text = CStr::from_ptr(c_event.text).to_string_lossy();
            assert_eq!(text, "Test event description");

            assert!(!c_event.extended_text.is_null());
            let extended_text = CStr::from_ptr(c_event.extended_text).to_string_lossy();
            assert_eq!(extended_text, "Extended description");

            // Test ratings array
            assert_eq!(c_event.num_ratings, 2);
            assert!(!c_event.ratings.is_null());
            let ratings_slice = std::slice::from_raw_parts(c_event.ratings, 2);
            assert_eq!(ratings_slice[0].age, 18);
            assert_eq!(c_char_array_to_string(&ratings_slice[0].country_code), "US");
            assert_eq!(ratings_slice[1].age, 12);
            assert_eq!(c_char_array_to_string(&ratings_slice[1].country_code), "GB");

            // Test categories array
            assert_eq!(c_event.num_categories, 4);
            assert!(!c_event.categories.is_null());
            let categories_slice = std::slice::from_raw_parts(c_event.categories, 4);
            assert_eq!(categories_slice, &[1, 2, 3, 4]);

            // Cleanup
            EPGEventRust::cleanup_c_event(c_event);
        }
    }

    #[test]
    fn test_c_to_rust_conversion() {
        unsafe {
            let c_event = create_test_c_event();
            let rust_event = EPGEventRust::from_ctype(c_event).expect("Conversion should succeed");

            // Test basic fields
            assert_eq!(rust_event.id, 12345);
            assert_eq!(rust_event.running_status, 4);
            assert_eq!(rust_event.free_ca_mode, 0);
            assert_eq!(rust_event.service_id, 256);
            assert_eq!(rust_event.count, 42);
            assert!(rust_event.has_simple);
            assert!(!rust_event.live_output);

            // Test strings
            assert_eq!(rust_event.start_time_string, "20231201120000 +0000");
            assert_eq!(rust_event.end_time_string, "20231201130000 +0000");
            assert_eq!(rust_event.iso_639_language_code, "eng");
            assert_eq!(rust_event.extended_iso_639_language_code, "eng");

            // Test optional strings
            assert_eq!(rust_event.event_name, Some("Test Event Name".to_string()));
            assert_eq!(rust_event.text, Some("Test event description".to_string()));
            assert_eq!(
                rust_event.extended_text,
                Some("Extended description".to_string())
            );

            // Test ratings
            assert_eq!(rust_event.ratings.len(), 2);
            assert_eq!(rust_event.ratings[0].age, 18);
            assert_eq!(rust_event.ratings[0].country_code, "US");
            assert_eq!(rust_event.ratings[1].age, 12);
            assert_eq!(rust_event.ratings[1].country_code, "GB");

            // Test categories
            assert_eq!(rust_event.categories, vec![1, 2, 3, 4]);

            // Cleanup
            EPGEventRust::cleanup_c_event(c_event);
        }
    }

    #[test]
    fn test_round_trip_conversion() {
        let original_rust = create_test_rust_event();

        unsafe {
            // Rust -> C -> Rust
            let c_event = original_rust.to_ctype();
            let converted_rust =
                EPGEventRust::from_ctype(c_event).expect("Conversion should succeed");

            // Compare all fields
            assert_eq!(original_rust.id, converted_rust.id);
            assert_eq!(
                original_rust.start_time_string,
                converted_rust.start_time_string
            );
            assert_eq!(
                original_rust.end_time_string,
                converted_rust.end_time_string
            );
            assert_eq!(original_rust.running_status, converted_rust.running_status);
            assert_eq!(original_rust.free_ca_mode, converted_rust.free_ca_mode);
            assert_eq!(
                original_rust.iso_639_language_code,
                converted_rust.iso_639_language_code
            );
            assert_eq!(original_rust.event_name, converted_rust.event_name);
            assert_eq!(original_rust.text, converted_rust.text);
            assert_eq!(
                original_rust.extended_iso_639_language_code,
                converted_rust.extended_iso_639_language_code
            );
            assert_eq!(original_rust.extended_text, converted_rust.extended_text);
            assert_eq!(original_rust.has_simple, converted_rust.has_simple);
            assert_eq!(original_rust.ratings, converted_rust.ratings);
            assert_eq!(original_rust.categories, converted_rust.categories);
            assert_eq!(original_rust.service_id, converted_rust.service_id);
            assert_eq!(original_rust.count, converted_rust.count);
            assert_eq!(original_rust.live_output, converted_rust.live_output);

            // Should be completely equal
            assert_eq!(original_rust, converted_rust);

            // Cleanup
            EPGEventRust::cleanup_c_event(c_event);
        }
    }

    #[test]
    fn test_null_pointers() {
        let rust_event = EPGEventRust {
            id: 1,
            start_time_string: "20231201120000 +0000".to_string(),
            end_time_string: "20231201130000 +0000".to_string(),
            running_status: 0,
            free_ca_mode: 0,
            iso_639_language_code: "eng".to_string(),
            event_name: None,
            text: None,
            extended_iso_639_language_code: "".to_string(),
            extended_text: None,
            has_simple: false,
            ratings: Vec::new(),
            categories: Vec::new(),
            service_id: 0,
            count: 0,
            live_output: false,
        };

        unsafe {
            let c_event = rust_event.to_ctype();

            // Test null pointers
            assert!(c_event.event_name.is_null());
            assert!(c_event.text.is_null());
            assert!(c_event.extended_text.is_null());
            assert!(c_event.ratings.is_null());
            assert!(c_event.categories.is_null());
            assert_eq!(c_event.num_ratings, 0);
            assert_eq!(c_event.num_categories, 0);

            // Test conversion back
            let converted_rust =
                EPGEventRust::from_ctype(c_event).expect("Conversion should succeed");
            assert_eq!(converted_rust.event_name, None);
            assert_eq!(converted_rust.text, None);
            assert_eq!(converted_rust.extended_text, None);
            assert!(converted_rust.ratings.is_empty());
            assert!(converted_rust.categories.is_empty());

            // Cleanup (should be safe with null pointers)
            EPGEventRust::cleanup_c_event(c_event);
        }
    }

    #[test]
    fn test_empty_strings() {
        let rust_event = EPGEventRust {
            id: 1,
            start_time_string: "".to_string(),
            end_time_string: "".to_string(),
            running_status: 0,
            free_ca_mode: 0,
            iso_639_language_code: "".to_string(),
            event_name: Some("".to_string()),
            text: Some("".to_string()),
            extended_iso_639_language_code: "".to_string(),
            extended_text: Some("".to_string()),
            has_simple: false,
            ratings: Vec::new(),
            categories: Vec::new(),
            service_id: 0,
            count: 0,
            live_output: false,
        };

        unsafe {
            let c_event = rust_event.to_ctype();
            let converted_rust =
                EPGEventRust::from_ctype(c_event).expect("Conversion should succeed");

            // Empty strings should be preserved
            assert_eq!(converted_rust.start_time_string, "");
            assert_eq!(converted_rust.end_time_string, "");
            assert_eq!(converted_rust.iso_639_language_code, "");
            assert_eq!(converted_rust.event_name, Some("".to_string()));
            assert_eq!(converted_rust.text, Some("".to_string()));
            assert_eq!(converted_rust.extended_text, Some("".to_string()));

            EPGEventRust::cleanup_c_event(c_event);
        }
    }

    #[test]
    fn test_long_strings() {
        let long_string = "a".repeat(100);
        let rust_event = EPGEventRust {
            id: 1,
            start_time_string: long_string.clone(),
            end_time_string: long_string.clone(),
            running_status: 0,
            free_ca_mode: 0,
            iso_639_language_code: long_string.clone(),
            event_name: Some(long_string.clone()),
            text: Some(long_string.clone()),
            extended_iso_639_language_code: long_string.clone(),
            extended_text: Some(long_string.clone()),
            has_simple: false,
            ratings: Vec::new(),
            categories: Vec::new(),
            service_id: 0,
            count: 0,
            live_output: false,
        };

        unsafe {
            let c_event = rust_event.to_ctype();
            let converted_rust =
                EPGEventRust::from_ctype(c_event).expect("Conversion should succeed");

            // Fixed-size arrays should be truncated to fit
            assert!(converted_rust.start_time_string.len() <= 20); // Array size - 1 for null terminator
            assert!(converted_rust.end_time_string.len() <= 20);
            assert!(converted_rust.iso_639_language_code.len() <= 3);
            assert!(converted_rust.extended_iso_639_language_code.len() <= 3);

            // Dynamically allocated strings should preserve full length
            assert_eq!(converted_rust.event_name, Some(long_string.clone()));
            assert_eq!(converted_rust.text, Some(long_string.clone()));
            assert_eq!(converted_rust.extended_text, Some(long_string.clone()));

            EPGEventRust::cleanup_c_event(c_event);
        }
    }

    #[test]
    fn test_boolean_conversions() {
        let test_cases = vec![(true, false), (false, true), (true, true), (false, false)];

        for (has_simple, live_output) in test_cases {
            let rust_event = EPGEventRust {
                id: 1,
                start_time_string: "test".to_string(),
                end_time_string: "test".to_string(),
                running_status: 0,
                free_ca_mode: 0,
                iso_639_language_code: "eng".to_string(),
                event_name: None,
                text: None,
                extended_iso_639_language_code: "eng".to_string(),
                extended_text: None,
                has_simple,
                ratings: Vec::new(),
                categories: Vec::new(),
                service_id: 0,
                count: 0,
                live_output,
            };

            unsafe {
                let c_event = rust_event.to_ctype();
                assert_eq!(c_event.has_simple, if has_simple { 1 } else { 0 });
                assert_eq!(c_event.live_output, if live_output { 1 } else { 0 });

                let converted_rust =
                    EPGEventRust::from_ctype(c_event).expect("Conversion should succeed");
                assert_eq!(converted_rust.has_simple, has_simple);
                assert_eq!(converted_rust.live_output, live_output);

                EPGEventRust::cleanup_c_event(c_event);
            }
        }
    }
    #[test]
    fn test_epg_rating_conversion() {
        // Test individual rating conversion
        let rust_rating = EPGRatingRust {
            country_code: "US".to_string(),
            age: 18,
        };

        unsafe {
            let c_rating = rust_rating.to_ctype();
            assert_eq!(c_rating.age, 18);
            assert_eq!(c_char_array_to_string(&c_rating.country_code), "US");

            let converted_rating =
                EPGRatingRust::from_ctype(c_rating).expect("Rating conversion should succeed");
            assert_eq!(converted_rating.country_code, "US");
            assert_eq!(converted_rating.age, 18);
            assert_eq!(converted_rating, rust_rating);
        }
    }

    #[test]
    fn test_epg_rating_edge_cases() {
        // Test empty country code
        let rust_rating = EPGRatingRust {
            country_code: "".to_string(),
            age: 0,
        };

        unsafe {
            let c_rating = rust_rating.to_ctype();
            let converted_rating =
                EPGRatingRust::from_ctype(c_rating).expect("Rating conversion should succeed");
            assert_eq!(converted_rating.country_code, "");
            assert_eq!(converted_rating.age, 0);
        }

        // Test long country code (should be truncated)
        let rust_rating = EPGRatingRust {
            country_code: "TOOLONG".to_string(),
            age: 21,
        };

        unsafe {
            let c_rating = rust_rating.to_ctype();
            let converted_rating =
                EPGRatingRust::from_ctype(c_rating).expect("Rating conversion should succeed");
            // Should be truncated to fit in 4 chars (3 + null terminator)
            assert!(converted_rating.country_code.len() <= 3);
            assert_eq!(converted_rating.age, 21);
        }

        // Test maximum age value
        let rust_rating = EPGRatingRust {
            country_code: "MAX".to_string(),
            age: 255,
        };

        unsafe {
            let c_rating = rust_rating.to_ctype();
            let converted_rating =
                EPGRatingRust::from_ctype(c_rating).expect("Rating conversion should succeed");
            assert_eq!(converted_rating.country_code, "MAX");
            assert_eq!(converted_rating.age, 255);
        }
    }

    #[test]
    fn test_multiple_ratings_conversion() {
        let ratings = vec![
            EPGRatingRust {
                country_code: "US".to_string(),
                age: 13,
            },
            EPGRatingRust {
                country_code: "GB".to_string(),
                age: 15,
            },
            EPGRatingRust {
                country_code: "DE".to_string(),
                age: 16,
            },
            EPGRatingRust {
                country_code: "FR".to_string(),
                age: 12,
            },
        ];

        let rust_event = EPGEventRust {
            id: 1,
            start_time_string: "test".to_string(),
            end_time_string: "test".to_string(),
            running_status: 0,
            free_ca_mode: 0,
            iso_639_language_code: "eng".to_string(),
            event_name: None,
            text: None,
            extended_iso_639_language_code: "eng".to_string(),
            extended_text: None,
            has_simple: false,
            ratings: ratings.clone(),
            categories: Vec::new(),
            service_id: 0,
            count: 0,
            live_output: false,
        };

        unsafe {
            let c_event = rust_event.to_ctype();
            assert_eq!(c_event.num_ratings, 4);

            let ratings_slice = std::slice::from_raw_parts(c_event.ratings, 4);
            for (i, rating) in ratings_slice.iter().enumerate() {
                assert_eq!(rating.age, ratings[i].age);
                assert_eq!(
                    c_char_array_to_string(&rating.country_code),
                    ratings[i].country_code
                );
            }

            let converted_rust =
                EPGEventRust::from_ctype(c_event).expect("Conversion should succeed");
            assert_eq!(converted_rust.ratings, ratings);

            EPGEventRust::cleanup_c_event(c_event);
        }
    }
}
