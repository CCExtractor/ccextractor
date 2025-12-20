use crate::avc::common_types::*;
use crate::avc::nal::*;
use crate::avc::sei::*;
use crate::bindings::{cc_subtitle, encoder_ctx, lib_cc_decode, realloc};
use crate::ctorust::FromCType;
use crate::libccxr_exports::time::ccxr_set_fts;
use crate::{anchor_hdcc, current_fps, process_hdcc, store_hdcc, MPEG_CLOCK_FREQ};
use lib_ccxr::common::AvcNalType;
use lib_ccxr::util::log::DebugMessageFlag;
use lib_ccxr::{debug, info};
#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
use std::arch::x86_64::*;
use std::os::raw::c_void;
use std::slice;

/// Portable round function
pub fn round_portable(x: f64) -> f64 {
    (x + 0.5).floor()
}

// HEVC NAL unit types (defined for completeness, not all currently used)
#[allow(dead_code)]
const HEVC_NAL_TRAIL_N: u8 = 0;
#[allow(dead_code)]
const HEVC_NAL_TRAIL_R: u8 = 1;
#[allow(dead_code)]
const HEVC_NAL_TSA_N: u8 = 2;
#[allow(dead_code)]
const HEVC_NAL_TSA_R: u8 = 3;
#[allow(dead_code)]
const HEVC_NAL_STSA_N: u8 = 4;
#[allow(dead_code)]
const HEVC_NAL_STSA_R: u8 = 5;
#[allow(dead_code)]
const HEVC_NAL_RADL_N: u8 = 6;
#[allow(dead_code)]
const HEVC_NAL_RADL_R: u8 = 7;
#[allow(dead_code)]
const HEVC_NAL_RASL_N: u8 = 8;
#[allow(dead_code)]
const HEVC_NAL_RASL_R: u8 = 9;
#[allow(dead_code)]
const HEVC_NAL_BLA_W_LP: u8 = 16;
#[allow(dead_code)]
const HEVC_NAL_BLA_W_RADL: u8 = 17;
#[allow(dead_code)]
const HEVC_NAL_BLA_N_LP: u8 = 18;
const HEVC_NAL_IDR_W_RADL: u8 = 19;
const HEVC_NAL_IDR_N_LP: u8 = 20;
#[allow(dead_code)]
const HEVC_NAL_CRA_NUT: u8 = 21;
const HEVC_NAL_VPS: u8 = 32;
const HEVC_NAL_SPS: u8 = 33;
const HEVC_NAL_PPS: u8 = 34;
const HEVC_NAL_PREFIX_SEI: u8 = 39;
const HEVC_NAL_SUFFIX_SEI: u8 = 40;

/// Helper to check if HEVC NAL is a VCL (Video Coding Layer) type
fn is_hevc_vcl_nal(nal_type: u8) -> bool {
    // VCL NAL types are 0-31
    // Most common are 0-21 (slice types)
    nal_type <= 21
}

