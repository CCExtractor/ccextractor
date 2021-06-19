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

enum DTVCC_COMMANDS_C0_CODES
{
	DTVCC_C0_NUL = 	0x00,
	DTVCC_C0_ETX = 	0x03,
	DTVCC_C0_BS = 	0x08,
	DTVCC_C0_FF = 	0x0c,
	DTVCC_C0_CR = 	0x0d,
	DTVCC_C0_HCR = 	0x0e,
	DTVCC_C0_EXT1 = 0x10,
	DTVCC_C0_P16 = 	0x18
};

enum DTVCC_COMMANDS_C1_CODES
{
	DTVCC_C1_CW0 = 0x80,
	DTVCC_C1_CW1 = 0x81,
	DTVCC_C1_CW2 = 0x82,
	DTVCC_C1_CW3 = 0x83,
	DTVCC_C1_CW4 = 0x84,
	DTVCC_C1_CW5 = 0x85,
	DTVCC_C1_CW6 = 0x86,
	DTVCC_C1_CW7 = 0x87,
	DTVCC_C1_CLW = 0x88,
	DTVCC_C1_DSW = 0x89,
	DTVCC_C1_HDW = 0x8A,
	DTVCC_C1_TGW = 0x8B,
	DTVCC_C1_DLW = 0x8C,
	DTVCC_C1_DLY = 0x8D,
	DTVCC_C1_DLC = 0x8E,
	DTVCC_C1_RST = 0x8F,
	DTVCC_C1_SPA = 0x90,
	DTVCC_C1_SPC = 0x91,
	DTVCC_C1_SPL = 0x92,
	DTVCC_C1_RSV93 = 0x93,
	DTVCC_C1_RSV94 = 0x94,
	DTVCC_C1_RSV95 = 0x95,
	DTVCC_C1_RSV96 = 0x96,
	DTVCC_C1_SWA = 0x97,
	DTVCC_C1_DF0 = 0x98,
	DTVCC_C1_DF1 = 0x99,
	DTVCC_C1_DF2 = 0x9A,
	DTVCC_C1_DF3 = 0x9B,
	DTVCC_C1_DF4 = 0x9C,
	DTVCC_C1_DF5 = 0x9D,
	DTVCC_C1_DF6 = 0x9E,
	DTVCC_C1_DF7 = 0x9F
};

struct DTVCC_S_COMMANDS_C1
{
	int code;
	const char *name;
	const char *description;
	int length;
};

enum dtvcc_window_justify
{
	DTVCC_WINDOW_JUSTIFY_LEFT	= 0,
	DTVCC_WINDOW_JUSTIFY_RIGHT	= 1,
	DTVCC_WINDOW_JUSTIFY_CENTER	= 2,
	DTVCC_WINDOW_JUSTIFY_FULL	= 3
};

enum dtvcc_window_pd //Print Direction
{
	DTVCC_WINDOW_PD_LEFT_RIGHT = 0, //left -> right
	DTVCC_WINDOW_PD_RIGHT_LEFT = 1,
	DTVCC_WINDOW_PD_TOP_BOTTOM = 2,
	DTVCC_WINDOW_PD_BOTTOM_TOP = 3
};

enum dtvcc_window_sd //Scroll Direction
{
	DTVCC_WINDOW_SD_LEFT_RIGHT = 0,
	DTVCC_WINDOW_SD_RIGHT_LEFT = 1,
	DTVCC_WINDOW_SD_TOP_BOTTOM = 2,
	DTVCC_WINDOW_SD_BOTTOM_TOP = 3
};

enum dtvcc_window_sde //Scroll Display Effect
{
	DTVCC_WINDOW_SDE_SNAP = 0,
	DTVCC_WINDOW_SDE_FADE = 1,
	DTVCC_WINDOW_SDE_WIPE = 2
};

enum dtvcc_window_ed //Effect Direction
{
	DTVCC_WINDOW_ED_LEFT_RIGHT = 0,
	DTVCC_WINDOW_ED_RIGHT_LEFT = 1,
	DTVCC_WINDOW_ED_TOP_BOTTOM = 2,
	DTVCC_WINDOW_ED_BOTTOM_TOP = 3
};

enum dtvcc_window_fo //Fill Opacity
{
	DTVCC_WINDOW_FO_SOLID		= 0,
	DTVCC_WINDOW_FO_FLASH		= 1,
	DTVCC_WINDOW_FO_TRANSLUCENT	= 2,
	DTVCC_WINDOW_FO_TRANSPARENT = 3
};

