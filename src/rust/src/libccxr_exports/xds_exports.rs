use lib_ccxr::decoder_xds::functions_xds::*;
use lib_ccxr::decoder_xds::structs_xds::*;
use lib_ccxr::time::timing::*;

#[no_mangle]
pub extern "C" fn ccxr_ccx_decoders_xds_init_library(
    timing: TimingContext,
    xds_write_to_file: i64,
) -> CcxDecodersXdsContext {
    ccx_decoders_xds_init_library(timing, xds_write_to_file) // Call the pure Rust function
}

#[no_mangle]
pub extern "C" fn ccxr_write_xds_string(
    sub: &mut CcSubtitle,
    ctx: &mut CcxDecodersXdsContext,
    p: *const u8,
    len: usize,
) -> i32 {
    write_xds_string(sub, ctx, p, len) // Call the pure Rust function
}

/// # Safety
///
/// The `fmt` pointer must be a valid null-terminated C string,
/// and `args` must contain valid formatting arguments. Ensure that
/// `fmt` is not null and points to a properly allocated memory region.
#[no_mangle]
pub unsafe extern "C" fn ccxr_xdsprint(
    sub: &mut CcSubtitle,
    ctx: &mut CcxDecodersXdsContext,
    fmt: *const std::os::raw::c_char,
    args: *const std::os::raw::c_char,
) {
    // Call the pure Rust function
    xdsprint(
        sub,
        ctx,
        unsafe { std::ffi::CStr::from_ptr(fmt).to_str().unwrap_or("") },
        format_args!("{}", unsafe {
            std::ffi::CStr::from_ptr(args).to_str().unwrap_or("")
        }),
    );
}

#[no_mangle]
pub extern "C" fn ccxr_clear_xds_buffer(ctx: &mut CcxDecodersXdsContext, num: i64) {
    clear_xds_buffer(ctx, num); // Call the pure Rust function
}

#[no_mangle]
pub extern "C" fn ccxr_how_many_used(ctx: &CcxDecodersXdsContext) -> i64 {
    how_many_used(ctx) // Call the pure Rust function
}

#[no_mangle]
pub extern "C" fn ccxr_process_xds_bytes(ctx: &mut CcxDecodersXdsContext, hi: u8, lo: i64) {
    process_xds_bytes(ctx, hi, lo); // Call the pure Rust function
}

#[no_mangle]
pub extern "C" fn ccxr_xds_do_copy_generation_management_system(
    sub: &mut CcSubtitle,
    ctx: &mut CcxDecodersXdsContext,
    c1: u8,
    c2: u8,
) {
    xds_do_copy_generation_management_system(sub, ctx, c1, c2); // Call the pure Rust function
}

#[no_mangle]
pub extern "C" fn ccxr_xds_do_content_advisory(
    sub: &mut CcSubtitle,
    ctx: &mut CcxDecodersXdsContext,
    c1: u8,
    c2: u8,
) {
    xds_do_content_advisory(sub, ctx, c1, c2); // Call the pure Rust function
}

#[no_mangle]
pub extern "C" fn ccxr_xds_do_private_data(
    sub: &mut CcSubtitle,
    ctx: &mut CcxDecodersXdsContext,
) -> i64 {
    xds_do_private_data(sub, ctx) // Call the pure Rust function
}

#[no_mangle]
pub extern "C" fn ccxr_xds_do_misc(ctx: &CcxDecodersXdsContext) -> i64 {
    xds_do_misc(ctx) // Call the pure Rust function
}

#[no_mangle]
pub extern "C" fn ccxr_xds_do_current_and_future(
    sub: &mut CcSubtitle,
    ctx: &mut CcxDecodersXdsContext,
) -> i64 {
    xds_do_current_and_future(sub, ctx) // Call the pure Rust function
}

#[no_mangle]
pub extern "C" fn ccxr_do_end_of_xds(
    sub: &mut CcSubtitle,
    ctx: &mut CcxDecodersXdsContext,
    expected_checksum: i64,
) {
    do_end_of_xds(sub, ctx, expected_checksum); // Call the pure Rust function
}

#[no_mangle]
pub extern "C" fn ccxr_xds_do_channel(
    sub: &mut CcSubtitle,
    ctx: &mut CcxDecodersXdsContext,
) -> i64 {
    xds_do_channel(sub, ctx) // Call the pure Rust function
}

#[no_mangle]
pub extern "C" fn ccxr_xds_debug_test(ctx: &mut CcxDecodersXdsContext, sub: &mut CcSubtitle) {
    xds_debug_test(ctx, sub); // Call the pure Rust function
}

#[no_mangle]
pub extern "C" fn ccxr_xds_cea608_test(ctx: &mut CcxDecodersXdsContext, sub: &mut CcSubtitle) {
    xds_cea608_test(ctx, sub); // Call the pure Rust function
}
