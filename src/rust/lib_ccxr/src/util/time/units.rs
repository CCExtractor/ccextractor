use derive_more::{Add, Neg, Sub};
use std::convert::TryInto;
use std::fmt::Write;
use std::num::TryFromIntError;
use std::time::{SystemTime, UNIX_EPOCH};
use thiserror::Error;
use time::macros::{datetime, format_description};
use time::{error::Format, Duration};

/// Represents a timestamp in milliseconds.
///
/// The number can be negetive.
#[derive(Copy, Clone, Debug, Eq, PartialEq, Ord, PartialOrd, Add, Sub, Neg)]
pub struct Timestamp {
    millis: i64,
}

/// Represents an error during operations on [`Timestamp`].
#[derive(Error, Debug)]
pub enum TimestampError {
    #[error("input parameter given is out of range")]
    InputOutOfRangeError,
    #[error("timestamp is out of range")]
    OutOfRangeError(#[from] TryFromIntError),
    #[error("error ocurred during formatting")]
    FormattingError(#[from] std::fmt::Error),
    #[error("error ocurred during formatting a date")]
    DateFormattingError(#[from] Format),
    #[error("error ocurred during parsing")]
    ParsingError,
}

/// Represents the different string formats for [`Timestamp`].
pub enum TimestampFormat {
    /// Format: blank string.
    ///
    /// # Examples
    /// ```rust
    /// # use lib_ccxr::util::time::{Timestamp, TimestampFormat};
    /// let timestamp = Timestamp::from_millis(6524365);
    /// let mut output = String::new();
    /// timestamp.write_formatted_time(&mut output, TimestampFormat::None);
    /// assert_eq!(output, "");
    /// ```
    None,

    /// Format: `{hour:02}:{minute:02}:{second:02}`.
    ///
    /// # Examples
    /// ```rust
    /// # use lib_ccxr::util::time::{Timestamp, TimestampFormat};
    /// let timestamp = Timestamp::from_millis(6524365);
    /// let mut output = String::new();
    /// timestamp.write_formatted_time(&mut output, TimestampFormat::HHMMSS);
    /// assert_eq!(output, "01:48:44");
    /// ```
    HHMMSS,

    /// Format: `{second:02}{millis_separator}{millis:03}`.
    ///
    /// # Examples
    /// ```rust
    /// # use lib_ccxr::util::time::{Timestamp, TimestampFormat};
    /// let timestamp = Timestamp::from_millis(6524365);
    /// let mut output = String::new();
    /// timestamp.write_formatted_time(
    ///     &mut output,
    ///     TimestampFormat::Seconds {
    ///         millis_separator: ',',
    ///     },
    /// );
    /// assert_eq!(output, "6524,365");
    /// ```
    Seconds { millis_separator: char },

    /// Format:
    /// `{year:04}{month:02}{day:02}{hour:02}{minute:02}{second:02}{millis_separator}{millis:03}`.
    ///
    /// # Examples
    /// ```rust
    /// # use lib_ccxr::util::time::{Timestamp, TimestampFormat};
    /// // 11 March 2023 14:53:36.749 in UNIX timestamp.
    /// let timestamp = Timestamp::from_millis(1678546416749);
    /// let mut output = String::new();
    /// timestamp.write_formatted_time(
    ///     &mut output,
    ///     TimestampFormat::Date {
    ///         millis_separator: ',',
    ///     },
    /// );
    /// assert_eq!(output, "20230311145336,749");
    /// ```
    Date { millis_separator: char },

    /// Format: `{hour:02}:{minute:02}:{second:02},{millis:03}`.
    ///
    /// # Examples
    /// ```rust
    /// # use lib_ccxr::util::time::{Timestamp, TimestampFormat};
    /// let timestamp = Timestamp::from_millis(6524365);
    /// let mut output = String::new();
    /// timestamp.write_formatted_time(&mut output, TimestampFormat::HHMMSSFFF);
    /// assert_eq!(output, "01:48:44,365");
    /// ```
    HHMMSSFFF,
}

impl Timestamp {
    /// Create a new [`Timestamp`] based on the number of milliseconds since the Unix Epoch.
    pub fn now() -> Timestamp {
        let duration = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .expect("System Time cannot be behind the Unix Epoch");

        Timestamp {
            millis: duration.as_millis() as i64,
        }
    }

    /// Create a new [`Timestamp`] from number of milliseconds.
    pub const fn from_millis(millis: i64) -> Timestamp {
        Timestamp { millis }
    }

    /// Create a new [`Timestamp`] from hours, minutes, seconds and milliseconds.
    ///
    /// It will fail if any parameter doesn't follow their respective ranges:
    ///
    /// | Parameter | Range   |
    /// |-----------|---------|
    /// | minutes   | 0 - 59  |
    /// | seconds   | 0 - 59  |
    /// | millis    | 0 - 999 |
    pub fn from_hms_millis(
        hours: u8,
        minutes: u8,
        seconds: u8,
        millis: u16,
    ) -> Result<Timestamp, TimestampError> {
        if minutes < 60 && seconds < 60 && millis < 1000 {
            Ok(Timestamp::from_millis(
                (hours as i64) * 3_600_000
                    + (minutes as i64) * 60_000
                    + (seconds as i64) * 1000
                    + millis as i64,
            ))
        } else {
            Err(TimestampError::InputOutOfRangeError)
        }
    }

    /// Returns the number of milliseconds.
    ///
    /// # Examples
    /// ```rust
    /// # use lib_ccxr::util::time::Timestamp;
    /// let timestamp = Timestamp::from_millis(6524365);
    /// assert_eq!(timestamp.millis(), 6524365);
    /// ```
    pub fn millis(&self) -> i64 {
        self.millis
    }

    /// Returns the number of whole seconds.
    ///
    /// # Examples
    /// ```rust
    /// # use lib_ccxr::util::time::Timestamp;
    /// let timestamp = Timestamp::from_millis(6524365);
    /// assert_eq!(timestamp.seconds(), 6524);
    /// ```
    pub fn seconds(&self) -> i64 {
        self.millis / 1000
    }

    /// Returns the number of whole seconds and leftover milliseconds as unsigned integers.
    ///
    /// It will return an [`TimestampError::OutOfRangeError`] if the timestamp is negetive.
    ///
    /// # Examples
    /// ```rust
    /// # use lib_ccxr::util::time::Timestamp;
    /// let timestamp = Timestamp::from_millis(6524365);
    /// assert_eq!(timestamp.as_sec_millis().unwrap(), (6524, 365));
    /// ```
    pub fn as_sec_millis(&self) -> Result<(u64, u16), TimestampError> {
        let millis: u64 = self.millis.try_into()?;
        let s = millis / 1000;
        let u = millis % 1000;
        Ok((s, u as u16))
    }

    /// Returns the time in the form of hours, minutes, seconds and milliseconds as unsigned
    /// integers.
    ///
    /// It will return an [`TimestampError::OutOfRangeError`] if the timestamp is negetive.
    ///
    /// # Examples
    /// ```rust
    /// # use lib_ccxr::util::time::Timestamp;
    /// let timestamp = Timestamp::from_millis(6524365);
    /// assert_eq!(timestamp.as_hms_millis().unwrap(), (1, 48, 44, 365));
    /// ```
    /// ```rust
    /// # use lib_ccxr::util::time::{Timestamp, TimestampError};
    /// let timestamp = Timestamp::from_millis(1678546416749);
    /// assert!(matches!(
    ///     timestamp.as_hms_millis().unwrap_err(),
    ///     TimestampError::OutOfRangeError(_)
    /// ));
    /// ```
    pub fn as_hms_millis(&self) -> Result<(u8, u8, u8, u16), TimestampError> {
        let millis: u64 = self.millis.try_into()?;
        let h = millis / 3600000;
        let m = millis / 60000 - 60 * h;
        let s = millis / 1000 - 3600 * h - 60 * m;
        let u = millis - 3600000 * h - 60000 * m - 1000 * s;
        if h > 24 {
            println!("{}", h)
        }
        Ok((h.try_into()?, m as u8, s as u8, u as u16))
    }

    /// Fills `output` with the [`Timestamp`] using SRT's timestamp format.
    ///
    /// # Examples
    /// ```rust
    /// # use lib_ccxr::util::time::Timestamp;
    /// let timestamp = Timestamp::from_millis(6524365);
    /// let mut output = String::new();
    /// timestamp.write_srt_time(&mut output);
    /// assert_eq!(output, "01:48:44,365");
    /// ```
    pub fn write_srt_time(&self, output: &mut String) -> Result<(), TimestampError> {
        let (h, m, s, u) = self.as_hms_millis()?;
        write!(output, "{:02}:{:02}:{:02},{:03}", h, m, s, u)?;
        Ok(())
    }

    /// Fills `output` with the [`Timestamp`] using VTT's timestamp format.
    ///
    /// # Examples
    /// ```rust
    /// # use lib_ccxr::util::time::Timestamp;
    /// let timestamp = Timestamp::from_millis(6524365);
    /// let mut output = String::new();
    /// timestamp.write_vtt_time(&mut output);
    /// assert_eq!(output, "01:48:44.365");
    /// ```
    pub fn write_vtt_time(&self, output: &mut String) -> Result<(), TimestampError> {
        let (h, m, s, u) = self.as_hms_millis()?;
        write!(output, "{:02}:{:02}:{:02}.{:03}", h, m, s, u)?;
        Ok(())
    }

    /// Fills `output` with the [`Timestamp`] using
    /// "{sign}{hour:02}:{minute:02}:{second:02}{sep}{millis:03}" format, where `sign` can be `-`
    /// if time is negetive or blank if it is positive.
    ///
    /// # Examples
    /// ```rust
    /// # use lib_ccxr::util::time::Timestamp;
    /// let timestamp = Timestamp::from_millis(6524365);
    /// let mut output = String::new();
    /// timestamp.write_hms_millis_time(&mut output, ':');
    /// assert_eq!(output, "01:48:44:365");
    /// ```
    pub fn write_hms_millis_time(
        &self,
        output: &mut String,
        sep: char,
    ) -> Result<(), TimestampError> {
        let sign = if self.millis < 0 { "-" } else { "" };
        let timestamp = if self.millis < 0 { -*self } else { *self };
        let (h, m, s, u) = timestamp.as_hms_millis()?;
        write!(output, "{}{:02}:{:02}:{:02}{}{:03}", sign, h, m, s, sep, u)?;
        Ok(())
    }

    /// Fills `output` with the [`Timestamp`] using ctime's format.
    ///
    /// # Examples
    /// ```rust
    /// # use lib_ccxr::util::time::Timestamp;
    /// let timestamp = Timestamp::from_millis(6524365);
    /// let mut output = String::new();
    /// timestamp.write_ctime(&mut output);
    /// assert_eq!(output, "Thu Jan 01 01:48:44 1970");
    /// ```
    pub fn write_ctime(&self, output: &mut String) -> Result<(), TimestampError> {
        let (sec, millis) = self.as_sec_millis()?;
        let d = datetime!(1970-01-01 0:00)
            + Duration::new(sec.try_into()?, (millis as i32) * 1_000_000);
        let format = format_description!(
            "[weekday repr:short] [month repr:short] [day] [hour]:[minute]:[second] [year]"
        );
        write!(output, "{}", d.format(&format)?)?;
        Ok(())
    }

    /// Fills `output` with the [`Timestamp`] using format specified by [`TimestampFormat`].
    ///
    /// See [`TimestampFormat`] for examples.
    pub fn write_formatted_time(
        &self,
        output: &mut String,
        format: TimestampFormat,
    ) -> Result<(), TimestampError> {
        match format {
            TimestampFormat::None => Ok(()),
            TimestampFormat::HHMMSS => {
                let (h, m, s, _) = self.as_hms_millis()?;
                write!(output, "{:02}:{:02}:{:02}", h, m, s)?;
                Ok(())
            }
            TimestampFormat::Seconds { millis_separator } => {
                let (sec, millis) = self.as_sec_millis()?;
                write!(output, "{}{}{:03}", sec, millis_separator, millis)?;
                Ok(())
            }
            TimestampFormat::Date { millis_separator } => {
                let (sec, millis) = self.as_sec_millis()?;
                let d = datetime!(1970-01-01 0:00)
                    + Duration::new(sec.try_into()?, (millis as i32) * 1_000_000);
                let format1 = format_description!("[year][month][day][hour][minute][second]");
                let format2 = format_description!("[subsecond digits:3]");

                write!(
                    output,
                    "{}{}{}",
                    d.format(&format1)?,
                    millis_separator,
                    d.format(&format2)?
                )?;
                Ok(())
            }
            TimestampFormat::HHMMSSFFF => self.write_srt_time(output),
        }
    }

    pub fn to_srt_time(&self) -> Result<String, TimestampError> {
        let mut s = String::new();
        self.write_srt_time(&mut s)?;
        Ok(s)
    }

    pub fn to_vtt_time(&self) -> Result<String, TimestampError> {
        let mut s = String::new();
        self.write_vtt_time(&mut s)?;
        Ok(s)
    }

    pub fn to_hms_millis_time(&self, sep: char) -> Result<String, TimestampError> {
        let mut s = String::new();
        self.write_hms_millis_time(&mut s, sep)?;
        Ok(s)
    }

    pub fn to_ctime(&self) -> Result<String, TimestampError> {
        let mut s = String::new();
        self.write_ctime(&mut s)?;
        Ok(s)
    }

    pub fn to_formatted_time(&self, format: TimestampFormat) -> Result<String, TimestampError> {
        let mut s = String::new();
        self.write_formatted_time(&mut s, format)?;
        Ok(s)
    }

    /// Creates a [`Timestamp`] by parsing `input` using format `SS` or `MM:SS` or `HH:MM:SS`.
    ///
    /// # Examples
    /// ```rust
    /// # use lib_ccxr::util::time::Timestamp;
    /// let timestamp = Timestamp::parse_optional_hhmmss_from_str("01:12:45").unwrap();
    /// assert_eq!(timestamp, Timestamp::from_millis(4_365_000));
    /// ```
    pub fn parse_optional_hhmmss_from_str(input: &str) -> Result<Timestamp, TimestampError> {
        let mut numbers = input
            .split(':')
            .map(|x| x.parse::<u8>().map_err(|_| TimestampError::ParsingError))
            .rev();

        let mut millis: u64 = 0;

        let seconds: u64 = numbers.next().ok_or(TimestampError::ParsingError)??.into();
        if seconds > 59 {
            return Err(TimestampError::InputOutOfRangeError);
        }
        millis += seconds * 1000;

        if let Some(x) = numbers.next() {
            let minutes: u64 = x?.into();
            if minutes > 59 {
                return Err(TimestampError::InputOutOfRangeError);
            }
            millis += 60_000 * minutes;
        }

        if let Some(x) = numbers.next() {
            let hours: u64 = x?.into();
            millis += 3_600_000 * hours;
        }

        if numbers.next().is_some() {
            return Err(TimestampError::ParsingError);
        }

        Ok(Timestamp::from_millis(millis.try_into()?))
    }
}

/// Represent the number of clock ticks as defined in Mpeg standard.
///
/// This number can never be negetive.
#[derive(Copy, Clone, Debug, Eq, PartialEq, Ord, PartialOrd, Add, Sub)]
pub struct MpegClockTick(i64);

impl MpegClockTick {
    /// The ratio to convert a clock tick to time duration.
    pub const MPEG_CLOCK_FREQ: i64 = 90000;

    /// Create a value representing `ticks` clock ticks.
    pub fn new(ticks: i64) -> MpegClockTick {
        MpegClockTick(ticks)
    }

    /// Returns the number of clock ticks.
    pub fn as_i64(&self) -> i64 {
        self.0
    }

    /// Converts the clock ticks to its equivalent time duration.
    ///
    /// The conversion ratio used is [`MpegClockTick::MPEG_CLOCK_FREQ`].
    pub fn as_timestamp(&self) -> Timestamp {
        Timestamp::from_millis(self.0 / (MpegClockTick::MPEG_CLOCK_FREQ / 1000))
    }
}

/// Represents the number of frames.
///
/// This number can never be negetive.
#[derive(Copy, Clone, Debug, Eq, PartialEq, Ord, PartialOrd, Add, Sub)]
pub struct FrameCount(u64);

impl FrameCount {
    /// Create a value representing `frames` number of frames.
    pub const fn new(frames: u64) -> FrameCount {
        FrameCount(frames)
    }

    /// Returns the number of frames.
    pub fn as_u64(&self) -> u64 {
        self.0
    }

    /// Converts the frames to its equivalent time duration.
    ///
    /// The conversion ratio used is `fps`.
    pub fn as_timestamp(&self, fps: f64) -> Timestamp {
        Timestamp::from_millis((self.0 as f64 * 1000.0 / fps) as i64)
    }

    /// Converts the frames to its equivalent number of clock ticks.
    ///
    /// The conversion ratio used is [`MpegClockTick::MPEG_CLOCK_FREQ`] and `fps`.
    pub fn as_mpeg_clock_tick(&self, fps: f64) -> MpegClockTick {
        MpegClockTick::new(((self.0 * MpegClockTick::MPEG_CLOCK_FREQ as u64) as f64 / fps) as i64)
    }
}

/// Represents a GOP Time code as defined in the Mpeg standard.
///
/// This structure stores its time in the form of hours, minutes, seconds and pictures. This
/// structure also stores its time in the form of a [`Timestamp`] when it is created. This
/// [`Timestamp`] can be modified by [`timestamp_mut`](GopTimeCode::timestamp_mut) and an
/// additional 24 hours may be added on rollover, so it is not necessary that the above two
/// formats refer to the same time. Therefore it is recommended to only rely on the
/// [`Timestamp`] instead of the other format.
#[derive(Copy, Clone, Debug)]
pub struct GopTimeCode {
    drop_frame: bool,
    time_code_hours: u8,
    time_code_minutes: u8,
    time_code_seconds: u8,
    time_code_pictures: u8,
    timestamp: Timestamp,
}

impl GopTimeCode {
    /// Create a new [`GopTimeCode`] from the specified parameters.
    ///
    /// The number of frames or pictures is converted to time duration using `fps`.
    ///
    /// If `rollover` is true, then an extra of 24 hours will added.
    ///
    /// It will return [`None`] if any parameter doesn't follow their respective ranges:
    ///
    /// | Parameter | Range  |
    /// |-----------|--------|
    /// | hours     | 0 - 23 |
    /// | minutes   | 0 - 59 |
    /// | seconds   | 0 - 59 |
    /// | pictures  | 0 - 59 |
    pub fn new(
        drop_frame: bool,
        hours: u8,
        minutes: u8,
        seconds: u8,
        pictures: u8,
        fps: f64,
        rollover: bool,
    ) -> Option<GopTimeCode> {
        if hours < 24 && minutes < 60 && seconds < 60 && pictures < 60 {
            let millis = (1000.0 * (pictures as f64) / fps) as u16;
            let extra_hours = if rollover { 24 } else { 0 };
            let timestamp =
                Timestamp::from_hms_millis(hours + extra_hours, minutes, seconds, millis)
                    .expect("The fps given is probably too low");

            Some(GopTimeCode {
                drop_frame,
                time_code_hours: hours,
                time_code_minutes: minutes,
                time_code_seconds: seconds,
                time_code_pictures: pictures,
                timestamp,
            })
        } else {
            None
        }
    }

    /// Returns the GOP time code in its equivalent time duration.
    pub fn timestamp(&self) -> Timestamp {
        self.timestamp
    }

    /// Returns a mutable reference to internal [`Timestamp`].
    pub fn timestamp_mut(&mut self) -> &mut Timestamp {
        &mut self.timestamp
    }

    /// Check if a rollover has ocurred by comparing the previous [`GopTimeCode`] that is `prev`
    /// with the current [`GopTimeCode`].
    pub fn did_rollover(&self, prev: &GopTimeCode) -> bool {
        prev.time_code_hours == 23
            && prev.time_code_minutes == 59
            && self.time_code_hours == 0
            && self.time_code_minutes == 0
    }

    /// Constructs a [`GopTimeCode`] from its individual fields.
    ///
    /// # Safety
    ///
    /// The fields other than [`Timestamp`] may not be accurate if it is changed using
    /// [`timestamp_mut`](GopTimeCode::timestamp_mut).
    pub unsafe fn from_raw_parts(
        drop_frame: bool,
        hours: u8,
        minutes: u8,
        seconds: u8,
        pictures: u8,
        timestamp: Timestamp,
    ) -> GopTimeCode {
        GopTimeCode {
            drop_frame,
            time_code_hours: hours,
            time_code_minutes: minutes,
            time_code_seconds: seconds,
            time_code_pictures: pictures,
            timestamp,
        }
    }

    /// Returns the individuals field of a [`GopTimeCode`].
    ///
    /// # Safety
    ///
    /// The fields other than [`Timestamp`] may not be accurate if it is changed using
    /// [`timestamp_mut`](GopTimeCode::timestamp_mut).
    pub unsafe fn as_raw_parts(&self) -> (bool, u8, u8, u8, u8, Timestamp) {
        let GopTimeCode {
            drop_frame,
            time_code_hours,
            time_code_minutes,
            time_code_seconds,
            time_code_pictures,
            timestamp,
        } = *self;

        (
            drop_frame,
            time_code_hours,
            time_code_minutes,
            time_code_seconds,
            time_code_pictures,
            timestamp,
        )
    }
}