enum dtvcc_window_border
{
	DTVCC_WINDOW_BORDER_NONE			= 0,
	DTVCC_WINDOW_BORDER_RAISED			= 1,
	DTVCC_WINDOW_BORDER_DEPRESSED		= 2,
	DTVCC_WINDOW_BORDER_UNIFORM			= 3,
	DTVCC_WINDOW_BORDER_SHADOW_LEFT		= 4,
	DTVCC_WINDOW_BORDER_SHADOW_RIGHT	= 5
};

enum dtvcc_pen_size
{
	DTVCC_PEN_SIZE_SMALL 	= 0,
	DTVCC_PEN_SIZE_STANDART = 1,
	DTVCC_PEN_SIZE_LARGE	= 2
};

enum dtvcc_pen_font_style
{
	DTVCC_PEN_FONT_STYLE_DEFAULT_OR_UNDEFINED					= 0,
	DTVCC_PEN_FONT_STYLE_MONOSPACED_WITH_SERIFS					= 1,
	DTVCC_PEN_FONT_STYLE_PROPORTIONALLY_SPACED_WITH_SERIFS		= 2,
	DTVCC_PEN_FONT_STYLE_MONOSPACED_WITHOUT_SERIFS				= 3,
	DTVCC_PEN_FONT_STYLE_PROPORTIONALLY_SPACED_WITHOUT_SERIFS	= 4,
	DTVCC_PEN_FONT_STYLE_CASUAL_FONT_TYPE						= 5,
	DTVCC_PEN_FONT_STYLE_CURSIVE_FONT_TYPE						= 6,
	DTVCC_PEN_FONT_STYLE_SMALL_CAPITALS							= 7
};

enum dtvcc_pen_text_tag
{
	DTVCC_PEN_TEXT_TAG_DIALOG						= 0,
	DTVCC_PEN_TEXT_TAG_SOURCE_OR_SPEAKER_ID			= 1,
	DTVCC_PEN_TEXT_TAG_ELECTRONIC_VOICE				= 2,
	DTVCC_PEN_TEXT_TAG_FOREIGN_LANGUAGE				= 3,
	DTVCC_PEN_TEXT_TAG_VOICEOVER					= 4,
	DTVCC_PEN_TEXT_TAG_AUDIBLE_TRANSLATION			= 5,
	DTVCC_PEN_TEXT_TAG_SUBTITLE_TRANSLATION			= 6,
	DTVCC_PEN_TEXT_TAG_VOICE_QUALITY_DESCRIPTION	= 7,
	DTVCC_PEN_TEXT_TAG_SONG_LYRICS					= 8,
	DTVCC_PEN_TEXT_TAG_SOUND_EFFECT_DESCRIPTION		= 9,
	DTVCC_PEN_TEXT_TAG_MUSICAL_SCORE_DESCRIPTION	= 10,
	DTVCC_PEN_TEXT_TAG_EXPLETIVE					= 11,
	DTVCC_PEN_TEXT_TAG_UNDEFINED_12					= 12,
	DTVCC_PEN_TEXT_TAG_UNDEFINED_13					= 13,
	DTVCC_PEN_TEXT_TAG_UNDEFINED_14					= 14,
	DTVCC_PEN_TEXT_TAG_NOT_TO_BE_DISPLAYED			= 15
};

enum dtvcc_pen_offset
{
	DTVCC_PEN_OFFSET_SUBSCRIPT		= 0,
	DTVCC_PEN_OFFSET_NORMAL			= 1,
	DTVCC_PEN_OFFSET_SUPERSCRIPT	= 2
};

enum dtvcc_pen_edge
{
	DTVCC_PEN_EDGE_NONE					= 0,
	DTVCC_PEN_EDGE_RAISED				= 1,
	DTVCC_PEN_EDGE_DEPRESSED			= 2,
	DTVCC_PEN_EDGE_UNIFORM				= 3,
	DTVCC_PEN_EDGE_LEFT_DROP_SHADOW		= 4,
	DTVCC_PEN_EDGE_RIGHT_DROP_SHADOW	= 5
};

enum dtvcc_pen_anchor_point
{
	DTVCC_ANCHOR_POINT_TOP_LEFT 		= 0,
	DTVCC_ANCHOR_POINT_TOP_CENTER 		= 1,
	DTVCC_ANCHOR_POINT_TOP_RIGHT 		= 2,
	DTVCC_ANCHOR_POINT_MIDDLE_LEFT 		= 3,
	DTVCC_ANCHOR_POINT_MIDDLE_CENTER 	= 4,
	DTVCC_ANCHOR_POINT_MIDDLE_RIGHT 	= 5,
	DTVCC_ANCHOR_POINT_BOTTOM_LEFT 		= 6,
	DTVCC_ANCHOR_POINT_BOTTOM_CENTER 	= 7,
	DTVCC_ANCHOR_POINT_BOTTOM_RIGHT 	= 8
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
} dtvcc_window_attribs;

