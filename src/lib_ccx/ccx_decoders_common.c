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
#include "ccx_dtvcc.h"


uint64_t utc_refvalue = UINT64_MAX;  /* _UI64_MAX means don't use UNIX, 0 = use current system time as reference, +1 use a specific reference */
extern int in_xds_mode;


/* This function returns a FTS that is guaranteed to be at least 1 ms later than the end of the previous screen. It shouldn't be needed
   obviously but it guarantees there's no timing overlap */
LLONG get_visible_start (struct ccx_common_timing_ctx *ctx, int current_field)
{
	LLONG fts = get_fts(ctx, current_field);
	if (fts <= ctx->minimum_fts)
		fts = ctx->minimum_fts + 1;
	ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "Visible Start time=%s\n", print_mstime(fts));
	return fts;
}

/* This function returns the current FTS and saves it so it can be used by ctxget_visible_start */
LLONG get_visible_end (struct ccx_common_timing_ctx *ctx, int current_field)
{
	LLONG fts = get_fts(ctx, current_field);
	if (fts > ctx->minimum_fts)
		ctx->minimum_fts = fts;
	ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "Visible End time=%s\n", print_mstime(fts));
	return fts;
}

int process_cc_data (struct lib_cc_decode *ctx, unsigned char *cc_data, int cc_count, struct cc_subtitle *sub)
{
	int ret = -1;
	for (int j = 0; j < cc_count * 3; j = j + 3)
	{
		if (validate_cc_data_pair( cc_data + j ) )
			continue;
		ret = do_cb(ctx, cc_data + j, sub);
		if (ret == 1) //1 means success here
			ret = 0;
	}
	return ret;
}
int validate_cc_data_pair (unsigned char *cc_data_pair)
{
	unsigned char cc_valid = (*cc_data_pair & 4) >>2;
	unsigned char cc_type = *cc_data_pair & 3;

	if (!cc_valid)
		return -1;

	if (cc_type==0 || cc_type==1)
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
			cc_data_pair[1]=0x7F;
		}
	}
	return 0;

}
int do_cb (struct lib_cc_decode *ctx, unsigned char *cc_block, struct cc_subtitle *sub)
{
	unsigned char cc_valid = (*cc_block & 4) >>2;
	unsigned char cc_type = *cc_block & 3;

	int timeok = 1;

	if ( ctx->fix_padding
		&& cc_valid==0 && cc_type <= 1 // Only fix NTSC packets
		&& cc_block[1]==0 && cc_block[2]==0 )
	{
		/* Padding */
		cc_valid=1;
		cc_block[1]=0x80;
		cc_block[2]=0x80;
	}

	if ( ctx->write_format!=CCX_OF_RAW && // In raw we cannot skip padding because timing depends on it
		 ctx->write_format!=CCX_OF_DVDRAW &&
		(cc_block[0]==0xFA || cc_block[0]==0xFC || cc_block[0]==0xFD )
		&& (cc_block[1]&0x7F)==0 && (cc_block[2]&0x7F)==0) // CFS: Skip non-data, makes debugging harder.
		return 1;

	// Print raw data with FTS.
	dbg_print(CCX_DMT_CBRAW, "%s   %d   %02X:%c%c:%02X", print_mstime(ctx->timing->fts_now + ctx->timing->fts_global),in_xds_mode,
			cc_block[0], cc_block[1]&0x7f,cc_block[2]&0x7f, cc_block[2]);

	/* In theory the writercwtdata() function could return early and not
	 * go through the 608/708 cases below.  We do that to get accurate
	 * counts for cb_field1, cb_field2 and cb_708.
	 * Note that printdata() and dtvcc_process_data() must not be called for
	 * the CCX_OF_RCWT case. */

	if (cc_valid || cc_type==3)
	{
		ctx->cc_stats[cc_type]++;

		switch (cc_type)
		{
			case 0:
				dbg_print(CCX_DMT_CBRAW, "    %s   ..   ..\n",  debug_608toASC( cc_block, 0));

				ctx->current_field = 1;
				ctx->saw_caption_block = 1;

				if (ctx->extraction_start.set &&
						get_fts(ctx->timing, ctx->current_field) < ctx->extraction_start.time_in_ms)
					timeok = 0;
				if (ctx->extraction_end.set &&
						get_fts(ctx->timing, ctx->current_field) > ctx->extraction_end.time_in_ms)
				{
					timeok = 0;
					ctx->processed_enough=1;
				}
				if (timeok)
				{
					if(ctx->write_format!=CCX_OF_RCWT)
						printdata (ctx, cc_block+1,2,0,0, sub);
					else
						writercwtdata(ctx, cc_block, sub);
				}
				cb_field1++;
				break;
			case 1:
				dbg_print(CCX_DMT_CBRAW, "    ..   %s   ..\n",  debug_608toASC( cc_block, 1));

				ctx->current_field = 2;
				ctx->saw_caption_block = 1;

				if (ctx->extraction_start.set &&
						get_fts(ctx->timing, ctx->current_field) < ctx->extraction_start.time_in_ms)
					timeok = 0;
				if (ctx->extraction_end.set &&
						get_fts(ctx->timing, ctx->current_field) > ctx->extraction_end.time_in_ms)
				{
					timeok = 0;
					ctx->processed_enough=1;
				}
				if (timeok)
				{
					if(ctx->write_format!=CCX_OF_RCWT)
						printdata (ctx, 0,0,cc_block+1,2, sub);
					else
						writercwtdata(ctx, cc_block, sub);
				}
				cb_field2++;
				break;
			case 2: //EIA-708
				// DTVCC packet data
				// Fall through
			case 3: //EIA-708
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
					ctx->processed_enough=1;
				}
				char temp[4];
				temp[0]=cc_valid;
				temp[1]=cc_type;
				temp[2]=cc_block[1];
				temp[3]=cc_block[2];
				if (timeok)
				{
					if (ctx->write_format != CCX_OF_RCWT)
						ccx_dtvcc_process_data(ctx, (const unsigned char *) temp, 4);
					else
						writercwtdata(ctx, cc_block, sub);
				}
				cb_708++;
				// Check for bytes read
				// printf ("Warning: Losing EIA-708 data!\n");
				break;
			default:
				fatal(CCX_COMMON_EXIT_BUG_BUG, "Cannot be reached!");
		} // switch (cc_type)
	} // cc_valid
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
	ccx_dtvcc_free(&lctx->dtvcc);
	dinit_avc(&lctx->avc_ctx);
	ccx_decoder_608_dinit_library(&lctx->context_cc608_field_1);
	ccx_decoder_608_dinit_library(&lctx->context_cc608_field_2);
	dinit_timing_ctx(&lctx->timing);
	freep(ctx);
}

