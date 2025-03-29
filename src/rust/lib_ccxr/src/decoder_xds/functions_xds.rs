use crate::time::c_functions::get_fts;
use crate::time::timing::CaptionField;
use crate::util::log::{debug, info, DebugMessageFlag};

use crate::decoder_xds::exit_codes::*;
use crate::decoder_xds::structs_xds::*;

//----------------------------------------------------------------

#[derive(Debug)]
pub enum XdsError {
    WriteError,
    OtherError(String),
}
pub fn write_xds_string(
    sub: &mut CcSubtitle,
    ctx: &mut CcxDecodersXdsContext,
    p: String,
    len: i64,
) -> Result<(), XdsError> {
    let new_screen = Eia608Screen {
        format: CcxEia608Format::SformatXds,
        start_time: TS_START_OF_XDS,
        end_time: (get_fts(&mut ctx.timing, CaptionField::Field2)).millis(),
        xds_str: p,
        xds_len: len,
        cur_xds_packet_class: ctx.cur_xds_packet_class,
        ..Default::default() // Use default values for the remaining fields
    };

    sub.data.push(new_screen);
    sub.datatype = SubDataType::Generic;
    sub.nb_data = (sub.data.len() + 1) as i64;
    sub.subtype = SubType::Cc608;
    sub.got_output = true;

    Ok(())
}
//----------------------------------------------------------------

// macro could be used, bad coz nothing is returned, could be an impl
pub fn xdsprint(sub: &mut CcSubtitle, ctx: &mut CcxDecodersXdsContext, args: std::fmt::Arguments) {
    if !ctx.xds_write_to_file {
        return;
    }

    let formatted = format!("{}", args);
    let formatted_len = formatted.len() as i64;
    write_xds_string(sub, ctx, formatted, formatted_len).unwrap(); // unhandled err variant
}

//----------------------------------------------------------------

pub fn process_xds_bytes(ctx: &mut CcxDecodersXdsContext, hi: i64, lo: i64) {
    if (0x01..=0x0f).contains(&hi) {
        let xds_class = (hi - 1) / 2;
        let is_new = hi % 2 != 0;

        let mut first_free_buf: Option<usize> = None;
        let mut matching_buf: Option<usize> = None;

        for (i, buf) in ctx.xds_buffers.iter_mut().enumerate() {
            if buf.in_use && buf.xds_class == xds_class && buf.xds_type == lo {
                matching_buf = Some(i);
                break;
            }
            if first_free_buf.is_none() && !buf.in_use {
                first_free_buf = Some(i);
            }
        }

        let buf_idx = match matching_buf.or(first_free_buf) {
            Some(idx) => idx,
            None => {
                println!("All XDS buffers full, ignoring.");
                ctx.cur_xds_buffer_idx = None;
                return;
            }
        };

        ctx.cur_xds_buffer_idx = Some(buf_idx as i64);

        let buf = &mut ctx.xds_buffers[buf_idx];

        if is_new || !buf.in_use {
            buf.xds_class = xds_class;
            buf.xds_type = lo;
            buf.used_bytes = 0;
            buf.in_use = true;
            buf.bytes = vec![0; NUM_BYTES_PER_PACKET];
        }

        if !is_new {
            return;
        }
    } else if (0x01..=0x1f).contains(&hi) || (0x01..=0x1f).contains(&lo) {
        println!("Illegal XDS data");
        return;
    }

    if let Some(buf_idx) = ctx.cur_xds_buffer_idx {
        let buf_idx = buf_idx as usize;
        let buf = &mut ctx.xds_buffers[buf_idx];
        if buf.used_bytes <= 32 && buf.used_bytes < ((buf.bytes.len() as i64) - 1) {
            buf.bytes[buf.used_bytes as usize] = hi;
            buf.used_bytes += 1;
            buf.bytes[buf.used_bytes as usize] = lo;
            buf.used_bytes += 1;
            buf.bytes[buf.used_bytes as usize] = 0;
        }
    }
}

//----------------------------------------------------------------

pub struct XdsCopyState {
    last_c1: Option<u8>,
    last_c2: Option<u8>,
    copy_permitted: String,
    aps: String,
    rcd: String,
}
impl XdsCopyState {
    fn new() -> Self {
        Self {
            last_c1: None,
            last_c2: None,
            copy_permitted: String::new(),
            aps: String::new(),
            rcd: String::new(),
        }
    }
}

impl Default for XdsCopyState {
    fn default() -> Self {
        Self::new()
    }
}

