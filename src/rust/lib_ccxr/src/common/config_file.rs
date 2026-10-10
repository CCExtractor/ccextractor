//! Support for `ccextractor.cnf`, an optional file with default settings.
//!
//! Each line has the form `KEY=value`, and everything after a `#` is a comment.
//! The supported keys are described in `docs/ccextractor.cnf.sample`.
//!
//! The settings are applied before the command line is parsed, so command line
//! options override them.

use std::path::PathBuf;

use crate::common::{DataSource, Options, OutputFormat};
use crate::time::units::TimestampFormat;
use crate::util::time::stringztoms;

/// Name of the configuration file, looked up in the current directory.
pub const CONFIG_FILE_NAME: &str = "ccextractor.cnf";

/// A line of the configuration file that could not be applied.
#[derive(Debug, PartialEq, Eq)]
pub struct ConfigWarning {
    /// Line number, starting at 1.
    pub line: usize,
    pub message: String,
}

impl Options {
    /// Apply the settings in `contents`, the text of a configuration file.
    ///
    /// Lines that can't be applied are skipped and returned as warnings.
    pub fn apply_config(&mut self, contents: &str) -> Vec<ConfigWarning> {
        let mut warnings = Vec::new();
        for (index, line) in contents.lines().enumerate() {
            let line = line.split('#').next().unwrap_or_default().trim();
            if line.is_empty() {
                continue;
            }
            let result = match line.split_once('=') {
                Some((key, value)) => self.apply_config_setting(key.trim(), value.trim()),
                None => Err(format!("expected KEY=value, found \"{}\"", line)),
            };
            if let Err(message) = result {
                warnings.push(ConfigWarning {
                    line: index + 1,
                    message,
                });
            }
        }
        warnings
    }

    fn apply_config_setting(&mut self, key: &str, value: &str) -> Result<(), String> {
        if value.is_empty() {
            return Err(format!("no value given for {}", key));
        }
        let int = || parse_int(key, value);
        let flag = || parse_int(key, value).map(|v| v != 0);

        match key {
            "INPUT_SOURCE" => {
                self.input_source = match int()? {
                    0 => DataSource::File,
                    1 => DataSource::Stdin,
                    2 => DataSource::Network,
                    3 => DataSource::Tcp,
                    _ => return Err(out_of_range(key, value)),
                }
            }
            "BUFFER_INPUT" => self.buffer_input = flag()?,
            "NOFONT_COLOR" => self.nofontcolor = flag()?,
            "NOTYPE_SETTING" => self.notypesetting = flag()?,
            "OUTPUT_FORMAT" => {
                self.write_format = match int()? {
                    0 => OutputFormat::Raw,
                    1 => OutputFormat::Srt,
                    2 => OutputFormat::Sami,
                    3 => OutputFormat::Transcript,
                    4 => OutputFormat::Rcwt,
                    5 => OutputFormat::Null,
                    6 => OutputFormat::SmpteTt,
                    7 => OutputFormat::SpuPng,
                    8 => OutputFormat::DvdRaw,
                    9 => OutputFormat::WebVtt,
                    10 => OutputFormat::SimpleXml,
                    11 => OutputFormat::G608,
                    12 => OutputFormat::Curl,
                    13 => OutputFormat::Ssa,
                    14 => OutputFormat::Mcc,
                    15 => OutputFormat::Scc,
                    16 => OutputFormat::Ccd,
                    _ => return Err(out_of_range(key, value)),
                }
            }
            "VIDEO_EDITED" => self.binary_concat = flag()?,
            // Same encoding as ccx_s_options: 1 = GOP, -1 = never GOP, 0 = decide automatically
            "GOP_TIME" => {
                self.use_gop_as_pts = match int()? {
                    1 => Some(true),
                    -1 => Some(false),
                    0 => None,
                    _ => return Err(out_of_range(key, value)),
                }
            }
            "FIX_PADDING" => self.fix_padding = flag()?,
            "GUI_MODE_REPORTS" => self.gui_mode_reports = flag()?,
            "NO_PROGRESS_BAR" => self.no_progress_bar = flag()?,
            "CAP_FILE" => self.sentence_cap_file = PathBuf::from(value),
            "PROFANITY_FILE" => self.filter_profanity_file = PathBuf::from(value),
            "START_AT" => self.extraction_start = Some(parse_time(key, value)?),
            "END_AT" => self.extraction_end = Some(parse_time(key, value)?),
            "INVESTIGATE_PACKET" => self.investigate_packets = flag()?,
            "FULL_BIN" => self.fullbin = flag()?,
            "NO_SYNC" => self.nosync = flag()?,
            "HAUPPAUGE_MODE" => self.hauppauge_mode = flag()?,
            "MP4_VIDEO_TRACK" => self.mp4vidtrack = flag()?,
            "USE_PIC_ORDER" => self.usepicorder = flag()?,
            // 0 = not MythTV, 1 = MythTV, 2 = detect automatically
            "AUTO_MYTH" => {
                self.auto_myth = match int()? {
                    0 => Some(false),
                    1 => Some(true),
                    2 => None,
                    _ => return Err(out_of_range(key, value)),
                }
            }
            "WTV_MPEG2" => self.wtvmpeg2 = flag()?,
            "DATE_FORMAT" => {
                self.date_format = match int()? {
                    0 => TimestampFormat::None,
                    1 => TimestampFormat::HHMMSS,
                    2 => TimestampFormat::Seconds {
                        millis_separator: ',',
                    },
                    3 => TimestampFormat::Date {
                        millis_separator: ',',
                    },
                    4 => TimestampFormat::HHMMSSFFF,
                    _ => return Err(out_of_range(key, value)),
                }
            }
            "NO_ROLL_UP" => self.settings_608.no_rollup = flag()?,
            "FORCED_RU" => self.settings_608.force_rollup = int()?,
            "DIRECT_ROLLUP" => self.settings_608.direct_rollup = int()?,
            "SCREEN_TO_PROCESS" => self.settings_608.screens_to_process = int()?,
            _ => return Err(format!("unknown setting {}", key)),
        }
        Ok(())
    }
}

