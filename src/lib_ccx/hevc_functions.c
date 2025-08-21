#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "utility.h"
#include <math.h>
#include "hevc_functions.h"

#define hvprint(...) dbg_print(CCX_DMT_VIDES, __VA_ARGS__)

static void parse_hevc_cc_sei(struct hevc_ctx *ctx, unsigned char *cc_buf, unsigned char *cc_end);
static void copy_hevc_cc_data(struct hevc_ctx *ctx, char *source, int cc_count);

// HEVC/H.265 NAL Unit Types (ISO/IEC 23008-2)
enum ccx_hevc_nal_types
{
    // VCL NAL unit types
    CCX_HEVC_NAL_TRAIL_N = 0,           // Coded slice segment of a non-TSA, non-STSA trailing picture
    CCX_HEVC_NAL_TRAIL_R = 1,           // Coded slice segment of a non-TSA, non-STSA trailing picture
    CCX_HEVC_NAL_TSA_N = 2,             // Coded slice segment of a TSA picture
    CCX_HEVC_NAL_TSA_R = 3,             // Coded slice segment of a TSA picture
    CCX_HEVC_NAL_STSA_N = 4,            // Coded slice segment of an STSA picture
    CCX_HEVC_NAL_STSA_R = 5,            // Coded slice segment of an STSA picture
    CCX_HEVC_NAL_RADL_N = 6,            // Coded slice segment of a RADL picture
    CCX_HEVC_NAL_RADL_R = 7,            // Coded slice segment of a RADL picture
    CCX_HEVC_NAL_RASL_N = 8,            // Coded slice segment of a RASL picture
    CCX_HEVC_NAL_RASL_R = 9,            // Coded slice segment of a RASL picture
    CCX_HEVC_NAL_BLA_W_LP = 16,         // Coded slice segment of a BLA picture
    CCX_HEVC_NAL_BLA_W_RADL = 17,       // Coded slice segment of a BLA picture
    CCX_HEVC_NAL_BLA_N_LP = 18,         // Coded slice segment of a BLA picture
    CCX_HEVC_NAL_IDR_W_RADL = 19,       // Coded slice segment of an IDR picture
    CCX_HEVC_NAL_IDR_N_LP = 20,         // Coded slice segment of an IDR picture
    CCX_HEVC_NAL_CRA_NUT = 21,          // Coded slice segment of a CRA picture
    
    // Non-VCL NAL unit types
    CCX_HEVC_NAL_VPS = 32,              // Video parameter set
    CCX_HEVC_NAL_SPS = 33,              // Sequence parameter set
    CCX_HEVC_NAL_PPS = 34,              // Picture parameter set
    CCX_HEVC_NAL_AUD = 35,              // Access unit delimiter
    CCX_HEVC_NAL_EOS_NUT = 36,          // End of sequence
    CCX_HEVC_NAL_EOB_NUT = 37,          // End of bitstream
    CCX_HEVC_NAL_FD_NUT = 38,           // Filler data
    CCX_HEVC_NAL_PREFIX_SEI = 39,       // Prefix SEI
    CCX_HEVC_NAL_SUFFIX_SEI = 40        // Suffix SEI
};

// HEVC Context Structure
struct hevc_ctx
{
    // Caption data buffer
    unsigned char *cc_data;
    long cc_databufsize;
    int cc_count;
    int cc_buffer_saved;
    
    // Parameter sets
    int got_vps;
    int got_sps; 
    int got_pps;
    
    // Video parameter set data
    int vps_max_layers_minus1;
    int vps_max_sub_layers_minus1;
    int vps_temporal_id_nesting_flag;
    
    // Sequence parameter set data  
    int sps_video_parameter_set_id;
    int sps_max_sub_layers_minus1;
    int sps_temporal_id_nesting_flag;
    int pic_width_in_luma_samples;
    int pic_height_in_luma_samples;
    int log2_max_pic_order_cnt_lsb_minus4;
    int log2_max_pic_order_cnt_lsb;
    
    // Picture parameter set data
    int pps_pic_parameter_set_id;
    int pps_seq_parameter_set_id;
    
    // Frame tracking
    int frame_num;
    int lastframe_num;
    LLONG last_pic_order_cnt_lsb;
    LLONG last_slice_pts;
    
    // Statistics
    int ccblocks_in_hevc_total;
    int ccblocks_in_hevc_lost;
    int num_jump_in_frames;
    int num_unexpected_sei_length;
};

