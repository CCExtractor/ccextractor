use crate::time::timing::TimingContext;

use crate::decoder_xds::exit_codes::*;

#[derive(Clone)]
pub struct XdsBuffer {
    pub in_use: bool,
    pub xds_class: i64,
    pub xds_type: i64,
    pub bytes: Vec<i64>, // of size NUM_BYTES_PER_PACKET
    pub used_bytes: i64,
}

impl XdsBuffer {
    pub fn clear(&mut self) {
        self.in_use = false;
        self.xds_class = -1;
        self.xds_type = -1;
        self.bytes = vec![0; NUM_BYTES_PER_PACKET];
        self.used_bytes = 0;
    }
}

impl Default for XdsBuffer {
    fn default() -> Self {
        XdsBuffer {
            in_use: false,
            xds_class: -1,
            xds_type: -1,
            bytes: vec![0; NUM_BYTES_PER_PACKET],
            used_bytes: 0,
        }
    }
}

#[repr(C)]
pub struct CcxDecodersXdsContext {
    pub current_xds_min: i64,
    pub current_xds_hour: i64,
    pub current_xds_date: i64,
    pub current_xds_month: i64,
    pub current_program_type_reported: i64,
    pub xds_start_time_shown: i64,
    pub xds_program_length_shown: i64,
    pub xds_program_description: Vec<String>, // 8 string of 33 bytes

    pub current_xds_network_name: String, // 33 bytes
    pub current_xds_program_name: String, // 33 bytes
    pub current_xds_call_letters: String, // 7 bytes
    pub current_xds_program_type: String, // 33 bytes

    pub xds_buffers: Vec<XdsBuffer>, // of size NUM_BYTES_PER_PACKET
    pub cur_xds_buffer_idx: Option<i64>,
    pub cur_xds_packet_class: i64,
    pub cur_xds_payload: Vec<u8>,
    pub cur_xds_payload_length: i64,
    pub cur_xds_packet_type: i64,
    pub timing: TimingContext, // use TimingContext as ccx_common_timing_ctx

    pub current_ar_start: i64,
    pub current_ar_end: i64,

    pub xds_write_to_file: bool, // originally i64
}

impl CcxDecodersXdsContext {
    pub fn how_many_used(&self) -> i64 {
        let mut count = 0;
        for buffer in self.xds_buffers.iter() {
            if buffer.in_use {
                count += 1;
            }
        }
        count
    }

    pub fn clear_xds_buffer(&mut self, index: usize) -> Result<(), &str> {
        if index < self.xds_buffers.len() {
            self.xds_buffers[index].clear();
            Ok(())
        } else {
            Err("Index out of bounds")
        }
    }
}

impl CcxDecodersXdsContext {
    pub fn xds_init_library(timing: TimingContext, xds_write_to_file: bool) -> Self {
        Self {
            timing,
            xds_write_to_file,
            ..Default::default()
        }
    }
}

impl Default for CcxDecodersXdsContext {
    fn default() -> Self {
        CcxDecodersXdsContext {
            current_xds_min: -1,
            current_xds_hour: -1,
            current_xds_date: -1,
            current_xds_month: -1,
            current_program_type_reported: 0,
            xds_start_time_shown: 0,
            xds_program_length_shown: 0,

            xds_program_description: vec![String::new(); 8],
            current_xds_network_name: String::new(),
            current_xds_program_name: String::new(),
            current_xds_call_letters: String::new(),
            current_xds_program_type: String::new(),
            xds_buffers: vec![XdsBuffer::default(); NUM_XDS_BUFFERS],
            cur_xds_buffer_idx: None,
            cur_xds_packet_class: 0,
            cur_xds_payload: Vec::new(),
            cur_xds_payload_length: 0,
            cur_xds_packet_type: 0,
            timing: TimingContext::default(),
            current_ar_start: 0,
            current_ar_end: 0,
            xds_write_to_file: true,
        }
    }
}

//----------------------------------------------------------------

use std::ptr::NonNull;

// #[derive(Debug)]
// pub enum SubtitleData {
//     None,
//     Eia608(Vec<Eia608Screen>),
// }

#[derive(Debug)]
pub struct CcSubtitle {
    pub data: Vec<Eia608Screen>, // originally SubtitleData
    pub datatype: SubDataType,
    pub nb_data: i64,
    pub subtype: SubType,
    pub enc_type: CcxEncodingType,
    pub start_time: i64,
    pub end_time: i64,
    pub flags: i64,
    pub lang_index: i64,
    pub got_output: bool,
    pub mode: Vec<u8>, // of size 5
    pub info: Vec<u8>, // of size 4
    pub time_out: i64,
    pub next: Option<NonNull<CcSubtitle>>,
    pub prev: Option<NonNull<CcSubtitle>>,
}

