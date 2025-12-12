use crate::bindings::{cc_subtitle, eia608_screen};
use crate::encoder_ctx;
use crate::encoder::common::encode_line;
use crate::libccxr_exports::time::ccxr_millis_to_time;

use std::os::raw::c_int;



fn write_stringz_as_smptett(
    text: &str,
    context: &mut encoder_ctx,
    ms_start: i64,
    ms_end: i64,
) {
    let (mut h1, mut m1, mut s1, mut ms1) = (0, 0, 0, 0);
    let (mut h2, mut m2, mut s2, mut ms2) = (0, 0, 0, 0);

    unsafe {
        ccxr_millis_to_time(ms_start, &mut h1, &mut m1, &mut s1, &mut ms1);
        ccxr_millis_to_time(ms_end,  &mut h2, &mut m2, &mut s2, &mut ms2);
    }

    let header = format!(
        "<p begin=\"{:02}:{:02}:{:02}.{:03}\" end=\"{:02}:{:02}:{:02}.{:03}\">",
        h1, m1, s1, ms1,
        h2, m2, s2, ms2
    );

    // Turn raw buffer ptr into slice
    let buffer = unsafe {
        std::slice::from_raw_parts_mut(context.buffer , context.capacity as usize)
    };

    encode_line(context, buffer, header.as_bytes());

    for line in text.split('\n') {
    let buffer = unsafe {
        std::slice::from_raw_parts_mut(context.buffer, context.capacity as usize)
    };
    encode_line(context, buffer, line.as_bytes());

    let crlf = context.encoded_crlf;

    let buffer = unsafe {
        std::slice::from_raw_parts_mut(context.buffer, context.capacity as usize)
    };
    encode_line(context, buffer, &crlf);
}

    let buffer = unsafe {
        std::slice::from_raw_parts_mut(context.buffer, context.capacity as usize)
    };
    encode_line(context, buffer, b"</p>");
}

