#!/usr/bin/env python3
"""
Build a minimal, spec-valid MPEG-TS sample that reproduces the CCExtractor bug:

    dvbsub_parse_object_segment() in src/lib_ccx/dvb_subtitle_decoder.c
    silently returns 0 (success) for a DVB subtitle "object data segment"
    whose object_coding_method == 1 (character/"string"-coded object),
    instead of treating it as an unhandled/failed object.

Structure produced:
  - PAT (PID 0x0000) -> program 1 -> PMT PID 0x0100
  - PMT (PID 0x0100) -> one ES: stream_type 0x06 (private/DVB) on PID 0x0101,
    with a subtitling_descriptor (tag 0x59): lang "eng", subtitling_type 0x10,
    composition_page_id = 1, ancillary_page_id = 1.
  - PES packets (stream_id 0xBD, private_stream_1) on PID 0x0101 carrying the
    DVB subtitle payload:
        data_identifier(0x20) + subtitle_stream_id(0x00) +
        PAGE_SEGMENT(0x10, page_id=1) +
        REGION_SEGMENT(0x11, page_id=1) with one object (object_id=1) +
        OBJECT_SEGMENT(0x13, page_id=1) for object_id=1 with
            object_coding_method = 1  <-- triggers the bug
        DISPLAY_SEGMENT(0x80, page_id=1) +
        end_of_PES_data_field_marker (0xff)

All of this is wrapped into 188-byte TS packets with correct sync byte,
PID, payload_unit_start_indicator, continuity_counter and a standard
MPEG-2 CRC32 for the PSI sections.
"""

import struct

OUT_PATH = "dvb_string_coding_sample.ts"

# ---------------------------------------------------------------------------
# CRC32/MPEG2 (used by PAT/PMT section CRC)
# ---------------------------------------------------------------------------
def crc32_mpeg2(data: bytes) -> int:
    crc = 0xFFFFFFFF
    for byte in data:
        crc ^= byte << 24
        for _ in range(8):
            if crc & 0x80000000:
                crc = ((crc << 1) ^ 0x04C11DB7) & 0xFFFFFFFF
            else:
                crc = (crc << 1) & 0xFFFFFFFF
    return crc


# ---------------------------------------------------------------------------
# TS packetizer: takes an arbitrary payload (PSI section incl. pointer_field,
# or PES packet bytes) and splits it into 188-byte TS packets for a PID.
# ---------------------------------------------------------------------------
class TSWriter:
    def __init__(self):
        self.packets = []
        self.cc = {}  # pid -> continuity_counter (0..15)

    def _next_cc(self, pid):
        cc = self.cc.get(pid, 0)
        self.cc[pid] = (cc + 1) & 0x0F
        return cc

    def write_section(self, pid, section_bytes):
        """section_bytes already includes the pointer_field byte (0x00) prefix."""
        self._write_payload(pid, section_bytes, is_pes=False)

    def write_pes(self, pid, pes_bytes):
        self._write_payload(pid, pes_bytes, is_pes=True)

    def _write_payload(self, pid, data, is_pes):
        offset = 0
        first = True
        total = len(data)
        while offset < total or first:
            chunk = data[offset:offset + 184]
            pusi = 1 if first else 0
            cc = self._next_cc(pid)

            header = bytearray()
            header.append(0x47)  # sync byte
            b1 = (pusi << 6) | ((pid >> 8) & 0x1F)
            header.append(b1)
            header.append(pid & 0xFF)
            adaptation_field_control = 0b01  # payload only, no adaptation field
            b3 = (0 << 6) | (adaptation_field_control << 4) | cc  # transport_scrambling=0
            header.append(b3)

            payload = bytearray(chunk)
            # pad last packet with 0xFF using an adaptation field if needed
            remaining_space = 184 - len(payload)
            if remaining_space > 0:
                # use adaptation field stuffing to pad to exactly 184 bytes
                if remaining_space == 1:
                    # adaptation field of length 0 needs at least 1 byte (the length byte itself = 0)
                    af = bytearray([0x00])
                else:
                    af_len = remaining_space - 1  # 1 byte for the length field itself
                    af = bytearray([af_len, 0x00]) + bytearray([0xFF] * (af_len - 1))
                header[3] = (0 << 6) | (0b11 << 4) | cc  # adaptation field + payload present
                packet = bytes(header) + bytes(af) + bytes(payload)
            else:
                packet = bytes(header) + bytes(payload)

            assert len(packet) == 188, f"bad TS packet length {len(packet)}"
            self.packets.append(packet)

            offset += len(chunk)
            first = False

    def bytes(self):
        return b"".join(self.packets)


