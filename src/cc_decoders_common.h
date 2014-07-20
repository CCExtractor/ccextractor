#ifndef _CC_DECODER_COMMON
#define _CC_DECODER_COMMON

struct cc_subtitle
{
	void *data;
	unsigned int nb_data;
	int got_output;
};

int process608(const unsigned char *data, int length, struct s_context_cc608 *context, struct cc_subtitle *sub);
void handle_end_of_data(struct s_context_cc608 *context, struct cc_subtitle *sub);
#endif
