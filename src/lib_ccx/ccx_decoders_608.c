#include "ccx_decoders_608.h"
#include "ccx_common_common.h"
#include "ccx_common_structs.h"
#include "ccx_common_constants.h"
#include "ccx_common_timing.h"
#include "ccx_decoders_structs.h"
#include "ccx_decoders_xds.h"

static const int rowdata[] = {11, -1, 1, 2, 3, 4, 12, 13, 14, 15, 5, 6, 7, 8, 9, 10};
// Relationship between the first PAC byte and the row number
int in_xds_mode = 0;

// unsigned char str[2048]; // Another generic general purpose buffer

const unsigned char pac2_attribs[][3] = // Color, font, ident
    {
	{COL_WHITE, FONT_REGULAR, 0},		 // 0x40 || 0x60
	{COL_WHITE, FONT_UNDERLINED, 0},	 // 0x41 || 0x61
	{COL_GREEN, FONT_REGULAR, 0},		 // 0x42 || 0x62
	{COL_GREEN, FONT_UNDERLINED, 0},	 // 0x43 || 0x63
	{COL_BLUE, FONT_REGULAR, 0},		 // 0x44 || 0x64
	{COL_BLUE, FONT_UNDERLINED, 0},		 // 0x45 || 0x65
	{COL_CYAN, FONT_REGULAR, 0},		 // 0x46 || 0x66
	{COL_CYAN, FONT_UNDERLINED, 0},		 // 0x47 || 0x67
	{COL_RED, FONT_REGULAR, 0},		 // 0x48 || 0x68
	{COL_RED, FONT_UNDERLINED, 0},		 // 0x49 || 0x69
	{COL_YELLOW, FONT_REGULAR, 0},		 // 0x4a || 0x6a
	{COL_YELLOW, FONT_UNDERLINED, 0},	 // 0x4b || 0x6b
	{COL_MAGENTA, FONT_REGULAR, 0},		 // 0x4c || 0x6c
	{COL_MAGENTA, FONT_UNDERLINED, 0},	 // 0x4d || 0x6d
	{COL_WHITE, FONT_ITALICS, 0},		 // 0x4e || 0x6e
	{COL_WHITE, FONT_UNDERLINED_ITALICS, 0}, // 0x4f || 0x6f
	{COL_WHITE, FONT_REGULAR, 0},		 // 0x50 || 0x70
	{COL_WHITE, FONT_UNDERLINED, 0},	 // 0x51 || 0x71
	{COL_WHITE, FONT_REGULAR, 4},		 // 0x52 || 0x72
	{COL_WHITE, FONT_UNDERLINED, 4},	 // 0x53 || 0x73
	{COL_WHITE, FONT_REGULAR, 8},		 // 0x54 || 0x74
	{COL_WHITE, FONT_UNDERLINED, 8},	 // 0x55 || 0x75
	{COL_WHITE, FONT_REGULAR, 12},		 // 0x56 || 0x76
	{COL_WHITE, FONT_UNDERLINED, 12},	 // 0x57 || 0x77
	{COL_WHITE, FONT_REGULAR, 16},		 // 0x58 || 0x78
	{COL_WHITE, FONT_UNDERLINED, 16},	 // 0x59 || 0x79
	{COL_WHITE, FONT_REGULAR, 20},		 // 0x5a || 0x7a
	{COL_WHITE, FONT_UNDERLINED, 20},	 // 0x5b || 0x7b
	{COL_WHITE, FONT_REGULAR, 24},		 // 0x5c || 0x7c
	{COL_WHITE, FONT_UNDERLINED, 24},	 // 0x5d || 0x7d
	{COL_WHITE, FONT_REGULAR, 28},		 // 0x5e || 0x7e
	{COL_WHITE, FONT_UNDERLINED, 28}	 // 0x5f || 0x7f
};

static const char *command_type[] =
    {
	"Unknown",
	"EDM - EraseDisplayedMemory",
	"RCL - ResumeCaptionLoading",
	"EOC - End Of Caption",
	"TO1 - Tab Offset, 1 column",
	"TO2 - Tab Offset, 2 column",
	"TO3 - Tab Offset, 3 column",
	"RU2 - Roll up 2 rows",
	"RU3 - Roll up 3 rows",
	"RU4 - Roll up 4 rows",
	"CR  - Carriage Return",
	"ENM - Erase non-displayed memory",
	"BS  - Backspace",
	"RTD - Resume Text Display",
	"AOF - Not Used (Alarm Off)",
	"AON - Not Used (Alarm On)",
	"DER - Delete to End of Row",
	"RDC - Resume Direct Captioning",
	"RU1 - Fake Roll up 1 rows"};

static const char *font_text[] =
    {
	"regular",
	"italics",
	"underlined",
	"underlined italics"};

#if 0
static const char *cc_modes_text[] =
{
	"Pop-Up captions"
};
#endif

const char *color_text[MAX_COLOR][2] =
    {
	{"white", ""},
	{"green", "<font color=\"#00ff00\">"},
	{"blue", "<font color=\"#0000ff\">"},
	{"cyan", "<font color=\"#00ffff\">"},
	{"red", "<font color=\"#ff0000\">"},
	{"yellow", "<font color=\"#ffff00\">"},
	{"magenta", "<font color=\"#ff00ff\">"},
	{"userdefined", "<font color=\""},
	{"black", ""},
	{"transparent", ""}};

void clear_eia608_cc_buffer(ccx_decoder_608_context *context, struct eia608_screen *data)
{
	for (int i = 0; i < CCX_DECODER_608_SCREEN_ROWS; i++)
	{
		memset(data->characters[i], ' ', CCX_DECODER_608_SCREEN_WIDTH);
		data->characters[i][CCX_DECODER_608_SCREEN_WIDTH] = 0;

		for (int j = 0; j < CCX_DECODER_608_SCREEN_WIDTH + 1; ++j)
		{
			data->colors[i][j] = context->settings->default_color;
			data->fonts[i][j] = FONT_REGULAR;
		}

		data->row_used[i] = 0;
	}
	data->empty = 1;
}

void ccx_decoder_608_dinit_library(void **ctx)
{
	freep(ctx);
}
ccx_decoder_608_context *ccx_decoder_608_init_library(struct ccx_decoder_608_settings *settings, int channel,
						      int field, int *halt,
						      int cc_to_stdout,
						      enum ccx_output_format output_format, struct ccx_common_timing_ctx *timing)
{
	ccx_decoder_608_context *data = NULL;

