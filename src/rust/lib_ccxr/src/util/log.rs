//! Provides primitives for logging functionality
//!
//! The interface of this module is highly inspired by the famous log crate of rust.
//!
//! The first step before using any of the logging functionality is to setup a logger. This can be
//! done by creating a [`CCExtractorLogger`] and calling [`set_logger`] with it. To gain access to
//! the instance of [`CCExtractorLogger`], [`logger`] or [`logger_mut`] can be used.
//!
//! There are 4 types of logging messages based on its importance and severity denoted by their
//! respective macros.
//! - [`fatal!`]
//! - [`error!`]
//! - [`info!`]
//! - [`debug!`]
//!
//! Hex dumps can be logged for debugging by [`hex_dump`] and [`hex_dump_with_start_idx`]. Communication
//! with the GUI is possible through [`send_gui`].
//!
//! # Conversion Guide
//!
//! | From                                                                                                                                     | To                          |
//! |------------------------------------------------------------------------------------------------------------------------------------------|-----------------------------|
//! | `EXIT_*`, `CCX_COMMON_EXIT_*`                                                                                                            | [`ExitCause`]               |
//! | `CCX_MESSAGES_*`                                                                                                                         | [`OutputTarget`]            |
//! | `CCX_DMT_*`, `ccx_debug_message_types`                                                                                                   | [`DebugMessageFlag`]        |
//! | `temp_debug`, `ccx_options.debug_mask`, `ccx_options.debug_mask_on_debug`                                                                | [`DebugMessageMask`]        |
//! | `ccx_options.messages_target`, `temp_debug`, `ccx_options.debug_mask`, `ccx_options.debug_mask_on_debug`, `ccx_options.gui_mode_reports` | [`CCExtractorLogger`]       |
//! | `fatal`, `ccx_common_logging.fatal_ftn`                                                                                                  | [`fatal!`]                  |
//! | `mprint`, `ccx_common_logging.log_ftn`                                                                                                   | [`info!`]                   |
//! | `dbg_print`, `ccx_common_logging.debug_ftn`                                                                                              | [`debug!`]                  |
//! | `activity_library_process`, `ccx_common_logging.gui_ftn`                                                                                 | [`send_gui`]                |
//! | `dump`                                                                                                                                   | [`hex_dump`]                |
//! | `dump`                                                                                                                                   | [`hex_dump_with_start_idx`] |

use bitflags::bitflags;
use std::fmt::Arguments;
use std::sync::{OnceLock, RwLock, RwLockReadGuard, RwLockWriteGuard};

static LOGGER: OnceLock<RwLock<CCExtractorLogger>> = OnceLock::new();

/// The possible targets for logging messages.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum OutputTarget {
    Stdout,
    Stderr,
    Quiet,
}

bitflags! {
    /// A bitflag for the types of a Debug Message.
    ///
    /// Each debug message can belong to one or more of these types. The
    /// constants of this struct can be used as bitflags for one message to
    /// belong to more than one type.
    #[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
    pub struct DebugMessageFlag: u16 {
        /// Show information related to parsing the container
        const PARSE          = 0b0000000000000001;
        /// Show video stream related information
        const VIDEO_STREAM   = 0b0000000000000010;
        /// Show GOP and PTS timing information
        const TIME           = 0b0000000000000100;
        /// Show lots of debugging output
        const VERBOSE        = 0b0000000000001000;
        /// Show CC-608 decoder debug
        const DECODER_608    = 0b0000000000010000;
        /// Show CC-708 decoder debug
        const DECODER_708    = 0b0000000000100000;
        /// Show XDS decoder debug
        const DECODER_XDS    = 0b0000000001000000;
        /// Show Caption blocks with FTS timing
        const CB_RAW         = 0b0000000010000000;
        /// Generic, always displayed even if no debug is selected
        const GENERIC_NOTICE = 0b0000000100000000;
        /// Show teletext debug
        const TELETEXT       = 0b0000001000000000;
        /// Show Program Allocation Table dump
        const PAT            = 0b0000010000000000;
        /// Show Program Map Table dump
        const PMT            = 0b0000100000000000;
        /// Show Levenshtein distance calculations
        const LEVENSHTEIN    = 0b0001000000000000;
        /// Show DVB debug
        const DVB            = 0b0010000000000000;
        /// Dump defective TS packets
        const DUMP_DEF       = 0b0100000000000000;
        /// Extracted captions sharing service
        #[cfg(feature = "enable_sharing")]
        const SHARE          = 0b1000000000000000;
    }
}

