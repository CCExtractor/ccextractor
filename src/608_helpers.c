#include "ccextractor.h"
#include "utility.h"
#include "ccx_encoders_common.h"
//extern unsigned char encoded_crlf[16];

// Encodes a generic string. Note that since we use the encoders for closed caption
// data, text would have to be encoded as CCs... so using special characters here
// it's a bad idea. 
unsigned encode_line (unsigned char *buffer, unsigned char *text)
{ 
    unsigned bytes=0;
    while (*text)
    {		
        switch (ccx_options.encoding)
        {
            case CCX_ENC_UTF_8:
            case CCX_ENC_LATIN_1:
                *buffer=*text;
                bytes++;
                buffer++;
                break;
            case CCX_ENC_UNICODE:				
                *buffer=*text;				
                *(buffer+1)=0;
                bytes+=2;				
                buffer+=2;
                break;
        }		
        text++;
    }
    return bytes;
}

void correct_case(int line_num,struct eia608_screen *data)
{
	char delim[64] = {
		' ' ,'\n','\r', 0x89,0x99,
		'!' , '"', '#', '%' , '&',
		'\'', '(', ')', ';' , '<',
		'=' , '>', '?', '[' ,'\\',
		']' , '*', '+', ',' , '-',
		'.' , '/', ':', '^' , '_',
		'{' , '|', '}', '~' ,'\0' };

	char *line = strdup(((char*)data->characters[line_num]));
	char *oline = (char*)data->characters[line_num];
	char *c = strtok(line,delim);
	do
	{
		char **index = bsearch(&c,spell_lower,spell_words,sizeof(*spell_lower),string_cmp);

		if(index)
		{
			char *correct_c = *(spell_correct + (index - spell_lower));
			size_t len=strlen (correct_c);
			memcpy(oline + (c - line),correct_c,len);
		}
	} while ( ( c = strtok(NULL,delim) ) != NULL );
	free(line);
}

void capitalize (int line_num, struct eia608_screen *data)
{	
	for (int i = 0; i<CCX_DECODER_608_SCREEN_WIDTH; i++)
    {
        switch (data->characters[line_num][i])
        {
            case ' ': 
            case 0x89: // This is a transparent space
            case '-':
                break; 
            case '.': // Fallthrough
            case '?': // Fallthrough
            case '!':
            case ':':
                new_sentence=1;
                break;
            default:
                if (new_sentence)			
                    data->characters[line_num][i]=cctoupper (data->characters[line_num][i]);
                else
                    data->characters[line_num][i]=cctolower (data->characters[line_num][i]);
                new_sentence=0;
                break;
        }
    }
}

void find_limit_characters (unsigned char *line, int *first_non_blank, int *last_non_blank)
{
    *last_non_blank=-1;
    *first_non_blank=-1;
	for (int i = 0; i<CCX_DECODER_608_SCREEN_WIDTH; i++)
    {
        unsigned char c=line[i];
        if (c!=' ' && c!=0x89)
        {
            if (*first_non_blank==-1)
                *first_non_blank=i;
            *last_non_blank=i;
        }
    }
}

unsigned get_decoder_line_basic (unsigned char *buffer, int line_num, struct eia608_screen *data)
{
    unsigned char *line = data->characters[line_num];
    int last_non_blank=-1;
    int first_non_blank=-1;
    unsigned char *orig=buffer; // Keep for debugging
	find_limit_characters (line, &first_non_blank, &last_non_blank);
	if (!ccx_options.trim_subs)	
		first_non_blank=0;
   
    if (first_non_blank==-1)
    {
        *buffer=0;
        return 0;
    }

    int bytes=0;
    for (int i=first_non_blank;i<=last_non_blank;i++)
    {
        char c=line[i];
        switch (ccx_options.encoding)
        {
            case CCX_ENC_UTF_8:
                bytes=get_char_in_utf_8 (buffer,c);
                break;
            case CCX_ENC_LATIN_1:
                get_char_in_latin_1 (buffer,c);
                bytes=1;
                break;
            case CCX_ENC_UNICODE:
                get_char_in_unicode (buffer,c);
                bytes=2;				
                break;
        }
        buffer+=bytes;
    }
    *buffer=0;
    return (unsigned) (buffer-orig); // Return length
}

