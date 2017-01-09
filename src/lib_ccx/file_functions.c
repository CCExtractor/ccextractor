#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "activity.h"
#include "file_buffer.h"
long FILEBUFFERSIZE = 1024*1024*16; // 16 Mbytes no less. Minimize number of real read calls()

#ifdef _WIN32
WSADATA wsaData = {0};
int iResult = 0;
#endif

LLONG get_file_size (int in)
{
	int ret = 0;
	LLONG current = LSEEK (in, 0, SEEK_CUR);
	LLONG length  = LSEEK (in, 0, SEEK_END);
	if(current < 0 ||length < 0)
		return -1;

	ret = LSEEK (in, current, SEEK_SET);
	if (ret < 0)
		return -1;

	return length;
}

LLONG get_total_file_size (struct lib_ccx_ctx *ctx) // -1 if one or more files failed to open
{
	LLONG ts=0;
	int h;
	for (int i = 0; i < ctx->num_input_files; i++)
	{
		if (0 == strcmp(ctx->inputfile[i], "-")) // Skip stdin
			continue;
#ifdef _WIN32
		h = OPEN (ctx->inputfile[i], O_RDONLY | O_BINARY);
#else
		h = OPEN (ctx->inputfile[i], O_RDONLY);
#endif

		if (h == -1) {
			switch (errno)
			{
			case ENOENT:
				return -1 * ENOENT;
			case EACCES:
				return -1 * EACCES;
			case EINVAL:
				return -1 * EINVAL;
			case EMFILE:
				return -1 * EMFILE;
			default:
				return -1;
			}
		}

		if (!ccx_options.live_stream)
			ts += get_file_size (h);
		close (h);
	}
	return ts;
}

void prepare_for_new_file (struct lib_ccx_ctx *ctx)
{
	// Init per file variables
	ctx->last_reported_progress =-1;
	ctx->stat_numuserheaders    = 0;
	ctx->stat_dvdccheaders      = 0;
	ctx->stat_scte20ccheaders   = 0;
	ctx->stat_replay5000headers = 0;
	ctx->stat_replay4000headers = 0;
	ctx->stat_dishheaders       = 0;
	ctx->stat_hdtv              = 0;
	ctx->stat_divicom           = 0;
	total_frames_count          = 0;
	ctx->false_pict_header      = 0;
	frames_since_ref_time       = 0;
	gop_time.inited             = 0;
	first_gop_time.inited       = 0;
	gop_rollover                = 0;
	printed_gop.inited          = 0;
	pts_big_change              = 0;
	firstcall                   = 1;

	if(ctx->epg_inited)
	{
		for(int x = 0; x < 0xfff; x++)
		{
			ctx->epg_buffers[x].buffer   = NULL;
			ctx->epg_buffers[x].ccounter = 0;
		}
		for (int i = 0; i < TS_PMT_MAP_SIZE; i++)
		{
			ctx->eit_programs[i].array_len = 0;
			ctx->eit_current_events[i] = -1;
		}
		ctx->epg_last_output      = -1;
		ctx->epg_last_live_output = -1;
	}
}

/* Close input file if there is one and let the GUI know */
void close_input_file (struct lib_ccx_ctx *ctx)
{
	ctx->demux_ctx->close(ctx->demux_ctx);
}

/* Close current file and open next one in list -if any- */
/* bytesinbuffer is the number of bytes read (in some buffer) that haven't been added
to 'past' yet. We provide this number to switch_to_next_file() so a final sanity check
can be done */

