#include "ccextractor.h"

// Program Identification Number (Start Time) for current program
static int current_xds_min=-1;
static int current_xds_hour=-1;
static int current_xds_date=-1;
static int current_xds_month=-1;
static int current_program_type_reported=0; // No.
static int xds_start_time_shown=0;
static int xds_program_length_shown=0;
static int xds_program_type_shown=0;
static char xds_program_description[8][33];

static char current_xds_network_name[33]; 
static char current_xds_program_name[33]; 
static char current_xds_call_letters[7];
static char current_xds_program_type[33];

static const char *XDSclasses[]=
{
	"Current",
	"Future",
	"Channel",
	"Miscellaneous",
	"Public service",
	"Reserved",
	"Private data",
	"End"
};

static const char *XDSclasses_short[]=
{
	"CUR",
	"FUT",
	"CHN",
	"MIS",
	"PUB",
	"RES",
	"PRV",
	"END"
};

static const char *XDSProgramTypes[]=
{
	"Education","Entertainment", "Movie", "News", "Religious",
	"Sports", "Other", "Action","Advertisement", "Animated",
	"Anthology","Automobile","Awards","Baseball","Basketball",
	"Bulletin","Business","Classical","College","Combat",
	"Comedy","Commentary","Concert","Consumer","Contemporary",
	"Crime","Dance","Documentary","Drama","Elementary",
	"Erotica","Exercise","Fantasy","Farm","Fashion",
	"Fiction","Food","Football","Foreign","Fund-Raiser",
	"Game/Quiz","Garden","Golf","Government","Health",
	"High_School","History","Hobby","Hockey","Home",
	"Horror","Information","Instruction","International","Interview",
	"Language","Legal","Live","Local","Math",
	"Medical","Meeting","Military","Mini-Series","Music",
	"Mystery","National","Nature","Police","Politics",
	"Premiere","Pre-Recorded","Product","Professional","Public",
	"Racing","Reading","Repair","Repeat","Review",
	"Romance","Science","Series","Service","Shopping",
	"Soap_Opera","Special","Suspense","Talk","Technical",
	"Tennis","Travel","Variety","Video","Weather",
	"Western"
};

#define XDS_CLASS_CURRENT	0
#define XDS_CLASS_FUTURE	1
#define XDS_CLASS_CHANNEL	2
#define XDS_CLASS_MISC		3
#define XDS_CLASS_PUBLIC	4
#define XDS_CLASS_RESERVED	5
#define XDS_CLASS_PRIVATE	6
#define XDS_CLASS_END		7

// Types for the classes current and future
#define XDS_TYPE_PIN_START_TIME	1
#define XDS_TYPE_LENGH_AND_CURRENT_TIME	2
#define XDS_TYPE_PROGRAM_NAME 3
#define XDS_TYPE_PROGRAM_TYPE 4
#define XDS_TYPE_CONTENT_ADVISORY 5
#define XDS_TYPE_AUDIO_SERVICES 6
#define XDS_TYPE_CGMS 8 // Copy Generation Management System
#define XDS_TYPE_PROGRAM_DESC_1 0x10
#define XDS_TYPE_PROGRAM_DESC_2 0x11
#define XDS_TYPE_PROGRAM_DESC_3 0x12
#define XDS_TYPE_PROGRAM_DESC_4 0x13
#define XDS_TYPE_PROGRAM_DESC_5 0x14
#define XDS_TYPE_PROGRAM_DESC_6 0x15
#define XDS_TYPE_PROGRAM_DESC_7 0x16
#define XDS_TYPE_PROGRAM_DESC_8 0x17

// Types for the class channel
#define XDS_TYPE_NETWORK_NAME 1	
#define XDS_TYPE_CALL_LETTERS_AND_CHANNEL 2	
#define XDS_TYPE_TSID 4	// Transmission Signal Identifier

// Types for miscellaneous packets
#define XDS_TYPE_TIME_OF_DAY 1	
#define XDS_TYPE_LOCAL_TIME_ZONE 4

#define NUM_XDS_BUFFERS 9  // CEA recommends no more than one level of interleaving. Play it safe
#define NUM_BYTES_PER_PACKET 35 // Class + type (repeated for convenience) + data + zero

struct xds_buffer
{
	unsigned in_use;
	int xds_class;
	int xds_type;
	unsigned char bytes[NUM_BYTES_PER_PACKET]; // Class + type (repeated for convenience) + data + zero
	unsigned char used_bytes;
} xds_buffers[NUM_XDS_BUFFERS]; 

static int cur_xds_buffer_idx=-1;
static int cur_xds_packet_class=-1;
static unsigned char *cur_xds_payload;
static int cur_xds_payload_length;
static int cur_xds_packet_type;


