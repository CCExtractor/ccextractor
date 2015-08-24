#include "ccx_decoders_708.h"
#include "ccx_decoders_708_encoding.h"
#include "ccx_decoders_708_output.h"
#include "ccx_common_common.h"
#include "ccx_common_constants.h"
#include "ccx_common_structs.h"
#include "ccx_common_timing.h"
#include "lib_ccx.h"
#include "utility.h"

/* Portions by Daniel Kristjansson, extracted from MythTV's source */

const char *DTVCC_COMMANDS_C0[32] =
{
	"NUL", // 0 = NUL
	NULL,  // 1 = Reserved
	NULL,  // 2 = Reserved
	"ETX", // 3 = ETX
	NULL,  // 4 = Reserved
	NULL,  // 5 = Reserved
	NULL,  // 6 = Reserved
	NULL,  // 7 = Reserved
	"BS",  // 8 = Backspace
	NULL,  // 9 = Reserved
	NULL,  // A = Reserved
	NULL,  // B = Reserved
	"FF",  // C = FF
	"CR",  // D = CR
	"HCR", // E = HCR
	NULL,  // F = Reserved
	"EXT1",// 0x10 = EXT1,
	NULL,  // 0x11 = Reserved
	NULL,  // 0x12 = Reserved
	NULL,  // 0x13 = Reserved
	NULL,  // 0x14 = Reserved
	NULL,  // 0x15 = Reserved
	NULL,  // 0x16 = Reserved
	NULL,  // 0x17 = Reserved
	"P16", // 0x18 = P16
	NULL,  // 0x19 = Reserved
	NULL,  // 0x1A = Reserved
	NULL,  // 0x1B = Reserved
	NULL,  // 0x1C = Reserved
	NULL,  // 0x1D = Reserved
	NULL,  // 0x1E = Reserved
	NULL,  // 0x1F = Reserved
};

struct DTVCC_S_COMMANDS_C1 DTVCC_COMMANDS_C1[32] =
{
	{CW0, "CW0", "SetCurrentWindow0",     1},
	{CW1, "CW1", "SetCurrentWindow1",     1},
	{CW2, "CW2", "SetCurrentWindow2",     1},
	{CW3, "CW3", "SetCurrentWindow3",     1},
	{CW4, "CW4", "SetCurrentWindow4",     1},
	{CW5, "CW5", "SetCurrentWindow5",     1},
	{CW6, "CW6", "SetCurrentWindow6",     1},
	{CW7, "CW7", "SetCurrentWindow7",     1},
	{CLW, "CLW", "ClearWindows",          2},
	{DSW, "DSW", "DisplayWindows",        2},
	{HDW, "HDW", "HideWindows",           2},
	{TGW, "TGW", "ToggleWindows",         2},
	{DLW, "DLW", "DeleteWindows",         2},
	{DLY, "DLY", "Delay",                 2},
	{DLC, "DLC", "DelayCancel",           1},
	{RST, "RST", "Reset",                 1},
	{SPA, "SPA", "SetPenAttributes",      3},
	{SPC, "SPC", "SetPenColor",           4},
	{SPL, "SPL", "SetPenLocation",        3},
	{RSV93, "RSV93", "Reserved",          1},
	{RSV94, "RSV94", "Reserved",          1},
	{RSV95, "RSV95", "Reserved",          1},
	{RSV96, "RSV96", "Reserved",          1},
	{SWA, "SWA", "SetWindowAttributes",   5},
	{DF0, "DF0", "DefineWindow0",         7},
	{DF1, "DF1", "DefineWindow1",         7},
	{DF2, "DF2", "DefineWindow2",         7},
	{DF3, "DF3", "DefineWindow3",         7},
	{DF4, "DF4", "DefineWindow4",         7},
	{DF5, "DF5", "DefineWindow5",         7},
	{DF6, "DF6", "DefineWindow6",         7},
	{DF7, "DF7", "DefineWindow7",         7}
};


dtvcc_pen_color dtvcc_default_pen_color =
{
	0,
	0,
	0,
	0,
	0
};

dtvcc_pen_attribs dtvcc_default_pen_attribs =
{
	pensize_standard,
	0,
	texttag_undefined_12,
	0,
	edgetype_none,
	0,
	0
};

//---------------------------------- HELPERS ------------------------------------

void _dtvcc_clear_packet(ccx_dtvcc_ctx_t *ctx)
{
	ctx->current_packet_length = 0;
	memset(ctx->current_packet, 0, DTVCC_MAX_PACKET_LENGTH * sizeof(unsigned char));
}

void _dtvcc_tv_clear(dtvcc_service_decoder *decoder)
{
	for (int i = 0; i < DTVCC_SCREENGRID_ROWS; i++)
		memset(decoder->tv->chars[i], 0, DTVCC_SCREENGRID_COLUMNS * sizeof(ccx_dtvcc_symbol_t));
	decoder->tv->time_ms_show = -1;
	decoder->tv->time_ms_hide = -1;
};

int _dtvcc_decoder_has_visible_windows(dtvcc_service_decoder *decoder)
{
	for (int i = 0; i < DTVCC_MAX_WINDOWS; i++)
	{
		if (decoder->windows[i].visible)
			return 1;
	}
	return 0;
}

void _dtvcc_window_clear_row(dtvcc_window *window, int row_index)
{
	if (window->memory_reserved)
	{
		memset(window->rows[row_index], 0, DTVCC_MAX_COLUMNS * sizeof(ccx_dtvcc_symbol_t));
		window->pen_attribs[row_index] = dtvcc_default_pen_attribs;
		window->pen_colors[row_index] = dtvcc_default_pen_color;
	}
}

void _dtvcc_window_clear_text(dtvcc_window *window)
{
	for (int i = 0; i < DTVCC_MAX_ROWS; i++)
		_dtvcc_window_clear_row(window, i);
	window->is_empty = 1;
}

