#include "ccextractor.h"

static const int     rowdata[] = {11,-1,1,2,3,4,12,13,14,15,5,6,7,8,9,10};
// Relationship between the first PAC byte and the row number
int in_xds_mode=0; 

#define INITIAL_ENC_BUFFER_CAPACITY		2048

unsigned char *enc_buffer=NULL; // Generic general purpose buffer
unsigned char str[2048]; // Another generic general purpose buffer
unsigned enc_buffer_used;
unsigned enc_buffer_capacity;



LLONG minimum_fts=0; // No screen should start before this FTS

int general_608_init (void)
{
    enc_buffer=(unsigned char *) malloc (INITIAL_ENC_BUFFER_CAPACITY);
    if (enc_buffer==NULL)
        return -1;
    enc_buffer_capacity=INITIAL_ENC_BUFFER_CAPACITY;
    return 0;
}

// Preencoded strings
unsigned char encoded_crlf[16]; 
unsigned int encoded_crlf_length;
unsigned char encoded_br[16];
unsigned int encoded_br_length;

unsigned char *subline; // Temp storage for .srt lines
int new_sentence=1; // Capitalize next letter?

// Default color
unsigned char usercolor_rgb[8]="";


static const char *sami_header= // TODO: Revise the <!-- comments
"<SAMI>\n\
<HEAD>\n\
<STYLE TYPE=\"text/css\">\n\
<!--\n\
P {margin-left: 16pt; margin-right: 16pt; margin-bottom: 16pt; margin-top: 16pt;\n\
text-align: center; font-size: 18pt; font-family: arial; font-weight: bold; color: #f0f0f0;}\n\
.UNKNOWNCC {Name:Unknown; lang:en-US; SAMIType:CC;}\n\
-->\n\
</STYLE>\n\
</HEAD>\n\n\
<BODY>\n";

static const char *smptett_header = 
"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n\
<tt xmlns=\"http://www.w3.org/ns/ttml\" xml:lang=\"en\">\n\
<body>\n<div>\n" ;

static const char *command_type[] =
{
    "Unknown",
    "EDM - EraseDisplayedMemory",
    "RCL - ResumeCaptionLoading",
    "EOC - End Of Caption",
    "TO1 - Tab Offset, 1 column",
    "TO2 - Tab Offset, 2 column",
    "TO3 - Tab Offset, 3 column",
    "RU2 - Roll up 2 rows",
    "RU3 - Roll up 3 rows",
    "RU4 - Roll up 4 rows",
    "CR  - Carriage Return",
    "ENM - Erase non-displayed memory",
    "BS  - Backspace",
    "RTD - Resume Text Display",
	"AOF - Not Used (Alarm Off)",
	"AON - Not Used (Alarm On)",
	"DER - Delete to End of Row",
	"RDC - Resume Direct Captioning",
	"RU1 - Fake Roll up 1 rows"
};

static const char *font_text[]=
{
    "regular",
    "italics",
    "underlined",
    "underlined italics"
};

static const char *cc_modes_text[]=
{
    "Pop-Up captions"
};

const char *color_text[][2]=
{
    {"white",""},
    {"green","<font color=\"#00ff00\">"},
    {"blue","<font color=\"#0000ff\">"},
    {"cyan","<font color=\"#00ffff\">"},
    {"red","<font color=\"#ff0000\">"},
    {"yellow","<font color=\"#ffff00\">"},
    {"magenta","<font color=\"#ff00ff\">"},
    {"userdefined","<font color=\""},
    {"black",""},
    {"transparent",""}
};


void clear_eia608_cc_buffer (struct eia608_screen *data)
{
    for (int i=0;i<15;i++)
    {
        memset(data->characters[i],' ',CC608_SCREEN_WIDTH);
        data->characters[i][CC608_SCREEN_WIDTH]=0;		
        memset (data->colors[i],ccx_options.cc608_default_color,CC608_SCREEN_WIDTH+1);
        memset (data->fonts[i],FONT_REGULAR,CC608_SCREEN_WIDTH+1); 
        data->row_used[i]=0;        
    }
    data->empty=1;
}

void init_eia608 (struct eia608 *data)
{
    data->cursor_column=0;
    data->cursor_row=0;
    clear_eia608_cc_buffer (&data->buffer1);
    clear_eia608_cc_buffer (&data->buffer2);
    data->visible_buffer=1;
    data->last_c1=0;
    data->last_c2=0;
    data->mode=MODE_POPON;
    // data->current_visible_start_cc=0;
    data->current_visible_start_ms=0;
    data->srt_counter=0;
    data->screenfuls_counter=0;
    data->channel=1;	
    data->color=ccx_options.cc608_default_color;
    data->font=FONT_REGULAR;
    data->rollup_base_row=14;
	data->ts_start_of_current_line=-1;
	data->ts_last_char_received=-1;
	data->new_channel=1;
}

eia608_screen *get_writing_buffer (struct ccx_s_write *wb)
{
    eia608_screen *use_buffer=NULL;
    switch (wb->data608->mode)
    {
        case MODE_POPON: // Write on the non-visible buffer
            if (wb->data608->visible_buffer==1)
                use_buffer = &wb->data608->buffer2;
            else
                use_buffer = &wb->data608->buffer1;
            break;
		case MODE_FAKE_ROLLUP_1: // Write directly to screen
        case MODE_ROLLUP_2: 
        case MODE_ROLLUP_3:
        case MODE_ROLLUP_4:
		case MODE_PAINTON:
		case MODE_TEXT:
			// TODO: Fix this. Text uses a different buffer, and contains non-program information.
            if (wb->data608->visible_buffer==1)
                use_buffer = &wb->data608->buffer1;
            else
                use_buffer = &wb->data608->buffer2;
            break;
        default:
            fatal (EXIT_BUG_BUG, "Caption mode has an illegal value at get_writing_buffer(), this is a bug.\n");
    }
    return use_buffer;
}

void delete_to_end_of_row (struct ccx_s_write *wb)
{
    if (wb->data608->mode!=MODE_TEXT) 
    {		
		eia608_screen * use_buffer=get_writing_buffer(wb);
		for (int i=wb->data608->cursor_column; i<=31 ; i++)
		{
			// TODO: This can change the 'used' situation of a column, so we'd
			// need to check and correct.
	        use_buffer->characters[wb->data608->cursor_row][i]=' ';			
			use_buffer->colors[wb->data608->cursor_row][i]=ccx_options.cc608_default_color;
			use_buffer->fonts[wb->data608->cursor_row][i]=wb->data608->font;	
		}
	}
}

