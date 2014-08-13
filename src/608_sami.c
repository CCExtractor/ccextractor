#include "ccextractor.h"
#include "ccx_encoders_common.h"
#include "png.h"
#include "spupng_encoder.h"
#include "ocr.h"
#include "utility.h"

void write_stringz_as_sami(char *string, struct encoder_ctx *context, LLONG ms_start, LLONG ms_end)
{
	int used;
    sprintf ((char *) str,
    		"<SYNC start=%llu><P class=\"UNKNOWNCC\">\r\n",
    		(unsigned long long)ms_start);
    if (ccx_options.encoding!=CCX_ENC_UNICODE)
    {
        dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
    }

	used = encode_line(context->buffer,(unsigned char *) str);
	write (context->out->fh, context->buffer, used);
    int len=strlen (string);
    unsigned char *unescaped= (unsigned char *) malloc (len+1); 
    unsigned char *el = (unsigned char *) malloc (len*3+1); // Be generous
    if (el==NULL || unescaped==NULL)
        fatal (EXIT_NOT_ENOUGH_MEMORY, "In write_stringz_as_sami() - not enough memory.\n");
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
		write(context->out->fh, encoded_br, encoded_br_length);
        
		write(context->out->fh, encoded_crlf, encoded_crlf_length);
        begin+= strlen ((const char *) begin)+1;
    }

    sprintf ((char *) str,"</P></SYNC>\r\n");
    if (ccx_options.encoding!=CCX_ENC_UNICODE)
    {
        dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
    }
	used=encode_line (context->buffer,(unsigned char *) str);
	write(context->out->fh, context->buffer, used);
    sprintf ((char *) str,
    		"<SYNC start=%llu><P class=\"UNKNOWNCC\">&nbsp;</P></SYNC>\r\n\r\n",
    		(unsigned long long)ms_end);
    if (ccx_options.encoding!=CCX_ENC_UNICODE)
    {
        dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
    }
	write(context->out->fh, context->buffer, used);
	free(el);
	free(unescaped);
}


int write_cc_bitmap_as_sami(struct cc_subtitle *sub, struct encoder_ctx *context)
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
	//int used;
#ifdef ENABLE_OCR
	unsigned h1, m1, s1, ms1;
	unsigned h2, m2, s2, ms2;
#endif;
	LLONG ms_start, ms_end;
	//char timeline[128];
	int len = 0;

        x_pos = -1;
        y_pos = -1;
        width = 0;
        height = 0;

	if (context->prev_start != -1 && (sub->flags & SUB_EOD_MARKER))
	{
		ms_start = context->prev_start + subs_delay;
		ms_end = sub->start_time - 1;
	}
	else if ( !(sub->flags & SUB_EOD_MARKER))
	{
		ms_start = sub->start_time + subs_delay;
		ms_end = sub->end_time - 1;
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
			char *token = NULL;
			sprintf(context->buffer,
				"<SYNC start=%llu><P class=\"UNKNOWNCC\">\r\n"
				,(unsigned long long)ms_start);
			write(context->out->fh,context->buffer,strlen(context->buffer));
			token = strtok(str,"\r\n");
			while (token)
			{

				sprintf(context->buffer,"%s",token);
				token = strtok(NULL,"\r\n");
				if(token)
					strcat(context->buffer,"<br>\n");
				else
					strcat(context->buffer,"\n");
				write(context->out->fh,context->buffer,strlen(context->buffer));

			}
			sprintf(context->buffer,
				"<SYNC start=%llu><P class=\"UNKNOWNCC\">&nbsp;</P></SYNC>\r\n\r\n"
				,(unsigned long long)ms_end);
			write(context->out->fh,context->buffer,strlen(context->buffer));

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
int write_cc_buffer_as_sami(struct eia608_screen *data, struct encoder_ctx *context)
{
	int used;
	LLONG startms, endms;
    int wrote_something=0;
    startms = data->start_time;

    startms+=subs_delay;
    if (startms<0) // Drop screens that because of subs_delay start too early
        return 0; 

    endms   = data->end_time;
    endms--; // To prevent overlapping with next line.
    sprintf ((char *) str,
    		"<SYNC start=%llu><P class=\"UNKNOWNCC\">\r\n",
    		(unsigned long long)startms);
    if (ccx_options.encoding!=CCX_ENC_UNICODE)
    {
        dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
    }
	used = encode_line(context->buffer,(unsigned char *) str);
	write (context->out->fh, context->buffer, used);
    for (int i=0;i<15;i++)
    {
        if (data->row_used[i])
        {				
            int length = get_decoder_line_encoded (subline, i, data);
            if (ccx_options.encoding!=CCX_ENC_UNICODE)
            {
                dbg_print(CCX_DMT_DECODER_608, "\r");
                dbg_print(CCX_DMT_DECODER_608, "%s\n",subline);
            }
            write (context->out->fh, subline, length);            
            wrote_something=1;
            if (i!=14)            
                write (context->out->fh, encoded_br, encoded_br_length);            
            write (context->out->fh,encoded_crlf, encoded_crlf_length);
        }
    }
    sprintf ((char *) str,"</P></SYNC>\r\n");
    if (ccx_options.encoding!=CCX_ENC_UNICODE)
    {
        dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
    }
	used = encode_line(context->buffer,(unsigned char *) str);
	write (context->out->fh, context->buffer, used);
    sprintf ((char *) str,
    		"<SYNC start=%llu><P class=\"UNKNOWNCC\">&nbsp;</P></SYNC>\r\n\r\n",
    		(unsigned long long)endms);
    if (ccx_options.encoding!=CCX_ENC_UNICODE)
    {
        dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
    }
	used = encode_line(context->buffer,(unsigned char *) str);
	write (context->out->fh, context->buffer, used);
    return wrote_something;
}
