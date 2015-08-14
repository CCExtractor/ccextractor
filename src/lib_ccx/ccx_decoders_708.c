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

ccx_dtvcc_ctx_t ccx_dtvcc_ctx;

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
	{DF1, "DF0", "DefineWindow1",         7},
	{DF2, "DF0", "DefineWindow2",         7},
	{DF3, "DF0", "DefineWindow3",         7},
	{DF4, "DF0", "DefineWindow4",         7},
	{DF5, "DF0", "DefineWindow5",         7},
	{DF6, "DF0", "DefineWindow6",         7},
	{DF7, "DF0", "DefineWindow7",         7}
};

//---------------------------------- HELPERS ------------------------------------

void _dtvcc_clear_packet(void)
{
	ccx_dtvcc_ctx.current_packet_length = 0;
	memset(ccx_dtvcc_ctx.current_packet, 0, DTVCC_MAX_PACKET_LENGTH * sizeof(unsigned char));
}

void _dtvcc_tv_clear(dtvcc_service_decoder *decoder, int buffer_index) // Buffer => 1 or 2
{
	dtvcc_tv_screen *tv = (buffer_index == 1) ? &decoder->tv1 : &decoder->tv2;
	for (int i = 0; i < DTVCC_SCREENGRID_ROWS; i++)
	{
		memset(tv->chars[i], ' ', DTVCC_SCREENGRID_COLUMNS);
		tv->chars[i][DTVCC_SCREENGRID_COLUMNS] = 0;
	}
};

void _dtvcc_windows_reset(dtvcc_service_decoder *decoder)
{
	// There's lots of other stuff that we need to do, such as canceling delays
	for (int j = 0; j < DTVCC_MAX_WINDOWS; j++)
	{
		decoder->windows[j].is_defined = 0;
		decoder->windows[j].visible = 0;
		decoder->windows[j].memory_reserved = 0;
		decoder->windows[j].is_empty = 1;
		memset(decoder->windows[j].commands, 0, sizeof(decoder->windows[j].commands));
	}
	decoder->current_window = -1;
	decoder->current_visible_start_ms = 0;

	_dtvcc_tv_clear(decoder, 1);
	_dtvcc_tv_clear(decoder, 2);

	decoder->tv = &decoder->tv1;
	//TODO: decoder->cur_tv = 1; ?
	decoder->inited = 1;
}

void _dtvcc_decoders_reset(struct lib_cc_decode *ctx)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] _dtvcc_decoders_reset: Resetting all decoders\n");

	for (int i = 0; i < DTVCC_MAX_SERVICES; i++)
	{
		if (!ccx_dtvcc_ctx.services_active[i])
			continue;
		ccx_dtvcc_ctx.decoders[i].output_format = ctx->write_format;
		ccx_dtvcc_ctx.decoders[i].subs_delay = ctx->subs_delay;
		_dtvcc_windows_reset(&ccx_dtvcc_ctx.decoders[i]);
	}

	_dtvcc_clear_packet();

	ccx_dtvcc_ctx.last_sequence = DTVCC_NO_LAST_SEQUENCE;
	ccx_dtvcc_ctx.reset_count++;
}

int _dtvcc_compare_win_priorities(const void *a, const void *b)
{
	dtvcc_window *w1 = *(dtvcc_window **)a;
	dtvcc_window *w2 = *(dtvcc_window **)b;
	return (w1->priority - w2->priority);
}

void _dtvcc_decoder_init_write(struct lib_ccx_ctx *ctx, dtvcc_service_decoder *decoder, int id)
{
	decoder->output_format = ctx->write_format;
	char *ext = get_file_extension(decoder->output_format);

	size_t bfname_len = strlen(ctx->basefilename);
	size_t ext_len = strlen(ext);
	size_t temp_len = strlen(DTVCC_FILENAME_TEMPLATE); //seems to be enough

	decoder->filename = (char *) malloc(bfname_len + temp_len + ext_len + 1);
	if (!decoder->filename)
		ccx_common_logging.fatal_ftn(
				EXIT_NOT_ENOUGH_MEMORY, "[CEA-708] _dtvcc_decoder_init_write: not enough memory");

	sprintf(decoder->filename, DTVCC_FILENAME_TEMPLATE, ctx->basefilename, id);
	strcat(decoder->filename, ext);

	free(ext);
}

