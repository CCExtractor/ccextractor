// Reference: https://www.govinfo.gov/app/details/CFR-2011-title47-vol1/CFR-2011-title47-vol1-sec15-119/summary
// Implementatin issue: https://github.com/CCExtractor/ccextractor/issues/1120

#include "lib_ccx.h"
#include <stdbool.h>

// Adds a parity bit if needed
unsigned char odd_parity(const unsigned char byte)
{
	return byte | !(cc608_parity(byte) % 2) << 7;
}

struct control_code_info
{
	unsigned int byte1_odd;
	unsigned int byte1_even;
	unsigned int byte2;
	char *assembly;
};

// TO Codes (Tab Offset)
// Are the column offset the caption should be placed at to be more precise
// than preamble codes as preamble codes can only be precise by 4 columns
// e.g.
// indent * 4 + tab_offset
// where tab_offset can be 1 to 3. If the offset isn't within 1 to 3, it is
// unneeded and disappears, as the indent is enough.

enum control_code
{
	// Mid-row
	Wh,
	WhU,
	Gr,
	GrU,
	Bl,
	BlU,
	Cy,
	CyU,
	R,
	RU,
	Y,
	YU,
	Ma,
	MaU,
	Bk,
	BkU,
	I,
	IU,

	// Miscellaneous
	RCL,
	EDM,
	ENM,
	EOC,

	TAB_OFFSET_START,
	TO1,
	TO2,
	TO3,

	// Preamble
	PREAMBLE_CC_START,
	// Prefixed with underscores because identifiers can't start with digits
	_0100,
	_0104,
	_0108,
	_0112,
	_0116,
	_0120,
	_0124,
	_0128,

	_0200,
	_0204,
	_0208,
	_0212,
	_0216,
	_0220,
	_0224,
	_0228,

	_0300,
	_0304,
	_0308,
	_0312,
	_0316,
	_0320,
	_0324,
	_0328,

	_0400,
	_0404,
	_0408,
	_0412,
	_0416,
	_0420,
	_0424,
	_0428,

	_0500,
	_0504,
	_0508,
	_0512,
	_0516,
	_0520,
	_0524,
	_0528,

	_0600,
	_0604,
	_0608,
	_0612,
	_0616,
	_0620,
	_0624,
	_0628,

	_0700,
	_0704,
	_0708,
	_0712,
	_0716,
	_0720,
	_0724,
	_0728,

	_0800,
	_0804,
	_0808,
	_0812,
	_0816,
	_0820,
	_0824,
	_0828,

	_0900,
	_0904,
	_0908,
	_0912,
	_0916,
	_0920,
	_0924,
	_0928,

	_1000,
	_1004,
	_1008,
	_1012,
	_1016,
	_1020,
	_1024,
	_1028,

	_1100,
	_1104,
	_1108,
	_1112,
	_1116,
	_1120,
	_1124,
	_1128,

	_1200,
	_1204,
	_1208,
	_1212,
	_1216,
	_1220,
	_1224,
	_1228,

	_1300,
	_1304,
	_1308,
	_1312,
	_1316,
	_1320,
	_1324,
	_1328,

	_1400,
	_1404,
	_1408,
	_1412,
	_1416,
	_1420,
	_1424,
	_1428,

	_1500,
	_1504,
	_1508,
	_1512,
	_1516,
	_1520,
	_1524,
	_1528,

	// Must be kept at the end
	CONTROL_CODE_MAX
};

