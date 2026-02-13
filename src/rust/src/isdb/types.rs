//! ISDB subtitle decoder types, structures, and constants.
//!
//! # Key Types
//! - [`IsdbSubContext`] - Main decoder state (Rust-owned, passed as `void*` through C)
//! - [`IsdbSubState`] - Current decoding state (layout, CLUT, rollup mode)
//! - [`IsdbSubLayout`] - Display layout (format, position, font, spacing)
//! - [`IsdbText`] - Single text node with buffer and position
//! - [`IsdbSubtitleData`] - Decoded subtitle output (text + timestamps)
//!
//! # Enums
//! - [`WritingFormat`] - Display format variants (horizontal/vertical, resolution)
//! - [`IsdbCcComposition`] - Character composition modes
//! - [`IsdbTmd`] - Time control mode
//!
//! # Constants
//!
//! - [`DEFAULT_CLUT`] - 128-entry default Color Look-Up Table
//!
//! # Conversion Guide
//!
//! | C (ccx_decoders_isdb.c)            | Rust                             |
//! |------------------------------------|----------------------------------|
//! | `ISDBSubContext` struct            | [`IsdbSubContext`]               |
//! | `struct ISDBText`                  | [`IsdbText`]                     |
//! | `enum writing_format`              | [`WritingFormat`]                |
//! | `enum isdb_CC_composition`         | [`IsdbCcComposition`]            |
//! | `enum isdb_tmd`                    | [`IsdbTmd`]                      |
//! | cursor pos fields (x, y)           | [`IsdbPos`]                      |
//! | display area fields (w, h, x, y)   | [`DispArea`]                     |
//! | font scale fields (fscx, fscy)     | [`FontScale`]                    |
//! | cell spacing fields (col, row)     | [`CellSpacing`]                  |
//! | offset time fields                 | [`OffsetTime`]                   |
//! | layout fields in `ISDBSubContext`  | [`IsdbSubLayout`]                |
//! | state fields in `ISDBSubContext`   | [`IsdbSubState`]                 |
//! | `add_cc_sub_text` output params    | [`IsdbSubtitleData`]             |
//! | `RGBA(r,g,b,a)` macro              | [`rgba`]                         |
//! | `default_clut` array               | [`DEFAULT_CLUT`]                 |
//! | `prev_timestamp = UINT_MAX`        | `prev_timestamp: Option<u64>`    |
//! | `list_head text_list_head`         | `text_list: Vec<IsdbText>`       |
//! | `list_head buffered_text`          | `buffered_text: Vec<IsdbText>`   |
//! | `allocate_text_node` + `malloc`    | [`IsdbText::new`]                |
//! | `init_layout`                      | [`IsdbSubLayout::default`]       |
//!
//      ---- unused (but defined anyway for future purposes) below ----
//!
//  | `enum fontSize`                    | [`FontSize`]                     |
//  | `enum csi_command`                 | [`CsiCommand`]                   |
//  | foreground/background color fields | [`Color`]                        |
//  | `reserve_buf`                      | [`IsdbText::reserve`]            |

/// format: 0xAABBGGRR (alpha inverted: 255-a)
pub const fn rgba(r: u8, g: u8, b: u8, a: u8) -> u32 {
    ((255 - a) as u32) << 24 // transparency
        | (b as u32) << 16 // blue
        | (g as u32) << 8 // green
        | (r as u32) // red
}

