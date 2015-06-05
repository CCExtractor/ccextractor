#include "lib_ccx.h"
#include "ccx_common_option.h"
long FILEBUFFERSIZE = 1024*1024*16; // 16 Mbytes no less. Minimize number of real read calls()
LLONG buffered_read_opt_file (unsigned char *buffer, unsigned int bytes);

#ifdef _WIN32
WSADATA wsaData = {0};
int iResult = 0;
#endif

LLONG getfilesize (int in)
{
	int ret = 0;
	LLONG current=LSEEK (in, 0, SEEK_CUR);
	LLONG length = LSEEK (in,0,SEEK_END);
	if(current < 0 ||length < 0)
		return -1;

	ret = LSEEK (in,current,SEEK_SET);
	if (ret < 0)
		return -1;

	return length;
}

LLONG gettotalfilessize (struct lib_ccx_ctx *ctx) // -1 if one or more files failed to open
{
	LLONG ts=0;
	int h;
	for (int i=0;i<ctx->num_input_files;i++)
	{
		if (0 == strcmp(ctx->inputfile[i],"-")) // Skip stdin
			continue;
#ifdef _WIN32
		h=OPEN (ctx->inputfile[i],O_RDONLY | O_BINARY);
#else
		h=OPEN (ctx->inputfile[i],O_RDONLY);
#endif
		if (h==-1)
		{
			mprint ("\rUnable to open %s\r\n",ctx->inputfile[i]);
			return -1;
		}
		if (!ccx_options.live_stream)
			ts+=getfilesize (h);
		close (h);
	}
	return ts;
}

void prepare_for_new_file (struct lib_ccx_ctx *ctx)
{
	struct lib_cc_decode *dec_ctx = NULL;
	dec_ctx = ctx->dec_ctx;
	// Init per file variables
	min_pts=0x01FFFFFFFFLL; // 33 bit
	sync_pts=0;
	pts_set = 0;
	// inputsize=0; Now responsibility of switch_to_next_file()
	ctx->last_reported_progress=-1;
	ctx->stat_numuserheaders = 0;
	ctx->stat_dvdccheaders = 0;
	ctx->stat_scte20ccheaders = 0;
	ctx->stat_replay5000headers = 0;
	ctx->stat_replay4000headers = 0;
	ctx->stat_dishheaders = 0;
	ctx->stat_hdtv = 0;
	ctx->stat_divicom = 0;
	total_frames_count = 0;
	ctx->total_pulldownfields = 0;
	ctx->total_pulldownframes = 0;
	dec_ctx->cc_stats[0]=0; dec_ctx->cc_stats[1]=0; dec_ctx->cc_stats[2]=0; dec_ctx->cc_stats[3]=0;
	ctx->false_pict_header=0;
	ctx->frames_since_last_gop=0;
	frames_since_ref_time=0;
	gop_time.inited=0;
	first_gop_time.inited=0;
	gop_rollover=0;
	printed_gop.inited=0;
	dec_ctx->saw_caption_block=0;
	ctx->past=0;
	pts_big_change=0;
	ctx->startbytes_pos=0;
	ctx->startbytes_avail=0;
	init_file_buffer();
	anchor_hdcc(-1);
	firstcall = 1;
	for(int x=0; x<0xfff; x++)
	{
		ctx->epg_buffers[x].buffer=NULL;
		ctx->epg_buffers[x].ccounter=0;
	}
	for (int i = 0; i < TS_PMT_MAP_SIZE; i++)
	{
		ctx->eit_programs[i].array_len=0;
		ctx->eit_current_events[i]=-1;
	}
	ctx->epg_last_output=-1;
	ctx->epg_last_live_output=-1;
}

/* Close input file if there is one and let the GUI know */
void close_input_file (struct lib_ccx_ctx *ctx)
{
	if (ctx->infd!=-1 && ccx_options.input_source==CCX_DS_FILE)
	{
		close (ctx->infd);
		ctx->infd=-1;
		activity_input_file_closed();
	}
}

