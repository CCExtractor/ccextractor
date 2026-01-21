#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "activity.h"
#include "utility.h"
#include "dvb_subtitle_decoder.h"
#include "ccx_decoders_708.h"
#include "ccx_decoders_isdb.h"
#include "ccx_decoders_common.h"
#ifdef ENABLE_OCR
#include "ocr.h"
#endif

struct ccx_common_logging_t ccx_common_logging;
static struct ccx_decoders_common_settings_t *init_decoder_setting(
    struct ccx_s_options *opt)
{
	struct ccx_decoders_common_settings_t *setting;

	setting = calloc(1, sizeof(struct ccx_decoders_common_settings_t));
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
	// program_number, codec, and private_data are zero-initialized by calloc

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
		if (ctx->basefilename)
		{
			size_t len = strlen(ctx->basefilename);
			if (len > 0 && (ctx->basefilename[len - 1] == '/' || ctx->basefilename[len - 1] == '\\'))
			{
				char *input_base = NULL;
				if (opt->input_source == CCX_DS_FILE && ctx->inputfile && ctx->inputfile[0])
					input_base = get_basename(ctx->inputfile[0]);
				else if (opt->input_source == CCX_DS_STDIN)
					input_base = get_basename("stdin");
				else if (opt->input_source == CCX_DS_NETWORK || opt->input_source == CCX_DS_TCP)
					input_base = get_basename("network");

				if (input_base)
				{
					size_t new_len = len + strlen(input_base) + 1;
					char *new_base = malloc(new_len);
					if (new_base)
					{
						snprintf(new_base, new_len, "%s%s", ctx->basefilename, input_base);
						free(ctx->basefilename);
						ctx->basefilename = new_base;
					}
					free(input_base);
				}
			}
		}
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

	activity_header(); // Brag about writing it :-)

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
		if (!ctx->epg_buffers || !ctx->eit_programs || !ctx->eit_current_events || !ctx->ATSC_source_pg_map)
			ccx_common_logging.fatal_ftn(EXIT_NOT_ENOUGH_MEMORY, "lib_ccx_ctx");
		memset(ctx->epg_buffers, 0, sizeof(struct PSI_buffer) * (0xfff + 1));
		memset(ctx->eit_programs, 0, sizeof(struct EIT_program) * (TS_PMT_MAP_SIZE + 1));
		memset(ctx->eit_current_events, 0, sizeof(int32_t) * (TS_PMT_MAP_SIZE + 1));
		memset(ctx->ATSC_source_pg_map, 0, sizeof(int16_t) * (0xffff));
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
		free(report_608);
		free(report_dtvcc);
		EPG_free(ctx);
		free(ctx);
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
	if (!ctx->pesheaderbuf)
		ccx_common_logging.fatal_ftn(EXIT_NOT_ENOUGH_MEMORY, "init_libraries: Not enough memory allocating pesheaderbuf");

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

