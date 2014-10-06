#include "ccextractor.h"
#include "ccx_encoders_common.h"
#include "png.h"
#include "spupng_encoder.h"
#include "ocr.h"
#include "utility.h"

/* The timing here is not PTS based, but output based, i.e. user delay must be accounted for
   if there is any */
void write_stringz_as_srt(char *string, struct encoder_ctx *context, LLONG ms_start, LLONG ms_end)
{
	int used;
	unsigned h1,m1,s1,ms1;
	unsigned h2,m2,s2,ms2;

	mstotime (ms_start,&h1,&m1,&s1,&ms1);
	mstotime (ms_end-1,&h2,&m2,&s2,&ms2); // -1 To prevent overlapping with next line.
	char timeline[128];
	context->srt_counter++;
	sprintf(timeline, "%u\r\n", context->srt_counter);
	used = encode_line(context->buffer,(unsigned char *) timeline);
	write(context->out->fh, context->buffer, used);
	sprintf (timeline, "%02u:%02u:%02u,%03u --> %02u:%02u:%02u,%03u\r\n",
			h1,m1,s1,ms1, h2,m2,s2,ms2);
	used = encode_line(context->buffer,(unsigned char *) timeline);
	dbg_print(CCX_DMT_DECODER_608, "\n- - - SRT caption - - -\n");
	dbg_print(CCX_DMT_DECODER_608, "%s",timeline);

	write(context->out->fh, context->buffer, used);
	int len=strlen (string);
	unsigned char *unescaped= (unsigned char *) malloc (len+1);
	unsigned char *el = (unsigned char *) malloc (len*3+1); // Be generous
	if (el==NULL || unescaped==NULL)
		fatal (EXIT_NOT_ENOUGH_MEMORY, "In write_stringz_as_srt() - not enough memory.\n");
	int pos_r=0;
	int pos_w=0;
	// Scan for \n in the string and replace it with a 0
	while (pos_r<len)
	{
		if (string[pos_r]=='\\' && string[pos_r+1]=='n')
		{
			unescaped[pos_w]=0;
			pos_r+=2;
		}
		else
		{
			unescaped[pos_w]=string[pos_r];
			pos_r++;
		}
		pos_w++;
	}
	unescaped[pos_w]=0;
	// Now read the unescaped string (now several string'z and write them)
	unsigned char *begin=unescaped;
	while (begin<unescaped+len)
	{
		unsigned int u = encode_line (el, begin);
		if (ccx_options.encoding!=CCX_ENC_UNICODE)
		{
			dbg_print(CCX_DMT_DECODER_608, "\r");
			dbg_print(CCX_DMT_DECODER_608, "%s\n",subline);
		}
		write(context->out->fh, el, u);
		write(context->out->fh, encoded_crlf, encoded_crlf_length);
		begin+= strlen ((const char *) begin)+1;
	}

	dbg_print(CCX_DMT_DECODER_608, "- - - - - - - - - - - -\r\n");

	write(context->out->fh, encoded_crlf, encoded_crlf_length);
	free(el);
	free(unescaped);
}

