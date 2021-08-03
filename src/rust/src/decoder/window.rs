use std::{
    alloc::{alloc_zeroed, dealloc, Layout},
    intrinsics::copy_nonoverlapping,
};

use log::{debug, error};

use super::{
    timing::get_time_str, CCX_DTVCC_MAX_COLUMNS, CCX_DTVCC_MAX_ROWS, CCX_DTVCC_SCREENGRID_COLUMNS,
    CCX_DTVCC_SCREENGRID_ROWS,
};
use crate::{bindings::*, utils::is_true};

impl dtvcc_window {
    pub fn set_style(&mut self, preset: WindowPreset) {
        let style_id = preset as i32;
        let window_style = WindowStyle::new(preset);
        self.win_style = style_id;
        self.attribs.border_color = window_style.border_color as i32;
        self.attribs.border_type = window_style.border_type as i32;
        self.attribs.display_effect = window_style.display_effect as i32;
        self.attribs.effect_direction = window_style.effect_direction as i32;
        self.attribs.effect_speed = window_style.effect_speed as i32;
        self.attribs.fill_color = window_style.fill_color as i32;
        self.attribs.fill_opacity = window_style.fill_opacity as i32;
        self.attribs.justify = window_style.justify as i32;
        self.attribs.print_direction = window_style.print_direction as i32;
        self.attribs.scroll_direction = window_style.scroll_direction as i32;
        self.attribs.word_wrap = window_style.word_wrap as i32;
    }
    pub fn set_pen_style(&mut self, preset: PenPreset) {
        let pen_style = PenStyle::new(preset);
        let pen = &mut self.pen_attribs_pattern;
        pen.pen_size = pen_style.pen_size as i32;
        pen.offset = pen_style.offset as i32;
        pen.edge_type = pen_style.edge_type as i32;
        pen.underline = pen_style.underline as i32;
        pen.italic = pen_style.italics as i32;

        let pen_color = &mut self.pen_color_pattern;
        pen_color.fg_color = pen_style.color.fg_color as i32;
        pen_color.fg_opacity = pen_style.color.fg_opacity as i32;
        pen_color.bg_color = pen_style.color.bg_color as i32;
        pen_color.bg_opacity = pen_style.color.bg_opacity as i32;
        pen_color.edge_color = pen_style.color.edge_color as i32;
    }
    pub fn update_time_show(&mut self, timing: &mut ccx_common_timing_ctx) {
        self.time_ms_show = timing.get_visible_start(3);
        let time = get_time_str(self.time_ms_show);
        debug!("[W-{}] show time updated to {}", self.number, time);
    }
    pub fn update_time_hide(&mut self, timing: &mut ccx_common_timing_ctx) {
        self.time_ms_hide = timing.get_visible_end(3);
        let time = get_time_str(self.time_ms_hide);
        debug!("[W-{}] hide time updated to {}", self.number, time);
    }
    pub fn get_dimensions(&self) -> Result<(i32, i32, i32, i32), String> {
        let anchor = dtvcc_pen_anchor_point::new(self.anchor_point)?;
        let (mut x1, mut x2, mut y1, mut y2) = match anchor {
            dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_TOP_LEFT => (
                self.anchor_vertical,
                self.anchor_vertical + self.row_count,
                self.anchor_horizontal,
                self.anchor_horizontal + self.col_count,
            ),
            dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_TOP_CENTER => (
                self.anchor_vertical,
                self.anchor_vertical + self.row_count,
                self.anchor_horizontal - self.col_count,
                self.anchor_horizontal + self.col_count / 2,
            ),
            dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_TOP_RIGHT => (
                self.anchor_vertical,
                self.anchor_vertical + self.row_count,
                self.anchor_horizontal - self.col_count,
                self.anchor_horizontal,
            ),
            dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_MIDDLE_LEFT => (
                self.anchor_vertical - self.row_count / 2,
                self.anchor_vertical + self.row_count / 2,
                self.anchor_horizontal,
                self.anchor_horizontal + self.col_count,
            ),
            dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_MIDDLE_CENTER => (
                self.anchor_vertical - self.row_count / 2,
                self.anchor_vertical + self.row_count / 2,
                self.anchor_horizontal - self.col_count / 2,
                self.anchor_horizontal + self.col_count / 2,
            ),
            dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_MIDDLE_RIGHT => (
                self.anchor_vertical - self.row_count / 2,
                self.anchor_vertical + self.row_count / 2,
                self.anchor_horizontal - self.col_count,
                self.anchor_horizontal,
            ),
            dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_BOTTOM_LEFT => (
                self.anchor_vertical - self.row_count,
                self.anchor_vertical,
                self.anchor_horizontal,
                self.anchor_horizontal + self.col_count,
            ),
            dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_BOTTOM_CENTER => (
                self.anchor_vertical - self.row_count,
                self.anchor_vertical,
                self.anchor_horizontal - self.col_count / 2,
                self.anchor_horizontal + self.col_count / 2,
            ),
            dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_BOTTOM_RIGHT => (
                self.anchor_vertical - self.row_count,
                self.anchor_vertical,
                self.anchor_horizontal - self.col_count,
                self.anchor_horizontal,
            ),
        };
        if x1 < 0 {
            x1 = 0
        }
        if y1 < 0 {
            y1 = 0
        }
        if x2 > CCX_DTVCC_SCREENGRID_ROWS as i32 {
            x2 = CCX_DTVCC_SCREENGRID_ROWS as i32
        }
        if y2 > CCX_DTVCC_SCREENGRID_COLUMNS as i32 {
            y2 = CCX_DTVCC_SCREENGRID_COLUMNS as i32
        }
        Ok((x1, x2, y1, y2))
    }
    pub fn clear_text(&mut self) {
        // Set pen color to default value
        self.pen_color_pattern = dtvcc_pen_color::default();
        // Set pen attributes to default value
        self.pen_attribs_pattern = dtvcc_pen_attribs::default();
        for row in 0..CCX_DTVCC_MAX_ROWS as usize {
            self.clear_row(row);
        }
        self.is_empty = 1;
    }
    pub fn clear_row(&mut self, row_index: usize) {
        if is_true(self.memory_reserved) {
            unsafe {
                let layout = Layout::array::<dtvcc_symbol>(CCX_DTVCC_MAX_COLUMNS as usize);
                if let Err(e) = layout {
                    error!("clear_row: Incorrect Layout, {}", e);
                } else {
                    let layout = layout.unwrap();
                    // deallocate previous memory
                    dealloc(self.rows[row_index] as *mut u8, layout);

                    // allocate new zero initialized memory
                    let ptr = alloc_zeroed(layout);
                    if ptr.is_null() {
                        error!("clear_row: Not enough memory",);
                    }
                    self.rows[row_index] = ptr as *mut dtvcc_symbol;
                }
            }
            for col in 0..CCX_DTVCC_MAX_COLUMNS as usize {
                // Set pen attributes to default value
                self.pen_attribs[row_index][col] = dtvcc_pen_attribs {
                    pen_size: dtvcc_pen_size::DTVCC_PEN_SIZE_STANDART as i32,
                    offset: 0,
                    text_tag: dtvcc_pen_text_tag::DTVCC_PEN_TEXT_TAG_UNDEFINED_12 as i32,
                    font_tag: 0,
                    edge_type: dtvcc_pen_edge::DTVCC_PEN_EDGE_NONE as i32,
                    underline: 0,
                    italic: 0,
                };
                // Set pen color to default value
                self.pen_colors[row_index][col] = dtvcc_pen_color {
                    fg_color: 0x3F,
                    fg_opacity: 0,
                    bg_color: 0,
                    bg_opacity: 0,
                    edge_color: 0,
                };
            }
        }
    }
    pub fn rollup(&mut self) {
        debug!("roller");
        for row_index in 0..(self.row_count - 1) as usize {
            let curr_row = self.rows[row_index];
            let next_row = self.rows[row_index + 1] as *const dtvcc_symbol;
            unsafe { copy_nonoverlapping(next_row, curr_row, CCX_DTVCC_MAX_COLUMNS as usize) };
            for col_index in 0..CCX_DTVCC_MAX_COLUMNS as usize {
                self.pen_colors[row_index][col_index] = self.pen_colors[row_index + 1][col_index];
                self.pen_attribs[row_index][col_index] = self.pen_attribs[row_index + 1][col_index];
            }
        }
        self.clear_row((self.row_count - 1) as usize);
    }
}

