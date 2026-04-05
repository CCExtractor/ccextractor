//! FFmpeg-based MP4 demuxer for CCExtractor
//!
//! This module provides MP4 demuxing using FFmpeg's libavformat as an
//! alternative to the GPAC-based implementation. It is enabled when
//! building with the `enable_mp4_ffmpeg` feature flag.

use crate::bindings::{
    cc_subtitle, ccx_mp4_flush_tx3g, ccx_mp4_process_avc_sample, ccx_mp4_process_cc_packet,
    ccx_mp4_process_hevc_sample, lib_ccx_ctx, mprint,
};
use rsmpeg::avcodec::AVPacket;
use rsmpeg::avformat::AVFormatContextInput;
use rsmpeg::ffi::{
    AVMEDIA_TYPE_DATA, AVMEDIA_TYPE_SUBTITLE, AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_DVD_SUBTITLE,
    AV_CODEC_ID_EIA_608, AV_CODEC_ID_H264, AV_CODEC_ID_HEVC, AV_CODEC_ID_MOV_TEXT,
};
use std::ffi::CString;

#[derive(Debug, PartialEq, Clone)]
pub enum Mp4TrackType {
    AvcH264,
    HevcH265,
    Tx3g,
    Cea608,
    Cea708,
    VobSub,
    Unknown,
}

#[derive(Debug, Clone)]
pub struct Mp4Track {
    pub stream_index: usize,
    pub track_type: Mp4TrackType,
    pub time_base_den: u32,
    pub nal_unit_size: u8,
}

pub fn avc_nal_unit_size(extradata: &[u8]) -> u8 {
    if extradata.len() < 5 || extradata[0] != 1 {
        return 4;
    }
    (extradata[4] & 0x03) + 1
}

pub fn hevc_nal_unit_size(extradata: &[u8]) -> u8 {
    if extradata.len() < 22 {
        return 4;
    }
    (extradata[21] & 0x03) + 1
}

pub fn classify_stream(codec_type: i32, codec_id: u32, codec_tag: u32) -> Mp4TrackType {
    if codec_type == AVMEDIA_TYPE_VIDEO {
        if codec_id == AV_CODEC_ID_H264 {
            return Mp4TrackType::AvcH264;
        } else if codec_id == AV_CODEC_ID_HEVC {
            return Mp4TrackType::HevcH265;
        }
    } else if codec_type == AVMEDIA_TYPE_SUBTITLE {
        if codec_id == AV_CODEC_ID_MOV_TEXT {
            return Mp4TrackType::Tx3g;
        } else if codec_id == AV_CODEC_ID_EIA_608 {
            return Mp4TrackType::Cea608;
        } else if codec_id == AV_CODEC_ID_DVD_SUBTITLE {
            return Mp4TrackType::VobSub;
        }
    } else if codec_type == AVMEDIA_TYPE_DATA {
        // c608/c708 tracks show up as data streams with a codec tag.
        // FFmpeg stores codec_tag in little-endian (native) byte order on x86.
        let c608 = u32::from_le_bytes(*b"c608");
        let c708 = u32::from_le_bytes(*b"c708");
        if codec_tag == c608 {
            return Mp4TrackType::Cea608;
        } else if codec_tag == c708 {
            return Mp4TrackType::Cea708;
        }
    }
    Mp4TrackType::Unknown
}

/// Open an MP4 file, enumerate its tracks, and return both tracks and the
/// open format context - avoiding the need to open the file twice.
pub fn open_and_enumerate(path: &str) -> Result<(Vec<Mp4Track>, AVFormatContextInput), String> {
    let path_cstr = CString::new(path).map_err(|e| e.to_string())?;

    let fmt_ctx = AVFormatContextInput::open(&path_cstr, None, &mut None)
        .map_err(|e| format!("Failed to open '{}': {}", path, e))?;

    let mut tracks = Vec::new();

    for (i, stream) in fmt_ctx.streams().iter().enumerate() {
        let codecpar = stream.codecpar();
        let codec_type = codecpar.codec_type;
        let codec_id = codecpar.codec_id;
        let codec_tag = codecpar.codec_tag;
        let time_base_den = stream.time_base.den as u32;

        let extradata = if codecpar.extradata.is_null() || codecpar.extradata_size <= 0 {
            &[][..]
        } else {
            unsafe {
                std::slice::from_raw_parts(codecpar.extradata, codecpar.extradata_size as usize)
            }
        };

        let track_type = classify_stream(codec_type, codec_id, codec_tag);

        let nal_unit_size = match track_type {
            Mp4TrackType::AvcH264 => avc_nal_unit_size(extradata),
            Mp4TrackType::HevcH265 => hevc_nal_unit_size(extradata),
            _ => 0,
        };

        tracks.push(Mp4Track {
            stream_index: i,
            track_type,
            time_base_den,
            nal_unit_size,
        });
    }

    Ok((tracks, fmt_ctx))
}

