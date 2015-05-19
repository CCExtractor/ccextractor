#include "ccx_decoders_xds.h"
#include "ccx_common_constants.h"
#include "ccx_common_timing.h"

LLONG ts_start_of_xds = -1; // Time at which we switched to XDS mode, =-1 hasn't happened yet

struct ccx_decoders_xds_context_t {
	ccx_encoders_transcript_format transcriptFormat;
	LLONG subsDelay;
	char millisSeparator;
} ccx_decoders_xds_context;

// Program Identification Number (Start Time) for current program
static int current_xds_min=-1;
static int current_xds_hour=-1;
static int current_xds_date=-1;
static int current_xds_month=-1;
static int current_program_type_reported=0; // No.
static int xds_start_time_shown=0;
static int xds_program_length_shown=0;
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
#define XDS_CLASS_OUT_OF_BAND 0x40 // Not a real class, a marker for packets for out-of-band data.

// Types for the classes current and future
#define XDS_TYPE_PIN_START_TIME	1
#define XDS_TYPE_LENGH_AND_CURRENT_TIME	2
#define XDS_TYPE_PROGRAM_NAME 3
#define XDS_TYPE_PROGRAM_TYPE 4
#define XDS_TYPE_CONTENT_ADVISORY 5
#define XDS_TYPE_AUDIO_SERVICES 6
#define XDS_TYPE_CGMS 8 // Copy Generation Management System
#define XDS_TYPE_ASPECT_RATIO_INFO 9 // Appears in CEA-608-B but in E it's been removed as is "reserved"
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
#define XDS_TYPE_OUT_OF_BAND_CHANNEL_NUMBER 0x40

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

void ccx_decoders_xds_init_library(ccx_encoders_transcript_format *transcriptSettings, LLONG subs_delay, char millis_separator)
{
	ccx_decoders_xds_context.transcriptFormat = *transcriptSettings;
	ccx_decoders_xds_context.subsDelay = subs_delay;
	ccx_decoders_xds_context.millisSeparator = millis_separator;

	for (int i = 0; i<NUM_XDS_BUFFERS; i++)
	{
		xds_buffers[i].in_use = 0;
		xds_buffers[i].xds_class = -1;
		xds_buffers[i].xds_type = -1;
		xds_buffers[i].used_bytes = 0;
		memset(xds_buffers[i].bytes, 0, NUM_BYTES_PER_PACKET);
	}
	for (int i = 0; i<9; i++)
		memset(xds_program_description, 0, 32);

	memset(current_xds_network_name, 0, 33);
	memset(current_xds_program_name, 0, 33);
	memset(current_xds_call_letters, 0, 7);
	memset(current_xds_program_type, 0, 33);
}

int write_xds_string(struct cc_subtitle *sub,char *p,size_t len)
{
	struct eia608_screen *data = NULL;
	data = (struct eia608_screen *) realloc(sub->data,( sub->nb_data + 1 ) * sizeof(*data));
	if (!data)
	{
		freep(&sub->data);
		sub->nb_data = 0;
		ccx_common_logging.log_ftn("No Memory left");
		return -1;
	}
	else
	{
		sub->data = data;
		data = (struct eia608_screen *)sub->data + sub->nb_data;
		data->format = SFORMAT_XDS;
		data->start_time = ts_start_of_xds;
		data->end_time =  get_fts();
		data->xds_str = p;
		data->xds_len = len;
		data->cur_xds_packet_class = cur_xds_packet_class;
		sub->nb_data++;
		sub->type = CC_608;
		sub->got_output = 1;
	}

	return 0;

}


void xds_write_transcript_line_suffix (struct ccx_s_write *wb)
{
	if (!wb || wb->fh==-1)
		return;
	write (wb->fh, encoded_crlf, encoded_crlf_length);
}