impl dtvcc_window_pd {
    pub fn new(direction: i32) -> Result<Self, String> {
        match direction {
            0 => Ok(dtvcc_window_pd::DTVCC_WINDOW_PD_LEFT_RIGHT),
            1 => Ok(dtvcc_window_pd::DTVCC_WINDOW_PD_RIGHT_LEFT),
            2 => Ok(dtvcc_window_pd::DTVCC_WINDOW_PD_TOP_BOTTOM),
            3 => Ok(dtvcc_window_pd::DTVCC_WINDOW_PD_BOTTOM_TOP),
            _ => Err(String::from("Invalid print direction")),
        }
    }
}

impl dtvcc_pen_anchor_point {
    pub fn new(anchor: i32) -> Result<Self, String> {
        match anchor {
            0 => Ok(dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_TOP_LEFT),
            1 => Ok(dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_TOP_CENTER),
            2 => Ok(dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_TOP_RIGHT),
            3 => Ok(dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_MIDDLE_LEFT),
            4 => Ok(dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_MIDDLE_CENTER),
            5 => Ok(dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_MIDDLE_RIGHT),
            6 => Ok(dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_BOTTOM_LEFT),
            7 => Ok(dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_BOTTOM_CENTER),
            8 => Ok(dtvcc_pen_anchor_point::DTVCC_ANCHOR_POINT_BOTTOM_RIGHT),
            _ => Err(String::from("Invalid pen anchor")),
        }
    }
}

