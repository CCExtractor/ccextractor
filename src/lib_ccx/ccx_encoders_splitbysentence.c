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

int write_cc_bitmap_to_sentence_buffer(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	int ret = 0;
#ifdef ENABLE_OCR
	struct cc_bitmap* rect;

	LLONG ms_start, ms_end;

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
		return ret;
	rect = sub->data;

	if (sub->flags & SUB_EOD_MARKER)
		context->prev_start = sub->start_time;


	if (rect[0].ocr_text && *(rect[0].ocr_text))
	{
		lbl_start_block(ms_start, context);
		if (context->prev_start != -1 || !(sub->flags & SUB_EOD_MARKER))
		{
			char *token = NULL;
			token = paraof_ocrtext(sub, " ", 1); // Get text with spaces instead of newlines
			uint32_t offset=0;
			utf8proc_ssize_t ls; // Last size
			char *s = token;
			int32_t uc;
			while ((ls=utf8proc_iterate(s, -1, &uc))) 
			{
				ccx_sbs_utf8_character sbsc;
				// Note: We don't care about uc here, since we will be writing the encoded bytes, not the code points in binary.
				//TODO: Deal with ls < 0
				if (!uc) // End of string
					break; 
				printf("%3ld | %08X | %c %c %c %c\n", ls, uc, ((uc & 0xFF000000) >> 24),  ((uc & 0xFF0000) >> 16), 
					((uc & 0xFF00) >> 8), ( uc & 0xFF));				
				sbsc.ch = uc;
				sbsc.encoded[0] = 0; sbsc.encoded[1] = 0; sbsc.encoded[2] = 0; sbsc.encoded[3] = 0;
				memcpy(sbsc.encoded, s, ls);
				sbsc.enc_len = ls;
				sbsc.ts = 0; // We don't know yet
				lbl_add_character(context, sbsc);
				s += ls;				
				
				// TO-DO: Add each of these characters to the buffer, splitting the timestamps. Remember to add character length to the array
			}
			printf("-------\n");

			/*
			while (token)
			{
				char *newline_pos = strstr(token, context->encoded_crlf);
				if (!newline_pos)
				{
					fdprintf(context->out->fh, "%s", token);
					break;
				}
				else
				{
					while (token != newline_pos)
					{
						fdprintf(context->out->fh, "%c", *token);
						token++;
					}
					token += context->encoded_crlf_length;
					fdprintf(context->out->fh, "%c", ' ');
				}
			}*/

		}
		lbl_end_block(ms_end, context);
	}
#endif

	sub->nb_data = 0;
	freep(&sub->data);
	return ret;

}
