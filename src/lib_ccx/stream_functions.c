#include "lib_ccx.h"
#include "ccx_common_option.h"
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include "activity.h"
#include "utility.h"
#include "ccx_common_timing.h"
#include "file_buffer.h"
#include "ccx_gxf.h"
#include "ccx_demuxer_mxf.h"

void detect_stream_type (struct ccx_demuxer *ctx)
{
	ctx->stream_mode=CCX_SM_ELEMENTARY_OR_NOT_FOUND; // Not found
	ctx->startbytes_avail = (int) buffered_read_opt(ctx, ctx->startbytes, STARTBYTESLENGTH);

	if( ctx->startbytes_avail == -1)
		fatal (EXIT_READ_ERROR, "Error reading input file!\n");

	if (ctx->startbytes_avail>=4)
	{
		// Check for ASF magic bytes
		if (ctx->startbytes[0]==0x30 &&
				ctx->startbytes[1]==0x26 &&
				ctx->startbytes[2]==0xb2 &&
				ctx->startbytes[3]==0x75)
			ctx->stream_mode=CCX_SM_ASF;
	}

	// WARNING: Always check containers first (such as Matroska),
	// because they contain bytes that can be mistaken as a separate video file.
	if (ctx->stream_mode == CCX_SM_ELEMENTARY_OR_NOT_FOUND && ctx->startbytes_avail >= 4)
	{
		// Check for Matroska magic bytes - EBML head
		if(ctx->startbytes[0]==0x1a &&
		   ctx->startbytes[1]==0x45 &&
		   ctx->startbytes[2]==0xdf &&
		   ctx->startbytes[3]==0xa3)
			ctx->stream_mode = CCX_SM_MKV;
		// Check for Matroska magic bytes - Segment
		if(ctx->startbytes[0]==0x18 &&
		   ctx->startbytes[1]==0x53 &&
		   ctx->startbytes[2]==0x80 &&
		   ctx->startbytes[3]==0x67)
			ctx->stream_mode = CCX_SM_MKV;
	}
	if (ctx->stream_mode == CCX_SM_ELEMENTARY_OR_NOT_FOUND)
	{
		if (ccx_gxf_probe(ctx->startbytes, ctx->startbytes_avail) == CCX_TRUE)
		{
			ctx->stream_mode = CCX_SM_GXF;
			ctx->private_data = ccx_gxf_init(ctx);
		}
	}
	if (ctx->stream_mode == CCX_SM_ELEMENTARY_OR_NOT_FOUND && ctx->startbytes_avail >= 4)
	{
		if(ctx->startbytes[0]==0xb7 &&
				ctx->startbytes[1]==0xd8 &&
				ctx->startbytes[2]==0x00 &&
				ctx->startbytes[3]==0x20)
			ctx->stream_mode = CCX_SM_WTV;
	}
#ifdef WTV_DEBUG
	if (ctx->stream_mode==CCX_SM_ELEMENTARY_OR_NOT_FOUND && ctx->startbytes_avail>=6)
	{
		// Check for hexadecimal dump generated by wtvccdump
		// ; CCHD
		if (ctx->startbytes[0]==';' &&
				ctx->startbytes[1]==' ' &&
				ctx->startbytes[2]=='C' &&
				ctx->startbytes[3]=='C' &&
				ctx->startbytes[4]=='H' &&
				ctx->startbytes[5]=='D')
			ctx->stream_mode= CCX_SM_HEX_DUMP;
	}
#endif

	if (ctx->stream_mode==CCX_SM_ELEMENTARY_OR_NOT_FOUND && ctx->startbytes_avail>=11)
	{
		// Check for CCExtractor magic bytes
		if (ctx->startbytes[0]==0xCC &&
				ctx->startbytes[1]==0xCC &&
				ctx->startbytes[2]==0xED &&
				ctx->startbytes[8]==0 &&
				ctx->startbytes[9]==0 &&
				ctx->startbytes[10]==0)
			ctx->stream_mode=CCX_SM_RCWT;
	}
	if ((ctx->stream_mode == CCX_SM_ELEMENTARY_OR_NOT_FOUND || ccx_options.print_file_reports)
			&& ctx->startbytes_avail >= 4) // Still not found
	{
		size_t idx = 0, nextBoxLocation = 0;
		int boxScore = 0;
		// Scan the buffer for valid succeeding MP4 boxes.
		while (idx < ctx->startbytes_avail - 8){
			// Check if we have a valid box
			if (isValidMP4Box(ctx->startbytes, idx, &nextBoxLocation, &boxScore))
			{
				idx = nextBoxLocation; // If the box is valid, a new box should be found on the next location... Not somewhere in between.
				if (boxScore > 7)
				{
					break;
				}
				continue;
			}
			else
			{
				// Not a valid box, reset score. We need a couple of successive boxes to identify a MP4 file.
				boxScore = 0;
				idx++;
			}
		}
		if (boxScore > 1)
		{
			// We had at least one box (or multiple) at the end to "claim" this is MP4. A single valid box at the end is doubtful...
			ctx->stream_mode = CCX_SM_MP4;
		}
	}

	if (ctx->stream_mode == CCX_SM_ELEMENTARY_OR_NOT_FOUND) // Search for MXF header
	{
		if (ccx_probe_mxf(ctx) == CCX_TRUE )
		{
			ctx->stream_mode = CCX_SM_MXF;
			ctx->private_data = ccx_mxf_init(ctx);
		}
	}
	if (ctx->stream_mode==CCX_SM_ELEMENTARY_OR_NOT_FOUND) // Still not found
	{
		if (ctx->startbytes_avail > 188*8) // Otherwise, assume no TS
		{
			// First check for TS
			for (unsigned i=0; i<188;i++)
			{
				if (ctx->startbytes[i]==0x47 && ctx->startbytes[i+188]==0x47 &&
						ctx->startbytes[i+188*2]==0x47 && ctx->startbytes[i+188*3]==0x47 &&
						ctx->startbytes[i+188*4]==0x47 && ctx->startbytes[i+188*5]==0x47 &&
						ctx->startbytes[i+188*6]==0x47 && ctx->startbytes[i+188*7]==0x47
				   )
				{
					// Eight sync bytes, that's good enough
					ctx->startbytes_pos=i;
					ctx->stream_mode=CCX_SM_TRANSPORT;
					ctx->m2ts = 0;
					break;
				}
			}
			if (ctx->stream_mode == CCX_SM_TRANSPORT)
			{
				dbg_print(CCX_DMT_PARSE, "detect_stream_type: detected as TS\n");
				return_to_buffer (ctx, ctx->startbytes, (unsigned int)ctx->startbytes_avail);
				return;
			}

			// Check for M2TS
			for (unsigned i = 0; i<192; i++)
			{
				if (ctx->startbytes[i+4] == 0x47 && ctx->startbytes[i + 4 + 192] == 0x47 &&
						ctx->startbytes[i + 192 * 2+4] == 0x47 && ctx->startbytes[i + 192 * 3+4] == 0x47 &&
						ctx->startbytes[i + 192 * 4+4] == 0x47 && ctx->startbytes[i + 192 * 5+4] == 0x47 &&
						ctx->startbytes[i + 192 * 6+4] == 0x47 && ctx->startbytes[i + 192 * 7+4] == 0x47
				   )
				{
					// Eight sync bytes, that's good enough
					ctx->startbytes_pos = i;
					ctx->stream_mode = CCX_SM_TRANSPORT;
					ctx->m2ts = 1;
					break;
				}
			}
			if (ctx->stream_mode == CCX_SM_TRANSPORT)
			{
				dbg_print(CCX_DMT_PARSE, "detect_stream_type: detected as M2TS\n");
				return_to_buffer (ctx, ctx->startbytes, (unsigned int)ctx->startbytes_avail);
				return;
			}

			// Now check for PS (Needs PACK header)
			for (unsigned i=0;
					i < (unsigned) (ctx->startbytes_avail<50000?ctx->startbytes_avail-3:49997);
					i++)
			{
				if (ctx->startbytes[i]==0x00 && ctx->startbytes[i+1]==0x00 &&
						ctx->startbytes[i+2]==0x01 && ctx->startbytes[i+3]==0xBA)
				{
					// If we find a PACK header it is not an ES
					ctx->startbytes_pos=i;
					ctx->stream_mode=CCX_SM_PROGRAM;
					break;
				}
			}
			if (ctx->stream_mode == CCX_SM_PROGRAM)
			{
				dbg_print(CCX_DMT_PARSE, "detect_stream_type: detected as PS\n");
			}

			// TiVo is also a PS
			if (ctx->startbytes[0]=='T' && ctx->startbytes[1]=='i' &&
					ctx->startbytes[2]=='V' && ctx->startbytes[3]=='o')
			{
				// The TiVo header is longer, but the PS loop will find the beginning
				dbg_print(CCX_DMT_PARSE, "detect_stream_type: detected as Tivo PS\n");
				ctx->startbytes_pos = 187;
				ctx->stream_mode = CCX_SM_PROGRAM;
				ctx->strangeheader = 1; // Avoid message about unrecognized header
			}
		}
		else
		{
			ctx->startbytes_pos=0;
			ctx->stream_mode=CCX_SM_ELEMENTARY_OR_NOT_FOUND;
		}
	}
	// Don't use STARTBYTESLENGTH. It might be longer than the file length!
	return_to_buffer (ctx, ctx->startbytes, ctx->startbytes_avail);
}