const struct control_code_info control_codes[CONTROL_CODE_MAX] = {
    // Mid-row
    [Wh] = {0x11, 0x19, 0x20, "{Wh}"},	 // White
    [WhU] = {0x11, 0x19, 0x21, "{WhU}"}, // White underline
    [Gr] = {0x11, 0x19, 0xa2, "{Gr}"},	 // Green
    [GrU] = {0x11, 0x19, 0x23, "{GrU}"}, // Green underline
    [Bl] = {0x11, 0x19, 0xa4, "{Bl}"},	 // Blue
    [BlU] = {0x11, 0x19, 0x25, "{BlU}"}, // Blue underline
    [Cy] = {0x11, 0x19, 0x26, "{Cy}"},	 // Cyan
    [CyU] = {0x11, 0x19, 0xa7, "{CyU}"}, // Cyan underline
    [R] = {0x11, 0x19, 0xa8, "{R}"},	 // Red
    [RU] = {0x11, 0x19, 0x29, "{RU}"},	 // Red underline
    [Y] = {0x11, 0x19, 0x2a, "{Y}"},	 // Yellow
    [YU] = {0x11, 0x19, 0xab, "{YU}"},	 // Yellow underline
    [Ma] = {0x11, 0x19, 0x2c, "{Ma}"},	 // Magenta
    [MaU] = {0x11, 0x19, 0xad, "{MaU}"}, // Magenta underline
    [Bk] = {0x11, 0x19, 0xae, "{Bk}"},	 // Black
    [BkU] = {0x11, 0x19, 0x2f, "{BkU}"}, // Black underline
    [I] = {0x11, 0x19, 0x2e, "{I}"},	 // White italic
    [IU] = {0x11, 0x19, 0x2f, "{IU}"},	 // White italic underline

    // Miscellaneous
    [RCL] = {0x14, 0x1c, 0x20, "{RCL}"}, // Resume caption loading
    [EDM] = {0x14, 0x1c, 0x2c, "{EDM}"}, // Erase displayed memory
    [ENM] = {0x14, 0x1c, 0x2e, "{ENM}"}, // Erase non-displayed memory
    [EOC] = {0x14, 0x1c, 0x2f, "{EOC}"}, // End of caption (swap memory)

    [TO1] = {0x17, 0x1f, 0x21, "{TO1}"}, // Tab offset 1 column
    [TO2] = {0x17, 0x1f, 0x22, "{TO2}"}, // Tab offset 2 column
    [TO3] = {0x17, 0x1f, 0x23, "{TO3}"}, // Tab offset 3 column

    // Preamble
    // Format: _[COL][ROW]
    [_0100] = {0x11, 0x19, 0x50, "{0100}"},
    [_0104] = {0x11, 0x19, 0x52, "{0104}"},
    [_0108] = {0x11, 0x19, 0x54, "{0108}"},
    [_0112] = {0x11, 0x19, 0x56, "{0112}"},
    [_0116] = {0x11, 0x19, 0x58, "{0116}"},
    [_0120] = {0x11, 0x19, 0x5a, "{0120}"},
    [_0124] = {0x11, 0x19, 0x5c, "{0124}"},
    [_0128] = {0x11, 0x19, 0x5f, "{0128}"},

    [_0200] = {0x11, 0x19, 0x70, "{0200}"},
    [_0204] = {0x11, 0x19, 0x72, "{0204}"},
    [_0208] = {0x11, 0x19, 0x74, "{0208}"},
    [_0212] = {0x11, 0x19, 0x76, "{0212}"},
    [_0216] = {0x11, 0x19, 0x78, "{0216}"},
    [_0220] = {0x11, 0x19, 0x7a, "{0220}"},
    [_0224] = {0x11, 0x19, 0x7c, "{0224}"},
    [_0228] = {0x11, 0x19, 0x7f, "{0228}"},

    [_0300] = {0x12, 0x1a, 0x50, "{0300}"},
    [_0304] = {0x12, 0x1a, 0x52, "{0304}"},
    [_0308] = {0x12, 0x1a, 0x54, "{0308}"},
    [_0312] = {0x12, 0x1a, 0x56, "{0312}"},
    [_0316] = {0x12, 0x1a, 0x58, "{0316}"},
    [_0320] = {0x12, 0x1a, 0x5a, "{0320}"},
    [_0324] = {0x12, 0x1a, 0x5c, "{0324}"},
    [_0328] = {0x12, 0x1a, 0x5f, "{0328}"},

    [_0400] = {0x12, 0x1a, 0x70, "{0400}"},
    [_0404] = {0x12, 0x1a, 0x72, "{0404}"},
    [_0408] = {0x12, 0x1a, 0x74, "{0408}"},
    [_0412] = {0x12, 0x1a, 0x76, "{0412}"},
    [_0416] = {0x12, 0x1a, 0x78, "{0416}"},
    [_0420] = {0x12, 0x1a, 0x7a, "{0420}"},
    [_0424] = {0x12, 0x1a, 0x7c, "{0424}"},
    [_0428] = {0x12, 0x1a, 0x7f, "{0428}"},

    [_0500] = {0x15, 0x1d, 0x50, "{0500}"},
    [_0504] = {0x15, 0x1d, 0x52, "{0504}"},
    [_0508] = {0x15, 0x1d, 0x54, "{0508}"},
    [_0512] = {0x15, 0x1d, 0x56, "{0512}"},
    [_0516] = {0x15, 0x1d, 0x58, "{0516}"},
    [_0520] = {0x15, 0x1d, 0x5a, "{0520}"},
    [_0524] = {0x15, 0x1d, 0x5c, "{0524}"},
    [_0528] = {0x15, 0x1d, 0x5f, "{0528}"},

    [_0600] = {0x15, 0x1d, 0x70, "{0600}"},
    [_0604] = {0x15, 0x1d, 0x72, "{0604}"},
    [_0608] = {0x15, 0x1d, 0x74, "{0608}"},
    [_0612] = {0x15, 0x1d, 0x76, "{0612}"},
    [_0616] = {0x15, 0x1d, 0x78, "{0616}"},
    [_0620] = {0x15, 0x1d, 0x7a, "{0620}"},
    [_0624] = {0x15, 0x1d, 0x7c, "{0624}"},
    [_0628] = {0x15, 0x1d, 0x7f, "{0628}"},

    [_0700] = {0x16, 0x1e, 0x50, "{0700}"},
    [_0704] = {0x16, 0x1e, 0x52, "{0704}"},
    [_0708] = {0x16, 0x1e, 0x54, "{0708}"},
    [_0712] = {0x16, 0x1e, 0x56, "{0712}"},
    [_0716] = {0x16, 0x1e, 0x58, "{0716}"},
    [_0720] = {0x16, 0x1e, 0x5a, "{0720}"},
    [_0724] = {0x16, 0x1e, 0x5c, "{0724}"},
    [_0728] = {0x16, 0x1e, 0x5f, "{0728}"},

    [_0800] = {0x16, 0x1e, 0x70, "{0800}"},
    [_0804] = {0x16, 0x1e, 0x72, "{0804}"},
    [_0808] = {0x16, 0x1e, 0x74, "{0808}"},
    [_0812] = {0x16, 0x1e, 0x76, "{0812}"},
    [_0816] = {0x16, 0x1e, 0x78, "{0816}"},
    [_0820] = {0x16, 0x1e, 0x7a, "{0820}"},
    [_0824] = {0x16, 0x1e, 0x7c, "{0824}"},
    [_0828] = {0x16, 0x1e, 0x7f, "{0828}"},

    [_0900] = {0x17, 0x1f, 0x50, "{0900}"},
    [_0904] = {0x17, 0x1f, 0x52, "{0904}"},
    [_0908] = {0x17, 0x1f, 0x54, "{0908}"},
    [_0912] = {0x17, 0x1f, 0x56, "{0912}"},
    [_0916] = {0x17, 0x1f, 0x58, "{0916}"},
    [_0920] = {0x17, 0x1f, 0x5a, "{0920}"},
    [_0924] = {0x17, 0x1f, 0x5c, "{0924}"},
    [_0928] = {0x17, 0x1f, 0x5f, "{0928}"},

    [_1000] = {0x17, 0x1f, 0x70, "{1000}"},
    [_1004] = {0x17, 0x1f, 0x72, "{1004}"},
    [_1008] = {0x17, 0x1f, 0x74, "{1008}"},
    [_1012] = {0x17, 0x1f, 0x76, "{1012}"},
    [_1016] = {0x17, 0x1f, 0x78, "{1016}"},
    [_1020] = {0x17, 0x1f, 0x7a, "{1020}"},
    [_1024] = {0x17, 0x1f, 0x7c, "{1024}"},
    [_1028] = {0x17, 0x1f, 0x7f, "{1028}"},

    [_1100] = {0x10, 0x18, 0x50, "{1100}"},
    [_1104] = {0x10, 0x18, 0x52, "{1104}"},
    [_1108] = {0x10, 0x18, 0x54, "{1108}"},
    [_1112] = {0x10, 0x18, 0x56, "{1112}"},
    [_1116] = {0x10, 0x18, 0x58, "{1116}"},
    [_1120] = {0x10, 0x18, 0x5a, "{1120}"},
    [_1124] = {0x10, 0x18, 0x5c, "{1124}"},
    [_1128] = {0x10, 0x18, 0x5f, "{1128}"},

    [_1200] = {0x13, 0x1b, 0x50, "{1200}"},
    [_1204] = {0x13, 0x1b, 0x52, "{1204}"},
    [_1208] = {0x13, 0x1b, 0x54, "{1208}"},
    [_1212] = {0x13, 0x1b, 0x56, "{1212}"},
    [_1216] = {0x13, 0x1b, 0x58, "{1216}"},
    [_1220] = {0x13, 0x1b, 0x5a, "{1220}"},
    [_1224] = {0x13, 0x1b, 0x5c, "{1224}"},
    [_1228] = {0x13, 0x1b, 0x5f, "{1228}"},

    [_1300] = {0x13, 0x1b, 0x70, "{1300}"},
    [_1304] = {0x13, 0x1b, 0x72, "{1304}"},
    [_1308] = {0x13, 0x1b, 0x74, "{1308}"},
    [_1312] = {0x13, 0x1b, 0x76, "{1312}"},
    [_1316] = {0x13, 0x1b, 0x78, "{1316}"},
    [_1320] = {0x13, 0x1b, 0x7a, "{1320}"},
    [_1324] = {0x13, 0x1b, 0x7c, "{1324}"},
    [_1328] = {0x13, 0x1b, 0x7f, "{1328}"},

    [_1400] = {0x14, 0x1c, 0x50, "{1400}"},
    [_1404] = {0x14, 0x1c, 0x52, "{1404}"},
    [_1408] = {0x14, 0x1c, 0x54, "{1408}"},
    [_1412] = {0x14, 0x1c, 0x56, "{1412}"},
    [_1416] = {0x14, 0x1c, 0x58, "{1416}"},
    [_1420] = {0x14, 0x1c, 0x5a, "{1420}"},
    [_1424] = {0x14, 0x1c, 0x5c, "{1424}"},
    [_1428] = {0x14, 0x1c, 0x5f, "{1428}"},

    [_1500] = {0x14, 0x1c, 0x70, "{1500}"},
    [_1504] = {0x14, 0x1c, 0x72, "{1504}"},
    [_1508] = {0x14, 0x1c, 0x74, "{1508}"},
    [_1512] = {0x14, 0x1c, 0x76, "{1512}"},
    [_1516] = {0x14, 0x1c, 0x78, "{1516}"},
    [_1520] = {0x14, 0x1c, 0x7a, "{1520}"},
    [_1524] = {0x14, 0x1c, 0x7c, "{1524}"},
    [_1528] = {0x14, 0x1c, 0x7f, "{1528}"},
};

