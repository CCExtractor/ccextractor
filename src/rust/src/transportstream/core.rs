use crate::bindings::*;
use crate::demuxer::common_structs::{CcxDemuxer, ProgramInfo, Stream_Type};
use crate::demuxer::demuxer_data::CCX_NOPTS;
use crate::demuxer::stream_functions::read_video_pes_header;
use crate::file_functions::file::buffered_read;
use crate::gxf_demuxer::common_structs::DemuxerError;
use crate::hlist::{cinfo_cremation, freep, get_cinfo};
use crate::transportstream::common_structs::{TSPayloadRust, HAUPPAGE_CCPID, RAI_MASK};
use crate::transportstream::epg_tables::parse_epg_packet;
use crate::transportstream::pmt::parse_pmt;
use crate::transportstream::tables::{parse_pat, parse_sdt, ts_buffer_psi_packet};
use lib_ccxr::common::{Options, BUFSIZE};
use lib_ccxr::time::Timestamp;
use lib_ccxr::util::log::{DebugMessageFlag, ExitCause};
use lib_ccxr::{debug, fatal, info};
use std::os::raw::c_void;
use std::ptr::null_mut;
use std::{ptr, slice};
/* Note
We are using demuxer_data and cap_info from lib_ccx from bindings,
because data.next_stream and cinfo.all_stream(and others) are linked lists,
and are impossible to copy to rust with good performance in mind.
So, when the core C library is made into rust, please make sure to update these structs to DemuxerData and CapInfo, respectively.
*/
// Return 1 for successfully read ts packet
/// # Safety
/// This function is unsafe because it calls `from_raw_parts_mut` and operates on raw file descriptors.
pub unsafe fn ts_readpacket(
    ctx: &mut CcxDemuxer,
    payload: &mut TSPayloadRust,
    ccx_options: &mut Options,
    tspacket: &mut [u8; 188],
) -> i32 {
    let mut adaptation_field_length: u32 = 0;

    let mut result: usize;

    if ctx.m2ts != 0 {
        /* M2TS just adds 4 bytes to each packet (so size goes from 188 to 192)
           The 4 bytes are not important to us, so just skip
        // TP_extra_header {
        Copy_permission_indicator 2  unimsbf
        Arrival_time_stamp 30 unimsbf
        } */
        let mut tp_extra_header = [0u8; 4];
        result = buffered_read(ctx, Some(&mut tp_extra_header), 4, ccx_options);
        ctx.past += result as i64;
        if result != 4 {
            if result > 0 {
                info!("Premature end of file (incomplete TS packer header, expected 4 bytes to skip M2TS extra bytes, got {}).\n", result);
            }
            return DemuxerError::EOF as i32;
        }
    }

    result = buffered_read(
        ctx,
        Some(slice::from_raw_parts_mut(tspacket.as_mut_ptr(), 188)),
        188,
        ccx_options,
    );
    ctx.past += result as i64;
    if result != 188 {
        if result > 0 {
            info!("Premature end of file - Transport Stream packet is incomplete (expected 188 bytes, got {}).\n", result);
        }
        return DemuxerError::EOF as i32;
    }

    let mut printtsprob = 1;
    while tspacket[0] != 0x47 {
        if printtsprob != 0 {
            debug!(msg_type = DebugMessageFlag::DUMP_DEF; "\nProblem: No TS header mark (filepos={}). Received bytes:\n", ctx.past);
            dump(tspacket.as_ptr(), 4, 0, 0);

            debug!(msg_type = DebugMessageFlag::DUMP_DEF; "Skip forward to the next TS header mark.\n");
            printtsprob = 0;
        }

        // The amount of bytes read into tspacket
        let tslen = 188;

        // Check for 0x47 in the remaining bytes of tspacket
        let tstemp = std::slice::from_raw_parts(tspacket.as_ptr().add(1), tslen - 1)
            .iter()
            .position(|&x| x == 0x47);

        if let Some(pos) = tstemp {
            // Found it
            let atpos = pos + 1; // +1 because we started searching from index 1

            // memmove equivalent
            ptr::copy(
                tspacket.as_ptr().add(atpos),
                tspacket.as_mut_ptr(),
                tslen - atpos,
            );

            let remaining_slice =
                slice::from_raw_parts_mut(tspacket.as_mut_ptr().add(tslen - atpos), atpos);
            result = buffered_read(ctx, Some(remaining_slice), atpos, ccx_options);
            ctx.past += result as i64;
            if result != atpos {
                info!("Premature end of file!\n");
                return DemuxerError::EOF as i32;
            }
        } else {
            // Read the next 188 bytes.
            result = buffered_read(
                ctx,
                Some(slice::from_raw_parts_mut(tspacket.as_mut_ptr(), tslen)),
                tslen,
                ccx_options,
            );
            ctx.past += result as i64;
            if result != tslen {
                info!("Premature end of file!\n");
                return DemuxerError::EOF as i32;
            }
        }
    }

    payload.transport_error = ((tspacket[1] & 0x80) >> 7) as i32;
    payload.pesstart = ((tspacket[1] & 0x40) >> 6) as usize;
    // unsigned transport_priority = (tspacket[1]&0x20)>>5;
    payload.pid = (((tspacket[1] & 0x1F) as u32) << 8 | tspacket[2] as u32) & 0x1FFF;
    // unsigned transport_scrambling_control = (tspacket[3]&0xC0)>>6;
    let adaptation_field_control: u32 = ((tspacket[3] & 0x30) >> 4) as u32;
    payload.counter = (tspacket[3] & 0xF) as i32;

    if payload.transport_error != 0 {
        debug!(msg_type = DebugMessageFlag::DUMP_DEF; "Warning: Defective (error indicator on) TS packet (filepos={}):\n", ctx.past);
        dump(tspacket.as_ptr(), 188, 0, 0);
    }

    payload.start = tspacket.as_mut_ptr().add(4);
    payload.length = 188 - 4;

    if (adaptation_field_control & 2) != 0 {
        // Take the PCR (Program Clock Reference) from here, in case PTS is not available (copied from telxcc).
        adaptation_field_length = tspacket[4] as u32;

        payload.have_pcr = ((tspacket[5] & 0x10) >> 4) as i32;
        if payload.have_pcr != 0 {
            payload.pcr = 0;
            payload.pcr |= (tspacket[6] as i64) << 25;
            payload.pcr |= (tspacket[7] as i64) << 17;
            payload.pcr |= (tspacket[8] as i64) << 9;
            payload.pcr |= (tspacket[9] as i64) << 1;
            payload.pcr |= (tspacket[10] as i64) >> 7;
            /* Ignore 27 Mhz clock since we dont deal in nanoseconds*/
            // payload.pcr = ((tspacket[10] & 0x01) << 8);
            // payload.pcr |= tspacket[11];
        }

        payload.has_random_access_indicator = if (tspacket[5] & RAI_MASK) != 0 { 1 } else { 0 };

        // Catch bad packages with adaptation_field_length > 184 and
        // the unsigned nature of payload_length leading to huge numbers.
        if adaptation_field_length < payload.length as u32 {
            payload.start = payload.start.add((adaptation_field_length + 1) as usize);
            payload.length -= (adaptation_field_length + 1) as usize;
        } else {
            // This renders the package invalid
            payload.length = 0;
            debug!(msg_type = DebugMessageFlag::PARSE; "  Reject package - set length to zero.\n");
        }
    } else {
        payload.has_random_access_indicator = 0;
    }

    debug!(msg_type = DebugMessageFlag::PARSE; "TS pid: {}  PES start: {}  counter: {}  payload length: {}  adapt length: {}\n",
          payload.pid, payload.pesstart, payload.counter, payload.length,
          adaptation_field_length as i32);

    if payload.length == 0 {
        debug!(msg_type = DebugMessageFlag::PARSE; "  No payload in package.\n");
    }
    // Store packet information
    0
}
/// # Safety
/// This function is unsafe because it may dereference a raw pointer.
pub unsafe fn dump(start: *const u8, l: i32, abs_start: u64, clear_high_bit: u32) {
    let data = slice::from_raw_parts(start, l as usize);

    let mut x = 0;
    while x < l {
        info!("{:08} | ", x + abs_start as i32);

        for j in 0..16 {
            if x + j < l {
                info!("{:02X} ", data[(x + j) as usize]);
            } else {
                info!("   ");
            }
        }
        info!(" | ");

        for j in 0..16 {
            if x + j < l && data[(x + j) as usize] >= b' ' {
                let ch = data[(x + j) as usize] & (if clear_high_bit != 0 { 0x7F } else { 0xFF });
                info!("{}", ch as char);
            } else {
                info!(" ");
            }
        }
        info!("\n");
        x += 16;
    }
}
/// # Safety
/// This function is unsafe because it may dereference a raw pointer.
pub unsafe fn get_pts_ts(buffer: *mut u8) -> u64 {
    // Safety guard: check for null pointer
    if buffer.is_null() {
        return u64::MAX;
    }

    let mut optional_pes_header_included: u8 = 0;
    let mut optional_pes_header_length: u16 = 0;
    let mut pts: u64;

    // Create a slice from the raw pointer with bounds checking
    // We need at least 14 bytes to safely read all the data this function accesses
    let buffer_slice = slice::from_raw_parts(buffer, 14);

    // Packetized Elementary Stream (PES) 32-bit start code
    let pes_prefix: u64 = ((buffer_slice[0] as u64) << 16)
        | ((buffer_slice[1] as u64) << 8)
        | (buffer_slice[2] as u64);

    // check for PES header
    if pes_prefix == 0x000001 {
        // optional PES header marker bits (10.. ....)
        if (buffer_slice[6] & 0xc0) == 0x80 {
            optional_pes_header_included = 1;
            optional_pes_header_length = buffer_slice[8] as u16;
        }

        if optional_pes_header_included == 1
            && optional_pes_header_length > 0
            && (buffer_slice[7] & 0x80) > 0
        {
            // get associated PTS as it exists
            pts = (buffer_slice[9] & 0x0e) as u64;
            pts <<= 29;
            pts |= (buffer_slice[10] as u64) << 22;
            pts |= ((buffer_slice[11] & 0xfe) as u64) << 14;
            pts |= (buffer_slice[12] as u64) << 7;
            pts |= ((buffer_slice[13] & 0xfe) as u64) >> 1;
            return pts;
        }
    }
    u64::MAX
}
/// Delete demuxer data nodes by PID
/// # Safety
/// This function is unsafe because it manipulates raw pointers and deallocates memory
pub unsafe fn delete_demuxer_data_node_by_pid(data: *mut *mut demuxer_data, pid: i32) {
    let mut ptr = *data;
    let mut sptr: *mut demuxer_data = null_mut();

    while !ptr.is_null() {
        if (*ptr).stream_pid == pid {
            // Remove node from linked list
            if sptr.is_null() {
                *data = (*ptr).next_stream;
            } else {
                (*sptr).next_stream = (*ptr).next_stream;
            }

            // Delete the node
            delete_demuxer_data(ptr);
            ptr = null_mut();
        } else {
            sptr = ptr;
            ptr = (*ptr).next_stream;
        }
    }
}
/// Delete a single demuxer data node
/// # Safety
/// This function is unsafe because it deallocates raw pointers
pub unsafe fn delete_demuxer_data(data: *mut demuxer_data) {
    if data.is_null() {
        return;
    }

    // Free the buffer if it exists
    if !(*data).buffer.is_null() {
        let _ = Box::from_raw((*data).buffer);
    }

    // Free the demuxer_data struct itself
    let _ = Box::from_raw(data);
}