unsigned get_decoder_line_encoded_for_gui (unsigned char *buffer, int line_num, struct eia608_screen *data)
{
    unsigned char *line = data->characters[line_num];	
    unsigned char *orig=buffer; // Keep for debugging
    int first=0, last=31;
    find_limit_characters(line,&first,&last);
    for (int i=first;i<=last;i++)
    {	
        get_char_in_latin_1 (buffer,line[i]);
        buffer++;
    }
    *buffer=0;
    return (unsigned) (buffer-orig); // Return length

}

unsigned char *close_tag (unsigned char *buffer, char *tagstack, char tagtype, int *punderlined, int *pitalics, int *pchanged_font)
{		
	for (int l=strlen (tagstack)-1; l>=0;l--)
	{
		char cur=tagstack[l];
		switch (cur)
		{
			case 'F':
				buffer+= encode_line (buffer,(unsigned char *) "</font>");
				(*pchanged_font)--;
				break;
			case 'U':
				buffer+=encode_line (buffer, (unsigned char *) "</u>");
				(*punderlined)--;
				break;
			case 'I':
				buffer+=encode_line (buffer, (unsigned char *) "</i>");
				(*pitalics)--;
				break;
		}
		tagstack[l]=0; // Remove from stack
		if (cur==tagtype) // We closed up to the required tag, done
			return buffer;
	}
	if (tagtype!='A') // All
		fatal (CCX_COMMON_EXIT_BUG_BUG, "Mismatched tags in encoding, this is a bug, please report");
	return buffer;
}

unsigned get_decoder_line_encoded (unsigned char *buffer, int line_num, struct eia608_screen *data)
{
    int col = COL_WHITE;
    int underlined = 0;
    int italics = 0;	
	int changed_font=0;
	char tagstack[128]=""; // Keep track of opening/closing tags

    unsigned char *line = data->characters[line_num];	
    unsigned char *orig=buffer; // Keep for debugging
    int first=0, last=31;
    if (ccx_options.trim_subs)
        find_limit_characters(line,&first,&last);
    for (int i=first;i<=last;i++)
    {	
        // Handle color
        int its_col = data->colors[line_num][i];
        if (its_col != col  && !ccx_options.nofontcolor && 
			!(col==COL_USERDEFINED && its_col==COL_WHITE)) // Don't replace user defined with white
        {
			if (changed_font)
				buffer = close_tag(buffer,tagstack,'F',&underlined,&italics,&changed_font);
            // Add new font tag
            buffer+=encode_line (buffer, (unsigned char*) color_text[its_col][1]);
            if (its_col==COL_USERDEFINED)
            {
                // The previous sentence doesn't copy the whole 
                // <font> tag, just up to the quote before the color
                buffer+=encode_line (buffer, (unsigned char*) usercolor_rgb);
                buffer+=encode_line (buffer, (unsigned char*) "\">");				
            }
			if (color_text[its_col][1][0]) // That means a <font> was added to the buffer
			{
				strcat (tagstack,"F");
				changed_font++;
			}
            col = its_col;
        }
        // Handle underlined
        int is_underlined = data->fonts[line_num][i] & FONT_UNDERLINED;
        if (is_underlined && underlined==0 && !ccx_options.notypesetting) // Open underline
        {
            buffer+=encode_line (buffer, (unsigned char *) "<u>");
			strcat (tagstack,"U");
			underlined++;
        }
        if (is_underlined==0 && underlined && !ccx_options.notypesetting) // Close underline
        {
			buffer = close_tag(buffer,tagstack,'U',&underlined,&italics,&changed_font);
        }         
        // Handle italics
        int has_ita = data->fonts[line_num][i] & FONT_ITALICS;		
        if (has_ita && italics==0 && !ccx_options.notypesetting) // Open italics
        {
            buffer+=encode_line (buffer, (unsigned char *) "<i>");
			strcat (tagstack,"I");
			italics++;
        }
        if (has_ita==0 && italics && !ccx_options.notypesetting) // Close italics
        {
			buffer = close_tag(buffer,tagstack,'I',&underlined,&italics,&changed_font);            
        }         
        int bytes=0;
        switch (ccx_options.encoding)
        {
            case CCX_ENC_UTF_8:
                bytes=get_char_in_utf_8 (buffer,line[i]);
                break;
            case CCX_ENC_LATIN_1:
                get_char_in_latin_1 (buffer,line[i]);
                bytes=1;
                break;
            case CCX_ENC_UNICODE:
                get_char_in_unicode (buffer,line[i]);
                bytes=2;				
                break;
        }
        buffer+=bytes;        
    }
	buffer = close_tag(buffer,tagstack,'A',&underlined,&italics,&changed_font);
	if (underlined || italics || changed_font)
		fatal (CCX_COMMON_EXIT_BUG_BUG, "Not all tags closed in encoding, this is a bug, please report.\n");
    *buffer=0;
    return (unsigned) (buffer-orig); // Return length
}