/* Close current file and open next one in list -if any- */
/* bytesinbuffer is the number of bytes read (in some buffer) that haven't been added
to 'past' yet. We provide this number to switch_to_next_file() so a final sanity check
can be done */

int switch_to_next_file (struct lib_ccx_ctx *ctx, LLONG bytesinbuffer)
{
	struct lib_cc_decode *dec_ctx = NULL;
	dec_ctx = ctx->dec_ctx;
	if (ctx->current_file==-1 || !ccx_options.binary_concat)
	{
		memset (ctx->PIDs_seen,0,65536*sizeof (int));
		memset (ctx->PIDs_programs,0,65536*sizeof (struct PMT_entry *));
	}

	if (ccx_options.input_source==CCX_DS_STDIN)
	{
		if (ctx->infd!=-1) // Means we had already processed stdin. So we're done.
		{
			if (ccx_options.print_file_reports)
				print_file_report(ctx);
			return 0;
		}
		ctx->infd=0;
		mprint ("\n\r-----------------------------------------------------------------\n");
		mprint ("\rReading from standard input\n");
		return 1;
	}
	if (ccx_options.input_source==CCX_DS_NETWORK)
	{
		if (ctx->infd!=-1) // Means we have already bound a socket.
		{
			if (ccx_options.print_file_reports)
				print_file_report(ctx);

			return 0;
		}

		ctx->infd = start_upd_srv(ccx_options.udpaddr, ccx_options.udpport);
		if(ctx->infd < 0)
			fatal (CCX_COMMON_EXIT_BUG_BUG, "socket() failed.");
		return 1;

	}

	if (ccx_options.input_source==CCX_DS_TCP)
	{
		if (ctx->infd != -1)
		{
			if (ccx_options.print_file_reports)
				print_file_report(ctx);

			return 0;
		}

		ctx->infd = start_tcp_srv(ccx_options.tcpport, ccx_options.tcp_password);
		return 1;
	}

    /* Close current and make sure things are still sane */
    if (ctx->infd!=-1)
    {
		if (ccx_options.print_file_reports)
			print_file_report(ctx);
		close_input_file (ctx);
		if (ctx->inputsize>0 && ((ctx->past+bytesinbuffer) < ctx->inputsize) && !dec_ctx->processed_enough)
		{
			mprint("\n\n\n\nATTENTION!!!!!!\n");
			mprint("In switch_to_next_file(): Processing of %s %d ended prematurely %lld < %lld, please send bug report.\n\n",
					ctx->inputfile[ctx->current_file], ctx->current_file, ctx->past, ctx->inputsize);
		}
		if (ccx_options.binary_concat)
		{
			ctx->total_past+=ctx->inputsize;
			ctx->past=0; // Reset always or at the end we'll have double the size
		}
    }
    for (;;)
    {
	    ctx->current_file++;
	    if (ctx->current_file>=ctx->num_input_files)
		    break;

	    // The following \n keeps the progress percentage from being overwritten.
	    mprint ("\n\r-----------------------------------------------------------------\n");
	    mprint ("\rOpening file: %s\n", ctx->inputfile[ctx->current_file]);
#ifdef _WIN32
	    ctx->infd=OPEN (ctx->inputfile[ctx->current_file],O_RDONLY | O_BINARY);
#else
	    ctx->infd=OPEN (ctx->inputfile[ctx->current_file],O_RDONLY);
#endif
	    if (ctx->infd == -1)
		    mprint ("\rWarning: Unable to open input file [%s]\n", ctx->inputfile[ctx->current_file]);
	    else
	    {
		    activity_input_file_open (ctx->inputfile[ctx->current_file]);
		    if (!ccx_options.live_stream)
		    {
			    ctx->inputsize = getfilesize (ctx->infd);
			    if (!ccx_options.binary_concat)
				    ctx->total_inputsize=ctx->inputsize;
		    }
		    return 1; // Succeeded
	    }
    }
    return 0;
}