void write_char (const unsigned char c, struct ccx_s_write *wb)
{
    if (wb->data608->mode!=MODE_TEXT) 
    {		
        eia608_screen * use_buffer=get_writing_buffer(wb);
        /* printf ("\rWriting char [%c] at %s:%d:%d\n",c,
        use_buffer == &wb->data608->buffer1?"B1":"B2",
        wb->data608->cursor_row,wb->data608->cursor_column); */
        use_buffer->characters[wb->data608->cursor_row][wb->data608->cursor_column]=c;
        use_buffer->colors[wb->data608->cursor_row][wb->data608->cursor_column]=wb->data608->color;
        use_buffer->fonts[wb->data608->cursor_row][wb->data608->cursor_column]=wb->data608->font;	
        use_buffer->row_used[wb->data608->cursor_row]=1;

        if (use_buffer->empty)
        {
            if (MODE_POPON != wb->data608->mode)
                wb->data608->current_visible_start_ms = get_visible_start();
        }
        use_buffer->empty=0;

        if (wb->data608->cursor_column<31)
            wb->data608->cursor_column++;
		if (wb->data608->ts_start_of_current_line == -1) 
			wb->data608->ts_start_of_current_line=get_fts();
		wb->data608->ts_last_char_received=get_fts();
    }

}

/* Handle MID-ROW CODES. */
void handle_text_attr (const unsigned char c1, const unsigned char c2, struct ccx_s_write *wb)
{
    // Handle channel change
    wb->data608->channel=wb->data608->new_channel;
    if (wb->data608->channel!=ccx_options.cc_channel)
        return;
    dbg_print(CCX_DMT_608, "\r608: text_attr: %02X %02X",c1,c2);
    if ( ((c1!=0x11 && c1!=0x19) ||
        (c2<0x20 || c2>0x2f)))
    {
        dbg_print(CCX_DMT_608, "\rThis is not a text attribute!\n");
    }
    else
    {
        int i = c2-0x20;
        wb->data608->color=pac2_attribs[i][0];
        wb->data608->font=pac2_attribs[i][1];
        dbg_print(CCX_DMT_608, "  --  Color: %s,  font: %s\n",
            color_text[wb->data608->color][0],
            font_text[wb->data608->font]);
        // Mid-row codes should put a non-transparent space at the current position
        // and advance the cursor
        //so use write_char
        write_char(0x20, wb);
    }
}


void write_subtitle_file_footer (struct ccx_s_write *wb)
{
    switch (ccx_options.write_format)
    {
        case CCX_OF_SAMI:
            sprintf ((char *) str,"</BODY></SAMI>\n");
            if (ccx_options.encoding!=CCX_ENC_UNICODE)
            {
                dbg_print(CCX_DMT_608, "\r%s\n", str);
            }
            enc_buffer_used=encode_line (enc_buffer,(unsigned char *) str);
            write (wb->fh, enc_buffer,enc_buffer_used);
            break;
		case CCX_OF_SMPTETT:
			sprintf ((char *) str,"</div></body></tt>\n");
			if (ccx_options.encoding!=CCX_ENC_UNICODE)
			{
				dbg_print(CCX_DMT_608, "\r%s\n", str);
			}
			enc_buffer_used=encode_line (enc_buffer,(unsigned char *) str);
			write (wb->fh, enc_buffer,enc_buffer_used);
			break;
        case CCX_OF_SPUPNG:
            write_spumux_footer(wb);
            break;
		default: // Nothing to do, no footer on this format
            break;
    }
}


void write_subtitle_file_header (struct ccx_s_write *wb)
{
    switch (ccx_options.write_format)
    {
        case CCX_OF_SRT: // Subrip subtitles have no header
            break; 
        case CCX_OF_SAMI: // This header brought to you by McPoodle's CCASDI  
            //fprintf_encoded (wb->fh, sami_header);
            REQUEST_BUFFER_CAPACITY(strlen (sami_header)*3);
            enc_buffer_used=encode_line (enc_buffer,(unsigned char *) sami_header);
            write (wb->fh, enc_buffer,enc_buffer_used);
            break;
        case CCX_OF_SMPTETT: // This header brought to you by McPoodle's CCASDI  
			//fprintf_encoded (wb->fh, sami_header);
			REQUEST_BUFFER_CAPACITY(strlen (smptett_header)*3);
            enc_buffer_used=encode_line (enc_buffer,(unsigned char *) smptett_header);
            write (wb->fh, enc_buffer,enc_buffer_used);
            break;
        case CCX_OF_RCWT: // Write header
            write (wb->fh, rcwt_header, sizeof(rcwt_header));
            break;
        case CCX_OF_SPUPNG:
            write_spumux_header(wb);
            break;
        case CCX_OF_TRANSCRIPT: // No header. Fall thru
        default:
            break;
    }
}

