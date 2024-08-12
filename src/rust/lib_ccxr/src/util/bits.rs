//! This module provides various bit-level operations, including parity calculation,
//! bit reversal, and hamming code decoding.
//!
//! - [`get_parity`]: Calculate the parity of an 8-bit value.
//! - [`get_reverse_byte`]: Reverse the bits in an 8-bit value.
//! - [`decode_hamming_8_4`]: Decode a Hamming(8,4) encoded byte.
//! - [`decode_hamming_24_18`]: Decode a Hamming(24,18) encoded value.
//!
//! # Conversion Guide
//!
//! | From                                       | To                             |
//! |--------------------------------------------|--------------------------------|
//! | `PARITY_8`                                 | [`get_parity`]                 |
//! | `REVERSE_8`                                | [`get_reverse_byte`]           |
//! | `UNHAM_8_4` const or `unham_8_4` funcn     | [`decode_hamming_8_4`]         |
//! | `unham_24_18`                              | [`decode_hamming_24_18`]       |
//! | `crc32_table`                              | [`get_crc32_byte`]             |
//! | `verify_crc32`                             | [`verify_crc32`]               |

/// Equivalent to `PARITY_8[256]` const in `hamming.h` C code.
/// Instead use [`get_parity`] function to get parity bit based on your input.
const PARITY_TABLE: [bool; 256] = [
    false, true, true, false, true, false, false, true, true, false, false, true, false, true,
    true, false, true, false, false, true, false, true, true, false, false, true, true, false,
    true, false, false, true, true, false, false, true, false, true, true, false, false, true,
    true, false, true, false, false, true, false, true, true, false, true, false, false, true,
    true, false, false, true, false, true, true, false, true, false, false, true, false, true,
    true, false, false, true, true, false, true, false, false, true, false, true, true, false,
    true, false, false, true, true, false, false, true, false, true, true, false, false, true,
    true, false, true, false, false, true, true, false, false, true, false, true, true, false,
    true, false, false, true, false, true, true, false, false, true, true, false, true, false,
    false, true, true, false, false, true, false, true, true, false, false, true, true, false,
    true, false, false, true, false, true, true, false, true, false, false, true, true, false,
    false, true, false, true, true, false, false, true, true, false, true, false, false, true,
    true, false, false, true, false, true, true, false, true, false, false, true, false, true,
    true, false, false, true, true, false, true, false, false, true, false, true, true, false,
    true, false, false, true, true, false, false, true, false, true, true, false, true, false,
    false, true, false, true, true, false, false, true, true, false, true, false, false, true,
    true, false, false, true, false, true, true, false, false, true, true, false, true, false,
    false, true, false, true, true, false, true, false, false, true, true, false, false, true,
    false, true, true, false,
];

/// Equivalent to `REVERSE_8[256]` const in `hamming.h` C code
/// Instead use [`get_reverse_byte`] function to get any reversed bit based on your input.
const BIT_REVERSE_TABLE: [u8; 256] = [
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
];

