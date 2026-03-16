//! Track listing functionality for --list-tracks option
//!
//! This module provides the ability to list all tracks in media files
//! without processing them for subtitle extraction.

use std::convert::TryFrom;
use std::fs::File;
use std::io::{BufReader, Read, Seek, SeekFrom};
use std::path::Path;
/// Represents a track found in a media file
#[derive(Debug)]
pub struct TrackInfo {
    pub track_number: u32,
    pub track_type: TrackType,
    pub codec: String,
    pub language: Option<String>,
}

/// Type of media track
#[derive(Debug, Clone)]
pub enum TrackType {
    Video,
    Audio,
    Subtitle,
    ClosedCaption,
    Other(String),
}

impl std::fmt::Display for TrackType {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            TrackType::Video => write!(f, "Video"),
            TrackType::Audio => write!(f, "Audio"),
            TrackType::Subtitle => write!(f, "Subtitle"),
            TrackType::ClosedCaption => write!(f, "Closed Caption"),
            TrackType::Other(s) => write!(f, "{}", s),
        }
    }
}

/// Detected file format
#[derive(Debug, PartialEq)]
pub enum FileFormat {
    Mkv,
    Mp4,
    TransportStream,
    Unknown,
}

/// Detect the format of a file based on magic bytes
pub fn detect_format(path: &Path) -> std::io::Result<FileFormat> {
    let file = File::open(path)?;
    let mut reader = BufReader::new(file);
    let mut header = [0u8; 12];

    let bytes_read = reader.read(&mut header)?;
    if bytes_read < 4 {
        return Ok(FileFormat::Unknown);
    }

    // Check for Matroska/WebM (EBML header)
    if header[0] == 0x1a && header[1] == 0x45 && header[2] == 0xdf && header[3] == 0xa3 {
        return Ok(FileFormat::Mkv);
    }

    // Check for MP4/MOV (ftyp box or other common boxes)
    if bytes_read >= 8 {
        let box_type = &header[4..8];
        if box_type == b"ftyp" || box_type == b"moov" || box_type == b"mdat" || box_type == b"free"
        {
            return Ok(FileFormat::Mp4);
        }
    }

    // Check for MPEG Transport Stream (sync byte 0x47 at regular intervals)
    if header[0] == 0x47 {
        // Check for sync bytes at 188-byte intervals (standard TS packet size)
        reader.seek(SeekFrom::Start(188))?;
        let mut sync_check = [0u8; 1];
        if reader.read(&mut sync_check)? == 1 && sync_check[0] == 0x47 {
            return Ok(FileFormat::TransportStream);
        }
        // Check for 192-byte packets (M2TS)
        reader.seek(SeekFrom::Start(192))?;
        if reader.read(&mut sync_check)? == 1 && sync_check[0] == 0x47 {
            return Ok(FileFormat::TransportStream);
        }
    }

    // M2TS files start with 4-byte timestamp before sync byte
    if bytes_read >= 5 && header[4] == 0x47 {
        reader.seek(SeekFrom::Start(192 + 4))?;
        let mut sync_check = [0u8; 1];
        if reader.read(&mut sync_check)? == 1 && sync_check[0] == 0x47 {
            return Ok(FileFormat::TransportStream);
        }
    }

    Ok(FileFormat::Unknown)
}