pub fn xds_do_copy_generation_management_system(
    sub: &mut CcSubtitle,
    ctx: &mut CcxDecodersXdsContext,
    c1: u8,
    c2: u8,
    state: &mut XdsCopyState,
) {
    let c1_6 = (c1 & 0x40) >> 6;
    let c2_6 = (c2 & 0x40) >> 6;

    if c1_6 == 0 || c2_6 == 0 {
        return;
    }

    let changed = state.last_c1 != Some(c1) || state.last_c2 != Some(c2);

    if changed {
        state.last_c1 = Some(c1);
        state.last_c2 = Some(c2);

        let copytext = [
            "Copy permitted (no restrictions)",
            "No more copies (one generation copy has been made)",
            "One generation of copies can be made",
            "No copying is permitted",
        ];

        let apstext = [
            "No APS",
            "PSP On; Split Burst Off",
            "PSP On; 2 line Split Burst On",
            "PSP On; 4 line Split Burst On",
        ];

        let cgms_index = ((c1 & 0x10) >> 3) | ((c1 & 0x08) >> 3);
        let aps_index = ((c1 & 0x04) >> 1) | ((c1 & 0x02) >> 1);
        let rcd0 = c2 & 0x01;

        state.copy_permitted = format!("CGMS: {}", copytext[cgms_index as usize]);
        state.aps = format!("APS: {}", apstext[aps_index as usize]);
        state.rcd = format!("Redistribution Control Descriptor: {}", rcd0);
    }

    xdsprint(sub, ctx, format_args!("{}", state.copy_permitted));
    xdsprint(sub, ctx, format_args!("{}", state.aps));
    xdsprint(sub, ctx, format_args!("{}", state.rcd));

    if changed {
        info!("{}", state.copy_permitted);
        info!("{}", state.aps);
        info!("{}", state.rcd);
    }

    debug!(msg_type = DebugMessageFlag::DECODER_XDS; "{}", state.copy_permitted);
    debug!(msg_type = DebugMessageFlag::DECODER_XDS; "{}", state.aps);
    debug!(msg_type = DebugMessageFlag::DECODER_XDS; "{}", state.rcd);
}

//----------------------------------------------------------------

struct Bits {
    // c1_6: bool,
    da2: bool,
    a1: bool,
    a0: bool,
    r2: bool,
    r1: bool,
    r0: bool,
    // c2_6: bool,
    fv: bool,
    s: bool,
    la3: bool,
    g2: bool,
    g1: bool,
    g0: bool,
}

fn decode_bits(byte: u8) -> Bits {
    Bits {
        // c1_6: byte & 0x40 != 0,
        da2: byte & 0x20 != 0,
        a1: byte & 0x10 != 0,
        a0: byte & 0x08 != 0,
        r2: byte & 0x04 != 0,
        r1: byte & 0x02 != 0,
        r0: byte & 0x01 != 0,
        // c2_6: byte & 0x40 != 0,
        fv: byte & 0x20 != 0,
        s: byte & 0x10 != 0,
        la3: byte & 0x08 != 0,
        g2: byte & 0x04 != 0,
        g1: byte & 0x02 != 0,
        g0: byte & 0x01 != 0,
    }
}

fn bits_to_number(b2: bool, b1: bool, b0: bool) -> usize {
    (b2 as usize) * 4 + (b1 as usize) * 2 + (b0 as usize)
}