struct lib_cc_decode* init_cc_decode (struct ccx_decoders_common_settings_t *setting)
{
	struct lib_cc_decode *ctx = NULL;

	ctx = malloc(sizeof(struct lib_cc_decode));
	if(!ctx)
		return NULL;

	ctx->avc_ctx = init_avc();
	ctx->codec = setting->codec;
	ctx->timing = init_timing_ctx(&ccx_common_timing_settings);

	setting->settings_dtvcc->timing = ctx->timing;

	setting->settings_608->no_rollup = setting->no_rollup;
	setting->settings_dtvcc->no_rollup = setting->no_rollup;
	ctx->no_rollup = setting->no_rollup;
	ctx->noscte20 = setting->noscte20;

	ctx->dtvcc = ccx_dtvcc_init(setting->settings_dtvcc);
	ctx->dtvcc->is_active = setting->settings_dtvcc->enabled;

	if(setting->codec == CCX_CODEC_ATSC_CC)
	{
		// Prepare 608 context
		ctx->context_cc608_field_1 = ccx_decoder_608_init_library(
				setting->settings_608,
				setting->cc_channel,
				1,
				&ctx->processed_enough,
				setting->cc_to_stdout,
				setting->output_format,
				ctx->timing
				);
		ctx->context_cc608_field_2 = ccx_decoder_608_init_library(
				setting->settings_608,
				setting->cc_channel,
				2,
				&ctx->processed_enough,
				setting->cc_to_stdout,
				setting->output_format,
				ctx->timing
				);
	}
	else
	{
		ctx->context_cc608_field_1 = NULL;
		ctx->context_cc608_field_2 = NULL;
	}
	ctx->current_field = 1;
	ctx->private_data = setting->private_data;
	ctx->fix_padding = setting->fix_padding;
	ctx->write_format =  setting->output_format;
	ctx->subs_delay =  setting->subs_delay;
	ctx->extract = setting->extract;
	ctx->fullbin = setting->fullbin;
	ctx->hauppauge_mode = setting->hauppauge_mode;
	ctx->program_number = setting->program_number;
	ctx->processed_enough = 0;
	ctx->max_gop_length = 0;
	ctx->has_ccdata_buffered = 0;
	ctx->in_bufferdatatype = CCX_UNKNOWN;
	ctx->frames_since_last_gop = 0;
	memcpy(&ctx->extraction_start, &setting->extraction_start,sizeof(struct ccx_boundary_time));
	memcpy(&ctx->extraction_end, &setting->extraction_end,sizeof(struct ccx_boundary_time));