void _dtvcc_screen_toggle(dtvcc_service_decoder *decoder, int tv_id)
{
	_dtvcc_tv_clear(decoder, tv_id);
	decoder->cur_tv = tv_id;
	decoder->tv = (tv_id == 1 ? &decoder->tv1 : &decoder->tv2);
}

void _dtvcc_screen_write(dtvcc_service_decoder *decoder, int tv_id)
{
	if (!decoder->output_started)
	{
		if (decoder->output_format != CCX_OF_NULL)
		{
			ccx_common_logging.log_ftn("[CEA-708] _dtvcc_screen_write: creating %s\n", decoder->filename);
			decoder->fh = open(decoder->filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
			if (decoder->fh == -1)
			{
				ccx_common_logging.fatal_ftn(
						CCX_COMMON_EXIT_FILE_CREATION_FAILED, "[CEA-708] Failed to open a file\n");
			}
		}
		decoder->output_started = 1;
	}

	switch (decoder->output_format)
	{
		case CCX_OF_NULL:
			break;
		case CCX_OF_SRT:
			printTVtoSRT(decoder, tv_id);
			break;
		case CCX_OF_TRANSCRIPT:
			break;
		default:
			printTVtoConsole(decoder, tv_id);
			break;
	}
}

void _dtvcc_screen_update(dtvcc_service_decoder *decoder)
{
	// Print the previous screenful, which wasn't possible because we had no timing info
	int toggled_id = (decoder->cur_tv == 1 ? 2 : 1);
	_dtvcc_screen_write(decoder, toggled_id);
	_dtvcc_screen_toggle(decoder, toggled_id);

	// THIS FUNCTION WILL DO THE MAGIC OF ACTUALLY EXPORTING THE DECODER STATUS
	// TO SEVERAL FILES
	dtvcc_window *wnd[DTVCC_MAX_WINDOWS]; // We'll store here the visible windows that contain anything
	size_t visible = 0;
	for (int i = 0; i < DTVCC_MAX_WINDOWS; i++)
	{
		if (decoder->windows[i].is_defined && decoder->windows[i].visible && !decoder->windows[i].is_empty)
			wnd[visible++] = &decoder->windows[i];
	}
	qsort(wnd, visible, sizeof(dtvcc_window *), _dtvcc_compare_win_priorities);
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] Visible (and populated) windows in priority order: ");
	for (int i = 0; i < visible; i++)
	{
		ccx_common_logging.debug_ftn(CCX_DMT_708, "%d (%d) | ", wnd[i]->number, wnd[i]->priority);
	}
	ccx_common_logging.debug_ftn(CCX_DMT_708, "\n");
	for (int i = 0; i < visible; i++)
	{
		int top, left;
		// For each window we calculate the top, left position depending on the
		// anchor
		switch (wnd[i]->anchor_point)
		{
			case anchorpoint_top_left:
				top = wnd[i]->anchor_vertical;
				left = wnd[i]->anchor_horizontal;
				break;
			case anchorpoint_top_center:
				top = wnd[i]->anchor_vertical;
				left = wnd[i]->anchor_horizontal - wnd[i]->col_count / 2;
				break;
			case anchorpoint_top_right:
				top = wnd[i]->anchor_vertical;
				left = wnd[i]->anchor_horizontal - wnd[i]->col_count;
				break;
			case anchorpoint_middle_left:
				top = wnd[i]->anchor_vertical - wnd[i]->row_count / 2;
				left = wnd[i]->anchor_horizontal;
				break;
			case anchorpoint_middle_center:
				top = wnd[i]->anchor_vertical - wnd[i]->row_count / 2;
				left = wnd[i]->anchor_horizontal - wnd[i]->col_count / 2;
				break;
			case anchorpoint_middle_right:
				top = wnd[i]->anchor_vertical - wnd[i]->row_count / 2;
				left = wnd[i]->anchor_horizontal - wnd[i]->col_count;
				break;
			case anchorpoint_bottom_left:
				top = wnd[i]->anchor_vertical - wnd[i]->row_count;
				left = wnd[i]->anchor_horizontal;
				break;
			case anchorpoint_bottom_center:
				top = wnd[i]->anchor_vertical - wnd[i]->row_count;
				left = wnd[i]->anchor_horizontal - wnd[i]->col_count / 2;
				break;
			case anchorpoint_bottom_right:
				top = wnd[i]->anchor_vertical - wnd[i]->row_count;
				left = wnd[i]->anchor_horizontal - wnd[i]->col_count;
				break;
			default: // Shouldn't happen, but skip the window just in case
				continue;
		}
		ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] For window %d: Anchor point -> %d, size %d:%d, real position %d:%d\n",
				wnd[i]->number, wnd[i]->anchor_point, wnd[i]->row_count, wnd[i]->col_count,
				top, left
		);

		top = top < 0 ? 0 : top;
		left = left < 0 ? 0 : left;

		int copyrows = top + wnd[i]->row_count >= DTVCC_SCREENGRID_ROWS ?
			DTVCC_SCREENGRID_ROWS - top : wnd[i]->row_count;
		int copycols = left + wnd[i]->col_count >= DTVCC_SCREENGRID_COLUMNS ?
			DTVCC_SCREENGRID_COLUMNS - left : wnd[i]->col_count;

		ccx_common_logging.debug_ftn(
				CCX_DMT_708, "[CEA-708] %d*%d will be copied to the TV.\n", copyrows, copycols);

		for (int j = 0; j < copyrows; j++)
		{
			memcpy(decoder->tv->chars[top + j], wnd[i]->rows[j], copycols * sizeof(unsigned char));
		}
	}
	decoder->current_visible_start_ms = get_visible_start();
}