pub fn xds_do_content_advisory(
    sub: &mut CcSubtitle,
    ctx: Option<&mut CcxDecodersXdsContext>,
    c1: u8,
    c2: u8,
    state: &mut XdsCopyState,
) {
    if ctx.is_none() {
        return;
    }
    let ctx = ctx.unwrap();

    if (c1 & 0x40) == 0 || (c2 & 0x40) == 0 {
        return;
    }

    let changed = state.last_c1 != Some(c1) || state.last_c2 != Some(c2);

    if changed {
        state.last_c1 = Some(c1);
        state.last_c2 = Some(c2);
    }

    let c1_bits = decode_bits(c1);
    let c2_bits = decode_bits(c2);

    let mut supported = false;
    let mut age_info = String::new();
    let mut content_info = String::new();
    let mut rating_info = String::new();

    if !c1_bits.a1 && c1_bits.a0 {
        // US TV parental guidelines
        let age_labels = [
            "None",
            "TV-Y (All Children)",
            "TV-Y7 (Older Children)",
            "TV-G (General Audience)",
            "TV-PG (Parental Guidance Suggested)",
            "TV-14 (Parents Strongly Cautioned)",
            "TV-MA (Mature Audience Only)",
            "None",
        ];
        let age_idx = bits_to_number(c2_bits.g2, c2_bits.g1, c2_bits.g0);
        age_info = format!(
            "ContentAdvisory: US TV Parental Guidelines. Age Rating: {}",
            age_labels.get(age_idx).unwrap_or(&"Unknown")
        );

        if !c2_bits.g2 && c2_bits.g1 && !c2_bits.g0 {
            if c2_bits.fv {
                content_info.push_str("[Fantasy Violence] ");
            }
        } else if c2_bits.fv {
            content_info.push_str("[Violence] ");
        }

        if c2_bits.s {
            content_info.push_str("[Sexual Situations] ");
        }

        if c2_bits.la3 {
            content_info.push_str("[Adult Language] ");
        }

        if c1_bits.da2 {
            content_info.push_str("[Sexually Suggestive Dialog] ");
        }

        supported = true;
    }

    if !c1_bits.a0 {
        // MPA rating
        let rating_labels = ["N/A", "G", "PG", "PG-13", "R", "NC-17", "X", "Not Rated"];
        let rating_idx = bits_to_number(c1_bits.r2, c1_bits.r1, c1_bits.r0);
        rating_info = format!(
            "ContentAdvisory: MPA Rating: {}",
            rating_labels.get(rating_idx).unwrap_or(&"Unknown")
        );

        supported = true;
    }

    if c1_bits.a0 && c1_bits.a1 && !c1_bits.da2 && !c2_bits.la3 {
        // Canadian English rating
        let rating_labels = [
            "Exempt",
            "Children",
            "Children eight years and older",
            "General programming suitable for all audiences",
            "Parental Guidance",
            "Viewers 14 years and older",
            "Adult Programming",
            "[undefined]",
        ];
        let rating_idx = bits_to_number(c2_bits.g2, c2_bits.g1, c2_bits.g0);
        rating_info = format!(
            "ContentAdvisory: Canadian English Rating: {}",
            rating_labels.get(rating_idx).unwrap_or(&"Unknown")
        );

        supported = true;
    }

    if c1_bits.a0 && c1_bits.a1 && c1_bits.da2 && !c2_bits.la3 {
        // Canadian French rating
        let rating_labels = [
            "Exemptées",
            "Général",
            "Général - Déconseillé aux jeunes enfants",
            "Cette émission peut ne pas convenir aux enfants de moins de 13 ans",
            "Cette émission ne convient pas aux moins de 16 ans",
            "Cette émission est réservée aux adultes",
            "[invalid]",
            "[invalid]",
        ];
        let rating_idx = bits_to_number(c2_bits.g2, c2_bits.g1, c2_bits.g0);
        rating_info = format!(
            "ContentAdvisory: Canadian French Rating: {}",
            rating_labels.get(rating_idx).unwrap_or(&"Unknown")
        );

        supported = true;
    }

    if !c1_bits.a1 && c1_bits.a0 {
        // US TV

        xdsprint(sub, ctx, format_args!("{}", age_info));
        xdsprint(sub, ctx, format_args!("{}", content_info));
        if changed {
            info!("{}", age_info);
            info!("{}", content_info);
        }
        debug!(msg_type = DebugMessageFlag::DECODER_XDS; "{}", age_info);
        debug!(msg_type = DebugMessageFlag::DECODER_XDS; "{}", content_info);
    }

    if !c1_bits.a0 || (c1_bits.a1 && !c1_bits.da2 && !c2_bits.la3) {
        xdsprint(sub, ctx, format_args!("{}", rating_info));
        if changed {
            info!("{}", rating_info);
        }
        debug!(msg_type = DebugMessageFlag::DECODER_XDS; "{}", rating_info);
    }

    if changed && !supported {
        info!("XDS: Unsupported ContentAdvisory encoding, please submit sample.");
    }
}

//----------------------------------------------------------------

use std::fmt::Write;

