/* Functions used by both the 608 and 708 decoders. An effort should be
made to reuse, not duplicate, as many functions as possible */

#include "ccx_decoders_common.h"
#include "ccx_common_structs.h"
#include "ccx_common_char_encoding.h"
#include "ccx_common_constants.h"
#include "ccx_common_timing.h"
#include "ccx_common_common.h"
#include "lib_ccx.h"
#include "ccx_decoders_608.h"
#include "ccx_decoders_708.h"
#include "ccx_decoders_xds.h"
#include "ccx_decoders_vbi.h"
#include "ccx_encoders_mcc.h"
#include "ccx_dtvcc.h"

extern int ccxr_process_cc_data(struct lib_cc_decode *dec_ctx, unsigned char *cc_data, int cc_count);
extern void ccxr_flush_decoder(struct dtvcc_ctx *dtvcc, struct dtvcc_service_decoder *decoder);

uint64_t utc_refvalue = UINT64_MAX; /* _UI64_MAX/UINT64_MAX means don't use UNIX, 0 = use current system time as reference, +1 use a specific reference */
extern int in_xds_mode;

LLONG ccxr_get_visible_start(struct ccx_common_timing_ctx *ctx, int current_field);
LLONG ccxr_get_visible_end(struct ccx_common_timing_ctx *ctx, int current_field);

/* This function returns a FTS that is guaranteed to be at least 1 ms later than the end of the previous screen. It shouldn't be needed
   obviously but it guarantees there's no timing overlap */
LLONG get_visible_start(struct ccx_common_timing_ctx *ctx, int current_field)
{
	LLONG fts = ccxr_get_visible_start(ctx, current_field);
	ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "Visible Start time=%s\n", print_mstime_static(fts));
	return fts;
}

/* This function returns the current FTS and saves it so it can be used by get_visible_start */
LLONG get_visible_end(struct ccx_common_timing_ctx *ctx, int current_field)
{
	LLONG fts = ccxr_get_visible_end(ctx, current_field);
	ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "Visible End time=%s\n", print_mstime_static(fts));
	return fts;
}

