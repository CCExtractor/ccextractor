//! Provides types to extract subtitles from Teletext streams.
//!
//! # Conversion Guide
//!
//! | From                        | To                                                          |
//! |-----------------------------|-------------------------------------------------------------|
//! | `MAX_TLT_PAGES`             | `MAX_TLT_PAGES`                                             |
//! | `teletext_page_t`           | [`TeletextPage`]                                            |
//! | `s_states`                  | `TeletextState`                                             |
//! | `transmission_mode_t`       | `TransmissionMode`                                          |
//! | `data_unit_t`               | [`DataUnit`]                                                |
//! | `TeletextCtx`               | [`TeletextContext`]                                         |
//! | `TTXT_COLOURS`              | `TELETEXT_COLORS`                                           |
//! | `teletext_packet_payload_t` | [`TeletextPacketPayload`]                                   |
//! | `ccx_s_teletext_config`     | [`TeletextConfig`]                                          |
//! | `s_primary_charset`         | [`G0Charset`]                                               |
//! | `g0_charsets_type`          | [`G0CharsetType`]                                           |
//! | `ENTITIES`                  | `map_entities`                                              |
//! | `LAT_RUS`                   | `map_latin_to_russian`                                      |
//! | `unham_8_4`                 | [`decode_hamming_8_4`]                                      |
//! | `unham_24_18`               | [`decode_hamming_24_18`]                                    |
//! | `set_g0_charset`            | [`G0Charset::set_charset`], [`G0CharsetType::from_triplet`] |
//! | `remap_g0_charset`          | `G0Charset::remap_g0_charset`                               |
//! | `telx_to_ucs2`              | [`G0Charset::ucs2_char`]                                    |
//! | `bcd_page_to_int`           | [`TeletextPageNumber::bcd_page_to_u16`]                     |
//! | `telx_case_fix`             | `TeletextContext::telx_case_fix`                            |
//! | `telxcc_dump_prev_page`     | `TeletextContext::telxcc_dump_prev_page`                    |
//! | `process_page`              | `TeletextContext::process_page`                             |
//! | `process_telx_packet`       | [`TeletextContext::process_telx_packet`]                    |
//! | `telxcc_init`               | [`TeletextContext::new`]                                    |
//! | `telxcc_close`              | [`TeletextContext::close`]                                  |
//! | `fuzzy_memcmp`              | [`fuzzy_cmp`]                                               |

use num_enum::{IntoPrimitive, TryFromPrimitive};
use std::cell::Cell;
use std::fmt;
use std::fmt::Write;
use std::sync::RwLock;

use crate::common::OutputFormat;
use crate::subtitle::Subtitle;
use crate::util::encoding::{Ucs2Char, Ucs2String};
use crate::util::log::{debug, info, logger, DebugMessageFlag};
use crate::util::time::{Timestamp, TimestampFormat};
use crate::util::{decode_hamming_24_18, decode_hamming_8_4, levenshtein, parity};

/// UTC referential value.
///
/// It has different meanings based on its value:
/// - `u64::MAX` means don't use UNIX
/// - 0 means use current system time as reference
/// - +1 means use a specific reference
pub static UTC_REFVALUE: RwLock<u64> = RwLock::new(u64::MAX);

const MAX_TLT_PAGES: usize = 1000;

const TELETEXT_COLORS: [&str; 8] = [
    "#000000", // black
    "#ff0000", // red
    "#00ff00", // green
    "#ffff00", // yellow
    "#0000ff", // blue
    "#ff00ff", // magenta
    "#00ffff", // cyan
    "#ffffff", // white
];

const LATIN_TO_RUSSIAN: [(Ucs2Char, char); 63] = [
    (65, 'А'),
    (66, 'Б'),
    (87, 'В'),
    (71, 'Г'),
    (68, 'Д'),
    (69, 'Е'),
    (86, 'Ж'),
    (90, 'З'),
    (73, 'И'),
    (74, 'Й'),
    (75, 'К'),
    (76, 'Л'),
    (77, 'М'),
    (78, 'Н'),
    (79, 'О'),
    (80, 'П'),
    (82, 'Р'),
    (83, 'С'),
    (84, 'Т'),
    (85, 'У'),
    (70, 'Ф'),
    (72, 'Х'),
    (67, 'Ц'),
    (238, 'Ч'),
    (235, 'Ш'),
    (249, 'Щ'),
    (35, 'Ы'),
    (88, 'Ь'),
    (234, 'Э'),
    (224, 'Ю'),
    (81, 'Я'),
    (97, 'а'),
    (98, 'б'),
    (119, 'в'),
    (103, 'г'),
    (100, 'д'),
    (101, 'е'),
    (118, 'ж'),
    (122, 'з'),
    (105, 'и'),
    (106, 'й'),
    (107, 'к'),
    (108, 'л'),
    (109, 'м'),
    (110, 'н'),
    (111, 'о'),
    (112, 'п'),
    (114, 'р'),
    (115, 'с'),
    (116, 'т'),
    (117, 'у'),
    (102, 'ф'),
    (104, 'х'),
    (99, 'ц'),
    (231, 'ч'),
    (226, 'ш'),
    (251, 'щ'),
    (121, 'ъ'),
    (38, 'ы'),
    (120, 'ь'),
    (244, 'э'),
    (232, 'ю'),
    (113, 'я'),
];

const ENTITIES: [(u8, &str); 3] = [(b'<', "&lt;"), (b'>', "&gt;"), (b'&', "&amp;")];

/// Represents a Teletext Packet.
pub struct TeletextPacketPayload {
    _clock_in: u8,     // clock run in
    _framing_code: u8, // framing code, not needed, ETSI 300 706: const 0xe4
    address: [u8; 2],
    data: [u8; 40],
}

/// Represents the possible kinds of G0 character set.
#[derive(Clone, Copy, Debug, PartialEq, Eq, IntoPrimitive, TryFromPrimitive)]
#[repr(u8)]
pub enum G0CharsetType {
    Latin = 0,
    Cyrillic1 = 1,
    Cyrillic2 = 2,
    Cyrillic3 = 3,
    Greek = 4,
    Arabic = 5,
    Hebrew = 6,
}

impl G0CharsetType {
    /// Create a [`G0CharsetType`] from the triple from a Teletext triplet.
    pub fn from_triplet(value: u32) -> G0CharsetType {
        // ETS 300 706, Table 32
        if (value & 0x3c00) == 0x1000 {
            match value & 0x0380 {
                0x0000 => G0CharsetType::Cyrillic1,
                0x0200 => G0CharsetType::Cyrillic2,
                0x0280 => G0CharsetType::Cyrillic3,
                _ => G0CharsetType::Latin,
            }
        } else {
            G0CharsetType::Latin
        }
    }
}

/// Represents the bitcode representation of a [`G0LatinNationalSubset`].
///
/// It can be easily contructed from a [`u8`].
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct G0LatinNationalSubsetId(u8);

impl From<u8> for G0LatinNationalSubsetId {
    fn from(value: u8) -> G0LatinNationalSubsetId {
        G0LatinNationalSubsetId(value)
    }
}

impl fmt::Display for G0LatinNationalSubsetId {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "0x{:1x}.{:1x}", self.0 >> 3, self.0 & 0x07)
    }
}

/// Represents the possible kinds of National Option Subset for G0 Latin character set.
#[derive(Clone, Copy, Debug, PartialEq, Eq, IntoPrimitive, TryFromPrimitive)]
#[repr(u8)]
pub enum G0LatinNationalSubset {
    English = 0x0,
    French = 0x1,
    SwedishFinnishHungarian = 0x2,
    CzechSlovak = 0x3,
    German = 0x4,
    PortugueseSpanish = 0x5,
    Italian = 0x6,
    Rumanian = 0x7,
    Polish = 0x8,
    Turkish = 0x9,
    SerbianCroatianSlovenian = 0xa,
    Estonian = 0xb,
    LettishLithuanian = 0xc,
}

