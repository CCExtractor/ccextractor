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

// TODO: CHANNEL_2 has been disabled, because I am not sure how to know if channel 1 or 2 should be used.
// TODO: deal with "\n" vs "\r\n"
// TODO: colors. I don't have example files to work with and double check what I do

// TO Codes (Tab Offset)
// Are the column offset the caption should be placed at to be more precise
// than preamble codes as preamble codes can only be precise by 4 columns
// e.g.
// indent * 4 + tab_offset
// where tab_offset can be 1 to 3. If no offset wants to be used tab_offset disappears

const unsigned char TO_CHANNEL_1 = 0x17;
// const unsigned char TO_CHANNEL_2 = 0x1f;

// The second byte for tab offsets, is the 0x20 + column

// First bytes of Control Codes
const unsigned char CHANNEL_1 = 0x14;
// const unsigned char CHANNEL_2 = 0x1c;

// These are all the second bytes of the Control Codes
// TODO: use an enum for all the control codes

// Resume caption loading
const unsigned char RCL = 0x20;
// End of Caption
const unsigned char EOC = 0x2f;
// Erase Non-Displayed Memory
const unsigned char ENM = 0x2e;

void write_control_code(const int fd, const unsigned char first, const unsigned char second)
{
	dprintf(fd, "%02x%02x", odd_parity(first), odd_parity(second));
}

// row and column start at 0
void add_preamble_code(const int fd, const unsigned char row, const unsigned char column)
{
	dprintf(fd, "%02x%02x", odd_parity(FIRST_BYTE_CODE_PAIR_CHANNEL_1[row]), odd_parity(0x50 + column / 4 * 2 + ROW_OFFSET[row]));
	unsigned char offset = column % 4;
	if (offset)
	{
		// TODO: channels
		write(fd, " ", 1);
		write_control_code(fd, TO_CHANNEL_1, 0x20 + offset);
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

void write_font(int fd, /* unsigned char channel, */ enum font_bits font)
{
	// TODO: Dependant on channel
	unsigned char channel = MIDROW_CHANNEL_1;
	unsigned char attrib;
	switch (font)
	{
		case FONT_REGULAR:
			attrib = 0x20;
			break;
		case FONT_UNDERLINED:
			attrib = 0x21;
			break;
		case FONT_ITALICS:
			attrib = 0x2e;
			break;
		case FONT_UNDERLINED_ITALICS:
			attrib = 0x2f;
			break;
		default:
			fatal(1, "Unknown font");
			break;
	}
	dprintf(fd, "%02x%02x", odd_parity(MIDROW_CHANNEL_1), odd_parity(attrib));
}

int write_cc_buffer_as_scc(const struct eia608_screen *data, struct encoder_ctx *context)
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
				write_control_code(context->out->fh, CHANNEL_1, RCL);
			}
			write(context->out->fh, " ", 1);
			first_row = false;

			int first, last;
			find_limit_characters(data->characters[row], &first, &last, CCX_DECODER_608_SCREEN_WIDTH);
			for (unsigned char column = first; column <= last; ++column)
			{
				if (current_font != data->fonts[row][column])
				{
					if ((column - first) % 2 != 0)
					{
						dprintf(context->out->fh, "%02x ", odd_parity('\0'));
					}
					if (data->characters[row][column] == ' ')
					{
						// The MID-ROW code is going to move the cursor to the
						// right so there is no need to have a space
						add_preamble_code(context->out->fh, row, column);
						++column;
					}
					else
					{
						add_preamble_code(context->out->fh, row, column - 1);
					}
					initial_preamble_code_added = true;
					write(context->out->fh, " ", 1);
					write_font(context->out->fh, data->fonts[row][column]);
					current_font = data->fonts[row][column];
				}
				if (!initial_preamble_code_added)
				{
					add_preamble_code(context->out->fh, row, first);
					initial_preamble_code_added = true;
				}
				if ((column - first) % 2 == 0)
				{
					write(context->out->fh, " ", 1);
				}
				dprintf(context->out->fh, "%02x", odd_parity(data->characters[row][column]));
			}
			// Add '\0' (0x80 with parity) to have pairs
			if ((last - first) % 2 == 0)
			{
				dprintf(context->out->fh, "%02x", odd_parity('\0'));
			}
		}
	}
	write(context->out->fh, "\n\n", 2);
	// Clear screen
	add_timestamp(context->out->fh, data->end_time);
	write_control_code(context->out->fh, CHANNEL_1, RCL);
	write(context->out->fh, " ", 1);
	write_control_code(context->out->fh, CHANNEL_1, EOC);
	write(context->out->fh, " ", 1);
	write_control_code(context->out->fh, CHANNEL_1, ENM);

	write(context->out->fh, "\n\n", 2);
	return 1;
}