int switch_to_next_file (struct lib_ccx_ctx *ctx, LLONG bytesinbuffer)
{
	int ret = 0;
	if (ctx->current_file == -1 || !ccx_options.binary_concat)
	{
		ctx->demux_ctx->reset(ctx->demux_ctx);
	}

	switch(ccx_options.input_source)
	{
		case CCX_DS_STDIN:
		case CCX_DS_NETWORK:
		case CCX_DS_TCP:
			ret = ctx->demux_ctx->open(ctx->demux_ctx, NULL);
			if (ret < 0)
				return 0;
			else if (ret)
				return ret;
			else
				return 1;
			break;
		default:
			break;
	}
	/* Close current and make sure things are still sane */
	if (ctx->demux_ctx->is_open(ctx->demux_ctx))
	{
		dbg_print(CCX_DMT_708, "[CEA-708] The 708 decoder was reset [%d] times.\n", ctx->freport.data_from_708->reset_count);
		if (ccx_options.print_file_reports)
			print_file_report(ctx);

		if (ctx->inputsize > 0 && ((ctx->demux_ctx->past+bytesinbuffer) < ctx->inputsize) && is_decoder_processed_enough(ctx) == CCX_FALSE)
		{
			mprint("\n\n\n\nATTENTION!!!!!!\n");
			mprint("In switch_to_next_file(): Processing of %s %d ended prematurely %lld < %lld, please send bug report.\n\n",
					ctx->inputfile[ctx->current_file], ctx->current_file, ctx->demux_ctx->past, ctx->inputsize);
		}
		close_input_file (ctx);

		if (ccx_options.binary_concat)
		{
			ctx->total_past     += ctx->inputsize;
			ctx->demux_ctx->past = 0; // Reset always or at the end we'll have double the size
		}
	}
	for (;;)
	{
		ctx->current_file++;
		if (ctx->current_file >= ctx->num_input_files)
			break;

		// The following \n keeps the progress percentage from being overwritten.
		mprint ("\n\r-----------------------------------------------------------------\n");
		mprint ("\rOpening file: %s\n", ctx->inputfile[ctx->current_file]);
		ret = ctx->demux_ctx->open(ctx->demux_ctx, ctx->inputfile[ctx->current_file]);
		if (ret < 0)
			mprint ("\rWarning: Unable to open input file [%s]\n", ctx->inputfile[ctx->current_file]);
		else
		{
			activity_input_file_open (ctx->inputfile[ctx->current_file]);
			if (!ccx_options.live_stream)
			{
				ctx->inputsize = ctx->demux_ctx->get_filesize (ctx->demux_ctx);
				if (!ccx_options.binary_concat)
					ctx->total_inputsize = ctx->inputsize;
			}
			return 1; // Succeeded
		}
	}
	return 0;
}

void position_sanity_check(struct ccx_demuxer *ctx)
{
#ifdef SANITY_CHECK
	if (ctx->infd!=-1)
	{
		LLONG realpos = LSEEK (ctx->infd,0,SEEK_CUR);
		if (realpos == -1) // Happens for example when infd==stdin.
			return; 
		if (realpos != ctx->past - ctx->filebuffer_pos + ctx->bytesinbuffer)
		{
			fatal (CCX_COMMON_EXIT_BUG_BUG, "Position desync, THIS IS A BUG. Real pos =%lld, past=%lld.\n", realpos, ctx->past);
		}
	}
#endif
}


int init_file_buffer(struct ccx_demuxer *ctx)
{
	ctx->filebuffer_start = 0;
	ctx->filebuffer_pos = 0;
	if (ctx->filebuffer == NULL)
	{
		ctx->filebuffer = (unsigned char *) malloc (FILEBUFFERSIZE);
		ctx->bytesinbuffer = 0;
	}
	if (ctx->filebuffer == NULL)
	{
		return -1;
	}
	return 0;
}

void buffered_seek (struct ccx_demuxer *ctx, int offset)
{
	position_sanity_check(ctx);
	if (offset < 0)
	{
		ctx->filebuffer_pos += offset;
		if (ctx->filebuffer_pos < 0)
		{
			// We got into the start buffer (hopefully)
			if ((ctx->filebuffer_pos + ctx->startbytes_pos) < 0)
			{
				fatal (CCX_COMMON_EXIT_BUG_BUG, "PANIC: Attempt to seek before buffer start, this is a bug!");
			}
			ctx->startbytes_pos += ctx->filebuffer_pos;
			ctx->filebuffer_pos = 0;
		}
	}
	else
	{
		buffered_read_opt (ctx, NULL, offset);
		position_sanity_check(ctx);
	}
}

void sleepandchecktimeout (time_t start)
{
	if (ccx_options.input_source == CCX_DS_STDIN)
	{
		// CFS: Not 100% sure about this. Fine for files, not so sure what happens if stdin is
		// real time input from hardware.
		sleep_secs (1);
		ccx_options.live_stream = 0;
		return;
	}

	if (ccx_options.live_stream == -1) // Just sleep, no timeout to check
	{
		sleep_secs (1);
		return;
	}
	if (time(NULL) > start + ccx_options.live_stream) // More than live_stream seconds elapsed. No more live
		ccx_options.live_stream = 0;
	else
		sleep_secs(1);
}

