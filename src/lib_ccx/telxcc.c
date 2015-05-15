/*!
(c) 2011-2013 Forers, s. r. o.: telxcc

telxcc conforms to ETSI 300 706 Presentation Level 1.5: Presentation Level 1 defines the basic Teletext page,
characterised by the use of spacing attributes only and a limited alphanumeric and mosaics repertoire.
Presentation Level 1.5 decoder responds as Level 1 but the character repertoire is extended via packets X/26.
Selection of national option sub-sets related features from Presentation Level 2.5 feature set have been implemented, too.
(X/28/0 Format 1, X/28/4, M/29/0 and M/29/4 packets)

Further documentation:
ETSI TS 101 154 V1.9.1 (2009-09), Technical Specification
  Digital Video Broadcasting (DVB); Specification for the use of Video and Audio Coding in Broadcasting Applications based on the MPEG-2 Transport Stream
ETSI EN 300 231 V1.3.1 (2003-04), European Standard (Telecommunications series)
  Television systems; Specification of the domestic video Programme Delivery Control system (PDC)
ETSI EN 300 472 V1.3.1 (2003-05), European Standard (Telecommunications series)
  Digital Video Broadcasting (DVB); Specification for conveying ITU-R System B Teletext in DVB bitstreams
ETSI EN 301 775 V1.2.1 (2003-05), European Standard (Telecommunications series)
  Digital Video Broadcasting (DVB); Specification for the carriage of Vertical Blanking Information (VBI) data in DVB bitstreams
ETS 300 706 (May 1997)
  Enhanced Teletext Specification
ETS 300 708 (March 1997)
  Television systems; Data transmission within Teletext
ISO/IEC STANDARD 13818-1 Second edition (2000-12-01)
  Information technology — Generic coding of moving pictures and associated audio information: Systems
ISO/IEC STANDARD 6937 Third edition (2001-12-15)
  Information technology — Coded graphic character set for text communication — Latin alphabet
Werner Brückner -- Teletext in digital television
*/


#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "hamming.h"
#include "teletext.h"
#include <signal.h>
#ifdef I18N
#include <libintl.h>
#include <locale.h>
#define _(STRING) gettext(STRING)
#endif

#define TELXCC_VERSION "2.4.4"

#ifdef __MINGW32__
// switch stdin and all normal files into binary mode -- needed for Windows
#include <fcntl.h>
int _CRT_fmode = _O_BINARY;

// for better UX in Windows we want to detect that app is not run by "double-clicking" in Explorer
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0502
#define _WIN32_IE 0x0400
#include <windows.h>
#include <commctrl.h>
#endif

// size of a TS packet in bytes
#define TS_PACKET_SIZE 188

// size of a TS packet payload in bytes
#define TS_PACKET_PAYLOAD_SIZE (TS_PACKET_SIZE - 4)

// size of a packet payload buffer
#define PAYLOAD_BUFFER_SIZE 4096

typedef struct {
	uint8_t sync;
	uint8_t transport_error;
	uint8_t payload_unit_start;
	uint8_t transport_priority;
	uint16_t pid;
	uint8_t scrambling_control;
	uint8_t adaptation_field_exists;
	uint8_t continuity_counter;
} ts_packet_t;

typedef struct {
	uint16_t program_num;
	uint16_t program_pid;
} pat_section_t;

typedef struct {
	uint8_t pointer_field;
	uint8_t table_id;
	uint16_t section_length;
	uint8_t current_next_indicator;
} pat_t;

typedef struct {
	uint8_t ccx_stream_type;
	uint16_t elementary_pid;
	uint16_t es_info_length;
} pmt_program_descriptor_t;

typedef struct {
	uint8_t pointer_field;
	uint8_t table_id;
	uint16_t section_length;
	uint16_t program_num;
	uint8_t current_next_indicator;
	uint16_t pcr_pid;
	uint16_t program_info_length;
} pmt_t;

typedef enum {
	DATA_UNIT_EBU_TELETEXT_NONSUBTITLE = 0x02,
	DATA_UNIT_EBU_TELETEXT_SUBTITLE = 0x03,
	DATA_UNIT_EBU_TELETEXT_INVERTED = 0x0c,
	DATA_UNIT_VPS = 0xc3,
	DATA_UNIT_CLOSED_CAPTIONS = 0xc5
} data_unit_t;

typedef enum {
	TRANSMISSION_MODE_PARALLEL = 0,
	TRANSMISSION_MODE_SERIAL = 1
} transmission_mode_t;

static const char* TTXT_COLOURS[8] = {
	//black,   red,       green,     yellow,    blue,      magenta,   cyan,      white
	"#000000", "#ff0000", "#00ff00", "#ffff00", "#0000ff", "#ff00ff", "#00ffff", "#ffffff"
};

#define MAX_TLT_PAGES 1000

short int seen_sub_page[MAX_TLT_PAGES];

// 1-byte alignment; just to be sure, this struct is being used for explicit type conversion
// FIXME: remove explicit type conversion from buffer to structs
#pragma pack(push)
#pragma pack(1)
typedef struct {
	uint8_t _clock_in; // clock run in
	uint8_t _framing_code; // framing code, not needed, ETSI 300 706: const 0xe4
	uint8_t address[2];
	uint8_t data[40];
} teletext_packet_payload_t;
#pragma pack(pop)

typedef struct {
	uint64_t show_timestamp; // show at timestamp (in ms)
	uint64_t hide_timestamp; // hide at timestamp (in ms)
	uint16_t text[25][40]; // 25 lines x 40 cols (1 screen/page) of wide chars
	uint8_t tainted; // 1 = text variable contains any data
} teletext_page_t;

// application config global variable
struct ccx_s_teletext_config tlt_config = { NO, 0, 0, 0, NO, NO, 0, NULL};

// macro -- output only when increased verbosity was turned on
#define VERBOSE_ONLY if (tlt_config.verbose == YES)

// application states -- flags for notices that should be printed only once
static struct s_states {
	uint8_t programme_info_processed;
	uint8_t pts_initialized;
} states = { NO, NO };

// SRT frames produced
uint32_t tlt_frames_produced = 0;

// subtitle type pages bitmap, 2048 bits = 2048 possible pages in teletext (excl. subpages)
static uint8_t cc_map[256] = { 0 };

// last timestamp computed
static uint64_t last_timestamp = 0;

// working teletext page buffer
teletext_page_t page_buffer = { 0 };

// teletext transmission mode
static transmission_mode_t transmission_mode = TRANSMISSION_MODE_SERIAL;

// flag indicating if incoming data should be processed or ignored
static uint8_t receiving_data = NO;

// current charset (charset can be -- and always is -- changed during transmission)
struct s_primary_charset {
	uint8_t current;
	uint8_t g0_m29;
	uint8_t g0_x28;
} primary_charset = {
	0x00, UNDEF, UNDEF
};

// entities, used in colour mode, to replace unsafe HTML tag chars
struct {
	uint16_t character;
	const char *entity;
} const ENTITIES[] = {
	{ '<', "&lt;" },
	{ '>', "&gt;" },
	{ '&', "&amp;" }
};

// PMTs table
#define TS_PMT_MAP_SIZE 128
static uint16_t pmt_map[TS_PMT_MAP_SIZE] = { 0 };
static uint16_t pmt_map_count = 0;

// TTXT streams table
#define TS_PMT_TTXT_MAP_SIZE 128
static uint16_t pmt_ttxt_map[TS_PMT_MAP_SIZE] = { 0 };
static uint16_t pmt_ttxt_map_count = 0;

// FYI, packet counter
uint32_t tlt_packet_counter = 0;

static int telxcc_inited=0;

#define array_length(a) (sizeof(a)/sizeof(a[0]))

