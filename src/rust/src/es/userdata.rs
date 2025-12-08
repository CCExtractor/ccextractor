/* Return a pointer to a string that holds the printable characters
 * of the caption data block. FOR DEBUG PURPOSES ONLY! */
use crate::bindings::{cc_subtitle, encoder_ctx, lib_cc_decode};
use crate::current_fps;
use crate::es::core::dump;
use crate::{decode_vbi, do_cb, store_hdcc};
use lib_ccxr::common::{BitStreamRust, BitstreamError};
use lib_ccxr::util::log::{DebugMessageFlag, ExitCause};
use lib_ccxr::{debug, fatal, info};

fn debug_608_to_asc(cc_data: &[u8], channel: i32) -> String {
    let mut output = "  ".to_string();

    if cc_data.len() < 3 {
        return output;
    }

    let cc_valid = (cc_data[0] & 4) >> 2;
    let cc_type = (cc_data[0] & 3) as i32;

    if cc_valid != 0 && cc_type == channel {
        let hi = cc_data[1] & 0x7F; // Get rid of parity bit
        let lo = cc_data[2] & 0x7F; // Get rid of parity bit
        if hi >= 0x20 {
            output = format!(
                "{}{}",
                hi as char,
                if lo >= 0x20 { lo as char } else { '.' }
            );
        } else {
            output = "<>".to_string();
        }
    }

    output
}

