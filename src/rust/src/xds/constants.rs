//! XDS constants, enums, and static data
//!
//! # Conversion Guide
//!
//! | C (ccx_decoders_xds.c/.h)       | Rust (constants.rs)                              |
//! |----------------------------------|--------------------------------------------------|
//! | `XDS_CLASS_*` defines            | [`XdsClass`] enum                                |
//! | `XDS_TYPE_*` defines             | [`XdsType`], [`XdsCurrentFutureType`], etc.      |
//! | `xds_class` (int field)          | [`XdsClass::from_c_int`], [`XdsClass::to_c_int`] |
//! | `xds_type` (int field)           | [`XdsType::from_c_int`], [`XdsType::to_c_int`]   |
//! | `xds_program_type[]` array       | [`XDS_PROGRAM_TYPES`]                            |
//! | `xds_classes[]` array            | [`XDS_CLASSES`]                                  |

use std::os::raw::c_int;

pub const NUM_BYTES_PER_PACKET: usize = 35; // Class + type (repeated for convenience) + data + zero
pub const NUM_XDS_BUFFERS: usize = 9; // CEA recommends no more than one level of interleaving. Play it safe

pub static XDS_CLASSES: [&str; 8] = [
    "Current",
    "Future",
    "Channel",
    "Miscellaneous",
    "Public service",
    "Reserved",
    "Private data",
    "End",
];

pub static XDS_PROGRAM_TYPES: [&str; 96] = [
    "Education",
    "Entertainment",
    "Movie",
    "News",
    "Religious",
    "Sports",
    "Other",
    "Action",
    "Advertisement",
    "Animated",
    "Anthology",
    "Automobile",
    "Awards",
    "Baseball",
    "Basketball",
    "Bulletin",
    "Business",
    "Classical",
    "College",
    "Combat",
    "Comedy",
    "Commentary",
    "Concert",
    "Consumer",
    "Contemporary",
    "Crime",
    "Dance",
    "Documentary",
    "Drama",
    "Elementary",
    "Erotica",
    "Exercise",
    "Fantasy",
    "Farm",
    "Fashion",
    "Fiction",
    "Food",
    "Football",
    "Foreign",
    "Fund-Raiser",
    "Game/Quiz",
    "Garden",
    "Golf",
    "Government",
    "Health",
    "High_School",
    "History",
    "Hobby",
    "Hockey",
    "Home",
    "Horror",
    "Information",
    "Instruction",
    "International",
    "Interview",
    "Language",
    "Legal",
    "Live",
    "Local",
    "Math",
    "Medical",
    "Meeting",
    "Military",
    "Mini-Series",
    "Music",
    "Mystery",
    "National",
    "Nature",
    "Police",
    "Politics",
    "Premiere",
    "Pre-Recorded",
    "Product",
    "Professional",
    "Public",
    "Racing",
    "Reading",
    "Repair",
    "Repeat",
    "Review",
    "Romance",
    "Science",
    "Series",
    "Service",
    "Shopping",
    "Soap_Opera",
    "Special",
    "Suspense",
    "Talk",
    "Technical",
    "Tennis",
    "Travel",
    "Variety",
    "Video",
    "Weather",
    "Western",
];

#[derive(Debug, Clone, Copy, PartialEq)]
#[repr(u8)]
pub enum XdsClass {
    Current = 0,
    Future = 1,
    Channel = 2,
    Misc = 3,
    Public = 4,
    Reserved = 5,
    Private = 6,
    End = 7,
    OutOfBand = 0x40,
}

impl XdsClass {
    pub fn from_c_int(value: c_int) -> Option<Self> {
        match value {
            0 => Some(XdsClass::Current),
            1 => Some(XdsClass::Future),
            2 => Some(XdsClass::Channel),
            3 => Some(XdsClass::Misc),
            4 => Some(XdsClass::Public),
            5 => Some(XdsClass::Reserved),
            6 => Some(XdsClass::Private),
            7 => Some(XdsClass::End),
            0x40 => Some(XdsClass::OutOfBand),
            _ => None,
        }
    }

