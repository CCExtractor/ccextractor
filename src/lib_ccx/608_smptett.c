/*
	Produces minimally-compliant SMPTE Timed Text (W3C TTML)
	format-compatible output

	See http://www.w3.org/TR/ttaf1-dfxp/ and
	https://www.smpte.org/sites/default/files/st2052-1-2010.pdf

	Copyright (C) 2012 John Kemp

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "ccx_encoders_common.h"
#include "png.h"
#include "spupng_encoder.h"
#include "ocr.h"
#include "utility.h"


void write_stringz_as_smptett(char *string, struct encoder_ctx *context, LLONG ms_start, LLONG ms_end)
{
	int used;
	unsigned h1, m1, s1, ms1;
	unsigned h2, m2, s2, ms2;
	int len = strlen (string);
	unsigned char *unescaped= (unsigned char *) malloc (len+1);
	unsigned char *el = (unsigned char *) malloc (len*3+1); // Be generous
	int pos_r = 0;
	int pos_w = 0;

	if (el == NULL || unescaped == NULL)
		fatal (EXIT_NOT_ENOUGH_MEMORY, "In write_stringz_as_sami() - not enough memory.\n");

	mstotime (ms_start, &h1, &m1, &s1, &ms1);
	mstotime (ms_end-1, &h2, &m2, &s2, &ms2);

	sprintf ((char *) str, "<p begin=\"%02u:%02u:%02u.%03u\" end=\"%02u:%02u:%02u.%03u\">\r\n", h1, m1, s1, ms1, h2, m2, s2, ms2);
	if (context->encoding!=CCX_ENC_UNICODE)
	{
		dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
	}
	used = encode_line(context->buffer, (unsigned char *) str);
	write (context->out->fh, context->buffer, used);
	// Scan for \n in the string and replace it with a 0
	while (pos_r < len)
	{
		if (string[pos_r] == '\\' && string[pos_r+1] == 'n')
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
		unsigned int u = encode_line (el, begin);
		if (context->encoding != CCX_ENC_UNICODE)
		{
			dbg_print(CCX_DMT_DECODER_608, "\r");
			dbg_print(CCX_DMT_DECODER_608, "%s\n", subline);
		}
		write(context->out->fh, el, u);
		//write (wb->fh, encoded_br, encoded_br_length);

		write(context->out->fh, encoded_crlf, encoded_crlf_length);
		begin += strlen ((const char *) begin)+1;
	}

	sprintf ((char *) str, "</p>\n");
	if (context->encoding != CCX_ENC_UNICODE)
	{
		dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
	}
	used = encode_line(context->buffer, (unsigned char *) str);
	write(context->out->fh, context->buffer, used);
	sprintf ((char *) str, "<p begin=\"%02u:%02u:%02u.%03u\">\n\n", h2, m2, s2, ms2);
	if (context->encoding != CCX_ENC_UNICODE)
	{
		dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
	}
	used = encode_line(context->buffer, (unsigned char *) str);
	write (context->out->fh, context->buffer, used);
	sprintf ((char *) str, "</p>\n");
	free(el);
	free(unescaped);
}


int write_cc_bitmap_as_smptett(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	int ret = 0;
	struct cc_bitmap* rect;
	LLONG ms_start, ms_end;
	//char timeline[128];
	int len = 0;


	if (context->prev_start != -1 && (sub->flags & SUB_EOD_MARKER))
	{
		ms_start = context->prev_start + context->subs_delay;
		ms_end = sub->start_time - 1;
	}
	else if ( !(sub->flags & SUB_EOD_MARKER))
	{
		ms_start = sub->start_time + context->subs_delay;
		ms_end = sub->end_time - 1;
	}
	else if (context->prev_start == -1 && (sub->flags & SUB_EOD_MARKER))
	{
		ms_start = 1 + context->subs_delay;
		ms_end = sub->start_time - 1;
	}

	if(sub->nb_data == 0 )
		return 0;
	rect = sub->data;

	if ( sub->flags & SUB_EOD_MARKER )
		context->prev_start =  sub->start_time;

#ifdef ENABLE_OCR
	if (rect[0].ocr_text && *(rect[0].ocr_text))
	{
		if (context->prev_start != -1 || !(sub->flags & SUB_EOD_MARKER))
		{
			char *buf = (char *) context->buffer;
			unsigned h1, m1, s1, ms1;
			unsigned h2, m2, s2, ms2;
			mstotime (ms_start,&h1,&m1,&s1,&ms1);
			mstotime (ms_end-1,&h2,&m2,&s2,&ms2); // -1 To prevent overlapping with next line.
			sprintf ((char *) context->buffer,"<p begin=\"%02u:%02u:%02u.%03u\" end=\"%02u:%02u:%02u.%03u\">\n",h1,m1,s1,ms1, h2,m2,s2,ms2);
			write (context->out->fh, buf,strlen(buf) );
			len = strlen(rect[0].ocr_text);
			write (context->out->fh, rect[0].ocr_text, len);
			write (context->out->fh, encoded_crlf, encoded_crlf_length);
			sprintf ( buf,"</p>\n");
			write (context->out->fh, buf,strlen(buf) );

		}
	}
#endif

	sub->nb_data = 0;
	freep(&sub->data);
	return ret;

}

int write_cc_buffer_as_smptett(struct eia608_screen *data, struct encoder_ctx *context)
{
	int used;
	unsigned h1,m1,s1,ms1;
	unsigned h2,m2,s2,ms2;
	LLONG endms;
	int wrote_something=0;
	LLONG startms = data->start_time;

	startms+=context->subs_delay;
	if (startms<0) // Drop screens that because of subs_delay start too early
		return 0;

	endms  = data->end_time;
	endms--; // To prevent overlapping with next line.
	mstotime (startms,&h1,&m1,&s1,&ms1);
	mstotime (endms-1,&h2,&m2,&s2,&ms2);

	sprintf ((char *) str,"<p begin=\"%02u:%02u:%02u.%03u\" end=\"%02u:%02u:%02u.%03u\">\n",h1,m1,s1,ms1, h2,m2,s2,ms2);

	if (context->encoding!=CCX_ENC_UNICODE)
	{
		dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
	}
	used = encode_line(context->buffer,(unsigned char *) str);
	write (context->out->fh, context->buffer, used);
	for (int i=0; i < 15; i++)
	{
		if (data->row_used[i])
		{
			int length = get_decoder_line_encoded (subline, i, data);
			if (context->encoding!=CCX_ENC_UNICODE)
			{
				dbg_print(CCX_DMT_DECODER_608, "\r");
				dbg_print(CCX_DMT_DECODER_608, "%s\n",subline);
			}
			write(context->out->fh, subline, length);
			wrote_something=1;

			write(context->out->fh, encoded_crlf, encoded_crlf_length);
		}
	}
	sprintf ((char *) str,"</p>\n");
	if (context->encoding!=CCX_ENC_UNICODE)
	{
		dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
	}
	used = encode_line(context->buffer,(unsigned char *) str);
	write (context->out->fh, context->buffer, used);

	if (context->encoding!=CCX_ENC_UNICODE)
	{
		dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
	}
	used = encode_line(context->buffer,(unsigned char *) str);
	//write (wb->fh, enc_buffer,enc_buffer_used);

	return wrote_something;
}
