#include "ccextractor.h"
#include "utility.h"
#include "ccx_encoders_common.h"
//extern unsigned char encoded_crlf[16];

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