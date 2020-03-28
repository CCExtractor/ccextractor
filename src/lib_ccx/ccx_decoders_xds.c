#include "ccx_decoders_xds.h"
#include "ccx_common_constants.h"
#include "ccx_common_timing.h"
#include "ccx_common_common.h"
#include "utility.h"

LLONG ts_start_of_xds = -1; // Time at which we switched to XDS mode, =-1 hasn't happened yet

static const char *XDSclasses[] =
    {
	"Current",
	"Future",
	"Channel",
	"Miscellaneous",
	"Public service",
	"Reserved",
	"Private data",
	"End"};

static const char *XDSProgramTypes[] =
    {
	"Education", "Entertainment", "Movie", "News", "Religious",
	"Sports", "Other", "Action", "Advertisement", "Animated",
	"Anthology", "Automobile", "Awards", "Baseball", "Basketball",
	"Bulletin", "Business", "Classical", "College", "Combat",
	"Comedy", "Commentary", "Concert", "Consumer", "Contemporary",
	"Crime", "Dance", "Documentary", "Drama", "Elementary",
	"Erotica", "Exercise", "Fantasy", "Farm", "Fashion",
	"Fiction", "Food", "Football", "Foreign", "Fund-Raiser",
	"Game/Quiz", "Garden", "Golf", "Government", "Health",
	"High_School", "History", "Hobby", "Hockey", "Home",
	"Horror", "Information", "Instruction", "International", "Interview",
	"Language", "Legal", "Live", "Local", "Math",
	"Medical", "Meeting", "Military", "Mini-Series", "Music",
	"Mystery", "National", "Nature", "Police", "Politics",
	"Premiere", "Pre-Recorded", "Product", "Professional", "Public",
	"Racing", "Reading", "Repair", "Repeat", "Review",
	"Romance", "Science", "Series", "Service", "Shopping",
	"Soap_Opera", "Special", "Suspense", "Talk", "Technical",
	"Tennis", "Travel", "Variety", "Video", "Weather",
	"Western"};

#define XDS_CLASS_CURRENT 0
#define XDS_CLASS_FUTURE 1
#define XDS_CLASS_CHANNEL 2
#define XDS_CLASS_MISC 3
#define XDS_CLASS_PUBLIC 4
#define XDS_CLASS_RESERVED 5
#define XDS_CLASS_PRIVATE 6
#define XDS_CLASS_END 7
#define XDS_CLASS_OUT_OF_BAND 0x40 // Not a real class, a marker for packets for out-of-band data.

// Types for the classes current and future
#define XDS_TYPE_PIN_START_TIME 1
#define XDS_TYPE_LENGH_AND_CURRENT_TIME 2
#define XDS_TYPE_PROGRAM_NAME 3
#define XDS_TYPE_PROGRAM_TYPE 4
#define XDS_TYPE_CONTENT_ADVISORY 5
#define XDS_TYPE_AUDIO_SERVICES 6
#define XDS_TYPE_CGMS 8		     // Copy Generation Management System
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
#define XDS_TYPE_TSID 4 // Transmission Signal Identifier

// Types for miscellaneous packets
#define XDS_TYPE_TIME_OF_DAY 1
#define XDS_TYPE_LOCAL_TIME_ZONE 4
#define XDS_TYPE_OUT_OF_BAND_CHANNEL_NUMBER 0x40

struct ccx_decoders_xds_context *ccx_decoders_xds_init_library(struct ccx_common_timing_ctx *timing, int xds_write_to_file)
{
	int i;
	struct ccx_decoders_xds_context *ctx = NULL;

	ctx = malloc(sizeof(struct ccx_decoders_xds_context));
	if (!ctx)
		return NULL;

	ctx->current_xds_min = -1;
	ctx->current_xds_hour = -1;
	ctx->current_xds_date = -1;
	ctx->current_xds_month = -1;
	ctx->current_program_type_reported = 0; // No.
	ctx->xds_start_time_shown = 0;
	ctx->xds_program_length_shown = 0;

	for (i = 0; i < NUM_XDS_BUFFERS; i++)
	{
		ctx->xds_buffers[i].in_use = 0;
		ctx->xds_buffers[i].xds_class = -1;
		ctx->xds_buffers[i].xds_type = -1;
		ctx->xds_buffers[i].used_bytes = 0;
		memset(ctx->xds_buffers[i].bytes, 0, NUM_BYTES_PER_PACKET);
	}
	for (i = 0; i < 9; i++)
		memset(ctx->xds_program_description, 0, 32);

	memset(ctx->current_xds_network_name, 0, 33);
	memset(ctx->current_xds_program_name, 0, 33);
	memset(ctx->current_xds_call_letters, 0, 7);
	memset(ctx->current_xds_program_type, 0, 33);

	ctx->cur_xds_buffer_idx = -1;
	ctx->cur_xds_packet_class = -1;
	ctx->cur_xds_payload = NULL;
	ctx->cur_xds_payload_length = 0;
	ctx->cur_xds_packet_type = 0;
	ctx->timing = timing;

	ctx->xds_write_to_file = xds_write_to_file;

	return ctx;
}