	data = malloc(sizeof(ccx_decoder_608_context));
	if (!data)
		return NULL;

	data->cursor_column = 0;
	data->cursor_row = 0;
	data->visible_buffer = 1;
	data->last_c1 = 0;
	data->last_c2 = 0;
	data->mode = MODE_POPON;
	// data->current_visible_start_cc=0;
	data->current_visible_start_ms = 0;
	data->screenfuls_counter = 0;
	data->channel = 1;
	data->font = FONT_REGULAR;
	data->rollup_base_row = 14;
	data->ts_start_of_current_line = -1;
	data->ts_last_char_received = -1;
	data->new_channel = 1;
	data->bytes_processed_608 = 0;
	data->my_field = field;
	data->my_channel = channel;
	data->have_cursor_position = 0;
	data->rollup_from_popon = 0;
	data->output_format = output_format;
	data->cc_to_stdout = cc_to_stdout;
	data->textprinted = 0;
	// Note: ts_start_of_current_line already set to -1 above

	data->halt = halt;

	data->settings = settings;
	data->current_color = data->settings->default_color;
	data->report = settings->report;
	data->timing = timing;

	clear_eia608_cc_buffer(data, &data->buffer1);
	clear_eia608_cc_buffer(data, &data->buffer2);

	return data;
}

struct eia608_screen *get_writing_buffer(ccx_decoder_608_context *context)
{
	struct eia608_screen *use_buffer = NULL;
	switch (context->mode)
	{
		case MODE_POPON: // Write on the non-visible buffer
			if (context->visible_buffer == 1)
				use_buffer = &context->buffer2;
			else
				use_buffer = &context->buffer1;
			break;
		case MODE_FAKE_ROLLUP_1: // Write directly to screen
		case MODE_ROLLUP_2:
		case MODE_ROLLUP_3:
		case MODE_ROLLUP_4:
		case MODE_PAINTON:
		case MODE_TEXT:
			// TODO: Fix this. Text uses a different buffer, and contains non-program information.
			if (context->visible_buffer == 1)
				use_buffer = &context->buffer1;
			else
				use_buffer = &context->buffer2;
			break;
		default:
			ccx_common_logging.fatal_ftn(CCX_COMMON_EXIT_BUG_BUG, "Caption mode has an illegal value at get_writing_buffer(), this is a bug.\n");
	}
	return use_buffer;
}

void delete_to_end_of_row(ccx_decoder_608_context *context)
{
	if (context->mode != MODE_TEXT)
	{
		if (context->cursor_row >= CCX_DECODER_608_SCREEN_ROWS)
			return;

		struct eia608_screen *use_buffer = get_writing_buffer(context);
		for (int i = context->cursor_column; i <= CCX_DECODER_608_SCREEN_WIDTH - 1; i++)
		{
			// TODO: This can change the 'used' situation of a column, so we'd
			// need to check and correct.
			use_buffer->characters[context->cursor_row][i] = ' ';
			use_buffer->colors[context->cursor_row][i] = context->settings->default_color;
			use_buffer->fonts[context->cursor_row][i] = context->font;
		}
	}
}

void write_char(const unsigned char c, ccx_decoder_608_context *context)
{
	if (context->mode != MODE_TEXT)
	{
		struct eia608_screen *use_buffer = get_writing_buffer(context);
		/* printf ("\rWriting char [%c] at %s:%d:%d\n",c,
		use_buffer == &wb->data608->buffer1?"B1":"B2",
		wb->data608->cursor_row,wb->data608->cursor_column); */

		if (context->cursor_row >= CCX_DECODER_608_SCREEN_ROWS || context->cursor_column >= CCX_DECODER_608_SCREEN_WIDTH)
			return;

		use_buffer->characters[context->cursor_row][context->cursor_column] = c;
		use_buffer->colors[context->cursor_row][context->cursor_column] = context->current_color;
		use_buffer->fonts[context->cursor_row][context->cursor_column] = context->font;
		use_buffer->row_used[context->cursor_row] = 1;

		if (use_buffer->empty)
		{
			// Don't set start time if we're in a transition from pop-on to roll-up
			// In this case, start time will be set when CR causes scrolling
			if (MODE_POPON != context->mode && !context->rollup_from_popon)
				context->current_visible_start_ms = get_visible_start(context->timing, context->my_field);
		}
		use_buffer->empty = 0;

		if (context->cursor_column < CCX_DECODER_608_SCREEN_WIDTH - 1)
			context->cursor_column++;
		if (context->ts_start_of_current_line == -1)
			context->ts_start_of_current_line = get_fts(context->timing, context->my_field);
		context->ts_last_char_received = get_fts(context->timing, context->my_field);
	}
}

/* Handle MID-ROW CODES. */
void handle_text_attr(const unsigned char c1, const unsigned char c2, ccx_decoder_608_context *context)
{
	// Handle channel change
	context->channel = context->new_channel;
	if (context->channel != context->my_channel)
		return;
	ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "\r608: text_attr: %02X %02X", c1, c2);
	if (((c1 != 0x11 && c1 != 0x19) ||
	     (c2 < 0x20 || c2 > 0x2f)))
	{
		ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "\rThis is not a text attribute!\n");
	}
	else
	{
		int i = c2 - 0x20;
		context->current_color = pac2_attribs[i][0];
		context->font = pac2_attribs[i][1];
		ccx_common_logging.debug_ftn(
		    CCX_DMT_DECODER_608,
		    "  --  Color: %s,  font: %s\n",
		    color_text[context->current_color][0],
		    font_text[context->font]);
		// Mid-row codes should put a non-transparent space at the current position
		// and advance the cursor
		// so use write_char
		write_char(0x20, context);
	}
}

struct eia608_screen *get_current_visible_buffer(ccx_decoder_608_context *context)
{
	struct eia608_screen *data;
	if (context->visible_buffer == 1)
		data = &context->buffer1;
	else
		data = &context->buffer2;
	return data;
}

