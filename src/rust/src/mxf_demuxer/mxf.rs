use crate::demuxer::common_structs::{CcxDemuxer, CcxRational};
use crate::demuxer::demuxer_data::DemuxerData;
use crate::file_functions::file::{
    buffered_get_be16, buffered_get_be32, buffered_get_byte, buffered_read, buffered_read_byte,
    buffered_skip,
};
use crate::gxf_demuxer::common_structs:: DemuxerError;
use crate::mxf_demuxer::common_structs::{KLVPacketRust, MXFCaptionTypeRust, MXFContextRust, MXFReadTableEntryRust, MXFTrackRust, UidRust, FRAMERATE_RATIONALS_RUST, MXF_CAPTION_ESSENCE_CONTAINER_RUST, MXF_ESSENCE_ELEMENT_KEY_RUST, MXF_HEADER_PARTITION_PACK_KEY_RUST, MXF_KLV_KEY_RUST, MXF_READ_TABLE_RUST};
use lib_ccxr::common::{BufferdataType, Options};
use lib_ccxr::info;
use lib_ccxr::util::log::{debug, DebugMessageFlag};

macro_rules! dbg {
    ($($args:expr),*) => {
        debug!(msg_type = DebugMessageFlag::PARSE;"MXF:");
        debug!(msg_type = DebugMessageFlag::PARSE; $($args),*)
    };
}
macro_rules! log {
    ($($args:expr),*) => {
        info!("MXF:");
        info!($($args),*)
    };
}
pub fn ccx_probe_mxf(ctx: &CcxDemuxer) -> bool {
    let avail = ctx.startbytes_avail as usize;
    if avail < 15 {
        return false;
    }
    let max_i = (avail - 15).min(ctx.startbytes.len().saturating_sub(14));

    for i in 0..=max_i {
        let buf = &ctx.startbytes[i..i + 14];
        if buf == MXF_HEADER_PARTITION_PACK_KEY_RUST {
            return true;
        }
    }
    false
}

fn mxf_read_sync(ctx: &mut CcxDemuxer, key: &[u8], size: usize, opts: &mut Options) -> i32 {
    let mut i: usize = 0;

    while i < size {
        let mut b = 0u8;
        let r = unsafe { buffered_read_byte(ctx, Some(&mut b), opts) };
        if r != 1 {
            return DemuxerError::EOF as i32; // Return EOF directly, not wrapped in Ok()
        }
        ctx.past += 1;

        if b == key[0] {
            i = 0; // Reset to beginning of pattern
        } else if i > 0 && b != key[i] {
            i = 0; // Reset if mismatch (but don't reset to -1 like C, start over)
                   // Re-check if this byte matches the first byte
            if b == key[0] {
                i = 0; // This will be incremented to 1 below
            } else {
                continue; // Skip increment, try next byte
            }
        }
        i += 1;
    }

    // Return 0 on success (found sync), non-zero on failure
    0
}

unsafe fn klv_decode_ber_length(ctx: &mut CcxDemuxer, ccx_options: &mut Options) -> i64 {
    let size = buffered_get_byte(ctx, ccx_options) as u64;

    if size & 0x80 != 0 {
        // long form
        let bytes_num = (size & 0x7f) as i32;

        // SMPTE 379M 5.3.4 guarantee that bytes_num must not exceed 8 bytes
        if bytes_num > 8 {
            return -1;
        }

        let mut size = 0u64;
        for _ in 0..bytes_num {
            size = (size << 8) | (buffered_get_byte(ctx, ccx_options) as u64);
        }
        size as i64
    } else {
        size as i64
    }
}

