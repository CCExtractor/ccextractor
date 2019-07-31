#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "ccx_encoders_common.h"
#include "png.h"
#include "ocr.h"
#include "utility.h"
#include "ccx_encoders_helpers.h"

int write_stringz_as_sami(char *string, struct encoder_ctx *context, LLONG ms_start, LLONG ms_end)
{
	int used;
	int len = 0;
	int ret = 0;
	unsigned char *unescaped = NULL;
	unsigned char *el = NULL;
	char str[1024];

	sprintf (str,"<SYNC start=%llu><P class=\"UNKNOWNCC\">\r\n",(unsigned long long)ms_start);
	if (context->encoding != CCX_ENC_UNICODE)
	{
		dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
	}

	used = encode_line(context, context->buffer, (unsigned char *) str);
	ret = write (context->out->fh, context->buffer, used);
	if(ret != used)
	{
		return ret;
	}

	len = strlen (string);
	unescaped= (unsigned char *) malloc (len+1);
	if(!unescaped)
	{
		mprint ("In write_stringz_as_sami() - not enough memory for len %d.\n", len);
		ret = -1;
		goto end;
	}

	el = (unsigned char *) malloc (len*3+1); // Be generous
	if (el == NULL)
	{
		mprint ("In write_stringz_as_sami() - not enough memory for len %d.\n", len);
		ret = -1;
		goto end;
	}

	int pos_r=0;
	int pos_w=0;
	// Scan for \n in the string and replace it with a 0
	while (pos_r < len)
	{
		if (string[pos_r] == '\\' && string[pos_r+1]=='n')
		{
			unescaped[pos_w] = 0;
			pos_r += 2;
		}
		else
		{
			unescaped[pos_w] = string[pos_r];
			pos_r++;
		}
		pos_w++;
	}
	unescaped[pos_w] = 0;
	// Now read the unescaped string (now several string'z and write them)
	unsigned char *begin = unescaped;
	while (begin < unescaped+len)
	{
		unsigned int u = encode_line (context, el, begin);
		if (context->encoding != CCX_ENC_UNICODE)
		{
			dbg_print(CCX_DMT_DECODER_608, "\r");
			dbg_print(CCX_DMT_DECODER_608, "%s\n",context->subline);
		}
		ret = write(context->out->fh, el, u);
		if(ret != u)
			goto end;

		ret = write(context->out->fh, context->encoded_br, context->encoded_br_length);
		if(ret != context->encoded_br_length)
			goto end;

		ret = write(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
		if(ret != context->encoded_crlf_length)
			goto end;

		begin += strlen ((const char *) begin) + 1;
	}

	sprintf ((char *) str, "</P></SYNC>\r\n");
	if (context->encoding != CCX_ENC_UNICODE)
	{
		dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
	}
	used = encode_line (context, context->buffer,(unsigned char *) str);
	ret = write(context->out->fh, context->buffer, used);
	if(ret != used)
		goto end;
	sprintf ((char *) str,
			"<SYNC start=%llu><P class=\"UNKNOWNCC\">&nbsp;</P></SYNC>\r\n\r\n",
			(unsigned long long)ms_end);
	if (context->encoding != CCX_ENC_UNICODE)
	{
		dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
	}
	ret = write(context->out->fh, context->buffer, used);
	if(ret != used)
		goto end;

end:
	free(el);
	free(unescaped);
	return ret;
}

int write_cc_bitmap_as_sami(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	int ret = 0;
#ifdef ENABLE_OCR
	struct cc_bitmap* rect;
	LLONG ms_start, ms_end;

	ms_start = sub->start_time;
	ms_end = sub->end_time;


	rect = sub->data;

	if (sub->flags & SUB_EOD_MARKER)
		context->prev_start = sub->start_time;

	char *token = NULL;
	char *buf = (char*)context->buffer;

	if (sub->data != NULL) //then we should write the sub
	{
		sprintf(buf,
			"<SYNC start=%llu><P class=\"UNKNOWNCC\">\r\n"
			, (unsigned long long)ms_start);
		write(context->out->fh, buf, strlen(buf));
		for (int i = sub->nb_data - 1; i >= 0; i--)
		{
			if (rect[i].ocr_text && *(rect[i].ocr_text))
			{
				if (context->prev_start != -1 || !(sub->flags & SUB_EOD_MARKER))
				{
					token = strtok(rect[i].ocr_text, "\r\n");
					sprintf(buf, "%s", token);
					token = strtok(NULL, "\r\n");
					write(context->out->fh, buf, strlen(buf));
					if (i != 0)
						write(context->out->fh, context->encoded_br, context->encoded_br_length);
					write(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
				}
			}
		}
		sprintf(buf, "</P></SYNC>\r\n");
		write(context->out->fh, buf, strlen(buf));
	}
	else //we write an empty subtitle to clear the old one
	{
		sprintf(buf,
			"<SYNC start=%llu><P class=\"UNKNOWNCC\">&nbsp;</P></SYNC>\r\n\r\n"
			, (unsigned long long)ms_start);
		write(context->out->fh, buf, strlen(buf));
	}
#endif

	sub->nb_data = 0;
	freep(&sub->data);
	return ret;
}

int write_cc_subtitle_as_sami(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	int ret = 0;
	struct cc_subtitle *osub = sub;
	struct cc_subtitle *lsub = sub;
	while(sub)
	{
		if(sub->type == CC_TEXT)
		{
			ret = write_stringz_as_sami(sub->data, context, sub->start_time, sub->end_time);
			freep(&sub->data);
			sub->nb_data = 0;
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

int write_cc_buffer_as_sami(struct eia608_screen *data, struct encoder_ctx *context)
{
	int used;
	LLONG startms, endms;
	int wrote_something=0;
	char str[1024];

	startms = data->start_time;

	startms+=context->subs_delay;
	if (startms<0) // Drop screens that because of subs_delay start too early
		return 0;

	endms   = data->end_time;
	endms--; // To prevent overlapping with next line.
	sprintf (str,"<SYNC start=%llu><P class=\"UNKNOWNCC\">\r\n",
			(unsigned long long)startms);
	if (context->encoding != CCX_ENC_UNICODE)
	{
		dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
	}
	used = encode_line(context, context->buffer,(unsigned char *) str);
	write (context->out->fh, context->buffer, used);
	for (int i=0;i<15;i++)
	{
		if (data->row_used[i])
		{
			int length = get_decoder_line_encoded (context, context->subline, i, data);
			if (context->encoding != CCX_ENC_UNICODE)
			{
				dbg_print(CCX_DMT_DECODER_608, "\r");
				dbg_print(CCX_DMT_DECODER_608, "%s\n",context->subline);
			}
			write (context->out->fh, context->subline, length);
			wrote_something = 1;
			if (i!=14)
				write (context->out->fh, context->encoded_br, context->encoded_br_length);
			write (context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
		}
	}
	sprintf ((char *) str,"</P></SYNC>\r\n");
	if (context->encoding != CCX_ENC_UNICODE)
	{
		dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
	}
	used = encode_line(context, context->buffer,(unsigned char *) str);
	write (context->out->fh, context->buffer, used);
	sprintf ((char *) str,
			"<SYNC start=%llu><P class=\"UNKNOWNCC\">&nbsp;</P></SYNC>\r\n\r\n",
			(unsigned long long)endms);
	if (context->encoding!=CCX_ENC_UNICODE)
	{
		dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
	}
	used = encode_line(context, context->buffer,(unsigned char *) str);
	write (context->out->fh, context->buffer, used);
	return wrote_something;
}
