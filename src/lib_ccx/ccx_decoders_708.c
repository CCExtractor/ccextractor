#include "ccx_decoders_708.h"
#include "ccx_decoders_708_encoding.h"
#include "ccx_common_common.h"
#include "ccx_common_constants.h"
#include "ccx_common_structs.h"
#include "ccx_common_timing.h"
#include "lib_ccx.h"
#include "utility.h"
#include "ccx_decoders_708_output.h"

/* Portions by Daniel Kristjansson, extracted from MythTV's source */

const char *DTVCC_COMMANDS_C0[32] =
    {
	"NUL",	// 0 = NUL
	NULL,	// 1 = Reserved
	NULL,	// 2 = Reserved
	"ETX",	// 3 = ETX
	NULL,	// 4 = Reserved
	NULL,	// 5 = Reserved
	NULL,	// 6 = Reserved
	NULL,	// 7 = Reserved
	"BS",	// 8 = Backspace
	NULL,	// 9 = Reserved
	NULL,	// A = Reserved
	NULL,	// B = Reserved
	"FF",	// C = FF
	"CR",	// D = CR
	"HCR",	// E = HCR
	NULL,	// F = Reserved
	"EXT1", // 0x10 = EXT1,
	NULL,	// 0x11 = Reserved
	NULL,	// 0x12 = Reserved
	NULL,	// 0x13 = Reserved
	NULL,	// 0x14 = Reserved
	NULL,	// 0x15 = Reserved
	NULL,	// 0x16 = Reserved
	NULL,	// 0x17 = Reserved
	"P16",	// 0x18 = P16
	NULL,	// 0x19 = Reserved
	NULL,	// 0x1A = Reserved
	NULL,	// 0x1B = Reserved
	NULL,	// 0x1C = Reserved
	NULL,	// 0x1D = Reserved
	NULL,	// 0x1E = Reserved
	NULL,	// 0x1F = Reserved
};

struct CCX_DTVCC_S_COMMANDS_C1 DTVCC_COMMANDS_C1[32] =
    {
	{CCX_DTVCC_C1_CW0, "CW0", "SetCurrentWindow0", 1},
	{CCX_DTVCC_C1_CW1, "CW1", "SetCurrentWindow1", 1},
	{CCX_DTVCC_C1_CW2, "CW2", "SetCurrentWindow2", 1},
	{CCX_DTVCC_C1_CW3, "CW3", "SetCurrentWindow3", 1},
	{CCX_DTVCC_C1_CW4, "CW4", "SetCurrentWindow4", 1},
	{CCX_DTVCC_C1_CW5, "CW5", "SetCurrentWindow5", 1},
	{CCX_DTVCC_C1_CW6, "CW6", "SetCurrentWindow6", 1},
	{CCX_DTVCC_C1_CW7, "CW7", "SetCurrentWindow7", 1},
	{CCX_DTVCC_C1_CLW, "CLW", "ClearWindows", 2},
	{CCX_DTVCC_C1_DSW, "DSW", "DisplayWindows", 2},
	{CCX_DTVCC_C1_HDW, "HDW", "HideWindows", 2},
	{CCX_DTVCC_C1_TGW, "TGW", "ToggleWindows", 2},
	{CCX_DTVCC_C1_DLW, "DLW", "DeleteWindows", 2},
	{CCX_DTVCC_C1_DLY, "DLY", "Delay", 2},
	{CCX_DTVCC_C1_DLC, "DLC", "DelayCancel", 1},
	{CCX_DTVCC_C1_RST, "RST", "Reset", 1},
	{CCX_DTVCC_C1_SPA, "SPA", "SetPenAttributes", 3},
	{CCX_DTVCC_C1_SPC, "SPC", "SetPenColor", 4},
	{CCX_DTVCC_C1_SPL, "SPL", "SetPenLocation", 3},
	{CCX_DTVCC_C1_RSV93, "RSV93", "Reserved", 1},
	{CCX_DTVCC_C1_RSV94, "RSV94", "Reserved", 1},
	{CCX_DTVCC_C1_RSV95, "RSV95", "Reserved", 1},
	{CCX_DTVCC_C1_RSV96, "RSV96", "Reserved", 1},
	{CCX_DTVCC_C1_SWA, "SWA", "SetWindowAttributes", 5},
	{CCX_DTVCC_C1_DF0, "DF0", "DefineWindow0", 7},
	{CCX_DTVCC_C1_DF1, "DF1", "DefineWindow1", 7},
	{CCX_DTVCC_C1_DF2, "DF2", "DefineWindow2", 7},
	{CCX_DTVCC_C1_DF3, "DF3", "DefineWindow3", 7},
	{CCX_DTVCC_C1_DF4, "DF4", "DefineWindow4", 7},
	{CCX_DTVCC_C1_DF5, "DF5", "DefineWindow5", 7},
	{CCX_DTVCC_C1_DF6, "DF6", "DefineWindow6", 7},
	{CCX_DTVCC_C1_DF7, "DF7", "DefineWindow7", 7}};

//------------------------- DEFAULT AND PREDEFINED -----------------------------

ccx_dtvcc_pen_color ccx_dtvcc_default_pen_color =
    {
	0x3f,
	0,
	0,
	0,
	0};

ccx_dtvcc_pen_attribs ccx_dtvcc_default_pen_attribs =
    {
	CCX_DTVCC_PEN_SIZE_STANDART,
	0,
	CCX_DTVCC_PEN_TEXT_TAG_UNDEFINED_12,
	0,
	CCX_DTVCC_PEN_EDGE_NONE,
	0,
	0};

ccx_dtvcc_window_attribs ccx_dtvcc_predefined_window_styles[] =
    {
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // Dummy, unused (position 0 doesn't use the table)
	{				   //1 - NTSC Style PopUp Captions
	 CCX_DTVCC_WINDOW_JUSTIFY_LEFT,
	 CCX_DTVCC_WINDOW_PD_LEFT_RIGHT,
	 CCX_DTVCC_WINDOW_SD_BOTTOM_TOP,
	 0,
	 CCX_DTVCC_WINDOW_SDE_SNAP,
	 0,
	 0,
	 0,
	 CCX_DTVCC_WINDOW_FO_SOLID,
	 CCX_DTVCC_WINDOW_BORDER_NONE,
	 0},
	{//2 - PopUp Captions w/o Black Background
	 CCX_DTVCC_WINDOW_JUSTIFY_LEFT,
	 CCX_DTVCC_WINDOW_PD_LEFT_RIGHT,
	 CCX_DTVCC_WINDOW_SD_BOTTOM_TOP,
	 0,
	 CCX_DTVCC_WINDOW_SDE_SNAP,
	 0,
	 0,
	 0,
	 CCX_DTVCC_WINDOW_FO_TRANSPARENT,
	 CCX_DTVCC_WINDOW_BORDER_NONE,
	 0},
	{//3 - NTSC Style Centered PopUp Captions
	 CCX_DTVCC_WINDOW_JUSTIFY_CENTER,
	 CCX_DTVCC_WINDOW_PD_LEFT_RIGHT,
	 CCX_DTVCC_WINDOW_SD_BOTTOM_TOP,
	 0,
	 CCX_DTVCC_WINDOW_SDE_SNAP,
	 0,
	 0,
	 0,
	 CCX_DTVCC_WINDOW_FO_SOLID,
	 CCX_DTVCC_WINDOW_BORDER_NONE,
	 0},
	{//4 - NTSC Style RollUp Captions
	 CCX_DTVCC_WINDOW_JUSTIFY_LEFT,
	 CCX_DTVCC_WINDOW_PD_LEFT_RIGHT,
	 CCX_DTVCC_WINDOW_SD_BOTTOM_TOP,
	 1,
	 CCX_DTVCC_WINDOW_SDE_SNAP,
	 0,
	 0,
	 0,
	 CCX_DTVCC_WINDOW_FO_SOLID,
	 CCX_DTVCC_WINDOW_BORDER_NONE,
	 0},
	{//5 - RollUp Captions w/o Black Background
	 CCX_DTVCC_WINDOW_JUSTIFY_LEFT,
	 CCX_DTVCC_WINDOW_PD_LEFT_RIGHT,
	 CCX_DTVCC_WINDOW_SD_BOTTOM_TOP,
	 1,
	 CCX_DTVCC_WINDOW_SDE_SNAP,
	 0,
	 0,
	 0,
	 CCX_DTVCC_WINDOW_FO_TRANSPARENT,
	 CCX_DTVCC_WINDOW_BORDER_NONE,
	 0},
	{//6 - NTSC Style Centered RollUp Captions
	 CCX_DTVCC_WINDOW_JUSTIFY_CENTER,
	 CCX_DTVCC_WINDOW_PD_LEFT_RIGHT,
	 CCX_DTVCC_WINDOW_SD_BOTTOM_TOP,
	 1,
	 CCX_DTVCC_WINDOW_SDE_SNAP,
	 0,
	 0,
	 0,
	 CCX_DTVCC_WINDOW_FO_SOLID,
	 CCX_DTVCC_WINDOW_BORDER_NONE,
	 0},
	{//7 - Ticker tape
	 CCX_DTVCC_WINDOW_JUSTIFY_LEFT,
	 CCX_DTVCC_WINDOW_PD_TOP_BOTTOM,
	 CCX_DTVCC_WINDOW_SD_RIGHT_LEFT,
	 0,
	 CCX_DTVCC_WINDOW_SDE_SNAP,
	 0,
	 0,
	 0,
	 CCX_DTVCC_WINDOW_FO_SOLID,
	 CCX_DTVCC_WINDOW_BORDER_NONE,
	 0}};
//---------------------------------- HELPERS ------------------------------------

