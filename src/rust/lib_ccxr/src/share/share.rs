extern crate libc;
use std::cmp::PartialEq;
use std::io::Write;
use std::{ffi::{CStr, CString}, time::Duration};

use crate::share::ccxr_sub_entry_message::CcxSubEntryMessage;
use crate::util::log::{debug, DebugMessageFlag};
use lazy_static::lazy_static;
use nanomsg::{Endpoint, Protocol, Socket};
use prost::Message;
use std::os::raw::{c_char, c_int};
use std::sync::Mutex;
use std::thread::sleep;

// use crate::bindings::{cc_subtitle, ccx_output_format};

pub const CCX_DECODER_608_SCREEN_ROWS: usize = 15;
pub const CCX_DECODER_608_SCREEN_WIDTH: usize = 32;

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum CcxEia608Format {
    SFormatCcScreen = 0,
    SFormatCcLine = 1,
    SFormatXds = 2,
}

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum CcModes {
    ModePopOn = 0,
    ModeRollUp2 = 1,
    ModeRollUp3 = 2,
    ModeRollUp4 = 3,
    ModeText = 4,
    ModePaintOn = 5,
    ModeFakeRollUp1 = 100,
}


#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum CcxDecoder608ColorCode {
    White = 0,
    Green = 1,
    Blue = 2,
    Cyan = 3,
    Red = 4,
    Yellow = 5,
    Magenta = 6,
    UserDefined = 7,
    Black = 8,
    Transparent = 9,
    Max = 10,
}

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum FontBits {
    Normal = 0,
    Italics = 1,
    Underline = 2,
    UnderlineItalics = 3,
}


#[repr(C)]
#[derive(Debug, Clone)]
pub struct Eia608Screen {
    pub format: CcxEia608Format,
    pub characters: [[c_char; CCX_DECODER_608_SCREEN_WIDTH + 1]; CCX_DECODER_608_SCREEN_ROWS],
    pub colors: [[CcxDecoder608ColorCode; CCX_DECODER_608_SCREEN_WIDTH + 1]; CCX_DECODER_608_SCREEN_ROWS],
    pub fonts: [[FontBits; CCX_DECODER_608_SCREEN_WIDTH + 1]; CCX_DECODER_608_SCREEN_ROWS],
    pub row_used: [c_int; CCX_DECODER_608_SCREEN_ROWS],
    pub empty: c_int,
    pub start_time: i64,
    pub end_time: i64,
    pub mode: CcModes,
    pub channel: c_int,
    pub my_field: c_int,
    pub xds_str: *mut c_char,
    pub xds_len: usize,
    pub cur_xds_packet_class: c_int,
}

impl Default for Eia608Screen {
    fn default() -> Self {
        Self {
            format: CcxEia608Format::SFormatCcScreen,
            characters: [[0; CCX_DECODER_608_SCREEN_WIDTH + 1]; CCX_DECODER_608_SCREEN_ROWS],
            colors: [[CcxDecoder608ColorCode::Black; CCX_DECODER_608_SCREEN_WIDTH + 1]; CCX_DECODER_608_SCREEN_ROWS],
            fonts: [[FontBits::Normal; CCX_DECODER_608_SCREEN_WIDTH + 1]; CCX_DECODER_608_SCREEN_ROWS],
            row_used: [0; CCX_DECODER_608_SCREEN_ROWS],
            empty: 1,
            start_time: 0,
            end_time: 0,
            mode: CcModes::ModePopOn,
            channel: 0,
            my_field: 0,
            xds_str: std::ptr::null_mut(),
            xds_len: 0,
            cur_xds_packet_class: 0,
        }
    }
}

impl Eia608Screen {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn set_xds_str(&mut self, xds: &str) {
        let c_string = CString::new(xds).expect("CString::new failed");
        self.xds_str = c_string.into_raw();
        self.xds_len = xds.len();
    }

    pub fn free_xds_str(&mut self) {
        if !self.xds_str.is_null() {
            unsafe {
                let _ = CString::from_raw(self.xds_str);
            }
            self.xds_str = std::ptr::null_mut();
            self.xds_len = 0;
        }
    }
}

impl Drop for Eia608Screen {
    fn drop(&mut self) {
        self.free_xds_str();
    }
}


pub type SubDataType = std::os::raw::c_uint;
pub type LLONG = i64;
pub const SUBTYPE_CC_BITMAP: Subtype = 0;
pub const SUBTYPE_CC_608: Subtype = 1;
pub const SUBTYPE_CC_TEXT: Subtype = 2;
pub const SUBTYPE_CC_RAW: Subtype = 3;
pub type Subtype = std::os::raw::c_uint;

pub type CcxEncodingType = std::os::raw::c_uint;