/// Window style for a specific window preset
struct WindowStyle {
    justify: dtvcc_window_justify,
    print_direction: dtvcc_window_pd,
    scroll_direction: dtvcc_window_sd,
    /// Either 0 or 1
    word_wrap: u8,
    /// always snap
    display_effect: dtvcc_window_sde,
    /// N/A always 0
    effect_direction: u8,
    /// N/A always 0
    effect_speed: u8,
    /// Either N/A or black, still always 0
    fill_color: u8,
    fill_opacity: dtvcc_window_fo,
    /// always border_none
    border_type: dtvcc_window_border,
    /// N/A always 0
    border_color: u8,
}

impl WindowStyle {
    pub fn new(preset: WindowPreset) -> Self {
        let effect_direction = 0;
        let effect_speed = 0;
        let fill_color = 0;
        let border_color = 0;
        let display_effect = dtvcc_window_sde::DTVCC_WINDOW_SDE_SNAP;
        let border_type = dtvcc_window_border::DTVCC_WINDOW_BORDER_NONE;

        match preset {
            WindowPreset::NtscPopup => Self {
                justify: dtvcc_window_justify::DTVCC_WINDOW_JUSTIFY_LEFT,
                print_direction: dtvcc_window_pd::DTVCC_WINDOW_PD_LEFT_RIGHT,
                scroll_direction: dtvcc_window_sd::DTVCC_WINDOW_SD_BOTTOM_TOP,
                word_wrap: 0,
                display_effect,
                effect_direction,
                effect_speed,
                fill_color,
                fill_opacity: dtvcc_window_fo::DTVCC_WINDOW_FO_SOLID,
                border_type,
                border_color,
            },
            WindowPreset::Popup => Self {
                justify: dtvcc_window_justify::DTVCC_WINDOW_JUSTIFY_LEFT,
                print_direction: dtvcc_window_pd::DTVCC_WINDOW_PD_LEFT_RIGHT,
                scroll_direction: dtvcc_window_sd::DTVCC_WINDOW_SD_BOTTOM_TOP,
                word_wrap: 0,
                display_effect,
                effect_direction,
                effect_speed,
                fill_color,
                fill_opacity: dtvcc_window_fo::DTVCC_WINDOW_FO_TRANSPARENT,
                border_type,
                border_color,
            },
            WindowPreset::NtscCenteredPopup => Self {
                justify: dtvcc_window_justify::DTVCC_WINDOW_JUSTIFY_CENTER,
                print_direction: dtvcc_window_pd::DTVCC_WINDOW_PD_LEFT_RIGHT,
                scroll_direction: dtvcc_window_sd::DTVCC_WINDOW_SD_BOTTOM_TOP,
                word_wrap: 0,
                display_effect,
                effect_direction,
                effect_speed,
                fill_color,
                fill_opacity: dtvcc_window_fo::DTVCC_WINDOW_FO_SOLID,
                border_type,
                border_color,
            },
            WindowPreset::NtscRollup => Self {
                justify: dtvcc_window_justify::DTVCC_WINDOW_JUSTIFY_LEFT,
                print_direction: dtvcc_window_pd::DTVCC_WINDOW_PD_LEFT_RIGHT,
                scroll_direction: dtvcc_window_sd::DTVCC_WINDOW_SD_BOTTOM_TOP,
                word_wrap: 1,
                display_effect,
                effect_direction,
                effect_speed,
                fill_color,
                fill_opacity: dtvcc_window_fo::DTVCC_WINDOW_FO_SOLID,
                border_type,
                border_color,
            },
            WindowPreset::Rollup => Self {
                justify: dtvcc_window_justify::DTVCC_WINDOW_JUSTIFY_LEFT,
                print_direction: dtvcc_window_pd::DTVCC_WINDOW_PD_LEFT_RIGHT,
                scroll_direction: dtvcc_window_sd::DTVCC_WINDOW_SD_BOTTOM_TOP,
                word_wrap: 1,
                display_effect,
                effect_direction,
                effect_speed,
                fill_color,
                fill_opacity: dtvcc_window_fo::DTVCC_WINDOW_FO_TRANSPARENT,
                border_type,
                border_color,
            },
            WindowPreset::NtscCenteredRollup => Self {
                justify: dtvcc_window_justify::DTVCC_WINDOW_JUSTIFY_CENTER,
                print_direction: dtvcc_window_pd::DTVCC_WINDOW_PD_LEFT_RIGHT,
                scroll_direction: dtvcc_window_sd::DTVCC_WINDOW_SD_BOTTOM_TOP,
                word_wrap: 1,
                display_effect,
                effect_direction,
                effect_speed,
                fill_color,
                fill_opacity: dtvcc_window_fo::DTVCC_WINDOW_FO_SOLID,
                border_type,
                border_color,
            },
            WindowPreset::TickerTape => Self {
                justify: dtvcc_window_justify::DTVCC_WINDOW_JUSTIFY_LEFT,
                print_direction: dtvcc_window_pd::DTVCC_WINDOW_PD_TOP_BOTTOM,
                scroll_direction: dtvcc_window_sd::DTVCC_WINDOW_SD_RIGHT_LEFT,
                word_wrap: 0,
                display_effect,
                effect_direction,
                effect_speed,
                fill_color,
                fill_opacity: dtvcc_window_fo::DTVCC_WINDOW_FO_SOLID,
                border_type,
                border_color,
            },
        }
    }
}