void write_cc_line_as_transcript (struct eia608_screen *data, struct ccx_s_write *wb, int line_number)
{
	unsigned h1,m1,s1,ms1;
	unsigned h2,m2,s2,ms2;
    if (ccx_options.sentence_cap)
    {
        capitalize (line_number,data);
        correct_case(line_number,data);
    }
    int length = get_decoder_line_basic (subline, line_number, data);
    if (ccx_options.encoding!=CCX_ENC_UNICODE)
    {
        dbg_print(CCX_DMT_608, "\r");
        dbg_print(CCX_DMT_608, "%s\n",subline);
    }
    if (length>0)
    {
		if (timestamps_on_transcript)
		{
			const char *mode="???";
			switch (wb->data608->mode)
			{
				case MODE_POPON:
					mode="POP";
					break;
				case MODE_FAKE_ROLLUP_1:
					mode="RU1";
					break;
				case MODE_ROLLUP_2:
					mode="RU2";
					break;
				case MODE_ROLLUP_3:
					mode="RU3";
					break;
				case MODE_ROLLUP_4:
					mode="RU4";
					break;
				case MODE_TEXT:
					mode="TXT";
					break;
				case MODE_PAINTON:
					mode="PAI";
					break;
			}

			if (wb->data608->ts_start_of_current_line == -1) 
			{
				// CFS: Means that the line has characters but we don't have a timestamp for the first one. Since the timestamp
				// is set for example by the write_char function, it possible that we don't have one in empty lines (unclear)
				// For now, let's not consider this a bug as before and just return.
				// fatal (EXIT_BUG_BUG, "Bug in timedtranscript (ts_start_of_current_line==-1). Please report.");
				return;
			}
			if (ccx_options.ucla_settings)
			{
				mstotime (wb->data608->ts_start_of_current_line+subs_delay,&h1,&m1,&s1,&ms1);
				mstotime (get_fts()+subs_delay,&h2,&m2,&s2,&ms2);				

                // SSC-1182 BEGIN
                // Changed output format to be more like ZVBI

				char buffer[80];
                time_t start_time_int = (wb->data608->ts_start_of_current_line+subs_delay)/1000;
                int start_time_dec = (wb->data608->ts_start_of_current_line+subs_delay)%1000;
                struct tm *start_time_struct = gmtime(&start_time_int);
                strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", start_time_struct);
                fdprintf(wb->fh, "%s.%03d|", buffer, start_time_dec);

                time_t end_time_int = (get_fts()+subs_delay)/1000;
                int end_time_dec = (get_fts()+subs_delay)%1000;
                struct tm *end_time_struct = gmtime(&end_time_int);
                strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", end_time_struct);
                fdprintf(wb->fh, "%s.%03d|", buffer, end_time_dec);

				fdprintf(wb->fh, "CC%d|%s|", wb->my_field==1?wb->data608->channel:wb->data608->channel+2, // Data from field 2 is CC3 or 4
					mode);
                // SSC-1182 END
			}
			else
			{
				char buf1[80], buf2[80];
				char timeline[256];   
				millis_to_date (wb->data608->ts_start_of_current_line+subs_delay,buf1);
				millis_to_date (get_fts()+subs_delay, buf2);
				sprintf (timeline, "%s|%s|%s|",
					buf1, buf2,mode);
				enc_buffer_used=encode_line (enc_buffer,(unsigned char *) timeline);
				write (wb->fh, enc_buffer,enc_buffer_used);
			}			
		}
        write (wb->fh, subline, length);
        write (wb->fh, encoded_crlf, encoded_crlf_length);
    }
    // fprintf (wb->fh,encoded_crlf);
}

int write_cc_buffer_as_transcript (struct eia608_screen *data, struct ccx_s_write *wb)
{
    int wrote_something = 0;
	wb->data608->ts_start_of_current_line=wb->data608->current_visible_start_ms;
    dbg_print(CCX_DMT_608, "\n- - - TRANSCRIPT caption - - -\n");        
    
    for (int i=0;i<15;i++)
    {
        if (data->row_used[i])
        {		
            write_cc_line_as_transcript (data,wb, i);
        }
        wrote_something=1;
    }
    dbg_print(CCX_DMT_608, "- - - - - - - - - - - -\r\n");
    return wrote_something;
}




struct eia608_screen *get_current_visible_buffer (struct ccx_s_write *wb)
{
    struct eia608_screen *data;
    if (wb->data608->visible_buffer==1)
        data = &wb->data608->buffer1;
    else
        data = &wb->data608->buffer2;
    return data;
}


int write_cc_buffer (struct ccx_s_write *wb)
{
    struct eia608_screen *data;
    int wrote_something=0;
    if (ccx_options.screens_to_process!=-1 && 
		wb->data608->screenfuls_counter>=ccx_options.screens_to_process)
    {
        // We are done. 
        processed_enough=1;
        return 0;
    }
    if (wb->data608->visible_buffer==1)
        data = &wb->data608->buffer1;
    else
        data = &wb->data608->buffer2;

    if (!data->empty)
    {
		if (wb->data608->mode==MODE_FAKE_ROLLUP_1 && // Use the actual start of data instead of last buffer change
			wb->data608->ts_start_of_current_line!=-1)
			wb->data608->current_visible_start_ms=wb->data608->ts_start_of_current_line;

        new_sentence=1;
        switch (ccx_options.write_format)
        {
            case CCX_OF_SRT:
                if (!startcredits_displayed && ccx_options.start_credits_text!=NULL)
                    try_to_add_start_credits(wb);            
                wrote_something = write_cc_buffer_as_srt (data, wb);
                break;
            case CCX_OF_SAMI:
                if (!startcredits_displayed && ccx_options.start_credits_text!=NULL)
                    try_to_add_start_credits(wb);
                wrote_something = write_cc_buffer_as_sami (data,wb);
                break;
            case CCX_OF_SMPTETT:
				if (!startcredits_displayed && ccx_options.start_credits_text!=NULL)
					try_to_add_start_credits(wb);
				wrote_something = write_cc_buffer_as_smptett (data,wb);
                break;
            case CCX_OF_TRANSCRIPT:
                wrote_something = write_cc_buffer_as_transcript (data,wb);
                break;
            case CCX_OF_SPUPNG:
                wrote_something = write_cc_buffer_as_spupng (data,wb);
                break;
            default: 
                break;
        }
        if (wrote_something)
            last_displayed_subs_ms=get_fts()+subs_delay; 

        if (ccx_options.gui_mode_reports)
            write_cc_buffer_to_gui (data,wb);
    }
    return wrote_something;
}

// Check if a rollup would cause a line to go off the visible area
int check_roll_up (struct ccx_s_write *wb)
{
	int keep_lines=0;
	int firstrow=-1, lastrow=-1;
    eia608_screen *use_buffer;
    if (wb->data608->visible_buffer==1)
        use_buffer = &wb->data608->buffer1;
    else
        use_buffer = &wb->data608->buffer2;

	switch (wb->data608->mode)
    {
		case MODE_FAKE_ROLLUP_1:
            keep_lines=1;
            break;
        case MODE_ROLLUP_2:
            keep_lines=2;
            break;
        case MODE_ROLLUP_3:
            keep_lines=3;
            break;
        case MODE_ROLLUP_4:
            keep_lines=4;
            break;
		case MODE_TEXT:
			keep_lines=7; // CFS: can be 7 to 15 according to the handbook. No idea how this is selected.
			break;
        default: // Shouldn't happen
            return 0;
            break;
    }
	if (use_buffer->row_used[0]) // If top line is used it will go off the screen no matter what
		return 1;
    int rows_orig=0; // Number of rows in use right now
    for (int i=0;i<15;i++)
    {
        if (use_buffer->row_used[i])
        {
            rows_orig++;
            if (firstrow==-1)
                firstrow=i;
            lastrow=i;
        }
    }
    if (lastrow==-1) // Empty screen, nothing to rollup
        return 0;
	if ((lastrow-firstrow+1)>=keep_lines)
		return 1; // We have the roll-up area full, so yes

    if ((firstrow-1)<=wb->data608->cursor_row-keep_lines) // Roll up will delete those lines.
		return 1;    
	return 0;
}