int write_xds_string(struct cc_subtitle *sub, struct ccx_decoders_xds_context *ctx, char *p, size_t len)
{
	struct eia608_screen *data = NULL;
	data = (struct eia608_screen *)realloc(sub->data, (sub->nb_data + 1) * sizeof(*data));
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
		sub->datatype = CC_DATATYPE_GENERIC;
		data = (struct eia608_screen *)sub->data + sub->nb_data;
		data->format = SFORMAT_XDS;
		data->start_time = ts_start_of_xds;
		data->end_time = get_fts(ctx->timing, 2);
		data->xds_str = p;
		data->xds_len = len;
		data->cur_xds_packet_class = ctx->cur_xds_packet_class;
		sub->nb_data++;
		sub->type = CC_608;
		sub->got_output = 1;
	}

	return 0;
}

void xdsprint(struct cc_subtitle *sub, struct ccx_decoders_xds_context *ctx, const char *fmt, ...)
{
	if (!ctx->xds_write_to_file)
		return;
	/* Guess we need no more than 100 bytes. */
	int n, size = 100;
	char *p, *np;
	va_list ap;

	if ((p = (char *)malloc(size)) == NULL)
		return;

	while (1)
	{
		/* Try to print in the allocated space. */
		va_start(ap, fmt);
		n = vsnprintf(p, size, fmt, ap);
		va_end(ap);
		/* If that worked, return the string. */
		if (n > -1 && n < size)
		{
			write_xds_string(sub, ctx, p, n);
			return;
		}
		/* Else try again with more space. */
		if (n > -1)	      /* glibc 2.1 */
			size = n + 1; /* precisely what is needed */
		else		      /* glibc 2.0 */
			size *= 2;    /* twice the old size */
		if ((np = (char *)realloc(p, size)) == NULL)
		{
			free(p);
			return;
		}
		else
		{
			p = np;
		}
	}
}

void xds_debug_test(struct ccx_decoders_xds_context *ctx, struct cc_subtitle *sub)
{
	process_xds_bytes(ctx, 0x05, 0x02);
	process_xds_bytes(ctx, 0x20, 0x20);
	do_end_of_xds(sub, ctx, 0x2a);
}

void xds_cea608_test(struct ccx_decoders_xds_context *ctx, struct cc_subtitle *sub)
{
	/* This test is the sample data that comes in CEA-608. It sets the program name
	   to be "Star Trek". The checksum is 0x1d and the validation must succeed. */
	process_xds_bytes(ctx, 0x01, 0x03);
	process_xds_bytes(ctx, 0x53, 0x74);
	process_xds_bytes(ctx, 0x61, 0x72);
	process_xds_bytes(ctx, 0x20, 0x54);
	process_xds_bytes(ctx, 0x72, 0x65);
	process_xds_bytes(ctx, 0x02, 0x03);
	process_xds_bytes(ctx, 0x02, 0x03);
	process_xds_bytes(ctx, 0x6b, 0x00);
	do_end_of_xds(sub, ctx, 0x1d);
}

int how_many_used(struct ccx_decoders_xds_context *ctx)
{
	int c = 0;
	for (int i = 0; i < NUM_XDS_BUFFERS; i++)
		if (ctx->xds_buffers[i].in_use)
			c++;
	return c;
}

void clear_xds_buffer(struct ccx_decoders_xds_context *ctx, int num)
{
	ctx->xds_buffers[num].in_use = 0;
	ctx->xds_buffers[num].xds_class = -1;
	ctx->xds_buffers[num].xds_type = -1;
	ctx->xds_buffers[num].used_bytes = 0;
	memset(ctx->xds_buffers[num].bytes, 0, NUM_BYTES_PER_PACKET);
}