/// List tracks in a Matroska (MKV/WebM) file
pub fn list_mkv_tracks(path: &Path) -> std::io::Result<Vec<TrackInfo>> {
    let file = File::open(path)?;
    let file_size = file.metadata()?.len();
    let mut reader = BufReader::new(file);
    let mut tracks = Vec::new();

    // First, skip EBML header
    let ebml_id = read_element_id(&mut reader)?;
    if ebml_id != 0x1A45_DFA3 {
        return Err(std::io::Error::new(
            std::io::ErrorKind::InvalidData,
            "Not a valid EBML file",
        ));
    }
    let ebml_size = read_vint(&mut reader)?;
    reader.seek(SeekFrom::Current(ebml_size as i64))?;

    // Now look for Segment header
    let segment_id = read_element_id(&mut reader)?;
    if segment_id != 0x1853_8067 {
        return Err(std::io::Error::new(
            std::io::ErrorKind::InvalidData,
            "Segment not found",
        ));
    }

    // Read segment size - may be "unknown" (all 1s)
    let segment_size = read_vint(&mut reader)?;
    let segment_start = reader.stream_position()?;

    // If segment size is unknown (0xFFFFFFFFFFFFFF for 8-byte VINT), use file size
    let segment_end = if segment_size >= 0x00FF_FFFF_FFFF_FFFF {
        file_size
    } else {
        std::cmp::min(segment_start + segment_size, file_size)
    };

    // Limit how far we search to avoid processing huge files
    let max_search_bytes = 50 * 1024 * 1024; // 50MB max search
    let search_end = std::cmp::min(segment_end, segment_start + max_search_bytes);

    while reader.stream_position()? < search_end {
        let element_pos = reader.stream_position()?;

        let element_id = match read_element_id(&mut reader) {
            Ok(id) => id,
            Err(_) => break,
        };

        let element_size = match read_vint(&mut reader) {
            Ok(size) => size,
            Err(_) => break,
        };

        let element_start = reader.stream_position()?;

        // Tracks element ID: 0x1654AE6B
        if element_id == 0x1654_AE6B {
            tracks = parse_mkv_tracks(&mut reader, element_size)?;
            break;
        }

        // Skip known large elements we don't need
        // Cluster: 0x1F43B675, Cues: 0x1C53BB6B, Attachments: 0x1941A469
        if element_id == 0x1F43_B675 || element_id == 0x1C53_BB6B || element_id == 0x1941_A469 {
            // These are usually after tracks, stop searching
            break;
        }

        // Skip to next element, but check bounds
        let next_pos = element_start.saturating_add(element_size);
        if next_pos > search_end || next_pos <= element_pos {
            break;
        }
        reader.seek(SeekFrom::Start(next_pos))?;
    }

    Ok(tracks)
}

fn read_vint<R: Read>(reader: &mut R) -> std::io::Result<u64> {
    let mut first_byte = [0u8; 1];
    reader.read_exact(&mut first_byte)?;

    let leading_zeros = first_byte[0].leading_zeros();
    let length = (leading_zeros + 1) as usize;

    if length > 8 {
        return Err(std::io::Error::new(
            std::io::ErrorKind::InvalidData,
            "Invalid VINT length",
        ));
    }

    // Mask to extract data bits from first byte
    // For 1-byte: mask = 0x7F (127), for 2-byte: 0x3F, ..., for 8-byte: 0x00
    let mask = if length == 8 {
        0
    } else {
        (1u8 << (8 - length)) - 1
    };
    let mut value = (first_byte[0] & mask) as u64;

    for _ in 1..length {
        let mut byte = [0u8; 1];
        reader.read_exact(&mut byte)?;
        value = (value << 8) | byte[0] as u64;
    }

    Ok(value)
}

fn read_element_id<R: Read>(reader: &mut R) -> std::io::Result<u32> {
    let mut first_byte = [0u8; 1];
    reader.read_exact(&mut first_byte)?;

    let leading_zeros = first_byte[0].leading_zeros();
    let length = (leading_zeros + 1) as usize;

    let mut value = first_byte[0] as u32;

    for _ in 1..length {
        let mut byte = [0u8; 1];
        reader.read_exact(&mut byte)?;
        value = (value << 8) | byte[0] as u32;
    }

    Ok(value)
}

fn parse_mkv_tracks<R: Read + Seek>(reader: &mut R, size: u64) -> std::io::Result<Vec<TrackInfo>> {
    let mut tracks = Vec::new();
    let end_pos = reader.stream_position()? + size;

    while reader.stream_position()? < end_pos {
        let element_id = read_element_id(reader)?;
        let element_size = read_vint(reader)?;
        let element_start = reader.stream_position()?;

        // TrackEntry element ID: 0xAE
        if element_id == 0xAE {
            if let Ok(track) = parse_mkv_track_entry(reader, element_size) {
                tracks.push(track);
            }
        }

        reader.seek(SeekFrom::Start(element_start + element_size))?;
    }

    Ok(tracks)
}

