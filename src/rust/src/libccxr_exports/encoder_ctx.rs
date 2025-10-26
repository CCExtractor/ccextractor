use crate::bindings::{encoder_ctx, ccx_s_write};
use crate::encoder::FromCType;
use lib_ccxr::util::encoding::Encoding;
use std::ffi::CStr;

//Complete ccx_s_write in the rust side to match the C side and direct use in the rust code
#[derive(Debug, Clone)]
pub struct ccxr_s_write {
    pub fh: i32,
    pub temporarily_closed: bool,
    pub filename: Option<String>,
    pub original_filename: Option<String>,
    pub spupng_data: Option<*mut libc::c_void>,
    pub with_semaphore: bool,
    pub semaphore_filename: Option<String>,
    pub with_playlist: bool,
    pub playlist_filename: Option<String>,
    pub renaming_extension: i32,
    pub append_mode: bool,
}
impl Default for ccxr_s_write {
    fn default() -> Self {
        ccxr_s_write {
            fh: -1,
            temporarily_closed: false,
            filename: None,
            original_filename: None,
            spupng_data: None,
            with_semaphore: false,
            semaphore_filename: None,
            with_playlist: false,
            playlist_filename: None,
            renaming_extension: 0,
            append_mode: false,
        }
    }
}
// Convert the C struct (ccx_s_write) to the Rust struct (ccxr_s_write)
impl FromCType<ccx_s_write> for ccxr_s_write {
    unsafe fn from_ctype(c: ccx_s_write) -> Option<Self> {
        let filename = if !c.filename.is_null() {
            Some(CStr::from_ptr(c.filename).to_string_lossy().into_owned())
        } else {
            None
        };

        let original_filename = if !c.original_filename.is_null() {
            Some(CStr::from_ptr(c.original_filename).to_string_lossy().into_owned())
        } else {
            None
        };

        let semaphore_filename = if !c.semaphore_filename.is_null() {
            Some(CStr::from_ptr(c.semaphore_filename).to_string_lossy().into_owned())
        } else {
            None
        };

        let playlist_filename = if !c.playlist_filename.is_null() {
            Some(CStr::from_ptr(c.playlist_filename).to_string_lossy().into_owned())
        } else {
            None
        };

        Some(ccxr_s_write {
            fh: c.fh,
            temporarily_closed: c.temporarily_closed != 0,
            filename,
            original_filename,
            spupng_data: if c.spupng_data.is_null() { None } else { Some(c.spupng_data) },
            with_semaphore: c.with_semaphore != 0,
            semaphore_filename,
            with_playlist: c.with_playlist != 0,
            playlist_filename,
            renaming_extension: c.renaming_extension,
            append_mode: c.append_mode != 0,
        })
    }
}



// Complete encoder_ctx in the rust side to match the C side and direct use in the rust code
#[derive(Debug, Clone)]
pub struct ccxr_encoder_ctx {
    // Common buffer used by all encoder
    pub buffer: Option<Vec<u8>>,
    pub capacity: u32,
    
    // Keep count of subtitles
    pub srt_counter: u32,
    pub cea_708_counter: u32,
    
    // WebVTT header flag
    pub wrote_webvtt_header: u32,
    pub wrote_ccd_channel_header: i8,
    
    // Input outputs
    pub send_to_srv: u32, // Flag giving hint that output is send to server through network
    pub multiple_files: i32, // Used only in Spupng output
    pub first_input_file: Option<String>, // Used only in Spupng output and creating name of output file
    pub out: Option<Vec<ccxr_s_write>>, // Rust Vec of write structs (converted from C array)
    pub nb_out: i32, // number of member in array of write out array
    
    // Input file format used in Teletext for exceptional output
    pub in_fileformat: u32, // 1 = Normal, 2 = Teletext
    
    // Keep output file closed when not actually writing to it and start over each time
    pub keep_output_closed: u32,
    pub force_flush: i32, // Force a flush on the file buffer whenever content is written
    pub ucla: i32, // Keep track of whether -UCLA used
    