void ccx_dtvcc_clear_packet(ccx_dtvcc_ctx *ctx)
{
	ctx->current_packet_length = 0;
	memset(ctx->current_packet, 0, CCX_DTVCC_MAX_PACKET_LENGTH * sizeof(unsigned char));
}

void _dtvcc_tv_clear(ccx_dtvcc_service_decoder *decoder)
{
	for (int i = 0; i < CCX_DTVCC_SCREENGRID_ROWS; i++)
		memset(decoder->tv->chars[i], 0, CCX_DTVCC_SCREENGRID_COLUMNS * sizeof(ccx_dtvcc_symbol));
	decoder->tv->time_ms_show = -1;
	decoder->tv->time_ms_hide = -1;
};

int _dtvcc_decoder_has_visible_windows(ccx_dtvcc_service_decoder *decoder)
{
	for (int i = 0; i < CCX_DTVCC_MAX_WINDOWS; i++)
	{
		if (decoder->windows[i].visible)
			return 1;
	}
	return 0;
}

void _dtvcc_window_clear_row(ccx_dtvcc_window *window, int row_index)
{
	if (window->memory_reserved)
	{
		memset(window->rows[row_index], 0, CCX_DTVCC_MAX_COLUMNS * sizeof(ccx_dtvcc_symbol));
		for (int column_index = 0; column_index < CCX_DTVCC_MAX_COLUMNS; column_index++)
		{
			window->pen_attribs[row_index][column_index] = ccx_dtvcc_default_pen_attribs;
			window->pen_colors[row_index][column_index] = ccx_dtvcc_default_pen_color;
		}
	}
}

void _dtvcc_window_clear_text(ccx_dtvcc_window *window)
{
	window->pen_color_pattern = ccx_dtvcc_default_pen_color;
	window->pen_attribs_pattern = ccx_dtvcc_default_pen_attribs;
	for (int i = 0; i < CCX_DTVCC_MAX_ROWS; i++)
		_dtvcc_window_clear_row(window, i);
	window->is_empty = 1;
}

void _dtvcc_window_clear(ccx_dtvcc_service_decoder *decoder, int window_id)
{
	_dtvcc_window_clear_text(&decoder->windows[window_id]);
	//OPT fill window with a window fill color
}

void _dtvcc_window_apply_style(ccx_dtvcc_window *window, ccx_dtvcc_window_attribs *style)
{
	window->attribs.border_color = style->border_color;
	window->attribs.border_type = style->border_type;
	window->attribs.display_effect = style->display_effect;
	window->attribs.effect_direction = style->effect_direction;
	window->attribs.effect_speed = style->effect_speed;
	window->attribs.fill_color = style->fill_color;
	window->attribs.fill_opacity = style->fill_opacity;
	window->attribs.justify = style->justify;
	window->attribs.print_direction = style->print_direction;
	window->attribs.scroll_direction = style->scroll_direction;
	window->attribs.word_wrap = style->word_wrap;
}

//#define DTVCC_PRINT_DEBUG
#ifdef DTVCC_PRINT_DEBUG

int _dtvcc_is_win_row_empty(ccx_dtvcc_window *window, int row_index)
{
	for (int j = 0; j < CCX_DTVCC_MAX_COLUMNS; j++)
	{
		if (CCX_DTVCC_SYM_IS_SET(window->rows[row_index][j]))
			return 0;
	}
	return 1;
}

void _dtvcc_get_win_write_interval(ccx_dtvcc_window *window, int row_index, int *first, int *last)
{
	for (*first = 0; *first < CCX_DTVCC_MAX_COLUMNS; (*first)++)
		if (CCX_DTVCC_SYM_IS_SET(window->rows[row_index][*first]))
			break;
	for (*last = CCX_DTVCC_MAX_COLUMNS - 1; *last > 0; (*last)--)
		if (CCX_DTVCC_SYM_IS_SET(window->rows[row_index][*last]))
			break;
}

void _dtvcc_window_dump(ccx_dtvcc_service_decoder *decoder, ccx_dtvcc_window *window)
{
	ccx_common_logging.debug_ftn(CCX_DMT_GENERIC_NOTICES, "[CEA-708] Window %d dump:\n", window->number);

	if (!window->is_defined)
		return;

	char tbuf1[SUBLINESIZE],
	    tbuf2[SUBLINESIZE];
	char sym_buf[10];

	print_mstime_buff(window->time_ms_show, "%02u:%02u:%02u:%03u", tbuf1);
	print_mstime_buff(window->time_ms_hide, "%02u:%02u:%02u:%03u", tbuf2);

	ccx_common_logging.debug_ftn(CCX_DMT_GENERIC_NOTICES, "\r%s --> %s\n", tbuf1, tbuf2);
	for (int i = 0; i < CCX_DTVCC_MAX_ROWS; i++)
	{
		if (!_dtvcc_is_win_row_empty(window, i))
		{
			int first, last;
			ccx_dtvcc_symbol sym;
			_dtvcc_get_win_write_interval(window, i, &first, &last);
			for (int j = first; j <= last; j++)
			{
				sym = window->rows[i][j];
				int len = utf16_to_utf8(sym.sym, sym_buf);
				for (int index = 0; index < len; index++)
					ccx_common_logging.debug_ftn(CCX_DMT_GENERIC_NOTICES, "%c", sym_buf[index]);
			}
			ccx_common_logging.debug_ftn(CCX_DMT_GENERIC_NOTICES, "\n");
		}
	}

	ccx_common_logging.debug_ftn(CCX_DMT_GENERIC_NOTICES, "[CEA-708] Dump done\n", window->number);
}

#endif

void ccx_dtvcc_windows_reset(ccx_dtvcc_service_decoder *decoder)
{
	for (int j = 0; j < CCX_DTVCC_MAX_WINDOWS; j++)
	{
		_dtvcc_window_clear_text(&decoder->windows[j]);
		decoder->windows[j].is_defined = 0;
		decoder->windows[j].visible = 0;
		memset(decoder->windows[j].commands, 0, sizeof(decoder->windows[j].commands));
	}
	decoder->current_window = -1;
	_dtvcc_tv_clear(decoder);
}

void _dtvcc_decoders_reset(ccx_dtvcc_ctx *dtvcc)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] _dtvcc_decoders_reset: Resetting all decoders\n");

	for (int i = 0; i < CCX_DTVCC_MAX_SERVICES; i++)
	{
		if (!dtvcc->services_active[i])
			continue;
		ccx_dtvcc_windows_reset(&dtvcc->decoders[i]);
	}

	ccx_dtvcc_clear_packet(dtvcc);

	dtvcc->last_sequence = CCX_DTVCC_NO_LAST_SEQUENCE;
	dtvcc->report->reset_count++;
}

int _dtvcc_compare_win_priorities(const void *a, const void *b)
{
	ccx_dtvcc_window *w1 = *(ccx_dtvcc_window **)a;
	ccx_dtvcc_window *w2 = *(ccx_dtvcc_window **)b;
	return (w1->priority - w2->priority);
}

void _dtvcc_window_update_time_show(ccx_dtvcc_window *window, struct ccx_common_timing_ctx *timing)
{
	char buf[128];
	window->time_ms_show = get_visible_start(timing, 3);
	print_mstime_buff(window->time_ms_show, "%02u:%02u:%02u:%03u", buf);
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] "
						  "[W-%d] show time updated to %s\n",
				     window->number, buf);
}

void _dtvcc_window_update_time_hide(ccx_dtvcc_window *window, struct ccx_common_timing_ctx *timing)
{
	char buf[128];
	window->time_ms_hide = get_visible_end(timing, 3);
	print_mstime_buff(window->time_ms_hide, "%02u:%02u:%02u:%03u", buf);
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] "
						  "[W-%d] hide time updated to %s\n",
				     window->number, buf);
}

void _dtvcc_screen_update_time_show(dtvcc_tv_screen *tv, LLONG time)
{
	char buf1[128], buf2[128];
	print_mstime_buff(tv->time_ms_show, "%02u:%02u:%02u:%03u", buf1);
	print_mstime_buff(time, "%02u:%02u:%02u:%03u", buf2);
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] "
						  "Screen show time: %s -> %s\n",
				     buf1, buf2);

	if (tv->time_ms_show == -1)
		tv->time_ms_show = time;
	else if (tv->time_ms_show > time)
		tv->time_ms_show = time;
}

void _dtvcc_screen_update_time_hide(dtvcc_tv_screen *tv, LLONG time)
{
	char buf1[128], buf2[128];
	print_mstime_buff(tv->time_ms_hide, "%02u:%02u:%02u:%03u", buf1);
	print_mstime_buff(time, "%02u:%02u:%02u:%03u", buf2);
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] "
						  "Screen hide time: %s -> %s\n",
				     buf1, buf2);

	if (tv->time_ms_hide == -1)
		tv->time_ms_hide = time;
	else if (tv->time_ms_hide < time)
		tv->time_ms_hide = time;
}