fn packet_timestamps(pkt: &AVPacket) -> (u64, u32) {
    let dts = if pkt.dts != i64::MIN {
        pkt.dts as u64
    } else if pkt.pts != i64::MIN {
        pkt.pts as u64
    } else {
        0
    };
    let cts_offset = if pkt.pts != i64::MIN && pkt.pts > pkt.dts {
        (pkt.pts - pkt.dts).min(u32::MAX as i64) as u32
    } else {
        0
    };
    (dts, cts_offset)
}

fn packet_dts(pkt: &AVPacket) -> u64 {
    if pkt.dts != i64::MIN {
        pkt.dts as u64
    } else if pkt.pts != i64::MIN {
        pkt.pts.max(0) as u64
    } else {
        0
    }
}

fn process_avc_packet(
    ctx: *mut lib_ccx_ctx,
    track: &Mp4Track,
    pkt: &AVPacket,
    sub: *mut cc_subtitle,
) -> i32 {
    let (dts, cts_offset) = packet_timestamps(pkt);
    let data = unsafe { std::slice::from_raw_parts(pkt.data, pkt.size as usize) };
    unsafe {
        ccx_mp4_process_avc_sample(
            ctx,
            track.time_base_den,
            track.nal_unit_size,
            data.as_ptr(),
            data.len() as u32,
            dts,
            cts_offset,
            sub,
        )
    }
}

fn process_hevc_packet(
    ctx: *mut lib_ccx_ctx,
    track: &Mp4Track,
    pkt: &AVPacket,
    sub: *mut cc_subtitle,
) -> i32 {
    let (dts, cts_offset) = packet_timestamps(pkt);
    let data = unsafe { std::slice::from_raw_parts(pkt.data, pkt.size as usize) };
    unsafe {
        ccx_mp4_process_hevc_sample(
            ctx,
            track.time_base_den,
            track.nal_unit_size,
            data.as_ptr(),
            data.len() as u32,
            dts,
            cts_offset,
            sub,
        )
    }
}

