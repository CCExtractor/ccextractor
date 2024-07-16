//! Provides function for calculating levenshtein distance.

use std::cmp::min;

/// Calculates the levenshtein distance between two slices.
///
/// # Examples
/// ```rust
/// # use lib_ccxr::util::levenshtein::*;
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

/// Rust equivalent for `levenshtein_dist` function in C. Uses Rust-native types as input and output.
pub fn levenshtein_dist(s1: &[u64], s2: &[u64]) -> usize {
    levenshtein(s1, s2)
}

/// Rust equivalent for `levenshtein_dist_char` function in C. Uses Rust-native types as input and output.
pub fn levenshtein_dist_char<T: Copy + PartialEq>(s1: &[T], s2: &[T]) -> usize {
    levenshtein(s1, s2)
}