impl fmt::Display for G0LatinNationalSubset {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(
            f,
            "{}",
            match self {
                G0LatinNationalSubset::English => "English",
                G0LatinNationalSubset::French => "French",
                G0LatinNationalSubset::SwedishFinnishHungarian => "Swedish, Finnish, Hungarian",
                G0LatinNationalSubset::CzechSlovak => "Czech, Slovak",
                G0LatinNationalSubset::German => "German",
                G0LatinNationalSubset::PortugueseSpanish => "Portuguese, Spanish",
                G0LatinNationalSubset::Italian => "Italian",
                G0LatinNationalSubset::Rumanian => "Rumanian",
                G0LatinNationalSubset::Polish => "Polish",
                G0LatinNationalSubset::Turkish => "Turkish",
                G0LatinNationalSubset::SerbianCroatianSlovenian => "Serbian, Croatian, Slovenian",
                G0LatinNationalSubset::Estonian => "Estonian",
                G0LatinNationalSubset::LettishLithuanian => "Lettish, Lithuanian",
            }
        )
    }
}

impl G0LatinNationalSubset {
    // array positions where chars from G0_LATIN_NATIONAL_SUBSETS are injected into G0[LATIN]
    const G0_LATIN_NATIONAL_SUBSETS_POSITIONS: [usize; 13] = [
        0x03, 0x04, 0x20, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x5b, 0x5c, 0x5d, 0x5e,
    ];

    // ETS 300 706, chapter 15.2, table 32: Function of Default G0 and G2 Character Set Designation
    // and National Option Selection bits in packets X/28/0 Format 1, X/28/4, M/29/0 and M/29/4

    // Latin National Option Sub-sets
    const G0_LATIN_NATIONAL_SUBSETS: [[Ucs2Char; 13]; 13] = [
        // English
        [
            0x00a3, 0x0024, 0x0040, 0x00ab, 0x00bd, 0x00bb, 0x005e, 0x0023, 0x002d, 0x00bc, 0x00a6,
            0x00be, 0x00f7,
        ],
        // French
        [
            0x00e9, 0x00ef, 0x00e0, 0x00eb, 0x00ea, 0x00f9, 0x00ee, 0x0023, 0x00e8, 0x00e2, 0x00f4,
            0x00fb, 0x00e7,
        ],
        // Swedish, Finnish, Hungarian
        [
            0x0023, 0x00a4, 0x00c9, 0x00c4, 0x00d6, 0x00c5, 0x00dc, 0x005f, 0x00e9, 0x00e4, 0x00f6,
            0x00e5, 0x00fc,
        ],
        // Czech, Slovak
        [
            0x0023, 0x016f, 0x010d, 0x0165, 0x017e, 0x00fd, 0x00ed, 0x0159, 0x00e9, 0x00e1, 0x011b,
            0x00fa, 0x0161,
        ],
        // German
        [
            0x0023, 0x0024, 0x00a7, 0x00c4, 0x00d6, 0x00dc, 0x005e, 0x005f, 0x00b0, 0x00e4, 0x00f6,
            0x00fc, 0x00df,
        ],
        // Portuguese, Spanish
        [
            0x00e7, 0x0024, 0x00a1, 0x00e1, 0x00e9, 0x00ed, 0x00f3, 0x00fa, 0x00bf, 0x00fc, 0x00f1,
            0x00e8, 0x00e0,
        ],
        // Italian
        [
            0x00a3, 0x0024, 0x00e9, 0x00b0, 0x00e7, 0x00bb, 0x005e, 0x0023, 0x00f9, 0x00e0, 0x00f2,
            0x00e8, 0x00ec,
        ],
        // Rumanian
        [
            0x0023, 0x00a4, 0x0162, 0x00c2, 0x015e, 0x0102, 0x00ce, 0x0131, 0x0163, 0x00e2, 0x015f,
            0x0103, 0x00ee,
        ],
        // Polish
        [
            0x0023, 0x0144, 0x0105, 0x017b, 0x015a, 0x0141, 0x0107, 0x00f3, 0x0119, 0x017c, 0x015b,
            0x0142, 0x017a,
        ],
        // Turkish
        [
            0x0054, 0x011f, 0x0130, 0x015e, 0x00d6, 0x00c7, 0x00dc, 0x011e, 0x0131, 0x015f, 0x00f6,
            0x00e7, 0x00fc,
        ],
        // Serbian, Croatian, Slovenian
        [
            0x0023, 0x00cb, 0x010c, 0x0106, 0x017d, 0x0110, 0x0160, 0x00eb, 0x010d, 0x0107, 0x017e,
            0x0111, 0x0161,
        ],
        // Estonian
        [
            0x0023, 0x00f5, 0x0160, 0x00c4, 0x00d6, 0x017e, 0x00dc, 0x00d5, 0x0161, 0x00e4, 0x00f6,
            0x017e, 0x00fc,
        ],
        // Lettish, Lithuanian
        [
            0x0023, 0x0024, 0x0160, 0x0117, 0x0119, 0x017d, 0x010d, 0x016b, 0x0161, 0x0105, 0x0173,
            0x017e, 0x012f,
        ],
    ];

    // References to the G0_LATIN_NATIONAL_SUBSETS array
    const G0_LATIN_NATIONAL_SUBSETS_MAP: [u8; 56] = [
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x01, 0x02, 0x03, 0x04, 0xff, 0x06,
        0xff, 0x00, 0x01, 0x02, 0x09, 0x04, 0x05, 0x06, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0a,
        0xff, 0x07, 0xff, 0xff, 0x0b, 0x03, 0x04, 0xff, 0x0c, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x09, 0xff, 0xff, 0xff, 0xff,
    ];

    /// Create a [`G0LatinNationalSubset`] from its bitcode representation stored as a [`G0LatinNationalSubsetId`].
    pub fn from_subset_id(c: G0LatinNationalSubsetId) -> Option<G0LatinNationalSubset> {
        let p = *Self::G0_LATIN_NATIONAL_SUBSETS_MAP.get(c.0 as usize)?;
        if p == 0xff {
            None
        } else {
            Some(p.try_into().ok()?)
        }
    }

    /// Return an Iterator containing the position of replacement and the character to replace when
    /// changing the National Option Subset for G0 Latin character set.
    fn replacement_pos_and_char(&self) -> impl Iterator<Item = (usize, Ucs2Char)> {
        let lang_index: u8 = (*self).into();
        Self::G0_LATIN_NATIONAL_SUBSETS_POSITIONS
            .into_iter()
            .zip(Self::G0_LATIN_NATIONAL_SUBSETS[lang_index as usize].into_iter())
    }
}

fn map_latin_to_russian(latin_char: Ucs2Char) -> Option<char> {
    LATIN_TO_RUSSIAN
        .iter()
        .find(|&&(latin, _)| latin == latin_char)
        .map(|&(_, russian)| russian)
}

fn map_entities(c: Ucs2Char) -> Option<&'static str> {
    let c: u8 = if c >= 0x80 {
        return None;
    } else {
        c as u8
    };
    match ENTITIES.iter().find(|&&(symbol, _)| symbol == c) {
        Some(&(_, entity)) => Some(entity),
        None => None,
    }
}

/// A collective type to manage the entire G0 character set.
///
/// This type is used to change the G0 charecter set and its Latin National Option Subset. This
/// type also manages the subset priority between M/29 and X/28 packets.
pub struct G0Charset {
    g0_charset: Box<[[Ucs2Char; 96]; 5]>,
    charset_type: G0CharsetType,
    primary_charset_current: G0LatinNationalSubsetId,
    primary_charset_g0_m29: Option<G0LatinNationalSubsetId>,
    primary_charset_g0_x28: Option<G0LatinNationalSubsetId>,
    verbose_debug: bool,
}