/// Main entry point: process all tracks in an MP4 file in a single demux pass.
///
/// # Safety
/// ctx, path, and sub must be valid pointers from C
pub unsafe fn processmp4_rust(ctx: *mut lib_ccx_ctx, path: &str, sub: *mut cc_subtitle) -> i32 {
    let (tracks, mut fmt_ctx) = match open_and_enumerate(path) {
        Ok(t) => t,
        Err(e) => {
            let msg =
                std::ffi::CString::new(format!("ccx_mp4_rust: failed to open '{}': {}\n", path, e))
                    .unwrap_or_default();
            mprint(msg.as_ptr());
            return -1;
        }
    };

    let avc_count = tracks
        .iter()
        .filter(|t| t.track_type == Mp4TrackType::AvcH264)
        .count();
    let hevc_count = tracks
        .iter()
        .filter(|t| t.track_type == Mp4TrackType::HevcH265)
        .count();
    let cc_count = tracks
        .iter()
        .filter(|t| {
            matches!(
                t.track_type,
                Mp4TrackType::Cea608 | Mp4TrackType::Cea708 | Mp4TrackType::Tx3g
            )
        })
        .count();

    {
        let msg = std::ffi::CString::new(format!(
            "MP4 Rust demuxer: {} tracks ({} avc, {} hevc, {} cc)\n",
            tracks.len(),
            avc_count,
            hevc_count,
            cc_count
        ))
        .unwrap_or_default();
        mprint(msg.as_ptr());
    }

    let mut mp4_ret = 0i32;

    while let Ok(Some(pkt)) = fmt_ctx.read_packet() {
        let stream_idx = pkt.stream_index as usize;

        let track = match tracks.iter().find(|t| t.stream_index == stream_idx) {
            Some(t) => t,
            None => continue,
        };

        let status = match track.track_type {
            Mp4TrackType::AvcH264 => process_avc_packet(ctx, track, &pkt, sub),
            Mp4TrackType::HevcH265 => process_hevc_packet(ctx, track, &pkt, sub),
            Mp4TrackType::Cea608 => unsafe {
                let data = std::slice::from_raw_parts(pkt.data, pkt.size as usize);
                ccx_mp4_process_cc_packet(
                    ctx,
                    0, // CCX_MP4_TRACK_C608
                    track.time_base_den,
                    data.as_ptr(),
                    data.len() as u32,
                    packet_dts(&pkt),
                    sub,
                )
            },
            Mp4TrackType::Cea708 => unsafe {
                let data = std::slice::from_raw_parts(pkt.data, pkt.size as usize);
                ccx_mp4_process_cc_packet(
                    ctx,
                    1, // CCX_MP4_TRACK_C708
                    track.time_base_den,
                    data.as_ptr(),
                    data.len() as u32,
                    packet_dts(&pkt),
                    sub,
                )
            },
            Mp4TrackType::Tx3g => unsafe {
                let data = std::slice::from_raw_parts(pkt.data, pkt.size as usize);
                ccx_mp4_process_cc_packet(
                    ctx,
                    2, // CCX_MP4_TRACK_TX3G
                    track.time_base_den,
                    data.as_ptr(),
                    data.len() as u32,
                    packet_dts(&pkt),
                    sub,
                )
            },
            Mp4TrackType::VobSub => {
                let msg = std::ffi::CString::new(
                    "MP4: VobSub track found but not yet supported in FFmpeg demuxer\n",
                )
                .unwrap();
                unsafe { mprint(msg.as_ptr()) };
                0
            }
            Mp4TrackType::Unknown => 0,
        };

        if status < 0 {
            mp4_ret = status;
            break;
        } else if status > 0 {
            mp4_ret = status;
        }
    }

    // Flush any pending tx3g subtitle
    let has_tx3g = tracks.iter().any(|t| t.track_type == Mp4TrackType::Tx3g);
    if has_tx3g {
        unsafe { ccx_mp4_flush_tx3g(ctx, sub) };
    }

    mp4_ret
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_avc_nal_unit_size_valid() {
        let extradata = [0x01, 0x64, 0x00, 0x1F, 0xFF];
        assert_eq!(avc_nal_unit_size(&extradata), 4);
    }

    #[test]
    fn test_avc_nal_unit_size_invalid() {
        assert_eq!(avc_nal_unit_size(&[]), 4);
        assert_eq!(avc_nal_unit_size(&[0x00]), 4);
    }

    #[test]
    fn test_classify_stream_avc() {
        let t = classify_stream(AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_H264, 0);
        assert_eq!(t, Mp4TrackType::AvcH264);
    }

    #[test]
    fn test_classify_stream_hevc() {
        let t = classify_stream(AVMEDIA_TYPE_VIDEO, AV_CODEC_ID_HEVC, 0);
        assert_eq!(t, Mp4TrackType::HevcH265);
    }

    #[test]
    fn test_classify_stream_tx3g() {
        let t = classify_stream(AVMEDIA_TYPE_SUBTITLE, AV_CODEC_ID_MOV_TEXT, 0);
        assert_eq!(t, Mp4TrackType::Tx3g);
    }

    #[test]
    fn test_classify_stream_c608() {
        let tag = u32::from_le_bytes(*b"c608");
        let t = classify_stream(AVMEDIA_TYPE_DATA, 0, tag);
        assert_eq!(t, Mp4TrackType::Cea608);
    }

    #[test]
    fn test_classify_stream_c708() {
        let tag = u32::from_le_bytes(*b"c708");
        let t = classify_stream(AVMEDIA_TYPE_DATA, 0, tag);
        assert_eq!(t, Mp4TrackType::Cea708);
    }

    #[test]
    fn test_open_invalid_file() {
        let result = open_and_enumerate("nonexistent_file.mp4");
        assert!(result.is_err());
    }
}