void _dtvcc_window_clear(dtvcc_service_decoder *decoder, int window_id)
{
	_dtvcc_window_clear_text(&decoder->windows[window_id]);
	//OPT fill window with a window fill color
}

#define DTVCC_PRINT_DEBUG
#ifdef DTVCC_PRINT_DEBUG

int _dtvcc_is_win_row_empty(dtvcc_window *window, int row_index)
{
	for (int j = 0; j < DTVCC_MAX_COLUMNS; j++)
	{
		if (CCX_DTVCC_SYM_IS_SET(window->rows[row_index][j]))
			return 0;
	}
	return 1;
}

void _dtvcc_get_win_write_interval(dtvcc_window *window, int row_index, int *first, int *last)
{
	for (*first = 0; *first < DTVCC_MAX_COLUMNS; (*first)++)
		if (CCX_DTVCC_SYM_IS_SET(window->rows[row_index][*first]))
			break;
	for (*last = DTVCC_MAX_COLUMNS - 1; *last > 0; (*last)--)
		if (CCX_DTVCC_SYM_IS_SET(window->rows[row_index][*last]))
			break;
}

void _dtvcc_window_dump(dtvcc_service_decoder *decoder, dtvcc_window *window)
{
	ccx_common_logging.debug_ftn(CCX_DMT_GENERIC_NOTICES, "[CEA-708] Window %d dump:\n", window->number);

	if (!window->is_defined)
		return;

	char tbuf1[SUBLINESIZE],
			tbuf2[SUBLINESIZE];

	print_mstime2buf(window->time_ms_show + decoder->subs_delay, tbuf1);
	print_mstime2buf(window->time_ms_hide + decoder->subs_delay, tbuf2);

	ccx_common_logging.debug_ftn(CCX_DMT_GENERIC_NOTICES, "\r%s --> %s\n", tbuf1, tbuf2);
	for (int i = 0; i < DTVCC_MAX_ROWS; i++)
	{
		if (!_dtvcc_is_win_row_empty(window, i))
		{
			int first, last;
			ccx_dtvcc_symbol_t sym;
			_dtvcc_get_win_write_interval(window, i, &first, &last);
			for (int j = first; j <= last; j++)
			{
				sym = window->rows[i][j];
				if (CCX_DTVCC_SYM_IS_16(sym))
					ccx_common_logging.debug_ftn(CCX_DMT_GENERIC_NOTICES, "%c",CCX_DTVCC_SYM(sym));
				else
					ccx_common_logging.debug_ftn(CCX_DMT_GENERIC_NOTICES, "[%02X %02X]",
												 CCX_DTVCC_SYM_16_FIRST(sym), CCX_DTVCC_SYM_16_SECOND(sym));
			}
			ccx_common_logging.debug_ftn(CCX_DMT_GENERIC_NOTICES, "\n");
		}
	}

	ccx_common_logging.debug_ftn(CCX_DMT_GENERIC_NOTICES, "[CEA-708] Dump done\n", window->number);
}

#endif

void _dtvcc_windows_reset(dtvcc_service_decoder *decoder)
{
	for (int j = 0; j < DTVCC_MAX_WINDOWS; j++)
	{
		_dtvcc_window_clear_text(&decoder->windows[j]);
		decoder->windows[j].is_defined = 0;
		decoder->windows[j].visible = 0;
		memset(decoder->windows[j].commands, 0, sizeof(decoder->windows[j].commands));
	}
	decoder->current_window = -1;

	decoder->tv = &decoder->tv1;
	_dtvcc_tv_clear(decoder);
	decoder->inited = 1;
}

void _dtvcc_decoders_reset(ccx_dtvcc_ctx_t *dtvcc)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] _dtvcc_decoders_reset: Resetting all decoders\n");

	for (int i = 0; i < DTVCC_MAX_SERVICES; i++)
	{
		if (!dtvcc->services_active[i])
			continue;
		_dtvcc_windows_reset(&dtvcc->decoders[i]);
	}

	_dtvcc_clear_packet(dtvcc);

	dtvcc->last_sequence = DTVCC_NO_LAST_SEQUENCE;
	dtvcc->report->reset_count++;
}

int _dtvcc_compare_win_priorities(const void *a, const void *b)
{
	dtvcc_window *w1 = *(dtvcc_window **)a;
	dtvcc_window *w2 = *(dtvcc_window **)b;
	return (w1->priority - w2->priority);
}

void _dtvcc_decoder_init_write(dtvcc_service_decoder *decoder, char *basefilename, int id)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] Initializing write %s\n", basefilename);
	char *ext = get_file_extension(decoder->output_format);

	size_t bfname_len = strlen(basefilename);
	size_t ext_len = strlen(ext);
	size_t temp_len = strlen(DTVCC_FILENAME_TEMPLATE); //seems to be enough

	decoder->filename = (char *) malloc(bfname_len + temp_len + ext_len + 1);
	if (!decoder->filename)
		ccx_common_logging.fatal_ftn(
				EXIT_NOT_ENOUGH_MEMORY, "[CEA-708] _dtvcc_decoder_init_write: not enough memory");

	sprintf(decoder->filename, DTVCC_FILENAME_TEMPLATE, basefilename, id);
	strcat(decoder->filename, ext);
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] inited %s\n", decoder->filename);

	free(ext);
}

void _dtvcc_window_update_time_show(dtvcc_window *window)
{
	char buf[128];
	window->time_ms_show = get_visible_start();
	print_mstime2buf(window->time_ms_show, buf);
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] "
			"[W-%d] show time updated to %s\n", window->number, buf);
}

void _dtvcc_window_update_time_hide(dtvcc_window *window)
{
	char buf[128];
	window->time_ms_hide = get_visible_end();
	print_mstime2buf(window->time_ms_hide, buf);
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] "
			"[W-%d] hide time updated to %s\n", window->number, buf);
}