/// Predefined window style ids
#[derive(Clone, Copy)]
pub enum WindowPreset {
    /// #1 NTSC Style Popup Captions
    NtscPopup = 1,
    /// #2 Popup Captions w/o Black Background
    Popup = 2,
    /// #3 NTSC Style Centered Popup Captions
    NtscCenteredPopup = 3,
    /// #4 NTSC Style Rollup Captions
    NtscRollup = 4,
    /// #4 Rollup Captions w/o Black Background
    Rollup = 5,
    /// #6 NTSC Style Centered Rollup Captions
    NtscCenteredRollup = 6,
    /// #7 Ticker Tape
    TickerTape = 7,
}

impl WindowPreset {
    pub fn get_style(style_id: u8) -> Result<Self, String> {
        match style_id {
            1 => Ok(WindowPreset::NtscPopup),
            2 => Ok(WindowPreset::Popup),
            3 => Ok(WindowPreset::NtscCenteredPopup),
            4 => Ok(WindowPreset::NtscRollup),
            5 => Ok(WindowPreset::Rollup),
            6 => Ok(WindowPreset::NtscCenteredRollup),
            7 => Ok(WindowPreset::TickerTape),
            _ => Err("Invalid style".to_owned()),
        }
    }
}

#[derive(PartialEq)]
pub enum PenPreset {
    /// #1 Default NTSC Style
    NtscStyle,
    /// #2 NTSC Style Mono w/ Serif
    NtscStyleMonoSerif,
    /// #3 NTSC Style Prop w/ Serif
    NtscStylePropSerif,
    /// #4 NTSC Style Mono w/o Serif
    NtscStyleMono,
    /// #5 NTSC Style Prop w/o Serif
    NtscStyleProp,
    /// #6 Mono w/o Serif, Bordered Text, No bg
    MonoBordered,
    /// #7 Prop w/o Serif, Bordered Text, No bg
    PropBordered,
}

impl PenPreset {
    pub fn get_style(style_id: u8) -> Result<Self, String> {
        match style_id {
            1 => Ok(PenPreset::NtscStyle),
            2 => Ok(PenPreset::NtscStyleMonoSerif),
            3 => Ok(PenPreset::NtscStylePropSerif),
            4 => Ok(PenPreset::NtscStyleMono),
            5 => Ok(PenPreset::NtscStyleProp),
            6 => Ok(PenPreset::MonoBordered),
            7 => Ok(PenPreset::PropBordered),
            _ => Err("Invalid style".to_owned()),
        }
    }
}