int write_cc_buffer(ccx_decoder_608_context *context, struct cc_subtitle *sub)
{
	struct eia608_screen *data;
	int wrote_something = 0;
	LLONG start_time;
	LLONG end_time;

	if (context->settings->screens_to_process != -1 &&
	    context->screenfuls_counter >= context->settings->screens_to_process)
	{
		// We are done.
		*context->halt = 1;
		return 0;
	}

	data = get_current_visible_buffer(context);

	if (context->mode == MODE_FAKE_ROLLUP_1 && // Use the actual start of data instead of last buffer change
	    context->ts_start_of_current_line != -1)
		context->current_visible_start_ms = context->ts_start_of_current_line;

	start_time = context->current_visible_start_ms;
	end_time = get_visible_end(context->timing, context->my_field);
	sub->type = CC_608;
	data->format = SFORMAT_CC_SCREEN;
	data->start_time = 0;
	data->end_time = 0;
	data->mode = context->mode;
	data->channel = context->channel;
	data->my_field = context->my_field;

	if (!data->empty && context->output_format != CCX_OF_NULL)
	{
		size_t new_size;

		if (sub->nb_data + 1 > SIZE_MAX / sizeof(struct eia608_screen))
		{
			ccx_common_logging.log_ftn("Too many screens, cannot allocate more memory.\n");
			return 0;
		}

		new_size = (sub->nb_data + 1) * sizeof(struct eia608_screen);

		struct eia608_screen *new_data = (struct eia608_screen *)realloc(sub->data, new_size);
		if (!new_data)
		{
			ccx_common_logging.log_ftn("Out of memory while reallocating screen buffer\n");
			return 0;
		}
		sub->data = new_data;
		sub->datatype = CC_DATATYPE_GENERIC;
		memcpy(((struct eia608_screen *)sub->data) + sub->nb_data, data, sizeof(*data));
		sub->nb_data++;
		wrote_something = 1;

		// Buffer until next packet to get correct timing
		if (start_time == end_time && sub->got_output == 1)
		{
			sub->got_output = 0;
		}

		if (start_time < end_time)
		{
			int i = 0;
			int nb_data = sub->nb_data;
			data = (struct eia608_screen *)sub->data;
			for (i = 0; (unsigned)i < sub->nb_data; i++)
			{
				if (!data->start_time)
					break;
				nb_data--;
				data++;
			}
			for (i = 0; i < nb_data; i++)
			{
				// Don't update start time if Direct Rollup is selected
				if (context->settings->direct_rollup)
				{
					data->start_time = start_time;
				}
				else
				{
					data->start_time = start_time + (((end_time - start_time) / nb_data) * i);
				}
				data->end_time = start_time + (((end_time - start_time) / nb_data) * (i + 1));
				data++;
			}
			sub->got_output = 1;
		}
	}
	return wrote_something;
}
int write_cc_line(ccx_decoder_608_context *context, struct cc_subtitle *sub)
{
	struct eia608_screen *data;
	LLONG start_time;
	LLONG end_time;
	int i = 0;
	int wrote_something = 0;
	data = get_current_visible_buffer(context);

	start_time = context->ts_start_of_current_line;
	end_time = get_fts(context->timing, context->my_field);
	sub->type = CC_608;
	data->format = SFORMAT_CC_LINE;
	data->start_time = 0;
	data->end_time = 0;
	data->mode = context->mode;
	data->channel = context->channel;
	data->my_field = context->my_field;

	if (!data->empty)
	{
		size_t new_size;

		if (sub->nb_data + 1 > SIZE_MAX / sizeof(struct eia608_screen))
		{
			ccx_common_logging.log_ftn("Too many screens, cannot allocate more memory.\n");
			return 0;
		}

		new_size = (sub->nb_data + 1) * sizeof(struct eia608_screen);

		struct eia608_screen *new_data = (struct eia608_screen *)realloc(sub->data, new_size);
		if (!new_data)
		{
			ccx_common_logging.log_ftn("Out of memory while reallocating screen buffer\n");
			return 0;
		}
		sub->data = new_data;
		memcpy(((struct eia608_screen *)sub->data) + sub->nb_data, data, sizeof(*data));
		data = (struct eia608_screen *)sub->data + sub->nb_data;
		sub->datatype = CC_DATATYPE_GENERIC;
		sub->nb_data++;

		for (i = 0; i < CCX_DECODER_608_SCREEN_ROWS; i++)
		{
			if (i == context->cursor_row)
				data->row_used[i] = 1;
			else
				data->row_used[i] = 0;
		}
		wrote_something = 1;
		if (start_time < end_time)
		{
			int nb_data = sub->nb_data;
			data = (struct eia608_screen *)sub->data;
			for (i = 0; (unsigned)i < sub->nb_data; i++)
			{
				if (!data->start_time)
					break;
				nb_data--;
				data++;
			}
			for (i = 0; (int)i < nb_data; i++)
			{
				data->start_time = start_time + (((end_time - start_time) / nb_data) * i);
				data->end_time = start_time + (((end_time - start_time) / nb_data) * (i + 1));
				data++;
			}
			sub->got_output = 1;
		}
	}
	return wrote_something;
}

// Check if a rollup would cause a line to go off the visible area
int check_roll_up(ccx_decoder_608_context *context)
{
	int keep_lines = 0;
	int firstrow = -1, lastrow = -1;
	struct eia608_screen *use_buffer;
	if (context->visible_buffer == 1)
		use_buffer = &context->buffer1;
	else
		use_buffer = &context->buffer2;

	switch (context->mode)
	{
		case MODE_FAKE_ROLLUP_1:
			keep_lines = 1;
			break;
		case MODE_ROLLUP_2:
			keep_lines = 2;
			break;
		case MODE_ROLLUP_3:
			keep_lines = 3;
			break;
		case MODE_ROLLUP_4:
			keep_lines = 4;
			break;
		case MODE_TEXT:
			keep_lines = 7; // CFS: can be 7 to 15 according to the handbook. No idea how this is selected.
			break;
		default: // Shouldn't happen
			return 0;
			break;
	}
	if (use_buffer->row_used[0]) // If top line is used it will go off the screen no matter what
		return 1;
	int rows_orig = 0; // Number of rows in use right now
	for (int i = 0; i < CCX_DECODER_608_SCREEN_ROWS; i++)
	{
		if (use_buffer->row_used[i])
		{
			rows_orig++;
			if (firstrow == -1)
				firstrow = i;
			lastrow = i;
		}
	}
	if (lastrow == -1) // Empty screen, nothing to rollup
		return 0;
	if ((lastrow - firstrow + 1) >= keep_lines)
		return 1; // We have the roll-up area full, so yes

	if ((firstrow - 1) <= context->cursor_row - keep_lines) // Roll up will delete those lines.
		return 1;
	return 0;
}