void _dtvcc_screen_update_time_show(dtvcc_tv_screen *tv, LLONG time)
{
	char buf1[128], buf2[128];
	print_mstime2buf(tv->time_ms_show, buf1);
	print_mstime2buf(time, buf2);
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] "
			"Screen show time: %s -> %s\n", buf1, buf2);

	if (tv->time_ms_show == -1)
		tv->time_ms_show = time;
	else if (tv->time_ms_show > time)
		tv->time_ms_show = time;
}

void _dtvcc_screen_update_time_hide(dtvcc_tv_screen *tv, LLONG time)
{
	char buf1[128], buf2[128];
	print_mstime2buf(tv->time_ms_hide, buf1);
	print_mstime2buf(time, buf2);
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] "
			"Screen hide time: %s -> %s\n", buf1, buf2);

	if (tv->time_ms_hide == -1)
		tv->time_ms_hide = time;
	else if (tv->time_ms_hide < time)
		tv->time_ms_hide = time;
}

void _dtvcc_window_copy_to_screen(dtvcc_service_decoder *decoder, dtvcc_window *window)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] _dtvcc_window_copy_to_screen: W-%d\n", window->number);
	int top, left;
	// For each window we calculate the top, left position depending on the
	// anchor
	switch (window->anchor_point)
	{
		case anchorpoint_top_left:
			top = window->anchor_vertical;
			left = window->anchor_horizontal;
			break;
		case anchorpoint_top_center:
			top = window->anchor_vertical;
			left = window->anchor_horizontal - window->col_count / 2;
			break;
		case anchorpoint_top_right:
			top = window->anchor_vertical;
			left = window->anchor_horizontal - window->col_count;
			break;
		case anchorpoint_middle_left:
			top = window->anchor_vertical - window->row_count / 2;
			left = window->anchor_horizontal;
			break;
		case anchorpoint_middle_center:
			top = window->anchor_vertical - window->row_count / 2;
			left = window->anchor_horizontal - window->col_count / 2;
			break;
		case anchorpoint_middle_right:
			top = window->anchor_vertical - window->row_count / 2;
			left = window->anchor_horizontal - window->col_count;
			break;
		case anchorpoint_bottom_left:
			top = window->anchor_vertical - window->row_count;
			left = window->anchor_horizontal;
			break;
		case anchorpoint_bottom_center:
			top = window->anchor_vertical - window->row_count;
			left = window->anchor_horizontal - window->col_count / 2;
			break;
		case anchorpoint_bottom_right:
			top = window->anchor_vertical - window->row_count;
			left = window->anchor_horizontal - window->col_count;
			break;
		default: // Shouldn't happen, but skip the window just in case
			return;
			break;
	}
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] For window %d: Anchor point -> %d, size %d:%d, real position %d:%d\n",
								 window->number, window->anchor_point, window->row_count, window->col_count,
								 top, left
	);

	ccx_common_logging.debug_ftn(
			CCX_DMT_708, "[CEA-708] we have top [%d] and left [%d]\n", top, left);

	top = top < 0 ? 0 : top;
	left = left < 0 ? 0 : left;

	int copyrows = top + window->row_count >= DTVCC_SCREENGRID_ROWS ?
				   DTVCC_SCREENGRID_ROWS - top : window->row_count;
	int copycols = left + window->col_count >= DTVCC_SCREENGRID_COLUMNS ?
				   DTVCC_SCREENGRID_COLUMNS - left : window->col_count;

	ccx_common_logging.debug_ftn(
			CCX_DMT_708, "[CEA-708] %d*%d will be copied to the TV.\n", copyrows, copycols);

	for (int j = 0; j < copyrows; j++)
	{
		memcpy(decoder->tv->chars[top + j], window->rows[j], copycols * sizeof(ccx_dtvcc_symbol_t));
		decoder->tv->pen_attribs[top + j] = window->pen_attribs[j];
		decoder->tv->pen_colors[top + j] = window->pen_colors[j];
	}

	_dtvcc_screen_update_time_show(decoder->tv, window->time_ms_show);
	_dtvcc_screen_update_time_hide(decoder->tv, window->time_ms_hide);

#ifdef DTVCC_PRINT_DEBUG
	_dtvcc_window_dump(decoder, window);
#endif
}

void _dtvcc_screen_print(ccx_dtvcc_ctx_t *dtvcc, dtvcc_service_decoder *decoder)
{
	//TODO use priorities to solve windows overlap (with a video sample, please)
	//qsort(wnd, visible, sizeof(dtvcc_window *), _dtvcc_compare_win_priorities);

	_dtvcc_screen_update_time_hide(decoder->tv, get_visible_end());

#ifdef DTVCC_PRINT_DEBUG
	ccx_common_logging.debug_ftn(CCX_DMT_GENERIC_NOTICES, "[CEA-708] TV dump:\n");
	ccx_dtvcc_write_debug(decoder, dtvcc->encoder);
#endif

	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] _dtvcc_screen_print: printing screen tv\n");
	if (!decoder->output_started)
	{
		if (decoder->output_format != CCX_OF_NULL)
		{
			ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] "
					"_dtvcc_screen_print: creating %s\n", decoder->filename);
			decoder->fh = open(decoder->filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
			if (decoder->fh == -1)
			{
				ccx_common_logging.fatal_ftn(
						CCX_COMMON_EXIT_FILE_CREATION_FAILED, "[CEA-708] Failed to open a file\n");
			}
			if (!dtvcc->encoder->no_bom)
				write(decoder->fh, UTF8_BOM, sizeof(UTF8_BOM));
		}
		decoder->output_started = 1;
	}

	ccx_dtvcc_write(decoder, dtvcc->encoder);

	_dtvcc_tv_clear(decoder);
}

