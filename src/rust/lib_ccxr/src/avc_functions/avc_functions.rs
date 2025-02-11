pub struct AvcCtx {
    pub cc_count: u8,  // Number of closed caption blocks
    pub cc_data: *mut u8,  // Pointer to buffer holding CC data
    pub cc_databufsize: i64,  // Buffer size for CC data
    pub cc_buffer_saved: i32,  // Was the CC buffer saved after the last update?

    pub got_seq_para: i32,  // Flag indicating if sequence parameters were received
    pub nal_ref_idc: u32,  // NAL reference ID
    pub seq_parameter_set_id: i64,  // Sequence parameter set ID
    pub log2_max_frame_num: i32,  // Log2 of max frame number
    pub pic_order_cnt_type: i32,  // Picture order count type
    pub log2_max_pic_order_cnt_lsb: i32,  // Log2 of max picture order count LSB
    pub frame_mbs_only_flag: i32,  // Flag indicating if only frame MBs are used

    // Use and throw stats for debugging (TODO: clean up later)
    pub num_nal_unit_type_7: i64,  // Number of NAL units of type 7
    pub num_vcl_hrd: i64,  // Number of VCL HRD parameters encountered
    pub num_nal_hrd: i64,  // Number of NAL HRD parameters encountered
    pub num_jump_in_frames: i64,  // Number of frame jumps detected
    pub num_unexpected_sei_length: i64,  // Number of unexpected SEI lengths

    pub ccblocks_in_avc_total: i32,  // Total CC blocks in AVC stream
    pub ccblocks_in_avc_lost: i32,  // Lost CC blocks in AVC stream

    pub frame_num: i64,  // Current frame number
    pub lastframe_num: i64,  // Last processed frame number
    pub currref: i32,  // Current reference index
    pub maxidx: i32,  // Maximum index value for ordering
    pub lastmaxidx: i32,  // Last max index

    // Used to find tref zero in PTS mode
    pub minidx: i32,  // Minimum reference index
    pub lastminidx: i32,  // Last minimum reference index

    // Used to remember the max temporal reference number (POC mode)
    pub maxtref: i32,  // Max temporal reference
    pub last_gop_maxtref: i32,  // Last GOP max temporal reference

    // Used for PTS ordering of CC blocks
    pub currefpts: i64,  // Current reference PTS
    pub last_pic_order_cnt_lsb: i64,  // Last picture order count LSB
    pub last_slice_pts: i64,  // Last slice PTS
}
