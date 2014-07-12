#ifndef _CC_DECODER_COMMON
#define _CC_DECODER_COMMON

struct cc_subtitle
{
	void *data;
	size_t size;
	enum ccx_encoding_type format;
};

void process608(const unsigned char *data, int length, struct s_context_cc608 *context, struct cc_subtitle *sub);
#endif