/// All possible causes for crashing the program instantly. Used in `cause` key of [`fatal!`] macro.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ExitCause {
    Ok,
    Failure,
    NoInputFiles,
    TooManyInputFiles,
    IncompatibleParameters,
    UnableToDetermineFileSize,
    MalformedParameter,
    ReadError,
    NoCaptions,
    WithHelp,
    NotClassified,
    ErrorInCapitalizationFile,
    BufferFull,
    MissingAsfHeader,
    MissingRcwtHeader,

    FileCreationFailed,
    Unsupported,
    NotEnoughMemory,
    Bug,
}

/// A message to be sent to GUI for XDS. Used in [`send_gui`].
pub enum GuiXdsMessage<'a> {
    ProgramName(&'a str),
    ProgramIdNr {
        minute: u8,
        hour: u8,
        date: u8,
        month: u8,
    },
    ProgramDescription {
        line_num: i32,
        desc: &'a str,
    },
    CallLetters(&'a str),
}

/// A mask to filter the debug messages based on its type specified by [`DebugMessageFlag`].
///
/// This operates on one of the two modes: Normal Mode and Debug Mode. The mask used when in Debug Mode is a superset
/// of the mask used when in Normal Mode. One can switch between the two modes by [`DebugMessageMask::set_debug_mode`].
#[derive(Debug)]
pub struct DebugMessageMask {
    debug_mode: bool,
    mask_on_normal: DebugMessageFlag,
    mask_on_debug: DebugMessageFlag,
}

/// A global logger used throughout CCExtractor and stores the settings related to logging.
///
/// A global logger can be setup up initially using [`set_logger`]. Use the following convenience
/// macros for logging: [`fatal!`], [`error!`], [`info!`] and [`debug!`].
#[derive(Debug)]
pub struct CCExtractorLogger {
    target: OutputTarget,
    debug_mask: DebugMessageMask,
    gui_mode: bool,
}

impl DebugMessageMask {
    /// Creates a new [`DebugMessageFlag`] given a mask to be used for Normal Mode and an additional mask to be
    /// used in Debug Mode.
    ///
    /// Note that while in Debug Mode, the mask for Normal Mode will still be valid.
    /// `extra_mask_on_debug` only specifies additional flags to be set on Debug Mode.
    pub const fn new(
        mask_on_normal: DebugMessageFlag,
        extra_mask_on_debug: DebugMessageFlag,
    ) -> DebugMessageMask {
        DebugMessageMask {
            debug_mode: false,
            mask_on_normal,
            mask_on_debug: extra_mask_on_debug.union(mask_on_normal),
        }
    }

    /// Set the mode to Normal or Debug Mode based on `false` or `true` respectively.
    pub fn set_debug_mode(&mut self, mode: bool) {
        self.debug_mode = mode;
    }

    /// Check if the current mode is set to Debug Mode.
    pub fn debug_mode(&self) -> bool {
        self.debug_mode
    }

    /// Return the mask according to its mode.
    pub fn mask(&self) -> DebugMessageFlag {
        if self.debug_mode {
            self.mask_on_debug
        } else {
            self.mask_on_normal
        }
    }
}

impl ExitCause {
    /// Returns the exit code associated with the cause of the error.
    ///
    /// The GUI depends on these exit codes.
    /// Exit code of 0 means OK as usual.
    /// Exit code below 100 means display whatever was output to stderr as a warning.
    /// Exit code above or equal to 100 means display whatever was output to stdout as an error.
    pub fn exit_code(&self) -> i32 {
        match self {
            ExitCause::Ok => 0,
            ExitCause::Failure => 1,
            ExitCause::NoInputFiles => 2,
            ExitCause::TooManyInputFiles => 3,
            ExitCause::IncompatibleParameters => 4,
            ExitCause::UnableToDetermineFileSize => 6,
            ExitCause::MalformedParameter => 7,
            ExitCause::ReadError => 8,
            ExitCause::NoCaptions => 10,
            ExitCause::WithHelp => 11,
            ExitCause::NotClassified => 300,
            ExitCause::ErrorInCapitalizationFile => 501,
            ExitCause::BufferFull => 502,
            ExitCause::MissingAsfHeader => 1001,
            ExitCause::MissingRcwtHeader => 1002,

            ExitCause::FileCreationFailed => 5,
            ExitCause::Unsupported => 9,
            ExitCause::NotEnoughMemory => 500,
            ExitCause::Bug => 1000,
        }
    }
}

impl<'a> CCExtractorLogger {
    /// Returns a new instance of CCExtractorLogger with the provided settings.
    ///
    /// `gui_mode` is used to determine if the log massages are intercepted by a GUI.
    /// `target` specifies the location for printing the log messages.
    /// `debug_mask` is used to filter debug messages based on its type.
    pub const fn new(
        target: OutputTarget,
        debug_mask: DebugMessageMask,
        gui_mode: bool,
    ) -> CCExtractorLogger {
        CCExtractorLogger {
            target,
            debug_mask,
            gui_mode,
        }
    }

    /// Set the mode to Normal or Debug Mode based on `false` or `true` respectively for the
    /// underlying [`DebugMessageMask`].
    ///
    /// This method switches the mask used for filtering debug messages.
    /// Similar to [`DebugMessageMask::set_debug_mode`].
    pub fn set_debug_mode(&mut self, mode: bool) {
        self.debug_mask.set_debug_mode(mode)
    }

    /// Check if the current mode is set to Debug Mode.
    ///
    /// Similar to [`DebugMessageMask::debug_mode`].
    pub fn debug_mode(&self) -> bool {
        self.debug_mask.debug_mode()
    }

    /// Returns the currently set target for logging messages.
    pub fn target(&self) -> OutputTarget {
        self.target
    }

    /// Check if the messages are intercepted by GUI.
    pub fn is_gui_mode(&self) -> bool {
        self.gui_mode
    }

    fn print(&self, args: &Arguments<'a>) {
        match &self.target {
            OutputTarget::Stdout => print!("{}", args),
            OutputTarget::Stderr => eprint!("{}", args),
            OutputTarget::Quiet => {}
        }
    }

    /// Log a fatal error message. Use [`fatal!`] instead.
    ///
    /// Used for logging errors dangerous enough to crash the program instantly.
    pub fn log_fatal(&self, exit_cause: ExitCause, args: &Arguments<'a>) -> ! {
        self.log_error(args);
        println!(); // TODO: print end message
        std::process::exit(exit_cause.exit_code())
    }

    /// Log an error message. Use [`error!`] instead.
    ///
    /// Used for logging general errors occuring in the program.
    pub fn log_error(&self, args: &Arguments<'a>) {
        if self.gui_mode {
            eprint!("###MESSAGE#")
        } else {
            eprint!("\rError: ")
        }

        eprintln!("{}", args);
    }

    /// Log an informational message. Use [`info!`] instead.
    ///
    /// Used for logging extra information about the execution of the program.
    pub fn log_info(&self, args: &Arguments<'a>) {
        // TODO: call activity_header
        self.print(&format_args!("{}", args));
    }

    /// Log a debug message. Use [`debug!`] instead.
    ///
    /// Used for logging debug messages throughout the program.
    pub fn log_debug(&self, message_type: DebugMessageFlag, args: &Arguments<'a>) {
        if self.debug_mask.mask().intersects(message_type) {
            self.print(&format_args!("{}", args));
        }
    }

    /// Send a message to GUI. Use [`send_gui`] instead.
    ///
    /// Used for sending information related to XDS to the GUI.
    pub fn send_gui(&self, _message_type: GuiXdsMessage) {
        todo!()
    }

    /// Log a hex dump which is helpful for debugging purposes.
    /// Use [`hex_dump`] or [`hex_dump_with_start_idx`] instead.
    ///
    /// Setting `clear_high_bit` to true will ignore the highest bit whien displaying the
    /// characters. This makes visual CC inspection easier since the highest bit is usually used
    /// as a parity bit.
    ///
    /// The output will contain byte numbers which can be made to start from any number using
    /// `start_idx`. This is usually `0`.
    pub fn log_hex_dump(
        &self,
        message_type: DebugMessageFlag,
        data: &[u8],
        clear_high_bit: bool,
        start_idx: usize,
    ) {
        if self.debug_mask.mask().intersects(message_type) {
            let chunked_data = data.chunks(16);

            for (id, chunk) in chunked_data.enumerate() {
                self.print(&format_args!("{:05} | ", id * 16 + start_idx));
                for x in chunk {
                    self.print(&format_args!("{:02X} ", x));
                }

                for _ in 0..(16 - chunk.len()) {
                    self.print(&format_args!("   "));
                }

                self.print(&format_args!(" | "));

                for x in chunk {
                    let c = if x >= &b' ' {
                        // 0x7F < remove high bit, convenient for visual CC inspection
                        x & if clear_high_bit { 0x7F } else { 0xFF }
                    } else {
                        b' '
                    };

                    self.print(&format_args!("{}", c as char));
                }

                self.print(&format_args!("\n"));
            }
        }
    }
}