void position_sanity_check (void)
{
#ifdef SANITY_CHECK
	if (in!=-1)
	{
		LLONG realpos=LSEEK (in,0,SEEK_CUR);
		if (realpos!=ctx->past-filebuffer_pos+bytesinbuffer)
		{
			fatal (CCX_COMMON_EXIT_BUG_BUG, "Position desync, THIS IS A BUG. Real pos =%lld, past=%lld.\n",realpos,ctx->past);
		}
	}
#endif
}


int init_file_buffer(void)
{
	filebuffer_start=0;
	filebuffer_pos=0;
	if (filebuffer==NULL)
	{
		filebuffer=(unsigned char *) malloc (FILEBUFFERSIZE);
		bytesinbuffer=0;
	}
	if (filebuffer==NULL)
	{
		fatal (EXIT_NOT_ENOUGH_MEMORY, "Not enough memory\n");
	}
	return 0;
}

void buffered_seek (struct lib_ccx_ctx *ctx, int offset)
{
	position_sanity_check();
	if (offset<0)
	{
		filebuffer_pos+=offset;
		if (filebuffer_pos<0)
		{
			// We got into the start buffer (hopefully)
			if ((filebuffer_pos+ctx->startbytes_pos) < 0)
			{
				fatal (CCX_COMMON_EXIT_BUG_BUG, "PANIC: Attempt to seek before buffer start, this is a bug!");
			}
			ctx->startbytes_pos+=filebuffer_pos;
			filebuffer_pos=0;
		}
	}
	else
	{
		buffered_read_opt (ctx, NULL, offset);
		position_sanity_check();
	}
}

void sleepandchecktimeout (time_t start)
{
	if (ccx_options.input_source==CCX_DS_STDIN)
	{
		// CFS: Not 100% sure about this. Fine for files, not so sure what happens if stdin is
		// real time input from hardware.
		sleep_secs (1);
		ccx_options.live_stream=0;
		return;
	}

	if (ccx_options.live_stream==-1) // Just sleep, no timeout to check
	{
		sleep_secs (1);
		return;
	}
	if (time(NULL)>start+ccx_options.live_stream) // More than live_stream seconds elapsed. No more live
		ccx_options.live_stream=0;
	else
		sleep_secs(1);
}

void return_to_buffer (unsigned char *buffer, unsigned int bytes)
{
	if (bytes == filebuffer_pos)
	{
		// Usually we're just going back in the buffer and memcpy would be
		// unnecessary, but we do it in case we intentionally messed with the
		// buffer
		memcpy (filebuffer, buffer, bytes);
		filebuffer_pos=0;
		return;
	}
	if (filebuffer_pos>0) // Discard old bytes, because we may need the space
	{
		// Non optimal since data is moved later again but we don't care since
		// we're never here in ccextractor.
		memmove (filebuffer,filebuffer+filebuffer_pos,bytesinbuffer-filebuffer_pos);
		bytesinbuffer-=filebuffer_pos;
		bytesinbuffer=0;
		filebuffer_pos=0;
	}

	if (bytesinbuffer + bytes > FILEBUFFERSIZE)
		fatal (CCX_COMMON_EXIT_BUG_BUG, "Invalid return_to_buffer() - please submit a bug report.");
	memmove (filebuffer+bytes,filebuffer,bytesinbuffer);
	memcpy (filebuffer,buffer,bytes);
	bytesinbuffer+=bytes;
}