fn parse_mkv_track_entry<R: Read + Seek>(reader: &mut R, size: u64) -> std::io::Result<TrackInfo> {
    let end_pos = reader.stream_position()? + size;

    let mut track_number = 0u32;
    let mut track_type = TrackType::Other("Unknown".to_string());
    let mut codec = String::new();
    let mut language = None;

    while reader.stream_position()? < end_pos {
        let element_id = read_element_id(reader)?;
        let element_size = read_vint(reader)?;
        let element_start = reader.stream_position()?;

        match element_id {
            0xD7 => {
                // TrackNumber
                track_number = read_uint(reader, element_size)? as u32;
            }
            0x83 => {
                // TrackType
                let type_val = read_uint(reader, element_size)?;
                track_type = match type_val {
                    1 => TrackType::Video,
                    2 => TrackType::Audio,
                    0x11 => TrackType::Subtitle,
                    _ => TrackType::Other(format!("Type {}", type_val)),
                };
            }
            0x86 => {
                // CodecID
                codec = read_string(reader, element_size)?;
            }
            0x0022_B59C => {
                // Language
                language = Some(read_string(reader, element_size)?);
            }
            _ => {}
        }

        reader.seek(SeekFrom::Start(element_start + element_size))?;
    }

    Ok(TrackInfo {
        track_number,
        track_type,
        codec,
        language,
    })
}

fn read_uint<R: Read>(reader: &mut R, size: u64) -> std::io::Result<u64> {
    let mut value = 0u64;
    for _ in 0..size {
        let mut byte = [0u8; 1];
        reader.read_exact(&mut byte)?;
        value = (value << 8) | u64::from(byte[0]);
    }
    Ok(value)
}

fn read_string<R: Read>(reader: &mut R, size: u64) -> std::io::Result<String> {
    // Try to convert. If it fails (too big), return an IO error instead of crashing.
    let buffer_size = usize::try_from(size)
        .map_err(|e| std::io::Error::new(std::io::ErrorKind::InvalidInput, e))?;
    let mut buffer = vec![0u8; buffer_size];
    reader.read_exact(&mut buffer)?;
    // Remove null terminators if present
    while buffer.last() == Some(&0) {
        buffer.pop();
    }
    Ok(String::from_utf8_lossy(&buffer).to_string())
}

/// List tracks in an MP4 file.
///
/// # Errors
/// Returns an error if the file cannot be opened or parsed.
pub fn list_mp4_tracks(path: &Path) -> std::io::Result<Vec<TrackInfo>> {
    let file = File::open(path)?;
    let mut reader = BufReader::new(file);
    let file_size = reader.seek(SeekFrom::End(0))?;
    reader.seek(SeekFrom::Start(0))?;

    let mut tracks = Vec::new();

    // Parse top-level boxes to find moov
    while reader.stream_position()? < file_size {
        let pos = reader.stream_position()?;

        let mut size_buf = [0u8; 4];
        if reader.read(&mut size_buf)? < 4 {
            break;
        }
        let size = u64::from(u32::from_be_bytes(size_buf));

        let mut type_buf = [0u8; 4];
        if reader.read(&mut type_buf)? < 4 {
            break;
        }

        let actual_size = if size == 1 {
            // 64-bit size
            let mut size64_buf = [0u8; 8];
            reader.read_exact(&mut size64_buf)?;
            u64::from_be_bytes(size64_buf)
        } else if size == 0 {
            // Box extends to end of file
            file_size - pos
        } else {
            size
        };

        if &type_buf == b"moov" {
            // Parse moov box for tracks
            let header_size = if size == 1 { 16 } else { 8 };
            tracks = parse_mp4_moov(&mut reader, actual_size - header_size)?;
            break;
        }

        // Skip to next box
        if actual_size > 0 {
            reader.seek(SeekFrom::Start(pos + actual_size))?;
        } else {
            break;
        }
    }

    Ok(tracks)
}