/// Setup the global logger.
///
/// This function can only be called once throught the execution of program. The logger can then be
/// accessed by [`logger`] and [`logger_mut`].
pub fn set_logger(logger: CCExtractorLogger) -> Result<(), CCExtractorLogger> {
    LOGGER
        .set(logger.into())
        .map_err(|x| x.into_inner().unwrap())
}

/// Get an immutable instance of the global logger.
///
/// This function will return [`None`] if the logger is not setup initially by [`set_logger`] or if
/// the underlying RwLock fails to generate a read lock.
///
/// Use [`logger_mut`] to get a mutable instance.
pub fn logger() -> Option<RwLockReadGuard<'static, CCExtractorLogger>> {
    LOGGER.get()?.read().ok()
}

/// Get a mutable instance of the global logger.
///
/// This function will return [`None`] if the logger is not setup initially by [`set_logger`] or if
/// the underlying RwLock fails to generate a write lock.
///
/// Use [`logger`] to get an immutable instance.
pub fn logger_mut() -> Option<RwLockWriteGuard<'static, CCExtractorLogger>> {
    LOGGER.get()?.write().ok()
}

/// Log a fatal error message.
///
/// Used for logging errors dangerous enough to crash the program instantly. This macro does not
/// return (i.e. it returns `!`). A logger needs to be setup initially by [`set_logger`].
///
/// # Usage
/// This macro requires an [`ExitCause`] which provides the appropriate exit codes for shutting
/// down program. This is provided using a key called `cause` which comes before the `;`. After
/// `;`, the arguments works the same as a [`println!`] macro.
///
/// # Examples
/// ```no_run
/// # use lib_ccxr::util::log::*;
/// # let actual = 2;
/// # let required = 1;
/// fatal!(
///     cause = ExitCause::TooManyInputFiles;
///     "{} input files were provided but only {} were needed", actual, required
/// );
/// ```
#[macro_export]
macro_rules! fatal {
    (cause = $exit_cause:expr; $($args:expr),*) => {
        $crate::util::log::logger().expect("Logger is not yet initialized")
            .log_fatal($exit_cause, &format_args!($($args),*))
    };
}