// Roll-up: Returns true if a line was rolled over the visible area (it dissapears from screen), false
// if the rollup didn't delete any line.
int roll_up(struct ccx_s_write *wb)
{
    eia608_screen *use_buffer;
    if (wb->data608->visible_buffer==1)
        use_buffer = &wb->data608->buffer1;
    else
        use_buffer = &wb->data608->buffer2;
    int keep_lines;
    switch (wb->data608->mode)
    {
		case MODE_FAKE_ROLLUP_1:
            keep_lines=1;
            break;
        case MODE_ROLLUP_2:
            keep_lines=2;
            break;
        case MODE_ROLLUP_3:
            keep_lines=3;
            break;
        case MODE_ROLLUP_4:
            keep_lines=4;
            break;
		case MODE_TEXT:
			keep_lines=7; // CFS: can be 7 to 15 according to the handbook. No idea how this is selected.
			break;
        default: // Shouldn't happen
            keep_lines=0;
            break;
    }
    int firstrow=-1, lastrow=-1;
    // Look for the last line used
    int rows_orig=0; // Number of rows in use right now
    for (int i=0;i<15;i++)
    {
        if (use_buffer->row_used[i])
        {
            rows_orig++;
            if (firstrow==-1)
                firstrow=i;
            lastrow=i;
        }
    }
    
    dbg_print(CCX_DMT_608, "\rIn roll-up: %d lines used, first: %d, last: %d\n", rows_orig, firstrow, lastrow);

    if (lastrow==-1) // Empty screen, nothing to rollup
        return 0;

    for (int j=lastrow-keep_lines+1;j<lastrow; j++)
    {
        if (j>=0)
        {
            memcpy (use_buffer->characters[j],use_buffer->characters[j+1],CC608_SCREEN_WIDTH+1);
            memcpy (use_buffer->colors[j],use_buffer->colors[j+1],CC608_SCREEN_WIDTH+1);
            memcpy (use_buffer->fonts[j],use_buffer->fonts[j+1],CC608_SCREEN_WIDTH+1);
            use_buffer->row_used[j]=use_buffer->row_used[j+1];
        }
    }
    for (int j=0;j<(1+wb->data608->cursor_row-keep_lines);j++)
    {
        memset(use_buffer->characters[j],' ',CC608_SCREEN_WIDTH);			        
		memset(use_buffer->colors[j],ccx_options.cc608_default_color,CC608_SCREEN_WIDTH);
        memset(use_buffer->fonts[j],FONT_REGULAR,CC608_SCREEN_WIDTH);
        use_buffer->characters[j][CC608_SCREEN_WIDTH]=0;
        use_buffer->row_used[j]=0;
    }
    memset(use_buffer->characters[lastrow],' ',CC608_SCREEN_WIDTH);
    memset(use_buffer->colors[lastrow],ccx_options.cc608_default_color,CC608_SCREEN_WIDTH);
    memset(use_buffer->fonts[lastrow],FONT_REGULAR,CC608_SCREEN_WIDTH);

    use_buffer->characters[lastrow][CC608_SCREEN_WIDTH]=0;
    use_buffer->row_used[lastrow]=0;
    
    // Sanity check
    int rows_now=0;
    for (int i=0;i<15;i++)
        if (use_buffer->row_used[i])
            rows_now++;
    if (rows_now>keep_lines)
        mprint ("Bug in roll_up, should have %d lines but I have %d.\n",
            keep_lines, rows_now);

    // If the buffer is now empty, let's set the flag
    // This will allow write_char to set visible start time appropriately
	if (0 == rows_now)
		use_buffer->empty = 1;

	return (rows_now != rows_orig);
}

void erase_memory (struct ccx_s_write *wb, int displayed)
{
    eia608_screen *buf;
    if (displayed)
    {
        if (wb->data608->visible_buffer==1)
            buf=&wb->data608->buffer1;
        else
            buf=&wb->data608->buffer2;
    }
    else
    {
        if (wb->data608->visible_buffer==1)
            buf=&wb->data608->buffer2;
        else
            buf=&wb->data608->buffer1;
    }
    clear_eia608_cc_buffer (buf);
}

int is_current_row_empty (struct ccx_s_write *wb)
{
    eia608_screen *use_buffer;
    if (wb->data608->visible_buffer==1)
        use_buffer = &wb->data608->buffer1;
    else
        use_buffer = &wb->data608->buffer2;
    for (int i=0;i<CC608_SCREEN_WIDTH;i++)
    {
        if (use_buffer->characters[wb->data608->rollup_base_row][i]!=' ')
            return 0;
    }
    return 1;
}

