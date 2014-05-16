#include "ccextractor.h"


/* The timing here is not PTS based, but output based, i.e. user delay must be accounted for
   if there is any */
void write_stringz_as_srt (char *string, struct s_context_cc608 *context, LLONG ms_start, LLONG ms_end)
{
    unsigned h1,m1,s1,ms1;
    unsigned h2,m2,s2,ms2;

    mstotime (ms_start,&h1,&m1,&s1,&ms1);
    mstotime (ms_end-1,&h2,&m2,&s2,&ms2); // -1 To prevent overlapping with next line.
    char timeline[128];   
    context->srt_counter++;
	sprintf(timeline, "%u\r\n", context->srt_counter);
    enc_buffer_used=encode_line (enc_buffer,(unsigned char *) timeline);
	write(context->out->fh, enc_buffer, enc_buffer_used);
    sprintf (timeline, "%02u:%02u:%02u,%03u --> %02u:%02u:%02u,%03u\r\n",
        h1,m1,s1,ms1, h2,m2,s2,ms2);
    enc_buffer_used=encode_line (enc_buffer,(unsigned char *) timeline);
    dbg_print(CCX_DMT_608, "\n- - - SRT caption - - -\n");
    dbg_print(CCX_DMT_608, "%s",timeline);
    
	write(context->out->fh, enc_buffer, enc_buffer_used);
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
            dbg_print(CCX_DMT_608, "\r");
            dbg_print(CCX_DMT_608, "%s\n",subline);
        }
		write(context->out->fh, el, u);
		write(context->out->fh, encoded_crlf, encoded_crlf_length);
        begin+= strlen ((const char *) begin)+1;
    }

    dbg_print(CCX_DMT_608, "- - - - - - - - - - - -\r\n");
   
	write(context->out->fh, encoded_crlf, encoded_crlf_length);
	free(el);
}

int write_cc_buffer_as_srt(struct eia608_screen *data, struct s_context_cc608 *context)
{
    unsigned h1,m1,s1,ms1;
    unsigned h2,m2,s2,ms2;
	LLONG ms_start, ms_end;
    int wrote_something = 0;
	ms_start = context->current_visible_start_ms;

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

	ms_end=get_visible_end()+subs_delay;		

    mstotime (ms_start,&h1,&m1,&s1,&ms1);
    mstotime (ms_end-1,&h2,&m2,&s2,&ms2); // -1 To prevent overlapping with next line.
    char timeline[128];   
	context->srt_counter++;
	sprintf(timeline, "%u\r\n", context->srt_counter);
    enc_buffer_used=encode_line (enc_buffer,(unsigned char *) timeline);
	write(context->out->fh, enc_buffer, enc_buffer_used);
    sprintf (timeline, "%02u:%02u:%02u,%03u --> %02u:%02u:%02u,%03u\r\n",
        h1,m1,s1,ms1, h2,m2,s2,ms2);
    enc_buffer_used=encode_line (enc_buffer,(unsigned char *) timeline);

	dbg_print(CCX_DMT_608, "\n- - - SRT caption ( %d) - - -\n", context->srt_counter);
    dbg_print(CCX_DMT_608, "%s",timeline);

    write (context->out->fh, enc_buffer,enc_buffer_used);		
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
					while (colon_pos<CC608_SCREEN_WIDTH &&
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
                dbg_print(CCX_DMT_608, "\r");
                dbg_print(CCX_DMT_608, "%s\n",subline);
            }
			write(context->out->fh, subline, length);
			write(context->out->fh, encoded_crlf, encoded_crlf_length);
            wrote_something=1;
            // fprintf (wb->fh,encoded_crlf);
        }
    }
    dbg_print(CCX_DMT_608, "- - - - - - - - - - - -\r\n");
    
    // fprintf (wb->fh, encoded_crlf);
    write (context->out->fh, encoded_crlf, encoded_crlf_length);
    return wrote_something;
}