LLONG buffered_read_opt (struct lib_ccx_ctx *ctx, unsigned char *buffer, unsigned int bytes)
{
	LLONG copied=0;
	position_sanity_check();
	time_t seconds=0;
	if (ccx_options.live_stream>0)
		time (&seconds);
	if (ccx_options.buffer_input || filebuffer_pos<bytesinbuffer)
	{
		// Needs to return data from filebuffer_start+pos to filebuffer_start+pos+bytes-1;
		int eof = (ctx->infd==-1);

		while ((!eof || ccx_options.live_stream) && bytes)
		{
			if (eof)
			{
				// No more data available inmediately, we sleep a while to give time
				// for the data to come up
				sleepandchecktimeout (seconds);
			}
			size_t ready = bytesinbuffer-filebuffer_pos;
			if (ready==0) // We really need to read more
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
						if (buffer!=NULL) // Read
						{
							i=read (ctx->infd,buffer,bytes);
							if( i == -1)
								fatal (EXIT_READ_ERROR, "Error reading input file!\n");
							buffer+=i;
						}
						else // Seek
						{
							LLONG op, np;
							op =LSEEK (ctx->infd,0,SEEK_CUR); // Get current pos
							if (op+bytes<0) // Would mean moving beyond start of file: Not supported
								return 0;
							np =LSEEK (ctx->infd,bytes,SEEK_CUR); // Pos after moving
							i=(int) (np-op);
						}
						if (i==0 && ccx_options.live_stream)
						{
							if (ccx_options.input_source==CCX_DS_STDIN)
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
							copied+=i;
							bytes-=i;
						}

					}
					while ((i || ccx_options.live_stream ||
								(ccx_options.binary_concat && switch_to_next_file(ctx, copied))) && bytes);
					return copied;
				}
				// Keep the last 8 bytes, so we have a guaranteed
				// working seek (-8) - needed by mythtv.
				int keep = bytesinbuffer > 8 ? 8 : bytesinbuffer;
				memmove (filebuffer,filebuffer+(FILEBUFFERSIZE-keep),keep);
				int i;
				if (ccx_options.input_source==CCX_DS_FILE || ccx_options.input_source==CCX_DS_STDIN)
					i = read (ctx->infd, filebuffer+keep,FILEBUFFERSIZE-keep);
				else
					i = recvfrom(ctx->infd,(char *) filebuffer + keep, FILEBUFFERSIZE - keep, 0, NULL, NULL);
				if (i == -1)
					fatal (EXIT_READ_ERROR, "Error reading input stream!\n");
				if (i == 0)
				{
					/* If live stream, don't try to switch - acknowledge eof here as it won't
					   cause a loop end */
					if (ccx_options.live_stream || !(ccx_options.binary_concat && switch_to_next_file(ctx, copied)))
						eof = 1;
				}
				filebuffer_pos = keep;
				bytesinbuffer=(int) i + keep;
				ready = i;
			}
			int copy = (int) (ready>=bytes ? bytes:ready);
			if (copy)
			{
				if (buffer != NULL)
				{
					memcpy (buffer, filebuffer+filebuffer_pos, copy);
					buffer+=copy;
				}
				filebuffer_pos+=copy;
				bytes-=copy;
				copied+=copy;
			}
		}
		return copied;
	}
	else // Read without buffering
	{

		if (buffer!=NULL)
		{
			int i;
			while (bytes>0 && ctx->infd!=-1 &&
					((i=read(ctx->infd,buffer,bytes))!=0 || ccx_options.live_stream ||
					 (ccx_options.binary_concat && switch_to_next_file(ctx, copied))))
			{
				if( i == -1)
					fatal (EXIT_READ_ERROR, "Error reading input file!\n");
				else if (i==0)
					sleepandchecktimeout (seconds);
				else
				{
					copied+=i;
					bytes-=i;
					buffer+=i;
				}
			}
			return copied;
		}
		// return fread(buffer,1,bytes,in);
		//return FSEEK (in,bytes,SEEK_CUR);
		while (bytes!=0 && ctx->infd!=-1)
		{
			LLONG op, np;
			op =LSEEK (ctx->infd,0,SEEK_CUR); // Get current pos
			if (op+bytes<0) // Would mean moving beyond start of file: Not supported
				return 0;
			np =LSEEK (ctx->infd,bytes,SEEK_CUR); // Pos after moving
			copied=copied+(np-op);
			bytes=bytes-(unsigned int) copied;
			if (copied==0)
			{
				if (ccx_options.live_stream)
					sleepandchecktimeout (seconds);
				else
				{
					if (ccx_options.binary_concat)
						switch_to_next_file(ctx, 0);
					else
						break;
				}
			}
		}
		return copied;
	}
}
