#ifndef _INCLUDE_708_
#define _INCLUDE_708_

#include <sys/stat.h>
#include "ccx_common_platform.h"
#include "ccx_common_constants.h"
#include "ccx_common_structs.h"

#define CCX_DTVCC_MAX_PACKET_LENGTH 128 //According to EIA-708B, part 5
#define CCX_DTVCC_MAX_SERVICES 63

#define CCX_DTVCC_MAX_ROWS 15
/**
 * This value should be 32, but there were 16-bit encoded samples (from Korea),
 * where RowCount calculated another way and equals 46 (23[8bit]*2)
 */
#define CCX_DTVCC_MAX_COLUMNS (32*2)

#define CCX_DTVCC_SCREENGRID_ROWS 75
#define CCX_DTVCC_SCREENGRID_COLUMNS 210

#define CCX_DTVCC_MAX_WINDOWS 8
#define NOT_OVERLAPPED 0
#define OVERLAPPING_WITH_HIGH_PRIORITY 0
#define OVERLAPPED_BY_HIGH_PRIORITY 1

#define CCX_DTVCC_FILENAME_TEMPLATE ".p%u.svc%02u"

#define CCX_DTVCC_NO_LAST_SEQUENCE -1

enum CCX_DTVCC_COMMANDS_C0_CODES
{
	CCX_DTVCC_C0_NUL = 	0x00,
	CCX_DTVCC_C0_ETX = 	0x03,
	CCX_DTVCC_C0_BS = 	0x08,
	CCX_DTVCC_C0_FF = 	0x0c,
	CCX_DTVCC_C0_CR = 	0x0d,
	CCX_DTVCC_C0_HCR = 	0x0e,
	CCX_DTVCC_C0_EXT1 = 0x10,
	CCX_DTVCC_C0_P16 = 	0x18
};

enum CCX_DTVCC_COMMANDS_C1_CODES
{
	CCX_DTVCC_C1_CW0 = 0x80,
	CCX_DTVCC_C1_CW1 = 0x81,
	CCX_DTVCC_C1_CW2 = 0x82,
	CCX_DTVCC_C1_CW3 = 0x83,
	CCX_DTVCC_C1_CW4 = 0x84,
	CCX_DTVCC_C1_CW5 = 0x85,
	CCX_DTVCC_C1_CW6 = 0x86,
	CCX_DTVCC_C1_CW7 = 0x87,
	CCX_DTVCC_C1_CLW = 0x88,
	CCX_DTVCC_C1_DSW = 0x89,
	CCX_DTVCC_C1_HDW = 0x8A,
	CCX_DTVCC_C1_TGW = 0x8B,
	CCX_DTVCC_C1_DLW = 0x8C,
	CCX_DTVCC_C1_DLY = 0x8D,
	CCX_DTVCC_C1_DLC = 0x8E,
	CCX_DTVCC_C1_RST = 0x8F,
	CCX_DTVCC_C1_SPA = 0x90,
	CCX_DTVCC_C1_SPC = 0x91,
	CCX_DTVCC_C1_SPL = 0x92,
	CCX_DTVCC_C1_RSV93 = 0x93,
	CCX_DTVCC_C1_RSV94 = 0x94,
	CCX_DTVCC_C1_RSV95 = 0x95,
	CCX_DTVCC_C1_RSV96 = 0x96,
	CCX_DTVCC_C1_SWA = 0x97,
	CCX_DTVCC_C1_DF0 = 0x98,
	CCX_DTVCC_C1_DF1 = 0x99,
	CCX_DTVCC_C1_DF2 = 0x9A,
	CCX_DTVCC_C1_DF3 = 0x9B,
	CCX_DTVCC_C1_DF4 = 0x9C,
	CCX_DTVCC_C1_DF5 = 0x9D,
	CCX_DTVCC_C1_DF6 = 0x9E,
	CCX_DTVCC_C1_DF7 = 0x9F
};

struct CCX_DTVCC_S_COMMANDS_C1
{
	int code;
	const char *name;
	const char *description;
	int length;
};

enum ccx_dtvcc_window_justify
{
	CCX_DTVCC_WINDOW_JUSTIFY_LEFT	= 0,
	CCX_DTVCC_WINDOW_JUSTIFY_RIGHT	= 1,
	CCX_DTVCC_WINDOW_JUSTIFY_CENTER	= 2,
	CCX_DTVCC_WINDOW_JUSTIFY_FULL	= 3
};