    pub fn to_c_int(&self) -> c_int {
        match self {
            XdsClass::Current => 0,
            XdsClass::Future => 1,
            XdsClass::Channel => 2,
            XdsClass::Misc => 3,
            XdsClass::Public => 4,
            XdsClass::Reserved => 5,
            XdsClass::Private => 6,
            XdsClass::End => 7,
            XdsClass::OutOfBand => 0x40,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
#[repr(u8)]
pub enum XdsCurrentFutureType {
    PinStartTime = 1,
    LengthAndCurrentTime = 2,
    ProgramName = 3,
    ProgramType = 4,
    ContentAdvisory = 5,
    AudioServices = 6,
    Cgms = 8,
    AspectRatioInfo = 9,
    ProgramDesc1 = 0x10,
    ProgramDesc2 = 0x11,
    ProgramDesc3 = 0x12,
    ProgramDesc4 = 0x13,
    ProgramDesc5 = 0x14,
    ProgramDesc6 = 0x15,
    ProgramDesc7 = 0x16,
    ProgramDesc8 = 0x17,
}

#[derive(Debug, Clone, Copy, PartialEq)]
#[repr(u8)]
pub enum XdsChannelType {
    NetworkName = 1,
    CallLettersAndChannel = 2,
    Tsid = 4,
}

#[derive(Debug, Clone, Copy, PartialEq)]
#[repr(u8)]
pub enum XdsMiscType {
    TimeOfDay = 1,
    LocalTimeZone = 4,
    OutOfBandChannelNumber = 0x40,
}

#[derive(Debug, Clone, Copy, PartialEq)]
#[repr(u8)]
pub enum XdsType {
    CurrentFuture(XdsCurrentFutureType),
    Misc(XdsMiscType),
    Channel(XdsChannelType),
}

impl XdsType {
    pub fn from_c_int(class: Option<XdsClass>, type_value: c_int) -> Option<Self> {
        match class? {
            XdsClass::Current | XdsClass::Future => match type_value {
                1 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::PinStartTime)),
                2 => Some(XdsType::CurrentFuture(
                    XdsCurrentFutureType::LengthAndCurrentTime,
                )),
                3 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::ProgramName)),
                4 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::ProgramType)),
                5 => Some(XdsType::CurrentFuture(
                    XdsCurrentFutureType::ContentAdvisory,
                )),
                6 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::AudioServices)),
                8 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::Cgms)),
                9 => Some(XdsType::CurrentFuture(
                    XdsCurrentFutureType::AspectRatioInfo,
                )),
                0x10 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::ProgramDesc1)),
                0x11 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::ProgramDesc2)),
                0x12 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::ProgramDesc3)),
                0x13 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::ProgramDesc4)),
                0x14 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::ProgramDesc5)),
                0x15 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::ProgramDesc6)),
                0x16 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::ProgramDesc7)),
                0x17 => Some(XdsType::CurrentFuture(XdsCurrentFutureType::ProgramDesc8)),
                _ => None,
            },
            XdsClass::Channel => match type_value {
                1 => Some(XdsType::Channel(XdsChannelType::NetworkName)),
                2 => Some(XdsType::Channel(XdsChannelType::CallLettersAndChannel)),
                4 => Some(XdsType::Channel(XdsChannelType::Tsid)),
                _ => None,
            },
            XdsClass::Misc => match type_value {
                1 => Some(XdsType::Misc(XdsMiscType::TimeOfDay)),
                4 => Some(XdsType::Misc(XdsMiscType::LocalTimeZone)),
                0x40 => Some(XdsType::Misc(XdsMiscType::OutOfBandChannelNumber)),
                _ => None,
            },
            XdsClass::Public
            | XdsClass::Reserved
            | XdsClass::Private
            | XdsClass::End
            | XdsClass::OutOfBand => None,
        }
    }

    pub fn to_c_int(&self) -> c_int {
        match self {
            XdsType::CurrentFuture(t) => match t {
                XdsCurrentFutureType::PinStartTime => 1,
                XdsCurrentFutureType::LengthAndCurrentTime => 2,
                XdsCurrentFutureType::ProgramName => 3,
                XdsCurrentFutureType::ProgramType => 4,
                XdsCurrentFutureType::ContentAdvisory => 5,
                XdsCurrentFutureType::AudioServices => 6,
                XdsCurrentFutureType::Cgms => 8,
                XdsCurrentFutureType::AspectRatioInfo => 9,
                XdsCurrentFutureType::ProgramDesc1 => 0x10,
                XdsCurrentFutureType::ProgramDesc2 => 0x11,
                XdsCurrentFutureType::ProgramDesc3 => 0x12,
                XdsCurrentFutureType::ProgramDesc4 => 0x13,
                XdsCurrentFutureType::ProgramDesc5 => 0x14,
                XdsCurrentFutureType::ProgramDesc6 => 0x15,
                XdsCurrentFutureType::ProgramDesc7 => 0x16,
                XdsCurrentFutureType::ProgramDesc8 => 0x17,
            },
            XdsType::Channel(t) => match t {
                XdsChannelType::NetworkName => 1,
                XdsChannelType::CallLettersAndChannel => 2,
                XdsChannelType::Tsid => 4,
            },
            XdsType::Misc(t) => match t {
                XdsMiscType::TimeOfDay => 1,
                XdsMiscType::LocalTimeZone => 4,
                XdsMiscType::OutOfBandChannelNumber => 0x40,
            },
        }
    }
}