void _get_window_dimensions(ccx_dtvcc_window *window, int *x1, int *x2, int *y1, int *y2)
{
	int a_x1, a_x2, a_y1, a_y2;
	switch (window->anchor_point)
	{
		case CCX_DTVCC_ANCHOR_POINT_TOP_LEFT:
			a_x1 = window->anchor_vertical;
			a_x2 = window->anchor_vertical + window->row_count;
			a_y1 = window->anchor_horizontal;
			a_y2 = window->anchor_horizontal + window->col_count;
			break;
		case CCX_DTVCC_ANCHOR_POINT_TOP_CENTER:
			a_x1 = window->anchor_vertical;
			a_x2 = window->anchor_vertical + window->row_count;
			a_y1 = window->anchor_horizontal - window->col_count;
			a_y2 = window->anchor_horizontal + window->col_count / 2;
			break;
		case CCX_DTVCC_ANCHOR_POINT_TOP_RIGHT:
			a_x1 = window->anchor_vertical;
			a_x2 = window->anchor_vertical + window->row_count;
			a_y1 = window->anchor_horizontal - window->col_count;
			a_y2 = window->anchor_horizontal;
			break;
		case CCX_DTVCC_ANCHOR_POINT_MIDDLE_LEFT:
			a_x1 = window->anchor_vertical - window->row_count / 2;
			a_x2 = window->anchor_vertical + window->row_count / 2;
			a_y1 = window->anchor_horizontal;
			a_y2 = window->anchor_horizontal + window->col_count;
			break;
		case CCX_DTVCC_ANCHOR_POINT_MIDDLE_CENTER:
			a_x1 = window->anchor_vertical - window->row_count / 2;
			a_x2 = window->anchor_vertical + window->row_count / 2;
			a_y1 = window->anchor_horizontal - window->col_count / 2;
			a_y2 = window->anchor_horizontal + window->col_count / 2;
			break;
		case CCX_DTVCC_ANCHOR_POINT_MIDDLE_RIGHT:
			a_x1 = window->anchor_vertical - window->row_count / 2;
			a_x2 = window->anchor_vertical + window->row_count / 2;
			a_y1 = window->anchor_horizontal - window->col_count;
			a_y2 = window->anchor_horizontal;
			break;
		case CCX_DTVCC_ANCHOR_POINT_BOTTOM_LEFT:
			a_x1 = window->anchor_vertical - window->row_count;
			a_x2 = window->anchor_vertical;
			a_y1 = window->anchor_horizontal;
			a_y2 = window->anchor_horizontal + window->col_count;
			break;
		case CCX_DTVCC_ANCHOR_POINT_BOTTOM_CENTER:
			a_x1 = window->anchor_vertical - window->row_count;
			a_x2 = window->anchor_vertical;
			a_y1 = window->anchor_horizontal - window->col_count / 2;
			a_y2 = window->anchor_horizontal + window->col_count / 2;
			break;
		case CCX_DTVCC_ANCHOR_POINT_BOTTOM_RIGHT:
			a_x1 = window->anchor_vertical - window->row_count;
			a_x2 = window->anchor_vertical;
			a_y1 = window->anchor_horizontal - window->col_count;
			a_y2 = window->anchor_horizontal;
			break;
		default: // Shouldn't happen, but skip the window just in case
			return;
			break;
	}

	*x1 = a_x1 < 0 ? 0 : a_x1;
	*x2 = a_x2 > CCX_DTVCC_SCREENGRID_ROWS ? CCX_DTVCC_SCREENGRID_ROWS : a_x2;
	*y1 = a_y1 < 0 ? 0 : a_y1;
	*y2 = a_y2 > CCX_DTVCC_SCREENGRID_COLUMNS ? CCX_DTVCC_SCREENGRID_COLUMNS : a_y2;

	return;
}

int _is_window_overlapping(ccx_dtvcc_service_decoder *decoder, ccx_dtvcc_window *window)
{
	int a_x1, a_x2, a_y1, a_y2, b_x1, b_x2, b_y1, b_y2, flag = 0;
	_get_window_dimensions(window, &a_x1, &a_x2, &a_y1, &a_y2);
	ccx_dtvcc_window *windcompare = &decoder->windows[0];
	for (int i = 0; i < CCX_DTVCC_MAX_WINDOWS; i++, windcompare++)
	{

		_get_window_dimensions(windcompare, &b_x1, &b_x2, &b_y1, &b_y2);
		if (a_x1 == b_x1 && a_x2 == b_x2 && a_y1 == b_y1 && a_y2 == b_y2)
		{
			continue;
		}
		else
		{
			if ((a_x1 < b_x2) && (a_x2 > b_x1) && (a_y1 < b_y2) && (a_y2 > b_y1) && decoder->windows[i].visible)
			{
				if (decoder->windows[i].priority < window->priority)
				{
					flag = OVERLAPPED_BY_HIGH_PRIORITY;
				}
				else
				{
					flag = OVERLAPPING_WITH_HIGH_PRIORITY;
				}
				//priority is either higher or equals for *window , hence overlaps decoder->windows[i]
			}
		}
	}
	if (flag == 1)
	{
		return 1;
	}
	return 0;
}

void _dtvcc_window_copy_to_screen(ccx_dtvcc_service_decoder *decoder, ccx_dtvcc_window *window)
{
	switch (_is_window_overlapping(decoder, window))
	{
		case OVERLAPPING_WITH_HIGH_PRIORITY:
			ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] _dtvcc_window_copy_to_screen : no handling required \n");
			break;
		case OVERLAPPED_BY_HIGH_PRIORITY:
			ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] _dtvcc_window_copy_to_screen : window needs to be skipped \n");
			return;
	}

	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] _dtvcc_window_copy_to_screen: W-%d\n", window->number);
	int top, left;
	// For each window we calculate the top, left position depending on the
	// anchor
	switch (window->anchor_point)
	{
		case CCX_DTVCC_ANCHOR_POINT_TOP_LEFT:
			top = window->anchor_vertical;
			left = window->anchor_horizontal;
			break;
		case CCX_DTVCC_ANCHOR_POINT_TOP_CENTER:
			top = window->anchor_vertical;
			left = window->anchor_horizontal - window->col_count / 2;
			break;
		case CCX_DTVCC_ANCHOR_POINT_TOP_RIGHT:
			top = window->anchor_vertical;
			left = window->anchor_horizontal - window->col_count;
			break;
		case CCX_DTVCC_ANCHOR_POINT_MIDDLE_LEFT:
			top = window->anchor_vertical - window->row_count / 2;
			left = window->anchor_horizontal;
			break;
		case CCX_DTVCC_ANCHOR_POINT_MIDDLE_CENTER:
			top = window->anchor_vertical - window->row_count / 2;
			left = window->anchor_horizontal - window->col_count / 2;
			break;
		case CCX_DTVCC_ANCHOR_POINT_MIDDLE_RIGHT:
			top = window->anchor_vertical - window->row_count / 2;
			left = window->anchor_horizontal - window->col_count;
			break;
		case CCX_DTVCC_ANCHOR_POINT_BOTTOM_LEFT:
			top = window->anchor_vertical - window->row_count;
			left = window->anchor_horizontal;
			break;
		case CCX_DTVCC_ANCHOR_POINT_BOTTOM_CENTER:
			top = window->anchor_vertical - window->row_count;
			left = window->anchor_horizontal - window->col_count / 2;
			break;
		case CCX_DTVCC_ANCHOR_POINT_BOTTOM_RIGHT:
			top = window->anchor_vertical - window->row_count;
			left = window->anchor_horizontal - window->col_count;
			break;
		default: // Shouldn't happen, but skip the window just in case
			return;
			break;
	}
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] For window %d: Anchor point -> %d, size %d:%d, real position %d:%d\n",
				     window->number, window->anchor_point, window->row_count, window->col_count,
				     top, left);

	ccx_common_logging.debug_ftn(
	    CCX_DMT_708, "[CEA-708] we have top [%d] and left [%d]\n", top, left);

	top = top < 0 ? 0 : top;
	left = left < 0 ? 0 : left;

	int copyrows = top + window->row_count >= CCX_DTVCC_SCREENGRID_ROWS ? CCX_DTVCC_SCREENGRID_ROWS - top : window->row_count;
	int copycols = left + window->col_count >= CCX_DTVCC_SCREENGRID_COLUMNS ? CCX_DTVCC_SCREENGRID_COLUMNS - left : window->col_count;

	ccx_common_logging.debug_ftn(
	    CCX_DMT_708, "[CEA-708] %d*%d will be copied to the TV.\n", copyrows, copycols);

	for (int j = 0; j < copyrows; j++)
	{
		memcpy(decoder->tv->chars[top + j], window->rows[j], copycols * sizeof(ccx_dtvcc_symbol));
		for (int col = 0; col < CCX_DTVCC_SCREENGRID_COLUMNS; col++)
		{
			decoder->tv->pen_attribs[top + j][col] = window->pen_attribs[j][col];
			decoder->tv->pen_colors[top + j][col] = window->pen_colors[j][col];
		}
	}

	_dtvcc_screen_update_time_show(decoder->tv, window->time_ms_show);
	_dtvcc_screen_update_time_hide(decoder->tv, window->time_ms_hide);

#ifdef DTVCC_PRINT_DEBUG
	_dtvcc_window_dump(decoder, window);
#endif
}

void _dtvcc_screen_print(ccx_dtvcc_ctx *dtvcc, ccx_dtvcc_service_decoder *decoder)
{
	//TODO use priorities to solve windows overlap (with a video sample, please)
	//qsort(wnd, visible, sizeof(ccx_dtvcc_window *), _dtvcc_compare_win_priorities);

	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] _dtvcc_screen_print\n");

	_dtvcc_screen_update_time_hide(decoder->tv, get_visible_end(dtvcc->timing, 3));

#ifdef DTVCC_PRINT_DEBUG
	//ccx_common_logging.debug_ftn(CCX_DMT_GENERIC_NOTICES, "[CEA-708] TV dump:\n");
	//ccx_dtvcc_write_debug(decoder->tv);
#endif
	decoder->cc_count++;
	decoder->tv->cc_count++;

	struct encoder_ctx *encoder = (struct encoder_ctx *)dtvcc->encoder;
	int sn = decoder->tv->service_number;
	ccx_dtvcc_writer_ctx *writer = &encoder->dtvcc_writers[sn - 1];
	ccx_dtvcc_writer_output(writer, decoder, encoder);

	_dtvcc_tv_clear(decoder);
}