enum control_code color_codes[COL_MAX] = {
    [COL_WHITE] = Wh,
    [COL_GREEN] = Gr,
    [COL_BLUE] = Bl,
    [COL_CYAN] = Cy,
    [COL_RED] = R,
    [COL_YELLOW] = Y,
    [COL_MAGENTA] = Ma,
    [COL_BLACK] = Bk,

    // No direct mappings
    [COL_USERDEFINED] = Wh,
    [COL_TRANSPARENT] = Wh,
};

const char *disassemble_code(const enum control_code code, unsigned int *length)
{
	char *assembly = control_codes[code].assembly;
	*length = strlen(assembly);
	return assembly;
}

bool is_odd_channel(const unsigned char channel)
{
	// generally (as per the reference) channel 1 and 3 and channel 2 and 4
	// behave in a similar way
	return channel % 2 == 1;
}

unsigned get_first_byte(const unsigned char channel, const enum control_code code)
{
	const struct control_code_info *info = &control_codes[code];

	if (is_odd_channel(channel))
		return info->byte1_odd;
	else
		return info->byte1_even;
}

unsigned get_second_byte(const enum control_code code)
{
	return control_codes[code].byte2;
}

void add_padding(int fd, const char disassemble)
{

	if (disassemble)
	{
		write_wrapped(fd, "_", 1);
	}
	else
	{
		// 0x80 == odd_parity(0x00)
		write_wrapped(fd, "80", 2);
	}
}