int process_cc_data(struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx, unsigned char *cc_data, int cc_count, struct cc_subtitle *sub)
{
	int ret = -1;

	if (dec_ctx->write_format == CCX_OF_MCC)
	{
		mcc_encode_cc_data(enc_ctx, dec_ctx, cc_data, cc_count);
		return 0;
	}

	ret = ccxr_process_cc_data(dec_ctx, cc_data, cc_count);

	for (int j = 0; j < cc_count * 3; j = j + 3)
	{
		if (validate_cc_data_pair(cc_data + j))
			continue;
		ret = do_cb(dec_ctx, cc_data + j, sub);
		if (ret == 1) // 1 means success here
			ret = 0;
	}
	return ret;
}
int validate_cc_data_pair(unsigned char *cc_data_pair)
{
	unsigned char cc_valid = (*cc_data_pair & 4) >> 2;
	unsigned char cc_type = *cc_data_pair & 3;

	if (!cc_valid)
		return -1;

	if (cc_type == 0 || cc_type == 1)
	{
		// For EIA-608 data we verify parity.
		if (!cc608_parity_table[cc_data_pair[2]])
		{
			// If the second byte doesn't pass parity, ignore pair
			return -1;
		}
		if (!cc608_parity_table[cc_data_pair[1]])
		{
			// The first byte doesn't pass parity, we replace it with a solid blank
			// and process the pair.
			cc_data_pair[1] = 0x7F;
		}
	}
	return 0;
}
int do_cb(struct lib_cc_decode *ctx, unsigned char *cc_block, struct cc_subtitle *sub)
{
	unsigned char cc_valid = (*cc_block & 4) >> 2;
	unsigned char cc_type = *cc_block & 3;

	int timeok = 1;

	if (ctx->fix_padding && cc_valid == 0 && cc_type <= 1 // Only fix NTSC packets
	    && cc_block[1] == 0 && cc_block[2] == 0)
	{
		/* Padding */
		cc_valid = 1;
		cc_block[1] = 0x80;
		cc_block[2] = 0x80;
	}

	if (ctx->write_format != CCX_OF_RAW && // In raw we cannot skip padding because timing depends on it
	    ctx->write_format != CCX_OF_DVDRAW &&
	    (cc_block[0] == 0xFA || cc_block[0] == 0xFC || cc_block[0] == 0xFD) && (cc_block[1] & 0x7F) == 0 && (cc_block[2] & 0x7F) == 0) // CFS: Skip non-data, makes debugging harder.
		return 1;

	// Print raw data with FTS.
	dbg_print(CCX_DMT_CBRAW, "%s   %d   %02X:%c%c:%02X", print_mstime_static(ctx->timing->fts_now + ctx->timing->fts_global), in_xds_mode,
		  cc_block[0], cc_block[1] & 0x7f, cc_block[2] & 0x7f, cc_block[2]);

	/* In theory the writercwtdata() function could return early and not
	 * go through the 608/708 cases below.  We do that to get accurate
	 * counts for cb_field1, cb_field2 and cb_708.
	 * Note that printdata() and dtvcc_process_data() must not be called for
	 * the CCX_OF_RCWT case. */

	if (cc_valid || cc_type == 3)
	{
		ctx->cc_stats[cc_type]++;

		switch (cc_type)
		{
			case 0:
				dbg_print(CCX_DMT_CBRAW, "    %s   ..   ..\n", debug_608_to_ASC(cc_block, 0));

				ctx->current_field = 1;
				ctx->saw_caption_block = 1;

				if (ctx->extraction_start.set &&
				    get_fts(ctx->timing, ctx->current_field) < ctx->extraction_start.time_in_ms)
					timeok = 0;
				if (ctx->extraction_end.set &&
				    get_fts(ctx->timing, ctx->current_field) > ctx->extraction_end.time_in_ms)
				{
					timeok = 0;
					ctx->processed_enough = 1;
				}
				if (timeok)
				{
					if (ctx->write_format != CCX_OF_RCWT)
						printdata(ctx, cc_block + 1, 2, 0, 0, sub);
					else
						writercwtdata(ctx, cc_block, sub);
				}
				// For container formats (H.264, MPEG-2 PES), don't increment cb_field
				// because the frame PTS already represents the correct timestamp.
				// The cb_field offset is only meaningful for raw/elementary streams.
				if (ctx->in_bufferdatatype != CCX_H264 && ctx->in_bufferdatatype != CCX_PES)
					cb_field1++;
				break;
			case 1:
				dbg_print(CCX_DMT_CBRAW, "    ..   %s   ..\n", debug_608_to_ASC(cc_block, 1));

				ctx->current_field = 2;
				ctx->saw_caption_block = 1;

				if (ctx->extraction_start.set &&
				    get_fts(ctx->timing, ctx->current_field) < ctx->extraction_start.time_in_ms)
					timeok = 0;
				if (ctx->extraction_end.set &&
				    get_fts(ctx->timing, ctx->current_field) > ctx->extraction_end.time_in_ms)
				{
					timeok = 0;
					ctx->processed_enough = 1;
				}
				if (timeok)
				{
					if (ctx->write_format != CCX_OF_RCWT)
						printdata(ctx, 0, 0, cc_block + 1, 2, sub);
					else
						writercwtdata(ctx, cc_block, sub);
				}
				// For container formats, don't increment cb_field (see comment above)
				if (ctx->in_bufferdatatype != CCX_H264 && ctx->in_bufferdatatype != CCX_PES)
					cb_field2++;
				break;
			case 2: // EIA-708
				//  DTVCC packet data
				//  Fall through
			case 3: // EIA-708
				dbg_print(CCX_DMT_CBRAW, "    ..   ..   DD\n");

				// DTVCC packet start
				ctx->current_field = 3;

				if (ctx->extraction_start.set &&
				    get_fts(ctx->timing, ctx->current_field) < ctx->extraction_start.time_in_ms)
					timeok = 0;
				if (ctx->extraction_end.set &&
				    get_fts(ctx->timing, ctx->current_field) > ctx->extraction_end.time_in_ms)
				{
					timeok = 0;
					ctx->processed_enough = 1;
				}
				if (timeok)
				{
					if (ctx->write_format == CCX_OF_RCWT)
						writercwtdata(ctx, cc_block, sub);
				}
				// For container formats, don't increment cb_708 (see comment above)
				if (ctx->in_bufferdatatype != CCX_H264 && ctx->in_bufferdatatype != CCX_PES)
					cb_708++;
				// Check for bytes read
				// printf ("Warning: Losing EIA-708 data!\n");
				break;
			default:
				fatal(CCX_COMMON_EXIT_BUG_BUG, "In do_cb: Impossible value for cc_type, Please file a bug report on GitHub.\n");
		} // switch (cc_type)
	}	  // cc_valid
	else
	{
		dbg_print(CCX_DMT_CBRAW, "    ..   ..   ..\n");
		dbg_print(CCX_DMT_VERBOSE, "Found !(cc_valid || cc_type==3) - ignore this block\n");
	}

	return 1;
}

