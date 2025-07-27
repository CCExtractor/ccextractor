#![allow(unused)] // currently TS is WIP
use crate::bindings::cap_info;
use crate::demuxer::common_structs::{
    CcxDemuxer, PSIBuffer, ProgramInfo, Stream_Type, MAX_PROGRAM, MAX_PROGRAM_NAME_LEN,
};
use crate::hlist::dinit_cap;
use crate::tlt_config;
use crate::transportstream::epg_tables::epg_dvb_decode_string;
use lib_ccxr::activity::ActivityExt;
use lib_ccxr::common::{MpegDescriptor, Options, StreamType};
use lib_ccxr::util::log::{debug, info, DebugMessageFlag};
pub const TS_PMT_MAP_SIZE: usize = 8192;

pub fn get_printable_stream_type(stream_type: StreamType) -> StreamType {
    match stream_type {
        StreamType::VideoMpeg2
        | StreamType::VideoH264
        | StreamType::PrivateMpeg2
        | StreamType::PrivateTableMpeg2
        | StreamType::MhegPackets
        | StreamType::Mpeg2AnnexADsmCc
        | StreamType::IsoIec13818_6TypeA
        | StreamType::IsoIec13818_6TypeB
        | StreamType::IsoIec13818_6TypeC
        | StreamType::IsoIec13818_6TypeD
        | StreamType::ItuTH222_1
        | StreamType::VideoMpeg1
        | StreamType::AudioMpeg1
        | StreamType::AudioMpeg2
        | StreamType::AudioAac
        | StreamType::VideoMpeg4
        | StreamType::AudioAc3
        | StreamType::AudioDts
        | StreamType::AudioHdmvDts => stream_type,
        _ => {
            let stream_value = stream_type as u8;
            if (0x80..=0xFF).contains(&stream_value) {
                StreamType::PrivateUserMpeg2
            } else {
                StreamType::Unknownstream
            }
        }
    }
}

pub fn clear_pmt_array(ctx: &mut CcxDemuxer) {
    if !ctx.flag_ts_forced_pn {
        ctx.nb_program = 0;
    }
}

