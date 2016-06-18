#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "dvd_subtitle_decoder.h"
#include "ccx_common_common.h"

#define MAX_BUFFERSIZE 4096 // arbitrary value

struct dvd_cc_data
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

void process_ctrl_seq(struct dvd_cc_data *data)
{
	uint16_t date, next_ctrl;
	data->ctrl = malloc(sizeof(struct ctrl_seq));
	struct ctrl_seq *control = data->ctrl; 
	int command; // next command
	int seq_end, pack_end = 0;

	data->pos = data->size_data;
	dbg_print(CCX_DMT_VERBOSE, "In process_ctrl_seq()\n");

	while(data->pos <= data->len && pack_end == 0)
	{
		date = (data->buffer[data->pos] << 8) | data->buffer[data->pos + 1];
		next_ctrl = (data->buffer[data->pos + 2] << 8) | data->buffer[data->pos + 3];
		if(next_ctrl == (data->pos))
		{
			// If it is the last control sequence, it points to itself
			pack_end = 1;
		}
		data->pos += 4;
		seq_end = 0;

		while(seq_end == 0)
		{
			command = data->buffer[data->pos];
			data->pos += 1;
			switch(command)
			{
				case 0x01:	control->start_time = date;
							break;
				case 0x02:	control->stop_time = date; 
							break;
				case 0x03:	// SET_COLOR
							control->color[0] = (data->buffer[data->pos] & 0xf0) >> 4;
							control->color[1] = data->buffer[data->pos] & 0x0f;
							control->color[2] = (data->buffer[data->pos + 1] & 0xf0) >> 4;
							control->color[3] = data->buffer[data->pos + 1] & 0x0f;
							data->pos+=2;
							break;
				case 0x04:	//SET_CONTR
							control->alpha[0] = (data->buffer[data->pos] & 0xf0) >> 4;
							control->alpha[1] = data->buffer[data->pos] & 0x0f;
							control->alpha[2] = (data->buffer[data->pos + 1] & 0xf0) >> 4;
							control->alpha[3] = data->buffer[data->pos + 1] & 0x0f;
							data->pos+=2;
							break;
				case 0x05:	//SET_DAREA
							control->coord[0] = ((data->buffer[data->pos] << 8) | (data->buffer[data->pos + 1] & 0xf0)) >> 4 ; //starting x coordinate
							control->coord[1] = ((data->buffer[data->pos + 1] & 0x0f) << 8 ) | data->buffer[data->pos + 2] ; //ending x coordinate
							control->coord[2] = ((data->buffer[data->pos + 3] << 8) | (data->buffer[data->pos + 4] & 0xf0)) >> 4 ; //starting y coordinate
							control->coord[3] = ((data->buffer[data->pos + 4] & 0x0f) << 8 ) | data->buffer[data->pos + 5] ; //ending y coordinate
							data->pos+=6;
							//(x1-x2+1)*(y1-y2+1)
							break;
				case 0x06:	//SET_DSPXA - Pixel address
							control->pixoffset[0] = (data->buffer[data->pos] << 8) | data->buffer[data->pos + 1];
							control->pixoffset[1] = (data->buffer[data->pos + 2] << 8) | data->buffer[data->pos + 3];
							data->pos+=4;
							break;
				case 0x07:	dbg_print(CCX_DMT_VERBOSE, "Command 0x07 found\n");
							uint16_t skip = (data->buffer[data->pos] << 8) | data->buffer[data->pos + 1];
							data->pos+=skip;
							break;
				case 0xff:	seq_end = 1;
							break;
				default:	dbg_print(CCX_DMT_VERBOSE, "Unknown command in control sequence!\n");
			}
		}
	}





}


int process_spu(unsigned char *buff, int length)
{
	struct dvd_cc_data *data = malloc( sizeof(struct dvd_cc_data) );
	data->buffer = buff;
	data->len = length;
	
	data->size_spu = (data->buffer[0] << 8) | data->buffer[1];
	if(data->size_spu > length)
	{
		// TODO: Data might be spread over several packets, handle this case to append to one buffer
		dbg_print(CCX_DMT_VERBOSE, "Data might be spread over several packets\n");
		return length;
	}

	if(data->size_spu != length)
	{
		dbg_print(CCX_DMT_VERBOSE, "SPU size mismatch\n");
		// return -1;
		return length; //FIXME: not the write thing to return
	}

	data->size_data = (data->buffer[2] << 8) | data->buffer[3];
	if(data->size_data > data->size_spu)
	{
		dbg_print(CCX_DMT_VERBOSE, "Invalid SPU Packet\n");
		// return -1;
		return length; //FIXME: not the write thing to return
	} 

	process_ctrl_seq(data);

	return length;
}