// extracts magazine number from teletext page
#define MAGAZINE(p) ((p >> 8) & 0xf)

// extracts page number from teletext page
#define PAGE(p) (p & 0xff)

// Current and previous page buffers. This is the output written to file when
// the time comes.
static char *page_buffer_prev=NULL;
static char *page_buffer_cur=NULL;
static unsigned page_buffer_cur_size=0;
static unsigned page_buffer_cur_used=0;
static unsigned page_buffer_prev_size=0;
static unsigned page_buffer_prev_used=0;
// Current and previous page compare strings. This is plain text (no colors,
// tags, etc) in UCS2 (fixed length), so we can compare easily.
static uint64_t *ucs2_buffer_prev=NULL;
static uint64_t *ucs2_buffer_cur=NULL;
static unsigned ucs2_buffer_cur_size=0;
static unsigned ucs2_buffer_cur_used=0;
static unsigned ucs2_buffer_prev_size=0;
static unsigned ucs2_buffer_prev_used=0;
// Buffer timestamp
static uint64_t prev_hide_timestamp;
static uint64_t prev_show_timestamp;

void page_buffer_add_string (const char *s)
{
	if (page_buffer_cur_size<(page_buffer_cur_used+strlen (s)+1))
	{
		int add=strlen (s)+4096; // So we don't need to realloc often
		page_buffer_cur_size=page_buffer_cur_size+add;
		page_buffer_cur=(char *) realloc (page_buffer_cur,page_buffer_cur_size);
		if (!page_buffer_cur)
			fatal (EXIT_NOT_ENOUGH_MEMORY, "Not enough memory to process teletext page.\n");
	}
	memcpy (page_buffer_cur+page_buffer_cur_used, s, strlen (s));
	page_buffer_cur_used+=strlen (s);
	page_buffer_cur[page_buffer_cur_used]=0;
}

void ucs2_buffer_add_char (uint64_t c)
{
	if (ucs2_buffer_cur_size<(ucs2_buffer_cur_used+2))
	{
		int add=4096; // So we don't need to realloc often
		ucs2_buffer_cur_size=ucs2_buffer_cur_size+add;
		ucs2_buffer_cur=(uint64_t *) realloc (ucs2_buffer_cur,ucs2_buffer_cur_size*sizeof (uint64_t));
		if (!ucs2_buffer_cur)
			fatal (EXIT_NOT_ENOUGH_MEMORY, "Not enough memory to process teletext page.\n");
	}
	ucs2_buffer_cur[ucs2_buffer_cur_used++]=c;
	ucs2_buffer_cur[ucs2_buffer_cur_used]=0;
}

void page_buffer_add_char (char c)
{
	char t[2];
	t[0]=c;
	t[1]=0;
	page_buffer_add_string (t);
}

// ETS 300 706, chapter 8.2
uint8_t unham_8_4(uint8_t a) {
	uint8_t r = UNHAM_8_4[a];
	if (r == 0xff) {
		dbg_print (CCX_DMT_TELETEXT, "- Unrecoverable data error; UNHAM8/4(%02x)\n", a);
	}
	return (r & 0x0f);
}

// ETS 300 706, chapter 8.3
uint32_t unham_24_18(uint32_t a) {
	uint8_t test = 0;

	//Tests A-F correspond to bits 0-6 respectively in 'test'.
	for (uint8_t i = 0; i < 23; i++) test ^= ((a >> i) & 0x01) * (i + 33);
	//Only parity bit is tested for bit 24
	test ^= ((a >> 23) & 0x01) * 32;

	if ((test & 0x1f) != 0x1f) {
		//Not all tests A-E correct
		if ((test & 0x20) == 0x20) {
			//F correct: Double error
			return 0xffffffff;
		}
		//Test F incorrect: Single error
		a ^= 1 << (30 - test);
	}

	return (a & 0x000004) >> 2 | (a & 0x000070) >> 3 | (a & 0x007f00) >> 4 | (a & 0x7f0000) >> 5;
}

void remap_g0_charset(uint8_t c) {
	if (c != primary_charset.current) {
		uint8_t m = G0_LATIN_NATIONAL_SUBSETS_MAP[c];
		if (m == 0xff) {
			fprintf(stderr, "- G0 Latin National Subset ID 0x%1x.%1x is not implemented\n", (c >> 3), (c & 0x7));
		}
		else {
			for (uint8_t j = 0; j < 13; j++) G0[LATIN][G0_LATIN_NATIONAL_SUBSETS_POSITIONS[j]] = G0_LATIN_NATIONAL_SUBSETS[m].characters[j];
			VERBOSE_ONLY fprintf(stderr, "- Using G0 Latin National Subset ID 0x%1x.%1x (%s)\n", (c >> 3), (c & 0x7), G0_LATIN_NATIONAL_SUBSETS[m].language);
			primary_charset.current = c;
		}
	}
}



// wide char (16 bits) to utf-8 conversion
void ucs2_to_utf8(char *r, uint16_t ch) {
	if (ch < 0x80) {
		r[0] = ch & 0x7f;
		r[1] = 0;
		r[2] = 0;
	}
	else if (ch < 0x800) {
		r[0] = (ch >> 6) | 0xc0;
		r[1] = (ch & 0x3f) | 0x80;
		r[2] = 0;
	}
	else {
		r[0] = (ch >> 12) | 0xe0;
		r[1] = ((ch >> 6) & 0x3f) | 0x80;
		r[2] = (ch & 0x3f) | 0x80;
	}
	r[3] = 0;
}

// check parity and translate any reasonable teletext character into ucs2
uint16_t telx_to_ucs2(uint8_t c)
{
	if (PARITY_8[c] == 0) {
		dbg_print (CCX_DMT_TELETEXT,  "- Unrecoverable data error; PARITY(%02x)\n", c);
		return 0x20;
	}

	uint16_t r = c & 0x7f;
	if (r >= 0x20) r = G0[LATIN][r - 0x20];
	return r;
}

uint16_t bcd_page_to_int (uint16_t bcd)
{
	return ((bcd&0xf00)>>8)*100 + ((bcd&0xf0)>>4)*10 + (bcd&0xf);
}

void telxcc_dump_prev_page (struct lib_ccx_ctx *ctx)
{
	char c_temp1[80],c_temp2[80]; // For timing
	if (!page_buffer_prev)
		return;

	if (tlt_config.transcript_settings->showStartTime){
		millis_to_date(prev_show_timestamp, c_temp1, tlt_config.date_format, tlt_config.millis_separator); // Note: Delay not added here because it was already accounted for
		fdprintf(ctx->wbout1.fh, "%s|", c_temp1);
	}
	if (tlt_config.transcript_settings->showEndTime)
	{
		millis_to_date (prev_hide_timestamp, c_temp2, tlt_config.date_format, tlt_config.millis_separator);
		fdprintf(ctx->wbout1.fh,"%s|",c_temp2);
	}
	if (tlt_config.transcript_settings->showCC){
		fdprintf(ctx->wbout1.fh, "%.3u|", bcd_page_to_int(tlt_config.page));
	}
	if (tlt_config.transcript_settings->showMode){
		fdprintf(ctx->wbout1.fh, "TLT|");
	}

	if (ctx->wbout1.fh!=-1) fdprintf(ctx->wbout1.fh, "%s",page_buffer_prev);
	fdprintf(ctx->wbout1.fh,"%s",encoded_crlf);
	if (page_buffer_prev) free (page_buffer_prev);
	if (ucs2_buffer_prev) free (ucs2_buffer_prev);
	// Switch "dump" buffers
	page_buffer_prev_used=page_buffer_cur_used;
	page_buffer_prev_size=page_buffer_cur_size;
	page_buffer_prev=page_buffer_cur;
	page_buffer_cur_size=0;
	page_buffer_cur_used=0;
	page_buffer_cur=NULL;
	// Also switch compare buffers
	ucs2_buffer_prev_used=ucs2_buffer_cur_used;
	ucs2_buffer_prev_size=ucs2_buffer_cur_size;
	ucs2_buffer_prev=ucs2_buffer_cur;
	ucs2_buffer_cur_size=0;
	ucs2_buffer_cur_used=0;
	ucs2_buffer_cur=NULL;
}