    pub timing: Option<*mut crate::bindings::ccx_common_timing_ctx>, // Some encoders need access to PTS, such as WebVTT
    
    // Encoding and output format
    pub encoding: Encoding,
    pub write_format: crate::bindings::ccx_output_format, // 0=Raw, 1=srt, 2=SMI
    pub generates_file: i32,
    pub transcript_settings: Option<*mut crate::bindings::ccx_encoders_transcript_format>, // Keeps the settings for generating transcript output files
    
    // Various flags
    pub no_bom: i32,
    pub sentence_cap: i32, // FIX CASE? = Fix case?
    pub filter_profanity: i32,
    pub trim_subs: i32, // " Remove spaces at sides? "
    pub autodash: i32, // Add dashes (-) before each speaker automatically?
    pub no_font_color: i32,
    pub no_type_setting: i32,
    pub gui_mode_reports: i32, // If 1, output in stderr progress updates so the GUI can grab them
    
    // Temp storage
    pub subline: Option<Vec<u8>>, // Temp storage for storing each line
    
    // Extraction flags
    pub extract: i32,
    pub dtvcc_extract: i32, // 1 or 0 depending if we have to handle dtvcc
    pub dtvcc_writers: [Option<crate::bindings::dtvcc_writer_ctx>; 4], // CCX_DTVCC_MAX_SERVICES = 4
    
    // Timing related variables
    pub prev_start: i64, // start time of previous sub
    pub subs_delay: i64,
    pub last_displayed_subs_ms: i64,
    pub date_format: crate::bindings::ccx_output_date_format, // enum ccx_output_date_format
    pub millis_separator: i8,
    
    // Credit stuff
    pub startcredits_displayed: i32,
    pub start_credits_text: Option<String>,
    pub end_credits_text: Option<String>,
    pub startcreditsnotbefore: Option<crate::bindings::ccx_boundary_time>, // Where to insert start credits, if possible
    pub startcreditsnotafter: Option<crate::bindings::ccx_boundary_time>,
    pub startcreditsforatleast: Option<crate::bindings::ccx_boundary_time>, // How long to display them?
    pub startcreditsforatmost: Option<crate::bindings::ccx_boundary_time>,
    pub endcreditsforatleast: Option<crate::bindings::ccx_boundary_time>,
    pub endcreditsforatmost: Option<crate::bindings::ccx_boundary_time>,
    
    // Preencoded strings
    pub encoded_crlf: Option<String>,
    pub encoded_crlf_length: u32,
    pub encoded_br: Option<String>,
    pub encoded_br_length: u32,
    
    // MCC File
    pub header_printed_flag: i32,
    pub next_caption_time: Option<crate::bindings::ccx_mcc_caption_time>,
    pub cdp_hdr_seq: u32,
    pub force_dropframe: i32,
    pub new_sentence: i32, // Capitalize next letter?
    pub program_number: i32,
    
    // List and other flags
    pub list: Option<crate::bindings::list_head>,
    pub sbs_enabled: i32, // split-by-sentence stuff
    pub prev: Option<*mut encoder_ctx>, // for dvb subs
    pub write_previous: i32, // for dvb in .mkv
    pub is_mkv: i32, // are we working with .mkv file
    pub last_string: Option<String>, // last recognized DVB sub
    
    // Segmenting
    pub segment_pending: i32,
    pub segment_last_key_frame: i32,
    
    // OCR in SPUPNG
    pub nospupngocr: i32,
}