//=============================================================================
// FUNCTION: init_hevc
// PURPOSE: Initialize HEVC context structure
// RETURNS: Pointer to initialized HEVC context or NULL on failure
//=============================================================================
struct hevc_ctx *init_hevc(void)
{
    struct hevc_ctx *ctx = malloc(sizeof(struct hevc_ctx));
    if (!ctx)
        return NULL;
        
    // Allocate CC data buffer
    ctx->cc_data = (unsigned char *)malloc(1024);
    if (!ctx->cc_data)
    {
        free(ctx);
        return NULL;
    }
    
    // Initialize CC data fields
    ctx->cc_count = 0;
    ctx->cc_databufsize = 1024;
    ctx->cc_buffer_saved = CCX_TRUE;
    
    // Initialize parameter set flags
    ctx->got_vps = 0;
    ctx->got_sps = 0;
    ctx->got_pps = 0;
    
    // Initialize VPS data
    ctx->vps_max_layers_minus1 = 0;
    ctx->vps_max_sub_layers_minus1 = 0;
    ctx->vps_temporal_id_nesting_flag = 0;
    
    // Initialize SPS data
    ctx->sps_video_parameter_set_id = 0;
    ctx->sps_max_sub_layers_minus1 = 0;
    ctx->sps_temporal_id_nesting_flag = 0;
    ctx->pic_width_in_luma_samples = 0;
    ctx->pic_height_in_luma_samples = 0;
    ctx->log2_max_pic_order_cnt_lsb_minus4 = 0;
    ctx->log2_max_pic_order_cnt_lsb = 4; // Default minimum value
    
    // Initialize PPS data
    ctx->pps_pic_parameter_set_id = 0;
    ctx->pps_seq_parameter_set_id = 0;
    
    // Initialize frame tracking
    ctx->frame_num = -1;
    ctx->lastframe_num = -1;
    ctx->last_pic_order_cnt_lsb = -1;
    ctx->last_slice_pts = -1;
    
    // Initialize statistics
    ctx->ccblocks_in_hevc_total = 0;
    ctx->ccblocks_in_hevc_lost = 0;
    ctx->num_jump_in_frames = 0;
    ctx->num_unexpected_sei_length = 0;
    
    return ctx;
}

//=============================================================================
// FUNCTION: dinit_hevc
// PURPOSE: Cleanup and free HEVC context
// PARAMETERS: ctx - Pointer to HEVC context pointer
//=============================================================================
void dinit_hevc(struct hevc_ctx **ctx)
{
    struct hevc_ctx *lctx = *ctx;
    if (!lctx)
        return;
        
    // Print statistics if any CC blocks were lost
    if (lctx->ccblocks_in_hevc_lost > 0)
    {
        mprint("Total caption blocks received (HEVC): %d\n", lctx->ccblocks_in_hevc_total);
        mprint("Total caption blocks lost (HEVC): %d\n", lctx->ccblocks_in_hevc_lost);
    }
    
    // Free allocated memory
    freep(&lctx->cc_data);
    freep(ctx);
}

//=============================================================================
// FUNCTION: remove_hevc_emulation_prevention
// PURPOSE: Remove emulation prevention bytes (0x000003) from HEVC NAL units
// PARAMETERS: from - Source buffer, to - End of source buffer
// RETURNS: Pointer to end of processed data or NULL on error
//=============================================================================
static unsigned char *remove_hevc_emulation_prevention(unsigned char *from, unsigned char *to)
{
    unsigned char *src = from;
    unsigned char *dst = from;
    int consecutive_zeros = 0;
    
    while (src < to)
    {
        if (consecutive_zeros == 2 && *src == 0x03)
        {
            // Skip emulation prevention byte
            src++;
            consecutive_zeros = 0;
            if (src >= to)
                break;
        }
        
        *dst = *src;
        
        if (*src == 0x00)
            consecutive_zeros++;
        else
            consecutive_zeros = 0;
            
        src++;
        dst++;
    }
    
    return dst;
}