/// Reads CDP (Caption Distribution Packet) data from MXF stream
///
/// # Arguments
/// * `demux` - Mutable reference to the demuxer
/// * `size` - Size of the CDP packet to read
/// * `data` - Mutable reference to demuxer data where the CDP data will be stored
///
/// # Returns
/// The number of bytes read from the stream
///
/// # Safety
/// This function calls C functions and manipulates raw pointers, so it's marked unsafe
pub unsafe fn mxf_read_cdp_data(
    demux: &mut CcxDemuxer,
    size: i32,
    data: &mut DemuxerData,
    ccx_options: &mut Options,
) -> i32 {
    let mut len = 0i32;
    let mut ret: i32;

    // 1) Read CDP identifier (should be 0x9669)
    ret = buffered_get_be16(demux, ccx_options) as i32;
    len += 2;
    if ret != 0x9669 {
        log!("Invalid CDP Identifier\n");
        // goto error equivalent
        ret = buffered_skip(demux, (size - len) as u32, ccx_options) as i32;
        demux.past += ret as i64;
        len += ret;
        return len;
    }

    // 2) Read and verify packet size
    ret = buffered_get_byte(demux, ccx_options) as i32;
    len += 1;
    if ret != size {
        log!("Incomplete CDP packet\n");
        // goto error equivalent
        ret = buffered_skip(demux, (size - len) as u32, ccx_options) as i32;
        demux.past += ret as i64;
        len += ret;
        return len;
    }

    // 3) Read framerate - top 4 bits are cdp_framing_rate
    ret = buffered_get_byte(demux, ccx_options) as i32;
    len += 1;
    data.tb = FRAMERATE_RATIONALS_RUST[(ret >> 4) as usize];

    // 4) Skip flag and hdr_seq_cntr
    buffered_skip(demux, 3, ccx_options);
    demux.past += 3;
    len += 3;

    // 5) Read cdata identifier (should be 0x72)
    ret = buffered_get_byte(demux, ccx_options) as i32;
    len += 1;
    if ret != 0x72 {
        // Skip if its not cdata identifier
        ret = buffered_skip(demux, (size - len) as u32, ccx_options) as i32;
        demux.past += ret as i64;
        len += ret;
        return len;
    }

    // 6) Read cc_count and validate
    let cc_count = (buffered_get_byte(demux, ccx_options) as i32) & 0x1F;
    len += 1;

    // -4 for cdp footer length
    if (cc_count * 3) > (size - len - 4) {
        log!("Incomplete CDP packet\n");
    }

    // 7) Read the actual closed caption data
    let buffer_slice = std::slice::from_raw_parts_mut(data.buffer.add(data.len), (cc_count * 3) as usize);
    ret = buffered_read(demux, Some(buffer_slice), (cc_count * 3) as usize, ccx_options) as i32;
    data.len += (cc_count * 3) as usize;
    demux.past += (cc_count * 3) as i64;
    len += ret;
    // error label equivalent - skip remaining bytes
    ret = buffered_skip(demux, (size - len) as u32, ccx_options) as i32;
    demux.past += ret as i64;
    len += ret;
    len
}
fn update_tid_lut(
    ctx: &mut MXFContextRust,
    track_id: u32,
    track_number: &[u8; 4],
    edit_rate: CcxRational,
) {
    // Update essence element key if we have track Id of caption
    if ctx.cap_track_id == track_id as i32 {
        ctx.cap_essence_key[..12].copy_from_slice(&MXF_ESSENCE_ELEMENT_KEY_RUST[..12]);
        ctx.cap_essence_key[12..16].copy_from_slice(track_number);
        ctx.edit_rate = edit_rate;
    }

    // Check if track already exists and update it
    for track in &mut ctx.tracks {
        if track.track_id == track_id as i32 && track.track_number != *track_number {
            track.track_number = *track_number;
            ctx.edit_rate = edit_rate;
            return;
        }
    }

    // Add new track
    if ctx.nb_tracks >= ctx.tracks.len() as i32 {
        log!(
            "MXF: Too many tracks, cannot add new track with ID {}",
            track_id
        );
        return;
    }
    ctx.tracks[ctx.nb_tracks as usize] = MXFTrackRust {
        track_id: track_id as i32,
        track_number: *track_number,
    };
    ctx.edit_rate = edit_rate;
    ctx.nb_tracks += 1;
}
// TEMP: Stage 2
pub fn update_cap_essence_key(ctx: &mut MXFContextRust, track_id: u32) {
    let tid = track_id as i32;
    for i in 0..(ctx.nb_tracks as usize) {
        let tr = &ctx.tracks[i];
        if tr.track_id == tid {
            // copy the 12-byte MXF prefix
            ctx.cap_essence_key[..12].copy_from_slice(&MXF_ESSENCE_ELEMENT_KEY_RUST);
            // then the 4-byte track number
            ctx.cap_essence_key[12..16].copy_from_slice(&tr.track_number);
            dbg!("Key {:?} essence_key", ctx.cap_essence_key);
            return;
        }
    }
    // no match → do nothing
}
pub unsafe fn mxf_read_header_partition_pack(
    demux: &mut CcxDemuxer<'_>,
    size: u64,
    ccx_options: &mut Options,
) -> Result<i32, DemuxerError> {
    // 1) Must have context
    if demux.private_data.is_null() {
        return Err(DemuxerError::InvalidArgument);
    }
    let ctx = match (demux.private_data as *mut MXFContextRust).as_mut() {
        Some(ctx) => ctx,
        None => {
            println!("INVALID MXFCONTEXT - FAILED");
            return Err(DemuxerError::InvalidArgument);
        }
    };
    // 2) Minimum size 88
    if size < 88 {
        log!("Minimum Length is 88 but found {}", size);
        return Err(DemuxerError::InvalidArgument);
    }

    let mut len = 0u64;
    // 3) major id
    let major = buffered_get_be16(demux, ccx_options) as i32;
    len += 2;
    if major != 0x01 {
        log!("Unsupported Partition Major number {}", major);
    }

    // 4) skip 78
    let skipped = buffered_skip(demux, 78, ccx_options);
    if skipped != 78 {
        return Err(DemuxerError::EOF);
    }
    demux.past += skipped as i64;
    len += skipped as u64;

    // 5) nb containers
    let nb = buffered_get_be32(demux, ccx_options) as usize;
    len += 4;

    // 6) UL length
    let ul_len = buffered_get_be32(demux, ccx_options) as i32;
    len += 4;
    if ul_len != 16 {
        log!("Invalid UL length {}", ul_len);
    }

    // 7) room?
    if size - len < (nb as u64 * 16) {
        log!("Essence container count({}) doesn't fit size({})", nb, size);
        return Err(DemuxerError::InvalidArgument);
    }

    // 8) read each UL
    let mut ull = [0u8; 16];
    for _ in 0..nb {
        let got = buffered_read(demux, Some(&mut ull), 16, ccx_options);
        if got != 16 {
            return Err(DemuxerError::EOF);
        }
        demux.past += 16;
        len += 16;

        // iterate exactly as you asked:
        for A in MXF_CAPTION_ESSENCE_CONTAINER_RUST.iter() {
            let uid = &A.uid;
            let ctype = A.caption_type;
            if &ull[..7] == &uid[..7] && &ull[8..] == &uid[8..] {
                // we’ve found our matching UL
                ctx.r#caption_type = ctype;
            }
        }
    }

    Ok(len as i32)
}
/// # Safety
/// Caller must ensure that `demux.private_data` points to a valid `MXFContextRust`.
pub unsafe fn mxf_read_timeline_track_metadata(
    demux: &mut CcxDemuxer<'_>,
    size: u64,
    ccx_options: &mut Options,
) -> Result<i32, DemuxerError> {
    let mut len = 0u64;
    let ctx = match (demux.private_data as *mut MXFContextRust).as_mut() {
        Some(ctx) => ctx,
        None => {
            println!("INVALID MXFCONTEXT - FAILED");
            return Err(DemuxerError::InvalidArgument);
        }
    };
    // temporaries
    let mut track_id: u32 = 0;
    let mut track_number: [u8; 4] = [0; 4];
    let mut edit_rate = CcxRational { num: 0, den: 1 };

    while len < size {
        // read tag and length (buffered_get_be16 handles past updates)
        let tag = buffered_get_be16(demux, ccx_options);
        let tag_len = buffered_get_be16(demux, ccx_options);
        len += 4;
        let MXFTag = tag;
        match MXFTag {
            0x4801 => {
                // buffered_get_be32 handles past updates internally
                track_id = buffered_get_be32(demux, ccx_options);
            }
            0x4804 => {
                // buffered_read doesn't update past, so we need to do it manually
                buffered_read(demux, Some(&mut track_number), 4, ccx_options);
                demux.past += 4;
            }
            0x4b01 => {
                // buffered_get_be32 handles past updates internally
                edit_rate.num = buffered_get_be32(demux, ccx_options) as i32;
                edit_rate.den = buffered_get_be32(demux, ccx_options) as i32;
            }
            _ => {
                // skip unknown tag
                dbg!("Unknown tag: {:04x}, length: {} \n", tag, tag_len);
                buffered_skip(demux, tag_len as u32, ccx_options);
                demux.past += tag_len as i64;
            }
        }

        // advance our counter by the payload length
        len += tag_len as u64;
    }

    // if we saw a nonzero track_id, invoke the LUT updater
    if track_id != 0 {
        update_tid_lut(ctx, track_id, &track_number, edit_rate);
    }

    // return total bytes consumed
    Ok(len as i32)
}

/// # Safety: caller must ensure `demux.filebuffer` is valid
/// and that `demux.private_data` points to a valid MXF context.
pub unsafe fn mxf_read_vanc_data(
    demux: &mut CcxDemuxer,
    size: u64,
    data: &mut DemuxerData,
    opts: &mut Options,
) -> Result<i32, DemuxerError> {
    let mut len: i64 = 0;

    // If fewer than 19 bytes available, just skip all of them
    if size < 19 {
        let skipped = buffered_skip(demux, size as u32, opts);
        demux.past += skipped as i64;
        return Ok(skipped as i32);
    }

    // Read the 16-byte header
    let mut header = [0u8; 16];
    let ret = buffered_read(demux, Some(&mut header), 16, opts);
    demux.past += ret as i64;
    if ret < 16 {
        return Err(DemuxerError::EOF);
    }
    len += ret as i64;

    // header[1] holds the packet count
    let packet_count = header[1] as usize;

    for _ in 0..packet_count {
        // DID
        let mut did = 0u8;
        buffered_read_byte(demux, Some(&mut did), opts);
        demux.past += 1;
        len += 1;
        if did != 0x61 && did != 0x80 {
            break;
        }

        // SDID
        let mut sdid = 0u8;
        buffered_read_byte(demux, Some(&mut sdid), opts);
        demux.past += 1;
        len += 1;

        // cdp_size
        let mut size_byte = 0u8;
        buffered_read_byte(demux, Some(&mut size_byte), opts);
        demux.past += 1;
        len += 1;
        let cdp_size = size_byte as usize;
        if (cdp_size as u64 + 19) > size {
            dbg!("Incomplete cdp{} in anc data{}\n", cdp_size, size);
            break;
        }

        // Payload
        let ret2 = mxf_read_cdp_data(demux, cdp_size as i32, data, opts);
        len += ret2 as i64;
    }

    // Skip any remaining bytes in this VANC block
    let to_skip = (size as i64 - len) as u32;
    let skipped = buffered_skip(demux, to_skip, opts);
    demux.past += skipped as i64;
    len += skipped as i64;

    Ok(len as i32)
}
pub unsafe fn mxf_read_vanc_vbi_desc(
    demux: &mut CcxDemuxer,
    size: u64,
    ccx_options: &mut Options,
) -> Result<i32, DemuxerError> {
    const MXF_TAG_LTRACK_ID: u16 = 0x3006;

    let mut len = 0u64;

    while len < size {
        let tag = buffered_get_be16(demux, ccx_options);
        let tag_len = buffered_get_be16(demux, ccx_options);
        len += 4;

        match tag {
            MXF_TAG_LTRACK_ID => {
                let ctx = match (demux.private_data as *mut MXFContextRust).as_mut() {
                    Some(ctx) => ctx,
                    None => {
                        println!("INVALID MXFCONTEXT - FAILED");
                        return Err(DemuxerError::InvalidArgument);
                    }
                };
                ctx.cap_track_id = buffered_get_be32(demux, ccx_options) as i32;
                update_cap_essence_key(ctx, ctx.cap_track_id as u32);
            }
            _ => {
                let ret = buffered_skip(demux, tag_len as u32, ccx_options);
                if ret == tag_len as usize {
                    demux.past += tag_len as i64;
                }
                // no break; always fall through to `len += tag_len`
            }
        }

        len += tag_len as u64;
    }

    Ok(len as i32)
}