void xds_init()
{
	for (int i=0;i<NUM_XDS_BUFFERS;i++)
	{
		xds_buffers[i].in_use=0;
		xds_buffers[i].xds_class=-1;
		xds_buffers[i].xds_type=-1;
		xds_buffers[i].used_bytes=0;
		memset (xds_buffers[i].bytes , 0, NUM_BYTES_PER_PACKET);
	}
	for (int i=0; i<9; i++)	
		memset (xds_program_description,0,32); 
	
	memset (current_xds_network_name,0,33); 
	memset (current_xds_program_name,0,33); 
	memset (current_xds_call_letters,0,7);
	memset (current_xds_program_type,0,33); 
}

void xds_write_transcript_line_suffix (struct ccx_s_write *wb)
{
	if (!wb || wb->fh==-1)
		return;
    write (wb->fh, encoded_crlf, encoded_crlf_length);
}

void xds_write_transcript_line_prefix (struct ccx_s_write *wb)
{
	unsigned h1,m1,s1,ms1;
	unsigned h2,m2,s2,ms2;
	if (!wb || wb->fh==-1)
		return;

	if (ts_start_of_xds == -1) 
	{
		// Means we entered XDS mode without making a note of the XDS start time. This is a bug.
		fatal (EXIT_BUG_BUG, "Bug in timedtranscript (XDS). Please report.");
		;
	}

	if (ccx_options.transcript_settings.showStartTime){
		char buffer[80];
		if (ccx_options.transcript_settings.relativeTimestamp){
			if (utc_refvalue == UINT64_MAX)
			{
				mstotime(ts_start_of_xds + subs_delay, &h1, &m1, &s1, &ms1);
				fdprintf(wb->fh, "%02u:%02u:%02u%c%03u|", h1, m1, s1, ccx_options.millis_separator, ms1);
			}
			else {
				fdprintf(wb->fh, "%lld%c%03d|", (ts_start_of_xds + subs_delay) / 1000, ccx_options.millis_separator, (ts_start_of_xds + subs_delay) % 1000);
			}
		}
		else {
			mstotime(ts_start_of_xds + subs_delay, &h1, &m1, &s1, &ms1);
			time_t start_time_int = (ts_start_of_xds + subs_delay) / 1000;
			int start_time_dec = (ts_start_of_xds + subs_delay) % 1000;
			struct tm *start_time_struct = gmtime(&start_time_int);
			strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", start_time_struct);
			fdprintf(wb->fh, "%s%c%03d|", buffer,ccx_options.millis_separator,start_time_dec);
		}
	}

	if (ccx_options.transcript_settings.showEndTime){
		char buffer[80];
		if (ccx_options.transcript_settings.relativeTimestamp){
			if (utc_refvalue == UINT64_MAX)
			{
				mstotime(get_fts() + subs_delay, &h2, &m2, &s2, &ms2);
				fdprintf(wb->fh, "%02u:%02u:%02u%c%03u|", h2, m2, s2, ccx_options.millis_separator, ms2);
			}
			else
			{
				fdprintf(wb->fh, "%lld%s%03d|", (get_fts() + subs_delay) / 1000, ccx_options.millis_separator, (get_fts() + subs_delay) % 1000);
			}
		}
		else {
			mstotime(get_fts() + subs_delay, &h2, &m2, &s2, &ms2);
			time_t end_time_int = (get_fts() + subs_delay) / 1000;
			int end_time_dec = (get_fts() + subs_delay) % 1000;
			struct tm *end_time_struct = gmtime(&end_time_int);
			strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", end_time_struct);
			fdprintf(wb->fh, "%s%c%03d|", buffer, ccx_options.millis_separator, end_time_dec);
		}
	}

	if (ccx_options.transcript_settings.showMode){
		const char *mode = "XDS";
		fdprintf(wb->fh, "%s|", mode);
	}

	if (ccx_options.transcript_settings.showCC){
		fdprintf(wb->fh, "%s|", XDSclasses_short[cur_xds_packet_class]);
	}	
}

void xdsprint (const char *fmt,...)
{
	if (wbxdsout==NULL || wbxdsout->fh==-1) 
		return;
	xds_write_transcript_line_prefix (wbxdsout);

     /* Guess we need no more than 100 bytes. */
     int n, size = 100;
     char *p, *np;
     va_list ap;

     if ((p = (char *) malloc (size)) == NULL)
        return;

     while (1) 
	 {
        /* Try to print in the allocated space. */
        va_start(ap, fmt);
        n = vsnprintf (p, size, fmt, ap);
        va_end(ap);
        /* If that worked, return the string. */
        if (n > -1 && n < size)
		{
			write (wbxdsout->fh, p, n);			
			free (p);
			xds_write_transcript_line_suffix (wbxdsout);
            return;
		}
        /* Else try again with more space. */
        if (n > -1)    /* glibc 2.1 */
           size = n+1; /* precisely what is needed */
        else           /* glibc 2.0 */
           size *= 2;  /* twice the old size */
        if ((np = (char *) realloc (p, size)) == NULL) 
		{
           free(p);
		   xds_write_transcript_line_suffix (wbxdsout);
           return ;
        } else {
           p = np;
        }
     } 	
}

