#[derive(Default, Debug, Clone, Copy)]
pub enum OcrMode {
    #[default]
    Frame = 0,
    Word = 1,
    Letter = 2,
}

#[derive(Default, Debug, Clone, Copy)]
pub enum ColorHue {
    #[default]
    White,
    Yellow,
    Green,
    Cyan,
    Blue,
    Magenta,
    Red,
    Custom(f64),
}

impl ColorHue {
    pub fn get_hue(&self) -> f64 {
        match self {
            ColorHue::White => 0.0,
            ColorHue::Yellow => 60.0,
            ColorHue::Green => 120.0,
            ColorHue::Cyan => 180.0,
            ColorHue::Blue => 240.0,
            ColorHue::Magenta => 300.0,
            ColorHue::Red => 0.0,
            ColorHue::Custom(hue) => *hue,
        }
    }
}