# ---------------------------------------------------------------------------
# PAT
# ---------------------------------------------------------------------------
def build_pat(pmt_pid):
    # section body starting AFTER pointer_field, from table_id
    table_id = 0x00
    program_number = 1

    section_data = bytearray()
    section_data += struct.pack(">H", 0xE000 | program_number)  # program_number bits go after... build carefully below

    # Build properly:
    body = bytearray()
    body += struct.pack(">H", program_number)          # program_number
    body += struct.pack(">H", 0xE000 | (pmt_pid & 0x1FFF))  # reserved(3)=111 + PMT PID(13)

    section_length_placeholder = 5 + len(body) + 4  # after section_length field: 5 bytes header fields + body + CRC(4)
    # section_syntax fields between length and CRC:
    #   transport_stream_id(2) version/current(1) section_number(1) last_section_number(1) = 5 bytes
    transport_stream_id = 1
    version_current = 0xC1  # version=0, current_next_indicator=1, reserved bits=11
    section_number = 0x00
    last_section_number = 0x00

    section_after_length = bytearray()
    section_after_length += struct.pack(">H", transport_stream_id)
    section_after_length.append(version_current)
    section_after_length.append(section_number)
    section_after_length.append(last_section_number)
    section_after_length += body

    section_length = len(section_after_length) + 4  # + CRC32
    b1 = 0xB0 | ((section_length >> 8) & 0x0F)  # section_syntax_indicator=1, '0', reserved=11, length high nibble
    b2 = section_length & 0xFF

    full_no_crc = bytearray()
    full_no_crc.append(table_id)
    full_no_crc.append(b1)
    full_no_crc.append(b2)
    full_no_crc += section_after_length

    crc = crc32_mpeg2(bytes(full_no_crc))
    full = bytes(full_no_crc) + struct.pack(">I", crc)

    return b"\x00" + full  # pointer_field=0x00 then section


# ---------------------------------------------------------------------------
# PMT with DVB subtitling_descriptor (tag 0x59) on a stream_type 0x06 ES
# ---------------------------------------------------------------------------
def build_pmt(program_number, pcr_pid, es_pid, es_stream_type,
              lang="eng", subtitling_type=0x10, composition_page_id=1, ancillary_page_id=1):
    table_id = 0x02

    # subtitling_descriptor payload (8 bytes per language entry):
    #   ISO_639_language_code (3 bytes)
    #   subtitling_type (1 byte)
    #   composition_page_id (2 bytes)
    #   ancillary_page_id (2 bytes)
    desc_payload = bytearray()
    desc_payload += lang.encode("ascii")
    desc_payload.append(subtitling_type)
    desc_payload += struct.pack(">H", composition_page_id)
    desc_payload += struct.pack(">H", ancillary_page_id)
    descriptor = bytearray()
    descriptor.append(0x59)               # descriptor_tag: subtitling_descriptor
    descriptor.append(len(desc_payload))  # descriptor_length
    descriptor += desc_payload

    es_info = bytes(descriptor)

    es_loop = bytearray()
    es_loop.append(es_stream_type)                              # stream_type = 0x06
    es_loop += struct.pack(">H", 0xE000 | (es_pid & 0x1FFF))     # reserved(3)=111 + elementary_PID
    es_loop += struct.pack(">H", 0xF000 | (len(es_info) & 0x0FFF))  # reserved(4)=1111 + ES_info_length
    es_loop += es_info

    program_info_length = 0
    body_after_length = bytearray()
    body_after_length += struct.pack(">H", program_number)
    body_after_length.append(0xC1)                               # version=0, current_next=1, reserved=11
    body_after_length.append(0x00)                                # section_number
    body_after_length.append(0x00)                                # last_section_number
    body_after_length += struct.pack(">H", 0xE000 | (pcr_pid & 0x1FFF))  # reserved(3) + PCR_PID
    body_after_length += struct.pack(">H", 0xF000 | (program_info_length & 0x0FFF))  # program_info_length = 0
    body_after_length += es_loop

    section_length = len(body_after_length) + 4  # + CRC32
    b1 = 0xB0 | ((section_length >> 8) & 0x0F)
    b2 = section_length & 0xFF

    full_no_crc = bytearray()
    full_no_crc.append(table_id)
    full_no_crc.append(b1)
    full_no_crc.append(b2)
    full_no_crc += body_after_length

    crc = crc32_mpeg2(bytes(full_no_crc))
    full = bytes(full_no_crc) + struct.pack(">I", crc)

    return b"\x00" + full