/// default color lookup table
pub static DEFAULT_CLUT: [u32; 128] = [
    // 0-7
    rgba(0, 0, 0, 255),
    rgba(255, 0, 0, 255),
    rgba(0, 255, 0, 255),
    rgba(255, 255, 0, 255),
    rgba(0, 0, 255, 255),
    rgba(255, 0, 255, 255),
    rgba(0, 255, 255, 255),
    rgba(255, 255, 255, 255),
    // 8-15
    rgba(0, 0, 0, 0),
    rgba(170, 0, 0, 255),
    rgba(0, 170, 0, 255),
    rgba(170, 170, 0, 255),
    rgba(0, 0, 170, 255),
    rgba(170, 0, 170, 255),
    rgba(0, 170, 170, 255),
    rgba(170, 170, 170, 255),
    // 16-23
    rgba(0, 0, 85, 255),
    rgba(0, 85, 0, 255),
    rgba(0, 85, 85, 255),
    rgba(0, 85, 170, 255),
    rgba(0, 85, 255, 255),
    rgba(0, 170, 85, 255),
    rgba(0, 170, 255, 255),
    rgba(0, 255, 85, 255),
    // 24-31
    rgba(0, 255, 170, 255),
    rgba(85, 0, 0, 255),
    rgba(85, 0, 85, 255),
    rgba(85, 0, 170, 255),
    rgba(85, 0, 255, 255),
    rgba(85, 85, 0, 255),
    rgba(85, 85, 85, 255),
    rgba(85, 85, 170, 255),
    // 32-39
    rgba(85, 85, 255, 255),
    rgba(85, 170, 0, 255),
    rgba(85, 170, 85, 255),
    rgba(85, 170, 170, 255),
    rgba(85, 170, 255, 255),
    rgba(85, 255, 0, 255),
    rgba(85, 255, 85, 255),
    rgba(85, 255, 170, 255),
    // 40-47
    rgba(85, 255, 255, 255),
    rgba(170, 0, 85, 255),
    rgba(170, 0, 255, 255),
    rgba(170, 85, 0, 255),
    rgba(170, 85, 85, 255),
    rgba(170, 85, 170, 255),
    rgba(170, 85, 255, 255),
    rgba(170, 170, 85, 255),
    // 48-55
    rgba(170, 170, 255, 255),
    rgba(170, 255, 0, 255),
    rgba(170, 255, 85, 255),
    rgba(170, 255, 170, 255),
    rgba(170, 255, 255, 255),
    rgba(255, 0, 85, 255),
    rgba(255, 0, 170, 255),
    rgba(255, 85, 0, 255),
    // 56-63
    rgba(255, 85, 85, 255),
    rgba(255, 85, 170, 255),
    rgba(255, 85, 255, 255),
    rgba(255, 170, 0, 255),
    rgba(255, 170, 85, 255),
    rgba(255, 170, 170, 255),
    rgba(255, 170, 255, 255),
    rgba(255, 255, 85, 255),
    // 64
    rgba(255, 255, 170, 255),
    // 65-127: zeroed (unused in og C code)
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, // 65-80
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, // 81-96
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, // 97-112
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, // 113-127
];

// ---- enums ----

#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
#[repr(i32)]
pub enum WritingFormat {
    #[default]
    HorizontalStdDensity = 0,
    VerticalStdDensity = 1,
    HorizontalHighDensity = 2,
    VerticalHighDensity = 3,
    HorizontalWesternLang = 4,
    Horizontal1920x1080 = 5,
    Vertical1920x1080 = 6,
    Horizontal960x540 = 7,
    Vertical960x540 = 8,
    Horizontal720x480 = 9,
    Vertical720x480 = 10,
    Horizontal1280x720 = 11,
    Vertical1280x720 = 12,
    HorizontalCustom = 100,
    None = 101,
}

impl WritingFormat {
    pub fn is_horizontal(&self) -> bool {
        matches!(
            self,
            WritingFormat::HorizontalStdDensity
                | WritingFormat::HorizontalHighDensity
                | WritingFormat::HorizontalWesternLang
                | WritingFormat::Horizontal1920x1080
                | WritingFormat::Horizontal960x540
                | WritingFormat::Horizontal720x480
                | WritingFormat::Horizontal1280x720
                | WritingFormat::HorizontalCustom
        )
    }

    pub fn from_i32(value: i32) -> Self {
        match value {
            0 => WritingFormat::HorizontalStdDensity,
            1 => WritingFormat::VerticalStdDensity,
            2 => WritingFormat::HorizontalHighDensity,
            3 => WritingFormat::VerticalHighDensity,
            4 => WritingFormat::HorizontalWesternLang,
            5 => WritingFormat::Horizontal1920x1080,
            6 => WritingFormat::Vertical1920x1080,
            7 => WritingFormat::Horizontal960x540,
            8 => WritingFormat::Vertical960x540,
            9 => WritingFormat::Horizontal720x480,
            10 => WritingFormat::Vertical720x480,
            11 => WritingFormat::Horizontal1280x720,
            12 => WritingFormat::Vertical1280x720,
            100 => WritingFormat::HorizontalCustom,
            _ => WritingFormat::None,
        }
    }
}
// commented to sort clippy warnings - also unused in c code - defined anyway for future purposes
// #[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
// #[repr(i32)]
// pub enum FontSize {
//     Small = 0,
//     Middle = 1,
//     #[default]
//     Standard = 2,
// }