	if (setting->send_to_srv)
		ctx->writedata = net_send_cc;
	else if (setting->output_format==CCX_OF_RAW ||
		setting->output_format==CCX_OF_DVDRAW ||
		setting->output_format==CCX_OF_RCWT )
		ctx->writedata = writeraw;
	else if (setting->output_format==CCX_OF_SMPTETT ||
		setting->output_format==CCX_OF_SAMI ||
		setting->output_format==CCX_OF_SRT ||
		setting->output_format == CCX_OF_WEBVTT ||
		setting->output_format==CCX_OF_TRANSCRIPT ||
		setting->output_format==CCX_OF_SPUPNG ||
		setting->output_format==CCX_OF_SIMPLE_XML ||
		setting->output_format==CCX_OF_G608 ||
		setting->output_format==CCX_OF_NULL)
		ctx->writedata = process608;
	else
		fatal(CCX_COMMON_EXIT_BUG_BUG, "Invalid Write Format Selected");

	memset (&ctx->dec_sub, 0,sizeof(ctx->dec_sub));

	// Initialize HDTV caption buffer
	init_hdcc(ctx);

	ctx->current_hor_size = 0;
	ctx->current_vert_size = 0;
	ctx->current_aspect_ratio = 0;
	ctx->current_frame_rate = 4; // Assume standard fps, 29.97

        //Variables used while parsing elementry stream
	ctx->no_bitstream_error = 0;
	ctx->saw_seqgoppic = 0;
	ctx->in_pic_data = 0;

	ctx->current_progressive_sequence = 2;
	ctx->current_pulldownfields = 32768;

	ctx->temporal_reference = 0;
	ctx->maxtref = 0;
	ctx->picture_coding_type = CCX_FRAME_TYPE_RESET_OR_UNKNOWN;
	ctx->picture_structure = 0;
	ctx->top_field_first = 0;
	ctx->repeat_first_field = 0;
	ctx->progressive_frame = 0;
	ctx->pulldownfields = 0;
        //es parser related variable ends here

	memset(ctx->cc_stats, 0, 4 * sizeof(int)); 

	ctx->anchor_seq_number = -1;
	// Init XDS buffers
	if(setting->ignore_xds == CCX_TRUE)
		ctx->xds_ctx = NULL;
	else
		ctx->xds_ctx = ccx_decoders_xds_init_library(ctx->timing);
	//xds_cea608_test(ctx->xds_ctx);

	return ctx;
}

void flush_cc_decode(struct lib_cc_decode *ctx, struct cc_subtitle *sub)
{
	if(ctx->codec == CCX_CODEC_ATSC_CC)
	{
		if (ctx->extract != 2)
		{
			if (ctx->write_format==CCX_OF_SMPTETT || ctx->write_format==CCX_OF_SAMI || 
					ctx->write_format==CCX_OF_SRT || ctx->write_format==CCX_OF_TRANSCRIPT ||
					ctx->write_format == CCX_OF_WEBVTT || ctx->write_format == CCX_OF_SPUPNG)
			{
				flush_608_context(ctx->context_cc608_field_1, sub);
			}
			else if(ctx->write_format == CCX_OF_RCWT)
			{
				// Write last header and data
				writercwtdata (ctx, NULL, sub);
			}
		}
		if (ctx->extract != 1)
		{
			if (ctx->write_format == CCX_OF_SMPTETT || ctx->write_format == CCX_OF_SAMI ||
					ctx->write_format == CCX_OF_SRT || ctx->write_format == CCX_OF_TRANSCRIPT ||
					ctx->write_format == CCX_OF_WEBVTT || ctx->write_format == CCX_OF_SPUPNG)
			{
				flush_608_context(ctx->context_cc608_field_2, sub);
			}
		}
	}
	if (ctx->dtvcc->is_active)
	{
		for (int i = 0; i < CCX_DTVCC_MAX_SERVICES; i++)
		{
			ccx_dtvcc_service_decoder *decoder = &ctx->dtvcc->decoders[i];
			if (!ctx->dtvcc->services_active[i])
				continue;
			if (decoder->cc_count > 0)
			{
				ctx->current_field = 3;
				ccx_dtvcc_decoder_flush(ctx->dtvcc, decoder);
			}
		}
	}
}