pub fn xds_do_current_and_future(
    sub: &mut CcSubtitle,
    ctx: &mut CcxDecodersXdsContext,
) -> Result<bool, &'static str> {
    let mut was_proc = false;
    let mut str_buf = String::with_capacity(1024);

    match ctx.cur_xds_packet_type {
        XDS_TYPE_PIN_START_TIME => {
            was_proc = true;
            if ctx.cur_xds_payload_length < 7 {
                return Ok(was_proc);
            }

            let min = ctx.cur_xds_payload[2] & 0x3f;
            let hour = ctx.cur_xds_payload[3] & 0x1f;
            let date = ctx.cur_xds_payload[4] & 0x1f;
            let month = ctx.cur_xds_payload[5] & 0x0f;

            if ctx.current_xds_min != min.into()
                || ctx.current_xds_hour != hour.into()
                || ctx.current_xds_date != date.into()
                || ctx.current_xds_month != month.into()
            {
                ctx.xds_start_time_shown = 0;
                ctx.current_xds_min = min.into();
                ctx.current_xds_hour = hour.into();
                ctx.current_xds_date = date.into();
                ctx.current_xds_month = month.into();
            }

            debug!(
                msg_type = DebugMessageFlag::DECODER_XDS;
                "PIN (Start Time): {}  {:02}-{:02} {:02}:{:02}",
                if ctx.cur_xds_packet_class == XDS_CLASS_CURRENT {
                    "Current"
                } else {
                    "Future"
                },
                date,
                month,
                hour,
                min
            );

            xdsprint(
                sub,
                ctx,
                format_args!(
                    "PIN (Start Time): {}  {:02}-{:02} {:02}:{:02}",
                    if ctx.cur_xds_packet_class == XDS_CLASS_CURRENT {
                        "Current"
                    } else {
                        "Future"
                    },
                    date,
                    month,
                    hour,
                    min
                ),
            );

            if ctx.xds_start_time_shown == 0 && ctx.cur_xds_packet_class == XDS_CLASS_CURRENT {
                info!("XDS: Program changed.");
                info!(
                    "XDS program start time (DD/MM HH:MM) {:02}-{:02} {:02}:{:02}",
                    date, month, hour, min
                );
                // GUI update would go here
                ctx.xds_start_time_shown = 1;
            }
        }
        XDS_TYPE_LENGTH_AND_CURRENT_TIME => {
            was_proc = true;
            if ctx.cur_xds_payload_length < 5 {
                return Ok(was_proc);
            }

            let min = ctx.cur_xds_payload[2] & 0x3f;
            let hour = ctx.cur_xds_payload[3] & 0x1f;

            if ctx.xds_program_length_shown == 0 {
                info!("XDS: Program length (HH:MM): {:02}:{:02}  ", hour, min);
            } else {
                debug!(
                    msg_type = DebugMessageFlag::DECODER_XDS;
                    "XDS: Program length (HH:MM): {:02}:{:02}  ",
                    hour,
                    min
                );
            }

            xdsprint(
                sub,
                ctx,
                format_args!("Program length (HH:MM): {:02}:{:02}  ", hour, min),
            );

            if ctx.cur_xds_payload_length > 6 {
                let el_min = ctx.cur_xds_payload[4] & 0x3f;
                let el_hour = ctx.cur_xds_payload[5] & 0x1f;

                if ctx.xds_program_length_shown == 0 {
                    info!("Elapsed (HH:MM): {:02}:{:02}", el_hour, el_min);
                } else {
                    debug!(
                        msg_type = DebugMessageFlag::DECODER_XDS;
                        "Elapsed (HH:MM): {:02}:{:02}",
                        el_hour,
                        el_min
                    );
                }
                xdsprint(
                    sub,
                    ctx,
                    format_args!("Elapsed (HH:MM): {:02}:{:02}", el_hour, el_min),
                );
            }

            if ctx.cur_xds_payload_length > 8 {
                let el_sec = ctx.cur_xds_payload[6] & 0x3f;
                if ctx.xds_program_length_shown == 0 {
                    debug!(msg_type = DebugMessageFlag::DECODER_XDS; ":{:02}", el_sec);
                }
                xdsprint(sub, ctx, format_args!("Elapsed (SS) :{:02}", el_sec));
            }

            if ctx.xds_program_length_shown == 0 {
                info!("");
            } else {
                debug!(msg_type = DebugMessageFlag::DECODER_XDS; "");
            }
            ctx.xds_program_length_shown = 1;
        }
        XDS_TYPE_PROGRAM_NAME => {
            was_proc = true;
            let mut xds_program_name = String::new();
            for i in 2..ctx.cur_xds_payload_length - 1 {
                xds_program_name.push(ctx.cur_xds_payload[i as usize] as char);
            }

            debug!(
                msg_type = DebugMessageFlag::DECODER_XDS;
                "XDS Program name: {}",
                xds_program_name
            );
            xdsprint(sub, ctx, format_args!("Program name: {}", xds_program_name));

            if ctx.cur_xds_packet_class == XDS_CLASS_CURRENT
                && ctx.current_xds_program_name != xds_program_name
            {
                info!("XDS Notice: Program is now {}", xds_program_name);
                ctx.current_xds_program_name = xds_program_name;
                // GUI update would go here
            }
        }
        XDS_TYPE_PROGRAM_TYPE => {
            was_proc = true;
            if ctx.cur_xds_payload_length < 5 {
                return Ok(was_proc);
            }

            if ctx.current_program_type_reported != 0 {
                for i in 0..ctx.cur_xds_payload_length {
                    if ctx.cur_xds_payload[i as usize]
                        != ctx.current_xds_program_type.as_bytes()[i as usize]
                    {
                        ctx.current_program_type_reported = 0;
                        break;
                    }
                }
            }

            ctx.current_xds_program_type = String::from_utf8_lossy(
                &ctx.cur_xds_payload[..ctx.cur_xds_payload_length as usize],
            )
            .into_owned();

            if ctx.current_program_type_reported == 0 {
                info!("XDS Program Type: ");
            }

            str_buf.clear();
            for i in 2..ctx.cur_xds_payload_length - 1 {
                if ctx.cur_xds_payload[i as usize] == 0 {
                    continue;
                }

                if ctx.current_program_type_reported == 0 {
                    info!("[{:02X}-", ctx.cur_xds_payload[i as usize]);
                }

                if ctx.cur_xds_payload[i as usize] >= 0x20 && ctx.cur_xds_payload[i as usize] < 0x7F
                {
                    let type_str =
                        XDS_PROGRAM_TYPES[(ctx.cur_xds_payload[i as usize] - 0x20) as usize];
                    write!(str_buf, "[{}] ", type_str).unwrap();
                }

                if ctx.current_program_type_reported == 0 {
                    if ctx.cur_xds_payload[i as usize] >= 0x20
                        && ctx.cur_xds_payload[i as usize] < 0x7F
                    {
                        info!(
                            "{}",
                            XDS_PROGRAM_TYPES[(ctx.cur_xds_payload[i as usize] - 0x20) as usize]
                        );
                    } else {
                        info!("ILLEGAL VALUE");
                    }
                    info!("] ");
                }
            }

            xdsprint(sub, ctx, format_args!("Program type {}", str_buf));
            if ctx.current_program_type_reported == 0 {
                info!("");
            }
            ctx.current_program_type_reported = 1;
        }
        XDS_TYPE_CONTENT_ADVISORY => {
            was_proc = true;
            if ctx.cur_xds_payload_length < 5 {
                return Ok(was_proc);
            }
            let xds_do_content_advisory_c1: u8 = ctx.cur_xds_payload[2];
            let xds_do_content_advisory_c2: u8 = ctx.cur_xds_payload[3];
            let mut state = XdsCopyState::new();
            xds_do_content_advisory(
                sub,
                Some(ctx),
                xds_do_content_advisory_c1,
                xds_do_content_advisory_c2,
                &mut state,
            );
        }
        XDS_TYPE_AUDIO_SERVICES => {
            was_proc = true;
        }
        XDS_TYPE_CGMS => {
            was_proc = true;
            let mut state = XdsCopyState::new();
            xds_do_copy_generation_management_system(
                sub,
                ctx,
                ctx.cur_xds_payload[2],
                ctx.cur_xds_payload[3],
                &mut state,
            );
        }
        XDS_TYPE_ASPECT_RATIO_INFO => {
            was_proc = true;
            if ctx.cur_xds_payload_length < 5 {
                return Ok(was_proc);
            }
            if (ctx.cur_xds_payload[2] & 0x20) == 0 || (ctx.cur_xds_payload[3] & 0x20) == 0 {
                return Ok(was_proc);
            }

            let ar_start = (ctx.cur_xds_payload[2] & 0x1F) as u32 + 22;
            let ar_end = 262 - (ctx.cur_xds_payload[3] & 0x1F) as u32;
            let active_picture_height = ar_end - ar_start;
            let aspect_ratio = 320.0 / active_picture_height as f32;

            if ar_start != ctx.current_ar_start as u32 {
                ctx.current_ar_start = ar_start as i64;
                ctx.current_ar_end = ar_end as i64;
                info!(
                    "XDS Notice: Aspect ratio info, start line={}, end line={}",
                    ar_start, ar_end
                );
                info!(
                    "XDS Notice: Aspect ratio info, active picture height={}, ratio={}",
                    active_picture_height, aspect_ratio
                );
            } else {
                debug!(
                    msg_type = DebugMessageFlag::DECODER_XDS;
                    "XDS Notice: Aspect ratio info, start line={}, end line={}",
                    ar_start,
                    ar_end
                );
                debug!(
                    msg_type = DebugMessageFlag::DECODER_XDS;
                    "XDS Notice: Aspect ratio info, active picture height={}, ratio={}",
                    active_picture_height,
                    aspect_ratio
                );
            }
        }
        XDS_TYPE_PROGRAM_DESC_1..=XDS_TYPE_PROGRAM_DESC_8 => {
            was_proc = true;
            let mut xds_desc = String::new();
            for i in 2..ctx.cur_xds_payload_length - 1 {
                xds_desc.push(ctx.cur_xds_payload[i as usize] as char);
            }

            if !xds_desc.is_empty() {
                let line_num = (ctx.cur_xds_packet_type - XDS_TYPE_PROGRAM_DESC_1) as usize;
                let changed = ctx.xds_program_description[line_num] != xds_desc;

                if changed {
                    info!("XDS description line {}: {}", line_num, xds_desc);
                    ctx.xds_program_description[line_num] = xds_desc.clone();
                } else {
                    debug!(
                        msg_type = DebugMessageFlag::DECODER_XDS;
                        "XDS description line {}: {}",
                        line_num,
                        xds_desc
                    );
                }
                xdsprint(
                    sub,
                    ctx,
                    format_args!("XDS description line {}: {}", line_num, xds_desc),
                );
                // at end, to-do : GUI Update
            }
        }
        _ => {}
    }

    Ok(was_proc)
}