# ---------------------------------------------------------------------------
# DVB subtitle segments (ETSI EN 300 743)
# ---------------------------------------------------------------------------
def seg(segment_type, page_id, payload):
    out = bytearray()
    out.append(0x0F)                     # sync_byte
    out.append(segment_type)
    out += struct.pack(">H", page_id)
    out += struct.pack(">H", len(payload))
    out += payload
    return bytes(out)


def build_page_segment(page_id, region_id=1, page_version=0, page_state=2, timeout=60):
    # payload: page_time_out(1) | page_version(4)+page_state(2)+reserved(2) | region entries...
    payload = bytearray()
    payload.append(timeout & 0xFF)
    payload.append(((page_version & 0x0F) << 4) | ((page_state & 0x03) << 2))
    # region entry: region_id(1) reserved(1) region_horizontal_address(2) region_vertical_address(2)
    payload.append(region_id & 0xFF)
    payload.append(0x00)                   # reserved
    payload += struct.pack(">H", 0)        # region_horizontal_address
    payload += struct.pack(">H", 0)        # region_vertical_address
    return seg(0x10, page_id, bytes(payload))


def build_region_segment(page_id, region_id=1, width=16, height=16, object_id=1,
                          region_version=0, depth_code=3, clut_id=0, bgcolor=0):
    # header (10 bytes):
    #   region_id(1)
    #   region_version(4)+region_fill_flag(1)+reserved(3)
    #   region_width(2) region_height(2)
    #   region_level_of_compatibility(3)+region_depth(3)+reserved(2)  [we only need depth bits]
    #   region_CLUT_id(1)
    #   region_8bit_pixel_code(1) [bgcolor when depth==8]
    #   reserved/unused(1)
    payload = bytearray()
    payload.append(region_id & 0xFF)
    payload.append(((region_version & 0x0F) << 4) | (1 << 3))   # fill flag = 1
    payload += struct.pack(">H", width)
    payload += struct.pack(">H", height)
    payload.append((depth_code & 0x07) << 2)   # region_depth code (3 -> 8bpp)
    payload.append(clut_id & 0xFF)
    payload.append(bgcolor & 0xFF)
    payload.append(0x00)  # padding byte consumed by the depth==8 branch (buf+=1)

    # one object entry (6 bytes, object_type = 0 -> no fg/bg color bytes needed):
    #   object_id(2)
    #   object_type(2)+object_provider_flag(2)+object_horizontal_position high nibble(4) -> top byte
    #   object_horizontal_position low byte
    #   reserved(4)+object_vertical_position high nibble -> top byte
    #   object_vertical_position low byte
    payload += struct.pack(">H", object_id)
    x_pos = 0
    y_pos = 0
    payload += struct.pack(">H", x_pos & 0x0FFF)   # object_type=0 in top bits
    payload += struct.pack(">H", y_pos & 0x0FFF)

    return seg(0x11, page_id, bytes(payload))


def build_object_segment_string_coded(page_id, object_id=1):
    """
    object_data_segment with object_coding_method == 1 (character/"string" coding).
    This is the exact condition that triggers dvbsub_parse_object_segment()'s
    coding_method == 1 branch (the FIXME / bug under test).

    Byte layout consumed by dvbsub_parse_object_segment():
        object_id (2 bytes)
        flags byte: bits[3:2] = object_coding_method, bit[1] = non_modifying_color_flag
    coding_method == 1  =>  bits[3:2] == 01  =>  e.g. byte value 0x04
    """
    payload = bytearray()
    payload += struct.pack(">H", object_id)
    payload.append(0x04)  # coding_method = (0x04 >> 2) & 3 = 1 ; non_modifying = (0x04 >> 1) & 1 = 0
    return seg(0x13, page_id, bytes(payload))