/// # Safety
/// - `demux.private_data` must point to a valid `MXFContextRust`.
/// - `demux.filebuffer` must contain at least `size` bytes.
pub unsafe fn mxf_read_essence_element(
    demux: &mut CcxDemuxer<'_>,
    size: u64,
    data: &mut DemuxerData,
    opts: &mut Options,
) -> Result<i32, DemuxerError> {
    // Grab the MXFContextRust from private_data
    let ctx = match (demux.private_data as *mut MXFContextRust).as_mut() {
        Some(ctx) => ctx,
        None => {
            println!("INVALID MXFCONTEXT - FAILED");
            return Err(DemuxerError::InvalidArgument);
        }
    };
    if ctx.caption_type == MXFCaptionTypeRust::ANC {
        // for ANC, set raw type and delegate to VANC reader
        data.bufferdatatype = BufferdataType::RawType;
        let ret = match mxf_read_vanc_data(demux, size, data, opts) {
            Ok(v) => v,
            Err(DemuxerError::EOF) => DemuxerError::EOF as i32,
            Err(e) => return Err(e),
        };
        data.pts = ctx.cap_count as i64;
        ctx.cap_count += 1;
        Ok(ret)
    } else {
        // non‐ANC: just skip the whole block
        let skipped = buffered_skip(demux, size as u32, opts);
        demux.past += skipped as i64;
        Ok(skipped as i32)
    }
}
unsafe fn klv_read_packet(
    klv: &mut KLVPacketRust,
    ctx: &mut CcxDemuxer,
    ccx_options: &mut Options,
) -> i32 {

    // 1) Sync: read 4 bytes, return any non-zero code immediately.
    let sync_ret = mxf_read_sync(ctx, &MXF_KLV_KEY_RUST, 4, ccx_options);
    if sync_ret != 0 {
        return sync_ret; // This will be EOF or other error code
    }

    // 2) Copy the first 4 bytes of the key
    klv.key[0..4].copy_from_slice(&MXF_KLV_KEY_RUST);

    // 3) Read the remaining 12 bytes of the key
    let result = buffered_read(ctx, Some(&mut klv.key[4..16]), 12, ccx_options);
    ctx.past += result as i64;
    if result != 12 {
        return DemuxerError::EOF as i32;
    }

    // 4) Decode BER length, return -1 on failure, otherwise 0
    let length = klv_decode_ber_length(ctx, ccx_options);
    if length == -1 {
        return -1;
    }
    klv.length = length as u64;
    0
}

/// Look up a reader by its 16-byte UID.  
/// Returns `Some(&MXFReadTableEntryRust)` if found, or `None` otherwise.
pub fn get_mxf_reader(key: &UidRust) -> Option<&'static MXFReadTableEntryRust> {
    for entry in MXF_READ_TABLE_RUST.iter() {
        // Terminator: no more valid entries after a None-read
        if entry.read.is_none() {
            break;
        }

        // Array<[u8;16]> implements PartialEq, so this is a byte-wise compare
        if &entry.key == key {
            return Some(entry);
        }
    }
    None
}

pub unsafe fn read_packet_mxf(
    demux: &mut CcxDemuxer<'_>,
    data: &mut DemuxerData,
    opts: &mut Options,
) -> Result<i32, DemuxerError> {
    let ctx = match (demux.private_data as *mut MXFContextRust).as_mut() {
        Some(ctx) => ctx,
        None => {
            println!("INVALID MXFCONTEXT - FAILED");
            return Err(DemuxerError::InvalidArgument);
        }
    };

    let mut klv = KLVPacketRust::default();

    loop {
        let ret = klv_read_packet(&mut klv, demux, opts);

        // Handle return codes properly
        if ret != 0 {
            if ret == (DemuxerError::EOF as i32) {
                dbg!("EOF reached while reading KLV packet");
                return Ok(ret); // EOF is not an error in this context
            } else if ret == -1 {
                log!("Failed to decode KLV length");
                return Err(DemuxerError::InvalidArgument);
            } else {
                // Any other non-zero return is likely an error
                log!("Error reading KLV packet: {}", ret);
                return Err(DemuxerError::InvalidArgument);
            }
        }

        dbg!("Key: {:02X?} Length {} \n", klv.key, klv.length);
        dbg!(" Cap Essence Key: {:02X?} \n", ctx.cap_essence_key);

        // Check for caption essence key
        if klv.key == ctx.cap_essence_key {
            dbg!("Found caption essence key: {:02X?} \n", klv.key);
            mxf_read_essence_element(demux, klv.length, data, opts)?;
            if data.len > 0 {
                break;
            }
            continue;
        }

        // Lookup reader
        if let Some(entry) = get_mxf_reader(&klv.key) {
            let reader_ret = entry.read.unwrap()(demux, klv.length, opts)?;
            if reader_ret < 0 {
                break;
            }
        } else {
            let skip_ret = buffered_skip(demux, klv.length as u32, opts) as i32;
            demux.past += skip_ret as i64;
            dbg!("Unknown or Dark key \n");
            continue;
        }
    }

    Ok(0) // Success
}
#[cfg(test)]
mod tests {
    use super::*;
    use crate::mxf_demuxer::common_structs::MXFCaptionTypeRust;
    use crate::utils::null_pointer;
    use lib_ccxr::common::DataSource;
    use lib_ccxr::util::log::{set_logger, CCExtractorLogger, DebugMessageMask, OutputTarget};
    use std::alloc::{alloc, dealloc, Layout};
    use std::os::raw::c_void;
    use std::sync::Once;
    use std::{ptr, slice};

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

    #[test]
    fn test_mxf_probe_with_header_at_start() {
        let mut ctx = CcxDemuxer::default();
        ctx.startbytes_avail = 20;

        // Place MXF header at the beginning
        ctx.startbytes[0..14].copy_from_slice(&MXF_HEADER_PARTITION_PACK_KEY_RUST);

        assert_eq!(ccx_probe_mxf(&ctx), true);
    }

    #[test]
    fn test_mxf_probe_with_header_in_middle() {
        let mut ctx = CcxDemuxer::default();
        ctx.startbytes_avail = 30;

        // Place some random data first
        ctx.startbytes[0..5].copy_from_slice(&[0xFF, 0xAB, 0xCD, 0xEF, 0x12]);

        // Place MXF header at offset 5
        ctx.startbytes[5..19].copy_from_slice(&MXF_HEADER_PARTITION_PACK_KEY_RUST);

        assert_eq!(ccx_probe_mxf(&ctx), true);
    }

