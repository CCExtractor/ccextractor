//! Provides Rust equivalent for functions in C. Uses Rust-native types as input and output.

use super::*;
use crc32fast::hash;

/// Rust equivalent for `verify_crc32` function in C. Uses Rust-native types as input and output.
pub fn verify_crc32(buf: &[u8]) -> bool {
    hash(buf) == 0
}

/// Rust equivalent for `levenshtein_dist` function in C. Uses Rust-native types as input and output.
pub fn levenshtein_dist(s1: &[u64], s2: &[u64]) -> usize {
    levenshtein(s1, s2)
}

/// Rust equivalent for `levenshtein_dist_char` function in C. Uses Rust-native types as input and output.
pub fn levenshtein_dist_char<T: Copy + PartialEq>(s1: &[T], s2: &[T]) -> usize {
    levenshtein(s1, s2)
}