/// Equivalent to `UNHAM_8_4[256]` const in `hamming.h` C code
/// Instead use [`decode_hamming_8_4`] function to get decoded hamming code.
const HAMMING_8_4_DECODER_TABLE: [u8; 256] = [
    0x01, 0xff, 0x01, 0x01, 0xff, 0x00, 0x01, 0xff, 0xff, 0x02, 0x01, 0xff, 0x0a, 0xff, 0xff, 0x07,
    0xff, 0x00, 0x01, 0xff, 0x00, 0x00, 0xff, 0x00, 0x06, 0xff, 0xff, 0x0b, 0xff, 0x00, 0x03, 0xff,
    0xff, 0x0c, 0x01, 0xff, 0x04, 0xff, 0xff, 0x07, 0x06, 0xff, 0xff, 0x07, 0xff, 0x07, 0x07, 0x07,
    0x06, 0xff, 0xff, 0x05, 0xff, 0x00, 0x0d, 0xff, 0x06, 0x06, 0x06, 0xff, 0x06, 0xff, 0xff, 0x07,
    0xff, 0x02, 0x01, 0xff, 0x04, 0xff, 0xff, 0x09, 0x02, 0x02, 0xff, 0x02, 0xff, 0x02, 0x03, 0xff,
    0x08, 0xff, 0xff, 0x05, 0xff, 0x00, 0x03, 0xff, 0xff, 0x02, 0x03, 0xff, 0x03, 0xff, 0x03, 0x03,
    0x04, 0xff, 0xff, 0x05, 0x04, 0x04, 0x04, 0xff, 0xff, 0x02, 0x0f, 0xff, 0x04, 0xff, 0xff, 0x07,
    0xff, 0x05, 0x05, 0x05, 0x04, 0xff, 0xff, 0x05, 0x06, 0xff, 0xff, 0x05, 0xff, 0x0e, 0x03, 0xff,
    0xff, 0x0c, 0x01, 0xff, 0x0a, 0xff, 0xff, 0x09, 0x0a, 0xff, 0xff, 0x0b, 0x0a, 0x0a, 0x0a, 0xff,
    0x08, 0xff, 0xff, 0x0b, 0xff, 0x00, 0x0d, 0xff, 0xff, 0x0b, 0x0b, 0x0b, 0x0a, 0xff, 0xff, 0x0b,
    0x0c, 0x0c, 0xff, 0x0c, 0xff, 0x0c, 0x0d, 0xff, 0xff, 0x0c, 0x0f, 0xff, 0x0a, 0xff, 0xff, 0x07,
    0xff, 0x0c, 0x0d, 0xff, 0x0d, 0xff, 0x0d, 0x0d, 0x06, 0xff, 0xff, 0x0b, 0xff, 0x0e, 0x0d, 0xff,
    0x08, 0xff, 0xff, 0x09, 0xff, 0x09, 0x09, 0x09, 0xff, 0x02, 0x0f, 0xff, 0x0a, 0xff, 0xff, 0x09,
    0x08, 0x08, 0x08, 0xff, 0x08, 0xff, 0xff, 0x09, 0x08, 0xff, 0xff, 0x0b, 0xff, 0x0e, 0x03, 0xff,
    0xff, 0x0c, 0x0f, 0xff, 0x04, 0xff, 0xff, 0x09, 0x0f, 0xff, 0x0f, 0x0f, 0xff, 0x0e, 0x0f, 0xff,
    0x08, 0xff, 0xff, 0x05, 0xff, 0x0e, 0x0d, 0xff, 0xff, 0x0e, 0x0f, 0xff, 0x0e, 0x0e, 0xff, 0x0e,
];

/// Equivalent to `crc32_table[256]` const in `utility.c` C code
/// Instead use [`get_crc32_byte`] function to get CRC32 bit.
const CRC32_TABLE: [u32; 256] = [
    0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
    0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61, 0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
    0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
    0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
    0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039, 0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
    0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
    0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
    0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1, 0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
    0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
    0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
    0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde, 0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
    0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
    0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6, 0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
    0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
    0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
    0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637, 0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
    0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
    0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
    0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff, 0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
    0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
    0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
    0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7, 0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
    0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
    0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
    0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8, 0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
    0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
    0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0, 0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
    0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
    0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
    0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668, 0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4,
];

/// Returns the parity of the given 8-bit unsigned integer.
///
/// # Exmaples
/// ```rust
/// # use lib_ccxr::util::bits::*;
/// assert_eq!(get_parity(0), false);
/// assert_eq!(get_parity(1), true);
/// ```
pub fn get_parity(value: u8) -> bool {
    PARITY_TABLE[value as usize]
}

/// Returns a byte with its bits flipped from given 8-bit unsigned integer.
///
/// # Exmaples
/// ```rust
/// # use lib_ccxr::util::bits::*;
/// assert_eq!(get_reverse_byte(0), 0x00);
/// assert_eq!(get_reverse_byte(1), 0x80);
/// ```
pub fn get_reverse_byte(value: u8) -> u8 {
    BIT_REVERSE_TABLE[value as usize]
}