	// Initialize pipeline infrastructure
	ctx->pipeline_count = 0;
	ctx->dec_dvb_default = NULL;
#ifdef _WIN32
	InitializeCriticalSection(&ctx->pipeline_mutex);
#else
	pthread_mutex_init(&ctx->pipeline_mutex, NULL);
#endif
	ctx->pipeline_mutex_initialized = 1;
	ctx->shared_ocr_ctx = NULL;
	memset(ctx->pipelines, 0, sizeof(ctx->pipelines));

end:
	if (ret != EXIT_OK)
	{
		dinit_decoder_setting(&ctx->dec_global_setting);
		free(ctx->freport.data_from_608);
		free(ctx->freport.data_from_708);
		EPG_free(ctx);
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
		void *saved_private_data = dec_ctx->private_data; // Save before close NULLs it
		if (dec_ctx->codec == CCX_CODEC_DVB)
			dvbsub_close_decoder(&dec_ctx->private_data);
		// Test memory for teletext
		else if (dec_ctx->codec == CCX_CODEC_TELETEXT)
			telxcc_close(&dec_ctx->private_data, &dec_ctx->dec_sub);
		else if (dec_ctx->codec == CCX_CODEC_ISDB_CC)
			delete_isdb_decoder(&dec_ctx->private_data);

		// Also NULL out any cinfo entries that shared this private_data pointer
		// to prevent double-free in dinit_cap
		if (saved_private_data && lctx->demux_ctx)
		{
			struct cap_info *cinfo_iter;
			list_for_each_entry(cinfo_iter, &lctx->demux_ctx->cinfo_tree.all_stream, all_stream, struct cap_info)
			{
				if (cinfo_iter->codec_private_data == saved_private_data)
					cinfo_iter->codec_private_data = NULL;
			}
		}

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

	// Cleanup subtitle pipelines (split DVB mode)
	for (i = 0; i < lctx->pipeline_count; i++)
	{
		struct ccx_subtitle_pipeline *p = lctx->pipelines[i];
		if (!p)
			continue;

		// Correct closing order to ensure last subtitle is written
		if (p->dec_ctx && p->encoder)
		{
			// Check if there's a pending subtitle in the prev buffer
			if (p->dec_ctx->dec_sub.prev && p->dec_ctx->dec_sub.prev->data && p->encoder->prev)
			{
				// Calculate end time for the last subtitle
				LLONG current_fts = 0;
				if (p->dec_ctx->timing)
				{
					current_fts = get_fts(p->dec_ctx->timing, p->dec_ctx->current_field);
				}

				// Force end time if missing
				if (p->dec_ctx->dec_sub.prev->end_time == 0)
				{
					if (current_fts > p->dec_ctx->dec_sub.prev->start_time)
						p->dec_ctx->dec_sub.prev->end_time = current_fts;
					else
						p->dec_ctx->dec_sub.prev->end_time = p->dec_ctx->dec_sub.prev->start_time + 2000; // 2s fallback
				}

				encode_sub(p->encoder->prev, p->dec_ctx->dec_sub.prev);
			}
		}

		if (p->decoder)
			dvbsub_close_decoder(&p->decoder);

#ifdef ENABLE_OCR
		if (p->ocr_ctx)
			delete_ocr(&p->ocr_ctx);
#endif

		if (p->encoder)
			dinit_encoder(&p->encoder, 0);

		if (p->timing)
			dinit_timing_ctx(&p->timing);

		if (p->dec_ctx)
		{
			// private_data points to p->decoder which is freed above
			if (p->dec_ctx->prev)
			{
				// prev->private_data is allocated by copy_decoder_context
				freep(&p->dec_ctx->prev->private_data);
				free(p->dec_ctx->prev);
			}
			// Free dec_sub.prev which was allocated in init_cc_decode or general_loop
			// Note: free_subtitle already frees the struct via freep(&sub), so we don't call free() again
			if (p->dec_ctx->dec_sub.prev)
			{
				free_subtitle(p->dec_ctx->dec_sub.prev);
				p->dec_ctx->dec_sub.prev = NULL;
			}
			p->dec_ctx->private_data = NULL;
			free(p->dec_ctx);
		}
		free_subtitle(p->sub.prev);
		free(p);
		lctx->pipelines[i] = NULL;
	}
	lctx->pipeline_count = 0;

	// Cleanup mutex
	if (lctx->pipeline_mutex_initialized)
	{
#ifdef _WIN32
		DeleteCriticalSection(&lctx->pipeline_mutex);
#else
		pthread_mutex_destroy(&lctx->pipeline_mutex);
#endif
		lctx->pipeline_mutex_initialized = 0;
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
	freep(&lctx->inputfile);
#ifdef ENABLE_OCR
	if (lctx->shared_ocr_ctx)
		delete_ocr(&lctx->shared_ocr_ctx);
#endif
	freep(ctx);
}

int is_decoder_processed_enough(struct lib_ccx_ctx *ctx)
{
	struct lib_cc_decode *dec_ctx;

	// If the decoder list is empty, no user-defined limits could have been reached
	if (list_empty(&ctx->dec_ctx_head))
		return CCX_FALSE;

	if (ctx->multiprogram == CCX_FALSE)
	{
		// In single-program mode, return TRUE if ANY decoder has processed enough
		list_for_each_entry(dec_ctx, &ctx->dec_ctx_head, list, struct lib_cc_decode)
		{
			if (dec_ctx->processed_enough == CCX_TRUE)
				return CCX_TRUE;
		}
		return CCX_FALSE;
	}
	else
	{
		// In multiprogram mode, return TRUE only if ALL decoders have processed enough
		list_for_each_entry(dec_ctx, &ctx->dec_ctx_head, list, struct lib_cc_decode)
		{
			if (dec_ctx->processed_enough == CCX_FALSE)
				return CCX_FALSE;
		}
		return CCX_TRUE;
	}
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
			if (!dec_ctx->prev || !dec_ctx->dec_sub.prev)
				ccx_common_logging.fatal_ftn(EXIT_NOT_ENOUGH_MEMORY, "update_decoder_list: Not enough memory for DVB context");
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
		{
			/* Update private_data from cinfo if available.
			   This is needed after PAT changes when dinit_cap() freed the old context
			   and a new cap_info was created with a new codec_private_data. */
			if (cinfo && cinfo->codec_private_data)
				dec_ctx->private_data = cinfo->codec_private_data;
			return dec_ctx;
		}

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
			// Format: "%s_%06d%s" needs: basefilename + '_' + up to 10 digits + extension + null
			size_t len = strlen(ctx->basefilename) + 1 + 10 + strlen(extension) + 1;

			freep(&ccx_options.enc_cfg.output_filename);
			ccx_options.enc_cfg.output_filename = malloc(len);
			if (!ccx_options.enc_cfg.output_filename)
			{
				return NULL;
			}

			snprintf(ccx_options.enc_cfg.output_filename, len, "%s_%06d%s", ctx->basefilename, ctx->segment_counter + 1, extension);
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
		// Format: "%s_%d%s" needs: basefilename + '_' + up to 10 digits + extension + null
		size_t len = strlen(ctx->basefilename) + 1 + 10 + strlen(extension) + 1;

		ccx_options.enc_cfg.program_number = pn;
		ccx_options.enc_cfg.output_filename = malloc(len);
		if (!ccx_options.enc_cfg.output_filename)
		{
			return NULL;
		}

		snprintf(ccx_options.enc_cfg.output_filename, len, "%s_%d%s", ctx->basefilename, pn, extension);
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

/**
 * Get or create a subtitle pipeline for a specific PID/language.
 * Used when --split-dvb-subs is enabled to route each DVB stream to its own output file.
 */
struct ccx_subtitle_pipeline *get_or_create_pipeline(struct lib_ccx_ctx *ctx, int pid, int stream_type, const char *lang)
{
	int i;

	// Lock mutex for thread safety
#ifdef _WIN32
	EnterCriticalSection(&ctx->pipeline_mutex);
#else
	pthread_mutex_lock(&ctx->pipeline_mutex);
#endif

	// Search for existing pipeline
	for (i = 0; i < ctx->pipeline_count; i++)
	{
		struct ccx_subtitle_pipeline *p = ctx->pipelines[i];
		if (p && p->pid == pid && p->stream_type == stream_type &&
		    strcmp(p->lang, lang) == 0)
		{
#ifdef _WIN32
			LeaveCriticalSection(&ctx->pipeline_mutex);
#else
			pthread_mutex_unlock(&ctx->pipeline_mutex);
#endif
			return p;
		}
	}

	// Check capacity
	if (ctx->pipeline_count >= MAX_SUBTITLE_PIPELINES)
	{
		mprint("Warning: Maximum subtitle pipelines (%d) reached, cannot create new pipeline for PID 0x%X\n",
		       MAX_SUBTITLE_PIPELINES, pid);
#ifdef _WIN32
		LeaveCriticalSection(&ctx->pipeline_mutex);
#else
		pthread_mutex_unlock(&ctx->pipeline_mutex);
#endif
		return NULL;
	}

	// Allocate new pipeline
	struct ccx_subtitle_pipeline *pipe = calloc(1, sizeof(struct ccx_subtitle_pipeline));
	if (!pipe)
	{
		mprint("Error: Failed to allocate memory for subtitle pipeline\n");
#ifdef _WIN32
		LeaveCriticalSection(&ctx->pipeline_mutex);
#else
		pthread_mutex_unlock(&ctx->pipeline_mutex);
#endif
		return NULL;
	}

	pipe->pid = pid;
	pipe->stream_type = stream_type;
	snprintf(pipe->lang, sizeof(pipe->lang), "%.3s", lang ? lang : "und");

	// Generate output filename: {basefilename}_{lang}_{PID}.ext
	// Always include PID to handle multiple streams with same language
	const char *ext = ctx->extension ? ctx->extension : ".srt";
	if (strcmp(pipe->lang, "und") == 0 || strcmp(pipe->lang, "unk") == 0 || pipe->lang[0] == '\0')
	{
		snprintf(pipe->filename, sizeof(pipe->filename), "%s_0x%04X%s",
			 ctx->basefilename, pid, ext);
	}
	else
	{
		snprintf(pipe->filename, sizeof(pipe->filename), "%s_%s_0x%04X%s",
			 ctx->basefilename, pipe->lang, pid, ext);
	}

	// Initialize encoder for this pipeline
	struct encoder_cfg cfg = ccx_options.enc_cfg;
	cfg.output_filename = pipe->filename;
	pipe->encoder = init_encoder(&cfg);
	// DVB subtitles require a 'prev' encoder context for buffering (write_previous logic).
	// Without this, the first call to dvbsub_handle_display_segment may fail or result in no output.
	if (pipe->encoder)
	{
		pipe->encoder->prev = copy_encoder_context(pipe->encoder);
		if (!pipe->encoder->prev)
		{
			mprint("Error: Failed to allocate prev context for PID 0x%X\n", pid);
			dinit_encoder(&pipe->encoder, 0);
			free(pipe);
			return NULL;
		}
		// FIX Bug 3: Set write_previous=1 so the FIRST subtitle gets written
		// With write_previous=0, first subtitle is only buffered and never encoded
		// unless a second subtitle arrives. Setting to 1 enables immediate encoding.
		pipe->encoder->write_previous = 1;

		// Issue 4: Ensure prev context exists and is initialized
		// This forces the "previous" subtitle (which is effectively the first one we see)
		// to be eligible for writing when the NEXT segment arrives.
		// pipe->sub.prev will be allocated when we set up dec_ctx below.
	}

	if (!pipe->encoder)
	{
		mprint("Error: Failed to create encoder for pipeline PID 0x%X\n", pid);
		free(pipe);
#ifdef _WIN32
		LeaveCriticalSection(&ctx->pipeline_mutex);
#else
		pthread_mutex_unlock(&ctx->pipeline_mutex);
#endif
		return NULL;
	}

	// Timing context: Create independent timing context for pipeline
	// This ensures DVB streams track their own PTS/FTS without race conditions
	// with the primary Teletext stream.
	pipe->timing = init_timing_ctx(&ccx_common_timing_settings);
	if (!pipe->timing)
	{
		mprint("Error: Failed to initialize timing for pipeline PID 0x%X\n", pid);
		dinit_encoder(&pipe->encoder, 0);
		free(pipe);
#ifdef _WIN32
		LeaveCriticalSection(&ctx->pipeline_mutex);
#else
		pthread_mutex_unlock(&ctx->pipeline_mutex);
#endif
		return NULL;
	}

	// Initialize DVB decoder
	struct dvb_config dvb_cfg = {0};
	dvb_cfg.n_language = 1;

	// Lookup metadata to use correct Composition and Ancillary Page IDs
	// This ensures we respect the configuration advertised in the PMT
	if (ctx->demux_ctx)
	{
		for (i = 0; i < ctx->demux_ctx->potential_stream_count; i++)
		{
			if (ctx->demux_ctx->potential_streams[i].pid == pid)
			{
				dvb_cfg.composition_id[0] = ctx->demux_ctx->potential_streams[i].composition_id;
				dvb_cfg.ancillary_id[0] = ctx->demux_ctx->potential_streams[i].ancillary_id;
				break;
			}
		}
	}

	pipe->decoder = dvbsub_init_decoder(&dvb_cfg);
	if (!pipe->decoder)
	{
		mprint("Error: Failed to create DVB decoder for pipeline PID 0x%X\n", pid);
		dinit_encoder(&pipe->encoder, 0);
		dinit_timing_ctx(&pipe->timing);
		free(pipe);
#ifdef _WIN32
		LeaveCriticalSection(&ctx->pipeline_mutex);
#else
		pthread_mutex_unlock(&ctx->pipeline_mutex);
#endif
		return NULL;
	}

	// Initialize per-pipeline decoder context for DVB state management
	pipe->dec_ctx = calloc(1, sizeof(struct lib_cc_decode));
	if (!pipe->dec_ctx)
	{
		mprint("Error: Failed to create decoder context for pipeline PID 0x%X\n", pid);
		dvbsub_close_decoder(&pipe->decoder);
		dinit_encoder(&pipe->encoder, 0);
		free(pipe);
#ifdef _WIN32
		LeaveCriticalSection(&ctx->pipeline_mutex);
#else
		pthread_mutex_unlock(&ctx->pipeline_mutex);
#endif
		return NULL;
	}
	pipe->dec_ctx->private_data = pipe->decoder;
	pipe->dec_ctx->codec = CCX_CODEC_DVB;
	pipe->dec_ctx->pid = pid;
	pipe->dec_ctx->prev = calloc(1, sizeof(struct lib_cc_decode));
	if (pipe->dec_ctx->prev)
	{
		pipe->dec_ctx->prev->private_data = malloc(dvbsub_get_context_size());
		if (pipe->dec_ctx->prev->private_data)
		{
			dvbsub_copy_context(pipe->dec_ctx->prev->private_data, pipe->decoder);
		}
		pipe->dec_ctx->prev->codec = CCX_CODEC_DVB;
	}

	// Initialize persistent cc_subtitle for DVB prev tracking
	memset(&pipe->sub, 0, sizeof(struct cc_subtitle));
	pipe->sub.prev = calloc(1, sizeof(struct cc_subtitle));
	if (pipe->sub.prev)
	{
		pipe->sub.prev->start_time = -1;
		pipe->sub.prev->end_time = 0;
	}

	// Register pipeline
	ctx->pipelines[ctx->pipeline_count++] = pipe;

	mprint("Created subtitle pipeline for PID 0x%X lang=%s -> %s\n", pid, pipe->lang, pipe->filename);

#ifdef _WIN32
	LeaveCriticalSection(&ctx->pipeline_mutex);
#else
	pthread_mutex_unlock(&ctx->pipeline_mutex);
#endif
	return pipe;
}

/**
 * Set PTS for a subtitle pipeline's timing context.
 * This ensures the DVB decoder uses the correct timestamp for each packet.
 */
void set_pipeline_pts(struct ccx_subtitle_pipeline *pipe, LLONG pts)
{
	if (!pipe || !pipe->timing || pts == CCX_NOPTS)
		return;

	set_current_pts(pipe->timing, pts);

	// Initialize min_pts if not set
	if (pipe->timing->min_pts == 0x01FFFFFFFFLL)
	{
		pipe->timing->min_pts = pts;
		pipe->timing->pts_set = 2; // MinPtsSet
		pipe->timing->sync_pts = pts;
	}

	// For DVB subtitle pipelines, directly calculate fts_now
	// The standard set_fts() relies on video frame type detection which doesn't
	// work for DVB-only streams. Simple calculation: (current_pts - min_pts) in ms
	// MPEG_CLOCK_FREQ = 90000, so divide by 90 to get milliseconds
	pipe->timing->fts_now = (pts - pipe->timing->min_pts) / 90;

	// Also update fts_max if this is the highest timestamp seen
	if (pipe->timing->fts_now > pipe->timing->fts_max)
		pipe->timing->fts_max = pipe->timing->fts_now;
}