/// Parse a decimal or `0x`-prefixed hexadecimal integer.
fn parse_int(key: &str, value: &str) -> Result<i32, String> {
    let parsed = match value
        .strip_prefix("0x")
        .or_else(|| value.strip_prefix("0X"))
    {
        Some(hex) => i32::from_str_radix(hex, 16),
        None => value.parse(),
    };
    parsed.map_err(|_| format!("{} expects a number, found \"{}\"", key, value))
}

fn parse_time(key: &str, value: &str) -> Result<crate::time::units::Timestamp, String> {
    stringztoms(value)
        .ok_or_else(|| format!("{} expects SS, MM:SS or HH:MM:SS, found \"{}\"", key, value))
}

fn out_of_range(key: &str, value: &str) -> String {
    format!("{} value {} is out of range", key, value)
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::time::units::Timestamp;

    fn apply(contents: &str) -> (Options, Vec<ConfigWarning>) {
        let mut options = Options::default();
        let warnings = options.apply_config(contents);
        (options, warnings)
    }

    #[test]
    fn test_flags_and_numbers() {
        let (options, warnings) = apply(
            "GUI_MODE_REPORTS=1\nNO_PROGRESS_BAR=1\nOUTPUT_FORMAT=9\nSCREEN_TO_PROCESS=0x10\nFORCED_RU=2\n",
        );
        assert!(warnings.is_empty());
        assert!(options.gui_mode_reports);
        assert!(options.no_progress_bar);
        assert_eq!(options.write_format, OutputFormat::WebVtt);
        assert_eq!(options.settings_608.screens_to_process, 16);
        assert_eq!(options.settings_608.force_rollup, 2);
    }

    #[test]
    fn test_enum_settings() {
        let (options, warnings) =
            apply("INPUT_SOURCE=1\nGOP_TIME=-1\nAUTO_MYTH=0\nDATE_FORMAT=4\n");
        assert!(warnings.is_empty());
        assert_eq!(options.input_source, DataSource::Stdin);
        assert_eq!(options.use_gop_as_pts, Some(false));
        assert_eq!(options.auto_myth, Some(false));
        assert_eq!(options.date_format, TimestampFormat::HHMMSSFFF);
    }

    #[test]
    fn test_times_and_paths() {
        let (options, warnings) =
            apply("START_AT=1:30\nEND_AT=01:02:03\nCAP_FILE=/tmp/words.txt\n");
        assert!(warnings.is_empty());
        assert_eq!(
            options.extraction_start,
            Some(Timestamp::from_millis(90_000))
        );
        assert_eq!(
            options.extraction_end,
            Some(Timestamp::from_millis(3_723_000))
        );
        assert_eq!(options.sentence_cap_file, PathBuf::from("/tmp/words.txt"));
    }

    #[test]
    fn test_comments_whitespace_and_crlf() {
        let (options, warnings) =
            apply("# comment\r\n  NO_SYNC = 1  # trailing comment\r\n\r\nFULL_BIN=1\r\n");
        assert!(warnings.is_empty());
        assert!(options.nosync);
        assert!(options.fullbin);
    }

    #[test]
    fn test_bad_lines_are_reported_and_skipped() {
        let (options, warnings) =
            apply("NO_SYNC=yes\nOUTPUT_FORMAT=99\nBOGUS=1\nFULL_BIN\nSTART_AT=abc\nFIX_PADDING=\nWTV_MPEG2=1\n");
        let lines: Vec<usize> = warnings.iter().map(|w| w.line).collect();
        assert_eq!(lines, vec![1, 2, 3, 4, 5, 6]);
        assert!(!options.nosync);
        assert_eq!(options.write_format, Options::default().write_format);
        assert_eq!(options.extraction_start, None);
        // Valid lines after bad ones still apply
        assert!(options.wtvmpeg2);
    }

    #[test]
    fn test_documented_sample_applies_cleanly() {
        let sample = include_str!("../../../../../docs/ccextractor.cnf.sample");
        let (_, warnings) = apply(sample);
        assert_eq!(warnings, vec![]);
    }
}