void _dtvcc_process_hcr(ccx_dtvcc_service_decoder *decoder)
{
	if (decoder->current_window == -1)
	{
		ccx_common_logging.log_ftn("[CEA-708] _dtvcc_process_hcr: Window has to be defined first\n");
		return;
	}

	ccx_dtvcc_window *window = &decoder->windows[decoder->current_window];
	window->pen_column = 0;
	_dtvcc_window_clear_row(window, window->pen_row);
}

void _dtvcc_process_ff(ccx_dtvcc_service_decoder *decoder)
{
	if (decoder->current_window == -1)
	{
		ccx_common_logging.log_ftn("[CEA-708] _dtvcc_process_ff: Window has to be defined first\n");
		return;
	}
	ccx_dtvcc_window *window = &decoder->windows[decoder->current_window];
	window->pen_column = 0;
	window->pen_row = 0;
	//CEA-708-D doesn't say we have to clear neither window text nor text line,
	//but it seems we have to clean the line
	//_dtvcc_window_clear_text(window);
}

void _dtvcc_process_etx(ccx_dtvcc_service_decoder *decoder)
{
	//it can help decoders with screen output, but could it help us?
}

void _dtvcc_process_bs(ccx_dtvcc_service_decoder *decoder)
{
	if (decoder->current_window == -1)
	{
		ccx_common_logging.log_ftn("[CEA-708] _dtvcc_process_bs: Window has to be defined first\n");
		return;
	}

	//it looks strange, but in some videos (rarely) we have a backspace command
	//we just print one character over another
	int cw = decoder->current_window;
	ccx_dtvcc_window *window = &decoder->windows[cw];

	switch (window->attribs.print_direction)
	{
		case CCX_DTVCC_WINDOW_PD_RIGHT_LEFT:
			if (window->pen_column + 1 < window->col_count)
				window->pen_column++;
			break;
		case CCX_DTVCC_WINDOW_PD_LEFT_RIGHT:
			if (decoder->windows->pen_column > 0)
				window->pen_column--;
			break;
		case CCX_DTVCC_WINDOW_PD_BOTTOM_TOP:
			if (window->pen_row + 1 < window->row_count)
				window->pen_row++;
			break;
		case CCX_DTVCC_WINDOW_PD_TOP_BOTTOM:
			if (window->pen_row > 0)
				window->pen_row--;
			break;
		default:
			ccx_common_logging.log_ftn("[CEA-708] _dtvcc_process_character: unhandled branch (%02d)\n",
						   window->attribs.print_direction);
			break;
	}
}

void _dtvcc_window_rollup(ccx_dtvcc_service_decoder *decoder, ccx_dtvcc_window *window)
{
	for (int i = 0; i < window->row_count - 1; i++)
	{
		memcpy(window->rows[i], window->rows[i + 1], CCX_DTVCC_MAX_COLUMNS * sizeof(ccx_dtvcc_symbol));
		for (int z = 0; z < CCX_DTVCC_MAX_COLUMNS; z++)
		{
			window->pen_colors[i][z] = window->pen_colors[i + 1][z];
			window->pen_attribs[i][z] = window->pen_attribs[i + 1][z];
		}
	}

	_dtvcc_window_clear_row(window, window->row_count - 1);
}

void _dtvcc_process_cr(ccx_dtvcc_ctx *dtvcc, ccx_dtvcc_service_decoder *decoder)
{
	if (decoder->current_window == -1)
	{
		ccx_common_logging.log_ftn("[CEA-708] _dtvcc_process_cr: Window has to be defined first\n");
		return;
	}

	ccx_dtvcc_window *window = &decoder->windows[decoder->current_window];

	int rollup_required = 0;
	switch (window->attribs.print_direction)
	{
		case CCX_DTVCC_WINDOW_PD_LEFT_RIGHT:
			window->pen_column = 0;
			if (window->pen_row + 1 < window->row_count)
				window->pen_row++;
			else
				rollup_required = 1;
			break;
		case CCX_DTVCC_WINDOW_PD_RIGHT_LEFT:
			window->pen_column = window->col_count;
			if (window->pen_row + 1 < window->row_count)
				window->pen_row++;
			else
				rollup_required = 1;
			break;
		case CCX_DTVCC_WINDOW_PD_TOP_BOTTOM:
			window->pen_row = 0;
			if (window->pen_column + 1 < window->col_count)
				window->pen_column++;
			else
				rollup_required = 1;
			break;
		case CCX_DTVCC_WINDOW_PD_BOTTOM_TOP:
			window->pen_row = window->row_count;
			if (window->pen_column + 1 < window->col_count)
				window->pen_column++;
			else
				rollup_required = 1;
			break;
		default:
			ccx_common_logging.log_ftn("[CEA-708] _dtvcc_process_cr: unhandled branch\n");
			break;
	}

	if (window->is_defined)
	{
		ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] _dtvcc_process_cr: rolling up\n");

		_dtvcc_window_update_time_hide(window, dtvcc->timing);
		_dtvcc_window_copy_to_screen(decoder, window);
		_dtvcc_screen_print(dtvcc, decoder);

		if (rollup_required)
		{
			if (dtvcc->no_rollup)
				_dtvcc_window_clear_row(window, window->pen_row);
			else
				_dtvcc_window_rollup(decoder, window);
		}
		_dtvcc_window_update_time_show(window, dtvcc->timing);
	}
}

void _dtvcc_process_character(ccx_dtvcc_service_decoder *decoder, ccx_dtvcc_symbol symbol)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] %d\n", decoder->current_window);
	int cw = decoder->current_window;
	ccx_dtvcc_window *window = &decoder->windows[cw];

	ccx_common_logging.debug_ftn(
	    CCX_DMT_708, "[CEA-708] _dtvcc_process_character: "
			 "%c [%02X]  - Window: %d %s, Pen: %d:%d\n",
	    CCX_DTVCC_SYM(symbol), CCX_DTVCC_SYM(symbol),
	    cw, window->is_defined ? "[OK]" : "[undefined]",
	    cw != -1 ? window->pen_row : -1, cw != -1 ? window->pen_column : -1);

	if (cw == -1 || !window->is_defined) // Writing to a non existing window, skipping
		return;

	window->is_empty = 0;
	window->rows[window->pen_row][window->pen_column] = symbol;
	window->pen_attribs[window->pen_row][window->pen_column] = window->pen_attribs_pattern; // "Painting" char by pen - attribs
	window->pen_colors[window->pen_row][window->pen_column] = window->pen_color_pattern;	// "Painting" char by pen - colors
	switch (window->attribs.print_direction)
	{
		case CCX_DTVCC_WINDOW_PD_LEFT_RIGHT:
			if (window->pen_column + 1 < window->col_count)
				window->pen_column++;
			break;
		case CCX_DTVCC_WINDOW_PD_RIGHT_LEFT:
			if (decoder->windows->pen_column > 0)
				window->pen_column--;
			break;
		case CCX_DTVCC_WINDOW_PD_TOP_BOTTOM:
			if (window->pen_row + 1 < window->row_count)
				window->pen_row++;
			break;
		case CCX_DTVCC_WINDOW_PD_BOTTOM_TOP:
			if (window->pen_row > 0)
				window->pen_row--;
			break;
		default:
			ccx_common_logging.log_ftn("[CEA-708] _dtvcc_process_character: unhandled branch (%02d)\n",
						   window->attribs.print_direction);
			break;
	}
}

void ccx_dtvcc_decoder_flush(ccx_dtvcc_ctx *dtvcc, ccx_dtvcc_service_decoder *decoder)
{
	ccx_common_logging.debug_ftn(
	    CCX_DMT_708, "[CEA-708] _dtvcc_decoder_flush: Flushing decoder\n");
	int screen_content_changed = 0;
	for (int i = 0; i < CCX_DTVCC_MAX_WINDOWS; i++)
	{
		ccx_dtvcc_window *window = &decoder->windows[i];
		if (window->visible)
		{
			screen_content_changed = 1;
			_dtvcc_window_update_time_hide(window, dtvcc->timing);
			_dtvcc_window_copy_to_screen(decoder, window);
			window->visible = 0;
		}
	}
	if (screen_content_changed)
		_dtvcc_screen_print(dtvcc, decoder);
	ccx_dtvcc_write_done(decoder->tv, dtvcc->encoder);
}

//---------------------------------- COMMANDS ------------------------------------

void dtvcc_handle_CWx_SetCurrentWindow(ccx_dtvcc_service_decoder *decoder, int window_id)
{
	ccx_common_logging.debug_ftn(
	    CCX_DMT_708, "[CEA-708] dtvcc_handle_CWx_SetCurrentWindow: [%d]\n", window_id);
	if (decoder->windows[window_id].is_defined)
		decoder->current_window = window_id;
	else
		ccx_common_logging.log_ftn("[CEA-708] dtvcc_handle_CWx_SetCurrentWindow: "
					   "window [%d] is not defined\n",
					   window_id);
}