void xds_write_transcript_line_prefix (struct ccx_s_write *wb, LLONG start_time, LLONG end_time, int cur_xds_packet_class)
{
	unsigned h1,m1,s1,ms1;
	unsigned h2,m2,s2,ms2;
	if (!wb || wb->fh==-1)
		return;

	if (start_time == -1)
	{
		// Means we entered XDS mode without making a note of the XDS start time. This is a bug.
		ccx_common_logging.fatal_ftn(CCX_COMMON_EXIT_BUG_BUG, "Bug in timedtranscript (XDS). Please report.");
	}

	if (ccx_decoders_xds_context.transcriptFormat.showStartTime)
	{
		char buffer[80];
		if (ccx_decoders_xds_context.transcriptFormat.relativeTimestamp)
		{
			if (utc_refvalue == UINT64_MAX)
			{
				mstotime(start_time + ccx_decoders_xds_context.subsDelay, &h1, &m1, &s1, &ms1);
				fdprintf(wb->fh, "%02u:%02u:%02u%c%03u|", h1, m1, s1, ccx_decoders_xds_context.millisSeparator, ms1);
			}
			else {
				fdprintf(wb->fh, "%lld%c%03d|", (start_time + ccx_decoders_xds_context.subsDelay) / 1000, ccx_decoders_xds_context.millisSeparator, (start_time + ccx_decoders_xds_context.subsDelay) % 1000);
			}
		}
		else
		{
			mstotime(start_time + ccx_decoders_xds_context.subsDelay, &h1, &m1, &s1, &ms1);
			time_t start_time_int = (start_time + ccx_decoders_xds_context.subsDelay) / 1000;
			int start_time_dec = (start_time + ccx_decoders_xds_context.subsDelay) % 1000;
			struct tm *start_time_struct = gmtime(&start_time_int);
			strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", start_time_struct);
			fdprintf(wb->fh, "%s%c%03d|", buffer, ccx_decoders_xds_context.millisSeparator, start_time_dec);
		}
	}

	if (ccx_decoders_xds_context.transcriptFormat.showEndTime)
	{
		char buffer[80];
		if (ccx_decoders_xds_context.transcriptFormat.relativeTimestamp)
		{
			if (utc_refvalue == UINT64_MAX)
			{
				mstotime(end_time + ccx_decoders_xds_context.subsDelay, &h2, &m2, &s2, &ms2);
				fdprintf(wb->fh, "%02u:%02u:%02u%c%03u|", h2, m2, s2, ccx_decoders_xds_context.millisSeparator, ms2);
			}
			else
			{
				fdprintf(wb->fh, "%lld%s%03d|", (end_time + ccx_decoders_xds_context.subsDelay) / 1000, ccx_decoders_xds_context.millisSeparator, (end_time + ccx_decoders_xds_context.subsDelay) % 1000);
			}
		}
		else
		{
			mstotime(end_time + ccx_decoders_xds_context.subsDelay, &h2, &m2, &s2, &ms2);
			time_t end_time_int = (end_time + ccx_decoders_xds_context.subsDelay) / 1000;
			int end_time_dec = (end_time + ccx_decoders_xds_context.subsDelay) % 1000;
			struct tm *end_time_struct = gmtime(&end_time_int);
			strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", end_time_struct);
			fdprintf(wb->fh, "%s%c%03d|", buffer, ccx_decoders_xds_context.millisSeparator, end_time_dec);
		}
	}

	if (ccx_decoders_xds_context.transcriptFormat.showMode)
	{
		const char *mode = "XDS";
		fdprintf(wb->fh, "%s|", mode);
	}

	if (ccx_decoders_xds_context.transcriptFormat.showCC)
	{
		fdprintf(wb->fh, "%s|", XDSclasses_short[cur_xds_packet_class]);
	}
}
void xdsprint (struct cc_subtitle *sub,const char *fmt,...)
{
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
			write_xds_string(sub, p, n);
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
			return ;
		}
		else
		{
			p = np;
		}
	}
}

void xds_debug_test(struct cc_subtitle *sub)
{
	process_xds_bytes (0x05,0x02);
	process_xds_bytes (0x20,0x20);
	do_end_of_xds (sub, 0x2a);

}

