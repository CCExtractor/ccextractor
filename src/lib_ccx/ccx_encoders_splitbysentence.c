/*

  This is an implementation of Sentence Break Buffer
  It is still in development and could contain bugs
  If you will find bugs in SBS, you could try to create an
  issue in the forked repository:

  https://github.com/maxkoryukov/ccextractor/issues

  Only SBS-related bugs!

 */


#include "ccx_common_platform.h"
#include "ccx_encoders_common.h"
#include "lib_ccx.h"
#include "ocr.h"
#include "utility.h"

#define DEBUG_SBS 1

#ifdef DEBUG_SBS
#define LOG_DEBUG(...) printf(__VA_ARGS__)
#else
#define LOG_DEBUG ;
#endif

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

int sbs_char_equal_CI(const char A, const char B) {
	char a = tolower(A);
	char b = tolower(B);
	if (a > b) return 1;
	if (a < b) return -1;
	return 0;
}

char * sbs_find_insert_point(char * old_tail, const char * new_start, size_t n, const int maxerr)
{

	const int PARTIAL_CHANGE_LENGTH_MIN = 7;
	const int FEW_ERRORS_RATE = 10;

#ifdef DEBUG_SBS
	char fmtbuf[20000];
#endif

	int few_errors = maxerr/FEW_ERRORS_RATE;

	size_t len_r = n/2;
	size_t len_l = n - len_r;

	int dist_l = -1;
	int dist_r = -1;
	int partial_shift;

	int i; // top level indexer for strings

	dist_l = levenshtein_dist_char(old_tail, new_start, len_l, len_l);
	dist_r = levenshtein_dist_char(old_tail + len_l, new_start + len_l, len_r, len_r);

	if (dist_l + dist_r > maxerr) {
#ifdef DEBUG_SBS
		sprintf(fmtbuf, "SBS: sbs_find_insert_point: compare\n\
\tnot EQ:          [TRUE]\n\
\tmaxerr:          [%%d]\n\
\tL buffer:          [%%.%zus]\n\
\tL string:          [%%.%zus]\n\
\tL dist_l:          [%%d]\n\
\tR buffer:          [%%.%zus]\n\
\tR string:          [%%.%zus]\n\
\tR dist_r:          [%%d]\n\
",
			len_l,
			len_l,
			len_r,
			len_r
		);
		LOG_DEBUG(fmtbuf,
			maxerr,
			old_tail,
			new_start,
			dist_l,
			old_tail + len_l,
			new_start + len_l,
			dist_r
		);
#endif
		return NULL;
	};

	if (
		dist_r <= few_errors               // right part almost the same
		&& n > PARTIAL_CHANGE_LENGTH_MIN   // the sentense is long enough for analyzis
	) {
		LOG_DEBUG("SBS: sbs_find_insert_point: LEFT CHANGED,\n\tbuf:[%s]\n\tstr:[%s]\n\
\tmaxerr:[%d]\n\
\tdist_l:[%d]\n\
\tdist_r:[%d]\n\
",
			old_tail,
			new_start,
			maxerr,
			dist_l,
			dist_r
		);
		// searching for first mismatched symbol at the end of buf
		// This is a naive implementation of error detection
		//
		// Will travel from the end of string to the beginning, and
		// compare OLD value (from buffer) and new value (new STR)
		partial_shift = 0;
		while (
			old_tail[partial_shift] != 0
			&& old_tail[partial_shift+1] != 0
			&& (0 != sbs_char_equal_CI(old_tail[partial_shift], new_start[partial_shift]))
		) {
			partial_shift += 1;
		}

printf("SBS: sbs_find_insert_point: PARTIAL SHIFT, [%d]\n",
			partial_shift
		);

		return old_tail + partial_shift;
	}

	// SHIFT test.
	// Levenshtein checks for DELETE symbol
	// And we are moving string samples one along the other
	// So, when we have 2 equal strings, the Levenshtein will told us
	// that there is tiny difference (2 chars)
	// BUT!! It is a gready. On the next step of this algorithm we will get
	// 0 difference. Lets test for such case
	//
	// TODO: implement NON GREADY algorithm, instead of this test

	int non_shift_error = 0;
	for (i = 1; i < n; i++) {

		if (old_tail[i] == 0) break;

		if (old_tail[i] != new_start[i-1]) {
			non_shift_error += 1;
		}
	}


#ifdef DEBUG_SBS

		sprintf(fmtbuf, "SBS: sbs_find_insert_point: REPLACE ENTIRE TAIL !![%%d]\n\
\tmaxerr:          [%%d]\n\
\tL buffer:        [%%.%zus]\n\
\tL string:        [%%.%zus]\n\
\tL dist_l:        [%%d]\n\
\tR buffer:        [%%.%zus]\n\
\tR string:        [%%.%zus]\n\
\tR dist_r:        [%%d]\n\
\tnon shift err:   [%%d]\n\
",
			len_l,
			len_l,
			len_r,
			len_r
		);
		LOG_DEBUG(fmtbuf,
			non_shift_error,
			maxerr,
			old_tail,
			new_start,
			dist_l,
			old_tail + len_l,
			new_start + len_l,
			dist_r,
			non_shift_error
		);
#endif

	if (non_shift_error <= dist_l + dist_r) {
		return NULL;
	}

	return old_tail;
}

