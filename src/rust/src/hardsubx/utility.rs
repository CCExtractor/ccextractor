#[cfg(feature = "hardsubx_ocr")]
use rsmpeg::avutil::*;
#[cfg(feature = "hardsubx_ocr")]
use rsmpeg::ffi::AVRational;
use std::os::raw::{c_char, c_int};
use std::{cmp, ffi};

const AV_TIME_BASE: i32 = 1000000;
const AV_TIME_BASE_Q: AVRational = AVRational {
    num: 1,
    den: AV_TIME_BASE,
};

#[no_mangle]
pub extern "C" fn convert_pts_to_ns(pts: i64, time_base: AVRational) -> i64 {
    av_rescale_q(pts, time_base, AV_TIME_BASE_Q)
}

#[no_mangle]
pub extern "C" fn convert_pts_to_ms(pts: i64, time_base: AVRational) -> i64 {
    av_rescale_q(pts, time_base, AV_TIME_BASE_Q) / 1000
}

#[no_mangle]
pub extern "C" fn convert_pts_to_s(pts: i64, time_base: AVRational) -> i64 {
    av_rescale_q(pts, time_base, AV_TIME_BASE_Q) / 1000000
}

fn _edit_distance_rec(
    word1: &[u8],
    word2: &[u8],
    i: usize,
    j: usize,
    dp_array: &mut Vec<Vec<i32>>,
) -> i32 {
    // Recursive implementation with DP of Levenshtein distance

    if dp_array.is_empty() || dp_array[0].is_empty() {
        // in case word1  or word2 has length 0

        cmp::max(i as i32, j as i32)
    } else if dp_array[i][j] != -1 {
        dp_array[i][j]
    } else if cmp::min(i, j) == 0 {
        dp_array[i][j] = cmp::max(i as i32, j as i32);
        cmp::max(i as i32, j as i32)
    } else {
        let length_branch_one = _edit_distance_rec(word1, word2, i - 1, j, dp_array) + 1;
        let length_branch_two = _edit_distance_rec(word1, word2, i, j - 1, dp_array) + 1;

        let length_branch_three = _edit_distance_rec(word1, word2, i - 1, j - 1, dp_array)
            + if word1[i - 1] == word2[j - 1] { 0 } else { 1 };
        dp_array[i][j] = cmp::min(
            length_branch_one,
            cmp::min(length_branch_two, length_branch_three),
        );
        dp_array[i][j]
    }
}

/// # Safety
///
/// Function deals with C string pointers
/// which might be null

#[no_mangle]
pub unsafe extern "C" fn edit_distance(
    word1: *mut c_char,
    word2: *mut c_char,
    len1: c_int,
    len2: c_int,
) -> c_int {
    // The actual edit_distance function

    let word1_string: &ffi::CStr = ffi::CStr::from_ptr(word1);
    let word2_string: &ffi::CStr = ffi::CStr::from_ptr(word2);

    let len1 = len1 as usize;
    let len2 = len2 as usize;

    let mut dp_array = vec![vec![-1; len2 + 1]; len1 + 1];

    _edit_distance_rec(
        word1_string.to_bytes(),
        word2_string.to_bytes(),
        len1,
        len2,
        &mut dp_array,
    ) as c_int
}