// Roll-up: Returns true if a line was rolled over the visible area (it disappears from screen), false
// if the rollup didn't delete any line.
int roll_up(ccx_decoder_608_context *context)
{
	struct eia608_screen *use_buffer;
	if (context->visible_buffer == 1)
		use_buffer = &context->buffer1;
	else
		use_buffer = &context->buffer2;
	int keep_lines;
	switch (context->mode)
	{
		case MODE_FAKE_ROLLUP_1:
			keep_lines = 1;
			break;
		case MODE_ROLLUP_2:
			keep_lines = 2;
			break;
		case MODE_ROLLUP_3:
			keep_lines = 3;
			break;
		case MODE_ROLLUP_4:
			keep_lines = 4;
			break;
		case MODE_TEXT:
			keep_lines = 7; // CFS: can be 7 to 15 according to the handbook. No idea how this is selected.
			break;
		default: // Shouldn't happen
			keep_lines = 0;
			break;
	}
	int firstrow = -1, lastrow = -1;
	// Look for the last line used
	int rows_orig = 0; // Number of rows in use right now
	for (int i = 0; i < CCX_DECODER_608_SCREEN_ROWS; i++)
	{
		if (use_buffer->row_used[i])
		{
			rows_orig++;
			if (firstrow == -1)
				firstrow = i;
			lastrow = i;
		}
	}

	ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "\rIn roll-up: %d lines used, first: %d, last: %d\n", rows_orig, firstrow, lastrow);

	if (lastrow == -1) // Empty screen, nothing to rollup
		return 0;

	for (int j = lastrow - keep_lines + 1; j < lastrow; j++)
	{
		if (j >= 0)
		{
			memcpy(use_buffer->characters[j], use_buffer->characters[j + 1], CCX_DECODER_608_SCREEN_WIDTH + 1);

			for (int i = 0; i < CCX_DECODER_608_SCREEN_WIDTH + 1; ++i)
			{
				use_buffer->colors[j][i] = use_buffer->colors[j + 1][i];
				use_buffer->fonts[j][i] = use_buffer->fonts[j + 1][i];
			}

			use_buffer->row_used[j] = use_buffer->row_used[j + 1];
		}
	}
	for (int j = 0; j < (1 + context->cursor_row - keep_lines); j++)
	{
		memset(use_buffer->characters[j], ' ', CCX_DECODER_608_SCREEN_WIDTH);
		use_buffer->characters[j][CCX_DECODER_608_SCREEN_WIDTH] = 0;

		for (int i = 0; i < CCX_DECODER_608_SCREEN_WIDTH; ++i)
		{
			use_buffer->colors[j][i] = context->settings->default_color;
			use_buffer->fonts[j][i] = FONT_REGULAR;
		}

		use_buffer->row_used[j] = 0;
	}

	memset(use_buffer->characters[lastrow], ' ', CCX_DECODER_608_SCREEN_WIDTH);
	use_buffer->characters[lastrow][CCX_DECODER_608_SCREEN_WIDTH] = 0;

	for (int i = 0; i < CCX_DECODER_608_SCREEN_WIDTH; ++i)
	{
		use_buffer->colors[lastrow][i] = context->settings->default_color;
		use_buffer->fonts[lastrow][i] = FONT_REGULAR;
	}

	use_buffer->row_used[lastrow] = 0;

	// Sanity check
	int rows_now = 0;
	for (int i = 0; i < CCX_DECODER_608_SCREEN_ROWS; i++)
		if (use_buffer->row_used[i])
			rows_now++;
	if (rows_now > keep_lines)
		ccx_common_logging.log_ftn("Bug in roll_up, should have %d lines but I have %d.\n",
					   keep_lines, rows_now);

	// If the buffer is now empty, let's set the flag
	// This will allow write_char to set visible start time appropriately
	if (0 == rows_now)
		use_buffer->empty = 1;

	return (rows_now != rows_orig);
}

void erase_memory(ccx_decoder_608_context *context, int displayed)
{
	struct eia608_screen *buf;
	if (displayed)
	{
		if (context->visible_buffer == 1)
			buf = &context->buffer1;
		else
			buf = &context->buffer2;
	}
	else
	{
		if (context->visible_buffer == 1)
			buf = &context->buffer2;
		else
			buf = &context->buffer1;
	}
	clear_eia608_cc_buffer(context, buf);
}

int is_current_row_empty(ccx_decoder_608_context *context)
{
	struct eia608_screen *use_buffer;
	if (context->visible_buffer == 1)
		use_buffer = &context->buffer1;
	else
		use_buffer = &context->buffer2;
	for (int i = 0; i < CCX_DECODER_608_SCREEN_WIDTH; i++)
	{
		if (use_buffer->characters[context->rollup_base_row][i] != ' ')
			return 0;
	}
	return 1;
}