/* Process GLOBAL CODES */
void handle_command (/*const */ unsigned char c1, const unsigned char c2, struct ccx_s_write *wb)
{
	int changes=0; 

    // Handle channel change
    wb->data608->channel=wb->data608->new_channel;
    if (wb->data608->channel!=ccx_options.cc_channel)
        return;

    command_code command = COM_UNKNOWN;
    if (c1==0x15)
        c1=0x14;
    if ((c1==0x14 || c1==0x1C) && c2==0x2C)
        command = COM_ERASEDISPLAYEDMEMORY;
    if ((c1==0x14 || c1==0x1C) && c2==0x20)
        command = COM_RESUMECAPTIONLOADING;
    if ((c1==0x14 || c1==0x1C) && c2==0x2F)
        command = COM_ENDOFCAPTION;
    if ((c1==0x14 || c1==0x1C) && c2==0x22)
		command = COM_ALARMOFF;
    if ((c1==0x14 || c1==0x1C) && c2==0x23)
		command = COM_ALARMON;
    if ((c1==0x14 || c1==0x1C) && c2==0x24)
		command = COM_DELETETOENDOFROW;
    if ((c1==0x17 || c1==0x1F) && c2==0x21)
        command = COM_TABOFFSET1;
    if ((c1==0x17 || c1==0x1F) && c2==0x22)
        command = COM_TABOFFSET2;
    if ((c1==0x17 || c1==0x1F) && c2==0x23)
        command = COM_TABOFFSET3;
    if ((c1==0x14 || c1==0x1C) && c2==0x25)
        command = COM_ROLLUP2;
    if ((c1==0x14 || c1==0x1C) && c2==0x26)
        command = COM_ROLLUP3;
    if ((c1==0x14 || c1==0x1C) && c2==0x27)
        command = COM_ROLLUP4;
    if ((c1==0x14 || c1==0x1C) && c2==0x29)
        command = COM_RESUMEDIRECTCAPTIONING;
    if ((c1==0x14 || c1==0x1C) && c2==0x2D)
        command = COM_CARRIAGERETURN;
    if ((c1==0x14 || c1==0x1C) && c2==0x2E)
        command = COM_ERASENONDISPLAYEDMEMORY;
    if ((c1==0x14 || c1==0x1C) && c2==0x21)
        command = COM_BACKSPACE;
    if ((c1==0x14 || c1==0x1C) && c2==0x2b)
        command = COM_RESUMETEXTDISPLAY;

	if ((command == COM_ROLLUP2 || command == COM_ROLLUP3 || command==COM_ROLLUP4) && ccx_options.forced_ru==1)
		command=COM_FAKE_RULLUP1;

	if ((command == COM_ROLLUP3 || command==COM_ROLLUP4) && ccx_options.forced_ru==2)
		command=COM_ROLLUP2;
	else if (command==COM_ROLLUP4 && ccx_options.forced_ru==3)
		command=COM_ROLLUP3;
	
	dbg_print(CCX_DMT_608, "\rCommand begin: %02X %02X (%s)\n",c1,c2,command_type[command]);
	dbg_print(CCX_DMT_608, "\rCurrent mode: %d  Position: %d,%d  VisBuf: %d\n",wb->data608->mode,
		wb->data608->cursor_row,wb->data608->cursor_column, wb->data608->visible_buffer);        

    switch (command)
    {
        case COM_BACKSPACE:
            if (wb->data608->cursor_column>0)
            {
                wb->data608->cursor_column--;
                get_writing_buffer(wb)->characters[wb->data608->cursor_row][wb->data608->cursor_column]=' ';
            }
            break;
        case COM_TABOFFSET1:
            if (wb->data608->cursor_column<31)
                wb->data608->cursor_column++;
            break;
        case COM_TABOFFSET2:
            wb->data608->cursor_column+=2;
            if (wb->data608->cursor_column>31)
                wb->data608->cursor_column=31;
            break;
        case COM_TABOFFSET3:
            wb->data608->cursor_column+=3;
            if (wb->data608->cursor_column>31)
                wb->data608->cursor_column=31;
            break;
        case COM_RESUMECAPTIONLOADING:
            wb->data608->mode=MODE_POPON;
			// TODO: Is this right? 
			// Needed for 2013-03-23_0100_US_MSNBC_The_Rachel_Maddow_Show.mpg
			// But I can't find anything in the specs about RCL requiring a
			// visible buffer switch
            // The currently *visible* buffer is leaving, so now we know its ending
            // time. Time to actually write it to file.
			/*
            if (write_cc_buffer (wb))
                wb->data608->screenfuls_counter++;
            wb->data608->visible_buffer = (wb->data608->visible_buffer==1) ? 2 : 1;
            wb->data608->current_visible_start_ms=get_visible_start();
            wb->data608->cursor_column=0;
            wb->data608->cursor_row=0;
            wb->data608->color=default_color;
            wb->data608->font=FONT_REGULAR; */

            break;
        case COM_RESUMETEXTDISPLAY:
            wb->data608->mode=MODE_TEXT;
            break;
		case COM_FAKE_RULLUP1:
        case COM_ROLLUP2:            
		case COM_ROLLUP3:
		case COM_ROLLUP4:
            if (wb->data608->mode==MODE_POPON || wb->data608->mode==MODE_PAINTON)
            {
				/* CEA-608 C.10 Style Switching (regulatory) 
				[...]if pop-up or paint-on captioning is already present in 
				either memory it shall be erased[...] */
                if (write_cc_buffer (wb))
                    wb->data608->screenfuls_counter++;
                erase_memory (wb, true);			
            }
            erase_memory (wb, false);            

			// If the reception of data for a row is interrupted by data for the alternate 
			// data channel or for text mode, the display of caption text will resume from the same
			// cursor position if a roll-up caption command is received and no PAC is given [...]
			if (wb->data608->mode==MODE_TEXT)					
			{
				wb->data608->cursor_row=14; // Default if the previous mode wasn't roll up already.
				wb->data608->cursor_column=0;
			}
			else
			{
				// Cursor position only reset if not already in rollup. 
				// If no Preamble Address Code  is received, the base row shall default to 
				// Row 15 or, ifa roll-up caption is currently displayed, to the same 
				//base row last received, and the cursor shall be placed atColumn 1.
				//" This rule is meant to be applied only when no roll-up    caption is 
				// currently displayed,
				if (wb->data608->mode!=MODE_FAKE_ROLLUP_1 && 
					wb->data608->mode!=MODE_ROLLUP_2 && 
					wb->data608->mode!=MODE_ROLLUP_3 && 
					wb->data608->mode==MODE_ROLLUP_4)
				{
					wb->data608->cursor_row=wb->data608->rollup_base_row;
					wb->data608->cursor_column=0;
				}
			}
			switch (command)			
			{
				case COM_FAKE_RULLUP1:
					wb->data608->mode=MODE_FAKE_ROLLUP_1;
					break;
				case COM_ROLLUP2:
					wb->data608->mode=MODE_ROLLUP_2;
					break;
				case COM_ROLLUP3:
					wb->data608->mode=MODE_ROLLUP_3;
					break;
				case COM_ROLLUP4:
					wb->data608->mode=MODE_ROLLUP_4;
					break;
				default: // Impossible, but remove compiler warnings
					break;
			}
            break;
        case COM_CARRIAGERETURN:
			if (wb->data608->mode==MODE_PAINTON) // CR has no effect on painton mode according to zvbis' code
				break; 
			if (wb->data608->mode==MODE_POPON) // CFS: Not sure about this. Is there a valid reason for CR in popup?
			{
				wb->data608->cursor_column=0;
				if (wb->data608->cursor_row<15)
					wb->data608->cursor_row++;					
				break;
			}
			if (ccx_options.write_format==CCX_OF_TRANSCRIPT)
			{
                write_cc_line_as_transcript(get_current_visible_buffer (wb), wb, wb->data608->cursor_row);
			}

            // In transcript mode, CR doesn't write the whole screen, to avoid
            // repeated lines.
			changes=check_roll_up (wb);
			if (changes)
			{
				// Only if the roll up would actually cause a line to disappear we write the buffer
				if (ccx_options.write_format!=CCX_OF_TRANSCRIPT)
				{
					if (write_cc_buffer(wb))
						wb->data608->screenfuls_counter++;
					if (ccx_options.norollup)
	                    erase_memory (wb,true); // Make sure the lines we just wrote aren't written again
				}
			}
			roll_up(wb); // The roll must be done anyway of course.
			wb->data608->ts_start_of_current_line = -1; // Unknown. 
			if (changes)
				wb->data608->current_visible_start_ms=get_visible_start();
            wb->data608->cursor_column=0;
            break;
        case COM_ERASENONDISPLAYEDMEMORY:
            erase_memory (wb,false);
            break;
        case COM_ERASEDISPLAYEDMEMORY:
            // Write it to disk before doing this, and make a note of the new
            // time it became clear.
            if (ccx_options.write_format==CCX_OF_TRANSCRIPT && 
                (wb->data608->mode==MODE_FAKE_ROLLUP_1 ||
				wb->data608->mode==MODE_ROLLUP_2 || wb->data608->mode==MODE_ROLLUP_3 ||
                wb->data608->mode==MODE_ROLLUP_4))
            {
                // In transcript mode we just write the cursor line. The previous lines
                // should have been written already, so writing everything produces
                // duplicate lines.				
                write_cc_line_as_transcript(get_current_visible_buffer (wb), wb, wb->data608->cursor_row);
            }
            else
            {
                if (write_cc_buffer (wb))
                    wb->data608->screenfuls_counter++;
            }
            erase_memory (wb,true);
            wb->data608->current_visible_start_ms=get_visible_start();
            break;
        case COM_ENDOFCAPTION: // Switch buffers
            // The currently *visible* buffer is leaving, so now we know its ending
            // time. Time to actually write it to file.
            if (write_cc_buffer (wb))
                wb->data608->screenfuls_counter++;
            wb->data608->visible_buffer = (wb->data608->visible_buffer==1) ? 2 : 1;
            wb->data608->current_visible_start_ms=get_visible_start();
            wb->data608->cursor_column=0;
            wb->data608->cursor_row=0;
            wb->data608->color=ccx_options.cc608_default_color;
            wb->data608->font=FONT_REGULAR;
			wb->data608->mode=MODE_POPON;
            break;
		case COM_DELETETOENDOFROW:
			delete_to_end_of_row (wb);
			break;
		case COM_ALARMOFF:
		case COM_ALARMON:
			// These two are unused according to Robson's, and we wouldn't be able to do anything useful anyway
			break;
		case COM_RESUMEDIRECTCAPTIONING:
			wb->data608->mode = MODE_PAINTON;
			//mprint ("\nWarning: Received ResumeDirectCaptioning, this mode is almost impossible.\n");
			//mprint ("to transcribe to a text file.\n");
			break;
        default:
            dbg_print(CCX_DMT_608, "\rNot yet implemented.\n");            
            break;
    }
	dbg_print(CCX_DMT_608, "\rCurrent mode: %d  Position: %d,%d    VisBuf: %d\n",wb->data608->mode,
		wb->data608->cursor_row,wb->data608->cursor_column, wb->data608->visible_buffer);        
	dbg_print(CCX_DMT_608, "\rCommand end: %02X %02X (%s)\n",c1,c2,command_type[command]);

}