/// Dump PES header information from buffer
/// # Safety
/// This function is unsafe because it dereferences raw pointers
pub unsafe fn pes_header_dump(buffer: *mut u8, len: i64, LAST_PTS: &mut u64) {
    let len = len as usize;

    // Check if we have at least some data to work with
    if len < 6 || buffer.is_null() {
        return;
    }

    let buffer_slice = std::slice::from_raw_parts(buffer, len);

    // Packetized Elementary Stream (PES) 32-bit start code
    let pes_prefix = ((buffer_slice[0] as u64) << 16)
        | ((buffer_slice[1] as u64) << 8)
        | (buffer_slice[2] as u64);
    let pes_stream_id = buffer_slice[3];

    // Check for PES header
    if pes_prefix != 0x000001 {
        return;
    }

    let pes_packet_length = 6 + (((buffer_slice[4] as u16) << 8) | (buffer_slice[5] as u16));

    print!("Packet start code prefix: {pes_prefix:04x} # ");
    print!("Stream ID: {pes_stream_id:04x} # ");
    print!("Packet length: {pes_packet_length} ");

    if pes_packet_length == 6 {
        // Great, there is only a header and no extension + payload
        println!();
        return;
    }

    let mut optional_pes_header_included = false;
    let mut optional_pes_header_length = 0u8;

    // Optional PES header marker bits (10.. ....)
    if len > 6 && (buffer_slice[6] & 0xc0) == 0x80 {
        optional_pes_header_included = true;
        if len > 8 {
            optional_pes_header_length = buffer_slice[8];
        }
    }

    if optional_pes_header_included
        && optional_pes_header_length > 0
        && len > 7
        && (buffer_slice[7] & 0x80) > 0
        && len >= 14
    {
        // Get associated PTS as it exists
        let mut pts = (buffer_slice[9] & 0x0e) as u64;
        pts <<= 29;
        pts |= (buffer_slice[10] as u64) << 22;
        pts |= ((buffer_slice[11] & 0xfe) as u64) << 14;
        pts |= (buffer_slice[12] as u64) << 7;
        pts |= ((buffer_slice[13] & 0xfe) as u64) >> 1;

        print!("# Associated PTS: {pts} # ");
        println!("Diff: {}", pts.wrapping_sub(*LAST_PTS));
        *LAST_PTS = pts;
    } else {
        println!();
    }
}
/// Search for existing demuxer data node by PID or allocate a new one
/// # Safety  
/// This function is unsafe because it manipulates raw pointers and allocates memory
pub unsafe fn search_or_alloc_demuxer_data_node_by_pid(
    data: *mut *mut demuxer_data,
    pid: i32,
) -> *mut demuxer_data {
    // If no data exists, allocate first node
    if (*data).is_null() {
        *data = alloc_demuxer_data();
        if (*data).is_null() {
            return null_mut();
        }

        (**data).program_number = -1;
        (**data).stream_pid = pid;
        (**data).bufferdatatype = ccx_bufferdata_type_CCX_UNKNOWN;
        (**data).len = 0;
        (**data).next_program = null_mut();
        (**data).next_stream = null_mut();
        return *data;
    }

    let mut ptr = *data;
    let mut sptr: *mut demuxer_data;

    loop {
        // Check if current node matches PID
        if (*ptr).stream_pid == pid {
            return ptr;
        }

        sptr = ptr;
        ptr = (*ptr).next_stream;

        // Break if we've reached the end
        if ptr.is_null() {
            break;
        }
    }

    // Allocate new node at end of list
    (*sptr).next_stream = alloc_demuxer_data();
    ptr = (*sptr).next_stream;

    if ptr.is_null() {
        return null_mut();
    }

    (*ptr).program_number = -1;
    (*ptr).stream_pid = pid;
    (*ptr).bufferdatatype = ccx_bufferdata_type_CCX_UNKNOWN;
    (*ptr).len = 0;
    (*ptr).next_program = null_mut();
    (*ptr).next_stream = null_mut();

    ptr
}