/* Process GLOBAL CODES */
void handle_command(unsigned char c1, const unsigned char c2, ccx_decoder_608_context *context, struct cc_subtitle *sub)
{
	int changes = 0;

	// Handle channel change
	context->channel = context->new_channel;
	if (context->channel != context->my_channel)
		return;

	enum command_code command = COM_UNKNOWN;
	if (c1 == 0x15)
		c1 = 0x14;
	if ((c1 == 0x14 || c1 == 0x1C) && c2 == 0x2C)
		command = COM_ERASEDISPLAYEDMEMORY;
	if ((c1 == 0x14 || c1 == 0x1C) && c2 == 0x20)
		command = COM_RESUMECAPTIONLOADING;
	if ((c1 == 0x14 || c1 == 0x1C) && c2 == 0x2F)
		command = COM_ENDOFCAPTION;
	if ((c1 == 0x14 || c1 == 0x1C) && c2 == 0x22)
		command = COM_ALARMOFF;
	if ((c1 == 0x14 || c1 == 0x1C) && c2 == 0x23)
		command = COM_ALARMON;
	if ((c1 == 0x14 || c1 == 0x1C) && c2 == 0x24)
		command = COM_DELETETOENDOFROW;
	if ((c1 == 0x17 || c1 == 0x1F) && c2 == 0x21)
		command = COM_TABOFFSET1;
	if ((c1 == 0x17 || c1 == 0x1F) && c2 == 0x22)
		command = COM_TABOFFSET2;
	if ((c1 == 0x17 || c1 == 0x1F) && c2 == 0x23)
		command = COM_TABOFFSET3;
	if ((c1 == 0x14 || c1 == 0x1C) && c2 == 0x25)
		command = COM_ROLLUP2;
	if ((c1 == 0x14 || c1 == 0x1C) && c2 == 0x26)
		command = COM_ROLLUP3;
	if ((c1 == 0x14 || c1 == 0x1C) && c2 == 0x27)
		command = COM_ROLLUP4;
	if ((c1 == 0x14 || c1 == 0x1C) && c2 == 0x29)
		command = COM_RESUMEDIRECTCAPTIONING;
	if ((c1 == 0x14 || c1 == 0x1C) && c2 == 0x2D)
		command = COM_CARRIAGERETURN;
	if ((c1 == 0x14 || c1 == 0x1C) && c2 == 0x2E)
		command = COM_ERASENONDISPLAYEDMEMORY;
	if ((c1 == 0x14 || c1 == 0x1C) && c2 == 0x21)
		command = COM_BACKSPACE;
	if ((c1 == 0x14 || c1 == 0x1C) && c2 == 0x2b)
		command = COM_RESUMETEXTDISPLAY;

	if ((command == COM_ROLLUP2 || command == COM_ROLLUP3 || command == COM_ROLLUP4) && context->settings->force_rollup == 1)
		command = COM_FAKE_RULLUP1;

	if ((command == COM_ROLLUP3 || command == COM_ROLLUP4) && context->settings->force_rollup == 2)
		command = COM_ROLLUP2;
	else if (command == COM_ROLLUP4 && context->settings->force_rollup == 3)
		command = COM_ROLLUP3;

	ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "\rCommand begin: %02X %02X (%s)\n", c1, c2, command_type[command]);
	ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "\rCurrent mode: %d  Position: %d,%d  VisBuf: %d\n", context->mode,
				     context->cursor_row, context->cursor_column, context->visible_buffer);

	switch (command)
	{
		case COM_BACKSPACE:
			if (context->cursor_column > 0)
			{
				context->cursor_column--;
				get_writing_buffer(context)->characters[context->cursor_row][context->cursor_column] = ' ';
			}
			break;
		case COM_TABOFFSET1:
			if (context->cursor_column < CCX_DECODER_608_SCREEN_WIDTH - 1)
				context->cursor_column++;
			break;
		case COM_TABOFFSET2:
			context->cursor_column += 2;
			if (context->cursor_column > CCX_DECODER_608_SCREEN_WIDTH - 1)
				context->cursor_column = CCX_DECODER_608_SCREEN_WIDTH - 1;
			break;
		case COM_TABOFFSET3:
			context->cursor_column += 3;
			if (context->cursor_column > CCX_DECODER_608_SCREEN_WIDTH - 1)
				context->cursor_column = CCX_DECODER_608_SCREEN_WIDTH - 1;
			break;
		case COM_RESUMECAPTIONLOADING:
			context->mode = MODE_POPON;
			break;
		case COM_RESUMETEXTDISPLAY:
			context->mode = MODE_TEXT;
			break;
		case COM_FAKE_RULLUP1:
		case COM_ROLLUP2:
		case COM_ROLLUP3:
		case COM_ROLLUP4:
			if (context->mode == MODE_POPON || context->mode == MODE_PAINTON)
			{
				/* CEA-608 C.10 Style Switching (regulatory)
				[...]if pop-up or paint-on captioning is already present in
				either memory it shall be erased[...] */
				if (write_cc_buffer(context, sub))
					context->screenfuls_counter++;
				erase_memory(context, true);
				// Track transition from pop-on/paint-on to roll-up for timing adjustment
				// Start time will be set when CR causes scrolling (matching FFmpeg behavior)
				context->rollup_from_popon = 1;
				context->ts_start_of_current_line = -1;
			}
			erase_memory(context, false);

			// If the reception of data for a row is interrupted by data for the alternate
			// data channel or for text mode, the display of caption text will resume from the same
			// cursor position if a roll-up caption command is received and no PAC is given [...]
			if (context->mode != MODE_TEXT && context->have_cursor_position == 0)
			{
				// If no Preamble Address Code  is received, the base row shall default to row 15
				context->cursor_row = 14; // Default if the previous mode wasn't roll up already.
				context->cursor_column = 0;
				context->have_cursor_position = 1;
			}

			switch (command)
			{
				case COM_FAKE_RULLUP1:
					context->mode = MODE_FAKE_ROLLUP_1;
					break;
				case COM_ROLLUP2:
					context->mode = MODE_ROLLUP_2;
					break;
				case COM_ROLLUP3:
					context->mode = MODE_ROLLUP_3;
					break;
				case COM_ROLLUP4:
					context->mode = MODE_ROLLUP_4;
					break;
				default: // Impossible, but remove compiler warnings
					break;
			}
			break;
		case COM_CARRIAGERETURN:
			if (context->mode == MODE_PAINTON) // CR has no effect on painton mode according to zvbis' code
				break;
			if (context->mode == MODE_POPON) // CFS: Not sure about this. Is there a valid reason for CR in popup?
			{
				context->cursor_column = 0;
				if (context->cursor_row < CCX_DECODER_608_SCREEN_ROWS)
					context->cursor_row++;
				break;
			}
			if (context->output_format == CCX_OF_TRANSCRIPT)
			{
				write_cc_line(context, sub);
			}

			// In transcript mode, CR doesn't write the whole screen, to avoid
			// repeated lines.
			changes = check_roll_up(context);
			if (changes)
			{
				// Handle pop-on to roll-up transition timing
				// Use ts_start_of_current_line (when current line started) as the start time
				// This matches FFmpeg's behavior of timestamping when the display changed
				if (context->rollup_from_popon && context->ts_start_of_current_line > 0)
				{
					context->current_visible_start_ms = context->ts_start_of_current_line;
					context->rollup_from_popon = 0;
				}

				// Only if the roll up would actually cause a line to disappear we write the buffer
				if (context->output_format != CCX_OF_TRANSCRIPT)
				{
					if (write_cc_buffer(context, sub))
						context->screenfuls_counter++;
					if (context->settings->no_rollup)
						erase_memory(context, true); // Make sure the lines we just wrote aren't written again
				}
			}
			roll_up(context); // The roll must be done anyway of course.
			// When in pop-on to roll-up transition with changes=0 (first CR, only 1 line),
			// preserve the CR time so the next caption uses the display state change time,
			// not the character typing time. This matches FFmpeg's timing behavior.
			if (context->rollup_from_popon && !changes)
			{
				context->ts_start_of_current_line = get_fts(context->timing, context->my_field);
			}
			else
			{
				context->ts_start_of_current_line = -1; // Unknown.
			}
			if (changes)
				context->current_visible_start_ms = get_visible_start(context->timing, context->my_field);
			context->cursor_column = 0;
			break;
		case COM_ERASENONDISPLAYEDMEMORY:
			erase_memory(context, false);
			break;
		case COM_ERASEDISPLAYEDMEMORY:
			// Write it to disk before doing this, and make a note of the new
			// time it became clear.
			if (context->output_format == CCX_OF_TRANSCRIPT &&
			    (context->mode == MODE_FAKE_ROLLUP_1 ||
			     context->mode == MODE_ROLLUP_2 ||
			     context->mode == MODE_ROLLUP_3 ||
			     context->mode == MODE_ROLLUP_4))
			{
				// In transcript mode we just write the cursor line. The previous lines
				// should have been written already, so writing everything produces
				// duplicate lines.
				write_cc_line(context, sub);
			}
			else
			{
				if (context->output_format == CCX_OF_TRANSCRIPT)
					context->ts_start_of_current_line = context->current_visible_start_ms;
				if (write_cc_buffer(context, sub))
					context->screenfuls_counter++;
			}
			erase_memory(context, true);
			context->current_visible_start_ms = get_visible_start(context->timing, context->my_field);
			break;
		case COM_ENDOFCAPTION: // Switch buffers
			// The currently *visible* buffer is leaving, so now we know its ending
			// time. Time to actually write it to file.
			if (write_cc_buffer(context, sub))
				context->screenfuls_counter++;
			context->visible_buffer = (context->visible_buffer == 1) ? 2 : 1;
			context->current_visible_start_ms = get_visible_start(context->timing, context->my_field);
			context->cursor_column = 0;
			context->cursor_row = 0;
			context->current_color = context->settings->default_color;
			context->font = FONT_REGULAR;
			context->mode = MODE_POPON;
			break;
		case COM_DELETETOENDOFROW:
			delete_to_end_of_row(context);
			break;
		case COM_ALARMOFF:
		case COM_ALARMON:
			// These two are unused according to Robson's, and we wouldn't be able to do anything useful anyway
			break;
		case COM_RESUMEDIRECTCAPTIONING:
			context->mode = MODE_PAINTON;
			// ccx_common_logging.log_ftn ("\nWarning: Received ResumeDirectCaptioning, this mode is almost impossible.\n");
			// ccx_common_logging.log_ftn ("to transcribe to a text file.\n");
			break;
		default:
			ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "\rNot yet implemented.\n");
			break;
	}
	ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "\rCurrent mode: %d  Position: %d,%d	VisBuf: %d\n", context->mode,
				     context->cursor_row, context->cursor_column, context->visible_buffer);
	ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "\rCommand end: %02X %02X (%s)\n", c1, c2, command_type[command]);
}