pub const XDS_PROGRAM_TYPES: [&str; 96] = [
    "Education",
    "Entertainment",
    "Movie",
    "News",
    "Religious",
    "Sports",
    "Other",
    "Action",
    "Advertisement",
    "Animated",
    "Anthology",
    "Automobile",
    "Awards",
    "Baseball",
    "Basketball",
    "Bulletin",
    "Business",
    "Classical",
    "College",
    "Combat",
    "Comedy",
    "Commentary",
    "Concert",
    "Consumer",
    "Contemporary",
    "Crime",
    "Dance",
    "Documentary",
    "Drama",
    "Elementary",
    "Erotica",
    "Exercise",
    "Fantasy",
    "Farm",
    "Fashion",
    "Fiction",
    "Food",
    "Football",
    "Foreign",
    "Fund-Raiser",
    "Game/Quiz",
    "Garden",
    "Golf",
    "Government",
    "Health",
    "High_School",
    "History",
    "Hobby",
    "Hockey",
    "Home",
    "Horror",
    "Information",
    "Instruction",
    "International",
    "Interview",
    "Language",
    "Legal",
    "Live",
    "Local",
    "Math",
    "Medical",
    "Meeting",
    "Military",
    "Mini-Series",
    "Music",
    "Mystery",
    "National",
    "Nature",
    "Police",
    "Politics",
    "Premiere",
    "Pre-Recorded",
    "Product",
    "Professional",
    "Public",
    "Racing",
    "Reading",
    "Repair",
    "Repeat",
    "Review",
    "Romance",
    "Science",
    "Series",
    "Service",
    "Shopping",
    "Soap_Opera",
    "Special",
    "Suspense",
    "Talk",
    "Technical",
    "Tennis",
    "Travel",
    "Variety",
    "Video",
    "Weather",
    "Western",
];