void _dtvcc_process_hcr(dtvcc_service_decoder *decoder)
{
	if (decoder->current_window == -1)
	{
		ccx_common_logging.log_ftn("[CEA-708] _dtvcc_process_hcr: Window has to be defined first\n");
		return;
	}

	dtvcc_window *window = &decoder->windows[decoder->current_window];
	window->pen_column = 0;
	_dtvcc_window_clear_row(window, window->pen_row);
}

void _dtvcc_process_ff(dtvcc_service_decoder *decoder)
{
	if (decoder->current_window == -1)
	{
		ccx_common_logging.log_ftn("[CEA-708] _dtvcc_process_ff: Window has to be defined first\n");
		return;
	}
	dtvcc_window *window = &decoder->windows[decoder->current_window];
	window->pen_column = 0;
	window->pen_row = 0;
	//CEA-708-D doesn't say we have to clear neither window text nor text line,
	//but it seems we have to clean the line
	//_dtvcc_window_clear_text(window);
}

void _dtvcc_process_etx(dtvcc_service_decoder *decoder)
{
	//it can help decoders with screen output, but could it help us?
}

void _dtvcc_window_rollup(dtvcc_service_decoder *decoder, dtvcc_window *window)
{
	for (int i = 0; i < window->row_count - 1; i++)
	{
		memcpy(window->rows[i], window->rows[i + 1], DTVCC_MAX_COLUMNS * sizeof(ccx_dtvcc_symbol_t));
		window->pen_colors[i] = window->pen_colors[i + 1];
		window->pen_attribs[i] = window->pen_attribs[i + 1];
	}

	_dtvcc_window_clear_row(window, window->row_count - 1);
}

void _dtvcc_process_cr(ccx_dtvcc_ctx_t *dtvcc, dtvcc_service_decoder *decoder)
{
	if (decoder->current_window == -1)
	{
		ccx_common_logging.log_ftn("[CEA-708] _dtvcc_process_cr: Window has to be defined first\n");
		return;
	}

	dtvcc_window *window = &decoder->windows[decoder->current_window];

	int rollup_required = 0;
	switch (window->attribs.print_dir)
	{
		case pd_left_to_right:
			window->pen_column = 0;
			if (window->pen_row + 1 < window->row_count)
				window->pen_row++;
			else rollup_required = 1;
			break;
		case pd_right_to_left:
			window->pen_column = window->col_count;
			if (window->pen_row + 1 < window->row_count)
				window->pen_row++;
			else rollup_required = 1;
			break;
		case pd_top_to_bottom:
			window->pen_row = 0;
			if (window->pen_column + 1 < window->col_count)
				window->pen_column++;
			else rollup_required = 1;
			break;
		case pd_bottom_to_top:
			window->pen_row = window->row_count;
			if (window->pen_column + 1 < window->col_count)
				window->pen_column++;
			else rollup_required = 1;
			break;
		default:
			ccx_common_logging.log_ftn("[CEA-708] _dtvcc_process_cr: unhandled branch\n");
			break;
	}

	if (window->is_defined)
	{
		ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] _dtvcc_process_cr: rolling up\n");

		_dtvcc_window_update_time_hide(window);
		_dtvcc_window_copy_to_screen(decoder, window);
		_dtvcc_screen_print(dtvcc, decoder);

		if (rollup_required)
		{
			if (ccx_options.settings_608.no_rollup)
				_dtvcc_window_clear_row(window, window->pen_row);
			else
				_dtvcc_window_rollup(decoder, window);
		}
		_dtvcc_window_update_time_show(window);
	}
}

void _dtvcc_process_character(dtvcc_service_decoder *decoder, ccx_dtvcc_symbol_t symbol)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] %d\n", decoder->current_window);
	int cw = decoder->current_window;
	dtvcc_window *window = &decoder->windows[cw];

	ccx_common_logging.debug_ftn(
			CCX_DMT_708, "[CEA-708] _dtvcc_process_character: "
					"%c [%02X]  - Window: %d %s, Pen: %d:%d\n",
			CCX_DTVCC_SYM(symbol), CCX_DTVCC_SYM(symbol),
			cw, window->is_defined ? "[OK]" : "[undefined]",
			cw != -1 ? window->pen_row : -1, cw != -1 ? window->pen_column : -1
	);

	if (cw == -1 || !window->is_defined) // Writing to a non existing window, skipping
		return;

	window->is_empty = 0;
	window->rows[window->pen_row][window->pen_column] = symbol;
	switch (window->attribs.print_dir)
	{
		case pd_left_to_right:
			if (window->pen_column + 1 < window->col_count)
				window->pen_column++;
			break;
		case pd_right_to_left:
			if (decoder->windows->pen_column > 0)
				window->pen_column--;
			break;
		case pd_top_to_bottom:
			if (window->pen_row + 1 < window->row_count)
				window->pen_row++;
			break;
		case pd_bottom_to_top:
			if (window->pen_row > 0)
				window->pen_row--;
			break;
		default:
			ccx_common_logging.log_ftn("[CEA-708] _dtvcc_process_character: unhandled branch\n");
			break;
	}
}

void _dtvcc_decoder_flush(ccx_dtvcc_ctx_t *dtvcc, dtvcc_service_decoder *decoder)
{
	ccx_common_logging.debug_ftn(
			CCX_DMT_708, "[CEA-708] _dtvcc_decoder_flush: Flushing decoder\n");
	int screen_content_changed = 0;
	for (int i = 0; i < DTVCC_MAX_WINDOWS; i++)
	{
		dtvcc_window *window = &decoder->windows[i];
		if (window->visible)
		{
			screen_content_changed = 1;
			_dtvcc_window_update_time_hide(window);
			_dtvcc_window_copy_to_screen(decoder, window);
			window->visible = 0;
		}
	}
	if (screen_content_changed)
		_dtvcc_screen_print(dtvcc, decoder);
}

//---------------------------------- COMMANDS ------------------------------------

