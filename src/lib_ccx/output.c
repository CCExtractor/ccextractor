#include "lib_ccx.h"
#include "ccx_common_option.h"
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif


void init_write (struct ccx_s_write *wb,char *filename)
{
	memset(wb, 0, sizeof(struct ccx_s_write));
    wb->fh=-1;
    wb->filename=filename;
}

void writeraw (const unsigned char *data, int length, struct ccx_s_write *wb)
{
    write (wb->fh,data,length);
}

void writedata(const unsigned char *data, int length, ccx_decoder_608_context *context, struct cc_subtitle *sub)
{
    // Don't do anything for empty data
    if (data==NULL)
        return;

    if (ccx_options.write_format==CCX_OF_RAW || ccx_options.write_format==CCX_OF_DVDRAW)
	{
		if (context && context->out)
			writeraw (data,length,context->out);
	}
    else if (ccx_options.write_format==CCX_OF_SMPTETT ||
             ccx_options.write_format==CCX_OF_SAMI ||
             ccx_options.write_format==CCX_OF_SRT ||
             ccx_options.write_format==CCX_OF_TRANSCRIPT ||
             ccx_options.write_format==CCX_OF_SPUPNG ||
			 ccx_options.write_format==CCX_OF_NULL)
        process608 (data,length,context, sub);
    else
		fatal(CCX_COMMON_EXIT_BUG_BUG, "Should not be reached!");
}

void flushbuffer (struct lib_ccx_ctx *ctx, struct ccx_s_write *wb, int closefile)
{
    if (closefile && wb!=NULL && wb->fh!=-1 && !ctx->cc_to_stdout)
        close (wb->fh);
}

void writeDVDraw (const unsigned char *data1, int length1,
                  const unsigned char *data2, int length2,
				  struct ccx_s_write *wb)
{
    /* these are only used by DVD raw mode: */
    static int loopcount = 1; /* loop 1: 5 elements, loop 2: 8 elements,
                                 loop 3: 11 elements, rest: 15 elements */
    static int datacount = 0; /* counts within loop */

    if (datacount==0)
    {
        write (wb->fh,DVD_HEADER,sizeof (DVD_HEADER));
        if (loopcount==1)
            write (wb->fh,lc1,sizeof (lc1));
        if (loopcount==2)
            write (wb->fh,lc2,sizeof (lc2));
        if (loopcount==3)
        {
            write (wb->fh,lc3,sizeof (lc3));
            if (data2 && length2)
				write (wb->fh,data2,length2);
        }
        if (loopcount>3)
        {
            write (wb->fh,lc4,sizeof (lc4));
			if (data2 && length2)
				write (wb->fh,data2,length2);
        }
    }
    datacount++;
    write (wb->fh,lc5,sizeof (lc5));
	if (data1 && length1)
		write (wb->fh,data1,length1);
    if (((loopcount == 1) && (datacount < 5)) || ((loopcount == 2) &&
        (datacount < 8)) || (( loopcount == 3) && (datacount < 11)) ||
        ((loopcount > 3) && (datacount < 15)))
    {
        write (wb->fh,lc6,sizeof (lc6));
		if (data2 && length2)
			write (wb->fh,data2,length2);
    }
    else
    {
        if (loopcount==1)
        {
            write (wb->fh,lc6,sizeof (lc6));
			if (data2 && length2)
				write (wb->fh,data2,length2);
        }
        loopcount++;
        datacount=0;
    }

}

void printdata (struct lib_ccx_ctx *ctx, const unsigned char *data1, int length1,
                const unsigned char *data2, int length2, struct cc_subtitle *sub)
{
	if (ccx_options.write_format==CCX_OF_DVDRAW)
		writeDVDraw (data1,length1,data2,length2,&ctx->wbout1);
	else /* Broadcast raw or any non-raw */
	{
		if (length1 && ccx_options.extract != 2)
		{
			writedata(data1, length1, &ctx->context_cc608_field_1, sub);
		}
		if (length2)
		{
			if (ccx_options.extract != 1)
				writedata(data2, length2, &ctx->context_cc608_field_2, sub);
			else // User doesn't want field 2 data, but we want XDS.
				writedata (data2,length2,NULL, sub);
		}
	}
}