//----------------------------------------------------------------

pub fn xds_do_channel(
    sub: &mut CcSubtitle,
    ctx: &mut CcxDecodersXdsContext,
) -> Result<bool, &'static str> {
    let mut was_proc = false;

    match ctx.cur_xds_packet_type {
        XDS_TYPE_NETWORK_NAME => {
            was_proc = true;
            let mut xds_network_name = String::new();
            for i in 2..ctx.cur_xds_payload_length - 1 {
                xds_network_name.push(ctx.cur_xds_payload[i as usize] as char);
            }

            debug!(
                msg_type = DebugMessageFlag::DECODER_XDS;
                "XDS Network name: {}",
                xds_network_name
            );
            xdsprint(sub, ctx, format_args!("Network: {}", xds_network_name));

            if ctx.current_xds_network_name != xds_network_name {
                info!("XDS Notice: Network is now {}", xds_network_name);
                ctx.current_xds_network_name = xds_network_name;
            }
        }
        XDS_TYPE_CALL_LETTERS_AND_CHANNEL => {
            was_proc = true;
            if ctx.cur_xds_payload_length != 7 && ctx.cur_xds_payload_length != 9 {
                return Ok(was_proc);
            }

            let mut xds_call_letters = String::new();
            for i in 2..ctx.cur_xds_payload_length - 1 {
                xds_call_letters.push(ctx.cur_xds_payload[i as usize] as char);
            }

            debug!(
                msg_type = DebugMessageFlag::DECODER_XDS;
                "XDS Network call letters: {}",
                xds_call_letters
            );
            xdsprint(sub, ctx, format_args!("Call Letters: {}", xds_call_letters));

            if ctx.current_xds_call_letters != xds_call_letters {
                info!("XDS Notice: Network call letters now {}", xds_call_letters);
                ctx.current_xds_call_letters = xds_call_letters;
                // GUI update would go here
            }
        }
        XDS_TYPE_TSID => {
            was_proc = true;
            if ctx.cur_xds_payload_length < 7 {
                return Ok(was_proc);
            }

            let b1 = (ctx.cur_xds_payload[2] & 0x0F) as u32;
            let b2 = (ctx.cur_xds_payload[3] & 0x0F) as u32;
            let b3 = (ctx.cur_xds_payload[4] & 0x0F) as u32;
            let b4 = (ctx.cur_xds_payload[5] & 0x0F) as u32;
            let tsid = (b4 << 12) | (b3 << 8) | (b2 << 4) | b1;

            if tsid != 0 {
                xdsprint(sub, ctx, format_args!("TSID: {}", tsid));
            }
        }
        _ => {}
    }

    Ok(was_proc)
}