void xds_cea608_test(struct cc_subtitle *sub)
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
	do_end_of_xds (sub, 0x1d);
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
		ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "XDS Start: %u.%u  Is new: %d  | Class: %d (%s), Used buffers: %d\n",hi,lo, is_new,xds_class, XDSclasses[xds_class], how_many_used());
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
			ccx_common_logging.log_ftn ("Note: All XDS buffers full (bug or suicidal stream). Ignoring this one (%d,%d).\n",xds_class,lo);
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
		ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "XDS: %02X.%02X (%c, %c)\n",hi,lo,hi,lo);
		if ((hi>0 && hi<=0x1f) || (lo>0 && lo<=0x1f))
		{
			ccx_common_logging.log_ftn ("\rNote: Illegal XDS data");
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

void xds_do_copy_generation_management_system (struct cc_subtitle *sub, unsigned c1, unsigned c2)
{
	static unsigned last_c1=-1, last_c2=-1;
	static char copy_permited[256];
	static char aps[256];
	static char rcd[256];
	int changed=0;
	unsigned c1_6=(c1&0x40)>>6;
	/* unsigned unused1=(c1&0x20)>>5; */
	unsigned cgms_a_b4=(c1&0x10)>>4;
	unsigned cgms_a_b3=(c1&0x8)>>3;
	unsigned aps_b2=(c1&0x4)>>2;
	unsigned aps_b1=(c1&0x2)>>1;
	/* unsigned asb_0=(c1&0x1); */
	unsigned c2_6=(c2&0x40)>>6;
	/* unsigned c2_5=(c2&0x20)>>5; */
	/* unsigned c2_4=(c2&0x10)>>4; */
	/* unsigned c2_3=(c2&0x8)>>3; */
	/* unsigned c2_2=(c2&0x4)>>2; */
	/* unsigned c2_1=(c2&0x2)>>1; */
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

	if (ccx_decoders_xds_context.transcriptFormat.xds)
	{
		xdsprint(sub, copy_permited);
		xdsprint(sub, aps);
		xdsprint(sub, rcd);
	}
	if (changed)
	{
		ccx_common_logging.log_ftn ("\rXDS: %s\n",copy_permited);
		ccx_common_logging.log_ftn ("\rXDS: %s\n",aps);
		ccx_common_logging.log_ftn ("\rXDS: %s\n",rcd);
	}
	ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "\rXDS: %s\n",copy_permited);
	ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "\rXDS: %s\n",aps);
	ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "\rXDS: %s\n",rcd);
}

void xds_do_content_advisory (struct cc_subtitle *sub, unsigned c1, unsigned c2)
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
		if (a0 && a1 && Da2 && !La3) // Canadian French Language Rating
		{
			const char *ratingtext[8] = { "Exemptées", "Général", "Général - Déconseillé aux jeunes enfants",
				"Cette émission peut ne pas convenir aux enfants de moins de 13 ans",
				"Cette émission ne convient pas aux moins de 16 ans",
				"Cette émission est réservée aux adultes", "[invalid]", "[invalid]" };
			sprintf(rating, "ContentAdvisory: Canadian French Rating: %s", ratingtext[g2 * 4 + g1 * 2 + g0]);
			supported = 1;
		}

	}
	// Bits a1 and a0 determine the encoding. I'll add parsing as more samples become available
	if (!a1 && a0) // US TV parental guidelines
	{
		if (ccx_decoders_xds_context.transcriptFormat.xds)
		{
			xdsprint(sub, age);
			xdsprint(sub, content);
		}
		if (changed)
		{
			ccx_common_logging.log_ftn ("\rXDS: %s\n  ",age);
			ccx_common_logging.log_ftn ("\rXDS: %s\n  ",content);
		}
		ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "\rXDS: %s\n",age);
		ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "\rXDS: %s\n",content);
	}
	if (!a0 || // MPA
			(a0 && a1 && !Da2 && !La3) ||  // Canadian English Language Rating
			(a0 && a1 && Da2 && !La3) // Canadian French Language Rating
	   )
	{
		if (ccx_decoders_xds_context.transcriptFormat.xds)
			xdsprint(sub, rating);
		if (changed)
			ccx_common_logging.log_ftn ("\rXDS: %s\n  ",rating);
		ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "\rXDS: %s\n",rating);
	}

	if (changed && !supported)
		ccx_common_logging.log_ftn ("XDS: Unsupported ContentAdvisory encoding, please submit sample.\n");


}

