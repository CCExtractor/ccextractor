#include "ccx_decoders_708_output.h"
#include "ccx_common_common.h"

void printTVtoSRT(dtvcc_service_decoder *decoder, int which)
{
	if (decoder->output_format == CCX_OF_NULL)
		return;

	/* dtvcc_tv_screen *tv = (which==1)? &decoder->tv1:&decoder->tv2; */
	unsigned h1, m1, s1, ms1;
	unsigned h2, m2, s2, ms2;
	LLONG ms_start = decoder->current_visible_start_ms;
	LLONG ms_end = get_visible_end() + decoder->subs_delay;
	int empty = 1;
	ms_start += decoder->subs_delay;
	if (ms_start < 0) // Drop screens that because of subs_delay start too early
		return;

	for (int i = 0; i < DTVCC_SCREENGRID_ROWS; i++)
	{
		for (int j = 0; j < DTVCC_SCREENGRID_COLUMNS; j++)
		{
			if (decoder->tv->chars[i][j] != ' ')
			{
				empty = 0;
				break;
			}
		}
		if (!empty)
			break;
	}
	if (empty)
		return; // Nothing to write

	if (decoder->fh == -1) // File not yet open, do it now
	{

		ccx_common_logging.log_ftn("[CEA-708] Creating %s\n", decoder->filename);
		// originally wbout1.filename, but since line below uses decoder->filename, assume it's good to use?
		decoder->fh = open(decoder->filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
		if (decoder->fh == -1)
		{
			ccx_common_logging.fatal_ftn(
					CCX_COMMON_EXIT_FILE_CREATION_FAILED, "[CEA-708] Failed to open a file\n");
		}
	}
	mstotime(ms_start, &h1, &m1, &s1, &ms1);
	mstotime(ms_end - 1, &h2, &m2, &s2, &ms2); // -1 To prevent overlapping with next line.

	decoder->srt_counter++;
	char timeline[128];

	sprintf(timeline, "%u\r\n", decoder->srt_counter);
	write(decoder->fh, timeline, strlen(timeline));
	sprintf(timeline, "%02u:%02u:%02u,%03u --> %02u:%02u:%02u,%03u\r\n",
			h1, m1, s1, ms1,
			h2, m2, s2, ms2
	);
	write(decoder->fh, timeline, strlen(timeline));

	for (int i = 0; i < DTVCC_SCREENGRID_ROWS; i++)
	{
		empty = 1;
		for (int j = 0; j < DTVCC_SCREENGRID_COLUMNS; j++)
		{
			if (decoder->tv->chars[i][j] != ' ')
			{
				empty = 0;
				break;
			}
		}
		if (!empty)
		{
			int f, l; // First, last used char
			for (f = 0; f < DTVCC_SCREENGRID_COLUMNS; f++)
				if (decoder->tv->chars[i][f] != ' ')
					break;
			for (l = DTVCC_SCREENGRID_COLUMNS - 1; l > 0; l--)
				if (decoder->tv->chars[i][l] != ' ')
					break;
			for (int j = f; j <= l; j++)
				write(decoder->fh, &decoder->tv->chars[i][j], sizeof(char));
			write(decoder->fh, "\r\n", 2 * sizeof(char));
		}
	}
	write (decoder->fh, "\r\n", 2 * sizeof(char));
}

void printTVtoConsole (dtvcc_service_decoder *decoder, int which)
{
	/* dtvcc_tv_screen *tv = (which==1)? &decoder->tv1:&decoder->tv2; */
	char tbuf1[DTVCC_MAX_ROWS],
			tbuf2[DTVCC_MAX_ROWS];

	print_mstime2buf(decoder->current_visible_start_ms, tbuf1);
	print_mstime2buf(get_visible_end(), tbuf2);

	ccx_common_logging.debug_ftn(CCX_DMT_GENERIC_NOTICES, "\r%s --> %s\n", tbuf1, tbuf2);
	for (int i = 0; i < DTVCC_SCREENGRID_ROWS; i++)
	{
		int empty = 1;
		for (int j = 0; j < DTVCC_SCREENGRID_COLUMNS; j++)
		{
			if (decoder->tv->chars[i][j] != ' ')
				empty = 0;
		}
		if (!empty)
		{
			int f, l; // First,last used char
			for (f = 0; f < DTVCC_SCREENGRID_COLUMNS; f++)
			{
				if (decoder->tv->chars[i][f] != ' ')
					break;
			}
			for (l = DTVCC_SCREENGRID_COLUMNS - 1; l > 0 ; l--)
			{
				if (decoder->tv->chars[i][l] != ' ')
					break;
			}
			for (int j = f; j <= l; j++)
			{
				ccx_common_logging.debug_ftn(CCX_DMT_GENERIC_NOTICES, "%c", decoder->tv->chars[i][j]);
			}
			ccx_common_logging.debug_ftn(CCX_DMT_GENERIC_NOTICES, "\n");
		}
	}
}