/// Process NAL unit data
/// # Safety
/// This function is unsafe because it processes raw NAL data
pub unsafe fn do_nal(
    enc_ctx: &mut encoder_ctx,
    dec_ctx: &mut lib_cc_decode,
    nal_start: &mut [u8],
    nal_length: i64,
    sub: &mut cc_subtitle,
) -> Result<(), Box<dyn std::error::Error>> {
    if nal_start.is_empty() {
        return Ok(());
    }

    let is_hevc = (*dec_ctx.avc_ctx).is_hevc != 0;
    let (nal_unit_type_raw, nal_header_size): (u8, usize) = if is_hevc {
        // HEVC: NAL type is in bits [6:1] of byte 0, header is 2 bytes
        ((nal_start[0] >> 1) & 0x3F, 2)
    } else {
        // H.264: NAL type is in bits [4:0] of byte 0, header is 1 byte
        (nal_start[0] & 0x1F, 1)
    };

    let nal_unit_type =
        AvcNalType::from_ctype(nal_unit_type_raw).unwrap_or(AvcNalType::Unspecified0);

    // Calculate NAL_stop pointer equivalent
    let original_length = nal_length as usize;
    if original_length > nal_start.len() {
        info!(
            "NAL length ({}) exceeds buffer size ({})",
            original_length,
            nal_start.len()
        );
        return Ok(());
    }

    // Get the working slice (skip header bytes for remove_03emu)
    if original_length <= nal_header_size {
        return Ok(());
    }
    let mut working_buffer = nal_start[nal_header_size..original_length].to_vec();

    let processed_length = match remove_03emu(&mut working_buffer) {
        Some(len) => len,
        None => {
            info!(
                "Notice: NAL of type {} had to be skipped because remove_03emu failed. (HEVC: {})",
                nal_unit_type_raw, is_hevc
            );
            return Ok(());
        }
    };

    // Truncate buffer to actual processed length
    working_buffer.truncate(processed_length);

    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM;
        "BEGIN NAL unit type: {} length {} ref_idc: {} - Buffered captions before: {} (HEVC: {})",
        nal_unit_type_raw,
        working_buffer.len(),
        (*dec_ctx.avc_ctx).nal_ref_idc,
        if (*dec_ctx.avc_ctx).cc_buffer_saved != 0 { 0 } else { 1 },
        is_hevc
    );

    if is_hevc {
        // HEVC NAL unit processing
        match nal_unit_type_raw {
            HEVC_NAL_VPS | HEVC_NAL_SPS | HEVC_NAL_PPS => {
                // Found HEVC parameter set - mark as having sequence params
                // We don't parse HEVC SPS fully, but we need to enable SEI processing
                (*dec_ctx.avc_ctx).got_seq_para = 1;
            }
            HEVC_NAL_PREFIX_SEI | HEVC_NAL_SUFFIX_SEI if (*dec_ctx.avc_ctx).got_seq_para != 0 => {
                // Found HEVC SEI (used for subtitles)
                // SEI payload format is similar to H.264
                ccxr_set_fts(enc_ctx.timing);

                // Load current context to preserve SPS info needed by sei_rbsp
                let mut ctx_rust = AvcContextRust::from_ctype(*dec_ctx.avc_ctx).unwrap();

                // Reset CC count for this SEI (don't accumulate across SEIs)
                // IMPORTANT: Don't clear cc_data vector - copy_ccdata_to_buffer checks
                // cc_data.len(), which becomes 0 if we clear(). Just reset cc_count.
                ctx_rust.cc_count = 0;

                sei_rbsp(&mut ctx_rust, &working_buffer);

                // If this SEI contained CC data, process it immediately
                if ctx_rust.cc_count > 0 {
                    let required_size = (ctx_rust.cc_count as usize * 3) + 1;
                    if required_size > (*dec_ctx.avc_ctx).cc_databufsize as usize {
                        let new_size = required_size * 2;
                        let new_ptr =
                            realloc((*dec_ctx.avc_ctx).cc_data as *mut c_void, new_size as _)
                                as *mut u8;
                        if new_ptr.is_null() {
                            return Err("Failed to realloc cc_data".into());
                        }
                        (*dec_ctx.avc_ctx).cc_data = new_ptr;
                        (*dec_ctx.avc_ctx).cc_databufsize = new_size as _;
                    }

                    if !(*dec_ctx.avc_ctx).cc_data.is_null() {
                        let c_data = slice::from_raw_parts_mut(
                            (*dec_ctx.avc_ctx).cc_data,
                            (*dec_ctx.avc_ctx).cc_databufsize as usize,
                        );
                        // Only copy the actual data (cc_count * 3 + 1 for 0xFF marker), not the full vector
                        let copy_len = required_size.min(c_data.len()).min(ctx_rust.cc_data.len());
                        c_data[..copy_len].copy_from_slice(&ctx_rust.cc_data[..copy_len]);
                    }

                    (*dec_ctx.avc_ctx).cc_count = ctx_rust.cc_count;
                    (*dec_ctx.avc_ctx).ccblocks_in_avc_total += ctx_rust.ccblocks_in_avc_total;
                    (*dec_ctx.avc_ctx).ccblocks_in_avc_lost += ctx_rust.ccblocks_in_avc_lost;

                    // Calculate PTS-based sequence number for proper B-frame reordering
                    // This is similar to H.264's PTS ordering mode

                    // Initialize currefpts on first frame if it hasn't been set yet
                    if (*dec_ctx.avc_ctx).currefpts == 0 && (*dec_ctx.timing).current_pts > 0 {
                        (*dec_ctx.avc_ctx).currefpts = (*dec_ctx.timing).current_pts;
                        anchor_hdcc(dec_ctx, 0);
                    }

                    // Calculate sequence index from PTS difference
                    let pts_diff = (*dec_ctx.timing).current_pts - (*dec_ctx.avc_ctx).currefpts;
                    let fps_factor = MPEG_CLOCK_FREQ as f64 / current_fps;
                    let calculated_index =
                        round_portable(2.0 * pts_diff as f64 / fps_factor) as i32;

                    // If the calculated index is out of range, flush buffer and reset
                    let current_index = if calculated_index.abs() >= MAXBFRAMES {
                        // Flush any buffered captions before resetting
                        if dec_ctx.has_ccdata_buffered != 0 {
                            process_hdcc(enc_ctx, dec_ctx, sub);
                        }
                        // Reset the reference point to current PTS
                        (*dec_ctx.avc_ctx).currefpts = (*dec_ctx.timing).current_pts;
                        anchor_hdcc(dec_ctx, 0);
                        0
                    } else {
                        calculated_index
                    };

                    store_hdcc(
                        enc_ctx,
                        dec_ctx,
                        (*dec_ctx.avc_ctx).cc_data,
                        ctx_rust.cc_count as i32,
                        current_index,
                        (*dec_ctx.timing).fts_now,
                        sub,
                    );
                    (*dec_ctx.avc_ctx).cc_buffer_saved = 1;
                }
            }
            // HEVC VCL NAL units (slice data)
            // Only flush on IDR frames (NAL types 19, 20) to allow B-frame reordering
            HEVC_NAL_IDR_W_RADL | HEVC_NAL_IDR_N_LP if (*dec_ctx.avc_ctx).got_seq_para != 0 => {
                // Found HEVC IDR frame - flush buffered CC data and reset anchor
                if dec_ctx.has_ccdata_buffered != 0 {
                    process_hdcc(enc_ctx, dec_ctx, sub);
                }
                // Reset reference point for new GOP
                (*dec_ctx.avc_ctx).currefpts = (*dec_ctx.timing).current_pts;
                anchor_hdcc(dec_ctx, 0);
            }
            // Other VCL NAL types - don't flush, let reordering work
            nal_type if is_hevc_vcl_nal(nal_type) && (*dec_ctx.avc_ctx).got_seq_para != 0 => {
                // Non-IDR slice - don't flush to allow B-frame reordering
            }
            _ => {
                // Other HEVC NAL types - ignore
            }
        }
    } else {
        // H.264 NAL unit processing (original code)
        match nal_unit_type {
            AvcNalType::AccessUnitDelimiter9 => {
                // Found Access Unit Delimiter
                debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "Found Access Unit Delimiter");
            }

            AvcNalType::SequenceParameterSet7 => {
                // Found sequence parameter set
                // We need this to parse NAL type 1 (CodedSliceNonIdrPicture1)
                (*dec_ctx.avc_ctx).num_nal_unit_type_7 += 1;

                let mut ctx_rust = AvcContextRust::from_ctype(*dec_ctx.avc_ctx).unwrap();
                seq_parameter_set_rbsp(&mut ctx_rust, &working_buffer)?;

                (*dec_ctx.avc_ctx).seq_parameter_set_id = ctx_rust.seq_parameter_set_id;
                (*dec_ctx.avc_ctx).log2_max_frame_num = ctx_rust.log2_max_frame_num;
                (*dec_ctx.avc_ctx).pic_order_cnt_type = ctx_rust.pic_order_cnt_type;
                (*dec_ctx.avc_ctx).log2_max_pic_order_cnt_lsb = ctx_rust.log2_max_pic_order_cnt_lsb;
                (*dec_ctx.avc_ctx).frame_mbs_only_flag =
                    if ctx_rust.frame_mbs_only_flag { 1 } else { 0 };
                (*dec_ctx.avc_ctx).num_nal_hrd = ctx_rust.num_nal_hrd as _;
                (*dec_ctx.avc_ctx).num_vcl_hrd = ctx_rust.num_vcl_hrd as _;

                (*dec_ctx.avc_ctx).got_seq_para = 1;
            }

            AvcNalType::CodedSliceNonIdrPicture1 | AvcNalType::CodedSliceIdrPicture
                if (*dec_ctx.avc_ctx).got_seq_para != 0 =>
            {
                // Found coded slice of a non-IDR picture or IDR picture
                // We only need the slice header data, no need to implement
                // slice_layer_without_partitioning_rbsp( );
                slice_header(enc_ctx, dec_ctx, &mut working_buffer, &nal_unit_type, sub)?;
            }

            AvcNalType::Sei if (*dec_ctx.avc_ctx).got_seq_para != 0 => {
                // Found SEI (used for subtitles)
                ccxr_set_fts(enc_ctx.timing);

                let mut ctx_rust = AvcContextRust::from_ctype(*dec_ctx.avc_ctx).unwrap();
                let old_cc_count = ctx_rust.cc_count;

                sei_rbsp(&mut ctx_rust, &working_buffer);

                if ctx_rust.cc_count > old_cc_count {
                    let required_size = (ctx_rust.cc_count as usize * 3) + 1;
                    if required_size > (*dec_ctx.avc_ctx).cc_databufsize as usize {
                        let new_size = required_size * 2; // Some headroom
                        let new_ptr =
                            realloc((*dec_ctx.avc_ctx).cc_data as *mut c_void, new_size as _)
                                as *mut u8;
                        if new_ptr.is_null() {
                            return Err("Failed to realloc cc_data".into());
                        }
                        (*dec_ctx.avc_ctx).cc_data = new_ptr;
                        (*dec_ctx.avc_ctx).cc_databufsize = new_size as _;
                    }

                    if !(*dec_ctx.avc_ctx).cc_data.is_null() {
                        let c_data = slice::from_raw_parts_mut(
                            (*dec_ctx.avc_ctx).cc_data,
                            (*dec_ctx.avc_ctx).cc_databufsize as usize,
                        );
                        let rust_data_len = ctx_rust.cc_data.len().min(c_data.len());
                        c_data[..rust_data_len].copy_from_slice(&ctx_rust.cc_data[..rust_data_len]);
                    }

                    // Update the essential fields
                    (*dec_ctx.avc_ctx).cc_count = ctx_rust.cc_count;
                    (*dec_ctx.avc_ctx).cc_buffer_saved =
                        if ctx_rust.cc_buffer_saved { 1 } else { 0 };
                    (*dec_ctx.avc_ctx).ccblocks_in_avc_total = ctx_rust.ccblocks_in_avc_total;
                    (*dec_ctx.avc_ctx).ccblocks_in_avc_lost = ctx_rust.ccblocks_in_avc_lost;
                }
            }

            AvcNalType::PictureParameterSet if (*dec_ctx.avc_ctx).got_seq_para != 0 => {
                // Found Picture parameter set
                debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "Found Picture parameter set");
            }

            _ => {
                // Handle other NAL unit types or do nothing
            }
        }
    }

    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM;
        "END   NAL unit type: {} length {} ref_idc: {} - Buffered captions after: {}",
        nal_unit_type_raw,
        working_buffer.len(),
        (*dec_ctx.avc_ctx).nal_ref_idc,
        if (*dec_ctx.avc_ctx).cc_buffer_saved != 0 { 0 } else { 1 }
    );
    Ok(())
}