void dtvcc_handle_CWx_SetCurrentWindow(dtvcc_service_decoder *decoder, int window_id)
{
	ccx_common_logging.debug_ftn(
			CCX_DMT_708, "[CEA-708] dtvcc_handle_CWx_SetCurrentWindow: [%d]\n", window_id);
	if (decoder->windows[window_id].is_defined)
		decoder->current_window = window_id;
	else
		ccx_common_logging.log_ftn("[CEA-708] dtvcc_handle_CWx_SetCurrentWindow: "
										   "window [%d] is not defined\n", window_id);
}

void dtvcc_handle_CLW_ClearWindows(dtvcc_service_decoder *decoder, int windows_bitmap)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_CLW_ClearWindows: windows: ");
	if (windows_bitmap == 0)
		ccx_common_logging.debug_ftn(CCX_DMT_708, "none\n");
	else
	{
		for (int i = 0; i < DTVCC_MAX_WINDOWS; i++)
		{
			if (windows_bitmap & 1)
			{
				ccx_common_logging.debug_ftn(CCX_DMT_708, "[W%d] ", i);
				_dtvcc_window_clear(decoder, i);
			}
			windows_bitmap >>= 1;
		}
	}
	ccx_common_logging.debug_ftn(CCX_DMT_708, "\n");
}

void dtvcc_handle_DSW_DisplayWindows(dtvcc_service_decoder *decoder, int windows_bitmap)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_DSW_DisplayWindows: windows: ");
	if (windows_bitmap == 0)
		ccx_common_logging.debug_ftn(CCX_DMT_708, "none\n");
	else
	{
		for (int i = 0; i < DTVCC_MAX_WINDOWS; i++)
		{
			if (windows_bitmap & 1)
			{
				ccx_common_logging.debug_ftn(CCX_DMT_708, "[Window %d] ", i);
				if (!decoder->windows[i].is_defined)
				{
					ccx_common_logging.log_ftn("[CEA-708] Error: window %d was not defined", i);
					continue;
				}
				if (!decoder->windows[i].visible)
				{
					decoder->windows[i].visible = 1;
					_dtvcc_window_update_time_show(&decoder->windows[i]);
				}
			}
			windows_bitmap >>= 1;
		}
		ccx_common_logging.debug_ftn(CCX_DMT_708, "\n");
	}
}