impl G0Charset {
    fn new(verbose_debug: bool) -> G0Charset {
        let charset = Box::new([
            [
                // Latin G0 Primary Set
                0x0020, 0x0021, 0x0022, 0x00a3, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029,
                0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 0x0030, 0x0031, 0x0032, 0x0033,
                0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d,
                0x003e, 0x003f, 0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
                0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 0x0050, 0x0051,
                0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0x00ab,
                0x00bd, 0x00bb, 0x005e, 0x0023, 0x002d, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065,
                0x0066, 0x0067, 0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
                0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079,
                0x007a, 0x00bc, 0x00a6, 0x00be, 0x00f7, 0x007f,
            ],
            [
                // Cyrillic G0 Primary Set - Option 1 - Serbian/Croatian
                0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x044b, 0x0027, 0x0028, 0x0029,
                0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 0x0030, 0x0031, 0x3200, 0x0033,
                0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d,
                0x003e, 0x003f, 0x0427, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413,
                0x0425, 0x0418, 0x0408, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e, 0x041f, 0x040c,
                0x0420, 0x0421, 0x0422, 0x0423, 0x0412, 0x0403, 0x0409, 0x040a, 0x0417, 0x040b,
                0x0416, 0x0402, 0x0428, 0x040f, 0x0447, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435,
                0x0444, 0x0433, 0x0445, 0x0438, 0x0428, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e,
                0x043f, 0x042c, 0x0440, 0x0441, 0x0442, 0x0443, 0x0432, 0x0423, 0x0429, 0x042a,
                0x0437, 0x042b, 0x0436, 0x0422, 0x0448, 0x042f,
            ],
            [
                // Cyrillic G0 Primary Set - Option 2 - Russian/Bulgarian
                0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x044b, 0x0027, 0x0028, 0x0029,
                0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 0x0030, 0x0031, 0x0032, 0x0033,
                0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d,
                0x003e, 0x003f, 0x042e, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413,
                0x0425, 0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e, 0x041f, 0x042f,
                0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412, 0x042c, 0x042a, 0x0417, 0x0428,
                0x042d, 0x0429, 0x0427, 0x042b, 0x044e, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435,
                0x0444, 0x0433, 0x0445, 0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e,
                0x043f, 0x044f, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432, 0x044c, 0x044a,
                0x0437, 0x0448, 0x044d, 0x0449, 0x0447, 0x044b,
            ],
            [
                // Cyrillic G0 Primary Set - Option 3 - Ukrainian
                0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x00ef, 0x0027, 0x0028, 0x0029,
                0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 0x0030, 0x0031, 0x0032, 0x0033,
                0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d,
                0x003e, 0x003f, 0x042e, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413,
                0x0425, 0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e, 0x041f, 0x042f,
                0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412, 0x042c, 0x0049, 0x0417, 0x0428,
                0x042d, 0x0429, 0x0427, 0x00cf, 0x044e, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435,
                0x0444, 0x0433, 0x0445, 0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e,
                0x043f, 0x044f, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432, 0x044c, 0x0069,
                0x0437, 0x0448, 0x044d, 0x0449, 0x0447, 0x00ff,
            ],
            [
                // Greek G0 Primary Set
                0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029,
                0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f, 0x0030, 0x0031, 0x0032, 0x0033,
                0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d,
                0x003e, 0x003f, 0x0390, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397,
                0x0398, 0x0399, 0x039a, 0x039b, 0x039c, 0x039d, 0x039e, 0x039f, 0x03a0, 0x03a1,
                0x03a2, 0x03a3, 0x03a4, 0x03a5, 0x03a6, 0x03a7, 0x03a8, 0x03a9, 0x03aa, 0x03ab,
                0x03ac, 0x03ad, 0x03ae, 0x03af, 0x03b0, 0x03b1, 0x03b2, 0x03b3, 0x03b4, 0x03b5,
                0x03b6, 0x03b7, 0x03b8, 0x03b9, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf,
                0x03c0, 0x03c1, 0x03c2, 0x03c3, 0x03c4, 0x03c5, 0x03c6, 0x03c7, 0x03c8, 0x03c9,
                0x03ca, 0x03cb, 0x03cc, 0x03cd, 0x03ce, 0x03cf,
            ],
        ]);

        G0Charset {
            g0_charset: charset,
            charset_type: G0CharsetType::Latin,
            primary_charset_current: G0LatinNationalSubsetId(0),
            primary_charset_g0_m29: None,
            primary_charset_g0_x28: None,
            verbose_debug,
        }
    }

    /// Return the equivalent UCS-2 character for the given teletext character based on the current
    /// character set.
    pub fn ucs2_char(&self, telx_char: u8) -> Ucs2Char {
        if parity(telx_char) {
            debug!(msg_type = DebugMessageFlag::TELETEXT; "- Unrecoverable data error; PARITY({:02x})\n", telx_char);
            return 0x20;
        }

        let r: Ucs2Char = (telx_char & 0x7f).into();
        if r >= 0x20 {
            self.g0_charset[self.charset_type as usize][r as usize - 0x20]
        } else {
            r
        }
    }

    /// Change the G0 character set.
    pub fn set_charset(&mut self, charset: G0CharsetType) {
        self.charset_type = charset;
    }

    /// Set the G0 Latin National Option Subset for M/29 packets.
    ///
    /// It will change the mapping only if a Subset for X/28 is not set since X/28 has a higher
    /// priority than M/29. This method will do nothing if the G0 charset is not
    /// [`G0CharsetType::Latin`].
    pub fn set_g0_m29_latin_subset(&mut self, subset: G0LatinNationalSubsetId) {
        if self.charset_type == G0CharsetType::Latin {
            self.primary_charset_g0_m29 = Some(subset);
            if self.primary_charset_g0_x28.is_none() {
                self.remap_g0_charset(subset);
            }
        }
    }

    /// Set the G0 Latin National Option Subset for X/28 packets.
    ///
    /// This method will do nothing if the G0 charset is not [`G0CharsetType::Latin`].
    pub fn set_g0_x28_latin_subset(&mut self, subset: G0LatinNationalSubsetId) {
        if self.charset_type == G0CharsetType::Latin {
            self.primary_charset_g0_x28 = Some(subset);
            self.remap_g0_charset(subset);
        }
    }

    /// Remove the G0 Latin National Option Subset for X/28 packets.
    ///
    /// It will change the mapping back to the one set for M/29. If the subset for M/29 is not set
    /// then `extra_subset` will be used in place of it. This method will do nothing if the G0
    /// charset is not [`G0CharsetType::Latin`].
    pub fn remove_g0_x28_latin_subset(&mut self, extra_subset: G0LatinNationalSubsetId) {
        if self.charset_type == G0CharsetType::Latin {
            self.primary_charset_g0_x28 = None;
            let subset = self.primary_charset_g0_m29.unwrap_or(extra_subset);
            self.remap_g0_charset(subset);
        }
    }

    /// Replace the characters in `g0_charset` based on the given G0 National Option Subset in
    /// `subset`.
    fn remap_g0_charset(&mut self, subset: G0LatinNationalSubsetId) {
        if self.primary_charset_current != subset {
            if let Some(s) = G0LatinNationalSubset::from_subset_id(subset) {
                for (pos, ch) in s.replacement_pos_and_char() {
                    self.g0_charset[0x00][pos] = ch;
                }
                if self.verbose_debug {
                    eprintln!("- Using G0 Latin National Subset ID {} ({})", subset, s);
                }
                self.primary_charset_current = subset;
            } else {
                eprintln!(
                    "- G0 Latin National Subset ID {} is not implemented",
                    subset
                );
            }
        }
    }
}

/// A collective type to manage the entire G0 character set.
pub struct G2Charset;

impl G2Charset {
    const G2_CHARSET: [[Ucs2Char; 96]; 1] = [
        [
            // Latin G2 Supplementary Set
            0x0020, 0x00a1, 0x00a2, 0x00a3, 0x0024, 0x00a5, 0x0023, 0x00a7, 0x00a4, 0x2018, 0x201c,
            0x00ab, 0x2190, 0x2191, 0x2192, 0x2193, 0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00d7, 0x00b5,
            0x00b6, 0x00b7, 0x00f7, 0x2019, 0x201d, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf, 0x0020,
            0x0300, 0x0301, 0x0302, 0x0303, 0x0304, 0x0306, 0x0307, 0x0308, 0x0000, 0x030a, 0x0327,
            0x005f, 0x030b, 0x0328, 0x030c, 0x2015, 0x00b9, 0x00ae, 0x00a9, 0x2122, 0x266a, 0x20ac,
            0x2030, 0x03B1, 0x0000, 0x0000, 0x0000, 0x215b, 0x215c, 0x215d, 0x215e, 0x03a9, 0x00c6,
            0x0110, 0x00aa, 0x0126, 0x0000, 0x0132, 0x013f, 0x0141, 0x00d8, 0x0152, 0x00ba, 0x00de,
            0x0166, 0x014a, 0x0149, 0x0138, 0x00e6, 0x0111, 0x00f0, 0x0127, 0x0131, 0x0133, 0x0140,
            0x0142, 0x00f8, 0x0153, 0x00df, 0x00fe, 0x0167, 0x014b, 0x0020,
        ],
        //	[ // Cyrillic G2 Supplementary Set
        //	],
        //	[ // Greek G2 Supplementary Set
        //	],
        //	[ // Arabic G2 Supplementary Set
        //	]
    ];