void xds_debug_test()
{
	process_xds_bytes (0x05,0x02);
	process_xds_bytes (0x20,0x20);
	do_end_of_xds (0x2a);

}

void xds_cea608_test()
{
	/* This test is the sample data that comes in CEA-608. It sets the program name
	to be "Star Trek". The checksum is 0x1d and the validation must succeed. */
	process_xds_bytes (0x01,0x03);
	process_xds_bytes (0x53,0x74);
	process_xds_bytes (0x61,0x72);
	process_xds_bytes (0x20,0x54);
	process_xds_bytes (0x72,0x65);
	process_xds_bytes (0x02,0x03);
	process_xds_bytes (0x02,0x03);
	process_xds_bytes (0x6b,0x00);
	do_end_of_xds (0x1d);
}

int how_many_used()
{
	int c=0;
	for (int i=0;i<NUM_XDS_BUFFERS;i++)
		if (xds_buffers[i].in_use)
			c++;
	return c;

}

void clear_xds_buffer (int num)
{
	xds_buffers[num].in_use=0;
	xds_buffers[num].xds_class=-1;
	xds_buffers[num].xds_type=-1;
	xds_buffers[num].used_bytes=0;
	memset (xds_buffers[num].bytes , 0, NUM_BYTES_PER_PACKET);
}

void process_xds_bytes (const unsigned char hi, int lo)
{	
	int is_new;
	if (hi>=0x01 && hi<=0x0f)
	{
		int xds_class=(hi-1)/2; // Start codes 1 and 2 are "class type" 0, 3-4 are 2, and so on.
		is_new=hi%2; // Start codes are even
		dbg_print(CCX_DMT_XDS, "XDS Start: %u.%u  Is new: %d  | Class: %d (%s), Used buffers: %d\n",hi,lo, is_new,xds_class, XDSclasses[xds_class], how_many_used());
		int first_free_buf=-1;
		int matching_buf=-1;
		for (int i=0;i<NUM_XDS_BUFFERS;i++)
		{
			if (xds_buffers[i].in_use && 
				xds_buffers[i].xds_class==xds_class &&
				xds_buffers[i].xds_type==lo)
			{
				matching_buf=i;
				break;
			}
			if (first_free_buf==-1 && !xds_buffers[i].in_use)
				first_free_buf=i;
		}
		/* Here, 3 possibilities: 
			1) We already had a buffer for this class/type and matching_buf points to it
			2) We didn't have a buffer for this class/type and first_free_buf points to an unused one
			3) All buffers are full and we will have to skip this packet.
			*/
		if (matching_buf==-1 && first_free_buf==-1)
		{
			mprint ("Note: All XDS buffers full (bug or suicidal stream). Ignoring this one (%d,%d).\n",xds_class,lo);
			cur_xds_buffer_idx=-1;
			return;

		}
		cur_xds_buffer_idx=(matching_buf!=-1)? matching_buf:first_free_buf;

		if (is_new || !xds_buffers[cur_xds_buffer_idx].in_use)
		{
			// Whatever we had before we discard; must belong to an interrupted packet
			xds_buffers[cur_xds_buffer_idx].xds_class=xds_class;
			xds_buffers[cur_xds_buffer_idx].xds_type=lo;
			xds_buffers[cur_xds_buffer_idx].used_bytes=0;
			xds_buffers[cur_xds_buffer_idx].in_use=1;
			memset (xds_buffers[cur_xds_buffer_idx].bytes,0,NUM_BYTES_PER_PACKET);						
		}
		if (!is_new)
		{
			// Continue codes aren't added to packet.
			return;
		}
	}
	else
	{
		// Informational: 00, or 0x20-0x7F, so 01-0x1f forbidden
		dbg_print(CCX_DMT_XDS, "XDS: %02X.%02X (%c, %c)\n",hi,lo,hi,lo);
		if ((hi>0 && hi<=0x1f) || (lo>0 && lo<=0x1f))
		{
			mprint ("\rNote: Illegal XDS data");
			return;
		}
	}
	if (xds_buffers[cur_xds_buffer_idx].used_bytes<=32)
	{
		// Should always happen
		xds_buffers[cur_xds_buffer_idx].bytes[xds_buffers[cur_xds_buffer_idx].used_bytes++]=hi;
		xds_buffers[cur_xds_buffer_idx].bytes[xds_buffers[cur_xds_buffer_idx].used_bytes++]=lo;
		xds_buffers[cur_xds_buffer_idx].bytes[xds_buffers[cur_xds_buffer_idx].used_bytes]=0; 
	}
}

