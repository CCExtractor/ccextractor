use crate::util::encoding::Ucs2Char;
use crate::util::log::{debug, DebugMessageFlag};
use std::cmp::min;

/// Calculates the levenshtein distance between two slices.
///
/// # Examples
/// ```rust
/// # use lib_ccxr::util::levenshtein;
/// assert_eq!(levenshtein(&[1,2,3,4,5], &[1,3,2,4,5,6]), 3);
/// ```
pub fn levenshtein<T: Copy + PartialEq>(a: &[T], b: &[T]) -> usize {
    let mut column: Vec<usize> = (0..).take(a.len() + 1).collect();

    for x in 1..=b.len() {
        column[0] = x;
        let mut lastdiag = x - 1;
        for y in 1..=a.len() {
            let olddiag = column[y];
            column[y] = min(
                min(column[y] + 1, column[y - 1] + 1),
                lastdiag + (if a[y - 1] == b[x - 1] { 0 } else { 1 }),
            );
            lastdiag = olddiag;
        }
    }

    column[a.len()]
}

/// Check the given two lines can be considered similar using levenshtein
/// distance.
///
/// If the levenshtein distance between `ucs2_buf1` and `ucs2_buf2` is less than either
/// `levdistmincnt` or `levdistmaxpct`% of the length of the shorter line, then the lines are
/// considered to be similar. `c1` and `c2` are used for displaying a debug message only.
///
/// # Examples
/// ```
/// # use lib_ccxr::util::fuzzy_cmp;
/// # use lib_ccxr::util::log::*;
/// # let mask = DebugMessageMask::new(DebugMessageFlag::LEVENSHTEIN, DebugMessageFlag::LEVENSHTEIN);
/// # set_logger(CCExtractorLogger::new(OutputTarget::Quiet, mask, false));
/// let hello_world = [72, 101, 108, 108, 111, 32, 119, 111, 114, 108, 100];
/// let hello_Aorld = [72, 101, 108, 108, 111, 32, 65, 111, 114, 108, 100];
/// let helld_Aorld = [72, 101, 108, 108, 100, 32, 65, 111, 114, 108, 100];
///
/// // Returns true if both lines are same
/// assert!(fuzzy_cmp("", "", &hello_world, &hello_world, 10, 2));
///
/// // Returns true since the distance is 1 which is less than 2.
/// assert!(fuzzy_cmp("", "", &hello_world, &hello_Aorld, 10, 2));
///
/// // Returns false since the distance is 2 which is not less than both 2 and 10% of length.
/// assert!(!fuzzy_cmp("", "", &hello_world, &helld_Aorld, 10, 2));
///
/// // Returns true since the distance is 1 which is less than 20% of length.
/// assert!(fuzzy_cmp("", "", &hello_world, &hello_Aorld, 20, 2));
/// ```
pub fn fuzzy_cmp(
    c1: &str,
    c2: &str,
    ucs2_buf1: &[Ucs2Char],
    ucs2_buf2: &[Ucs2Char],
    levdistmaxpct: u8,
    levdistmincnt: u8,
) -> bool {
    let short_len = std::cmp::min(ucs2_buf1.len(), ucs2_buf2.len());
    let max = std::cmp::max(
        (short_len * levdistmaxpct as usize) / 100,
        levdistmincnt.into(),
    );

    // For the second string, only take the first chars (up to the first string length, that's short_len).
    let l = levenshtein(ucs2_buf1, &ucs2_buf2[..short_len]);
    let is_same = l < max;
    debug!(msg_type = DebugMessageFlag::LEVENSHTEIN; "\rLEV | {} | {} | Max: {} | Calc: {} | Match: {}\n", c1, c2, max, l, is_same);
    is_same
}
