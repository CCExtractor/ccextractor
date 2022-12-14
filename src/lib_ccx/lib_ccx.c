#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "activity.h"
#include "utility.h"
#include "dvb_subtitle_decoder.h"
#include "ccx_decoders_708.h"
#include "ccx_decoders_isdb.h"

struct ccx_common_logging_t ccx_common_logging;
static struct ccx_decoders_common_settings_t *init_decoder_setting(
    struct ccx_s_options *opt)
{
	struct ccx_decoders_common_settings_t *setting;

	setting = malloc(sizeof(struct ccx_decoders_common_settings_t));
	if (!setting)
		return NULL;

	setting->subs_delay = opt->subs_delay;
	setting->output_format = opt->write_format;
	setting->fix_padding = opt->fix_padding;
	setting->extract = opt->extract;
	setting->fullbin = opt->fullbin;
	setting->no_rollup = opt->no_rollup;
	setting->noscte20 = opt->noscte20;
	memcpy(&setting->extraction_start, &opt->extraction_start, sizeof(struct ccx_boundary_time));
	memcpy(&setting->extraction_end, &opt->extraction_end, sizeof(struct ccx_boundary_time));
	setting->cc_to_stdout = opt->cc_to_stdout;
	setting->settings_608 = &opt->settings_608;
	setting->settings_dtvcc = &opt->settings_dtvcc;
	setting->cc_channel = opt->cc_channel;
	setting->send_to_srv = opt->send_to_srv;
	setting->hauppauge_mode = opt->hauppauge_mode;
	setting->xds_write_to_file = opt->transcript_settings.xds;
	setting->ocr_quantmode = opt->ocr_quantmode;

	return setting;
}
static void dinit_decoder_setting(struct ccx_decoders_common_settings_t **setting)
{
	freep(setting);
}

static int init_ctx_outbase(struct ccx_s_options *opt, struct lib_ccx_ctx *ctx)
{
	char *file;

	if (opt->output_filename)
	{
		ctx->basefilename = get_basename(opt->output_filename);
	}
	else
	{
		switch (opt->input_source)
		{
			case CCX_DS_FILE:
				if (!ctx->inputfile || !ctx->inputfile[0])
				{
					errno = EINVAL;
					return -1;
				}
				file = ctx->inputfile[0];
				break;
			case CCX_DS_STDIN:
				file = "stdin";
				break;
			case CCX_DS_NETWORK:
			case CCX_DS_TCP:
				file = "network";
				break;
			default:
				errno = EINVAL;
				return -1;
		}

		ctx->basefilename = get_basename(file);
	}
	return 0;
}

struct encoder_ctx *get_encoder_by_pn(struct lib_ccx_ctx *ctx, int pn)
{
	struct encoder_ctx *enc_ctx;
	list_for_each_entry(enc_ctx, &ctx->enc_ctx_head, list, struct encoder_ctx)
	{
		if (enc_ctx && enc_ctx->program_number == pn)
			return enc_ctx;
	}
	return NULL;
}

struct lib_ccx_ctx *init_libraries(struct ccx_s_options *opt)
{
	int ret = 0;

	// Set logging functions for libraries
	ccx_common_logging.debug_ftn = &dbg_print;
	ccx_common_logging.debug_mask = opt->debug_mask;
	ccx_common_logging.fatal_ftn = &fatal;
	ccx_common_logging.log_ftn = &mprint;
	ccx_common_logging.gui_ftn = &activity_library_process;

	struct lib_ccx_ctx *ctx = malloc(sizeof(struct lib_ccx_ctx));
	if (!ctx)
		ccx_common_logging.fatal_ftn(EXIT_NOT_ENOUGH_MEMORY, "init_libraries: Not enough memory allocating lib_ccx_ctx context.");
	memset(ctx, 0, sizeof(struct lib_ccx_ctx));

