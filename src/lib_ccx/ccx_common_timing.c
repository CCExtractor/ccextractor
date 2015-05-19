#include "ccx_common_timing.h"
#include "ccx_common_constants.h"
#include "ccx_common_structs.h"
#include "ccx_common_common.h"

/* Provide the current time since the file (or the first file) started
 * in ms using PTS time information.
 * Requires: frames_since_ref_time, current_tref
 */

// Count 608 (per field) and 708 blocks since last set_fts() call
int cb_field1, cb_field2, cb_708;

int pts_set; //0 = No, 1 = received, 2 = min_pts set
int MPEG_CLOCK_FREQ = 90000; // This "constant" is part of the standard

LLONG min_pts, max_pts, sync_pts;
LLONG current_pts = 0;

int max_dif = 5;
unsigned pts_big_change;

// PTS timing related stuff
LLONG fts_now; // Time stamp of current file (w/ fts_offset, w/o fts_global)
LLONG fts_offset; // Time before first sync_pts
LLONG fts_fc_offset; // Time before first GOP
LLONG fts_max; // Remember the maximum fts that we saw in current file
LLONG fts_global = 0; // Duration of previous files (-ve mode), see c1global

enum ccx_frame_type current_picture_coding_type = CCX_FRAME_TYPE_RESET_OR_UNKNOWN;
int current_tref = 0; // Store temporal reference of current frame
double current_fps = (double) 30000.0 / 1001; /* 29.97 */ // TODO: Get from framerates_values[] instead

int frames_since_ref_time = 0;
unsigned total_frames_count;

// Remember the current field for 608 decoding
int current_field = 1; // 1 = field 1, 2 = field 2, 3 = 708

struct gop_time_code gop_time, first_gop_time, printed_gop;
LLONG fts_at_gop_start = 0;
int gop_rollover = 0;

struct ccx_common_timing_settings_t ccx_common_timing_settings;

void ccx_common_timing_init(LLONG *file_position,int no_sync)
{
	ccx_common_timing_settings.disable_sync_check = 0;
	ccx_common_timing_settings.is_elementary_stream = 0;
	ccx_common_timing_settings.file_position = file_position;
	ccx_common_timing_settings.no_sync = no_sync;
}

