use crate::avc::common_types::*;
use crate::avc::nal::*;
use crate::avc::sei::*;
use crate::bindings::{cc_subtitle, encoder_ctx, lib_cc_decode, realloc};
use crate::ctorust::FromCType;
use crate::libccxr_exports::time::ccxr_set_fts;
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

    let nal_unit_type =
        AvcNalType::from_ctype(nal_start[0] & 0x1F).unwrap_or(AvcNalType::Unspecified0);

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

    // Get the working slice (skip first byte for remove_03emu)
    let mut working_buffer = nal_start[1..original_length].to_vec();

    let processed_length = match remove_03emu(&mut working_buffer) {
        Some(len) => len,
        None => {
            info!(
                "Notice: NAL of type {:?} had to be skipped because remove_03emu failed.",
                nal_unit_type
            );
            return Ok(());
        }
    };

    // Truncate buffer to actual processed length
    working_buffer.truncate(processed_length);

    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM;
        "BEGIN NAL unit type: {:?} length {} ref_idc: {} - Buffered captions before: {}",
        nal_unit_type,
        working_buffer.len(),
        (*dec_ctx.avc_ctx).nal_ref_idc,
        if (*dec_ctx.avc_ctx).cc_buffer_saved != 0 { 0 } else { 1 }
    );

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
                    let new_ptr = realloc((*dec_ctx.avc_ctx).cc_data as *mut c_void, new_size as _)
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
                (*dec_ctx.avc_ctx).cc_buffer_saved = if ctx_rust.cc_buffer_saved { 1 } else { 0 };
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

    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM;
        "END   NAL unit type: {:?} length {} ref_idc: {} - Buffered captions after: {}",
        nal_unit_type,
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

    // Warning there should be only leading zeros, nothing else
    if !(avcbuf[0] == 0x00 && avcbuf[1] == 0x00) {
        return Err(AvcError::BrokenStream(
            "Leading bytes are non-zero".to_string(),
        ));
    }

    let mut buffer_position = 2usize;
    let mut firstloop = true;

    // Loop over NAL units
    while buffer_position < avcbuflen.saturating_sub(2) {
        let mut zeropad = 0;

        // Find next NAL_start
        while buffer_position < avcbuflen {
            if avcbuf[buffer_position] == 0x01 {
                break;
            } else if firstloop && avcbuf[buffer_position] != 0x00 {
                return Err(AvcError::BrokenStream(
                    "Leading bytes are non-zero".to_string(),
                ));
            }
            buffer_position += 1;
            zeropad += 1;
        }

        firstloop = false;

        if buffer_position >= avcbuflen {
            break;
        }

        let nal_start_pos = buffer_position + 1;
        let mut nal_stop_pos = avcbuflen;

        buffer_position += 1;
        let restlen = avcbuflen.saturating_sub(buffer_position + 2);

        // Use optimized zero search
        if restlen > 0 {
            if let Some(zero_offset) =
                find_next_zero(&avcbuf[buffer_position..buffer_position + restlen])
            {
                let zero_pos = buffer_position + zero_offset;

                if zero_pos + 2 < avcbuflen {
                    if avcbuf[zero_pos + 1] == 0x00 && (avcbuf[zero_pos + 2] | 0x01) == 0x01 {
                        nal_stop_pos = zero_pos;
                        buffer_position = zero_pos + 2;
                    } else {
                        // Continue searching from after this zero
                        buffer_position = zero_pos + 1;
                        // Recursive search for next start code
                        while buffer_position < avcbuflen.saturating_sub(2) {
                            if let Some(next_zero_offset) = find_next_zero(
                                &avcbuf[buffer_position..avcbuflen.saturating_sub(2)],
                            ) {
                                let next_zero_pos = buffer_position + next_zero_offset;
                                if next_zero_pos + 2 < avcbuflen {
                                    if avcbuf[next_zero_pos + 1] == 0x00
                                        && (avcbuf[next_zero_pos + 2] | 0x01) == 0x01
                                    {
                                        nal_stop_pos = next_zero_pos;
                                        buffer_position = next_zero_pos + 2;
                                        break;
                                    }
                                } else {
                                    nal_stop_pos = avcbuflen;
                                    buffer_position = avcbuflen;
                                    break;
                                }
                                buffer_position = next_zero_pos + 1;
                            } else {
                                nal_stop_pos = avcbuflen;
                                buffer_position = avcbuflen;
                                break;
                            }
                        }
                    }
                } else {
                    nal_stop_pos = avcbuflen;
                    buffer_position = avcbuflen;
                }
            } else {
                nal_stop_pos = avcbuflen;
                buffer_position = avcbuflen;
            }
        } else {
            nal_stop_pos = avcbuflen;
            buffer_position = avcbuflen;
        }

        if nal_start_pos >= avcbuflen {
            break;
        }

        if (avcbuf[nal_start_pos] & 0x80) != 0 {
            let dump_start = nal_start_pos.saturating_sub(4);
            let dump_len = std::cmp::min(10, avcbuflen - dump_start);
            dump(avcbuf[dump_start..].as_ptr(), dump_len as i32, 0, 0);

            return Err(AvcError::ForbiddenZeroBit(
                "forbidden_zero_bit not zero".to_string(),
            ));
        }

        (*dec_ctx.avc_ctx).nal_ref_idc = (avcbuf[nal_start_pos] >> 5) as u32;

        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "process_avc: zeropad {}", zeropad);
        let nal_length = (nal_stop_pos - nal_start_pos) as i64;
        let mut nal_slice = avcbuf[nal_start_pos..nal_stop_pos].to_vec();

        if let Err(e) = do_nal(enc_ctx, dec_ctx, &mut nal_slice, nal_length, sub) {
            info!("Error processing NAL unit: {}", e);
        }
    }

    Ok(avcbuflen)
}