void check_padding(const int fd, bool disassemble, unsigned int *bytes_written)
{
	if (*bytes_written % 2 == 1)
	{
		add_padding(fd, disassemble);
		++*bytes_written;
	}
}

void write_character(const int fd, const unsigned char character, const bool disassemble, unsigned int *bytes_written)
{
	if (disassemble)
	{
		write_wrapped(fd, &character, 1);
	}
	else
	{
		if (*bytes_written % 2 == 0)
			write_wrapped(fd, " ", 1);

		fdprintf(fd, "%02x", odd_parity(character));
	}
	++*bytes_written; // increment int pointed to by (unsigned int *) bytes_written
}

/**
 * @param bytes_written Number of control codes written, this doesn't
 *                      correspond to the actual number of bytes written on
 *                      disk. It is the same with the scc and ccd
 *                      (disassembly) format. It's purpose is to know if
 *                      padding should be added
 */
void write_control_code(const int fd, const unsigned char channel, const enum control_code code, const bool disassemble, unsigned int *bytes_written)
{
	check_padding(fd, disassemble, bytes_written);
	if (disassemble)
	{
		unsigned int length;
		const char *assembly_code = disassemble_code(code, &length);
		write_wrapped(fd, assembly_code, length);
	}
	else
	{
		if (*bytes_written % 2 == 0)
			write_wrapped(fd, " ", 1);

		fdprintf(fd, "%02x%02x", odd_parity(get_first_byte(channel, code)), odd_parity(get_second_byte(code)));
	}
	*bytes_written += 2;
}