// ---- Default implementation ----
impl Default for ccxr_encoder_ctx {
    fn default() -> Self {
        ccxr_encoder_ctx {
            // Common buffer used by all encoder
            buffer: Some(Vec::with_capacity(1024)),
            capacity: 1024,
            
            // Keep count of subtitles
            srt_counter: 0,
            cea_708_counter: 0,
            
            // WebVTT header flag
            wrote_webvtt_header: 0,
            wrote_ccd_channel_header: 0,
            
            // Input outputs
            send_to_srv: 0,
            multiple_files: 0,
            first_input_file: None,
            out: None,
            nb_out: 0,
            
            // Input file format
            in_fileformat: 1, // 1 = Normal
            
            // Keep output file closed
            keep_output_closed: 0,
            force_flush: 0,
            ucla: 0,
            
            timing: None,
            
            // Encoding and output format
            encoding: Encoding::default(),
            write_format: unsafe { std::mem::transmute(1i32) }, // 1 = srt (ccx_output_format_CCX_OF_SRT)
            generates_file: 0,
            transcript_settings: None,
            
            // Various flags
            no_bom: 0,
            sentence_cap: 0,
            filter_profanity: 0,
            trim_subs: 0,
            autodash: 0,
            no_font_color: 0,
            no_type_setting: 0,
            gui_mode_reports: 0,
            
            // Temp storage
            subline: Some(Vec::with_capacity(1024)),
            
            // Extraction flags
            extract: 0,
            dtvcc_extract: 0,
            dtvcc_writers: [None; 4],
            
            // Timing related variables
            prev_start: 0,
            subs_delay: 0,
            last_displayed_subs_ms: 0,
            date_format: unsafe { std::mem::transmute(0i32) }, // 0 = ISO (ccx_output_date_format_CCX_DF_ISO)
            millis_separator: b',' as i8,
            
            // Credit stuff
            startcredits_displayed: 0,
            start_credits_text: None,
            end_credits_text: None,
            startcreditsnotbefore: None,
            startcreditsnotafter: None,
            startcreditsforatleast: None,
            startcreditsforatmost: None,
            endcreditsforatleast: None,
            endcreditsforatmost: None,
            
            // Preencoded strings
            encoded_crlf: Some("\r\n".to_string()),
            encoded_crlf_length: 2,
            encoded_br: Some("<br>".to_string()),
            encoded_br_length: 4,
            
            // MCC File
            header_printed_flag: 0,
            next_caption_time: None,
            cdp_hdr_seq: 0,
            force_dropframe: 0,
            new_sentence: 0,
            program_number: 0,
            
            // List and other flags
            list: None,
            sbs_enabled: 0,
            prev: None,
            write_previous: 0,
            is_mkv: 0,
            last_string: None,
            
            // Segmenting
            segment_pending: 0,
            segment_last_key_frame: 0,
            
            // OCR in SPUPNG
            nospupngocr: 0,
        }
    }
}