void flush_608_context(ccx_decoder_608_context *context, struct cc_subtitle *sub)
{
	// We issue a EraseDisplayedMemory here so if there's any captions pending
	// they get written to Subtitle.
	handle_command(0x14, 0x2c, context, sub); // EDM
}

// CEA-608, Anex F 1.1.1. - Character Set Table / Special Characters
void handle_double(const unsigned char c1, const unsigned char c2, ccx_decoder_608_context *context)
{
	unsigned char c;
	if (context->channel != context->my_channel)
		return;
	if (c2 >= 0x30 && c2 <= 0x3f)
	{
		c = c2 + 0x50; // So if c>=0x80 && c<=0x8f, it comes from here
		ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "\rDouble: %02X %02X  -->  %c\n", c1, c2, c);
		write_char(c, context);
	}
}

/* Process EXTENDED CHARACTERS */
unsigned char handle_extended(unsigned char hi, unsigned char lo, ccx_decoder_608_context *context)
{
	// Handle channel change
	if (context->new_channel > 2)
	{
		context->new_channel -= 2;
		ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "\nChannel correction, now %d\n", context->new_channel);
	}
	context->channel = context->new_channel;
	if (context->channel != context->my_channel)
		return 0;

	// For lo values between 0x20-0x3f
	unsigned char c = 0;

	ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "\rExtended: %02X %02X\n", hi, lo);
	if (lo >= 0x20 && lo <= 0x3f && (hi == 0x12 || hi == 0x13))
	{
		switch (hi)
		{
			case 0x12:
				c = lo + 0x70; // So if c>=0x90 && c<=0xaf it comes from here
				break;
			case 0x13:
				c = lo + 0x90; // So if c>=0xb0 && c<=0xcf it comes from here
				break;
		}
		// This column change is because extended characters replace
		// the previous character (which is sent for basic decoders
		// to show something similar to the real char)
		if (context->cursor_column > 0)
			context->cursor_column--;

		write_char(c, context);
	}
	return 1;
}