    const G2_ACCENTS: [[Ucs2Char; 52]; 15] = [
        // A B C D E F G H I J K L M N O P Q R S T U V W X Y Z a b c d e f g h i j k l m n o p q r s t u v w x y z
        [
            // grave
            0x00c0, 0x0000, 0x0000, 0x0000, 0x00c8, 0x0000, 0x0000, 0x0000, 0x00cc, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x00d2, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00d9, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x00e0, 0x0000, 0x0000, 0x0000, 0x00e8, 0x0000, 0x0000,
            0x0000, 0x00ec, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00f2, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x00f9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        ],
        [
            // acute
            0x00c1, 0x0000, 0x0106, 0x0000, 0x00c9, 0x0000, 0x0000, 0x0000, 0x00cd, 0x0000, 0x0000,
            0x0139, 0x0000, 0x0143, 0x00d3, 0x0000, 0x0000, 0x0154, 0x015a, 0x0000, 0x00da, 0x0000,
            0x0000, 0x0000, 0x00dd, 0x0179, 0x00e1, 0x0000, 0x0107, 0x0000, 0x00e9, 0x0000, 0x0123,
            0x0000, 0x00ed, 0x0000, 0x0000, 0x013a, 0x0000, 0x0144, 0x00f3, 0x0000, 0x0000, 0x0155,
            0x015b, 0x0000, 0x00fa, 0x0000, 0x0000, 0x0000, 0x00fd, 0x017a,
        ],
        [
            // circumflex
            0x00c2, 0x0000, 0x0108, 0x0000, 0x00ca, 0x0000, 0x011c, 0x0124, 0x00ce, 0x0134, 0x0000,
            0x0000, 0x0000, 0x0000, 0x00d4, 0x0000, 0x0000, 0x0000, 0x015c, 0x0000, 0x00db, 0x0000,
            0x0174, 0x0000, 0x0176, 0x0000, 0x00e2, 0x0000, 0x0109, 0x0000, 0x00ea, 0x0000, 0x011d,
            0x0125, 0x00ee, 0x0135, 0x0000, 0x0000, 0x0000, 0x0000, 0x00f4, 0x0000, 0x0000, 0x0000,
            0x015d, 0x0000, 0x00fb, 0x0000, 0x0175, 0x0000, 0x0177, 0x0000,
        ],
        [
            // tilde
            0x00c3, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0128, 0x0000, 0x0000,
            0x0000, 0x0000, 0x00d1, 0x00d5, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0168, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x00e3, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0129, 0x0000, 0x0000, 0x0000, 0x0000, 0x00f1, 0x00f5, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0169, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        ],
        [
            // macron
            0x0100, 0x0000, 0x0000, 0x0000, 0x0112, 0x0000, 0x0000, 0x0000, 0x012a, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x014c, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x016a, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0101, 0x0000, 0x0000, 0x0000, 0x0113, 0x0000, 0x0000,
            0x0000, 0x012b, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x014d, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x016b, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        ],
        [
            // breve
            0x0102, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x011e, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x016c, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0103, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x011f,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x016d, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        ],
        [
            // dot
            0x0000, 0x0000, 0x010a, 0x0000, 0x0116, 0x0000, 0x0120, 0x0000, 0x0130, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x017b, 0x0000, 0x0000, 0x010b, 0x0000, 0x0117, 0x0000, 0x0121,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x017c,
        ],
        [
            // umlaut
            0x00c4, 0x0000, 0x0000, 0x0000, 0x00cb, 0x0000, 0x0000, 0x0000, 0x00cf, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x00d6, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00dc, 0x0000,
            0x0000, 0x0000, 0x0178, 0x0000, 0x00e4, 0x0000, 0x0000, 0x0000, 0x00eb, 0x0000, 0x0000,
            0x0000, 0x00ef, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00f6, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x00fc, 0x0000, 0x0000, 0x0000, 0x00ff, 0x0000,
        ],
        [
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        ],
        [
            // ring
            0x00c5, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x016e, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x00e5, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x016f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        ],
        [
            // cedilla
            0x0000, 0x0000, 0x00c7, 0x0000, 0x0000, 0x0000, 0x0122, 0x0000, 0x0000, 0x0000, 0x0136,
            0x013b, 0x0000, 0x0145, 0x0000, 0x0000, 0x0000, 0x0156, 0x015e, 0x0162, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00e7, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0137, 0x013c, 0x0000, 0x0146, 0x0000, 0x0000, 0x0000, 0x0157,
            0x015f, 0x0163, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        ],
        [
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        ],
        [
            // double acute
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0150, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0170, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0151, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0171, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        ],
        [
            // ogonek
            0x0104, 0x0000, 0x0000, 0x0000, 0x0118, 0x0000, 0x0000, 0x0000, 0x012e, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0172, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0105, 0x0000, 0x0000, 0x0000, 0x0119, 0x0000, 0x0000,
            0x0000, 0x012f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0173, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        ],
        [
            // caron
            0x0000, 0x0000, 0x010c, 0x010e, 0x011a, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x013d, 0x0000, 0x0147, 0x0000, 0x0000, 0x0000, 0x0158, 0x0160, 0x0164, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x017d, 0x0000, 0x0000, 0x010d, 0x010f, 0x011b, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x013e, 0x0000, 0x0148, 0x0000, 0x0000, 0x0000, 0x0159,
            0x0161, 0x0165, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x017e,
        ],
    ];
}

/// Represents a Teletext Page Number in its bitcode representation.
///
/// It can be easily contructed from a [`u16`].
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub struct TeletextPageNumber(u16);

impl From<u16> for TeletextPageNumber {
    fn from(value: u16) -> TeletextPageNumber {
        TeletextPageNumber(value)
    }
}

impl fmt::Display for TeletextPageNumber {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{:03x}", self.0)
    }
}

impl TeletextPageNumber {
    /// Return the magazine and packet bits.
    pub fn magazine(&self) -> u8 {
        ((self.0 >> 8) & 0x0f) as u8
    }

    /// Return the page bits.
    pub fn page(&self) -> u8 {
        (self.0 & 0xff) as u8
    }

    /// Return the page number after converting the page bits in bcd format to normal integer.
    pub fn bcd_page_to_u16(&self) -> u16 {
        ((self.0 & 0xf00) >> 8) * 100 + ((self.0 & 0xf0) >> 4) * 10 + (self.0 & 0xf)
    }
}

/// Represents a teletext page along with timing information.
pub struct TeletextPage {
    show_timestamp: Timestamp,         // show at timestamp (in ms)
    hide_timestamp: Timestamp,         // hide at timestamp (in ms)
    text: [[Ucs2Char; 40]; 25],        // 25 lines x 40 cols (1 screen/page) of wide chars
    g2_char_present: [[bool; 40]; 25], // false-Supplementary G2 character set NOT used at this position, true-Supplementary G2 character set used at this position
    tainted: bool,                     // true = text variable contains any data
}

/// Settings required to contruct a [`TeletextContext`].
#[allow(dead_code)]
pub struct TeletextConfig {
    /// should telxcc logging be verbose?
    verbose: bool,
    /// teletext page containing cc we want to filter
    page: Cell<TeletextPageNumber>,
    /// Page selected by user, which MIGHT be different to `page` depending on autodetection stuff
    user_page: u16,
    /// false = Don't attempt to correct errors
    dolevdist: bool,
    /// Means 2 fails or less is "the same"
    levdistmincnt: u8,
    /// Means 10% or less is also "the same"
    levdistmaxpct: u8,
    /// Segment we actually process
    extraction_start: Option<Timestamp>,
    /// Segment we actually process
    extraction_end: Option<Timestamp>,
    write_format: OutputFormat,
    date_format: TimestampFormat,
    /// Do NOT set time automatically?
    noautotimeref: bool,
    nofontcolor: bool,
    nohtmlescape: bool,
    latrusmap: bool,
}