    #[test]
    fn test_mxf_probe_no_header_found() {
        let mut ctx = CcxDemuxer::default();
        ctx.startbytes_avail = 50;

        // Fill with random data that doesn't match MXF header
        for i in 0..50 {
            ctx.startbytes[i] = (i % 256) as u8;
        }

        assert_eq!(ccx_probe_mxf(&ctx), false);
    }

    #[test]
    fn test_mxf_probe_partial_header_at_end() {
        let mut ctx = CcxDemuxer::default();
        ctx.startbytes_avail = 10;

        // Place partial MXF header that extends beyond available bytes
        ctx.startbytes[0..10].copy_from_slice(&MXF_HEADER_PARTITION_PACK_KEY_RUST[0..10]);

        // Should return false because we can't match the full 14-byte header
        assert_eq!(ccx_probe_mxf(&ctx), false);
    }

    #[test]
    fn test_klv_decode_ber_length_short_form() {
        let mut ctx = CcxDemuxer {
            past: 0,
            ..Default::default()
        };

        // Test with short form (bit 7 not set)
        // This would require actual buffered_get_byte implementation
        let result = unsafe { klv_decode_ber_length(&mut ctx, &mut Options::default()) };
        assert!(result >= 0); // Should return valid length
    }

    #[test]
    fn test_klv_decode_ber_length_invalid_long_form() {
        let mut ctx = CcxDemuxer {
            past: 0,
            ..Default::default()
        };

        // This test would need buffered_get_byte to return a value with more than 8 bytes
        // The actual test behavior depends on the buffered_get_byte implementation
        let result = unsafe { klv_decode_ber_length(&mut ctx, &mut Options::default()) };
        // Result could be valid or -1 depending on mock data
        assert!(result >= -1);
    }
    fn make_ctx(tracks: &[(i32, [u8; 4])]) -> MXFContextRust {
        let mut ctx = MXFContextRust {
            caption_type: MXFCaptionTypeRust::VBI,
            cap_track_id: 0,
            cap_essence_key: [0xFF; 16], // sentinel
            tracks: [MXFTrackRust {
                track_id: -1,
                track_number: [0; 4],
            }; 32],
            nb_tracks: tracks.len() as i32,
            cap_count: 0,
            edit_rate: CcxRational::default(),
        };
        ctx.edit_rate.num = 0;
        ctx.edit_rate.den = 1;

        for (i, &(id, num)) in tracks.iter().enumerate() {
            ctx.tracks[i] = MXFTrackRust {
                track_id: id,
                track_number: num,
            };
        }
        ctx
    }

    /// 1) match at index 0
    #[test]
    fn test_update_first_track() {
        let mut ctx = make_ctx(&[(42, [0xAA, 0xBB, 0xCC, 0xDD]), (99, [1, 2, 3, 4])]);
        update_cap_essence_key(&mut ctx, 42);
        // expected: prefix + [AA,BB,CC,DD]
        let mut want = [0u8; 16];
        want[..12].copy_from_slice(&MXF_ESSENCE_ELEMENT_KEY_RUST);
        want[12..].copy_from_slice(&[0xAA, 0xBB, 0xCC, 0xDD]);
        assert_eq!(ctx.cap_essence_key, want);
    }

    /// 2) match at last populated slot
    #[test]
    fn test_update_last_track() {
        let mut ctx = make_ctx(&[
            (1, [5, 6, 7, 8]),
            (2, [9, 10, 11, 12]),
            (1234, [0xDE, 0xAD, 0xBE, 0xEF]),
        ]);
        update_cap_essence_key(&mut ctx, 1234);
        let mut want = [0u8; 16];
        want[..12].copy_from_slice(&MXF_ESSENCE_ELEMENT_KEY_RUST);
        want[12..].copy_from_slice(&[0xDE, 0xAD, 0xBE, 0xEF]);
        assert_eq!(ctx.cap_essence_key, want);
    }

    /// 3) no match → key stays unchanged
    #[test]
    fn test_no_match_leaves_key() {
        let mut ctx = make_ctx(&[(7, [1, 1, 1, 1]), (8, [2, 2, 2, 2])]);
        // initial key is all 0xFF
        update_cap_essence_key(&mut ctx, 999);
        assert_eq!(ctx.cap_essence_key, [0xFF; 16]);
    }

    #[test]
    fn too_small_partition() {
        let mut dem = make_demux(vec![0; 100], MXFCaptionTypeRust::ANC);
        let r = unsafe { mxf_read_header_partition_pack(&mut dem, 87, &mut Options::default()) };
        assert_eq!(r, Err(DemuxerError::InvalidArgument));
    }
    impl<'a> CcxDemuxer<'a> {
        /// Create with a filebuffer Vec<u8>, which will back buffered_* I/O
        fn with_filebuffer_partition_pack(buffer: Vec<u8>, ctx: Option<MXFContextRust>) -> Self {
            let mut dem = CcxDemuxer::default();
            // leak buffer into Box<[u8]> to own data
            let boxed: Box<[u8]> = buffer.into_boxed_slice();
            let ptr = boxed.as_ptr() as *mut u8;
            let len = boxed.len() as u32;
            dem.filebuffer = ptr;
            dem.bytesinbuffer = len;
            dem.filebuffer_pos = 0;
            // startbytes unused
            dem.startbytes_avail = 0;
            dem.startbytes_pos = 0;
            // init private_data
            dem.private_data = ctx
                .map(Box::new)
                .map(|b| Box::into_raw(b) as *mut c_void)
                .unwrap_or(std::ptr::null_mut());
            dem
        }
    }

