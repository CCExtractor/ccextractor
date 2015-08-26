#ifndef _INCLUDE_708_
#define _INCLUDE_708_

#include <sys/stat.h>
#include "ccx_common_platform.h"
#include "ccx_common_constants.h"
#include "ccx_common_structs.h"

#define DTVCC_MAX_PACKET_LENGTH 128 //According to EIA-708B, part 5
#define DTVCC_MAX_SERVICES 63

#define DTVCC_MAX_ROWS 15
/**
 * This value should be 32, but there were 16-bit encoded samples (from Korea),
 * where RowCount calculated another way and equals 46 (23[8bit]*2)
 */
#define DTVCC_MAX_COLUMNS (32*2)

#define DTVCC_SCREENGRID_ROWS 75
#define DTVCC_SCREENGRID_COLUMNS 210

#define DTVCC_MAX_WINDOWS 8

#define DTVCC_FILENAME_TEMPLATE ".p%u.svc%02u"

#define DTVCC_NO_LAST_SEQUENCE -1

enum DTVCC_COMMANDS_C0_CODES
{
	NUL		= 0,
	ETX		= 3,
	BS 		= 8,
	FF 		= 0xC,
	CR 		= 0xD,
	HCR 	= 0xE,
	EXT1 	= 0x10,
	P16 	= 0x18
};

enum DTVCC_COMMANDS_C1_CODES
{
	CW0 = 0x80,
	CW1 = 0x81,
	CW2 = 0x82,
	CW3 = 0x83,
	CW4 = 0x84,
	CW5 = 0x85,
	CW6 = 0x86,
	CW7 = 0x87,
	CLW = 0x88,
	DSW = 0x89,
	HDW = 0x8A,
	TGW = 0x8B,
	DLW = 0x8C,
	DLY = 0x8D,
	DLC = 0x8E,
	RST = 0x8F,
	SPA = 0x90,
	SPC = 0x91,
	SPL = 0x92,
	RSV93 = 0x93,
	RSV94 = 0x94,
	RSV95 = 0x95,
	RSV96 = 0x96,
	SWA = 0x97,
	DF0 = 0x98,
	DF1 = 0x99,
	DF2 = 0x9A,
	DF3 = 0x9B,
	DF4 = 0x9C,
	DF5 = 0x9D,
	DF6 = 0x9E,
	DF7 = 0x9F
};

struct DTVCC_S_COMMANDS_C1
{
	int code;
	const char *name;
	const char *description;
	int length;
};


enum dtvcc_eWindowsAttribJustify
{
	left	= 0,
	right	= 1,
	center	= 2,
	full	= 3
};

enum dtvcc_eWindowsAttribPrintDirection
{
	pd_left_to_right = 0,
	pd_right_to_left = 1,
	pd_top_to_bottom = 2,
	pd_bottom_to_top = 3
};

enum dtvcc_eWindowsAttribScrollDirection
{
	sd_left_to_right = 0,
	sd_right_to_left = 1,
	sd_top_to_bottom = 2,
	sd_bottom_to_top = 3
};

enum dtvcc_eWindowsAttribScrollDisplayEffect
{
	snap = 0,
	fade = 1,
	wipe = 2
};

enum dtvcc_eWindowsAttribEffectDirection
{
	left_to_right = 0,
	right_to_left = 1,
	top_to_bottom = 2,
	bottom_to_top = 3
};

enum dtvcc_eWindowsAttribFillOpacity
{
	solid 			= 0,
	flash 			= 1,
	traslucent 		= 2,
	transparent 	= 3
};

enum dtvcc_eWindowsAttribBorderType
{
	none			= 0,
	raised			= 1,
	depressed		= 2,
	uniform			= 3,
	shadow_left		= 4,
	shadow_right	= 5
};

enum dtvcc_ePenAttribSize
{
	pensize_small		= 0,
	pensize_standard	= 1,
	pensize_large 		= 2
};

enum dtvcc_ePenAttribFontStyle
{
	fontstyle_default_or_undefined			= 0,
	monospaced_with_serifs					= 1,
	proportionally_spaced_with_serifs		= 2,
	monospaced_without_serifs				= 3,
	proportionally_spaced_without_serifs	= 4,
	casual_font_type						= 5,
	cursive_font_type						= 6,
	small_capitals							= 7
};

enum dtvcc_ePanAttribTextTag
{
	texttag_dialog						= 0,
	texttag_source_or_speaker_id		= 1,
	texttag_electronic_voice			= 2,
	texttag_foreign_language			= 3,
	texttag_voiceover					= 4,
	texttag_audible_translation			= 5,
	texttag_subtitle_translation		= 6,
	texttag_voice_quality_description	= 7,
	texttag_song_lyrics					= 8,
	texttag_sound_effect_description	= 9,
	texttag_musical_score_description	= 10,
	texttag_expletitive					= 11,
	texttag_undefined_12				= 12,
	texttag_undefined_13				= 13,
	texttag_undefined_14				= 14,
	texttag_not_to_be_displayed			= 15
};

enum dtvcc_ePanAttribOffset
{
	offset_subscript	= 0,
	offset_normal		= 1,
	offset_superscript	= 2
};

enum dtvcc_ePanAttribEdgeType
{
	edgetype_none				= 0,
	edgetype_raised				= 1,
	edgetype_depressed			= 2,
	edgetype_uniform			= 3,
	edgetype_left_drop_shadow	= 4,
	edgetype_right_drop_shadow	= 5
};

enum dtvcc_eAnchorPoints
{
	anchorpoint_top_left 		= 0,
	anchorpoint_top_center 		= 1,
	anchorpoint_top_right 		= 2,
	anchorpoint_middle_left 	= 3,
	anchorpoint_middle_center 	= 4,
	anchorpoint_middle_right 	= 5,
	anchorpoint_bottom_left 	= 6,
	anchorpoint_bottom_center 	= 7,
	anchorpoint_bottom_right 	= 8
};

