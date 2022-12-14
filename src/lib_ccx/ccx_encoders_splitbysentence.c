/**
 *
 * This is an implementation of Sentence Break Buffer
 * It is still in development and could contain bugs
 * If you will find bugs in SBS, you could try to create an
 * issue in the forked repository:
 *
 * https://github.com/maxkoryukov/ccextractor/issues
 * Only SBS-related bugs!
 *
 * IMPORTANT: SBS is color-blind, and color tags mislead SBS.
 * Please, use `-sbs` option with `-nofc` (actual for CCE 0.8.5)
 */

#include "ccx_common_platform.h"
#include "ccx_encoders_common.h"
#include "lib_ccx.h"
#include "ocr.h"
#include "utility.h"

// #define DEBUG_SBS
// #define ENABLE_OCR

#ifdef DEBUG_SBS
#define LOG_DEBUG(...) fprintf(stdout, __VA_ARGS__)
#else
#define LOG_DEBUG(...)
#endif

#ifdef ENABLE_SHARING
#include "ccx_share.h"
#endif // ENABLE_SHARING

//---------------------------
// BEGIN of #BUG639
// HACK: this is workaround for https://github.com/CCExtractor/ccextractor/issues/639
// short: the outside function, called encode_sub changes ENCODER_CONTEXT when it required..
// as result SBS loses it internal state...

// #BUG639
typedef struct
{

	unsigned char *buffer; /// Storage for sentence-split buffer
	size_t handled_len;    /// The length of the string in the SBS-buffer, already handled, but preserved for DUP-detection.

	// ccx_sbs_utf8_character *sbs_newblock;
	LLONG time_from; // Used by the split-by-sentence code to know when the current block starts...
	LLONG time_trim; // ... and ends
	size_t capacity;

} sbs_context_t;

// DO NOT USE IT DIRECTLY!
// The only one exceptions:
// 1. init_sbs_context
sbs_context_t *____sbs_context = NULL;

/**
 * Initializes SBS settings for encoder context
 */

void sbs_reset_context()
{
	if (NULL != ____sbs_context)
	{
		free(____sbs_context);
		____sbs_context = NULL;
	}
}

// void init_encoder_sbs(struct encoder_ctx * ctx, const int splitbysentence);
sbs_context_t *sbs_init_context()
{

	LOG_DEBUG("SBS: init_sbs_context\n\
____sbs_context:   [%p]\n\
",
		  ____sbs_context);

	if (NULL == ____sbs_context)
	{

		LOG_DEBUG("SBS: init_sbs_context: INIT\n");

		____sbs_context = malloc(sizeof(sbs_context_t));
		____sbs_context->time_from = -1;
		____sbs_context->time_trim = -1;
		____sbs_context->capacity = 16;
		____sbs_context->buffer = malloc(____sbs_context->capacity * sizeof(unsigned char));
		____sbs_context->buffer[0] = 0;
		____sbs_context->handled_len = 0;
	}

	LOG_DEBUG("SBS: init_sbs_context: DONE\n\
____sbs_context:   [%p]\n\
",
		  ____sbs_context);

	return ____sbs_context;
}

// END of #BUG639
//---------------------------

void sbs_str_autofix(unsigned char *str)
{
	LOG_DEBUG("SBS: sbs_str_autofix\n\
\t old str:  [%s]\n\
",
		  str);

	int i;
	int j;

	i = 0;
	j = 0;

	// replace all whitespaces with spaces:
	while (str[i] != 0)
	{

		if (isspace(str[i]))
		{
			// \n
			// \t
			// \r
			// <WHITESPACES>
			// =>
			// <SPACE>
			while (isspace(str[i]))
			{
				i++;
			}

			if (j > 0)
			{
				str[j] = ' ';
				j++;
			}
		}
		else if (
		    str[i] == '|' && (i == 0 || isspace(str[i - 1])) && (str[i + 1] == 0 || isspace(str[i + 1]) || str[i + 1] == '\''))
		{
			// <SPACE>|'
			// <SPACE>|<SPACE>
			// =>
			// <SPACE>I'
			str[j] = 'I';
			i++;
			j++;
		}
		else
		{
			str[j] = str[i];
			i++;
			j++;
		}
	}

	str[j] = 0;

	LOG_DEBUG("SBS: sbs_str_autofix\n\
\t old str:  [%s]\n\
",
		  str);
}