void handle_end_of_data (struct ccx_s_write *wb)
{
    // We issue a EraseDisplayedMemory here so if there's any captions pending
    // they get written to file. 
    handle_command (0x14, 0x2c, wb); // EDM
}

// CEA-608, Anex F 1.1.1. - Character Set Table / Special Characters
void handle_double (const unsigned char c1, const unsigned char c2, struct ccx_s_write *wb)
{
    unsigned char c;
    if (wb->data608->channel!=ccx_options.cc_channel)
        return;
    if (c2>=0x30 && c2<=0x3f)
    {
        c=c2 + 0x50; // So if c>=0x80 && c<=0x8f, it comes from here
        dbg_print(CCX_DMT_608, "\rDouble: %02X %02X  -->  %c\n",c1,c2,c);
        write_char(c,wb);
    }
}

/* Process EXTENDED CHARACTERS */
unsigned char handle_extended (unsigned char hi, unsigned char lo, struct ccx_s_write *wb)
{
    // Handle channel change
    if (wb->data608->new_channel > 2) 
    {
        wb->data608->new_channel -= 2;
        dbg_print(CCX_DMT_608, "\nChannel correction, now %d\n", wb->data608->new_channel);
    }
    wb->data608->channel=wb->data608->new_channel;
    if (wb->data608->channel!=ccx_options.cc_channel)
        return 0;

    // For lo values between 0x20-0x3f
    unsigned char c=0;

    dbg_print(CCX_DMT_608, "\rExtended: %02X %02X\n",hi,lo);
    if (lo>=0x20 && lo<=0x3f && (hi==0x12 || hi==0x13))
    {
        switch (hi)
        {
            case 0x12:
                c=lo+0x70; // So if c>=0x90 && c<=0xaf it comes from here
                break;
            case 0x13:
                c=lo+0x90; // So if c>=0xb0 && c<=0xcf it comes from here
                break;
        }
        // This column change is because extended characters replace 
        // the previous character (which is sent for basic decoders
        // to show something similar to the real char)
        if (wb->data608->cursor_column>0)        
            wb->data608->cursor_column--;        

        write_char (c,wb);
    }
    return 1;
}