void dinit_cc_decode(struct lib_cc_decode **ctx)
{
	struct lib_cc_decode *lctx = *ctx;
#ifndef DISABLE_RUST
	ccxr_dtvcc_free(lctx->dtvcc_rust);
	lctx->dtvcc_rust = NULL;
#else
	dtvcc_free(&lctx->dtvcc);
#endif
	dinit_avc(&lctx->avc_ctx);
	ccx_decoder_608_dinit_library(&lctx->context_cc608_field_1);
	ccx_decoder_608_dinit_library(&lctx->context_cc608_field_2);
	dinit_timing_ctx(&lctx->timing);
	free_decoder_context(lctx->prev);
	free_subtitle(lctx->dec_sub.prev);
	/* Free the embedded dec_sub's data field (allocated by write_cc_buffer) */
	if (lctx->dec_sub.datatype == CC_DATATYPE_DVB)
	{
		struct cc_bitmap *bitmap = (struct cc_bitmap *)lctx->dec_sub.data;
		if (bitmap)
		{
			freep(&bitmap->data0);
			freep(&bitmap->data1);
		}
	}
	/* Free any leftover XDS strings that weren't processed by the encoder */
	if (lctx->dec_sub.type == CC_608 && lctx->dec_sub.data)
	{
		struct eia608_screen *data = (struct eia608_screen *)lctx->dec_sub.data;
		for (int i = 0; i < lctx->dec_sub.nb_data; i++, data++)
		{
			if (data->format == SFORMAT_XDS && data->xds_str)
			{
				freep(&data->xds_str);
			}
		}
	}
	freep(&lctx->dec_sub.data);
	/* Note: xds_ctx is freed in general_loop.c, mp4.c etc. during normal processing.
	   Don't free it here as it may cause double-free if already freed elsewhere. */
	freep(ctx);
}

struct lib_cc_decode *init_cc_decode(struct ccx_decoders_common_settings_t *setting)
{
	struct lib_cc_decode *ctx = NULL;

	ctx = malloc(sizeof(struct lib_cc_decode));
	if (!ctx)
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In init_cc_decode: Out of memory allocating ctx.");

	// Initialize all pointers to NULL
	ctx->avc_ctx = NULL;
	ctx->timing = NULL;
	ctx->dtvcc = NULL;
	ctx->context_cc608_field_1 = NULL;
	ctx->context_cc608_field_2 = NULL;
	ctx->xds_ctx = NULL;
	ctx->vbi_decoder = NULL;
	ctx->prev = NULL;
	memset(&ctx->dec_sub, 0, sizeof(ctx->dec_sub));

	ctx->avc_ctx = init_avc();
	if (!ctx->avc_ctx)
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In init_cc_decode: Out of memory initializing avc_ctx.");

	ctx->codec = setting->codec;
	ctx->timing = init_timing_ctx(&ccx_common_timing_settings);
	if (!ctx->timing)
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In init_cc_decode: Out of memory initializing timing.");

	setting->settings_dtvcc->timing = ctx->timing;

	setting->settings_608->no_rollup = setting->no_rollup;
	setting->settings_dtvcc->no_rollup = setting->no_rollup;
	ctx->no_rollup = setting->no_rollup;
	ctx->noscte20 = setting->noscte20;

#ifndef DISABLE_RUST
	ctx->dtvcc_rust = ccxr_dtvcc_init(setting->settings_dtvcc);
	ctx->dtvcc = NULL; // Not used when Rust is enabled
#else
	ctx->dtvcc = dtvcc_init(setting->settings_dtvcc);
	if (!ctx->dtvcc)
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In init_cc_decode: Out of memory initializing dtvcc.");
	ctx->dtvcc->is_active = setting->settings_dtvcc->enabled;
	ctx->dtvcc_rust = NULL;
#endif