void delete_all_lines_but_current(ccx_decoder_608_context *context, struct eia608_screen *data, int row)
{
    for (int i=0;i<15;i++)
    {
        if (i!=row)
        {
			memset(data->characters[i], ' ', CCX_DECODER_608_SCREEN_WIDTH);
			data->characters[i][CCX_DECODER_608_SCREEN_WIDTH] = 0;
			memset(data->colors[i], context->settings.default_color, CCX_DECODER_608_SCREEN_WIDTH + 1);
			memset(data->fonts[i], FONT_REGULAR, CCX_DECODER_608_SCREEN_WIDTH + 1);
            data->row_used[i]=0;        
        }
    }
}

void fprintf_encoded (struct encoder_ctx *ctx,FILE *fh, const char *string)
{
	int used;
	REQUEST_BUFFER_CAPACITY(ctx,strlen (string)*3);
	used=encode_line (ctx->buffer,(unsigned char *) string);
	fwrite (ctx->buffer,used,1,fh);
}

void write_cc_buffer_to_gui(struct eia608_screen *data, struct encoder_ctx *context)
{
    unsigned h1,m1,s1,ms1;
    unsigned h2,m2,s2,ms2;    
	LLONG ms_start;
	int with_data=0;

	for (int i=0;i<15;i++)
    {
        if (data->row_used[i])
			with_data=1;
    }
	if (!with_data)
		return;

    ms_start= data->start_time;

    ms_start+=subs_delay;
    if (ms_start<0) // Drop screens that because of subs_delay start too early
        return;
    int time_reported=0;    
    for (int i=0;i<15;i++)
    {
        if (data->row_used[i])
        {
            fprintf (stderr, "###SUBTITLE#");
            if (!time_reported)
            {
                LLONG ms_end = data->end_time;
                mstotime (ms_start,&h1,&m1,&s1,&ms1);
                mstotime (ms_end-1,&h2,&m2,&s2,&ms2); // -1 To prevent overlapping with next line.
                // Note, only MM:SS here as we need to save space in the preview window
                fprintf (stderr, "%02u:%02u#%02u:%02u#",
                    h1*60+m1,s1, h2*60+m2,s2);
                time_reported=1;
            }
            else
                fprintf (stderr, "##");
            
            // We don't capitalize here because whatever function that was used
            // before to write to file already took care of it.
            int length = get_decoder_line_encoded_for_gui (subline, i, data);
            fwrite (subline, 1, length, stderr);
            fwrite ("\n",1,1,stderr);
        }
    }
    fflush (stderr);
}