pub struct CcSubtitle {
    #[doc = " A generic data which contain data according to decoder\n @warn decoder cant output multiple types of data"]
    pub data: *mut std::os::raw::c_void,
    pub datatype: SubDataType,
    #[doc = " number of data"]
    pub nb_data: std::os::raw::c_uint,
    #[doc = "  type of subtitle"]
    pub type_: Subtype,
    #[doc = " Encoding type of Text, must be ignored in case of subtype as bitmap or cc_screen"]
    pub enc_type: CcxEncodingType,
    pub start_time: LLONG,
    pub end_time: LLONG,
    pub flags: c_int,
    pub lang_index: c_int,
    #[doc = " flag to tell that decoder has given output"]
    pub got_output: c_int,
    pub mode: [c_char; 5usize],
    pub info: [c_char; 4usize],
    #[doc = " Used for DVB end time in ms"]
    pub time_out: c_int,
    pub next: *mut CcSubtitle,
    pub prev: *mut CcSubtitle,
}

#[repr(C)]
#[derive(Debug)]
pub enum CcxShareStatus {
    Ok = 0,
    Fail,
}
impl PartialEq for CcxShareStatus {
    fn eq(&self, other: &Self) -> bool {
        match (self, other) {
            (CcxShareStatus::Ok, CcxShareStatus::Ok) => true,
            (CcxShareStatus::Fail, CcxShareStatus::Fail) => true,
            _ => false,
        }
    }
}
pub struct CcxShareServiceCtx {
    counter: i64,
    stream_name: Option<String>,
    nn_sock: Option<Socket>,
    nn_binder: Option<Endpoint>,
}

pub struct CcxSubEntries {
    pub messages: Vec<CcxSubEntryMessage>,
}
impl CcxShareServiceCtx {
    fn new() -> Self {
        CcxShareServiceCtx {
            counter: 0,
            stream_name: None,
            nn_sock: None,
            nn_binder: None,
        }
    }
}


lazy_static! {
    pub static ref CCX_SHARE_CTX: Mutex<CcxShareServiceCtx> = Mutex::new(CcxShareServiceCtx::new());
}



pub fn ccxr_sub_entry_msg_init(msg: &mut CcxSubEntryMessage) {
    msg.eos = 0;
    msg.stream_name = "".parse().unwrap();
    msg.counter = 0;
    msg.start_time = 0;
    msg.end_time = 0;
    msg.lines.clear();
}


pub fn ccxr_sub_entry_msg_cleanup(msg: &mut CcxSubEntryMessage) {
    msg.lines.clear();
    msg.stream_name = "".parse().unwrap();
}

pub fn ccxr_sub_entry_msg_print(msg: &CcxSubEntryMessage) {
    if msg.lines.is_empty() {
        debug!(msg_type = DebugMessageFlag::SHARE; "[share] no lines allocated");
        return;
    }

    debug!(msg_type = DebugMessageFlag::SHARE; "\n[share] sub msg #{}", msg.counter);
    if !msg.stream_name.is_empty() {
        debug!(msg_type = DebugMessageFlag::SHARE; "[share] name: {}", msg.stream_name);
    } else {
        debug!(msg_type = DebugMessageFlag::SHARE; "[share] name: None");
    }
    debug!(msg_type = DebugMessageFlag::SHARE; "[share] start: {}", msg.start_time);
    debug!(msg_type = DebugMessageFlag::SHARE; "[share] end: {}", msg.end_time);
    debug!(msg_type = DebugMessageFlag::SHARE; "[share] lines count: {}", msg.lines.len());

    if msg.lines.is_empty() {
        debug!(msg_type = DebugMessageFlag::SHARE; "[share] no lines allocated");
        return;
    }
    for (i, line) in msg.lines.iter().enumerate() {
        if !line.is_empty() {
            debug!(msg_type = DebugMessageFlag::SHARE; "[share] line[{}]: {}", i, line);
        } else {
            debug!(msg_type = DebugMessageFlag::SHARE; "[share] line[{}] is not allocated", i);
        }
    }
}


pub fn ccxr_sub_entries_cleanup(entries: &mut CcxSubEntries) {
    entries.messages.clear();
}

pub fn ccxr_sub_entries_print(entries: &CcxSubEntries) {
    eprintln!("[share] ccxr_sub_entries_print ({} entries)", entries.messages.len());
    for message in &entries.messages {
        ccxr_sub_entry_msg_print(message);
    }
}