void set_fts(void)
{
	int pts_jump = 0;

	// ES don't have PTS unless GOP timing is used
	if (!pts_set && ccx_common_timing_settings.is_elementary_stream)
		return;

	// First check for timeline jump (only when min_pts was set (implies sync_pts)).
	int dif = 0;
	if (pts_set == 2)
	{
		dif=(int) (current_pts-sync_pts)/MPEG_CLOCK_FREQ;

		if (ccx_common_timing_settings.disable_sync_check){
			// Disables sync check. Used for several input formats.
			dif = 0;
		}

		if (dif < -0.2 || dif >= max_dif )
		{
			// ATSC specs: More than 3501 ms means missing component
			ccx_common_logging.log_ftn ("\nWarning: Reference clock has changed abruptly (%d seconds filepos=%lld), attempting to synchronize\n", (int) dif, *ccx_common_timing_settings.file_position);
			ccx_common_logging.log_ftn ("Last sync PTS value: %lld\n",sync_pts);
			ccx_common_logging.log_ftn ("Current PTS value: %lld\n",current_pts);
			pts_jump = 1;
			pts_big_change = 1;

			// Discard the gap if it is not on an I-frame or temporal reference zero.
			if(current_tref != 0 && current_picture_coding_type != CCX_FRAME_TYPE_I_FRAME)
			{
				fts_now = fts_max;
				ccx_common_logging.log_ftn ("Change did not occur on first frame - probably a broken GOP\n");
				return;
			}
		}
	}

	// Set min_pts, fts_offset
	if (pts_set != 0)
	{
		pts_set = 2;

		// Use this part only the first time min_pts is set. Later treat
		// it as a reference clock change
		if (current_pts < min_pts && !pts_jump)
		{
			// If this is the first GOP, and seq 0 was not encountered yet
			// we might reset min_pts/fts_offset again

			min_pts = current_pts;

			// Avoid next async test
			sync_pts = (LLONG)(current_pts
					-current_tref*1000.0/current_fps
					*(MPEG_CLOCK_FREQ/1000));

			if(current_tref == 0)
			{   // Earliest time in GOP.
				fts_offset = 0;
			}
			else if ( total_frames_count-frames_since_ref_time == 0 )
			{   // If this is the first frame (PES) there cannot be an offset.
				// This part is also reached for dvr-ms/NTSC (RAW) as
				// total_frames_count = frames_since_ref_time = 0 when
				// this is called for the first time.
				fts_offset = 0;
			}
			else
			{   // It needs to be "+1" because the current frame is
				// not yet counted.
				fts_offset = (LLONG)((total_frames_count
							-frames_since_ref_time+1)
						*1000.0/current_fps);
			}
			ccx_common_logging.debug_ftn(CCX_DMT_TIME, "\nFirst sync time    PTS: %s %+lldms (time before this PTS)\n",
					print_mstime(min_pts/(MPEG_CLOCK_FREQ/1000)),
					fts_offset );
			ccx_common_logging.debug_ftn(CCX_DMT_TIME, "Total_frames_count %u frames_since_ref_time %u\n",
					total_frames_count, frames_since_ref_time);
		}

		// -nosync disables syncing
		if (pts_jump && !ccx_common_timing_settings.no_sync)
		{
			// The current time in the old time base is calculated using
			// sync_pts (set at the beginning of the last GOP) plus the
			// time of the frames since then.
			fts_offset = fts_offset
				+ (sync_pts-min_pts)/(MPEG_CLOCK_FREQ/1000)
				+ (LLONG) (frames_since_ref_time*1000/current_fps);
			fts_max = fts_offset;

			// Start counting again from here
			pts_set = 1; // Force min to be set again

			// Avoid next async test - the gap might have occured on
			// current_tref != 0.
			sync_pts = (LLONG) (current_pts
					-current_tref*1000.0/current_fps
					*(MPEG_CLOCK_FREQ/1000));
			// Set min_pts = sync_pts as this is used for fts_now
			min_pts = sync_pts;

			ccx_common_logging.debug_ftn(CCX_DMT_TIME, "\nNew min PTS time: %s %+lldms (time before this PTS)\n",
					print_mstime(min_pts/(MPEG_CLOCK_FREQ/1000)),
					fts_offset );
		}
	}

	// Set sync_pts, fts_offset
	if(current_tref == 0)
		sync_pts = current_pts;

	// Reset counters
	cb_field1 = 0;
	cb_field2 = 0;
	cb_708 = 0;

	// Avoid wrong "Calc. difference" and "Asynchronous by" numbers
	// for uninitialized min_pts
	if (1) // CFS: Remove or think decent condition
	{
		if ( pts_set )
		{
			// If pts_set is TRUE we have min_pts
			fts_now = (LLONG)((current_pts-min_pts)/(MPEG_CLOCK_FREQ/1000)
					+ fts_offset);
		}
		else
		{
			// No PTS info at all!!
			ccx_common_logging.fatal_ftn(CCX_COMMON_EXIT_BUG_BUG,
					"No PTS info. Please write bug report.");
		}
	}
	if ( fts_now > fts_max )
	{
		fts_max = fts_now;
	}
}


LLONG get_fts(void)
{
	LLONG fts;

	switch (current_field)
	{
		case 1:
			fts = fts_now + fts_global + cb_field1*1001/30;
			break;
		case 2:
			fts = fts_now + fts_global + cb_field2*1001/30;
			break;
		case 3:
			fts = fts_now + fts_global + cb_708*1001/30;
			break;
		default:
			ccx_common_logging.fatal_ftn(CCX_COMMON_EXIT_BUG_BUG, "Cannot be reached!");
	}

	return fts;
}