int sbs_is_pointer_on_sentence_breaker(char *start, char *current)
{
	char c = *current;
	char n = *(current + 1);

	if (0 == c)
		return 1;

	if ('.' == c || '!' == c || '?' == c)
	{
		if ('.' == n || '!' == n || '?' == n)
		{
			return 0;
		}

		return 1;
	}

	return 0;
}

int sbs_char_equal_CI(const char A, const char B)
{
	char a = tolower(A);
	char b = tolower(B);
	if (a > b)
		return 1;
	if (a < b)
		return -1;
	return 0;
}

char *sbs_find_insert_point_partial(char *old_tail, const char *new_start, size_t n, const int maxerr, int *errcount)
{

	const int PARTIAL_CHANGE_LENGTH_MIN = 7;
	const int FEW_ERRORS_RATE = 10;

#ifdef DEBUG_SBS
	char fmtbuf[20000];
#endif

	int few_errors = maxerr / FEW_ERRORS_RATE;

	size_t len_r = n / 2;
	size_t len_l = n - len_r;

	int dist_l = -1;
	int dist_r = -1;
	int partial_shift;

	dist_l = levenshtein_dist_char(old_tail, new_start, len_l, len_l);
	dist_r = levenshtein_dist_char(old_tail + len_l, new_start + len_l, len_r, len_r);

	*errcount = dist_r + dist_l;

	if (dist_l + dist_r > maxerr)
	{
		/*
#ifdef DEBUG_SBS
		sprintf(fmtbuf, "SBS: sbs_find_insert_point_partial: compare\n\
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
		*/
		return NULL;
	};

	if (
	    dist_r <= few_errors	     // right part almost the same
	    && n > PARTIAL_CHANGE_LENGTH_MIN // the sentence is long enough for analysis
	)
	{
		/*
		LOG_DEBUG("SBS: sbs_find_insert_point_partial: LEFT CHANGED,\n\tbuf:[%s]\n\tstr:[%s]\n\
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
		*/

		// searching for first mismatched symbol at the end of buf
		// This is a naive implementation of error detection
		//
		// Will travel from the end of string to the beginning, and
		// compare OLD value (from buffer) and new value (new STR)
		partial_shift = 0;
		while (
		    old_tail[partial_shift] != 0 && old_tail[partial_shift + 1] != 0 && (0 != sbs_char_equal_CI(old_tail[partial_shift], new_start[partial_shift])))
		{
			partial_shift += 1;
		}

		// LOG_DEBUG("SBS: sbs_find_insert_point_partial: PARTIAL SHIFT, [%d]\n",
		// 	partial_shift
		// );

		return old_tail + partial_shift;
	}

	/*
#ifdef DEBUG_SBS
		sprintf(fmtbuf, "SBS: sbs_find_insert_point_partial: REPLACE ENTIRE TAIL !!\n\
\tmaxerr:          [%%d]\n\
\tL buffer:        [%%.%zus]\n\
\tL string:        [%%.%zus]\n\
\tL dist_l:        [%%d]\n\
\tR buffer:        [%%.%zus]\n\
\tR string:        [%%.%zus]\n\
\tR dist_r:        [%%d]\n\
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
	*/

	return old_tail;
}

char *sbs_find_insert_point(char *buf, const char *str, int *ilen)
{

	int maxerr;
	size_t buf_len;
	unsigned char *buffer_tail;
	const unsigned char *prefix = str;

	int cur_err = 0;
	unsigned char *cur_ptr;
	int cur_len;

	int best_err;
	unsigned char *best_ptr;
	int best_len;

	buf_len = strlen(buf);
	cur_len = strlen(str);

	if (buf_len < cur_len)
		cur_len = buf_len;

	// init errcounter with value, greater than possible amount of errors in string
	best_err = cur_len + 1;
	best_ptr = NULL;
	best_len = 0;

	while (cur_len > 0)
	{
		maxerr = cur_len / 5;
		buffer_tail = buf + buf_len - cur_len;

		cur_ptr = sbs_find_insert_point_partial(buffer_tail, prefix, cur_len, maxerr, &cur_err);
		if (NULL != cur_ptr)
		{
			if ((cur_len - cur_err) >= (best_len - best_err))
			{
				best_err = cur_err;
				best_len = cur_len;
				best_ptr = cur_ptr;
			}
		}
		cur_len--;
	}

	*ilen = best_len;
	return best_ptr;
}