fn parse_mp4_moov<R: Read + Seek>(reader: &mut R, size: u64) -> std::io::Result<Vec<TrackInfo>> {
    let mut tracks = Vec::new();
    let end_pos = reader.stream_position()? + size;
    let mut track_num = 1u32;

    while reader.stream_position()? < end_pos {
        let pos = reader.stream_position()?;

        let mut size_buf = [0u8; 4];
        if reader.read(&mut size_buf)? < 4 {
            break;
        }
        let box_size = u64::from(u32::from_be_bytes(size_buf));

        let mut type_buf = [0u8; 4];
        if reader.read(&mut type_buf)? < 4 {
            break;
        }

        let actual_size = if box_size == 1 {
            let mut size64_buf = [0u8; 8];
            reader.read_exact(&mut size64_buf)?;
            u64::from_be_bytes(size64_buf)
        } else {
            box_size
        };

        if &type_buf == b"trak" {
            let header_size = if box_size == 1 { 16 } else { 8 };
            if let Ok(track) = parse_mp4_trak(reader, actual_size - header_size, track_num) {
                tracks.push(track);
            }
            track_num += 1;
        }

        if actual_size > 0 {
            reader.seek(SeekFrom::Start(pos + actual_size))?;
        } else {
            break;
        }
    }

    Ok(tracks)
}

fn parse_mp4_trak<R: Read + Seek>(
    reader: &mut R,
    size: u64,
    track_num: u32,
) -> std::io::Result<TrackInfo> {
    let end_pos = reader.stream_position()? + size;

    let mut track_type = TrackType::Other("Unknown".to_string());
    let mut codec = String::new();
    let mut language = None;

    while reader.stream_position()? < end_pos {
        let pos = reader.stream_position()?;

        let mut size_buf = [0u8; 4];
        if reader.read(&mut size_buf)? < 4 {
            break;
        }
        let box_size = u64::from(u32::from_be_bytes(size_buf));

        let mut type_buf = [0u8; 4];
        if reader.read(&mut type_buf)? < 4 {
            break;
        }

        let actual_size = if box_size == 1 {
            let mut size64_buf = [0u8; 8];
            reader.read_exact(&mut size64_buf)?;
            u64::from_be_bytes(size64_buf)
        } else {
            box_size
        };

        if &type_buf == b"mdia" {
            let header_size = if box_size == 1 { 16 } else { 8 };
            let (t, c, l) = parse_mp4_mdia(reader, actual_size - header_size)?;
            track_type = t;
            codec = c;
            language = l;
        }

        if actual_size > 0 {
            reader.seek(SeekFrom::Start(pos + actual_size))?;
        } else {
            break;
        }
    }

    Ok(TrackInfo {
        track_number: track_num,
        track_type,
        codec,
        language,
    })
}