enum ccx_dtvcc_window_pd //Print Direction
{
	CCX_DTVCC_WINDOW_PD_LEFT_RIGHT = 0, //left -> right
	CCX_DTVCC_WINDOW_PD_RIGHT_LEFT = 1,
	CCX_DTVCC_WINDOW_PD_TOP_BOTTOM = 2,
	CCX_DTVCC_WINDOW_PD_BOTTOM_TOP = 3
};

enum ccx_dtvcc_window_sd //Scroll Direction
{
	CCX_DTVCC_WINDOW_SD_LEFT_RIGHT = 0,
	CCX_DTVCC_WINDOW_SD_RIGHT_LEFT = 1,
	CCX_DTVCC_WINDOW_SD_TOP_BOTTOM = 2,
	CCX_DTVCC_WINDOW_SD_BOTTOM_TOP = 3
};

enum ccx_dtvcc_window_sde //Scroll Display Effect
{
	CCX_DTVCC_WINDOW_SDE_SNAP = 0,
	CCX_DTVCC_WINDOW_SDE_FADE = 1,
	CCX_DTVCC_WINDOW_SDE_WIPE = 2
};

enum ccx_dtvcc_window_ed //Effect Direction
{
	CCX_DTVCC_WINDOW_ED_LEFT_RIGHT = 0,
	CCX_DTVCC_WINDOW_ED_RIGHT_LEFT = 1,
	CCX_DTVCC_WINDOW_ED_TOP_BOTTOM = 2,
	CCX_DTVCC_WINDOW_ED_BOTTOM_TOP = 3
};

enum ccx_dtvcc_window_fo //Fill Opacity
{
	CCX_DTVCC_WINDOW_FO_SOLID		= 0,
	CCX_DTVCC_WINDOW_FO_FLASH		= 1,
	CCX_DTVCC_WINDOW_FO_TRANSLUCENT	= 2,
	CCX_DTVCC_WINDOW_FO_TRANSPARENT = 3
};

enum ccx_dtvcc_window_border
{
	CCX_DTVCC_WINDOW_BORDER_NONE			= 0,
	CCX_DTVCC_WINDOW_BORDER_RAISED			= 1,
	CCX_DTVCC_WINDOW_BORDER_DEPRESSED		= 2,
	CCX_DTVCC_WINDOW_BORDER_UNIFORM			= 3,
	CCX_DTVCC_WINDOW_BORDER_SHADOW_LEFT		= 4,
	CCX_DTVCC_WINDOW_BORDER_SHADOW_RIGHT	= 5
};

enum ccx_dtvcc_pen_size
{
	CCX_DTVCC_PEN_SIZE_SMALL 	= 0,
	CCX_DTVCC_PEN_SIZE_STANDART = 1,
	CCX_DTVCC_PEN_SIZE_LARGE	= 2
};

enum ccx_dtvcc_pen_font_style
{
	CCX_DTVCC_PEN_FONT_STYLE_DEFAULT_OR_UNDEFINED					= 0,
	CCX_DTVCC_PEN_FONT_STYLE_MONOSPACED_WITH_SERIFS					= 1,
	CCX_DTVCC_PEN_FONT_STYLE_PROPORTIONALLY_SPACED_WITH_SERIFS		= 2,
	CCX_DTVCC_PEN_FONT_STYLE_MONOSPACED_WITHOUT_SERIFS				= 3,
	CCX_DTVCC_PEN_FONT_STYLE_PROPORTIONALLY_SPACED_WITHOUT_SERIFS	= 4,
	CCX_DTVCC_PEN_FONT_STYLE_CASUAL_FONT_TYPE						= 5,
	CCX_DTVCC_PEN_FONT_STYLE_CURSIVE_FONT_TYPE						= 6,
	CCX_DTVCC_PEN_FONT_STYLE_SMALL_CAPITALS							= 7
};