int xds_do_current_and_future (struct cc_subtitle *sub)
{
	int was_proc=0;

	char *str = malloc(1024);
	char *tstr = NULL;
	int str_len = 1024;

	switch (cur_xds_packet_type)
	{
		case XDS_TYPE_PIN_START_TIME:
			was_proc=1;
			if (cur_xds_payload_length<7) // We need 4 data bytes
				break;
			int min=cur_xds_payload[2] & 0x3f; // 6 bits
			int hour = cur_xds_payload[3] & 0x1f; // 5 bits
			int date = cur_xds_payload[4] & 0x1f; // 5 bits
			int month = cur_xds_payload[5] & 0xf; // 4 bits
			/* int changed=0; */
			if (current_xds_min!=min ||
					current_xds_hour!=hour ||
					current_xds_date!=date ||
					current_xds_month!=month)
			{
				/* changed=1; */
				xds_start_time_shown=0;
				current_xds_min=min;
				current_xds_hour=hour;
				current_xds_date=date;
				current_xds_month=month;
			}

			ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "PIN (Start Time): %s  %02d-%02d %02d:%02d\n",
					(cur_xds_packet_class==XDS_CLASS_CURRENT?"Current":"Future"),
					date,month,hour,min);
			if (ccx_decoders_xds_context.transcriptFormat.xds)
				xdsprint (sub, "PIN (Start Time): %s  %02d-%02d %02d:%02d\n",
						(cur_xds_packet_class==XDS_CLASS_CURRENT?"Current":"Future"),
						date,month,hour,min);

			if (!xds_start_time_shown && cur_xds_packet_class==XDS_CLASS_CURRENT)
			{
				ccx_common_logging.log_ftn("\rXDS: Program changed.\n");
				ccx_common_logging.log_ftn ("XDS program start time (DD/MM HH:MM) %02d-%02d %02d:%02d\n",date,month,hour,min);
				ccx_common_logging.gui_ftn(CCX_COMMON_LOGGING_GUI_XDS_PROGRAM_ID_NR, current_xds_min, current_xds_hour, current_xds_date, current_xds_month);
				xds_start_time_shown=1;
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
					ccx_common_logging.log_ftn ("\rXDS: Program length (HH:MM): %02d:%02d  ",hour,min);
				else
					ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "\rXDS: Program length (HH:MM): %02d:%02d  ",hour,min);

				if (ccx_decoders_xds_context.transcriptFormat.xds)
					xdsprint(sub, "Program length (HH:MM): %02d:%02d  ",hour,min);

				if (cur_xds_payload_length>6) // Next two bytes (optional) available
				{
					int el_min=cur_xds_payload[4] & 0x3f; // 6 bits
					int el_hour = cur_xds_payload[5] & 0x1f; // 5 bits
					if (!xds_program_length_shown)
						ccx_common_logging.log_ftn ("Elapsed (HH:MM): %02d:%02d",el_hour,el_min);
					else
						ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "Elapsed (HH:MM): %02d:%02d",el_hour,el_min);
					if (ccx_decoders_xds_context.transcriptFormat.xds)
						xdsprint(sub, "Elapsed (HH:MM): %02d:%02d",el_hour,el_min);

				}
				if (cur_xds_payload_length>8) // Next two bytes (optional) available
				{
					int el_sec=cur_xds_payload[6] & 0x3f; // 6 bits
					if (!xds_program_length_shown)
						ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, ":%02d",el_sec);
					if (ccx_decoders_xds_context.transcriptFormat.xds)
						xdsprint(sub, "Elapsed (SS) :%02d",el_sec);
				}
				if (!xds_program_length_shown)
					ccx_common_logging.log_ftn("\n");
				else
					ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "\n");
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
				ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "\rXDS Program name: %s\n",xds_program_name);
				if (ccx_decoders_xds_context.transcriptFormat.xds)
					xdsprint(sub, "Program name: %s",xds_program_name);
				if (cur_xds_packet_class==XDS_CLASS_CURRENT &&
						strcmp (xds_program_name, current_xds_program_name)) // Change of program
				{
					ccx_common_logging.log_ftn ("\rXDS Notice: Program is now %s\n", xds_program_name);
					strncpy (current_xds_program_name,xds_program_name, 33);
					ccx_common_logging.gui_ftn(CCX_COMMON_LOGGING_GUI_XDS_PROGRAM_NAME, xds_program_name);
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
			if (!(ccx_common_logging.debug_mask & CCX_DMT_DECODER_XDS) && current_program_type_reported &&
					ccx_decoders_xds_context.transcriptFormat.xds == 0){
				break;
			}
			memcpy (current_xds_program_type,cur_xds_payload,cur_xds_payload_length);
			current_xds_program_type[cur_xds_payload_length]=0;
			if (!current_program_type_reported)
				ccx_common_logging.log_ftn ("\rXDS Program Type: ");

			*str = '\0';
			tstr = str;
			for (int i=2;i<cur_xds_payload_length - 1; i++)
			{
				if (cur_xds_payload[i]==0) // Padding
					continue;
				if (!current_program_type_reported)
					ccx_common_logging.log_ftn ("[%02X-", cur_xds_payload[i]);
				if (ccx_decoders_xds_context.transcriptFormat.xds)
				{
					if (cur_xds_payload[i]>=0x20 && cur_xds_payload[i]<0x7F)
					{
						snprintf(tstr,str_len - (tstr - str),"[%s] ",XDSProgramTypes[cur_xds_payload[i]-0x20]);
						tstr += strlen(tstr);
					}
				}
				if (!current_program_type_reported)
				{
					if (cur_xds_payload[i]>=0x20 && cur_xds_payload[i]<0x7F)
						ccx_common_logging.log_ftn ("%s",XDSProgramTypes[cur_xds_payload[i]-0x20]);
					else
						ccx_common_logging.log_ftn ("ILLEGAL VALUE");
					ccx_common_logging.log_ftn ("] ");
				}
			}
			if (ccx_decoders_xds_context.transcriptFormat.xds)
				xdsprint(sub,"Program type %s",str);
			if (!current_program_type_reported)
				ccx_common_logging.log_ftn ("\n");
			current_program_type_reported=1;
			break;
		case XDS_TYPE_CONTENT_ADVISORY:
			was_proc=1;
			if (cur_xds_payload_length<5) // We need 2 data bytes
				break;
			xds_do_content_advisory (sub, cur_xds_payload[2],cur_xds_payload[3]);
			break;
		case XDS_TYPE_AUDIO_SERVICES:
			was_proc=1; // I don't have any sample with this.
			break;
		case XDS_TYPE_CGMS:
			was_proc=1;
			xds_do_copy_generation_management_system (sub, cur_xds_payload[2],cur_xds_payload[3]);
			break;
		case XDS_TYPE_ASPECT_RATIO_INFO:
			{
				unsigned ar_start, ar_end;
				was_proc = 1;
				if (cur_xds_payload_length < 5) // We need 2 data bytes
					break;
				if (!cur_xds_payload[2] & 20 || !cur_xds_payload[3] & 20) // Bit 6 must be 1
					break;
				/* CEA-608-B: The starting line is computed by adding 22 to the decimal number
				   represented by bits S0 to S5. The ending line is computing by substracting
				   the decimal number represented by bits E0 to E5 from 262 */
				ar_start = (cur_xds_payload[2] & 0x1F) + 22;
				ar_end = 262 - (cur_xds_payload[3] & 0x1F);
				unsigned active_picture_height = ar_end - ar_start;
				float aspect_ratio = (float) 320 / active_picture_height;
				ccx_common_logging.log_ftn("\rXDS Notice: Aspect ratio info, start line=%u, end line=%u\n", ar_start,ar_end);
				ccx_common_logging.log_ftn("\rXDS Notice: Aspect ratio info, active picture height=%u, ratio=%f\n", active_picture_height, aspect_ratio);

			}
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
						ccx_common_logging.log_ftn ("\rXDS description line %d: %s\n",line_num,xds_desc);
						strcpy (xds_program_description[line_num], xds_desc);
					}
					else
					{
						ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "\rXDS description line %d: %s\n",line_num,xds_desc);
					}
					if (ccx_decoders_xds_context.transcriptFormat.xds)
						xdsprint(sub, "XDS description line %d: %s",line_num,xds_desc);
					ccx_common_logging.gui_ftn(CCX_COMMON_LOGGING_GUI_XDS_PROGRAM_DESCRIPTION, line_num, xds_desc);
				}
				break;
			}
	}

	free(str);
	return was_proc;
}