/// Allocate a new demuxer data structure
/// # Safety
/// This function is unsafe because it allocates raw pointers that need proper management
pub unsafe fn alloc_demuxer_data() -> *mut demuxer_data {
    // Allocate the main structure
    let data_box = Box::new(demuxer_data {
        program_number: -1,
        stream_pid: -1,
        codec: ccx_code_type_CCX_CODEC_NONE,
        bufferdatatype: ccx_bufferdata_type_CCX_PES,
        buffer: null_mut(),
        len: 0,
        rollover_bits: 0,
        pts: CCX_NOPTS,
        tb: ccx_rational { num: 1, den: 90000 },
        next_stream: null_mut(),
        next_program: null_mut(),
    });

    let data = Box::into_raw(data_box);

    // Allocate the buffer
    let buffer = vec![0u8; BUFSIZE].into_boxed_slice();
    (*data).buffer = Box::into_raw(buffer) as *mut u8;

    data
}
/// Get buffer type based on cap_info stream and codec
pub fn get_buffer_type(cinfo: &cap_info, ccx_options: &mut Options) -> ccx_bufferdata_type {
    // turn to StreamType after lib_ccx
    if cinfo.stream == ccx_stream_type_CCX_STREAM_TYPE_VIDEO_MPEG2 {
        ccx_bufferdata_type_CCX_PES
    } else if cinfo.stream == ccx_stream_type_CCX_STREAM_TYPE_VIDEO_H264 {
        ccx_bufferdata_type_CCX_H264
    } else if cinfo.stream == ccx_stream_type_CCX_STREAM_TYPE_PRIVATE_MPEG2
        && cinfo.codec == ccx_code_type_CCX_CODEC_DVB
    {
        ccx_bufferdata_type_CCX_DVB_SUBTITLE
    } else if cinfo.stream == ccx_stream_type_CCX_STREAM_TYPE_PRIVATE_MPEG2
        && cinfo.codec == ccx_code_type_CCX_CODEC_ISDB_CC
    {
        ccx_bufferdata_type_CCX_ISDB_SUBTITLE
    } else if cinfo.stream == ccx_stream_type_CCX_STREAM_TYPE_UNKNOWNSTREAM
        && ccx_options.hauppauge_mode
    {
        ccx_bufferdata_type_CCX_HAUPPAGE
    } else if cinfo.stream == ccx_stream_type_CCX_STREAM_TYPE_PRIVATE_MPEG2
        && cinfo.codec == ccx_code_type_CCX_CODEC_TELETEXT
    {
        ccx_bufferdata_type_CCX_TELETEXT
    } else if cinfo.stream == ccx_stream_type_CCX_STREAM_TYPE_PRIVATE_MPEG2
        && cinfo.codec == ccx_code_type_CCX_CODEC_ATSC_CC
    {
        ccx_bufferdata_type_CCX_PRIVATE_MPEG2_CC
    } else if cinfo.stream == ccx_stream_type_CCX_STREAM_TYPE_PRIVATE_USER_MPEG2
        && cinfo.codec == ccx_code_type_CCX_CODEC_ATSC_CC
    {
        ccx_bufferdata_type_CCX_PES
    } else {
        DemuxerError::InvalidArgument as _
    }
}