enum ccx_dtvcc_pen_text_tag
{
	CCX_DTVCC_PEN_TEXT_TAG_DIALOG						= 0,
	CCX_DTVCC_PEN_TEXT_TAG_SOURCE_OR_SPEAKER_ID			= 1,
	CCX_DTVCC_PEN_TEXT_TAG_ELECTRONIC_VOICE				= 2,
	CCX_DTVCC_PEN_TEXT_TAG_FOREIGN_LANGUAGE				= 3,
	CCX_DTVCC_PEN_TEXT_TAG_VOICEOVER					= 4,
	CCX_DTVCC_PEN_TEXT_TAG_AUDIBLE_TRANSLATION			= 5,
	CCX_DTVCC_PEN_TEXT_TAG_SUBTITLE_TRANSLATION			= 6,
	CCX_DTVCC_PEN_TEXT_TAG_VOICE_QUALITY_DESCRIPTION	= 7,
	CCX_DTVCC_PEN_TEXT_TAG_SONG_LYRICS					= 8,
	CCX_DTVCC_PEN_TEXT_TAG_SOUND_EFFECT_DESCRIPTION		= 9,
	CCX_DTVCC_PEN_TEXT_TAG_MUSICAL_SCORE_DESCRIPTION	= 10,
	CCX_DTVCC_PEN_TEXT_TAG_EXPLETIVE					= 11,
	CCX_DTVCC_PEN_TEXT_TAG_UNDEFINED_12					= 12,
	CCX_DTVCC_PEN_TEXT_TAG_UNDEFINED_13					= 13,
	CCX_DTVCC_PEN_TEXT_TAG_UNDEFINED_14					= 14,
	CCX_DTVCC_PEN_TEXT_TAG_NOT_TO_BE_DISPLAYED			= 15
};

enum ccx_dtvcc_pen_offset
{
	CCX_DTVCC_PEN_OFFSET_SUBSCRIPT		= 0,
	CCX_DTVCC_PEN_OFFSET_NORMAL			= 1,
	CCX_DTVCC_PEN_OFFSET_SUPERSCRIPT	= 2
};

enum ccx_dtvcc_pen_edge
{
	CCX_DTVCC_PEN_EDGE_NONE					= 0,
	CCX_DTVCC_PEN_EDGE_RAISED				= 1,
	CCX_DTVCC_PEN_EDGE_DEPRESSED			= 2,
	CCX_DTVCC_PEN_EDGE_UNIFORM				= 3,
	CCX_DTVCC_PEN_EDGE_LEFT_DROP_SHADOW		= 4,
	CCX_DTVCC_PEN_EDGE_RIGHT_DROP_SHADOW	= 5
};

enum ccx_dtvcc_pen_anchor_point
{
	CCX_DTVCC_ANCHOR_POINT_TOP_LEFT 		= 0,
	CCX_DTVCC_ANCHOR_POINT_TOP_CENTER 		= 1,
	CCX_DTVCC_ANCHOR_POINT_TOP_RIGHT 		= 2,
	CCX_DTVCC_ANCHOR_POINT_MIDDLE_LEFT 		= 3,
	CCX_DTVCC_ANCHOR_POINT_MIDDLE_CENTER 	= 4,
	CCX_DTVCC_ANCHOR_POINT_MIDDLE_RIGHT 	= 5,
	CCX_DTVCC_ANCHOR_POINT_BOTTOM_LEFT 		= 6,
	CCX_DTVCC_ANCHOR_POINT_BOTTOM_CENTER 	= 7,
	CCX_DTVCC_ANCHOR_POINT_BOTTOM_RIGHT 	= 8
};

typedef struct ccx_dtvcc_pen_color
{
	int fg_color;
	int fg_opacity;
	int bg_color;
	int bg_opacity;
	int edge_color;
} ccx_dtvcc_pen_color;

typedef struct ccx_dtvcc_pen_attribs
{
	int pen_size;
	int offset;
	int text_tag;
	int font_tag;
	int edge_type;
	int underline;
	int italic;
} ccx_dtvcc_pen_attribs;

typedef struct ccx_dtvcc_window_attribs
{
	int justify;
	int print_direction;
	int scroll_direction;
	int word_wrap;
	int display_effect;
	int effect_direction;
	int effect_speed;
	int fill_color;
	int fill_opacity;
	int border_type;
	int border_color;
} ccx_dtvcc_window_attribs;

/**
 * Since 1-byte and 2-byte symbols could appear in captions and
 * since we have to keep symbols alignment and several windows could appear on a screen at one time,
 * we use special structure for holding symbols
 */
typedef struct ccx_dtvcc_symbol
{
	unsigned short sym; //symbol itself, at least 16 bit
	unsigned char init; //initialized or not. could be 0 or 1
} ccx_dtvcc_symbol;

#define CCX_DTVCC_SYM_SET(x, c) {x.init = 1; x.sym = c;}
#define CCX_DTVCC_SYM_SET_16(x, c1, c2) {x.init = 1; x.sym = (c1 << 8) | c2;}
#define CCX_DTVCC_SYM(x) ((unsigned char)(x.sym))
#define CCX_DTVCC_SYM_IS_EMPTY(x) (x.init == 0)
#define CCX_DTVCC_SYM_IS_SET(x) (x.init == 1)

