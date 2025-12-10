use crate::bindings::{
    cc_subtitle,
    eia608_screen,
    encoder_ctx,
};

use crate::encoder::common::encode_line;
use crate::libccxr_exports::time::ccxr_millis_to_time;

use std::os::raw::c_int;

/// Core helper: write a SMPTE-TT paragraph from text + times
fn write_stringz_as_smptett(
    text: &str,
    context: &mut encoder_ctx,
    ms_start: i64,
    ms_end: i64,
) {
    let mut h1 = 0;
    let mut m1 = 0;
    let mut s1 = 0;
    let mut ms1 = 0;

    let mut h2 = 0;
    let mut m2 = 0;
    let mut s2 = 0;
    let mut ms2 = 0;

    unsafe {
        ccxr_millis_to_time(ms_start, &mut h1, &mut m1, &mut s1, &mut ms1);
        ccxr_millis_to_time(ms_end, &mut h2, &mut m2, &mut s2, &mut ms2);
    }

    let header = format!(
        "<p begin=\"{:02}:{:02}:{:02}.{:03}\" end=\"{:02}:{:02}:{:02}.{:03}\">",
        h1, m1, s1, ms1,
        h2, m2, s2, ms2
    );

    encode_line(context, &mut context.buffer, header.as_bytes());

        encode_line(context, &mut context.buffer, line.as_bytes());
    encode_line(context, &mut context.buffer, &context.encoded_crlf);
}
for line in text.split('\n') {
    encode_line(context, &mut context.buffer, line.as_bytes());
    encode_line(context, &mut context.buffer, &context.encoded_crlf[..]);
}

    encode_line(context, &mut context.buffer, b"</p>");
}

/// SMPTE-TT encoder entry for CC buffer (CEA-608)
pub fn write_cc_buffer_as_smptett(
    _data: &mut eia608_screen,
    _context: &mut encoder_ctx,
) -> c_int {
    0
}

/// SMPTE-TT encoder entry for subtitle list (text-based)
pub fn write_cc_subtitle_as_smptett(
    _sub: &mut cc_subtitle,
    _context: &mut encoder_ctx,
) -> c_int {
    0
}

/// SMPTE-TT encoder entry for bitmap subtitles (OCR)
pub fn write_cc_bitmap_as_smptett(
    _sub: &mut cc_subtitle,
    _context: &mut encoder_ctx,
) -> c_int {
    0
}