void process_xds_bytes(struct ccx_decoders_xds_context *ctx, const unsigned char hi, int lo)
{
	int is_new;
	if (!ctx)
		return;

	if (hi >= 0x01 && hi <= 0x0f)
	{
		int xds_class = (hi - 1) / 2; // Start codes 1 and 2 are "class type" 0, 3-4 are 2, and so on.
		is_new = hi % 2;	      // Start codes are even
		ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "XDS Start: %u.%u  Is new: %d  | Class: %d (%s), Used buffers: %d\n",
					     hi, lo, is_new, xds_class, XDSclasses[xds_class], how_many_used(ctx));
		int first_free_buf = -1;
		int matching_buf = -1;
		for (int i = 0; i < NUM_XDS_BUFFERS; i++)
		{
			if (ctx->xds_buffers[i].in_use &&
			    ctx->xds_buffers[i].xds_class == xds_class &&
			    ctx->xds_buffers[i].xds_type == lo)
			{
				matching_buf = i;
				break;
			}
			if (first_free_buf == -1 && !ctx->xds_buffers[i].in_use)
				first_free_buf = i;
		}
		/* Here, 3 possibilities:
		   1) We already had a buffer for this class/type and matching_buf points to it
		   2) We didn't have a buffer for this class/type and first_free_buf points to an unused one
		   3) All buffers are full and we will have to skip this packet.
		 */
		if (matching_buf == -1 && first_free_buf == -1)
		{
			ccx_common_logging.log_ftn("Note: All XDS buffers full (bug or suicidal stream). Ignoring this one (%d,%d).\n", xds_class, lo);
			ctx->cur_xds_buffer_idx = -1;
			return;
		}
		ctx->cur_xds_buffer_idx = (matching_buf != -1) ? matching_buf : first_free_buf;

		if (is_new || !ctx->xds_buffers[ctx->cur_xds_buffer_idx].in_use)
		{
			// Whatever we had before we discard; must belong to an interrupted packet
			ctx->xds_buffers[ctx->cur_xds_buffer_idx].xds_class = xds_class;
			ctx->xds_buffers[ctx->cur_xds_buffer_idx].xds_type = lo;
			ctx->xds_buffers[ctx->cur_xds_buffer_idx].used_bytes = 0;
			ctx->xds_buffers[ctx->cur_xds_buffer_idx].in_use = 1;
			memset(ctx->xds_buffers[ctx->cur_xds_buffer_idx].bytes, 0, NUM_BYTES_PER_PACKET);
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
		ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "XDS: %02X.%02X (%c, %c)\n", hi, lo, hi, lo);
		if ((hi > 0 && hi <= 0x1f) || (lo > 0 && lo <= 0x1f))
		{
			ccx_common_logging.log_ftn("\rNote: Illegal XDS data");
			return;
		}
	}
	if (ctx->xds_buffers[ctx->cur_xds_buffer_idx].used_bytes <= 32)
	{
		// Should always happen
		ctx->xds_buffers[ctx->cur_xds_buffer_idx].bytes[ctx->xds_buffers[ctx->cur_xds_buffer_idx].used_bytes++] = hi;
		ctx->xds_buffers[ctx->cur_xds_buffer_idx].bytes[ctx->xds_buffers[ctx->cur_xds_buffer_idx].used_bytes++] = lo;
		ctx->xds_buffers[ctx->cur_xds_buffer_idx].bytes[ctx->xds_buffers[ctx->cur_xds_buffer_idx].used_bytes] = 0;
	}
}
/**
 * ctx XDS context can be NULL, if user don't want to write xds in transcript
 */
void xds_do_copy_generation_management_system(struct cc_subtitle *sub, struct ccx_decoders_xds_context *ctx, unsigned c1, unsigned c2)
{
	static unsigned last_c1 = -1, last_c2 = -1;
	static char copy_permited[256];
	static char aps[256];
	static char rcd[256];
	int changed = 0;
	unsigned c1_6 = (c1 & 0x40) >> 6;
	/* unsigned unused1=(c1&0x20)>>5; */
	unsigned cgms_a_b4 = (c1 & 0x10) >> 4;
	unsigned cgms_a_b3 = (c1 & 0x8) >> 3;
	unsigned aps_b2 = (c1 & 0x4) >> 2;
	unsigned aps_b1 = (c1 & 0x2) >> 1;
	/* unsigned asb_0=(c1&0x1); */
	unsigned c2_6 = (c2 & 0x40) >> 6;
	/* unsigned c2_5=(c2&0x20)>>5; */
	/* unsigned c2_4=(c2&0x10)>>4; */
	/* unsigned c2_3=(c2&0x8)>>3; */
	/* unsigned c2_2=(c2&0x4)>>2; */
	/* unsigned c2_1=(c2&0x2)>>1; */
	unsigned rcd0 = (c2 & 0x1);

	/* User don't need xds */
	if (!ctx)
		return;

	if (!c1_6 || !c2_6) // These must be high. If not, not following specs
		return;
	if (last_c1 != c1 || last_c2 != c2)
	{
		changed = 1;
		last_c1 = c1;
		last_c2 = c2;
		// Changed since last time, decode

		const char *copytext[4] = {"Copy permitted (no restrictions)", "No more copies (one generation copy has been made)",
					   "One generation of copies can be made", "No copying is permitted"};
		const char *apstext[4] = {"No APS", "PSP On; Split Burst Off", "PSP On; 2 line Split Burst On", "PSP On; 4 line Split Burst On"};
		sprintf(copy_permited, "CGMS: %s", copytext[cgms_a_b4 * 2 + cgms_a_b3]);
		sprintf(aps, "APS: %s", apstext[aps_b2 * 2 + aps_b1]);
		sprintf(rcd, "Redistribution Control Descriptor: %d", rcd0);
	}

	xdsprint(sub, ctx, copy_permited);
	xdsprint(sub, ctx, aps);
	xdsprint(sub, ctx, rcd);
	if (changed)
	{
		ccx_common_logging.log_ftn("\rXDS: %s\n", copy_permited);
		ccx_common_logging.log_ftn("\rXDS: %s\n", aps);
		ccx_common_logging.log_ftn("\rXDS: %s\n", rcd);
	}
	ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "\rXDS: %s\n", copy_permited);
	ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "\rXDS: %s\n", aps);
	ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "\rXDS: %s\n", rcd);
}