int detect_myth( struct ccx_demuxer *ctx )
{
	int vbi_blocks=0;
	// VBI data? if yes, use myth loop
	// STARTBYTESLENGTH is 1MB, if the file is shorter we will never detect
	// it as a mythTV file
	if (ctx->startbytes_avail==STARTBYTESLENGTH)
	{
		unsigned char uc[3];
		memcpy (uc,ctx->startbytes,3);
		for (int i=3;i<ctx->startbytes_avail;i++)
		{
			if (((uc[0]=='t') && (uc[1]=='v') && (uc[2] == '0')) ||
					((uc[0]=='T') && (uc[1]=='V') && (uc[2] == '0')))
				vbi_blocks++;
			uc[0]=uc[1];
			uc[1]=uc[2];
			uc[2]=ctx->startbytes[i];
		}
	}
	if (vbi_blocks>10) // Too much coincidence
		return 1;

	return 0;
}

/* Read and evaluate the current video PES header. The function returns
 * the length of the payload if successful, otherwise -1 is returned
 * indicating a premature end of file / too small buffer.
 * If sbuflen is
 *    0 .. Read from file into nextheader
 *    >0 .. Use data in nextheader with the length of sbuflen
 */
int read_video_pes_header (struct ccx_demuxer *ctx, struct demuxer_data *data, unsigned char *nextheader, int *headerlength, int sbuflen)
{
	// Read the next video PES
	// ((nextheader[3]&0xf0)==0xe0)
	long long result;
	unsigned peslen=nextheader[4]<<8 | nextheader[5];
	unsigned payloadlength = 0; // Length of packet data bytes
	static LLONG current_pts_33=0; // Last PTS from the header, without rollover bits

	if ( !sbuflen )
	{
		// Extension present, get it
		result = buffered_read (ctx, nextheader+6,3);
		ctx->past=ctx->past+result;
		if (result!=3) {
			// Consider this the end of the show.
			return -1;
		}
	}
	else
	{
		if (data->bufferdatatype == CCX_DVB_SUBTITLE
				&& peslen == 1 && nextheader[6] == 0xFF)
		{
			*headerlength = sbuflen;
			return 0;
		}
		if (sbuflen < 9) // We need at least 9 bytes to continue
		{
			return -1;
		}
	}
	*headerlength = 6+3;

	unsigned hskip=0;

	// Assume header[8] is right, but double check
	if ( !sbuflen )
	{
		if (nextheader[8] > 0)
		{
			result = buffered_read (ctx, nextheader+9,nextheader[8]);
			ctx->past = ctx->past+result;
			if (result!=nextheader[8])
			{
				return -1;
			}
		}
	}
	else
	{
		// See if the buffer is big enough
		if( sbuflen < *headerlength + (int)nextheader[8] )
			return -1;
	}
	*headerlength += (int) nextheader[8];
	int falsepes = 0;
	/* int pesext = 0; */

	// Avoid false positives, check --- not really needed
	if ( (nextheader[7]&0xC0) == 0x80 )
	{
		// PTS only
		hskip += 5;
		if( (nextheader[9]&0xF1) != 0x21 || (nextheader[11]&0x01) != 0x01
				|| (nextheader[13]&0x01) != 0x01 )
		{
			falsepes = 1;
			mprint("False PTS\n");
		}
	}
	else if ( (nextheader[7]&0xC0) == 0xC0 )
	{
		// PTS and DTS
		hskip += 10;
		if( (nextheader[9]&0xF1) != 0x31 || (nextheader[11]&0x01) != 0x01
				|| (nextheader[13]&0x01) != 0x01
				|| (nextheader[14]&0xF1) != 0x11 || (nextheader[16]&0x01) != 0x01
				|| (nextheader[18]&0x01) != 0x01 ) {
			falsepes = 1;
			mprint("False PTS/DTS\n");
		}
	}
	else if ( (nextheader[7]&0xC0) == 0x40 )
	{
		// Forbidden
		falsepes = 1;
		mprint("False PTS/DTS flag\n");
	}
	if ( !falsepes && nextheader[7]&0x20 )
	{ // ESCR
		if ((nextheader[9+hskip]&0xC4) != 0x04 || !(nextheader[11+hskip]&0x04)
				|| !(nextheader[13+hskip]&0x04) || !(nextheader[14+hskip]&0x01) )
		{
			falsepes = 1;
			mprint("False ESCR\n");
		}
		hskip += 6;
	}
	if ( !falsepes && nextheader[7]&0x10 )
	{ // ES
		if ( !(nextheader[9+hskip]&0x80) || !(nextheader[11+hskip]&0x01) ) {
			mprint("False ES\n");
			falsepes = 1;
		}
		hskip += 3;
	}
	if ( !falsepes && nextheader[7]&0x04)
	{ // add copy info
		if ( !(nextheader[9+hskip]&0x80) ) {
			mprint("False add copy info\n");
			falsepes = 1;
		}
		hskip += 1;
	}
	if ( !falsepes && nextheader[7]&0x02)
	{ // PES CRC
		hskip += 2;
	}
	if ( !falsepes && nextheader[7]&0x01)
	{ // PES extension
		if ( (nextheader[9+hskip]&0x0E)!=0x0E )
		{
			mprint("False PES ext\n");
			falsepes = 1;
		}
		hskip += 1;
		/* pesext = 1; */
	}

	if ( !falsepes )
	{
		hskip = nextheader[8];
	}

	if ( !falsepes && nextheader[7]&0x80 )
	{
		// Read PTS from byte 9,10,11,12,13

		LLONG bits_9 = ((LLONG) nextheader[9] & 0x0E)<<29; // PTS 32..30 - Must be LLONG to prevent overflow
		unsigned bits_10 = nextheader[10] << 22; // PTS 29..22
		unsigned bits_11 = (nextheader[11] & 0xFE) << 14; // PTS 21..15
		unsigned bits_12 = nextheader[12] << 7; // PTS 14-7
		unsigned bits_13 = nextheader[13] >> 1; // PTS 6-0

		if (data->pts != CCX_NOPTS) // Otherwise can't check for rollovers yet
		{
			if (!bits_9 && ((current_pts_33>>30)&7)==7) // PTS about to rollover
				data->rollover_bits++;
			if ((bits_9>>30)==7 && ((current_pts_33>>30)&7)==0) // PTS rollback? Rare and if happens it would mean out of order frames
				data->rollover_bits--;
		}		

		current_pts_33 = bits_9 | bits_10 | bits_11 | bits_12 | bits_13;
		data->pts = (LLONG) data->rollover_bits<<33 | current_pts_33;

		/* The user data holding the captions seems to come between GOP and
		 * the first frame. The sync PTS (sync_pts) (set at picture 0)
		 * corresponds to the first frame of the current GOP. */
	}

	// This might happen in PES packets in TS stream. It seems that when the
	// packet length is unknown it is set to 0.
	if (peslen+6 >= hskip+9)
	{
		payloadlength = peslen - (hskip + 3); // for [6], [7] and [8]
	}

	// Use a save default if this is not a real PES header
	if (falsepes)
	{
		mprint("False PES header\n");
	}

	dbg_print(CCX_DMT_VERBOSE, "PES header length: %d (%d verified)  Data length: %d\n",
			*headerlength, hskip+9, payloadlength);

	return payloadlength;
}