void _dtvcc_process_cr(dtvcc_service_decoder *decoder)
{
	dtvcc_window *window = &decoder->windows[decoder->current_window];
	switch (window->attribs.print_dir)
	{
		case pd_left_to_right:
			window->pen_column = 0;
			if (window->pen_row + 1 < window->row_count)
				window->pen_row++;
			break;
		case pd_right_to_left:
			window->pen_column = window->col_count;
			if (window->pen_row + 1 < window->row_count)
				window->pen_row++;
			break;
		case pd_top_to_bottom:
			window->pen_row = 0;
			if (window->pen_column + 1 < window->col_count)
				window->pen_column++;
			break;
		case pd_bottom_to_top:
			window->pen_row = window->row_count;
			if (window->pen_column + 1 < window->col_count)
				window->pen_column++;
			break;
		default:
			ccx_common_logging.log_ftn("[CEA-708] _dtvcc_process_cr: unhandled branch\n");
			break;
	}
}

void _dtvcc_process_character(dtvcc_service_decoder *decoder, unsigned char internal_char)
{
	int cw = decoder->current_window;
	dtvcc_window *window = &decoder->windows[cw];

	ccx_common_logging.debug_ftn(
			CCX_DMT_708, "[CEA-708] _dtvcc_process_character: "
					"%c [%02X]  - Window: %d %s, Pen: %d:%d\n",
			internal_char, internal_char,
			cw, window->is_defined ? "[OK]" : "[undefined]",
			cw != -1 ? window->pen_row : -1, cw != -1 ? window->pen_column : -1
	);

	if (cw == -1 || !window->is_defined) // Writing to a non existing window, skipping
		return;

	switch (internal_char)
	{
		default:
			window->is_empty = 0;
			window->rows[window->pen_row][window->pen_column] = internal_char;
			/* Not positive this interpretation is correct. Word wrapping is optional, so
			   let's assume we don't need to autoscroll */
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
			break;
	}

}

//----------------------------- WINDOW HELPERS -----------------------------------

void _dtvcc_window_clear(dtvcc_service_decoder *decoder, int window_idx)
{
	// TODO: Removes any existing text from the specified window. When a window
	// is cleared the entire window is filled with the window fill color.
}

void _dtvcc_window_clear_text(dtvcc_window *window)
{
	for (int i = 0; i < DTVCC_MAX_ROWS; i++)
	{
		memset(window->rows[i], ' ', DTVCC_MAX_COLUMNS);
		window->rows[i][DTVCC_MAX_COLUMNS] = 0;
	}
	memset(window->rows[DTVCC_MAX_ROWS], 0, DTVCC_MAX_COLUMNS + 1);
	window->is_empty = 1;
}

void _dtvcc_window_delete(dtvcc_service_decoder *decoder, int window_idx)
{
	if (window_idx == decoder->current_window)
	{
		// If the current window is deleted, then the decoder's current window ID
		// is unknown and must be reinitialized with either the SetCurrentWindow
		// or DefineWindow command.
		decoder->current_window = -1;
	}
	// TODO: Do the actual deletion (remove from display if needed, etc), mark as not defined, etc
	if (decoder->windows[window_idx].is_defined)
	{
		_dtvcc_window_clear_text(&decoder->windows[window_idx]);
	}
	decoder->windows[window_idx].is_defined = 0;
}

