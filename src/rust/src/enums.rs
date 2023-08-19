impl Default for CcxDebugMessageTypes {
    fn default() -> Self {
        Self::None
    }
}

#[derive(Copy, Clone, Debug)]
pub enum CcxDebugMessageTypes {
    None = 0,
    Parse = 1,
    Vides = 2,
    Time = 4,
    Verbose = 8,
    Decoder608 = 0x10,
    // Decoder708 = 0x20,
    DecoderXds = 0x40,
    Cbraw = 0x80,
    // GenericNotices = 0x100,
    Teletext = 0x200,
    Pat = 0x400,
    Pmt = 0x800,
    Levenshtein = 0x1000,
    Dvb = 0x2000,
    Dumpdef = 0x4000,
    #[cfg(feature = "enable_sharing")]
    Share = 0x8000,
}