int write_cc_bitmap_as_srt(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	struct spupng_t *sp = (struct spupng_t *)context->out->spupng_data;
	int x_pos, y_pos, width, height, i;
	int x, y, y_off, x_off, ret;
	uint8_t *pbuf;
	//char *filename;
	struct cc_bitmap* rect;
	png_color *palette = NULL;
	png_byte *alpha = NULL;
#ifdef ENABLE_OCR
	char*str = NULL;
#endif
	LLONG ms_start, ms_end;
#ifdef ENABLE_OCR
	unsigned h1,m1,s1,ms1;
	unsigned h2,m2,s2,ms2;
	char timeline[128];
	int len = 0;
	int used;
#endif

        x_pos = -1;
        y_pos = -1;
        width = 0;
        height = 0;

	if (context->prev_start != -1 && (sub->flags & SUB_EOD_MARKER))
	{
		ms_start = context->prev_start;
		ms_end = sub->start_time;
	}
	else if ( !(sub->flags & SUB_EOD_MARKER))
	{
		ms_start = sub->start_time;
		ms_end = sub->end_time;
	}

	if(sub->nb_data == 0 )
		return 0;
	rect = sub->data;
	for(i = 0;i < sub->nb_data;i++)
	{
		if(x_pos == -1)
		{
			x_pos = rect[i].x;
			y_pos = rect[i].y;
			width = rect[i].w;
			height = rect[i].h;
		}
		else
		{
			if(x_pos > rect[i].x)
			{
				width += (x_pos - rect[i].x);
				x_pos = rect[i].x;
			}

                        if (rect[i].y < y_pos)
                        {
                                height += (y_pos - rect[i].y);
                                y_pos = rect[i].y;
                        }

                        if (rect[i].x + rect[i].w > x_pos + width)
                        {
                                width = rect[i].x + rect[i].w - x_pos;
                        }

                        if (rect[i].y + rect[i].h > y_pos + height)
                        {
                                height = rect[i].y + rect[i].h - y_pos;
                        }

		}
	}
	if ( sub->flags & SUB_EOD_MARKER )
		context->prev_start =  sub->start_time;
	pbuf = (uint8_t*) malloc(width * height);
	memset(pbuf, 0x0, width * height);

	for(i = 0;i < sub->nb_data;i++)
	{
		x_off = rect[i].x - x_pos;
		y_off = rect[i].y - y_pos;
		for (y = 0; y < rect[i].h; y++)
		{
			for (x = 0; x < rect[i].w; x++)
				pbuf[((y + y_off) * width) + x_off + x] = rect[i].data[0][y * rect[i].w + x];

		}
	}
	palette = (png_color*) malloc(rect[0].nb_colors * sizeof(png_color));
	if(!palette)
	{
		ret = -1;
		goto end;
	}
        alpha = (png_byte*) malloc(rect[0].nb_colors * sizeof(png_byte));
        if(!alpha)
        {
                ret = -1;
                goto end;
        }
	/* TODO do rectangle, wise one color table should not be used for all rectangle */
        mapclut_paletee(palette, alpha, (uint32_t *)rect[0].data[1],rect[0].nb_colors);
	quantize_map(alpha, palette, pbuf, width*height, 3, rect[0].nb_colors);
#ifdef ENABLE_OCR
	str = ocr_bitmap(palette,alpha,pbuf,width,height);
	if(str && str[0])
	{
		if (context->prev_start != -1 || !(sub->flags & SUB_EOD_MARKER))
		{
			mstotime (ms_start,&h1,&m1,&s1,&ms1);
			mstotime (ms_end-1,&h2,&m2,&s2,&ms2); // -1 To prevent overlapping with next line.
			context->srt_counter++;
			sprintf(timeline, "%u\r\n", context->srt_counter);
			used = encode_line(context->buffer,(unsigned char *) timeline);
			write(context->out->fh, context->buffer, used);
			sprintf (timeline, "%02u:%02u:%02u,%03u --> %02u:%02u:%02u,%03u\r\n",
				h1,m1,s1,ms1, h2,m2,s2,ms2);
			used = encode_line(context->buffer,(unsigned char *) timeline);
			write (context->out->fh, context->buffer, used);
			len = strlen(str);
			write (context->out->fh, str, len);
			write (context->out->fh, encoded_crlf, encoded_crlf_length);
		}
	}
#endif

end:
	sub->nb_data = 0;
	freep(&sub->data);
	freep(&palette);
	freep(&alpha);
	return ret;

}
int write_cc_buffer_as_srt(struct eia608_screen *data, struct encoder_ctx *context)
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
	if (empty_buf) // Prevent writing empty screens. Not needed in .srt
		return 0;

	ms_start+=subs_delay;
	if (ms_start<0) // Drop screens that because of subs_delay start too early
		return 0;

	ms_end = data->end_time;

	mstotime (ms_start,&h1,&m1,&s1,&ms1);
	mstotime (ms_end-1,&h2,&m2,&s2,&ms2); // -1 To prevent overlapping with next line.
	char timeline[128];
	context->srt_counter++;
	sprintf(timeline, "%u\r\n", context->srt_counter);
	used = encode_line(context->buffer,(unsigned char *) timeline);
	write(context->out->fh, context->buffer, used);
	sprintf (timeline, "%02u:%02u:%02u,%03u --> %02u:%02u:%02u,%03u\r\n",
			h1,m1,s1,ms1, h2,m2,s2,ms2);
	used = encode_line(context->buffer,(unsigned char *) timeline);

	dbg_print(CCX_DMT_DECODER_608, "\n- - - SRT caption ( %d) - - -\n", context->srt_counter);
	dbg_print(CCX_DMT_DECODER_608, "%s",timeline);

	write (context->out->fh, context->buffer, used);
	for (int i=0;i<15;i++)
	{
		if (data->row_used[i])
		{
			if (ccx_options.sentence_cap)
			{
				capitalize (i,data);
				correct_case(i,data);
			}
			if (ccx_options.autodash && ccx_options.trim_subs)
			{
				int first=0, last=31, center1=-1, center2=-1;
				unsigned char *line = data->characters[i];
				int do_dash=1, colon_pos=-1;
				find_limit_characters(line,&first,&last);
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
			int length = get_decoder_line_encoded (subline, i, data);
			if (ccx_options.encoding!=CCX_ENC_UNICODE)
			{
				dbg_print(CCX_DMT_DECODER_608, "\r");
				dbg_print(CCX_DMT_DECODER_608, "%s\n",subline);
			}
			write(context->out->fh, subline, length);
			write(context->out->fh, encoded_crlf, encoded_crlf_length);
			wrote_something=1;
			// fprintf (wb->fh,encoded_crlf);
		}
	}
	dbg_print(CCX_DMT_DECODER_608, "- - - - - - - - - - - -\r\n");

	// fprintf (wb->fh, encoded_crlf);
	write (context->out->fh, encoded_crlf, encoded_crlf_length);
	return wrote_something;
}