//=============================================================================
// FUNCTION: parse_hevc_vps
// PURPOSE: Parse HEVC Video Parameter Set
// PARAMETERS: ctx - HEVC context, vps_buf - VPS data buffer, vps_end - End of buffer
//=============================================================================
static void parse_hevc_vps(struct hevc_ctx *ctx, unsigned char *vps_buf, unsigned char *vps_end)
{
    struct bitstream bs;
    if (init_bitstream(&bs, vps_buf, vps_end))
    {
        mprint("Failed to initialize bitstream for HEVC VPS parsing.\n");
        return;
    }
    
    hvprint("HEVC VIDEO PARAMETER SET\n");
    
    // vps_video_parameter_set_id (4 bits)
    LLONG vps_id = read_int_unsigned(&bs, 4);
    hvprint("vps_video_parameter_set_id = %lld\n", vps_id);
    
    // vps_base_layer_internal_flag (1 bit)
    LLONG base_layer_internal_flag = read_int_unsigned(&bs, 1);
    hvprint("vps_base_layer_internal_flag = %lld\n", base_layer_internal_flag);
    
    // vps_base_layer_available_flag (1 bit) 
    LLONG base_layer_available_flag = read_int_unsigned(&bs, 1);
    hvprint("vps_base_layer_available_flag = %lld\n", base_layer_available_flag);
    
    // vps_max_layers_minus1 (6 bits)
    ctx->vps_max_layers_minus1 = (int)read_int_unsigned(&bs, 6);
    hvprint("vps_max_layers_minus1 = %d\n", ctx->vps_max_layers_minus1);
    
    // vps_max_sub_layers_minus1 (3 bits)
    ctx->vps_max_sub_layers_minus1 = (int)read_int_unsigned(&bs, 3);
    hvprint("vps_max_sub_layers_minus1 = %d\n", ctx->vps_max_sub_layers_minus1);
    
    // vps_temporal_id_nesting_flag (1 bit)
    ctx->vps_temporal_id_nesting_flag = (int)read_int_unsigned(&bs, 1);
    hvprint("vps_temporal_id_nesting_flag = %d\n", ctx->vps_temporal_id_nesting_flag);
    
    // vps_reserved_0xffff_16bits (16 bits)
    LLONG reserved = read_int_unsigned(&bs, 16);
    hvprint("vps_reserved_0xffff_16bits = 0x%04llX\n", reserved);
    
    ctx->got_vps = 1;
    hvprint("HEVC VPS parsed successfully\n");
}

//=============================================================================
// FUNCTION: parse_hevc_sps  
// PURPOSE: Parse HEVC Sequence Parameter Set
// PARAMETERS: ctx - HEVC context, sps_buf - SPS data buffer, sps_end - End of buffer
//=============================================================================
static void parse_hevc_sps(struct hevc_ctx *ctx, unsigned char *sps_buf, unsigned char *sps_end)
{
    struct bitstream bs;
    if (init_bitstream(&bs, sps_buf, sps_end))
    {
        mprint("Failed to initialize bitstream for HEVC SPS parsing.\n");
        return;
    }
    
    hvprint("HEVC SEQUENCE PARAMETER SET\n");
    
    // sps_video_parameter_set_id (4 bits)
    ctx->sps_video_parameter_set_id = (int)read_int_unsigned(&bs, 4);
    hvprint("sps_video_parameter_set_id = %d\n", ctx->sps_video_parameter_set_id);
    
    // sps_max_sub_layers_minus1 (3 bits)
    ctx->sps_max_sub_layers_minus1 = (int)read_int_unsigned(&bs, 3);
    hvprint("sps_max_sub_layers_minus1 = %d\n", ctx->sps_max_sub_layers_minus1);
    
    // sps_temporal_id_nesting_flag (1 bit)
    ctx->sps_temporal_id_nesting_flag = (int)read_int_unsigned(&bs, 1);
    hvprint("sps_temporal_id_nesting_flag = %d\n", ctx->sps_temporal_id_nesting_flag);
    
    // Skip profile_tier_level structure for now
    // This is complex and not needed for CC extraction
    
    // sps_seq_parameter_set_id (ue(v))
    LLONG sps_id = read_exp_golomb_unsigned(&bs);
    hvprint("sps_seq_parameter_set_id = %lld\n", sps_id);
    
    // chroma_format_idc (ue(v))
    LLONG chroma_format = read_exp_golomb_unsigned(&bs);
    hvprint("chroma_format_idc = %lld\n", chroma_format);
    
    if (chroma_format == 3)
    {
        // separate_colour_plane_flag (1 bit)
        LLONG separate_colour = read_int_unsigned(&bs, 1);
        hvprint("separate_colour_plane_flag = %lld\n", separate_colour);
    }
    
    // pic_width_in_luma_samples (ue(v))
    ctx->pic_width_in_luma_samples = (int)read_exp_golomb_unsigned(&bs);
    hvprint("pic_width_in_luma_samples = %d\n", ctx->pic_width_in_luma_samples);
    
    // pic_height_in_luma_samples (ue(v))
    ctx->pic_height_in_luma_samples = (int)read_exp_golomb_unsigned(&bs);
    hvprint("pic_height_in_luma_samples = %d\n", ctx->pic_height_in_luma_samples);
    
    // conformance_window_flag (1 bit)
    LLONG conformance_window = read_int_unsigned(&bs, 1);
    hvprint("conformance_window_flag = %lld\n", conformance_window);
    
    if (conformance_window)
    {
        // Skip conformance window parameters
        LLONG left_offset = read_exp_golomb_unsigned(&bs);
        LLONG right_offset = read_exp_golomb_unsigned(&bs);
        LLONG top_offset = read_exp_golomb_unsigned(&bs);
        LLONG bottom_offset = read_exp_golomb_unsigned(&bs);
        hvprint("Conformance window: left=%lld right=%lld top=%lld bottom=%lld\n",
               left_offset, right_offset, top_offset, bottom_offset);
    }
    
    // bit_depth_luma_minus8 (ue(v))
    LLONG bit_depth_luma = read_exp_golomb_unsigned(&bs);
    hvprint("bit_depth_luma_minus8 = %lld\n", bit_depth_luma);
    
    // bit_depth_chroma_minus8 (ue(v))
    LLONG bit_depth_chroma = read_exp_golomb_unsigned(&bs);
    hvprint("bit_depth_chroma_minus8 = %lld\n", bit_depth_chroma);
    
    // log2_max_pic_order_cnt_lsb_minus4 (ue(v))
    ctx->log2_max_pic_order_cnt_lsb_minus4 = (int)read_exp_golomb_unsigned(&bs);
    ctx->log2_max_pic_order_cnt_lsb = ctx->log2_max_pic_order_cnt_lsb_minus4 + 4;
    hvprint("log2_max_pic_order_cnt_lsb_minus4 = %d (actual = %d)\n",
           ctx->log2_max_pic_order_cnt_lsb_minus4, ctx->log2_max_pic_order_cnt_lsb);
    
    ctx->got_sps = 1;
    hvprint("HEVC SPS parsed successfully\n");
}