//---------------------------------- COMMANDS ------------------------------------

void dtvcc_handle_CWx_SetCurrentWindow(dtvcc_service_decoder *decoder, int new_window)
{
	ccx_common_logging.debug_ftn(
			CCX_DMT_708, "[CEA-708] dtvcc_handle_CWx_SetCurrentWindow: new window: [%d]\n", new_window);
	if (decoder->windows[new_window].is_defined)
		decoder->current_window = new_window;
	else
		ccx_common_logging.log_ftn(
				"[CEA-708] dtvcc_handle_CWx_SetCurrentWindow: window not defined [%d]", new_window);
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
				ccx_common_logging.debug_ftn(CCX_DMT_708, "[Window %d] ", i);
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
		int changed = 0;
		for (int i = 0; i < DTVCC_MAX_WINDOWS; i++)
		{
			if (windows_bitmap & 1)
			{
				ccx_common_logging.debug_ftn(CCX_DMT_708, "[Window %d] ", i);
				if (!decoder->windows[i].visible)
				{
					changed = 1;
					decoder->windows[i].visible = 1;
				}
			}
			windows_bitmap >>= 1;
		}
		ccx_common_logging.debug_ftn(CCX_DMT_708, "\n");
		if (changed)
			_dtvcc_screen_update(decoder);
	}
}

void dtvcc_handle_HDW_HideWindows(dtvcc_service_decoder *decoder, int windows_bitmap)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_HDW_HideWindows: windows: ");
	if (windows_bitmap == 0)
		ccx_common_logging.debug_ftn(CCX_DMT_708, "none\n");
	else
	{
		int changed = 0;
		for (int i = 0; i < DTVCC_MAX_WINDOWS; i++)
		{
			if (windows_bitmap & 1)
			{
				ccx_common_logging.debug_ftn(CCX_DMT_708, "[Window %d] ", i);
				if (decoder->windows[i].is_defined && decoder->windows[i].visible && !decoder->windows[i].is_empty)
				{
					changed = 1;
					decoder->windows[i].visible = 0;
				}
				// TODO: Actually Hide Window
			}
			windows_bitmap >>= 1;
		}
		ccx_common_logging.debug_ftn(CCX_DMT_708, "\n");
		if (changed)
			_dtvcc_screen_update(decoder);
	}
}

void dtvcc_handle_TGW_ToggleWindows(dtvcc_service_decoder *decoder, int windows_bitmap)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_TGW_ToggleWindows: windows: ");
	if (windows_bitmap == 0)
		ccx_common_logging.debug_ftn(CCX_DMT_708, "none\n");
	else
	{
		for (int i = 0; i < DTVCC_MAX_WINDOWS; i++)
		{
			if (windows_bitmap & 1)
			{
				ccx_common_logging.debug_ftn(CCX_DMT_708, "[W-%d] ", i);
				decoder->windows[i].visible = !decoder->windows[i].visible;
			}
			windows_bitmap >>= 1;
		}
		ccx_common_logging.debug_ftn(CCX_DMT_708, "\n");
		_dtvcc_screen_update(decoder);
	}
}

