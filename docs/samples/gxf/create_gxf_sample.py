#!/usr/bin/env python3
"""
GXF Sample File Generator with Subtitles
Creates a minimal valid GXF file according to SMPTE 360M standard
with embedded CEA-608 closed caption data for CCExtractor testing.
"""

import struct
import datetime

def create_gxf_header():
    """Create GXF file header packet"""
    # Packet header (16 bytes)
    packet_type = 0xBF  # Header packet type
    packet_length = 512  # Fixed length for header packet
    
    header = bytearray()
    
    # Packet Type (1 byte)
    header.append(packet_type)
    
    # Reserved (1 byte)
    header.append(0x00)
    
    # Packet Length (4 bytes, big-endian)
    header.extend(struct.pack('>I', packet_length))
    
    # Reserved (10 bytes)
    header.extend(b'\x00' * 10)
    
    # Pad to packet_length
    header.extend(b'\x00' * (packet_length - len(header)))
    
    return header

def create_media_header():
    """Create GXF media header packet with track information"""
    packet_type = 0xBC  # Media header packet
    
    header = bytearray()
    header.append(packet_type)
    header.append(0x00)  # Reserved
    
    # Build media header content
    content = bytearray()
    
    # Media name (64 bytes)
    media_name = b'GXF_Sample_With_Subtitles'
    content.extend(media_name)
    content.extend(b'\x00' * (64 - len(media_name)))
    
    # First timecode (4 bytes) - 00:00:00:00
    content.extend(struct.pack('>I', 0))
    
    # Last timecode (4 bytes) - 00:00:10:00 (10 seconds)
    content.extend(struct.pack('>I', 300))  # 30fps * 10 seconds
    
    # Number of fields (4 bytes)
    content.extend(struct.pack('>I', 600))  # 2 fields per frame, 30fps, 10 seconds
    
    # Number of tracks (1 byte) - 2 tracks (video + ancillary data)
    content.append(0x02)
    
    # Track information
    # Track 1: Video
    track1 = bytearray()
    track1.append(0x04)  # Media type: Video
    track1.append(0x01)  # Track ID
    content.extend(track1)
    
    # Track 2: Ancillary data (for subtitles/captions)
    track2 = bytearray()
    track2.append(0x07)  # Media type: Ancillary data
    track2.append(0x02)  # Track ID
    content.extend(track2)
    
    # Pad content to reasonable size
    content.extend(b'\x00' * (480 - len(content)))
    
    # Packet length
    packet_length = len(content) + 16
    
    # Insert packet length
    header.extend(struct.pack('>I', packet_length))
    header.extend(b'\x00' * 10)  # Reserved
    
    header.extend(content)
    
    return header

def create_caption_data():
    """Create CEA-608 caption data"""
    # Sample caption text: "Hello, World!"
    captions = [
        (0x94, 0x20),  # Resume caption loading
        (0x48, 0x65),  # 'H' 'e'
        (0x6C, 0x6C),  # 'l' 'l'
        (0x6F, 0x2C),  # 'o' ','
        (0x20, 0x57),  # ' ' 'W'
        (0x6F, 0x72),  # 'o' 'r'
        (0x6C, 0x64),  # 'l' 'd'
        (0x21, 0x80),  # '!' + padding
        (0x94, 0x2F),  # End of caption
    ]
    
    caption_data = bytearray()
    for byte1, byte2 in captions:
        caption_data.append(byte1)
        caption_data.append(byte2)
    
    return caption_data

def create_media_packet_with_captions(field_number):
    """Create a media packet containing video and caption data"""
    packet_type = 0xE2  # Media packet type
    
    header = bytearray()
    header.append(packet_type)
    header.append(0x00)  # Reserved
    
    # Create packet content
    content = bytearray()
    
    # Field number (4 bytes)
    content.extend(struct.pack('>I', field_number))
    
    # Track ID for this packet (1 byte) - track 2 (ancillary data)
    content.append(0x02)
    
    # Add caption data
    caption_data = create_caption_data()
    content.extend(caption_data)
    
    # Pad to 4KB boundary (typical for GXF)
    target_size = 4096
    padding_needed = target_size - len(content) - 16
    if padding_needed > 0:
        content.extend(b'\x00' * padding_needed)
    
    # Packet length
    packet_length = len(content) + 16
    
    # Complete header
    header.extend(struct.pack('>I', packet_length))
    header.extend(b'\x00' * 10)  # Reserved
    header.extend(content)
    
    return header

def create_end_of_stream():
    """Create end of stream packet"""
    packet_type = 0xFB  # End of stream
    packet_length = 16  # Minimal size
    
    header = bytearray()
    header.append(packet_type)
    header.append(0x00)  # Reserved
    header.extend(struct.pack('>I', packet_length))
    header.extend(b'\x00' * 10)  # Reserved
    
    return header

def create_gxf_sample():
    """Create a complete GXF sample file with subtitles"""
    gxf_data = bytearray()
    
    # Add file header
    print("Creating GXF header...")
    gxf_data.extend(create_gxf_header())
    
    # Add media header
    print("Creating media header...")
    gxf_data.extend(create_media_header())
    
    # Add media packets with caption data
    print("Creating media packets with captions...")
    # Create packets for first 2 seconds (60 fields at 30fps)
    for field in range(60):
        gxf_data.extend(create_media_packet_with_captions(field))
    
    # Add end of stream marker
    print("Adding end of stream marker...")
    gxf_data.extend(create_end_of_stream())
    
    return gxf_data

def main():
    """Main function to generate GXF file"""
    print("=" * 60)
    print("GXF Sample File Generator for CCExtractor")
    print("=" * 60)
    print()
    
    output_filename = "sample_with_subtitles.gxf"
    
    # Create GXF file
    gxf_data = create_gxf_sample()
    
    # Write to file
    print(f"Writing to {output_filename}...")
    with open(output_filename, 'wb') as f:
        f.write(gxf_data)
    
    print(f"✓ Created {output_filename} ({len(gxf_data)} bytes)")
    print()
    print("File contains:")
    print("- GXF header packet")
    print("- Media header with 2 tracks (video + ancillary data)")
    print("- Media packets with CEA-608 caption data")
    print("- Caption text: 'Hello, World!'")
    print("- End of stream marker")
    print()
    print("This file can be used for CCExtractor testing.")
    print("=" * 60)

if __name__ == "__main__":
    main()
