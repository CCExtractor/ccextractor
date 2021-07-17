use crate::bindings::*;

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
    /// Popup Captions w/o Black Background
    Popup = 2,
    /// #1 NTSC Style Centered Popup Captions
    NtscCenteredPopup = 3,
    /// #1 NTSC Style Rollup Captions
    NtscRollup = 4,
    /// Rollup Captions w/o Black Background
    Rollup = 5,
    /// #1 NTSC Style Centered Rollup Captions
    NtscCenteredRollup = 6,
    /// Ticker Tape
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
}