	if (opt->xmltv)
	{
		ctx->epg_inited = 1;
		ctx->epg_buffers = (struct PSI_buffer *)malloc(sizeof(struct PSI_buffer) * (0xfff + 1));
		ctx->eit_programs = (struct EIT_program *)malloc(sizeof(struct EIT_program) * (TS_PMT_MAP_SIZE + 1));
		ctx->eit_current_events = (int32_t *)malloc(sizeof(int32_t) * (TS_PMT_MAP_SIZE + 1));
		ctx->ATSC_source_pg_map = (int16_t *)malloc(sizeof(int16_t) * (0xffff));
		memset(ctx->epg_buffers, 0, sizeof(struct PSI_buffer) * (0xfff + 1));
		memset(ctx->eit_programs, 0, sizeof(struct EIT_program) * (TS_PMT_MAP_SIZE + 1));
		memset(ctx->eit_current_events, 0, sizeof(int32_t) * (TS_PMT_MAP_SIZE + 1));
		memset(ctx->ATSC_source_pg_map, 0, sizeof(int16_t) * (0xffff));
		if (!ctx->epg_buffers || !ctx->eit_programs || !ctx->eit_current_events || !ctx->ATSC_source_pg_map)
			ccx_common_logging.fatal_ftn(EXIT_NOT_ENOUGH_MEMORY, "lib_ccx_ctx");
	}
	else
	{
		ctx->epg_inited = 0;
		ctx->epg_buffers = NULL;
		ctx->eit_programs = NULL;
		ctx->eit_current_events = NULL;
		ctx->ATSC_source_pg_map = NULL;
	}

	struct ccx_decoder_608_report *report_608 = malloc(sizeof(struct ccx_decoder_608_report));
	if (!report_608)
		ccx_common_logging.fatal_ftn(EXIT_NOT_ENOUGH_MEMORY, "init_libraries: Not enough memory allocating report_608");
	memset(report_608, 0, sizeof(struct ccx_decoder_608_report));

	ccx_decoder_dtvcc_report *report_dtvcc = (ccx_decoder_dtvcc_report *)
	    malloc(sizeof(ccx_decoder_dtvcc_report));
	if (!report_dtvcc)
		ccx_common_logging.fatal_ftn(EXIT_NOT_ENOUGH_MEMORY, "init_libraries: Not enough memory allocating report_dtvcc");
	memset(report_dtvcc, 0, sizeof(ccx_decoder_dtvcc_report));

	// Initialize some constants
	ctx->screens_to_process = -1;
	ctx->current_file = -1;

	// Init shared decoder settings
	ctx->dec_global_setting = init_decoder_setting(opt);
	if (!ctx->dec_global_setting)
	{
		free(report_dtvcc);
		return NULL;
	}

	// Need to set the 608 data for the report to the correct variable.
	ctx->freport.data_from_608 = report_608;
	ctx->dec_global_setting->settings_608->report = report_608;
	ctx->freport.data_from_708 = report_dtvcc;
	ctx->dec_global_setting->settings_dtvcc->report = report_dtvcc;
	ctx->mp4_cfg.mp4vidtrack = opt->mp4vidtrack;
	// Initialize input files
	ctx->inputfile = opt->inputfile;
	ctx->num_input_files = opt->num_input_files;

	ret = init_ctx_outbase(opt, ctx);
	if (ret < 0)
	{
		goto end;
	}
	ctx->extension = get_file_extension(opt->write_format);

	ctx->subs_delay = opt->subs_delay;

	ctx->pesheaderbuf = (unsigned char *)malloc(188); // Never larger anyway

	ctx->cc_to_stdout = opt->cc_to_stdout;
	ctx->pes_header_to_stdout = opt->pes_header_to_stdout;
	ctx->ignore_pts_jumps = opt->ignore_pts_jumps;

	ctx->hauppauge_mode = opt->hauppauge_mode;
	ctx->live_stream = opt->live_stream;
	ctx->binary_concat = opt->binary_concat;
	build_parity_table();

	ctx->demux_ctx = init_demuxer(ctx, &opt->demux_cfg);
	INIT_LIST_HEAD(&ctx->dec_ctx_head);
	INIT_LIST_HEAD(&ctx->enc_ctx_head);

	// Init timing
	ccx_common_timing_init(&ctx->demux_ctx->past, opt->nosync);
	ctx->multiprogram = opt->multiprogram;
	ctx->write_format = opt->write_format;
	ctx->out_interval = opt->out_interval;
	ctx->segment_on_key_frames_only = opt->segment_on_key_frames_only;
	ctx->segment_counter = 0;
	ctx->system_start_time = -1;

end:
	if (ret != EXIT_OK)
	{
		free(ctx);
		return NULL;
	}
	return ctx;
}