/// Represents the possible states that [`TeletextContext`] can be in.
struct TeletextState {
    programme_info_processed: bool,
    pts_initialized: bool,
}

#[derive(Copy, Clone, Debug, Eq, PartialEq)]
enum TransmissionMode {
    Parallel,
    Serial,
}

#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub enum DataUnit {
    EbuTeletextNonsubtitle = 0x02,
    EbuTeletextSubtitle = 0x03,
    EbuTeletextInverted = 0x0c,
    Vps = 0xc3,
    ClosedCaptions = 0xc5,
}

/// A type used for decoding Teletext subtitles.
#[allow(dead_code)]
pub struct TeletextContext<'a> {
    config: &'a TeletextConfig,
    seen_sub_page: [bool; MAX_TLT_PAGES],
    global_timestamp: Timestamp,

    // Current and previous page buffers. This is the output written to file when
    // the time comes.
    page_buffer: TeletextPage,
    page_buffer_prev: Option<String>,
    page_buffer_cur: Option<String>,
    // Current and previous page compare strings. This is plain text (no colors,
    // tags, etc) in UCS2 (fixed length), so we can compare easily.
    ucs2_buffer_prev: Option<Ucs2String>,
    ucs2_buffer_cur: Option<Ucs2String>,
    // Buffer timestamp
    prev_hide_timestamp: Timestamp,
    prev_show_timestamp: Timestamp,
    // subtitle type pages bitmap, 2048 bits = 2048 possible pages in teletext (excl. subpages)
    cc_map: [u8; 256],
    // last timestamp computed
    last_timestamp: Timestamp,
    states: TeletextState,
    // FYI, packet counter
    tlt_packet_counter: u32,
    // teletext transmission mode
    transmission_mode: TransmissionMode,
    // flag indicating if incoming data should be processed or ignored
    receiving_data: bool,

    using_pts: Option<bool>,
    delta: Timestamp,
    t0: Timestamp,

    sentence_cap: bool, //Set to 1 if -sc is passed
    new_sentence: bool,

    g0_charset: G0Charset,

    de_ctr: i32, // a keeps count of packets with flag subtitle ON and data packets
}