/// Remove emulation prevention bytes (0x000003 sequences)
pub fn remove_emulation_prevention_bytes(input: &[u8]) -> Vec<u8> {
    let mut output = Vec::with_capacity(input.len());
    let mut i = 0;
    let mut consecutive_zeros = 0;

    while i < input.len() {
        let byte = input[i];

        if consecutive_zeros == 2 && byte == 0x03 {
            // Skip emulation prevention byte
            consecutive_zeros = 0;
        } else {
            output.push(byte);
            if byte == 0x00 {
                consecutive_zeros += 1;
            } else {
                consecutive_zeros = 0;
            }
        }
        i += 1;
    }

    output
}

/// Copied for reference decoder, see if it behaves different that Volker's code
fn EBSPtoRBSP(stream_buffer: &mut [u8], end_bytepos: usize, begin_bytepos: usize) -> isize {
    let mut count = 0i32;

    if end_bytepos < begin_bytepos {
        return end_bytepos as isize;
    }

    let mut j = begin_bytepos;
    let mut i = begin_bytepos;

    while i < end_bytepos {
        // starting from begin_bytepos to avoid header information
        // in NAL unit, 0x000000, 0x000001 or 0x000002 shall not occur at any byte-aligned position
        if count == ZEROBYTES_SHORTSTARTCODE && stream_buffer[i] < 0x03 {
            return -1;
        }

        if count == ZEROBYTES_SHORTSTARTCODE && stream_buffer[i] == 0x03 {
            // check the 4th byte after 0x000003, except when cabac_zero_word is used,
            // in which case the last three bytes of this NAL unit must be 0x000003
            if (i < end_bytepos - 1) && (stream_buffer[i + 1] > 0x03) {
                return -1;
            }

            // if cabac_zero_word is used, the final byte of this NAL unit(0x03) is discarded,
            // and the last two bytes of RBSP must be 0x0000
            if i == end_bytepos - 1 {
                return j as isize;
            }

            i += 1;
            count = 0;
        }

        stream_buffer[j] = stream_buffer[i];

        if stream_buffer[i] == 0x00 {
            count += 1;
        } else {
            count = 0;
        }

        j += 1;
        i += 1;
    }

    j as isize
}

