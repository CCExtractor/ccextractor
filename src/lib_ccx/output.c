#include "lib_ccx.h"
#include "ccx_common_option.h"
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

int init_write (struct ccx_s_write *wb,char *filename)
{
	memset(wb, 0, sizeof(struct ccx_s_write));
	wb->fh=-1;
	wb->filename = filename;
	mprint ("Creating %s\n", filename);
	wb->fh = open (filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
	if (wb->fh == -1)
	{
		return CCX_COMMON_EXIT_FILE_CREATION_FAILED;
	}
	return EXIT_OK;
}

int writeraw (const unsigned char *data, int length, void *private_data, struct cc_subtitle *sub)
{
	unsigned char* sub_data = NULL;
	// Don't do anything for empty data
	if (data==NULL)
		return -1;

	sub->data = realloc(sub->data, length + sub->nb_data);
	if (!sub->data)
		return EXIT_NOT_ENOUGH_MEMORY;

	sub_data = sub->data;
	memcpy(sub_data + sub->nb_data, data, length);
	sub->got_output = 1;
	sub->nb_data += length;
	sub->type = CC_RAW;


	return EXIT_SUCCESS;
}

void flushbuffer (struct lib_ccx_ctx *ctx, struct ccx_s_write *wb, int closefile)
{
	if (closefile && wb!=NULL && wb->fh!=-1 && !ctx->cc_to_stdout)
		close (wb->fh);
}

void writeDVDraw (const unsigned char *data1, int length1,
                  const unsigned char *data2, int length2,
				  struct cc_subtitle *sub)
{
	/* these are only used by DVD raw mode: */
	static int loopcount = 1; /* loop 1: 5 elements, loop 2: 8 elements,
				     loop 3: 11 elements, rest: 15 elements */
	static int datacount = 0; /* counts within loop */

	if (datacount==0)
	{
		writeraw (DVD_HEADER, sizeof (DVD_HEADER), NULL, sub);
		if (loopcount==1)
			writeraw (lc1, sizeof (lc1), NULL, sub);
		if (loopcount==2)
			writeraw (lc2, sizeof (lc2), NULL, sub);
		if (loopcount==3)
		{
			writeraw (lc3, sizeof (lc3), NULL, sub);
			if (data2 && length2)
				writeraw (data2, length2, NULL, sub);
		}
		if (loopcount>3)
		{
			writeraw (lc4, sizeof (lc4), NULL, sub);
			if (data2 && length2)
				writeraw (data2, length2, NULL, sub);
		}
	}
	datacount++;
	writeraw (lc5, sizeof (lc5), NULL, sub);
	if (data1 && length1)
		writeraw (data1, length1, NULL, sub);
	if (((loopcount == 1) && (datacount < 5)) || ((loopcount == 2) &&
				(datacount < 8)) || (( loopcount == 3) && (datacount < 11)) ||
			((loopcount > 3) && (datacount < 15)))
	{
		writeraw (lc6, sizeof(lc6), NULL, sub);
		if (data2 && length2)
			writeraw (data2, length2, NULL, sub);
	}
	else
	{
		if (loopcount==1)
		{
			writeraw (lc6, sizeof(lc6), NULL, sub);
			if (data2 && length2)
				writeraw (data2, length2, NULL, sub);
		}
		loopcount++;
		datacount=0;
	}

}

void printdata (struct lib_cc_decode *ctx, const unsigned char *data1, int length1,
                const unsigned char *data2, int length2, struct cc_subtitle *sub)
{
	struct ccx_decoder_608_context *field_1 = ctx->context_cc608_field_1;
	struct ccx_decoder_608_context *field_2 = ctx->context_cc608_field_2;
	if (ctx->write_format==CCX_OF_DVDRAW)
		writeDVDraw (data1, length1, data2, length2, sub);
	else /* Broadcast raw or any non-raw */
	{
		if (length1 && ctx->extract != 2)
		{
			ctx->writedata(data1, length1, field_1, sub);
		}
		if (length2)
		{
			if (ctx->extract != 1)
				ctx->writedata(data2, length2, field_2, sub);
			else // User doesn't want field 2 data, but we want XDS.
				ctx->writedata (data2,length2,NULL, sub);
		}
	}
}

/* Buffer data with the same FTS and write when a new FTS or data==NULL
 * is encountered */
void writercwtdata (struct lib_cc_decode *ctx, const unsigned char *data, struct cc_subtitle *sub)
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
							&& !ctx->fullbin // Unless we want to keep them
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
			ctx->writedata(cbheader, 10, ctx->context_cc608_field_1, sub);
			ctx->writedata(cbbuffer, 3 * cbcount, ctx->context_cc608_field_1, sub);
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
		ctx->writedata(cbheader, 10, ctx->context_cc608_field_1, sub);
		ctx->writedata(cbbuffer, 3 * cbcount, ctx->context_cc608_field_1, sub);

		cbcount = 0;
		cbempty = 0;

		dbg_print(CCX_DMT_CBRAW, "%s Write final padding RCWT blocks.\n",
				print_mstime(currfts));
	}

	prevfts = currfts;
}
