#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "activity.h"
#include "utility.h"

struct ccx_common_logging_t ccx_common_logging;
static struct ccx_decoders_common_settings_t *init_decoder_setting(
		struct ccx_s_options *opt)
{
	struct ccx_decoders_common_settings_t *setting;

	setting = malloc(sizeof(struct ccx_decoders_common_settings_t));
	if(!setting)
		return NULL;

	setting->subs_delay = opt->subs_delay;
	setting->output_format = opt->write_format;
	setting->fix_padding = opt->fix_padding;
	setting->extract = opt->extract;
	setting->fullbin = opt->fullbin;
	memcpy(&setting->extraction_start,&opt->extraction_start,sizeof(struct ccx_boundary_time));
	memcpy(&setting->extraction_end,&opt->extraction_end,sizeof(struct ccx_boundary_time));
	setting->cc_to_stdout = opt->cc_to_stdout;
	setting->settings_608 = &opt->settings_608;
	setting->cc_channel = opt->cc_channel;
	setting->send_to_srv = opt->send_to_srv;
	setting->hauppauge_mode = opt->hauppauge_mode;
	return setting;
}
static void dinit_decoder_setting (struct ccx_decoders_common_settings_t **setting)
{
	freep(setting);
}


static int init_ctx_input(struct ccx_s_options *opt, struct lib_ccx_ctx *ctx)
{
	char *file;

	switch (opt->input_source)
	{
		case CCX_DS_FILE:
			if(!ctx->inputfile || !ctx->inputfile[0]) {
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
	return 0;
}

struct lib_cc_decode *get_decoder_by_pn(struct lib_ccx_ctx *ctx, int pn)
{
	struct lib_cc_decode *dec_ctx;
	list_for_each_entry(dec_ctx, &ctx->dec_ctx_head, list, struct lib_cc_decode)
	{
		if (dec_ctx->program_number == pn)
			return dec_ctx;
	}
	return NULL;
}

struct lib_ccx_ctx* init_libraries(struct ccx_s_options *opt)
{
	int ret = 0;
	struct lib_ccx_ctx *ctx;
	struct ccx_decoder_608_report *report_608;

	ctx = malloc(sizeof(struct lib_ccx_ctx));
	if(!ctx)
		return NULL;
	memset(ctx,0,sizeof(struct lib_ccx_ctx));

	report_608 = malloc(sizeof(struct ccx_decoder_608_report));
	if (!report_608)
		return NULL;
	memset(report_608,0,sizeof(struct ccx_decoder_608_report));

	// Initialize some constants
	ctx->screens_to_process = -1;
	ctx->current_file = -1;

	// Set logging functions for libraries
	ccx_common_logging.debug_ftn = &dbg_print;
	ccx_common_logging.debug_mask = opt->debug_mask;
	ccx_common_logging.fatal_ftn = &fatal;
	ccx_common_logging.log_ftn = &mprint;
	ccx_common_logging.gui_ftn = &activity_library_process;

	// Need to set the 608 data for the report to the correct variable.
	ctx->freport.data_from_608 = report_608;
	// Same applies for 708 data
	ctx->freport.data_from_708 = &ccx_decoder_708_report;

	// Init shared decoder settings
	ctx->dec_global_setting = init_decoder_setting(opt);

	//Initialize input files
	ctx->inputfile = opt->inputfile;
	ctx->num_input_files = opt->num_input_files;

	ret = init_ctx_input(opt, ctx);
	if (ret < 0) {
		goto end;
	}

	// Init 708 decoder(s)
	ccx_decoders_708_init_library(opt->print_file_reports);

	// Init XDS buffers
	ccx_decoders_xds_init_library(&opt->transcript_settings, ctx->subs_delay, opt->millis_separator);
	//xds_cea608_test();

	ctx->subs_delay = opt->subs_delay;

	ctx->pesheaderbuf = (unsigned char *) malloc (188); // Never larger anyway

	ctx->cc_to_stdout = opt->cc_to_stdout;

	ctx->hauppauge_mode = opt->hauppauge_mode;
	ctx->live_stream = opt->live_stream;
	ctx->binary_concat = opt->binary_concat;
	build_parity_table();

	ctx->demux_ctx = init_demuxer(ctx, &opt->demux_cfg);
	INIT_LIST_HEAD(&ctx->dec_ctx_head);
	INIT_LIST_HEAD(&ctx->enc_ctx_head);

	// Init timing
	ccx_common_timing_init(&ctx->demux_ctx->past,opt->nosync);
	ctx->multiprogram = opt->multiprogram;
	ctx->write_format = opt->write_format;


end:
	if (ret != EXIT_OK)
	{
		free(ctx);
		return NULL;
	}
	return ctx;
}

void dinit_libraries( struct lib_ccx_ctx **ctx)
{
	struct lib_ccx_ctx *lctx = *ctx;
	struct encoder_ctx *enc_ctx;
	struct encoder_ctx *enc_ctx1;
	struct lib_cc_decode *dec_ctx;
	int i;
	list_for_each_entry_safe(enc_ctx, enc_ctx1, &lctx->enc_ctx_head, list, struct encoder_ctx)
	{
		dec_ctx = get_decoder_by_pn(lctx, enc_ctx->program_number);
		if(dec_ctx)
		{
			flush_cc_decode(dec_ctx, &dec_ctx->dec_sub);
			if (dec_ctx->dec_sub.got_output == CCX_TRUE)
			{
				encode_sub(enc_ctx, &dec_ctx->dec_sub);
				dec_ctx->dec_sub.got_output = CCX_FALSE;
			}
			list_del(&dec_ctx->list);
			dinit_cc_decode(&dec_ctx);
		}
		list_del(&enc_ctx->list);
		dinit_encoder(&enc_ctx);
	}


	// free EPG memory
	EPG_free(lctx);
	ccx_demuxer_delete(&lctx->demux_ctx);
	dinit_decoder_setting(&lctx->dec_global_setting);
	freep(&lctx->basefilename);
	freep(&lctx->pesheaderbuf);
	freep(&lctx->freport.data_from_608);
	for(i = 0;i < lctx->num_input_files;i++)
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
			fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory\n");
		list_add_tail( &(dec_ctx->list), &(ctx->dec_ctx_head) );
	}
	return dec_ctx;
}

struct lib_cc_decode *update_decoder_list_cinfo(struct lib_ccx_ctx *ctx, struct cap_info* cinfo)
{
	struct lib_cc_decode *dec_ctx = NULL;

	list_for_each_entry(dec_ctx, &ctx->dec_ctx_head, list, struct lib_cc_decode)
	{
		if (!cinfo)
			return dec_ctx;

		if (dec_ctx->program_number == cinfo->program_number)
			return dec_ctx;
	}
	if(cinfo)
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
	if(ctx->multiprogram == CCX_FALSE)
	{
		if (list_empty(&ctx->dec_ctx_head))
		{
			dec_ctx = init_cc_decode(ctx->dec_global_setting);
			if (!dec_ctx)
				fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory\n");
			list_add_tail( &(dec_ctx->list), &(ctx->dec_ctx_head) );
		}
	}
	else
	{
		dec_ctx = init_cc_decode(ctx->dec_global_setting);
		if (!dec_ctx)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory\n");
		list_add_tail( &(dec_ctx->list), &(ctx->dec_ctx_head) );
	}
	return dec_ctx;
}

struct encoder_ctx *update_encoder_list_cinfo(struct lib_ccx_ctx *ctx, struct cap_info* cinfo)
{
	struct encoder_ctx *enc_ctx;
	unsigned int pn = 0;
	unsigned char in_format = 1;

	if (ctx->write_format == CCX_OF_NULL)
		return NULL;

	if(cinfo)
	{
		pn = cinfo->program_number;
		if (cinfo->codec == CCX_CODEC_TELETEXT)
			in_format = 2;
		else
			in_format = 1;
	}
	list_for_each_entry(enc_ctx, &ctx->enc_ctx_head, list, struct encoder_ctx)
	{
		if (enc_ctx->program_number == pn)
			return enc_ctx;
	}
	if(ctx->multiprogram == CCX_FALSE)
	{
		if (list_empty(&ctx->enc_ctx_head))
		{
			ccx_options.enc_cfg.program_number = pn;
			ccx_options.enc_cfg.in_format = in_format;
			enc_ctx = init_encoder(&ccx_options.enc_cfg);
			if (!enc_ctx)
				fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory\n");
			list_add_tail( &(enc_ctx->list), &(ctx->enc_ctx_head) );
		}
	}
	else
	{
		int len;
		char *basefilename;
		char *extension;

		extension = get_file_extension(ccx_options.enc_cfg.write_format);
		if(ccx_options.output_filename)
			basefilename = get_basename(ccx_options.output_filename);
		else
			basefilename = get_basename(ccx_options.inputfile[0]);
		len = strlen(basefilename) + 10 + strlen(extension);

		ccx_options.enc_cfg.program_number = pn;
		ccx_options.enc_cfg.output_filename = malloc(len);
		sprintf(ccx_options.enc_cfg.output_filename, "%s_%d%s", basefilename, pn, extension);
		enc_ctx = init_encoder(&ccx_options.enc_cfg);
		if (!enc_ctx)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory\n");
		list_add_tail( &(enc_ctx->list), &(ctx->enc_ctx_head) );
	}
	return enc_ctx;
}

struct encoder_ctx *update_encoder_list(struct lib_ccx_ctx *ctx)
{
	return update_encoder_list_cinfo(ctx, NULL);
}