/*
 * Structure for a MP4 box type. Contains the name of the box type and the score we want to assign when a given box type is found.
 */
typedef struct ccx_stream_mp4_box
{
	char boxType[5]; // Name of the box structure
	int score; // The weight of how heavy a box structure should be counted in order to "recognize" an MP4 file.
} ccx_stream_mp4_box;

/* The box types below are taken from table 1 from ISO/IEC 14496-12_2012 (page 12 and further).
* An asterisk (*) marks a mandatory box for a regular file.
* Box types that are on the second level or deeper are omitted.
*/
ccx_stream_mp4_box ccx_stream_mp4_boxes[16] = {
		{ "ftyp", 6 }, // File type and compatibility*
		{ "pdin", 1 }, // Progressive download information
		{ "moov", 5 }, // Container for all metadata*
		{ "moof", 4 }, // Movie fragment
		{ "mfra", 1 }, // Movie fragment random access
		{ "mdat", 2 }, // Media data container
		{ "free", 1 }, // Free space
		{ "skip", 1 }, // Free space
		{ "meta", 1 }, // Metadata
		{ "wide", 1 }, // For boxes that are > 2^32 bytes (https://developer.apple.com/library/mac/documentation/QuickTime/QTFF/QTFFChap1/qtff1.html)
		{ "void", 1 },  // Unknown where this is from/for, assume free space.

		// new ones in standard ISO/IEC 14496-12:2015
		{ "meco", 1 }, // additional metadata container
		{ "styp", 1 }, // segment type
		{ "sidx", 1 }, // segment index
		{ "ssix", 1 }, // subsegment index
		{ "prft", 1 }  // producer reference time
};

