//! Provides types to represent different kinds of subtitle data in a unified format.
//!
//! NOTE: This module is incomplete and a lot of work is still left.

use crate::common::Language;
use crate::util::encoding::EncodedString;
use crate::util::time::Timestamp;

/// Represents the different formats in which subtitle data could be stored.
///
/// NOTE: Heavy Work in Progress.
pub enum SubtitleData {
    Dvb {
        /* bitmap: Bitmap, */
        lang: Language,
        is_eod: bool,
        time_out: Timestamp,
    },
    Dvd {
        /* bitmap: Bitmap, */
        lang: Language,
    },
    Xds(/* XdsScreen */),
    Eia608(/* Eia608Screen */),
    Text(EncodedString),
    Raw(Vec<u8>),
}

/// Represents a single subtitle instance on a screen with timing info.
pub struct Subtitle {
    /// The subtitle data.
    data: SubtitleData,

    /// The start time for this subtitle.
    start_time: Timestamp,

    /// The end time of this subtitle.
    end_time: Timestamp,

    /// A flag to tell that decoder has given output.
    got_output: bool,
    info: Option<String>,
    mode: String,
}

impl Subtitle {
    /// Create a new Text Subtitle.
    pub fn new_text(
        string: EncodedString,
        start_time: Timestamp,
        end_time: Timestamp,
        info: Option<String>,
        mode: String,
    ) -> Subtitle {
        Subtitle {
            data: SubtitleData::Text(string),
            start_time,
            end_time,
            got_output: true,
            info,
            mode,
        }
    }

    /// Return a reference to the subtitle data.
    pub fn data(&self) -> &SubtitleData {
        &self.data
    }

    /// Return the start time of this subtitle.
    pub fn start_time(&self) -> Timestamp {
        self.start_time
    }

    /// Return the end time of this subtitle.
    pub fn end_time(&self) -> Timestamp {
        self.end_time
    }

    /// Check if decoder has given output.
    pub fn got_output(&self) -> bool {
        self.got_output
    }

    /// Update the state if decoder has given output.
    pub fn set_got_output(&mut self, val: bool) {
        self.got_output = val;
    }

    pub fn info(&self) -> Option<&str> {
        self.info.as_deref()
    }

    pub fn mode(&self) -> &str {
        &self.mode
    }
}