pub fn need_program(ctx: &CcxDemuxer) -> bool {
    if ctx.nb_program == 0 {
        return true;
    }
    if ctx.nb_program == 1 && ctx.ts_autoprogram {
        return false;
    }
    false
}
fn update_pinfo(ctx: &mut CcxDemuxer, pid: i32, program_number: i32) -> Result<(), i32> {
    if ctx.nb_program >= MAX_PROGRAM {
        return Err(-1);
    }

    // Add a new ProgramInfo to the vector
    let new_pinfo = ProgramInfo {
        pid,
        program_number,
        initialized_ocr: false,
        analysed_pmt_once: 0, // CCX_FALSE equivalent
        version: 0,
        saved_section: [0; 1021],
        crc: 0,
        valid_crc: 0,
        name: [0; MAX_PROGRAM_NAME_LEN], // Equivalent to name[0] = '\0'
        pcr_pid: -1,
        got_important_streams_min_pts: [u64::MAX; Stream_Type::Count as usize],
        has_all_min_pts: false, // 0 equivalent
    };

    ctx.pinfo.push(new_pinfo);
    ctx.nb_program += 1;

    Ok(())
}
fn ts_buffer_psi_packet(ctx: &mut CcxDemuxer, tspacket: &[u8]) {
    if tspacket.len() < 188 {
        return;
    }

    let mut payload_start = 4;
    let mut payload_length = 188 - 4;

    // Extract fields from the packet header
    let payload_start_indicator = (tspacket[1] & 0x40) >> 6;
    let pid = (((tspacket[1] & 0x1F) as u16) << 8 | tspacket[2] as u16) & 0x1FFF;
    let adaptation_field_control = (tspacket[3] & 0x30) >> 4;
    let ccounter = tspacket[3] & 0xF;
    let mut adaptation_field_length = 0;

    if adaptation_field_control & 2 != 0 {
        adaptation_field_length = tspacket[4];
        payload_start = payload_start + adaptation_field_length as usize + 1;
        payload_length = 188 - payload_start;
    }

    let pid_index = pid as usize;

    // Ensure the pid_buffers vector is large enough
    if ctx.pid_buffers.len() <= pid_index {
        ctx.pid_buffers.resize(pid_index + 1, std::ptr::null_mut());
    }

    // First packet for this pid. Create a buffer
    if ctx.pid_buffers[pid_index].is_null() {
        let psi_buffer = Box::new(PSIBuffer {
            prev_ccounter: 0xFF,
            buffer: std::ptr::null_mut(),
            buffer_length: 0,
            ccounter: 0,
        });
        ctx.pid_buffers[pid_index] = Box::into_raw(psi_buffer);
    }

    // Skip the packet if the adaptation field length or payload length are out of bounds or broken
    if adaptation_field_length > 184 || payload_length > 184 {
        payload_length = 0;
        debug!(msg_type = DebugMessageFlag::GENERIC_NOTICE; "\rWarning: Bad packet, adaptation field too long, skipping.\n");
    }

    unsafe {
        let psi_buffer = &mut *ctx.pid_buffers[pid_index];

        if payload_start_indicator != 0 {
            if psi_buffer.ccounter > 0 {
                psi_buffer.ccounter = 0;
            }
            psi_buffer.prev_ccounter = ccounter as u32;

            if !psi_buffer.buffer.is_null() {
                let len = psi_buffer.buffer_length as usize;
                let slice_ptr: *mut [u8] =
                    std::ptr::slice_from_raw_parts_mut(psi_buffer.buffer, len);
                Box::from_raw(slice_ptr);
            }

            if payload_length > 0 {
                let new_buffer = vec![0u8; payload_length].into_boxed_slice();
                let buffer_ptr = Box::into_raw(new_buffer) as *mut u8;

                std::ptr::copy_nonoverlapping(
                    tspacket.as_ptr().add(payload_start),
                    buffer_ptr,
                    payload_length,
                );

                psi_buffer.buffer = buffer_ptr;
                psi_buffer.buffer_length = payload_length as u32;
            } else {
                psi_buffer.buffer = std::ptr::null_mut();
                psi_buffer.buffer_length = 0;
            }
            psi_buffer.ccounter += 1;
        } else if ccounter as u32 == (psi_buffer.prev_ccounter + 1) & 0xF
            || (psi_buffer.prev_ccounter == 0x0f && ccounter == 0)
        {
            psi_buffer.prev_ccounter = ccounter as u32;

            if payload_length > 0 {
                let old_length = psi_buffer.buffer_length as usize;
                let new_length = old_length + payload_length;

                // Create new buffer with combined size
                let mut new_buffer = vec![0u8; new_length].into_boxed_slice();

                // Copy old data if it exists
                if !psi_buffer.buffer.is_null() && old_length > 0 {
                    std::ptr::copy_nonoverlapping(
                        psi_buffer.buffer,
                        new_buffer.as_mut_ptr(),
                        old_length,
                    );

                    let slice_ptr: *mut [u8] =
                        std::ptr::slice_from_raw_parts_mut(psi_buffer.buffer, old_length);
                    let _ = Box::from_raw(slice_ptr);
                }

                // Copy new data
                std::ptr::copy_nonoverlapping(
                    tspacket.as_ptr().add(payload_start),
                    new_buffer.as_mut_ptr().add(old_length),
                    payload_length,
                );

                psi_buffer.buffer = Box::into_raw(new_buffer) as *mut u8;
                psi_buffer.buffer_length = new_length as u32;
            }

            psi_buffer.ccounter += 1;
        } else if psi_buffer.prev_ccounter <= 0x0f {
            debug!(
                msg_type = DebugMessageFlag::GENERIC_NOTICE;
                "\rWarning: Out of order packets detected for PID: {}.\n\
                psi_buffer.prev_ccounter: {} psi_buffer.ccounter: {}\n",
                pid,
                psi_buffer.prev_ccounter,
                psi_buffer.ccounter
            );
        }
    }
}
pub fn parse_pat(ctx: &mut CcxDemuxer, ccx_options: &mut Options, cap: &mut cap_info) -> i32 {
    let mut gotpes = 0;
    let is_multiprogram: bool;

    // Safety check - ensure we have a buffer for PID 0
    if ctx.pid_buffers.is_empty() || ctx.pid_buffers[0].is_null() {
        return 0;
    }

    unsafe {
        let pid_buffer = &*ctx.pid_buffers[0];

        if pid_buffer.buffer.is_null() || pid_buffer.buffer_length == 0 {
            return 0;
        }

        let buffer_slice =
            std::slice::from_raw_parts(pid_buffer.buffer, pid_buffer.buffer_length as usize);

        let pointer_field = buffer_slice[0];
        let payload_start_offset = (pointer_field + 1) as usize;

        if payload_start_offset >= buffer_slice.len() {
            return 0;
        }

        let payload_start = &buffer_slice[payload_start_offset..];
        let payload_length = buffer_slice.len() - payload_start_offset;

        if payload_start.len() < 8 {
            return 0; // Not enough data
        }

        let section_number = payload_start[6];
        let last_section_number = payload_start[7];

        let section_length = (((payload_start[1] & 0x0F) as u32) << 8) | payload_start[2] as u32;
        let program_data = section_length.saturating_sub(5).saturating_sub(4); // prev. bytes and CRC

        if program_data + 4 > payload_length as u32 {
            return 0; // We don't have the full section yet
        }

        if section_number > last_section_number {
            debug!(msg_type = DebugMessageFlag::PAT; "Skipped defective PAT packet, section_number={} but last_section_number={}\n",
                      section_number, last_section_number);
            return gotpes;
        }

        if last_section_number > 0 {
            debug!(msg_type = DebugMessageFlag::PAT; "Long PAT packet ({} / {}), skipping.\n",
                      section_number, last_section_number);
            return gotpes;
        }

        // Check if PAT has changed
        if !ctx.last_pat_payload.is_null() && payload_length == ctx.last_pat_length as usize {
            let last_pat_slice =
                std::slice::from_raw_parts(ctx.last_pat_payload, ctx.last_pat_length as usize);
            if payload_start[..payload_length] == last_pat_slice[..payload_length] {
                return 0;
            }
        }

        if !ctx.last_pat_payload.is_null() {
            info!("Notice: PAT changed, clearing all variables.\n");
            dinit_cap(cap);
            clear_pmt_array(ctx);

            // Clear PIDs_seen array
            for i in 0..65536 {
                if i < ctx.pids_seen.len() {
                    ctx.pids_seen[i] = 0;
                }
            }

            if tlt_config.user_page == 0 {
                tlt_config.page = 0;
            }

            gotpes = 1;
        }

        // Reallocate last_pat_payload if needed
        if ctx.last_pat_length < (payload_length + 8) as u32 {
            if !ctx.last_pat_payload.is_null() {
                let _ = Vec::from_raw_parts(
                    ctx.last_pat_payload,
                    ctx.last_pat_length as usize,
                    ctx.last_pat_length as usize,
                );
            }

            let new_buffer = vec![0u8; payload_length + 8].into_boxed_slice();
            ctx.last_pat_payload = Box::into_raw(new_buffer) as *mut u8;
        }

        // Copy payload to last_pat_payload
        std::ptr::copy_nonoverlapping(payload_start.as_ptr(), ctx.last_pat_payload, payload_length);
        ctx.last_pat_length = payload_length as u32;

        let table_id = payload_start[0];
        let transport_stream_id = ((payload_start[3] as u16) << 8) | payload_start[4] as u16;
        let version_number = (payload_start[5] & 0x3E) >> 1;
        let current_next_indicator = payload_start[5] & 0x01;

        if current_next_indicator == 0 {
            return 0; // This table is not active
        }

        let program_payload = &payload_start[8..];

        debug!(msg_type = DebugMessageFlag::PAT; "Read PAT packet (id: {}) ts-id: 0x{:04x}\n",
                  table_id, transport_stream_id);
        debug!(msg_type = DebugMessageFlag::PAT; "  section length: {}  number: {}  last: {}\n",
                  section_length, section_number, last_section_number);
        debug!(msg_type = DebugMessageFlag::PAT; "  version_number: {}  current_next_indicator: {}\n",
                  version_number, current_next_indicator);
        debug!(msg_type = DebugMessageFlag::PAT; "\nProgram association section (PAT)\n");

        // Count programs
        ctx.freport.program_cnt = 0;
        for i in (0..program_data).step_by(4) {
            let i = i as usize;
            if i + 1 < program_payload.len() {
                let program_number =
                    ((program_payload[i] as u16) << 8) | program_payload[i + 1] as u16;
                if program_number != 0 {
                    ctx.freport.program_cnt += 1;
                }
            }
        }

        is_multiprogram = ctx.freport.program_cnt > 1;

        // Process programs
        for i in (0..program_data).step_by(4) {
            let i = i as usize;
            if i + 3 < program_payload.len() {
                let program_number =
                    ((program_payload[i] as u16) << 8) | program_payload[i + 1] as u16;
                let prog_map_pid = (((program_payload[i + 2] as u16) << 8)
                    | program_payload[i + 3] as u16)
                    & 0x1FFF;

                debug!(msg_type = DebugMessageFlag::PAT; "  Program number: {}  -> PMTPID: {}\n",
                          program_number, prog_map_pid);

                if program_number == 0 {
                    continue;
                }

                let mut found = false;

                // Look for existing program
                for pinfo in ctx.pinfo.iter_mut() {
                    if pinfo.program_number == program_number as i32 {
                        if ctx.flag_ts_forced_pn && pinfo.pid == 0 {
                            pinfo.pid = prog_map_pid as i32;
                            pinfo.analysed_pmt_once = 0;
                        }
                        found = true;
                        break;
                    }
                }

                if !found && !ctx.flag_ts_forced_pn {
                    let _ = update_pinfo(ctx, prog_map_pid as i32, program_number as i32);
                }
            }
        }

        if is_multiprogram && !ctx.flag_ts_forced_pn {
            info!(
                "\nThis TS file has more than one program. These are the program numbers found: \n"
            );
            for i in (0..program_data).step_by(4) {
                let i = i as usize;
                if i + 1 < program_payload.len() {
                    let pn = ((program_payload[i] as u16) << 8) | program_payload[i + 1] as u16;
                    if pn != 0 {
                        info!("{}\n", pn);
                        ccx_options.activity_program_number(pn);
                    }
                }
            }
        }

        gotpes
    }
}
pub fn process_ccx_mpeg_descriptor(data: &[u8]) {
    const TXT_TELETEXT_TYPE: &[&str] = &[
        "Reserved",
        "Initial page",
        "Subtitle page",
        "Additional information page",
        "Programme schedule page",
        "Subtitle page for hearing impaired people",
    ];

    if data.is_empty() {
        return;
    }

    match data[0] {
        x if x == MpegDescriptor::Iso639Language as u8 => {
            if data.len() < 2 {
                return;
            }
            let l = data[1] as usize;
            if l + 2 < data.len() {
                return;
            }

            for i in (0..l).step_by(4) {
                if i + 4 < data.len() {
                    let c1 = data[i + 2] as char;
                    let c2 = data[i + 3] as char;
                    let c3 = data[i + 4] as char;

                    debug!(msg_type = DebugMessageFlag::PMT; "             ISO639: {}{}{}\n",
                              if c1 >= ' ' { c1 } else { ' ' },
                              if c2 >= ' ' { c2 } else { ' ' },
                              if c3 >= ' ' { c3 } else { ' ' });
                }
            }
        }

        x if x == MpegDescriptor::VbiDataDescriptor as u8 => {
            debug!(msg_type = DebugMessageFlag::PMT; "DVB VBI data descriptor (not implemented)\n");
        }

        x if x == MpegDescriptor::VbiTeletextDescriptor as u8 => {
            debug!(msg_type = DebugMessageFlag::PMT; "DVB VBI teletext descriptor\n");
        }

        x if x == MpegDescriptor::TeletextDescriptor as u8 => {
            debug!(msg_type = DebugMessageFlag::PMT; "             DVB teletext descriptor\n");
            if data.len() < 2 {
                return;
            }
            let l = data[1] as usize;
            if l + 2 < data.len() {
                return;
            }

            for i in (0..l).step_by(5) {
                if i + 6 < data.len() {
                    let c1 = data[i + 2] as char;
                    let c2 = data[i + 3] as char;
                    let c3 = data[i + 4] as char;
                    let teletext_type = (data[i + 5] & 0xF8) >> 3; // 5 MSB
                                                                   // let magazine_number = data[i + 5] & 0x7; // 3 LSB
                    let teletext_page_number = data[i + 6];

                    debug!(msg_type = DebugMessageFlag::PMT; "                        ISO639: {}{}{}\n",
                              if c1 >= ' ' { c1 } else { ' ' },
                              if c2 >= ' ' { c2 } else { ' ' },
                              if c3 >= ' ' { c3 } else { ' ' });

                    let teletext_type_str = if (teletext_type as usize) < TXT_TELETEXT_TYPE.len() {
                        TXT_TELETEXT_TYPE[teletext_type as usize]
                    } else {
                        "Reserved for future use"
                    };

                    debug!(msg_type = DebugMessageFlag::PMT; "                 Teletext type: {} ({:02X})\n",
                              teletext_type_str, teletext_type);
                    debug!(msg_type = DebugMessageFlag::PMT; "                  Initial page: {:02X}\n",
                              teletext_page_number);
                }
            }
        }

        x if x == MpegDescriptor::DvbSubtitle as u8 => {
            debug!(msg_type = DebugMessageFlag::PMT; "             DVB Subtitle descriptor\n");
        }

        x if x == MpegDescriptor::Registration as u8 => {
            // Registration descriptor, could be useful eventually
        }

        x if x == MpegDescriptor::DataStreamAlignment as u8 => {
            // Data stream alignment descriptor
        }

        x if (0x13..=0x3F).contains(&x) => {
            // Reserved
        }

        x if (0x40..=0xFF).contains(&x) => {
            // User private
        }

        _ => {
            // mprint("Still unsupported MPEG descriptor type={} ({:02X})\n", data[0], data[0]);
        }
    }
}

