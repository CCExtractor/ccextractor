#![allow(unexpected_cfgs)]
const NUM_BYTES_PER_PACKET: usize = 35; // Class + type (repeated for convenience) + data + zero
const NUM_XDS_BUFFERS: usize = 9; // CEA recommends no more than one level of interleaving. Play it safe

pub struct XdsBuffer {
    pub in_use: u32, // Indicates if the buffer is in use
    pub xds_class: i32, // XDS class
    pub xds_type: i32, // XDS type
    pub bytes: [u8; NUM_BYTES_PER_PACKET], // Class + type (repeated for convenience) + data + zero
    pub used_bytes: u8, // Number of bytes used in the buffer
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
    pub current_xds_call_letters: [char; 7], // Call letters
    pub current_xds_program_type: [char; 33], // Program type

    pub xds_buffers: [XdsBuffer; NUM_XDS_BUFFERS], // Array of XDS buffers
    pub cur_xds_buffer_idx: i32, // Current XDS buffer index
    pub cur_xds_packet_class: i32, // Current XDS packet class
    pub cur_xds_payload: *mut u8, // Pointer to the current XDS payload
    pub cur_xds_payload_length: i32, // Length of the current XDS payload
    pub cur_xds_packet_type: i32, // Current XDS packet type
    pub timing: *mut crate::time::TimingContext, // Pointer to timing context

    pub current_ar_start: u32, // Current AR start time
    pub current_ar_end: u32, // Current AR end time

    pub xds_write_to_file: i32, // Set to 1 if XDS data is to be written to a file
}