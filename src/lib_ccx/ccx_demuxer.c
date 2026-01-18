#include "ccx_demuxer.h"
#include "activity.h"
#include "lib_ccx.h"
#include "utility.h"
#include "ffmpeg_intgr.h"
#ifndef DISABLE_RUST
void ccxr_demuxer_reset(struct ccx_demuxer *ctx);
void ccxr_demuxer_close(struct ccx_demuxer *ctx);
int ccxr_demuxer_isopen(const struct ccx_demuxer *ctx);
int ccxr_demuxer_open(struct ccx_demuxer *ctx, const char *file);
LLONG ccxr_demuxer_get_file_size(struct ccx_demuxer *ctx);
void ccxr_demuxer_print_cfg(const struct ccx_demuxer *ctx);
#endif

static void ccx_demuxer_reset(struct ccx_demuxer *ctx)
{
#ifndef DISABLE_RUST
	ccxr_demuxer_reset(ctx);
#else
	ctx->startbytes_pos = 0;
	ctx->startbytes_avail = 0;
	ctx->num_of_PIDs = 0;
	memset(ctx->have_PIDs, -1, (MAX_PSI_PID + 1) * sizeof(int));
	memset(ctx->PIDs_seen, 0, 65536 * sizeof(int));
	for (int i = 0; i < (MAX_PSI_PID + 1); i++)
	{
		ctx->min_pts[i] = UINT64_MAX;
	}
	memset(ctx->stream_id_of_each_pid, 0, (MAX_PSI_PID + 1) * sizeof(uint8_t));
	memset(ctx->PIDs_programs, 0, 65536 * sizeof(struct PMT_entry *));
#endif
}

static void ccx_demuxer_close(struct ccx_demuxer *ctx)
{
#ifndef DISABLE_RUST
	ccxr_demuxer_close(ctx);
#else
	ctx->past = 0;
	if (ctx->infd != -1 && ccx_options.input_source == CCX_DS_FILE)
	{
		close(ctx->infd);
		ctx->infd = -1;
		activity_input_file_closed();
	}
#endif
}

static int ccx_demuxer_isopen(struct ccx_demuxer *ctx)
{
#ifndef DISABLE_RUST
	return ccxr_demuxer_isopen(ctx);
#else
	return ctx->infd != -1;
#endif
}

static int ccx_demuxer_open(struct ccx_demuxer *ctx, const char *file)
{
#ifndef DISABLE_RUST
	return ccxr_demuxer_open(ctx, file);
#else
	ctx->past = 0;
	ctx->min_global_timestamp = 0;
	ctx->global_timestamp_inited = 0;
	ctx->last_global_timestamp = 0;
	ctx->offset_global_timestamp = 0;

#ifdef ENABLE_FFMPEG
	ctx->ffmpeg_ctx = init_ffmpeg(file);
	if (ctx->ffmpeg_ctx)
	{
		ctx->stream_mode = CCX_SM_FFMPEG;
		ctx->auto_stream = CCX_SM_FFMPEG;
		return 0;
	}
	else
	{
		mprint("\rFailed to initialized ffmpeg falling back to legacy\n");
	}
#endif
	init_file_buffer(ctx);
	if (ccx_options.input_source == CCX_DS_STDIN)
	{
		if (ctx->infd != -1) // Means we had already processed stdin. So we're done.
		{
			if (ccx_options.print_file_reports)
				print_file_report(ctx->parent);
			return -1;
		}
		ctx->infd = 0;
		mprint("\n\r-----------------------------------------------------------------\n");
		mprint("\rReading from standard input\n");
	}
	else if (ccx_options.input_source == CCX_DS_NETWORK)
	{
		if (ctx->infd != -1) // Means we have already bound a socket.
		{
			if (ccx_options.print_file_reports)
				print_file_report(ctx->parent);

			return -1;
		}

		ctx->infd = start_upd_srv(ccx_options.udpsrc, ccx_options.udpaddr, ccx_options.udpport);
		if (ctx->infd < 0)
		{
			print_error(ccx_options.gui_mode_reports, "socket() failed.");
			return CCX_COMMON_EXIT_BUG_BUG;
		}
	}

	else if (ccx_options.input_source == CCX_DS_TCP)
	{
		if (ctx->infd != -1)
		{
			if (ccx_options.print_file_reports)
				print_file_report(ctx->parent);

			return -1;
		}

		ctx->infd = start_tcp_srv(ccx_options.tcpport, ccx_options.tcp_password);
	}
	else
	{
#ifdef _WIN32
		ctx->infd = OPEN(file, O_RDONLY | O_BINARY);
#else
		ctx->infd = OPEN(file, O_RDONLY);
#endif
		if (ctx->infd < 0)
			return -1;
	}

	if (ctx->auto_stream == CCX_SM_AUTODETECT)
	{
		detect_stream_type(ctx);
		switch (ctx->stream_mode)
		{
			case CCX_SM_ELEMENTARY_OR_NOT_FOUND:
				mprint("\rFile seems to be an elementary stream, enabling ES mode\n");
				break;
			case CCX_SM_TRANSPORT:
				mprint("\rFile seems to be a transport stream, enabling TS mode\n");
				break;
			case CCX_SM_PROGRAM:
				mprint("\rFile seems to be a program stream, enabling PS mode\n");
				break;
			case CCX_SM_ASF:
				mprint("\rFile seems to be an ASF, enabling DVR-MS mode\n");
				break;
			case CCX_SM_WTV:
				mprint("\rFile seems to be a WTV, enabling WTV mode\n");
				break;
			case CCX_SM_MCPOODLESRAW:
				mprint("\rFile seems to be McPoodle raw data\n");
				break;
			case CCX_SM_RCWT:
				mprint("\rFile seems to be a raw caption with time data\n");
				break;
			case CCX_SM_MP4:
				mprint("\rFile seems to be a MP4\n");
				break;
			case CCX_SM_GXF:
				mprint("\rFile seems to be a GXF\n");
				break;
			case CCX_SM_MKV:
				mprint("\rFile seems to be a Matroska/WebM container\n");
				break;
#ifdef WTV_DEBUG
			case CCX_SM_HEX_DUMP:
				mprint("\rFile seems to be an hexadecimal dump\n");
				break;
#endif
			case CCX_SM_MXF:
				mprint("\rFile seems to be an MXF\n");
				break;
			case CCX_SM_MYTH:
			case CCX_SM_AUTODETECT:
				fatal(CCX_COMMON_EXIT_BUG_BUG, "In ccx_demuxer_open: Impossible value in stream_mode. Please file a bug report on GitHub.\n");
				break;
		}
	}
	else
	{
		ctx->stream_mode = ctx->auto_stream;
	}

	// The myth loop autodetect will only be used with ES or PS streams
	switch (ccx_options.auto_myth)
	{
		case 0:
			// Use whatever stream mode says
			break;
		case 1:
			// Force stream mode to myth
			ctx->stream_mode = CCX_SM_MYTH;
			break;
		case 2:
			// autodetect myth files, but only if it does not conflict with
			// the current stream mode
			switch (ctx->stream_mode)
			{
				case CCX_SM_ELEMENTARY_OR_NOT_FOUND:
				case CCX_SM_PROGRAM:
					if (detect_myth(ctx))
					{
						ctx->stream_mode = CCX_SM_MYTH;
					}
					break;
				default:
					// Keep stream_mode
					break;
			}
			break;
	}

	return 0;
#endif
}