/* Buffer data with the same FTS and write when a new FTS or data==NULL
 * is encountered */
void writercwtdata (struct lib_ccx_ctx *ctx, const unsigned char *data)
{
    static LLONG prevfts = -1;
    LLONG currfts = fts_now + fts_global;
    static uint16_t cbcount = 0;
    static int cbempty=0;
    static unsigned char cbbuffer[0xFFFF*3]; // TODO: use malloc
    static unsigned char cbheader[8+2];

    if ( (prevfts != currfts && prevfts != -1)
         || data == NULL
         || cbcount == 0xFFFF)
    {
        // Remove trailing empty or 608 padding caption blocks
        if ( cbcount != 0xFFFF)
        {
            unsigned char cc_valid;
            unsigned char cc_type;
            int storecbcount=cbcount;

            for( int cb = cbcount-1; cb >= 0 ; cb-- )
            {
                cc_valid = (*(cbbuffer+3*cb) & 4) >>2;
                cc_type = *(cbbuffer+3*cb) & 3;

                // The -fullbin option disables pruning of 608 padding blocks
                if ( (cc_valid && cc_type <= 1 // Only skip NTSC padding packets
                      && !ccx_options.fullbin // Unless we want to keep them
                      && *(cbbuffer+3*cb+1)==0x80
                      && *(cbbuffer+3*cb+2)==0x80)
                     || !(cc_valid || cc_type==3) ) // or unused packets
                {
                    cbcount--;
                }
                else
                {
                    cb = -1;
                }
            }
            dbg_print(CCX_DMT_CBRAW, "%s Write %d RCWT blocks - skipped %d padding / %d unused blocks.\n",
                       print_mstime(prevfts), cbcount, storecbcount - cbcount, cbempty);
        }

        // New FTS, write data header
        // RCWT data header (10 bytes):
        //byte(s)   value   description
        //0-7       FTS     int64_t number with current FTS
        //8-9       blocks  Number of 3 byte data blocks with the same FTS that are
        //                  following this header
        memcpy(cbheader,&prevfts,8);
        memcpy(cbheader+8,&cbcount,2);

        if (cbcount > 0)
        {
			if (ccx_options.send_to_srv)
			{
				net_send_cc(cbheader, 10);
				net_send_cc(cbbuffer, 3*cbcount);
			}
			else
			{
				writeraw(cbheader,10,&ctx->wbout1);
				writeraw(cbbuffer,3*cbcount, &ctx->wbout1);
			}
        }
        cbcount = 0;
        cbempty = 0;
    }

    if ( data )
    {
        // Store the data while the FTS is unchanged

        unsigned char cc_valid = (*data & 4) >> 2;
        unsigned char cc_type = *data & 3;
        // Store only non-empty packets
        if (cc_valid || cc_type==3)
        {
            // Store in buffer until we know how many blocks come with
            // this FTS.
            memcpy(cbbuffer+cbcount*3, data, 3);
            cbcount++;
        }
        else
        {
            cbempty++;
        }
    }
    else
    {
        // Write a padding block for field 1 and 2 data if this is the final
        // call to this function.  This forces the RCWT file to have the
        // same length as the source video file.

        // currfts currently holds the end time, subtract one block length
        // so that the FTS corresponds to the time before the last block is
        // written
        currfts -= 1001/30;

        memcpy(cbheader,&currfts,8);
        cbcount = 2;
        memcpy(cbheader+8,&cbcount,2);

        memcpy(cbbuffer, "\x04\x80\x80", 3); // Field 1 padding
        memcpy(cbbuffer+3, "\x05\x80\x80", 3); // Field 2 padding

		if (ccx_options.send_to_srv)
		{
			net_send_cc(cbheader, 10);
			net_send_cc(cbbuffer, 3*cbcount);
		}
		else
		{
			writeraw(cbheader,10,&ctx->wbout1);
			writeraw(cbbuffer,3*cbcount, &ctx->wbout1);
		}

        cbcount = 0;
        cbempty = 0;

        dbg_print(CCX_DMT_CBRAW, "%s Write final padding RCWT blocks.\n",
                   print_mstime(currfts));
    }

    prevfts = currfts;
}
