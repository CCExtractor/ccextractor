use crate::bindings::{ccx_decoder_608_color_code, font_bits};
use ccx_decoder_608_color_code::*;
use font_bits::*;


pub(crate) const CCX_DECODER_608_SCREEN_ROWS : &'static usize = &15;
pub(crate) const CCX_DECODER_608_SCREEN_WIDTH : &'static usize = &32;

pub(crate) static rowdata_608: &'static [i32] = &[11, -1, 1, 2, 3, 4, 12, 13, 14, 15, 5, 6, 7, 8, 9, 10];

#[derive(Debug)]
#[allow(clippy::upper_case_acronyms)]
pub struct pac2_attrib {
    pub color: ccx_decoder_608_color_code,
    pub font : font_bits,
    pub ident: i32
}
pub(crate) static pac2_attribs : &'static [pac2_attrib] = // Color, font, ident
    &[
        pac2_attrib { color: COL_WHITE, font: FONT_REGULAR, ident: 0 },         // 0x40 || 0x60
        pac2_attrib { color: COL_WHITE, font: FONT_UNDERLINED, ident: 0 },     // 0x41 || 0x61
        pac2_attrib { color: COL_GREEN, font: FONT_REGULAR, ident: 0 },         // 0x42 || 0x62
        pac2_attrib { color: COL_GREEN, font: FONT_UNDERLINED, ident: 0 },     // 0x43 || 0x63
        pac2_attrib { color: COL_BLUE, font: FONT_REGULAR, ident: 0 },         // 0x44 || 0x64
        pac2_attrib { color: COL_BLUE, font: FONT_UNDERLINED, ident: 0 },         // 0x45 || 0x65
        pac2_attrib { color: COL_CYAN, font: FONT_REGULAR, ident: 0 },         // 0x46 || 0x66
        pac2_attrib { color: COL_CYAN, font: FONT_UNDERLINED, ident: 0 },         // 0x47 || 0x67
        pac2_attrib { color: COL_RED, font: FONT_REGULAR, ident: 0 },         // 0x48 || 0x68
        pac2_attrib { color: COL_RED, font: FONT_UNDERLINED, ident: 0 },         // 0x49 || 0x69
        pac2_attrib { color: COL_YELLOW, font: FONT_REGULAR, ident: 0 },         // 0x4a || 0x6a
        pac2_attrib { color: COL_YELLOW, font: FONT_UNDERLINED, ident: 0 },     // 0x4b || 0x6b
        pac2_attrib { color: COL_MAGENTA, font: FONT_REGULAR, ident: 0 },         // 0x4c || 0x6c
        pac2_attrib { color: COL_MAGENTA, font: FONT_UNDERLINED, ident: 0 },     // 0x4d || 0x6d
        pac2_attrib { color: COL_WHITE, font: FONT_ITALICS, ident: 0 },         // 0x4e || 0x6e
        pac2_attrib { color: COL_WHITE, font: FONT_UNDERLINED_ITALICS, ident: 0 }, // 0x4f || 0x6f
        pac2_attrib { color: COL_WHITE, font: FONT_REGULAR, ident: 0 },         // 0x50 || 0x70
        pac2_attrib { color: COL_WHITE, font: FONT_UNDERLINED, ident: 0 },     // 0x51 || 0x71
        pac2_attrib { color: COL_WHITE, font: FONT_REGULAR, ident: 4 },         // 0x52 || 0x72
        pac2_attrib { color: COL_WHITE, font: FONT_UNDERLINED, ident: 4 },     // 0x53 || 0x73
        pac2_attrib { color: COL_WHITE, font: FONT_REGULAR, ident: 8 },         // 0x54 || 0x74
        pac2_attrib { color: COL_WHITE, font: FONT_UNDERLINED, ident: 8 },     // 0x55 || 0x75
        pac2_attrib { color: COL_WHITE, font: FONT_REGULAR, ident: 12 },         // 0x56 || 0x76
        pac2_attrib { color: COL_WHITE, font: FONT_UNDERLINED, ident: 12 },     // 0x57 || 0x77
        pac2_attrib { color: COL_WHITE, font: FONT_REGULAR, ident: 16 },         // 0x58 || 0x78
        pac2_attrib { color: COL_WHITE, font: FONT_UNDERLINED, ident: 16 },     // 0x59 || 0x79
        pac2_attrib { color: COL_WHITE, font: FONT_REGULAR, ident: 20 },         // 0x5a || 0x7a
        pac2_attrib { color: COL_WHITE, font: FONT_UNDERLINED, ident: 20 },     // 0x5b || 0x7b
        pac2_attrib { color: COL_WHITE, font: FONT_REGULAR, ident: 24 },         // 0x5c || 0x7c
        pac2_attrib { color: COL_WHITE, font: FONT_UNDERLINED, ident: 24 },     // 0x5d || 0x7d
        pac2_attrib { color: COL_WHITE, font: FONT_REGULAR, ident: 28 },         // 0x5e || 0x7e
        pac2_attrib { color: COL_WHITE, font: FONT_UNDERLINED, ident: 28 }     // 0x5f || 0x7f
    ];