def build_display_segment(page_id):
    return seg(0x80, page_id, b"")


def build_dvb_subtitle_pes_payload(page_id=1, region_id=1, object_id=1):
    data_identifier = 0x20
    subtitle_stream_id = 0x00
    out = bytearray()
    out.append(data_identifier)
    out.append(subtitle_stream_id)
    out += build_page_segment(page_id, region_id=region_id)
    out += build_region_segment(page_id, region_id=region_id, object_id=object_id)
    out += build_object_segment_string_coded(page_id, object_id=object_id)
    out += build_display_segment(page_id)
    out.append(0xFF)  # end_of_PES_data_field_marker
    return bytes(out)


# ---------------------------------------------------------------------------
# PES wrapper (private_stream_1, stream_id 0xBD) with a PTS-only optional header
# ---------------------------------------------------------------------------
def encode_pts(pts, marker_prefix):
    """
    Encode a 33-bit PTS value into 5 bytes with the given 4-bit marker_prefix
    (0x2 for PTS-only 'prefix', 0x3 for PTS-in-PTS/DTS pair, 0x1 for DTS).
    """
    out = bytearray(5)
    out[0] = ((marker_prefix & 0x0F) << 4) | (((pts >> 30) & 0x07) << 1) | 1
    out[1] = (pts >> 22) & 0xFF
    out[2] = (((pts >> 15) & 0x7F) << 1) | 1
    out[3] = (pts >> 7) & 0xFF
    out[4] = ((pts & 0x7F) << 1) | 1
    return bytes(out)


def build_pes_packet(stream_id, payload, pts):
    header = bytearray()
    header += b"\x00\x00\x01"          # packet_start_code_prefix
    header.append(stream_id)           # 0xBD = private_stream_1

    pts_bytes = encode_pts(pts, 0x2)   # '0010' prefix marks PTS-only

    optional_header = bytearray()
    optional_header.append(0x80)       # '10' marker + flags (no scramble/priority/etc)
    optional_header.append(0x80)       # PTS_DTS_flags = '10' (PTS only)
    optional_header.append(0x05)       # PES_header_data_length = 5 (just the PTS)
    optional_header += pts_bytes

    pes_payload = optional_header + payload

    pes_packet_length = len(pes_payload)  # bytes following the length field
    header += struct.pack(">H", pes_packet_length)
    header += pes_payload

    return bytes(header)


# ---------------------------------------------------------------------------
# Assemble the full .ts file
# ---------------------------------------------------------------------------
def main():
    PMT_PID = 0x100
    SUB_PID = 0x101
    PROGRAM_NUMBER = 1
    COMPOSITION_PAGE_ID = 1
    ANCILLARY_PAGE_ID = 1

    w = TSWriter()

    pat = build_pat(pmt_pid=PMT_PID)
    w.write_section(0x0000, pat)

    pmt = build_pmt(
        program_number=PROGRAM_NUMBER,
        pcr_pid=SUB_PID,
        es_pid=SUB_PID,
        es_stream_type=0x06,
        lang="eng",
        subtitling_type=0x10,
        composition_page_id=COMPOSITION_PAGE_ID,
        ancillary_page_id=ANCILLARY_PAGE_ID,
    )
    w.write_section(PMT_PID, pmt)

    dvb_payload = build_dvb_subtitle_pes_payload(
        page_id=COMPOSITION_PAGE_ID, region_id=1, object_id=1
    )

    pts = 90000  # 1 second, arbitrary but valid 33-bit PTS
    pes = build_pes_packet(0xBD, dvb_payload, pts)

    # Send the same PES packet a couple of times to make sure the demuxer
    # has ample opportunity to pick up the PMT + stream before the payload
    # is parsed (some demuxers need a few packets of run-in).
    for i in range(3):
        w.write_pes(SUB_PID, pes)
        # re-issue PAT/PMT periodically like a real broadcast stream does
        w.write_section(0x0000, pat)
        w.write_section(PMT_PID, pmt)

    data = w.bytes()
    with open(OUT_PATH, "wb") as f:
        f.write(data)

    print(f"Wrote {OUT_PATH}: {len(data)} bytes, {len(data)//188} TS packets")


if __name__ == "__main__":
    main()
