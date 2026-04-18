//! FFmpeg-based MP4 demuxer for CCExtractor
//!
//! Uses rsmpeg (Rust FFmpeg bindings) to open MP4 files and extract subtitle
//! tracks, delegating actual caption processing to existing C functions
//! through the mp4_rust_bridge.
//!
//! # Supported track types
//! - H.264 video (SEI-embedded CEA-608/708 in NAL units)
//! - HEVC video (SEI-embedded CEA-608/708 in NAL units)
//! - `c608` subtitle (CEA-608, either cdat/cdt2-wrapped or bare cc_data triplets)
//! - `c708` subtitle (CEA-708 via ccdp)
//! - `tx3g` / `mov_text` timed-text subtitles
//!
//! # Known limitations
//! - **dvdsub / bitmap subtitles in MP4** are not supported. Samples such as
//!   `1f3e951d516b.mp4` contain `subp` tracks with DVD-style bitmap subtitles,
//!   which neither the GPAC backend nor this FFmpeg backend currently decodes.
//!   Extracting these requires rendering bitmaps and running OCR, which is out
//!   of scope for the MP4 demuxer itself; track it separately if needed.

#[cfg(feature = "enable_mp4_ffmpeg")]
use rsmpeg::avformat::AVFormatContextInput;
#[cfg(feature = "enable_mp4_ffmpeg")]
use rsmpeg::ffi;

use std::ffi::CStr;
use std::os::raw::{c_char, c_int, c_uint};

use crate::bindings::{cc_subtitle, encoder_ctx, lib_cc_decode, lib_ccx_ctx};

// C bridge functions from mp4_rust_bridge.c
extern "C" {
    fn ccx_mp4_process_nal_sample(
        ctx: *mut lib_ccx_ctx,
        timescale: c_uint,
        nal_unit_size: u8,
        is_hevc: c_int,
        data: *mut u8,
        data_length: c_uint,
        dts: i64,
        cts_offset: c_int,
        sub: *mut cc_subtitle,
    ) -> c_int;

    fn ccx_mp4_process_cc_packet(
        ctx: *mut lib_ccx_ctx,
        sub_type_c608: c_int,
        data: *mut u8,
        data_length: c_uint,
        dts: i64,
        cts_offset: c_int,
        timescale: c_uint,
        sub: *mut cc_subtitle,
    ) -> c_int;

    fn ccx_mp4_process_tx3g_packet(
        ctx: *mut lib_ccx_ctx,
        data: *mut u8,
        data_length: c_uint,
        dts: i64,
        cts_offset: c_int,
        timescale: c_uint,
        sub: *mut cc_subtitle,
    ) -> c_int;

    fn ccx_mp4_flush_tx3g(ctx: *mut lib_ccx_ctx, sub: *mut cc_subtitle);

    fn ccx_mp4_report_progress(ctx: *mut lib_ccx_ctx, cur: c_uint, total: c_uint);

    fn update_decoder_list(ctx: *mut lib_ccx_ctx) -> *mut lib_cc_decode;
    fn update_encoder_list(ctx: *mut lib_ccx_ctx) -> *mut encoder_ctx;

    fn mprint(fmt: *const c_char, ...);

    fn encode_sub(enc_ctx: *mut encoder_ctx, sub: *mut cc_subtitle) -> c_int;
}

/// Track types we can extract captions from
#[derive(Debug, Clone, Copy, PartialEq)]
enum TrackType {
    AvcH264,
    HevcH265,
    Cea608,
    Cea708,
    Tx3g,
}

/// Information about a track we want to process
#[derive(Debug)]
struct TrackInfo {
    stream_index: usize,
    track_type: TrackType,
    timescale: u32,
}

/// FourCC constants for codec identification
const FOURCC_C608: u32 = fourcc(b"c608");
const FOURCC_C708: u32 = fourcc(b"c708");
const FOURCC_TX3G: u32 = fourcc(b"tx3g");

const fn fourcc(s: &[u8; 4]) -> u32 {
    ((s[0] as u32) << 24) | ((s[1] as u32) << 16) | ((s[2] as u32) << 8) | (s[3] as u32)
}

/// AV_NOPTS_VALUE equivalent (INT64_MIN)
const AV_NOPTS_VALUE: i64 = i64::MIN;