void xds_do_content_advisory(struct cc_subtitle *sub, struct ccx_decoders_xds_context *ctx, unsigned c1, unsigned c2)
{
	static unsigned last_c1 = -1, last_c2 = -1;
	static char age[256];
	static char content[256];
	static char rating[256];
	int changed = 0;
	// Insane encoding
	unsigned c1_6 = (c1 & 0x40) >> 6;
	unsigned Da2 = (c1 & 0x20) >> 5;
	unsigned a1 = (c1 & 0x10) >> 4;
	unsigned a0 = (c1 & 0x8) >> 3;
	unsigned r2 = (c1 & 0x4) >> 2;
	unsigned r1 = (c1 & 0x2) >> 1;
	unsigned r0 = (c1 & 0x1);
	unsigned c2_6 = (c2 & 0x40) >> 6;
	unsigned FV = (c2 & 0x20) >> 5;
	unsigned S = (c2 & 0x10) >> 4;
	unsigned La3 = (c2 & 0x8) >> 3;
	unsigned g2 = (c2 & 0x4) >> 2;
	unsigned g1 = (c2 & 0x2) >> 1;
	unsigned g0 = (c2 & 0x1);
	unsigned supported = 0;

	if (!ctx)
		return;

	if (!c1_6 || !c2_6) // These must be high. If not, not following specs
		return;
	if (last_c1 != c1 || last_c2 != c2)
	{
		changed = 1;
		last_c1 = c1;
		last_c2 = c2;
		// Changed since last time, decode
		// Bits a1 and a0 determine the encoding. I'll add parsing as more samples become available
		if (!a1 && a0) // US TV parental guidelines
		{
			const char *agetext[8] = {"None", "TV-Y (All Children)", "TV-Y7 (Older Children)",
						  "TV-G (General Audience)", "TV-PG (Parental Guidance Suggested)",
						  "TV-14 (Parents Strongly Cautioned)", "TV-MA (Mature Audience Only)", "None"};
			sprintf(age, "ContentAdvisory: US TV Parental Guidelines. Age Rating: %s", agetext[g2 * 4 + g1 * 2 + g0]);
			content[0] = 0;
			if (!g2 && g1 && !g0) // For TV-Y7 (Older children), the Violence bit is "fantasy violence"
			{
				if (FV)
					strcpy(content, "[Fantasy Violence] ");
			}
			else // For all others, is real
			{
				if (FV)
					strcpy(content, "[Violence] ");
			}
			if (S)
				strcat(content, "[Sexual Situations] ");
			if (La3)
				strcat(content, "[Adult Language] ");
			if (Da2)
				strcat(content, "[Sexually Suggestive Dialog] ");
			supported = 1;
		}
		if (!a0) // MPA
		{
			const char *ratingtext[8] = {"N/A", "G", "PG", "PG-13", "R", "NC-17", "X", "Not Rated"};
			sprintf(rating, "ContentAdvisory: MPA Rating: %s", ratingtext[r2 * 4 + r1 * 2 + r0]);
			supported = 1;
		}
		if (a0 && a1 && !Da2 && !La3) // Canadian English Language Rating
		{
			const char *ratingtext[8] = {"Exempt", "Children", "Children eight years and older",
						     "General programming suitable for all audiences", "Parental Guidance",
						     "Viewers 14 years and older", "Adult Programming", "[undefined]"};
			sprintf(rating, "ContentAdvisory: Canadian English Rating: %s", ratingtext[g2 * 4 + g1 * 2 + g0]);
			supported = 1;
		}
		if (a0 && a1 && Da2 && !La3) // Canadian French Language Rating
		{
			const char *ratingtext[8] = {"Exempt?es", "G?n?ral", "G?n?ral - D?conseill? aux jeunes enfants",
						     "Cette ?mission peut ne pas convenir aux enfants de moins de 13 ans",
						     "Cette ?mission ne convient pas aux moins de 16 ans",
						     "Cette ?mission est r?serv?e aux adultes", "[invalid]", "[invalid]"};
			sprintf(rating, "ContentAdvisory: Canadian French Rating: %s", ratingtext[g2 * 4 + g1 * 2 + g0]);
			supported = 1;
		}
	}
	// Bits a1 and a0 determine the encoding. I'll add parsing as more samples become available
	if (!a1 && a0) // US TV parental guidelines
	{
		xdsprint(sub, ctx, age);
		xdsprint(sub, ctx, content);
		if (changed)
		{
			ccx_common_logging.log_ftn("\rXDS: %s\n  ", age);
			ccx_common_logging.log_ftn("\rXDS: %s\n  ", content);
		}
		ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "\rXDS: %s\n", age);
		ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "\rXDS: %s\n", content);
	}
	if (!a0 ||			  // MPA
	    (a0 && a1 && !Da2 && !La3) || // Canadian English Language Rating
	    (a0 && a1 && Da2 && !La3)	  // Canadian French Language Rating
	)
	{
		xdsprint(sub, ctx, rating);
		if (changed)
			ccx_common_logging.log_ftn("\rXDS: %s\n  ", rating);
		ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "\rXDS: %s\n", rating);
	}

	if (changed && !supported)
		ccx_common_logging.log_ftn("XDS: Unsupported ContentAdvisory encoding, please submit sample.\n");
}