/* Process PREAMBLE ACCESS CODES (PAC) */
void handle_pac(unsigned char c1, unsigned char c2, ccx_decoder_608_context *context)
{
	// Handle channel change
	if (context->new_channel > 2)
	{
		context->new_channel -= 2;
		ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "\nChannel correction, now %d\n", context->new_channel);
	}
	context->channel = context->new_channel;
	if (context->channel != context->my_channel)
		return;

	int row = rowdata[((c1 << 1) & 14) | ((c2 >> 5) & 1)];

	ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "\rPAC: %02X %02X", c1, c2);

	if (c2 >= 0x40 && c2 <= 0x5f)
	{
		c2 = c2 - 0x40;
	}
	else
	{
		if (c2 >= 0x60 && c2 <= 0x7f)
		{
			c2 = c2 - 0x60;
		}
		else
		{
			ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "\rThis is not a PAC!!!!!\n");
			return;
		}
	}
	context->current_color = pac2_attribs[c2][0];
	context->font = pac2_attribs[c2][1];
	int indent = pac2_attribs[c2][2];
	ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "  --  Position: %d:%d, color: %s,  font: %s\n", row,
				     indent, color_text[context->current_color][0], font_text[context->font]);
	if (context->settings->default_color == COL_USERDEFINED && (context->current_color == COL_WHITE || context->current_color == COL_TRANSPARENT))
		context->current_color = COL_USERDEFINED;
	if (context->mode != MODE_TEXT)
	{
		// According to Robson, row info is discarded in text mode
		// but column is accepted
		context->cursor_row = row - 1; // Since the array is 0 based
	}
	context->rollup_base_row = row - 1;
	context->cursor_column = indent;
	context->have_cursor_position = 1;
	if (context->mode == MODE_FAKE_ROLLUP_1 || context->mode == MODE_ROLLUP_2 ||
	    context->mode == MODE_ROLLUP_3 || context->mode == MODE_ROLLUP_4)
	{
		/* In roll-up, delete lines BELOW the PAC. Not sure (CFS) this is correct (possibly we may have to move the
		   buffer around instead) but it's better than leaving old characters in the buffer */
		struct eia608_screen *use_buffer = get_writing_buffer(context); // &wb->data608->buffer1;

		for (int j = row; j < CCX_DECODER_608_SCREEN_ROWS; j++)
		{
			if (use_buffer->row_used[j])
			{
				memset(use_buffer->characters[j], ' ', CCX_DECODER_608_SCREEN_WIDTH);
				use_buffer->characters[j][CCX_DECODER_608_SCREEN_WIDTH] = 0;

				for (int i = 0; i < CCX_DECODER_608_SCREEN_WIDTH; ++i)
				{
					use_buffer->colors[j][i] = context->settings->default_color;
					use_buffer->fonts[j][i] = FONT_REGULAR;
				}
				use_buffer->row_used[j] = 0;
			}
		}
	}
}

void handle_single(const unsigned char c1, ccx_decoder_608_context *context)
{
	if (c1 < 0x20 || context->channel != context->my_channel)
		return; // We don't allow special stuff here
	ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "%c", c1);

	write_char(c1, context);
}

void erase_both_memories(ccx_decoder_608_context *context, struct cc_subtitle *sub)
{
	erase_memory(context, false);
	// For the visible memory, we write the contents to disk
	// The currently *visible* buffer is leaving, so now we know its ending
	// time. Time to actually write it to file.
	if (write_cc_buffer(context, sub))
		context->screenfuls_counter++;
	context->current_visible_start_ms = get_visible_start(context->timing, context->my_field);
	context->cursor_column = 0;
	context->cursor_row = 0;
	context->current_color = context->settings->default_color;
	context->font = FONT_REGULAR;

	erase_memory(context, true);
}

int check_channel(unsigned char c1, ccx_decoder_608_context *context)
{
	int newchan = context->channel;
	if (c1 >= 0x10 && c1 <= 0x17)
		newchan = 1;
	else if (c1 >= 0x18 && c1 <= 0x1e)
		newchan = 2;
	if (newchan != context->channel)
	{
		ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "\nChannel change, now %d\n", newchan);
		if (context->channel != 3) // Don't delete memories if returning from XDS.
		{
			// erase_both_memories (wb); // 47cfr15.119.pdf, page 859, part f
			// CFS: Removed this because the specs say memories should be deleted if THE USER
			// changes the channel.
		}
	}
	return newchan;
}

/* Handle Command, special char or attribute and also check for
 * channel changes.
 * Returns 1 if something was written to screen, 0 otherwise */
int disCommand(unsigned char hi, unsigned char lo, ccx_decoder_608_context *context, struct cc_subtitle *sub)
{
	int wrote_to_screen = 0;

	/* Full channel changes are only allowed for "GLOBAL CODES",
	 * "OTHER POSITIONING CODES", "BACKGROUND COLOR CODES",
	 * "MID-ROW CODES".
	 * "PREAMBLE ACCESS CODES", "BACKGROUND COLOR CODES" and
	 * SPECIAL/SPECIAL CHARACTERS allow only switching
	 * between 1&3 or 2&4. */
	context->new_channel = check_channel(hi, context);
	// if (wb->data608->channel!=cc_channel)
	//	continue;

	if (hi >= 0x18 && hi <= 0x1f)
		hi = hi - 8;

	switch (hi)
	{
		case 0x10:
			if (lo >= 0x40 && lo <= 0x5f)
				handle_pac(hi, lo, context);
			break;
		case 0x11:
			if (lo >= 0x20 && lo <= 0x2f)
				handle_text_attr(hi, lo, context);
			if (lo >= 0x30 && lo <= 0x3f)
			{
				wrote_to_screen = 1;
				handle_double(hi, lo, context);
			}
			if (lo >= 0x40 && lo <= 0x7f)
				handle_pac(hi, lo, context);
			break;
		case 0x12:
		case 0x13:
			if (lo >= 0x20 && lo <= 0x3f)
			{
				wrote_to_screen = handle_extended(hi, lo, context);
			}
			if (lo >= 0x40 && lo <= 0x7f)
				handle_pac(hi, lo, context);
			break;
		case 0x14:
		case 0x15:
			if (lo >= 0x20 && lo <= 0x2f)
				handle_command(hi, lo, context, sub);
			if (lo >= 0x40 && lo <= 0x7f)
				handle_pac(hi, lo, context);
			break;
		case 0x16:
			if (lo >= 0x40 && lo <= 0x7f)
				handle_pac(hi, lo, context);
			break;
		case 0x17:
			if (lo >= 0x21 && lo <= 0x23)
				handle_command(hi, lo, context, sub);
			if (lo >= 0x2e && lo <= 0x2f)
				handle_text_attr(hi, lo, context);
			if (lo >= 0x40 && lo <= 0x7f)
				handle_pac(hi, lo, context);
			break;
	}
	return wrote_to_screen;
}