LLONG ccx_demuxer_get_file_size(struct ccx_demuxer *ctx)
{
#ifndef DISABLE_RUST
	return ccxr_demuxer_get_file_size(ctx);
#else
	LLONG ret = 0;
	int in = ctx->infd;
	LLONG current = LSEEK(in, 0, SEEK_CUR);
	LLONG length = LSEEK(in, 0, SEEK_END);
	if (current < 0 || length < 0)
		return -1;

	ret = LSEEK(in, current, SEEK_SET);
	if (ret < 0)
		return -1;

	return length;
#endif
}

static int ccx_demuxer_get_stream_mode(struct ccx_demuxer *ctx)
{
	return ctx->stream_mode;
}

static void ccx_demuxer_print_cfg(struct ccx_demuxer *ctx)
{
#ifndef DISABLE_RUST
	ccxr_demuxer_print_cfg(ctx);
#else
	switch (ctx->auto_stream)
	{
		case CCX_SM_ELEMENTARY_OR_NOT_FOUND:
			mprint("Elementary");
			break;
		case CCX_SM_TRANSPORT:
			mprint("Transport");
			break;
		case CCX_SM_PROGRAM:
			mprint("Program");
			break;
		case CCX_SM_ASF:
			mprint("DVR-MS");
			break;
		case CCX_SM_WTV:
			mprint("Windows Television (WTV)");
			break;
		case CCX_SM_MCPOODLESRAW:
			mprint("McPoodle's raw");
			break;
		case CCX_SM_AUTODETECT:
			mprint("Autodetect");
			break;
		case CCX_SM_RCWT:
			mprint("BIN");
			break;
		case CCX_SM_MP4:
			mprint("MP4");
			break;
		case CCX_SM_MKV:
			mprint("MKV");
			break;
		case CCX_SM_MXF:
			mprint("MXF");
			break;
		case CCX_SM_SCC:
			mprint("SCC");
			break;
#ifdef WTV_DEBUG
		case CCX_SM_HEX_DUMP:
			mprint("Hex");
			break;
#endif
		default:
			fatal(CCX_COMMON_EXIT_BUG_BUG, "BUG: Unknown stream mode. Please file a bug report on Github.\n");
			break;
	}
#endif
}

void ccx_demuxer_delete(struct ccx_demuxer **ctx)
{
	struct ccx_demuxer *lctx = *ctx;
	int i;
	dinit_cap(lctx);
	freep(&lctx->last_pat_payload);
	for (i = 0; i < MAX_PSI_PID; i++)
	{
		if (lctx->PID_buffers[i] != NULL && lctx->PID_buffers[i]->buffer != NULL)
		{
			free(lctx->PID_buffers[i]->buffer);
			lctx->PID_buffers[i]->buffer = NULL;
			lctx->PID_buffers[i]->buffer_length = 0;
		}
		freep(&lctx->PID_buffers[i]);
	}
	for (i = 0; i < MAX_PID; i++)
	{
		if (lctx->PIDs_programs[i])
			freep(lctx->PIDs_programs + i);
	}

	freep(&lctx->filebuffer);

	// Reset potential stream discovery data
	lctx->potential_stream_count = 0;
	memset(lctx->potential_streams, 0, sizeof(lctx->potential_streams));

	freep(ctx);
}

