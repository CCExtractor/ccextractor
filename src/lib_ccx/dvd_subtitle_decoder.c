#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "dvd_subtitle_decoder.h"
#include "ccx_common_common.h"

#define MAX_BUFFERSIZE 4096 // arbitrary value

struct DVD_Ctx
{
	unsigned char *buffer; // Buffer to store packet data
	size_t len; //length of buffer required
	int pos; //position in the buffer
	uint16_t size_spu; //total size of spu packet
	uint16_t size_data; //size of data in the packet, offset to control packet
	struct ctrl_seq *ctrl;
	unsigned char *bitmap;
};

struct ctrl_seq
{
	uint8_t color[4];
	uint8_t alpha[4]; // alpha channel
	uint16_t coord[4];
	uint16_t pixoffset[2]; // Offset to 1st and 2nd graphic line
	uint16_t start_time, stop_time;
};

#define bitoff(x) ((x) ? 0x0f : 0xf0)

// Get fisrt 4 or last 4 bits from the byte
#define next4(x,y) ((y) ? (x & 0x0f) : ((x & 0xf0) >> 4) )

// int get_bits(uint16_t temp, int n)
// {
// 	// Get n bits from 16 bits
// 	unsigned mask;
// 	mask = ((1 << n) - 1) << (16-n);
// 	return (temp & mask);
// }

/**
 * Get 4 bits data from buffer for RLE decoding
 */
int get_bits(struct DVD_Ctx *ctx, uint8_t *nextbyte, int *pos, int *m)
{
	int ret;
	ret = (*nextbyte & 0xf0) >> 4;
	if(*m == 0)
		*pos += 1;
	*nextbyte = (*nextbyte << 4) | next4(ctx->buffer[*pos] , *m);
	*m = (*m + 1)%2;

	return ret;
}


int rle_decode(struct DVD_Ctx *ctx, int *color, uint8_t *nextbyte, int *pos, int *m)
{
	int val;
	uint16_t rlen;

	val = 4;
	rlen = get_bits(ctx, nextbyte, pos, m);
	while(rlen < val && val <= 0x40)
	{
		rlen = (rlen << 4) | get_bits(ctx, nextbyte, pos, m);
		val = val << 2;
	}
	*color = rlen & 0x3;
	rlen = rlen >> 2;
	return rlen;
}

void get_bitmap(struct DVD_Ctx *ctx)
{
	int w, h, x, lineno;
	int pos, color, m;
	int len;
	uint8_t nextbyte;
	unsigned char *buffp; // Copy of pointer to buffer to change

	w = (ctx->ctrl->coord[1] - ctx->ctrl->coord[0]) + 1;
	h = (ctx->ctrl->coord[3] - ctx->ctrl->coord[2]) + 1;
	dbg_print(CCX_DMT_VERBOSE, "w:%d h:%d\n", w, h);

	pos = ctx->ctrl->pixoffset[0];
	m = 0;
	nextbyte = ctx->buffer[pos];

	ctx->bitmap = malloc(w*h);
	buffp = ctx->bitmap;
	x = 0; lineno = 0;

	while(lineno < (h+1)/2)
	{
		len = rle_decode(ctx, &color, &nextbyte, &pos, &m);
		// dbg_print(CCX_DMT_VERBOSE, "Len:%d Color:%d ", len, color);

		// len is 0 if data is 0x000* - End of line
		if(len > (w-x) || len == 0)
			len = w-x;

		memset(buffp + x, color, len);
		x+=len;
		if(x >= w)
		{
			// End of line
			x = 0;
			++lineno;

			// Skip 1 line due to interlacing
			buffp += (2*w);

			// Align byte at end of line
			if(bitoff(m) == 0x0f)
			{
				int discard = get_bits(ctx, &nextbyte, &pos, &m);
			}
		}
	}

	// Lines are interlaced, for other half of alternate lines
	// printf("posnow: %d\n", pos);
	if(pos > ctx->ctrl->pixoffset[1])
	{
		dbg_print(CCX_DMT_VERBOSE, "Error creating bitmap!");

		return;
	}

	pos = ctx->ctrl->pixoffset[1];
	buffp = ctx->bitmap + w;
	x = 0; lineno = 0;

	while(lineno < h/2)
	{
		len = rle_decode(ctx, &color, &nextbyte, &pos, &m);

		// len is 0 if data is 0x000* - End of line
		if(len > (w-x) || len == 0)
			len = w-x;

		memset(buffp + x, color, len);
		x+=len;
		if(x >= w)
		{
			// End of line
			x = 0;
			++lineno;

			// Skip 1 line due to interlacing
			buffp += (2*w);

			// Align byte at end of line
			if(bitoff(m) == 0x0f)
			{
				int discard = get_bits(ctx, &nextbyte, &pos, &m);
			}
		}
	}

	int i,j,c=0;
	for(i=0; i<h; ++i)
	{
		for(j=0;j<w;++j)
		{
			printf("%d", ctx->bitmap[c]);
			++c;
		}
		printf("\n");
	}
}


