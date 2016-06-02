#ifdef ENABLE_OCR
#include "hardsubx.h"
#include "capi.h"
#include "allheaders.h"
#include "ocr.h"
#include "utility.h"

//TODO: Correct FFMpeg integration
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

int hardsubx_process_data(struct lib_hardsubx_ctx *ctx)
{
	// 
}

void _hardsubx_params_dump(struct ccx_s_options *options, struct lib_hardsubx_ctx *ctx)
{
	// Print the relevant passed parameters onto the screen
	mprint("Input : %s\n", ctx->inputfile[0]);
}

struct lib_hardsubx_ctx* _init_hardsubx(struct ccx_s_options *options)
{
	// Initialize HardsubX data structures
	struct lib_hardsubx_ctx *ctx = (struct lib_hardsubx_ctx *)malloc(sizeof(struct lib_hardsubx_ctx));
	if(!ctx)
		ccx_common_logging.fatal_ftn(EXIT_NOT_ENOUGH_MEMORY, "lib_hardsubx_ctx");
	memset(ctx, 0, sizeof(struct lib_hardsubx_ctx));

	//Initialize attributes common to lib_ccx context
	ctx->basefilename = get_basename(options->output_filename);//TODO: Check validity, add stdin, network
	ctx->current_file = -1;
	ctx->inputfile = options->inputfile;
	ctx->num_input_files = options->num_input_files;
	ctx->extension = get_file_extension(options->write_format);
	ctx->write_format = options->write_format;
	ctx->subs_delay = options->subs_delay;
	ctx->cc_to_stdout = options->cc_to_stdout;

	return ctx;
}

void _dinit_hardsubx(struct lib_hardsubx_ctx **ctx)
{
	// Free all memory allocated to everything in the context
}

void hardsubx(struct ccx_s_options *options)
{
	// This is similar to the 'main' function in ccextractor.c, but for hard subs
	mprint("HardsubX (Hard Subtitle Extractor) - Burned-in subtitle extraction subsystem\n");

	// Initialize HardsubX data structures
	struct lib_hardsubx_ctx *ctx;
	ctx = _init_hardsubx(options);

	// Dump parameters (Not using params_dump since completely different parameters)
	_hardsubx_params_dump(options, ctx);

	// Configure output settings (write format, capitalization etc)

	// Data processing loop
	time_t start, end;
	time(&start);
	hardsubx_process_data(ctx);

	// Show statistics (time taken, frames processed, mode etc)
	time(&end);
	long processing_time=(long) (end-start);
	mprint ("\rDone, processing time = %ld seconds\n", processing_time);

	// Free all allocated memory for the data structures
}

#endif