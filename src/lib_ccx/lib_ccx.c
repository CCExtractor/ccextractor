#include "lib_ccx.h"
#include "ccx_common_option.h"

struct ccx_common_logging_t ccx_common_logging;
struct ccx_decoders_common_settings_t ccx_decoders_common_settings;
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
	return setting;
}

struct lib_ccx_ctx* init_libraries(struct ccx_s_options *opt)
{
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

	ctx->stream_mode = CCX_SM_ELEMENTARY_OR_NOT_FOUND;
	ctx->auto_stream = opt->auto_stream;
	ctx->screens_to_process = -1;
	ctx->current_file = -1;
	ctx->infd = -1;//Set to -1 to indicate no file is open.
	// Default name for output files if input is stdin
	ctx->basefilename_for_stdin=(char *) "stdin";
	// Default name for output files if input is network
	ctx->basefilename_for_network=(char *) "network";

	// Set logging functions for libraries
	ccx_common_logging.debug_ftn = &dbg_print;
	ccx_common_logging.debug_mask = ccx_options.debug_mask;
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

	// Init encoder helper variables
	ccx_encoders_helpers_setup(ccx_options.encoding, ccx_options.nofontcolor, ccx_options.notypesetting, ccx_options.trim_subs);

	// Init 708 decoder(s)
	ccx_decoders_708_init_library(ctx->basefilename,ctx->extension,ccx_options.print_file_reports);

	// Set output structures for the 608 decoder
	//ctx->dec_ctx->context_cc608_field_1->out = ctx->dec_ctx->wbout1;
	//ctx->dec_ctx->context_cc608_field_2->out = ctx->dec_ctx->wbout2;

	// Init XDS buffers
	ccx_decoders_xds_init_library(&ccx_options.transcript_settings, ctx->subs_delay, ccx_options.millis_separator);
	//xds_cea608_test();

	//Initialize input files
	ctx->inputfile = opt->inputfile;
	ctx->num_input_files = opt->num_input_files;
	ctx->subs_delay = opt->subs_delay;
	ctx->wbout1.filename = opt->wbout2.filename;
	ctx->wbout2.filename = opt->wbout2.filename;


	build_parity_table();
	return ctx;
}
