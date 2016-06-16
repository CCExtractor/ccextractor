#include <stdio.h>

#include "dvd_subtitle_decoder.h"
#include "ccx_common_common.h"


struct dvd_cc_data
{
	unsigned char *buffer; // Buffer to store packet data
	int pos; //position in the buffer
	uint16_t size_spu; //total size of spu packet
	uint16_t size_data; //size of data in the packet, offset to control packet
};


int process_spu(unsigned char *buff, int length)
{
	struct dvd_cc_data *data;

	data->buffer = buff;
	data->size_spu = (data->buffer[0] << 8) | data->buffer[1];
	if(data->size_spu != length)
	{
		//TODO: Might be data spread over multiple packets, check for it
		dbg_print(CCX_DMT_VERBOSE, "SPU size mismatch");
		return -1;
	}

	data->size_data = (data->buffer[2] << 8) | data->buffer[3];
	if(data->size_data > data->size_spu)
	{
		dbg_print(CCX_DMT_VERBOSE, "Invalid SPU Packet");
		return -1;
	} 
	
	return length;
}