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



struct lib_ccx_ctx* init_libraries(struct ccx_s_options *opt)
{
	int ret = 0;
	struct lib_ccx_ctx *ctx;
	struct ccx_decoder_608_report *report_608;
	struct ccx_decoders_common_settings_t *dec_setting;

	ctx = malloc(sizeof(struct lib_ccx_ctx));
	if(!ctx)
		return NULL;
	memset(ctx,0,sizeof(struct lib_ccx_ctx));

	report_608 = malloc(sizeof(struct ccx_decoder_608_report));
	if (!report_608)
		return NULL;
	memset(report_608,0,sizeof(struct ccx_decoder_608_report));

	// Initialize some constants
	ctx->avc_ctx = init_avc();

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
	dec_setting = init_decoder_setting(opt);
	ctx->dec_ctx = init_cc_decode(dec_setting);
	dinit_decoder_setting(&dec_setting);

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

	// Init timing
	ccx_common_timing_init(&ctx->demux_ctx->past,opt->nosync);
	ctx->multiprogram = opt->multiprogram;


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
	int i;
	// free EPG memory
	EPG_free(lctx);
	ccx_demuxer_delete(&lctx->demux_ctx);
	dinit_avc(&lctx->avc_ctx);
	dinit_cc_decode(&lctx->dec_ctx);
	freep(&lctx->basefilename);
	freep(&lctx->pesheaderbuf);
	freep(&lctx->freport.data_from_608);
	for(i = 0;i < lctx->num_input_files;i++)
		freep(&lctx->inputfile[i]);
	freep(&lctx->inputfile);
	freep(ctx);
}