fn decode_service_descriptors(ctx: &mut CcxDemuxer, buf: &[u8], length: u32, service_id: u32) {
    let mut offset = 0u32;

    while offset + 5 < length {
        if offset as usize >= buf.len() {
            break;
        }

        let descriptor_tag = buf[offset as usize];
        let descriptor_length = buf[(offset + 1) as usize];
        offset += 2;

        if descriptor_tag == 0x48 {
            // service descriptor
            if offset + 1 >= length || (offset + 1) as usize >= buf.len() {
                break;
            }

            // let service_type = buf[offset as usize];
            let service_provider_name_length = buf[(offset + 1) as usize];
            offset += 2;

            if offset + service_provider_name_length as u32 > length {
                debug!(
                    msg_type = DebugMessageFlag::GENERIC_NOTICE;
                    "\rWarning: Invalid SDT service_provider_name_length detected.\n"
                );
                return;
            }

            offset += service_provider_name_length as u32; // Service provider name. Not sure what this is useful for.

            if offset as usize >= buf.len() {
                break;
            }

            let service_name_length: u32 = buf[offset as usize] as u32;
            offset += 1;

            if offset + service_name_length > length {
                debug!(
                    msg_type = DebugMessageFlag::GENERIC_NOTICE;
                    "\rWarning: Invalid SDT service_name_length detected.\n"
                );
                return;
            }

            for x in 0..ctx.nb_program {
                // Not sure if programs can have multiple names (in different encodings?) Need more samples.
                // For now just assume the last one in the loop is as good as any if there are multiple.
                if ctx.pinfo[x].program_number == service_id as i32 && service_name_length < 199 {
                    if let Ok(s) = epg_dvb_decode_string(
                        &buf[offset as usize..(offset + service_name_length) as usize],
                    ) {
                        if s.len() < MAX_PROGRAM_NAME_LEN - 1 {
                            // Copy the string bytes to the name array
                            let raw = s.into_bytes();
                            let take = service_name_length.min(raw.len() as u32) as usize;
                            ctx.pinfo[x].name[..take].copy_from_slice(&raw[..take]);
                            ctx.pinfo[x].name[take] = 0;
                        }
                    }
                }
            }
            offset += service_name_length;
        } else {
            // Some other tag
            offset += descriptor_length as u32;
        }
    }
}
fn decode_sdt_services_loop(ctx: &mut CcxDemuxer, buf: &[u8], length: u32) {
    let mut offset = 0u32;

    while offset + 5 < length {
        if (offset + 4) as usize >= buf.len() {
            break;
        }

        let service_id = ((buf[offset as usize] as u16) << 8) | buf[(offset + 1) as usize] as u16;
        let descriptors_loop_length =
            (((buf[(offset + 3) as usize] & 0x0F) as u32) << 8) | buf[(offset + 4) as usize] as u32;
        offset += 5;

        if offset + descriptors_loop_length > length {
            debug!(
                msg_type = DebugMessageFlag::GENERIC_NOTICE;
                "\rWarning: Invalid SDT descriptors_loop_length detected.\n"
            );
            return;
        }

        if (offset + descriptors_loop_length) as usize <= buf.len() {
            decode_service_descriptors(
                ctx,
                &buf[offset as usize..(offset + descriptors_loop_length) as usize],
                descriptors_loop_length,
                service_id as u32,
            );
        }
        offset += descriptors_loop_length;
    }
}

