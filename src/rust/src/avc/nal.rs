use crate::avc::common_types::*;
use crate::avc::core::round_portable;
use crate::bindings::{
    anchor_hdcc, cc_subtitle, encoder_ctx, lib_cc_decode, process_hdcc, store_hdcc,
};
use crate::libccxr_exports::time::{
    ccxr_print_debug_timing, ccxr_print_mstime_static, ccxr_set_fts,
};
use crate::{ccx_options, current_fps, total_frames_count, MPEG_CLOCK_FREQ};
use lib_ccxr::common::{AvcNalType, BitStreamRust, BitstreamError, FRAMERATES_VALUES, SLICE_TYPES};
use lib_ccxr::util::log::DebugMessageFlag;
use lib_ccxr::{debug, info};
use std::os::raw::{c_char, c_long};

/// Process sequence parameter set RBSP
pub fn seq_parameter_set_rbsp(
    ctx: &mut AvcContextRust,
    seqbuf: &[u8],
) -> Result<(), BitstreamError> {
    // Calculate buffer length from pointer difference
    let mut q1 = BitStreamRust::new(seqbuf)?;
    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "SEQUENCE PARAMETER SET (bitlen: {})", q1.bits_left);

    let tmp = q1.read_bits(8)?;
    let profile_idc = tmp;
    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "profile_idc=                                   {:4} ({:#X})", tmp, tmp);

    let tmp = q1.read_bits(1)?;
    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "constraint_set0_flag=                          {:4} ({:#X})", tmp, tmp);

    let tmp = q1.read_bits(1)?;
    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "constraint_set1_flag=                          {:4} ({:#X})", tmp, tmp);

    let tmp = q1.read_bits(1)?;
    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "constraint_set2_flag=                          {:4} ({:#X})", tmp, tmp);

    let tmp = q1.read_bits(1)?;
    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "constraint_set3_flag=                          {:4} ({:#X})", tmp, tmp);

    let tmp = q1.read_bits(1)?;
    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "constraint_set4_flag=                          {:4} ({:#X})", tmp, tmp);

    let tmp = q1.read_bits(1)?;
    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "constraint_set5_flag=                          {:4} ({:#X})", tmp, tmp);

    let tmp = q1.read_bits(2)?;
    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "reserved=                                      {:4} ({:#X})", tmp, tmp);

    let tmp = q1.read_bits(8)?;
    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "level_idc=                                     {:4} ({:#X})", tmp, tmp);

    ctx.seq_parameter_set_id = q1.read_exp_golomb_unsigned()? as i64;
    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "seq_parameter_set_id=                          {:4} ({:#X})", ctx.seq_parameter_set_id, ctx.seq_parameter_set_id);

    if profile_idc == 100
        || profile_idc == 110
        || profile_idc == 122
        || profile_idc == 244
        || profile_idc == 44
        || profile_idc == 83
        || profile_idc == 86
        || profile_idc == 118
        || profile_idc == 128
    {
        let chroma_format_idc = q1.read_exp_golomb_unsigned()?;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "chroma_format_idc=                             {:4} ({:#X})", chroma_format_idc, chroma_format_idc);

        if chroma_format_idc == 3 {
            let tmp = q1.read_bits(1)?;
            debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "separate_colour_plane_flag=                    {:4} ({:#X})", tmp, tmp);
        }

        let tmp = q1.read_exp_golomb_unsigned()?;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "bit_depth_luma_minus8=                         {:4} ({:#X})", tmp, tmp);

        let tmp = q1.read_exp_golomb_unsigned()?;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "bit_depth_chroma_minus8=                       {:4} ({:#X})", tmp, tmp);

        let tmp = q1.read_bits(1)?;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "qpprime_y_zero_transform_bypass_flag=          {:4} ({:#X})", tmp, tmp);

        let tmp = q1.read_bits(1)?;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "seq_scaling_matrix_present_flag=               {:4} ({:#X})", tmp, tmp);

        if tmp == 1 {
            // WVI: untested, just copied from specs.
            for i in 0..if chroma_format_idc != 3 { 8 } else { 12 } {
                let tmp = q1.read_bits(1)?;
                debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "seq_scaling_list_present_flag[{}]=                 {:4} ({:#X})", i, tmp, tmp);

                if tmp != 0 {
                    // We use a "dummy"/slimmed-down replacement here. Actual/full code can be found in the spec (ISO/IEC 14496-10:2012(E)) chapter 7.3.2.1.1.1 - Scaling list syntax
                    if i < 6 {
                        // Scaling list size 16
                        // TODO: replace with full scaling list implementation?
                        let mut next_scale = 8i64;
                        let mut last_scale = 8i64;
                        for _j in 0..16 {
                            if next_scale != 0 {
                                let delta_scale = q1.read_exp_golomb()?;
                                next_scale = (last_scale + delta_scale + 256) % 256;
                            }
                            last_scale = if next_scale == 0 {
                                last_scale
                            } else {
                                next_scale
                            };
                        }
                        // END of TODO
                    } else {
                        // Scaling list size 64
                        // TODO: replace with full scaling list implementation?
                        let mut next_scale = 8i64;
                        let mut last_scale = 8i64;
                        for _j in 0..64 {
                            if next_scale != 0 {
                                let delta_scale = q1.read_exp_golomb()?;
                                next_scale = (last_scale + delta_scale + 256) % 256;
                            }
                            last_scale = if next_scale == 0 {
                                last_scale
                            } else {
                                next_scale
                            };
                        }
                        // END of TODO
                    }
                }
            }
        }
    }

    ctx.log2_max_frame_num = q1.read_exp_golomb_unsigned()? as i32;
    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "log2_max_frame_num4_minus4=                    {:4} ({:#X})", ctx.log2_max_frame_num, ctx.log2_max_frame_num);
    ctx.log2_max_frame_num += 4; // 4 is added due to the formula.

    ctx.pic_order_cnt_type = q1.read_exp_golomb_unsigned()? as i32;
    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "pic_order_cnt_type=                            {:4} ({:#X})", ctx.pic_order_cnt_type, ctx.pic_order_cnt_type);

    if ctx.pic_order_cnt_type == 0 {
        ctx.log2_max_pic_order_cnt_lsb = q1.read_exp_golomb_unsigned()? as i32;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "log2_max_pic_order_cnt_lsb_minus4=             {:4} ({:#X})", ctx.log2_max_pic_order_cnt_lsb, ctx.log2_max_pic_order_cnt_lsb);
        ctx.log2_max_pic_order_cnt_lsb += 4; // 4 is added due to formula.
    } else if ctx.pic_order_cnt_type == 1 {
        // CFS: Untested, just copied from specs.
        let tmp = q1.read_bits(1)?;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "delta_pic_order_always_zero_flag=              {:4} ({:#X})", tmp, tmp);

        let tmp = q1.read_exp_golomb()?;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "offset_for_non_ref_pic=                        {:4} ({:#X})", tmp, tmp);

        let tmp = q1.read_exp_golomb()?;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "offset_for_top_to_bottom_field                 {:4} ({:#X})", tmp, tmp);

        let num_ref_frame_in_pic_order_cnt_cycle = q1.read_exp_golomb_unsigned()?;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "num_ref_frame_in_pic_order_cnt_cycle           {:4} ({:#X})", num_ref_frame_in_pic_order_cnt_cycle, num_ref_frame_in_pic_order_cnt_cycle);

        for i in 0..num_ref_frame_in_pic_order_cnt_cycle {
            let tmp = q1.read_exp_golomb()?;
            debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "offset_for_ref_frame [{} / {}] =               {:4} ({:#X})", i, num_ref_frame_in_pic_order_cnt_cycle, tmp, tmp);
        }
    } else {
        // Nothing needs to be parsed when pic_order_cnt_type == 2
    }

    let tmp = q1.read_exp_golomb_unsigned()?;
    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "max_num_ref_frames=                            {:4} ({:#X})", tmp, tmp);

    let tmp = q1.read_bits(1)?;
    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "gaps_in_frame_num_value_allowed_flag=          {:4} ({:#X})", tmp, tmp);

    let tmp = q1.read_exp_golomb_unsigned()?;
    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "pic_width_in_mbs_minus1=                       {:4} ({:#X})", tmp, tmp);

    let tmp = q1.read_exp_golomb_unsigned()?;
    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "pic_height_in_map_units_minus1=                {:4} ({:#X})", tmp, tmp);

    ctx.frame_mbs_only_flag = q1.read_bits(1)? != 0;

    if !ctx.frame_mbs_only_flag {
        let tmp = q1.read_bits(1)?;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "mb_adaptive_fr_fi_flag=                        {:4} ({:#X})", tmp, tmp);
    }

    let tmp = q1.read_bits(1)?;
    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "direct_8x8_inference_f=                        {:4} ({:#X})", tmp, tmp);

    let tmp = q1.read_bits(1)?;
    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "frame_cropping_flag=                           {:4} ({:#X})", tmp, tmp);

    if tmp != 0 {
        let tmp = q1.read_exp_golomb_unsigned()?;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "frame_crop_left_offset=                        {:4} ({:#X})", tmp, tmp);

        let tmp = q1.read_exp_golomb_unsigned()?;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "frame_crop_right_offset=                       {:4} ({:#X})", tmp, tmp);

        let tmp = q1.read_exp_golomb_unsigned()?;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "frame_crop_top_offset=                         {:4} ({:#X})", tmp, tmp);

        let tmp = q1.read_exp_golomb_unsigned()?;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "frame_crop_bottom_offset=                      {:4} ({:#X})", tmp, tmp);
    }

    let tmp = q1.read_bits(1)?;
    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "vui_parameters_present=                        {:4} ({:#X})", tmp, tmp);

    if tmp != 0 {
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "VUI parameters");

        let tmp = q1.read_bits(1)?;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "aspect_ratio_info_pres=                        {:4} ({:#X})", tmp, tmp);

        if tmp != 0 {
            let tmp = q1.read_bits(8)?;
            debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "aspect_ratio_idc=                              {:4} ({:#X})", tmp, tmp);

            if tmp == 255 {
                let tmp = q1.read_bits(16)?;
                debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "sar_width=                                     {:4} ({:#X})", tmp, tmp);

                let tmp = q1.read_bits(16)?;
                debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "sar_height=                                    {:4} ({:#X})", tmp, tmp);
            }
        }

        let tmp = q1.read_bits(1)?;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "overscan_info_pres_flag=                       {:4} ({:#X})", tmp, tmp);

        if tmp != 0 {
            let tmp = q1.read_bits(1)?;
            debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "overscan_appropriate_flag=                     {:4} ({:#X})", tmp, tmp);
        }

        let tmp = q1.read_bits(1)?;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "video_signal_type_present_flag=                {:4} ({:#X})", tmp, tmp);

        if tmp != 0 {
            let tmp = q1.read_bits(3)?;
            debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "video_format=                                  {:4} ({:#X})", tmp, tmp);

            let tmp = q1.read_bits(1)?;
            debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "video_full_range_flag=                         {:4} ({:#X})", tmp, tmp);

            let tmp = q1.read_bits(1)?;
            debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "colour_description_present_flag=               {:4} ({:#X})", tmp, tmp);

            if tmp != 0 {
                let tmp = q1.read_bits(8)?;
                debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "colour_primaries=                              {:4} ({:#X})", tmp, tmp);

                let tmp = q1.read_bits(8)?;
                debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "transfer_characteristics=                      {:4} ({:#X})", tmp, tmp);

                let tmp = q1.read_bits(8)?;
                debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "matrix_coefficients=                           {:4} ({:#X})", tmp, tmp);
            }
        }

        let tmp = q1.read_bits(1)?;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "chroma_loc_info_present_flag=                  {:4} ({:#X})", tmp, tmp);

        if tmp != 0 {
            let tmp = q1.read_exp_golomb_unsigned()?;
            debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "chroma_sample_loc_type_top_field=                  {:4} ({:#X})", tmp, tmp);

            let tmp = q1.read_exp_golomb_unsigned()?;
            debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "chroma_sample_loc_type_bottom_field=               {:4} ({:#X})", tmp, tmp);
        }

        let tmp = q1.read_bits(1)?;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "timing_info_present_flag=                      {:4} ({:#X})", tmp, tmp);

        if tmp != 0 {
            let tmp = q1.read_bits(32)?;
            let num_units_in_tick = tmp;
            debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "num_units_in_tick=                             {:4} ({:#X})", tmp, tmp);

            let tmp = q1.read_bits(32)?;
            let time_scale = tmp;
            debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "time_scale=                                    {:4} ({:#X})", tmp, tmp);

            let tmp = q1.read_bits(1)?;
            let fixed_frame_rate_flag = tmp != 0;
            debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "fixed_frame_rate_flag=                         {:4} ({:#X})", tmp, tmp);

            // Change: use num_units_in_tick and time_scale to calculate FPS. (ISO/IEC 14496-10:2012(E), page 397 & further)
            if fixed_frame_rate_flag {
                let clock_tick = num_units_in_tick as f64 / time_scale as f64;
                debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "clock_tick= {}", clock_tick);

                unsafe {
                    current_fps = time_scale as f64 / (2.0 * num_units_in_tick as f64);
                } // Based on formula D-2, p. 359 of the ISO/IEC 14496-10:2012(E) spec.
            }
        }

        let tmp = q1.read_bits(1)?;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "nal_hrd_parameters_present_flag=               {:4} ({:#X})", tmp, tmp);

        if tmp != 0 {
            debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "nal_hrd. Not implemented for now. Hopefully not needed. Skipping rest of NAL");
            ctx.num_nal_hrd += 1;
            return Ok(());
        }

        let tmp1 = q1.read_bits(1)?;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "vcl_hrd_parameters_present_flag=               {:#X}", tmp1);

        if tmp != 0 {
            // TODO.
            println!(
                "vcl_hrd. Not implemented for now. Hopefully not needed. Skipping rest of NAL"
            );
            ctx.num_vcl_hrd += 1;
        }

        if tmp != 0 || tmp1 != 0 {
            let tmp = q1.read_bits(1)?;
            debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "low_delay_hrd_flag=                                {:4} ({:#X})", tmp, tmp);
            return Ok(());
        }

        let tmp = q1.read_bits(1)?;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "pic_struct_present_flag=                       {:4} ({:#X})", tmp, tmp);

        let tmp = q1.read_bits(1)?;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "bitstream_restriction_flag=                    {:4} ({:#X})", tmp, tmp);

        // ..
        // The hope was to find the GOP length in max_dec_frame_buffering, but
        // it was not set in the testfile.  Ignore the rest here, it's
        // currently not needed.
    }

    Ok(())
}
/// # Safety
/// This function is unsafe because it dereferences raw pointers and calls C functions.
pub unsafe fn slice_header(
    enc_ctx: &mut encoder_ctx,
    dec_ctx: &mut lib_cc_decode,
    heabuf: &mut [u8],
    nal_unit_type: &AvcNalType,
    sub: &mut cc_subtitle,
) -> Result<(), Box<dyn std::error::Error>> {
    let mut tmp: i64;

    let mut bottom_field_flag: i64 = 0;
    let mut pic_order_cnt_lsb: i64 = -1;

    let field_pic_flag: i64; // Moved here because it's needed for pic_order_cnt_type==2

    // Initialize bitstream
    let mut q1 = match BitStreamRust::new(heabuf) {
        Ok(bs) => bs,
        Err(_) => {
            info!("Skipping slice header due to failure in init_bitstream.");
            return Ok(());
        }
    };

    let ird_pic_flag: i32 = if *nal_unit_type == AvcNalType::CodedSliceIdrPicture {
        1
    } else {
        0
    };

    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "SLICE HEADER");

    tmp = q1.read_exp_golomb_unsigned()? as i64;
    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "first_mb_in_slice=     {:4} ({:#X})", tmp, tmp);

    let slice_type: i64 = q1.read_exp_golomb_unsigned()? as i64;
    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "slice_type=            {:4X}", slice_type);

    tmp = q1.read_exp_golomb_unsigned()? as i64;
    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "pic_parameter_set_id=  {:4} ({:#X})", tmp, tmp);

    (*dec_ctx.avc_ctx).lastframe_num = (*dec_ctx.avc_ctx).frame_num;
    let max_frame_num: i32 = (1 << (*dec_ctx.avc_ctx).log2_max_frame_num) - 1;

    // Needs log2_max_frame_num_minus4 + 4 bits
    (*dec_ctx.avc_ctx).frame_num =
        q1.read_bits((*dec_ctx.avc_ctx).log2_max_frame_num as u32)? as i64;
    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "frame_num=             {:4X}", (*dec_ctx.avc_ctx).frame_num);

    if (*dec_ctx.avc_ctx).frame_mbs_only_flag == 0 {
        field_pic_flag = q1.read_bits(1)? as i64;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "field_pic_flag=        {:4X}", field_pic_flag);

        if field_pic_flag != 0 {
            // bottom_field_flag
            bottom_field_flag = q1.read_bits(1)? as i64;
            debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "bottom_field_flag=     {:4X}", bottom_field_flag);

            // When bottom_field_flag is set the video is interlaced,
            // override current_fps.
            current_fps = FRAMERATES_VALUES[dec_ctx.current_frame_rate as usize];
        }
    }

    debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "ird_pic_flag=            {:4}", ird_pic_flag);

    if *nal_unit_type == AvcNalType::CodedSliceIdrPicture {
        tmp = q1.read_exp_golomb_unsigned()? as i64;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "idr_pic_id=            {:4} ({:#X})", tmp, tmp);
        // TODO
    }

    if (*dec_ctx.avc_ctx).pic_order_cnt_type == 0 {
        pic_order_cnt_lsb =
            q1.read_bits((*dec_ctx.avc_ctx).log2_max_pic_order_cnt_lsb as u32)? as i64;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "pic_order_cnt_lsb=     {:4X}", pic_order_cnt_lsb);
    }

    if (*dec_ctx.avc_ctx).pic_order_cnt_type == 1 {
        panic!("In slice_header: AVC: ctx->avc_ctx->pic_order_cnt_type == 1 not yet supported.");
    }

    // Ignore slice with same pic order or pts
    if ccx_options.usepicorder != 0 {
        if (*dec_ctx.avc_ctx).last_pic_order_cnt_lsb == pic_order_cnt_lsb {
            return Ok(());
        }
        (*dec_ctx.avc_ctx).last_pic_order_cnt_lsb = pic_order_cnt_lsb;
    } else {
        if (*dec_ctx.timing).current_pts == (*dec_ctx.avc_ctx).last_slice_pts {
            return Ok(());
        }
        (*dec_ctx.avc_ctx).last_slice_pts = (*dec_ctx.timing).current_pts;
    }

    // The rest of the data in slice_header() is currently unused.

    // A reference pic (I or P is always the last displayed picture of a POC
    // sequence. B slices can be reference pics, so ignore ctx->avc_ctx->nal_ref_idc.
    let mut isref = 0;
    match slice_type {
        // P-SLICES
        0 | 5 |
        // I-SLICES
        2 | 7 => {
            isref = 1;
        }
        _ => {}
    }

    let maxrefcnt = (1 << (*dec_ctx.avc_ctx).log2_max_pic_order_cnt_lsb) - 1;

    // If we saw a jump set maxidx, lastmaxidx to -1
    let mut dif = (*dec_ctx.avc_ctx).frame_num - (*dec_ctx.avc_ctx).lastframe_num;
    if dif == -(max_frame_num as i64) {
        dif = 0;
    }
    if (*dec_ctx.avc_ctx).lastframe_num > -1 && !(0..=1).contains(&dif) {
        (*dec_ctx.avc_ctx).num_jump_in_frames += 1;
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "Jump in frame numbers ({}/{})",
               (*dec_ctx.avc_ctx).frame_num, (*dec_ctx.avc_ctx).lastframe_num);
        // This will prohibit setting current_tref on potential jumps.
        (*dec_ctx.avc_ctx).maxidx = -1;
        (*dec_ctx.avc_ctx).lastmaxidx = -1;
    }

    // Sometimes two P-slices follow each other, see garbled_dishHD.mpg,
    // in this case we only treat the first as a reference pic
    if isref == 1 && dec_ctx.frames_since_last_gop <= 3 {
        // Used to be == 1, but the sample file
        // 2014 SugarHouse Casino Mummers Parade Fancy Brigades_new.ts was garbled
        // Probably doing a proper PTS sort would be a better solution.
        isref = 0;
        debug!(msg_type = DebugMessageFlag::TIME; "Ignoring this reference pic.");
    }

    // if slices are buffered - flush
    if isref == 1 {
        debug!(msg_type = DebugMessageFlag::VIDEO_STREAM; "Reference pic! [{}]", SLICE_TYPES[slice_type as usize]);
        debug!(msg_type = DebugMessageFlag::TIME; "Reference pic! [{}] maxrefcnt: {:3}",
               SLICE_TYPES[slice_type as usize], maxrefcnt);

        // Flush buffered cc blocks before doing the housekeeping
        if dec_ctx.has_ccdata_buffered != 0 {
            process_hdcc(enc_ctx, dec_ctx, sub);
        }

        dec_ctx.last_gop_length = dec_ctx.frames_since_last_gop;
        dec_ctx.frames_since_last_gop = 0;
        (*dec_ctx.avc_ctx).last_gop_maxtref = (*dec_ctx.avc_ctx).maxtref;
        (*dec_ctx.avc_ctx).maxtref = 0;
        (*dec_ctx.avc_ctx).lastmaxidx = (*dec_ctx.avc_ctx).maxidx;
        (*dec_ctx.avc_ctx).maxidx = 0;
        (*dec_ctx.avc_ctx).lastminidx = (*dec_ctx.avc_ctx).minidx;
        (*dec_ctx.avc_ctx).minidx = 10000;

        if ccx_options.usepicorder != 0 {
            // Use pic_order_cnt_lsb

            // Make sure that current_index never wraps for curidx values that
            // are smaller than currref
            (*dec_ctx.avc_ctx).currref = pic_order_cnt_lsb as i32;
            if (*dec_ctx.avc_ctx).currref < maxrefcnt / 3 {
                (*dec_ctx.avc_ctx).currref += maxrefcnt + 1;
            }

            // If we wrapped around lastmaxidx might be larger than
            // the current index - fix this.
            if (*dec_ctx.avc_ctx).lastmaxidx > (*dec_ctx.avc_ctx).currref + maxrefcnt / 2 {
                // implies lastmaxidx > 0
                (*dec_ctx.avc_ctx).lastmaxidx -= maxrefcnt + 1;
            }
        } else {
            // Use PTS ordering
            (*dec_ctx.avc_ctx).currefpts = (*dec_ctx.timing).current_pts;
            (*dec_ctx.avc_ctx).currref = 0;
        }

        anchor_hdcc(dec_ctx, (*dec_ctx.avc_ctx).currref);
    }

    let mut current_index = if ccx_options.usepicorder != 0 {
        // Use pic_order_cnt_lsb
        // Wrap (add max index value) current_index if needed.
        if (*dec_ctx.avc_ctx).currref as i64 - pic_order_cnt_lsb > (maxrefcnt / 2) as i64 {
            (pic_order_cnt_lsb + (maxrefcnt + 1) as i64) as i32
        } else {
            pic_order_cnt_lsb as i32
        }
    } else {
        // Use PTS ordering - calculate index position from PTS difference and
        // frame rate
        // The 2* accounts for a discrepancy between current and actual FPS
        // seen in some files (CCSample2.mpg)
        let pts_diff = (*dec_ctx.timing).current_pts - (*dec_ctx.avc_ctx).currefpts;
        let fps_factor = MPEG_CLOCK_FREQ as f64 / current_fps;
        round_portable(2.0 * pts_diff as f64 / fps_factor) as i32
    };

    if !ccx_options.usepicorder != 0 && current_index.abs() >= MAXBFRAMES {
        // Probably a jump in the timeline. Warn and handle gracefully.
        info!(
            "Found large gap({}) in PTS! Trying to recover ...",
            current_index
        );
        current_index = 0;
    }

    // Track maximum index for this GOP
    if current_index > (*dec_ctx.avc_ctx).maxidx {
        (*dec_ctx.avc_ctx).maxidx = current_index;
    }

    if ccx_options.usepicorder != 0 {
        // Calculate tref
        if (*dec_ctx.avc_ctx).lastmaxidx > 0 {
            (*dec_ctx.timing).current_tref = current_index - (*dec_ctx.avc_ctx).lastmaxidx - 1;
            // Set maxtref
            if (*dec_ctx.timing).current_tref > (*dec_ctx.avc_ctx).maxtref {
                (*dec_ctx.avc_ctx).maxtref = (*dec_ctx.timing).current_tref;
            }
            // Now an ugly workaround where pic_order_cnt_lsb increases in
            // steps of two. The 1.5 is an approximation, it should be:
            // last_gop_maxtref+1 == last_gop_length*2
            if (*dec_ctx.avc_ctx).last_gop_maxtref as f64 > dec_ctx.last_gop_length as f64 * 1.5 {
                (*dec_ctx.timing).current_tref /= 2;
            }
        } else {
            (*dec_ctx.timing).current_tref = 0;
        }

        if (*dec_ctx.timing).current_tref < 0 {
            info!("current_tref is negative!?");
        }
    } else {
        // Track minimum index for this GOP
        if current_index < (*dec_ctx.avc_ctx).minidx {
            (*dec_ctx.avc_ctx).minidx = current_index;
        }

        (*dec_ctx.timing).current_tref = 1;
        if current_index == (*dec_ctx.avc_ctx).lastminidx {
            // This implies that the minimal index (assuming its number is
            // fairly constant) sets the temporal reference to zero - needed to set sync_pts.
            (*dec_ctx.timing).current_tref = 0;
        }
        if (*dec_ctx.avc_ctx).lastmaxidx == -1 {
            // Set temporal reference to zero on minimal index and in the first GOP
            // to avoid setting a wrong fts_offset
            (*dec_ctx.timing).current_tref = 0;
        }
    }

    ccxr_set_fts(dec_ctx.timing); // Keep frames_since_ref_time==0, use current_tref

    debug!(msg_type = DebugMessageFlag::TIME; "  picordercnt:{:3} tref:{:3} idx:{:3} refidx:{:3} lmaxidx:{:3} maxtref:{:3}",
           pic_order_cnt_lsb, (*dec_ctx.timing).current_tref,
           current_index, (*dec_ctx.avc_ctx).currref, (*dec_ctx.avc_ctx).lastmaxidx, (*dec_ctx.avc_ctx).maxtref);

    let mut buf = [c_char::from(0i8); 64];
    debug!(
        msg_type = DebugMessageFlag::TIME;
        "  sync_pts:{} ({:8})",
        std::ffi::CStr::from_ptr(ccxr_print_mstime_static(
            ((*dec_ctx.timing).sync_pts / ((MPEG_CLOCK_FREQ as i64) / 1000i64)) as c_long,
            buf.as_mut_ptr()
        ))
        .to_str()
        .unwrap_or(""),
        (*dec_ctx.timing).sync_pts as u32
    );

    debug!(msg_type = DebugMessageFlag::TIME; " - {} since GOP: {:2}",
           SLICE_TYPES[slice_type as usize],
           dec_ctx.frames_since_last_gop as u32);

    debug!(msg_type = DebugMessageFlag::TIME; "  b:{}  frame# {}", bottom_field_flag, (*dec_ctx.avc_ctx).frame_num);

    // sync_pts is (was) set when current_tref was zero
    if (*dec_ctx.avc_ctx).lastmaxidx > -1 && (*dec_ctx.timing).current_tref == 0 {
        debug!(msg_type = DebugMessageFlag::TIME; "New temporal reference:");
        ccxr_print_debug_timing(dec_ctx.timing);
    }

    total_frames_count += 1;
    dec_ctx.frames_since_last_gop += 1;

    store_hdcc(
        enc_ctx,
        dec_ctx,
        (*dec_ctx.avc_ctx).cc_data,
        (*dec_ctx.avc_ctx).cc_count as i32,
        current_index,
        (*dec_ctx.timing).fts_now,
        sub,
    );

    (*dec_ctx.avc_ctx).cc_buffer_saved = 1; // store_hdcc supposedly saves the CC buffer to a sequence buffer
    (*dec_ctx.avc_ctx).cc_count = 0;

    Ok(())
}