int xds_do_current_and_future(struct cc_subtitle *sub, struct ccx_decoders_xds_context *ctx)
{
	int was_proc = 0;

	char *str = malloc(1024);
	char *tstr = NULL;
	int str_len = 1024;

	if (!ctx)
	{
		free(str);
		return CCX_EINVAL;
	}

	switch (ctx->cur_xds_packet_type)
	{
		case XDS_TYPE_PIN_START_TIME:
			was_proc = 1;
			if (ctx->cur_xds_payload_length < 7) // We need 4 data bytes
				break;
			int min = ctx->cur_xds_payload[2] & 0x3f;  // 6 bits
			int hour = ctx->cur_xds_payload[3] & 0x1f; // 5 bits
			int date = ctx->cur_xds_payload[4] & 0x1f; // 5 bits
			int month = ctx->cur_xds_payload[5] & 0xf; // 4 bits
			/* int changed=0; */
			if (ctx->current_xds_min != min ||
			    ctx->current_xds_hour != hour ||
			    ctx->current_xds_date != date ||
			    ctx->current_xds_month != month)
			{
				/* changed=1; */
				ctx->xds_start_time_shown = 0;
				ctx->current_xds_min = min;
				ctx->current_xds_hour = hour;
				ctx->current_xds_date = date;
				ctx->current_xds_month = month;
			}

			ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "PIN (Start Time): %s  %02d-%02d %02d:%02d\n",
						     (ctx->cur_xds_packet_class == XDS_CLASS_CURRENT ? "Current" : "Future"),
						     date, month, hour, min);
			xdsprint(sub, ctx, "PIN (Start Time): %s  %02d-%02d %02d:%02d\n",
				 (ctx->cur_xds_packet_class == XDS_CLASS_CURRENT ? "Current" : "Future"),
				 date, month, hour, min);

			if (!ctx->xds_start_time_shown && ctx->cur_xds_packet_class == XDS_CLASS_CURRENT)
			{
				ccx_common_logging.log_ftn("\rXDS: Program changed.\n");
				ccx_common_logging.log_ftn("XDS program start time (DD/MM HH:MM) %02d-%02d %02d:%02d\n", date, month, hour, min);
				ccx_common_logging.gui_ftn(CCX_COMMON_LOGGING_GUI_XDS_PROGRAM_ID_NR,
							   ctx->current_xds_min, ctx->current_xds_hour, ctx->current_xds_date, ctx->current_xds_month);
				ctx->xds_start_time_shown = 1;
			}
			break;
		case XDS_TYPE_LENGH_AND_CURRENT_TIME:
		{
			was_proc = 1;
			if (ctx->cur_xds_payload_length < 5) // We need 2 data bytes
				break;
			int min = ctx->cur_xds_payload[2] & 0x3f;  // 6 bits
			int hour = ctx->cur_xds_payload[3] & 0x1f; // 5 bits
			if (!ctx->xds_program_length_shown)
				ccx_common_logging.log_ftn("\rXDS: Program length (HH:MM): %02d:%02d  ", hour, min);
			else
				ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "\rXDS: Program length (HH:MM): %02d:%02d  ", hour, min);

			xdsprint(sub, ctx, "Program length (HH:MM): %02d:%02d  ", hour, min);

			if (ctx->cur_xds_payload_length > 6) // Next two bytes (optional) available
			{
				int el_min = ctx->cur_xds_payload[4] & 0x3f;  // 6 bits
				int el_hour = ctx->cur_xds_payload[5] & 0x1f; // 5 bits
				if (!ctx->xds_program_length_shown)
					ccx_common_logging.log_ftn("Elapsed (HH:MM): %02d:%02d", el_hour, el_min);
				else
					ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "Elapsed (HH:MM): %02d:%02d", el_hour, el_min);
				xdsprint(sub, ctx, "Elapsed (HH:MM): %02d:%02d", el_hour, el_min);
			}
			if (ctx->cur_xds_payload_length > 8) // Next two bytes (optional) available
			{
				int el_sec = ctx->cur_xds_payload[6] & 0x3f; // 6 bits
				if (!ctx->xds_program_length_shown)
					ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, ":%02d", el_sec);
				xdsprint(sub, ctx, "Elapsed (SS) :%02d", el_sec);
			}
			if (!ctx->xds_program_length_shown)
				ccx_common_logging.log_ftn("\n");
			else
				ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "\n");
			ctx->xds_program_length_shown = 1;
		}
		break;
		case XDS_TYPE_PROGRAM_NAME:
		{
			was_proc = 1;
			char xds_program_name[33];
			int i;
			for (i = 2; i < ctx->cur_xds_payload_length - 1; i++)
				xds_program_name[i - 2] = ctx->cur_xds_payload[i];
			xds_program_name[i - 2] = 0;
			ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "\rXDS Program name: %s\n", xds_program_name);
			xdsprint(sub, ctx, "Program name: %s", xds_program_name);
			if (ctx->cur_xds_packet_class == XDS_CLASS_CURRENT &&
			    strcmp(xds_program_name, ctx->current_xds_program_name)) // Change of program
			{
				ccx_common_logging.log_ftn("\rXDS Notice: Program is now %s\n", xds_program_name);
				strncpy(ctx->current_xds_program_name, xds_program_name, 33);
				ccx_common_logging.gui_ftn(CCX_COMMON_LOGGING_GUI_XDS_PROGRAM_NAME, xds_program_name);
			}
			break;
		}
		break;
		case XDS_TYPE_PROGRAM_TYPE:
			was_proc = 1;
			if (ctx->cur_xds_payload_length < 5) // We need 2 data bytes
				break;
			if (ctx->current_program_type_reported)
			{
				// Check if we should do it again
				for (int i = 0; i < ctx->cur_xds_payload_length; i++)
				{
					if (ctx->cur_xds_payload[i] != ctx->current_xds_program_type[i])
					{
						ctx->current_program_type_reported = 0;
						break;
					}
				}
			}
			//if (!(ccx_common_logging.debug_mask & CCX_DMT_DECODER_XDS) && ctx->current_program_type_reported)
			//	break;
			memcpy(ctx->current_xds_program_type, ctx->cur_xds_payload, ctx->cur_xds_payload_length);
			ctx->current_xds_program_type[ctx->cur_xds_payload_length] = 0;
			if (!ctx->current_program_type_reported)
				ccx_common_logging.log_ftn("\rXDS Program Type: ");

			*str = '\0';
			tstr = str;
			for (int i = 2; i < ctx->cur_xds_payload_length - 1; i++)
			{
				if (ctx->cur_xds_payload[i] == 0) // Padding
					continue;
				if (!ctx->current_program_type_reported)
					ccx_common_logging.log_ftn("[%02X-", ctx->cur_xds_payload[i]);
				if (ctx->cur_xds_payload[i] >= 0x20 && ctx->cur_xds_payload[i] < 0x7F)
				{
					snprintf(tstr, str_len - (tstr - str), "[%s] ", XDSProgramTypes[ctx->cur_xds_payload[i] - 0x20]);
					tstr += strlen(tstr);
				}
				if (!ctx->current_program_type_reported)
				{
					if (ctx->cur_xds_payload[i] >= 0x20 && ctx->cur_xds_payload[i] < 0x7F)
						ccx_common_logging.log_ftn("%s", XDSProgramTypes[ctx->cur_xds_payload[i] - 0x20]);
					else
						ccx_common_logging.log_ftn("ILLEGAL VALUE");
					ccx_common_logging.log_ftn("] ");
				}
			}
			xdsprint(sub, ctx, "Program type %s", str);
			if (!ctx->current_program_type_reported)
				ccx_common_logging.log_ftn("\n");
			ctx->current_program_type_reported = 1;
			break;
		case XDS_TYPE_CONTENT_ADVISORY:
			was_proc = 1;
			if (ctx->cur_xds_payload_length < 5) // We need 2 data bytes
				break;
			xds_do_content_advisory(sub, ctx, ctx->cur_xds_payload[2], ctx->cur_xds_payload[3]);
			break;
		case XDS_TYPE_AUDIO_SERVICES:
			was_proc = 1; // I don't have any sample with this.
			break;
		case XDS_TYPE_CGMS:
			was_proc = 1;
			xds_do_copy_generation_management_system(sub, ctx, ctx->cur_xds_payload[2], ctx->cur_xds_payload[3]);
			break;
		case XDS_TYPE_ASPECT_RATIO_INFO:
		{
			unsigned ar_start, ar_end;
			// int changed = 0; /* Currently unused */
			was_proc = 1;
			if (ctx->cur_xds_payload_length < 5) // We need 2 data bytes
				break;
			if (!(ctx->cur_xds_payload[2] & 0x20) || !(ctx->cur_xds_payload[3] & 0x20)) // Bit 6 must be 1
				break;								    // if bit 6 is not 1 - skip invalid data.

			/* CEA-608-B: The starting line is computed by adding 22 to the decimal number
			   represented by bits S0 to S5. The ending line is computing by subtracting
			   the decimal number represented by bits E0 to E5 from 262 */
			ar_start = (ctx->cur_xds_payload[2] & 0x1F) + 22;
			ar_end = 262 - (ctx->cur_xds_payload[3] & 0x1F);
			unsigned active_picture_height = ar_end - ar_start;
			float aspect_ratio = (float)320 / active_picture_height;

			if (ar_start != ctx->current_ar_start)
			{
				ctx->current_ar_start = ar_start;
				ctx->current_ar_end = ar_end;
				// changed = 1; /* Currently unused */
				ccx_common_logging.log_ftn("\rXDS Notice: Aspect ratio info, start line=%u, end line=%u\n", ar_start, ar_end);
				ccx_common_logging.log_ftn("\rXDS Notice: Aspect ratio info, active picture height=%u, ratio=%f\n", active_picture_height, aspect_ratio);
			}
			else
			{
				ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "\rXDS Notice: Aspect ratio info, start line=%u, end line=%u\n", ar_start, ar_end);
				ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "\rXDS Notice: Aspect ratio info, active picture height=%u, ratio=%f\n", active_picture_height, aspect_ratio);
			}
		}
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
			was_proc = 1;
			int changed = 0;
			char xds_desc[33];
			int i;
			for (i = 2; i < ctx->cur_xds_payload_length - 1; i++)
				xds_desc[i - 2] = ctx->cur_xds_payload[i];
			xds_desc[i - 2] = 0;

			if (xds_desc[0])
			{
				int line_num = ctx->cur_xds_packet_type - XDS_TYPE_PROGRAM_DESC_1;
				if (strcmp(xds_desc, ctx->xds_program_description[line_num]))
					changed = 1;
				if (changed)
				{
					ccx_common_logging.log_ftn("\rXDS description line %d: %s\n", line_num, xds_desc);
					strcpy(ctx->xds_program_description[line_num], xds_desc);
				}
				else
				{
					ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "\rXDS description line %d: %s\n", line_num, xds_desc);
				}
				xdsprint(sub, ctx, "XDS description line %d: %s", line_num, xds_desc);
				ccx_common_logging.gui_ftn(CCX_COMMON_LOGGING_GUI_XDS_PROGRAM_DESCRIPTION, line_num, xds_desc);
			}
			break;
		}
	}

	free(str);
	return was_proc;
}