void return_to_buffer (struct ccx_demuxer *ctx, unsigned char *buffer, unsigned int bytes)
{
	if (bytes == ctx->filebuffer_pos)
	{
		// Usually we're just going back in the buffer and memcpy would be
		// unnecessary, but we do it in case we intentionally messed with the
		// buffer
		memcpy (ctx->filebuffer, buffer, bytes);
		ctx->filebuffer_pos = 0;
		return;
	}
	if (ctx->filebuffer_pos > 0) // Discard old bytes, because we may need the space
	{
		// Non optimal since data is moved later again but we don't care since
		// we're never here in ccextractor.
		memmove (ctx->filebuffer, ctx->filebuffer + ctx->filebuffer_pos, ctx->bytesinbuffer-ctx->filebuffer_pos);
		ctx->bytesinbuffer  = 0;
		ctx->filebuffer_pos = 0;
	}

	if (ctx->bytesinbuffer + bytes > FILEBUFFERSIZE)
		fatal (CCX_COMMON_EXIT_BUG_BUG, "Invalid return_to_buffer() - please submit a bug report.");

	memmove (ctx->filebuffer + bytes, ctx->filebuffer, ctx->bytesinbuffer);
	memcpy (ctx->filebuffer, buffer, bytes);
	ctx->bytesinbuffer += bytes;
}

/**
 * @param buffer can be NULL, in case when user want to just buffer it or skip some data.
 *
 * Global options that have efffect on this function are following
 * 1) ccx_options.live_stream
 * 2) ccx_options.buffer_input
 * 3) ccx_options.input_source
 * 4) ccx_options.binary_concat
 *
 * TODO instead of using global ccx_options move them to ccx_demuxer
 */
size_t buffered_read_opt (struct ccx_demuxer *ctx, unsigned char *buffer, size_t bytes)
{
	size_t origin_buffer_size = bytes;
	size_t copied   = 0;
	time_t seconds = 0;

	position_sanity_check(ctx);

	if (ccx_options.live_stream > 0)
		time (&seconds);

	if (ccx_options.buffer_input || ctx->filebuffer_pos < ctx->bytesinbuffer)
	{
		// Needs to return data from filebuffer_start+pos to filebuffer_start+pos+bytes-1;
		int eof = (ctx->infd == -1);

		while ((!eof || ccx_options.live_stream) && bytes)
		{
			if (terminate_asap)
				break;
			if (eof)
			{
				// No more data available inmediately, we sleep a while to give time
				// for the data to come up
				sleepandchecktimeout (seconds);
			}
			size_t ready = ctx->bytesinbuffer - ctx->filebuffer_pos;
			if (ready == 0) // We really need to read more
			{
				if (!ccx_options.buffer_input)
				{
					// We got in the buffering code because of the initial buffer for
					// detection stuff. However we don't want more buffering so
					// we do the rest directly on the final buffer.
					int i;
					do
					{
						// No code for network support here, because network is always
						// buffered - if here, then it must be files.
						if (buffer != NULL) // Read
						{
							i = read (ctx->infd, buffer, bytes);
							if( i == -1)
								fatal (EXIT_READ_ERROR, "Error reading input file!\n");
							buffer += i;
						}
						else // Seek
						{
							LLONG op, np;
							op = LSEEK (ctx->infd, 0, SEEK_CUR); // Get current pos
							if (op + bytes < 0) // Would mean moving beyond start of file: Not supported
								return 0;
							np = LSEEK (ctx->infd, bytes, SEEK_CUR); // Pos after moving
							i = (int) (np - op);
						}
						// if both above lseek returned -1 (error); i would be 0 here and
						// in case when its not live stream copied would decrease and bytes would...
						if (i == 0 && ccx_options.live_stream)
						{
							if (ccx_options.input_source == CCX_DS_STDIN)
							{
								ccx_options.live_stream = 0;
								break;
							}
							else
							{
								sleepandchecktimeout (seconds);
							}
						}
						else
						{
							copied += i;
							bytes  -= i;
						}

					}
					while ((i || ccx_options.live_stream ||
								(ccx_options.binary_concat && switch_to_next_file(ctx->parent, copied))) && bytes);
					return copied;
				}
				// Keep the last 8 bytes, so we have a guaranteed
				// working seek (-8) - needed by mythtv.
				int keep = ctx->bytesinbuffer > 8 ? 8 : ctx->bytesinbuffer;
				memmove (ctx->filebuffer, ctx->filebuffer+(FILEBUFFERSIZE-keep),keep);
				int i;
				if (ccx_options.input_source == CCX_DS_FILE || ccx_options.input_source == CCX_DS_STDIN)
					i = read (ctx->infd, ctx->filebuffer + keep, FILEBUFFERSIZE-keep);
				else if (ccx_options.input_source == CCX_DS_TCP)
					i = net_tcp_read(ctx->infd, (char *) ctx->filebuffer + keep, FILEBUFFERSIZE - keep);
				else
					i = recvfrom(ctx->infd,(char *) ctx->filebuffer + keep, FILEBUFFERSIZE - keep, 0, NULL, NULL);
				if (terminate_asap) /* Looks like receiving a signal here will trigger a -1, so check that first */
					break;
				if (i == -1)
					fatal (EXIT_READ_ERROR, "Error reading input stream!\n");
				if (i == 0)
				{
					/* If live stream, don't try to switch - acknowledge eof here as it won't
					   cause a loop end */
					if (ccx_options.live_stream || ((struct lib_ccx_ctx *)ctx->parent)->inputsize <= origin_buffer_size || !(ccx_options.binary_concat && switch_to_next_file(ctx->parent, copied)))
						eof = 1;
				}
				ctx->filebuffer_pos = keep;
				ctx->bytesinbuffer = (int) i + keep;
				ready = i;
			}
			int copy = (int) (ready>=bytes ? bytes:ready);
			if (copy)
			{
				if (buffer != NULL)
				{
					memcpy (buffer, ctx->filebuffer + ctx->filebuffer_pos, copy);
					buffer += copy;
				}
				ctx->filebuffer_pos += copy;
				bytes  -= copy;
				copied += copy;
			}
		}
	}
	else // Read without buffering
	{

		if (buffer != NULL)
		{
			int i;
			while (bytes > 0 && ctx->infd != -1 &&
					((i = read(ctx->infd, buffer, bytes)) != 0 || ccx_options.live_stream ||
					 (ccx_options.binary_concat && switch_to_next_file(ctx->parent, copied))))
			{
				if (terminate_asap)
					break;
				if( i == -1)
					fatal (EXIT_READ_ERROR, "Error reading input file!\n");
				else if (i == 0)
					sleepandchecktimeout (seconds);
				else
				{
					copied += i;
					bytes  -= i;
					buffer += i;
				}
			}
			return copied;
		}
		while (bytes != 0 && ctx->infd != -1)
		{
			LLONG op, np;
			if (terminate_asap)
				break;
			op = LSEEK (ctx->infd, 0, SEEK_CUR); // Get current pos
			if (op + bytes < 0) // Would mean moving beyond start of file: Not supported
				return 0;

			np     = LSEEK (ctx->infd, bytes, SEEK_CUR); // Pos after moving
			if (op == -1 && np == -1) // Possibly a pipe that doesn't like "skipping"
			{
				char c;
				for (size_t i = 0; i<bytes; i++) read(ctx->infd, &c, 1);
				copied = bytes;
			}
			else
				copied = copied + (np - op);
			bytes  = bytes- (unsigned int) copied;
			if (copied == 0)
			{
				if (ccx_options.live_stream)
					sleepandchecktimeout (seconds);
				else
				{
					if (ccx_options.binary_concat)
						switch_to_next_file(ctx->parent, 0);
					else
						break;
				}
			}
		}
	}
	return copied;
}

