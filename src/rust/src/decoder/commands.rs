//! Different code sets as defined in CEA-708-E
//!
//! Refer section 7.1 CEA-708-E
//! Different code sets are:
//!
//! | Name | Description | Space |
//! | --- | --- | --- |
//! | C0 | Miscellaneous Control Codes | 0x00 to 0x1F |
//! | C2 |  Extended Control Code Set 1 | 0x00 to 0x1F |
//! | G0 |  ASCII Printable Characters | 0x20 to 0x7F |
//! | G2 | Extended Miscellaneous Characters | 0x20 to 0x7F |
//! | C1 | Captioning Command Control Codes | 0x80 to 0x9F |
//! | C3 | Extended Control Code Set 2 | 0x80 to 0x9F |
//! | G1 |  ISO 8859-1 Latin-1 Character Set | 0xA0 to 0xFF |
//! | G3 |  Future Expansion | 0xA0 to 0xFF |

/// Different C0 commands
#[allow(clippy::upper_case_acronyms)]
#[derive(Debug)]
pub enum C0CodeSet {
    NUL,
    ETX,
    BS,
    FF,
    CR,
    HCR,
    EXT1,
    P16,
    RESERVED,
}

impl C0CodeSet {
    /// Create a new command from the byte value
    pub fn new(data: u8) -> Self {
        match data {
            0 => C0CodeSet::NUL,
            0x3 => C0CodeSet::ETX,
            0x8 => C0CodeSet::BS,
            0xC => C0CodeSet::FF,
            0xD => C0CodeSet::CR,
            0xE => C0CodeSet::HCR,
            0x10 => C0CodeSet::EXT1,
            0x18 => C0CodeSet::P16,
            _ => C0CodeSet::RESERVED,
        }
    }
}

/// C0 command with its length
pub struct C0Command {
    pub command: C0CodeSet,
    pub length: u8,
}

impl C0Command {
    /// Create a new command from the byte value
    pub fn new(data: u8) -> Self {
        let command = C0CodeSet::new(data);
        // lengths are defined in CEA-708-E section 7.1.4
        let length = match data {
            0..=0xF => 1,
            0x10..=0x17 => 2,
            0x18..=0x1F => 3,
            // Unreachable arm as C0 code set is only from 0 - 0x1F
            _ => 1,
        };
        C0Command { command, length }
    }
}

/// Different C1 commands
#[allow(clippy::upper_case_acronyms)]
#[derive(Debug)]
pub enum C1CodeSet {
    CW0,
    CW1,
    CW2,
    CW3,
    CW4,
    CW5,
    CW6,
    CW7,
    CLW,
    DSW,
    HDW,
    TGW,
    DLW,
    DLY,
    DLC,
    RST,
    SPA,
    SPC,
    SPL,
    SWA,
    DF0,
    DF1,
    DF2,
    DF3,
    DF4,
    DF5,
    DF6,
    DF7,
    RESERVED,
}

/// C1 command with its length
pub struct C1Command {
    pub command: C1CodeSet,
    pub length: u8,
    pub name: String,
}

impl C1Command {
    /// Create a new command from the byte value
    pub fn new(data: u8) -> Self {
        let (command, name, length) = match data {
            0x80 => (C1CodeSet::CW0, "SetCurrentWindow0", 1),
            0x81 => (C1CodeSet::CW1, "SetCurrentWindow1", 1),
            0x82 => (C1CodeSet::CW2, "SetCurrentWindow2", 1),
            0x83 => (C1CodeSet::CW3, "SetCurrentWindow3", 1),
            0x84 => (C1CodeSet::CW4, "SetCurrentWindow4", 1),
            0x85 => (C1CodeSet::CW5, "SetCurrentWindow5", 1),
            0x86 => (C1CodeSet::CW6, "SetCurrentWindow6", 1),
            0x87 => (C1CodeSet::CW7, "SetCurrentWindow7", 1),
            0x88 => (C1CodeSet::CLW, "ClearWindows", 2),
            0x89 => (C1CodeSet::DSW, "DisplayWindows", 2),
            0x8A => (C1CodeSet::HDW, "HideWindows", 2),
            0x8B => (C1CodeSet::TGW, "ToggleWindows", 2),
            0x8C => (C1CodeSet::DLW, "DeleteWindows", 2),
            0x8D => (C1CodeSet::DLY, "Delay", 2),
            0x8E => (C1CodeSet::DLC, "DelayCancel", 1),
            0x8F => (C1CodeSet::RST, "Reset", 1),
            0x90 => (C1CodeSet::SPA, "SetPenAttributes", 3),
            0x91 => (C1CodeSet::SPC, "SetPenColor", 4),
            0x92 => (C1CodeSet::SPL, "SetPenLocation", 3),
            0x97 => (C1CodeSet::SWA, "SetWindowAttributes", 5),
            0x98 => (C1CodeSet::DF0, "DefineWindow0", 7),
            0x99 => (C1CodeSet::DF1, "DefineWindow1", 7),
            0x9A => (C1CodeSet::DF2, "DefineWindow2", 7),
            0x9B => (C1CodeSet::DF3, "DefineWindow3", 7),
            0x9C => (C1CodeSet::DF4, "DefineWindow4", 7),
            0x9D => (C1CodeSet::DF5, "DefineWindow5", 7),
            0x9E => (C1CodeSet::DF6, "DefineWindow6", 7),
            0x9F => (C1CodeSet::DF7, "DefineWindow7", 7),
            _ => (C1CodeSet::RESERVED, "Reserved", 1),
        };
        let name = name.to_owned();
        Self {
            command,
            length,
            name,
        }
    }
}

/// Handle C2 - Code Set - Extended Control Code Set 1
pub fn handle_C2(code: u8) -> u8 {
    match code {
        // ... Single-byte control bytes (0 additional bytes)
        0..=0x07 => 1,
        // ..two-byte control codes (1 additional byte)
        0x08..=0x0F => 2,
        // ..three-byte control codes (2 additional bytes)
        0x10..=0x17 => 3,
        // 18-1F => four-byte control codes (3 additional bytes)
        _ => 4,
    }
}

/// Handle C3 - Code Set - Extended Control Code Set 2
pub fn handle_C3(code: u8, next_code: u8) -> u8 {
    match code {
        // Five-byte control bytes (4 additional bytes)
        0x80..=0x87 => 5,
        // Six-byte control codes (5 additional byte)
        0x88..=0x8F => 6,
        // 90-9F variable length commands
        // Refer Section 7.1.11.2
        _ => {
            // next code is the header which specifies additional bytes
            let length = (next_code & 0x3F) + 1;
            // + 1 for current code
            length + 1
        }
    }
}
