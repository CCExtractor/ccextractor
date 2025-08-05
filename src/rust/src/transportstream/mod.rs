use crate::bindings::{cap_info, demuxer_data, lib_ccx_ctx};
use crate::ccx_options;
use crate::common::{copy_from_rust, copy_to_rust};
use crate::gxf_demuxer::common_structs::DemuxerError;
use crate::libccxr_exports::demuxer::{copy_demuxer_from_c_to_rust, copy_demuxer_from_rust_to_c};
use crate::transportstream::core::ts_readstream;
use lib_ccxr::common::Options;
use std::convert::TryInto;
use std::os::raw::{c_int, c_long, c_ulonglong};

pub mod common_structs;
pub mod core;
pub mod epg_tables;
pub mod pmt;
pub mod tables;
/// # Safety
/// This function is unsafe because it dereferences raw pointers and uses C structs.
#[no_mangle]
pub unsafe extern "C" fn ccxr_ts_get_more_data(
    ctx: *mut lib_ccx_ctx,
    data: *mut *mut demuxer_data,
    cap: *mut cap_info,
    haup_capbuf: *mut *mut u8,
    haup_capbufsize: *mut c_int,
    haup_capbuflen: *mut c_int,
    last_pts: *mut c_ulonglong,
    tspacket: *mut u8,
) -> c_long {
    let mut demux_ctx = copy_demuxer_from_c_to_rust((*ctx).demux_ctx);
    let mut CcxOptions: Options = copy_to_rust(&raw const ccx_options);
    let tspacket_slice: &mut [u8; 188] =
        match std::slice::from_raw_parts_mut(tspacket, 188).try_into() {
            Ok(arr) => arr,
            Err(_) => {
                return -1;
            }
        };
    let mut ret;
    loop {
        ret = ts_readstream(
            &mut demux_ctx,
            data,
            &mut *cap,
            &mut CcxOptions,
            haup_capbuf,
            &mut *haup_capbufsize,
            &mut *haup_capbuflen,
            &mut *last_pts,
            tspacket_slice,
        );
        if ret != DemuxerError::Retry as i64 {
            break;
        }
    }
    copy_from_rust(&raw mut ccx_options, CcxOptions);
    copy_demuxer_from_rust_to_c((*ctx).demux_ctx, &demux_ctx);
    ret as c_long
}