//----------------------------------------------------------------

pub fn xds_do_private_data(
    sub: &mut CcSubtitle,
    ctx: &mut CcxDecodersXdsContext,
) -> Result<bool, &'static str> {
    if ctx.cur_xds_payload_length < 3 {
        return Ok(false);
    }

    let mut hex_dump = String::with_capacity((ctx.cur_xds_payload_length as usize) * 3);

    for i in 2..ctx.cur_xds_payload_length - 1 {
        write!(hex_dump, "{:02X} ", ctx.cur_xds_payload[i as usize])
            .map_err(|_| "Failed to format hex string")?;
    }

    xdsprint(sub, ctx, format_args!("{}", hex_dump));
    Ok(true)
}

//----------------------------------------------------------------

pub fn xds_do_misc(ctx: &mut CcxDecodersXdsContext) -> Result<bool, &'static str> {
    let mut was_proc = false;

    match ctx.cur_xds_packet_type {
        XDS_TYPE_TIME_OF_DAY => {
            was_proc = true;
            if ctx.cur_xds_payload_length < 9 {
                return Ok(was_proc);
            }

            let min = ctx.cur_xds_payload[2] & 0x3f;
            let hour = ctx.cur_xds_payload[3] & 0x1f;
            let date = ctx.cur_xds_payload[4] & 0x1f;
            let month = ctx.cur_xds_payload[5] & 0x0f;
            let reset_seconds = (ctx.cur_xds_payload[5] & 0x20) != 0;
            let day_of_week = ctx.cur_xds_payload[6] & 0x07;
            let year = ((ctx.cur_xds_payload[7] as u16) & 0x3f) + 1990;

            debug!(
                msg_type = DebugMessageFlag::DECODER_XDS;
                "Time of day: (YYYY/MM/DD) {:04}/{:02}/{:02} (HH:MM) {:02}:{:02} DoW: {} Reset seconds: {}",
                year, month, date, hour, min, day_of_week, reset_seconds
            );
        }
        XDS_TYPE_LOCAL_TIME_ZONE => {
            was_proc = true;
            if ctx.cur_xds_payload_length < 5 {
                return Ok(was_proc);
            }

            let dst = (ctx.cur_xds_payload[2] & 0x20) != 0;
            let hour = ctx.cur_xds_payload[2] & 0x1f;

            debug!(
                msg_type = DebugMessageFlag::DECODER_XDS;
                "Local Time Zone: {:02} DST: {}",
                hour, dst
            );
        }
        _ => {}
    }

    Ok(was_proc)
}

//----------------------------------------------------------------

