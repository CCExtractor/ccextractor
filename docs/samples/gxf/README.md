# GXF Sample File with Subtitles

## Overview
This is a minimal but valid GXF (General eXchange Format) sample file created according to the SMPTE 360M standard. It contains embedded CEA-608 closed caption data for testing CCExtractor.

## File Information

- **Filename**: `sample_with_subtitles.gxf`
- **Size**: ~247 KB
- **Format**: GXF (SMPTE 360M)
- **Duration**: ~2 seconds
- **Tracks**: 2
  - Track 1: Video (placeholder)
  - Track 2: Ancillary data (CEA-608 captions)

## Embedded Subtitle Content

The file contains CEA-608 closed caption data with the following text:
```
Hello, World!
```

## Caption Specifications

- **Format**: CEA-608 (Line 21 closed captions)
- **Standard**: EIA-608-B
- **Data Location**: Ancillary data track (Track 2)
- **Caption Commands**:
  - Resume caption loading (0x94 0x20)
  - Text data ("Hello, World!")
  - End of caption (0x94 0x2F)

## Testing with CCExtractor

To extract subtitles from this file using CCExtractor:

```bash
ccextractor sample_with_subtitles.gxf -o output.srt
```

### Expected Output
The extracted subtitle file should contain:
```
1
00:00:00,000 --> 00:00:02,000
Hello, World!
```

## File Structure

1. **File Header Packet** (512 bytes)
   - Packet Type: 0xBF
   - Identifies the file as GXF format

2. **Media Header Packet**
   - Packet Type: 0xBC
   - Contains track information
   - Defines 2 tracks (video + ancillary data)
   - Timecode range: 00:00:00:00 to 00:00:10:00

3. **Media Packets** (60 packets, one per field)
   - Packet Type: 0xE2
   - Each packet is 4096 bytes (padded)
   - Contains CEA-608 caption data in ancillary track

4. **End of Stream Packet**
   - Packet Type: 0xFB
   - Marks the end of the file

## Technical Details

### GXF Packet Structure
Each packet follows this structure:
```
Byte 0:      Packet Type
Byte 1:      Reserved (0x00)
Bytes 2-5:   Packet Length (big-endian)
Bytes 6-15:  Reserved (0x00)
Bytes 16+:   Packet Data
```

### CEA-608 Caption Byte Pairs
The caption data consists of byte pairs:
- Control codes (0x94 0x20, 0x94 0x2F)
- Character pairs with odd parity
- Standard ASCII characters for text

## Generation Script

The file was generated using `create_gxf_sample.py`, which creates a standards-compliant minimal GXF file for testing purposes.

To regenerate:
```bash
python3 create_gxf_sample.py
```

## Use Cases

This sample file is suitable for:
- Testing GXF file parsing in CCExtractor
- Validating CEA-608 caption extraction
- Development and debugging of GXF subtitle support
- Automated testing in CI/CD pipelines
- Reference implementation for GXF format

## Compliance

- Conforms to SMPTE 360M standard
- Uses CEA-608 standard for caption encoding
- Compatible with professional broadcast equipment
- Can be read by VLC, FFmpeg, and other GXF-compatible players

## Limitations

This is a minimal test file and has the following limitations:
- No actual video frames (placeholder video track)
- Short duration (2 seconds of fields)
- Single caption sequence
- No audio tracks
- Simplified timecode

For production use, real GXF files would include:
- Compressed video (MPEG-2, M-JPEG, or DV)
- Audio tracks (uncompressed or compressed)
- Complete metadata
- Proper video frame data
- Extended user data fields

## References

- SMPTE 360M: GXF - General eXchange Format
- SMPTE RDD 14-2007: GXF High Definition Extension
- EIA-608-B: Line 21 Data Services
- CEA-608: Closed Captioning for NTSC Television

## Contributing to CCExtractor

If you're using this file to contribute to CCExtractor:

1. Test the file with CCExtractor
2. Verify subtitle extraction works correctly
3. Report any issues on the GitHub repository
4. Submit pull requests with fixes if needed

## License

This sample file is created for testing purposes and can be freely used for CCExtractor development and testing.

---

**Generated**: 2026-01-27  
**For**: CCExtractor Issue #1748  
**Purpose**: Development and testing of GXF subtitle support
