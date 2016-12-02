#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "ccx_encoders_common.h"
#include "utility.h"
#include "ccx_encoders_helpers.h"
#include "ocr.h"

int write_stringz_as_ssa(char *string, struct encoder_ctx *context, LLONG ms_start, LLONG ms_end)
{
	// TODO: write it
	return 0;
}

int write_cc_bitmap_as_ssa(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	// TODO: write it
	return 0;
}

int write_cc_subtitle_as_ssa(struct cc_subtitle *sub,struct encoder_ctx *context)
{
	int ret = 0;
	struct cc_subtitle *osub = sub;
	struct cc_subtitle *lsub = sub;

	while(sub)
	{
		if(sub->type == CC_TEXT)
		{
			ret = write_stringz_as_ssa(sub->data, context, sub->start_time, sub->end_time);
			freep(&sub->data);
			sub->nb_data = 0;
			ret = 1;
		}
		lsub = sub;
		sub = sub->next;
	}
	while(lsub != osub)
	{
		sub = lsub->prev;
		freep(&lsub);
		lsub = sub;
	}

	return ret;
}
int write_cc_buffer_as_ssa(struct eia608_screen *data, struct encoder_ctx *context)
{
	int used;
	unsigned h1,m1,s1,ms1;
	unsigned h2,m2,s2,ms2;
	LLONG ms_start, ms_end;
	int wrote_something = 0;
	ms_start = data->start_time;

	int prev_line_start=-1, prev_line_end=-1; // Column in which the previous line started and ended, for autodash
	int prev_line_center1=-1, prev_line_center2=-1; // Center column of previous line text
	int empty_buf=1;
	for (int i=0;i<15;i++)
	{
		if (data->row_used[i])
		{
			empty_buf=0;
			break;
		}
	}
	if (empty_buf)
		return 0;

	ms_start+=context->subs_delay;
	if (ms_start<0)
		return 0;

	ms_end = data->end_time;

	mstotime (ms_start,&h1,&m1,&s1,&ms1);
	mstotime (ms_end-1,&h2,&m2,&s2,&ms2); // -1 To prevent overlapping with next line.
	char timeline[128];
	sprintf (timeline, "Dialogue: 0,%02u:%02u:%02u.%01u,%02u:%02u:%02u.%02u,Default,,0000,0000,0000,,",
			 h1, m1, s1, ms1 / 10, h2, m2, s2, ms2 / 10);
	used = encode_line(context, context->buffer,(unsigned char *) timeline);

	dbg_print(CCX_DMT_DECODER_608, "\n- - - ASS/SSA caption - - -\n");
	dbg_print(CCX_DMT_DECODER_608, "%s",timeline);

	write (context->out->fh, context->buffer, used);

	int line_count = 0;
	for (int i=0;i<15;i++)
	{
		if (data->row_used[i])
		{
			if (context->sentence_cap)
			{
				if (clever_capitalize (context, i, data))
					correct_case_with_dictionary(i, data);
			}
			if (context->autodash && context->trim_subs)
			{
				int first=0, last=31, center1=-1, center2=-1;
				unsigned char *line = data->characters[i];
				int do_dash=1, colon_pos=-1;
				find_limit_characters(line, &first, &last, CCX_DECODER_608_SCREEN_WIDTH);
				if (first==-1 || last==-1)  // Probably a bug somewhere though
					break;
				// Is there a speaker named, for example: TOM: What are you doing?
				for (int j=first;j<=last;j++)
				{
					if (line[j]==':')
					{
						colon_pos=j;
						break;
					}
					if (!isupper (line[j]))
						break;
				}
				if (prev_line_start==-1)
					do_dash=0;
				if (first==prev_line_start) // Case of left alignment
					do_dash=0;
				if (last==prev_line_end)  // Right align
					do_dash=0;
				if (first>prev_line_start && last<prev_line_end) // Fully contained
					do_dash=0;
				if ((first>prev_line_start && first<prev_line_end) || // Overlap
					(last>prev_line_start && last<prev_line_end))
					do_dash=0;

				center1=(first+last)/2;
				if (colon_pos!=-1)
				{
					while (colon_pos<CCX_DECODER_608_SCREEN_WIDTH &&
						   (line[colon_pos]==':' ||
							line[colon_pos]==' ' ||
							line[colon_pos]==0x89))
						colon_pos++; // Find actual text
					center2=(colon_pos+last)/2;
				}
				else
					center2=center1;

				if (center1>=prev_line_center1-1 && center1<=prev_line_center1+1 && center1!=-1) // Center align
					do_dash=0;
				if (center2>=prev_line_center2-2 && center1<=prev_line_center2+2 && center1!=-1) // Center align
					do_dash=0;

				if (do_dash)
					write(context->out->fh, "- ", 2);
				prev_line_start=first;
				prev_line_end=last;
				prev_line_center1=center1;
				prev_line_center2=center2;

			}
			int length = get_decoder_line_encoded (context, context->subline, i, data);
			if (context->encoding!=CCX_ENC_UNICODE)
			{
				dbg_print(CCX_DMT_DECODER_608, "\r");
				dbg_print(CCX_DMT_DECODER_608, "%s\n",context->subline);
			}
			if (line_count)
				write(context->out->fh, "\\N", 2);
			write(context->out->fh, context->subline, length);
			line_count++;
			wrote_something=1;
		}
	}

	dbg_print(CCX_DMT_DECODER_608, "- - - - - - - - - - - -\r\n");

	write (context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
	return wrote_something;
}