// Note: c1 and c2 are just used for debug output, not for the actual comparison
int fuzzy_memcmp (const char *c1, const char *c2, const uint64_t *ucs2_buf1, unsigned ucs2_buf1_len,
				  const uint64_t *ucs2_buf2, unsigned ucs2_buf2_len)
{
	size_t l;
	size_t short_len=ucs2_buf1_len<ucs2_buf2_len?ucs2_buf1_len:ucs2_buf2_len;
	size_t max=(short_len * tlt_config.levdistmaxpct)/100;
	unsigned upto=(ucs2_buf1_len<ucs2_buf2_len)?ucs2_buf1_len:ucs2_buf2_len;
	if (max < tlt_config.levdistmincnt)
		max=tlt_config.levdistmincnt;

	// For the second string, only take the first chars (up to the first string length, that's upto).
	l = (size_t) levenshtein_dist (ucs2_buf1,ucs2_buf2,ucs2_buf1_len,upto);
	int res=(l>max);
	dbg_print(CCX_DMT_LEVENSHTEIN, "\rLEV | %s | %s | Max: %d | Calc: %d | Match: %d\n", c1,c2,max,l,!res);
	return res;
}

void process_page(struct lib_ccx_ctx *ctx, teletext_page_t *page) {
    if ((tlt_config.extraction_start.set && page->hide_timestamp < tlt_config.extraction_start.time_in_ms) ||
        (tlt_config.extraction_end.set && page->show_timestamp > tlt_config.extraction_end.time_in_ms) ||
        page->hide_timestamp == 0) {
        return;
    }
#ifdef DEBUG
	for (uint8_t row = 1; row < 25; row++) {
		fprintf(stdout, "DEBUG[%02u]: ", row);
		for (uint8_t col = 0; col < 40; col++) fprintf(stdout, "%3x ", page->text[row][col]);
		fprintf(stdout, "\n");
	}
	fprintf(stdout, "\n");
#endif
	char u[4] = {0, 0, 0, 0};

	// optimization: slicing column by column -- higher probability we could find boxed area start mark sooner
	uint8_t page_is_empty = YES;
	for (uint8_t col = 0; col < 40; col++) {
		for (uint8_t row = 1; row < 25; row++) {
			if (page->text[row][col] == 0x0b) {
				page_is_empty = NO;
				goto page_is_empty;
			}
		}
	}
	page_is_empty:
	if (page_is_empty == YES) return;

	if (page->show_timestamp > page->hide_timestamp)
		page->hide_timestamp = page->show_timestamp;

	char timecode_show[24] = { 0 }, timecode_hide[24] = { 0 };

	int time_reported=0;
	++tlt_frames_produced;
	char c_tempb[256]; // For buffering
    uint8_t line_count = 0;

	timestamp_to_srttime(page->show_timestamp, timecode_show);
	timecode_show[12] = 0;
	timestamp_to_srttime(page->hide_timestamp, timecode_hide);
	timecode_hide[12] = 0;

	// process data
	for (uint8_t row = 1; row < 25; row++) {
		// anchors for string trimming purpose
		uint8_t col_start = 40;
		uint8_t col_stop = 40;

		for (int8_t col = 39; col >= 0; col--) {
			if (page->text[row][col] == 0xb) {
				col_start = col;
                line_count++;
				break;
			}
		}
		// line is empty
		if (col_start > 39) continue;

		for (uint8_t col = col_start + 1; col <= 39; col++) {
			if (page->text[row][col] > 0x20) {
				if (col_stop > 39) col_start = col;
				col_stop = col;
			}
			if (page->text[row][col] == 0xa) break;
		}
		// line is empty
		if (col_stop > 39) continue;

		// ETS 300 706, chapter 12.2: Alpha White ("Set-After") - Start-of-row default condition.
		// used for colour changes _before_ start box mark
		// white is default as stated in ETS 300 706, chapter 12.2
		// black(0), red(1), green(2), yellow(3), blue(4), magenta(5), cyan(6), white(7)
		uint8_t foreground_color = 0x7;
		uint8_t font_tag_opened = NO;

        if (line_count > 1) {
            switch (tlt_config.write_format) {
                case CCX_OF_TRANSCRIPT:
                    page_buffer_add_string(" ");
                    break;
                case CCX_OF_SMPTETT:
                    page_buffer_add_string("<br/>");
                    break;
                default:
                    page_buffer_add_string((const char *) encoded_crlf);
            }
        }

		if (tlt_config.gui_mode_reports)
		{
			fprintf (stderr, "###SUBTITLE#");
			if (!time_reported)
			{
				char timecode_show_mmss[6], timecode_hide_mmss[6];
				memcpy (timecode_show_mmss, timecode_show+3, 5);
				memcpy (timecode_hide_mmss, timecode_hide+3, 5);
				timecode_show_mmss[5]=0;
				timecode_hide_mmss[5]=0;
				// Note, only MM:SS here as we need to save space in the preview window
				fprintf (stderr, "%s#%s#",
					timecode_show_mmss, timecode_hide_mmss);
				time_reported=1;
			}
			else
				fprintf (stderr, "##");
		}

		for (uint8_t col = 0; col <= col_stop; col++) {
			// v is just a shortcut
			uint16_t v = page->text[row][col];

			if (col < col_start) {
				if (v <= 0x7) foreground_color = (uint8_t) v;
			}

			if (col == col_start) {
				if ((foreground_color != 0x7) && !tlt_config.nofontcolor) {
					sprintf (c_tempb, "<font color=\"%s\">", TTXT_COLOURS[foreground_color]);
					page_buffer_add_string (c_tempb);
					// if (ctx->wbout1.fh!=-1) fdprintf(ctx->wbout1.fh, "<font color=\"%s\">", TTXT_COLOURS[foreground_color]);
					font_tag_opened = YES;
				}
			}

			if (col >= col_start) {
				if (v <= 0x7) {
					// ETS 300 706, chapter 12.2: Unless operating in "Hold Mosaics" mode,
					// each character space occupied by a spacing attribute is displayed as a SPACE.
					if (!tlt_config.nofontcolor) {
						if (font_tag_opened == YES) {
							page_buffer_add_string ("</font>");
							// if (ctx->wbout1.fh!=-1) fdprintf(ctx->wbout1.fh, "</font> ");
							font_tag_opened = NO;
						}

						// black is considered as white for telxcc purpose
						// telxcc writes <font/> tags only when needed
						if ((v > 0x0) && (v < 0x7)) {
							sprintf (c_tempb, "<font color=\"%s\">", TTXT_COLOURS[v]);
							page_buffer_add_string (c_tempb);
							// if (ctx->wbout1.fh!=-1) fdprintf(ctx->wbout1.fh, "<font color=\"%s\">", TTXT_COLOURS[v]);
							font_tag_opened = YES;
						}
					}
					else v = 0x20;
				}

				if (v >= 0x20) {
					ucs2_to_utf8(u, v);
					uint64_t ucs2_char=(u[0]<<24) | (u[1]<<16) | (u[2]<<8) | u[3];
					ucs2_buffer_add_char(ucs2_char);
				}

				if (v >= 0x20) {
						// translate some chars into entities, if in colour mode
						if (!tlt_config.nofontcolor) {
							for (uint8_t i = 0; i < array_length(ENTITIES); i++)
									if (v == ENTITIES[i].character) {
												//if (ctx->wbout1.fh!=-1) fdprintf(ctx->wbout1.fh, "%s", ENTITIES[i].entity);
												page_buffer_add_string (ENTITIES[i].entity);
												// v < 0x20 won't be printed in next block
												v = 0;
												break;
									}
						}
				}


				if (v >= 0x20) {
					//if (ctx->wbout1.fh!=-1) fdprintf(ctx->wbout1.fh, "%s", u);
					page_buffer_add_string (u);
					if (tlt_config.gui_mode_reports) // For now we just handle the easy stuff
						fprintf (stderr,"%s",u);
				}
			}
		}

		// no tag will left opened!
		if ((!tlt_config.nofontcolor) && (font_tag_opened == YES)) {
			//if (ctx->wbout1.fh!=-1) fdprintf(ctx->wbout1.fh, "</font>");
			page_buffer_add_string ("</font>");
			font_tag_opened = NO;
		}

		if (tlt_config.gui_mode_reports)
		{
			fprintf (stderr,"\n");
		}
	}
	time_reported=0;

	switch (tlt_config.write_format)
	{
		case CCX_OF_TRANSCRIPT:
			if (page_buffer_prev_used==0)
				prev_show_timestamp=page->show_timestamp;
			if (page_buffer_prev_used==0 ||
				fuzzy_memcmp (page_buffer_prev, page_buffer_cur,
					ucs2_buffer_prev, ucs2_buffer_prev_used,
					ucs2_buffer_cur, ucs2_buffer_cur_used
				)==0)
			{
				// If empty previous buffer, we just start one with the
				// current page and do nothing. Wait until we see more.
				if (page_buffer_prev) free (page_buffer_prev);
				page_buffer_prev_used=page_buffer_cur_used;
				page_buffer_prev_size=page_buffer_cur_size;
				page_buffer_prev=page_buffer_cur;
				page_buffer_cur_size=0;
				page_buffer_cur_used=0;
				page_buffer_cur=NULL;
				if (ucs2_buffer_prev) free (ucs2_buffer_prev);
				ucs2_buffer_prev_used=ucs2_buffer_cur_used;
				ucs2_buffer_prev_size=ucs2_buffer_cur_size;
				ucs2_buffer_prev=ucs2_buffer_cur;
				ucs2_buffer_cur_size=0;
				ucs2_buffer_cur_used=0;
				ucs2_buffer_cur=NULL;
				prev_hide_timestamp=page->hide_timestamp;
				break;
			}
			else
			{
				// OK, the old and new buffer don't match. So write the old
				telxcc_dump_prev_page(ctx);
				prev_hide_timestamp=page->hide_timestamp;
				prev_show_timestamp=page->show_timestamp;
			}
			break;
        case CCX_OF_SMPTETT:
            if (ctx->wbout1.fh!=-1) {
                timestamp_to_smptetttime(page->show_timestamp, timecode_show);
                timestamp_to_smptetttime(page->hide_timestamp, timecode_hide);
                fdprintf(ctx->wbout1.fh,"      <p region=\"speaker\" begin=\"%s\" end=\"%s\">%s</p>\n", timecode_show, timecode_hide, page_buffer_cur);
            }
            break;
		default: // Yes, this means everything else is .srt for now
            page_buffer_add_string ((const char *) encoded_crlf);
			page_buffer_add_string((const char *) encoded_crlf);
            if (ctx->wbout1.fh!=-1) {
                fdprintf(ctx->wbout1.fh,"%"PRIu32"%s%s --> %s%s", tlt_frames_produced, encoded_crlf, timecode_show, timecode_hide, encoded_crlf);
                fdprintf(ctx->wbout1.fh, "%s",page_buffer_cur);
            }
	}

	// Also update GUI...

	page_buffer_cur_used=0;
	if (page_buffer_cur)
		page_buffer_cur[0]=0;
	if (tlt_config.gui_mode_reports)
		fflush (stderr);
}