fn remove_03emu(buffer: &mut [u8]) -> Option<usize> {
    let num = buffer.len();
    let newsize = EBSPtoRBSP(buffer, num, 0);

    if newsize == -1 {
        None
    } else {
        Some(newsize as usize)
    }
}
/// Copy CC data to buffer
pub fn copy_ccdata_to_buffer(ctx: &mut AvcContextRust, source: &[u8], new_cc_count: usize) {
    ctx.ccblocks_in_avc_total += 1;
    if !ctx.cc_buffer_saved {
        info!("Warning: Probably loss of CC data, unsaved buffer being rewritten");
        ctx.ccblocks_in_avc_lost += 1;
    }

    // Copy the cc data
    let start_idx = ctx.cc_count as usize * 3;
    let copy_len = new_cc_count * 3 + 1;

    if start_idx + copy_len <= ctx.cc_data.len() {
        ctx.cc_data[start_idx..start_idx + copy_len].copy_from_slice(&source[..copy_len]);
        ctx.cc_count += new_cc_count as u8;
        ctx.cc_buffer_saved = false;
    }
}

/// Simple hex dump function for debugging
/// # Safety
/// This function is safe as it only reads data
pub fn hex_dump(data: &[u8]) {
    for (i, chunk) in data.chunks(16).enumerate() {
        print!("{:08X} | ", i * 16);

        // Print hex bytes
        for byte in chunk {
            print!("{:02X} ", byte);
        }

        // Pad if less than 16 bytes
        for _ in chunk.len()..16 {
            print!("   ");
        }

        print!(" | ");

        // Print ASCII representation
        for byte in chunk {
            if *byte >= 32 && *byte <= 126 {
                print!("{}", *byte as char);
            } else {
                print!(".");
            }
        }

        println!();
    }
}
/// Temporarily placed here, import from ts_core when ts module is merged
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
#[derive(Debug)]
pub enum AvcError {
    InsufficientBuffer(String),
    BrokenStream(String),
    ForbiddenZeroBit(String),
    Other(String),
}
#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
fn find_next_zero(slice: &[u8]) -> Option<usize> {
    let len = slice.len();
    let mut i = 0;
    let ptr = slice.as_ptr();
    if is_x86_feature_detected!("sse2") {
        unsafe {
            let zero = _mm_set1_epi8(0);
            while i + 16 <= len {
                let chunk = _mm_loadu_si128(ptr.add(i) as *const __m128i);
                let cmp = _mm_cmpeq_epi8(chunk, zero);
                let mask = _mm_movemask_epi8(cmp);
                if mask != 0 {
                    return Some(i + mask.trailing_zeros() as usize);
                }
                i += 16;
            }
        }
    }
    slice[i..]
        .iter()
        .position(|&b| b == 0x00)
        .map(|pos| i + pos)
}

