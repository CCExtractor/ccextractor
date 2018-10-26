#ifdef WITH_LIBCURL
#include "ccx_decoders_common.h"
#include "ccx_encoders_common.h"
#include "utility.h"
#include "ocr.h"
#include "ccx_decoders_608.h"
#include "ccx_decoders_708.h"
#include "ccx_decoders_708_output.h"
#include "ccx_encoders_xds.h"
#include "ccx_encoders_helpers.h"
#include "utf8proc.h"

extern  CURL *curl;
extern  CURLcode res;

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
			millis_to_time(ms_start, &h1, &m1, &s1, &ms1);
			millis_to_time(ms_end - 1, &h2, &m2, &s2, &ms2); // -1 To prevent overlapping with next line.
			context->srt_counter++;
			sprintf(timeline, "group_id=ccextractordev&start_time=%" PRIu64 "&end_time=%" PRIu64 "&lang=en", ms_start, ms_end);
			char *curlline = NULL;
			curlline = str_reallocncat(curlline, timeline);
			curlline = str_reallocncat(curlline, "&payload=");
			char *urlencoded=curl_easy_escape (curl, str, 0);
			curlline = str_reallocncat(curlline,urlencoded);
			curl_free (urlencoded);
			mprint("%s", curlline);

			char *result = malloc(strlen(ccx_options.curlposturl) + strlen("/frame/") + 1);
			strcpy(result, ccx_options.curlposturl);
			strcat(result, "/frame/");
			curl_easy_setopt(curl, CURLOPT_URL, result);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, curlline);
			free(result);

			res = curl_easy_perform(curl);
			/* Check for errors */
			if(res != CURLE_OK)
				mprint("curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));
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
