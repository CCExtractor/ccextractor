#include "lib_ccx.h"
#include "ccx_common_option.h"

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
	setting->trim_subs = opt->trim_subs;
	setting->send_to_srv = opt->send_to_srv;
	return setting;
}
static void dinit_decoder_setting (struct ccx_decoders_common_settings_t **setting)
{
	freep(setting);
}


static int init_ctx_input(struct ccx_s_options *opt, struct lib_ccx_ctx *ctx)
{
	int len;
	char *file,*c;

	switch (opt->input_source)
	{
		case CCX_DS_FILE:
			if(!ctx->inputfile || !ctx->inputfile[0]) {
				errno = EINVAL;
				return -1;
			}
			len = strlen (ctx->inputfile[0]) + 1;
			file = ctx->inputfile[0];
			break;
		case CCX_DS_STDIN:
			len = strlen ("stdin") + 1;
			file = "stdin";
			break;
		case CCX_DS_NETWORK:
		case CCX_DS_TCP:
			len = strlen ("network") + 1;
			file = "network";
			break;
		default:
			errno = EINVAL;
			return -1;
	}

	ctx->basefilename = (char *) malloc(len);
	if (ctx->basefilename == NULL) {
		return -1;
	}

	strcpy (ctx->basefilename, file);

	for (c = ctx->basefilename + len - 1; c > ctx->basefilename && *c != '.'; c--)
	{;} // Get last .
	if (*c == '.')
		*c = 0;

	return 0;
}


static int init_ctx_extension(struct ccx_s_options *opt, struct lib_ccx_ctx *ctx)
{
	switch (opt->write_format)
	{
		case CCX_OF_RAW:
			ctx->extension = ".raw";
			break;
		case CCX_OF_SRT:
			ctx->extension = ".srt";
			break;
		case CCX_OF_SAMI:
			ctx->extension = ".smi";
			break;
		case CCX_OF_SMPTETT:
			ctx->extension = ".ttml";
			break;
		case CCX_OF_TRANSCRIPT:
			ctx->extension = ".txt";
			break;
		case CCX_OF_RCWT:
			ctx->extension = ".bin";
			break;
		case CCX_OF_SPUPNG:
			ctx->extension = ".xml";
			break;
		case CCX_OF_NULL:
			ctx->extension = "";
			break;
		case CCX_OF_DVDRAW:
			ctx->extension = ".dvdraw";
			break;
		default:
			mprint ("write_format doesn't have any legal value, this is a bug.\n");
			errno = EINVAL;
			return -1;
	}
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

	ctx->capbufsize = 20000;
	ctx->capbuf = NULL;
	ctx->capbuflen = 0; // Bytes read in capbuf

	// Initialize some constants
	init_ts(ctx);
	init_avc();

	ctx->stream_mode = CCX_SM_ELEMENTARY_OR_NOT_FOUND;
	ctx->auto_stream = opt->auto_stream;
	ctx->m2ts = opt->m2ts;
	ctx->screens_to_process = -1;
	ctx->current_file = -1;
	ctx->infd = -1;//Set to -1 to indicate no file is open.

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

	// Init encoder helper variables
	ccx_encoders_helpers_setup(opt->encoding, opt->nofontcolor, opt->notypesetting, opt->trim_subs);

	//Initialize input files
	ctx->inputfile = opt->inputfile;
	ctx->num_input_files = opt->num_input_files;

	ret = init_ctx_input(opt, ctx);
	if (ret < 0) {
		goto end;
	}

	ret = init_ctx_extension(opt, ctx);
	if (ret < 0) {
		goto end;
	}
	// Init 708 decoder(s)
	ccx_decoders_708_init_library(ctx->basefilename, ctx->extension, opt->print_file_reports);

	// Set output structures for the 608 decoder
	//ctx->dec_ctx->context_cc608_field_1->out = ctx->dec_ctx->wbout1;
	//ctx->dec_ctx->context_cc608_field_2->out = ctx->dec_ctx->wbout2;

	// Init XDS buffers
	ccx_decoders_xds_init_library(&opt->transcript_settings, ctx->subs_delay, opt->millis_separator);
	//xds_cea608_test();

	ctx->subs_delay = opt->subs_delay;
	ctx->wbout1.filename = opt->wbout2.filename;
	ctx->wbout2.filename = opt->wbout2.filename;
	ctx->buffer = (unsigned char *) malloc (BUFSIZE);
	ctx->pesheaderbuf = (unsigned char *) malloc (188); // Never larger anyway

	// Init timing
	ccx_common_timing_init(&ctx->past,opt->nosync);

	ctx->cc_to_stdout = opt->cc_to_stdout;

	ctx->hauppauge_mode = opt->hauppauge_mode;
	ctx->live_stream = opt->live_stream;
	ctx->binary_concat = opt->binary_concat;
	build_parity_table();

end:
	if (ret < 0) {
		free(ctx);
		return NULL;
	}
	return ctx;
}

void dinit_libraries( struct lib_ccx_ctx **ctx)
{
	struct lib_ccx_ctx *lctx = *ctx;
	int i = 0;
	for (i = 0; i < MAX_PID; i++)
	{
		if( lctx->PIDs_programs[i])
			freep(lctx->PIDs_programs + i);
	}
	// free EPG memory
	EPG_free(lctx);
	dinit_ts(lctx);
	dinit_cc_decode(&lctx->dec_ctx);
	freep(&lctx->buffer);
	freep(&lctx->pesheaderbuf);
	freep(&lctx->freport.data_from_608);
	freep(ctx);
}
