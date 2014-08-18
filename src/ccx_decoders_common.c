/* Functions used by both the 608 and 708 decoders. An effort should be
made to reuse, not duplicate, as many functions as possible */

#include "ccx_decoders_common.h"
#include "ccx_common_structs.h"
#include "ccx_common_char_encoding.h"
#include "ccx_common_constants.h"
#include "ccx_common_timing.h"

uint64_t utc_refvalue = UINT64_MAX;  /* _UI64_MAX means don't use UNIX, 0 = use current system time as reference, +1 use a specific reference */

// Preencoded strings
unsigned char encoded_crlf[16];
unsigned int encoded_crlf_length;
unsigned char encoded_br[16];
unsigned int encoded_br_length;

void ccx_decoders_common_settings_init(LLONG subs_delay, enum ccx_output_format output_format){
	ccx_decoders_common_settings.subs_delay = subs_delay;
	ccx_decoders_common_settings.output_format = output_format;
}

LLONG minimum_fts = 0; // No screen should start before this FTS

/* This function returns a FTS that is guaranteed to be at least 1 ms later than the end of the previous screen. It shouldn't be needed
   obviously but it guarantees there's no timing overlap */
LLONG get_visible_start (void)
{
	LLONG fts = get_fts();
	if (fts <= minimum_fts)
		fts = minimum_fts+1;
	ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "Visible Start time=%s\n", print_mstime(fts));
	return fts;
}

/* This function returns the current FTS and saves it so it can be used by get_visible_start */
LLONG get_visible_end (void)
{
	LLONG fts = get_fts();
	if (fts>minimum_fts)
		minimum_fts=fts;
	ccx_common_logging.debug_ftn(CCX_DMT_DECODER_608, "Visible End time=%s\n", print_mstime(fts));
	return fts;
}

void find_limit_characters(unsigned char *line, int *first_non_blank, int *last_non_blank)
{
	*last_non_blank = -1;
	*first_non_blank = -1;
	for (int i = 0; i<CCX_DECODER_608_SCREEN_WIDTH; i++)
	{
		unsigned char c = line[i];
		if (c != ' ' && c != 0x89)
		{
			if (*first_non_blank == -1)
				*first_non_blank = i;
			*last_non_blank = i;
		}
	}
}

unsigned get_decoder_line_basic(unsigned char *buffer, int line_num, struct eia608_screen *data, int trim_subs, enum ccx_encoding_type encoding)
{
	unsigned char *line = data->characters[line_num];
	int last_non_blank = -1;
	int first_non_blank = -1;
	unsigned char *orig = buffer; // Keep for debugging
	find_limit_characters(line, &first_non_blank, &last_non_blank);
	if (!trim_subs)
		first_non_blank = 0;

	if (first_non_blank == -1)
	{
		*buffer = 0;
		return 0;
	}

	int bytes = 0;
	for (int i = first_non_blank; i <= last_non_blank; i++)
	{
		char c = line[i];
		switch (encoding)
		{
		case CCX_ENC_UTF_8:
			bytes = get_char_in_utf_8(buffer, c);
			break;
		case CCX_ENC_LATIN_1:
			get_char_in_latin_1(buffer, c);
			bytes = 1;
			break;
		case CCX_ENC_UNICODE:
			get_char_in_unicode(buffer, c);
			bytes = 2;
			break;
		}
		buffer += bytes;
	}
	*buffer = 0;
	return (unsigned)(buffer - orig); // Return length
}