void sbs_strcpy_without_dup(const unsigned char *str, sbs_context_t *context)
{
	int intersect_len;
	unsigned char *buffer_insert_point;

	unsigned long sbs_len;

	sbs_len = strlen(context->buffer);

	LOG_DEBUG("SBS: sbs_strcpy_without_dup: going to append, looking for common part\n\
\tbuffer:          [%p][%s]\n\
\tstring:          [%s]\n\
",
		  context->buffer,
		  context->buffer,
		  str);

	buffer_insert_point = sbs_find_insert_point(context->buffer, str, &intersect_len);

	LOG_DEBUG("SBS: sbs_strcpy_without_dup: analyze search results\n\
\t buffer:         [%s]\n\
\t string:         [%s]\n\
\t insert point: ->[%s]\n\
\t intersection len[%4d]\n\
\t sbslen          [%4ld]\n\
\t handled len     [%4zu]\n\
",
		  context->buffer,
		  str,
		  buffer_insert_point,
		  intersect_len,
		  sbs_len,
		  context->handled_len);

	if (intersect_len > 0)
	{
		// there is a common part (suffix of old sentence equals to prefix of new str)
		//
		// remove dup from buffer
		// we will use an appropriate part from the new string

		// context->buffer[sbs_len-intersect_len] = 0;

		LOG_DEBUG("SBS: sbs_strcpy_without_dup: cut buffer by insert point\n");
		*buffer_insert_point = 0;
	}

	// check, that new string does not contain data, from
	// already handled sentence:
	if ((sbs_len - intersect_len) >= context->handled_len)
	{
		// there is no intersection.
		// It is time to clean the buffer. Excepting the last uncomplete sentence
		LOG_DEBUG("SBS: sbs_strcpy_without_dup: DROP parsed part\n\
\t buffer:         [%s]\n\
\t handled len     [%4zu]\n\
\t new start at  ->[%s]\n\
",
			  context->buffer,
			  context->handled_len,
			  context->buffer + context->handled_len);

		// skip leading whitespaces:
		int skip_ws = context->handled_len;
		while (isspace(context->buffer[skip_ws]))
		{
			skip_ws++;
		}

		strcpy(context->buffer, context->buffer + skip_ws);
		context->handled_len = 0;
	}

	sbs_len = strlen(context->buffer);

	if (
	    !isspace(str[0]) // not a space char in the beginning of new str
	    //&& context->handled_len >0  // buffer is not empty (there is uncomplete sentence)
	    && sbs_len > 0			      // buffer is not empty (there is uncomplete sentence)
	    && !isspace(context->buffer[sbs_len - 1]) // not a space char at the end of existing buf
	)
	{
		strcat(context->buffer, " ");
	}

	strcat(context->buffer, str);
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
struct cc_subtitle *sbs_append_string(unsigned char *str, const LLONG time_from, const LLONG time_trim, sbs_context_t *context)
{
	struct cc_subtitle *resub;
	struct cc_subtitle *tmpsub;

	unsigned char *bp_current;
	unsigned char *bp_last_break;
	unsigned char *sbs_undone_start;

	int required_capacity;
	int new_capacity;

	LLONG alphanum_total;
	LLONG alphanum_cur;

	LLONG anychar_total;
	LLONG anychar_cur;

	LLONG duration;
	LLONG available_time;
	int use_alphanum_counters;

	if (!str)
		return NULL;

	LOG_DEBUG("SBS: sbs_append_string: after sbs init:\n\
\tsbs ptr:   [%p]\n\
",
		  context);

	LOG_DEBUG("SBS: sbs_append_string: after sbs init:\n\
\tsbs ptr:   [%p][%s]\n\
\tcur cap:   [%zu]\n\
",
		  context->buffer,
		  context->buffer,
		  context->capacity);

	sbs_str_autofix(str);

	// ===============================
	// grow sentence buffer
	// ===============================
	required_capacity =
	    strlen(context->buffer) // existing data in buf
	    + strlen(str)	    // length of new string
	    + 1			    // trailing \0
	    + 1			    // space control (will add one space , if required)
	    ;

	if (required_capacity >= context->capacity)
	{
		LOG_DEBUG("SBS: sbs_append_string: REALLOC BUF:\n\
\tsbs ptr:   [%p]\n\
\tcur cap:   [%zu]\n\
\treq cap:   [%d]\n\
",
			  context->buffer,
			  context->capacity,
			  required_capacity);

		new_capacity = context->capacity;

		while (new_capacity < required_capacity)
		{
			// increase NEW_capacity, and check, that increment
			// is less than 8 Mb. Because 8Mb - it is a lot
			// for a TEXT buffer. It is weird...
			new_capacity += (new_capacity > 1048576 * 8)
					    ? 1048576 * 8
					    : new_capacity;
		}

		context->buffer = (unsigned char *)realloc(
		    context->buffer,
		    new_capacity * sizeof(/*unsigned char*/ context->buffer[0]));

		if (!context->buffer)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In sbs_append_string: Not enough memory to append buffer");

		context->capacity = new_capacity;

		LOG_DEBUG("SBS: sbs_append_string: REALLOC BUF DONE:\n\
\tsbs ptr:   {%p}\n\
\tcur cap:   [%zu]\n\
\treq cap:   [%d]\n\
\tbuf:       [%s]\n\
",
			  context->buffer,
			  context->capacity,
			  required_capacity,
			  context->buffer);
	}

	// ===============================
	// append to buffer
	//
	// will update buffer, handled_len
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

	sbs_undone_start = context->buffer + context->handled_len;
	bp_last_break = sbs_undone_start;

	LOG_DEBUG("SBS: BEFORE sentence break.\n\
\tLast break: [%s]\n\
\tsbs_undone_start: [%zu]\n\
\tsbs_undone: [%s]\n\
",
		  bp_last_break,
		  context->handled_len,
		  sbs_undone_start);

	for (bp_current = sbs_undone_start; bp_current && *bp_current; bp_current++)
	{
		if (
		    0 < anychar_cur // skip empty!
		    && sbs_is_pointer_on_sentence_breaker(bp_last_break, bp_current))
		{
			// it is new sentence!
			tmpsub = malloc(sizeof(struct cc_subtitle));

			tmpsub->type = CC_TEXT;
			// length of new string:
			tmpsub->nb_data =
			    bp_current - bp_last_break + 1 // terminating '\0'
			    + 1				   // skip '.'
			    ;
			tmpsub->data = strndup(bp_last_break, tmpsub->nb_data - 1);
			tmpsub->datatype = CC_DATATYPE_GENERIC;
			tmpsub->got_output = 1;

			tmpsub->start_time = alphanum_cur;
			alphanum_cur = 0;
			tmpsub->end_time = anychar_cur;
			anychar_cur = 0;

			bp_last_break = bp_current + 1;

			// tune last break:
			while (
			    *bp_last_break && isspace(*bp_last_break))
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
	// incomplete sentence
	// ===============================
	if (bp_last_break != sbs_undone_start)
	{
		context->handled_len = bp_last_break - sbs_undone_start;
	}

	LOG_DEBUG("SBS: AFTER sentence break:\
\n\tHandled Len    [%4zu]\
\n\tAlphanum Total [%4ld]\
\n\tOverall chars  [%4ld]\
\n\tSTRING:[%s]\
\n\tBUFFER:[%s]\
\n",
		  context->handled_len,
		  alphanum_total,
		  anychar_total,
		  str,
		  context->buffer);

	// ===============================
	// Calculate time spans
	// ===============================
	if ((0 > context->time_from) && (0 > context->time_trim))
	{
		context->time_from = time_from;
		context->time_trim = time_trim;
	}

	available_time = time_trim - context->time_from;
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

		tmpsub->start_time = context->time_from;
		tmpsub->end_time = tmpsub->start_time + duration;

		context->time_from = tmpsub->end_time + 1;

		tmpsub = tmpsub->next;
	}
	return resub;
}

struct cc_subtitle *reformat_cc_bitmap_through_sentence_buffer(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	// this is a sub with a full sentence (or chain of such subs)
	struct cc_subtitle *resub = NULL;

#ifdef ENABLE_OCR
	struct cc_bitmap *rect;
	LLONG ms_start, ms_end;
	int i = 0;
	char *str;

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

	str = paraof_ocrtext(sub, context);

	if (str)
	{
		LOG_DEBUG("SBS: reformat_cc_bitmap: string is not empty\n");

		if (context->prev_start != -1 || !(sub->flags & SUB_EOD_MARKER))
		{
			resub = sbs_append_string(str, ms_start, ms_end, sbs_init_context());
		}
		freep(&str);
	}

	for (i = 0, rect = sub->data; i < sub->nb_data; i++, rect++)
	{
		freep(&rect->data0);
		freep(&rect->data1);
	}
#endif
	sub->nb_data = 0;
	freep(&sub->data);

	return resub;
}
