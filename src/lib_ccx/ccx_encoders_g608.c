#include "lib_ccx.h"
#include "../ccextractor.h"
#include "ccx_encoders_common.h"
#include "ccx_encoders_helpers.h"

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

	millis_to_time (ms_start,&h1,&m1,&s1,&ms1);
	millis_to_time (ms_end-1,&h2,&m2,&s2,&ms2); // -1 To prevent overlapping with next line.
	char timeline[128];
	context->srt_counter++;
	sprintf(timeline, "%u%s", context->srt_counter, context->encoded_crlf);
	used = encode_line(context, context->buffer,(unsigned char *) timeline);
    if (!signal_python_api)
        write(context->out->fh, context->buffer, used);
	sprintf (timeline, "%02u:%02u:%02u,%03u --> %02u:%02u:%02u,%03u%s",
		h1, m1, s1, ms1, h2, m2, s2, ms2, context->encoded_crlf);
	used = encode_line(context, context->buffer,(unsigned char *) timeline);


    if (!signal_python_api)
        write (context->out->fh, context->buffer, used);
    if (signal_python_api)
        python_extract_g608_grid(h1,m1,s1,ms1,h2,m2,s2,ms2,context->buffer,0);
	for (int i=0;i<15;i++)
	{
		int length = get_line_encoded (context, context->subline, i, data);
        if (!signal_python_api)
            write(context->out->fh, context->subline, length);
        if (signal_python_api)
            python_extract_g608_grid(h1,m1,s1,ms1,h2,m2,s2,ms2,context->subline,1);

		length = get_color_encoded (context, context->subline, i, data);
        if (!signal_python_api)
            write(context->out->fh, context->subline, length);
        if (signal_python_api)
            python_extract_g608_grid(h1,m1,s1,ms1,h2,m2,s2,ms2,context->subline,2);    

		length = get_font_encoded (context, context->subline, i, data);
        if (!signal_python_api)
            write(context->out->fh, context->subline, length);
        if (signal_python_api)
            python_extract_g608_grid(h1,m1,s1,ms1,h2,m2,s2,ms2,context->subline,3);   
        if (!signal_python_api)
            write(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
		wrote_something=1;
	}

    if (!signal_python_api)
        write (context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
	return wrote_something;
}