void dtvcc_handle_CLW_ClearWindows(ccx_dtvcc_ctx *dtvcc, ccx_dtvcc_service_decoder *decoder, int windows_bitmap)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_CLW_ClearWindows: windows: ");
	int screen_content_changed = 0,
	    window_had_content;
	if (windows_bitmap == 0)
		ccx_common_logging.debug_ftn(CCX_DMT_708, "none\n");
	else
	{
		for (int i = 0; i < CCX_DTVCC_MAX_WINDOWS; i++)
		{
			if (windows_bitmap & 1)
			{
				ccx_dtvcc_window *window = &decoder->windows[i];
				ccx_common_logging.debug_ftn(CCX_DMT_708, "[W%d] ", i);
				window_had_content = window->is_defined && window->visible && !window->is_empty;
				if (window_had_content)
				{
					screen_content_changed = 1;
					_dtvcc_window_update_time_hide(&decoder->windows[i], dtvcc->timing);
					_dtvcc_window_copy_to_screen(decoder, &decoder->windows[i]);
				}
				_dtvcc_window_clear(decoder, i);
			}
			windows_bitmap >>= 1;
		}
	}
	ccx_common_logging.debug_ftn(CCX_DMT_708, "\n");
	if (screen_content_changed)
		_dtvcc_screen_print(dtvcc, decoder);
}

void dtvcc_handle_DSW_DisplayWindows(ccx_dtvcc_service_decoder *decoder, int windows_bitmap, struct ccx_common_timing_ctx *timing)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_DSW_DisplayWindows: windows: ");
	if (windows_bitmap == 0)
		ccx_common_logging.debug_ftn(CCX_DMT_708, "none\n");
	else
	{
		for (int i = 0; i < CCX_DTVCC_MAX_WINDOWS; i++)
		{
			if (windows_bitmap & 1)
			{
				ccx_common_logging.debug_ftn(CCX_DMT_708, "[Window %d] ", i);
				if (!decoder->windows[i].is_defined)
				{
					ccx_common_logging.log_ftn("[CEA-708] Error: window %d was not defined\n", i);
					continue;
				}
				if (!decoder->windows[i].visible)
				{
					decoder->windows[i].visible = 1;
					_dtvcc_window_update_time_show(&decoder->windows[i], timing);
				}
			}
			windows_bitmap >>= 1;
		}
		ccx_common_logging.debug_ftn(CCX_DMT_708, "\n");
	}
}

void dtvcc_handle_HDW_HideWindows(ccx_dtvcc_ctx *dtvcc,
				  ccx_dtvcc_service_decoder *decoder,
				  int windows_bitmap)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_HDW_HideWindows: windows: ");
	if (windows_bitmap == 0)
		ccx_common_logging.debug_ftn(CCX_DMT_708, "none\n");
	else
	{
		int screen_content_changed = 0;
		for (int i = 0; i < CCX_DTVCC_MAX_WINDOWS; i++)
		{
			if (windows_bitmap & 1)
			{
				ccx_common_logging.debug_ftn(CCX_DMT_708, "[Window %d] ", i);
				if (decoder->windows[i].visible)
				{
					screen_content_changed = 1;
					decoder->windows[i].visible = 0;
					_dtvcc_window_update_time_hide(&decoder->windows[i], dtvcc->timing);
					if (!decoder->windows[i].is_empty)
						_dtvcc_window_copy_to_screen(decoder, &decoder->windows[i]);
				}
			}
			windows_bitmap >>= 1;
		}
		ccx_common_logging.debug_ftn(CCX_DMT_708, "\n");
		if (screen_content_changed && !_dtvcc_decoder_has_visible_windows(decoder))
			_dtvcc_screen_print(dtvcc, decoder);
	}
}

void dtvcc_handle_TGW_ToggleWindows(ccx_dtvcc_ctx *dtvcc,
				    ccx_dtvcc_service_decoder *decoder,
				    int windows_bitmap)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_TGW_ToggleWindows: windows: ");
	if (windows_bitmap == 0)
		ccx_common_logging.debug_ftn(CCX_DMT_708, "none\n");
	else
	{
		int screen_content_changed = 0;
		for (int i = 0; i < CCX_DTVCC_MAX_WINDOWS; i++)
		{
			ccx_dtvcc_window *window = &decoder->windows[i];
			if ((windows_bitmap & 1) && window->is_defined)
			{
				ccx_common_logging.debug_ftn(CCX_DMT_708, "[W-%d: %d->%d]", i, window->visible, !window->visible);
				window->visible = !window->visible;
				if (window->visible)
					_dtvcc_window_update_time_show(window, dtvcc->timing);
				else
				{
					_dtvcc_window_update_time_hide(window, dtvcc->timing);
					if (!window->is_empty)
					{
						screen_content_changed = 1;
						_dtvcc_window_copy_to_screen(decoder, window);
					}
				}
			}
			windows_bitmap >>= 1;
		}
		ccx_common_logging.debug_ftn(CCX_DMT_708, "\n");
		if (screen_content_changed && !_dtvcc_decoder_has_visible_windows(decoder))
			_dtvcc_screen_print(dtvcc, decoder);
	}
}

void dtvcc_handle_DFx_DefineWindow(ccx_dtvcc_service_decoder *decoder, int window_id, unsigned char *data, struct ccx_common_timing_ctx *timing)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_DFx_DefineWindow: "
						  "W[%d], attributes: \n",
				     window_id);

	ccx_dtvcc_window *window = &decoder->windows[window_id];

	if (window->is_defined && !memcmp(window->commands, data + 1, 6))
	{
		// When a decoder receives a DefineWindow command for an existing window, the
		// command is to be ignored if the command parameters are unchanged from the
		// previous window definition.
		ccx_common_logging.debug_ftn(
		    CCX_DMT_708, "[CEA-708] dtvcc_handle_DFx_DefineWindow: Repeated window definition, ignored\n");
		return;
	}

	window->number = window_id;

	int priority = (data[1]) & 0x7;
	int col_lock = (data[1] >> 3) & 0x1;
	int row_lock = (data[1] >> 4) & 0x1;
	int visible = (data[1] >> 5) & 0x1;
	int anchor_vertical = data[2] & 0x7f;
	int relative_pos = data[2] >> 7;
	int anchor_horizontal = data[3];
	int row_count = (data[4] & 0xf) + 1; //according to CEA-708-D
	int anchor_point = data[4] >> 4;
	int col_count = (data[5] & 0x3f) + 1; //according to CEA-708-D
	int pen_style = data[6] & 0x7;
	int win_style = (data[6] >> 3) & 0x7;

	int do_clear_window = 0;

	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] Visible: [%s]\n", visible ? "Yes" : "No");
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] Priority: [%d]\n", priority);
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] Row count: [%d]\n", row_count);
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] Column count: [%d]\n", col_count);
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] Anchor point: [%d]\n", anchor_point);
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] Anchor vertical: [%d]\n", anchor_vertical);
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] Anchor horizontal: [%d]\n", anchor_horizontal);
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] Relative pos: [%s]\n", relative_pos ? "Yes" : "No");
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] Row lock: [%s]\n", row_lock ? "Yes" : "No");
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] Column lock: [%s]\n", col_lock ? "Yes" : "No");
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] Pen style: [%d]\n", pen_style);
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] Win style: [%d]\n", win_style);

	/**
	 * Korean samples have "anchor_vertical" and "anchor_horizontal" mixed up,
	 * this seems to be an encoder issue, but we can workaround it
	 */
	if (anchor_vertical > CCX_DTVCC_SCREENGRID_ROWS - row_count)
		anchor_vertical = CCX_DTVCC_SCREENGRID_ROWS - row_count;
	if (anchor_horizontal > CCX_DTVCC_SCREENGRID_COLUMNS - col_count)
		anchor_horizontal = CCX_DTVCC_SCREENGRID_COLUMNS - col_count;

	window->priority = priority;
	window->col_lock = col_lock;
	window->row_lock = row_lock;
	window->visible = visible;
	window->anchor_vertical = anchor_vertical;
	window->relative_pos = relative_pos;
	window->anchor_horizontal = anchor_horizontal;
	window->row_count = row_count;
	window->anchor_point = anchor_point;
	window->col_count = col_count;

	// If changing the style of an existing window delete contents
	if (win_style > 0 && window->is_defined && window->win_style != win_style)
		do_clear_window = 1;

	/* If the window doesn't exist and win style==0 then default to win_style=1 */
	if (win_style == 0 && !window->is_defined)
	{
		win_style = 1;
	}
	/* If the window doesn't exist and pen style==0 then default to pen_style=1 */
	if (pen_style == 0 && !window->is_defined)
	{
		pen_style = 1;
	}

	//Apply windows attribute presets
	if (win_style > 0 && win_style < 8)
	{
		window->win_style = win_style;
		window->attribs.border_color = ccx_dtvcc_predefined_window_styles[win_style].border_color;
		window->attribs.border_type = ccx_dtvcc_predefined_window_styles[win_style].border_type;
		window->attribs.display_effect = ccx_dtvcc_predefined_window_styles[win_style].display_effect;
		window->attribs.effect_direction = ccx_dtvcc_predefined_window_styles[win_style].effect_direction;
		window->attribs.effect_speed = ccx_dtvcc_predefined_window_styles[win_style].effect_speed;
		window->attribs.fill_color = ccx_dtvcc_predefined_window_styles[win_style].fill_color;
		window->attribs.fill_opacity = ccx_dtvcc_predefined_window_styles[win_style].fill_opacity;
		window->attribs.justify = ccx_dtvcc_predefined_window_styles[win_style].justify;
		window->attribs.print_direction = ccx_dtvcc_predefined_window_styles[win_style].print_direction;
		window->attribs.scroll_direction = ccx_dtvcc_predefined_window_styles[win_style].scroll_direction;
		window->attribs.word_wrap = ccx_dtvcc_predefined_window_styles[win_style].word_wrap;
	}

	if (pen_style > 0)
	{
		//TODO apply static pen_style preset
		window->pen_style = pen_style;
	}

	if (!window->is_defined)
	{
		// If the window is being created, all character positions in the window
		// are set to the fill color and the pen location is set to (0,0)
		window->pen_column = 0;
		window->pen_row = 0;
		if (!window->memory_reserved)
		{
			for (int i = 0; i < CCX_DTVCC_MAX_ROWS; i++)
			{
				window->rows[i] = (ccx_dtvcc_symbol *)malloc(CCX_DTVCC_MAX_COLUMNS * sizeof(ccx_dtvcc_symbol));
				if (!window->rows[i])
					ccx_common_logging.fatal_ftn(EXIT_NOT_ENOUGH_MEMORY, "[CEA-708] dtvcc_handle_DFx_DefineWindow");
			}
			window->memory_reserved = 1;
		}
		window->is_defined = 1;
		_dtvcc_window_clear_text(window);

		// According to CEA-708-D if window_style is 0 for newly created window, we have to apply predefined style #1
		if (window->win_style == 0)
		{
			_dtvcc_window_apply_style(window, &ccx_dtvcc_predefined_window_styles[0]);
		}
		else if (window->win_style <= 7)
		{
			_dtvcc_window_apply_style(window, &ccx_dtvcc_predefined_window_styles[window->win_style - 1]);
		}
		else
		{
			ccx_common_logging.log_ftn("[CEA-708] dtvcc_handle_DFx_DefineWindow: "
						   "invalid win_style num %d\n",
						   window->win_style);
			_dtvcc_window_apply_style(window, &ccx_dtvcc_predefined_window_styles[0]);
		}
	}
	else
	{
		if (do_clear_window)
			_dtvcc_window_clear_text(window);
	}
	// ...also makes the defined windows the current window
	dtvcc_handle_CWx_SetCurrentWindow(decoder, window_id);
	memcpy(window->commands, data + 1, 6);

	if (window->visible)
		_dtvcc_window_update_time_show(window, timing);
	if (!window->memory_reserved)
	{
		for (int i = 0; i < CCX_DTVCC_MAX_ROWS; i++)
		{
			free(window->rows[i]);
		}
	}
}