void dtvcc_handle_DFx_DefineWindow(dtvcc_service_decoder *decoder, int window_idx, unsigned char *data)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_DFx_DefineWindow: "
			"window [%d], attributes: \n", window_idx);

	if (decoder->windows[window_idx].is_defined &&
			memcmp(decoder->windows[window_idx].commands, data + 1, 6) == 0)
	{
		// When a decoder receives a DefineWindow command for an existing window, the
		// command is to be ignored if the command parameters are unchanged from the
		// previous window definition.
		ccx_common_logging.debug_ftn(CCX_DMT_708, "\tRepeated window definition, ignored\n");
		return;
	}

	decoder->windows[window_idx].number = window_idx;

	int priority = (data[1]) & 0x7;
	int col_lock = (data[1] >> 3) & 0x1;
	int row_lock = (data[1] >> 4) & 0x1;
	int visible  = (data[1] >> 5) & 0x1;
	int anchor_vertical = data[2] & 0x7f;
	int relative_pos = data[2] >> 7;
	int anchor_horizontal = data[3];
	int row_count = data[4] & 0xf;
	int anchor_point = data[4] >> 4;
	int col_count = data[5] & 0x3f;
	int pen_style = data[6] & 0x7;
	int win_style = (data[6] >> 3) & 0x7;

	ccx_common_logging.debug_ftn(CCX_DMT_708, "                   Priority: [%d]        Column lock: [%3s]      Row lock: [%3s]\n",
			priority, col_lock ? "Yes" : "No", row_lock ? "Yes" : "No");
	ccx_common_logging.debug_ftn(CCX_DMT_708, "                    Visible: [%3s]  Anchor vertical: [%2d]   Relative pos: [%s]\n",
			visible ? "Yes" : "No", anchor_vertical, relative_pos ? "Yes" : "No");
	ccx_common_logging.debug_ftn(CCX_DMT_708, "          Anchor horizontal: [%3d]        Row count: [%2d]+1  Anchor point: [%d]\n",
			anchor_horizontal, row_count, anchor_point);
	ccx_common_logging.debug_ftn(CCX_DMT_708, "               Column count: [%2d]1      Pen style: [%d]      Win style: [%d]\n",
			col_count, pen_style, win_style);

	col_count++; // These increments seems to be needed but no documentation
	row_count++; // backs it up
	decoder->windows[window_idx].priority = priority;
	decoder->windows[window_idx].col_lock = col_lock;
	decoder->windows[window_idx].row_lock = row_lock;
	decoder->windows[window_idx].visible = visible;
	decoder->windows[window_idx].anchor_vertical = anchor_vertical;
	decoder->windows[window_idx].relative_pos = relative_pos;
	decoder->windows[window_idx].anchor_horizontal = anchor_horizontal;
	decoder->windows[window_idx].row_count = row_count;
	decoder->windows[window_idx].anchor_point = anchor_point;
	decoder->windows[window_idx].col_count = col_count;
	decoder->windows[window_idx].pen_style = pen_style;
	decoder->windows[window_idx].win_style = win_style;

	if (!decoder->windows[window_idx].is_defined)
	{
		// If the window is being created, all character positions in the window
		// are set to the fill color...
		// TODO: COLORS
		// ...and the pen location is set to (0,0)
		decoder->windows[window_idx].pen_column = 0;
		decoder->windows[window_idx].pen_row = 0;
		if (!decoder->windows[window_idx].memory_reserved)
		{
			for (int i = 0; i <= DTVCC_MAX_ROWS; i++)
			{
				decoder->windows[window_idx].rows[i] = (unsigned char *) malloc(DTVCC_MAX_COLUMNS + 1);
				if (!decoder->windows[window_idx].rows[i])
				{
					ccx_common_logging.fatal_ftn(EXIT_NOT_ENOUGH_MEMORY, "[CEA-708] dtvcc_handle_DFx_DefineWindow");
					decoder->windows[window_idx].is_defined = 0;
					decoder->current_window = -1;
					for (int j = 0; j < i; j++)
						free(decoder->windows[window_idx].rows[j]);
					return;
				}
			}
			decoder->windows[window_idx].memory_reserved = 1;
		}
		decoder->windows[window_idx].is_defined = 1;
		_dtvcc_window_clear_text(&decoder->windows[window_idx]);
	}
	else
	{
		// Specs unclear here: Do we need to delete the text in the existing window?
		// We do this because one of the sample files demands it.
		//  _dtvcc_window_clear_text (&decoder->windows[window]);
	}
	// ...also makes the defined windows the current window (setCurrentWindow)
	dtvcc_handle_CWx_SetCurrentWindow(decoder, window_idx);
	memcpy (decoder->windows[window_idx].commands, data + 1, 6);
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

void dtvcc_handle_DLW_DeleteWindows(dtvcc_service_decoder *decoder, int windows_bitmap)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_DLW_DeleteWindows: windows: ");

	int changed = 0;
	if (windows_bitmap == 0)
		ccx_common_logging.debug_ftn(CCX_DMT_708, "none\n");
	else
	{
		for (int i = 0; i < DTVCC_MAX_WINDOWS; i++)
		{
			if (windows_bitmap & 1)
			{
				ccx_common_logging.debug_ftn(CCX_DMT_708, "[Window %d] ", i);
				if (decoder->windows[i].is_defined && decoder->windows[i].visible && !decoder->windows[i].is_empty)
					changed = 1;
				_dtvcc_window_delete(decoder, i);
			}
			windows_bitmap >>= 1;
		}
	}
	ccx_common_logging.debug_ftn(CCX_DMT_708, "\n");
	if (changed)
		_dtvcc_screen_update(decoder);
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
	window->pen.pen_size = pen_size;
	window->pen.offset = offset;
	window->pen.text_tag = text_tag;
	window->pen.font_tag = font_tag;
	window->pen.edge_type = edge_type;
	window->pen.underline = underline;
	window->pen.italic = italic;
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
	window->pen_color.fg_color = fg_color;
	window->pen_color.fg_opacity = fg_opacity;
	window->pen_color.bg_color = bg_color;
	window->pen_color.bg_opacity = bg_opacity;
	window->pen_color.edge_color = edge_color;
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
}