int xds_do_channel(struct cc_subtitle *sub, struct ccx_decoders_xds_context *ctx)
{
	int was_proc = 0;
	if (!ctx)
		return CCX_EINVAL;

	switch (ctx->cur_xds_packet_type)
	{
		case XDS_TYPE_NETWORK_NAME:
			was_proc = 1;
			char xds_network_name[33];
			int i;
			for (i = 2; i < ctx->cur_xds_payload_length - 1; i++)
				xds_network_name[i - 2] = ctx->cur_xds_payload[i];
			xds_network_name[i - 2] = 0;
			ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "XDS Network name: %s\n", xds_network_name);
			xdsprint(sub, ctx, "Network: %s", xds_network_name);
			if (strcmp(xds_network_name, ctx->current_xds_network_name)) // Change of station
			{
				ccx_common_logging.log_ftn("XDS Notice: Network is now %s\n", xds_network_name);
				strcpy(ctx->current_xds_network_name, xds_network_name);
			}
			break;
		case XDS_TYPE_CALL_LETTERS_AND_CHANNEL:
		{
			was_proc = 1;
			char xds_call_letters[7];
			if (ctx->cur_xds_payload_length != 7 && ctx->cur_xds_payload_length != 9) // We need 4-6 data bytes
				break;
			for (i = 2; i < ctx->cur_xds_payload_length - 1; i++)
			{
				if (ctx->cur_xds_payload)
					xds_call_letters[i - 2] = ctx->cur_xds_payload[i];
			}
			xds_call_letters[i - 2] = 0;
			ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "XDS Network call letters: %s\n", xds_call_letters);
			xdsprint(sub, ctx, "Call Letters: %s", xds_call_letters);
			if (strncmp(xds_call_letters, ctx->current_xds_call_letters, 7)) // Change of station
			{
				ccx_common_logging.log_ftn("XDS Notice: Network call letters now %s\n", xds_call_letters);
				strncpy(ctx->current_xds_call_letters, xds_call_letters, 7);
				ccx_common_logging.gui_ftn(CCX_COMMON_LOGGING_GUI_XDS_CALL_LETTERS, ctx->current_xds_call_letters);
			}
		}
		break;
		case XDS_TYPE_TSID:
			// According to CEA-608, data here (4 bytes) are used to identify the
			// "originating analog licensee". No interesting data for us.
			was_proc = 1;
			if (ctx->cur_xds_payload_length < 7) // We need 4 data bytes
				break;
			unsigned b1 = (ctx->cur_xds_payload[2]) & 0x10; // Only low 4 bits from each byte
			unsigned b2 = (ctx->cur_xds_payload[3]) & 0x10;
			unsigned b3 = (ctx->cur_xds_payload[4]) & 0x10;
			unsigned b4 = (ctx->cur_xds_payload[5]) & 0x10;
			unsigned tsid = (b4 << 12) | (b3 << 8) | (b2 << 4) | b1;
			if (tsid)
				xdsprint(sub, ctx, "TSID: %u", tsid);
			break;
	}
	return was_proc;
}