pub fn do_end_of_xds(sub: &mut CcSubtitle, ctx: &mut CcxDecodersXdsContext, expected_checksum: u8) {
    if ctx.cur_xds_buffer_idx.is_none() {
        return;
    }
    let buffer_idx = ctx.cur_xds_buffer_idx.unwrap();

    if buffer_idx < 0
        || buffer_idx as usize >= ctx.xds_buffers.len()
        || !ctx.xds_buffers[buffer_idx as usize].in_use
    {
        return;
    }

    let buffer = &ctx.xds_buffers[buffer_idx as usize];
    ctx.cur_xds_packet_class = buffer.xds_class;
    ctx.cur_xds_payload = buffer.bytes.iter().map(|&x| x as u8).collect();
    ctx.cur_xds_payload_length = buffer.used_bytes;
    ctx.cur_xds_packet_type = buffer.bytes[1];

    ctx.cur_xds_payload.push(0x0F);
    ctx.cur_xds_payload_length += 1;

    let mut cs = 0;
    for i in 0..ctx.cur_xds_payload_length as usize {
        let byte = ctx.cur_xds_payload[i];
        cs = (cs + byte) & 0x7f;
        let c = byte & 0x7F;
        debug!(
            msg_type = DebugMessageFlag::DECODER_XDS;
            "{:02X} - {} cs: {:02X}",
            c,
            if c >= 0x20 { c as char } else { '?' },
            cs
        );
    }
    cs = (128 - cs) & 0x7F;

    debug!(
        msg_type = DebugMessageFlag::DECODER_XDS;
        "End of XDS. Class={} ({}), size={} Checksum OK: {} Used buffers: {}",
        ctx.cur_xds_packet_class,
        match ctx.cur_xds_packet_class {
            0 => "Current",
            1 => "Future",
            2 => "Channel",
            3 => "Misc",
            4 => "Private",
            5 => "Out-of-band",
            _ => "Unknown",
        },
        ctx.cur_xds_payload_length,
        cs == expected_checksum,
        ctx.how_many_used()
    );

    if cs != expected_checksum || ctx.cur_xds_payload_length < 3 {
        debug!(
            msg_type = DebugMessageFlag::DECODER_XDS;
            "Expected checksum: {:02X} Calculated: {:02X}",
            expected_checksum,
            cs
        );
        ctx.clear_xds_buffer(buffer_idx as usize)
            .unwrap_or_default();
        return;
    }

    if (ctx.cur_xds_packet_type & 0x40) != 0 {
        ctx.cur_xds_packet_class = XDS_CLASS_OUT_OF_BAND;
    }

    let was_proc = match ctx.cur_xds_packet_class {
        XDS_CLASS_FUTURE
            if !matches!(DebugMessageFlag::DECODER_XDS, DebugMessageFlag::DECODER_XDS) =>
        {
            true
        }
        XDS_CLASS_FUTURE | XDS_CLASS_CURRENT => {
            xds_do_current_and_future(sub, ctx).unwrap_or(false)
        }
        XDS_CLASS_CHANNEL => xds_do_channel(sub, ctx).unwrap_or(false),
        XDS_CLASS_MISC => xds_do_misc(ctx).unwrap_or(false),
        XDS_CLASS_PRIVATE => xds_do_private_data(sub, ctx).unwrap_or(false),
        XDS_CLASS_OUT_OF_BAND => {
            debug!(msg_type = DebugMessageFlag::DECODER_XDS; "Out-of-band data, ignored.");
            true
        }
        _ => false,
    };

    if !was_proc {
        info!("Note: We found a currently unsupported XDS packet.");
        //to-do : how to dump?
    }

    ctx.clear_xds_buffer(buffer_idx as usize)
        .unwrap_or_default();
}

//----------------------------------------------------------------

pub fn xds_debug_test(ctx: &mut CcxDecodersXdsContext, sub: &mut CcSubtitle) {
    process_xds_bytes(ctx, 0x05, 0x02);
    process_xds_bytes(ctx, 0x20, 0x20);
    do_end_of_xds(sub, ctx, 0x2a);
}

pub fn xds_cea608_test(ctx: &mut CcxDecodersXdsContext, sub: &mut CcSubtitle) {
    process_xds_bytes(ctx, 0x01, 0x03);
    process_xds_bytes(ctx, 0x53, 0x74);
    process_xds_bytes(ctx, 0x61, 0x72);
    process_xds_bytes(ctx, 0x20, 0x54);
    process_xds_bytes(ctx, 0x72, 0x65);
    process_xds_bytes(ctx, 0x02, 0x03);
    process_xds_bytes(ctx, 0x02, 0x03);
    process_xds_bytes(ctx, 0x6b, 0x00);
    do_end_of_xds(sub, ctx, 0x1d);
}