void dtvcc_handle_DLC_DelayCancel(dtvcc_service_decoder *decoder)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_handle_DLC_DelayCancel");
	// TODO: See above
}

//-------------------------- CHARACTERS AND COMMANDS -------------------------

// G0 - Code Set - ASCII printable characters
int _dtvcc_handle_G0(dtvcc_service_decoder *decoder, unsigned char *data, int data_length)
{
	// TODO: Substitution of the music note character for the ASCII DEL character
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] G0: [%02X]  (%c)\n", data[0], data[0]);
	unsigned char c = dtvcc_get_internal_from_G0(data[0]);
	_dtvcc_process_character(decoder, c);
	return 1;
}

// G1 Code Set - ISO 8859-1 LATIN-1 Character Set
int _dtvcc_handle_G1(dtvcc_service_decoder *decoder, unsigned char *data, int data_length)
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] G1: [%02X]  (%c)\n", data[0], data[0]);
	unsigned char c = dtvcc_get_internal_from_G1(data[0]);
	_dtvcc_process_character(decoder, c);
	return 1;
}

int _dtvcc_handle_C0(dtvcc_service_decoder *decoder, unsigned char *data, int data_length)
{
	const char *name = DTVCC_COMMANDS_C0[data[0]];
	if (name == NULL)
		name = "Reserved";

	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] C0: [%02X]  (%d)   [%s]\n", data[0], data_length, name);

	int len = -1;
	// These commands have a known length even if they are reserved.
	if (/* data[0]>=0x00 && */ data[0] <= 0xF) // Comment to silence warning
	{
		switch (data[0])
		{
			case 0x0d: //CR
				_dtvcc_process_cr(decoder);
				break;
			case 0x0e: // HCR (Horizontal Carriage Return)
				// TODO: Process HDR
				break;
			case 0x0c: // FF (Form Feed)
				// TODO: Process FF
				break;
			default:
				ccx_common_logging.log_ftn("[CEA-708] _dtvcc_handle_C0: unhandled branch\n");
				break;
		}
		len = 1;
	}
	else if (data[0] >= 0x10 && data[0] <= 0x17)
	{
		// Note that 0x10 is actually EXT1 and is dealt with somewhere else. Rest is undefined as per
		// CEA-708-D
		len = 2;
	}
	else if (data[0] >= 0x18 && data[0] <= 0x1F)
	{
		// Only PE16 is defined.
		if (data[0] == 0x18) // PE16
		{
			; // TODO: Handle PE16
		}
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
	// TODO: Do something useful eventually
	return len;
}

// C1 Code Set - Captioning Commands Control Codes
int _dtvcc_handle_C1(dtvcc_service_decoder *decoder, unsigned char *data, int data_length)
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
			dtvcc_handle_HDW_HideWindows(decoder, data[1]);
			break;
		case TGW:
			dtvcc_handle_TGW_ToggleWindows(decoder, data[1]);
			break;
		case DLW:
			dtvcc_handle_DLW_DeleteWindows(decoder, data[1]);
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
// TODO: This code is completely untested due to lack of samples. Just following specs!
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
	// TODO: Implement if a sample ever appears
	ccx_common_logging.fatal_ftn(
			CCX_COMMON_EXIT_UNSUPPORTED, "[CEA-708] This sample contains unsupported 708 data. "
			"PLEASE help us improve CCExtractor by submitting it.\n");
	return 0; // Unreachable, but otherwise there's compilers warnings
}

// This function handles extended codes (EXT1 + code), from the extended sets
// G2 (20-7F) => Mostly unmapped, except for a few characters.
// G3 (A0-FF) => A0 is the CC symbol, everything else reserved for future expansion in EIA708-B
// C2 (00-1F) => Reserved for future extended misc. control and captions command codes
// TODO: This code is completely untested due to lack of samples. Just following specs!
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
		_dtvcc_process_character(decoder, c);
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
		_dtvcc_process_character(decoder, c);
	}
	return used;
}