void xds_do_copy_generation_management_system (unsigned c1, unsigned c2)
{
	static unsigned last_c1=-1, last_c2=-1;
	static char copy_permited[256];	
	static char aps[256];
	static char rcd[256];
	int changed=0;	
	unsigned c1_6=(c1&0x40)>>6;
	unsigned unused1=(c1&0x20)>>5;
	unsigned cgms_a_b4=(c1&0x10)>>4;
	unsigned cgms_a_b3=(c1&0x8)>>3;
	unsigned aps_b2=(c1&0x4)>>2;
	unsigned aps_b1=(c1&0x2)>>1;
	unsigned asb_0=(c1&0x1);
	unsigned c2_6=(c2&0x40)>>6;
	unsigned c2_5=(c2&0x20)>>5;
	unsigned c2_4=(c2&0x10)>>4;
	unsigned c2_3=(c2&0x8)>>3;
	unsigned c2_2=(c2&0x4)>>2;
	unsigned c2_1=(c2&0x2)>>1;
	unsigned rcd0=(c2&0x1);
	if (!c1_6 || !c2_6) // These must be high. If not, not following specs
		return;
	if (last_c1!=c1 || last_c2!=c2)
	{
		changed=1;
		last_c1=c1; 
		last_c2=c2; 
		// Changed since last time, decode

		const char *copytext[4]={"Copy permited (no restrictions)", "No more copies (one generation copy has been made)", 
			"One generation of copies can be made", "No copying is permited"};
		const char *apstext[4]={"No APS", "PSP On; Split Burst Off", "PSP On; 2 line Split Burst On", "PSP On; 4 line Split Burst On"};
		sprintf (copy_permited,"CGMS: %s", copytext[cgms_a_b4*2+cgms_a_b3]);
		sprintf (aps,"APS: %s", apstext[aps_b2*2+aps_b1]);
		sprintf (rcd,"Redistribution Control Descriptor: %d", rcd0);

	}

	xdsprint(copy_permited);
	xdsprint(aps);
	xdsprint(rcd);
	if (changed)				
	{
		mprint ("\rXDS: %s\n",copy_permited);
		mprint ("\rXDS: %s\n",aps);
		mprint ("\rXDS: %s\n",rcd);
	}
	dbg_print(CCX_DMT_XDS, "\rXDS: %s\n",copy_permited);
	dbg_print(CCX_DMT_XDS, "\rXDS: %s\n",aps);
	dbg_print(CCX_DMT_XDS, "\rXDS: %s\n",rcd);
}

