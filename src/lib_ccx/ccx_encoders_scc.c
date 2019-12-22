// Reference: https://www.govinfo.gov/app/details/CFR-2011-title47-vol1/CFR-2011-title47-vol1-sec15-119/summary
// Implementatin issue: https://github.com/CCExtractor/ccextractor/issues/1120

#include "lib_ccx.h"
#include <stdbool.h>

// Adds a parity bit if needed
unsigned char odd_parity(const unsigned char byte)
{
	return byte | !(__builtin_parity(byte) % 2) << 7;
}

// Rows, 2, 4, 6, 8, 10, 13 and 15 need 0x20 to be added to them to give the
// second byte of the preamble address codes.
const unsigned char ROW_OFFSET[] = {
	0x00,
	0x20,
	0x00,
	0x20,
	0x00,
	0x20,
	0x00,
	0x20,
	0x00,
	0x20,
	0x00,
	0x00,
	0x20,
	0x00,
	0x20
};

const unsigned char FIRST_BYTE_CODE_PAIR_CHANNEL_1[] = {
	0x11,
	0x11,
	0x12,
	0x12,
	0x15,
	0x15,
	0x16,
	0x16,
	0x17,
	0x17,
	0x10,
	0x13,
	0x13,
	0x14,
	0x14
};

// TODO: CHANNEL_2 has been disabled, because I am not sure of the reliability of data->channel
// TODO: deal with "\n" vs "\r\n"
// TODO: colors. I don't have example files to work with and double check what I do

// TO Codes (Tab Offset)
// Are the column offset the caption should be placed at to be more precise
// than preamble codes as preamble codes can only be precise by 4 columns
// e.g.
// indent * 4 + tab_offset
// where tab_offset can be 1 to 3. If no offset wants to be used tab_offset disappears

const unsigned char TO_CHANNEL_1 = 0x17;
const unsigned char TO_CHANNEL_2 = 0x1f;
const unsigned char TO_OFFSET = 0x20;

// The second byte for tab offsets, is the 0x20 + column

// First bytes of Control Codes
const unsigned char MISCELLANEOUS_CHANNEL_1 = 0x14;
const unsigned char MISCELLANEOUS_CHANNEL_2 = 0x1c;

// These are all the second bytes of the Control Codes
// TODO: use an enum for all the control codes

// Resume caption loading
const unsigned char RCL = 0x20;
const char RCL_DIS[] = "{RCL}";
// End of Caption
const unsigned char EOC = 0x2f;
const char EOC_DIS[] = "{EOC}";
// Erase Non-Displayed Memory
const unsigned char ENM = 0x2e;
const char ENM_DIS[] = "{ENM}";

// Fonts

const char WHITE_DIS[] = "{Wh}";
const char ITALICS_DIS[] = "{I}";
const char ITALICS_UNDERLINE_DIS[] = "{I}";
const char WHITE_UNDERLINE_DIS[] = "{I}";

const char *disassemble_code(const unsigned char first, const unsigned char second)
{
	switch (first)
	{
		case TO_CHANNEL_1:
		case TO_CHANNEL_2:
			switch (second - TO_OFFSET)
			{
				case 1:
					return "{TO1}";
					break;
				case 2:
					return "{TO2}";
					break;
				case 3:
					return "{TO3}";
					break;
				default:
					fatal(1, "Tab offsets should only be 1 to 3");
					return NULL;
					break;
			}
			break;
		case MISCELLANEOUS_CHANNEL_1:
		case MISCELLANEOUS_CHANNEL_2:
			switch (second)
			{
				case RCL:
					return "{RCL}";
					break;
				case EOC:
					return "{EOC}";
					break;
				case ENM:
					return "{ENM}";
					break;
				default:
					fatal(1, "Unknown miscellaneous control codes (second byte)");
					return NULL;
					break;
			}
			break;
		default:
			fatal(1, "Invalid channel byte (first byte).");
			return NULL;
	}
}

void write_control_code(const int fd, const unsigned char first, const unsigned char second, bool disassemble)
{
	if (disassemble)
	{
		const char *disassembled = disassemble_code(first, second);
		write(fd, disassembled, strlen(disassembled));
	}
	else
	{
		dprintf(fd, "%02x%02x", odd_parity(first), odd_parity(second));
	}
}

void disassemble_preamble_code(const unsigned char row, const unsigned char column, char code[])
{
	sprintf(code, "{%02d%02d}", row + 1, column);
}

// row and column start at 0
void add_preamble_code(const int fd, const unsigned char row, const unsigned char column, bool disassemble)
{
	if (disassemble)
	{
		char preamble_code[9];
		disassemble_preamble_code(row, column - column % 4, preamble_code);
		write(fd, preamble_code, strlen(preamble_code));
	}
	else
	{
		dprintf(fd, "%02x%02x",
				odd_parity(FIRST_BYTE_CODE_PAIR_CHANNEL_1[row]),
				odd_parity(0x50 + column / 4 * 2 + ROW_OFFSET[row]));
	}
	unsigned char offset = column % 4;
	if (offset)
	{
		// TODO: channels
		if (!disassemble)
		{
			write(fd, " ", 1);
		}
		write_control_code(fd, TO_CHANNEL_1, 0x20 + offset, disassemble);
	}
}

void add_timestamp(int fd, LLONG time)
{
	unsigned hour, minute, second, frame;
	millis_to_time(time, &hour, &minute, &second, &frame);
	// This frame number seems like it couldn't be more wrong
	dprintf(fd, "%02d:%02d:%02d:%02.f\t", hour, minute, second, (float) frame / 30);
}