/**
 *
 * @param row 0-14 (inclusive)
 * @param column 0-31 (inclusive)
 *
 * //TODO: Preamble code need to take into account font as well
 *
 */
enum control_code get_preamble_code(const unsigned char row, const unsigned char column)
{
	return PREAMBLE_CC_START + 1 + (row * 8) + (column / 4);
}

enum control_code get_tab_offset_code(const unsigned char column)
{
	int offset = column % 4;
	return offset == 0 ? 0 : TAB_OFFSET_START + offset;
}

enum control_code get_font_code(enum font_bits font, enum ccx_decoder_608_color_code color)
{
	enum control_code color_code = color_codes[color];

	switch (font)
	{
		case FONT_REGULAR:
			return color_code;
		case FONT_UNDERLINED:
			return color_code + 1;
		case FONT_ITALICS:
			// Color isn't supported for this style
			return I;
		case FONT_UNDERLINED_ITALICS:
			// Color isn't supported for this style
			return IU;
		default:
			fatal(-1, "Unknown font");
	}
}

void add_timestamp(const struct encoder_ctx *context, LLONG time, const bool disassemble)
{
	write_wrapped(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);
	if (!disassemble)
		write_wrapped(context->out->fh, context->encoded_crlf, context->encoded_crlf_length);

	unsigned hour, minute, second, milli;
	millis_to_time(time, &hour, &minute, &second, &milli);

	// SMPTE format
	float frame = milli * 29.97 / 1000;
	fdprintf(context->out->fh, "%02u:%02u:%02u:%02.f\t", hour, minute, second, frame);
}

