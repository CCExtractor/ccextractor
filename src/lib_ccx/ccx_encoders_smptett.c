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
#include "ocr.h"
#include "utility.h"
#include "ccx_encoders_helpers.h"

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
	char str[1024];

	if (el == NULL || unescaped == NULL)
		fatal (EXIT_NOT_ENOUGH_MEMORY, "In write_stringz_as_smptett() - not enough memory.\n");

	millis_to_time (ms_start, &h1, &m1, &s1, &ms1);
	millis_to_time (ms_end-1, &h2, &m2, &s2, &ms2);

	sprintf ((char *) str, "<p begin=\"%02u:%02u:%02u.%03u\" end=\"%02u:%02u:%02u.%03u\">\r\n", h1, m1, s1, ms1, h2, m2, s2, ms2);
	if (context->encoding!=CCX_ENC_UNICODE)
	{
		dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
	}
	used = encode_line(context, context->buffer, (unsigned char *) str);
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
		unsigned int u = encode_line (context, el, begin);
		if (context->encoding != CCX_ENC_UNICODE)
		{
			dbg_print(CCX_DMT_DECODER_608, "\r");
			dbg_print(CCX_DMT_DECODER_608, "%s\n", context->subline);
		}
		write(context->out->fh, el, u);
		//write (wb->fh, encoded_br, encoded_br_length);

		write(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
		begin += strlen ((const char *) begin)+1;
	}

	sprintf ((char *) str, "</p>\n");
	if (context->encoding != CCX_ENC_UNICODE)
	{
		dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
	}
	used = encode_line(context, context->buffer, (unsigned char *) str);
	write(context->out->fh, context->buffer, used);
	sprintf ((char *) str, "<p begin=\"%02u:%02u:%02u.%03u\">\n\n", h2, m2, s2, ms2);
	if (context->encoding != CCX_ENC_UNICODE)
	{
		dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
	}
	used = encode_line(context, context->buffer, (unsigned char *) str);
	write (context->out->fh, context->buffer, used);
	sprintf ((char *) str, "</p>\n");
	free(el);
	free(unescaped);
}