int xds_do_private_data(struct cc_subtitle *sub, struct ccx_decoders_xds_context *ctx)
{
	char *str;
	int i;

	if (!ctx)
		return CCX_EINVAL;

	str = malloc((ctx->cur_xds_payload_length * 3) + 1);
	if (str == NULL) // Only thing we can do with private data is dump it.
		return 1;

	for (i = 2; i < ctx->cur_xds_payload_length - 1; i++)
		sprintf(str, "%02X ", ctx->cur_xds_payload[i]);

	xdsprint(sub, ctx, str);
	free(str);
	return 1;
}

int xds_do_misc(struct ccx_decoders_xds_context *ctx)
{
	int was_proc = 0;
	if (!ctx)
		return CCX_EINVAL;

	switch (ctx->cur_xds_packet_type)
	{
		case XDS_TYPE_TIME_OF_DAY:
		{
			was_proc = 1;
			if (ctx->cur_xds_payload_length < 9) // We need 6 data bytes
				break;
			int min = ctx->cur_xds_payload[2] & 0x3f;  // 6 bits
			int hour = ctx->cur_xds_payload[3] & 0x1f; // 5 bits
			int date = ctx->cur_xds_payload[4] & 0x1f; // 5 bits
			int month = ctx->cur_xds_payload[5] & 0xf; // 4 bits
			int reset_seconds = (ctx->cur_xds_payload[5] & 0x20);
			int day_of_week = ctx->cur_xds_payload[6] & 0x7;
			int year = (ctx->cur_xds_payload[7] & 0x3f) + 1990;
			ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "Time of day: (YYYY/MM/DD) %04d/%02d/%02d (HH:SS) %02d:%02d DoW: %d  Reset seconds: %d\n",
						     year, month, date, hour, min, day_of_week, reset_seconds);
			break;
		}
		case XDS_TYPE_LOCAL_TIME_ZONE:
		{
			was_proc = 1;
			if (ctx->cur_xds_payload_length < 5) // We need 2 data bytes
				break;
			// int b6 = (ctx->cur_xds_payload[2] & 0x40) >>6; // Bit 6 should always be 1
			int dst = (ctx->cur_xds_payload[2] & 0x20) >> 5; // Daylight Saving Time
			int hour = ctx->cur_xds_payload[2] & 0x1f;	 // 5 bits
			ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "Local Time Zone: %02d DST: %d\n",
						     hour, dst);
			break;
		}
		default:
			was_proc = 0;
			break;
	}
	return was_proc;
}