//=============================================================================
// FUNCTION: parse_hevc_pps
// PURPOSE: Parse HEVC Picture Parameter Set
// PARAMETERS: ctx - HEVC context, pps_buf - PPS data buffer, pps_end - End of buffer
//=============================================================================
static void parse_hevc_pps(struct hevc_ctx *ctx, unsigned char *pps_buf, unsigned char *pps_end)
{
    struct bitstream bs;
    if (init_bitstream(&bs, pps_buf, pps_end))
    {
        mprint("Failed to initialize bitstream for HEVC PPS parsing.\n");
        return;
    }
    
    hvprint("HEVC PICTURE PARAMETER SET\n");
    
    // pps_pic_parameter_set_id (ue(v))
    ctx->pps_pic_parameter_set_id = (int)read_exp_golomb_unsigned(&bs);
    hvprint("pps_pic_parameter_set_id = %d\n", ctx->pps_pic_parameter_set_id);
    
    // pps_seq_parameter_set_id (ue(v))
    ctx->pps_seq_parameter_set_id = (int)read_exp_golomb_unsigned(&bs);
    hvprint("pps_seq_parameter_set_id = %d\n", ctx->pps_seq_parameter_set_id);
    
    // Skip other PPS parameters for now - they're not needed for CC extraction
    
    ctx->got_pps = 1;
    hvprint("HEVC PPS parsed successfully\n");
}

//=============================================================================
// FUNCTION: parse_hevc_sei
// PURPOSE: Parse HEVC Supplemental Enhancement Information
// PARAMETERS: ctx - HEVC context, sei_buf - SEI data buffer, sei_end - End of buffer
//=============================================================================
static void parse_hevc_sei(struct hevc_ctx *ctx, unsigned char *sei_buf, unsigned char *sei_end)
{
    unsigned char *tbuf = sei_buf;
    
    hvprint("HEVC SEI MESSAGE\n");
    
    while (tbuf < sei_end - 1) // -1 for trailing byte
    {
        // Parse SEI message header
        int payload_type = 0;
        while (*tbuf == 0xff)
        {
            payload_type += 255;
            tbuf++;
            if (tbuf >= sei_end) return;
        }
        payload_type += *tbuf;
        tbuf++;
        
        int payload_size = 0;
        while (*tbuf == 0xff)
        {
            payload_size += 255;
            tbuf++;
            if (tbuf >= sei_end) return;
        }
        payload_size += *tbuf;
        tbuf++;
        
        hvprint("SEI payload_type = %d, payload_size = %d\n", payload_type, payload_size);
        
        unsigned char *payload_start = tbuf;
        tbuf += payload_size;
        
        if (tbuf > sei_end)
        {
            mprint("Warning: HEVC SEI payload extends beyond buffer\n");
            ctx->num_unexpected_sei_length++;
            break;
        }
        
        // Process closed caption SEI messages (payload_type 4 for user_data_registered_itu_t_t35)
        if (payload_type == 4)
        {
            parse_hevc_cc_sei(ctx, payload_start, payload_start + payload_size);
        }
        // Handle other SEI message types as needed
    }
}