void xds_do_content_advisory (unsigned c1, unsigned c2)
{
	static unsigned last_c1=-1, last_c2=-1;
	static char age[256];
	static char content[256];
	static char rating[256];
	int changed=0;
	// Insane encoding
	unsigned c1_6=(c1&0x40)>>6;
	unsigned Da2=(c1&0x20)>>5;
	unsigned a1=(c1&0x10)>>4;
	unsigned a0=(c1&0x8)>>3;
	unsigned r2=(c1&0x4)>>2;
	unsigned r1=(c1&0x2)>>1;
	unsigned r0=(c1&0x1);
	unsigned c2_6=(c2&0x40)>>6;
	unsigned FV=(c2&0x20)>>5;
	unsigned S=(c2&0x10)>>4;
	unsigned La3=(c2&0x8)>>3;
	unsigned g2=(c2&0x4)>>2;
	unsigned g1=(c2&0x2)>>1;
	unsigned g0=(c2&0x1);
	unsigned supported=0;
	if (!c1_6 || !c2_6) // These must be high. If not, not following specs
		return;
	if (last_c1!=c1 || last_c2!=c2)
	{
		changed=1;
		last_c1=c1; 
		last_c2=c2; 
		// Changed since last time, decode
		// Bits a1 and a0 determine the encoding. I'll add parsing as more samples become available
		if (!a1 && a0) // US TV parental guidelines
		{
			const char *agetext[8]={"None", "TV-Y (All Children)", "TV-Y7 (Older Children)", 
				"TV-G (General Audience)", "TV-PG (Parental Guidance Suggested)", 
				"TV-14 (Parents Strongly Cautioned)", "TV-MA (Mature Audience Only)", "None"};
			sprintf (age,"ContentAdvisory: US TV Parental Guidelines. Age Rating: %s", agetext[g2*4+g1*2+g0]);
			content[0]=0;
			if (!g2 && g1 && !g0) // For TV-Y7 (Older chidren), the Violence bit is "fantasy violence"
			{
				if (FV)
					strcpy (content, "[Fantasy Violence] ");
			}
			else // For all others, is real
			{
				if (FV)
					strcpy (content, "[Violence] ");
			}
			if (S)
				strcat (content, "[Sexual Situations] ");
			if (La3)
				strcat (content, "[Adult Language] ");
			if (Da2)
				strcat (content, "[Sexually Suggestive Dialog] ");
			supported=1;
		}
		if (!a0) // MPA
		{
			const char *ratingtext[8]={"N/A", "G", "PG", "PG-13", "R", "NC-17", "X", "Not Rated"};
			sprintf (rating,"ContentAdvisory: MPA Rating: %s", ratingtext[r2*4+r1*2+r0]);
			supported=1;
		}
		if (a0 && a1 && !Da2 && !La3) // Canadian English Language Rating
		{
			const char *ratingtext[8]={"Exempt", "Children", "Children eight years and older", 
				"General programming suitable for all audiences", "Parental Guidance", 
				"Viewers 14 years and older", "Adult Programming", "[undefined]"};
			sprintf (rating,"ContentAdvisory: Canadian English Rating: %s", ratingtext[g2*4+g1*2+g0]);
			supported=1;
		}

	}
	// Bits a1 and a0 determine the encoding. I'll add parsing as more samples become available
	if (!a1 && a0) // US TV parental guidelines
	{		
		xdsprint(age);
		xdsprint(content);
		if (changed)				
		{
			mprint ("\rXDS: %s\n  ",age);
			mprint ("\rXDS: %s\n  ",content);
		}
		dbg_print(CCX_DMT_XDS, "\rXDS: %s\n",age);
		dbg_print(CCX_DMT_XDS, "\rXDS: %s\n",content);
	}
	if (!a0 || // MPA
			(a0 && a1 && !Da2 && !La3) // Canadian English Language Rating
	) 
	{
		xdsprint(rating);
		if (changed)				
			mprint ("\rXDS: %s\n  ",rating);
		dbg_print(CCX_DMT_XDS, "\rXDS: %s\n",rating);
	}

	if (changed && !supported)							
		mprint ("XDS: Unsupported ContentAdvisory encoding, please submit sample.\n");
			

}

