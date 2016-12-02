#include "ccx_decoders_common.h"
#include "ccx_encoders_common.h"
#include "spupng_encoder.h"
#include "ccx_encoders_spupng.h"
#include "utility.h"
#include "ocr.h"
//#include "ccx_decoders_608.h"
//#include "ccx_decoders_708.h"
//#include "ccx_decoders_708_output.h"
//#include "ccx_encoders_xds.h"
//#include "ccx_encoders_helpers.h"
//#include "utf8proc.h"

#ifdef ENABLE_SHARING
#include "ccx_share.h"
#endif //ENABLE_SHARING

void lbl_start_block(LLONG start_time, struct encoder_ctx *context)
{
	context->sbs_newblock_start_time = start_time;
}

void lbl_add_character(struct encoder_ctx *context, ccx_sbs_utf8_character ch)
{
	if (context->sbs_newblock_capacity == context->sbs_newblock_size)
	{
		int newcapacity = (context->sbs_newblock_capacity < 512) ? 1024 : context->sbs_newblock_capacity * 2;
		context->sbs_newblock = (ccx_sbs_utf8_character *)realloc(context->sbs_newblock, newcapacity*sizeof(ccx_sbs_utf8_character));		
		if (!context->sbs_newblock)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory in lbl_add_character");
		context->sbs_newblock_capacity = newcapacity;
	}
	memcpy(&context->sbs_newblock[context->sbs_newblock_size++], &ch, sizeof ch);
}

void lbl_end_block(LLONG end_time, struct encoder_ctx *context)
{
	context->sbs_newblock_end_time = end_time;
}


int sb_append_string(unsigned char * str, struct encoder_ctx * context)
{
	unsigned char * ssb = context->split_sentence_buffer; // just a shortcut
	int required_capacity;
	unsigned char * search;

	if (NULL == ssb)
	{
		ssb = context->split_sentence_buffer = strdup(str);
	}
	else
	{
		required_capacity = strlen(str) + strlen(ssb);
		if (required_capacity < context->split_sentence_buffer_capacity)
		{
			while (required_capacity > context->split_sentence_buffer_capacity)
			{
				context->split_sentence_buffer_capacity *= 2;
			}
			ssb = context->split_sentence_buffer = (unsigned char *) realloc(
				ssb,
				context->split_sentence_buffer_capacity * sizeof(ssb[0])
			);
		};

		strcat(ssb, str);
	}

	for (search = ssb; search && *search != '.'; search ++)
		;

	if (search)
		return 1;
	return 0;
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
		}

if (1) //sub->nb_data)
{
	resub = malloc(sizeof(struct cc_subtitle));
	// for(;sub->next;sub = sub->next);
	// sub->next = malloc(sizeof(struct cc_subtitle));
	// if(!sub->next)
	// 	return -1;
	// sub->next->prev = sub;
	// sub = sub->next;
	resub->prev = NULL;
	resub->next = NULL;
}

resub->type = CC_TEXT;
// resub->enc_type = sub->e_type;
resub->data = strdup(str);
resub->nb_data = str? strlen(str): 0;
resub->start_time = ms_start;
resub->end_time = ms_end;
// if(info)
// 	strncpy(sub->info, info, 4);
// if(mode)
// 	strncpy(sub->mode, mode, 4);
resub->got_output = 1;

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