typedef struct ccx_dtvcc_window
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
	ccx_dtvcc_window_attribs attribs;
	int pen_row;
	int pen_column;
	ccx_dtvcc_symbol *rows[CCX_DTVCC_MAX_ROWS];
	ccx_dtvcc_pen_color pen_colors[CCX_DTVCC_MAX_ROWS][CCX_DTVCC_SCREENGRID_COLUMNS];
	ccx_dtvcc_pen_attribs pen_attribs[CCX_DTVCC_MAX_ROWS][CCX_DTVCC_SCREENGRID_COLUMNS];
	ccx_dtvcc_pen_color pen_color_pattern;
	ccx_dtvcc_pen_attribs pen_attribs_pattern;
	int memory_reserved;
	int is_empty;
	LLONG time_ms_show;
	LLONG time_ms_hide;
} ccx_dtvcc_window;

typedef struct dtvcc_tv_screen
{
	ccx_dtvcc_symbol chars[CCX_DTVCC_SCREENGRID_ROWS][CCX_DTVCC_SCREENGRID_COLUMNS];
	ccx_dtvcc_pen_color pen_colors[CCX_DTVCC_SCREENGRID_ROWS][CCX_DTVCC_SCREENGRID_COLUMNS];
	ccx_dtvcc_pen_attribs pen_attribs[CCX_DTVCC_SCREENGRID_ROWS][CCX_DTVCC_SCREENGRID_COLUMNS];
	LLONG time_ms_show;
	LLONG time_ms_hide;
	unsigned int cc_count;
	int service_number;
} dtvcc_tv_screen;

/**
 * Holds data on the CEA 708 services that are encountered during file parse
 * This can be interesting, so CCExtractor uses it for the report functionality.
 */
typedef struct ccx_decoder_dtvcc_report
{
	int reset_count;
	unsigned services[CCX_DTVCC_MAX_SERVICES];
} ccx_decoder_dtvcc_report;

typedef struct ccx_dtvcc_service_decoder
{
	ccx_dtvcc_window windows[CCX_DTVCC_MAX_WINDOWS];
	int current_window;
	dtvcc_tv_screen *tv;
	int cc_count;
} ccx_dtvcc_service_decoder;

typedef struct ccx_decoder_dtvcc_settings
{
	int enabled;
	int print_file_reports;
	int no_rollup;
	ccx_decoder_dtvcc_report *report;
	int active_services_count;
	int services_enabled[CCX_DTVCC_MAX_SERVICES];
	struct ccx_common_timing_ctx *timing;
} ccx_decoder_dtvcc_settings;

/**
 * TODO
 * solution requires "sink" or "writer" entity to write captions to output file
 * decoders have to know nothing about output files
 */

typedef struct ccx_dtvcc_ctx
{
	int is_active;
	int active_services_count;
	int services_active[CCX_DTVCC_MAX_SERVICES]; //0 - inactive, 1 - active
	int report_enabled;

	ccx_decoder_dtvcc_report *report;

	ccx_dtvcc_service_decoder decoders[CCX_DTVCC_MAX_SERVICES];

	unsigned char current_packet[CCX_DTVCC_MAX_PACKET_LENGTH];
	int current_packet_length;

	int last_sequence;

	void *encoder; //we can't include header, so keeping it this way
	int no_rollup;
	struct ccx_common_timing_ctx *timing;
} ccx_dtvcc_ctx;


void ccx_dtvcc_clear_packet(ccx_dtvcc_ctx *ctx);
void ccx_dtvcc_windows_reset(ccx_dtvcc_service_decoder *decoder);
void ccx_dtvcc_decoder_flush(ccx_dtvcc_ctx *dtvcc, ccx_dtvcc_service_decoder *decoder);

void ccx_dtvcc_process_current_packet(ccx_dtvcc_ctx *dtvcc);
void ccx_dtvcc_process_service_block(ccx_dtvcc_ctx *dtvcc,
									 ccx_dtvcc_service_decoder *decoder,
									 unsigned char *data,
									 int data_length);

extern ccx_dtvcc_pen_color ccx_dtvcc_default_pen_color;
extern ccx_dtvcc_pen_attribs ccx_dtvcc_default_pen_attribs;

#endif