impl<'a> TeletextContext<'a> {
    /// Create a new [`TeletextContext`] from parameters in [`TeletextConfig`].
    pub fn new(config: &'a TeletextConfig) -> TeletextContext<'a> {
        TeletextContext {
            config,
            seen_sub_page: [false; MAX_TLT_PAGES],
            global_timestamp: Timestamp::from_millis(0),
            page_buffer: TeletextPage {
                show_timestamp: Timestamp::from_millis(0),
                hide_timestamp: Timestamp::from_millis(0),
                text: [[0; 40]; 25],
                g2_char_present: [[false; 40]; 25],
                tainted: false,
            },
            page_buffer_prev: None,
            page_buffer_cur: None,
            ucs2_buffer_prev: None,
            ucs2_buffer_cur: None,
            prev_hide_timestamp: Timestamp::from_millis(0),
            prev_show_timestamp: Timestamp::from_millis(0),
            cc_map: [0; 256],
            last_timestamp: Timestamp::from_millis(0),
            states: TeletextState {
                programme_info_processed: false,
                pts_initialized: false,
            },
            tlt_packet_counter: 0,
            transmission_mode: TransmissionMode::Serial,
            receiving_data: false,
            using_pts: None,
            delta: Timestamp::from_millis(0),
            t0: Timestamp::from_millis(0),
            sentence_cap: false,
            new_sentence: false,
            g0_charset: G0Charset::new(config.verbose),
            de_ctr: 0,
        }
    }

    /// Fix the case for the sentences stored in `page_buffer_cur`.
    ///
    /// This method will convert the first character of a sentence to uppercase and the rest of the
    /// characters to lowercase.
    fn telx_case_fix(&mut self) {
        let page_buffer_cur = match self.page_buffer_cur.as_mut() {
            None => return,
            Some(p) => p,
        };

        let mut fixed_string = String::with_capacity(page_buffer_cur.len());

        let mut prev_newline = false;

        fixed_string.extend(page_buffer_cur.chars().enumerate().map(|(index, c)| {
            let r = match c {
                ' ' | '-' => c, // case 0x89: // This is a transparent space
                '.' | '?' | '!' | ':' => {
                    self.new_sentence = true;
                    c
                }
                _ => {
                    let result = if self.new_sentence && index != 0 && !prev_newline {
                        c.to_ascii_uppercase()
                    } else if !self.new_sentence && index != 0 && !prev_newline {
                        c.to_ascii_lowercase()
                    } else {
                        c
                    };

                    self.new_sentence = false;
                    result
                }
            };

            prev_newline = c == '\n';
            r
        }));

        *page_buffer_cur = fixed_string;

        todo!() // TODO: telx_correct_case(page_buffer_cur);
    }

    /// Reset the page buffers and return its contents in the form of a [`Subtitle`].
    ///
    /// It moves `page_buffer_cur` to `page_buffer_prev` and `ucs2_buffer_cur` to
    /// `ucs2_buffer_prev`.
    fn telxcc_dump_prev_page(&mut self) -> Option<Subtitle> {
        let page_buffer_prev = self.page_buffer_prev.take()?;

        self.page_buffer_prev = self.page_buffer_cur.take();
        self.ucs2_buffer_prev = self.ucs2_buffer_cur.take();

        Some(Subtitle::new_text(
            page_buffer_prev.into(),
            self.prev_show_timestamp,
            self.prev_hide_timestamp,
            Some(format!("{:03}", self.config.page.get().bcd_page_to_u16())),
            "TLT".into(),
        ))
    }

    fn process_page(&mut self) -> Option<Subtitle> {
        let mut ans = None;

        if self
            .config
            .extraction_start
            .map(|start| self.page_buffer.hide_timestamp < start)
            .unwrap_or(false)
            || self
                .config
                .extraction_end
                .map(|end| self.page_buffer.show_timestamp > end)
                .unwrap_or(false)
            || self.page_buffer.hide_timestamp.millis() == 0
        {
            return None;
        }

        #[cfg(feature = "debug")]
        {
            for (index, row) in self.page_buffer.text.iter().enumerate().skip(1) {
                print!("DEUBG[{:02}]: ", index);
                for c in row {
                    print!("{:3x} ", c)
                }
                println!();
            }
            println!();
        }

        // optimization: slicing column by column -- higher probability we could find boxed area start mark sooner
        let mut page_is_empty = true;
        for col in 0..40 {
            for row in 1..25 {
                if self.page_buffer.text[row][col] == 0x0b {
                    page_is_empty = false;
                    break;
                }
            }

            if !page_is_empty {
                break;
            }
        }

        if page_is_empty {
            return None;
        }

        if self.page_buffer.show_timestamp > self.page_buffer.hide_timestamp {
            self.page_buffer.hide_timestamp = self.page_buffer.show_timestamp;
        }

        let mut line_count: u8 = 0;
        let mut time_reported = false;
        let timecode_show = self
            .page_buffer
            .show_timestamp
            .to_srt_time()
            .expect("could not format to SRT time");
        let timecode_hide = self
            .page_buffer
            .hide_timestamp
            .to_srt_time()
            .expect("could not format to SRT time");

        // process data
        for row in 1..25 {
            let mut col_start: usize = 40;
            let col_stop: usize = 40;

            let mut box_open: bool = false;
            for col in 0..40 {
                // replace all 0/B and 0/A characters with 0/20, as specified in ETS 300 706:
                // Unless operating in "Hold Mosaics" mode, each character space occupied by a
                // spacing attribute is displayed as a SPACE
                if self.page_buffer.text[row][col] == 0x0b {
                    // open the box
                    if col_start == 40 {
                        col_start = col;
                        line_count += 1;
                    } else {
                        self.page_buffer.text[row][col] = 0x20;
                    }
                    box_open = true;
                } else if self.page_buffer.text[row][col] == 0xa {
                    // close the box
                    self.page_buffer.text[row][col] = 0x20;
                    box_open = false;
                }
                // characters between 0xA and 0xB shouldn't be displayed
                // page->text[row][col] > 0x20 added to preserve color information
                else if !box_open && col_start < 40 && self.page_buffer.text[row][col] > 0x20 {
                    self.page_buffer.text[row][col] = 0x20;
                }
            }
            // line is empty
            if col_start > 39 {
                continue;
            }

            // ETS 300 706, chapter 12.2: Alpha White ("Set-After") - Start-of-row default condition.
            // used for colour changes _before_ start box mark
            // white is default as stated in ETS 300 706, chapter 12.2
            // black(0), red(1), green(2), yellow(3), blue(4), magenta(5), cyan(6), white(7)
            let mut foreground_color: u8 = 0x7;
            let mut font_tag_opened = false;

            if line_count > 1 {
                match self.config.write_format {
                    OutputFormat::Transcript => {
                        self.page_buffer_cur.get_or_insert("".into()).push(' ')
                    }
                    OutputFormat::SmpteTt => self
                        .page_buffer_cur
                        .get_or_insert("".into())
                        .push_str("<br/>"),
                    _ => self
                        .page_buffer_cur
                        .get_or_insert("".into())
                        .push_str("\r\n"),
                }
            }

            if logger().expect("could not access logger").is_gui_mode() {
                if !time_reported {
                    let timecode_show_mmss = &timecode_show[3..8];
                    let timecode_hide_mmss = &timecode_hide[3..8];
                    // Note, only MM:SS here as we need to save space in the preview window
                    eprint!(
                        "###TIME###{}-{}\n###SUBTITLES###",
                        timecode_show_mmss, timecode_hide_mmss
                    );
                    time_reported = true;
                } else {
                    eprint!("###SUBTITLE###");
                }
            }

            for col in 0..=col_stop {
                // v is just a shortcut
                let mut v = self.page_buffer.text[row][col];

                if col < col_start && v <= 0x7 {
                    foreground_color = v as u8;
                }

                if col == col_start && (foreground_color != 0x7) && !self.config.nofontcolor {
                    let buffer = self.page_buffer_cur.get_or_insert("".into());
                    let _ = write!(
                        buffer,
                        "<font color=\"{}\">",
                        TELETEXT_COLORS[foreground_color as usize]
                    );
                    font_tag_opened = true;
                }

                if col >= col_start {
                    if v <= 0x7 {
                        // ETS 300 706, chapter 12.2: Unless operating in "Hold Mosaics" mode,
                        // each character space occupied by a spacing attribute is displayed as a SPACE.
                        if !self.config.nofontcolor {
                            if font_tag_opened {
                                self.page_buffer_cur
                                    .get_or_insert("".into())
                                    .push_str("</font>");
                                font_tag_opened = false;
                            }

                            self.page_buffer_cur.get_or_insert("".into()).push(' ');
                            // black is considered as white for telxcc purpose
                            // telxcc writes <font/> tags only when needed
                            if (v > 0x0) && (v < 0x7) {
                                let buffer = self.page_buffer_cur.get_or_insert("".into());
                                let _ = write!(
                                    buffer,
                                    "<font color=\"{}\">",
                                    TELETEXT_COLORS[v as usize]
                                );
                                font_tag_opened = true;
                            }
                        } else {
                            v = 0x20;
                        }
                    }

                    if v >= 0x20 {
                        self.ucs2_buffer_cur
                            .get_or_insert(Default::default())
                            .as_mut_vec()
                            .push(v);

                        if !font_tag_opened && self.config.latrusmap {
                            if let Some(ch) = map_latin_to_russian(v) {
                                v = 0;
                                self.page_buffer_cur.get_or_insert("".into()).push(ch);
                            }
                        }

                        // translate some chars into entities, if in colour mode
                        if !self.config.nofontcolor && !self.config.nohtmlescape {
                            if let Some(s) = map_entities(v) {
                                v = 0;
                                self.page_buffer_cur.get_or_insert("".into()).push_str(s);
                            }
                        }
                    }

                    if v >= 0x20 {
                        let u = char::from_u32(v as u32).unwrap();
                        self.page_buffer_cur.get_or_insert("".into()).push(u);
                        if logger().expect("could not access logger").is_gui_mode() {
                            // For now we just handle the easy stuff
                            eprint!("{}", u);
                        }
                    }
                }
            }

            // no tag will left opened!
            if !self.config.nofontcolor && font_tag_opened {
                self.page_buffer_cur
                    .get_or_insert("".into())
                    .push_str("</font>");
            }

            if logger().expect("could not access logger").is_gui_mode() {
                eprintln!();
            }
        }

        if self.sentence_cap {
            self.telx_case_fix()
        }

        match self.config.write_format {
            OutputFormat::Transcript | OutputFormat::SmpteTt => {
                let page_buffer_prev_len =
                    self.page_buffer_prev.as_ref().map(|s| s.len()).unwrap_or(0);
                if page_buffer_prev_len == 0 {
                    self.prev_show_timestamp = self.page_buffer.show_timestamp;
                }

                let page_buffer_prev = self.page_buffer_prev.as_deref().unwrap_or("");
                let page_buffer_cur = self.page_buffer_cur.as_deref().unwrap_or("");
                let ucs2_buffer_prev = self
                    .ucs2_buffer_prev
                    .as_ref()
                    .map(|x| &x.as_vec()[..])
                    .unwrap_or(&[]);
                let ucs2_buffer_cur = self
                    .ucs2_buffer_cur
                    .as_ref()
                    .map(|x| &x.as_vec()[..])
                    .unwrap_or(&[]);

                if page_buffer_prev_len == 0
                    || (self.config.dolevdist
                        && fuzzy_cmp(
                            page_buffer_prev,
                            page_buffer_cur,
                            ucs2_buffer_prev,
                            ucs2_buffer_cur,
                            self.config.levdistmaxpct,
                            self.config.levdistmincnt,
                        ))
                {
                    // If empty previous buffer, we just start one with the
                    // current page and do nothing. Wait until we see more.
                    self.page_buffer_prev = self.page_buffer_cur.take();
                    self.ucs2_buffer_prev = self.ucs2_buffer_cur.take();
                    self.prev_hide_timestamp = self.page_buffer.hide_timestamp;
                } else {
                    // OK, the old and new buffer don't match. So write the old
                    ans = self.telxcc_dump_prev_page();
                    self.prev_hide_timestamp = self.page_buffer.hide_timestamp;
                    self.prev_show_timestamp = self.page_buffer.show_timestamp;
                }
            }
            _ => {
                ans = Some(Subtitle::new_text(
                    self.page_buffer_cur.take().unwrap().into(),
                    self.page_buffer.show_timestamp,
                    self.page_buffer.hide_timestamp + Timestamp::from_millis(1),
                    None,
                    "TLT".into(),
                ));
            }
        }

        // Also update GUI...

        self.page_buffer_cur = None;
        ans
    }

    /// Process the teletext `packet` and append the extracted subtitles in `subtitles`.
    pub fn process_telx_packet(
        &mut self,
        data_unit: DataUnit,
        packet: &TeletextPacketPayload,
        timestamp: Timestamp,
        subtitles: &mut Vec<Subtitle>,
    ) {
        // variable names conform to ETS 300 706, chapter 7.1.2
        let address = (decode_hamming_8_4(packet.address[1]).unwrap() << 4)
            | decode_hamming_8_4(packet.address[0]).unwrap();
        let mut m = address & 0x7;
        if m == 0 {
            m = 8;
        }
        let y = (address >> 3) & 0x1f;
        let designation_code = if y > 25 {
            decode_hamming_8_4(packet.data[0]).unwrap()
        } else {
            0x00
        };

        if y == 0 {
            // CC map
            let i = (decode_hamming_8_4(packet.data[1]).unwrap() << 4)
                | decode_hamming_8_4(packet.data[0]).unwrap();
            let flag_subtitle = (decode_hamming_8_4(packet.data[5]).unwrap() & 0x08) >> 3;
            self.cc_map[i as usize] |= flag_subtitle << (m - 1);

            let flag_subtitle = flag_subtitle != 0;

            if flag_subtitle && (i < 0xff) {
                let mut thisp = ((m as u32) << 8)
                    | ((decode_hamming_8_4(packet.data[1]).unwrap() as u32) << 4)
                    | (decode_hamming_8_4(packet.data[0]).unwrap() as u32);
                let t1 = format!("{:x}", thisp); // Example: 1928 -> 788
                thisp = t1.parse().unwrap();
                if !self.seen_sub_page[thisp as usize] {
                    self.seen_sub_page[thisp as usize] = true;
                    info!(
                        "\rNotice: Teletext page with possible subtitles detected: {:03}\n",
                        thisp
                    );
                }
            }
            if (self.config.page.get() == 0.into()) && flag_subtitle && (i < 0xff) {
                self.config.page.replace(
                    (((m as u16) << 8)
                        | ((decode_hamming_8_4(packet.data[1]).unwrap() as u16) << 4)
                        | (decode_hamming_8_4(packet.data[0]).unwrap() as u16))
                        .into(),
                );
                info!("- No teletext page specified, first received suitable page is {}, not guaranteed\n", self.config.page.get());
            }

            // Page number and control bits
            let page_number: TeletextPageNumber = (((m as u16) << 8)
                | ((decode_hamming_8_4(packet.data[1]).unwrap() as u16) << 4)
                | (decode_hamming_8_4(packet.data[0]).unwrap() as u16))
                .into();
            let charset = ((decode_hamming_8_4(packet.data[7]).unwrap() & 0x08)
                | (decode_hamming_8_4(packet.data[7]).unwrap() & 0x04)
                | (decode_hamming_8_4(packet.data[7]).unwrap() & 0x02))
                >> 1;
            // let flag_suppress_header = decode_hamming_8_4(packet.data[6]).unwrap() & 0x01;
            // let flag_inhibit_display = (decode_hamming_8_4(packet.data[6]).unwrap() & 0x08) >> 3;

            // ETS 300 706, chapter 9.3.1.3:
            // When set to '1' the service is designated to be in Serial mode and the transmission of a page is terminated
            // by the next page header with a different page number.
            // When set to '0' the service is designated to be in Parallel mode and the transmission of a page is terminated
            // by the next page header with a different page number but the same magazine number.
            // The same setting shall be used for all page headers in the service.
            // ETS 300 706, chapter 7.2.1: Page is terminated by and excludes the next page header packet
            // having the same magazine address in parallel transmission mode, or any magazine address in serial transmission mode.
            self.transmission_mode = if decode_hamming_8_4(packet.data[7]).unwrap() & 0x01 == 0 {
                TransmissionMode::Parallel
            } else {
                TransmissionMode::Serial
            };

            // FIXME: Well, this is not ETS 300 706 kosher, however we are interested in EBU_TELETEXT_SUBTITLE only
            if (self.transmission_mode == TransmissionMode::Parallel)
                && (data_unit != DataUnit::EbuTeletextSubtitle)
                && !(self.de_ctr != 0 && flag_subtitle && self.receiving_data)
            {
                return;
            }

            if self.receiving_data
                && (((self.transmission_mode == TransmissionMode::Serial)
                    && (page_number.page() != self.config.page.get().page()))
                    || ((self.transmission_mode == TransmissionMode::Parallel)
                        && (page_number.page() != self.config.page.get().page())
                        && (m == self.config.page.get().magazine())))
            {
                self.receiving_data = false;
                if !(self.de_ctr != 0 && flag_subtitle) {
                    return;
                }
            }

            // Page transmission is terminated, however now we are waiting for our new page
            if page_number != self.config.page.get()
                && !(self.de_ctr != 0 && flag_subtitle && self.receiving_data)
            {
                return;
            }

            // Now we have the begining of page transmission; if there is page_buffer pending, process it
            if self.page_buffer.tainted {
                // Convert telx to UCS-2 before processing
                for yt in 1..=23 {
                    for it in 0..40 {
                        if self.page_buffer.text[yt][it] != 0x00
                            && !self.page_buffer.g2_char_present[yt][it]
                        {
                            self.page_buffer.text[yt][it] = self
                                .g0_charset
                                .ucs2_char(self.page_buffer.text[yt][it].try_into().unwrap());
                        }
                    }
                }
                // it would be nice, if subtitle hides on previous video frame, so we contract 40 ms (1 frame @25 fps)
                self.page_buffer.hide_timestamp = timestamp - Timestamp::from_millis(40);
                if self.page_buffer.hide_timestamp > timestamp {
                    self.page_buffer.hide_timestamp = Timestamp::from_millis(0);
                }
                if let Some(sub) = self.process_page() {
                    subtitles.push(sub);
                }
                self.de_ctr = 0;
            }

            self.page_buffer.show_timestamp = timestamp;
            self.page_buffer.hide_timestamp = Timestamp::from_millis(0);
            self.page_buffer.text = [[0; 40]; 25];
            self.page_buffer.g2_char_present = [[false; 40]; 25];
            self.page_buffer.tainted = false;
            self.receiving_data = false;
            if self.g0_charset.charset_type == G0CharsetType::Latin {
                // G0 Character National Option Sub-sets selection required only for Latin Character Sets
                self.g0_charset.remove_g0_x28_latin_subset(charset.into())
            }
            /*
            // I know -- not needed; in subtitles we will never need disturbing teletext page status bar
            // displaying tv station name, current time etc.
            if (flag_suppress_header == NO) {
                for (uint8_t i = 14; i < 40; i++) page_buffer.text[y][i] = telx_to_ucs2(packet->data[i]);
                //page_buffer.tainted = YES;
            }
            */
        } else if (m == self.config.page.get().magazine())
            && (1..=23).contains(&y)
            && self.receiving_data
        {
            // ETS 300 706, chapter 9.4.1: Packets X/26 at presentation Levels 1.5, 2.5, 3.5 are used for addressing
            // a character location and overwriting the existing character defined on the Level 1 page
            // ETS 300 706, annex B.2.2: Packets with Y = 26 shall be transmitted before any packets with Y = 1 to Y = 25;
            // so page_buffer.text[y][i] may already contain any character received
            // in frame number 26, skip original G0 character
            for i in 0..40 {
                if self.page_buffer.text[y as usize][i] == 0x00 {
                    self.page_buffer.text[y as usize][i] = packet.data[i] as Ucs2Char;
                }
            }
            self.page_buffer.tainted = true;
            self.de_ctr -= 1;
        } else if (m == self.config.page.get().magazine()) && (y == 26) && self.receiving_data {
            // ETS 300 706, chapter 12.3.2: X/26 definition
            let mut x26_row: u8 = 0;

            let mut triplets: [u32; 13] = [0; 13];
            for (j, triplet) in triplets.iter_mut().enumerate() {
                *triplet = decode_hamming_24_18(
                    ((packet.data[j * 3 + 3] as u32) << 16)
                        | ((packet.data[j * 3 + 2] as u32) << 8)
                        | (packet.data[j * 3 + 1] as u32),
                )
                .unwrap_or(0xffffffff);
            }

            for triplet in triplets {
                // invalid data (HAM24/18 uncorrectable error detected), skip group
                if triplet == 0xffffffff {
                    debug!(msg_type = DebugMessageFlag::TELETEXT; "- Unrecoverable data error; UNHAM24/18()={:04x}\n", triplet);
                    continue;
                }

                let data = ((triplet & 0x3f800) >> 11) as u8;
                let mode = ((triplet & 0x7c0) >> 6) as u8;
                let address = (triplet & 0x3f) as u8;
                let row_address_group = (40..=63).contains(&address);

                // ETS 300 706, chapter 12.3.1, table 27: set active position
                if (mode == 0x04) && row_address_group {
                    x26_row = address - 40;
                    if x26_row == 0 {
                        x26_row = 24;
                    }
                }

                // ETS 300 706, chapter 12.3.1, table 27: termination marker
                if (0x11..=0x1f).contains(&mode) && row_address_group {
                    break;
                }

                // ETS 300 706, chapter 12.3.1, table 27: character from G2 set
                if (mode == 0x0f) && !row_address_group && data > 31 {
                    self.page_buffer.text[x26_row as usize][address as usize] =
                        G2Charset::G2_CHARSET[0][data as usize - 0x20];
                    self.page_buffer.g2_char_present[x26_row as usize][address as usize] = true;
                }

                // ETS 300 706 v1.2.1, chapter 12.3.4, Table 29: G0 character without diacritical mark (display '@' instead of '*')
                if (mode == 0x10) && !row_address_group && data == 64 {
                    // check for @ symbol
                    self.g0_charset.remap_g0_charset(0.into());
                    self.page_buffer.text[x26_row as usize][address as usize] = 0x40;
                }

                // ETS 300 706, chapter 12.3.1, table 27: G0 character with diacritical mark
                if (0x11..=0x1f).contains(&mode) && !row_address_group {
                    // A - Z
                    if (65..=90).contains(&data) {
                        self.page_buffer.text[x26_row as usize][address as usize] =
                            G2Charset::G2_ACCENTS[mode as usize - 0x11][data as usize - 65];
                    }
                    // a - z
                    else if (97..=122).contains(&data) {
                        self.page_buffer.text[x26_row as usize][address as usize] =
                            G2Charset::G2_ACCENTS[mode as usize - 0x11][data as usize - 71];
                    // other
                    } else {
                        self.page_buffer.text[x26_row as usize][address as usize] =
                            self.g0_charset.ucs2_char(data);
                    }
                    self.page_buffer.g2_char_present[x26_row as usize][address as usize] = true;
                }
            }
        } else if (m == self.config.page.get().magazine()) && (y == 28) && self.receiving_data {
            // TODO:
            //   ETS 300 706, chapter 9.4.7: Packet X/28/4
            //   Where packets 28/0 and 28/4 are both transmitted as part of a page, packet 28/0 takes precedence over 28/4 for all but the colour map entry coding.
            if (designation_code == 0) || (designation_code == 4) {
                // ETS 300 706, chapter 9.4.2: Packet X/28/0 Format 1
                // ETS 300 706, chapter 9.4.7: Packet X/28/4
                if let Some(triplet0) = decode_hamming_24_18(
                    ((packet.data[3] as u32) << 16)
                        | ((packet.data[2] as u32) << 8)
                        | packet.data[1] as u32,
                ) {
                    // ETS 300 706, chapter 9.4.2: Packet X/28/0 Format 1 only
                    if (triplet0 & 0x0f) == 0x00 {
                        // ETS 300 706, Table 32
                        self.g0_charset
                            .set_charset(G0CharsetType::from_triplet(triplet0)); // Deciding G0 Character Set
                        self.g0_charset
                            .set_g0_x28_latin_subset((((triplet0 & 0x3f80) >> 7) as u8).into())
                    }
                } else {
                    // invalid data (HAM24/18 uncorrectable error detected), skip group
                    debug!(msg_type = DebugMessageFlag::TELETEXT; "! Unrecoverable data error; UNHAM24/18()={:04x}\n", 0xffffffffu32);
                }
            }
        } else if (m == self.config.page.get().magazine()) && (y == 29) {
            // TODO:
            //   ETS 300 706, chapter 9.5.1 Packet M/29/0
            //   Where M/29/0 and M/29/4 are transmitted for the same magazine, M/29/0 takes precedence over M/29/4.
            if (designation_code == 0) || (designation_code == 4) {
                // ETS 300 706, chapter 9.5.1: Packet M/29/0
                // ETS 300 706, chapter 9.5.3: Packet M/29/4
                if let Some(triplet0) = decode_hamming_24_18(
                    ((packet.data[3] as u32) << 16)
                        | ((packet.data[2] as u32) << 8)
                        | packet.data[1] as u32,
                ) {
                    // ETS 300 706, table 11: Coding of Packet M/29/0
                    // ETS 300 706, table 13: Coding of Packet M/29/4
                    if (triplet0 & 0xff) == 0x00 {
                        self.g0_charset
                            .set_charset(G0CharsetType::from_triplet(triplet0));
                        self.g0_charset
                            .set_g0_m29_latin_subset((((triplet0 & 0x3f80) >> 7) as u8).into())
                    }
                } else {
                    // invalid data (HAM24/18 uncorrectable error detected), skip group
                    debug!(msg_type = DebugMessageFlag::TELETEXT; "! Unrecoverable data error; UNHAM24/18()={:04x}\n", 0xffffffffu32);
                }
            }
        } else if (m == 8) && (y == 30) {
            // ETS 300 706, chapter 9.8: Broadcast Service Data Packets
            if !self.states.programme_info_processed {
                // ETS 300 706, chapter 9.8.1: Packet 8/30 Format 1
                if decode_hamming_8_4(packet.data[0])
                    .map(|x| x < 2)
                    .unwrap_or(false)
                {
                    let mut t: u32 = 0;
                    info!("- Programme Identification Data = ");
                    for i in 20..40 {
                        let c = self.g0_charset.ucs2_char(packet.data[i]);
                        // strip any control codes from PID, eg. TVP station
                        if c < 0x20 {
                            continue;
                        }

                        info!("{}", char::from_u32(c as u32).unwrap());
                    }
                    info!("\n");

                    // OMG! ETS 300 706 stores timestamp in 7 bytes in Modified Julian Day in BCD format + HH:MM:SS in BCD format
                    // + timezone as 5-bit count of half-hours from GMT with 1-bit sign
                    // In addition all decimals are incremented by 1 before transmission.
                    // 1st step: BCD to Modified Julian Day
                    t += ((packet.data[10] & 0x0f) as u32) * 10000;
                    t += (((packet.data[11] & 0xf0) >> 4) as u32) * 1000;
                    t += ((packet.data[11] & 0x0f) as u32) * 100;
                    t += (((packet.data[12] & 0xf0) >> 4) as u32) * 10;
                    t += (packet.data[12] & 0x0f) as u32;
                    t -= 11111;
                    // 2nd step: conversion Modified Julian Day to unix timestamp
                    t = (t - 40587) * 86400;
                    // 3rd step: add time
                    t += 3600
                        * (((packet.data[13] & 0xf0) >> 4) as u32 * 10
                            + (packet.data[13] & 0x0f) as u32);
                    t += 60
                        * (((packet.data[14] & 0xf0) >> 4) as u32 * 10
                            + (packet.data[14] & 0x0f) as u32);
                    t += ((packet.data[15] & 0xf0) >> 4) as u32 * 10
                        + (packet.data[15] & 0x0f) as u32;
                    t -= 40271;
                    // 4th step: conversion to time_t
                    let t0 = Timestamp::from_millis((t as i64) * 1000);

                    info!(
                        "- Universal Time Co-ordinated = {}\n",
                        t0.to_ctime().unwrap()
                    );

                    debug!(msg_type = DebugMessageFlag::TELETEXT; "- Transmission mode = {:?}\n", self.transmission_mode);

                    if self.config.write_format == OutputFormat::Transcript
                        && matches!(self.config.date_format, TimestampFormat::Date { .. })
                        && !self.config.noautotimeref
                    {
                        info!("- Broadcast Service Data Packet received, resetting UTC referential value to {}\n", t0.to_ctime().unwrap());
                        *UTC_REFVALUE.write().unwrap() = t as u64;
                        self.states.pts_initialized = false;
                    }

                    self.states.programme_info_processed = true;
                }
            }
        }
    }

    /// Consumes the [`TeletextContext`] and appends the pending extracted subtitles in `subtitles`.
    pub fn close(mut self, subtitles: Option<&mut Vec<Subtitle>>) {
        info!(
            "\nTeletext decoder: {} packets processed \n",
            self.tlt_packet_counter
        );
        if self.config.write_format != OutputFormat::Rcwt {
            if let Some(subtitles) = subtitles {
                // output any pending close caption
                if self.page_buffer.tainted {
                    // Convert telx to UCS-2 before processing
                    for yt in 1..=23 {
                        for it in 0..40 {
                            if self.page_buffer.text[yt][it] != 0x00
                                && !self.page_buffer.g2_char_present[yt][it]
                            {
                                self.page_buffer.text[yt][it] = self
                                    .g0_charset
                                    .ucs2_char(self.page_buffer.text[yt][it].try_into().unwrap());
                            }
                        }
                    }
                    // this time we do not subtract any frames, there will be no more frames
                    self.page_buffer.hide_timestamp = self.last_timestamp;
                    if let Some(sub) = self.process_page() {
                        subtitles.push(sub);
                    }
                }

                self.telxcc_dump_prev_page();
            }
        }
    }
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