	if (setting->codec == CCX_CODEC_ATSC_CC)
	{
		// Prepare 608 context
		ctx->context_cc608_field_1 = ccx_decoder_608_init_library(
		    setting->settings_608,
		    setting->cc_channel,
		    1,
		    &ctx->processed_enough,
		    setting->cc_to_stdout,
		    setting->output_format,
		    ctx->timing);
		if (!ctx->context_cc608_field_1)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In init_cc_decode: Out of memory initializing context_cc608_field_1.");
		ctx->context_cc608_field_2 = ccx_decoder_608_init_library(
		    setting->settings_608,
		    setting->cc_channel,
		    2,
		    &ctx->processed_enough,
		    setting->cc_to_stdout,
		    setting->output_format,
		    ctx->timing);
		if (!ctx->context_cc608_field_2)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In init_cc_decode: Out of memory initializing context_cc608_field_2.");
	}
	ctx->current_field = 1;
	ctx->private_data = setting->private_data;
	ctx->fix_padding = setting->fix_padding;
	ctx->write_format = setting->output_format;
	ctx->subs_delay = setting->subs_delay;
	ctx->extract = setting->extract;
	ctx->fullbin = setting->fullbin;
	ctx->hauppauge_mode = setting->hauppauge_mode;
	ctx->saw_caption_block = 0;
	ctx->program_number = setting->program_number;
	ctx->processed_enough = 0;
	ctx->max_gop_length = 0;
	ctx->has_ccdata_buffered = 0;
	ctx->in_bufferdatatype = CCX_UNKNOWN;
	ctx->frames_since_last_gop = 0;
	ctx->total_pulldownfields = 0;
	ctx->total_pulldownframes = 0;
	ctx->stat_numuserheaders = 0;
	ctx->stat_dvdccheaders = 0;
	ctx->stat_scte20ccheaders = 0;
	ctx->stat_replay5000headers = 0;
	ctx->stat_replay4000headers = 0;
	ctx->stat_dishheaders = 0;
	ctx->stat_hdtv = 0;
	ctx->stat_divicom = 0;
	ctx->false_pict_header = 0;
	ctx->is_alloc = 0;

	memcpy(&ctx->extraction_start, &setting->extraction_start, sizeof(struct ccx_boundary_time));
	memcpy(&ctx->extraction_end, &setting->extraction_end, sizeof(struct ccx_boundary_time));

	if (setting->send_to_srv)
		ctx->writedata = net_send_cc;
	else
	{
		// TODO: Add function to test this
		switch (setting->output_format)
		{
			case CCX_OF_RAW:
			case CCX_OF_DVDRAW:
			case CCX_OF_RCWT:
				ctx->writedata = writeraw;
				break;
			case CCX_OF_CCD:
			case CCX_OF_SCC:
			case CCX_OF_SMPTETT:
			case CCX_OF_SAMI:
			case CCX_OF_SRT:
			case CCX_OF_SSA:
			case CCX_OF_WEBVTT:
			case CCX_OF_TRANSCRIPT:
			case CCX_OF_SPUPNG:
			case CCX_OF_SIMPLE_XML:
			case CCX_OF_G608:
			case CCX_OF_NULL:
			case CCX_OF_MCC:
			case CCX_OF_CURL:
				ctx->writedata = process608;
				break;
			default:
				fatal(CCX_COMMON_EXIT_BUG_BUG, "Invalid Write Format Selected");
				break;
		}
	}

	memset(&ctx->dec_sub, 0, sizeof(ctx->dec_sub));

	// Initialize HDTV caption buffer
	init_hdcc(ctx);

	ctx->current_hor_size = 0;
	ctx->current_vert_size = 0;
	ctx->current_aspect_ratio = 0;
	ctx->current_frame_rate = 4; // Assume standard fps, 29.97

	// Variables used while parsing elementary stream
	ctx->no_bitstream_error = 0;
	ctx->saw_seqgoppic = 0;
	ctx->in_pic_data = 0;

	ctx->current_progressive_sequence = 2;
	ctx->current_pulldownfields = 32768;

	ctx->temporal_reference = 0;
	ctx->maxtref = 0;
	ctx->picture_coding_type = CCX_FRAME_TYPE_RESET_OR_UNKNOWN;
	ctx->num_key_frames = 0;
	ctx->picture_structure = 0;
	ctx->top_field_first = 0;
	ctx->repeat_first_field = 0;
	ctx->progressive_frame = 0;
	ctx->pulldownfields = 0;
	// es parser related variable ends here