pub fn ccxr_share_start(stream_name: &str) -> CcxShareStatus {
    debug!(msg_type = DebugMessageFlag::SHARE; "[share] ccx_share_start: starting service\n");
    let mut ctx = CCX_SHARE_CTX.lock().unwrap();
    ctx.nn_sock = Some(Socket::new(Protocol::Pub).unwrap());
    if ctx.nn_sock.is_none() {
        debug!(msg_type = DebugMessageFlag::SHARE; "[share] ccx_share_start: can't nn_socket()\n");
        return CcxShareStatus::Fail;
    }
    // TODO translating this line
    // if (!ccx_options.sharing_url)
    // {
    //     ccx_options.sharing_url = "tcp://*:3269";
    // }
    // dbg_print(CCX_DMT_SHARE, "[share] ccx_share_start: url=%s\n", ccx_options.sharing_url);
    let sharing_url = "tcp://*:3269"; // Default URL if none is provided
    debug!(msg_type = DebugMessageFlag::SHARE; "[share] ccx_share_start: url={}", sharing_url);
    let binder = match ctx.nn_sock.as_mut().unwrap().bind(sharing_url) {
        Ok(endpoint) => endpoint,
        Err(err) => {
            eprintln!("[share] ccx_share_start: can't bind to URL: {}", err);
            return CcxShareStatus::Fail;
        }
    };
    ctx.nn_binder = Some(binder);
    if let Err(err) = ctx.nn_sock.as_mut().unwrap().set_linger(-1) {
        eprintln!("[share] ccx_share_start: can't set linger option: {}", err);
        return CcxShareStatus::Fail;
    }
    ctx.stream_name = Some(stream_name.parse().unwrap());
    sleep(Duration::from_secs(1));

    CcxShareStatus::Ok
}


pub fn ccxr_share_stop() -> CcxShareStatus {
    let mut ctx = CCX_SHARE_CTX.lock().unwrap();

    debug!(msg_type = DebugMessageFlag::SHARE; "[share] ccx_share_stop: stopping service");


    if let Some(mut binder) = ctx.nn_binder.take() {
        if let Err(err) = binder.shutdown() {
            debug!(msg_type = DebugMessageFlag::SHARE; "[share] ccx_share_stop: error during shutdown: {}", err);
            return CcxShareStatus::Fail;
        }
    }
    ctx.stream_name = None;
    ctx.nn_sock = None;
    ctx.nn_binder = None;

    CcxShareStatus::Ok
}


pub fn ccxr_share_send(sub: &CcSubtitle) -> CcxShareStatus {
    debug!(msg_type = DebugMessageFlag::SHARE; "[share] ccx_share_send: sending");

    let mut entries = CcxSubEntries { messages: Vec::new() };
    ccxr_sub_entries_cleanup(&mut entries);

    if ccxr_share_sub_to_entries(sub, &mut entries) == CcxShareStatus::Fail {
        debug!(msg_type = DebugMessageFlag::SHARE; "[share] failed to convert subtitle to entries");
        return CcxShareStatus::Fail;
    }

    ccxr_sub_entries_print(&entries);
    debug!(msg_type = DebugMessageFlag::SHARE; "[share] entry obtained:");

    for i in 0..entries.messages.len() {
        debug!(msg_type = DebugMessageFlag::SHARE; "[share] ccx_share_send: sending entry {}", i);

        if entries.messages[i].lines.is_empty() {
            debug!(msg_type = DebugMessageFlag::SHARE; "[share] skipping empty message");
            continue;
        }

        if _ccxr_share_send(&entries.messages[i]) != CcxShareStatus::Ok {
            debug!(msg_type = DebugMessageFlag::SHARE; "[share] can't send message");
            ccxr_sub_entries_cleanup(&mut entries);
            return CcxShareStatus::Fail;
        }
    }

    ccxr_sub_entries_cleanup(&mut entries);

    CcxShareStatus::Ok
}

pub fn ccxr_sub_entry_message_get_packed_size(message: &CcxSubEntryMessage) -> usize {
    message.encoded_len()
}

pub fn ccxr_sub_entry_message_pack(message: &CcxSubEntryMessage, buf: &mut Vec<u8>) -> Result<(), prost::EncodeError> {
    message.encode(buf)
}


pub fn _ccxr_share_send(msg: &CcxSubEntryMessage) -> CcxShareStatus {
    debug!(msg_type = DebugMessageFlag::SHARE; "[share] _ccx_share_send");
    let len = ccxr_sub_entry_message_get_packed_size(msg);
    let mut buf = Vec::with_capacity(len);
    debug!(msg_type = DebugMessageFlag::SHARE; "[share] _ccx_share_send: packing");
    #[allow(unused)]
    let _ = ccxr_sub_entry_message_pack(msg, &mut buf);
    debug!(msg_type = DebugMessageFlag::SHARE; "[share] _ccx_share_send: sending");
    let mut ctx = CCX_SHARE_CTX.lock().unwrap();
    if let Some(sock) = ctx.nn_sock.as_mut() {
        match sock.write(&buf) {
            Ok(sent) => {
                if sent != buf.len() {
                    debug!(
                    msg_type = DebugMessageFlag::SHARE;
                    "[share] _ccx_share_send: len={} sent={}", buf.len(), sent
                );
                    return CcxShareStatus::Fail;
                }
            }
            Err(err) => {
                eprintln!("[share] _ccx_share_send: failed to send message: {}", err);
                return CcxShareStatus::Fail;
            }
        }
    } else {
        eprintln!("[share] _ccx_share_send: no active socket available");
        return CcxShareStatus::Fail;
    }

    debug!(msg_type = DebugMessageFlag::SHARE; "[share] _ccx_share_send: sent");
    CcxShareStatus::Ok
}