/**
 * Since 1-byte and 2-byte symbols could appear in captions and
 * since we have to keep symbols alignment and several windows could appear on a screen at one time,
 * we use special structure for holding symbols
 */
typedef struct dtvcc_symbol
{
	unsigned short sym; //symbol itself, at least 16 bit
	unsigned char init; //initialized or not. could be 0 or 1
} dtvcc_symbol;

#define CCX_DTVCC_SYM_SET(x, c) {x.init = 1; x.sym = c;}
#define CCX_DTVCC_SYM_SET_16(x, c1, c2) {x.init = 1; x.sym = (c1 << 8) | c2;}
#define CCX_DTVCC_SYM(x) ((unsigned char)(x.sym))
#define CCX_DTVCC_SYM_IS_EMPTY(x) (x.init == 0)
#define CCX_DTVCC_SYM_IS_SET(x) (x.init == 1)

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
	dtvcc_symbol *rows[CCX_DTVCC_MAX_ROWS];
	dtvcc_pen_color pen_colors[CCX_DTVCC_MAX_ROWS][CCX_DTVCC_SCREENGRID_COLUMNS];
	dtvcc_pen_attribs pen_attribs[CCX_DTVCC_MAX_ROWS][CCX_DTVCC_SCREENGRID_COLUMNS];
	dtvcc_pen_color pen_color_pattern;
	dtvcc_pen_attribs pen_attribs_pattern;
	int memory_reserved;
	int is_empty;
	LLONG time_ms_show;
	LLONG time_ms_hide;
} dtvcc_window;

