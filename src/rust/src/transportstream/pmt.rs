use crate::bindings::{
    cap_info, ccx_mpeg_descriptor, dvb_config, dvbsub_init_decoder, init_isdb_decoder,
    parse_dvb_description,
};
use crate::common::CType;
use crate::ctorust::FromCType;
use crate::demuxer::common_structs::{CcxDemuxer, PMTEntry, ProgramInfo, SUB_STREAMS_CNT};
use crate::hlist::{need_cap_info_for_pid, update_capinfo};
use crate::transportstream::tables::{get_printable_stream_type, process_ccx_mpeg_descriptor};
use lib_ccxr::common::{Codec, MpegDescriptor, Options, OutputFormat, StreamType};
use lib_ccxr::util::bits::verify_crc32;
use lib_ccxr::util::log::{DebugMessageFlag, ExitCause};
use lib_ccxr::{debug, fatal, info};
use std::os::raw::c_uint;
use std::ptr;

/// # Safety
/// This function is unsafe because it uses C structs in update_capinfo and uses C functions like parse_dvb_description and dvbsub_init_decoder.
pub unsafe fn parse_pmt(
    ctx: &mut CcxDemuxer,
    buf: &mut [u8],
    len: usize,
    pinfo: &mut ProgramInfo,
    ccx_options: &mut Options,
    cap: &mut cap_info,
) -> i32 {
    // implemented correctly but we can't use it until init_demuxer(basically a part of the core lib_ccx library) is made in Rust
    let must_flush = 0;
    let mut ret;
    let mut desc_len: u8;
    let _ = buf.as_ptr();
    let olen = len;

    if len < 12 {
        return 0; // Not enough data for minimum PMT structure
    }

    // Calculate CRC from the end of the buffer
    let crc = if len >= 4 {
        let crc_bytes = &buf[len - 4..len];
        i32::from_be_bytes([crc_bytes[0], crc_bytes[1], crc_bytes[2], crc_bytes[3]])
    } else {
        0
    };

    let mut table_id = buf[0];

    let mut current_len = len;

    // Handle Program Information Table (0xC0)
    if table_id == 0xC0 {
        debug!(msg_type = DebugMessageFlag::PMT; "PMT: PROGRAM INFORMATION Table need implementation");
        if current_len < 3 {
            return 0;
        }
        let c0length = (((buf[1] as u16) << 8) | (buf[2] as u16)) & 0xFFF;
        debug!(msg_type = DebugMessageFlag::PMT; "Program information table length: {}", c0length);

        let skip_bytes = c0length as usize + 3;
        if skip_bytes >= current_len {
            return 0;
        }

        // Move buffer content
        buf.copy_within(skip_bytes..current_len, 0);
        current_len -= skip_bytes;
        table_id = buf[0];
    }
    // Handle Program Name Table (0xC1)
    else if table_id == 0xC1 {
        debug!(msg_type = DebugMessageFlag::PMT; "PMT: PROGRAM NAME Table need implementation");
        if current_len < 3 {
            return 0;
        }
        let c0length = (((buf[1] as u16) << 8) | (buf[2] as u16)) & 0xFFF;
        debug!(msg_type = DebugMessageFlag::PMT; "Program name message length: {}", c0length);

        let skip_bytes = c0length as usize + 3;
        if skip_bytes >= current_len {
            return 0;
        }

        // Move buffer content
        buf.copy_within(skip_bytes..current_len, 0);
        current_len -= skip_bytes;
        table_id = buf[0];
    } else if table_id != 0x02 {
        info!(
            "Please Report: Unknown table id in PMT expected 0x02 found 0x{:X}",
            table_id
        );
        return 0;
    }

    if current_len < 12 {
        return 0;
    }

    let section_length = (((buf[1] & 0x0F) as u16) << 8) | (buf[2] as u16);
    if section_length as usize > (current_len - 3) {
        return 0; // We don't have the full section yet
    }

    let program_number = ((buf[3] as u16) << 8) | (buf[4] as u16);
    let version_number = (buf[5] & 0x3E) >> 1;
    let current_next_indicator = buf[5] & 0x01;

    // This table is not active, no need to evaluate
    if current_next_indicator == 0 && pinfo.version != 0xFF {
        return 0;
    }

    // Save the section
    let copy_len = std::cmp::min(current_len, pinfo.saved_section.len());
    pinfo.saved_section[..copy_len].copy_from_slice(&buf[..copy_len]);

    if pinfo.analysed_pmt_once == 1 && pinfo.version == version_number {
        if pinfo.version == version_number {
            // Same version number and there was valid or similar CRC last time
            if pinfo.valid_crc == 1 || pinfo.crc == crc {
                return 0;
            }
        } else if (pinfo.version + 1) % 32 != version_number {
            info!("TS PMT: Glitch in version number increment");
        }
    }
    pinfo.version = version_number;

    let section_number = buf[6];
    let last_section_number = buf[7];
    if last_section_number > 0 {
        info!("Long PMTs are not supported - skipped.");
        return 0;
    }

    pinfo.pcr_pid = (((buf[8] & 0x1F) as i16) << 8) | (buf[9] as i16);
    let pi_length = (((buf[10] & 0x0F) as u16) << 8) | (buf[11] as u16);

    if 12 + pi_length as usize > current_len {
        info!("program_info_length cannot be longer than the payload_length - skipped");
        return 0;
    }

    let current_pos = 12 + pi_length as usize;
    current_len -= current_pos;

    let stream_data = section_length as usize - 9 - pi_length as usize - 4; // prev. bytes and CRC

    debug!(msg_type = DebugMessageFlag::PMT; "Read PMT packet (id: {}) program number: {}",
          table_id, program_number);
    debug!(msg_type = DebugMessageFlag::PMT; "  section length: {}  number: {}  last: {}",
          section_length, section_number, last_section_number);
    debug!(msg_type = DebugMessageFlag::PMT; "  version_number: {}  current_next_indicator: {}",
          version_number, current_next_indicator);
    debug!(msg_type = DebugMessageFlag::PMT; "  PCR_PID: {}  data length: {}  payload_length: {}",
          pinfo.pcr_pid, stream_data, current_len);

    static mut PMT_WARNING_SHOWN: bool = false;
    if unsafe { !PMT_WARNING_SHOWN } && stream_data + 4 > current_len {
        debug!(msg_type = DebugMessageFlag::GENERIC_NOTICE;
               "Warning: Probably parsing incomplete PMT, expected data longer than available payload.");
        unsafe {
            PMT_WARNING_SHOWN = true;
        }
    }

    // First pass: Make a note of the program number for all PIDs
    let mut i = 0;
    while i < stream_data && (i + 4) < current_len {
        let stream_type = StreamType::from_ctype(buf[current_pos + i] as c_uint)
            .unwrap_or(StreamType::Unknownstream);
        let elementary_pid =
            (((buf[current_pos + i + 1] & 0x1F) as u16) << 8) | (buf[current_pos + i + 2] as u16);
        let es_info_length =
            (((buf[current_pos + i + 3] & 0x0F) as u16) << 8) | (buf[current_pos + i + 4] as u16);

        if elementary_pid < ctx.pids_programs.len() as u16 {
            if ctx.pids_programs[elementary_pid as usize].is_null() {
                ctx.pids_programs[elementary_pid as usize] = Box::into_raw(Box::new(PMTEntry {
                    elementary_pid: elementary_pid as u32,
                    stream_type,
                    program_number: program_number as u32,
                    printable_stream_type: get_printable_stream_type(stream_type).to_ctype() as u32,
                }));
            } else {
                let entry = unsafe { &mut *ctx.pids_programs[elementary_pid as usize] };
                entry.elementary_pid = elementary_pid as u32;
                entry.stream_type = stream_type;
                entry.program_number = program_number as u32;
                entry.printable_stream_type =
                    get_printable_stream_type(stream_type).to_ctype() as u32;
            }

            debug!(msg_type = DebugMessageFlag::VERBOSE; "{:6} | {:3X} | {}",
                  elementary_pid, stream_type as u8, stream_type as u8);
        }

        if current_pos + i + 5 + es_info_length as usize <= buf.len() {
            process_ccx_mpeg_descriptor(
                &buf[current_pos + i + 5..current_pos + i + 5 + es_info_length as usize],
            );
        }
        i += 5 + es_info_length as usize;
    }

    debug!(msg_type = DebugMessageFlag::VERBOSE; "---");
    debug!(msg_type = DebugMessageFlag::PMT; "Program map section (PMT)");

    // Second pass: Process file reports
    i = 0;
    while i < stream_data && (i + 4) < current_len {
        let stream_type = StreamType::from_ctype(buf[current_pos + i] as c_uint)
            .unwrap_or(StreamType::Unknownstream);
        let elementary_pid =
            (((buf[current_pos + i + 1] & 0x1F) as u16) << 8) | (buf[current_pos + i + 2] as u16);
        let es_info_length =
            (((buf[current_pos + i + 3] & 0x0F) as u16) << 8) | (buf[current_pos + i + 4] as u16);

        if !ccx_options.print_file_reports
            || stream_type != StreamType::PrivateMpeg2
            || es_info_length == 0
        {
            i += 5 + es_info_length as usize;
            continue;
        }

        let es_info_start = current_pos + i + 5;
        let es_info_end = es_info_start + es_info_length as usize;
        let mut es_info_pos = es_info_start;

        while es_info_pos < es_info_end && es_info_pos + 1 < buf.len() {
            let descriptor_tag =
                MpegDescriptor::from_ctype(buf[es_info_pos] as ccx_mpeg_descriptor)
                    .unwrap_or(MpegDescriptor::CaptionService);
            es_info_pos += 1;
            desc_len = buf[es_info_pos];
            es_info_pos += 1;

            if descriptor_tag == MpegDescriptor::DvbSubtitle {
                let mut k = 0;
                for j in 0..SUB_STREAMS_CNT {
                    if ctx.freport.dvb_sub_pid[j] == 0 {
                        k = j;
                    }
                    if ctx.freport.dvb_sub_pid[j] == elementary_pid as u32 {
                        k = j;
                        break;
                    }
                }
                ctx.freport.dvb_sub_pid[k] = elementary_pid as u32;
            }

            if descriptor_tag.is_valid_teletext_desc() {
                let mut k = 0;
                for j in 0..SUB_STREAMS_CNT {
                    if ctx.freport.tlt_sub_pid[j] == 0 {
                        k = j;
                    }
                    if ctx.freport.tlt_sub_pid[j] == elementary_pid as u32 {
                        k = j;
                        break;
                    }
                }
                ctx.freport.tlt_sub_pid[k] = elementary_pid as u32;
            }

            es_info_pos += desc_len as usize;
        }
        i += 5 + es_info_length as usize;
    }

    // Third pass: Process stream types for caption decoding
    i = 0;
    while i < stream_data && (i + 4) < current_len {
        let stream_type = StreamType::from_ctype(buf[current_pos + i] as c_uint)
            .unwrap_or(StreamType::Unknownstream);
        let elementary_pid =
            (((buf[current_pos + i + 1] & 0x1F) as u16) << 8) | (buf[current_pos + i + 2] as u16);
        let es_info_length =
            (((buf[current_pos + i + 3] & 0x0F) as u16) << 8) | (buf[current_pos + i + 4] as u16);

        if stream_type == StreamType::PrivateMpeg2 && es_info_length > 0 {
            let es_info_start = current_pos + i + 5;
            let es_info_end = es_info_start + es_info_length as usize;
            let mut es_info_pos = es_info_start;

            while es_info_pos < es_info_end && es_info_pos + 1 < buf.len() {
                let descriptor_tag =
                    MpegDescriptor::from_ctype(buf[es_info_pos] as ccx_mpeg_descriptor)
                        .unwrap_or(MpegDescriptor::CaptionService);
                es_info_pos += 1;
                desc_len = buf[es_info_pos];
                es_info_pos += 1;

                if descriptor_tag == MpegDescriptor::DataComp {
                    if !Codec::is_feasible(&ctx.codec, &ctx.nocodec, &Codec::IsdbCc) {
                        es_info_pos += desc_len as usize;
                        continue;
                    }
                    if desc_len < 2 {
                        break;
                    }

                    let component_id = if es_info_pos + 1 < buf.len() {
                        ((buf[es_info_pos] as i16) << 8) | (buf[es_info_pos + 1] as i16)
                    } else {
                        0
                    };

                    if component_id != 0x08 {
                        break;
                    }
                    info!("*****ISDB subtitles detected");
                    let ptr = init_isdb_decoder();
                    if ptr.is_null() {
                        break;
                    }
                    update_capinfo(
                        cap,
                        elementary_pid as i32,
                        stream_type,
                        Option::from(Codec::IsdbCc),
                        program_number as i32,
                        ptr,
                        ctx.flag_ts_forced_cappid,
                        ctx.ts_datastreamtype,
                    );
                }

                if descriptor_tag == MpegDescriptor::DvbSubtitle {
                    #[cfg(not(feature = "enable_ocr"))]
                    {
                        if ccx_options.write_format != OutputFormat::SpuPng {
                            info!("DVB subtitles detected, OCR subsystem not present. Use --out=spupng for graphic output");
                            es_info_pos += desc_len as usize;
                            continue;
                        }
                    }

                    if !Codec::is_feasible(&ctx.codec, &ctx.nocodec, &Codec::Dvb) {
                        es_info_pos += desc_len as usize;
                        continue;
                    }

                    let mut cnf = dvb_config::default();
                    ret = parse_dvb_description(
                        &mut cnf as *mut dvb_config,
                        buf.as_mut_ptr().add(es_info_pos),
                        desc_len as u32,
                    );
                    if ret < 0 {
                        break;
                    }
                    let ptr = dvbsub_init_decoder(
                        &mut cnf as *mut dvb_config,
                        if pinfo.initialized_ocr { 1 } else { 0 },
                    );
                    if !pinfo.initialized_ocr {
                        pinfo.initialized_ocr = true;
                    }
                    if ptr.is_null() {
                        break;
                    }
                    update_capinfo(
                        cap,
                        elementary_pid as i32,
                        stream_type,
                        Some(Codec::Dvb),
                        program_number as i32,
                        ptr,
                        ctx.flag_ts_forced_cappid,
                        ctx.ts_datastreamtype,
                    );
                    // max_dif = 30; // This would need to be handled globally
                }

                es_info_pos += desc_len as usize;
            }
        } else if stream_type == StreamType::PrivateUserMpeg2 && es_info_length > 0 {
            let es_info_start = current_pos + i + 5;
            let es_info_end = es_info_start + es_info_length as usize;
            let mut es_info_pos = es_info_start;

            while es_info_pos < es_info_end && es_info_pos + 1 < buf.len() {
                let descriptor_tag =
                    MpegDescriptor::from_ctype(buf[es_info_pos] as ccx_mpeg_descriptor)
                        .unwrap_or(MpegDescriptor::CaptionService);
                es_info_pos += 1;
                desc_len = buf[es_info_pos];
                es_info_pos += 1;

                if descriptor_tag == MpegDescriptor::CaptionService {
                    let nb_service = (buf[es_info_pos] & 0x1f) as usize;
                    let mut ser_i = 0;
                    while ser_i < nb_service && es_info_pos + 4 < buf.len() {
                        debug!(msg_type = DebugMessageFlag::PMT; "CC SERVICE {}: language ({}{}{})",
                              nb_service, buf[es_info_pos + 1] as char, buf[es_info_pos + 2] as char, buf[es_info_pos + 3] as char);
                        let is_608 = (buf[es_info_pos + 4] >> 7) != 0;
                        debug!(msg_type = DebugMessageFlag::PMT; "{}", if is_608 { " CEA-608" } else { " CEA-708" });
                        ser_i += 1;
                        es_info_pos += 5; // Move to next service
                    }
                }
                update_capinfo(
                    cap,
                    elementary_pid as i32,
                    stream_type,
                    Some(Codec::AtscCc),
                    program_number as i32,
                    ptr::null_mut(),
                    ctx.flag_ts_forced_cappid,
                    ctx.ts_datastreamtype,
                );
                es_info_pos += desc_len as usize;
            }
        }

        // Handle teletext
        if Codec::is_feasible(&ctx.codec, &ctx.nocodec, &Codec::Teletext)
            && es_info_length > 0
            && stream_type == StreamType::PrivateMpeg2
        {
            let es_info_start = current_pos + i + 5;
            let es_info_end = es_info_start + es_info_length as usize;
            let mut es_info_pos = es_info_start;

            while es_info_pos < es_info_end && es_info_pos + 1 < buf.len() {
                let descriptor_tag =
                    MpegDescriptor::from_ctype(buf[es_info_pos] as ccx_mpeg_descriptor)
                        .unwrap_or(MpegDescriptor::CaptionService);
                es_info_pos += 1;
                desc_len = buf[es_info_pos];
                es_info_pos += 1;
                if descriptor_tag.is_valid_teletext_desc() {
                    update_capinfo(
                        cap,
                        elementary_pid as i32,
                        stream_type,
                        Some(Codec::Teletext),
                        program_number as i32,
                        ptr::null_mut(),
                        ctx.flag_ts_forced_cappid,
                        ctx.ts_datastreamtype,
                    );
                    info!(
                        "VBI/teletext stream ID {} (0x{:x}) for SID {} (0x{:x})",
                        elementary_pid, elementary_pid, program_number, program_number
                    );
                }
                es_info_pos += desc_len as usize;
            }
        }

        // Handle non-teletext private streams
        if !Codec::is_feasible(&ctx.codec, &ctx.nocodec, &Codec::Teletext)
            && stream_type == StreamType::PrivateMpeg2
            && current_pos + i + 5 < buf.len()
        {
            let descriptor_tag = buf[current_pos + i + 5];
            if descriptor_tag == 0x45 {
                update_capinfo(
                    cap,
                    elementary_pid as i32,
                    stream_type,
                    Some(Codec::AtscCc),
                    program_number as i32,
                    ptr::null_mut(),
                    ctx.flag_ts_forced_cappid,
                    ctx.ts_datastreamtype,
                );
            }
        }

        // Handle video streams
        if stream_type == StreamType::VideoH264 || stream_type == StreamType::VideoMpeg2 {
            update_capinfo(
                cap,
                elementary_pid as i32,
                stream_type,
                Some(Codec::AtscCc),
                program_number as i32,
                ptr::null_mut(),
                ctx.flag_ts_forced_cappid,
                ctx.ts_datastreamtype,
            );
        }

        // Handle manually selected PID
        if need_cap_info_for_pid(cap, elementary_pid as i32) {
            if (stream_type as u16) >= 0x80 && (stream_type as u16) <= 0xFF {
                info!("I can't tell the stream type of the manually selected PID.");
                info!("Please pass -streamtype to select manually.");
                fatal!(cause = ExitCause::Failure; "-streamtype has to be manually selected.");
            }
            update_capinfo(
                cap,
                elementary_pid as i32,
                stream_type,
                Some(Codec::Any),
                program_number as i32,
                ptr::null_mut(),
                ctx.flag_ts_forced_cappid,
                ctx.ts_datastreamtype,
            );
            i += 5 + es_info_length as usize;
            continue;
        }
        i += 5 + es_info_length as usize;
    }

    pinfo.analysed_pmt_once = 1;

    // Verify CRC
    ret = if verify_crc32(&buf[..olen]) { 1 } else { 0 };
    if ret == 0 {
        pinfo.valid_crc = 0;
    } else {
        pinfo.valid_crc = 1;
    }

    pinfo.crc = crc;

    must_flush
}