int xds_do_current_and_future ()
{
	int was_proc=0;
	switch (cur_xds_packet_type)
	{
		case XDS_TYPE_PIN_START_TIME:
			{
				was_proc=1;
				if (cur_xds_payload_length<7) // We need 4 data bytes
					break;
				int min=cur_xds_payload[2] & 0x3f; // 6 bits
				int hour = cur_xds_payload[3] & 0x1f; // 5 bits
				int date = cur_xds_payload[4] & 0x1f; // 5 bits
				int month = cur_xds_payload[5] & 0xf; // 4 bits
				int changed=0;
				if (current_xds_min!=min ||
					current_xds_hour!=hour ||
					current_xds_date!=date ||
					current_xds_month!=month)
				{
					changed=1;
					xds_start_time_shown=0;
					current_xds_min=min;
					current_xds_hour=hour;
					current_xds_date=date;
					current_xds_month=month;
				}

				dbg_print(CCX_DMT_XDS, "PIN (Start Time): %s  %02d-%02d %02d:%02d\n",
						(cur_xds_packet_class==XDS_CLASS_CURRENT?"Current":"Future"),
						date,month,hour,min);
				xdsprint ( "PIN (Start Time): %s  %02d-%02d %02d:%02d\n",
						(cur_xds_packet_class==XDS_CLASS_CURRENT?"Current":"Future"),
						date,month,hour,min);

				if (!xds_start_time_shown && cur_xds_packet_class==XDS_CLASS_CURRENT)
				{
					mprint ("\rXDS: Program changed.\n");
					mprint ("XDS program start time (DD/MM HH:MM) %02d-%02d %02d:%02d\n",date,month,hour,min);
					activity_xds_program_identification_number (current_xds_min, 
						current_xds_hour, current_xds_date, current_xds_month);
					xds_start_time_shown=1;
				}			
			}
			break;
		case XDS_TYPE_LENGH_AND_CURRENT_TIME:
			{
				was_proc=1;
				if (cur_xds_payload_length<5) // We need 2 data bytes
					break;
				int min=cur_xds_payload[2] & 0x3f; // 6 bits
				int hour = cur_xds_payload[3] & 0x1f; // 5 bits
				if (!xds_program_length_shown)				
					mprint ("\rXDS: Program length (HH:MM): %02d:%02d  ",hour,min);
				else
					dbg_print(CCX_DMT_XDS, "\rXDS: Program length (HH:MM): %02d:%02d  ",hour,min);

				xdsprint("Program length (HH:MM): %02d:%02d  ",hour,min);

				if (cur_xds_payload_length>6) // Next two bytes (optional) available
				{
					int el_min=cur_xds_payload[4] & 0x3f; // 6 bits
					int el_hour = cur_xds_payload[5] & 0x1f; // 5 bits
					if (!xds_program_length_shown)
						mprint ("Elapsed (HH:MM): %02d:%02d",el_hour,el_min);
					else
						dbg_print(CCX_DMT_XDS, "Elapsed (HH:MM): %02d:%02d",el_hour,el_min);
					xdsprint("Elapsed (HH:MM): %02d:%02d",el_hour,el_min);

				}
				if (cur_xds_payload_length>8) // Next two bytes (optional) available
				{
					int el_sec=cur_xds_payload[6] & 0x3f; // 6 bits							
					if (!xds_program_length_shown)
						dbg_print(CCX_DMT_XDS, ":%02d",el_sec);
					xdsprint("Elapsed (SS) :%02d",el_sec);
				}
				if (!xds_program_length_shown)
					printf ("\n");
				else
					dbg_print(CCX_DMT_XDS, "\n");
				xds_program_length_shown=1;
			}
			break;
		case XDS_TYPE_PROGRAM_NAME:
			{
				was_proc=1;
				char xds_program_name[33];
				int i;
				for (i=2;i<cur_xds_payload_length-1;i++)
					xds_program_name[i-2]=cur_xds_payload[i];
				xds_program_name[i-2]=0;
				dbg_print(CCX_DMT_XDS, "\rXDS Program name: %s\n",xds_program_name);
				xdsprint("Program name: %s",xds_program_name);
				if (cur_xds_packet_class==XDS_CLASS_CURRENT && 
					strcmp (xds_program_name, current_xds_program_name)) // Change of program
				{
					if (!ccx_options.gui_mode_reports)
						mprint ("\rXDS Notice: Program is now %s\n", xds_program_name);
					strcpy (current_xds_program_name,xds_program_name);
					activity_xds_program_name (xds_program_name);
				}
				break;
			}
			break;
		case XDS_TYPE_PROGRAM_TYPE:
			was_proc=1;					
			if (cur_xds_payload_length<5) // We need 2 data bytes
				break;
			if (current_program_type_reported)
			{
				// Check if we should do it again
				for (int i=0;i<cur_xds_payload_length ; i++)
				{
					if (cur_xds_payload[i]!=current_xds_program_type[i])
					{
						current_program_type_reported=0;
						break;
					}
				}
			}
			if (!(ccx_options.debug_mask & CCX_DMT_XDS) && current_program_type_reported && 
				ccx_options.transcript_settings.xds == 0)
				//ccx_options.export_xds==0)
				break;
			memcpy (current_xds_program_type,cur_xds_payload,cur_xds_payload_length);
			current_xds_program_type[cur_xds_payload_length]=0;
			if (!current_program_type_reported)
				mprint ("\rXDS Program Type: ");
			xds_write_transcript_line_prefix(wbxdsout);
			if (wbxdsout && wbxdsout->fh!=-1)
				fdprintf (wbxdsout->fh,"Program type ");

			for (int i=2;i<cur_xds_payload_length - 1; i++)
			{								
				if (cur_xds_payload[i]==0) // Padding
					continue;														
				if (!current_program_type_reported)
					mprint ("[%02X-", cur_xds_payload[i]);
				if (wbxdsout && wbxdsout->fh!=-1)
				{
					if (cur_xds_payload[i]>=0x20 && cur_xds_payload[i]<0x7F)
						fdprintf (wbxdsout->fh,"[%s] ",XDSProgramTypes[cur_xds_payload[i]-0x20]);				
				}
				if (!current_program_type_reported)
				{
					if (cur_xds_payload[i]>=0x20 && cur_xds_payload[i]<0x7F)
						mprint ("%s",XDSProgramTypes[cur_xds_payload[i]-0x20]); 
					else
						mprint ("ILLEGAL VALUE");
					mprint ("] ");						
				}
			} 
			xds_write_transcript_line_suffix(wbxdsout);
			if (!current_program_type_reported)
				mprint ("\n");
			current_program_type_reported=1;
			break; 
		case XDS_TYPE_CONTENT_ADVISORY: 
			was_proc=1; 
			if (cur_xds_payload_length<5) // We need 2 data bytes
				break;
			xds_do_content_advisory (cur_xds_payload[2],cur_xds_payload[3]);
			break;
		case XDS_TYPE_AUDIO_SERVICES: 
			was_proc=1; // I don't have any sample with this.
			break;
		case XDS_TYPE_CGMS:
			was_proc=1; 
			xds_do_copy_generation_management_system (cur_xds_payload[2],cur_xds_payload[3]);
			break;
		case XDS_TYPE_PROGRAM_DESC_1:
		case XDS_TYPE_PROGRAM_DESC_2:
		case XDS_TYPE_PROGRAM_DESC_3:
		case XDS_TYPE_PROGRAM_DESC_4:
		case XDS_TYPE_PROGRAM_DESC_5:
		case XDS_TYPE_PROGRAM_DESC_6:
		case XDS_TYPE_PROGRAM_DESC_7:
		case XDS_TYPE_PROGRAM_DESC_8:
			{
				was_proc=1;
				int changed=0;
				char xds_desc[33];
				int i;
				for (i=2;i<cur_xds_payload_length-1;i++)
					xds_desc[i-2]=cur_xds_payload[i];
				xds_desc[i-2]=0;
				
				if (xds_desc[0])
				{					
					int line_num=cur_xds_packet_type-XDS_TYPE_PROGRAM_DESC_1;
					if (strcmp (xds_desc, xds_program_description[line_num]))
						changed=1;
					if (changed)
					{
						mprint ("\rXDS description line %d: %s\n",line_num,xds_desc);
						strcpy (xds_program_description[line_num], xds_desc);
					}
					else
					{					
						dbg_print(CCX_DMT_XDS, "\rXDS description line %d: %s\n",line_num,xds_desc);
					}
					xdsprint("XDS description line %d: %s",line_num,xds_desc);
					activity_xds_program_description (line_num, xds_desc);
				}
				break;
			}
	}
	return was_proc;
}