void decode_packet(struct DVD_Ctx *ctx)
{
	uint16_t date, next_ctrl;
	unsigned char *buff = ctx->buffer;

	struct ctrl_seq *control = ctx->ctrl;
	int command; // next command
	int seq_end, pack_end = 0;

	ctx->pos = ctx->size_data;
	dbg_print(CCX_DMT_VERBOSE, "In decode_packet()\n");

	while(ctx->pos <= ctx->len && pack_end == 0)
	{
		// Process control packet first
		date = (buff[ctx->pos] << 8) | buff[ctx->pos + 1];
		next_ctrl = (buff[ctx->pos + 2] << 8) | buff[ctx->pos + 3];
		if(next_ctrl == (ctx->pos))
		{
			// If it is the last control sequence, it points to itself
			pack_end = 1;
		}
		ctx->pos += 4;
		seq_end = 0;

		while(seq_end == 0)
		{
			command = buff[ctx->pos];
			ctx->pos += 1;

			switch(command)
			{
				case 0x01:	control->start_time = date;
							break;
				case 0x02:	control->stop_time = date;
							break;
				case 0x03:	// SET_COLOR
							control->color[0] = (buff[ctx->pos] & 0xf0) >> 4;
							control->color[1] = buff[ctx->pos] & 0x0f;
							control->color[2] = (buff[ctx->pos + 1] & 0xf0) >> 4;
							control->color[3] = buff[ctx->pos + 1] & 0x0f;
							dbg_print(CCX_DMT_VERBOSE, "col: %x col: %x col: %x col: %x\n", control->color[0], control->color[1], control->color[2], control->color[3]);
							ctx->pos+=2;
							break;
				case 0x04:	//SET_CONTR
							control->alpha[0] = (buff[ctx->pos] & 0xf0) >> 4;
							control->alpha[1] = buff[ctx->pos] & 0x0f;
							control->alpha[2] = (buff[ctx->pos + 1] & 0xf0) >> 4;
							control->alpha[3] = buff[ctx->pos + 1] & 0x0f;
							dbg_print(CCX_DMT_VERBOSE, "alp: %d alp: %d alp: %d alp: %d\n", control->alpha[0], control->alpha[1], control->alpha[2], control->alpha[3]);
							ctx->pos+=2;
							break;
				case 0x05:	//SET_DAREA
							control->coord[0] = ((buff[ctx->pos] << 8) | (buff[ctx->pos + 1] & 0xf0)) >> 4 ; //starting x coordinate
							control->coord[1] = ((buff[ctx->pos + 1] & 0x0f) << 8 ) | buff[ctx->pos + 2] ; //ending x coordinate
							control->coord[2] = ((buff[ctx->pos + 3] << 8) | (buff[ctx->pos + 4] & 0xf0)) >> 4 ; //starting y coordinate
							control->coord[3] = ((buff[ctx->pos + 4] & 0x0f) << 8 ) | buff[ctx->pos + 5] ; //ending y coordinate
							dbg_print(CCX_DMT_VERBOSE, "cord: %d cord: %d cord: %d cord: %d\n", control->coord[0], control->coord[1], control->coord[2], control->coord[3]);
							ctx->pos+=6;
							//(x2-x1+1)*(y2-y1+1)
							break;
				case 0x06:	//SET_DSPXA - Pixel address
							control->pixoffset[0] = (buff[ctx->pos] << 8) | buff[ctx->pos + 1];
							control->pixoffset[1] = (buff[ctx->pos + 2] << 8) | buff[ctx->pos + 3];
							dbg_print(CCX_DMT_VERBOSE, "off1: %d off2 %d\n", control->pixoffset[0], control->pixoffset[1]);
							ctx->pos+=4;
							break;
				case 0x07:	dbg_print(CCX_DMT_VERBOSE, "Command 0x07 found\n");
							uint16_t skip = (buff[ctx->pos] << 8) | buff[ctx->pos + 1];
							ctx->pos+=skip;
							break;
				case 0xff:	seq_end = 1;
							break;
				default:	dbg_print(CCX_DMT_VERBOSE, "Unknown command in control sequence!\n");
			}
		}

	}
	// Decode data
	get_bitmap(ctx);
}


int process_spu(struct lib_cc_decode *dec_ctx, unsigned char *buff, int length)
{
	struct DVD_Ctx *ctx = (struct DVD_Ctx *)init_dvdsub_decode();
	ctx->buffer = buff;
	ctx->len = length;

	ctx->size_spu = (ctx->buffer[0] << 8) | ctx->buffer[1];
	if(ctx->size_spu > length)
	{
		// TODO: Data might be spread over several packets, handle this case to append to one buffer
		dbg_print(CCX_DMT_VERBOSE, "Data might be spread over several packets\n");
		return length;
	}

	if(ctx->size_spu != length)
	{
		dbg_print(CCX_DMT_VERBOSE, "SPU size mismatch\n");
		// return -1;
		return length; //FIXME: not the write thing to return
	}

	ctx->size_data = (ctx->buffer[2] << 8) | ctx->buffer[3];
	if(ctx->size_data > ctx->size_spu)
	{
		dbg_print(CCX_DMT_VERBOSE, "Invalid SPU Packet\n");
		// return -1;
		return length; //FIXME: not the write thing to return
	}

	decode_packet(ctx);

	return length;
}

void *init_dvdsub_decode()
{
	struct DVD_Ctx *ctx = malloc(sizeof(struct DVD_Ctx));
	ctx->ctrl = malloc(sizeof(struct ctrl_seq));
	return (void *) ctx;
}