void dinit_libraries(struct lib_ccx_ctx **ctx)
{
	struct lib_ccx_ctx *lctx = *ctx;
	struct encoder_ctx *enc_ctx;
	struct lib_cc_decode *dec_ctx;
	struct lib_cc_decode *dec_ctx1;
	int i;
	list_for_each_entry_safe(dec_ctx, dec_ctx1, &lctx->dec_ctx_head, list, struct lib_cc_decode)
	{
		LLONG cfts;
		if (dec_ctx->codec == CCX_CODEC_DVB)
			dvbsub_close_decoder(&dec_ctx->private_data);
		// Test memory for teletext
		else if (dec_ctx->codec == CCX_CODEC_TELETEXT)
			telxcc_close(&dec_ctx->private_data, &dec_ctx->dec_sub);
		else if (dec_ctx->codec == CCX_CODEC_ISDB_CC)
			delete_isdb_decoder(&dec_ctx->private_data);

		flush_cc_decode(dec_ctx, &dec_ctx->dec_sub);
		cfts = get_fts(dec_ctx->timing, dec_ctx->current_field);
		enc_ctx = get_encoder_by_pn(lctx, dec_ctx->program_number);
		if (enc_ctx && dec_ctx->dec_sub.got_output == CCX_TRUE)
		{
			encode_sub(enc_ctx, &dec_ctx->dec_sub);
			dec_ctx->dec_sub.got_output = CCX_FALSE;
		}
		list_del(&dec_ctx->list);
		dinit_cc_decode(&dec_ctx);
		if (enc_ctx)
		{
			list_del(&enc_ctx->list);
			dinit_encoder(&enc_ctx, cfts);
		}
	}

	// free EPG memory
	EPG_free(lctx);
	freep(&lctx->freport.data_from_608);
	freep(&lctx->freport.data_from_708);
	ccx_demuxer_delete(&lctx->demux_ctx);
	dinit_decoder_setting(&lctx->dec_global_setting);
	freep(&ccx_options.enc_cfg.output_filename);
	freep(&lctx->basefilename);
	freep(&lctx->pesheaderbuf);
	for (i = 0; i < lctx->num_input_files; i++)
		freep(&lctx->inputfile[i]);
	freep(&lctx->inputfile);
	freep(ctx);
}

int is_decoder_processed_enough(struct lib_ccx_ctx *ctx)
{
	struct lib_cc_decode *dec_ctx;
	list_for_each_entry(dec_ctx, &ctx->dec_ctx_head, list, struct lib_cc_decode)
	{
		if (dec_ctx->processed_enough == CCX_TRUE && ctx->multiprogram == CCX_FALSE)
			return CCX_TRUE;
	}

	return CCX_FALSE;
}
struct lib_cc_decode *update_decoder_list(struct lib_ccx_ctx *ctx)
{
	struct lib_cc_decode *dec_ctx;
	list_for_each_entry(dec_ctx, &ctx->dec_ctx_head, list, struct lib_cc_decode)
	{
		return dec_ctx;
	}

	if (list_empty(&ctx->dec_ctx_head))
	{
		ctx->dec_global_setting->codec = CCX_CODEC_ATSC_CC;
		ctx->dec_global_setting->program_number = 0;
		dec_ctx = init_cc_decode(ctx->dec_global_setting);
		if (!dec_ctx)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In update_decoder_list: Not enough memory to init_cc_decode.\n");
		list_add_tail(&(dec_ctx->list), &(ctx->dec_ctx_head));

		// DVB related
		dec_ctx->prev = NULL;
		dec_ctx->dec_sub.prev = NULL;
		if (dec_ctx->codec == CCX_CODEC_DVB)
		{
			dec_ctx->prev = malloc(sizeof(struct lib_cc_decode));
			dec_ctx->dec_sub.prev = malloc(sizeof(struct cc_subtitle));
			memset(dec_ctx->dec_sub.prev, 0, sizeof(struct cc_subtitle));
		}
	}
	return dec_ctx;
}

struct lib_cc_decode *update_decoder_list_cinfo(struct lib_ccx_ctx *ctx, struct cap_info *cinfo)
{
	struct lib_cc_decode *dec_ctx = NULL;