int xds_do_channel ()
{
	int was_proc=0;
	switch (cur_xds_packet_type)
	{
		case XDS_TYPE_NETWORK_NAME:
			was_proc=1;
			char xds_network_name[33];
			int i;
			for (i=2;i<cur_xds_payload_length-1;i++)
				xds_network_name[i-2]=cur_xds_payload[i];
			xds_network_name[i-2]=0;
			dbg_print(CCX_DMT_XDS, "XDS Network name: %s\n",xds_network_name);
			xdsprint ("Network: %s",xds_network_name);
			if (strcmp (xds_network_name, current_xds_network_name)) // Change of station
			{
				mprint ("XDS Notice: Network is now %s\n", xds_network_name);
				strcpy (current_xds_network_name,xds_network_name);
			}
			break;
		case XDS_TYPE_CALL_LETTERS_AND_CHANNEL:
			{
				was_proc=1;
				char xds_call_letters[33];
				if (cur_xds_payload_length<7) // We need 4-6 data bytes
					break;												
				for (i=2;i<cur_xds_payload_length-1;i++)
				{
					if (cur_xds_payload)
						xds_call_letters[i-2]=cur_xds_payload[i];
				}
				xds_call_letters[i-2]=0;				
				dbg_print(CCX_DMT_XDS, "XDS Network call letters: %s\n",xds_call_letters);
				xdsprint ("Call Letters: %s",xds_call_letters);
				if (strcmp (xds_call_letters, current_xds_call_letters)) // Change of station
				{
					mprint ("XDS Notice: Network call letters now %s\n", xds_call_letters);
					strcpy (current_xds_call_letters,xds_call_letters);
					activity_xds_network_call_letters (current_xds_call_letters);
				}
			}
			break;
		case XDS_TYPE_TSID:
			// According to CEA-608, data here (4 bytes) are used to identify the 
			// "originating analog licensee". No interesting data for us.
			was_proc=1;
			if (cur_xds_payload_length<7) // We need 4 data bytes
				break;												
			unsigned b1=(cur_xds_payload[2])&0x10; // Only low 4 bits from each byte
			unsigned b2=(cur_xds_payload[3])&0x10;
			unsigned b3=(cur_xds_payload[4])&0x10;
			unsigned b4=(cur_xds_payload[5])&0x10;
			unsigned tsid=(b4<<12) | (b3<<8) | (b2<<4) | b1;
			if (tsid)
				xdsprint ("TSID: %u",tsid);
			break;
	}
	return was_proc;
}



int xds_do_private_data ()
{
	if (wbxdsout==NULL) // Only thing we can do with private data is dump it.
		return 1;
	xds_write_transcript_line_prefix (wbxdsout);
	for (int i=2;i<cur_xds_payload_length-1;i++)
		fdprintf(wbxdsout->fh, "%02X ",cur_xds_payload[i]);		

	xds_write_transcript_line_suffix (wbxdsout);
	return 1;
}