/// Get the NAL unit size from an AVC/HEVC extradata.
#[cfg(feature = "enable_mp4_ffmpeg")]
fn get_nal_unit_size_from_extradata(extradata: &[u8], is_hevc: bool) -> u8 {
    if is_hevc {
        // HEVCDecoderConfigurationRecord: byte 21 has lengthSizeMinusOne in bits [0:1]
        if extradata.len() >= 23 {
            (extradata[21] & 0x03) + 1
        } else {
            4
        }
    } else {
        // AVCDecoderConfigurationRecord: byte 4 has lengthSizeMinusOne in bits [0:1]
        if extradata.len() >= 7 {
            (extradata[4] & 0x03) + 1
        } else {
            4
        }
    }
}

/// Main MP4 processing function using FFmpeg via rsmpeg
///
/// # Safety
/// ctx and sub must be valid pointers
#[cfg(feature = "enable_mp4_ffmpeg")]
pub unsafe fn processmp4_rust(ctx: *mut lib_ccx_ctx, path: &CStr, sub: *mut cc_subtitle) -> c_int {
    let path_str = path.to_bytes();
    let path_display = String::from_utf8_lossy(path_str);

    let open_msg = format!("Opening '{}' with FFmpeg: \0", path_display);
    mprint(open_msg.as_ptr() as *const c_char);

    // Open the file with FFmpeg
    let fmt_ctx = match AVFormatContextInput::open(path, None, &mut None) {
        Ok(ctx) => ctx,
        Err(e) => {
            let err_msg = format!("Failed to open input file with FFmpeg: {}\n\0", e);
            mprint(err_msg.as_ptr() as *const c_char);
            return -2;
        }
    };

    let ok_msg = b"ok\n\0";
    mprint(ok_msg.as_ptr() as *const c_char);

    // Set up encoder/decoder
    let dec_ctx = update_decoder_list(ctx);
    let enc_ctx = update_encoder_list(ctx);

    if !enc_ctx.is_null() && !dec_ctx.is_null() {
        (*enc_ctx).timing = (*dec_ctx).timing;
        crate::bindings::ccxr_dtvcc_set_encoder((*dec_ctx).dtvcc_rust, enc_ctx);
    }

    // Enumerate tracks
    let mut tracks: Vec<TrackInfo> = Vec::new();
    let nb_streams = (*fmt_ctx.as_ptr()).nb_streams as usize;

    for i in 0..nb_streams {
        let stream = *(*fmt_ctx.as_ptr()).streams.add(i);
        let codecpar = (*stream).codecpar;
        if codecpar.is_null() {
            continue;
        }

        let codec_type = (*codecpar).codec_type;
        let codec_tag = (*codecpar).codec_tag;
        let codec_id = (*codecpar).codec_id;
        let time_base = (*stream).time_base;
        let timescale = if time_base.den > 0 {
            time_base.den as u32
        } else {
            90000
        };

        let track_type = if codec_type == ffi::AVMEDIA_TYPE_VIDEO {
            if codec_id == ffi::AV_CODEC_ID_H264 {
                Some(TrackType::AvcH264)
            } else if codec_id == ffi::AV_CODEC_ID_HEVC {
                Some(TrackType::HevcH265)
            } else {
                None
            }
        } else if codec_type == ffi::AVMEDIA_TYPE_SUBTITLE {
            if codec_tag == FOURCC_C608 {
                Some(TrackType::Cea608)
            } else if codec_tag == FOURCC_C708 {
                Some(TrackType::Cea708)
            } else if codec_tag == FOURCC_TX3G {
                Some(TrackType::Tx3g)
            } else if codec_id == ffi::AV_CODEC_ID_MOV_TEXT {
                Some(TrackType::Tx3g)
            } else if codec_id == ffi::AV_CODEC_ID_EIA_608 {
                Some(TrackType::Cea608)
            } else {
                None
            }
        } else {
            None
        };

        if let Some(tt) = track_type {
            let type_name = match tt {
                TrackType::AvcH264 => "AVC/H.264",
                TrackType::HevcH265 => "HEVC/H.265",
                TrackType::Cea608 => "CEA-608",
                TrackType::Cea708 => "CEA-708",
                TrackType::Tx3g => "tx3g",
            };
            let msg = format!(
                "Track {}, type={} timescale={}\n\0",
                i, type_name, timescale
            );
            mprint(msg.as_ptr() as *const c_char);

            tracks.push(TrackInfo {
                stream_index: i,
                track_type: tt,
                timescale,
            });
        }
    }

    // Count track types
    let avc_count = tracks
        .iter()
        .filter(|t| t.track_type == TrackType::AvcH264)
        .count();
    let hevc_count = tracks
        .iter()
        .filter(|t| t.track_type == TrackType::HevcH265)
        .count();
    let cc_count = tracks
        .iter()
        .filter(|t| {
            matches!(
                t.track_type,
                TrackType::Cea608 | TrackType::Cea708 | TrackType::Tx3g
            )
        })
        .count();

    let summary = format!(
        "MP4 (FFmpeg): found {} tracks: {} avc, {} hevc, {} cc\n\0",
        tracks.len(),
        avc_count,
        hevc_count,
        cc_count
    );
    mprint(summary.as_ptr() as *const c_char);

    if tracks.is_empty() {
        let msg = b"No processable tracks found\n\0";
        mprint(msg.as_ptr() as *const c_char);
        return 0;
    }

    // Determine NAL unit sizes from extradata for video tracks
    let mut nal_sizes: Vec<(usize, u8)> = Vec::new();
    for track in &tracks {
        if track.track_type == TrackType::AvcH264 || track.track_type == TrackType::HevcH265 {
            let stream = *(*fmt_ctx.as_ptr()).streams.add(track.stream_index);
            let codecpar = (*stream).codecpar;
            let extradata = (*codecpar).extradata;
            let extradata_size = (*codecpar).extradata_size;
            let is_hevc = track.track_type == TrackType::HevcH265;

            let nal_unit_size = if !extradata.is_null() && extradata_size > 0 {
                let data = std::slice::from_raw_parts(extradata, extradata_size as usize);
                process_extradata_params(ctx, data, is_hevc, sub);
                get_nal_unit_size_from_extradata(data, is_hevc)
            } else {
                4
            };
            nal_sizes.push((track.stream_index, nal_unit_size));
        }
    }

    // Read packets and dispatch
    let mut mp4_ret: c_int = 0;
    let mut pkt: ffi::AVPacket = std::mem::zeroed();
    ffi::av_init_packet(&mut pkt);

    let mut packet_count: u32 = 0;
    let mut has_tx3g = false;

    loop {
        let ret = ffi::av_read_frame(fmt_ctx.as_ptr() as *mut _, &mut pkt);
        if ret < 0 {
            break;
        }

        let stream_idx = pkt.stream_index as usize;

        if let Some(track) = tracks.iter().find(|t| t.stream_index == stream_idx) {
            let dts = if pkt.dts != AV_NOPTS_VALUE {
                pkt.dts
            } else {
                pkt.pts
            };
            let pts = if pkt.pts != AV_NOPTS_VALUE {
                pkt.pts
            } else {
                dts
            };
            let cts_offset = (pts - dts) as c_int;
            let timescale = track.timescale;

            match track.track_type {
                TrackType::AvcH264 | TrackType::HevcH265 => {
                    let nal_unit_size = nal_sizes
                        .iter()
                        .find(|(idx, _)| *idx == stream_idx)
                        .map(|(_, s)| *s)
                        .unwrap_or(4);
                    let is_hevc = (track.track_type == TrackType::HevcH265) as c_int;

                    if pkt.size > 0 && !pkt.data.is_null() {
                        ccx_mp4_process_nal_sample(
                            ctx,
                            timescale,
                            nal_unit_size,
                            is_hevc,
                            pkt.data,
                            pkt.size as c_uint,
                            dts,
                            cts_offset,
                            sub,
                        );
                    }
                }
                TrackType::Cea608 => {
                    if pkt.size > 0 && !pkt.data.is_null() {
                        let r = ccx_mp4_process_cc_packet(
                            ctx,
                            1,
                            pkt.data,
                            pkt.size as c_uint,
                            dts,
                            cts_offset,
                            timescale,
                            sub,
                        );
                        if r == 0 {
                            mp4_ret = 1;
                        }
                    }
                }
                TrackType::Cea708 => {
                    if pkt.size > 0 && !pkt.data.is_null() {
                        let r = ccx_mp4_process_cc_packet(
                            ctx,
                            0,
                            pkt.data,
                            pkt.size as c_uint,
                            dts,
                            cts_offset,
                            timescale,
                            sub,
                        );
                        if r == 0 {
                            mp4_ret = 1;
                        }
                    }
                }
                TrackType::Tx3g => {
                    if pkt.size > 0 && !pkt.data.is_null() {
                        if has_tx3g {
                            ccx_mp4_flush_tx3g(ctx, sub);
                        }
                        let r = ccx_mp4_process_tx3g_packet(
                            ctx,
                            pkt.data,
                            pkt.size as c_uint,
                            dts,
                            cts_offset,
                            timescale,
                            sub,
                        );
                        if r == 0 {
                            has_tx3g = true;
                            mp4_ret = 1;
                        }
                    }
                }
            }
        }

        ffi::av_packet_unref(&mut pkt);

        packet_count += 1;
        if packet_count % 100 == 0 {
            ccx_mp4_report_progress(ctx, packet_count, packet_count + 100);
        }
    }

    // Flush last tx3g subtitle
    if has_tx3g {
        ccx_mp4_flush_tx3g(ctx, sub);
    }

    // End-of-stream: encode any caption that finished on the last processed
    // sample but hasn't been flushed by slice_header yet. GPAC's mp4.c does
    // the equivalent via encode_sub after its per-track loop returns.
    // Intentionally NOT calling process_hdcc here: the last IDR already
    // flushed has_ccdata_buffered through slice_header, and running
    // process_hdcc again would re-process any cc_data that arrived after
    // that IDR — which can include half-typed trailing characters the
    // source encoder never intended to display.
    let enc_ctx = update_encoder_list(ctx);
    if (*sub).got_output != 0 {
        encode_sub(enc_ctx, sub);
        (*sub).got_output = 0;
    }

    ccx_mp4_report_progress(ctx, 100, 100);

    let close_msg = b"\nClosing media: ok\n\0";
    mprint(close_msg.as_ptr() as *const c_char);

    if avc_count > 0 {
        let msg = format!("Found {} AVC track(s). \0", avc_count);
        mprint(msg.as_ptr() as *const c_char);
    } else {
        let msg = b"Found no AVC track(s). \0";
        mprint(msg.as_ptr() as *const c_char);
    }
    if hevc_count > 0 {
        let msg = format!("Found {} HEVC track(s). \0", hevc_count);
        mprint(msg.as_ptr() as *const c_char);
    }
    if cc_count > 0 {
        let msg = format!("Found {} CC track(s).\n\0", cc_count);
        mprint(msg.as_ptr() as *const c_char);
    } else {
        let msg = b"Found no dedicated CC track(s).\n\0";
        mprint(msg.as_ptr() as *const c_char);
    }

    (*ctx).freport.mp4_cc_track_cnt = cc_count as u32;

    mp4_ret
}