/*
 * Checks if the buffer, beginning at the given position, contains a valid MP4 box (defined in the ccx_stream_mp4_boxes array). 
 * If it does contain one, it will set the nextBoxLocation to position + current box size (this should be the place in the buffer 
 * where the next box should be in case of an mp4) and increment the box score with the defined score for the found box.
 *
 * Returns 1 when a box is found, 0 when none is found.
 */
int isValidMP4Box(unsigned char *buffer, size_t position, size_t *nextBoxLocation, int *boxScore)
{
	for (int idx = 0; idx < 16; idx++)
	{
		if (buffer[position + 4] == ccx_stream_mp4_boxes[idx].boxType[0] && buffer[position + 5] == ccx_stream_mp4_boxes[idx].boxType[1] &&
				buffer[position + 6] == ccx_stream_mp4_boxes[idx].boxType[2] && buffer[position + 7] == ccx_stream_mp4_boxes[idx].boxType[3]){
			mprint("Detected MP4 box with name: %s\n", ccx_stream_mp4_boxes[idx].boxType);
			// Box name matches. Do crude validation of possible box size, and if valid, add points for "valid" box
			size_t boxSize = buffer[position] << 24;
			boxSize |= buffer[position + 1] << 16;
			boxSize |= buffer[position + 2] << 8;
			boxSize |= buffer[position + 3];
			*boxScore += ccx_stream_mp4_boxes[idx].score;
			if (boxSize == 0 || boxSize == 1)
			{
				// If 0, runs to end of file. If 1, next field contains int_64 containing size, which is definitely bigger than the cache. Both times we can skip further checks by setting
				// nextBoxLocation to the max possible.
				*nextBoxLocation = UINT32_MAX;

			}
			else
			{
				// Set nextBoxLocation to current position + boxSize, as that will be the location of the next box.
				*nextBoxLocation = position + boxSize;
			}
			return 1;
		}

	}
	return 0;
}
