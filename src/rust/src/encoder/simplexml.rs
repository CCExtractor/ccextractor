use crate::bindings::{cc_subtitle, ccx_encoding_type_CCX_ENC_ASCII, eia608_screen, encoder_ctx};
use crate::encoder::ccxr_get_str_basic;
use crate::encoder::common::write_raw;
use crate::ffi_alloc;
use lib_ccxr::common::CCX_DECODER_608_SCREEN_WIDTH;
use lib_ccxr::info;
use std::os::raw::{c_int, c_void};
use std::ptr;

pub fn write_cc_line_as_simplexml(
    data: &mut eia608_screen,
    context: &mut encoder_ctx,
    line_number: usize,
) {
    let cap = b"<caption>";
    let cap1 = b"</caption>";

    let length = unsafe {
        ccxr_get_str_basic(
            // change to get_str_basic after encoder_ctx is replaced with EncoderCtx
            context.subline,
            data.characters[line_number].as_mut_ptr(),
            context.trim_subs,
            ccx_encoding_type_CCX_ENC_ASCII,
            context.encoding,
            CCX_DECODER_608_SCREEN_WIDTH as i32,
        )
    };

    // Write opening caption tag
    let _ret = unsafe { write_raw((*context.out).fh, cap.as_ptr() as *const c_void, cap.len()) };

    // Write the subtitle content
    let ret = unsafe {
        write_raw(
            (*context.out).fh,
            context.subline as *const c_void,
            length as usize,
        )
    };

    if ret < length as isize {
        info!("Warning:Loss of data\n");
    }

    // Write closing caption tag
    let _ret = unsafe {
        write_raw(
            (*context.out).fh,
            cap1.as_ptr() as *const c_void,
            cap1.len(),
        )
    };

    // Write CRLF
    let _ret = unsafe {
        write_raw(
            (*context.out).fh,
            context.encoded_crlf.as_ptr() as *const c_void,
            context.encoded_crlf_length as usize,
        )
    };
}

pub fn write_cc_buffer_as_simplexml(data: &mut eia608_screen, context: &mut encoder_ctx) -> c_int {
    let mut wrote_something = 0;

    for i in 0..15 {
        if data.row_used[i] != 0 {
            write_cc_line_as_simplexml(data, context, i);
            wrote_something = 1;
        }
    }

    wrote_something
}

pub fn write_cc_bitmap_as_simplexml(sub: &mut cc_subtitle, _context: &mut encoder_ctx) -> c_int {
    sub.nb_data = 0;

    if !sub.data.is_null() {
        unsafe {
            ffi_alloc::c_free(sub.data);
        }
        sub.data = ptr::null_mut();
    }

    0
}