fn parse_mp4_mdia<R: Read + Seek>(
    reader: &mut R,
    size: u64,
) -> std::io::Result<(TrackType, String, Option<String>)> {
    let end_pos = reader.stream_position()? + size;

    let mut track_type = TrackType::Other("Unknown".to_string());
    let mut codec = String::new();
    let mut language = None;

    while reader.stream_position()? < end_pos {
        let pos = reader.stream_position()?;

        let mut size_buf = [0u8; 4];
        if reader.read(&mut size_buf)? < 4 {
            break;
        }
        let box_size = u64::from(u32::from_be_bytes(size_buf));

        let mut type_buf = [0u8; 4];
        if reader.read(&mut type_buf)? < 4 {
            break;
        }

        let actual_size = if box_size == 1 {
            let mut size64_buf = [0u8; 8];
            reader.read_exact(&mut size64_buf)?;
            u64::from_be_bytes(size64_buf)
        } else {
            box_size
        };

        match &type_buf {
            b"hdlr" => {
                // Handler reference box - contains media type
                reader.seek(SeekFrom::Current(8))?; // Skip version/flags and pre_defined
                let mut handler_type = [0u8; 4];
                reader.read_exact(&mut handler_type)?;
                track_type = match &handler_type {
                    b"vide" => TrackType::Video,
                    b"soun" => TrackType::Audio,
                    b"subt" | b"sbtl" | b"text" => TrackType::Subtitle,
                    b"clcp" => TrackType::ClosedCaption,
                    _ => TrackType::Other(String::from_utf8_lossy(&handler_type).to_string()),
                };
            }
            b"mdhd" => {
                // Media header box - contains language
                let mut version = [0u8; 1];
                reader.read_exact(&mut version)?;
                reader.seek(SeekFrom::Current(3))?; // flags

                if version[0] == 1 {
                    reader.seek(SeekFrom::Current(16))?; // creation/modification time (8+8)
                } else {
                    reader.seek(SeekFrom::Current(8))?; // creation/modification time (4+4)
                }
                reader.seek(SeekFrom::Current(4))?; // timescale
                if version[0] == 1 {
                    reader.seek(SeekFrom::Current(8))?; // duration
                } else {
                    reader.seek(SeekFrom::Current(4))?; // duration
                }

                let mut lang_buf = [0u8; 2];
                reader.read_exact(&mut lang_buf)?;
                let lang_code = u16::from_be_bytes(lang_buf);
                // ISO 639-2/T language code packed in 15 bits
                if lang_code != 0x55C4 {
                    // 'und' (undefined)
                    let c1 = ((lang_code >> 10) & 0x1F) as u8 + 0x60;
                    let c2 = ((lang_code >> 5) & 0x1F) as u8 + 0x60;
                    let c3 = (lang_code & 0x1F) as u8 + 0x60;
                    language = Some(format!("{}{}{}", c1 as char, c2 as char, c3 as char));
                }
            }
            b"minf" => {
                let header_size = if box_size == 1 { 16 } else { 8 };
                codec = parse_mp4_minf(reader, actual_size - header_size)?;
            }
            _ => {}
        }

        if actual_size > 0 {
            reader.seek(SeekFrom::Start(pos + actual_size))?;
        } else {
            break;
        }
    }

    Ok((track_type, codec, language))
}

fn parse_mp4_minf<R: Read + Seek>(reader: &mut R, size: u64) -> std::io::Result<String> {
    let end_pos = reader.stream_position()? + size;

    while reader.stream_position()? < end_pos {
        let pos = reader.stream_position()?;

        let mut size_buf = [0u8; 4];
        if reader.read(&mut size_buf)? < 4 {
            break;
        }
        let box_size = u64::from(u32::from_be_bytes(size_buf));

        let mut type_buf = [0u8; 4];
        if reader.read(&mut type_buf)? < 4 {
            break;
        }

        let actual_size = if box_size == 1 {
            let mut size64_buf = [0u8; 8];
            reader.read_exact(&mut size64_buf)?;
            u64::from_be_bytes(size64_buf)
        } else {
            box_size
        };

        if &type_buf == b"stbl" {
            let header_size = if box_size == 1 { 16 } else { 8 };
            return parse_mp4_stbl(reader, actual_size - header_size);
        }

        if actual_size > 0 {
            reader.seek(SeekFrom::Start(pos + actual_size))?;
        } else {
            break;
        }
    }

    Ok(String::new())
}

fn parse_mp4_stbl<R: Read + Seek>(reader: &mut R, size: u64) -> std::io::Result<String> {
    let end_pos = reader.stream_position()? + size;

    while reader.stream_position()? < end_pos {
        let pos = reader.stream_position()?;

        let mut size_buf = [0u8; 4];
        if reader.read(&mut size_buf)? < 4 {
            break;
        }
        let box_size = u64::from(u32::from_be_bytes(size_buf));

        let mut type_buf = [0u8; 4];
        if reader.read(&mut type_buf)? < 4 {
            break;
        }

        let actual_size = if box_size == 1 {
            let mut size64_buf = [0u8; 8];
            reader.read_exact(&mut size64_buf)?;
            u64::from_be_bytes(size64_buf)
        } else {
            box_size
        };

        if &type_buf == b"stsd" {
            // Sample description box
            reader.seek(SeekFrom::Current(4))?; // version + flags
            let mut entry_count_buf = [0u8; 4];
            reader.read_exact(&mut entry_count_buf)?;

            // Read first entry's codec
            if reader.stream_position()? + 8 <= pos + actual_size {
                reader.seek(SeekFrom::Current(4))?; // entry size
                let mut codec_buf = [0u8; 4];
                reader.read_exact(&mut codec_buf)?;
                return Ok(String::from_utf8_lossy(&codec_buf).to_string());
            }
        }

        if actual_size > 0 {
            reader.seek(SeekFrom::Start(pos + actual_size))?;
        } else {
            break;
        }
    }

    Ok(String::new())
}