void dtvcc_handle_HDW_HideWindows(ccx_dtvcc_ctx_t *dtvcc, dtvcc_service_decoder *decoder, int windows_bitmap)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_HDW_HideWindows: windows: ");
	if (windows_bitmap == 0)
		ccx_common_logging.debug_ftn(CCX_DMT_708, "none\n");
	else
	{
		int screen_content_changed = 0;
		for (int i = 0; i < DTVCC_MAX_WINDOWS; i++)
		{
			if (windows_bitmap & 1)
			{
				ccx_common_logging.debug_ftn(CCX_DMT_708, "[Window %d] ", i);
				if (decoder->windows[i].visible)
				{
					screen_content_changed = 1;
					decoder->windows[i].visible = 0;
					_dtvcc_window_update_time_hide(&decoder->windows[i]);
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

void dtvcc_handle_TGW_ToggleWindows(ccx_dtvcc_ctx_t *dtvcc, dtvcc_service_decoder *decoder, int windows_bitmap)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_TGW_ToggleWindows: windows: ");
	if (windows_bitmap == 0)
		ccx_common_logging.debug_ftn(CCX_DMT_708, "none\n");
	else
	{
		int screen_content_changed = 0;
		for (int i = 0; i < DTVCC_MAX_WINDOWS; i++)
		{
			dtvcc_window *window = &decoder->windows[i];
			if ((windows_bitmap & 1) && window->is_defined)
			{
				ccx_common_logging.debug_ftn(CCX_DMT_708, "[W-%d: %d->%d]", i, window->visible, !window->visible);
				window->visible = !window->visible;
				if (window->visible)
					_dtvcc_window_update_time_show(window);
				else
				{
					_dtvcc_window_update_time_hide(window);
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

void dtvcc_handle_DFx_DefineWindow(dtvcc_service_decoder *decoder, int window_id, unsigned char *data)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_DFx_DefineWindow: "
			"W[%d], attributes: \n", window_id);

	dtvcc_window *window = &decoder->windows[window_id];

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
	int visible  = (data[1] >> 5) & 0x1;
	int anchor_vertical = data[2] & 0x7f;
	int relative_pos = data[2] >> 7;
	int anchor_horizontal = data[3];
	int row_count = (data[4] & 0xf) + 1; //according to CEA-708-D
	int anchor_point = data[4] >> 4;
	int col_count = (data[5] & 0x3f) + 1; //according to CEA-708-D
	int pen_style = data[6] & 0x7;
	int win_style = (data[6] >> 3) & 0x7;

	/**
	 * Korean samples have "anchor_vertical" and "anchor_horizontal" mixed up,
	 * this seems to be an encoder issue, but we can workaround it
	 */
	if (anchor_vertical > DTVCC_SCREENGRID_ROWS - row_count)
		anchor_vertical = DTVCC_SCREENGRID_ROWS - row_count;
	if (anchor_horizontal > DTVCC_SCREENGRID_COLUMNS - col_count)
		anchor_horizontal = DTVCC_SCREENGRID_COLUMNS - col_count;

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
	window->pen_style = pen_style;
	window->win_style = win_style;

	if (win_style == 0) {
		window->win_style = 1;
	}
	//TODO apply static win_style preset
	//TODO apply static pen_style preset

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

	if (!window->is_defined)
	{
		// If the window is being created, all character positions in the window
		// are set to the fill color and the pen location is set to (0,0)
		window->pen_column = 0;
		window->pen_row = 0;
		if (!window->memory_reserved)
		{
			for (int i = 0; i < DTVCC_MAX_ROWS; i++)
			{
				window->rows[i] = (ccx_dtvcc_symbol_t *) malloc(DTVCC_MAX_COLUMNS * sizeof(ccx_dtvcc_symbol_t));
				if (!window->rows[i])
					ccx_common_logging.fatal_ftn(EXIT_NOT_ENOUGH_MEMORY, "[CEA-708] dtvcc_handle_DFx_DefineWindow");
			}
			window->memory_reserved = 1;
		}
		window->is_defined = 1;
		_dtvcc_window_clear_text(window);
	}
	else
	{
		// Specs unclear here: Do we need to delete the text in the existing window?
		// We do this because one of the sample files demands it.
		_dtvcc_window_clear_text(window);
	}
	// ...also makes the defined windows the current window
	dtvcc_handle_CWx_SetCurrentWindow(decoder, window_id);
	memcpy(window->commands, data + 1, 6);

	if (window->visible)
		_dtvcc_window_update_time_show(window);
}

void dtvcc_handle_SWA_SetWindowAttributes(dtvcc_service_decoder *decoder, unsigned char *data)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_SWA_SetWindowAttributes: attributes: \n");

	int fill_color    = (data[1]     ) & 0x3f;
	int fill_opacity  = (data[1] >> 6) & 0x03;
	int border_color  = (data[2]     ) & 0x3f;
	int border_type01 = (data[2] >> 6) & 0x03;
	int justify       = (data[3]     ) & 0x03;
	int scroll_dir    = (data[3] >> 2) & 0x03;
	int print_dir     = (data[3] >> 4) & 0x03;
	int word_wrap     = (data[3] >> 6) & 0x01;
	int border_type   = (data[3] >> 5) | border_type01;
	int display_eff   = (data[4]     ) & 0x03;
	int effect_dir    = (data[4] >> 2) & 0x03;
	int effect_speed  = (data[4] >> 4) & 0x0f;

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

	dtvcc_window *window = &decoder->windows[decoder->current_window];

	window->attribs.fill_color = fill_color;
	window->attribs.fill_opacity = fill_opacity;
	window->attribs.border_color = border_color;
	window->attribs.border_type01 = border_type01;
	window->attribs.justify = justify;
	window->attribs.scroll_dir = scroll_dir;
	window->attribs.print_dir = print_dir;
	window->attribs.word_wrap = word_wrap;
	window->attribs.border_type = border_type;
	window->attribs.display_eff = display_eff;
	window->attribs.effect_dir = effect_dir;
	window->attribs.effect_speed = effect_speed;
}

void dtvcc_handle_DLW_DeleteWindows(ccx_dtvcc_ctx_t *dtvcc, dtvcc_service_decoder *decoder, int windows_bitmap)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_DLW_DeleteWindows: windows: ");

	int screen_content_changed = 0,
		window_had_content;
	if (windows_bitmap == 0)
		ccx_common_logging.debug_ftn(CCX_DMT_708, "none\n");
	else
	{
		for (int i = 0; i < DTVCC_MAX_WINDOWS; i++)
		{
			if (windows_bitmap & 1)
			{
				dtvcc_window *window = &decoder->windows[i];
				ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] Deleting [W-%d]\n", i);
				window_had_content = window->is_defined && window->visible && !window->is_empty;
				if (window_had_content)
				{
					screen_content_changed = 1;
					_dtvcc_window_update_time_hide(window);
					_dtvcc_window_copy_to_screen(decoder, &decoder->windows[i]);
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

void dtvcc_handle_SPA_SetPenAttributes(dtvcc_service_decoder *decoder, unsigned char *data)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_SPA_SetPenAttributes: attributes: \n");

	int pen_size  = (data[1]     ) & 0x3;
	int offset    = (data[1] >> 2) & 0x3;
	int text_tag  = (data[1] >> 4) & 0xf;
	int font_tag  = (data[2]     ) & 0x7;
	int edge_type = (data[2] >> 3) & 0x7;
	int underline = (data[2] >> 4) & 0x1;
	int italic    = (data[2] >> 5) & 0x1;

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

	dtvcc_window *window = &decoder->windows[decoder->current_window];

	if (window->pen_row == -1)
	{
		ccx_common_logging.log_ftn("[CEA-708] dtvcc_handle_SPA_SetPenAttributes: "
										   "can't set pen attribs for undefined row\n");
		return;
	}

	dtvcc_pen_attribs *pen = &window->pen_attribs[window->pen_row];

	pen->pen_size = pen_size;
	pen->offset = offset;
	pen->text_tag = text_tag;
	pen->font_tag = font_tag;
	pen->edge_type = edge_type;
	pen->underline = underline;
	pen->italic = italic;
}

void dtvcc_handle_SPC_SetPenColor(dtvcc_service_decoder *decoder, unsigned char *data)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_SPC_SetPenColor: attributes: \n");

	int fg_color   = (data[1]     ) & 0x3f;
	int fg_opacity = (data[1] >> 6) & 0x03;
	int bg_color   = (data[2]     ) & 0x3f;
	int bg_opacity = (data[2] >> 6) & 0x03;
	int edge_color = (data[3] >> 6) & 0x3f;

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

	dtvcc_window *window = &decoder->windows[decoder->current_window];

	if (window->pen_row == -1)
	{
		ccx_common_logging.log_ftn("[CEA-708] dtvcc_handle_SPA_SetPenAttributes: "
										   "can't set pen color for undefined row\n");
		return;
	}

	dtvcc_pen_color *color = &window->pen_colors[window->pen_row];

	color->fg_color = fg_color;
	color->fg_opacity = fg_opacity;
	color->bg_color = bg_color;
	color->bg_opacity = bg_opacity;
	color->edge_color = edge_color;
}

void dtvcc_handle_SPL_SetPenLocation(dtvcc_service_decoder *decoder, unsigned char *data)
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

	dtvcc_window *window = &decoder->windows[decoder->current_window];
	window->pen_row = row;
	window->pen_column = col;
}

void dtvcc_handle_RST_Reset(dtvcc_service_decoder *decoder)
{
	_dtvcc_windows_reset(decoder);
}

//------------------------- SYNCHRONIZATION COMMANDS -------------------------

void dtvcc_handle_DLY_Delay(dtvcc_service_decoder *decoder, int tenths_of_sec)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_DLY_Delay: "
			"delay for [%d] tenths of second", tenths_of_sec);
	// TODO: Probably ask for the current FTS and wait for this time before resuming - not sure it's worth it though
	// TODO: No, seems to me that idea above will not work
}

void dtvcc_handle_DLC_DelayCancel(dtvcc_service_decoder *decoder)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_DLC_DelayCancel");
	// TODO: See above
}

//-------------------------- CHARACTERS AND COMMANDS -------------------------

int _dtvcc_handle_C0_P16(dtvcc_service_decoder *decoder, unsigned char *data) //16-byte chars always have 2 bytes
{
	if (decoder->current_window == -1) {
		ccx_common_logging.log_ftn("[CEA-708] _dtvcc_handle_C0_P16: Window has to be defined first\n");
		return 3;
	}

	ccx_dtvcc_symbol_t sym;

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
int _dtvcc_handle_G0(dtvcc_service_decoder *decoder, unsigned char *data, int data_length)
{
	if (decoder->current_window == -1)
	{
		ccx_common_logging.log_ftn("[CEA-708] _dtvcc_handle_G0: Window has to be defined first\n");
		return data_length;
	}

	unsigned char c = data[0];
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] G0: [%02X]  (%c)\n", c, c);
	unsigned char uc = dtvcc_get_internal_from_G0(c);
	ccx_dtvcc_symbol_t sym;
	CCX_DTVCC_SYM_SET(sym, uc);
	_dtvcc_process_character(decoder, sym);
	return 1;
}

// G1 Code Set - ISO 8859-1 LATIN-1 Character Set
int _dtvcc_handle_G1(dtvcc_service_decoder *decoder, unsigned char *data, int data_length)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] G1: [%02X]  (%c)\n", data[0], data[0]);
	unsigned char c = dtvcc_get_internal_from_G1(data[0]);
	ccx_dtvcc_symbol_t sym;
	CCX_DTVCC_SYM_SET(sym, c);
	_dtvcc_process_character(decoder, sym);
	return 1;
}