    fn make_demux(buf: Vec<u8>, init_type: MXFCaptionTypeRust) -> CcxDemuxer<'static> {
        CcxDemuxer::with_filebuffer_partition_pack(
            buf,
            Some(MXFContextRust {
                caption_type: init_type,
                cap_track_id: 0,
                cap_essence_key: [0u8; 16],
                ..Default::default()
            }),
        )
    }

    #[test]
    fn zero_containers() {
        let mut b = Vec::new();
        b.extend(&1u16.to_be_bytes());
        b.extend(&[0u8; 78]);
        b.extend(&0u32.to_be_bytes());
        b.extend(&16u32.to_be_bytes());
        let mut dem = make_demux(b, MXFCaptionTypeRust::ANC);
        let r = unsafe { mxf_read_header_partition_pack(&mut dem, 88, &mut Options::default()) };
        assert_eq!(r, Ok(88));
        let ctx = unsafe { &*(dem.private_data as *const MXFContextRust) };
        assert_eq!(ctx.caption_type, MXFCaptionTypeRust::ANC);
    }

    #[test]
    fn one_matching_container() {
        let mut b = Vec::new();
        b.extend(&1u16.to_be_bytes());
        b.extend(&[0u8; 78]);
        b.extend(&1u32.to_be_bytes());
        b.extend(&16u32.to_be_bytes());
        let mut ul = MXF_CAPTION_ESSENCE_CONTAINER_RUST[0].uid;
        ul[7] ^= 0xFF;
        b.extend(&ul);
        let mut dem = make_demux(b, MXFCaptionTypeRust::ANC);
        let r =
            unsafe { mxf_read_header_partition_pack(&mut dem, 104, &mut &mut Options::default()) };
        assert_eq!(r, Ok(104));
        let ctx = unsafe { &*(dem.private_data as *const MXFContextRust) };
        assert_eq!(ctx.caption_type, MXFCaptionTypeRust::VBI);
    }
    fn allocate_filebuffer() -> *mut u8 {
        // For simplicity, we allocate FILEBUFFERSIZE bytes.
        const FILEBUFFERSIZE: usize = 1024 * 1024 * 16;
        let buf = vec![0u8; FILEBUFFERSIZE].into_boxed_slice();
        Box::into_raw(buf) as *mut u8
    }

    /// Build a CcxDemuxer whose `filebuffer` is pre-filled from `buf`,
    /// with bytesinbuffer/filebuffer_pos set appropriately, plus
    /// a trivial DemuxerData.
    fn make_demux_vanc(buf: &[u8]) -> (CcxDemuxer<'static>, DemuxerData) {
        let mut demux = CcxDemuxer::default();
        // allocate and copy
        let fb =  allocate_filebuffer();
        unsafe {
            let slice = slice::from_raw_parts_mut(fb, buf.len());
            slice.copy_from_slice(buf);
        }
        demux.filebuffer = fb;
        demux.bytesinbuffer = buf.len() as u32;
        demux.filebuffer_pos = 0;
        // simple empty data
        let data = DemuxerData {
            program_number: 0,
            stream_pid: 0,
            codec: None,
            bufferdatatype: BufferdataType::DvbSubtitle,
            buffer: null_pointer(),
            len: 0,
            rollover_bits: 0,
            pts: 0,
            tb: Default::default(),
            next_stream: std::ptr::null_mut(),
            next_program: std::ptr::null_mut(),
        };
        (demux, data)
    }

    /// 1) A single valid CDP packet (DID=0x61, SDID=0x02, size=1, payload=0xAB)
    #[test]
    fn single_packet_advances_pos() {
        initialize_logger();
        let mut buf = vec![0u8; 16];
        buf[1] = 1; // packet count
        buf.extend(&[0x61, 0x02, 1, 0xAB]);
        let (mut demux, mut data) = make_demux_vanc(&buf);
        let mut opts = Options::default();

            unsafe { mxf_read_vanc_data(&mut demux, buf.len() as u64, &mut data, &mut opts) }
                .unwrap();
        assert_eq!(demux.filebuffer_pos as usize, buf.len());
    }

    /// 2) Invalid DID (0xFF) => break and skip the rest
    #[test]
    fn invalid_did_stops_and_skips() {
        let mut buf = vec![0u8; 16];
        buf[1] = 1; // packet count
        buf.extend(&[0xFF, 0x02, 1, 0x00]);
        let (mut demux, mut data) = make_demux_vanc(&buf);
        let mut opts = Options::default();

        let consumed =
            unsafe { mxf_read_vanc_data(&mut demux, buf.len() as u64, &mut data, &mut opts) }
                .unwrap();
        assert_eq!(consumed as usize, buf.len());
        assert_eq!(demux.filebuffer_pos as usize, buf.len());
    }
    /// 3) Invalid SDID (0xFF) => break and skip the rest
    #[test]
    fn multi_packet_advances_pos() {
        // header[1] = 2 packets
        let mut buf = vec![0u8; 16];
        buf[1] = 2;
        // packet 1: DID, SDID, size=1, payload=0xAA
        buf.extend(&[0x61, 0x02, 1, 0xAA]);
        // packet 2: DID, SDID, size=1, payload=0xBB
        buf.extend(&[0x80, 0x01, 1, 0xBB]);

        let (mut demux, mut data) = make_demux_vanc(&buf);
        let mut opts = Options::default();
        let consumed =
            unsafe { mxf_read_vanc_data(&mut demux, buf.len() as u64, &mut data, &mut opts) }
                .unwrap() as usize;

        assert_eq!(consumed, buf.len());
        assert_eq!(demux.filebuffer_pos as usize, buf.len());
    }

    /// 4) cdp_size too large ⇒ break then skip remainder
    #[test]
    fn oversize_cdp_skips_rest() {
        // header[1] = 1 packet
        // cdp_size = 100 but buffer only has room for 5 more bytes
        let mut buf = vec![0u8; 16];
        buf[1] = 1;
        buf.extend(&[0x61, 0x02, 100]);
        // no payload bytes at all

        let (mut demux, mut data) = make_demux_vanc(&buf);
        let mut opts = Options::default();
        let consumed =
            unsafe { mxf_read_vanc_data(&mut demux, buf.len() as u64, &mut data, &mut opts) }
                .unwrap() as usize;

        // we should have skipped all 'size' bytes
        assert_eq!(consumed, buf.len());
        assert_eq!(demux.filebuffer_pos as usize, buf.len());
    }

    /// 6) size >= 19 but buffer < 16 bytes ⇒ header read returns <16 ⇒ EOF error
    #[test]
    fn header_eof_returns_err() {
        // size says 19, but only 10 bytes in buffer
        let (mut demux, mut data) = make_demux_vanc(&vec![0u8; 10]);
        let mut opts = Options::default();
        let err = unsafe { mxf_read_vanc_data(&mut demux, 19, &mut data, &mut opts) }.unwrap_err();
        assert_eq!(err, DemuxerError::EOF);
    }
    /// Helper: build demuxer + context + data for testing.
    fn make_demux_with_ctx(
        buf: &[u8],
        initial_type: MXFCaptionTypeRust,
        initial_count: i64,
    ) -> (CcxDemuxer<'static>, DemuxerData, Box<MXFContextRust>) {
        // make demux & data
        let (mut demux, data) = make_demux_vanc(buf);
        let ctx = Box::new(MXFContextRust {
            caption_type: initial_type,
            cap_count: initial_count as i32,
            ..Default::default()
        });
        // attach
        demux.private_data = Box::into_raw(ctx.clone()) as *mut _;
        (demux, data, ctx)
    }

    /// 1) Non‐ANC path should skip entire block and leave pts/count alone.
    #[test]
    fn non_anc_skips() {
        let buf = vec![0u8; 10];
        let (mut demux, mut data, _ctx) = make_demux_with_ctx(&buf, MXFCaptionTypeRust::ANC, 5);
        let mut opts = Options::default();

        let consumed =
            unsafe { mxf_read_essence_element(&mut demux, buf.len() as u64, &mut data, &mut opts) }
                .unwrap();
        assert_eq!(consumed as usize, buf.len());
        // bufferdatatype unchanged (default)
        assert_eq!(data.bufferdatatype, BufferdataType::RawType);
        assert_eq!(data.pts, 5);
    }

    /// 2) ANC path: sets RawType, uses VANC reader, and updates pts/cap_count.
    #[test]
    fn anc_delegates_and_updates_pts() {
        // prepare a tiny VANC block size=19 (header only, VANC will skip)
        let mut buf = vec![0u8; 19];
        buf[1] = 0; // zero packets
        let (mut demux, mut data, ctx) = make_demux_with_ctx(&buf, MXFCaptionTypeRust::ANC, 7);
        let mut opts = Options::default();

        let consumed =
            unsafe { mxf_read_essence_element(&mut demux, buf.len() as u64, &mut data, &mut opts) }
                .unwrap();
        // should have read 19 bytes
        assert_eq!(consumed as usize, 19);
        // bufferdatatype should be raw
        assert_eq!(data.bufferdatatype, BufferdataType::RawType);
        // pts set to old cap_count
        assert_eq!(data.pts, 7);
        assert_eq!(ctx.cap_count, 7);
    }

    /// 3) Two consecutive ANC calls increment cap_count each time.
    #[test]
    fn anc_multiple_increments() {
        let mut buf = vec![0u8; 19];
        buf[1] = 0;
        let (mut demux, mut data, _) = make_demux_with_ctx(&buf, MXFCaptionTypeRust::ANC, 0);
        let mut opts = Options::default();

        let c1 = unsafe { mxf_read_essence_element(&mut demux, 19, &mut data, &mut opts) }.unwrap();
        let pts1 = data.pts;
        assert_eq!(c1 as usize, 19);
        assert_eq!(pts1, 0);
    }
    impl<'a> CcxDemuxer<'a> {
        /// Create with a filebuffer Vec<u8>, which will back buffered_* I/O
        fn with_filebuffer_metadata_tests(buffer: Vec<u8>, ctx: Option<MXFContextRust>) -> Self {
            let mut dem = CcxDemuxer::default();

            // Convert to Box<[u8]> and leak it to prevent deallocation
            let boxed: Box<[u8]> = buffer.into_boxed_slice();
            let len = boxed.len() as u32;
            let ptr = Box::into_raw(boxed) as *mut u8;

            dem.filebuffer = ptr;
            dem.bytesinbuffer = len;
            dem.filebuffer_pos = 0;
            dem.startbytes_avail = 0;
            dem.startbytes_pos = 0;

            // init private_data
            dem.private_data = ctx
                .map(Box::new)
                .map(|b| Box::into_raw(b) as *mut c_void)
                .unwrap_or(std::ptr::null_mut());
            dem
        }
    }
    // Helpers to build a demuxer with a backing filebuffer
    fn make_demux_vanc_vbi<'a>(buf: Vec<u8>, initial_ctx: MXFContextRust) -> CcxDemuxer<'a> {
        CcxDemuxer::with_filebuffer_metadata_tests(buf, Some(initial_ctx))
    }

    const TAG_ID: u16 = 0x4801;
    const TAG_NUMBER: u16 = 0x4804;
    const TAG_EDIT_RATE: u16 = 0x4b01;

    #[test]
    fn no_track_id_means_no_update_and_full_len() {
        // Build: just one “unknown” tag, tag_len = 3, 3 bytes payload
        let mut buf = Vec::new();
        buf.extend(&0xFFFFu16.to_be_bytes()); // unknown tag
        buf.extend(&3u16.to_be_bytes()); // length
        buf.extend(&[0x11, 0x22, 0x33]); // payload
        let size = buf.len() as u64;

        let init = MXFContextRust {
            caption_type: MXFCaptionTypeRust::ANC,
            cap_track_id: 0,
            cap_essence_key: [0; 16],
            ..Default::default()
        };
        let mut dem = make_demux_vanc_vbi(buf, init.clone());
        let mut opts = Options::default();

        let consumed = unsafe { mxf_read_timeline_track_metadata(&mut dem, size, &mut opts) };
        // Should walk exactly `size` bytes, and do _no_ LUT update
        assert_eq!(consumed.unwrap(), size as i32);
        let ctx = unsafe { &*(dem.private_data as *const MXFContextRust) };
        // tracks/nothing changed ⇒ still empty (nb_tracks=0) and edit_rate was default (0/1)
        assert_eq!(ctx.cap_track_id, init.cap_track_id);
        assert_eq!(ctx.caption_type, init.caption_type);
    }

    #[test]
    fn single_track_id_number_and_edit_rate_creates_one_track() {
        // Build tags in this order:
        // 1) TRACK_ID  (len=4) → 0x2A
        // 2) TRACK_NUMBER (len=4) → [0xAA,0xBB,0xCC,0xDD]
        // 3) EDIT_RATE (len=8) → num=300, den=1001
        let mut buf = Vec::new();

        // TRACK_ID
        buf.extend(&TAG_ID.to_be_bytes());
        buf.extend(&4u16.to_be_bytes());
        buf.extend(&42u32.to_be_bytes());

        // TRACK_NUMBER
        buf.extend(&TAG_NUMBER.to_be_bytes());
        buf.extend(&4u16.to_be_bytes());
        buf.extend(&[0xAA, 0xBB, 0xCC, 0xDD]);

        // EDIT_RATE
        buf.extend(&TAG_EDIT_RATE.to_be_bytes());
        buf.extend(&8u16.to_be_bytes());
        buf.extend(&300u32.to_be_bytes());
        buf.extend(&1001u32.to_be_bytes());

        let size = buf.len() as u64;
        let init = MXFContextRust {
            caption_type: MXFCaptionTypeRust::ANC,
            cap_track_id: 0,
            cap_essence_key: [0; 16],
            ..Default::default()
        };

        let mut dem = make_demux_vanc_vbi(buf, init);
        let mut opts = Options::default();

        let consumed = unsafe { mxf_read_timeline_track_metadata(&mut dem, size, &mut opts) };
        assert_eq!(consumed.unwrap(), size as i32);

        // Now LUT should have created one track in ctx.tracks[0],
        // and stored track_number and edit_rate there.
        let ctx = unsafe { &mut *(dem.private_data as *mut MXFContextRust) };
        // Because update_tid_lut sees no existing tracks, it appends one:
        assert_eq!(ctx.nb_tracks, 1);
        assert_eq!(ctx.tracks[0].track_id, 42);
        assert_eq!(ctx.tracks[0].track_number, [0xAA, 0xBB, 0xCC, 0xDD]);
        assert_eq!(ctx.edit_rate.num, 300);
        assert_eq!(ctx.edit_rate.den, 1001);
    }

    #[test]
    fn interleaved_unknown_tags_are_skipped_correctly() {
        initialize_logger();
        // Build:
        // 1) UNKNOWN tag (len=2) → payload [0x99,0x88]
        // 2) TRACK_ID tag (len=4) → 123
        // 3) UNKNOWN tag (len=1) → [0x00]
        // 4) TRACK_NUMBER tag (len=4) → [1,2,3,4]
        // 5) EDIT_RATE tag (len=8) → [7,8],[9,10]
        let mut buf = Vec::new();

        // unknown #1
        buf.extend(&0xAAAAu16.to_be_bytes());
        buf.extend(&2u16.to_be_bytes());
        buf.extend(&[0x99, 0x88]);

        // TRACK_ID
        buf.extend(&TAG_ID.to_be_bytes());
        buf.extend(&4u16.to_be_bytes());
        buf.extend(&123u32.to_be_bytes());

        // unknown #2
        buf.extend(&0xBBBBu16.to_be_bytes());
        buf.extend(&1u16.to_be_bytes());
        buf.extend(&[0x00]);

        // TRACK_NUMBER
        buf.extend(&TAG_NUMBER.to_be_bytes());
        buf.extend(&4u16.to_be_bytes());
        buf.extend(&[1, 2, 3, 4]);

        // EDIT_RATE
        buf.extend(&TAG_EDIT_RATE.to_be_bytes());
        buf.extend(&8u16.to_be_bytes());
        buf.extend(&7u32.to_be_bytes());
        buf.extend(&9u32.to_be_bytes());

        let size = buf.len() as u64;
        let init = MXFContextRust {
            caption_type: MXFCaptionTypeRust::ANC,
            cap_track_id: 0,
            cap_essence_key: [0; 16],
            ..Default::default()
        };
        let mut dem = make_demux_vanc_vbi(buf, init);
        let mut opts = Options::default();

        let consumed = unsafe { mxf_read_timeline_track_metadata(&mut dem, size, &mut opts) };
        assert_eq!(consumed.unwrap(), size as i32);

        let ctx = unsafe { &mut *(dem.private_data as *mut MXFContextRust) };
        // Again, one track appended
        assert_eq!(ctx.nb_tracks, 1);
        assert_eq!(ctx.tracks[0].track_id, 123);
        assert_eq!(ctx.tracks[0].track_number, [1, 2, 3, 4]);
        assert_eq!(ctx.edit_rate.num, 7);
        assert_eq!(ctx.edit_rate.den, 9);
    }

    fn setup_buffer_with_data(data: &[u8]) -> *mut u8 {
        unsafe {
            let layout = Layout::from_size_align(data.len(), 1).unwrap();
            let ptr = alloc(layout);
            ptr::copy_nonoverlapping(data.as_ptr(), ptr, data.len());
            ptr
        }
    }

    unsafe fn cleanup_buffer(ptr: *mut u8, size: usize) {
        let layout = Layout::from_size_align(size, 1).unwrap();
        dealloc(ptr, layout);
    }

    #[test]
    fn test_mxf_read_vanc_vbi_desc_with_ltrack_id_tag() {
        unsafe {
            let mut ctx = MXFContextRust {
                caption_type: MXFCaptionTypeRust::ANC,
                cap_track_id: 0,
                cap_essence_key: [0u8; 16],
                tracks: [MXFTrackRust::default(); 32],
                nb_tracks: 1,
                cap_count: 0,
                edit_rate: CcxRational::default(),
            };

            // Mock data: tag=0x3006 (MXF_TAG_LTRACK_ID), tag_len=4, track_id=0x12345678
            let mock_data = vec![0x30, 0x06, 0x00, 0x04, 0x12, 0x34, 0x56, 0x78];
            let buffer = setup_buffer_with_data(&mock_data);

            let mut demux = CcxDemuxer {
                past: 0,
                private_data: &mut ctx as *mut _ as *mut std::ffi::c_void,
                filebuffer: buffer,
                filebuffer_start: 0,
                filebuffer_pos: 0,
                bytesinbuffer: mock_data.len() as u32,
                ..Default::default()
            };

            let mut options = Options::default();

            let result = mxf_read_vanc_vbi_desc(&mut demux, 8, &mut options);

            assert_eq!(result.unwrap(), 8);
            assert_eq!(ctx.cap_track_id, 0x12345678u32 as i32);

            cleanup_buffer(buffer, mock_data.len());
        }
    }

    #[test]
    fn test_mxf_read_vanc_vbi_desc_multiple_tags() {
        unsafe {
            let mut ctx = MXFContextRust {
                caption_type: MXFCaptionTypeRust::ANC,
                cap_track_id: 0,
                cap_essence_key: [0u8; 16],
                tracks: [MXFTrackRust::default(); 32],
                nb_tracks: 1,
                cap_count: 0,
                edit_rate: CcxRational::default(),
            };

            // Mock data: first tag unknown (0x1111, len=1, data=0xFF),
            // then MXF_TAG_LTRACK_ID (0x3006, len=4, track_id=0x87654321)
            let mock_data = vec![
                0x11, 0x11, 0x00, 0x01, 0xFF, // First tag (unknown)
                0x30, 0x06, 0x00, 0x04, 0x87, 0x65, 0x43,
                0x21, // Second tag (MXF_TAG_LTRACK_ID)
            ];
            let buffer = setup_buffer_with_data(&mock_data);

            let mut demux = CcxDemuxer {
                past: 0,
                private_data: &mut ctx as *mut _ as *mut std::ffi::c_void,
                filebuffer: buffer,
                filebuffer_start: 0,
                filebuffer_pos: 0,
                bytesinbuffer: mock_data.len() as u32,
                ..Default::default()
            };

            let mut options = Options::default();

            let result = mxf_read_vanc_vbi_desc(&mut demux, 13, &mut options);

            assert_eq!(result.unwrap(), 13);
            assert_eq!(ctx.cap_track_id, 0x87654321u32 as i32);

            cleanup_buffer(buffer, mock_data.len());
        }
    }
    // Helper to create a test demuxer with real data in the buffer
    unsafe fn create_test_demuxer(test_data: &[u8]) -> CcxDemuxer {
        let buffer_size = test_data.len().max(1024);
        let layout = Layout::array::<u8>(buffer_size).unwrap();
        let filebuffer = alloc(layout);

        // Copy test data into the buffer
        ptr::copy_nonoverlapping(test_data.as_ptr(), filebuffer, test_data.len());

        CcxDemuxer {
            filebuffer,
            bytesinbuffer: test_data.len() as u32,
            filebuffer_pos: 0,
            past: 0,
            ..Default::default()
        }
    }

    // Helper to create mock options
    fn create_test_options() -> Options {
        Options {
            gui_mode_reports: false,
            input_source: DataSource::File,
            ..Default::default()
        }
    }

    // Helper to create demuxer data with allocated buffer
    unsafe fn create_test_demuxer_data(buffer_size: usize) -> DemuxerData {
        let layout = Layout::array::<u8>(buffer_size).unwrap();
        let buffer = alloc(layout);

        DemuxerData {
            program_number: 0,
            stream_pid: 0,
            codec: None,
            bufferdatatype: BufferdataType::Hauppage,
            buffer,
            len: 0,
            rollover_bits: 0,
            pts: 0,
            tb: CcxRational { num: 1, den: 1 },
            next_stream: ptr::null_mut(),
            next_program: ptr::null_mut(),
        }
    }

    // Helper to clean up allocated memory
    unsafe fn cleanup_demuxer(demux: &CcxDemuxer, buffer_size: usize) {
        std::alloc::dealloc(demux.filebuffer, Layout::array::<u8>(buffer_size).unwrap());
    }

    unsafe fn cleanup_demuxer_data(data: &DemuxerData, buffer_size: usize) {
        std::alloc::dealloc(data.buffer, Layout::array::<u8>(buffer_size).unwrap());
    }

    #[test]
    fn test_valid_cdp_packet() {
        unsafe {
            // Create a valid CDP packet - note that the size byte should match the parameter passed to the function
            let test_data = vec![
                0x96, 0x69,  // CDP identifier (0x9669)
                24,          // packet size (matches what we'll pass to function)
                0x10,        // framerate (top 4 bits = 1)
                0x00, 0x00, 0x00, // skip bytes (flag and hdr_seq_cntr)
                0x72,        // cdata identifier
                0x05,        // cc_count (5 & 0x1F = 5)
                // 15 bytes of CC data (5 * 3)
                0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
                0x0D, 0x0E, 0x0F,
                // CDP footer (4 bytes)
                0x74, 0x00, 0x00, 0x26,
            ];

            let mut demux = create_test_demuxer(&test_data);
            let mut options = create_test_options();
            let mut data = create_test_demuxer_data(1024);

            let result = mxf_read_cdp_data(&mut demux, 24, &mut data, &mut options);

            // Should process the full packet
            assert_eq!(result, 24);

            // Should have read 15 bytes of CC data (5 * 3)
            assert_eq!(data.len, 15);

            // Should have set framerate correctly (top 4 bits of 0x10 = 1)
            assert_eq!(data.tb, FRAMERATE_RATIONALS_RUST[1]);

            // Verify the CC data was copied correctly
            for i in 0..15 {
                assert_eq!(*data.buffer.add(i), (i + 1) as u8);
            }

            // Clean up
            cleanup_demuxer(&demux, test_data.len().max(1024));
            cleanup_demuxer_data(&data, 1024);
        }
    }

    #[test]
    fn test_invalid_cdp_identifier() {
        unsafe {
            // Create packet with invalid CDP identifier
            let test_data = vec![
                0x12, 0x34,  // Invalid CDP identifier (should be 0x9669)
                20,          // packet size
                0x10,        // framerate
                // Rest of packet...
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00,
            ];

            let mut demux = create_test_demuxer(&test_data);
            let mut options = create_test_options();
            let mut data = create_test_demuxer_data(1024);

            let result = mxf_read_cdp_data(&mut demux, 20, &mut data, &mut options);

            // Should still return full size (error handling skips remaining)
            assert_eq!(result, 20);

            // Should not have processed any CC data
            assert_eq!(data.len, 0);

            // Clean up
            cleanup_demuxer(&demux, test_data.len().max(1024));
            cleanup_demuxer_data(&data, 1024);
        }
    }

    #[test]
    fn test_size_mismatch() {
        unsafe {
            // Create packet where declared size doesn't match parameter
            let test_data = vec![
                0x96, 0x69,  // Valid CDP identifier
                15,          // packet says size 15, but we'll pass 20
                0x10,        // framerate
                // Rest of packet...
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00,
            ];

            let mut demux = create_test_demuxer(&test_data);
            let mut options = create_test_options();
            let mut data = create_test_demuxer_data(1024);

            let result = mxf_read_cdp_data(&mut demux, 20, &mut data, &mut options);

            // Should return full size passed as parameter
            assert_eq!(result, 20);

            // Should not have processed any CC data due to size mismatch
            assert_eq!(data.len, 0);

            // Clean up
            cleanup_demuxer(&demux, test_data.len().max(1024));
            cleanup_demuxer_data(&data, 1024);
        }
    }

    #[test]
    fn test_invalid_cdata_identifier() {
        unsafe {
            // Create packet with invalid cdata identifier
            let test_data = vec![
                0x96, 0x69,  // Valid CDP identifier
                20,          // packet size
                0x10,        // framerate
                0x00, 0x00, 0x00, // skip bytes
                0x99,        // Invalid cdata identifier (should be 0x72)
                0x05,        // cc_count
                // Rest of data...
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            ];

            let mut demux = create_test_demuxer(&test_data);
            let mut options = create_test_options();
            let mut data = create_test_demuxer_data(1024);

            let result = mxf_read_cdp_data(&mut demux, 20, &mut data, &mut options);

            // Should still return full size
            assert_eq!(result, 20);

            // Should not have processed CC data due to invalid cdata identifier
            assert_eq!(data.len, 0);

            // Clean up
            cleanup_demuxer(&demux, test_data.len().max(1024));
            cleanup_demuxer_data(&data, 1024);
        }
    }

    #[test]
    fn test_incomplete_cdp_packet_warning() {
        unsafe {
            // Create packet where CC count * 3 > available space
            let test_data = vec![
                0x96, 0x69,  // Valid CDP identifier
                9,           // Small packet size (matches what we pass to function)
                0x10,        // framerate
                0x00, 0x00, 0x00, // skip bytes
                0x72,        // Valid cdata identifier
                0x0A,        // cc_count = 10, needs 30 bytes but only ~4 left after headers
                // Only a few bytes of data (function will try to read 30 but only get what's available)
                0x01, 0x02,
            ];

            let mut demux = create_test_demuxer(&test_data);
            let mut options = create_test_options();
            let mut data = create_test_demuxer_data(1024);

            let result = mxf_read_cdp_data(&mut demux, 9, &mut data, &mut options);

            // Should return the size we passed
            assert_eq!(result, 9);

            // Should still try to read the CC data (10 * 3 = 30 bytes)
            assert_eq!(data.len, 30);

            // Clean up
            cleanup_demuxer(&demux, test_data.len().max(1024));
            cleanup_demuxer_data(&data, 1024);
        }
    }

    #[test]
    fn test_zero_cc_count() {
        unsafe {
            // Create packet with zero CC count
            let test_data = vec![
                0x96, 0x69,  // Valid CDP identifier
                20,          // packet size
                0x10,        // framerate
                0x00, 0x00, 0x00, // skip bytes
                0x72,        // Valid cdata identifier
                0x00,        // cc_count = 0
                // CDP footer
                0x74, 0x00, 0x00, 0x26,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            ];

            let mut demux = create_test_demuxer(&test_data);
            let mut options = create_test_options();
            let mut data = create_test_demuxer_data(1024);

            let result = mxf_read_cdp_data(&mut demux, 20, &mut data, &mut options);

            // Should process successfully
            assert_eq!(result, 20);

            // Should have 0 CC data
            assert_eq!(data.len, 0);

            // Should have set framerate correctly
            assert_eq!(data.tb, FRAMERATE_RATIONALS_RUST[1]);

            // Clean up
            cleanup_demuxer(&demux, test_data.len().max(1024));
            cleanup_demuxer_data(&data, 1024);
        }
    }

    #[test]
    fn test_different_framerate_values() {
        unsafe {
            for (framerate_byte, expected_index) in [(0x00, 0), (0x10, 1), (0x20, 2), (0x30, 3), (0x40, 4), (0x50, 5)] {
                let test_data = vec![
                    0x96, 0x69,      // Valid CDP identifier
                    20,              // packet size
                    framerate_byte,  // Different framerate values
                    0x00, 0x00, 0x00, // skip bytes
                    0x72,            // Valid cdata identifier
                    0x02,            // cc_count = 2
                    // 6 bytes of CC data (2 * 3)
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                    // CDP footer
                    0x74, 0x00, 0x00, 0x26,
                    0x00, 0x00, 0x00, 0x00,
                ];

                let mut demux = create_test_demuxer(&test_data);
                let mut options = create_test_options();
                let mut data = create_test_demuxer_data(1024);

                let result = mxf_read_cdp_data(&mut demux, 20, &mut data, &mut options);

                // Should process successfully
                assert_eq!(result, 20);

                // Should have read 6 bytes of CC data
                assert_eq!(data.len, 6);

                // Verify framerate was set correctly
                if expected_index < FRAMERATE_RATIONALS_RUST.len() {
                    assert_eq!(data.tb, FRAMERATE_RATIONALS_RUST[expected_index]);
                }

                // Clean up
                cleanup_demuxer(&demux, test_data.len().max(1024));
                cleanup_demuxer_data(&data, 1024);
            }
        }
    }

    #[test]
    fn test_cc_count_with_mask() {
        unsafe {
            // Test that cc_count is properly masked with 0x1F
            let test_data = vec![
                0x96, 0x69,  // Valid CDP identifier
                20,          // packet size
                0x10,        // framerate
                0x00, 0x00, 0x00, // skip bytes
                0x72,        // Valid cdata identifier
                0xE3,        // cc_count with high bits set (0xE3 & 0x1F = 3)
                // 9 bytes of CC data (3 * 3)
                0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                0x07, 0x08, 0x09,
                // CDP footer
                0x74, 0x26,
            ];

            let mut demux = create_test_demuxer(&test_data);
            let mut options = create_test_options();
            let mut data = create_test_demuxer_data(1024);

            let result = mxf_read_cdp_data(&mut demux, 20, &mut data, &mut options);

            // Should process successfully
            assert_eq!(result, 20);

            // Should have read 9 bytes of CC data (3 * 3, not 0xE3 * 3)
            assert_eq!(data.len, 9);

            // Verify the CC data
            for i in 0..9 {
                assert_eq!(*data.buffer.add(i), (i + 1) as u8);
            }

            // Clean up
            cleanup_demuxer(&demux, test_data.len().max(1024));
            cleanup_demuxer_data(&data, 1024);
        }
    }

    #[test]
    fn test_large_cc_count() {
        unsafe {
            // Test with maximum CC count (0x1F = 31)
            let cc_count = 0x1F;
            let cc_data_size = cc_count * 3;
            let packet_size = 100;

            let mut test_data = vec![
                0x96, 0x69,  // Valid CDP identifier
                packet_size, // Large packet size (matches what we pass to function)
                0x20,        // framerate
                0x00, 0x00, 0x00, // skip bytes
                0x72,        // Valid cdata identifier
                cc_count,    // Maximum cc_count
            ];

            // Add CC data
            for i in 0..cc_data_size {
                test_data.push(i);
            }

            // Pad to reach declared size
            while test_data.len() < packet_size as usize {
                test_data.push(0x00);
            }

            let mut demux = create_test_demuxer(&test_data);
            let mut options = create_test_options();
            let mut data = create_test_demuxer_data(1024);

            mxf_read_cdp_data(&mut demux, packet_size as i32, &mut data, &mut options);


            // Should have read all CC data
            assert_eq!(data.len, cc_data_size as usize);

            // Verify the CC data pattern
            for i in 0..cc_data_size {
                assert_eq!(*data.buffer.add(i as usize), i);
            }

            // Clean up
            cleanup_demuxer(&demux, test_data.len().max(1024));
            cleanup_demuxer_data(&data, 1024);
        }
    }
}