	memset(ctx->cc_stats, 0, 4 * sizeof(int));

	ctx->anchor_seq_number = -1;
	// Init XDS buffers
	if (setting->output_format != CCX_OF_TRANSCRIPT)
	{
		setting->xds_write_to_file = 0;
	}
	ctx->xds_ctx = ccx_decoders_xds_init_library(ctx->timing, setting->xds_write_to_file);
	if (!ctx->xds_ctx)
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In init_cc_decode: Out of memory initializing xds_ctx.");

	ctx->vbi_decoder = NULL;
	ctx->ocr_quantmode = setting->ocr_quantmode;
	return ctx;
}

void flush_cc_decode(struct lib_cc_decode *ctx, struct cc_subtitle *sub)
{
	if (ctx->codec == CCX_CODEC_ATSC_CC)
	{
		if (ctx->extract != 2)
		{
			if (ctx->write_format == CCX_OF_CCD ||
			    ctx->write_format == CCX_OF_SCC ||
			    ctx->write_format == CCX_OF_SMPTETT ||
			    ctx->write_format == CCX_OF_SAMI ||
			    ctx->write_format == CCX_OF_SRT ||
			    ctx->write_format == CCX_OF_TRANSCRIPT ||
			    ctx->write_format == CCX_OF_WEBVTT ||
			    ctx->write_format == CCX_OF_SPUPNG ||
			    ctx->write_format == CCX_OF_SSA ||
			    ctx->write_format == CCX_OF_MCC)
			{
				flush_608_context(ctx->context_cc608_field_1, sub);
			}
			else if (ctx->write_format == CCX_OF_RCWT)
			{
				// Write last header and data
				writercwtdata(ctx, NULL, sub);
			}
		}
		if (ctx->extract != 1)
		{
			// TODO: Use a function to prevent repeating these lines
			if (ctx->write_format == CCX_OF_CCD ||
			    ctx->write_format == CCX_OF_SCC ||
			    ctx->write_format == CCX_OF_SMPTETT ||
			    ctx->write_format == CCX_OF_SAMI ||
			    ctx->write_format == CCX_OF_SRT ||
			    ctx->write_format == CCX_OF_TRANSCRIPT ||
			    ctx->write_format == CCX_OF_WEBVTT ||
			    ctx->write_format == CCX_OF_SPUPNG ||
			    ctx->write_format == CCX_OF_SSA ||
			    ctx->write_format == CCX_OF_MCC)
			{
				flush_608_context(ctx->context_cc608_field_2, sub);
			}
		}
	}
#ifndef DISABLE_RUST
	if (ccxr_dtvcc_is_active(ctx->dtvcc_rust))
	{
		ctx->current_field = 3;
		ccxr_flush_active_decoders(ctx->dtvcc_rust);
	}
#else
	if (ctx->dtvcc->is_active)
	{
		for (int i = 0; i < CCX_DTVCC_MAX_SERVICES; i++)
		{
			dtvcc_service_decoder *decoder = &ctx->dtvcc->decoders[i];
			if (!ctx->dtvcc->services_active[i])
				continue;
			if (decoder->cc_count > 0)
			{
				ctx->current_field = 3;
				ccxr_flush_decoder(ctx->dtvcc, decoder);
			}
		}
	}
#endif
}
struct encoder_ctx *copy_encoder_context(struct encoder_ctx *ctx)
{
	struct encoder_ctx *ctx_copy = NULL;
	ctx_copy = malloc(sizeof(struct encoder_ctx));
	if (!ctx_copy)
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In copy_encoder_context: Out of memory allocating ctx_copy.");
	memcpy(ctx_copy, ctx, sizeof(struct encoder_ctx));

	// Initialize copied pointers to NULL before re-allocating
	ctx_copy->buffer = NULL;
	ctx_copy->first_input_file = NULL;
	ctx_copy->out = NULL;
	ctx_copy->timing = NULL;
	ctx_copy->transcript_settings = NULL;
	ctx_copy->subline = NULL;
	ctx_copy->start_credits_text = NULL;
	ctx_copy->end_credits_text = NULL;
	ctx_copy->prev = NULL;
	ctx_copy->last_str = NULL;