// commented to sort clippy warnings - also unused in c code - defined anyway for future purposes
// CSI (Control Sequence Introducer) commands.
// #[derive(Debug, Clone, Copy, PartialEq, Eq)]
// #[repr(u8)]
// pub enum CsiCommand {
//     /// GSM - Character Deformation
//     Gsm = 0x42,
//     /// SWF - Set Writing Format
//     Swf = 0x53,
//     /// CCC - Composite Character Composition
//     Ccc = 0x54,
//     /// SDF - Set Display Format
//     Sdf = 0x56,
//     /// SSM - Character composition dot designation
//     Ssm = 0x57,
//     /// SHS - Set Horizontal Spacing
//     Shs = 0x58,
//     /// SVS - Set Vertical Spacing
//     Svs = 0x59,
//     /// PLD - Partially Line Down
//     Pld = 0x5B,
//     /// PLU - Partially Line Up
//     Plu = 0x5C,
//     /// GAA - Colouring block
//     Gaa = 0x5D,
//     /// SRC - Raster Colour Designation
//     Src = 0x5E,
//     /// SDP - Set Display Position
//     Sdp = 0x5F,
//     /// ACPS - Active Coordinate Position Set
//     Acps = 0x61,
//     /// TCC - Switch control
//     Tcc = 0x62,
//     /// ORN - Ornament Control
//     Orn = 0x63,
//     /// MDF - Font
//     Mdf = 0x64,
//     /// CFS - Character Font Set
//     Cfs = 0x65,
//     /// XCS - External Character Set
//     Xcs = 0x66,
//     /// PRA - Built-in sound replay
//     Pra = 0x68,
//     /// ACS - Alternative Character Set
//     Acs = 0x69,
//     /// RCS - Raster Color Command
//     Rcs = 0x6E,
//     /// SCS - Skip Character Set
//     Scs = 0x6F,
// }

/// ISDB Closed Caption composition mode.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
#[repr(i32)]
pub enum IsdbCcComposition {
    #[default]
    None = 0,
    And = 2,
    Or = 3,
    Xor = 4,
}

impl IsdbCcComposition {
    pub fn from_i32(value: i32) -> Self {
        match value {
            2 => IsdbCcComposition::And,
            3 => IsdbCcComposition::Or,
            4 => IsdbCcComposition::Xor,
            _ => IsdbCcComposition::None,
        }
    }
}
// commented to sort clippy warnings - also unused in c code - defined anyway for future purposes
// Color indices for ISDB.
// #[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
// #[repr(i32)]
// pub enum Color {
//     #[default]
//     Black = 0,
//     FiRed = 1,
//     FiGreen = 2,
//     FiYellow = 3,
//     FiBlue = 4,
//     FiMagenta = 5,
//     FiCyan = 6,
//     FiWhite = 7,
//     Transparent = 8,
//     HiRed = 9,
//     HiGreen = 10,
//     HiYellow = 11,
//     HiBlue = 12,
//     HiMagenta = 13,
//     HiCyan = 14,
//     HiWhite = 15,
// }

/// ISDB Time Mode.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
#[repr(i32)]
pub enum IsdbTmd {
    #[default]
    Free = 0,
    RealTime = 1,
    OffsetTime = 2,
}

impl IsdbTmd {
    pub fn from_i32(value: i32) -> Self {
        match value {
            1 => IsdbTmd::RealTime,
            2 => IsdbTmd::OffsetTime,
            _ => IsdbTmd::Free,
        }
    }
}

// ---- structs ----

/// Position (x, y) coordinates.
#[derive(Debug, Clone, Copy, Default)]
pub struct IsdbPos {
    pub x: i32,
    pub y: i32,
}

/// Display area dimensions.
#[derive(Debug, Default, Clone, Copy)]
pub struct DispArea {
    pub x: i32,
    pub y: i32,
    pub w: i32,
    pub h: i32,
}

/// Font scale in percent.
#[derive(Debug, Clone, Copy)]
pub struct FontScale {
    pub fscx: i32,
    pub fscy: i32,
}

impl Default for FontScale {
    fn default() -> Self {
        Self {
            fscx: 100,
            fscy: 100,
        }
    }
}

/// Cell spacing (column and row).
#[derive(Debug, Clone, Copy, Default)]
pub struct CellSpacing {
    pub col: i32,
    pub row: i32,
}

/// Offset time for ISDB captions.
#[derive(Debug, Clone, Copy, Default)]
pub struct OffsetTime {
    pub hour: i32,
    pub min: i32,
    pub sec: i32,
    pub milli: i32,
}

