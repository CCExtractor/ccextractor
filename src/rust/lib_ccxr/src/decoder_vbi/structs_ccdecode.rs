use crate::decoder_vbi::exit_codes::*;
use crate::decoder_vbi::structs_isdb::CcxDecoderVbiCtx;
use crate::decoder_vbi::structs_xds::CcSubtitle;
use crate::decoder_vbi::structs_xds::XdsBuffer;

use crate::time::Timestamp;

use std::ptr::null_mut;

pub struct LibCcDecode {
    pub cc_stats: [i32; 4],
    pub saw_caption_block: i32,
    pub processed_enough: i32, // If 1, we have enough lines, time, etc.

    /* 608 contexts - note that this shouldn't be global, they should be
     * per program */
    pub context_cc608_field_1: *mut std::ffi::c_void,
    pub context_cc608_field_2: *mut std::ffi::c_void,

    pub no_rollup: i32, // If 1, write one line at a time
    pub noscte20: i32,
    pub fix_padding: i32, // Replace 0000 with 8080 in HDTV (needed for some cards)
    pub write_format: crate::common::OutputFormat, // 0 = Raw, 1 = srt, 2 = SMI
    pub extraction_start: Option<Timestamp>,
    pub extraction_end: Option<Timestamp>, // Segment we actually process
    pub subs_delay: i64,                   // ms to delay (or advance) subs
    pub extract: i32,                      // Extract 1st, 2nd or both fields
    pub fullbin: i32,                      // Disable pruning of padding cc blocks
    // TODO when cc_subtitle completed
    // pub dec_sub: cc_subtitle,
    pub in_bufferdatatype: crate::common::BufferdataType,
    pub hauppauge_mode: u32, // If 1, use PID=1003, process specially and so on

    pub frames_since_last_gop: i32,
    /* GOP-based timing */
    pub saw_gop_header: i32,
    /* Time info for timed-transcript */
    pub max_gop_length: i32,  // (Maximum) length of a group of pictures
    pub last_gop_length: i32, // Length of the previous group of pictures
    pub total_pulldownfields: u32,
    pub total_pulldownframes: u32,
    pub program_number: i32,
    pub list: HList,
    pub timing: *mut crate::time::TimingContext,
    pub codec: crate::common::Codec, // Can also be SelectCodec

    // Set to true if data is buffered
    pub has_ccdata_buffered: i32,
    pub is_alloc: i32,

    pub avc_ctx: *mut AvcCtx,
    pub private_data: *mut std::ffi::c_void,

    /* General video information */
    pub current_hor_size: u32,
    pub current_vert_size: u32,
    pub current_aspect_ratio: u32,
    pub current_frame_rate: u32, // Assume standard fps, 29.97

    /* Required in es_function.c */
    pub no_bitstream_error: i32,
    pub saw_seqgoppic: i32,
    pub in_pic_data: i32,

    pub current_progressive_sequence: u32,
    pub current_pulldownfields: u32,

    pub temporal_reference: i32,
    pub picture_coding_type: crate::common::FrameType,
    pub num_key_frames: u32,
    pub picture_structure: u32,
    pub repeat_first_field: u32,
    pub progressive_frame: u32,
    pub pulldownfields: u32,

    /* Required in es_function.c and es_userdata.c */
    pub top_field_first: u32, // Needs to be global

    /* Stats. Modified in es_userdata.c */
    pub stat_numuserheaders: i32,
    pub stat_dvdccheaders: i32,
    pub stat_scte20ccheaders: i32,
    pub stat_replay5000headers: i32,
    pub stat_replay4000headers: i32,
    pub stat_dishheaders: i32,
    pub stat_hdtv: i32,
    pub stat_divicom: i32,
    pub false_pict_header: i32,
    // TODO when 708 completed
    // pub dtvcc: *mut DtvccCtx,
    pub current_field: i32,