void process_telx_packet(struct lib_ccx_ctx *ctx, data_unit_t data_unit_id, teletext_packet_payload_t *packet, uint64_t timestamp) {
	// variable names conform to ETS 300 706, chapter 7.1.2
	uint8_t address, m, y, designation_code;
	address = (unham_8_4(packet->address[1]) << 4) | unham_8_4(packet->address[0]);
	m = address & 0x7;
	if (m == 0) m = 8;
	y = (address >> 3) & 0x1f;
	designation_code = (y > 25) ? unham_8_4(packet->data[0]) : 0x00;

	if (y == 0) {
		// CC map
		uint8_t i = (unham_8_4(packet->data[1]) << 4) | unham_8_4(packet->data[0]);
		uint8_t flag_subtitle = (unham_8_4(packet->data[5]) & 0x08) >> 3;
		uint16_t page_number;
		uint8_t charset;
		uint8_t c;
		cc_map[i] |= flag_subtitle << (m - 1);

		if ((flag_subtitle == YES) && (i < 0xff))
		{
			int thisp= (m << 8) | (unham_8_4(packet->data[1]) << 4) | unham_8_4(packet->data[0]);
			char t1[10];
			sprintf (t1,"%x",thisp); // Example: 1928 -> 788
			thisp=atoi (t1);
			if (!seen_sub_page[thisp])
			{
				seen_sub_page[thisp]=1;
				mprint ("\rNotice: Teletext page with possible subtitles detected: %03d\n",thisp);
			}
		}
		if ((tlt_config.page == 0) && (flag_subtitle == YES) && (i < 0xff)) {
			tlt_config.page = (m << 8) | (unham_8_4(packet->data[1]) << 4) | unham_8_4(packet->data[0]);
			mprint ("- No teletext page specified, first received suitable page is %03x, not guaranteed\n", tlt_config.page);
		}

		// Page number and control bits
		page_number = (m << 8) | (unham_8_4(packet->data[1]) << 4) | unham_8_4(packet->data[0]);
		charset = ((unham_8_4(packet->data[7]) & 0x08) | (unham_8_4(packet->data[7]) & 0x04) | (unham_8_4(packet->data[7]) & 0x02)) >> 1;
		//uint8_t flag_suppress_header = unham_8_4(packet->data[6]) & 0x01;
		//uint8_t flag_inhibit_display = (unham_8_4(packet->data[6]) & 0x08) >> 3;

		// ETS 300 706, chapter 9.3.1.3:
		// When set to '1' the service is designated to be in Serial mode and the transmission of a page is terminated
		// by the next page header with a different page number.
		// When set to '0' the service is designated to be in Parallel mode and the transmission of a page is terminated
		// by the next page header with a different page number but the same magazine number.
		// The same setting shall be used for all page headers in the service.
		// ETS 300 706, chapter 7.2.1: Page is terminated by and excludes the next page header packet
		// having the same magazine address in parallel transmission mode, or any magazine address in serial transmission mode.
		transmission_mode = (transmission_mode_t) (unham_8_4(packet->data[7]) & 0x01);

		// FIXME: Well, this is not ETS 300 706 kosher, however we are interested in DATA_UNIT_EBU_TELETEXT_SUBTITLE only
		if ((transmission_mode == TRANSMISSION_MODE_PARALLEL) && (data_unit_id != DATA_UNIT_EBU_TELETEXT_SUBTITLE)) return;

		if ((receiving_data == YES) && (
				((transmission_mode == TRANSMISSION_MODE_SERIAL) && (PAGE(page_number) != PAGE(tlt_config.page))) ||
				((transmission_mode == TRANSMISSION_MODE_PARALLEL) && (PAGE(page_number) != PAGE(tlt_config.page)) && (m == MAGAZINE(tlt_config.page)))
			)) {
			receiving_data = NO;
			return;
		}

		// Page transmission is terminated, however now we are waiting for our new page
		if (page_number != tlt_config.page) return;

		// Now we have the begining of page transmission; if there is page_buffer pending, process it
		if (page_buffer.tainted == YES) {
			// it would be nice, if subtitle hides on previous video frame, so we contract 40 ms (1 frame @25 fps)
			page_buffer.hide_timestamp = timestamp - 40;
            if (page_buffer.hide_timestamp > timestamp) {
                page_buffer.hide_timestamp = 0;
            }
			process_page(ctx, &page_buffer);

		}

		page_buffer.show_timestamp = timestamp;
		page_buffer.hide_timestamp = 0;
		memset(page_buffer.text, 0x00, sizeof(page_buffer.text));
		page_buffer.tainted = NO;
		receiving_data = YES;
		primary_charset.g0_x28 = UNDEF;

		c = (primary_charset.g0_m29 != UNDEF) ? primary_charset.g0_m29 : charset;
		remap_g0_charset(c);

		/*
		// I know -- not needed; in subtitles we will never need disturbing teletext page status bar
		// displaying tv station name, current time etc.
		if (flag_suppress_header == NO) {
			for (uint8_t i = 14; i < 40; i++) page_buffer.text[y][i] = telx_to_ucs2(packet->data[i]);
			//page_buffer.tainted = YES;
		}
		*/
	}
	else if ((m == MAGAZINE(tlt_config.page)) && (y >= 1) && (y <= 23) && (receiving_data == YES)) {
		// ETS 300 706, chapter 9.4.1: Packets X/26 at presentation Levels 1.5, 2.5, 3.5 are used for addressing
		// a character location and overwriting the existing character defined on the Level 1 page
		// ETS 300 706, annex B.2.2: Packets with Y = 26 shall be transmitted before any packets with Y = 1 to Y = 25;
		// so page_buffer.text[y][i] may already contain any character received
		// in frame number 26, skip original G0 character
		for (uint8_t i = 0; i < 40; i++) if (page_buffer.text[y][i] == 0x00) page_buffer.text[y][i] = telx_to_ucs2(packet->data[i]);
		page_buffer.tainted = YES;
	}
	else if ((m == MAGAZINE(tlt_config.page)) && (y == 26) && (receiving_data == YES)) {
		// ETS 300 706, chapter 12.3.2: X/26 definition
		uint8_t x26_row = 0;
		uint8_t x26_col = 0;

		uint32_t triplets[13] = { 0 };
		for (uint8_t i = 1, j = 0; i < 40; i += 3, j++) triplets[j] = unham_24_18((packet->data[i + 2] << 16) | (packet->data[i + 1] << 8) | packet->data[i]);

		for (uint8_t j = 0; j < 13; j++) {
			uint8_t data;
			uint8_t mode;
			uint8_t address;
			uint8_t row_address_group;
			// invalid data (HAM24/18 uncorrectable error detected), skip group
			if (triplets[j] == 0xffffffff) {
				dbg_print (CCX_DMT_TELETEXT, "- Unrecoverable data error; UNHAM24/18()=%04x\n", triplets[j]);
				continue;
			}

			data = (triplets[j] & 0x3f800) >> 11;
			mode = (triplets[j] & 0x7c0) >> 6;
			address = triplets[j] & 0x3f;
			row_address_group = (address >= 40) && (address <= 63);

			// ETS 300 706, chapter 12.3.1, table 27: set active position
			if ((mode == 0x04) && (row_address_group == YES)) {
				x26_row = address - 40;
				if (x26_row == 0) x26_row = 24;
				x26_col = 0;
			}

			// ETS 300 706, chapter 12.3.1, table 27: termination marker
			if ((mode >= 0x11) && (mode <= 0x1f) && (row_address_group == YES)) break;

			// ETS 300 706, chapter 12.3.1, table 27: character from G2 set
			if ((mode == 0x0f) && (row_address_group == NO)) {
				x26_col = address;
				if (data > 31) page_buffer.text[x26_row][x26_col] = G2[0][data - 0x20];
			}

			// ETS 300 706, chapter 12.3.1, table 27: G0 character with diacritical mark
			if ((mode >= 0x11) && (mode <= 0x1f) && (row_address_group == NO)) {
				x26_col = address;

				// A - Z
				if ((data >= 65) && (data <= 90)) page_buffer.text[x26_row][x26_col] = G2_ACCENTS[mode - 0x11][data - 65];
				// a - z
				else if ((data >= 97) && (data <= 122)) page_buffer.text[x26_row][x26_col] = G2_ACCENTS[mode - 0x11][data - 71];
				// other
				else page_buffer.text[x26_row][x26_col] = telx_to_ucs2(data);
			}
		}
	}
	else if ((m == MAGAZINE(tlt_config.page)) && (y == 28) && (receiving_data == YES)) {
		// TODO:
		//   ETS 300 706, chapter 9.4.7: Packet X/28/4
		//   Where packets 28/0 and 28/4 are both transmitted as part of a page, packet 28/0 takes precedence over 28/4 for all but the colour map entry coding.
		if ((designation_code == 0) || (designation_code == 4)) {
			// ETS 300 706, chapter 9.4.2: Packet X/28/0 Format 1
			// ETS 300 706, chapter 9.4.7: Packet X/28/4
			uint32_t triplet0 = unham_24_18((packet->data[3] << 16) | (packet->data[2] << 8) | packet->data[1]);

			if (triplet0 == 0xffffffff) {
				// invalid data (HAM24/18 uncorrectable error detected), skip group
				dbg_print (CCX_DMT_TELETEXT, "! Unrecoverable data error; UNHAM24/18()=%04x\n", triplet0);
			}
			else {
				// ETS 300 706, chapter 9.4.2: Packet X/28/0 Format 1 only
				if ((triplet0 & 0x0f) == 0x00) {
					primary_charset.g0_x28 = (triplet0 & 0x3f80) >> 7;
					remap_g0_charset(primary_charset.g0_x28);
				}
			}
		}
	}
	else if ((m == MAGAZINE(tlt_config.page)) && (y == 29)) {
		// TODO:
		//   ETS 300 706, chapter 9.5.1 Packet M/29/0
		//   Where M/29/0 and M/29/4 are transmitted for the same magazine, M/29/0 takes precedence over M/29/4.
		if ((designation_code == 0) || (designation_code == 4)) {
			// ETS 300 706, chapter 9.5.1: Packet M/29/0
			// ETS 300 706, chapter 9.5.3: Packet M/29/4
			uint32_t triplet0 = unham_24_18((packet->data[3] << 16) | (packet->data[2] << 8) | packet->data[1]);

			if (triplet0 == 0xffffffff) {
				// invalid data (HAM24/18 uncorrectable error detected), skip group
				dbg_print (CCX_DMT_TELETEXT, "! Unrecoverable data error; UNHAM24/18()=%04x\n", triplet0);
			}
			else {
				// ETS 300 706, table 11: Coding of Packet M/29/0
				// ETS 300 706, table 13: Coding of Packet M/29/4
				if ((triplet0 & 0xff) == 0x00) {
					primary_charset.g0_m29 = (triplet0 & 0x3f80) >> 7;
					// X/28 takes precedence over M/29
					if (primary_charset.g0_x28 == UNDEF) {
						remap_g0_charset(primary_charset.g0_m29);
					}
				}
			}
		}
	}
	else if ((m == 8) && (y == 30)) {
		// ETS 300 706, chapter 9.8: Broadcast Service Data Packets
		if (states.programme_info_processed == NO) {
			// ETS 300 706, chapter 9.8.1: Packet 8/30 Format 1
			if (unham_8_4(packet->data[0]) < 2) {
				uint32_t t = 0;
				time_t t0;
				mprint ("- Programme Identification Data = ");
				for (uint8_t i = 20; i < 40; i++) {
					char u[4] = { 0, 0, 0, 0 };
					uint8_t c = telx_to_ucs2(packet->data[i]);
					// strip any control codes from PID, eg. TVP station
					if (c < 0x20) continue;

					ucs2_to_utf8(u, c);
					mprint ( "%s", u);
				}
				mprint ("\n");

				// OMG! ETS 300 706 stores timestamp in 7 bytes in Modified Julian Day in BCD format + HH:MM:SS in BCD format
				// + timezone as 5-bit count of half-hours from GMT with 1-bit sign
				// In addition all decimals are incremented by 1 before transmission.
				// 1st step: BCD to Modified Julian Day
				t += (packet->data[10] & 0x0f) * 10000;
				t += ((packet->data[11] & 0xf0) >> 4) * 1000;
				t += (packet->data[11] & 0x0f) * 100;
				t += ((packet->data[12] & 0xf0) >> 4) * 10;
				t += (packet->data[12] & 0x0f);
				t -= 11111;
				// 2nd step: conversion Modified Julian Day to unix timestamp
				t = (t - 40587) * 86400;
				// 3rd step: add time
				t += 3600 * ( ((packet->data[13] & 0xf0) >> 4) * 10 + (packet->data[13] & 0x0f) );
				t +=   60 * ( ((packet->data[14] & 0xf0) >> 4) * 10 + (packet->data[14] & 0x0f) );
				t +=        ( ((packet->data[15] & 0xf0) >> 4) * 10 + (packet->data[15] & 0x0f) );
				t -= 40271;
				// 4th step: conversion to time_t
				t0 = (time_t)t;
				// ctime output itself is \n-ended
				mprint ("- Universal Time Co-ordinated = %s", ctime(&t0));

				dbg_print (CCX_DMT_TELETEXT, "- Transmission mode = %s\n", (transmission_mode == TRANSMISSION_MODE_SERIAL ? "serial" : "parallel"));

				if (tlt_config.write_format == CCX_OF_TRANSCRIPT && tlt_config.date_format==ODF_DATE && !tlt_config.noautotimeref) {
                    mprint ("- Broadcast Service Data Packet received, resetting UTC referential value to %s", ctime(&t0));
                    utc_refvalue = t;
                    states.pts_initialized = NO;
				}

				states.programme_info_processed = YES;
			}
		}
	}
}