/// Process SPS/PPS/VPS NAL units from extradata
#[cfg(feature = "enable_mp4_ffmpeg")]
unsafe fn process_extradata_params(
    ctx: *mut lib_ccx_ctx,
    extradata: &[u8],
    is_hevc: bool,
    sub: *mut cc_subtitle,
) {
    let dec_ctx = update_decoder_list(ctx);
    let enc_ctx = update_encoder_list(ctx);

    extern "C" {
        fn do_NAL(
            enc_ctx: *mut encoder_ctx,
            dec_ctx: *mut lib_cc_decode,
            nal_start: *mut u8,
            nal_length: i64,
            sub: *mut cc_subtitle,
        );
    }

    if is_hevc {
        if let Some(avc) = (*dec_ctx).avc_ctx.as_mut() {
            avc.is_hevc = 1;
        }
        // Parse HEVCDecoderConfigurationRecord
        if extradata.len() < 23 {
            return;
        }
        let num_arrays = extradata[22] as usize;
        let mut offset = 23;
        for _ in 0..num_arrays {
            if offset + 3 > extradata.len() {
                break;
            }
            let num_nalus =
                ((extradata[offset + 1] as usize) << 8) | (extradata[offset + 2] as usize);
            offset += 3;
            for _ in 0..num_nalus {
                if offset + 2 > extradata.len() {
                    break;
                }
                let nal_size =
                    ((extradata[offset] as usize) << 8) | (extradata[offset + 1] as usize);
                offset += 2;
                if offset + nal_size > extradata.len() {
                    break;
                }
                let mut nal_data = extradata[offset..offset + nal_size].to_vec();
                do_NAL(
                    enc_ctx,
                    dec_ctx,
                    nal_data.as_mut_ptr(),
                    nal_size as i64,
                    sub,
                );
                offset += nal_size;
            }
        }
    } else {
        // Parse AVCDecoderConfigurationRecord
        if extradata.len() < 7 {
            return;
        }
        let num_sps = (extradata[5] & 0x1F) as usize;
        let mut offset = 6;
        for _ in 0..num_sps {
            if offset + 2 > extradata.len() {
                break;
            }
            let sps_size = ((extradata[offset] as usize) << 8) | (extradata[offset + 1] as usize);
            offset += 2;
            if offset + sps_size > extradata.len() {
                break;
            }
            let mut sps_data = extradata[offset..offset + sps_size].to_vec();
            do_NAL(
                enc_ctx,
                dec_ctx,
                sps_data.as_mut_ptr(),
                sps_size as i64,
                sub,
            );
            offset += sps_size;
        }
        // PPS
        if offset >= extradata.len() {
            return;
        }
        let num_pps = extradata[offset] as usize;
        offset += 1;
        for _ in 0..num_pps {
            if offset + 2 > extradata.len() {
                break;
            }
            let pps_size = ((extradata[offset] as usize) << 8) | (extradata[offset + 1] as usize);
            offset += 2;
            if offset + pps_size > extradata.len() {
                break;
            }
            let mut pps_data = extradata[offset..offset + pps_size].to_vec();
            do_NAL(
                enc_ctx,
                dec_ctx,
                pps_data.as_mut_ptr(),
                pps_size as i64,
                sub,
            );
            offset += pps_size;
        }
    }
}