//=============================================================================
// FUNCTION: parse_hevc_cc_sei
// PURPOSE: Parse closed caption data from HEVC SEI message
// PARAMETERS: ctx - HEVC context, cc_buf - CC data buffer, cc_end - End of buffer  
//=============================================================================
static void parse_hevc_cc_sei(struct hevc_ctx *ctx, unsigned char *cc_buf, unsigned char *cc_end)
{
    unsigned char *tbuf = cc_buf;
    
    if (tbuf + 3 > cc_end)
        return;
        
    // Check for ITU-T T.35 country code (0xB5 for North America)
    int country_code = *tbuf++;
    if (country_code != 0xB5)
    {
        hvprint("Unsupported country code in HEVC CC SEI: 0x%02X\n", country_code);
        return;
    }
    
    // Provider code (2 bytes)
    int provider_code = (*tbuf << 8) | *(tbuf + 1);
    tbuf += 2;
    
    hvprint("HEVC CC SEI: country_code=0x%02X, provider_code=0x%04X\n", 
           country_code, provider_code);
    
    // Handle different provider codes
    switch (provider_code)
    {
        case 0x002F: // Direct TV
        case 0x0031: // ATSC
        {
            if (tbuf + 4 > cc_end)
                return;
                
            // Check for ATSC identifier "GA94"
            if (tbuf[0] == 'G' && tbuf[1] == 'A' && tbuf[2] == '9' && tbuf[3] == '4')
            {
                tbuf += 4;
                if (tbuf >= cc_end)
                    return;
                    
                // user_data_type_code
                int user_data_type = *tbuf++;
                hvprint("user_data_type_code = 0x%02X\n", user_data_type);
                
                if (user_data_type == 0x03) // cc_data
                {
                    if (tbuf + 2 > cc_end)
                        return;
                        
                    // cc_data structure
                    int cc_count = *tbuf & 0x1F;
                    int process_cc_data_flag = (*tbuf & 0x40) >> 6;
                    tbuf += 2; // Skip cc_count byte and reserved byte
                    
                    hvprint("cc_count = %d, process_flag = %d\n", cc_count, process_cc_data_flag);
                    
                    if (!process_cc_data_flag)
                    {
                        hvprint("process_cc_data_flag is 0, skipping\n");
                        return;
                    }
                    
                    if (tbuf + cc_count * 3 > cc_end)
                    {
                        mprint("Warning: HEVC CC data extends beyond buffer\n");
                        return;
                    }
                    
                    // Check for end marker
                    if (tbuf[cc_count * 3] != 0xFF)
                    {
                        mprint("Warning: HEVC CC end marker missing\n");
                        return;
                    }
                    
                    // Store CC data
                    copy_hevc_cc_data(ctx, (char *)tbuf, cc_count);
                }
            }
            break;
        }
        default:
            hvprint("Unsupported HEVC CC provider code: 0x%04X\n", provider_code);
            break;
    }
}

//=============================================================================
// FUNCTION: copy_hevc_cc_data
// PURPOSE: Copy closed caption data to HEVC context buffer
// PARAMETERS: ctx - HEVC context, source - CC data, cc_count - Number of CC triplets
//=============================================================================
static void copy_hevc_cc_data(struct hevc_ctx *ctx, char *source, int cc_count)
{
    ctx->ccblocks_in_hevc_total++;
    
    if (ctx->cc_buffer_saved == CCX_FALSE)
    {
        mprint("Warning: HEVC CC data loss, unsaved buffer being overwritten\n");
        ctx->ccblocks_in_hevc_lost++;
    }
    
    // Ensure buffer is large enough
    int needed_size = (ctx->cc_count + cc_count) * 3 + 1;
    if (needed_size > ctx->cc_databufsize)
    {
        ctx->cc_data = (unsigned char *)realloc(ctx->cc_data, needed_size * 2);
        if (!ctx->cc_data)
        {
            fatal(EXIT_NOT_ENOUGH_MEMORY, "Failed to reallocate HEVC CC buffer");
            return;
        }
        ctx->cc_databufsize = needed_size * 2;
    }
    
    // Copy CC data (cc_count * 3 bytes + 1 end marker)
    memcpy(ctx->cc_data + ctx->cc_count * 3, source, cc_count * 3 + 1);
    ctx->cc_count += cc_count;
    ctx->cc_buffer_saved = CCX_FALSE;
    
    hvprint("Copied %d CC triplets to HEVC buffer (total: %d)\n", cc_count, ctx->cc_count);
}