LLONG get_fts_max(void)
{
	// This returns the maximum FTS that belonged to a frame.  Caption block
	// counters are not applicable.
	return fts_max + fts_global;
}

/* Fill a static buffer with a time string (hh:mm:ss:ms) corresponding
   to the microsecond value in mstime. */
char *print_mstime2buf( LLONG mstime , char *buf )
{
	unsigned hh,mm,ss,ms;
	int signoffset = (mstime < 0 ? 1 : 0);

	if (mstime<0) // Avoid loss of data warning with abs()
		mstime=-mstime;
	hh = (unsigned) (mstime/1000/60/60);
	mm = (unsigned) (mstime/1000/60 - 60*hh);
	ss = (unsigned) (mstime/1000 - 60*(mm + 60*hh));
	ms = (int) (mstime - 1000*(ss + 60*(mm + 60*hh)));

	buf[0]='-';
	sprintf (buf+signoffset, "%02u:%02u:%02u:%03u",hh,mm,ss,ms);

	return buf;
}

/* Fill a static buffer with a time string (hh:mm:ss:ms) corresponding
   to the microsecond value in mstime. */
char *print_mstime( LLONG mstime )
{
	static char buf[15]; // 14 should be long enough
	return print_mstime2buf (mstime, buf);
}

/* Helper function for to display debug timing info. */
void print_debug_timing( void )
{
	// Avoid wrong "Calc. difference" and "Asynchronous by" numbers
	// for uninitialized min_pts
	LLONG tempmin_pts = (min_pts==0x01FFFFFFFFLL ? sync_pts : min_pts);

	ccx_common_logging.log_ftn("Sync time stamps:  PTS: %s                ",
			print_mstime((sync_pts)/(MPEG_CLOCK_FREQ/1000)) );
	ccx_common_logging.log_ftn("GOP: %s      \n", print_mstime(gop_time.ms));

	// Length first GOP to last GOP
	LLONG goplenms = (LLONG) (gop_time.ms - first_gop_time.ms);
	// Length at last sync point
	LLONG ptslenms = (unsigned)((sync_pts-tempmin_pts)/(MPEG_CLOCK_FREQ/1000)
			+ fts_offset);

	ccx_common_logging.log_ftn("Last               FTS: %s",
			print_mstime(get_fts_max()));
	ccx_common_logging.log_ftn("      GOP start FTS: %s\n",
			print_mstime(fts_at_gop_start));

	// Times are based on last GOP and/or sync time
	ccx_common_logging.log_ftn("Max FTS diff. to   PTS:       %6lldms              GOP:       %6lldms\n\n",
			get_fts_max()+(LLONG)(1000.0/current_fps)-ptslenms,
			get_fts_max()+(LLONG)(1000.0/current_fps)-goplenms);
}

void calculate_ms_gop_time (struct gop_time_code *g)
{
	int seconds=(g->time_code_hours*3600)+(g->time_code_minutes*60)+g->time_code_seconds;
	g->ms = (LLONG)( 1000*(seconds + g->time_code_pictures/current_fps) );
	if (gop_rollover)
		g->ms += 24*60*60*1000;
}

int gop_accepted(struct gop_time_code* g )
{
	if (! ((g->time_code_hours <= 23)
				&& (g->time_code_minutes <= 59)
				&& (g->time_code_seconds <= 59)
				&& (g->time_code_pictures <= 59)))
		return 0;

	if (gop_time.time_code_hours==23 && gop_time.time_code_minutes==59 &&
			g->time_code_hours==0 && g->time_code_minutes==0)
	{
		gop_rollover = 1;
		return 1;
	}
	if (gop_time.inited)
	{
		if (gop_time.ms > g->ms)
		{
			// We are going back in time but it's not a complete day rollover
			return 0;
		}
	}
	return 1;
}