/// List tracks in a Transport Stream file
/// # Errors
/// Returns an error if the file cannot be opened or parsed.
#[allow(clippy::too_many_lines)]
pub fn list_ts_tracks(path: &Path) -> std::io::Result<Vec<TrackInfo>> {
    let file = File::open(path)?;
    let mut reader = BufReader::new(file);
    let file_size = reader.seek(SeekFrom::End(0))?;
    reader.seek(SeekFrom::Start(0))?;

    let mut tracks = Vec::new();
    let mut pmt_pids: Vec<u16> = Vec::new();
    let mut found_pids: std::collections::HashSet<u16> = std::collections::HashSet::new();

    // Determine packet size (188 or 192 for M2TS)
    let mut first_byte = [0u8; 1];
    reader.read_exact(&mut first_byte)?;
    reader.seek(SeekFrom::Start(0))?;

    let (packet_size, offset) = if first_byte[0] == 0x47 {
        (188usize, 0usize)
    } else {
        (192usize, 4usize) // M2TS has 4-byte timestamp prefix
    };

    let mut packet = vec![0u8; packet_size];
    let mut packets_read = 0u32;
    let max_packets = 10000; // Limit scanning to avoid processing entire file

    while reader.stream_position()? + packet_size as u64 <= file_size && packets_read < max_packets
    {
        reader.read_exact(&mut packet)?;
        packets_read += 1;

        let sync_byte = packet[offset];
        if sync_byte != 0x47 {
            continue;
        }

        let pid = ((u16::from(packet[offset + 1] & 0x1F)) << 8) | u16::from(packet[offset + 2]);
        let payload_start = (packet[offset + 1] & 0x40) != 0;

        // PAT (PID 0)
        if pid == 0 && payload_start {
            let adaptation_field = (packet[offset + 3] & 0x30) >> 4;
            let mut payload_offset = offset + 4;

            if adaptation_field == 2 || adaptation_field == 3 {
                let af_length = packet[payload_offset] as usize;
                payload_offset += 1 + af_length;
            }

            if payload_offset < packet_size {
                // Skip pointer field
                let pointer = packet[payload_offset] as usize;
                payload_offset += 1 + pointer;

                if payload_offset + 8 < packet_size {
                    // Parse PAT
                    let section_length = (((packet[payload_offset + 1] & 0x0F) as usize) << 8)
                        | packet[payload_offset + 2] as usize;
                    payload_offset += 8; // Skip to program loop

                    let end = std::cmp::min(payload_offset + section_length - 9, packet_size - 4);
                    while payload_offset + 4 <= end {
                        let program_num = ((u16::from(packet[payload_offset])) << 8)
                            | u16::from(packet[payload_offset + 1]);
                        let pmt_pid = ((u16::from(packet[payload_offset + 2] & 0x1F)) << 8)
                            | u16::from(packet[payload_offset + 3]);
                        if program_num != 0 {
                            pmt_pids.push(pmt_pid);
                        }
                        payload_offset += 4;
                    }
                }
            }
        }

        // PMT
        if pmt_pids.contains(&pid) && payload_start && !found_pids.contains(&pid) {
            found_pids.insert(pid);

            let adaptation_field = (packet[offset + 3] & 0x30) >> 4;
            let mut payload_offset = offset + 4;

            if adaptation_field == 2 || adaptation_field == 3 {
                let af_length = packet[payload_offset] as usize;
                payload_offset += 1 + af_length;
            }

            if payload_offset < packet_size {
                let pointer = packet[payload_offset] as usize;
                payload_offset += 1 + pointer;

                if payload_offset + 12 < packet_size {
                    let section_length = (((packet[payload_offset + 1] & 0x0F) as usize) << 8)
                        | packet[payload_offset + 2] as usize;
                    let _pcr_pid = ((u16::from(packet[payload_offset + 8] & 0x1F)) << 8)
                        | u16::from(packet[payload_offset + 9]);
                    let program_info_length = (((packet[payload_offset + 10] & 0x0F) as usize)
                        << 8)
                        | packet[payload_offset + 11] as usize;

                    payload_offset += 12 + program_info_length;
                    let end = std::cmp::min(
                        payload_offset + section_length - 13 - program_info_length,
                        packet_size - 4,
                    );

                    let mut track_num = 1u32;
                    while payload_offset + 5 <= end {
                        let stream_type = packet[payload_offset];
                        let elementary_pid = ((u16::from(packet[payload_offset + 1] & 0x1F)) << 8)
                            | u16::from(packet[payload_offset + 2]);
                        let es_info_length = (((packet[payload_offset + 3] & 0x0F) as usize) << 8)
                            | packet[payload_offset + 4] as usize;

                        let (track_type, codec) = match stream_type {
                            0x01 | 0x02 => (TrackType::Video, "MPEG-1/2 Video".to_string()),
                            0x1B => (TrackType::Video, "H.264/AVC".to_string()),
                            0x24 => (TrackType::Video, "H.265/HEVC".to_string()),
                            0x03 | 0x04 => (TrackType::Audio, "MPEG Audio".to_string()),
                            0x0F => (TrackType::Audio, "AAC".to_string()),
                            0x81 => (TrackType::Audio, "AC-3".to_string()),
                            0x06 => (
                                TrackType::Subtitle,
                                "Private Data (may contain subtitles)".to_string(),
                            ),
                            0x05 => (
                                TrackType::Other("Private Section".to_string()),
                                "Private Section".to_string(),
                            ),
                            _ => (
                                TrackType::Other(format!("Type 0x{:02X}", stream_type)),
                                format!("Stream type 0x{:02X}", stream_type),
                            ),
                        };

                        tracks.push(TrackInfo {
                            track_number: track_num,
                            track_type,
                            codec: format!("{} (PID {})", codec, elementary_pid),
                            language: None,
                        });

                        track_num += 1;
                        payload_offset += 5 + es_info_length;
                    }
                }
            }
        }

        // Stop if we've found PMT data
        if !tracks.is_empty() && packets_read > 1000 {
            break;
        }
    }

    Ok(tracks)
}