void tlt_write_rcwt(struct lib_ccx_ctx *ctx, uint8_t data_unit_id, uint8_t *packet, uint64_t timestamp)
{
	struct lib_cc_decode *dec_ctx = ctx->dec_ctx;
	dec_ctx->writedata((unsigned char *) &data_unit_id, sizeof(uint8_t), dec_ctx->context_cc608_field_1, NULL);
	dec_ctx->writedata((unsigned char *) &timestamp, sizeof(uint64_t), dec_ctx->context_cc608_field_1, NULL);
	dec_ctx->writedata((unsigned char *) packet, 44, dec_ctx->context_cc608_field_1, NULL);
}

void tlt_read_rcwt(struct lib_ccx_ctx *ctx)
{
	int len = 1 + 8 + 44;
	unsigned char *buf = (unsigned char *) malloc(len);
	if (buf == NULL)
		fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory");

	while(1) {
		buffered_read(ctx, buf, len);
		ctx->past += result;

		if (result != len) {
			end_of_file = 1;
			free(buf);
			return;
		}

		data_unit_t id = buf[0];
		uint64_t t;
		memcpy(&t, &buf[1], sizeof(uint64_t));
		teletext_packet_payload_t *pl = (teletext_packet_payload_t *)&buf[9];

		last_timestamp = t;

		process_telx_packet(ctx, id, pl, t);
	}
}