typedef struct dtvcc_tv_screen
{
	dtvcc_symbol chars[CCX_DTVCC_SCREENGRID_ROWS][CCX_DTVCC_SCREENGRID_COLUMNS];
	dtvcc_pen_color pen_colors[CCX_DTVCC_SCREENGRID_ROWS][CCX_DTVCC_SCREENGRID_COLUMNS];
	dtvcc_pen_attribs pen_attribs[CCX_DTVCC_SCREENGRID_ROWS][CCX_DTVCC_SCREENGRID_COLUMNS];
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

typedef struct dtvcc_service_decoder
{
	dtvcc_window windows[CCX_DTVCC_MAX_WINDOWS];
	int current_window;
	dtvcc_tv_screen *tv;
	int cc_count;
} dtvcc_service_decoder;

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

typedef struct dtvcc_ctx
{
	int is_active;
	int active_services_count;
	int services_active[CCX_DTVCC_MAX_SERVICES]; //0 - inactive, 1 - active
	int report_enabled;

	ccx_decoder_dtvcc_report *report;

	dtvcc_service_decoder decoders[CCX_DTVCC_MAX_SERVICES];

	unsigned char current_packet[CCX_DTVCC_MAX_PACKET_LENGTH];
	int current_packet_length;
	int is_current_packet_header_parsed;

	int last_sequence;

	void *encoder; //we can't include header, so keeping it this way
	int no_rollup;
	struct ccx_common_timing_ctx *timing;
} dtvcc_ctx;


void dtvcc_clear_packet(dtvcc_ctx *ctx);
void dtvcc_windows_reset(dtvcc_service_decoder *decoder);
void dtvcc_decoder_flush(dtvcc_ctx *dtvcc, dtvcc_service_decoder *decoder);

void dtvcc_process_current_packet(dtvcc_ctx *dtvcc, int len);
void dtvcc_process_service_block(dtvcc_ctx *dtvcc,
									 dtvcc_service_decoder *decoder,
									 unsigned char *data,
									 int data_length);

void dtvcc_tv_clear(dtvcc_service_decoder *decoder);
int dtvcc_decoder_has_visible_windows(dtvcc_service_decoder *decoder);
void dtvcc_window_clear_row(dtvcc_window *window, int row_index);
void dtvcc_window_clear_text(dtvcc_window *window);
void dtvcc_window_clear(dtvcc_service_decoder *decoder, int window_id);
void dtvcc_window_apply_style(dtvcc_window *window, dtvcc_window_attribs *style);

#ifdef DTVCC_PRINT_DEBUG
	int dtvcc_is_win_row_empty(dtvcc_window *window, int row_index);
	void dtvcc_get_win_write_interval(dtvcc_window *window, int row_index, int *first, int *last);
	void dtvcc_window_dump(dtvcc_service_decoder *decoder, dtvcc_window *window);
#endif

void dtvcc_decoders_reset(dtvcc_ctx *dtvcc);
int dtvcc_compare_win_priorities(const void *a, const void *b);
void dtvcc_window_update_time_show(dtvcc_window *window, struct ccx_common_timing_ctx *timing);
void dtvcc_window_update_time_hide(dtvcc_window *window, struct ccx_common_timing_ctx *timing);
void dtvcc_screen_update_time_show(dtvcc_tv_screen *tv, LLONG time);
void dtvcc_screen_update_time_hide(dtvcc_tv_screen *tv, LLONG time);
void dtvcc_get_window_dimensions(dtvcc_window *window, int *x1, int *x2, int *y1, int *y2);
int dtvcc_is_window_overlapping(dtvcc_service_decoder *decoder, dtvcc_window *window);
void dtvcc_window_copy_to_screen(dtvcc_service_decoder *decoder, dtvcc_window *window);
void dtvcc_screen_print(dtvcc_ctx *dtvcc, dtvcc_service_decoder *decoder);
void dtvcc_process_hcr(dtvcc_service_decoder *decoder);
void dtvcc_process_ff(dtvcc_service_decoder *decoder);
void dtvcc_process_etx(dtvcc_service_decoder *decoder);
void dtvcc_process_bs(dtvcc_service_decoder *decoder);
void dtvcc_window_rollup(dtvcc_service_decoder *decoder, dtvcc_window *window);
void dtvcc_process_cr(dtvcc_ctx *dtvcc, dtvcc_service_decoder *decoder);
void dtvcc_process_character(dtvcc_service_decoder *decoder, dtvcc_symbol symbol);
void dtvcc_handle_CWx_SetCurrentWindow(dtvcc_service_decoder *decoder, int window_id);
void dtvcc_handle_CLW_ClearWindows(dtvcc_ctx *dtvcc, dtvcc_service_decoder *decoder, int windows_bitmap);
void dtvcc_handle_DSW_DisplayWindows(dtvcc_service_decoder *decoder, int windows_bitmap, struct ccx_common_timing_ctx *timing);
void dtvcc_handle_HDW_HideWindows(dtvcc_ctx *dtvcc,dtvcc_service_decoder *decoder,
				  int windows_bitmap);
void dtvcc_handle_TGW_ToggleWindows(dtvcc_ctx *dtvcc,
				    dtvcc_service_decoder *decoder,
				    int windows_bitmap);
void dtvcc_handle_DFx_DefineWindow(dtvcc_service_decoder *decoder, int window_id, unsigned char *data, struct ccx_common_timing_ctx *timing);
void dtvcc_handle_SWA_SetWindowAttributes(dtvcc_service_decoder *decoder, unsigned char *data);
void dtvcc_handle_DLW_DeleteWindows(dtvcc_ctx *dtvcc,
				    dtvcc_service_decoder *decoder,
				    int windows_bitmap);
void dtvcc_handle_SPA_SetPenAttributes(dtvcc_service_decoder *decoder, unsigned char *data);
void dtvcc_handle_SPC_SetPenColor(dtvcc_service_decoder *decoder, unsigned char *data);
void dtvcc_handle_SPL_SetPenLocation(dtvcc_service_decoder *decoder, unsigned char *data);
void dtvcc_handle_RST_Reset(dtvcc_service_decoder *decoder);
void dtvcc_handle_DLY_Delay(dtvcc_service_decoder *decoder, int tenths_of_sec);
void dtvcc_handle_DLC_DelayCancel(dtvcc_service_decoder *decoder);
int dtvcc_handle_C0_P16(dtvcc_service_decoder *decoder, unsigned char *data);
int dtvcc_handle_G0(dtvcc_service_decoder *decoder, unsigned char *data, int data_length);
int dtvcc_handle_G1(dtvcc_service_decoder *decoder, unsigned char *data, int data_length);
int dtvcc_handle_C0(dtvcc_ctx *dtvcc,
		     dtvcc_service_decoder *decoder,
		     unsigned char *data,
		     int data_length);
int dtvcc_handle_C1(dtvcc_ctx *dtvcc,
		     dtvcc_service_decoder *decoder,
		     unsigned char *data,
		     int data_length);
int dtvcc_handle_C2(dtvcc_service_decoder *decoder, unsigned char *data, int data_length);
int dtvcc_handle_C3(dtvcc_service_decoder *decoder, unsigned char *data, int data_length);
int dtvcc_handle_extended_char(dtvcc_service_decoder *decoder, unsigned char *data, int data_length);

extern dtvcc_pen_color dtvcc_default_pen_color;
extern dtvcc_pen_attribs dtvcc_default_pen_attribs;

#endif