    // Analyse/use the picture information
    pub maxtref: i32, // Use to remember the temporal reference number

    pub cc_data_count: [i32; SORTBUF],
    // Store fts;
    pub cc_fts: [i64; SORTBUF],
    // Store HD CC packets
    pub cc_data_pkts: [[u8; 10 * 31 * 3 + 1]; SORTBUF], // *10, because MP4 seems to have different limits

    // The sequence number of the current anchor frame. All currently read
    // B-Frames belong to this I- or P-frame.
    pub anchor_seq_number: i32,
    pub xds_ctx: *mut XdsContext,
    // TODO when vbi completed
    pub vbi_decoder: *mut CcxDecoderVbiCtx,

    // TODO when cc_subtitle completed
    pub writedata: Option<
        extern "C" fn(
            data: *const u8,
            length: i32,
            private_data: *mut std::ffi::c_void,
            sub: *mut CcSubtitle,
        ) -> i32,
    >,

    // DVB subtitle related
    pub ocr_quantmode: i32,
    pub prev: *mut LibCcDecode,
}
impl Default for LibCcDecode {
    fn default() -> Self {
        LibCcDecode {
            vbi_decoder: std::ptr::null_mut(),
            writedata: None,
            cc_stats: [0; 4],
            saw_caption_block: 0,
            processed_enough: 0,
            context_cc608_field_1: std::ptr::null_mut(),
            context_cc608_field_2: std::ptr::null_mut(),
            no_rollup: 0,
            noscte20: 0,
            fix_padding: 0,
            write_format: crate::common::OutputFormat::Raw,
            extraction_start: None,
            extraction_end: None,
            subs_delay: 0,
            extract: 0,
            fullbin: 0,
            in_bufferdatatype: crate::common::BufferdataType::Unknown,
            hauppauge_mode: 0,
            frames_since_last_gop: 0,
            saw_gop_header: 0,
            max_gop_length: 0,
            last_gop_length: 0,
            total_pulldownfields: 0,
            total_pulldownframes: 0,
            program_number: 0,
            list: HList::default(),
            timing: std::ptr::null_mut(),
            codec: crate::common::Codec::Dvb,
            has_ccdata_buffered: 0,
            is_alloc: 0,
            avc_ctx: std::ptr::null_mut(),
            private_data: std::ptr::null_mut(),
            current_hor_size: 0,
            current_vert_size: 0,
            current_aspect_ratio: 0,
            current_frame_rate: 0,
            no_bitstream_error: 0,
            saw_seqgoppic: 0,
            in_pic_data: 0,
            current_progressive_sequence: 0,
            current_pulldownfields: 0,
            temporal_reference: 0,
            picture_coding_type: crate::common::FrameType::ResetOrUnknown,
            num_key_frames: 0,
            picture_structure: 0,
            repeat_first_field: 0,
            progressive_frame: 0,
            pulldownfields: 0,
            top_field_first: 0,
            stat_numuserheaders: 0,
            stat_dvdccheaders: 0,
            stat_scte20ccheaders: 0,
            stat_replay5000headers: 0,
            stat_replay4000headers: 0,
            stat_dishheaders: 0,
            stat_hdtv: 0,
            stat_divicom: 0,
            false_pict_header: 0,
            current_field: 0,
            maxtref: 0,
            cc_data_count: [0; SORTBUF],
            cc_fts: [0; SORTBUF],
            cc_data_pkts: [[0; 10 * 31 * 3 + 1]; SORTBUF],
            anchor_seq_number: 0,
            xds_ctx: std::ptr::null_mut(),
            ocr_quantmode: 0,
            prev: std::ptr::null_mut(),
        }
    }
}

// HList (Hyperlinked List)
#[derive(Debug)]
pub struct HList {
    // A lot of the HList struct is not implemented yet
    pub next: *mut HList,
    pub prev: *mut HList,
}
impl Default for HList {
    fn default() -> Self {
        HList {
            next: null_mut(),
            prev: null_mut(),
        }
    }
}