// ---- From C to Rust ----
impl ccxr_encoder_ctx {
    pub unsafe fn from_ctype(c: *mut encoder_ctx) -> Option<Self> {
        if c.is_null() {
            return None;
        }
        let c_ref = &*c;

        // Copy buffer
        let buffer = if !c_ref.buffer.is_null() && c_ref.capacity > 0 {
            let slice = std::slice::from_raw_parts(c_ref.buffer, c_ref.capacity as usize);
            Some(slice.to_vec())
        } else {
            None
        };

        // Copy subline
        let subline = if !c_ref.subline.is_null() {
            let slice = std::slice::from_raw_parts(c_ref.subline, 1024);
            Some(slice.to_vec())
        } else {
            None
        };

        // Copy string fields
        let first_input_file = if !c_ref.first_input_file.is_null() {
            Some(CStr::from_ptr(c_ref.first_input_file).to_string_lossy().into_owned())
        } else {
            None
        };

        let start_credits_text = if !c_ref.start_credits_text.is_null() {
            Some(CStr::from_ptr(c_ref.start_credits_text).to_string_lossy().into_owned())
        } else {
            None
        };

        let end_credits_text = if !c_ref.end_credits_text.is_null() {
            Some(CStr::from_ptr(c_ref.end_credits_text).to_string_lossy().into_owned())
        } else {
            None
        };

        let last_string = if !c_ref.last_string.is_null() {
            Some(CStr::from_ptr(c_ref.last_string).to_string_lossy().into_owned())
        } else {
            None
        };

        // Copy encoded_crlf
        let encoded_crlf = {
            let bytes = &c_ref.encoded_crlf;
            let len = bytes.iter().position(|&b| b == 0).unwrap_or(16);
            Some(String::from_utf8_lossy(&bytes[..len]).to_string())
        };

        // Copy encoded_br
        let encoded_br = {
            let bytes = &c_ref.encoded_br;
            let len = bytes.iter().position(|&b| b == 0).unwrap_or(16);
            Some(String::from_utf8_lossy(&bytes[..len]).to_string())
        };

        // Copy dtvcc_writers array
        let mut dtvcc_writers = [None; 4];
        for i in 0..4 {
            dtvcc_writers[i] = Some(c_ref.dtvcc_writers[i]);
        }

        Some(ccxr_encoder_ctx {
            // Common buffer used by all encoder
            buffer,
            capacity: c_ref.capacity,
            
            // Keep count of subtitles
            srt_counter: c_ref.srt_counter,
            cea_708_counter: c_ref.cea_708_counter,
            
            // WebVTT header flag
            wrote_webvtt_header: c_ref.wrote_webvtt_header,
            wrote_ccd_channel_header: c_ref.wrote_ccd_channel_header,
            
            // Input outputs
            send_to_srv: c_ref.send_to_srv,
            multiple_files: c_ref.multiple_files,
            first_input_file,
            out: if !c_ref.out.is_null() && c_ref.nb_out > 0 {
                let mut out_vec = Vec::new();
                for i in 0..c_ref.nb_out {
                    let c_write = unsafe { &*c_ref.out.offset(i as isize) };
                    // Use the FromCType trait you found!
                    if let Some(rust_write) = unsafe { ccxr_s_write::from_ctype(*c_write) } {
                        out_vec.push(rust_write);
                    }
                }
                Some(out_vec)
            } else {
                None
            },
            nb_out: c_ref.nb_out,
            
            // Input file format
            in_fileformat: c_ref.in_fileformat,
            
            // Keep output file closed
            keep_output_closed: c_ref.keep_output_closed,
            force_flush: c_ref.force_flush,
            ucla: c_ref.ucla,
            
            timing: if !c_ref.timing.is_null() { Some(c_ref.timing) } else { None },
            
            // Encoding and output format
            encoding: Encoding::from_ctype(c_ref.encoding).unwrap_or(Encoding::default()),
            write_format: c_ref.write_format,
            generates_file: c_ref.generates_file,
            transcript_settings: if !c_ref.transcript_settings.is_null() { Some(c_ref.transcript_settings) } else { None },
            
            // Various flags
            no_bom: c_ref.no_bom,
            sentence_cap: c_ref.sentence_cap,
            filter_profanity: c_ref.filter_profanity,
            trim_subs: c_ref.trim_subs,
            autodash: c_ref.autodash,
            no_font_color: c_ref.no_font_color,
            no_type_setting: c_ref.no_type_setting,
            gui_mode_reports: c_ref.gui_mode_reports,
            
            // Temp storage
            subline,
            
            // Extraction flags
            extract: c_ref.extract,
            dtvcc_extract: c_ref.dtvcc_extract,
            dtvcc_writers,
            
            // Timing related variables
            prev_start: c_ref.prev_start,
            subs_delay: c_ref.subs_delay,
            last_displayed_subs_ms: c_ref.last_displayed_subs_ms,
            date_format: c_ref.date_format,
            millis_separator: c_ref.millis_separator,
            
            // Credit stuff
            startcredits_displayed: c_ref.startcredits_displayed,
            start_credits_text,
            end_credits_text,
            startcreditsnotbefore: Some(c_ref.startcreditsnotbefore),
            startcreditsnotafter: Some(c_ref.startcreditsnotafter),
            startcreditsforatleast: Some(c_ref.startcreditsforatleast),
            startcreditsforatmost: Some(c_ref.startcreditsforatmost),
            endcreditsforatleast: Some(c_ref.endcreditsforatleast),
            endcreditsforatmost: Some(c_ref.endcreditsforatmost),
            
            // Preencoded strings
            encoded_crlf,
            encoded_crlf_length: c_ref.encoded_crlf_length,
            encoded_br,
            encoded_br_length: c_ref.encoded_br_length,
            
            // MCC File
            header_printed_flag: c_ref.header_printed_flag,
            next_caption_time: Some(c_ref.next_caption_time),
            cdp_hdr_seq: c_ref.cdp_hdr_seq,
            force_dropframe: c_ref.force_dropframe,
            new_sentence: c_ref.new_sentence,
            program_number: c_ref.program_number,
            
            // List and other flags
            list: Some(c_ref.list),
            sbs_enabled: c_ref.sbs_enabled,
            prev: if !c_ref.prev.is_null() { Some(c_ref.prev) } else { None },
            write_previous: c_ref.write_previous,
            is_mkv: c_ref.is_mkv,
            last_string,
            
            // Segmenting
            segment_pending: c_ref.segment_pending,
            segment_last_key_frame: c_ref.segment_last_key_frame,
            
            // OCR in SPUPNG
            nospupngocr: c_ref.nospupngocr,
        })
    }
}