//------------------------------- PROCESSING --------------------------------

void dtvcc_process_service_block(dtvcc_service_decoder *decoder, unsigned char *data, int data_length)
{
	int i = 0;
	while (i < data_length)
	{
		int used = -1;
		if (data[i] != EXT1)
		{
			// Group C0
			if (/* data[i]>=0x00 && */ data[i] <= 0x1F) // Comment to silence warning
			{
				used = _dtvcc_handle_C0(decoder, data + i, data_length - i);
			}
			// Group G0
			else if (data[i] >= 0x20 && data[i] <= 0x7F)
			{
				used = _dtvcc_handle_G0(decoder, data + i, data_length - i);
			}
			// Group C1
			else if (data[i]>=0x80 && data[i]<=0x9F)
			{
				used = _dtvcc_handle_C1(decoder, data + i, data_length - i);
			}
			// Group C2
			else
			{
				used = _dtvcc_handle_G1(decoder, data + i, data_length - i);
			}

			if (used == -1)
			{
				ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_process_service_block: "
						"There was a problem handling the data. Reseting service decoder\n");
				// TODO: Not sure if a local reset is going to be helpful here.
				_dtvcc_windows_reset(decoder);
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

void dtvcc_process_current_packet(struct lib_cc_decode *ctx)
{
	int seq = (ccx_dtvcc_ctx.current_packet[0] & 0xC0) >> 6; // Two most significants bits
	int len = ccx_dtvcc_ctx.current_packet[0] & 0x3F; // 6 least significants bits
#ifdef DEBUG_708_PACKETS
	ccx_common_logging.log_ftn("[CEA-708] dtvcc_process_current_packet: length=%d, seq=%d\n",
							   ccx_dtvcc_ctx.current_packet_length, seq);
#endif
	if (ccx_dtvcc_ctx.current_packet_length == 0)
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
	if (ccx_dtvcc_ctx.current_packet_length != len) // Is this possible?
	{
		_dtvcc_decoders_reset(ctx);
		return;
	}
	if (ccx_dtvcc_ctx.last_sequence != DTVCC_NO_LAST_SEQUENCE &&
			(ccx_dtvcc_ctx.last_sequence + 1) % 4 != seq)
	{
		ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_process_current_packet: "
											 "Unexpected sequence number, it is [%d] but should be [%d]\n",
				seq, (ccx_dtvcc_ctx.last_sequence + 1 ) % 4);
		_dtvcc_decoders_reset(ctx);
		return;
	}
	ccx_dtvcc_ctx.last_sequence = seq;

	unsigned char *pos = ccx_dtvcc_ctx.current_packet + 1;

	while (pos < ccx_dtvcc_ctx.current_packet + len)
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
//			if (pos != (ccx_dtvcc_ctx.current_packet + len - 1)) // i.e. last byte in packet
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
			pos = ccx_dtvcc_ctx.current_packet + len; // Move to end
			break;
		}

		if (block_length != 0)
		{
			ccx_dtvcc_ctx.report.services[service_number] = 1;
		}

		if (service_number > 0 && ccx_dtvcc_ctx.decoders[service_number - 1].inited)
			dtvcc_process_service_block(&ccx_dtvcc_ctx.decoders[service_number - 1], pos, block_length);

		pos += block_length; // Skip data
	}

	_dtvcc_clear_packet();

	if (pos != ccx_dtvcc_ctx.current_packet +len) // For some reason we didn't parse the whole packet
	{
		ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_process_current_packet:"
				" There was a problem with this packet, reseting\n");
		_dtvcc_decoders_reset(ctx);
	}

	if (len < 128 && *pos) // Null header is mandatory if there is room
	{
		ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_process_current_packet: "
				"Warning: Null header expected but not found.\n");
	}
}

//---------------------------- EXPORTED FUNCTIONS ---------------------------

