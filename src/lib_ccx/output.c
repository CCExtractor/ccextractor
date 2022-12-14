#include "lib_ccx.h"
#include "ccextractor.h"
#include "ccx_common_option.h"
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

void dinit_write(struct ccx_s_write *wb)
{
	if (wb == NULL)
	{
		return;
	}
	if (wb->fh > 0)
		close(wb->fh);
	freep(&wb->filename);
	freep(&wb->original_filename);
	if (wb->with_semaphore && wb->semaphore_filename)
	{
		unlink(wb->semaphore_filename);
	}
	freep(&wb->semaphore_filename);
}

int temporarily_close_output(struct ccx_s_write *wb)
{
	close(wb->fh);
	wb->fh = -1;
	wb->temporarily_closed = 1;
	return 0;
}

int temporarily_open_output(struct ccx_s_write *wb)
{
	int t = 0;
	// Try a few times before giving up. This is because this close/open stuff exists for processes
	// that demand exclusive access to the file, so often we'll find out that we cannot reopen the
	// file immediately.
	while (t < 5 && wb->fh == -1)
	{
		wb->fh = open(wb->filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
		sleep(1);
	}
	if (wb->fh == -1)
	{
		return CCX_COMMON_EXIT_FILE_CREATION_FAILED;
	}
	wb->temporarily_closed = 0;
	return EXIT_OK;
}

int init_write(struct ccx_s_write *wb, char *filename, int with_semaphore)
{
	memset(wb, 0, sizeof(struct ccx_s_write));
	wb->fh = -1;
	wb->temporarily_closed = 0;
	wb->filename = filename;
	wb->original_filename = strdup(filename);

	wb->with_semaphore = with_semaphore;
	wb->append_mode = ccx_options.enc_cfg.append_mode;
	if (!(wb->append_mode))
		wb->fh = open(filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
	else
		wb->fh = open(filename, O_RDWR | O_CREAT | O_APPEND | O_BINARY, S_IREAD | S_IWRITE);
	wb->renaming_extension = 0;
	if (wb->fh == -1)
	{
		return CCX_COMMON_EXIT_FILE_CREATION_FAILED;
	}
	if (with_semaphore)
	{
		wb->semaphore_filename = (char *)malloc(strlen(filename) + 6);
		if (!wb->semaphore_filename)
			return EXIT_NOT_ENOUGH_MEMORY;
		sprintf(wb->semaphore_filename, "%s.sem", filename);
		int t = open(wb->semaphore_filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
		if (t == -1)
		{
			close(wb->fh);
			return CCX_COMMON_EXIT_FILE_CREATION_FAILED;
		}
		close(t);
	}
	return EXIT_OK;
}

int writeraw(const unsigned char *data, int length, void *private_data, struct cc_subtitle *sub)
{
	unsigned char *sub_data = NULL;
	// Don't do anything for empty data
	if (data == NULL)
		return -1;

	sub->data = realloc(sub->data, length + sub->nb_data);
	if (!sub->data)
		return EXIT_NOT_ENOUGH_MEMORY;
	sub_data = sub->data;
	sub->datatype = CC_DATATYPE_GENERIC;
	memcpy(sub_data + sub->nb_data, data, length);
	sub->got_output = 1;
	sub->nb_data += length;
	sub->type = CC_RAW;

	return EXIT_SUCCESS;
}

void writeDVDraw(const unsigned char *data1, int length1,
		 const unsigned char *data2, int length2,
		 struct cc_subtitle *sub)
{
	/* these are only used by DVD raw mode: */
	static int loopcount = 1; /* loop 1: 5 elements, loop 2: 8 elements,
				     loop 3: 11 elements, rest: 15 elements */
	static int datacount = 0; /* counts within loop */

	if (datacount == 0)
	{
		writeraw(DVD_HEADER, sizeof(DVD_HEADER), NULL, sub);
		if (loopcount == 1)
			writeraw(lc1, sizeof(lc1), NULL, sub);
		if (loopcount == 2)
			writeraw(lc2, sizeof(lc2), NULL, sub);
		if (loopcount == 3)
		{
			writeraw(lc3, sizeof(lc3), NULL, sub);
			if (data2 && length2)
				writeraw(data2, length2, NULL, sub);
		}
		if (loopcount > 3)
		{
			writeraw(lc4, sizeof(lc4), NULL, sub);
			if (data2 && length2)
				writeraw(data2, length2, NULL, sub);
		}
	}
	datacount++;
	writeraw(lc5, sizeof(lc5), NULL, sub);
	if (data1 && length1)
		writeraw(data1, length1, NULL, sub);
	if (((loopcount == 1) && (datacount < 5)) || ((loopcount == 2) && (datacount < 8)) || ((loopcount == 3) && (datacount < 11)) ||
	    ((loopcount > 3) && (datacount < 15)))
	{
		writeraw(lc6, sizeof(lc6), NULL, sub);
		if (data2 && length2)
			writeraw(data2, length2, NULL, sub);
	}
	else
	{
		if (loopcount == 1)
		{
			writeraw(lc6, sizeof(lc6), NULL, sub);
			if (data2 && length2)
				writeraw(data2, length2, NULL, sub);
		}
		loopcount++;
		datacount = 0;
	}
}

void printdata(struct lib_cc_decode *ctx, const unsigned char *data1, int length1,
	       const unsigned char *data2, int length2, struct cc_subtitle *sub)
{
	if (ctx->write_format == CCX_OF_DVDRAW)
		writeDVDraw(data1, length1, data2, length2, sub);
	else /* Broadcast raw or any non-raw */
	{
		if (length1 && ctx->extract != 2)
		{
			ctx->current_field = 1;
			ctx->writedata(data1, length1, ctx, sub);
		}
		if (length2)
		{
			ctx->current_field = 2;
			if (ctx->extract != 1)
				ctx->writedata(data2, length2, ctx, sub);
			else // User doesn't want field 2 data, but we want XDS.
			{
				ctx->writedata(data2, length2, ctx, sub);
			}
		}
	}
}

/* Buffer data with the same FTS and write when a new FTS or data==NULL
 * is encountered */
void writercwtdata(struct lib_cc_decode *ctx, const unsigned char *data, struct cc_subtitle *sub)
{
	static LLONG prevfts = -1;
	LLONG currfts = ctx->timing->fts_now + ctx->timing->fts_global;
	static uint16_t cbcount = 0;
	static int cbempty = 0;
	static unsigned char cbbuffer[0xFFFF * 3]; // TODO: use malloc
	static unsigned char cbheader[8 + 2];

	if ((prevfts != currfts && prevfts != -1) || data == NULL || cbcount == 0xFFFF)
	{
		// Remove trailing empty or 608 padding caption blocks
		if (cbcount != 0xFFFF)
		{
			unsigned char cc_valid;
			unsigned char cc_type;
			int storecbcount = cbcount;

			for (int cb = cbcount - 1; cb >= 0; cb--)
			{
				cc_valid = (*(cbbuffer + 3 * cb) & 4) >> 2;
				cc_type = *(cbbuffer + 3 * cb) & 3;

				// The -fullbin option disables pruning of 608 padding blocks
				if ((cc_valid && cc_type <= 1 // Only skip NTSC padding packets
				     && !ctx->fullbin	      // Unless we want to keep them
				     && *(cbbuffer + 3 * cb + 1) == 0x80 && *(cbbuffer + 3 * cb + 2) == 0x80) ||
				    !(cc_valid || cc_type == 3)) // or unused packets
				{
					cbcount--;
				}
				else
				{
					cb = -1;
				}
			}
			dbg_print(CCX_DMT_CBRAW, "%s Write %d RCWT blocks - skipped %d padding / %d unused blocks.\n",
				  print_mstime_static(prevfts), cbcount, storecbcount - cbcount, cbempty);
		}

		// New FTS, write data header
		// RCWT data header (10 bytes):
		// byte(s)   value   description
		// 0-7       FTS     int64_t number with current FTS
		// 8-9       blocks  Number of 3 byte data blocks with the same FTS that are
		//                  following this header
		memcpy(cbheader, &prevfts, 8);
		memcpy(cbheader + 8, &cbcount, 2);

		if (cbcount > 0)
		{
			ctx->writedata(cbheader, 10, ctx->context_cc608_field_1, sub);
			ctx->writedata(cbbuffer, 3 * cbcount, ctx->context_cc608_field_1, sub);
		}
		cbcount = 0;
		cbempty = 0;
	}

	if (data)
	{
		// Store the data while the FTS is unchanged

		unsigned char cc_valid = (*data & 4) >> 2;
		unsigned char cc_type = *data & 3;
		// Store only non-empty packets
		if (cc_valid || cc_type == 3)
		{
			// Store in buffer until we know how many blocks come with
			// this FTS.
			memcpy(cbbuffer + cbcount * 3, data, 3);
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
		currfts -= 1001 / 30;

		memcpy(cbheader, &currfts, 8);
		cbcount = 2;
		memcpy(cbheader + 8, &cbcount, 2);

		memcpy(cbbuffer, "\x04\x80\x80", 3);	 // Field 1 padding
		memcpy(cbbuffer + 3, "\x05\x80\x80", 3); // Field 2 padding
		ctx->writedata(cbheader, 10, ctx->context_cc608_field_1, sub);
		ctx->writedata(cbbuffer, 3 * cbcount, ctx->context_cc608_field_1, sub);

		cbcount = 0;
		cbempty = 0;

		dbg_print(CCX_DMT_CBRAW, "%s Write final padding RCWT blocks.\n",
			  print_mstime_static(currfts));
	}

	prevfts = currfts;
}
