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
if (ctx->buffer == NULL) { fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory for reallocating buffer, bailing out\n"); } \
}

// CC page dimensions
#define ROWS                    15
#define COLUMNS                 32

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
	/* common buffer used by all encoder */
	unsigned char *buffer;
	/* capacity of buffer */
	unsigned int capacity;
	/* keep count of srt subtitle*/
	unsigned int srt_counter;
	/* keep count of CEA-708 subtitle*/
	unsigned int cea_708_counter;

	/* Did we write the WebVTT header already? */
	unsigned int wrote_webvtt_header;

	/* Input outputs */
	/* Flag giving hint that output is send to server through network */
	unsigned int send_to_srv;
	/* Used only in Spupng output */
	int multiple_files;
	/* Used only in Spupng output and creating name of output file*/
	char *first_input_file;
	/* Its array with length of number of languages */
	struct ccx_s_write *out;
	/* number of member in array of write out array */
	int nb_out;
	/* Input file format used in Teletext for exceptional output */
	unsigned int in_fileformat; //1 = Normal, 2 = Teletext
	/* Keep output file closed when not actually writing to it and start over each time (add headers, etc) */
	unsigned int keep_output_closed;
	/* Force a flush on the file buffer whenever content is written */
	int force_flush;
	/* Keep track of whether -UCLA used */
	int ucla;

	struct ccx_common_timing_ctx *timing; /* Some encoders need access to PTS, such as WebVTT */

	/* Flag saying BOM to be written in each output file */
	enum ccx_encoding_type encoding;
	enum ccx_output_format write_format;                        // 0=Raw, 1=srt, 2=SMI
	int generates_file;
	struct ccx_encoders_transcript_format *transcript_settings; // Keeps the settings for generating transcript output files.
	int no_bom;
	int sentence_cap ;                                          // FIX CASE? = Fix case?

	int trim_subs;                                              // "    Remove spaces at sides?    "
	int autodash;                                               // Add dashes (-) before each speaker automatically?
	int no_font_color;
	int no_type_setting;
	int gui_mode_reports;                                       // If 1, output in stderr progress updates so the GUI can grab them
	unsigned char *subline;                                     // Temp storage for storing each line
	int extract;

	int dtvcc_extract;                                          // 1 or 0 depending if we have to handle dtvcc
	ccx_dtvcc_writer_ctx dtvcc_writers[CCX_DTVCC_MAX_SERVICES];

	/* Timing related variables*/
	/* start time of previous sub */
	LLONG prev_start;
	LLONG subs_delay;
	LLONG last_displayed_subs_ms;
	enum ccx_output_date_format date_format;
	char millis_separator;

	/* Credit stuff */
	int startcredits_displayed;
	char *start_credits_text;
	char *end_credits_text;
	struct ccx_boundary_time startcreditsnotbefore, startcreditsnotafter;   // Where to insert start credits, if possible
	struct ccx_boundary_time startcreditsforatleast, startcreditsforatmost; // How long to display them?
	struct ccx_boundary_time endcreditsforatleast, endcreditsforatmost;

	// Preencoded strings
	unsigned char encoded_crlf[16];
	unsigned int encoded_crlf_length;
	unsigned char encoded_br[16];
	unsigned int encoded_br_length;

	int new_sentence; // Capitalize next letter?

	int program_number;
	struct list_head list;

	/* split-by-sentence stuff */
	int sbs_enabled;

	//for dvb subs
	struct encoder_ctx* prev;
	int write_previous;

	// Segmenting
	int segment_pending;
	int segment_last_key_frame;

	// OCR in SPUPNG
	int nospupngocr;
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
int write_cc_buffer_as_spupng         (struct eia608_screen *data, struct encoder_ctx *context);
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
int write_cc_bitmap_as_libcurl         (struct cc_subtitle *sub, struct encoder_ctx *context);

void write_spumux_header(struct encoder_ctx *ctx, struct ccx_s_write *out);
void write_spumux_footer(struct ccx_s_write *out);

struct cc_subtitle * reformat_cc_bitmap_through_sentence_buffer (struct cc_subtitle *sub, struct encoder_ctx *context);

void set_encoder_last_displayed_subs_ms(struct encoder_ctx *ctx, LLONG last_displayed_subs_ms);
void set_encoder_subs_delay(struct encoder_ctx *ctx, LLONG subs_delay);
void set_encoder_startcredits_displayed(struct encoder_ctx *ctx, int startcredits_displayed);
void set_encoder_rcwt_fileformat(struct encoder_ctx *ctx, short int format);

int reset_output_ctx(struct encoder_ctx *ctx, struct encoder_cfg *cfg);

void find_limit_characters(unsigned char *line, int *first_non_blank, int *last_non_blank, int max_len);
int get_str_basic(unsigned char *out_buffer, unsigned char *in_buffer, int trim_subs,
	enum ccx_encoding_type in_enc, enum ccx_encoding_type out_enc, int max_len);


unsigned int get_line_encoded(struct encoder_ctx *ctx, unsigned char *buffer, int line_num, struct eia608_screen *data);
unsigned int get_color_encoded(struct encoder_ctx *ctx, unsigned char *buffer, int line_num, struct eia608_screen *data);
unsigned int get_font_encoded(struct encoder_ctx *ctx, unsigned char *buffer, int line_num, struct eia608_screen *data);
int pass_cc_buffer_to_python(struct eia608_screen *data, struct encoder_ctx *context);

struct lib_ccx_ctx;
void switch_output_file(struct lib_ccx_ctx *ctx, struct encoder_ctx *enc_ctx, int track_id);
#endif