int write_cc_bitmap_as_smptett(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	int ret = 0;
#ifdef ENABLE_OCR
	struct cc_bitmap* rect;
	LLONG ms_start, ms_end;
	//char timeline[128];
	int i,len = 0;

	ms_start = sub->start_time;
	ms_end = sub->end_time;

	if(sub->nb_data == 0 )
		return 0;

	rect = sub->data;

	if ( sub->flags & SUB_EOD_MARKER )
		context->prev_start =  sub->start_time;

	for (i = sub->nb_data - 1; i >= 0; i--)
	{
		if (rect[i].ocr_text && *(rect[i].ocr_text))
		{
			if (context->prev_start != -1 || !(sub->flags & SUB_EOD_MARKER))
			{
				char *buf = (char *)context->buffer;
				unsigned h1, m1, s1, ms1;
				unsigned h2, m2, s2, ms2;
				millis_to_time(ms_start, &h1, &m1, &s1, &ms1);
				millis_to_time(ms_end - 1, &h2, &m2, &s2, &ms2); // -1 To prevent overlapping with next line.
				sprintf((char *)context->buffer, "<p begin=\"%02u:%02u:%02u.%03u\" end=\"%02u:%02u:%02u.%03u\">\n", h1, m1, s1, ms1, h2, m2, s2, ms2);
                write(context->out->fh, buf, strlen(buf));
				len = strlen(rect[i].ocr_text);
                write(context->out->fh, rect[i].ocr_text, len);
				write(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
				sprintf(buf, "</p>\n");
				write(context->out->fh, buf, strlen(buf));

			}
		}
	}
	for (i = 0, rect = sub->data; i < sub->nb_data; i++, rect++)
	{
		freep(rect->data);
		freep(rect->data + 1);
	}
#endif

	sub->nb_data = 0;
	freep(&sub->data);
	return ret;

}

int write_cc_subtitle_as_smptett(struct cc_subtitle *sub, struct encoder_ctx *context)
{
	int ret = 0;
	struct cc_subtitle *osub = sub;
	struct cc_subtitle *lsub = sub;
	while(sub)
	{
		if(sub->type == CC_TEXT)
		{
			write_stringz_as_smptett(sub->data, context, sub->start_time, sub->end_time);
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

	 
int write_cc_buffer_as_smptett(struct eia608_screen *data, struct encoder_ctx *context)
{
	int used;
	unsigned h1,m1,s1,ms1;
	unsigned h2,m2,s2,ms2;
	LLONG endms;
	int wrote_something=0;
	LLONG startms = data->start_time;
	char str[1024];

	startms+=context->subs_delay;
	if (startms<0) // Drop screens that because of subs_delay start too early
		return 0;

	endms  = data->end_time;
	endms--; // To prevent overlapping with next line.
	millis_to_time (startms,&h1,&m1,&s1,&ms1);
	millis_to_time (endms-1,&h2,&m2,&s2,&ms2);

	for (int row=0; row < 15; row++)
	{
		if (data->row_used[row])
		{
			if (context->sentence_cap)
			{
				if (clever_capitalize(context, row, data))
			correct_case_with_dictionary(row, data);
			}
		
			float row1=0;
			float col1=0;
			int firstcol=-1;
			
			// ROWS is actually 90% of the screen size
			// Add +10% because row 0 is at position 10%
			row1 = ((100*row)/(ROWS/0.8))+10;
			
			for (int column = 0; column < COLUMNS ; column++) 
			{
				int unicode = 0;
				get_char_in_unicode((unsigned char*)&unicode, data->characters[row][column]);
				//if (COL_TRANSPARENT != data->colors[row][column])
				if (unicode != 0x20)
				{
					if (firstcol<0)
					{
						firstcol = column;
					}
				}
			}
			// COLUMNS is actually 90% of the screen size
			// Add +10% because column 0 is at position 10%
			col1 = ((100*firstcol)/(COLUMNS/0.8))+10;
		
			if (firstcol>=0)
			{
				wrote_something=1;
			
				sprintf ((char *) str,"      <p begin=\"%02u:%02u:%02u.%03u\" end=\"%02u:%02u:%02u.%03u\" tts:origin=\"%1.3f%% %1.3f%%\">\n        <span>",h1,m1,s1,ms1, h2,m2,s2,ms2,col1,row1);
				if (context->encoding!=CCX_ENC_UNICODE)
				{
					dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
				}
				used = encode_line(context, context->buffer,(unsigned char *) str);
				write (context->out->fh, context->buffer, used);
				// Trimming subs because the position is defined by "tts:origin"
				int old_trim_subs = context->trim_subs;
				context->trim_subs=1;
				if (context->encoding!=CCX_ENC_UNICODE)
				{
					dbg_print(CCX_DMT_DECODER_608, "\r");
					dbg_print(CCX_DMT_DECODER_608, "%s\n",context->subline);
				}
				int length = get_decoder_line_encoded (context, context->subline, row, data);


				unsigned char *final = malloc ( strlen((context->subline)) + 1000);	//Being overly generous? :P
				unsigned char *temp = malloc ( strlen((context->subline)) + 1000);
				*final=0;
				*temp=0;
				/*
					final	: stores formatted HTML sentence. This will be written in subtitle file.
					temp	: stored temporary sentences required while formatting
					
					+1000 because huge <span> and <style> tags are added. This will just prevent overflow (hopefully).
				*/

				int style = 0; 

				/*

				0 = None or font colour
				1 = itlics
				2 = bold
				3 = underline

				*/

				//Now, searching for first occurance of <i> OR <u> OR <b>

				unsigned char * start = strstr((context->subline), "<i>"); 
				if(start==NULL)
				{
					start = strstr((context->subline), "<b>");

					if(start==NULL)
					{
						start = strstr((context->subline), "<u>");
						style = 3;   //underline
					}

					else
					style = 2;   //bold
				}

				else
					style = 1;      //italics

				if(start!=NULL)		//subtitle has style associated with it, will need formatting.
				{
					unsigned char *end_tag;
					if(style == 1)
					{
						end_tag ="</i>";
					}

					else if(style == 2)
					{
						end_tag = "</b>";
					}
					    
					else
					{
					end_tag = "</u>";
					}

				    unsigned char *end = strstr((context->subline), end_tag);	//occurance of closing tag (</i> OR </b> OR </u>)
				    
				    if(end==NULL)
				    {
				        //Incorrect styling, writing as it is
				        strcpy(final,(context->subline));			            
				    }
				    
					else
					{
						int start_index = start-(context->subline);
						int end_index = end-(context->subline);
									            
						strncat(final,(context->subline),start_index);     // copying content before opening tag e.g. <i> 
						
						strcat(final,"<span>");                 //adding <span> : replacement of <i>

						//The content in italics is between <i> and </i>, i.e. between (start_index + 3) and end_index.

						strncat(temp, (context->subline) + start_index + 3, end_index - start_index - 3); //the content in italics

						strcat(final,temp);	//attaching to final sentence.
						
						if (style == 1)
							strcpy(temp,"<style tts:backgroundColor=\"#000000FF\" tts:fontSize=\"18px\" tts:fontStyle=\"italic\"/> </span>");
						
						else if(style == 2)
							strcpy(temp,"<style tts:backgroundColor=\"#000000FF\" tts:fontSize=\"18px\" tts:fontWeight=\"bold\"/> </span>");

						else
							strcpy(temp,"<style tts:backgroundColor=\"#000000FF\" tts:fontSize=\"18px\" tts:textDecoration=\"underline\"/> </span>");
						
						strcat(final,temp);		// adding appropriate style tag.

						sprintf(temp,"%s", (context->subline) + end_index + 4);	//finding remaining sentence.

						strcat (final,temp);	//adding remaining sentence.

					}
				}

			    else	//No style or Font Color
			    {
				   
					start = strstr((context->subline), "<font color");  //spec : <font color="#xxxxxx"> cc </font>
					if(start!=NULL) //font color attribute is present
					{
						unsigned char *end = strstr((context->subline), "</font>");
						if(end == NULL)
						{
							//Incorrect styling, writing as it is
							strcpy(final,(context->subline));
						}

						else
						{
							int start_index = start-(context->subline);
							int end_index = end-(context->subline);

							strncat(final,(context->subline),start_index);     // copying content before opening tag e.g. <font ..> 

							strcat(final,"<span>");                 //adding <span> : replacement of <font ..>


							unsigned char *temp_pointer = strchr((context->subline),'#');     //locating color code

							unsigned char color_code[7];
							strncpy(color_code, temp_pointer + 1, 6);		//obtained color code
							color_code[6]='\0';
							


							temp_pointer = strchr((context->subline), '>');                   //The content is in between <font ..> and </font>

							strncat(temp, temp_pointer + 1, end_index - (temp_pointer - (context->subline) + 1));

							strcat(final,temp);	//attaching to final sentence.

							sprintf(temp,"<style tts:backgroundColor=\"#FFFF00FF\" tts:color=\"%s\" tts:fontSize=\"18px\"/></span>",color_code);

							strcat(final,temp);	//adding font color tag

							sprintf(temp,"%s", (context->subline) + end_index + 7);   	//finding remaining sentence.

							strcat(final,temp);	//adding remaining sentence
						}
					}

					else
					{
						//NO styling, writing as it is
						strcpy(final,(context->subline));
					}

				}


				write(context->out->fh, final, strlen(final));


				write(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
				context->trim_subs=old_trim_subs;
				
				sprintf ((char *) str,"        <style tts:backgroundColor=\"#000000FF\" tts:fontSize=\"18px\"/></span>\n      </p>\n");
				if (context->encoding!=CCX_ENC_UNICODE)
				{
					dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
				}
				used = encode_line(context, context->buffer,(unsigned char *) str);
				write (context->out->fh, context->buffer, used);

				if (context->encoding!=CCX_ENC_UNICODE)
				{
					dbg_print(CCX_DMT_DECODER_608, "\r%s\n", str);
				}
				used = encode_line(context, context->buffer,(unsigned char *) str);
				//write (wb->fh, enc_buffer,enc_buffer_used);
				
				freep(&final);
				freep(&temp);
	
			}
		}
	}

	return wrote_something;
}