void tlt_process_pes_packet(struct lib_ccx_ctx *ctx, uint8_t *buffer, uint16_t size)
{
	uint64_t pes_prefix;
	uint8_t pes_stream_id;
	uint16_t pes_packet_length;
	uint8_t optional_pes_header_included;
	uint16_t optional_pes_header_length;
	static uint8_t using_pts = UNDEF;
	uint32_t t = 0;
	uint16_t i;
	static int64_t delta = 0;
	static uint32_t t0 = 0;
	tlt_packet_counter++;
	if (size < 6) return;

	// Packetized Elementary Stream (PES) 32-bit start code
	pes_prefix = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
	pes_stream_id = buffer[3];

	// check for PES header
	if (pes_prefix != 0x000001) return;

	// stream_id is not "Private Stream 1" (0xbd)
	if (pes_stream_id != 0xbd) return;

	// PES packet length
	// ETSI EN 301 775 V1.2.1 (2003-05) chapter 4.3: (N x 184) - 6 + 6 B header
	pes_packet_length = 6 + ((buffer[4] << 8) | buffer[5]);
	// Can be zero. If the "PES packet length" is set to zero, the PES packet can be of any length.
	// A value of zero for the PES packet length can be used only when the PES packet payload is a video elementary stream.
	if (pes_packet_length == 6) return;

	// truncate incomplete PES packets
	if (pes_packet_length > size) pes_packet_length = size;

	optional_pes_header_included = NO;
	optional_pes_header_length = 0;
	// optional PES header marker bits (10.. ....)
	if ((buffer[6] & 0xc0) == 0x80) {
		optional_pes_header_included = YES;
		optional_pes_header_length = buffer[8];
	}

	// should we use PTS or PCR?
	if (using_pts == UNDEF) {
		if ((optional_pes_header_included == YES) && ((buffer[7] & 0x80) > 0)) {
			using_pts = YES;
			dbg_print (CCX_DMT_TELETEXT, "- PID 0xbd PTS available\n");
		} else {
			using_pts = NO;
			dbg_print (CCX_DMT_TELETEXT, "- PID 0xbd PTS unavailable, using TS PCR\n");
		}
	}

	// If there is no PTS available, use global PCR
	if (using_pts == NO) {
		t = ctx->global_timestamp;
	}
	// if (using_pts == NO) t = get_pts();
	else {
		// PTS is 33 bits wide, however, timestamp in ms fits into 32 bits nicely (PTS/90)
		// presentation and decoder timestamps use the 90 KHz clock, hence PTS/90 = [ms]
		uint64_t pts = 0;
		// __MUST__ assign value to uint64_t and __THEN__ rotate left by 29 bits
		// << is defined for signed int (as in "C" spec.) and overflow occures
		pts = (buffer[9] & 0x0e);
		pts <<= 29;
		pts |= (buffer[10] << 22);
		pts |= ((buffer[11] & 0xfe) << 14);
		pts |= (buffer[12] << 7);
		pts |= ((buffer[13] & 0xfe) >> 1);
		t = (uint32_t) (pts / 90);
	}

	if (states.pts_initialized == NO) {
		if (utc_refvalue == UINT64_MAX)
			delta = (uint64_t) (ctx->subs_delay - t);
		else
			delta = (uint64_t) (ctx->subs_delay + 1000 * utc_refvalue - t);
		t0 = t;
		states.pts_initialized = YES;
		if ((using_pts == NO) && (ctx->global_timestamp == 0)) {
			// We are using global PCR, nevertheless we still have not received valid PCR timestamp yet
			states.pts_initialized = NO;
		}
	}
	if (t < t0) delta = last_timestamp;
	last_timestamp = t + delta;
    if (delta < 0 && last_timestamp > t) {
        last_timestamp = 0;
    }
	t0 = t;

	// skip optional PES header and process each 46-byte teletext packet
	i = 7;
	if (optional_pes_header_included == YES) i += 3 + optional_pes_header_length;
	while (i <= pes_packet_length - 6) {
		uint8_t data_unit_id = buffer[i++];
		uint8_t data_unit_len = buffer[i++];

		if ((data_unit_id == DATA_UNIT_EBU_TELETEXT_NONSUBTITLE) || (data_unit_id == DATA_UNIT_EBU_TELETEXT_SUBTITLE)) {
			// teletext payload has always size 44 bytes
			if (data_unit_len == 44) {
				// reverse endianess (via lookup table), ETS 300 706, chapter 7.1
				for (uint8_t j = 0; j < data_unit_len; j++) buffer[i + j] = REVERSE_8[buffer[i + j]];

				if (tlt_config.write_format == CCX_OF_RCWT)
					tlt_write_rcwt(ctx, data_unit_id, &buffer[i], last_timestamp);
				else
					// FIXME: This explicit type conversion could be a problem some day -- do not need to be platform independant
					process_telx_packet(ctx, (data_unit_t) data_unit_id, (teletext_packet_payload_t *)&buffer[i], last_timestamp);
			}
		}

		i += data_unit_len;
	}
}

