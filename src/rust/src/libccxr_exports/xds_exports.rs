use lib_ccxr::decoder_xds::functions_xds::{do_end_of_xds, process_xds_bytes, xds_cea608_test};
use lib_ccxr::decoder_xds::structs_xds::{CcSubtitle, CcxDecodersXdsContext};
use lib_ccxr::time::TimingContext;

#[no_mangle]
/// # Safety
/// - `sub` must be a non-null pointer to a valid, mutable `CcSubtitle` instance.
/// - `ctx` must be a non-null pointer to a valid, mutable `CcxDecodersXdsContext` instance.
/// - The caller must ensure that the memory referenced by `sub` and `ctx` remains valid for the duration of the function call.
/// - Passing null or invalid pointers will result in undefined behavior.
pub unsafe extern "C" fn ccxr_do_end_of_xds(
    sub: *mut CcSubtitle,
    ctx: *mut CcxDecodersXdsContext,
    expected_checksum: u8,
) {
    if sub.is_null() || ctx.is_null() {
        return;
    }

    let sub = unsafe { &mut *sub };
    let ctx = unsafe { &mut *ctx };

    do_end_of_xds(sub, ctx, expected_checksum);
}

#[no_mangle]
/// # Safety
/// - `ctx` must be a non-null pointer to a valid, mutable `CcxDecodersXdsContext` instance.
/// - The caller must ensure that the memory referenced by `ctx` remains valid for the duration of the function call.
/// - Passing a null or invalid pointer will result in undefined behavior.
pub unsafe extern "C" fn ccxr_process_xds_bytes(ctx: *mut CcxDecodersXdsContext, hi: u8, lo: u8) {
    if ctx.is_null() {
        return;
    }

    let ctx = unsafe { &mut *ctx };

    process_xds_bytes(ctx, hi as i64, lo as i64);
}

#[no_mangle]
/// # Safety
/// - `timing` must be a non-null pointer to a valid, mutable `TimingContext` instance.
/// - The caller must ensure that the memory referenced by `timing` remains valid for the duration of the function call.
/// - The returned pointer must be managed properly by the caller. It points to a heap-allocated `CcxDecodersXdsContext` instance, and the caller is responsible for freeing it using `Box::from_raw` when it is no longer needed.
/// - Failure to free the returned pointer will result in a memory leak.
/// - Passing a null or invalid pointer for `timing` will result in undefined behavior.
pub unsafe extern "C" fn ccxr_ccx_decoders_xds_init_library(
    timing: *mut TimingContext,
    xds_write_to_file: bool,
) -> *mut CcxDecodersXdsContext {
    if timing.is_null() {
        return std::ptr::null_mut();
    }

    let timing = unsafe { &mut *timing };

    let ctx = CcxDecodersXdsContext::xds_init_library(timing.clone(), xds_write_to_file);

    Box::into_raw(Box::new(ctx))
}

#[no_mangle]
/// # Safety
/// - `ctx` must be a non-null pointer to a valid, mutable `CcxDecodersXdsContext` instance.
/// - `sub` must be a non-null pointer to a valid, mutable `CcSubtitle` instance.
/// - The caller must ensure that the memory referenced by `ctx` and `sub` remains valid for the duration of the function call.
/// - Passing null or invalid pointers will result in undefined behavior.
pub unsafe extern "C" fn ccxr_xds_cea608_test(
    ctx: *mut CcxDecodersXdsContext,
    sub: *mut CcSubtitle,
) {
    if ctx.is_null() || sub.is_null() {
        return;
    }

    let ctx = unsafe { &mut *ctx };
    let sub = unsafe { &mut *sub };

    xds_cea608_test(ctx, sub);
}