pub struct PenStyle {
    /// always standard pen size
    pen_size: dtvcc_pen_size,
    /// Font style, ranged from 1-7
    /// Not being used current in the C code(bindings)
    _font_style: dtvcc_pen_font_style,
    offset: dtvcc_pen_offset,
    /// always no, i.e. 0
    italics: u8,
    /// always no, i.e. 0
    underline: u8,
    edge_type: dtvcc_pen_edge,
    color: PenColor,
}

impl PenStyle {
    pub fn new(preset: PenPreset) -> Self {
        let pen_size = dtvcc_pen_size::DTVCC_PEN_SIZE_STANDART;
        let offset = dtvcc_pen_offset::DTVCC_PEN_OFFSET_NORMAL;
        let italics = 0;
        let underline = 0;
        let bg_opacity = match preset {
            PenPreset::MonoBordered | PenPreset::PropBordered => Opacity::Transparent,
            _ => Opacity::Solid,
        };

        let color = PenColor {
            /// White(2,2,2) i.e 10,10,10 i.e 42
            fg_color: 42,
            fg_opacity: Opacity::Solid,
            /// Either N/A or black, still always 0
            bg_color: 0,
            bg_opacity,
            /// Either N/A or black, still always 0
            edge_color: 0,
        };

        let (font_style, edge_type) = match preset {
            PenPreset::NtscStyle => (
                dtvcc_pen_font_style::DTVCC_PEN_FONT_STYLE_DEFAULT_OR_UNDEFINED,
                dtvcc_pen_edge::DTVCC_PEN_EDGE_UNIFORM,
            ),
            PenPreset::NtscStyleMonoSerif => (
                dtvcc_pen_font_style::DTVCC_PEN_FONT_STYLE_MONOSPACED_WITH_SERIFS,
                dtvcc_pen_edge::DTVCC_PEN_EDGE_UNIFORM,
            ),
            PenPreset::NtscStylePropSerif => (
                dtvcc_pen_font_style::DTVCC_PEN_FONT_STYLE_PROPORTIONALLY_SPACED_WITH_SERIFS,
                dtvcc_pen_edge::DTVCC_PEN_EDGE_UNIFORM,
            ),
            PenPreset::NtscStyleMono => (
                dtvcc_pen_font_style::DTVCC_PEN_FONT_STYLE_MONOSPACED_WITHOUT_SERIFS,
                dtvcc_pen_edge::DTVCC_PEN_EDGE_UNIFORM,
            ),
            PenPreset::NtscStyleProp => (
                dtvcc_pen_font_style::DTVCC_PEN_FONT_STYLE_PROPORTIONALLY_SPACED_WITHOUT_SERIFS,
                dtvcc_pen_edge::DTVCC_PEN_EDGE_UNIFORM,
            ),
            PenPreset::MonoBordered => (
                dtvcc_pen_font_style::DTVCC_PEN_FONT_STYLE_MONOSPACED_WITHOUT_SERIFS,
                dtvcc_pen_edge::DTVCC_PEN_EDGE_UNIFORM,
            ),
            PenPreset::PropBordered => (
                dtvcc_pen_font_style::DTVCC_PEN_FONT_STYLE_PROPORTIONALLY_SPACED_WITHOUT_SERIFS,
                dtvcc_pen_edge::DTVCC_PEN_EDGE_UNIFORM,
            ),
        };

        Self {
            pen_size,
            _font_style: font_style,
            offset,
            italics,
            underline,
            edge_type,
            color,
        }
    }
}

struct PenColor {
    /// Color of text forground body
    fg_color: u8,
    /// Opacity of text foreground body
    fg_opacity: Opacity,
    /// Color of background box surrounding the text
    bg_color: u8,
    /// Opacity of background box surrounding the text
    bg_opacity: Opacity,
    /// Color of the outlined edges of text
    edge_color: u8,
}
enum Opacity {
    Solid = 0,
    _Flash = 1,
    _Translucent = 2,
    Transparent = 3,
}

impl Default for dtvcc_pen_color{
    fn default() -> Self {
        Self{
            fg_color: 0x3F,
            fg_opacity: 0,
            bg_color: 0,
            bg_opacity: 0,
            edge_color: 0,
        }
    }
}

impl Default for  dtvcc_pen_attribs{
    fn default() -> Self {
        Self{
            pen_size: dtvcc_pen_size::DTVCC_PEN_SIZE_STANDART as i32,
            offset: 0,
            text_tag: dtvcc_pen_text_tag::DTVCC_PEN_TEXT_TAG_UNDEFINED_12 as i32,
            font_tag: 0,
            edge_type: dtvcc_pen_edge::DTVCC_PEN_EDGE_NONE as i32,
            underline: 0,
            italic: 0,
        }
    }
}