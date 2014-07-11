#ifndef _CC_DECODER_COMMON
#define _CC_DECODER_COMMON

struct cc_subtitle
{
	void *data;
	size_t size;
	enum ccx_encoding_type format;
};
#endif
