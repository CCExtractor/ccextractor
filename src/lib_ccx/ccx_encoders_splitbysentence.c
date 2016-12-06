#include "ccx_common_platform.h"
#include "ccx_encoders_common.h"
#include "lib_ccx.h"
#include "ocr.h"

#ifdef ENABLE_SHARING
#include "ccx_share.h"
#endif //ENABLE_SHARING

int sbs_is_pointer_on_sentence_breaker(char * start, char * current)
{
	char c = *current;
	char n = *(current + 1);
	char p = *(current - 1);

	if (0 == c) n = 0;
	if (current == start) p = 0;

	if (0 == c) return 1;

	if ('.' == c
		|| '!' == c
		|| '?' == c
	)
	{
		if ('.' == n
			|| '!' == n
			|| '?' == n
		)
		{
			return 0;
		}

		return 1;
	}

	return 0;
}

/**
 * Appends the function to the sentence buffer, and returns a list of full sentences (if there are any), or NULL
 *
 * @param  str       Partial (or full) sub to append.
 * @param  time_from Starting timestamp
 * @param  time_trim Ending timestamp
 * @param  context   Encoder context
 * @return           New <struct cc_subtitle *> subtitle, or NULL, if <str> doesn't contain the ending part of the sentence. If there are more than one sentence, the remaining sentences will be chained using <result->next> reference.
 */
struct cc_subtitle * sbs_append_string(unsigned char * str, LLONG time_from, LLONG time_trim, struct encoder_ctx * context)
{
	struct cc_subtitle * resub;
	struct cc_subtitle * tmpsub;

	unsigned char * sbs;

	unsigned char * bp_current;
	unsigned char * bp_last_break;

	int is_buf_initialized;
	int required_len;
	int new_capacity;

	LLONG alphanum_total;
	LLONG alphanum_cur;

	LLONG anychar_total;
	LLONG anychar_cur;

	LLONG duration;
	LLONG available_time;
	int use_alphanum_counters;

	if (! str)
		return NULL;

	sbs = context->sbs_buffer; // just a shortcut

	is_buf_initialized = (NULL == sbs || context->sbs_capacity == 0)
		? 0
		: 1;

	// ===============================
	// grow sentence buffer
	// ===============================
	required_len = (is_buf_initialized ? strlen(sbs) : 0) + strlen(str);

	if (required_len >= context->sbs_capacity)
	{
		new_capacity = context->sbs_capacity;
		if (! is_buf_initialized)
		{
			new_capacity = 16;
		}

		while (new_capacity < required_len)
		{
			if (new_capacity > 1048576 * 8) // more than 8 Mb in TEXT buffer. It is weird...
			{
				new_capacity += 1048576 * 8;
			}
			else
			{
				new_capacity *= 2;
			}
		}
		context->sbs_capacity = new_capacity;
		context->sbs_buffer = (unsigned char *)realloc(context->sbs_buffer, new_capacity * sizeof(unsigned char /*context->sbs_buffer[0]*/ ));
		if (!context->sbs_buffer)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory in sbs_append_string");
		sbs = context->sbs_buffer;
	}

	// ===============================
	// append to buffer
	// ===============================
	if (is_buf_initialized)
	{
		strcat(sbs, str);
	}
	else
	{
		context->sbs_time_from = time_from;
		context->sbs_time_trim = time_trim;
		strcpy(sbs, str);
	}

	resub = NULL;
	tmpsub = NULL;

	alphanum_total = 0;
	alphanum_cur = 0;

	anychar_total = 0;
	anychar_cur = 0;

	bp_last_break = sbs;
	for (bp_current = sbs; bp_current && *bp_current; bp_current++)
	{
		if (
			0 < anychar_cur	// skip empty!
			&& sbs_is_pointer_on_sentence_breaker(bp_last_break, bp_current) )
		{
			// it is new sentence!
			tmpsub = malloc(sizeof(struct cc_subtitle));

			tmpsub->type = CC_TEXT;
			// length of new string:
			tmpsub->nb_data =
				bp_current - bp_last_break
				+ 1	 // terminating '\0'
				+ 1  // skip '.'
			;
			tmpsub->data = strndup(bp_last_break, tmpsub->nb_data - 1);
			tmpsub->got_output = 1;

			tmpsub->start_time = alphanum_cur;
			alphanum_cur = 0;
			tmpsub->end_time = anychar_cur;
			anychar_cur = 0;

			bp_last_break = bp_current + 1;

			// tune last break:
			while (
				*bp_last_break
				&& isspace(*bp_last_break)
			)
			{
				bp_last_break++;
			}

			// ???
			// tmpsub->info = NULL;
			// tmpsub->mode = NULL;

			// link with prev sub:
			tmpsub->next = NULL;
			tmpsub->prev = resub;
			if (NULL != resub)
			{
				resub->next = tmpsub;
			}

			resub = tmpsub;
		}

		if (*bp_current && isalnum(*bp_current))
		{
			alphanum_total++;
			alphanum_cur++;
		}
		anychar_total++;
		anychar_cur++;
	}

	// remove processed data from buffer:
	if (bp_last_break != sbs)
	{
		strcpy(sbs, bp_last_break);
	}

	#ifdef DEBUG
	printf ("BUFFER:\n\t%s\nSTRING:\n\t%s\nCHARS:\n\tAlphanum Total: %d\n\tOverall chars: %d\n====================\n", sbs, str, alphanum_total, anychar_total);
	#endif

	available_time = time_trim - context->sbs_time_from;
	use_alphanum_counters = alphanum_total > 0 ? 1 : 0;

	// update time spans
	tmpsub = resub;
	while (tmpsub)
	{
		alphanum_cur = tmpsub->start_time;
		anychar_cur = tmpsub->end_time;

		if (use_alphanum_counters)
		{
			duration = available_time * alphanum_cur / alphanum_total;
		}
		else
		{
			duration = available_time * anychar_cur / anychar_total;
		}

		tmpsub->start_time = context->sbs_time_from;
		tmpsub->end_time = tmpsub->start_time + duration;

		context->sbs_time_from = tmpsub->end_time + 1;

		tmpsub = tmpsub->next;
	}

	return resub;
}

struct cc_subtitle * reformat_cc_bitmap_through_sentence_buffer(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	struct cc_bitmap* rect;
	LLONG ms_start, ms_end;
	int used;
	int i = 0;
	char *str;

	// this is a sub with a full sentence (or chain of such subs)
	struct cc_subtitle * resub = NULL;

#ifdef ENABLE_OCR

	if (sub->flags & SUB_EOD_MARKER)
	{
		// the last sub from input

		if (context->prev_start == -1)
		{
			ms_start = 1;
			ms_end = sub->start_time;
		}
		else
		{
			ms_start = context->prev_start;
			ms_end = sub->start_time;
		}
	}
	else
	{
		// not the last sub from input
		ms_start = sub->start_time;
		ms_end = sub->end_time;
	}

	if (sub->nb_data == 0)
		return 0;

	if (sub->flags & SUB_EOD_MARKER)
		context->prev_start = sub->start_time;

	str = paraof_ocrtext(sub, " ", 1);
	if (str)
	{
		if (context->prev_start != -1 || !(sub->flags & SUB_EOD_MARKER))
		{
			resub = sbs_append_string(str, ms_start, ms_end, context);
		}
		freep(&str);
	}

	for(i = 0, rect = sub->data; i < sub->nb_data; i++, rect++)
	{
		freep(rect->data);
		freep(rect->data+1);
	}
#endif
	sub->nb_data = 0;
	freep(&sub->data);
	return resub;

}
