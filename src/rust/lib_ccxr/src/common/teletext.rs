use crate::time::units::{Timestamp, TimestampFormat};
use std::{cell::Cell, fmt};

use super::OutputFormat;

#[derive(Debug)]
pub struct TeletextConfig {
    /// should telxcc logging be verbose?
    pub verbose: bool,
    /// teletext page containing cc we want to filter
    pub page: Cell<TeletextPageNumber>,
    /// Page selected by user, which MIGHT be different to `page` depending on autodetection stuff
    pub user_page: u16,
    /// false = Don't attempt to correct errors
    pub dolevdist: bool,
    /// Means 2 fails or less is "the same"
    pub levdistmincnt: u8,
    /// Means 10% or less is also "the same"
    pub levdistmaxpct: u8,
    /// Segment we actually process
    pub extraction_start: Option<Timestamp>,
    /// Segment we actually process
    pub extraction_end: Option<Timestamp>,
    pub write_format: OutputFormat,
    pub date_format: TimestampFormat,
    /// Do NOT set time automatically?
    pub noautotimeref: bool,
    pub nofontcolor: bool,
    pub nohtmlescape: bool,
    pub latrusmap: bool,
}

impl Default for TeletextConfig {
    fn default() -> Self {
        Self {
            verbose: true,
            page: TeletextPageNumber(0).into(),
            user_page: 0,
            dolevdist: false,
            levdistmincnt: 0,
            levdistmaxpct: 0,
            extraction_start: None,
            extraction_end: None,
            write_format: OutputFormat::default(),
            date_format: TimestampFormat::default(),
            noautotimeref: false,
            nofontcolor: false,
            nohtmlescape: false,
            latrusmap: false,
        }
    }
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