int _dtvcc_handle_C0(ccx_dtvcc_ctx_t *dtvcc, dtvcc_service_decoder *decoder, unsigned char *data, int data_length)
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
			case 0x0d: //CR
				_dtvcc_process_cr(dtvcc, decoder);
				break;
			case 0x0e: // HCR (Horizontal Carriage Return)
				_dtvcc_process_hcr(decoder);
				break;
			case 0x0c: // FF (Form Feed)
				_dtvcc_process_ff(decoder);
				break;
			case 0x03: // ETX (service symbol, terminates segment)
				_dtvcc_process_etx(decoder);
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
		if (c0 == 0x18) // PE16
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
				"command is %d bytes long but we only have %d\n", len, data_length);
		return -1;
	}
	return len;
}

// C1 Code Set - Captioning Commands Control Codes
int _dtvcc_handle_C1(ccx_dtvcc_ctx_t *dtvcc, dtvcc_service_decoder *decoder, unsigned char *data, int data_length)
{
	struct DTVCC_S_COMMANDS_C1 com = DTVCC_COMMANDS_C1[data[0] - 0x80];
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] C1: %s | [%02X]  [%s] [%s] (%d)\n",
			print_mstime(get_fts()),
			data[0], com.name, com.description, com.length);

	if (com.length > data_length)
	{
		ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] C1: Warning: Not enough bytes for command.\n");
		return -1;
	}

	switch (com.code)
	{
		case CW0: /* SetCurrentWindow */
		case CW1:
		case CW2:
		case CW3:
		case CW4:
		case CW5:
		case CW6:
		case CW7:
			dtvcc_handle_CWx_SetCurrentWindow(decoder, com.code - CW0); /* Window 0 to 7 */
			break;
		case CLW:
			dtvcc_handle_CLW_ClearWindows(decoder, data[1]);
			break;
		case DSW:
			dtvcc_handle_DSW_DisplayWindows(decoder, data[1]);
			break;
		case HDW:
			dtvcc_handle_HDW_HideWindows(dtvcc, decoder, data[1]);
			break;
		case TGW:
			dtvcc_handle_TGW_ToggleWindows(dtvcc, decoder, data[1]);
			break;
		case DLW:
			dtvcc_handle_DLW_DeleteWindows(dtvcc, decoder, data[1]);
			break;
		case DLY:
			dtvcc_handle_DLY_Delay(decoder, data[1]);
			break;
		case DLC:
			dtvcc_handle_DLC_DelayCancel(decoder);
			break;
		case RST:
			dtvcc_handle_RST_Reset(decoder);
			break;
		case SPA:
			dtvcc_handle_SPA_SetPenAttributes(decoder, data);
			break;
		case SPC:
			dtvcc_handle_SPC_SetPenColor(decoder, data);
			break;
		case SPL:
			dtvcc_handle_SPL_SetPenLocation(decoder, data);
			break;
		case RSV93:
		case RSV94:
		case RSV95:
		case RSV96:
			ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] Warning, found Reserved codes, ignored.\n");
			break;
		case SWA:
			dtvcc_handle_SWA_SetWindowAttributes(decoder, data);
			break;
		case DF0:
		case DF1:
		case DF2:
		case DF3:
		case DF4:
		case DF5:
		case DF6:
		case DF7:
			dtvcc_handle_DFx_DefineWindow(decoder, com.code - DF0, data); /* Window 0 to 7 */
			break;
		default:
			ccx_common_logging.log_ftn ("[CEA-708] BUG: Unhandled code in _dtvcc_handle_C1.\n");
			break;
	}

	return com.length;
}