void dtvcc_process_data(struct lib_cc_decode *ctx, const unsigned char *data, int data_length)
{
	/*
	 * Note: the data has following format:
	 * 1 byte for cc_valid
	 * 1 byte for cc_type
	 * 2 bytes for the actual data
	 */

	if (!ccx_dtvcc_ctx.is_active && !ccx_dtvcc_ctx.report_enabled)
		return;

	for (int i = 0; i < data_length; i += 4)
	{
		unsigned char cc_valid = data[i];
		unsigned char cc_type = data[i + 1];

		switch (cc_type)
		{
			case 2:
				ccx_common_logging.debug_ftn (CCX_DMT_708, "[CEA-708] dtvcc_process_data: DTVCC Channel Packet Data\n");
				if (cc_valid == 0) // This ends the previous packet
					dtvcc_process_current_packet(ctx);
				else
				{
					if (ccx_dtvcc_ctx.current_packet_length > 253) //TODO why DTVCC_MAX_PACKET_LENGTH == 128 then?
					{
						ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_process_data: "
								"Warning: Legal packet size exceeded (1), data not added.\n");
					}
					else
					{
						ccx_dtvcc_ctx.current_packet[ccx_dtvcc_ctx.current_packet_length++] = data[i + 2];
						ccx_dtvcc_ctx.current_packet[ccx_dtvcc_ctx.current_packet_length++] = data[i + 3];
					}
				}
				break;
			case 3:
				ccx_common_logging.debug_ftn (CCX_DMT_708, "[CEA-708] dtvcc_process_data: DTVCC Channel Packet Start\n");
				dtvcc_process_current_packet(ctx);
				if (cc_valid)
				{
					if (ccx_dtvcc_ctx.current_packet_length > 127)
					{
						ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_process_data: "
								"Warning: Legal packet size exceeded (2), data not added.\n");
					}
					else
					{
						ccx_dtvcc_ctx.current_packet[ccx_dtvcc_ctx.current_packet_length++] = data[i + 2];
						ccx_dtvcc_ctx.current_packet[ccx_dtvcc_ctx.current_packet_length++] = data[i + 3];
					}
				}
				break;
			default:
				ccx_common_logging.fatal_ftn (CCX_COMMON_EXIT_BUG_BUG, "[CEA-708] dtvcc_process_data: "
						"shouldn't be here - cc_type: %d\n", cc_type);
		}
	}
}

void dtvcc_init(struct lib_ccx_ctx *ctx, struct ccx_s_options *opt)
{
	for (int i = 0; i < DTVCC_MAX_SERVICES; i++)
	{
		if (!ccx_dtvcc_ctx.services_active[i])
			continue;

		ccx_dtvcc_ctx.decoders[i].fh = -1;
		ccx_dtvcc_ctx.decoders[i].cc_count = 0;
		ccx_dtvcc_ctx.decoders[i].filename = NULL;
		ccx_dtvcc_ctx.decoders[i].output_started = 0;

		_dtvcc_windows_reset(&ccx_dtvcc_ctx.decoders[i]);
		_dtvcc_decoder_init_write(ctx, &ccx_dtvcc_ctx.decoders[i], i + 1);

		if (opt->cc_to_stdout)
		{
			ccx_dtvcc_ctx.decoders[i].fh = STDOUT_FILENO;
			ccx_dtvcc_ctx.decoders[i].output_started = 1;
		}
	}
	ccx_dtvcc_ctx.report_enabled = opt->print_file_reports;
}

void dtvcc_free()
{
	ccx_common_logging.debug_ftn(CCX_DMT_708, "[CEA-708] dtvcc_free: cleaning up\n");
	for (int i = 0; i < DTVCC_MAX_SERVICES; i++)
	{
		if (!ccx_dtvcc_ctx.services_active[i])
			continue;

		if (ccx_dtvcc_ctx.decoders[i].fh != -1 && ccx_dtvcc_ctx.decoders[i].fh != STDOUT_FILENO) {
			close(ccx_dtvcc_ctx.decoders[i].fh);
		}
		free(ccx_dtvcc_ctx.decoders[i].filename);
	}
}

//---------------------------------- CONTEXT -----------------------------------

void dtvcc_ctx_init(ccx_dtvcc_ctx_t *ctx)
{
	ctx->reset_count = 0;
	ctx->is_active = 0;
	ctx->report_enabled = 0;

	memset(ctx->services_active, 0, DTVCC_MAX_SERVICES * sizeof(int));

	memset(ctx->current_packet, 0, DTVCC_MAX_PACKET_LENGTH * sizeof(unsigned char));
	ctx->current_packet_length = 0;

	ctx->last_sequence = DTVCC_NO_LAST_SEQUENCE;
}

void dtvcc_ctx_free(ccx_dtvcc_ctx_t *ctx)
{

}