#[cfg(not(any(target_arch = "x86", target_arch = "x86_64")))]
fn find_next_zero(slice: &[u8]) -> Option<usize> {
    slice.iter().position(|&b| b == 0x00)
}
/// Find the first NAL start code (0x00 0x00 0x01 or 0x00 0x00 0x00 0x01) in a buffer.
/// Returns the position of the 0x01 byte if found, or None if not found.
fn find_nal_start_code(buf: &[u8]) -> Option<usize> {
    if buf.len() < 3 {
        return None;
    }

    for i in 0..buf.len().saturating_sub(2) {
        // Check for 0x00 0x00 0x01 (3-byte start code)
        if buf[i] == 0x00 && buf[i + 1] == 0x00 && buf[i + 2] == 0x01 {
            return Some(i + 2); // Position of the 0x01
        }
        // Also check for 0x00 0x00 0x00 0x01 (4-byte start code)
        if i + 3 < buf.len()
            && buf[i] == 0x00
            && buf[i + 1] == 0x00
            && buf[i + 2] == 0x00
            && buf[i + 3] == 0x01
        {
            return Some(i + 3); // Position of the 0x01
        }
    }
    None
}

/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls `dump` and `do_nal`.
pub unsafe fn process_avc(
    enc_ctx: &mut encoder_ctx,
    dec_ctx: &mut lib_cc_decode,
    avcbuf: &mut [u8],
    sub: &mut cc_subtitle,
) -> Result<usize, AvcError> {
    let avcbuflen = avcbuf.len();

    // At least 5 bytes are needed for a NAL unit
    if avcbuflen <= 5 {
        return Err(AvcError::InsufficientBuffer(
            "NAL unit needs at least 5 bytes in the buffer to process AVC video stream".to_string(),
        ));
    }

    // If the buffer doesn't start with leading zeros, try to find the first NAL start code.
    // This can happen with:
    // - HLS/Twitch stream segments that start mid-stream
    // - Streams with garbage data at the beginning
    // - Buffer accumulation issues after previous errors
    let start_offset = if avcbuf[0] == 0x00 && avcbuf[1] == 0x00 {
        // Normal case: buffer starts with zeros
        0
    } else {
        // Try to find the first NAL start code
        if let Some(nal_pos) = find_nal_start_code(avcbuf) {
            // Found a NAL start code, skip to the position before it (the zeros)
            // The position returned is the 0x01, so we need to go back to find the zeros
            let zeros_start = if nal_pos >= 3 && avcbuf[nal_pos - 3] == 0x00 {
                nal_pos - 3 // 4-byte start code
            } else {
                nal_pos - 2 // 3-byte start code
            };
            debug!(msg_type = DebugMessageFlag::VERBOSE;
                "Skipped {} bytes of garbage before first NAL start code", zeros_start);
            zeros_start
        } else {
            // No NAL start code found - return full buffer length to clear it
            debug!(msg_type = DebugMessageFlag::VERBOSE;
                "No NAL start code found in buffer of {} bytes, clearing", avcbuflen);
            return Ok(avcbuflen);
        }
    };

    // Work with the buffer starting from start_offset
    let working_buf = &avcbuf[start_offset..];
    let working_len = working_buf.len();

    if working_len <= 5 {
        // Not enough data after skipping garbage
        return Ok(avcbuflen);
    }

    let mut buffer_position = 2usize;

    // Loop over NAL units
    while buffer_position < working_len.saturating_sub(2) {
        let mut zeropad = 0;

        // Find next NAL_start
        while buffer_position < working_len {
            if working_buf[buffer_position] == 0x01 {
                break;
            } else if working_buf[buffer_position] != 0x00 {
                // Non-zero byte found where we expected zeros - skip to next potential start code
                if let Some(next_nal) = find_nal_start_code(&working_buf[buffer_position..]) {
                    buffer_position += next_nal - 1; // -1 because we'll increment at end of loop
                    zeropad = 0;
                } else {
                    // No more NAL units found
                    return Ok(avcbuflen);
                }
            }
            buffer_position += 1;
            zeropad += 1;
        }

        if buffer_position >= working_len {
            break;
        }

        let nal_start_pos = buffer_position + 1;
        let mut nal_stop_pos = working_len;

        buffer_position += 1;
        let restlen = working_len.saturating_sub(buffer_position + 2);

        // Use optimized zero search
        if restlen > 0 {
            if let Some(zero_offset) =
                find_next_zero(&working_buf[buffer_position..buffer_position + restlen])
            {
                let zero_pos = buffer_position + zero_offset;

                if zero_pos + 2 < working_len {
                    if working_buf[zero_pos + 1] == 0x00
                        && (working_buf[zero_pos + 2] | 0x01) == 0x01
                    {
                        nal_stop_pos = zero_pos;
                        buffer_position = zero_pos + 2;
                    } else {
                        // Continue searching from after this zero
                        buffer_position = zero_pos + 1;
                        // Recursive search for next start code
                        while buffer_position < working_len.saturating_sub(2) {
                            if let Some(next_zero_offset) = find_next_zero(
                                &working_buf[buffer_position..working_len.saturating_sub(2)],
                            ) {
                                let next_zero_pos = buffer_position + next_zero_offset;
                                if next_zero_pos + 2 < working_len {
                                    if working_buf[next_zero_pos + 1] == 0x00
                                        && (working_buf[next_zero_pos + 2] | 0x01) == 0x01
                                    {
                                        nal_stop_pos = next_zero_pos;
                                        buffer_position = next_zero_pos + 2;
                                        break;
                                    }
                                } else {
                                    nal_stop_pos = working_len;
                                    buffer_position = working_len;
                                    break;
                                }
                                buffer_position = next_zero_pos + 1;
                            } else {
                                nal_stop_pos = working_len;
                                buffer_position = working_len;
                                break;
                            }
                        }
                    }
                } else {
                    nal_stop_pos = working_len;
                    buffer_position = working_len;
                }
            } else {
                nal_stop_pos = working_len;
                buffer_position = working_len;
            }
        } else {
            nal_stop_pos = working_len;
            buffer_position = working_len;
        }

        if nal_start_pos >= working_len {
            break;
        }

        if (working_buf[nal_start_pos] & 0x80) != 0 {
            let dump_start = nal_start_pos.saturating_sub(4);
            let dump_len = std::cmp::min(10, working_len - dump_start);
            dump(working_buf[dump_start..].as_ptr(), dump_len as i32, 0, 0);

            // Don't return an error - just skip this NAL and continue
            // This allows processing to continue even with some corrupt data
            debug!(msg_type = DebugMessageFlag::VERBOSE;
                "Skipping NAL with forbidden_zero_bit set");
            continue;
        }

        (*dec_ctx.avc_ctx).nal_ref_idc = (working_buf[nal_start_pos] >> 5) as u32;

        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "process_avc: zeropad {}", zeropad);
        let nal_length = (nal_stop_pos - nal_start_pos) as i64;
        let mut nal_slice = working_buf[nal_start_pos..nal_stop_pos].to_vec();

        if let Err(e) = do_nal(enc_ctx, dec_ctx, &mut nal_slice, nal_length, sub) {
            info!("Error processing NAL unit: {}", e);
        }
    }

    Ok(avcbuflen)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_find_nal_start_code_3byte() {
        // 3-byte start code at position 0
        let buf = [0x00, 0x00, 0x01, 0x65, 0x88];
        assert_eq!(find_nal_start_code(&buf), Some(2));
    }

    #[test]
    fn test_find_nal_start_code_4byte() {
        // 4-byte start code at position 0
        let buf = [0x00, 0x00, 0x00, 0x01, 0x67, 0x64];
        assert_eq!(find_nal_start_code(&buf), Some(3));
    }

    #[test]
    fn test_find_nal_start_code_with_garbage() {
        // Garbage data followed by 3-byte start code
        let buf = [0xFF, 0xAB, 0xCD, 0x00, 0x00, 0x01, 0x09, 0xF0];
        assert_eq!(find_nal_start_code(&buf), Some(5));
    }

    #[test]
    fn test_find_nal_start_code_no_start_code() {
        // No start code in buffer
        let buf = [0xFF, 0xAB, 0xCD, 0xEF];
        assert_eq!(find_nal_start_code(&buf), None);
    }

    #[test]
    fn test_find_nal_start_code_too_short() {
        // Buffer too short
        let buf = [0x00, 0x00];
        assert_eq!(find_nal_start_code(&buf), None);
    }

    #[test]
    fn test_find_nal_start_code_empty() {
        let buf: [u8; 0] = [];
        assert_eq!(find_nal_start_code(&buf), None);
    }

    #[test]
    fn test_find_nal_start_code_partial_match() {
        // 0x00 0x00 but no 0x01 following
        let buf = [0x00, 0x00, 0x02, 0x00, 0x00, 0x01, 0x65];
        assert_eq!(find_nal_start_code(&buf), Some(5));
    }
}