void dtvcc_handle_SWA_SetWindowAttributes(ccx_dtvcc_service_decoder *decoder, unsigned char *data)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_SWA_SetWindowAttributes: attributes: \n");

	int fill_color = (data[1]) & 0x3f;
	int fill_opacity = (data[1] >> 6) & 0x03;
	int border_color = (data[2]) & 0x3f;
	int border_type01 = (data[2] >> 6) & 0x03;
	int justify = (data[3]) & 0x03;
	int scroll_dir = (data[3] >> 2) & 0x03;
	int print_dir = (data[3] >> 4) & 0x03;
	int word_wrap = (data[3] >> 6) & 0x01;
	int border_type = ((data[3] >> 5) & 0x04) | border_type01;
	int display_eff = (data[4]) & 0x03;
	int effect_dir = (data[4] >> 2) & 0x03;
	int effect_speed = (data[4] >> 4) & 0x0f;

	ccx_common_logging.debug_ftn(CCX_DMT_708, "       Fill color: [%d]     Fill opacity: [%d]    Border color: [%d]  Border type: [%d]\n",
				     fill_color, fill_opacity, border_color, border_type01);
	ccx_common_logging.debug_ftn(CCX_DMT_708, "          Justify: [%d]       Scroll dir: [%d]       Print dir: [%d]    Word wrap: [%d]\n",
				     justify, scroll_dir, print_dir, word_wrap);
	ccx_common_logging.debug_ftn(CCX_DMT_708, "      Border type: [%d]      Display eff: [%d]      Effect dir: [%d] Effect speed: [%d]\n",
				     border_type, display_eff, effect_dir, effect_speed);

	if (decoder->current_window == -1)
	{
		ccx_common_logging.log_ftn("[CEA-708] dtvcc_handle_SWA_SetWindowAttributes: "
					   "Window has to be defined first\n");
		return;
	}

	ccx_dtvcc_window *window = &decoder->windows[decoder->current_window];

	window->attribs.fill_color = fill_color;
	window->attribs.fill_opacity = fill_opacity;
	window->attribs.border_color = border_color;
	window->attribs.justify = justify;
	window->attribs.scroll_direction = scroll_dir;
	window->attribs.print_direction = print_dir;
	window->attribs.word_wrap = word_wrap;
	window->attribs.border_type = border_type;
	window->attribs.display_effect = display_eff;
	window->attribs.effect_direction = effect_dir;
	window->attribs.effect_speed = effect_speed;
}

void dtvcc_handle_DLW_DeleteWindows(ccx_dtvcc_ctx *dtvcc,
				    ccx_dtvcc_service_decoder *decoder,
				    int windows_bitmap)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_DLW_DeleteWindows: windows: ");

	int screen_content_changed = 0, window_had_content = 0;
	// int current_win_deleted = 0; /* currently unused */

	if (windows_bitmap == 0)
		ccx_common_logging.debug_ftn(CCX_DMT_708, "none\n");
	else
	{
		for (int i = 0; i < CCX_DTVCC_MAX_WINDOWS; i++)
		{
			if (windows_bitmap & 1)
			{
				ccx_dtvcc_window *window = &decoder->windows[i];
				ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] Deleting [W-%d]\n", i);
				window_had_content = window->is_defined && window->visible && !window->is_empty;
				if (window_had_content)
				{
					screen_content_changed = 1;
					_dtvcc_window_update_time_hide(window, dtvcc->timing);
					_dtvcc_window_copy_to_screen(decoder, &decoder->windows[i]);
					if (i == decoder->current_window)
					{
						_dtvcc_screen_print(dtvcc, decoder);
					}
				}
				decoder->windows[i].is_defined = 0;
				decoder->windows[i].visible = 0;
				decoder->windows[i].time_ms_hide = -1;
				decoder->windows[i].time_ms_show = -1;
				if (i == decoder->current_window)
				{
					// If the current window is deleted, then the decoder's current window ID
					// is unknown and must be reinitialized with either the SetCurrentWindow
					// or DefineWindow command.
					decoder->current_window = -1;
				}
			}
			windows_bitmap >>= 1;
		}
	}
	ccx_common_logging.debug_ftn(CCX_DMT_708, "\n");
	if (screen_content_changed && !_dtvcc_decoder_has_visible_windows(decoder))
		_dtvcc_screen_print(dtvcc, decoder);
}

void dtvcc_handle_SPA_SetPenAttributes(ccx_dtvcc_service_decoder *decoder, unsigned char *data)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_SPA_SetPenAttributes: attributes: \n");

	int pen_size = (data[1]) & 0x3;
	int offset = (data[1] >> 2) & 0x3;
	int text_tag = (data[1] >> 4) & 0xf;
	int font_tag = (data[2]) & 0x7;
	int edge_type = (data[2] >> 3) & 0x7;
	int underline = (data[2] >> 6) & 0x1;
	int italic = (data[2] >> 7) & 0x1;

	ccx_common_logging.debug_ftn(CCX_DMT_708, "       Pen size: [%d]     Offset: [%d]  Text tag: [%d]   Font tag: [%d]\n",
				     pen_size, offset, text_tag, font_tag);
	ccx_common_logging.debug_ftn(CCX_DMT_708, "      Edge type: [%d]  Underline: [%d]    Italic: [%d]\n",
				     edge_type, underline, italic);

	if (decoder->current_window == -1)
	{
		ccx_common_logging.log_ftn("[CEA-708] dtvcc_handle_SPA_SetPenAttributes: "
					   "Window has to be defined first\n");
		return;
	}

	ccx_dtvcc_window *window = &decoder->windows[decoder->current_window];

	if (window->pen_row == -1)
	{
		ccx_common_logging.log_ftn("[CEA-708] dtvcc_handle_SPA_SetPenAttributes: "
					   "can't set pen attribs for undefined row\n");
		return;
	}

	ccx_dtvcc_pen_attribs *pen = &window->pen_attribs_pattern;

	pen->pen_size = pen_size;
	pen->offset = offset;
	pen->text_tag = text_tag;
	pen->font_tag = font_tag;
	pen->edge_type = edge_type;
	pen->underline = underline;
	pen->italic = italic;
}

void dtvcc_handle_SPC_SetPenColor(ccx_dtvcc_service_decoder *decoder, unsigned char *data)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_SPC_SetPenColor: attributes: \n");

	int fg_color = (data[1]) & 0x3f;
	int fg_opacity = (data[1] >> 6) & 0x03;
	int bg_color = (data[2]) & 0x3f;
	int bg_opacity = (data[2] >> 6) & 0x03;
	int edge_color = (data[3]) & 0x3f;

	ccx_common_logging.debug_ftn(CCX_DMT_708, "      Foreground color: [%d]     Foreground opacity: [%d]\n",
				     fg_color, fg_opacity);
	ccx_common_logging.debug_ftn(CCX_DMT_708, "      Background color: [%d]     Background opacity: [%d]\n",
				     bg_color, bg_opacity);
	ccx_common_logging.debug_ftn(CCX_DMT_708, "            Edge color: [%d]\n",
				     edge_color);

	if (decoder->current_window == -1)
	{
		ccx_common_logging.log_ftn("[CEA-708] dtvcc_handle_SPC_SetPenColor: "
					   "Window has to be defined first\n");
		return;
	}

	ccx_dtvcc_window *window = &decoder->windows[decoder->current_window];

	if (window->pen_row == -1)
	{
		ccx_common_logging.log_ftn("[CEA-708] dtvcc_handle_SPA_SetPenAttributes: "
					   "can't set pen color for undefined row\n");
		return;
	}

	ccx_dtvcc_pen_color *color = &window->pen_color_pattern;

	color->fg_color = fg_color;
	color->fg_opacity = fg_opacity;
	color->bg_color = bg_color;
	color->bg_opacity = bg_opacity;
	color->edge_color = edge_color;
}

