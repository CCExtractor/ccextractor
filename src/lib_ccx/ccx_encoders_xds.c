#include "ccx_common_platform.h"
#include "ccx_common_common.h"
#include "ccx_encoders_common.h"
#include "ccx_decoders_common.h"

static const char *XDSclasses_short[] =
    {
	"CUR",
	"FUT",
	"CHN",
	"MIS",
	"PUB",
	"RES",
	"PRV",
	"END"};

void xds_write_transcript_line_prefix(struct encoder_ctx *context, struct ccx_s_write *wb, LLONG start_time, LLONG end_time, int cur_xds_packet_class)
{
	unsigned h1, m1, s1, ms1;
	unsigned h2, m2, s2, ms2;
	if (!wb || wb->fh == -1)
		return;

	if (start_time == -1)
	{
		// Means we entered XDS mode without making a note of the XDS start time. This is a bug.
		ccx_common_logging.fatal_ftn(CCX_COMMON_EXIT_BUG_BUG, "Bug in timedtranscript (XDS). Please report.");
	}

	if (context->transcript_settings->showStartTime)
	{
		char buffer[80];
		if (context->transcript_settings->relativeTimestamp)
		{
			if (utc_refvalue == UINT64_MAX)
			{
				millis_to_time(start_time, &h1, &m1, &s1, &ms1);
				fdprintf(wb->fh, "%02u:%02u:%02u%c%03u|", h1, m1, s1, context->millis_separator, ms1);
			}
			else
			{
				fdprintf(wb->fh, "%lld%c%03d|", start_time / 1000,
					 context->millis_separator, start_time % 1000);
			}
		}
		else
		{
			millis_to_time(start_time, &h1, &m1, &s1, &ms1);
			time_t start_time_int = start_time / 1000;
			int start_time_dec = start_time % 1000;
			struct tm *start_time_struct = gmtime(&start_time_int);
			strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", start_time_struct);
			fdprintf(wb->fh, "%s%c%03d|", buffer, context->millis_separator, start_time_dec);
		}
	}

	if (context->transcript_settings->showEndTime)
	{
		char buffer[80];
		if (context->transcript_settings->relativeTimestamp)
		{
			if (utc_refvalue == UINT64_MAX)
			{
				millis_to_time(end_time, &h2, &m2, &s2, &ms2);
				fdprintf(wb->fh, "%02u:%02u:%02u%c%03u|", h2, m2, s2, context->millis_separator, ms2);
			}
			else
			{
				fdprintf(wb->fh, "%lld%s%03d|", end_time / 1000, context->millis_separator, end_time % 1000);
			}
		}
		else
		{
			millis_to_time(end_time, &h2, &m2, &s2, &ms2);
			time_t end_time_int = end_time / 1000;
			int end_time_dec = end_time % 1000;
			struct tm *end_time_struct = gmtime(&end_time_int);
			strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", end_time_struct);
			fdprintf(wb->fh, "%s%c%03d|", buffer, context->millis_separator, end_time_dec);
		}
	}

	if (context->transcript_settings->showMode)
	{
		const char *mode = "XDS";
		fdprintf(wb->fh, "%s|", mode);
	}

	if (context->transcript_settings->showCC)
	{
		fdprintf(wb->fh, "%s|", XDSclasses_short[cur_xds_packet_class]);
	}
}