	if (ctx->buffer)
	{
		ctx_copy->buffer = malloc(ctx->capacity * sizeof(unsigned char));
		if (!ctx_copy->buffer)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In copy_encoder_context: Out of memory allocating buffer.");
		memcpy(ctx_copy->buffer, ctx->buffer, ctx->capacity * sizeof(unsigned char));
	}
	if (ctx->first_input_file)
	{
		size_t len = strlen(ctx->first_input_file) + 1;
		ctx_copy->first_input_file = malloc(len);
		if (!ctx_copy->first_input_file)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In copy_encoder_context: Out of memory allocating first_input_file.");
		memcpy(ctx_copy->first_input_file, ctx->first_input_file, len);
	}
	if (ctx->out)
	{
		ctx_copy->out = malloc(sizeof(struct ccx_s_write));
		if (!ctx_copy->out)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In copy_encoder_context: Out of memory allocating out.");
		memcpy(ctx_copy->out, ctx->out, sizeof(struct ccx_s_write));
	}
	if (ctx->timing)
	{
		ctx_copy->timing = malloc(sizeof(struct ccx_common_timing_ctx));
		if (!ctx_copy->timing)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In copy_encoder_context: Out of memory allocating timing.");
		memcpy(ctx_copy->timing, ctx->timing, sizeof(struct ccx_common_timing_ctx));
	}
	if (ctx->transcript_settings)
	{
		ctx_copy->transcript_settings = malloc(sizeof(struct ccx_encoders_transcript_format));
		if (!ctx_copy->transcript_settings)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In copy_encoder_context: Out of memory allocating transcript_settings.");
		memcpy(ctx_copy->transcript_settings, ctx->transcript_settings, sizeof(struct ccx_encoders_transcript_format));
	}
	if (ctx->subline)
	{
		ctx_copy->subline = malloc(SUBLINESIZE);
		if (!ctx_copy->subline)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In copy_encoder_context: Out of memory allocating subline.");
		memcpy(ctx_copy->subline, ctx->subline, SUBLINESIZE);
	}
	if (ctx->start_credits_text)
	{
		size_t len = strlen(ctx->start_credits_text) + 1;
		ctx_copy->start_credits_text = malloc(len);
		if (!ctx_copy->start_credits_text)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In copy_encoder_context: Out of memory allocating start_credits_text.");
		memcpy(ctx_copy->start_credits_text, ctx->start_credits_text, len);
	}
	if (ctx->end_credits_text)
	{
		size_t len = strlen(ctx->end_credits_text) + 1;
		ctx_copy->end_credits_text = malloc(len);
		if (!ctx_copy->end_credits_text)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In copy_encoder_context: Out of memory allocating end_credits_text.");
		memcpy(ctx_copy->end_credits_text, ctx->end_credits_text, len);
	}
	return ctx_copy;
}
struct lib_cc_decode *copy_decoder_context(struct lib_cc_decode *ctx)
{
	struct lib_cc_decode *ctx_copy = NULL;
	ctx_copy = malloc(sizeof(struct lib_cc_decode));
	if (!ctx_copy)
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In copy_decoder_context: Out of memory allocating ctx_copy.");
	memcpy(ctx_copy, ctx, sizeof(struct lib_cc_decode));

	// Initialize copied pointers to NULL before re-allocating
	ctx_copy->context_cc608_field_1 = NULL;
	ctx_copy->context_cc608_field_2 = NULL;
	ctx_copy->timing = NULL;
	ctx_copy->avc_ctx = NULL;
	ctx_copy->private_data = NULL;
	ctx_copy->dtvcc = NULL;
	ctx_copy->xds_ctx = NULL;
	ctx_copy->vbi_decoder = NULL;