/// Log an error message.
///
/// Used for logging general errors occuring in the program. A logger needs to be setup
/// initially by [`set_logger`].
///
/// # Usage
/// The arguments works the same as a [`println!`] macro.
///
/// # Examples
/// ```no_run
/// # use lib_ccxr::util::log::*;
/// # let missing_blocks = 2;
/// error!("missing {} additional blocks", missing_blocks);
/// ```
#[macro_export]
macro_rules! error {
    ($($args:expr),*) => {
        $crate::util::log::logger().expect("Logger is not yet initialized")
            .log_error(&format_args!($($args),*))
    }
}

/// Log an informational message.
///
/// Used for logging extra information about the execution of the program. A logger needs to be
/// setup initially by [`set_logger`].
///
/// # Usage
/// The arguments works the same as a [`println!`] macro.
///
/// # Examples
/// ```no_run
/// # use lib_ccxr::util::log::*;
/// info!("Processing the header section");
/// ```
#[macro_export]
macro_rules! info {
    ($($args:expr),*) => {
        $crate::util::log::logger().expect("Logger is not yet initialized")
            .log_info(&format_args!($($args),*))
    };
}

/// Log a debug message.
///
/// Used for logging debug messages throughout the program. A logger needs to be setup initially
/// by [`set_logger`].
///
/// # Usage
/// This macro requires an [`DebugMessageFlag`] which represents the type of debug message. It is
/// used for filtering the messages. This is provided using a key called `msg_type` which comes
/// before the `;`. After `;`, the arguments works the same as a [`println!`] macro.
///
/// # Examples
/// ```no_run
/// # use lib_ccxr::util::log::*;
/// # let byte1 = 23u8;
/// # let byte2 = 45u8;
/// debug!(
///     msg_type = DebugMessageFlag::DECODER_708;
///     "Packet Start with contents {} {}", byte1, byte2
/// );
/// ```
#[macro_export]
macro_rules! debug {
    (msg_type = $msg_flag:expr; $($args:expr),*) => {
        $crate::util::log::logger().expect("Logger is not yet initialized")
            .log_debug($msg_flag, &format_args!($($args),*))
    };
}