void analyze_pat(uint8_t *buffer, uint8_t size) {
	pat_t pat = { 0 };
	if (size < 7) return;
	pat.pointer_field = buffer[0];

//!
if (pat.pointer_field > 0) {
	fatal(CCX_COMMON_EXIT_UNSUPPORTED, "! pat.pointer_field > 0 (0x%02x)\n\n", pat.pointer_field);
}

	pat.table_id = buffer[1];
	if (pat.table_id == 0x00) {
		pat.section_length = ((buffer[2] & 0x03) << 8) | buffer[3];
		pat.current_next_indicator = buffer[6] & 0x01;
		// already valid PAT
		if (pat.current_next_indicator == 1) {
			uint16_t i = 9;
			while ((i < 9 + (pat.section_length - 5 - 4)) && (i < size)) {
				pat_section_t section = { 0 };
				section.program_num = (buffer[i] << 8) | buffer[i + 1];
				section.program_pid = ((buffer[i + 2] & 0x1f) << 8) | buffer[i + 3];

				if (in_array(pmt_map, pmt_map_count, section.program_pid) == NO) {
					if (pmt_map_count < TS_PMT_MAP_SIZE) {
						pmt_map[pmt_map_count++] = section.program_pid;
#ifdef DEBUG
						fprintf(stderr, "- Found PMT for SID %"PRIu16" (0x%x)\n", section.program_num, section.program_num);
#endif
					}
				}

				i += 4;
			}
		}
	}
}

void analyze_pmt(uint8_t *buffer, uint8_t size) {
	pmt_t pmt = { 0 };
	if (size < 7) return;

	pmt.pointer_field = buffer[0];

//!
if (pmt.pointer_field > 0) {
	fatal(CCX_COMMON_EXIT_UNSUPPORTED, "! pmt.pointer_field > 0 (0x%02x)\n\n", pmt.pointer_field);
}

	pmt.table_id = buffer[1];
	if (pmt.table_id == 0x02) {
		pmt.section_length = ((buffer[2] & 0x03) << 8) | buffer[3];
		pmt.program_num = (buffer[4] << 8) | buffer[5];
		pmt.current_next_indicator = buffer[6] & 0x01;
		pmt.pcr_pid = ((buffer[9] & 0x1f) << 8) | buffer[10];
		pmt.program_info_length = ((buffer[11] & 0x03) << 8) | buffer[12];
		// already valid PMT
		if (pmt.current_next_indicator == 1) {
			uint16_t i = 13 + pmt.program_info_length;
			while ((i < 13 + (pmt.program_info_length + pmt.section_length - 4 - 9)) && (i < size)) {
				uint8_t descriptor_tag;
				pmt_program_descriptor_t desc = { 0 };
				desc.ccx_stream_type = buffer[i];
				desc.elementary_pid = ((buffer[i + 1] & 0x1f) << 8) | buffer[i + 2];
				desc.es_info_length = ((buffer[i + 3] & 0x03) << 8) | buffer[i + 4];

				descriptor_tag = buffer[i + 5];
                                // descriptor_tag: 0x45 = VBI_data_descriptor, 0x46 = VBI_teletext_descriptor, 0x56 = teletext_descriptor
				if ((desc.ccx_stream_type == 0x06) && ((descriptor_tag == 0x45) || (descriptor_tag == 0x46) || (descriptor_tag == 0x56))) {
					if (in_array(pmt_ttxt_map, pmt_ttxt_map_count, desc.elementary_pid) == NO) {
						if (pmt_ttxt_map_count < TS_PMT_TTXT_MAP_SIZE) {
							pmt_ttxt_map[pmt_ttxt_map_count++] = desc.elementary_pid;
							if (tlt_config.tid == 0) tlt_config.tid = desc.elementary_pid;
							mprint ("- Found VBI/teletext stream ID %"PRIu16" (0x%x) for SID %"PRIu16" (0x%x)\n", desc.elementary_pid, desc.elementary_pid, pmt.program_num, pmt.program_num);
						}
					}
				}

				i += 5 + desc.es_info_length;
			}
		}
	}
}

// graceful exit support
uint8_t exit_request = NO;

// Called only when teletext is detected or forced and it's going to be used for extraction.
void telxcc_init(struct lib_ccx_ctx *ctx)
{
	if (!telxcc_inited)
	{
		telxcc_inited=1;
		if (ctx->wbout1.fh!=-1 && tlt_config.encoding!=CCX_ENC_UTF_8) // If encoding it UTF8 then this was already done
			fdprintf(ctx->wbout1.fh, "\xef\xbb\xbf");
		memset (seen_sub_page,0,MAX_TLT_PAGES*sizeof (short int));
	}
}

// Close output
void telxcc_close(struct lib_ccx_ctx *ctx)
{
	if (telxcc_inited && tlt_config.write_format != CCX_OF_RCWT)
	{
		// output any pending close caption
		if (page_buffer.tainted == YES) {
			// this time we do not subtract any frames, there will be no more frames
			page_buffer.hide_timestamp = last_timestamp;
			process_page(ctx, &page_buffer);
		}

		telxcc_dump_prev_page(ctx);

		if ((tlt_frames_produced == 0) && (tlt_config.nonempty == YES)) {
			if (ctx->wbout1.fh!=-1) fdprintf(ctx->wbout1.fh, "1\r\n00:00:00,000 --> 00:00:10,000\r\n(no closed captions available)\r\n\r\n");
			tlt_frames_produced++;
		}
	}
}

