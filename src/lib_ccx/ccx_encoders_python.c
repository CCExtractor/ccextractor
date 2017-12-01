#include "lib_ccx.h"
#include "../ccextractor.h"
#include "ccx_encoders_common.h"
#include "ccx_encoders_helpers.h"

#ifdef ENABLE_PYTHON

int pass_cc_buffer_to_python(struct eia608_screen *data, struct encoder_ctx *context)
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
	sprintf (timeline, "%02u:%02u:%02u,%03u --> %02u:%02u:%02u,%03u%s",
		h1, m1, s1, ms1, h2, m2, s2, ms2, context->encoded_crlf);
	used = encode_line(context, context->buffer,(unsigned char *) timeline);


	python_extract_g608_grid(h1,m1,s1,ms1,h2,m2,s2,ms2,context->buffer,0,context->srt_counter,context->encoding);
	for (int i=0;i<15;i++)
	{
		int length = get_line_encoded (context, context->subline, i, data);
		python_extract_g608_grid(h1,m1,s1,ms1,h2,m2,s2,ms2,context->subline,1,context->srt_counter, context->encoding);

		length = get_color_encoded (context, context->subline, i, data);
		python_extract_g608_grid(h1,m1,s1,ms1,h2,m2,s2,ms2,context->subline,2,context->srt_counter, context->encoding);    

		length = get_font_encoded (context, context->subline, i, data);
		python_extract_g608_grid(h1,m1,s1,ms1,h2,m2,s2,ms2,context->subline,3,context->srt_counter, context->encoding);   
		wrote_something=1;
	}
	python_extract_g608_grid(h1,m1,s1,ms1,h2,m2,s2,ms2,context->subline,4,context->srt_counter, context->encoding);   
	return wrote_something;
}

#endif