	if (ctx->context_cc608_field_1)
	{
		ctx_copy->context_cc608_field_1 = malloc(sizeof(struct ccx_decoder_608_context));
		if (!ctx_copy->context_cc608_field_1)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In copy_decoder_context: Out of memory allocating context_cc608_field_1.");
		memcpy(ctx_copy->context_cc608_field_1, ctx->context_cc608_field_1, sizeof(struct ccx_decoder_608_context));
	}
	if (ctx->context_cc608_field_2)
	{
		ctx_copy->context_cc608_field_2 = malloc(sizeof(struct ccx_decoder_608_context));
		if (!ctx_copy->context_cc608_field_2)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In copy_decoder_context: Out of memory allocating context_cc608_field_2.");
		memcpy(ctx_copy->context_cc608_field_2, ctx->context_cc608_field_2, sizeof(struct ccx_decoder_608_context));
	}
	if (ctx->timing)
	{
		ctx_copy->timing = malloc(sizeof(struct ccx_common_timing_ctx));
		if (!ctx_copy->timing)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In copy_decoder_context: Out of memory allocating timing.");
		memcpy(ctx_copy->timing, ctx->timing, sizeof(struct ccx_common_timing_ctx));
	}
	if (ctx->avc_ctx)
	{
		ctx_copy->avc_ctx = malloc(sizeof(struct avc_ctx));
		if (!ctx_copy->avc_ctx)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In copy_decoder_context: Out of memory allocating avc_ctx.");
		memcpy(ctx_copy->avc_ctx, ctx->avc_ctx, sizeof(struct avc_ctx));
	}
	if (ctx->dtvcc)
	{
		ctx_copy->dtvcc = malloc(sizeof(struct dtvcc_ctx));
		if (!ctx_copy->dtvcc)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In copy_decoder_context: Out of memory allocating dtvcc.");
		memcpy(ctx_copy->dtvcc, ctx->dtvcc, sizeof(struct dtvcc_ctx));
	}
	if (ctx->xds_ctx)
	{
		ctx_copy->xds_ctx = malloc(sizeof(struct ccx_decoders_xds_context));
		if (!ctx_copy->xds_ctx)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In copy_decoder_context: Out of memory allocating xds_ctx.");
		memcpy(ctx_copy->xds_ctx, ctx->xds_ctx, sizeof(struct ccx_decoders_xds_context));
	}
	if (ctx->vbi_decoder)
	{
		ctx_copy->vbi_decoder = malloc(sizeof(struct ccx_decoder_vbi_ctx));
		if (!ctx_copy->vbi_decoder)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In copy_decoder_context: Out of memory allocating vbi_decoder.");
		memcpy(ctx_copy->vbi_decoder, ctx->vbi_decoder, sizeof(struct ccx_decoder_vbi_ctx));
	}
	return ctx_copy;
}
struct cc_subtitle *copy_subtitle(struct cc_subtitle *sub)
{
	struct cc_subtitle *sub_copy = NULL;
	sub_copy = malloc(sizeof(struct cc_subtitle));
	if (!sub_copy)
		fatal(EXIT_NOT_ENOUGH_MEMORY, "In copy_subtitle: Out of memory allocating sub_copy.");
	memcpy(sub_copy, sub, sizeof(struct cc_subtitle));
	sub_copy->datatype = sub->datatype;
	sub_copy->data = NULL;

	if (sub->data)
	{
		sub_copy->data = malloc(sub->nb_data * sizeof(struct eia608_screen));
		if (!sub_copy->data)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In copy_subtitle: Out of memory allocating data.");
		memcpy(sub_copy->data, sub->data, sub->nb_data * sizeof(struct eia608_screen));
	}
	return sub_copy;
}
void free_encoder_context(struct encoder_ctx *ctx)
{
	if (!ctx)
		return;

	freep(&ctx->first_input_file);
	freep(&ctx->buffer);
	freep(&ctx->out);
	freep(&ctx->timing);
	freep(&ctx->transcript_settings);
	freep(&ctx->subline);
	freep(&ctx->start_credits_text);
	freep(&ctx->end_credits_text);
	freep(&ctx->prev);
	freep(&ctx->last_str);
	freep(&ctx);
}
void free_decoder_context(struct lib_cc_decode *ctx)
{
	if (!ctx)
		return;

	freep(&ctx->context_cc608_field_1);
	freep(&ctx->context_cc608_field_2);
	freep(&ctx->timing);
	freep(&ctx->avc_ctx);
	freep(&ctx->private_data);
	freep(&ctx->dtvcc);
	freep(&ctx->xds_ctx);
	freep(&ctx->vbi_decoder);
	freep(&ctx);
}
void free_subtitle(struct cc_subtitle *sub)
{
	if (!sub)
		return;

	if (sub->datatype == CC_DATATYPE_DVB)
	{
		struct cc_bitmap *bitmap = (struct cc_bitmap *)sub->data;
		if (bitmap)
		{
			freep(&bitmap->data0);
			freep(&bitmap->data1);
		}
	}
	freep(&sub->data);
	freep(&sub);
}