typedef struct dtvcc_pen_color
{
	int fg_color;
	int fg_opacity;
	int bg_color;
	int bg_opacity;
	int edge_color;
} dtvcc_pen_color;

typedef struct dtvcc_pen_attribs
{
	int pen_size;
	int offset;
	int text_tag;
	int font_tag;
	int edge_type;
	int underline;
	int italic;
} dtvcc_pen_attribs;

typedef struct dtvcc_window_attribs
{
	int fill_color;
	int fill_opacity;
	int border_color;
	int border_type01;
	int justify;
	int scroll_dir;
	int print_dir;
	int word_wrap;
	int border_type;
	int display_eff;
	int effect_dir;
	int effect_speed;
} dtvcc_window_attribs;

/**
 * Since 1-byte and 2-byte symbols could appear in captions and
 * since we have to keep symbols alignment and several windows could appear on a screen at one time,
 * we use special structure for holding symbols
 */
typedef struct ccx_dtvcc_symbol_t
{
	unsigned short sym; //symbol itself, at least 16 bit
	unsigned char len; //length. could be 1 or 2
} ccx_dtvcc_symbol_t;

#define CCX_DTVCC_SYM_SET(x, c) {x.len = 1; x.sym = c;}
#define CCX_DTVCC_SYM_SET_16(x, c1, c2) {x.len = 2; x.sym = (c1 << 8) | c2;}
#define CCX_DTVCC_SYM_IS_16(x) (x.len == 2)
#define CCX_DTVCC_SYM(x) ((unsigned char)(x.sym))
#define CCX_DTVCC_SYM_16_FIRST(x) ((unsigned char)(x.sym >> 8))
#define CCX_DTVCC_SYM_16_SECOND(x) ((unsigned char)(x.sym & 0xff))
#define CCX_DTVCC_SYM_IS_EMPTY(x) (x.len == 0)
#define CCX_DTVCC_SYM_IS_SET(x) (x.len > 0)

typedef struct dtvcc_window
{
	int is_defined;
	int number;
	int priority;
	int col_lock;
	int row_lock;
	int visible;
	int anchor_vertical;
	int relative_pos;
	int anchor_horizontal;
	int row_count;
	int anchor_point;
	int col_count;
	int pen_style;
	int win_style;
	unsigned char commands[6]; // Commands used to create this window
	dtvcc_window_attribs attribs;
	int pen_row;
	int pen_column;
	ccx_dtvcc_symbol_t *rows[DTVCC_MAX_ROWS];
	dtvcc_pen_color pen_colors[DTVCC_MAX_ROWS];
	dtvcc_pen_attribs pen_attribs[DTVCC_MAX_ROWS];
	int memory_reserved;
	int is_empty;
	LLONG time_ms_show;
	LLONG time_ms_hide;
} dtvcc_window;

typedef struct dtvcc_tv_screen
{
	ccx_dtvcc_symbol_t chars[DTVCC_SCREENGRID_ROWS][DTVCC_SCREENGRID_COLUMNS];
	dtvcc_pen_color pen_colors[DTVCC_SCREENGRID_ROWS];
	dtvcc_pen_attribs pen_attribs[DTVCC_SCREENGRID_ROWS];
	LLONG time_ms_show;
	LLONG time_ms_hide;
	unsigned int cc_count;
	int service_number;
} dtvcc_tv_screen;

/**
 * Holds data on the CEA 708 services that are encountered during file parse
 * This can be interesting, so CCExtractor uses it for the report functionality.
 */
typedef struct ccx_decoder_dtvcc_report_t
{
	int reset_count;
	unsigned services[DTVCC_MAX_SERVICES];
} ccx_decoder_dtvcc_report_t;

typedef struct dtvcc_service_decoder
{
	dtvcc_window windows[DTVCC_MAX_WINDOWS];
	int current_window;
	dtvcc_tv_screen *tv;
	int cc_count;
} dtvcc_service_decoder;

typedef struct ccx_decoder_dtvcc_settings_t
{
	int enabled;
	int print_file_reports;
	int no_rollup;
	ccx_decoder_dtvcc_report_t *report;

	int active_services_count;
	int services_enabled[DTVCC_MAX_SERVICES];
} ccx_decoder_dtvcc_settings_t;

/**
 * TODO
 * solution requires "sink" or "writer" entity to write captions to output file
 * decoders have to know nothing about output files
 */

typedef struct ccx_dtvcc_ctx_t
{
	int is_active; //processing CEA-708
	int active_services_count;
	int services_active[DTVCC_MAX_SERVICES]; //0 - inactive, 1 - active
	int report_enabled;

	ccx_decoder_dtvcc_report_t *report;

	dtvcc_service_decoder decoders[DTVCC_MAX_SERVICES];

	unsigned char current_packet[DTVCC_MAX_PACKET_LENGTH];
	int current_packet_length;

	int last_sequence;

	void *encoder; //we can't include header, so keeping it this way
	int no_rollup;
} ccx_dtvcc_ctx_t;


void _dtvcc_clear_packet(ccx_dtvcc_ctx_t *ctx);
void _dtvcc_windows_reset(dtvcc_service_decoder *decoder);
void dtvcc_decoder_flush(ccx_dtvcc_ctx_t *dtvcc, dtvcc_service_decoder *decoder);

void dtvcc_process_current_packet(ccx_dtvcc_ctx_t *dtvcc);
void dtvcc_process_service_block(ccx_dtvcc_ctx_t *dtvcc,
								 dtvcc_service_decoder *decoder,
								 unsigned char *data,
								 int data_length);

#endif