/* Process PREAMBLE ACCESS CODES (PAC) */
void handle_pac (unsigned char c1, unsigned char c2, struct ccx_s_write *wb)
{
    // Handle channel change
    if (wb->data608->new_channel > 2) 
    {
        wb->data608->new_channel -= 2;
        dbg_print(CCX_DMT_608, "\nChannel correction, now %d\n", wb->data608->new_channel);
    }
    wb->data608->channel=wb->data608->new_channel;
    if (wb->data608->channel!=ccx_options.cc_channel)
        return;

    int row=rowdata[((c1<<1)&14)|((c2>>5)&1)];

    dbg_print(CCX_DMT_608, "\rPAC: %02X %02X",c1,c2);

    if (c2>=0x40 && c2<=0x5f)
    {
        c2=c2-0x40;
    }
    else
    {
        if (c2>=0x60 && c2<=0x7f)
        {
            c2=c2-0x60;
        }
        else
        {
            dbg_print(CCX_DMT_608, "\rThis is not a PAC!!!!!\n");
            return;
        }
    }	
    wb->data608->color=pac2_attribs[c2][0];	
    wb->data608->font=pac2_attribs[c2][1];
    int indent=pac2_attribs[c2][2];
    dbg_print(CCX_DMT_608, "  --  Position: %d:%d, color: %s,  font: %s\n",row,
        indent,color_text[wb->data608->color][0],font_text[wb->data608->font]);
	if (ccx_options.cc608_default_color==COL_USERDEFINED && (wb->data608->color==COL_WHITE || wb->data608->color==COL_TRANSPARENT))
		wb->data608->color=COL_USERDEFINED;
    if (wb->data608->mode!=MODE_TEXT)
    {
        // According to Robson, row info is discarded in text mode
        // but column is accepted
        wb->data608->cursor_row=row-1 ; // Since the array is 0 based
    }
    wb->data608->rollup_base_row=row-1;
    wb->data608->cursor_column=indent;	
	if (wb->data608->mode==MODE_FAKE_ROLLUP_1 || wb->data608->mode==MODE_ROLLUP_2 || 
		wb->data608->mode==MODE_ROLLUP_3 || wb->data608->mode==MODE_ROLLUP_4)
	{
		/* In roll-up, delete lines BELOW the PAC. Not sure (CFS) this is correct (possibly we may have to move the
		   buffer around instead) but it's better than leaving old characters in the buffer */
			eia608_screen *use_buffer = get_writing_buffer (wb); // &wb->data608->buffer1;
                
		for (int j=row;j<15;j++)
		{
			if (use_buffer->row_used[j])
			{
				memset(use_buffer->characters[j],' ',CC608_SCREEN_WIDTH);							
				memset(use_buffer->colors[j],ccx_options.cc608_default_color,CC608_SCREEN_WIDTH);
				memset(use_buffer->fonts[j],FONT_REGULAR,CC608_SCREEN_WIDTH);
				use_buffer->characters[j][CC608_SCREEN_WIDTH]=0;
				use_buffer->row_used[j]=0;
			}
		}
	}

}


void handle_single (const unsigned char c1, struct ccx_s_write *wb)
{	
    if (c1<0x20 || wb->data608->channel!=ccx_options.cc_channel)
        return; // We don't allow special stuff here
    dbg_print(CCX_DMT_608, "%c",c1);

    write_char (c1,wb);
}

void erase_both_memories (struct ccx_s_write *wb)
{
	erase_memory (wb,false);
	// For the visible memory, we write the contents to disk
            // The currently *visible* buffer is leaving, so now we know its ending
            // time. Time to actually write it to file.
    if (write_cc_buffer (wb))
		wb->data608->screenfuls_counter++;
	wb->data608->current_visible_start_ms=get_visible_start();
    wb->data608->cursor_column=0;
    wb->data608->cursor_row=0;
    wb->data608->color=ccx_options.cc608_default_color;
    wb->data608->font=FONT_REGULAR;

	erase_memory (wb,true);
}

int check_channel (unsigned char c1, struct ccx_s_write *wb)
{
	int newchan=wb->data608->channel;
	if (c1>=0x10 && c1<=0x17)
		newchan=1;
	else if (c1>=0x18 && c1<=0x1e)
		newchan=2;
	if (newchan!=wb->data608->channel)	
	{
		dbg_print(CCX_DMT_608, "\nChannel change, now %d\n", newchan);
		if (wb->data608->channel!=3) // Don't delete memories if returning from XDS. 
		{
			// erase_both_memories (wb); // 47cfr15.119.pdf, page 859, part f
			// CFS: Removed this because the specs say memories should be deleted if THE USER
			// changes the channel. 
		}
	}
	return newchan;
}

/* Handle Command, special char or attribute and also check for
* channel changes.
* Returns 1 if something was written to screen, 0 otherwise */
int disCommand (unsigned char hi, unsigned char lo, struct ccx_s_write *wb)
{
    int wrote_to_screen=0;

    /* Full channel changes are only allowed for "GLOBAL CODES",
    * "OTHER POSITIONING CODES", "BACKGROUND COLOR CODES",
    * "MID-ROW CODES".
    * "PREAMBLE ACCESS CODES", "BACKGROUND COLOR CODES" and
    * SPECIAL/SPECIAL CHARACTERS allow only switching
    * between 1&3 or 2&4. */
    wb->data608->new_channel = check_channel (hi,wb);
    //if (wb->data608->channel!=cc_channel)
    //    continue;

    if (hi>=0x18 && hi<=0x1f)
        hi=hi-8;

    switch (hi)
    {
        case 0x10:
            if (lo>=0x40 && lo<=0x5f)
                handle_pac (hi,lo,wb);
            break;
        case 0x11:
            if (lo>=0x20 && lo<=0x2f)
                handle_text_attr (hi,lo,wb);
            if (lo>=0x30 && lo<=0x3f)
            {
                wrote_to_screen=1;
                handle_double (hi,lo,wb);
            }
            if (lo>=0x40 && lo<=0x7f)
                handle_pac (hi,lo,wb);
            break;
        case 0x12:
        case 0x13:
            if (lo>=0x20 && lo<=0x3f)
            {
                wrote_to_screen=handle_extended (hi,lo,wb);
            }
            if (lo>=0x40 && lo<=0x7f)
                handle_pac (hi,lo,wb);
            break;
        case 0x14:
        case 0x15:
            if (lo>=0x20 && lo<=0x2f)
                handle_command (hi,lo,wb);
            if (lo>=0x40 && lo<=0x7f)
                handle_pac (hi,lo,wb);
            break;
        case 0x16:
            if (lo>=0x40 && lo<=0x7f)
                handle_pac (hi,lo,wb);
            break;
        case 0x17:
            if (lo>=0x21 && lo<=0x23)
                handle_command (hi,lo,wb);
            if (lo>=0x2e && lo<=0x2f)
                handle_text_attr (hi,lo,wb);
            if (lo>=0x40 && lo<=0x7f)
                handle_pac (hi,lo,wb);
            break;
    }
    return wrote_to_screen;
}

