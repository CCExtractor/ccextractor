#include "ccx_demuxer.h"
#include "activity.h"
#include "lib_ccx.h"
#include "utility.h"

static void ccx_demuxer_reset(struct ccx_demuxer *ctx)
{
	ctx->startbytes_pos=0;
	ctx->startbytes_avail=0;
	memset (ctx->PIDs_seen, 0, 65536*sizeof (int));
	memset (ctx->PIDs_programs, 0, 65536*sizeof (struct PMT_entry *));
}

static void ccx_demuxer_close(struct ccx_demuxer *ctx)
{ 
	ctx->past = 0;
	if (ctx->infd!=-1 && ccx_options.input_source==CCX_DS_FILE)
	{
		close (ctx->infd);
		ctx->infd=-1;
		activity_input_file_closed();
	}
}

static int ccx_demuxer_isopen(struct ccx_demuxer *ctx)
{
	return ctx->infd != -1;
}
static int ccx_demuxer_open(struct ccx_demuxer *ctx, const char *file)
{
	if (ccx_options.input_source==CCX_DS_STDIN)
	{
		if (ctx->infd != -1) // Means we had already processed stdin. So we're done.
		{
			if (ccx_options.print_file_reports)
				print_file_report(ctx->parent);
			return -1;
		}
		ctx->infd = 0;
		mprint ("\n\r-----------------------------------------------------------------\n");
		mprint ("\rReading from standard input\n");
		return 0;
	}
	if (ccx_options.input_source == CCX_DS_NETWORK)
	{
		if (ctx->infd != -1) // Means we have already bound a socket.
		{
			if (ccx_options.print_file_reports)
				print_file_report(ctx->parent);

			return -1;
		}

		ctx->infd = start_upd_srv(ccx_options.udpaddr, ccx_options.udpport);
		if(ctx->infd < 0)
		{
			print_error(ccx_options.gui_mode_reports,"socket() failed.");
			return CCX_COMMON_EXIT_BUG_BUG;
		}
		return 0;

	}

	if (ccx_options.input_source == CCX_DS_TCP)
	{
		if (ctx->infd != -1)
		{
			if (ccx_options.print_file_reports)
				print_file_report(ctx->parent);

			return -1;
		}

		ctx->infd = start_tcp_srv(ccx_options.tcpport, ccx_options.tcp_password);
		return 0;
	}
#ifdef _WIN32
	ctx->infd = OPEN (file, O_RDONLY | O_BINARY);
#else
	ctx->infd = OPEN (file, O_RDONLY);
#endif
	if (ctx->infd < 0)
		return -1;

	if (ctx->auto_stream == CCX_SM_AUTODETECT)
	{
		detect_stream_type(ctx);
		switch (ctx->stream_mode)
		{
			case CCX_SM_ELEMENTARY_OR_NOT_FOUND:
				mprint ("\rFile seems to be an elementary stream, enabling ES mode\n");
				break;
			case CCX_SM_TRANSPORT:
				mprint ("\rFile seems to be a transport stream, enabling TS mode\n");
				break;
			case CCX_SM_PROGRAM:
				mprint ("\rFile seems to be a program stream, enabling PS mode\n");
				break;
			case CCX_SM_ASF:
				mprint ("\rFile seems to be an ASF, enabling DVR-MS mode\n");
				break;
			case CCX_SM_WTV:
				mprint ("\rFile seems to be a WTV, enabling WTV mode\n");
				break;
			case CCX_SM_MCPOODLESRAW:
				mprint ("\rFile seems to be McPoodle raw data\n");
				break;
			case CCX_SM_RCWT:
				mprint ("\rFile seems to be a raw caption with time data\n");
				break;
			case CCX_SM_MP4:
				mprint ("\rFile seems to be a MP4\n");
				break;
#ifdef WTV_DEBUG
			case CCX_SM_HEX_DUMP:
				mprint ("\rFile seems to be an hexadecimal dump\n");					
				break;
#endif
			case CCX_SM_MYTH:
			case CCX_SM_AUTODETECT:
				fatal(CCX_COMMON_EXIT_BUG_BUG, "Cannot be reached!");
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
			ctx->stream_mode=CCX_SM_MYTH;
			break;
		case 2:
			// autodetect myth files, but only if it does not conflict with
			// the current stream mode
			switch (ctx->stream_mode)
			{
				case CCX_SM_ELEMENTARY_OR_NOT_FOUND:
				case CCX_SM_PROGRAM:
					if ( detect_myth(ctx->parent) )
					{
						ctx->stream_mode=CCX_SM_MYTH;
					}
					break;
				default:
					// Keep stream_mode
					break;
			}
			break;					
	}


	return 0;
}
LLONG ccx_demuxer_getfilesize (struct ccx_demuxer *ctx)
{
	int ret = 0;
	int in = ctx->infd;
	LLONG current=LSEEK (in, 0, SEEK_CUR);
	LLONG length = LSEEK (in,0,SEEK_END);
	if(current < 0 ||length < 0)
		return -1;

	ret = LSEEK (in,current,SEEK_SET);
	if (ret < 0)
		return -1;

	return length;
}

static int ccx_demuxer_get_stream_mode(struct ccx_demuxer *ctx)
{
	return ctx->stream_mode;
}

void ccx_demuxer_delete(struct ccx_demuxer **ctx)
{
	struct ccx_demuxer *lctx = *ctx;
	int i;
	for (i = 0; i < MAX_PID; i++)
	{
		if( lctx->PIDs_programs[i])
			freep(lctx->PIDs_programs + i);
	}
	dinit_ts(lctx);
	if (lctx->fh_out_elementarystream!=NULL)
		fclose (lctx->fh_out_elementarystream);
	freep(ctx);
}

static void ccx_demuxer_print_report(struct ccx_demuxer *ctx)
{
	printf("Stream Mode: ");
	switch (ctx->stream_mode)
	{
		case CCX_SM_TRANSPORT:
			printf("Transport Stream\n");

			printf("Program Count: %d\n", ctx->freport.program_cnt);

			printf("Program Numbers: ");
			for (int i = 0; i < pmt_array_length; i++)
			{
				if (pmt_array[i].program_number == 0)
					continue;

				printf("%u ", pmt_array[i].program_number);
			}
			printf("\n");

			for (int i = 0; i < 65536; i++)
			{
				if (ctx->PIDs_programs[i] == 0)
					continue;

				printf("PID: %u, Program: %u, ", i, ctx->PIDs_programs[i]->program_number);
				int j;
				for (j = 0; j < SUB_STREAMS_CNT; j++)
				{
					if (ctx->freport.dvb_sub_pid[j] == i)
					{
						printf("DVB Subtitles\n");
						break;
					}
					if (ctx->freport.tlt_sub_pid[j] == i)
					{
						printf("Teletext Subtitles\n");
						break;
					}
				}
				if (j == SUB_STREAMS_CNT)
					printf("%s\n", desc[ctx->PIDs_programs[i]->printable_stream_type]);
			}

			break;
		case CCX_SM_PROGRAM:
			printf("Program Stream\n");
			break;
		case CCX_SM_ASF:
			printf("ASF\n");
			break;
		case CCX_SM_WTV:
			printf("WTV\n");
			break;
		case CCX_SM_ELEMENTARY_OR_NOT_FOUND:
			printf("Not Found\n");
			break;
		case CCX_SM_MP4:
			printf("MP4\n");
			break;
		case CCX_SM_MCPOODLESRAW:
			printf("McPoodle's raw\n");
			break;
		case CCX_SM_RCWT:
			printf("BIN\n");
			break;
#ifdef WTV_DEBUG
		case CCX_SM_HEX_DUMP:
			printf("Hex\n");
			break;
#endif
		default:
			break;
	}
	if (ctx->freport.program_cnt > 1)
		printf("//////// Program #%u: ////////\n", TS_program_number);

	printf("DVB Subtitles: ");
	int j;
	for (j = 0; j < SUB_STREAMS_CNT; j++)
	{
		unsigned pid = ctx->freport.dvb_sub_pid[j];
		if (pid == 0)
			continue;
		if (ctx->PIDs_programs[pid]->program_number == TS_program_number)
		{
			printf("Yes\n");
			break;
		}
	}
	if (j == SUB_STREAMS_CNT)
		printf("No\n");

	printf("Teletext: ");
	for (j = 0; j < SUB_STREAMS_CNT; j++)
	{
		unsigned pid = ctx->freport.tlt_sub_pid[j];
		if (pid == 0)
			continue;
		if (ctx->PIDs_programs[pid]->program_number == TS_program_number)
		{
			printf("Yes\n");

			printf("Pages With Subtitles: ");
			for (int i = 0; i < MAX_TLT_PAGES; i++)
			{
				if (seen_sub_page[i] == 0)
					continue;

				printf("%d ", i);
			}
			printf("\n");
			break;
		}
	}
	if (j == SUB_STREAMS_CNT)
		printf("No\n");

}
static void ccx_demuxer_print_cfg(struct ccx_demuxer *ctx)
{
	switch (ctx->auto_stream)
	{
		case CCX_SM_ELEMENTARY_OR_NOT_FOUND:
			mprint ("Elementary");
			break;
		case CCX_SM_TRANSPORT:
			mprint ("Transport");
			break;
		case CCX_SM_PROGRAM:
			mprint ("Program");
			break;
		case CCX_SM_ASF:
			mprint ("DVR-MS");
			break;
		case CCX_SM_WTV:
			mprint ("Windows Television (WTV)");
			break;
		case CCX_SM_MCPOODLESRAW:
			mprint ("McPoodle's raw");
			break;
		case CCX_SM_AUTODETECT:
			mprint ("Autodetect");
			break;
		case CCX_SM_RCWT:
			mprint ("BIN");
			break;
		case CCX_SM_MP4:
			mprint ("MP4");
			break;
#ifdef WTV_DEBUG
		case CCX_SM_HEX_DUMP:
			mprint ("Hex");
			break;
#endif
		default:
			fatal(CCX_COMMON_EXIT_BUG_BUG, "BUG: Unknown stream mode.\n");
			break;
	}
}
int ccx_demuxer_write_es(struct ccx_demuxer *ctx, unsigned char* buf, size_t len)
{
		if (ctx->fh_out_elementarystream!=NULL)
			fwrite (buf, 1, len,ctx->fh_out_elementarystream);
}
struct ccx_demuxer *init_demuxer(void *parent, struct demuxer_cfg *cfg)
{
	struct ccx_demuxer *ctx = malloc(sizeof(struct ccx_demuxer));
	if(!ctx)
		return NULL;

	ctx->infd = -1;//Set to -1 to indicate no file is open.
	ctx->m2ts = cfg->m2ts;
	ctx->auto_stream = cfg->auto_stream;
	ctx->stream_mode = CCX_SM_ELEMENTARY_OR_NOT_FOUND;

	ctx->capbufsize = 20000;
	ctx->capbuf = NULL;
	ctx->capbuflen = 0; // Bytes read in capbuf

	ctx->reset = ccx_demuxer_reset;
	ctx->close = ccx_demuxer_close;
	ctx->open = ccx_demuxer_open;
	ctx->is_open = ccx_demuxer_isopen;
	ctx->get_filesize = ccx_demuxer_getfilesize;
	ctx->get_stream_mode = ccx_demuxer_get_stream_mode;
	ctx->print_cfg = ccx_demuxer_print_cfg;
	ctx->print_report = ccx_demuxer_print_report;
	ctx->write_es = ccx_demuxer_write_es;
	ctx->parent = parent;

	ctx->fh_out_elementarystream = NULL;
	if (cfg->out_elementarystream_filename != NULL)
	{
		if ((ctx->fh_out_elementarystream = fopen (cfg->out_elementarystream_filename,"wb"))==NULL)
		{
			print_error(CCX_COMMON_EXIT_FILE_CREATION_FAILED, "Unable to open clean file: %s\n", cfg->out_elementarystream_filename);
			return NULL;
		}
	}

	init_ts(ctx);

	return ctx;
}

struct demuxer_data* alloc_demuxer_data(void)
{
	struct demuxer_data* data = malloc(sizeof(struct demuxer_data));
	data->buffer = (unsigned char *) malloc (BUFSIZE);

	return data;
	
}
