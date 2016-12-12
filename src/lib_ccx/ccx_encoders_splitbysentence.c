#include "ccx_common_platform.h"
#include "ccx_encoders_common.h"
#include "lib_ccx.h"
#include "ocr.h"
#include "debug_def.h"

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

int sbs_fuzzy_strncmp(const char * a, const char * b, size_t n, const size_t maxerr)
{
	// TODO: implement fuzzy comparing
	// Error counter DOES NOT WORK!!!

	int i;
	//int err;
	char A, B;

	i = -1;
	do
	{
		i++;

		// Bound check (compare to N)
		if (i == n) return 0;

		A = a[i];
		B = b[i];

		// bound check (line endings)
		if (A == 0)
		{
			if (B == 0) return 0;
			return 1;
		}
		else
		{
			if (B == 0) return -1;
		}

		if (A == B) continue;
		if (isspace(A) && isspace(B)) continue;

		if (A > B) return 1;
		return -1;

	} while(1);
}

void sbs_strcpy_without_dup(const unsigned char * str, struct encoder_ctx * context)
{
	int intersect_len;
	unsigned char * suffix;
	const unsigned char * prefix = str;

	unsigned long sbs_len;
	unsigned long str_len;

	str_len = strlen(str);
	sbs_len = strlen(context->sbs_buffer);

	intersect_len = str_len;
	if (sbs_len < intersect_len)
		intersect_len = sbs_len;

	while (intersect_len>0)
	{
		suffix = context->sbs_buffer + sbs_len - intersect_len;
		if (0 == sbs_fuzzy_strncmp(prefix, suffix, intersect_len, 1))
		{
			break;
		}
		intersect_len--;
	}

	LOG_DEBUG("Sentence Buffer: sbs_strcpy_without_dup, intersection len [%4d]\n", intersect_len);

	// check, that new string does not contain data, from
	// already handled sentence:
	LOG_DEBUG("Sentence Buffer: sbs_strcpy_without_dup, sbslen [%4d] handled len [%4d]\n", sbs_len, context->sbs_handled_len);
	if ( (sbs_len - intersect_len) >= context->sbs_handled_len)
	{
		// there is no intersection.
		// It is time to clean the buffer. Excepting the last uncomplete sentence
		strcpy(context->sbs_buffer, context->sbs_buffer + context->sbs_handled_len);
		context->sbs_handled_len = 0;
		sbs_len = strlen(context->sbs_buffer);

		LOG_DEBUG("Sentence Buffer: Clean buffer, after BUF [%s]\n\n\n", context->sbs_buffer);
	}

	if (intersect_len > 0)
	{
		// there is a common part (suffix of old sentence equals to prefix of new str)
		//
		// remove dup from buffer
		// we will use an appropriate part from the new string
		context->sbs_buffer[sbs_len-intersect_len] = 0;
	}

	sbs_len = strlen(context->sbs_buffer);

	// whitespace control. Add space between subs
	if (
		!isspace(str[0])                // not a space char in the beginning of new str
		&& context->sbs_handled_len >0  // buffer is not empty (there is uncomplete sentence)
		&& !isspace(context->sbs_buffer[sbs_len-1])  // not a space char at the end of existing buf
	)
	{
		//strcat(context->sbs_buffer, " ");
	}

	strcat(context->sbs_buffer, str);
}

void sbs_str_autofix(unsigned char * str)
{
	int i;

	// replace all whitespaces with spaces:
	for (i = 0; str[i] != 0; i++)
	{
		if (isspace(str[i]))
		{
			str[i] = ' ';
		}

		if (
			str[i] == '|'
			&& (i==0 || isspace(str[i-1]))
			&& (str[i+1] == 0 || isspace(str[i+1]) || str[i+1]=='\'')
		)
		{
			// try to convert to "I"
			str[i] = 'I';
		}
	}

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
struct cc_subtitle * sbs_append_string(unsigned char * str, const LLONG time_from, const LLONG time_trim, struct encoder_ctx * context)
{
	struct cc_subtitle * resub;
	struct cc_subtitle * tmpsub;

	unsigned char * bp_current;
	unsigned char * bp_last_break;
	unsigned char * sbs_undone_start;

	int is_buf_initialized;
	int required_capacity;
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

	sbs_str_autofix(str);

	is_buf_initialized = (NULL == context->sbs_buffer || context->sbs_capacity == 0)
		? 0
		: 1;

	// ===============================
	// grow sentence buffer
	// ===============================
	required_capacity =
		(is_buf_initialized ? strlen(context->sbs_buffer) : 0)    // existing data in buf
		+ strlen(str)     // length of new string
		+ 1               // trailing \0
		+ 1               // space control (will add one space , if required)
	;

	if (required_capacity >= context->sbs_capacity)
	{
		new_capacity = context->sbs_capacity;
		if (! is_buf_initialized) new_capacity = 16;

		while (new_capacity < required_capacity)
		{
			// increase NEW_capacity, and check, that increment
			// is less than 8 Mb. Because 8Mb - it is a lot
			// for a TEXT buffer. It is weird...
			new_capacity += (new_capacity > 1048576 * 8)
				? 1048576 * 8
				: new_capacity;
		}

		context->sbs_buffer = (unsigned char *)realloc(
			context->sbs_buffer,
			new_capacity * sizeof(/*unsigned char*/ context->sbs_buffer[0] )
		);

		if (!context->sbs_buffer)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory in sbs_append_string");

		context->sbs_capacity = new_capacity;

		// if buffer wasn't initialized, we will se trash in buffer.
		// but we need just empty string, so here we will get it:
		if (! is_buf_initialized)
		{
			// INIT SBS
			context->sbs_buffer[0] = 0;
			context->sbs_handled_len = 0;
		}

	}

	// ===============================
	// append to buffer
	//
	// will update sbs_buffer, sbs_handled_len
	// ===============================
	sbs_strcpy_without_dup(str, context);

	// ===============================
	// break to sentences
	// ===============================
	resub = NULL;
	tmpsub = NULL;

	alphanum_total = 0;
	alphanum_cur = 0;

	anychar_total = 0;
	anychar_cur = 0;

	sbs_undone_start = context->sbs_buffer + context->sbs_handled_len;
	bp_last_break = sbs_undone_start;

	LOG_DEBUG("Sentence Buffer: BEFORE sentence break. Last break: [%s]  sbs_undone_start: [%d], sbs_undone: [%s]\n",
		bp_last_break, context->sbs_handled_len, sbs_undone_start
	);

	for (bp_current = sbs_undone_start; bp_current && *bp_current; bp_current++)
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

	// ===============================
	// okay, we have extracted several sentences, now we should
	// save the position of the "remainder" - start of the last
	// incomplete sentece
	// ===============================
	if (bp_last_break != sbs_undone_start)
	{
		context->sbs_handled_len = bp_last_break - sbs_undone_start;
	}

	LOG_DEBUG("Sentence Buffer: AFTER sentence break: Handled Len [%4d]\n", context->sbs_handled_len);

	LOG_DEBUG("Sentence Buffer: Alphanum Total: [%4d]  Overall chars: [%4d]  STRING:[%20s]  BUFFER:[%20s]\n", alphanum_total, anychar_total, str, context->sbs_buffer);

	// ===============================
	// Calculate time spans
	// ===============================
	if (!is_buf_initialized)
	{
		context->sbs_time_from = time_from;
		context->sbs_time_trim = time_trim;
	}

	available_time = time_trim - context->sbs_time_from;
	use_alphanum_counters = alphanum_total > 0 ? 1 : 0;

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