/// Dump chapters from MP4 file using FFmpeg
///
/// # Safety
/// ctx must be a valid pointer, path must be a valid C string
#[cfg(feature = "enable_mp4_ffmpeg")]
pub unsafe fn dumpchapters_rust(_ctx: *mut lib_ccx_ctx, path: &CStr) -> c_int {
    let path_str = path.to_bytes();
    let path_display = String::from_utf8_lossy(path_str);

    let open_msg = format!("Opening '{}': \0", path_display);
    mprint(open_msg.as_ptr() as *const c_char);

    let fmt_ctx = match AVFormatContextInput::open(path, None, &mut None) {
        Ok(ctx) => ctx,
        Err(_) => {
            let msg = b"failed to open\n\0";
            mprint(msg.as_ptr() as *const c_char);
            return 5;
        }
    };

    let ok_msg = b"ok\n\0";
    mprint(ok_msg.as_ptr() as *const c_char);

    let nb_chapters = (*fmt_ctx.as_ptr()).nb_chapters as usize;
    if nb_chapters == 0 {
        let msg = b"No chapters information found!\n\0";
        mprint(msg.as_ptr() as *const c_char);
        return 0;
    }

    let out_name = format!("{}.txt", path_display);
    let mut file = match std::fs::File::create(&out_name) {
        Ok(f) => f,
        Err(_) => return 5,
    };

    let writing_msg = format!("Writing chapters into {}\n\0", out_name);
    mprint(writing_msg.as_ptr() as *const c_char);

    use std::io::Write;
    for i in 0..nb_chapters {
        let chapter = *(*fmt_ctx.as_ptr()).chapters.add(i);
        let start = (*chapter).start;
        let time_base = (*chapter).time_base;

        let start_ms = (start as f64 * time_base.num as f64 / time_base.den as f64 * 1000.0) as u64;
        let h = start_ms / 3600000;
        let m = (start_ms / 60000) % 60;
        let s = (start_ms / 1000) % 60;
        let ms = start_ms % 1000;

        let title = if !(*chapter).metadata.is_null() {
            let key = std::ffi::CString::new("title").unwrap();
            let entry = ffi::av_dict_get((*chapter).metadata, key.as_ptr(), std::ptr::null(), 0);
            if !entry.is_null() && !(*entry).value.is_null() {
                CStr::from_ptr((*entry).value)
                    .to_string_lossy()
                    .into_owned()
            } else {
                String::new()
            }
        } else {
            String::new()
        };

        if writeln!(
            file,
            "CHAPTER{:02}={:02}:{:02}:{:02}.{:03}",
            i + 1,
            h,
            m,
            s,
            ms
        )
        .is_err()
        {
            return 5;
        }
        if writeln!(file, "CHAPTER{:02}NAME={}", i + 1, title).is_err() {
            return 5;
        }
    }

    1
}