void sbs_strcpy_without_dup(const unsigned char * str, struct encoder_ctx * context)
{
	int intersect_len;
	int maxerr;
	unsigned char * buffer_tail;
	const unsigned char * prefix = str;
	unsigned char * buffer_insert_point;

	unsigned long sbs_len;
	unsigned long str_len;

	str_len = strlen(str);
	sbs_len = strlen(context->sbs_buffer);

	intersect_len = str_len;
	if (sbs_len < intersect_len)
		intersect_len = sbs_len;

	LOG_DEBUG("SBS: sbs_strcpy_without_dup: going to append, looking for common part\n\
\tbuffer:          [%s]\n\
\tstring:          [%s]\n\
",
		context->sbs_buffer,
		str
	);

	while (intersect_len > 0)
	{
		maxerr = intersect_len / 5;
		buffer_tail = context->sbs_buffer + sbs_len - intersect_len;

		buffer_insert_point = sbs_find_insert_point(buffer_tail, prefix, intersect_len, maxerr);
		if (NULL != buffer_insert_point)
		{
			break;
		}
		intersect_len--;
	}

	LOG_DEBUG("SBS: sbs_strcpy_without_dup: analyze search results\n\
\tbuffer:          [%s]\n\
\tstring:          [%s]\n\
\tintersection len [%4d]\n\
\tsbslen           [%4ld]\n\
\thandled len      [%4zu]\n\
",
		context->sbs_buffer,
		str,
		intersect_len,
		sbs_len,
		context->sbs_handled_len
	);

	if (intersect_len > 0)
	{
		// there is a common part (suffix of old sentence equals to prefix of new str)
		//
		// remove dup from buffer
		// we will use an appropriate part from the new string

		//context->sbs_buffer[sbs_len-intersect_len] = 0;
printf("DROP THE END\n\
\tend is here: [%s]\n\
\tminus 1 is : [%s]\n\
",
		buffer_insert_point,
		buffer_insert_point - 1
);

		*buffer_insert_point = 0;


printf("AFTER DROP:\n\
\tcontext buf : [%s]\n\
",
		context->sbs_buffer
);
	}

	// check, that new string does not contain data, from
	// already handled sentence:
	if ( (sbs_len - intersect_len) >= context->sbs_handled_len)
	{
		// there is no intersection.
		// It is time to clean the buffer. Excepting the last uncomplete sentence
		strcpy(context->sbs_buffer, context->sbs_buffer + context->sbs_handled_len);
		context->sbs_handled_len = 0;
		sbs_len = strlen(context->sbs_buffer);
	}

	sbs_len = strlen(context->sbs_buffer);

	if (
		!isspace(str[0])                // not a space char in the beginning of new str
		//&& context->sbs_handled_len >0  // buffer is not empty (there is uncomplete sentence)
		&& sbs_len > 0  // buffer is not empty (there is uncomplete sentence)
		&& !isspace(context->sbs_buffer[sbs_len-1])  // not a space char at the end of existing buf
	)
	{
		strcat(context->sbs_buffer, " ");
	}

	strcat(context->sbs_buffer, str);
}

void sbs_str_autofix(unsigned char * str)
{
	int i;

	// replace all whitespaces with spaces:
	for (i = 0; str[i] != 0; i++)
	{
		// \n
		// \t
		// \r
		// <WHITESPACES>
		// =>
		// <SPACE>
		if (isspace(str[i]))
		{
			str[i] = ' ';
		}

		// <SPACE>|'
		// <SPACE>|<SPACE>
		// =>
		// <SPACE>I'
		if (
			str[i] == '|'
			&& (i==0 || isspace(str[i-1]))
			&& (str[i+1] == 0 || isspace(str[i+1]) || str[i+1]=='\'')
		)
		{
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

	LOG_DEBUG("SBS: BEFORE sentence break.\n\tLast break: [%s]\n\tsbs_undone_start: [%zu]\n\tsbs_undone: [%s]\n",
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

	LOG_DEBUG("SBS: AFTER sentence break:\
\n\tHandled Len    [%4zu]\
\n\tAlphanum Total [%4ld]\
\n\tOverall chars  [%4ld]\
\n\tSTRING:[%20s]\
\n\tBUFFER:[%20s]\
\n",
		context->sbs_handled_len,
		alphanum_total,
		anychar_total,
		str,
		context->sbs_buffer
	);

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