int xds_do_misc ()
{
	int was_proc=0;
	switch (cur_xds_packet_type)
	{				
		case XDS_TYPE_TIME_OF_DAY:
		{
			was_proc=1;
			if (cur_xds_payload_length<9) // We need 6 data bytes
				break;
			int min=cur_xds_payload[2] & 0x3f; // 6 bits
			int hour = cur_xds_payload[3] & 0x1f; // 5 bits
			int date = cur_xds_payload[4] & 0x1f; // 5 bits
			int month = cur_xds_payload[5] & 0xf; // 4 bits
			int reset_seconds = (cur_xds_payload[5] & 0x20); 
			int day_of_week = cur_xds_payload[6] & 0x7;
			int year = (cur_xds_payload[7] & 0x3f) + 1990;
			dbg_print(CCX_DMT_XDS, "Time of day: (YYYY/MM/DD) %04d/%02d/%02d (HH:SS) %02d:%02d DoW: %d  Reset seconds: %d\n",							
					year,month,date,hour,min, day_of_week, reset_seconds);				
			break;
		}
		case XDS_TYPE_LOCAL_TIME_ZONE:
		{
			was_proc=1;
			if (cur_xds_payload_length<5) // We need 2 data bytes
				break;
			int b6 = (cur_xds_payload[2] & 0x40) >>6; // Bit 6 should always be 1
			int dst = (cur_xds_payload[2] & 0x20) >>5; // Daylight Saving Time
			int hour = cur_xds_payload[2] & 0x1f; // 5 bits
			dbg_print(CCX_DMT_XDS, "Local Time Zone: %02d DST: %d\n",
					hour, dst);	
			break;
		}
		default:
			was_proc=0;
			break;
	} 
	return was_proc;
}

void do_end_of_xds (unsigned char expected_checksum)
{
	if (cur_xds_buffer_idx== -1 || /* Unknown buffer, or not in use (bug) */
		!xds_buffers[cur_xds_buffer_idx].in_use)
		return;
	cur_xds_packet_class=xds_buffers[cur_xds_buffer_idx].xds_class;	
	cur_xds_payload=xds_buffers[cur_xds_buffer_idx].bytes;
	cur_xds_payload_length=xds_buffers[cur_xds_buffer_idx].used_bytes;
	cur_xds_packet_type=cur_xds_payload[1];
	cur_xds_payload[cur_xds_payload_length++]=0x0F; // The end byte itself, added to the packet
	
	int cs=0;
	for (int i=0; i<cur_xds_payload_length;i++)
	{
		cs=cs+cur_xds_payload[i];
		cs=cs & 0x7f; // Keep 7 bits only
		int c=cur_xds_payload[i]&0x7F;
		dbg_print(CCX_DMT_XDS, "%02X - %c cs: %02X\n",
			c,(c>=0x20)?c:'?', cs);
	}
	cs=(128-cs) & 0x7F; // Convert to 2's complement & discard high-order bit

	dbg_print(CCX_DMT_XDS, "End of XDS. Class=%d (%s), size=%d  Checksum OK: %d   Used buffers: %d\n",
			cur_xds_packet_class,XDSclasses[cur_xds_packet_class],
			cur_xds_payload_length,
			cs==expected_checksum, how_many_used());	

	if (cs!=expected_checksum || cur_xds_payload_length<3)
	{
		dbg_print(CCX_DMT_XDS, "Expected checksum: %02X  Calculated: %02X\n", expected_checksum, cs);
		clear_xds_buffer (cur_xds_buffer_idx); 
		return; // Bad packets ignored as per specs
	}
	
	int was_proc=0; /* Indicated if the packet was processed. Not processed means "code to do it doesn't exist yet", not an error. */
	
	switch (cur_xds_packet_class)
	{
		case XDS_CLASS_FUTURE: // Info on future program
			if (!(ccx_options.debug_mask & CCX_DMT_XDS)) // Don't bother processing something we don't need
			{
				was_proc=1;
				break; 
			}
		case XDS_CLASS_CURRENT: // Info on current program	
			was_proc = xds_do_current_and_future();
			break;
		case XDS_CLASS_CHANNEL:
			was_proc = xds_do_channel();
			break;
			
		case XDS_CLASS_MISC:
			was_proc = xds_do_misc();
			break;
		case XDS_CLASS_PRIVATE: // CEA-608:
			// The Private Data Class is for use in any closed system for whatever that 
			// system wishes. It shall not be defined by this standard now or in the future. 			 
			was_proc=xds_do_private_data();
			break;
	}
	
	if (!was_proc)
	{
		mprint ("Note: We found an currently unsupported XDS packet.\n");
	}
	clear_xds_buffer (cur_xds_buffer_idx);

}