/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls unsafe functions like `search_or_alloc_demuxer_data_node_by_pid`.
pub unsafe fn copy_capbuf_demux_data(
    ctx: &mut CcxDemuxer,
    data: *mut *mut demuxer_data,
    cinfo: &mut cap_info,
    ccx_options: &mut Options,
    haup_capbuf: *mut u8,
    haup_capbuflen: &mut i32,
    last_pts: &mut u64,
) -> i32 {
    let mut pesheaderlen: i32 = 0;

    let ptr: *mut demuxer_data = search_or_alloc_demuxer_data_node_by_pid(data, cinfo.pid);
    (*ptr).program_number = cinfo.program_number;
    (*ptr).codec = cinfo.codec;
    (*ptr).bufferdatatype = get_buffer_type(cinfo, ccx_options);

    if cinfo.capbuf.is_null() || cinfo.capbuflen == 0 {
        return -1;
    }

    if (*ptr).bufferdatatype == ccx_bufferdata_type_CCX_PRIVATE_MPEG2_CC {
        dump(cinfo.capbuf, cinfo.capbuflen as i32, 0, 1);
        // Bogus data, so we return something
        *(*ptr).buffer.add((*ptr).len) = 0xFA;
        (*ptr).len += 1;
        *(*ptr).buffer.add((*ptr).len) = 0x80;
        (*ptr).len += 1;
        *(*ptr).buffer.add((*ptr).len) = 0x80;
        (*ptr).len += 1;
        return 0; // CCX_OK
    }

    if cinfo.codec == ccx_code_type_CCX_CODEC_TELETEXT {
        ptr::copy_nonoverlapping(
            cinfo.capbuf,
            (*ptr).buffer.add((*ptr).len),
            cinfo.capbuflen as usize,
        );
        (*ptr).len += cinfo.capbuflen as usize;
        return 0; // CCX_OK
    }

    let capbuf_slice = slice::from_raw_parts_mut(cinfo.capbuf, cinfo.capbuflen as usize);
    let vpesdatalen: i32 = read_video_pes_header(
        ctx,
        ptr,
        capbuf_slice,
        &mut pesheaderlen as *mut i32,
        cinfo.capbuflen as i32,
        ccx_options,
    );

    if ccx_options.pes_header_to_stdout && cinfo.codec == ccx_code_type_CCX_CODEC_DVB {
        // for teletext we have its own header dump
        pes_header_dump(cinfo.capbuf, pesheaderlen as i64, last_pts);
    }

    if vpesdatalen < 0 {
        debug!(msg_type=DebugMessageFlag::VERBOSE; "Seems to be a broken PES. Terminating file handling.\n");
        return DemuxerError::EOF as _;
    }

    if ccx_options.hauppauge_mode {
        if *haup_capbuflen % 12 != 0 {
            info!("Warning: Inconsistent Hauppage's buffer length\n");
        }
        if *haup_capbuflen == 0 {
            // Do this so that we always return something until EOF. This will be skipped.
            *(*ptr).buffer.add((*ptr).len) = 0xFA;
            (*ptr).len += 1;
            *(*ptr).buffer.add((*ptr).len) = 0x80;
            (*ptr).len += 1;
            *(*ptr).buffer.add((*ptr).len) = 0x80;
            (*ptr).len += 1;
        }

        let mut i = 0;
        while !haup_capbuf.is_null() && i < *haup_capbuflen {
            let haup_stream_id = *haup_capbuf.add((i + 3) as usize);
            if haup_stream_id == 0xbd
                && *haup_capbuf.add((i + 4) as usize) == 0
                && *haup_capbuf.add((i + 5) as usize) == 6
            {
                // Because I (CFS) don't have a lot of samples for this, for now I make sure everything is like the one I have:
                // 12 bytes total length, stream id = 0xbd (Private non-video and non-audio), etc
                if 2 > BUFSIZE - (*ptr).len {
                    fatal!(
                        cause = ExitCause::Bug;
                        "Remaining buffer ({}) not enough to hold the 3 Hauppage bytes.\nPlease send bug report!",
                        BUFSIZE - (*ptr).len
                    );
                }
                let field_byte = *haup_capbuf.add((i + 9) as usize);
                if field_byte == 1 || field_byte == 2 {
                    // Field match. // TODO: If extract==12 this won't work!
                    if field_byte == 1 {
                        *(*ptr).buffer.add((*ptr).len) = 4; // Field 1 + cc_valid=1
                    } else {
                        *(*ptr).buffer.add((*ptr).len) = 5; // Field 2 + cc_valid=1
                    }
                    (*ptr).len += 1;
                    *(*ptr).buffer.add((*ptr).len) = *haup_capbuf.add((i + 10) as usize);
                    (*ptr).len += 1;
                    *(*ptr).buffer.add((*ptr).len) = *haup_capbuf.add((i + 11) as usize);
                    (*ptr).len += 1;
                }
                /*
                if (inbuf>1024) // Just a way to send the bytes to the decoder from time to time, otherwise the buffer will fill up.
                break;
                else
                continue; */
            }
            i += 12;
        }
        *haup_capbuflen = 0;
    }

    let databuf: *mut u8 = cinfo.capbuf.add(pesheaderlen as usize);
    let databuflen: i64 = cinfo.capbuflen - pesheaderlen as i64;

    if !ccx_options.hauppauge_mode {
        // in Haup mode the buffer is filled somewhere else
        if (*ptr).len + databuflen as usize >= BUFSIZE {
            fatal!(
                cause = ExitCause::Bug;
                "PES data packet ({}) larger than remaining buffer ({}).\nPlease send bug report!",
                databuflen,
                BUFSIZE - (*ptr).len
            );
        }
        ptr::copy_nonoverlapping(databuf, (*ptr).buffer.add((*ptr).len), databuflen as usize);
        (*ptr).len += databuflen as usize;
    }

    0 // CCX_OK
}
pub fn look_for_caption_data(ctx: &mut CcxDemuxer, payload: &TSPayloadRust) {
    let mut i: usize = 0;

    // See if we find the usual CC data marker (GA94) in this packet.
    if payload.length < 4 || ctx.pids_seen[payload.pid as usize] == 3 {
        // Second thing means we already inspected this PID
        return;
    }

    while i < (payload.length - 3) {
        // Create a slice from the raw pointer to safely access the data
        let data_slice = unsafe { slice::from_raw_parts(payload.start, payload.length) };

        if data_slice[i] == b'G'
            && data_slice[i + 1] == b'A'
            && data_slice[i + 2] == b'9'
            && data_slice[i + 3] == b'4'
        {
            info!("PID {} seems to contain CEA-608 captions.\n\0", payload.pid);
            ctx.pids_seen[payload.pid as usize] = 3;
            return;
        }
        i += 1;
    }
}
/// # Safety
/// This function is unsafe because it manipulates raw pointers and may cause memory issues if not used correctly.
pub unsafe fn copy_payload_to_capbuf(
    cinfo: &mut cap_info,
    payload: &TSPayloadRust,
    ccx_options: &mut Options,
) -> i32 {
    if cinfo.ignore == 1
        && (cinfo.stream != ccx_stream_type_CCX_STREAM_TYPE_VIDEO_MPEG2
            || !ccx_options.analyze_video_stream)
    {
        return 0;
    }

    if cinfo.capbuflen == 0 {
        let data_slice = slice::from_raw_parts(payload.start, payload.length);

        if data_slice[0] != 0x00 || data_slice[1] != 0x00 || data_slice[2] != 0x01 {
            info!("Notice: Missing PES header");
            dump(payload.start, payload.length as i32, 0, 0);
            cinfo.saw_pesstart = 0;
            return -1;
        }
    }

    let newcapbuflen: i64 = cinfo.capbuflen + payload.length as i64;
    if newcapbuflen > cinfo.capbufsize {
        let old_size = cinfo.capbufsize as usize;
        let new_size = newcapbuflen as usize;

        let mut new_vec = Vec::<u8>::with_capacity(new_size);

        if !cinfo.capbuf.is_null() && old_size > 0 {
            let old_slice = slice::from_raw_parts(cinfo.capbuf, cinfo.capbuflen as usize);
            new_vec.extend_from_slice(old_slice);
        }

        let new_ptr = new_vec.as_mut_ptr();
        std::mem::forget(new_vec);
        cinfo.capbuf = new_ptr;
        cinfo.capbufsize = newcapbuflen;
    }

    // Copy payload data to capbuf using pure Rust
    copy_memory(
        cinfo.capbuf.add(cinfo.capbuflen as usize),
        payload.start,
        payload.length,
    );
    cinfo.capbuflen = newcapbuflen;

    0
}
unsafe fn copy_memory(dest: *mut u8, src: *const u8, count: usize) {
    // very primitive and unsafe implementation, replace with a better one on lib_ccx transform
    for i in 0..count {
        *dest.add(i) = *src.add(i);
    }
}
/// # Safety
/// This function is unsafe because it manipulates raw pointers and calls unsafe functions like `ts_readpacket`
#[allow(clippy::too_many_arguments)]
pub unsafe fn ts_readstream(
    ctx: &mut CcxDemuxer,
    data: *mut *mut demuxer_data,
    cap: &mut cap_info, // This should be the tree root, not a single cap_info
    ccx_options: &mut Options,
    haup_capbuf: *mut *mut u8,
    haup_capbufsize: &mut i32,
    haup_capbuflen: &mut i32,
    last_pts: &mut u64,
    tspacket: &mut [u8; 188],
) -> i64 {
    let mut gotpes = 0;
    let mut pespcount: i64 = 0;
    let mut pcount: i64 = 0;
    let packet_analysis_mode = 0;
    let mut ret;
    let mut pinfo: *mut ProgramInfo = std::ptr::null_mut();
    let mut cinfo: *mut cap_info;
    let mut payload = TSPayloadRust::default();

    loop {
        pcount += 1;

        // Exit the loop at EOF
        ret = ts_readpacket(ctx, &mut payload, ccx_options, tspacket);
        if ret != 0 {
            break;
        }

        // Skip damaged packets
        if payload.transport_error != 0 {
            debug!(msg_type = DebugMessageFlag::VERBOSE;
                "Packet (pid {}) skipped - transport error.\n", payload.pid);
            continue;
        }

        // Check for PAT
        if payload.pid == 0 {
            ts_buffer_psi_packet(ctx, tspacket);
            if !ctx.pid_buffers[payload.pid as usize].is_null()
                && (*ctx.pid_buffers[payload.pid as usize]).buffer_length > 0
            {
                parse_pat(ctx, ccx_options, cap);
            }
            continue;
        }

        if ccx_options.xmltv >= 1 && payload.pid == 0x11 {
            ts_buffer_psi_packet(ctx, tspacket);
            if !ctx.pid_buffers[payload.pid as usize].is_null()
                && (*ctx.pid_buffers[payload.pid as usize]).buffer_length > 0
            {
                parse_sdt(ctx);
            }
        }

        if ccx_options.xmltv >= 1 && payload.pid == 0x12 {
            parse_epg_packet(ctx.parent.as_mut().unwrap(), tspacket, ccx_options);
        }
        if ccx_options.xmltv >= 1 && payload.pid >= 0x1000 {
            parse_epg_packet(ctx.parent.as_mut().unwrap(), tspacket, ccx_options);
        }

        // PCR and PMT handling
        let mut j = 0;
        while j < ctx.nb_program {
            if ctx.pinfo[j].analysed_pmt_once == 1
                && ctx.pinfo[j].pcr_pid == payload.pid as i16
                && payload.have_pcr != 0
            {
                ctx.last_global_timestamp = ctx.global_timestamp;
                ctx.global_timestamp = Timestamp::from_millis(payload.pcr / 90);
                if ctx.global_timestamp_inited.millis() == 0 {
                    ctx.min_global_timestamp = ctx.global_timestamp;
                    ctx.global_timestamp_inited = Timestamp::from_millis(1);
                }
                if ctx.min_global_timestamp > ctx.global_timestamp {
                    ctx.offset_global_timestamp =
                        ctx.last_global_timestamp - ctx.min_global_timestamp;
                    ctx.min_global_timestamp = ctx.global_timestamp;
                }
            }
            if ctx.pinfo[j].pid == payload.pid as i32 {
                if ctx.pids_seen[payload.pid as usize] == 0 {
                    debug!(msg_type = DebugMessageFlag::PAT;
                        "This PID ({}) is a PMT for program {}.\n", 
                        payload.pid, ctx.pinfo[j].program_number);
                }
                pinfo = &mut ctx.pinfo[j] as *mut ProgramInfo;
                break;
            }
            j += 1;
        }

        // PMT processing
        if j != ctx.nb_program {
            ctx.pids_seen[payload.pid as usize] = 2;
            ts_buffer_psi_packet(ctx, tspacket);
            if !ctx.pid_buffers[payload.pid as usize].is_null()
                && (*ctx.pid_buffers[payload.pid as usize]).buffer_length > 0
            {
                let buffer_ptr = (*ctx.pid_buffers[payload.pid as usize]).buffer.add(1);
                let buffer_len =
                    ((*ctx.pid_buffers[payload.pid as usize]).buffer_length - 1) as i32;
                let buffer_slice = std::slice::from_raw_parts_mut(buffer_ptr, buffer_len as usize);
                if parse_pmt(
                    ctx,
                    buffer_slice,
                    buffer_len as usize,
                    &mut *pinfo,
                    ccx_options,
                    cap,
                ) != 0
                {
                    gotpes = 1;
                }
            }
            continue;
        }

        // PID tracking
        match ctx.pids_seen[payload.pid as usize] {
            0 => {
                if payload.pid < ctx.pids_programs.len() as u32
                    && !ctx.pids_programs[payload.pid as usize].is_null()
                {
                    debug!(msg_type = DebugMessageFlag::PARSE;
                        "\nNew PID found: {} ({}), belongs to program: {}\n", 
                        payload.pid,
                        "stream_type", // You'll need to get the actual stream type description
                        (*ctx.pids_programs[payload.pid as usize]).program_number);
                    ctx.pids_seen[payload.pid as usize] = 2;
                } else {
                    debug!(msg_type = DebugMessageFlag::PARSE; 
                        "\nNew PID found: {}, program number still unknown\n", 
                        payload.pid);
                    ctx.pids_seen[payload.pid as usize] = 1;
                }
                if (ctx.num_of_pids as usize) < ctx.have_pids.len() {
                    ctx.have_pids[ctx.num_of_pids as usize] = payload.pid as i32;
                    ctx.num_of_pids += 1;
                }
            }
            1 => {
                if payload.pid < ctx.pids_programs.len() as u32
                    && !ctx.pids_programs[payload.pid as usize].is_null()
                {
                    debug!(msg_type = DebugMessageFlag::PARSE;
                        "\nProgram for PID: {} (previously unknown) is: {} ({})\n", 
                        payload.pid,
                        (*ctx.pids_programs[payload.pid as usize]).program_number,
                        "stream_type");
                    ctx.pids_seen[payload.pid as usize] = 2;
                }
            }
            2 | 3 => {
                // Already processed
            }
            _ => {}
        }

        // PTS calculation
        if payload.pesstart != 0 {
            let data_slice =
                std::slice::from_raw_parts(payload.start, std::cmp::min(payload.length, 4));
            if data_slice.len() >= 4 {
                let pes_prefix = ((data_slice[0] as u64) << 16)
                    | ((data_slice[1] as u64) << 8)
                    | (data_slice[2] as u64);
                let pes_stream_id = data_slice[3];

                if pes_prefix == 0x000001 {
                    let pts = get_pts_ts(payload.start);
                    let mut pid_index = 0;
                    for i in 0..ctx.num_of_pids {
                        if payload.pid as i32 == ctx.have_pids[i as usize] {
                            pid_index = i;
                            break;
                        }
                    }
                    if (pid_index as usize) < ctx.stream_id_of_each_pid.len() {
                        ctx.stream_id_of_each_pid[pid_index as usize] = pes_stream_id;
                        if pts < ctx.min_pts[pid_index as usize] {
                            ctx.min_pts[pid_index as usize] = pts;
                        }
                    }
                }
            }
        }

        // Skip null packets
        if payload.pid == 8191 {
            continue;
        }

        // Hauppauge warning
        if payload.pid == 1003 && !ctx.hauppauge_warning_shown && !ccx_options.hauppauge_mode {
            info!("\n\nNote: This TS could be a recording from a Hauppage card. If no captions are detected, try --hauppauge\n\n");
            ctx.hauppauge_warning_shown = true;
        }

        // Hauppauge mode processing
        if ccx_options.hauppauge_mode && payload.pid == HAUPPAGE_CCPID {
            let haup_newcapbuflen = *haup_capbuflen + payload.length as i32;
            if haup_newcapbuflen > *haup_capbufsize {
                let new_ptr = realloc(*haup_capbuf as *mut c_void, haup_newcapbuflen as _);
                if new_ptr.is_null() {
                    fatal!(cause = ExitCause::Bug; "Not enough memory to store hauppauge packets");
                }
                *haup_capbuf = new_ptr as *mut u8;
                *haup_capbufsize = haup_newcapbuflen;
            }
            if !(*haup_capbuf).is_null() {
                std::ptr::copy_nonoverlapping(
                    payload.start,
                    (*haup_capbuf).add(*haup_capbuflen as usize),
                    payload.length,
                );
            }
            *haup_capbuflen = haup_newcapbuflen;
        }

        // Skip empty payloads
        if payload.length == 0 {
            debug!(msg_type = DebugMessageFlag::VERBOSE;
                "Packet (pid {}) skipped - no payload.\n", payload.pid);
            continue;
        }

        // Get caption info for this PID
        cinfo = get_cinfo(cap, payload.pid as i32);
        if cinfo.is_null() {
            if packet_analysis_mode == 0 {
                debug!(msg_type = DebugMessageFlag::PARSE;
                    "Packet (pid {}) skipped - no stream with captions identified yet.\n", payload.pid);
            } else {
                look_for_caption_data(ctx, &payload);
            }
            continue;
        } else if (*cinfo).ignore == 1
            && ((*cinfo).stream != ccx_stream_type_CCX_STREAM_TYPE_VIDEO_MPEG2
                || !ccx_options.analyze_video_stream)
        {
            // Clean up ignored streams
            if !(*cinfo).codec_private_data.is_null() {
                match (*cinfo).codec {
                    ccx_code_type_CCX_CODEC_TELETEXT => {
                        telxcc_close(&mut (*cinfo).codec_private_data, std::ptr::null_mut());
                    }
                    ccx_code_type_CCX_CODEC_DVB => {
                        dvbsub_close_decoder(&mut (*cinfo).codec_private_data);
                    }
                    ccx_code_type_CCX_CODEC_ISDB_CC => {
                        delete_isdb_decoder(&mut (*cinfo).codec_private_data);
                    }
                    _ => {}
                }
                (*cinfo).codec_private_data = std::ptr::null_mut();
            }

            if (*cinfo).capbuflen > 0 {
                freep(&mut (*cinfo).capbuf);
                (*cinfo).capbufsize = 0;
                (*cinfo).capbuflen = 0;
                delete_demuxer_data_node_by_pid(data, (*cinfo).pid);
            }
            continue;
        }

        // Video PES start
        if payload.pesstart != 0 {
            (*cinfo).saw_pesstart = 1;
            (*cinfo).prev_counter = payload.counter.wrapping_sub(1);
        }

        // Discard packets when no pesstart was found
        if (*cinfo).saw_pesstart == 0 {
            continue;
        }

        // Check continuity counter
        let expected_counter = if (*cinfo).prev_counter == 15 {
            0
        } else {
            (*cinfo).prev_counter + 1
        };
        if expected_counter != payload.counter {
            info!(
                "TS continuity counter not incremented prev/curr {}/{}\n",
                (*cinfo).prev_counter,
                payload.counter
            );
        }
        (*cinfo).prev_counter = payload.counter;

        // Process complete PES
        if payload.pesstart != 0 && (*cinfo).capbuflen > 0 {
            debug!(msg_type = DebugMessageFlag::PARSE;
                "\nPES finished ({} bytes/{} PES packets/{} total packets)\n",
                (*cinfo).capbuflen, pespcount, pcount);

            let hauppauge_buf = if !haup_capbuf.is_null() {
                *haup_capbuf
            } else {
                null_mut()
            };
            ret = copy_capbuf_demux_data(
                ctx,
                data,
                &mut *cinfo,
                ccx_options,
                hauppauge_buf,
                haup_capbuflen,
                last_pts,
            );
            (*cinfo).capbuflen = 0;
            gotpes = 1;
        }

        // Copy payload to capture buffer
        let copy_ret = copy_payload_to_capbuf(&mut *cinfo, &payload, ccx_options);
        if copy_ret < 0 {
            if copy_ret == -1 {
                // EINVAL equivalent
                continue;
            } else {
                break;
            }
        }

        pespcount += 1;

        if gotpes != 0 {
            break;
        }
    }

    // Process minimum PTS for all programs
    for i in 0..ctx.nb_program {
        pinfo = &mut ctx.pinfo[i] as *mut ProgramInfo;
        if !(*pinfo).has_all_min_pts {
            for j in 0..ctx.num_of_pids {
                let pid_idx = ctx.have_pids[j as usize];
                if pid_idx >= 0
                    && (pid_idx as usize) < ctx.pids_programs.len()
                    && !ctx.pids_programs[pid_idx as usize].is_null()
                    && (*ctx.pids_programs[pid_idx as usize]).program_number
                        == (*pinfo).program_number as u32
                    && ctx.min_pts[j as usize] != u64::MAX
                {
                    let stream_id = ctx.stream_id_of_each_pid[j as usize];
                    let min_pts = ctx.min_pts[j as usize];

                    if stream_id == 0xbd
                        && min_pts
                            < (*pinfo).got_important_streams_min_pts
                                [Stream_Type::PrivateStream1 as usize]
                    {
                        (*pinfo).got_important_streams_min_pts
                            [Stream_Type::PrivateStream1 as usize] = min_pts;
                    }
                    if (0xc0..=0xdf).contains(&stream_id)
                        && min_pts
                            < (*pinfo).got_important_streams_min_pts[Stream_Type::Audio as usize]
                    {
                        (*pinfo).got_important_streams_min_pts[Stream_Type::Audio as usize] =
                            min_pts;
                    }
                    if (0xe0..=0xef).contains(&stream_id)
                        && min_pts
                            < (*pinfo).got_important_streams_min_pts[Stream_Type::Video as usize]
                    {
                        (*pinfo).got_important_streams_min_pts[Stream_Type::Video as usize] =
                            min_pts;
                    }

                    // Check if we have all minimum PTS values
                    if (*pinfo).got_important_streams_min_pts[Stream_Type::PrivateStream1 as usize]
                        != u64::MAX
                        && (*pinfo).got_important_streams_min_pts[Stream_Type::Audio as usize]
                            != u64::MAX
                        && (*pinfo).got_important_streams_min_pts[Stream_Type::Video as usize]
                            != u64::MAX
                    {
                        (*pinfo).has_all_min_pts = true;
                    }
                }
            }
        }
    }

    // Handle EOF
    if ret == DemuxerError::EOF as i32 {
        let hauppauge_buf = if !haup_capbuf.is_null() {
            *haup_capbuf
        } else {
            null_mut()
        };
        cinfo_cremation(
            ctx,
            data,
            cap,
            ccx_options,
            hauppauge_buf,
            haup_capbuflen,
            last_pts,
        );
    }

    ret as i64
}
