#ifndef _CC_ENCODER_COMMON_H
#define _CC_ENCODER_COMMON_H

#ifdef WIN32
	#include "..\\win_iconv\\iconv.h"
#else
	#include "iconv.h"
#endif

#include "ccx_common_structs.h"
#include "ccx_decoders_structs.h"
#include "ccx_encoders_structs.h"
#include "ccx_common_option.h"

#define REQUEST_BUFFER_CAPACITY(ctx,length) if (length>ctx->capacity) \
{ctx->capacity = length * 2; ctx->buffer = (unsigned char*)realloc(ctx->buffer, ctx->capacity); \
if (ctx->buffer == NULL) { fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory, bailing out\n"); } \
}

typedef struct ccx_dtvcc_writer_ctx
{
	int fd;
	char *filename;
	iconv_t cd;
} ccx_dtvcc_writer_ctx;

typedef struct ccx_sbs_utf8_character
{
	int32_t ch;
	LLONG ts;
	char encoded[4];
	int enc_len;
} ccx_sbs_utf8_character;

/**
 * Context of encoder, This structure gives single interface
 * to all encoder
 */
struct encoder_ctx
{
	// common buffer used by all encoder
	unsigned char *buffer;
	// capacity of buffer
	unsigned int capacity;
	// keep count of srt subtitle
	unsigned int srt_counter;

	// did we write the WebVTT sync header already?
	unsigned int wrote_webvtt_sync_header;

	// inputs and outputs
	// flag giving hint that output is send to server through network
	unsigned int send_to_srv;
	// used only in Spupng output
	int multiple_files;
	// used only in Spupng output and creating name of output file
	char *first_input_file;
	// its array with length of number of languages
	struct ccx_s_write *out;
	// number of member in array of write out array
	int nb_out;
	// input file format used in Teletext for exceptional output
	unsigned int in_fileformat; // 1 =Normal, 2=Teletext
	// keep output file closed when not actually writing to it and start over each time (add headers, etc)
	unsigned int keep_output_closed; 
	// force a flush on the file buffer whenever content is written
	int force_flush;
	// keep track of whether -UCLA used
	int ucla;

	struct ccx_common_timing_ctx *timing; // some encoders need access to PTS, such as WebVTT

	// flag saying BOM to be written in each output file
	enum ccx_encoding_type encoding;
	enum ccx_output_format write_format; // 0 = Raw, 1 = srt, 2 = SMI
	int generates_file;
	// keeps the settings for generating transcript output files
	struct ccx_encoders_transcript_format *transcript_settings;
	int no_bom;
	// FIX CASE? = Fix case?
	int sentence_cap ;

	// remove spaces at sides?   
	int trim_subs;
	// add dashes (-) before each speaker automatically?
	int autodash;
	int no_font_color;
	int no_type_setting;
	// if 1, output in stderr progress updates so the GUI can grab them
	int gui_mode_reports;
	// temp storage for storing each line
	unsigned char *subline;
	int extract;

	// 1 or 0 depending if we have to handle dtvcc
	int dtvcc_extract;
	ccx_dtvcc_writer_ctx dtvcc_writers[CCX_DTVCC_MAX_SERVICES];

	/**
	 * Timing related variables
	 */
	// start time of previous sub
	LLONG prev_start;
	LLONG subs_delay;
	LLONG last_displayed_subs_ms;
	enum ccx_output_date_format date_format;
	char millis_separator;

	// credit stuff
	int startcredits_displayed;
	char *start_credits_text;
	char *end_credits_text;
	struct ccx_boundary_time startcreditsnotbefore, startcreditsnotafter; // where to insert start credits, if possible
	struct ccx_boundary_time startcreditsforatleast, startcreditsforatmost; // how long to display them?
	struct ccx_boundary_time endcreditsforatleast, endcreditsforatmost;

	// preencoded strings
	unsigned char encoded_crlf[16];
	unsigned int encoded_crlf_length;
	unsigned char encoded_br[16];
	unsigned int encoded_br_length;

	// capitalize next letter?
	int new_sentence;

	int program_number;
	struct list_head list;

	// split-by-sentence stuff
	int splitbysentence;
	// used by the split-by-sentence code to know when the current block starts...
	LLONG sbs_newblock_start_time;
	LLONG sbs_newblock_end_time; // ... and ends
	ccx_sbs_utf8_character *sbs_newblock;
	int sbs_newblock_capacity;
	int sbs_newblock_size;
	ccx_sbs_utf8_character *sbs_buffer;
	int sbs_buffer_capacity;
	int sbs_buffer_size;

};

#define INITIAL_ENC_BUFFER_CAPACITY	2048
/**
 * Inialize encoder context with output context
 * allocate initial memory to buffer of context
 * write subtitle header to file refrenced by
 * output context
 *
 * @param cfg Option to initilaize encoder cfg params
 *
 * @return Allocated and properly initilaized Encoder Context, NULL on failure
 */
struct encoder_ctx *init_encoder(struct encoder_cfg *opt);

/**
 * try to add end credits in subtitle file and then write subtitle
 * footer
 *
 * deallocate encoder ctx, so before using encoder_ctx again
 * after deallocating user need to allocate encoder ctx again
 *
 * @oaram arg pointer to initialized encoder ctx using init_encoder
 * 
 * @param current_fts to calculate window for end credits
 */
void dinit_encoder(struct encoder_ctx **arg, LLONG current_fts);

/**
 * @param ctx encoder context
 * @param sub subtitle context returned by decoder
 */
int encode_sub(struct encoder_ctx *ctx,struct cc_subtitle *sub);

int write_cc_buffer_as_srt            (struct eia608_screen *data, struct encoder_ctx *context);
int write_cc_buffer_as_ssa            (struct eia608_screen *data, struct encoder_ctx *context);
int write_cc_buffer_as_webvtt         (struct eia608_screen *data, struct encoder_ctx *context);
int write_cc_buffer_as_sami           (struct eia608_screen *data, struct encoder_ctx *context);
int write_cc_buffer_as_smptett        (struct eia608_screen *data, struct encoder_ctx *context);
void write_cc_buffer_to_gui           (struct eia608_screen *data, struct encoder_ctx *context);

int write_cc_buffer_as_g608           (struct eia608_screen *data, struct encoder_ctx *context);
int write_cc_buffer_as_transcript2    (struct eia608_screen *data, struct encoder_ctx *context);

void write_cc_line_as_transcript2     (struct eia608_screen *data, struct encoder_ctx *context, int line_number);

int write_cc_subtitle_as_srt          (struct cc_subtitle *sub, struct encoder_ctx *context);
int write_cc_subtitle_as_ssa          (struct cc_subtitle *sub, struct encoder_ctx *context);
int write_cc_subtitle_as_webvtt       (struct cc_subtitle *sub, struct encoder_ctx *context);
int write_cc_subtitle_as_sami         (struct cc_subtitle *sub, struct encoder_ctx *context);
int write_cc_subtitle_as_smptett      (struct cc_subtitle *sub, struct encoder_ctx *context);
int write_cc_subtitle_as_spupng       (struct cc_subtitle *sub, struct encoder_ctx *context);
int write_cc_subtitle_as_transcript   (struct cc_subtitle *sub, struct encoder_ctx *context);


int write_stringz_as_srt              (char *string, struct encoder_ctx *context, LLONG ms_start, LLONG ms_end);
int write_stringz_as_ssa              (char *string, struct encoder_ctx *context, LLONG ms_start, LLONG ms_end);
int write_stringz_as_webvtt           (char *string, struct encoder_ctx *context, LLONG ms_start, LLONG ms_end);
int write_stringz_as_sami             (char *string, struct encoder_ctx *context, LLONG ms_start, LLONG ms_end);
void write_stringz_as_smptett         (char *string, struct encoder_ctx *context, LLONG ms_start, LLONG ms_end);


int write_cc_bitmap_as_srt             (struct cc_subtitle *sub, struct encoder_ctx *context);
int write_cc_bitmap_as_ssa             (struct cc_subtitle *sub, struct encoder_ctx *context);
int write_cc_bitmap_as_webvtt          (struct cc_subtitle *sub, struct encoder_ctx *context);
int write_cc_bitmap_as_sami            (struct cc_subtitle *sub, struct encoder_ctx *context);
int write_cc_bitmap_as_smptett         (struct cc_subtitle *sub, struct encoder_ctx *context);
int write_cc_bitmap_as_spupng          (struct cc_subtitle *sub, struct encoder_ctx *context);
int write_cc_bitmap_as_transcript      (struct cc_subtitle *sub, struct encoder_ctx *context);
int write_cc_bitmap_to_sentence_buffer (struct cc_subtitle *sub, struct encoder_ctx *context);
int write_cc_bitmap_as_libcurl         (struct cc_subtitle *sub, struct encoder_ctx *context);



void set_encoder_last_displayed_subs_ms(struct encoder_ctx *ctx, LLONG last_displayed_subs_ms);
void set_encoder_subs_delay	       (struct encoder_ctx *ctx, LLONG subs_delay);
void set_encoder_startcredits_displayed(struct encoder_ctx *ctx, int startcredits_displayed);
void set_encoder_rcwt_fileformat       (struct encoder_ctx *ctx, short int format);

void find_limit_characters	       (unsigned char *line,
					int *first_non_blank,
					int *last_non_blank,
					int max_len);
int get_str_basic       	       (unsigned char *out_buffer,
					unsigned char *in_buffer,
					int trim_subs,
					enum ccx_encoding_type in_enc,
					enum ccx_encoding_type out_enc,
					int max_len);
#endif