/// Main function to list tracks for any supported file
///
/// # Errors
/// Returns an error if the file format cannot be detected or parsing fails.
pub fn list_tracks(path: &Path) -> Result<(), String> {
    let format = detect_format(path).map_err(|e| format!("Error detecting file format: {}", e))?;

    println!("\nCCExtractor Track Listing");
    println!("-------------------------");
    println!("File: {}", path.display());

    let tracks = match format {
        FileFormat::Mkv => {
            println!("Format: Matroska (MKV/WebM)\n");
            list_mkv_tracks(path).map_err(|e| format!("Error parsing MKV: {}", e))?
        }
        FileFormat::Mp4 => {
            println!("Format: MP4/MOV\n");
            list_mp4_tracks(path).map_err(|e| format!("Error parsing MP4: {}", e))?
        }
        FileFormat::TransportStream => {
            println!("Format: MPEG Transport Stream\n");
            list_ts_tracks(path).map_err(|e| format!("Error parsing TS: {}", e))?
        }
        FileFormat::Unknown => {
            return Err("Unknown or unsupported file format".to_string());
        }
    };

    if tracks.is_empty() {
        println!("No tracks found in file.");
    } else {
        println!("Available tracks:");
        for track in &tracks {
            print!("  Track {}: Type: {}", track.track_number, track.track_type);
            if !track.codec.is_empty() {
                print!(", Codec: {}", track.codec);
            }
            if let Some(ref lang) = track.language {
                print!(", Language: {}", lang);
            }
            println!();
        }
    }

    println!("\nTrack listing completed.");
    Ok(())
}