struct ccx_demuxer *init_demuxer(void *parent, struct demuxer_cfg *cfg)
{
	int i;
	struct ccx_demuxer *ctx = calloc(1, sizeof(struct ccx_demuxer));
	if (!ctx)
		return NULL;

	ctx->infd = -1; // Set to -1 to indicate no file is open.
	ctx->m2ts = cfg->m2ts;
	ctx->auto_stream = cfg->auto_stream;
	ctx->stream_mode = CCX_SM_ELEMENTARY_OR_NOT_FOUND;

	ctx->ts_autoprogram = cfg->ts_autoprogram;
	ctx->ts_allprogram = cfg->ts_allprogram;
	ctx->ts_datastreamtype = cfg->ts_datastreamtype;
	ctx->nb_program = 0;
	ctx->multi_stream_per_prog = 0;

	for (int i = 0; i < MAX_PROGRAM; i++)
	{
		ctx->pinfo[i].has_all_min_pts = 0;
		for (int j = 0; j < COUNT; j++)
		{
			ctx->pinfo[i].got_important_streams_min_pts[j] = UINT64_MAX;
		}
		ctx->pinfo[i].version = 0xFF; // Not real in a real stream since it's 5 bits. FF => Not initialized
	}

	if (cfg->ts_forced_program != -1)
	{
		ctx->pinfo[ctx->nb_program].pid = CCX_UNKNOWN;
		ctx->pinfo[ctx->nb_program].program_number = cfg->ts_forced_program;
		ctx->flag_ts_forced_pn = CCX_TRUE;
		ctx->nb_program++;
	}
	else
	{
		ctx->flag_ts_forced_pn = CCX_FALSE;
	}

	INIT_LIST_HEAD(&ctx->cinfo_tree.all_stream);
	INIT_LIST_HEAD(&ctx->cinfo_tree.sib_stream);
	INIT_LIST_HEAD(&ctx->cinfo_tree.pg_stream);

	ctx->codec = cfg->codec;

	ctx->flag_ts_forced_cappid = CCX_FALSE;
	for (i = 0; i < cfg->nb_ts_cappid; i++)
	{
		if (ctx->codec == CCX_CODEC_ANY)
			update_capinfo(ctx, cfg->ts_cappids[i], cfg->ts_datastreamtype, CCX_CODEC_NONE, 0, NULL);
		else
			update_capinfo(ctx, cfg->ts_cappids[i], cfg->ts_datastreamtype, ctx->codec, 0, NULL);
	}

	ctx->flag_ts_forced_cappid = cfg->nb_ts_cappid ? CCX_TRUE : CCX_FALSE;
	ctx->nocodec = cfg->nocodec;

	ctx->reset = ccx_demuxer_reset;
	ctx->close = ccx_demuxer_close;
	ctx->open = ccx_demuxer_open;
	ctx->is_open = ccx_demuxer_isopen;
	ctx->get_filesize = ccx_demuxer_get_file_size;
	ctx->get_stream_mode = ccx_demuxer_get_stream_mode;
	ctx->print_cfg = ccx_demuxer_print_cfg;
	ctx->hauppauge_warning_shown = 0;
	ctx->parent = parent;
	ctx->last_pat_payload = NULL;
	ctx->last_pat_length = 0;

	ctx->warning_program_not_found_shown = CCX_FALSE;
	ctx->strangeheader = 0;
	memset(&ctx->freport, 0, sizeof(ctx->freport));

	for (i = 0; i < MAX_PSI_PID; i++)
		ctx->PID_buffers[i] = NULL;

	init_ts(ctx);
	ctx->filebuffer = NULL;

	// Initialize stream discovery for multi-stream DVB subtitle extraction
	ctx->potential_stream_count = 0;

	return ctx;
}

void delete_demuxer_data(struct demuxer_data *data)
{
	free(data->buffer);
	free(data);
}

struct demuxer_data *alloc_demuxer_data(void)
{
	struct demuxer_data *data = malloc(sizeof(struct demuxer_data));

	if (!data)
	{
		return NULL;
	}
	data->buffer = (unsigned char *)malloc(BUFSIZE);
	if (!data->buffer)
	{
		free(data);
		return NULL;
	}
	data->len = 0;
	data->bufferdatatype = CCX_PES;

	data->program_number = -1;
	data->stream_pid = -1;
	data->codec = CCX_CODEC_NONE;
	data->len = 0;
	data->pts = CCX_NOPTS;
	data->rollover_bits = 0;
	data->tb.num = 1;
	data->tb.den = 90000;
	data->next_stream = 0;
	data->next_program = 0;
	return data;
}