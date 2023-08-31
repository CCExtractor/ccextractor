//! Provides basic utilities used throughout the program.
//!
//! # Conversion Guide
//!
//! | From                                       | To                             |
//! |--------------------------------------------|--------------------------------|
//! | `PARITY_8`                                 | [`parity`]                     |
//! | `REVERSE_8`                                | [`reverse`]                    |
//! | `UNHAM_8_4`                                | [`decode_hamming_8_4`]         |
//! | `unham_24_18`                              | [`decode_hamming_24_18`]       |
//! | `levenshtein_dist`, levenshtein_dist_char` | [`levenshtein`](levenshtein()) |

mod bits;
mod levenshtein;

pub mod c_functions;

pub use bits::*;
pub use levenshtein::*;