#define MIDROW_CHANNEL_1 0x11

void write_font(int fd, /* unsigned char channel, */ enum font_bits font, bool disassemble)
{
	// TODO: Dependant on channel
	unsigned char channel = MIDROW_CHANNEL_1;
	switch (font)
	{
		case FONT_REGULAR:
			if (disassemble)
			{
				write(fd, WHITE_DIS, sizeof(WHITE_DIS) - 1);
			}
			else
			{
				dprintf(fd, "%02x%02x", odd_parity(MIDROW_CHANNEL_1), odd_parity(0x20));
			}
			break;
		case FONT_UNDERLINED:
			if (disassemble)
			{
				write(fd, WHITE_UNDERLINE_DIS, sizeof(WHITE_UNDERLINE_DIS) - 1);
			}
			else
			{
				dprintf(fd, "%02x%02x", odd_parity(MIDROW_CHANNEL_1), odd_parity(0x21));
			}
			break;
		case FONT_ITALICS:
			if (disassemble)
			{
				write(fd, ITALICS_DIS, sizeof(ITALICS_DIS) - 1);
			}
			else
			{
				dprintf(fd, "%02x%02x", odd_parity(MIDROW_CHANNEL_1), odd_parity(0x2e));
			}
			break;
		case FONT_UNDERLINED_ITALICS:
			if (disassemble)
			{
				write(fd, ITALICS_UNDERLINE_DIS, sizeof(ITALICS_UNDERLINE_DIS) - 1);
			}
			else
			{
				dprintf(fd, "%02x%02x", odd_parity(MIDROW_CHANNEL_1), odd_parity(0x2f));
			}
			break;
		default:
			fatal(1, "Unknown font");
			break;
	}
}

void clear_screen(int fd, LLONG end_time, bool disassemble)
{
	add_timestamp(fd, end_time);
	write_control_code(fd, MISCELLANEOUS_CHANNEL_1, RCL, disassemble);
	if (!disassemble)
		write(fd, " ", 1);
	write_control_code(fd, MISCELLANEOUS_CHANNEL_1, EOC, disassemble);
	if (!disassemble)
		write(fd, " ", 1);
	write_control_code(fd, MISCELLANEOUS_CHANNEL_1, ENM, disassemble);
	if (disassemble)
	{
		write(fd, "\n", 1);
	}
	else
	{
		write(fd, "\n\n", 2);
	}
}

void add_padding(int fd, bool disassemble)
{

	if (disassemble)
	{
		write(fd, "_", 1);
	}
	else
	{
		dprintf(fd, "%02x ", odd_parity('\0'));
	}
}

int write_cc_buffer_as_scenarist(const struct eia608_screen *data, struct encoder_ctx *context, bool disassemble)
{
	add_timestamp(context->out->fh, data->start_time);
	bool first_row = true;
	enum font_bits current_font = FONT_REGULAR;
	for (uint8_t row = 0; row < 15; ++row)
	{
		if (data->row_used[row])
		{
			bool initial_preamble_code_added = false;
			if (first_row)
			{
				write_control_code(context->out->fh, MISCELLANEOUS_CHANNEL_1, RCL, disassemble);
			}
			if (!disassemble)
			{
				write(context->out->fh, " ", 1);
			}
			first_row = false;

			int first, last;
			find_limit_characters(data->characters[row], &first, &last, CCX_DECODER_608_SCREEN_WIDTH);
			for (unsigned char column = first; column <= last; ++column)
			{
				if (current_font != data->fonts[row][column])
				{
					if ((column - first) % 2 != 0)
					{
						add_padding(context->out->fh, disassemble);
					}
					if (data->characters[row][column] == ' ')
					{
						// The MID-ROW code is going to move the cursor to the
						// right so there is no need to have a space
						add_preamble_code(context->out->fh, row, column, disassemble);
						++column;
					}
					else
					{
						add_preamble_code(context->out->fh, row, column - 1, disassemble);
					}
					initial_preamble_code_added = true;
					if (!disassemble)
						write(context->out->fh, " ", 1);
					write_font(context->out->fh, data->fonts[row][column], disassemble);
					current_font = data->fonts[row][column];
				}
				if (!initial_preamble_code_added)
				{
					add_preamble_code(context->out->fh, row, first, disassemble);
					initial_preamble_code_added = true;
				}
				if ((column - first) % 2 == 0 && !disassemble)
				{
					write(context->out->fh, " ", 1);
				}
				if (disassemble)
				{
					dprintf(context->out->fh, "%c", data->characters[row][column]);
				}
				else
				{
					dprintf(context->out->fh, "%02x", odd_parity(data->characters[row][column]));
				}
			}
			// Add '\0' (0x80 with parity) to have pairs
			if ((last - first) % 2 == 0)
			{
				add_padding(context->out->fh, disassemble);
			}
		}
	}
	write(context->out->fh, "\n\n", disassemble ? 1 : 2);
	clear_screen(context->out->fh, data->end_time, disassemble);
	return 1;
}

int write_cc_buffer_as_ccd(const struct eia608_screen *data, struct encoder_ctx *context)
{
	if (!context->wrote_ccd_channel_header)
	{
		dprintf(context->out->fh, "CHANNEL %d\n\n", data->channel);
		context->wrote_ccd_channel_header = true;
	}
	return write_cc_buffer_as_scenarist(data, context, true);
}


int write_cc_buffer_as_scc(const struct eia608_screen *data, struct encoder_ctx *context)
{
	return write_cc_buffer_as_scenarist(data, context, false);
}