/* If wb is NULL, then only XDS will be processed */
void process608 (const unsigned char *data, int length, struct ccx_s_write *wb)
{
    static int textprinted = 0;	
	if (wb)
		wb->bytes_processed_608+=length;
    if (data!=NULL)
    {
        for (int i=0;i<length;i=i+2)
        {
            unsigned char hi, lo;
            int wrote_to_screen=0; 
            hi = data[i] & 0x7F; // Get rid of parity bit
            lo = data[i+1] & 0x7F; // Get rid of parity bit

            if (hi==0 && lo==0) // Just padding
                continue;
			
            // printf ("\r[%02X:%02X]\n",hi,lo);

			if (hi>=0x01 && hi<=0x0E && (wb==NULL || wb->my_field==2)) // XDS can only exist in field 2.
            {
                if (wb)
					wb->data608->channel=3; 
				if (!in_xds_mode)
				{
					ts_start_of_xds=get_fts();
					in_xds_mode=1;
				}
            }
            if (hi==0x0F && in_xds_mode && (wb==NULL || wb->my_field==2)) // End of XDS block
            {
                in_xds_mode=0;
				do_end_of_xds (lo);
				if (wb) 
					wb->data608->channel=wb->data608->new_channel; // Switch from channel 3
                continue;
            }
            if (hi>=0x10 && hi<=0x1F) // Non-character code or special/extended char
                // http://www.geocities.com/mcpoodle43/SCC_TOOLS/DOCS/CC_CODES.HTML
                // http://www.geocities.com/mcpoodle43/SCC_TOOLS/DOCS/CC_CHARS.HTML
            {
                // We were writing characters before, start a new line for
                // diagnostic output from disCommand()
                if (textprinted == 1 )
                {
                    dbg_print(CCX_DMT_608, "\n");
                    textprinted = 0;
                }
				if (!wb || wb->my_field==2)
					in_xds_mode=0; // Back to normal (CEA 608-8.6.2)
				if (!wb) // Not XDS and we don't have a writebuffer, nothing else would have an effect
					continue; 
				if (wb->data608->last_c1==hi && wb->data608->last_c2==lo)
				{
					// Duplicate dual code, discard. Correct to do it only in
					// non-XDS, XDS codes shall not be repeated.
					 dbg_print(CCX_DMT_608, "Skipping command %02X,%02X Duplicate\n", hi, lo);
                    // Ignore only the first repetition
                    wb->data608->last_c1=-1;
                    wb->data608->last_c2=-1;
					continue;
				}
				wb->data608->last_c1=hi;
				wb->data608->last_c2=lo;
				wrote_to_screen=disCommand (hi,lo,wb);
            }
			else
			{
				if (in_xds_mode && (wb==NULL || wb->my_field==2 ))
				{
					process_xds_bytes (hi,lo);
					continue;
				}
				if (!wb) // No XDS code after this point, and user doesn't want captions.
					continue;

				wb->data608->last_c1=-1;
				wb->data608->last_c2=-1;

				if (hi>=0x20) // Standard characters (always in pairs)
				{					
					// Only print if the channel is active
					if (wb->data608->channel!=ccx_options.cc_channel)
						continue;

					if( textprinted == 0 )
					{
						dbg_print(CCX_DMT_608, "\n");
						textprinted = 1;
					}
					
					handle_single(hi,wb);
					handle_single(lo,wb);
					wrote_to_screen=1;
					wb->data608->last_c1=0;
					wb->data608->last_c2=0;
				}

				if (!textprinted && wb->data608->channel==ccx_options.cc_channel )
				{   // Current FTS information after the characters are shown
					dbg_print(CCX_DMT_608, "Current FTS: %s\n", print_mstime(get_fts()));
					//printf("  N:%u", unsigned(fts_now) );
					//printf("  G:%u", unsigned(fts_global) );
					//printf("  F:%d %d %d %d\n",
					//       current_field, cb_field1, cb_field2, cb_708 );
				}

				if (wrote_to_screen && ccx_options.direct_rollup && // If direct_rollup is enabled and
					(wb->data608->mode==MODE_FAKE_ROLLUP_1 || // we are in rollup mode, write now.
					wb->data608->mode==MODE_ROLLUP_2 || 
					wb->data608->mode==MODE_ROLLUP_3 ||
					wb->data608->mode==MODE_ROLLUP_4))
				{
					// We don't increase screenfuls_counter here.
					write_cc_buffer (wb);
					wb->data608->current_visible_start_ms=get_visible_start();
				}
			}
			if (wrote_to_screen && cc_to_stdout)
				fflush (stdout);			
        } // for
    }
}


/* Return a pointer to a string that holds the printable characters
 * of the caption data block. FOR DEBUG PURPOSES ONLY! */
unsigned char *debug_608toASC (unsigned char *cc_data, int channel)
{
    static unsigned char output[3];

    unsigned char cc_valid = (cc_data[0] & 4) >>2;
    unsigned char cc_type = cc_data[0] & 3;
    unsigned char hi, lo;

    output[0]=' ';
    output[1]=' ';
    output[2]='\x00';

    if (cc_valid && cc_type==channel)
    {
        hi = cc_data[1] & 0x7F; // Get rid of parity bit
        lo = cc_data[2] & 0x7F; // Get rid of parity bit
        if (hi>=0x20)
        {
            output[0]=hi;
            output[1]=(lo>=20 ? lo : '.');
            output[2]='\x00';
        }
        else
        {
            output[0]='<';
            output[1]='>';
            output[2]='\x00';
        }
    }
    return output;
}