fn parse_sdt(ctx: &mut CcxDemuxer) {
    // Safety check: ensure PID_buffers[0x11] exists and is valid
    if ctx.pid_buffers.len() <= 0x11 || ctx.pid_buffers[0x11].is_null() {
        return;
    }

    unsafe {
        let pid_buffer = &*ctx.pid_buffers[0x11];

        if pid_buffer.buffer.is_null() || pid_buffer.buffer_length == 0 {
            return;
        }

        let buffer_slice =
            std::slice::from_raw_parts(pid_buffer.buffer, pid_buffer.buffer_length as usize);

        let pointer_field = buffer_slice[0];
        let payload_start_offset = (pointer_field + 1) as usize;
        let payload_length = pid_buffer.buffer_length - (pointer_field as u32 + 1);

        if payload_start_offset >= buffer_slice.len() {
            return;
        }

        let payload_start = &buffer_slice[payload_start_offset..];

        if payload_start.len() < 11 {
            return;
        }

        let table_id = payload_start[0];
        let section_length = (((payload_start[1] & 0x0F) as u32) << 8) | payload_start[2] as u32;

        if section_length > payload_length - 4 {
            return;
        }

        let current_next_indicator = payload_start[5] & 0x01;

        if current_next_indicator == 0 {
            // This table is not active, no need to evaluate
            return;
        }

        if table_id != 0x42 {
            // This table isn't for the active TS
            return;
        }

        if section_length >= 12 && payload_start.len() >= 11 + (section_length - 12) as usize {
            decode_sdt_services_loop(ctx, &payload_start[11..], section_length - 4 - 8);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    use crate::demuxer::common_structs::CcxDemuxReport;

    use lib_ccxr::common::{Codec, StreamMode};
    use lib_ccxr::time::Timestamp;
    use lib_ccxr::util::log::{set_logger, CCExtractorLogger, DebugMessageMask, OutputTarget};

    use std::sync::Once;

    static INIT: Once = Once::new();

    fn initialize_logger() {
        INIT.call_once(|| {
            set_logger(CCExtractorLogger::new(
                OutputTarget::Stdout,
                DebugMessageMask::new(DebugMessageFlag::VERBOSE, DebugMessageFlag::VERBOSE),
                false,
            ))
            .ok();
        });
    }
    fn create_mock_demuxer() -> CcxDemuxer<'static> {
        CcxDemuxer {
            m2ts: 0,
            stream_mode: StreamMode::default(),
            auto_stream: StreamMode::default(),
            startbytes: Vec::new(),
            startbytes_pos: 0,
            startbytes_avail: 0,
            ts_autoprogram: false,
            ts_allprogram: false,
            flag_ts_forced_pn: false,
            flag_ts_forced_cappid: false,
            ts_datastreamtype: StreamType::default(),
            pinfo: vec![
                ProgramInfo {
                    pid: 100,
                    program_number: 1,
                    initialized_ocr: false,
                    analysed_pmt_once: 0,
                    version: 0,
                    saved_section: [0; 1021],
                    crc: 0,
                    valid_crc: 0,
                    name: [0; MAX_PROGRAM_NAME_LEN],
                    pcr_pid: -1,
                    got_important_streams_min_pts: [0; 3],
                    has_all_min_pts: false,
                },
                ProgramInfo {
                    pid: 200,
                    program_number: 2,
                    initialized_ocr: false,
                    analysed_pmt_once: 0,
                    version: 0,
                    saved_section: [0; 1021],
                    crc: 0,
                    valid_crc: 0,
                    name: [0; MAX_PROGRAM_NAME_LEN],
                    pcr_pid: -1,
                    got_important_streams_min_pts: [0; 3],
                    has_all_min_pts: false,
                },
            ],
            nb_program: 2,
            codec: Codec::Dvb,
            nocodec: Codec::Dvb,
            infd: -1,
            past: 0,
            global_timestamp: Timestamp::default(),
            min_global_timestamp: Timestamp::default(),
            offset_global_timestamp: Timestamp::default(),
            last_global_timestamp: Timestamp::default(),
            global_timestamp_inited: Timestamp::default(),
            pid_buffers: Vec::new(),
            pids_seen: Vec::new(),
            stream_id_of_each_pid: Vec::new(),
            min_pts: Vec::new(),
            have_pids: Vec::new(),
            num_of_pids: 0,
            pids_programs: Vec::new(),
            freport: CcxDemuxReport::default(),
            hauppauge_warning_shown: false,
            multi_stream_per_prog: 0,
            last_pat_payload: std::ptr::null_mut(),
            last_pat_length: 0,
            filebuffer: std::ptr::null_mut(),
            filebuffer_start: 0,
            filebuffer_pos: 0,
            bytesinbuffer: 0,
            warning_program_not_found_shown: false,
            strangeheader: 0,
            parent: None,
            private_data: std::ptr::null_mut(),
            #[cfg(feature = "enable_ffmpeg")]
            ffmpeg_ctx: std::ptr::null_mut(),
        }
    }

    // Tests for epg_dvb_decode_string
    #[test]
    fn test_epg_dvb_decode_string_empty_input() {
        let result = epg_dvb_decode_string(&[]);
        assert!(result.is_ok());
        assert_eq!(result.unwrap(), "");
    }

    #[test]
    fn test_epg_dvb_decode_string_ascii_input() {
        let input = b"Hello World";
        let result = epg_dvb_decode_string(input);
        assert!(result.is_ok());
        assert_eq!(result.unwrap(), "Hello World");
    }

    #[test]
    fn test_epg_dvb_decode_string_utf8_encoding() {
        let input = vec![0x15, b'T', b'e', b's', b't'];
        let result = epg_dvb_decode_string(&input);
        assert!(result.is_ok());
        assert_eq!(result.unwrap(), "Test");
    }

    // Tests for decode_service_descriptors
    #[test]
    fn test_decode_service_descriptors_valid_service_descriptor() {
        let mut ctx = create_mock_demuxer();

        // Create a valid service descriptor buffer
        // Format: descriptor_tag(0x48) | descriptor_length | service_type | provider_name_len | provider_name | service_name_len | service_name
        let buf = vec![
            0x48, // descriptor_tag = 0x48 (service descriptor)
            0x0C, // descriptor_length = 12
            0x01, // service_type = 1
            0x04, // service_provider_name_length = 4
            b'T', b'e', b's', b't', // provider name "Test"
            0x04, // service_name_length = 4
            b'C', b'B', b'C', b'1', // service name "CBC1"
        ];

        decode_service_descriptors(&mut ctx, &buf, buf.len() as u32, 1);

        // Check that the program name was updated
        let program_name = std::str::from_utf8(&ctx.pinfo[0].name[..4]).unwrap();
        assert_eq!(program_name, "CBC1"); // Should contain the decoded string
    }

    #[test]
    fn test_decode_service_descriptors_invalid_length() {
        let mut ctx = create_mock_demuxer();

        // Create an invalid service descriptor with wrong length
        let buf = vec![
            0x48, // descriptor_tag = 0x48
            0x0C, // descriptor_length = 12
            0x01, // service_type = 1
            0xFF, // service_provider_name_length = 255 (invalid - too long)
        ];

        decode_service_descriptors(&mut ctx, &buf, buf.len() as u32, 1);

        // Should not crash and should not modify the program name
        let program_name_bytes = &ctx.pinfo[0].name[..10];
        assert_eq!(program_name_bytes, &[0; 10]); // Should remain unchanged
    }

    #[test]
    fn test_decode_service_descriptors_non_service_descriptor() {
        let mut ctx = create_mock_demuxer();

        // Create a non-service descriptor
        let buf = vec![
            0x40, // descriptor_tag = 0x40 (not a service descriptor)
            0x04, // descriptor_length = 4
            0x01, 0x02, 0x03, 0x04, // some data
        ];

        decode_service_descriptors(&mut ctx, &buf, buf.len() as u32, 1);

        // Should not crash and should not modify the program name
        let program_name_bytes = &ctx.pinfo[0].name[..10];
        assert_eq!(program_name_bytes, &[0; 10]); // Should remain unchanged
    }

    // Tests for decode_sdt_services_loop
    #[test]
    fn test_decode_sdt_services_loop_valid_service() {
        initialize_logger();
        let mut ctx = create_mock_demuxer();

        // Create a valid SDT services loop buffer
        let buf = vec![
            0x00, 0x01, // service_id = 1
            0x00, // reserved and EIT_schedule_flag, EIT_present_following_flag
            0x00, 0x0E, // descriptors_loop_length = 14
            // Service descriptor follows
            0x48, // descriptor_tag = 0x48
            0x0C, // descriptor_length = 12
            0x01, // service_type = 1
            0x04, // service_provider_name_length = 4
            b'T', b'e', b's', b't', // provider name
            0x04, // service_name_length = 4
            b'C', b'B', b'C', b'1', // service name
        ];

        decode_sdt_services_loop(&mut ctx, &buf, buf.len() as u32);

        // Should not crash and should process the service descriptor
        // The actual name update depends on the epg_dvb_decode_string implementation
    }

    #[test]
    fn test_decode_sdt_services_loop_invalid_descriptors_length() {
        let mut ctx = create_mock_demuxer();

        // Create an invalid SDT services loop with wrong descriptors_loop_length
        let buf = vec![
            0x00, 0x01, // service_id = 1
            0x00, // reserved bits
            0x0F, 0xFF, // descriptors_loop_length = 4095 (too long for buffer)
        ];

        decode_sdt_services_loop(&mut ctx, &buf, buf.len() as u32);

        // Should not crash and should return early due to invalid length
    }

    // Tests for parse_sdt
    #[test]
    fn test_parse_sdt_valid_table() {
        let mut ctx = create_mock_demuxer();

        let sdt_data = vec![
            0x00, // pointer_field = 0
            0x42, // table_id = 0x42 (SDT)
            0x80, 0x17, // section_syntax_indicator=1, reserved=0, section_length=23
            0x00, 0x01, // transport_stream_id = 1
            0x01, // version_number=0, current_next_indicator=1
            0x00, // section_number = 0
            0x00, // last_section_number = 0
            0x00, 0x01, // original_network_id = 1
            0xFF, // reserved
            // Services loop starts here (offset 11)
            0x00, 0x01, // service_id = 1
            0x00, // reserved
            0x00, 0x08, // descriptors_loop_length = 8
            0x48, 0x06, 0x01, 0x02, b'T', b'V', 0x02, b'H', b'D', // service descriptor
        ];

        let buffer_ptr = sdt_data.as_ptr() as *mut u8;
        let psi_buffer = PSIBuffer {
            buffer: buffer_ptr,
            buffer_length: sdt_data.len() as u32,
            ..Default::default()
        };

        // Ensure we have enough space in pid_buffers
        ctx.pid_buffers.resize(0x12, std::ptr::null_mut());
        ctx.pid_buffers[0x11] = &psi_buffer as *const PSIBuffer as *mut PSIBuffer;

        // This should not crash
        parse_sdt(&mut ctx);
    }

    #[test]
    fn test_parse_sdt_invalid_table_id() {
        let mut ctx = create_mock_demuxer();

        let sdt_data = vec![
            0x00, // pointer_field = 0
            0x40, // table_id = 0x40 (not SDT)
            0x80, 0x17, // section_syntax_indicator=1, reserved=0, section_length=23
            0x00, 0x01, // transport_stream_id = 1
            0x01, // version_number=0, current_next_indicator=1
            0x00, // section_number = 0
            0x00, // last_section_number = 0
            0x00, 0x01, // original_network_id = 1
            0xFF, // reserved
        ];

        let buffer_ptr = sdt_data.as_ptr() as *mut u8;
        let psi_buffer = PSIBuffer {
            buffer: buffer_ptr,
            buffer_length: sdt_data.len() as u32,
            ..Default::default()
        };

        ctx.pid_buffers.resize(0x12, std::ptr::null_mut());
        ctx.pid_buffers[0x11] = &psi_buffer as *const PSIBuffer as *mut PSIBuffer;

        // Should return early due to invalid table_id
        parse_sdt(&mut ctx);
    }

    #[test]
    fn test_parse_sdt_null_buffer() {
        let mut ctx = create_mock_demuxer();

        // Ensure pid_buffers[0x11] is null
        ctx.pid_buffers.resize(0x12, std::ptr::null_mut());

        // Should not crash with null buffer
        parse_sdt(&mut ctx);
    }
}