unsigned short buffered_get_be16(struct ccx_demuxer *ctx)
{
	unsigned char a,b;
	unsigned char *a_p = &a; // Just to suppress warnings
	unsigned char *b_p = &b;
	buffered_read_byte(ctx, a_p);
	ctx->past++;
	buffered_read_byte(ctx, b_p);
	ctx->past++;
	return ( (unsigned short) (a<<8) )| ( (unsigned short) b);
}

unsigned char buffered_get_byte (struct ccx_demuxer *ctx)
{
	unsigned char b;
	unsigned char *b_p = &b;
	size_t result;

	result = buffered_read_byte(ctx, b_p);
	if (result == 1)
	{
		ctx->past++;
		return b;
	}
	else
		return 0;
}

unsigned int buffered_get_be32(struct ccx_demuxer *ctx)
{
	unsigned int val;
	val = buffered_get_be16(ctx) << 16;
	val |= buffered_get_be16(ctx);
	return val;
}

unsigned short buffered_get_le16(struct ccx_demuxer *ctx)
{
	unsigned char a,b;
	unsigned char *a_p = &a; // Just to suppress warnings
	unsigned char *b_p = &b;
	buffered_read_byte(ctx, a_p);
	ctx->past++;
	buffered_read_byte(ctx, b_p);
	ctx->past++;
	return ( (unsigned short) (b<<8) )| ( (unsigned short) a);
}

unsigned int buffered_get_le32(struct ccx_demuxer *ctx)
{
	unsigned int val;
	val = buffered_get_le16(ctx);
	val |= buffered_get_le16(ctx) << 16;
	return val;
}
