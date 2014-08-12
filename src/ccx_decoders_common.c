/* Functions used by both the 608 and 708 decoders. An effort should be
made to reuse, not duplicate, as many functions as possible */

#include "ccextractor.h"

uint64_t utc_refvalue = UINT64_MAX;  /* _UI64_MAX means don't use UNIX, 0 = use current system time as reference, +1 use a specific reference */

// Preencoded strings
unsigned char encoded_crlf[16];
unsigned int encoded_crlf_length;
unsigned char encoded_br[16];
unsigned int encoded_br_length;

/* This function returns a FTS that is guaranteed to be at least 1 ms later than the end of the previous screen. It shouldn't be needed
   obviously but it guarantees there's no timing overlap */
LLONG get_visible_start (void)
{
	LLONG fts = get_fts();
	if (fts <= minimum_fts)
		fts = minimum_fts+1;
	dbg_print(CCX_DMT_DECODER_608, "Visible Start time=%s\n", print_mstime(fts));
	return fts;
}

/* This function returns the current FTS and saves it so it can be used by get_visible_start */
LLONG get_visible_end (void)
{
	LLONG fts = get_fts();
	if (fts>minimum_fts)
		minimum_fts=fts;
	dbg_print(CCX_DMT_DECODER_608, "Visible End time=%s\n", print_mstime(fts));
	return fts;
}
