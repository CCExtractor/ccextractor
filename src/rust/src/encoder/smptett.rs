use crate::bindings::{cc_subtitle, eia608_screen, encoder_ctx};
use crate::encoder::common::encode_line;
use crate::libccxr_exports::time::ccxr_millis_to_time;

use std::os::raw::c_int;

fn escape_xml(s: &str) -> String {
    let mut out = String::with_capacity(s.len());
    for c in s.chars() {
        match c {
            '&' => out.push_str("&amp;"),
            '<' => out.push_str("&lt;"),
            '>' => out.push_str("&gt;"),
            '"' => out.push_str("&quot;"),
            '\'' => out.push_str("&apos;"),
            _ => out.push(c),
        }
    }
    out
}

fn write_stringz_as_smptett(
    text: &str,
    context: &mut encoder_ctx,
    ms_start: i64,
    ms_end: i64,
){
    let (mut h1, mut m1, mut s1, mut ms1) = (0, 0, 0, 0);
    let (mut h2, mut m2, mut s2, mut ms2) = (0, 0, 0, 0);

    unsafe {
        ccxr_millis_to_time(ms_start, &mut h1, &mut m1, &mut s1, &mut ms1);
        ccxr_millis_to_time(ms_end, &mut h2, &mut m2, &mut s2, &mut ms2);
    }

    // Convert raw pointer buffer → Rust slice
    let buffer: &mut [u8] = unsafe {
        std::slice::from_raw_parts_mut(context.buffer, context.capacity as usize)
    };

    // Copy CRLF slice so we don't immutably borrow from context
    let crlf = context.encoded_crlf;

    let header = format!(
        "<p begin=\"{:02}:{:02}:{:02}.{:03}\" end=\"{:02}:{:02}:{:02}.{:03}\">",
        h1, m1, s1, ms1,
        h2, m2, s2, ms2
    );

    encode_line(context, buffer, header.as_bytes());
    encode_line(context, buffer, &crlf);

    for line in text.split('\n') {
        let escaped = escape_xml(line);
        encode_line(context, buffer, escaped.as_bytes());
        encode_line(context, buffer, &crlf);
    }

    encode_line(context, buffer, b"</p>");
    encode_line(context, buffer, &crlf);
}



pub fn write_cc_buffer_as_smptett(
    _: &mut encoder_ctx,
    _: &eia608_screen,
    _: i64,
    _: i64,
) -> c_int {
    unimplemented!()
}

pub fn write_cc_subtitle_as_smptett(_: &mut encoder_ctx, _: &cc_subtitle) -> c_int {
    unimplemented!()
}

pub fn write_cc_bitmap_as_smptett(
    _: &mut encoder_ctx,
    _: &[u8],
    _: usize,
    _: usize,
) -> c_int {
    unimplemented!()
}

#[cfg(test)]
mod tests {
    use super::escape_xml;

    #[test]
    fn test_smptett_escape_basic() {
        assert_eq!(escape_xml("a & b < c"), "a &amp; b &lt; c");
        assert_eq!(
            escape_xml("\"quote\" 'apos' & < >"),
            "&quot;quote&quot; &apos;apos&apos; &amp; &lt; &gt;"
        );
    }
}