// Copy the C encoder_ctx to the Rust encoder_ctx through a function
pub fn copy_encoder_ctx_c_to_rust(c_ctx: *mut encoder_ctx) -> ccxr_encoder_ctx {
    if c_ctx.is_null() {
        return ccxr_encoder_ctx::default();
    }
    
    unsafe {
        ccxr_encoder_ctx::from_ctype(c_ctx).unwrap_or_default()
    }
}
// Copy the Rust ccxr_encoder_ctx to the C encoder_ctx through a function
pub unsafe fn copy_encoder_ctx_rust_to_c(rust_ctx: &mut ccxr_encoder_ctx, c_ctx: *mut encoder_ctx) {
    if c_ctx.is_null() {
        return;
    }
    
    let c = &mut *c_ctx;
    
    // Copy the most commonly used fields back
    c.srt_counter = rust_ctx.srt_counter;
    c.cea_708_counter = rust_ctx.cea_708_counter;
    
    // Copy buffer back (only if C buffer exists and has capacity)
    if let Some(ref buffer) = rust_ctx.buffer {
        if !c.buffer.is_null() && c.capacity > 0 {
            let copy_len = std::cmp::min(buffer.len(), c.capacity as usize);
            std::ptr::copy_nonoverlapping(buffer.as_ptr(), c.buffer, copy_len);
        }
    }
    
    // Copy subline back
    if let Some(ref subline) = rust_ctx.subline {
        if !c.subline.is_null() {
            let copy_len = std::cmp::min(subline.len(), 1024);
            std::ptr::copy_nonoverlapping(subline.as_ptr(), c.subline, copy_len);
        }
    }
    
    // Copy encoded_crlf back as fixed array [u8; 16]
    if let Some(ref crlf) = rust_ctx.encoded_crlf {
        let crlf_bytes = crlf.as_bytes();
        let copy_len = std::cmp::min(crlf_bytes.len(), 15);
        c.encoded_crlf[..copy_len].copy_from_slice(&crlf_bytes[..copy_len]);
        c.encoded_crlf[copy_len] = 0; // Null terminate
    }
    
    // Copy encoded_br back as fixed array [u8; 16]
    if let Some(ref br) = rust_ctx.encoded_br {
        let br_bytes = br.as_bytes();
        let copy_len = std::cmp::min(br_bytes.len(), 15);
        c.encoded_br[..copy_len].copy_from_slice(&br_bytes[..copy_len]);
        c.encoded_br[copy_len] = 0; // Null terminate
    }
    
    // Copy other important fields
    c.encoded_crlf_length = rust_ctx.encoded_crlf_length;
    c.encoded_br_length = rust_ctx.encoded_br_length;
    c.new_sentence = rust_ctx.new_sentence;
    c.sbs_enabled = rust_ctx.sbs_enabled;
    c.segment_pending = rust_ctx.segment_pending;
    c.segment_last_key_frame = rust_ctx.segment_last_key_frame;
    
    // Note: We don't copy pointer fields back as they shouldn't change
    // during the encoding process and could cause memory issues
}