pub fn ccxr_share_stream_done(stream_name: &str) -> CcxShareStatus {
    let mut msg = CcxSubEntryMessage {
        eos: 1,
        stream_name: stream_name.parse().unwrap(),
        counter: 0,
        start_time: 0,
        end_time: 0,
        lines: Vec::new(),
    };
    #[allow(unused)]
    let mut ctx = CCX_SHARE_CTX.lock().unwrap();

    if _ccxr_share_send(&msg) != CcxShareStatus::Ok {
        ccxr_sub_entry_msg_cleanup(&mut msg);
        debug!(msg_type = DebugMessageFlag::SHARE; "[share] ccx_share_stream_done: can't send message");
        return CcxShareStatus::Fail;
    }
    debug!(msg_type = DebugMessageFlag::SHARE; "[share] ccx_share_stream_done: message sent successfully");
    CcxShareStatus::Ok
}

pub fn ccxr_share_sub_to_entries(sub: &CcSubtitle, entries: &mut CcxSubEntries) -> CcxShareStatus {
    unsafe {
        let mut ctx = CCX_SHARE_CTX.lock().unwrap();

        debug!(msg_type = DebugMessageFlag::SHARE; "[share] _ccx_share_sub_to_entries");

        if sub.type_ == SUBTYPE_CC_608 {
            debug!(msg_type = DebugMessageFlag::SHARE; "[share] CC_608");

            let data_ptr = sub.data as *const Eia608Screen;
            let mut nb_data = sub.nb_data;

            while nb_data > 0 {
                let data = &*data_ptr.add(sub.nb_data as usize - nb_data as usize);

                debug!(msg_type = DebugMessageFlag::SHARE; "[share] data item");

                if data.format == CcxEia608Format::SFormatXds {
                    debug!(msg_type = DebugMessageFlag::SHARE; "[share] XDS. Skipping");
                    nb_data -= 1;
                    continue;
                }

                if data.start_time == 0 {
                    debug!(msg_type = DebugMessageFlag::SHARE; "[share] No start time. Skipping");
                    break;
                }

                entries.messages.push(CcxSubEntryMessage {
                    eos: 0,
                    stream_name: String::new(),
                    counter: ctx.counter + 1,
                    start_time: data.start_time,
                    end_time: data.end_time,
                    lines: Vec::new(),
                });

                let entry_index = entries.messages.len() - 1;
                let entry = &mut entries.messages[entry_index];

                for row in 0..CCX_DECODER_608_SCREEN_ROWS {
                    if data.row_used[row] != 0 {
                        let characters = CStr::from_ptr(data.characters[row].as_ptr())
                            .to_string_lossy()
                            .to_string();
                        entry.lines.push(characters);
                    }
                }

                if entry.lines.is_empty() {
                    debug!(msg_type = DebugMessageFlag::SHARE; "[share] buffer is empty");
                    entries.messages.pop();
                    return CcxShareStatus::Ok;
                }

                debug!(
                    msg_type = DebugMessageFlag::SHARE;
                    "[share] Copied {} lines", entry.lines.len()
                );

                ctx.counter += 1;
                nb_data -= 1;

                debug!(msg_type = DebugMessageFlag::SHARE; "[share] item done");
            }
        } else {
            match sub.type_ {
                SUBTYPE_CC_BITMAP => {
                    debug!(msg_type = DebugMessageFlag::SHARE; "[share] CC_BITMAP. Skipping");
                }
                SUBTYPE_CC_RAW => {
                    debug!(msg_type = DebugMessageFlag::SHARE; "[share] CC_RAW. Skipping");
                }
                SUBTYPE_CC_TEXT => {
                    debug!(msg_type = DebugMessageFlag::SHARE; "[share] CC_TEXT. Skipping");
                }
                _ => {
                    debug!(msg_type = DebugMessageFlag::SHARE; "[share] Unknown subtitle type");
                }
            }
        }

        debug!(msg_type = DebugMessageFlag::SHARE; "[share] done");
        CcxShareStatus::Ok
    }
}