/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls C functions like `do_cb`
pub unsafe fn user_data(
    enc_ctx: &mut encoder_ctx,
    dec_ctx: &mut lib_cc_decode,
    ustream: &mut BitStreamRust,
    udtype: i32,
    sub: &mut cc_subtitle,
) -> Result<i32, BitstreamError> {
    debug!(msg_type = DebugMessageFlag::VERBOSE; "user_data({})", udtype);

    // Shall not happen
    if ustream.error || ustream.bits_left <= 0 {
        // ustream->error=1;
        return Ok(0); // Actually discarded on call.
                      // CFS: Seen in a Wobble edited file.
                      // fatal(CCX_COMMON_EXIT_BUG_BUG, "user_data: Impossible!");
    }

    // Do something
    dec_ctx.stat_numuserheaders += 1;
    // header+=4;

    let ud_header = ustream.next_bytes(4)?;
    if ustream.error || ustream.bits_left <= 0 {
        return Ok(0); // Actually discarded on call.
                      // CFS: Seen in Stick_VHS.mpg.
                      // fatal(CCX_COMMON_EXIT_BUG_BUG, "user_data: Impossible!");
    }

    // DVD CC header, see
    // <http://www.theneitherworld.com/mcpoodle/SCC_TOOLS/DOCS/SCC_FORMAT.HTML>
    if ud_header.starts_with(&[0x43, 0x43]) {
        dec_ctx.stat_dvdccheaders += 1;

        // Probably unneeded, but keep looking for extra caption blocks
        let mut maybeextracb = true;

        ustream.read_bytes(4)?; // "43 43 01 F8"

        let pattern_flag = ustream.read_bits(1)? as u8;
        ustream.read_bits(1)?;
        let mut capcount = ustream.read_bits(5)? as i32;
        let truncate_flag = ustream.read_bits(1)? as i32; // truncate_flag - one CB extra

        let mut field1packet = 0; // expect Field 1 first
        if pattern_flag == 0x00 {
            field1packet = 1; // expect Field 1 second
        }

        debug!(msg_type = DebugMessageFlag::VERBOSE; "Reading {}{} DVD CC segments",
                  capcount, if truncate_flag != 0 { "+1" } else { "" });

        capcount += truncate_flag;

        // This data comes before the first frame header, so
        // in order to get the correct timing we need to set the
        // current time to one frame after the maximum time of the
        // last GOP.  Only useful when there are frames before
        // the GOP.
        if (*dec_ctx.timing).fts_max > 0 {
            (*dec_ctx.timing).fts_now = (*dec_ctx.timing).fts_max + (1000.0 / current_fps) as i64;
        }

        let mut rcbcount = 0;
        for i in 0..capcount {
            for j in 0..2 {
                let mut data = [0u8; 3];
                data[0] = ustream.bitstream_get_num(1, true)? as u8;
                data[1] = ustream.bitstream_get_num(1, true)? as u8;
                data[2] = ustream.bitstream_get_num(1, true)? as u8;

                // Obey the truncate flag.
                if truncate_flag != 0 && i == capcount - 1 && j == 1 {
                    maybeextracb = false;
                    break;
                }
                /* Field 1 and 2 data can be in either order,
                with marker bytes of \xff and \xfe
                Since markers can be repeated, use pattern as well */
                if (data[0] & 0xFE) == 0xFE {
                    // Check if valid
                    if data[0] == 0xff && j == field1packet {
                        data[0] = 0x04; // Field 1
                    } else {
                        data[0] = 0x05; // Field 2
                    }
                    do_cb(dec_ctx, data.as_mut_ptr(), sub);
                    rcbcount += 1;
                } else {
                    debug!(msg_type = DebugMessageFlag::VERBOSE; "Illegal caption segment - stop here.");
                    maybeextracb = false;
                    break;
                }
            }
        }
        // Theoretically this should not happen, oh well ...
        // Deal with extra closed captions some DVD have.
        let mut ecbcount = 0;
        while maybeextracb && (ustream.bitstream_get_num(1, false)? as u8 & 0xFE) == 0xFE {
            for j in 0..2 {
                let mut data = [0u8; 3];
                data[0] = ustream.bitstream_get_num(1, true)? as u8;
                data[1] = ustream.bitstream_get_num(1, true)? as u8;
                data[2] = ustream.bitstream_get_num(1, true)? as u8;
                /* Field 1 and 2 data can be in either order,
                with marker bytes of \xff and \xfe
                Since markers can be repeated, use pattern as well */
                if (data[0] & 0xFE) == 0xFE {
                    // Check if valid
                    if data[0] == 0xff && j == field1packet {
                        data[0] = 0x04; // Field 1
                    } else {
                        data[0] = 0x05; // Field 2
                    }
                    do_cb(dec_ctx, data.as_mut_ptr(), sub);
                    ecbcount += 1;
                } else {
                    debug!(msg_type = DebugMessageFlag::VERBOSE; "Illegal (extra) caption segment - stop here.");
                    maybeextracb = false;
                    break;
                }
            }
        }

        debug!(msg_type = DebugMessageFlag::VERBOSE; "Read {}/{} DVD CC blocks", rcbcount, ecbcount);
    }
    // SCTE 20 user data
    else if dec_ctx.noscte20 == 0 && ud_header[0] == 0x03 {
        // reserved - unspecified
        if (ud_header[1] & 0x7F) == 0x01 {
            let mut cc_data = [0u8; 3 * 31 + 1]; // Maximum cc_count is 31

            dec_ctx.stat_scte20ccheaders += 1;
            ustream.read_bytes(2)?; // "03 01"

            let cc_count = ustream.read_bits(5)? as u32;
            debug!(msg_type = DebugMessageFlag::VERBOSE; "Reading {} SCTE 20 CC blocks", cc_count);

            for j in 0..cc_count {
                ustream.skip_bits(2)?; // priority - unused
                let field_number = ustream.read_bits(2)? as u32;
                ustream.skip_bits(5)?; // line_offset - unused
                let cc_data1 = ustream.read_bits(8)? as u32;
                let cc_data2 = ustream.read_bits(8)? as u32;
                ustream.read_bits(1)?; // TODO: Add syntax check */
                if ustream.bits_left < 0 {
                    fatal!(cause = ExitCause::Bug; "In user_data: ustream->bitsleft < 0. Cannot continue.");
                }

                // Field_number is either
                //  0 .. forbidden
                //  1 .. field 1 (odd)
                //  2 .. field 2 (even)
                //  3 .. repeated, from repeat_first_field, effectively field 1
                if field_number < 1 {
                    // 0 is invalid
                    cc_data[(j * 3) as usize] = 0x00; // Set to invalid
                    cc_data[(j * 3 + 1) as usize] = 0x00;
                    cc_data[(j * 3 + 2) as usize] = 0x00;
                } else {
                    // Treat field_number 3 as 1
                    let mut field_number = (field_number - 1) & 0x01;
                    // top_field_first also affects to which field the caption
                    // belongs.
                    if dec_ctx.top_field_first == 0 {
                        field_number ^= 0x01;
                    }
                    cc_data[(j * 3) as usize] = 0x04 | (field_number as u8);
                    cc_data[(j * 3 + 1) as usize] = BitStreamRust::reverse8(cc_data1 as u8);
                    cc_data[(j * 3 + 2) as usize] = BitStreamRust::reverse8(cc_data2 as u8);
                }
            }
            cc_data[(cc_count * 3) as usize] = 0xFF;
            store_hdcc(
                enc_ctx,
                dec_ctx,
                cc_data.as_mut_ptr(),
                cc_count as _,
                (*dec_ctx.timing).current_tref,
                (*dec_ctx.timing).fts_now,
                sub,
            );

            debug!(msg_type = DebugMessageFlag::VERBOSE; "Reading SCTE 20 CC blocks - done");
        }
    }
    // ReplayTV 4000/5000 caption header - parsing information
    // derived from CCExtract.bdl
    else if (ud_header[0] == 0xbb    // ReplayTV 4000
        || ud_header[0] == 0x99) // ReplayTV 5000
        && ud_header[1] == 0x02
    {
        let mut data = [0u8; 3];

        if ud_header[0] == 0xbb {
            dec_ctx.stat_replay4000headers += 1;
        } else {
            dec_ctx.stat_replay5000headers += 1;
        }

        ustream.read_bytes(2)?; // "BB 02" or "99 02"
        data[0] = 0x05; // Field 2
        data[1] = ustream.bitstream_get_num(1, true)? as u8;
        data[2] = ustream.bitstream_get_num(1, true)? as u8;
        do_cb(dec_ctx, data.as_mut_ptr(), sub);
        ustream.read_bytes(2)?; // Skip "CC 02" for R4000 or "AA 02" for R5000
        data[0] = 0x04; // Field 1
        data[1] = ustream.bitstream_get_num(1, true)? as u8;
        data[2] = ustream.bitstream_get_num(1, true)? as u8;
        do_cb(dec_ctx, data.as_mut_ptr(), sub);
    }
    // HDTV - see A/53 Part 4 (Video)
    else if ud_header.starts_with(&[0x47, 0x41, 0x39, 0x34]) {
        dec_ctx.stat_hdtv += 1;

        ustream.read_bytes(4)?; // "47 41 39 34"

        let type_code = ustream.bitstream_get_num(1, true)? as u8;
        if type_code == 0x03 {
            // CC data.
            ustream.skip_bits(1)?; // reserved
            let process_cc_data = ustream.read_bits(1)? as u8;
            ustream.skip_bits(1)?; // additional_data - unused
            let cc_count = ustream.read_bits(5)? as u8;
            ustream.read_bytes(1)?; // "FF"
            if process_cc_data != 0 {
                debug!(msg_type = DebugMessageFlag::VERBOSE; "Reading {} HDTV CC blocks", cc_count);

                let mut proceed = true;
                let cc_data = ustream.read_bytes(cc_count as usize * 3 + 1)?;
                if ustream.bits_left < 0 {
                    fatal!(cause = ExitCause::Bug; "In user_data: ustream->bitsleft < 0. Cannot continue.");
                }

                // Check for proper marker - This read makes sure that
                // cc_count*3+1 bytes are read and available in cc_data.
                if ustream.bitstream_get_num(1, true)? as u8 != 0xFF {
                    proceed = false;
                }

                if !proceed {
                    debug!(msg_type = DebugMessageFlag::VERBOSE; "\rThe following payload is not properly terminated.");
                    dump(cc_data.to_vec().as_mut_ptr(), (cc_count * 3 + 1) as _, 0, 0);
                }
                debug!(msg_type = DebugMessageFlag::VERBOSE; "Reading {} HD CC blocks", cc_count);

                // B-frames might be (temporal) before or after the anchor
                // frame they belong to. Store the buffer until the next anchor
                // frame occurs.  The buffer will be flushed (sorted) in the
                // picture header (or GOP) section when the next anchor occurs.
                // Please note we store the current value of the global
                // fts_now variable (and not get_fts()) as we are going to
                // re-create the timeline in process_hdcc() (Slightly ugly).
                store_hdcc(
                    enc_ctx,
                    dec_ctx,
                    cc_data.to_vec().as_mut_ptr(),
                    cc_count as _,
                    (*dec_ctx.timing).current_tref,
                    (*dec_ctx.timing).fts_now,
                    sub,
                );

                debug!(msg_type = DebugMessageFlag::VERBOSE; "Reading HDTV blocks - done");
            }
        }
        // reserved - additional_cc_data
    }
    // DVB closed caption header for Dish Network (Field 1 only) */
    else if ud_header.starts_with(&[0x05, 0x02]) {
        // Like HDTV (above) Dish Network captions can be stored at each
        // frame, but maximal two caption blocks per frame and only one
        // field is stored.
        // To process this with the HDTV framework we create a "HDTV" caption
        // format compatible array. Two times 3 bytes plus one for the 0xFF
        // marker at the end. Pre-init to field 1 and set the 0xFF marker.
        let mut DISHDATA: [u8; 7] = [0x04, 0, 0, 0x04, 0, 0, 0xFF];
        let mut cc_count: usize;

        debug!(msg_type = DebugMessageFlag::VERBOSE; "Reading Dish Network user data");

        dec_ctx.stat_dishheaders += 1;

        ustream.read_bytes(2)?; // "05 02"

        // The next bytes are like this:
        // header[2] : ID: 0x04 (MPEG?), 0x03 (H264?)
        // header[3-4]: Two byte counter (counting (sub-)GOPs?)
        // header[5-6]: Two bytes, maybe checksum?
        // header[7]: Pattern type
        //            on B-frame: 0x02, 0x04
        //            on I-/P-frame: 0x05
        let id = ustream.bitstream_get_num(1, true)? as u8;
        let dishcount = ustream.bitstream_get_num(2, true)? as u32;
        let something = ustream.bitstream_get_num(2, true)? as u32;
        let mut pattern_type = ustream.bitstream_get_num(1, true)? as u8;
        debug!(msg_type = DebugMessageFlag::PARSE; "DN  ID: {:02X}  Count: {:5}  Unknown: {:04X}  Pattern: {:X}",
                  id, dishcount, something, pattern_type);

        // The following block needs 4 to 6 bytes starting from the
        // current position
        let dcd_pos = ustream.pos; // dish caption data position
        match pattern_type {
            0x02 => {
                // Two byte caption - always on B-frame
                // The following 4 bytes are:
                // 0  :  0x09
                // 1-2: caption block
                // 3  : REPEAT - 02: two bytes
                //             - 04: four bytes (repeat first two)
                let dcd_data = &ustream.data[dcd_pos..dcd_pos + 4];
                debug!(msg_type = DebugMessageFlag::PARSE; "\n02 {:02X}  {:02X}:{:02X} - R:{:02X} :",
                          dcd_data[0], dcd_data[1], dcd_data[2], dcd_data[3]);

                cc_count = 1;
                unsafe {
                    DISHDATA[1] = dcd_data[1];
                    DISHDATA[2] = dcd_data[2];

                    debug!(msg_type = DebugMessageFlag::PARSE; "{}", debug_608_to_asc(&DISHDATA, 0));

                    pattern_type = dcd_data[3]; // repeater (0x02 or 0x04)
                    let hi = DISHDATA[1] & 0x7f; // Get only the 7 low bits
                    if pattern_type == 0x04 && hi < 32 {
                        // repeat (only for non-character pairs)
                        cc_count = 2;
                        DISHDATA[3] = 0x04; // Field 1
                        DISHDATA[4] = DISHDATA[1];
                        DISHDATA[5] = DISHDATA[2];

                        debug!(msg_type = DebugMessageFlag::PARSE; "{}:", debug_608_to_asc(&DISHDATA[3..], 0));
                    } else {
                        debug!(msg_type = DebugMessageFlag::PARSE; ":");
                    }

                    DISHDATA[cc_count * 3] = 0xFF; // Set end marker

                    store_hdcc(
                        enc_ctx,
                        dec_ctx,
                        DISHDATA.as_mut_ptr(),
                        cc_count as _,
                        (*dec_ctx.timing).current_tref,
                        (*dec_ctx.timing).fts_now,
                        sub,
                    );
                }

                // Ignore 3 (0x0A, followed by two unknown) bytes.
            }
            0x04 => {
                // Four byte caption - always on B-frame
                // The following 5 bytes are:
                // 0  :  0x09
                // 1-2: caption block
                // 3-4: caption block
                let dcd_data = &ustream.data[dcd_pos..dcd_pos + 5];
                debug!(msg_type = DebugMessageFlag::PARSE; "\n04 {:02X}  {:02X}:{:02X}:{:02X}:{:02X}  :",
                          dcd_data[0], dcd_data[1], dcd_data[2], dcd_data[3], dcd_data[4]);

                cc_count = 2;
                unsafe {
                    DISHDATA[1] = dcd_data[1];
                    DISHDATA[2] = dcd_data[2];

                    DISHDATA[3] = 0x04; // Field 1
                    DISHDATA[4] = dcd_data[3];
                    DISHDATA[5] = dcd_data[4];
                    DISHDATA[6] = 0xFF; // Set end marker

                    debug!(msg_type = DebugMessageFlag::PARSE; "{}", debug_608_to_asc(&DISHDATA, 0));
                    debug!(msg_type = DebugMessageFlag::PARSE; "{}:", debug_608_to_asc(&DISHDATA[3..], 0));

                    store_hdcc(
                        enc_ctx,
                        dec_ctx,
                        DISHDATA.as_mut_ptr(),
                        cc_count as _,
                        (*dec_ctx.timing).current_tref,
                        (*dec_ctx.timing).fts_now,
                        sub,
                    );
                }

                // Ignore 4 (0x020A, followed by two unknown) bytes.
            }
            0x05 => {
                // Buffered caption - always on I-/P-frame
                // The following six bytes are:
                // 0  :  0x04
                // - the following are from previous 0x05 caption header -
                // 1  : prev dcd[2]
                // 2-3: prev dcd[3-4]
                // 4-5: prev dcd[5-6]
                let dcd_data = &ustream.data[dcd_pos..dcd_pos + 10]; // Need more bytes for this case
                debug!(msg_type = DebugMessageFlag::PARSE; " - {:02X}  pch: {:02X} {:5} {:02X}:{:02X}",
                          dcd_data[0], dcd_data[1],
                          (dcd_data[2] as u32) * 256 + (dcd_data[3] as u32),
                          dcd_data[4], dcd_data[5]);

                // Now one of the "regular" 0x02 or 0x04 captions follows
                debug!(msg_type = DebugMessageFlag::PARSE; "{:02X} {:02X}  {:02X}:{:02X}",
                          dcd_data[6], dcd_data[7], dcd_data[8], dcd_data[9]);

                pattern_type = dcd_data[6]; // Number of caption bytes (0x02 or 0x04)

                cc_count = 1;
                unsafe {
                    DISHDATA[1] = dcd_data[8];
                    DISHDATA[2] = dcd_data[9];

                    if pattern_type == 0x02 {
                        pattern_type = dcd_data[10]; // repeater (0x02 or 0x04)

                        debug!(msg_type = DebugMessageFlag::PARSE; " - R:{:02X} :{}", pattern_type, debug_608_to_asc(&DISHDATA, 0));

                        let hi = DISHDATA[1] & 0x7f; // Get only the 7 low bits
                        if pattern_type == 0x04 && hi < 32 {
                            cc_count = 2;
                            DISHDATA[3] = 0x04; // Field 1
                            DISHDATA[4] = DISHDATA[1];
                            DISHDATA[5] = DISHDATA[2];
                            debug!(msg_type = DebugMessageFlag::PARSE; "{}:", debug_608_to_asc(&DISHDATA[3..], 0));
                        } else {
                            debug!(msg_type = DebugMessageFlag::PARSE; ":");
                        }
                        DISHDATA[cc_count * 3] = 0xFF; // Set end marker
                    } else {
                        debug!(msg_type = DebugMessageFlag::PARSE; ":{:02X}:{:02X}  ",
                                  dcd_data[10], dcd_data[11]);
                        cc_count = 2;
                        DISHDATA[3] = 0x04; // Field 1
                        DISHDATA[4] = dcd_data[10];
                        DISHDATA[5] = dcd_data[11];
                        DISHDATA[6] = 0xFF; // Set end marker

                        debug!(msg_type = DebugMessageFlag::PARSE; ":{}", debug_608_to_asc(&DISHDATA, 0));
                        debug!(msg_type = DebugMessageFlag::PARSE; "{}:", debug_608_to_asc(&DISHDATA[3..], 0));
                    }

                    store_hdcc(
                        enc_ctx,
                        dec_ctx,
                        DISHDATA.as_mut_ptr(),
                        cc_count as _,
                        (*dec_ctx.timing).current_tref,
                        (*dec_ctx.timing).fts_now,
                        sub,
                    );
                }

                // Ignore 3 (0x0A, followed by 2 unknown) bytes.
            }
            _ => {
                // printf ("Unknown?\n");
            }
        } // match

        debug!(msg_type = DebugMessageFlag::VERBOSE; "Reading Dish Network user data - done");
    }
    // CEA 608 / aka "Divicom standard", see:
    // http://www.pixeltools.com/tech_tip_closed_captioning.html
    else if ud_header.starts_with(&[0x02, 0x09]) {
        // Either a documentation or more examples are needed.
        dec_ctx.stat_divicom += 1;

        let mut data = [0u8; 3];

        ustream.read_bytes(2)?; // "02 09"
        ustream.read_bytes(2)?; // "80 80" ???
        ustream.read_bytes(2)?; // "02 0A" ???
        data[0] = 0x04; // Field 1
        data[1] = ustream.bitstream_get_num(1, true)? as u8;
        data[2] = ustream.bitstream_get_num(1, true)? as u8;
        do_cb(dec_ctx, data.as_mut_ptr(), sub);
        // This is probably incomplete!
    }
    // GXF vbi OEM code
    else if ud_header.starts_with(&[0x73, 0x52, 0x21, 0x06]) {
        let udatalen = ustream.data.len() - ustream.pos;
        ustream.read_bytes(4)?; // skip header code
        ustream.read_bytes(2)?; // skip data length
        let line_nb = ustream.read_bits(16)? as u16;
        let line_type = ustream.bitstream_get_num(1, true)? as u8;
        let field = line_type & 0x03;
        if field == 0 {
            info!("MPEG:VBI: Invalid field");
        }

        let line_type = line_type >> 2;
        if line_type != 1 {
            info!("MPEG:VBI: only support Luma line");
        }

        if udatalen < 720 {
            info!("MPEG:VBI: Minimum 720 bytes in luma line required");
        }

        let vbi_data = &ustream.data[ustream.pos..ustream.pos + 720];
        decode_vbi(dec_ctx, field, vbi_data.to_vec().as_mut_ptr(), 720, sub);
        debug!(msg_type = DebugMessageFlag::VERBOSE; "GXF (vbi line {}) user data:", line_nb);
    } else {
        // Some other user data
        // 06 02 ... Seems to be DirectTV
        debug!(msg_type = DebugMessageFlag::VERBOSE; "Unrecognized user data:");
        let udatalen = ustream.data.len() - ustream.pos;
        let dump_len = if udatalen > 128 { 128 } else { udatalen };
        dump(
            ustream.data[ustream.pos..ustream.pos + dump_len]
                .to_vec()
                .as_mut_ptr(),
            dump_len as _,
            0,
            0,
        );
    }

    debug!(msg_type = DebugMessageFlag::VERBOSE; "User data - processed");

    // Read complete
    Ok(1)
}