#ifdef DEBUG_TELXCC
int main_telxcc (int argc, char *argv[]) {
	FILE *infile;

	// TS packet buffer
	uint8_t ts_buffer[TS_PACKET_SIZE] = { 0 };

	// 255 means not set yet
	uint8_t continuity_counter = 255;

	// PES packet buffer
	uint8_t payload_buffer[PAYLOAD_BUFFER_SIZE] = { 0 };
	uint16_t payload_counter = 0;

/*
	// command line params parsing
	for (uint8_t i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-h") == 0) {
			fprintf(stderr, "Usage: telxcc [-h] [-v] [-p PAGE] [-t TID] [-o OFFSET] [-n] [-1] [-c]\n");
			fprintf(stderr, "  STDIN       transport stream\n");
			fprintf(stderr, "  STDOUT      subtitles in SubRip SRT file format (UTF-8 encoded)\n");
			fprintf(stderr, "  -h          this help text\n");
			fprintf(stderr, "  -v          be verbose\n");
			fprintf(stderr, "  -p PAGE     teletext page number carrying closed captions\n");
			fprintf(stderr, "  -t TID      transport stream PID of teletext data sub-stream\n");
			fprintf(stderr, "  -o OFFSET   subtitles offset in seconds\n");
			fprintf(stderr, "  -n          do not print UTF-8 BOM characters to the file\n");
			fprintf(stderr, "  -1          produce at least one (dummy) frame\n");
			fprintf(stderr, "  -c          output colour information in font HTML tags\n");
			fprintf(stderr, "\n");
			exit(EXIT_SUCCESS);
		}
		else if ((strcmp(argv[i], "-t") == 0) && (argc > i + 1))
			tlt_config.tid = atoi(argv[++i]);
		else if ((strcmp(argv[i], "-o") == 0) && (argc > i + 1))
			tlt_config.offset = atof(argv[++i]);
		else if (strcmp(argv[i], "-n") == 0)
			tlt_config.bom = NO;
		else if (strcmp(argv[i], "-1") == 0)
			tlt_config.nonempty = YES;
		else if (strcmp(argv[i], "-c") == 0)
			tlt_config.colours = YES;
		else {
			fprintf(stderr, "- Unknown option %s\n", argv[i]);
			exit(EXIT_FAILURE);
		}
	}
*/

	infile=fopen ("F:\\ConBackup\\Closed captions\\BrokenSamples\\RDVaughan\\uk_dvb_example.mpg","rb");

	// full buffering -- disables flushing after CR/FL, we will flush manually whole SRT frames
	// setvbuf(stdout, (char*)NULL, _IOFBF, 0);

	// print UTF-8 BOM chars
	if (tlt_config.bom == YES) {
		if (ctx->wbout1.fh!=-1) fdprintf(ctx->wbout1.fh, "\xef\xbb\xbf");
	}
	// reading input
	while ((exit_request == NO) && (fread(&ts_buffer, 1, TS_PACKET_SIZE, infile) == TS_PACKET_SIZE)) {
		uint8_t af_discontinuity = 0;
		// Transport Stream Header
		// We do not use buffer to struct loading (e.g. ts_packet_t *header = (ts_packet_t *)buffer;)
		// -- struct packing is platform dependant and not performing well.
		ts_packet_t header = { 0 };
		header.sync = ts_buffer[0];
		header.transport_error = (ts_buffer[1] & 0x80) >> 7;
		header.payload_unit_start = (ts_buffer[1] & 0x40) >> 6;
		header.transport_priority = (ts_buffer[1] & 0x20) >> 5;
		header.pid = ((ts_buffer[1] & 0x1f) << 8) | ts_buffer[2];
		header.scrambling_control = (ts_buffer[3] & 0xc0) >> 6;
		header.adaptation_field_exists = (ts_buffer[3] & 0x20) >> 5;
		header.continuity_counter = ts_buffer[3] & 0x0f;
		//uint8_t ts_payload_exists = (ts_buffer[3] & 0x10) >> 4;

		if (header.adaptation_field_exists > 0) {
			af_discontinuity = (ts_buffer[5] & 0x80) >> 7;
		}

		// not TS packet?
		if (header.sync != 0x47)
			fatal (CCX_COMMON_EXIT_UNSUPPORTED, "- Invalid TS packet header\n- telxcc does not work with unaligned TS.\n\n");

		// uncorrectable error?
		if (header.transport_error > 0) {
			dbg_print (CCX_DMT_TELETEXT, "- Uncorrectable TS packet error (received CC %1x)\n", header.continuity_counter);
			continue;
		}

		// if available, calculate current PCR
		if (header.adaptation_field_exists > 0) {
			// PCR in adaptation field
			uint8_t af_pcr_exists = (ts_buffer[5] & 0x10) >> 4;
			if (af_pcr_exists > 0) {
				uint64_t pts = 0;
				pts |= (ts_buffer[6] << 25);
				pts |= (ts_buffer[7] << 17);
				pts |= (ts_buffer[8] << 9);
				pts |= (ts_buffer[9] << 1);
				pts |= (ts_buffer[10] >> 7);
				ctx->global_timestamp = (uint32_t) pts / 90;
				pts = ((ts_buffer[10] & 0x01) << 8);
				pts |= ts_buffer[11];
				ctx->global_timestamp += (uint32_t) (pts / 27000);
				if (!ctx->global_timestamp_inited)
				{
					min_global_timestamp = ctx->global_timestamp;
					ctx->global_timestamp_inited = 1;
				}
			}
		}

		// null packet
		if (header.pid == 0x1fff) continue;

		// TID not specified, autodetect via PAT/PMT
		if (tlt_config.tid == 0) {
			// process PAT
			if (header.pid == 0x0000) {
				analyze_pat(&ts_buffer[4], TS_PACKET_PAYLOAD_SIZE);
				continue;
			}

			// process PMT
			if (in_array(pmt_map, pmt_map_count, header.pid) == YES) {
				analyze_pmt(&ts_buffer[4], TS_PACKET_PAYLOAD_SIZE);
				continue;
			}
		}

		if (tlt_config.tid == header.pid) {
			// TS continuity check
			if (continuity_counter == 255) continuity_counter = header.continuity_counter;
			else {
				if (af_discontinuity == 0) {
					continuity_counter = (continuity_counter + 1) % 16;
					if (header.continuity_counter != continuity_counter) {
						dbg_print (CCX_DMT_TELETEXT, "- Missing TS packet, flushing pes_buffer (expected CC %1x, received CC %1x, TS discontinuity %s, TS priority %s)\n",
							continuity_counter, header.continuity_counter, (af_discontinuity ? "YES" : "NO"), (header.transport_priority ? "YES" : "NO"));
						payload_counter = 0;
						continuity_counter = 255;
					}
				}
			}

			// waiting for first payload_unit_start indicator
			if ((header.payload_unit_start == 0) && (payload_counter == 0)) continue;

			// proceed with payload buffer
			if ((header.payload_unit_start > 0) && (payload_counter > 0)) tlt_process_pes_packet(payload_buffer, payload_counter);

			// new payload frame start
			if (header.payload_unit_start > 0) payload_counter = 0;

			// add payload data to buffer
			if (payload_counter < (PAYLOAD_BUFFER_SIZE - TS_PACKET_PAYLOAD_SIZE)) {
				memcpy(&payload_buffer[payload_counter], &ts_buffer[4], TS_PACKET_PAYLOAD_SIZE);
				payload_counter += TS_PACKET_PAYLOAD_SIZE;
				tlt_packet_counter++;
			}
			else
				dbg_print (CCX_DMT_TELETEXT, "- packet payload size exceeds payload_buffer size, probably not teletext stream\n");
		}
	}


	VERBOSE_ONLY {
		if (tlt_config.tid == 0) dbg_print (CCX_DMT_TELETEXT, "- No teletext PID specified, no suitable PID found in PAT/PMT tables. Please specify teletext PID via -t parameter.\n");
		if (tlt_frames_produced == 0) dbg_print (CCX_DMT_TELETEXT, "- No frames produced. CC teletext page number was probably wrong.\n");
		dbg_print (CCX_DMT_TELETEXT, "- There were some CC data carried via pages = ");
		// We ignore i = 0xff, because 0xffs are teletext ending frames
		for (uint16_t i = 0; i < 255; i++)
			for (uint8_t j = 0; j < 8; j++) {
				uint8_t v = cc_map[i] & (1 << j);
				if (v > 0) fprintf(stderr, "%03x ", ((j + 1) << 8) | i);
			}
		fprintf(stderr, "\n");
	}


	return EXIT_SUCCESS;
}
#endif
