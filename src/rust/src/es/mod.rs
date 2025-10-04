use crate::bindings::{bitstream, cc_subtitle, encoder_ctx, lib_cc_decode};
use crate::ccx_options;
use crate::es::eau::read_eau_info;
use crate::es::gop::read_gop_info;
use crate::es::pic::{read_pic_data, read_pic_info};
use crate::es::seq::read_seq_info;
use crate::libccxr_exports::bitstream::{copy_bitstream_c_to_rust, copy_bitstream_from_rust_to_c};
use lib_ccxr::common::Options;

pub mod core;
pub mod eau;
pub mod gop;
pub mod pic;
pub mod seq;
pub mod userdata;
/// # Safety
/// This function is unsafe because it copies from C to Rust and back.
#[no_mangle]
pub unsafe extern "C" fn ccxr_next_start_code(esstream: *mut bitstream) -> u8 {
    let mut bs = copy_bitstream_c_to_rust(esstream);
    let result = bs.next_start_code().unwrap_or(0);
    copy_bitstream_from_rust_to_c(esstream, &bs);
    result
}
/// # Safety
/// This function is unsafe because it copies from C to Rust and back.
#[no_mangle]
pub unsafe extern "C" fn ccxr_search_start_code(esstream: *mut bitstream) -> u8 {
    let mut bs = copy_bitstream_c_to_rust(esstream);
    let result = bs.search_start_code().unwrap_or(0);
    copy_bitstream_from_rust_to_c(esstream, &bs);
    result
}
/// # Safety
/// This function is unsafe because it copies from C to Rust and back.
#[no_mangle]
pub unsafe extern "C" fn ccxr_read_seq_info(
    ctx: *mut lib_cc_decode,
    esstream: *mut bitstream,
) -> i32 {
    let mut bs = copy_bitstream_c_to_rust(esstream);
    let mut CcxOptions = Options {
        gui_mode_reports: ccx_options.gui_mode_reports != 0,
        ..Default::default()
    };
    let result = read_seq_info(&mut *ctx, &mut bs, &mut CcxOptions).unwrap_or(false);
    copy_bitstream_from_rust_to_c(esstream, &bs);
    i32::from(result)
}
/// # Safety
/// This function is unsafe because it copies from C to Rust and back and calls `read_gop_info` which is also unsafe.
#[no_mangle]
pub unsafe extern "C" fn ccxr_read_gop_info(
    enc_ctx: *mut encoder_ctx,
    dec_ctx: *mut lib_cc_decode,
    esstream: *mut bitstream,
    sub: *mut cc_subtitle,
) -> i32 {
    let mut bs = copy_bitstream_c_to_rust(esstream);
    let result = read_gop_info(&mut *enc_ctx, &mut *dec_ctx, &mut bs, &mut *sub).unwrap_or(false);
    copy_bitstream_from_rust_to_c(esstream, &bs);
    i32::from(result)
}
/// # Safety
/// This function is unsafe because it copies from C to Rust and calls `read_pic_info` which is also unsafe.
#[no_mangle]
pub unsafe extern "C" fn ccxr_read_pic_info(
    enc_ctx: *mut encoder_ctx,
    dec_ctx: *mut lib_cc_decode,
    esstream: *mut bitstream,
    sub: *mut cc_subtitle,
) -> i32 {
    let mut bs = copy_bitstream_c_to_rust(esstream);
    let result = read_pic_info(&mut *enc_ctx, &mut *dec_ctx, &mut bs, &mut *sub).unwrap_or(false);
    copy_bitstream_from_rust_to_c(esstream, &bs);
    i32::from(result)
}
/// # Safety
/// This function is unsafe because it copies from C to Rust and back.
#[no_mangle]
pub unsafe extern "C" fn ccxr_read_eau_info(
    enc_ctx: *mut encoder_ctx,
    dec_ctx: *mut lib_cc_decode,
    esstream: *mut bitstream,
    udtype: i32,
    sub: *mut cc_subtitle,
) -> i32 {
    let mut bs = copy_bitstream_c_to_rust(esstream);

    let result =
        read_eau_info(&mut *enc_ctx, &mut *dec_ctx, &mut bs, udtype, &mut *sub).unwrap_or(false);
    copy_bitstream_from_rust_to_c(esstream, &bs);
    i32::from(result)
}
/// # Safety
/// This function is unsafe because it copies from C to Rust and back.
#[no_mangle]
pub unsafe extern "C" fn ccxr_read_pic_data(esstream: *mut bitstream) -> i32 {
    let mut bs = copy_bitstream_c_to_rust(esstream);
    let result = read_pic_data(&mut bs).unwrap_or(false);
    copy_bitstream_from_rust_to_c(esstream, &bs);
    i32::from(result)
}