int xds_do_channel (struct cc_subtitle *sub)
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
			ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "XDS Network name: %s\n",xds_network_name);
			if (ccx_decoders_xds_context.transcriptFormat.xds)
				xdsprint (sub, "Network: %s",xds_network_name);
			if (strcmp (xds_network_name, current_xds_network_name)) // Change of station
			{
				ccx_common_logging.log_ftn ("XDS Notice: Network is now %s\n", xds_network_name);
				strcpy (current_xds_network_name,xds_network_name);
			}
			break;
		case XDS_TYPE_CALL_LETTERS_AND_CHANNEL:
			{
				was_proc=1;
				char xds_call_letters[7];
				if (cur_xds_payload_length != 7 && cur_xds_payload_length != 9) // We need 4-6 data bytes
					break;
				for (i=2;i<cur_xds_payload_length-1;i++)
				{
					if (cur_xds_payload)
						xds_call_letters[i-2]=cur_xds_payload[i];
				}
				xds_call_letters[i-2]=0;
				ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "XDS Network call letters: %s\n",xds_call_letters);
				if (ccx_decoders_xds_context.transcriptFormat.xds)
					xdsprint (sub, "Call Letters: %s",xds_call_letters);
				if (strncmp (xds_call_letters, current_xds_call_letters, 7)) // Change of station
				{
					ccx_common_logging.log_ftn ("XDS Notice: Network call letters now %s\n", xds_call_letters);
					strncpy (current_xds_call_letters, xds_call_letters, 7);
					ccx_common_logging.gui_ftn(CCX_COMMON_LOGGING_GUI_XDS_CALL_LETTERS, current_xds_call_letters);
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
			if (tsid && ccx_decoders_xds_context.transcriptFormat.xds)
				xdsprint (sub, "TSID: %u",tsid);
			break;
	}
	return was_proc;
}