void dtvcc_handle_SPL_SetPenLocation(ccx_dtvcc_service_decoder *decoder, unsigned char *data)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_SPL_SetPenLocation: attributes: \n");

	int row = data[1] & 0x0f;
	int col = data[2] & 0x3f;

	ccx_common_logging.debug_ftn(CCX_DMT_708, "      row: [%d]     Column: [%d]\n", row, col);

	if (decoder->current_window == -1)
	{
		ccx_common_logging.log_ftn("[CEA-708] dtvcc_handle_SPL_SetPenLocation: "
					   "Window has to be defined first\n");
		return;
	}

	ccx_dtvcc_window *window = &decoder->windows[decoder->current_window];
	window->pen_row = row;
	window->pen_column = col;
}

void dtvcc_handle_RST_Reset(ccx_dtvcc_service_decoder *decoder)
{
	ccx_dtvcc_windows_reset(decoder);
}

//------------------------- SYNCHRONIZATION COMMANDS -------------------------

void dtvcc_handle_DLY_Delay(ccx_dtvcc_service_decoder *decoder, int tenths_of_sec)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_DLY_Delay: "
						  "delay for [%d] tenths of second",
				     tenths_of_sec);
	// TODO: Probably ask for the current FTS and wait for this time before resuming - not sure it's worth it though
	// TODO: No, seems to me that idea above will not work
}

void dtvcc_handle_DLC_DelayCancel(ccx_dtvcc_service_decoder *decoder)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_DLC_DelayCancel");
	// TODO: See above
}

//-------------------------- CHARACTERS AND COMMANDS -------------------------

int _dtvcc_handle_C0_P16(ccx_dtvcc_service_decoder *decoder, unsigned char *data) //16-byte chars always have 2 bytes
{
	if (decoder->current_window == -1)
	{
		ccx_common_logging.log_ftn("[CEA-708] _dtvcc_handle_C0_P16: Window has to be defined first\n");
		return 3;
	}

	ccx_dtvcc_symbol sym;

	if (data[0])
	{
		CCX_DTVCC_SYM_SET_16(sym, data[0], data[1]);
	}
	else
	{
		CCX_DTVCC_SYM_SET(sym, data[1]);
	}

	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] _dtvcc_handle_C0_P16: [%04X]\n", sym.sym);
	_dtvcc_process_character(decoder, sym);

	return 3;
}

// G0 - Code Set - ASCII printable characters
int _dtvcc_handle_G0(ccx_dtvcc_service_decoder *decoder, unsigned char *data, int data_length)
{
	if (decoder->current_window == -1)
	{
		ccx_common_logging.log_ftn("[CEA-708] _dtvcc_handle_G0: Window has to be defined first\n");
		return data_length;
	}

	unsigned char c = data[0];
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] G0: [%02X]  (%c)\n", c, c);
	ccx_dtvcc_symbol sym;
	if (c == 0x7F)
	{ // musical note replaces the Delete command code in ASCII
		CCX_DTVCC_SYM_SET(sym, CCX_DTVCC_MUSICAL_NOTE_CHAR);
	}
	else
	{
		unsigned char uc = dtvcc_get_internal_from_G0(c);
		CCX_DTVCC_SYM_SET(sym, uc);
	}
	_dtvcc_process_character(decoder, sym);
	return 1;
}

// G1 Code Set - ISO 8859-1 LATIN-1 Character Set
int _dtvcc_handle_G1(ccx_dtvcc_service_decoder *decoder, unsigned char *data, int data_length)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] G1: [%02X]  (%c)\n", data[0], data[0]);
	unsigned char c = dtvcc_get_internal_from_G1(data[0]);
	ccx_dtvcc_symbol sym;
	CCX_DTVCC_SYM_SET(sym, c);
	_dtvcc_process_character(decoder, sym);
	return 1;
}

int _dtvcc_handle_C0(ccx_dtvcc_ctx *dtvcc,
		     ccx_dtvcc_service_decoder *decoder,
		     unsigned char *data,
		     int data_length)
{
	unsigned char c0 = data[0];
	const char *name = DTVCC_COMMANDS_C0[c0];
	if (name == NULL)
		name = "Reserved";

	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] C0: [%02X]  (%d)   [%s]\n", c0, data_length, name);

	int len = -1;
	// These commands have a known length even if they are reserved.
	if (c0 <= 0xF)
	{
		switch (c0)
		{
			case CCX_DTVCC_C0_NUL:
				// No idea what they use NUL for, specs say they come from ASCII,
				// ASCII say it's "do nothing"
				break;
			case CCX_DTVCC_C0_CR:
				_dtvcc_process_cr(dtvcc, decoder);
				break;
			case CCX_DTVCC_C0_HCR:
				_dtvcc_process_hcr(decoder);
				break;
			case CCX_DTVCC_C0_FF:
				_dtvcc_process_ff(decoder);
				break;
			case CCX_DTVCC_C0_ETX:
				_dtvcc_process_etx(decoder);
				break;
			case CCX_DTVCC_C0_BS:
				_dtvcc_process_bs(decoder);
				break;
			default:
				ccx_common_logging.log_ftn("[CEA-708] _dtvcc_handle_C0: unhandled branch\n");
				break;
		}
		len = 1;
	}
	else if (c0 >= 0x10 && c0 <= 0x17)
	{
		// Note that 0x10 is actually EXT1 and is dealt with somewhere else. Rest is undefined as per
		// CEA-708-D
		len = 2;
	}
	else if (c0 >= 0x18 && c0 <= 0x1F)
	{
		if (c0 == CCX_DTVCC_C0_P16) // PE16
			_dtvcc_handle_C0_P16(decoder, data + 1);
		len = 3;
	}
	if (len == -1)
	{
		ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] _dtvcc_handle_C0: impossible len == -1");
		return -1;
	}
	if (len > data_length)
	{
		ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] _dtvcc_handle_C0: "
							  "command is %d bytes long but we only have %d\n",
					     len, data_length);
		return -1;
	}
	return len;
}

// C1 Code Set - Captioning Commands Control Codes
int _dtvcc_handle_C1(ccx_dtvcc_ctx *dtvcc,
		     ccx_dtvcc_service_decoder *decoder,
		     unsigned char *data,
		     int data_length)
{
	struct CCX_DTVCC_S_COMMANDS_C1 com = DTVCC_COMMANDS_C1[data[0] - 0x80];
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] C1: %s | [%02X]  [%s] [%s] (%d)\n",
				     print_mstime_static(get_fts(dtvcc->timing, 3)),
				     data[0], com.name, com.description, com.length);

	if (com.length > data_length)
	{
		ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] C1: Warning: Not enough bytes for command.\n");
		return -1;
	}

	switch (com.code)
	{
		case CCX_DTVCC_C1_CW0: /* SetCurrentWindow */
		case CCX_DTVCC_C1_CW1:
		case CCX_DTVCC_C1_CW2:
		case CCX_DTVCC_C1_CW3:
		case CCX_DTVCC_C1_CW4:
		case CCX_DTVCC_C1_CW5:
		case CCX_DTVCC_C1_CW6:
		case CCX_DTVCC_C1_CW7:
			dtvcc_handle_CWx_SetCurrentWindow(decoder, com.code - CCX_DTVCC_C1_CW0); /* Window 0 to 7 */
			break;
		case CCX_DTVCC_C1_CLW:
			dtvcc_handle_CLW_ClearWindows(dtvcc, decoder, data[1]);
			break;
		case CCX_DTVCC_C1_DSW:
			dtvcc_handle_DSW_DisplayWindows(decoder, data[1], dtvcc->timing);
			break;
		case CCX_DTVCC_C1_HDW:
			dtvcc_handle_HDW_HideWindows(dtvcc, decoder, data[1]);
			break;
		case CCX_DTVCC_C1_TGW:
			dtvcc_handle_TGW_ToggleWindows(dtvcc, decoder, data[1]);
			break;
		case CCX_DTVCC_C1_DLW:
			dtvcc_handle_DLW_DeleteWindows(dtvcc, decoder, data[1]);
			break;
		case CCX_DTVCC_C1_DLY:
			dtvcc_handle_DLY_Delay(decoder, data[1]);
			break;
		case CCX_DTVCC_C1_DLC:
			dtvcc_handle_DLC_DelayCancel(decoder);
			break;
		case CCX_DTVCC_C1_RST:
			dtvcc_handle_RST_Reset(decoder);
			break;
		case CCX_DTVCC_C1_SPA:
			dtvcc_handle_SPA_SetPenAttributes(decoder, data);
			break;
		case CCX_DTVCC_C1_SPC:
			dtvcc_handle_SPC_SetPenColor(decoder, data);
			break;
		case CCX_DTVCC_C1_SPL:
			dtvcc_handle_SPL_SetPenLocation(decoder, data);
			break;
		case CCX_DTVCC_C1_RSV93:
		case CCX_DTVCC_C1_RSV94:
		case CCX_DTVCC_C1_RSV95:
		case CCX_DTVCC_C1_RSV96:
			ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] Warning, found Reserved codes, ignored.\n");
			break;
		case CCX_DTVCC_C1_SWA:
			dtvcc_handle_SWA_SetWindowAttributes(decoder, data);
			break;
		case CCX_DTVCC_C1_DF0:
		case CCX_DTVCC_C1_DF1:
		case CCX_DTVCC_C1_DF2:
		case CCX_DTVCC_C1_DF3:
		case CCX_DTVCC_C1_DF4:
		case CCX_DTVCC_C1_DF5:
		case CCX_DTVCC_C1_DF6:
		case CCX_DTVCC_C1_DF7:
			dtvcc_handle_DFx_DefineWindow(decoder, com.code - CCX_DTVCC_C1_DF0, data, dtvcc->timing); /* Window 0 to 7 */
			break;
		default:
			ccx_common_logging.log_ftn("[CEA-708] BUG: Unhandled code in _dtvcc_handle_C1.\n");
			break;
	}

	return com.length;
}