pub use debug;
pub use error;
pub use fatal;
pub use info;

/// Log a hex dump which is helpful for debugging purposes.
///
/// Setting `clear_high_bit` to true will ignore the highest bit whien displaying the
/// characters. This makes visual CC inspection easier since the highest bit is usually used
/// as a parity bit.
///
/// The byte numbers start from `0` by default. Use [`hex_dump_with_start_idx`] if a
/// different starting index is required.
pub fn hex_dump(message_type: DebugMessageFlag, data: &[u8], clear_high_bit: bool) {
    logger()
        .expect("Logger is not yet initialized")
        .log_hex_dump(message_type, data, clear_high_bit, 0)
}

/// Log a hex dump which is helpful for debugging purposes.
///
/// Setting `clear_high_bit` to true will ignore the highest bit whien displaying the
/// characters. This makes visual CC inspection easier since the highest bit is usually used
/// as a parity bit.
///
/// The output will contain byte numbers which can be made to start from any number using
/// `start_idx`. This is usually `0`.
pub fn hex_dump_with_start_idx(
    message_type: DebugMessageFlag,
    data: &[u8],
    clear_high_bit: bool,
    start_idx: usize,
) {
    logger()
        .expect("Logger is not yet initialized")
        .log_hex_dump(message_type, data, clear_high_bit, start_idx)
}

/// Send a message to GUI.
///
/// Used for sending information related to XDS to the GUI.
pub fn send_gui(message: GuiXdsMessage) {
    logger()
        .expect("Logger is not yet initialized")
        .send_gui(message)
}
