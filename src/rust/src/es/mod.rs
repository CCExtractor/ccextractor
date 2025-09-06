use crate::bindings::{
    cc_subtitle, ccx_frame_type, ccx_frame_type_CCX_FRAME_TYPE_B_FRAME,
    ccx_frame_type_CCX_FRAME_TYPE_D_FRAME, ccx_frame_type_CCX_FRAME_TYPE_I_FRAME,
    ccx_frame_type_CCX_FRAME_TYPE_P_FRAME, ccx_frame_type_CCX_FRAME_TYPE_RESET_OR_UNKNOWN,
    encoder_ctx, lib_cc_decode,
};
use crate::ccx_options;
use crate::encoder::FromCType;
use crate::es::core::process_m2v;
use lib_ccxr::common::{FrameType, Options};

pub mod core;
pub mod eau;
pub mod gop;
pub mod pic;
pub mod seq;
pub mod userdata;

impl FromCType<ccx_frame_type> for FrameType {
    // TODO move to ctorust.rs when demuxer is merged
    unsafe fn from_ctype(c_value: ccx_frame_type) -> Option<Self> {
        match c_value {
            ccx_frame_type_CCX_FRAME_TYPE_RESET_OR_UNKNOWN => Some(FrameType::ResetOrUnknown),
            ccx_frame_type_CCX_FRAME_TYPE_I_FRAME => Some(FrameType::IFrame),
            ccx_frame_type_CCX_FRAME_TYPE_P_FRAME => Some(FrameType::PFrame),
            ccx_frame_type_CCX_FRAME_TYPE_B_FRAME => Some(FrameType::BFrame),
            ccx_frame_type_CCX_FRAME_TYPE_D_FRAME => Some(FrameType::DFrame),
            _ => None,
        }
    }
}
/// # Safety
/// This function is unsafe because it dereferences raw pointers from C structs
#[no_mangle]
pub unsafe extern "C" fn ccxr_process_m2v(
    enc_ctx: *mut encoder_ctx,
    dec_ctx: *mut lib_cc_decode,
    data: *const u8,
    length: usize,
    sub: *mut cc_subtitle,
) -> usize {
    if enc_ctx.is_null() || dec_ctx.is_null() || data.is_null() || sub.is_null() {
        // This shouldn't happen
        return 0;
    }
    let mut CcxOptions = Options {
        // that's the only thing that's required for now
        gui_mode_reports: ccx_options.gui_mode_reports != 0,
        ..Default::default()
    };

    let data_slice = std::slice::from_raw_parts(data, length);
    process_m2v(
        &mut *enc_ctx,
        &mut *dec_ctx,
        data_slice,
        length,
        &mut *sub,
        &mut CcxOptions,
    )
    .unwrap_or(0)
}