/* This function handles future codes. While by definition we can't do any work on them, we must return
   how many bytes would be consumed if these codes were supported, as defined in the specs.
Note: EXT1 not included */
// C2: Extended Miscellaneous Control Codes
// WARN: This code is completely untested due to lack of samples. Just following specs!
int _dtvcc_handle_C2(ccx_dtvcc_service_decoder *decoder, unsigned char *data, int data_length)
{
	if (data[0] <= 0x07)	  // 00-07...
		return 1;	  // ... Single-byte control bytes (0 additional bytes)
	else if (data[0] <= 0x0f) // 08-0F ...
		return 2;	  // ..two-byte control codes (1 additional byte)
	else if (data[0] <= 0x17) // 10-17 ...
		return 3;	  // ..three-byte control codes (2 additional bytes)
	return 4;		  // 18-1F => four-byte control codes (3 additional bytes)
}

int _dtvcc_handle_C3(ccx_dtvcc_service_decoder *decoder, unsigned char *data, int data_length)
{
	if (data[0] < 0x80 || data[0] > 0x9F)
		ccx_common_logging.fatal_ftn(
		    CCX_COMMON_EXIT_BUG_BUG, "[CEA-708] Entry in _dtvcc_handle_C3 with an out of range value.");
	if (data[0] <= 0x87)	  // 80-87...
		return 5;	  // ... Five-byte control bytes (4 additional bytes)
	else if (data[0] <= 0x8F) // 88-8F ...
		return 6;	  // ..Six-byte control codes (5 additional byte)
	// If here, then 90-9F ...

	// These are variable length commands, that can even span several segments
	// (they allow even downloading fonts or graphics).
	ccx_common_logging.fatal_ftn(
	    CCX_COMMON_EXIT_UNSUPPORTED, "[CEA-708] This sample contains unsupported 708 data. "
					 "PLEASE help us improve CCExtractor by submitting it.\n");
	return 0; // Unreachable, but otherwise there's compilers warnings
}

// This function handles extended codes (EXT1 + code), from the extended sets
// G2 (20-7F) => Mostly unmapped, except for a few characters.
// G3 (A0-FF) => A0 is the CC symbol, everything else reserved for future expansion in EIA708-B
// C2 (00-1F) => Reserved for future extended misc. control and captions command codes
// WARN: This code is completely untested due to lack of samples. Just following specs!
// Returns number of used bytes, usually 1 (since EXT1 is not counted).
int _dtvcc_handle_extended_char(ccx_dtvcc_service_decoder *decoder, unsigned char *data, int data_length)
{
	int used;
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] In _dtvcc_handle_extended_char, "
						  "first data code: [%c], length: [%u]\n",
				     data[0], data_length);
	unsigned char c = 0x20; // Default to space
	unsigned char code = data[0];
	if (/* data[i]>=0x00 && */ code <= 0x1F) // Comment to silence warning
	{
		used = _dtvcc_handle_C2(decoder, data, data_length);
	}
	// Group G2 - Extended Miscellaneous Characters
	else if (code >= 0x20 && code <= 0x7F)
	{
		c = dtvcc_get_internal_from_G2(code);
		used = 1;
		ccx_dtvcc_symbol sym;
		CCX_DTVCC_SYM_SET(sym, c);
		_dtvcc_process_character(decoder, sym);
	}
	// Group C3
	else if (code >= 0x80 && code <= 0x9F)
	{
		used = _dtvcc_handle_C3(decoder, data, data_length);
		// TODO: Something
	}
	// Group G3
	else
	{
		c = dtvcc_get_internal_from_G3(code);
		used = 1;
		ccx_dtvcc_symbol sym;
		CCX_DTVCC_SYM_SET(sym, c);
		_dtvcc_process_character(decoder, sym);
	}
	return used;
}

//------------------------------- PROCESSING --------------------------------

void ccx_dtvcc_process_service_block(ccx_dtvcc_ctx *dtvcc,
				     ccx_dtvcc_service_decoder *decoder,
				     unsigned char *data,
				     int data_length)
{
	//dump(CCX_DMT_708, data, data_length, 0, 0);

	int i = 0;
	while (i < data_length)
	{
		int used = -1;
		if (data[i] != CCX_DTVCC_C0_EXT1)
		{
			if (data[i] <= 0x1F)
				used = _dtvcc_handle_C0(dtvcc, decoder, data + i, data_length - i);
			else if (data[i] >= 0x20 && data[i] <= 0x7F)
				used = _dtvcc_handle_G0(decoder, data + i, data_length - i);
			else if (data[i] >= 0x80 && data[i] <= 0x9F)
				used = _dtvcc_handle_C1(dtvcc, decoder, data + i, data_length - i);
			else
				used = _dtvcc_handle_G1(decoder, data + i, data_length - i);

			if (used == -1)
			{
				ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] ccx_dtvcc_process_service_block: "
									  "There was a problem handling the data. Reseting service decoder\n");
				// TODO: Not sure if a local reset is going to be helpful here.
				//ccx_dtvcc_windows_reset(decoder);
				return;
			}
		}
		else // Use extended set
		{
			used = _dtvcc_handle_extended_char(decoder, data + i + 1, data_length - 1);
			used++; // Since we had CCX_DTVCC_C0_EXT1
		}
		i += used;
	}
}

void ccx_dtvcc_process_current_packet(ccx_dtvcc_ctx *dtvcc)
{
	int seq = (dtvcc->current_packet[0] & 0xC0) >> 6; // Two most significants bits
	int len = dtvcc->current_packet[0] & 0x3F;	  // 6 least significants bits
#ifdef DEBUG_708_PACKETS
	ccx_common_logging.log_ftn("[CEA-708] dtvcc_process_current_packet: length=%d, seq=%d\n",
				   dtvcc->current_packet_length, seq);
#endif
	if (dtvcc->current_packet_length == 0)
		return;
	if (len == 0) // This is well defined in EIA-708; no magic.
		len = 128;
	else
		len = len * 2;
		// Note that len here is the length including the header
#ifdef DEBUG_708_PACKETS
	ccx_common_logging.log_ftn("[CEA-708] dtvcc_process_current_packet: "
				   "Sequence: %d, packet length: %d\n",
				   seq, len);
#endif
	if (dtvcc->current_packet_length != len) // Is this possible?
	{
		_dtvcc_decoders_reset(dtvcc);
		return;
	}
	if (dtvcc->last_sequence != CCX_DTVCC_NO_LAST_SEQUENCE &&
	    (dtvcc->last_sequence + 1) % 4 != seq)
	{
		ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] ccx_dtvcc_process_current_packet: "
							  "Unexpected sequence number, it is [%d] but should be [%d]\n",
					     seq, (dtvcc->last_sequence + 1) % 4);
		//WARN: if we reset decoders here, buffer will not be written
		//WARN: resetting decoders breaks some samples
		//_dtvcc_decoders_reset(dtvcc);
		//return;
	}
	dtvcc->last_sequence = seq;

	unsigned char *pos = dtvcc->current_packet + 1;

	while (pos < dtvcc->current_packet + len)
	{
		int service_number = (pos[0] & 0xE0) >> 5; // 3 more significant bits
		int block_length = (pos[0] & 0x1F);	   // 5 less significant bits

		ccx_common_logging.debug_ftn(
		    CCX_DMT_708, "[CEA-708] ccx_dtvcc_process_current_packet: Standard header: "
				 "Service number: [%d] Block length: [%d]\n",
		    service_number, block_length);

		if (service_number == 7) // There is an extended header
		{
			pos++;
			service_number = (pos[0] & 0x3F); // 6 more significant bits
			// printf ("Extended header: Service number: [%d]\n",service_number);
			if (service_number < 7)
			{
				ccx_common_logging.debug_ftn(
				    CCX_DMT_708, "[CEA-708] ccx_dtvcc_process_current_packet: "
						 "Illegal service number in extended header: [%d]\n",
				    service_number);
			}
		}

		//		if (service_number == 0 && block_length == 0) // Null header already?
		//		{
		//			if (pos != (dtvcc->current_packet + len - 1)) // i.e. last byte in packet
		//			{
		//				// Not sure if this is correct
		//				printf ("Null header before it was expected.\n");
		//				// break;
		//			}
		//		}

		pos++;					      // Move to service data
		if (service_number == 0 && block_length != 0) // Illegal, but specs say what to do...
		{
			ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] ccx_dtvcc_process_current_packet: "
								  "Data received for service 0, skipping rest of packet.");
			pos = dtvcc->current_packet + len; // Move to end
			break;
		}

		if (block_length != 0)
		{
			dtvcc->report->services[service_number] = 1;
		}

		if (service_number > 0 && dtvcc->services_active[service_number - 1])
			ccx_dtvcc_process_service_block(dtvcc, &dtvcc->decoders[service_number - 1], pos, block_length);

		pos += block_length; // Skip data
	}

	ccx_dtvcc_clear_packet(dtvcc);

	if (pos != dtvcc->current_packet + len) // For some reason we didn't parse the whole packet
	{
		ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] ccx_dtvcc_process_current_packet:"
							  " There was a problem with this packet, reseting\n");
		_dtvcc_decoders_reset(dtvcc);
	}

	if (len < 128 && *pos) // Null header is mandatory if there is room
	{
		ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] ccx_dtvcc_process_current_packet: "
							  "Warning: Null header expected but not found.\n");
	}
}