impl Default for CcSubtitle {
    fn default() -> Self {
        Self {
            data: Vec::new(),
            datatype: SubDataType::Default,
            nb_data: 0,
            subtype: SubType::Default,
            enc_type: CcxEncodingType::Default,
            start_time: 0,
            end_time: 0,
            flags: 0,
            lang_index: 0,
            got_output: false,
            mode: vec![0; 5],
            info: vec![0; 4],
            time_out: 0,
            next: None,
            prev: None,
        }
    }
}

#[derive(Debug, Copy, Clone, PartialEq, Eq, Default)]
pub enum SubDataType {
    #[default]
    Generic = 0,
    Dvb = 1,
    Default, // fallback variant
}

#[derive(Debug, Copy, Clone, PartialEq, Eq, Default)]
pub enum SubType {
    #[default]
    Bitmap,
    Cc608,
    Text,
    Raw,
    Default, // fallback variant
}

#[derive(Debug, Copy, Clone, PartialEq, Eq, Default)]
pub enum CcxEncodingType {
    #[default]
    Unicode = 0,
    Latin1 = 1,
    Utf8 = 2,
    Ascii = 3,
    Default, // fallback variant
}

//----------------------------------------------------------------

#[derive(Debug)]
pub struct Eia608Screen {
    pub format: CcxEia608Format,
    // pub characters:  [[u8; CCX_DECODER_608_SCREEN_WIDTH + 1]; CCX_DECODER_608_SCREEN_ROWS],
    pub characters: Vec<Vec<u8>>,
    // pub colors:      [[CcxDecoder608ColorCode; CCX_DECODER_608_SCREEN_WIDTH + 1]; CCX_DECODER_608_SCREEN_ROWS],
    pub colors: Vec<Vec<CcxDecoder608ColorCode>>,
    // pub fonts:       [[FontBits; CCX_DECODER_608_SCREEN_WIDTH + 1]; CCX_DECODER_608_SCREEN_ROWS],
    pub fonts: Vec<Vec<FontBits>>,
    // pub row_used:    [i64; CCX_DECODER_608_SCREEN_ROWS],
    pub row_used: Vec<i64>,

    pub empty: i64,
    pub start_time: i64,
    pub end_time: i64,
    pub mode: CcModes,
    pub channel: i64,
    pub my_field: i64,
    pub xds_str: String,
    pub xds_len: i64,
    pub cur_xds_packet_class: i64,
}

#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum CcxEia608Format {
    SformatCcScreen,
    SformatCcLine,
    SformatXds,
    Default, // fallback variant
}

#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum CcxDecoder608ColorCode {
    ColWhite = 0,
    ColGreen = 1,
    ColBlue = 2,
    ColCyan = 3,
    ColRed = 4,
    ColYellow = 5,
    ColMagenta = 6,
    ColUserDefined = 7,
    ColBlack = 8,
    ColTransparent = 9,
}

#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum FontBits {
    FontRegular = 0,
    FontItalics = 1,
    FontUnderlined = 2,
    FontUnderlinedItalics = 3,
}

#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum CcModes {
    ModePopOn = 0,
    ModeRollUp2 = 1,
    ModeRollUp3 = 2,
    ModeRollUp4 = 3,
    ModeText = 4,
    ModePaintOn = 5,
    ModeFakeRollUp1 = 100, // Fake mode, also used as default mode
}

impl Default for Eia608Screen {
    fn default() -> Eia608Screen {
        Eia608Screen {
            format: CcxEia608Format::Default,
            characters: vec![
                vec![0; CCX_DECODER_608_SCREEN_WIDTH + 1];
                CCX_DECODER_608_SCREEN_ROWS
            ],
            colors: vec![
                vec![CcxDecoder608ColorCode::ColWhite; CCX_DECODER_608_SCREEN_WIDTH + 1];
                CCX_DECODER_608_SCREEN_ROWS
            ],
            fonts: vec![
                vec![FontBits::FontRegular; CCX_DECODER_608_SCREEN_WIDTH + 1];
                CCX_DECODER_608_SCREEN_ROWS
            ],
            row_used: vec![0; CCX_DECODER_608_SCREEN_ROWS],
            empty: 0,
            start_time: TS_START_OF_XDS,
            end_time: 0,
            mode: CcModes::ModeFakeRollUp1,
            channel: 0,
            my_field: 0,
            xds_str: String::new(),
            xds_len: 0,
            cur_xds_packet_class: 0,
        }
    }
}