//=============================================================================
// FUNCTION: parse_hevc_slice_header
// PURPOSE: Parse HEVC slice header for timing information
// PARAMETERS: enc_ctx - Encoder context, dec_ctx - Decoder context, slice_buf - Slice data, 
//            slice_end - End of buffer, nal_unit_type - NAL unit type, sub - Subtitle output
//=============================================================================
static void parse_hevc_slice_header(struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx, 
                                   unsigned char *slice_buf, unsigned char *slice_end, 
                                   int nal_unit_type, struct cc_subtitle *sub)
{
    struct bitstream bs;
    if (init_bitstream(&bs, slice_buf, slice_end))
    {
        mprint("Failed to initialize bitstream for HEVC slice header parsing.\n");
        return;
    }
    
    struct hevc_ctx *ctx = (struct hevc_ctx *)dec_ctx->private_data;
    if (!ctx || !ctx->got_sps)
    {
        hvprint("Cannot parse HEVC slice header: missing context or SPS\n");
        return;
    }
    
    hvprint("HEVC SLICE HEADER (NAL type %d)\n", nal_unit_type);
    
    // first_slice_segment_in_pic_flag (1 bit)
    int first_slice = (int)read_int_unsigned(&bs, 1);
    hvprint("first_slice_segment_in_pic_flag = %d\n", first_slice);
    
    // Check if this is an IRAP (Intra Random Access Point) picture
    int is_irap = (nal_unit_type >= CCX_HEVC_NAL_BLA_W_LP && nal_unit_type <= CCX_HEVC_NAL_CRA_NUT);
    
    if (is_irap)
    {
        // no_output_of_prior_pics_flag (1 bit)
        int no_output = (int)read_int_unsigned(&bs, 1);
        hvprint("no_output_of_prior_pics_flag = %d\n", no_output);
    }
    
    // slice_pic_parameter_set_id (ue(v))
    LLONG pps_id = read_exp_golomb_unsigned(&bs);
    hvprint("slice_pic_parameter_set_id = %lld\n", pps_id);
    
    // Parse slice_pic_order_cnt_lsb if not first slice
    LLONG pic_order_cnt_lsb = 0;
    if (!first_slice)
    {
        // Skip dependent_slice_segment_flag and other fields for now
        // In a full implementation, these would need to be parsed
    }
    else
    {
        // This is the first slice in the picture
        if (!is_irap)
        {
            // slice_pic_order_cnt_lsb (v bits, where v = log2_max_pic_order_cnt_lsb_minus4 + 4)
            pic_order_cnt_lsb = read_int_unsigned(&bs, ctx->log2_max_pic_order_cnt_lsb);
            hvprint("slice_pic_order_cnt_lsb = %lld\n", pic_order_cnt_lsb);
        }
        
        // Check if we should process this slice
        if (ccx_options.usepicorder)
        {
            if (ctx->last_pic_order_cnt_lsb == pic_order_cnt_lsb)
            {
                hvprint("Skipping duplicate POC slice\n");
                return; // Skip duplicate POC
            }
            ctx->last_pic_order_cnt_lsb = pic_order_cnt_lsb;
        }
        else
        {
            if (dec_ctx->timing->current_pts == ctx->last_slice_pts)
            {
                hvprint("Skipping duplicate PTS slice\n");
                return; // Skip duplicate PTS
            }
            ctx->last_slice_pts = dec_ctx->timing->current_pts;
        }
        
        // Set timing information
        if (ccx_options.usepicorder)
        {
            dec_ctx->timing->current_tref = (int)pic_order_cnt_lsb;
        }
        else
        {
            // Use PTS-based timing
            dec_ctx->timing->current_tref = 0;
        }
        
        // Update frame timing
        set_fts(dec_ctx->timing);
        
        hvprint("HEVC slice: POC=%lld, PTS=%lld, tref=%d\n", 
               pic_order_cnt_lsb, dec_ctx->timing->current_pts, dec_ctx->timing->current_tref);
        
        // Process accumulated CC data if this is a reference picture
        if (first_slice && ctx->cc_count > 0)
        {
            // Store CC data for processing
            store_hdcc(enc_ctx, dec_ctx, ctx->cc_data, ctx->cc_count, 0, dec_ctx->timing->fts_now, sub);
            ctx->cc_buffer_saved = CCX_TRUE;
            ctx->cc_count = 0;
        }
    }
}

