#include "lib_ccx.h"
#include "../ccextractor.h"
#include "ccx_encoders_common.h"
#include "ccx_encoders_helpers.h"

int write_cc_buffer_as_g608(struct eia608_screen *data, struct encoder_ctx *context)
{
	int used;
	unsigned h1,m1,s1,ms1;
	unsigned h2,m2,s2,ms2;
	int wrote_something = 0;

	millis_to_time (data->start_time,&h1,&m1,&s1,&ms1);
	millis_to_time (data->end_time - 1,&h2,&m2,&s2,&ms2); // -1 To prevent overlapping with next line.
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