/// ISDB subtitle layout state.
#[derive(Debug, Clone)]
pub struct IsdbSubLayout {
    pub format: WritingFormat,
    pub display_area: DispArea,
    pub font_size: i32,
    pub font_scale: FontScale,
    pub cell_spacing: CellSpacing,
    pub cursor_pos: IsdbPos,
    pub ccc: IsdbCcComposition,
    pub acps: [i32; 2],
}

impl Default for IsdbSubLayout {
    fn default() -> Self {
        Self {
            format: WritingFormat::default(),
            display_area: DispArea::default(),
            font_size: 36,
            font_scale: FontScale::default(),
            cell_spacing: CellSpacing::default(),
            cursor_pos: IsdbPos::default(),
            ccc: IsdbCcComposition::default(),
            acps: [0, 0],
        }
    }
}

/// ISDB subtitle state (colors, layout, flags).
// fields `auto_display`, `need_init`, and `mat_color` are never read
// hence commented to sort clippy warnings - also unused in c code - defined anyway for future purposes
#[derive(Debug, Clone, Default)]
pub struct IsdbSubState {
    // pub auto_display: bool,
    pub rollup_mode: bool,
    // pub need_init: bool,
    pub clut_high_idx: u8,
    pub fg_color: u32,
    pub bg_color: u32,
    pub hfg_color: u32,
    pub hbg_color: u32,
    // pub mat_color: u32,
    pub raster_color: u32,
    pub layout_state: IsdbSubLayout,
}

/// A single text entry in the ISDB subtitle display.
// fields `txt_tail` and `timestamp` are never read -
// hence commented to sort clippy warnings - also unused in c code - defined anyway for future purposes
#[derive(Debug, Clone)]
pub struct IsdbText {
    pub buf: Vec<u8>,
    pub pos: IsdbPos,
    // pub txt_tail: usize,
    // pub timestamp: u64,
}

impl IsdbText {
    /// create new text node w default buffer size
    pub fn new(pos: IsdbPos) -> Self {
        Self {
            buf: Vec::with_capacity(128),
            pos,
            // txt_tail: 0,
            // timestamp: 0,
        }
    }

    // rust counterpart of reserve_buf C function - defined anyway for tracking purpose
    // pub fn reserve(&mut self, additional: usize) {
    //     self.buf.reserve(additional);
    // }
}

/// Main ISDB subtitle decoder context.
#[derive(Debug, Clone)]
pub struct IsdbSubContext {
    pub nb_char: i32,
    pub nb_line: i32,
    pub timestamp: u64,
    pub prev_timestamp: Option<u64>,
    pub text_list: Vec<IsdbText>,
    pub buffered_text: Vec<IsdbText>,
    pub current_state: IsdbSubState,
    pub tmd: IsdbTmd,
    pub nb_lang: i32,
    pub offset_time: OffsetTime,
    pub dmf: u8,
    pub dc: u8,
    pub cfg_no_rollup: bool,
}

impl IsdbSubContext {
    pub fn new() -> Self {
        Self {
            nb_char: 0,
            nb_line: 0,
            timestamp: 0,
            prev_timestamp: None,
            text_list: Vec::new(),
            buffered_text: Vec::new(),
            current_state: IsdbSubState::default(),
            tmd: IsdbTmd::default(),
            nb_lang: 0,
            offset_time: OffsetTime::default(),
            dmf: 0,
            dc: 0,
            cfg_no_rollup: false,
        }
    }
}

impl Default for IsdbSubContext {
    fn default() -> Self {
        Self::new()
    }
}

/// Subtitle data produced by the ISDB decoder.
/// Used to pass data from Rust to the FFI layer.
pub struct IsdbSubtitleData {
    pub text: Vec<u8>,
    pub start_time: u64,
    pub end_time: u64,
}

#[cfg(test)]
mod tests {
    use crate::isdb::types::*;

    // ---- init_layout (covered by IsdbSubLayout::default()) ----

    #[test]
    fn test_layout_defaults() {
        let ls = IsdbSubLayout::default();
        assert_eq!(ls.font_size, 36);
        assert_eq!(ls.display_area.x, 0);
        assert_eq!(ls.display_area.y, 0);
        assert_eq!(ls.font_scale.fscx, 100);
        assert_eq!(ls.font_scale.fscy, 100);
    }
}