//=============================================================================
// FUNCTION: do_hevc_nal
// PURPOSE: Process individual HEVC NAL units
// PARAMETERS: enc_ctx - Encoder context, dec_ctx - Decoder context, 
//            nal_start - Start of NAL, nal_length - Length of NAL, sub - Subtitle output
//=============================================================================
void do_hevc_nal(struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx, 
                 unsigned char *nal_start, LLONG nal_length, struct cc_subtitle *sub)
{
    if (nal_length < 2)
        return;
        
    unsigned char *nal_stop;
    
    // Extract NAL unit header (2 bytes in HEVC)
    int forbidden_zero_bit = (nal_start[0] & 0x80) >> 7;
    enum ccx_hevc_nal_types nal_unit_type = (enum ccx_hevc_nal_types)(((int)nal_start[0] & 0x7E) >> 1);
    int nuh_layer_id = ((nal_start[0] & 0x01) << 5) | ((nal_start[1] & 0xF8) >> 3);    
    int nuh_temporal_id_plus1 = nal_start[1] & 0x07;
    
    if (forbidden_zero_bit)
    {
        mprint("Warning: HEVC NAL forbidden_zero_bit is set\n");
        return;
    }
    
    nal_stop = nal_start + nal_length;
    nal_stop = remove_hevc_emulation_prevention(nal_start + 2, nal_stop); // Skip 2-byte header
    
    hvprint("HEVC NAL: type=%d, layer=%d, temporal=%d, length=%lld\n",
           nal_unit_type, nuh_layer_id, nuh_temporal_id_plus1, nal_stop - nal_start - 2);
    
    if (nal_stop == NULL)
    {
        mprint("Warning: HEVC emulation prevention removal failed\n");
        return;
    }
    
    struct hevc_ctx *ctx = (struct hevc_ctx *)dec_ctx->private_data;
    if (!ctx)
    {
        mprint("Error: HEVC context not initialized\n");
        return;
    }
    
    // Process different NAL unit types
    switch (nal_unit_type)
    {
        case CCX_HEVC_NAL_VPS: // Video Parameter Set
            hvprint("Processing HEVC VPS\n");
            parse_hevc_vps(ctx, nal_start + 2, nal_stop);
            break;
            
        case CCX_HEVC_NAL_SPS: // Sequence Parameter Set
            hvprint("Processing HEVC SPS\n");
            parse_hevc_sps(ctx, nal_start + 2, nal_stop);
            break;
            
        case CCX_HEVC_NAL_PPS: // Picture Parameter Set
            hvprint("Processing HEVC PPS\n");
            parse_hevc_pps(ctx, nal_start + 2, nal_stop);
            break;
            
        case CCX_HEVC_NAL_AUD: // Access Unit Delimiter
            hvprint("HEVC Access Unit Delimiter\n");
            break;
            
        case CCX_HEVC_NAL_PREFIX_SEI: // Prefix SEI
        case CCX_HEVC_NAL_SUFFIX_SEI: // Suffix SEI
            hvprint("Processing HEVC SEI (type %s)\n", 
                   (nal_unit_type == CCX_HEVC_NAL_PREFIX_SEI) ? "PREFIX" : "SUFFIX");
            parse_hevc_sei(ctx, nal_start + 2, nal_stop);
            break;
            
        // VCL NAL units (coded slices)
        case CCX_HEVC_NAL_TRAIL_N:
        case CCX_HEVC_NAL_TRAIL_R:
        case CCX_HEVC_NAL_TSA_N:
        case CCX_HEVC_NAL_TSA_R:
        case CCX_HEVC_NAL_STSA_N:
        case CCX_HEVC_NAL_STSA_R:
        case CCX_HEVC_NAL_RADL_N:
        case CCX_HEVC_NAL_RADL_R:
        case CCX_HEVC_NAL_RASL_N:
        case CCX_HEVC_NAL_RASL_R:
        case CCX_HEVC_NAL_BLA_W_LP:
        case CCX_HEVC_NAL_BLA_W_RADL:
        case CCX_HEVC_NAL_BLA_N_LP:
        case CCX_HEVC_NAL_IDR_W_RADL:
        case CCX_HEVC_NAL_IDR_N_LP:
        case CCX_HEVC_NAL_CRA_NUT:
            if (ctx->got_sps && ctx->got_pps)
            {
                hvprint("Processing HEVC slice (type %d)\n", nal_unit_type);
                parse_hevc_slice_header(enc_ctx, dec_ctx, nal_start + 2, nal_stop, nal_unit_type, sub);
            }
            else
            {
                hvprint("Skipping HEVC slice - missing parameter sets\n");
            }
            break;
            
        default:
            hvprint("Unhandled HEVC NAL unit type: %d\n", nal_unit_type);
            break;
    }
}