	list_for_each_entry(dec_ctx, &ctx->dec_ctx_head, list, struct lib_cc_decode)
	{
		if (!cinfo || ctx->multiprogram == CCX_FALSE)
			return dec_ctx;

		if (dec_ctx->program_number == cinfo->program_number)
			return dec_ctx;
	}
	if (cinfo)
	{
		ctx->dec_global_setting->program_number = cinfo->program_number;
		ctx->dec_global_setting->codec = cinfo->codec;
		ctx->dec_global_setting->private_data = cinfo->codec_private_data;
	}
	else
	{
		ctx->dec_global_setting->program_number = 0;
		ctx->dec_global_setting->codec = CCX_CODEC_ATSC_CC;
	}
	if (ctx->multiprogram == CCX_FALSE)
	{
		if (list_empty(&ctx->dec_ctx_head))
		{
			dec_ctx = init_cc_decode(ctx->dec_global_setting);
			if (!dec_ctx)
				fatal(EXIT_NOT_ENOUGH_MEMORY, "In update_decoder_list_cinfo: Not enough memory allocating dec_ctx (multiprogram == false)\n");
			list_add_tail(&(dec_ctx->list), &(ctx->dec_ctx_head));
		}
	}
	else
	{
		dec_ctx = init_cc_decode(ctx->dec_global_setting);
		if (!dec_ctx)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In update_decoder_list_cinfo: Not enough memory allocating dec_ctx ((multiprogram == true)\n");
		list_add_tail(&(dec_ctx->list), &(ctx->dec_ctx_head));
	}

	// DVB related
	dec_ctx->prev = NULL;
	dec_ctx->dec_sub.prev = NULL;

	return dec_ctx;
}

struct encoder_ctx *update_encoder_list_cinfo(struct lib_ccx_ctx *ctx, struct cap_info *cinfo)
{
	struct encoder_ctx *enc_ctx;
	unsigned int pn = 0;
	unsigned char in_format = 1;

	if (ctx->write_format == CCX_OF_NULL)
		return NULL;

	if (cinfo)
	{
		pn = cinfo->program_number;
		if (cinfo->codec == CCX_CODEC_ISDB_CC)
			in_format = 3;
		else if (cinfo->codec == CCX_CODEC_TELETEXT)
			in_format = 2;
		else
			in_format = 1;
	}
	list_for_each_entry(enc_ctx, &ctx->enc_ctx_head, list, struct encoder_ctx)
	{
		if (ctx->multiprogram == CCX_FALSE)
			return enc_ctx;

		if (enc_ctx->program_number == pn)
			return enc_ctx;
	}

	const char *extension = get_file_extension(ccx_options.enc_cfg.write_format);
	if (!extension && ccx_options.enc_cfg.write_format != CCX_OF_CURL)
		return NULL;

	if (ctx->multiprogram == CCX_FALSE)
	{
		if (ctx->out_interval != -1)
		{
			int len;

			len = strlen(ctx->basefilename) + 10 + strlen(extension);

			freep(&ccx_options.enc_cfg.output_filename);
			ccx_options.enc_cfg.output_filename = malloc(len);

			sprintf(ccx_options.enc_cfg.output_filename, "%s_%06d%s", ctx->basefilename, ctx->segment_counter + 1, extension);
		}
		if (list_empty(&ctx->enc_ctx_head))
		{
			ccx_options.enc_cfg.program_number = pn;
			ccx_options.enc_cfg.in_format = in_format;
			enc_ctx = init_encoder(&ccx_options.enc_cfg);
			if (!enc_ctx)
				return NULL;
			list_add_tail(&(enc_ctx->list), &(ctx->enc_ctx_head));
		}
	}
	else
	{
		int len;

		len = strlen(ctx->basefilename) + 10 + strlen(extension);

		ccx_options.enc_cfg.program_number = pn;
		ccx_options.enc_cfg.output_filename = malloc(len);
		if (!ccx_options.enc_cfg.output_filename)
		{
			return NULL;
		}

		sprintf(ccx_options.enc_cfg.output_filename, "%s_%d%s", ctx->basefilename, pn, extension);
		enc_ctx = init_encoder(&ccx_options.enc_cfg);
		if (!enc_ctx)
		{
			freep(&ccx_options.enc_cfg.output_filename);
			return NULL;
		}

		list_add_tail(&(enc_ctx->list), &(ctx->enc_ctx_head));
		freep(ccx_options.enc_cfg.output_filename);
	}
	// DVB related
	enc_ctx->prev = NULL;
	if (cinfo)
		if (cinfo->codec == CCX_CODEC_DVB)
			enc_ctx->write_previous = 0;
	return enc_ctx;
}

struct encoder_ctx *update_encoder_list(struct lib_ccx_ctx *ctx)
{
	return update_encoder_list_cinfo(ctx, NULL);
}