int xds_do_private_data (struct cc_subtitle *sub)
{
	char *str = malloc((cur_xds_payload_length *3) + 1);
	if (str==NULL) // Only thing we can do with private data is dump it.
		return 1;
	for (int i=2;i<cur_xds_payload_length-1;i++)
		sprintf(str, "%02X ",cur_xds_payload[i]);

	xdsprint(sub,str);
	free(str);
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
				ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "Time of day: (YYYY/MM/DD) %04d/%02d/%02d (HH:SS) %02d:%02d DoW: %d  Reset seconds: %d\n",
						year,month,date,hour,min, day_of_week, reset_seconds);
				break;
			}
		case XDS_TYPE_LOCAL_TIME_ZONE:
			{
				was_proc=1;
				if (cur_xds_payload_length<5) // We need 2 data bytes
					break;
				// int b6 = (cur_xds_payload[2] & 0x40) >>6; // Bit 6 should always be 1
				int dst = (cur_xds_payload[2] & 0x20) >>5; // Daylight Saving Time
				int hour = cur_xds_payload[2] & 0x1f; // 5 bits
				ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "Local Time Zone: %02d DST: %d\n",
						hour, dst);
				break;
			}
		default:
			was_proc=0;
			break;
	}
	return was_proc;
}

void do_end_of_xds (struct cc_subtitle *sub, unsigned char expected_checksum)
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
		ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "%02X - %c cs: %02X\n",
				c,(c>=0x20)?c:'?', cs);
	}
	cs=(128-cs) & 0x7F; // Convert to 2's complement & discard high-order bit

	ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "End of XDS. Class=%d (%s), size=%d  Checksum OK: %d   Used buffers: %d\n",
			cur_xds_packet_class,XDSclasses[cur_xds_packet_class],
			cur_xds_payload_length,
			cs==expected_checksum, how_many_used());

	if (cs!=expected_checksum || cur_xds_payload_length<3)
	{
		ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "Expected checksum: %02X  Calculated: %02X\n", expected_checksum, cs);
		clear_xds_buffer (cur_xds_buffer_idx);
		return; // Bad packets ignored as per specs
	}

	int was_proc=0; /* Indicated if the packet was processed. Not processed means "code to do it doesn't exist yet", not an error. */
	if (cur_xds_packet_type & 0x40) // Bit 6 set
	{
		cur_xds_packet_class = XDS_CLASS_OUT_OF_BAND;
	}

	switch (cur_xds_packet_class)
	{
		case XDS_CLASS_FUTURE: // Info on future program
			if (!(ccx_common_logging.debug_mask & CCX_DMT_DECODER_XDS)) // Don't bother processing something we don't need
			{
				was_proc=1;
				break;
			}
		case XDS_CLASS_CURRENT: // Info on current program
			was_proc = xds_do_current_and_future(sub);
			break;
		case XDS_CLASS_CHANNEL:
			was_proc = xds_do_channel(sub);
			break;

		case XDS_CLASS_MISC:
			was_proc = xds_do_misc();
			break;
		case XDS_CLASS_PRIVATE: // CEA-608:
			// The Private Data Class is for use in any closed system for whatever that
			// system wishes. It shall not be defined by this standard now or in the future.
			if (ccx_decoders_xds_context.transcriptFormat.xds)
				was_proc=xds_do_private_data(sub);
			break;
		case XDS_CLASS_OUT_OF_BAND:
			ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "Out-of-band data, ignored.");
			was_proc = 1;
			break;
	}

	if (!was_proc)
	{
		ccx_common_logging.log_ftn ("Note: We found an currently unsupported XDS packet.\n");
	}
	clear_xds_buffer (cur_xds_buffer_idx);

}