void do_end_of_xds(struct cc_subtitle *sub, struct ccx_decoders_xds_context *ctx, unsigned char expected_checksum)
{
	int cs = 0;
	int i;

	if (!ctx)
		return;

	if (ctx->cur_xds_buffer_idx == -1 || /* Unknown buffer, or not in use (bug) */
	    !ctx->xds_buffers[ctx->cur_xds_buffer_idx].in_use)
		return;
	ctx->cur_xds_packet_class = ctx->xds_buffers[ctx->cur_xds_buffer_idx].xds_class;
	ctx->cur_xds_payload = ctx->xds_buffers[ctx->cur_xds_buffer_idx].bytes;
	ctx->cur_xds_payload_length = ctx->xds_buffers[ctx->cur_xds_buffer_idx].used_bytes;
	ctx->cur_xds_packet_type = ctx->cur_xds_payload[1];
	ctx->cur_xds_payload[ctx->cur_xds_payload_length++] = 0x0F; // The end byte itself, added to the packet

	for (i = 0; i < ctx->cur_xds_payload_length; i++)
	{
		cs = cs + ctx->cur_xds_payload[i];
		cs = cs & 0x7f; // Keep 7 bits only
		int c = ctx->cur_xds_payload[i] & 0x7F;
		ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "%02X - %c cs: %02X\n",
					     c, (c >= 0x20) ? c : '?', cs);
	}
	cs = (128 - cs) & 0x7F; // Convert to 2's complement & discard high-order bit

	ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "End of XDS. Class=%d (%s), size=%d  Checksum OK: %d   Used buffers: %d\n",
				     ctx->cur_xds_packet_class, XDSclasses[ctx->cur_xds_packet_class],
				     ctx->cur_xds_payload_length,
				     cs == expected_checksum, how_many_used(ctx));

	if (cs != expected_checksum || ctx->cur_xds_payload_length < 3)
	{
		ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "Expected checksum: %02X  Calculated: %02X\n", expected_checksum, cs);
		clear_xds_buffer(ctx, ctx->cur_xds_buffer_idx);
		return; // Bad packets ignored as per specs
	}

	int was_proc = 0;		     /* Indicated if the packet was processed. Not processed means "code to do it doesn't exist yet", not an error. */
	if (ctx->cur_xds_packet_type & 0x40) // Bit 6 set
	{
		ctx->cur_xds_packet_class = XDS_CLASS_OUT_OF_BAND;
	}

	switch (ctx->cur_xds_packet_class)
	{
		case XDS_CLASS_FUTURE:						    // Info on future program
			if (!(ccx_common_logging.debug_mask & CCX_DMT_DECODER_XDS)) // Don't bother processing something we don't need
			{
				was_proc = 1;
				break;
			}
		case XDS_CLASS_CURRENT: // Info on current program
			was_proc = xds_do_current_and_future(sub, ctx);
			break;
		case XDS_CLASS_CHANNEL:
			was_proc = xds_do_channel(sub, ctx);
			break;

		case XDS_CLASS_MISC:
			was_proc = xds_do_misc(ctx);
			break;
		case XDS_CLASS_PRIVATE: // CEA-608:
			// The Private Data Class is for use in any closed system for whatever that
			// system wishes. It shall not be defined by this standard now or in the future.
			was_proc = xds_do_private_data(sub, ctx);
			break;
		case XDS_CLASS_OUT_OF_BAND:
			ccx_common_logging.debug_ftn(CCX_DMT_DECODER_XDS, "Out-of-band data, ignored.");
			was_proc = 1;
			break;
	}

	if (!was_proc)
	{
		ccx_common_logging.log_ftn("Note: We found a currently unsupported XDS packet.\n");
		dump(CCX_DMT_DECODER_XDS, ctx->cur_xds_payload, ctx->cur_xds_payload_length, 0, 0);
	}
	clear_xds_buffer(ctx, ctx->cur_xds_buffer_idx);
}