/// Returns an Option of the decoded byte given a \[8,4\] hamming code byte.
/// (ETS 300 706, chapter 8.2)
///
/// # Exmaples
/// ```rust
/// # use lib_ccxr::util::bits::*;
/// assert_eq!(decode_hamming_8_4(0x00), Some(0x01));
/// assert_eq!(decode_hamming_8_4(0x01), None);
/// ```
pub fn decode_hamming_8_4(value: u8) -> Option<u8> {
    let decoded = HAMMING_8_4_DECODER_TABLE[value as usize];
    if decoded == 0xff {
        None
    } else {
        Some(decoded & 0x0f)
    }
}

/// Returns an Option of the decoded byte given a \[24,18\] hamming code byte.
/// (ETS 300 706, chapter 8.3)
///
/// # Exmaples
/// ```rust
/// # use lib_ccxr::util::bits::*;
/// assert_eq!(decode_hamming_24_18(0x00000000), Some(0x00000000));
/// assert_eq!(decode_hamming_24_18(0x00000001), None);
/// ```
pub fn decode_hamming_24_18(mut value: u32) -> Option<u32> {
    let mut test: u8 = 0;

    // Tests A-F correspond to bits 0-6 respectively in 'test'.
    for i in 0..23 {
        test ^= (((value >> i) & 0x01) as u8) * (i + 33);
    }

    // Only parity bit is tested for bit 24
    test ^= (((value >> 23) & 0x01) as u8) * 32u8;

    if (test & 0x1f) != 0x1f {
        // Not all tests A-E correct
        if (test & 0x20) == 0x20 {
            // F correct: Double error
            return None;
        }
        // Test F incorrect: Single error
        value ^= 1 << (30 - test);
    }

    Some(
        (value & 0x000004) >> 2
            | (value & 0x000070) >> 3
            | (value & 0x007f00) >> 4
            | (value & 0x7f0000) >> 5,
    )
}

/// Returns a crc 32-bit from given 8-bit unsigned integer.
///
/// # Exmaples
/// ```rust
/// # use lib_ccxr::util::bits::*;
/// assert_eq!(get_crc32_byte(0), 0x00000000);
/// assert_eq!(get_crc32_byte(1), 0x04c11db7);
/// ```
pub fn get_crc32_byte(value: u8) -> u32 {
    CRC32_TABLE[value as usize]
}

/// Verifies the CRC32-bit value
/// Rust equivalent for `verify_crc32` function in C. Uses Rust-native types as input and output.
pub fn verify_crc32(buf: &[u8]) -> bool {
    let mut crc: i32 = -1;
    for &byte in buf {
        let expr = (crc >> 24) as u8 ^ byte;
        crc = (crc << 8) ^ get_crc32_byte(expr) as i32;
    }
    crc == 0
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_get_parity() {
        assert_eq!(get_parity(0), false);
        assert_eq!(get_parity(1), true);
        assert_eq!(get_parity(128), true);
        assert_eq!(get_parity(255), false);
    }

    #[test]
    fn test_get_reverse_byte() {
        assert_eq!(get_reverse_byte(0), 0x00);
        assert_eq!(get_reverse_byte(1), 0x80);
        assert_eq!(get_reverse_byte(255), 0xFF);
        assert_eq!(get_reverse_byte(0b10101010), 0b01010101);
    }

    #[test]
    fn test_decode_hamming_8_4() {
        assert_eq!(decode_hamming_8_4(0x00), Some(0x01));
        assert_eq!(decode_hamming_8_4(0x01), None);
        assert_eq!(decode_hamming_8_4(0xFF), Some(0x0e));
    }

    #[test]
    fn test_decode_hamming_24_18() {
        assert_eq!(decode_hamming_24_18(0x00000000), Some(0x00000000));
        assert_eq!(decode_hamming_24_18(0x00000001), None);
        assert_eq!(decode_hamming_24_18(0xFFFFFFFF), Some(0x003FFFF));
        assert_eq!(
            decode_hamming_24_18(0b101010101010101010101010),
            Some(0b10101001010100100)
        );
    }

    #[test]
    fn test_get_crc32_byte() {
        assert_eq!(get_crc32_byte(0), 0x00000000);
        assert_eq!(get_crc32_byte(1), 0x04c11db7);
        assert_eq!(get_crc32_byte(255), 0xb1f740b4);
    }
}