/* If private data is NULL, then only XDS will be processed */
int process608(const unsigned char *data, int length, void *private_data, struct cc_subtitle *sub)
{
	struct ccx_decoder_608_report *report = NULL;
	struct lib_cc_decode *dec_ctx = private_data;
	struct ccx_decoder_608_context *context;
	int i;

	if (dec_ctx->current_field == 1 && (dec_ctx->extract == 1 || dec_ctx->extract == 12))
	{
		context = dec_ctx->context_cc608_field_1;
	}
	else if (dec_ctx->current_field == 2 && (dec_ctx->extract == 2 || dec_ctx->extract == 12))
	{
		context = dec_ctx->context_cc608_field_2;
	}
	else if (dec_ctx->current_field == 2 && dec_ctx->extract == 1)
	{
		context = NULL;
	}
	else
	{
		return -1;
	}
	if (context)
	{
		report = context->report;
		context->bytes_processed_608 += length;
	}
	if (!data)
	{
		return -1;
	}
	for (i = 0; i < length; i = i + 2)
	{
		unsigned char hi, lo;
		int wrote_to_screen = 0;

		hi = data[i] & 0x7F;	 // Get rid of parity bit
		lo = data[i + 1] & 0x7F; // Get rid of parity bit

		if (hi == 0 && lo == 0) // Just padding
			continue;

		// printf ("\r[%02X:%02X]\n",hi,lo);

		if (hi >= 0x10 && hi <= 0x1e)
		{
			int ch = (hi <= 0x17) ? 1 : 2;
			if (context == NULL || context->my_field == 2) // Originally: current_field from sequencing.c. Seems to be just to change channel, so context->my_field seems good.
				ch += 2;

			if (report)
				report->cc_channels[ch - 1] = 1;
		}

		if (hi >= 0x01 && hi <= 0x0E && (context == NULL || context->my_field == 2)) // XDS can only exist in field 2.
		{
			if (context)
				context->channel = 3;
			if (!in_xds_mode)
			{
				ts_start_of_xds = get_fts(dec_ctx->timing, dec_ctx->current_field);
				in_xds_mode = 1;
			}
			if (report)
				report->xds = 1;
		}
		if (hi == 0x0F && in_xds_mode && (context == NULL || context->my_field == 2)) // End of XDS block
		{
			in_xds_mode = 0;
			do_end_of_xds(sub, dec_ctx->xds_ctx, lo);
			if (context)
				context->channel = context->new_channel; // Switch from channel 3
			continue;
		}
		if (hi >= 0x10 && hi <= 0x1F) // Non-character code or special/extended char
					      // http://www.theneitherworld.com/mcpoodle/SCC_TOOLS/DOCS/CC_CODES.HTML
					      // http://www.theneitherworld.com/mcpoodle/SCC_TOOLS/DOCS/CC_CHARS.HTML
		{
			if (!context || context->my_field == 2)
				in_xds_mode = 0; // Back to normal (CEA 608-8.6.2)
			if (!context)		 // Not XDS and we don't have a writebuffer, nothing else would have an effect
				continue;

			// We were writing characters before, start a new line for
			// diagnostic output from disCommand()
			if (context->textprinted == 1)
			{
				ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "\n");
				context->textprinted = 0;
			}

			if (context->last_c1 == hi && context->last_c2 == lo)
			{
				// Duplicate dual code, discard. Correct to do it only in
				// non-XDS, XDS codes shall not be repeated.
				ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "Skipping command %02X,%02X Duplicate\n", hi, lo);
				// Ignore only the first repetition
				context->last_c1 = -1;
				context->last_c2 = -1;
				continue;
			}
			context->last_c1 = hi;
			context->last_c2 = lo;
			wrote_to_screen = disCommand(hi, lo, context, sub);
			if (sub->got_output)
			{
				i += 2; // Otherwise we wouldn't be counting this byte pair
				break;
			}
		}
		else
		{
			if (in_xds_mode && (context == NULL || context->my_field == 2))
			{
				process_xds_bytes(dec_ctx->xds_ctx, hi, lo);
				continue;
			}
			if (!context) // No XDS code after this point, and user doesn't want captions.
				continue;

			context->last_c1 = -1;
			context->last_c2 = -1;

			if (hi >= 0x20) // Standard characters (always in pairs)
			{
				// Only print if the channel is active
				if (context->channel != context->my_channel)
					continue;

				if (context->textprinted == 0)
				{
					ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "\n");
					context->textprinted = 1;
				}

				handle_single(hi, context);
				handle_single(lo, context);
				wrote_to_screen = 1;
				context->last_c1 = 0;
				context->last_c2 = 0;
			}

			if (!context->textprinted && context->channel == context->my_channel)
			{ // Current FTS information after the characters are shown
				ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "Current FTS: %s\n", print_mstime_static(get_fts(dec_ctx->timing, context->my_field)));
				// printf("  N:%u", unsigned(fts_now) );
				// printf("  G:%u", unsigned(fts_global) );
				// printf("  F:%d %d %d %d\n",
				//	   current_field, cb_field1, cb_field2, cb_708 );
			}

			if (wrote_to_screen && context->settings->direct_rollup && // If direct_rollup is enabled and
			    (context->mode == MODE_FAKE_ROLLUP_1 ||		   // we are in rollup mode, write now.
			     context->mode == MODE_ROLLUP_2 ||
			     context->mode == MODE_ROLLUP_3 ||
			     context->mode == MODE_ROLLUP_4))
			{
				// We don't increase screenfuls_counter here.
				write_cc_buffer(context, sub);
				context->current_visible_start_ms = get_visible_start(context->timing, context->my_field);
			}
		}
		if (wrote_to_screen && context->cc_to_stdout)
			fflush(stdout);
	} // for
	return i;
}

/* Return a pointer to a string that holds the printable characters
 * of the caption data block. FOR DEBUG PURPOSES ONLY! */
unsigned char *debug_608_to_ASC(unsigned char *cc_data, int channel)
{
	static unsigned char output[3];

	unsigned char cc_valid = (cc_data[0] & 4) >> 2;
	unsigned char cc_type = cc_data[0] & 3;
	unsigned char hi, lo;

	output[0] = ' ';
	output[1] = ' ';
	output[2] = '\x00';

	if (cc_valid && cc_type == channel)
	{
		hi = cc_data[1] & 0x7F; // Get rid of parity bit
		lo = cc_data[2] & 0x7F; // Get rid of parity bit
		if (hi >= 0x20)
		{
			output[0] = hi;
			output[1] = (lo >= 0x20 ? lo : '.');
			output[2] = '\x00';
		}
		else
		{
			output[0] = '<';
			output[1] = '>';
			output[2] = '\x00';
		}
	}
	return output;
}