//=============================================================================
// FUNCTION: process_hevc
// PURPOSE: Main HEVC processing function - finds and processes NAL units
// PARAMETERS: enc_ctx - Encoder context, dec_ctx - Decoder context,
//            hevc_buf - HEVC data buffer, hevc_buflen - Buffer length, sub - Subtitle output
// RETURNS: Number of bytes processed
//=============================================================================
size_t process_hevc(struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx,
                   unsigned char *hevc_buf, size_t hevc_buflen, struct cc_subtitle *sub)
{
    unsigned char *buffer_pos = hevc_buf;
    unsigned char *nal_start;
    unsigned char *nal_stop;
    
    // Minimum NAL unit needs at least 6 bytes (start code + 2-byte header + data)
    if (hevc_buflen <= 6)
    {
        mprint("HEVC buffer too small: %zu bytes\n", hevc_buflen);
        return 0;
    }
    
    hvprint("Processing HEVC buffer: %zu bytes\n", hevc_buflen);
    
    // Initialize HEVC context if needed
    if (!dec_ctx->private_data)
    {
        dec_ctx->private_data = init_hevc();
        if (!dec_ctx->private_data)
        {
            fatal(EXIT_NOT_ENOUGH_MEMORY, "Failed to initialize HEVC context");
            return 0;
        }
    }
    
    // Verify start code (0x000001 or 0x00000001)
    if (!(buffer_pos[0] == 0x00 && buffer_pos[1] == 0x00))
    {
        mprint("Invalid HEVC start code\n");
        return 0;
    }
    buffer_pos += 2;
    
    int first_nal = 1;
    
    // Process NAL units
    while (buffer_pos < hevc_buf + hevc_buflen - 3)
    {
        // Find start code (0x000001)
        while (buffer_pos < hevc_buf + hevc_buflen)
        {
            if (*buffer_pos == 0x01)
            {
                break; // Found start code
            }
            else if (first_nal && *buffer_pos != 0x00)
            {
                mprint("Invalid HEVC start sequence\n");
                return 0;
            }
            buffer_pos++;
        }
        
        first_nal = 0;
        
        if (buffer_pos >= hevc_buf + hevc_buflen)
            break; // No more start codes
            
        nal_start = buffer_pos + 1; // Skip 0x01
        
        // Find next start code or end of buffer
        buffer_pos++;
        LLONG remaining = hevc_buf + hevc_buflen - buffer_pos - 2;
        
        while (remaining > 0)
        {
            // Look for next 0x00
            buffer_pos = (unsigned char *)memchr(buffer_pos, 0x00, (size_t)remaining);
            
            if (!buffer_pos)
            {
                // No more zeros, end of data
                nal_stop = hevc_buf + hevc_buflen;
                buffer_pos = nal_stop;
                break;
            }
            
            // Check if this is start of next NAL (0x000001)
            if (buffer_pos[1] == 0x00 && (buffer_pos[2] | 0x01) == 0x01)
            {
                nal_stop = buffer_pos;
                buffer_pos += 2; // Position after 0x0000
                break;
            }
            
            buffer_pos++;
            remaining = hevc_buf + hevc_buflen - buffer_pos - 2;
        }
        
        if (remaining <= 0)
        {
            nal_stop = hevc_buf + hevc_buflen;
            buffer_pos = nal_stop;
        }
        
        // Process this NAL unit
        LLONG nal_length = nal_stop - nal_start;
        if (nal_length >= 2) // Minimum for HEVC NAL header
        {
            do_hevc_nal(enc_ctx, dec_ctx, nal_start, nal_length, sub);
        }
    }
    
    return hevc_buflen;
}