pub struct AvcCtx {
    pub cc_count: u8,         // Number of closed caption blocks
    pub cc_data: *mut u8,     // Pointer to buffer holding CC data
    pub cc_databufsize: i64,  // Buffer size for CC data
    pub cc_buffer_saved: i32, // Was the CC buffer saved after the last update?

    pub got_seq_para: i32, // Flag indicating if sequence parameters were received
    pub nal_ref_idc: u32,  // NAL reference ID
    pub seq_parameter_set_id: i64, // Sequence parameter set ID
    pub log2_max_frame_num: i32, // Log2 of max frame number
    pub pic_order_cnt_type: i32, // Picture order count type
    pub log2_max_pic_order_cnt_lsb: i32, // Log2 of max picture order count LSB
    pub frame_mbs_only_flag: i32, // Flag indicating if only frame MBs are used

    // Use and throw stats for debugging (TODO: clean up later)
    pub num_nal_unit_type_7: i64,       // Number of NAL units of type 7
    pub num_vcl_hrd: i64,               // Number of VCL HRD parameters encountered
    pub num_nal_hrd: i64,               // Number of NAL HRD parameters encountered
    pub num_jump_in_frames: i64,        // Number of frame jumps detected
    pub num_unexpected_sei_length: i64, // Number of unexpected SEI lengths

    pub ccblocks_in_avc_total: i32, // Total CC blocks in AVC stream
    pub ccblocks_in_avc_lost: i32,  // Lost CC blocks in AVC stream

    pub frame_num: i64,     // Current frame number
    pub lastframe_num: i64, // Last processed frame number
    pub currref: i32,       // Current reference index
    pub maxidx: i32,        // Maximum index value for ordering
    pub lastmaxidx: i32,    // Last max index

    // Used to find tref zero in PTS mode
    pub minidx: i32,     // Minimum reference index
    pub lastminidx: i32, // Last minimum reference index

    // Used to remember the max temporal reference number (POC mode)
    pub maxtref: i32,          // Max temporal reference
    pub last_gop_maxtref: i32, // Last GOP max temporal reference

    // Used for PTS ordering of CC blocks
    pub currefpts: i64,              // Current reference PTS
    pub last_pic_order_cnt_lsb: i64, // Last picture order count LSB
    pub last_slice_pts: i64,         // Last slice PTS
}

pub struct XdsContext {
    // Program Identification Number (Start Time) for current program
    pub current_xds_min: i32,
    pub current_xds_hour: i32,
    pub current_xds_date: i32,
    pub current_xds_month: i32,
    pub current_program_type_reported: i32, // No.
    pub xds_start_time_shown: i32,
    pub xds_program_length_shown: i32,
    pub xds_program_description: [[char; 33]; 8], // Program descriptions (8 entries of 33 characters each)

    pub current_xds_network_name: [char; 33], // Network name
    pub current_xds_program_name: [char; 33], // Program name
    pub current_xds_call_letters: [char; 7],  // Call letters
    pub current_xds_program_type: [char; 33], // Program type

    pub xds_buffers: [XdsBuffer; NUM_XDS_BUFFERS as usize], // Array of XDS buffers
    pub cur_xds_buffer_idx: i32,                            // Current XDS buffer index
    pub cur_xds_packet_class: i32,                          // Current XDS packet class
    pub cur_xds_payload: *mut u8,                           // Pointer to the current XDS payload
    pub cur_xds_payload_length: i32,                        // Length of the current XDS payload
    pub cur_xds_packet_type: i32,                           // Current XDS packet type
    pub timing: *mut crate::time::TimingContext,            // Pointer to timing context

    pub current_ar_start: u32, // Current AR start time
    pub current_ar_end: u32,   // Current AR end time

    pub xds_write_to_file: i32, // Set to 1 if XDS data is to be written to a file
}