void clear_screen(const struct encoder_ctx *context, LLONG end_time, const unsigned char channel, const bool disassemble)
{
	add_timestamp(context, end_time, disassemble);
	unsigned int bytes_written = 0;
	write_control_code(context->out->fh, channel, EDM, disassemble, &bytes_written);
}

int write_cc_buffer_as_scenarist(const struct eia608_screen *data, struct encoder_ctx *context, const char disassemble)
{
	unsigned int bytes_written = 0;
	enum font_bits current_font = FONT_REGULAR;
	enum ccx_decoder_608_color_code current_color = COL_WHITE;
	unsigned char current_row = 14;
	unsigned char current_column = 0;

	// 1. Load the caption
	add_timestamp(context, data->start_time, disassemble);
	write_control_code(context->out->fh, data->channel, RCL, disassemble, &bytes_written);
	for (uint8_t row = 0; row < 15; ++row)
	{
		// If there is nothing to display on this row, skip it.
		if (!data->row_used[row])
			continue;

		int first, last;
		find_limit_characters(data->characters[row], &first, &last, CCX_DECODER_608_SCREEN_WIDTH);

		for (unsigned char column = first; column <= last; ++column)
		{
			enum control_code position_code;
			enum control_code tab_offset_code;
			enum control_code font_code = 0;

			bool switch_font = current_font != data->fonts[row][column];
			bool switch_color = current_color != data->colors[row][column];

			if (current_row != row ||
			    current_column != column ||
			    switch_font ||
			    switch_color)
			{
				if (switch_font || switch_color)
				{
					if (data->characters[row][column] == ' ')
					{
						// The MID-ROW code is going to move the cursor to the
						// right so if the next character is a space, skip it
						position_code = get_preamble_code(row, column);
						tab_offset_code = get_tab_offset_code(column++);
					}
					else
					{
						// Column - 1 because the preamble code is going to
						// move the cursor to the right
						position_code = get_preamble_code(row, column - 1);
						tab_offset_code = get_tab_offset_code(column - 1);
					}
					font_code = get_font_code(data->fonts[row][column], data->colors[row][column]);
				}
				else
				{
					position_code = get_preamble_code(row, column);
					tab_offset_code = get_tab_offset_code(column);
				}

				write_control_code(context->out->fh, data->channel, position_code, disassemble, &bytes_written);
				if (tab_offset_code)
					write_control_code(context->out->fh, data->channel, tab_offset_code, disassemble, &bytes_written);
				if (switch_font || switch_color)
					write_control_code(context->out->fh, data->channel, font_code, disassemble, &bytes_written);

				current_row = row;
				current_column = column;
				current_font = data->fonts[row][column];
				current_color = data->colors[row][column];
			}
			write_character(context->out->fh, data->characters[row][column], disassemble, &bytes_written);
			++current_column;
		}
		check_padding(context->out->fh, disassemble, &bytes_written);
	}

	// 2. Show the caption
	write_control_code(context->out->fh, data->channel, EOC, disassemble, &bytes_written);
	write_control_code(context->out->fh, data->channel, ENM, disassemble, &bytes_written);

	// 3. Clear the caption
	clear_screen(context, data->end_time, data->channel, disassemble);

	return 1;
}

int write_cc_buffer_as_ccd(const struct eia608_screen *data, struct encoder_ctx *context)
{
	if (!context->wrote_ccd_channel_header)
	{
		fdprintf(context->out->fh, "CHANNEL %d%s", data->channel, context->encoded_crlf);
		context->wrote_ccd_channel_header = true;
	}
	return write_cc_buffer_as_scenarist(data, context, true);
}

int write_cc_buffer_as_scc(const struct eia608_screen *data, struct encoder_ctx *context)
{
	return write_cc_buffer_as_scenarist(data, context, false);
}
