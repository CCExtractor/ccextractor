#include "lib_ccx.h"
#include "ccx_common_option.h"


struct lib_ccx_ctx* init_libraries(void)
{
	struct lib_ccx_ctx *ctx;

	ctx = malloc(sizeof(struct lib_ccx_ctx));
	if(!ctx)
		return NULL;
	memset(ctx,0,sizeof(struct lib_ccx_ctx));

	ctx->stream_mode = CCX_SM_ELEMENTARY_OR_NOT_FOUND;
	ctx->auto_stream = CCX_SM_AUTODETECT;
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

	// Init shared decoder settings
	ccx_decoders_common_settings_init(ctx->subs_delay, ccx_options.write_format);
	// Init encoder helper variables
	ccx_encoders_helpers_setup(ccx_options.encoding, ccx_options.nofontcolor, ccx_options.notypesetting, ccx_options.trim_subs);

	// Prepare 608 context
	ctx->context_cc608_field_1 = ccx_decoder_608_init_library(
		ccx_options.settings_608,
		ccx_options.cc_channel,
		1,
		ccx_options.trim_subs,
		ccx_options.encoding,
		&ctx->processed_enough,
		&ctx->cc_to_stdout
		);
	ctx->context_cc608_field_2 = ccx_decoder_608_init_library(
		ccx_options.settings_608,
		ccx_options.cc_channel,
		2,
		ccx_options.trim_subs,
		ccx_options.encoding,
		&ctx->processed_enough,
		&ctx->cc_to_stdout
		);

	// Init 708 decoder(s)
	ccx_decoders_708_init_library(ctx->basefilename,ctx->extension,ccx_options.print_file_reports);

	// Set output structures for the 608 decoder
	ctx->context_cc608_field_1.out = &ctx->wbout1;
	ctx->context_cc608_field_2.out = &ctx->wbout2;

	// Init XDS buffers
	ccx_decoders_xds_init_library(&ccx_options.transcript_settings, ctx->subs_delay, ccx_options.millis_separator);
	//xds_cea608_test();
	return ctx;
}
