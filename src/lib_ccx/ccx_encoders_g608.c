#include "lib_ccx.h"
#include "ccx_encoders_common.h"
#include "ccx_encoders_helpers.h"

static unsigned int get_line_encoded(struct encoder_ctx *ctx, unsigned char *buffer, int line_num, struct eia608_screen *data)
{
	unsigned char *orig = buffer; // Keep for debugging
	unsigned char *line = data->characters[line_num];
	for (int i = 0; i < 32; i++)
	{
		int bytes = 0;
		switch (ctx->encoding)
		{
		case CCX_ENC_UTF_8:
			bytes = get_char_in_utf_8(buffer, line[i]);
			break;
		case CCX_ENC_LATIN_1:
			get_char_in_latin_1(buffer, line[i]);
			bytes = 1;
			break;
		case CCX_ENC_UNICODE:
			get_char_in_unicode(buffer, line[i]);
			bytes = 2;
		case CCX_ENC_ASCII:
		    *buffer = line[i];
			bytes = 1;
			break;
		}
		buffer += bytes;
	}
	return (unsigned int)(buffer - orig); // Return length
}
static unsigned int get_color_encoded(struct encoder_ctx *ctx, unsigned char *buffer, int line_num, struct eia608_screen *data)
{
	unsigned char *orig = buffer; // Keep for debugging
	for (int i = 0; i < 32; i++)
	{
		if (data->colors[line_num][i] < 10)
			*buffer++ = data->colors[line_num][i] + '0';
		else
			*buffer++ = 'E';
	}
	*buffer = 0;
	return (unsigned)(buffer - orig); // Return length
}
static unsigned int get_font_encoded(struct encoder_ctx *ctx, unsigned char *buffer, int line_num, struct eia608_screen *data)
{
	unsigned char *orig = buffer; // Keep for debugging
	for (int i = 0; i < 32; i++)
	{
		if(data->fonts[line_num][i] == FONT_REGULAR)
			*buffer++ = 'R';
		else if(data->fonts[line_num][i] == FONT_UNDERLINED_ITALICS)
			*buffer++ = 'B';
		else if(data->fonts[line_num][i] == FONT_UNDERLINED)
			*buffer++ = 'U';
		else if(data->fonts[line_num][i] == FONT_ITALICS)
			*buffer++ = 'I';
		else
			*buffer++ = 'E';
	}
	return (unsigned)(buffer - orig); // Return length
}
int write_cc_buffer_as_g608(struct eia608_screen *data, struct encoder_ctx *context)
{
	int used;
	unsigned h1,m1,s1,ms1;
	unsigned h2,m2,s2,ms2;
	LLONG ms_start, ms_end;
	int wrote_something = 0;
	ms_start = data->start_time;

	ms_start+=context->subs_delay;
	if (ms_start<0) // Drop screens that because of subs_delay start too early
		return 0;

	ms_end = data->end_time;

	mstotime (ms_start,&h1,&m1,&s1,&ms1);
	mstotime (ms_end-1,&h2,&m2,&s2,&ms2); // -1 To prevent overlapping with next line.
	char timeline[128];
	context->srt_counter++;
	sprintf(timeline, "%u%s", context->srt_counter, context->encoded_crlf);
	used = encode_line(context, context->buffer,(unsigned char *) timeline);
	write(context->out->fh, context->buffer, used);
	sprintf (timeline, "%02u:%02u:%02u,%03u --> %02u:%02u:%02u,%03u%s",
		h1, m1, s1, ms1, h2, m2, s2, ms2, context->encoded_crlf);
	used = encode_line(context, context->buffer,(unsigned char *) timeline);


	write (context->out->fh, context->buffer, used);

	for (int i=0;i<15;i++)
	{
		int length = get_line_encoded (context, context->subline, i, data);
		write(context->out->fh, context->subline, length);

		length = get_color_encoded (context, context->subline, i, data);
		write(context->out->fh, context->subline, length);

		length = get_font_encoded (context, context->subline, i, data);
		write(context->out->fh, context->subline, length);
		write(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
		wrote_something=1;
	}

	write (context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
	return wrote_something;
}
