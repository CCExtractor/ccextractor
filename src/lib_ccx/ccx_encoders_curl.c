#ifdef WITH_LIBCURL
#include "ccx_decoders_common.h"
#include "ccx_encoders_common.h"
#include "spupng_encoder.h"
#include "ccx_encoders_spupng.h"
#include "utility.h"
#include "ocr.h"
#include "ccx_decoders_608.h"
#include "ccx_decoders_708.h"
#include "ccx_decoders_708_output.h"
#include "ccx_encoders_xds.h"
#include "ccx_encoders_helpers.h"
#include "utf8proc.h"

int write_cc_bitmap_as_libcurl(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	int ret = 0;
#ifdef ENABLE_OCR
	struct cc_bitmap* rect;
	LLONG ms_start, ms_end;
	unsigned h1, m1, s1, ms1;
	unsigned h2, m2, s2, ms2;
	char timeline[128];
	int len = 0;
	int used;
	int i = 0;
	char *str;

	if (context->prev_start != -1 && (sub->flags & SUB_EOD_MARKER))
	{
		ms_start = context->prev_start;
		ms_end = sub->start_time;
	}
	else if (!(sub->flags & SUB_EOD_MARKER))
	{
		ms_start = sub->start_time;
		ms_end = sub->end_time;
	}
	else if (context->prev_start == -1 && (sub->flags & SUB_EOD_MARKER))
	{
		ms_start = 1;
		ms_end = sub->start_time;
	}

	if (sub->nb_data == 0)
		return 0;

	if (sub->flags & SUB_EOD_MARKER)
		context->prev_start = sub->start_time;

	str = paraof_ocrtext(sub, context->encoded_crlf, context->encoded_crlf_length);
	if (str)
	{
		if (context->prev_start != -1 || !(sub->flags & SUB_EOD_MARKER))
		{
			mstotime(ms_start, &h1, &m1, &s1, &ms1);
			mstotime(ms_end - 1, &h2, &m2, &s2, &ms2); // -1 To prevent overlapping with next line.
			context->srt_counter++;
			sprintf(timeline, "group_id=ccextractordev&start_time=%llu&end_time=%llu&lang=en", ms_start, ms_end);
			char *curlline = NULL;
			curlline = str_reallocncat(curlline, timeline);
			curlline = str_reallocncat(curlline, "&payload="); 
			curlline = str_reallocncat(curlline, str);
			mprint("%s", curlline);


		}
		freep(&str);
	}
	for (i = 0, rect = sub->data; i < sub->nb_data; i++, rect++)
	{
		freep(rect->data);
		freep(rect->data + 1);
	}
#endif
	sub->nb_data = 0;
	freep(&sub->data);
	return ret;

}
#endif