/* This function handles future codes. While by definition we can't do any work on them, we must return
   how many bytes would be consumed if these codes were supported, as defined in the specs.
Note: EXT1 not included */
// C2: Extended Miscellaneous Control Codes
// WARN: This code is completely untested due to lack of samples. Just following specs!
int _dtvcc_handle_C2(dtvcc_service_decoder *decoder, unsigned char *data, int data_length)
{
	if (data[0] <= 0x07) // 00-07...
		return 1; // ... Single-byte control bytes (0 additional bytes)
	else if (data[0] <= 0x0f) // 08-0F ...
		return 2; // ..two-byte control codes (1 additional byte)
	else if (data[0] <= 0x17)  // 10-17 ...
		return 3; // ..three-byte control codes (2 additional bytes)
	return 4; // 18-1F => four-byte control codes (3 additional bytes)
}

int _dtvcc_handle_C3(dtvcc_service_decoder *decoder, unsigned char *data, int data_length)
{
	if (data[0] < 0x80 || data[0] > 0x9F)
		ccx_common_logging.fatal_ftn(
				CCX_COMMON_EXIT_BUG_BUG, "[CEA-708] Entry in _dtvcc_handle_C3 with an out of range value.");
	if (data[0] <= 0x87) // 80-87...
		return 5; // ... Five-byte control bytes (4 additional bytes)
	else if (data[0] <= 0x8F) // 88-8F ...
		return 6; // ..Six-byte control codes (5 additional byte)
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
int _dtvcc_handle_extended_char(dtvcc_service_decoder *decoder, unsigned char *data, int data_length)
{
	int used;
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] In _dtvcc_handle_extended_char, "
			"first data code: [%c], length: [%u]\n", data[0], data_length);
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
		ccx_dtvcc_symbol_t sym;
		CCX_DTVCC_SYM_SET(sym, c);
		_dtvcc_process_character(decoder, sym);
	}
		// Group C3
	else if (code>= 0x80 && code <= 0x9F)
	{
		used = _dtvcc_handle_C3(decoder, data, data_length);
		// TODO: Something
	}
		// Group G3
	else
	{
		c = dtvcc_get_internal_from_G3(code);
		used = 1;
		ccx_dtvcc_symbol_t sym;
		CCX_DTVCC_SYM_SET(sym, c);
		_dtvcc_process_character(decoder, sym);
	}
	return used;
}

//------------------------------- PROCESSING --------------------------------

void dtvcc_process_service_block(ccx_dtvcc_ctx_t *dtvcc,
								 dtvcc_service_decoder *decoder,
								 unsigned char *data,
								 int data_length)
{
	//dump(CCX_DMT_708, data, data_length, 0, 0);

	int i = 0;
	while (i < data_length)
	{
		int used = -1;
		if (data[i] != EXT1)
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
				ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_process_service_block: "
						"There was a problem handling the data. Reseting service decoder\n");
				// TODO: Not sure if a local reset is going to be helpful here.
				//_dtvcc_windows_reset(decoder);
				return;
			}
		}
		else // Use extended set
		{
			used = _dtvcc_handle_extended_char(decoder, data + i + 1, data_length - 1);
			used++; // Since we had EXT1
		}
		i += used;
	}
}

void dtvcc_process_current_packet(ccx_dtvcc_ctx_t *dtvcc)
{
	int seq = (dtvcc->current_packet[0] & 0xC0) >> 6; // Two most significants bits
	int len = dtvcc->current_packet[0] & 0x3F; // 6 least significants bits
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
									   "Sequence: %d, packet length: %d\n", seq, len);
#endif
	if (dtvcc->current_packet_length != len) // Is this possible?
	{
		_dtvcc_decoders_reset(dtvcc);
		return;
	}
	if (dtvcc->last_sequence != DTVCC_NO_LAST_SEQUENCE &&
			(dtvcc->last_sequence + 1) % 4 != seq)
	{
		ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_process_current_packet: "
											 "Unexpected sequence number, it is [%d] but should be [%d]\n",
				seq, (dtvcc->last_sequence + 1 ) % 4);
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
		int block_length = (pos[0] & 0x1F); // 5 less significant bits

		ccx_common_logging.debug_ftn(
				CCX_DMT_708, "[CEA-708] dtvcc_process_current_packet: Standard header: "
						"Service number: [%d] Block length: [%d]\n", service_number, block_length);

		if (service_number == 7) // There is an extended header
		{
			pos++;
			service_number = (pos[0] & 0x3F); // 6 more significant bits
			// printf ("Extended header: Service number: [%d]\n",service_number);
			if (service_number < 7)
			{
				ccx_common_logging.debug_ftn(
						CCX_DMT_708, "[CEA-708] dtvcc_process_current_packet: "
						"Illegal service number in extended header: [%d]\n", service_number);
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

		pos++; // Move to service data
		if (service_number == 0 && block_length != 0) // Illegal, but specs say what to do...
		{
			ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_process_current_packet: "
					"Data received for service 0, skipping rest of packet.");
			pos = dtvcc->current_packet + len; // Move to end
			break;
		}

		if (block_length != 0)
		{
			dtvcc->report->services[service_number] = 1;
		}

		if (service_number > 0 && dtvcc->decoders[service_number - 1].inited)
			dtvcc_process_service_block(dtvcc, &dtvcc->decoders[service_number - 1], pos, block_length);

		pos += block_length; // Skip data
	}

	_dtvcc_clear_packet(dtvcc);

	if (pos != dtvcc->current_packet + len) // For some reason we didn't parse the whole packet
	{
		ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_process_current_packet:"
				" There was a problem with this packet, reseting\n");
		_dtvcc_decoders_reset(dtvcc);
	}

	if (len < 128 && *pos) // Null header is mandatory if there is room
	{
		ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_process_current_packet: "
				"Warning: Null header expected but not found.\n");
	}
}
