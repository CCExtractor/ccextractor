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

int MPEG_CLOCK_FREQ = 90000; // This "constant" is part of the standard

int max_dif = 5;
unsigned pts_big_change;

// PTS timing related stuff

double current_fps = (double) 30000.0 / 1001; /* 29.97 */ // TODO: Get from framerates_values[] instead

int frames_since_ref_time = 0;
unsigned total_frames_count;

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

void dinit_timing_ctx(struct ccx_common_timing_ctx **arg)
{
	freep(arg);
}
struct ccx_common_timing_ctx *init_timing_ctx(struct ccx_common_timing_settings_t *cfg)
{
	struct ccx_common_timing_ctx *ctx = malloc(sizeof(struct ccx_common_timing_ctx));
	if(!ctx)
		return NULL;

	ctx->pts_set = 0;
	ctx->current_tref = 0;
	ctx->current_pts = 0;
	ctx->current_picture_coding_type = CCX_FRAME_TYPE_RESET_OR_UNKNOWN;
	ctx->min_pts_adjusted = 0;
	ctx->min_pts = 0x01FFFFFFFFLL; // 33 bit
	ctx->max_pts = 0;
	ctx->sync_pts = 0;
	ctx->minimum_fts = 0;
	ctx->sync_pts2fts_set = 0;
	ctx->sync_pts2fts_fts = 0;
	ctx->sync_pts2fts_pts = 0;

	ctx->fts_now = 0; // Time stamp of current file (w/ fts_offset, w/o fts_global)
	ctx->fts_offset = 0; // Time before first sync_pts
	ctx->fts_fc_offset = 0; // Time before first GOP
	ctx->fts_max = 0; // Remember the maximum fts that we saw in current file
	ctx->fts_global = 0; // Duration of previous files (-ve mode), see c1global

	return ctx;
}

void add_current_pts(struct ccx_common_timing_ctx *ctx, LLONG pts)
{
	set_current_pts(ctx, ctx->current_pts + pts);
}

void set_current_pts(struct ccx_common_timing_ctx *ctx, LLONG pts)
{
	ctx->current_pts = pts;
	if(ctx->pts_set == 0)
		ctx->pts_set = 1;
	dbg_print(CCX_DMT_VIDES, "PTS: %s (%8u)", print_mstime_static(ctx->current_pts/(MPEG_CLOCK_FREQ/1000)),
				(unsigned) (ctx->current_pts));
	dbg_print(CCX_DMT_VIDES, "  FTS: %s \n",print_mstime_static(ctx->fts_now));
}

int set_fts(struct ccx_common_timing_ctx *ctx)
{
	int pts_jump = 0;

	// ES don't have PTS unless GOP timing is used
	if (!ctx->pts_set && ccx_common_timing_settings.is_elementary_stream)
		return CCX_OK;

	// First check for timeline jump (only when min_pts was set (implies sync_pts)).
	int dif = 0;
	if (ctx->pts_set == 2)
	{
		dif=(int) (ctx->current_pts - ctx->sync_pts)/MPEG_CLOCK_FREQ;

		if (ccx_common_timing_settings.disable_sync_check){
			// Disables sync check. Used for several input formats.
			dif = 0;
		}

		if (dif < -0.2 || dif >= max_dif )
		{
			// ATSC specs: More than 3501 ms means missing component
			ccx_common_logging.log_ftn ("\nWarning: Reference clock has changed abruptly (%d seconds filepos=%lld), attempting to synchronize\n", (int) dif, *ccx_common_timing_settings.file_position);
			ccx_common_logging.log_ftn ("Last sync PTS value: %lld\n",ctx->sync_pts);
			ccx_common_logging.log_ftn ("Current PTS value: %lld\n",ctx->current_pts);
			ccx_common_logging.log_ftn("Note: You can disable this behavior by adding -ignoreptsjumps to the parameters.\n");
			pts_jump = 1;
			pts_big_change = 1;

			// Discard the gap if it is not on an I-frame or temporal reference zero.
			if(ctx->current_tref != 0 && ctx->current_picture_coding_type != CCX_FRAME_TYPE_I_FRAME)
			{
				ctx->fts_now = ctx->fts_max;
				ccx_common_logging.log_ftn ("Change did not occur on first frame - probably a broken GOP\n");
				return CCX_OK;
			}
		}
	}

	// If min_pts was set just before a rollover we compensate by "roll-oving" it too
	if (ctx->pts_set == 2 && !ctx->min_pts_adjusted) // min_pts set
	{
		// We want to be aware of the upcoming rollover, not after it happened, so we don't take
		// the 3 most significant bits but the 3 next ones
		uint64_t min_pts_big_bits = (ctx->min_pts >> 30) & 7; 
		uint64_t cur_pts_big_bits = (ctx->current_pts >> 30) & 7;
		if (cur_pts_big_bits == 7 && !min_pts_big_bits)
		{
			// Huge difference possibly means the first min_pts was actually just over the boundary
			// Take the current_pts (smaller, accounting for the rollover) instead
			ctx->min_pts = ctx->current_pts;
			ctx->min_pts_adjusted = 1;
		}
		else if (cur_pts_big_bits>=1 && cur_pts_big_bits<=6) // Far enough from the boundary
		{
			// Prevent the eventual difference with min_pts to make a bad adjustment
			ctx->min_pts_adjusted = 1;
		}
	}
	// Set min_pts, fts_offset
	if (ctx->pts_set != 0)
	{
		ctx->pts_set = 2;

		// Use this part only the first time min_pts is set. Later treat
		// it as a reference clock change
		if (ctx->current_pts < ctx->min_pts && !pts_jump)
		{
			// If this is the first GOP, and seq 0 was not encountered yet
			// we might reset min_pts/fts_offset again

			ctx->min_pts = ctx->current_pts;

			// Avoid next async test
			ctx->sync_pts = (LLONG)(ctx->current_pts
					-ctx->current_tref*1000.0/current_fps
					*(MPEG_CLOCK_FREQ/1000));

			if(ctx->current_tref == 0)
			{   // Earliest time in GOP.
				ctx->fts_offset = 0;
			}
			else if ( total_frames_count-frames_since_ref_time == 0 )
			{   // If this is the first frame (PES) there cannot be an offset.
				// This part is also reached for dvr-ms/NTSC (RAW) as
				// total_frames_count = frames_since_ref_time = 0 when
				// this is called for the first time.
				ctx->fts_offset = 0;
			}
			else
			{   // It needs to be "+1" because the current frame is
				// not yet counted.
				ctx->fts_offset = (LLONG)((total_frames_count
							-frames_since_ref_time+1)
						*1000.0/current_fps);
			}
			ccx_common_logging.debug_ftn(CCX_DMT_TIME, "\nFirst sync time    PTS: %s %+lldms (time before this PTS)\n",
					print_mstime_static(ctx->min_pts/(MPEG_CLOCK_FREQ/1000)),
					ctx->fts_offset );
			ccx_common_logging.debug_ftn(CCX_DMT_TIME, "Total_frames_count %u frames_since_ref_time %u\n",
					total_frames_count, frames_since_ref_time);
		}

		// -nosync disables syncing
		if (pts_jump && !ccx_common_timing_settings.no_sync)
		{
			// The current time in the old time base is calculated using
			// sync_pts (set at the beginning of the last GOP) plus the
			// time of the frames since then.
			ctx->fts_offset = ctx->fts_offset
				+ (ctx->sync_pts - ctx->min_pts)/(MPEG_CLOCK_FREQ/1000)
				+ (LLONG) (frames_since_ref_time*1000/current_fps);
			ctx->fts_max = ctx->fts_offset;

			// Start counting again from here
			ctx->pts_set = 1; // Force min to be set again
			ctx->sync_pts2fts_set = 0; // Make note of the new conversion values

			// Avoid next async test - the gap might have occured on
			// current_tref != 0.
			ctx->sync_pts = (LLONG) (ctx->current_pts
					-ctx->current_tref*1000.0/current_fps
					*(MPEG_CLOCK_FREQ/1000));
			// Set min_pts = sync_pts as this is used for fts_now
			ctx->min_pts = ctx->sync_pts;

			ccx_common_logging.debug_ftn(CCX_DMT_TIME, "\nNew min PTS time: %s %+lldms (time before this PTS)\n",
					print_mstime_static(ctx->min_pts/(MPEG_CLOCK_FREQ/1000)),
					ctx->fts_offset );
		}
	}

	// Set sync_pts, fts_offset
	if(ctx->current_tref == 0)
		ctx->sync_pts = ctx->current_pts;

	// Reset counters
	cb_field1 = 0;
	cb_field2 = 0;
	cb_708 = 0;

	// Avoid wrong "Calc. difference" and "Asynchronous by" numbers
	// for uninitialized min_pts
	if (1) // CFS: Remove or think decent condition
	{
		if ( ctx->pts_set )
		{
			// If pts_set is TRUE we have min_pts
			ctx->fts_now = (LLONG)((ctx->current_pts - ctx->min_pts)/(MPEG_CLOCK_FREQ/1000)
					+ ctx->fts_offset);
			if (!ctx->sync_pts2fts_set)
			{
				ctx->sync_pts2fts_pts = ctx->current_pts;
				ctx->sync_pts2fts_fts = ctx->fts_now;
				ctx->sync_pts2fts_set = 1;
			}
		}
		else
		{
			// No PTS info at all!!
			ccx_common_logging.log_ftn("Set PTS called without any global timestamp set\n");
			return CCX_EINVAL;
		}
	}
	if ( ctx->fts_now > ctx->fts_max )
	{
		ctx->fts_max = ctx->fts_now;
	}
	return CCX_OK;
}


LLONG get_fts(struct ccx_common_timing_ctx *ctx, int current_field)
{
	LLONG fts;

	switch (current_field)
	{
		case 1:
			fts = ctx->fts_now + ctx->fts_global + cb_field1*1001/30;
			break;
		case 2:
			fts = ctx->fts_now + ctx->fts_global + cb_field2*1001/30;
			break;
		case 3:
			fts = ctx->fts_now + ctx->fts_global + cb_708*1001/30;
			break;
		default:
			ccx_common_logging.fatal_ftn(CCX_COMMON_EXIT_BUG_BUG, "get_fts: unhandled branch");
	}
//	ccx_common_logging.debug_ftn(CCX_DMT_TIME, "[FTS] "
//			"fts: %llu, fts_now: %llu, fts_global: %llu, current_field: %llu, cb_708: %llu\n",
//								 fts, fts_now, fts_global, current_field, cb_708);
	return fts;
}

LLONG get_fts_max(struct ccx_common_timing_ctx *ctx)
{
	// This returns the maximum FTS that belonged to a frame.  Caption block
	// counters are not applicable.
	return ctx->fts_max + ctx->fts_global;
}

/**
 * Fill buffer with a time string using specified format
 * @param fmt has to contain 4 format specifiers for h, m, s and ms respectively
 */
size_t print_mstime_buff(LLONG mstime, char* fmt, char* buf){
	unsigned hh, mm, ss, ms;
	int signoffset = (mstime < 0 ? 1 : 0);

	if (mstime < 0) // Avoid loss of data warning with abs()
		mstime = -mstime;

	hh = (unsigned) (mstime / 1000 / 60 / 60);
	mm = (unsigned) (mstime / 1000 / 60 - 60 * hh);
	ss = (unsigned) (mstime / 1000 - 60 * (mm + 60 * hh));
	ms = (unsigned) (mstime - 1000 * (ss + 60 * (mm + 60 * hh)));

	buf[0] = '-';

	return (size_t) sprintf(buf + signoffset, fmt, hh, mm, ss, ms);
}

/* Fill a static buffer with a time string (hh:mm:ss:ms) corresponding
   to the microsecond value in mstime. */
char *print_mstime_static( LLONG mstime )
{
	static char buf[15]; // 14 should be long enough
	print_mstime_buff(mstime, "%02u:%02u:%02u:%03u", buf);
	return buf;
}

/* Helper function for to display debug timing info. */
void print_debug_timing(struct ccx_common_timing_ctx *ctx)
{
	// Avoid wrong "Calc. difference" and "Asynchronous by" numbers
	// for uninitialized min_pts
	LLONG tempmin_pts = (ctx->min_pts==0x01FFFFFFFFLL ? ctx->sync_pts : ctx->min_pts);

	ccx_common_logging.log_ftn("Sync time stamps:  PTS: %s                ",
			print_mstime_static((ctx->sync_pts)/(MPEG_CLOCK_FREQ/1000)) );
	ccx_common_logging.log_ftn("GOP: %s      \n", print_mstime_static(gop_time.ms));

	// Length first GOP to last GOP
	LLONG goplenms = (LLONG) (gop_time.ms - first_gop_time.ms);
	// Length at last sync point
	LLONG ptslenms = (unsigned)((ctx->sync_pts-tempmin_pts)/(MPEG_CLOCK_FREQ/1000)
			+ ctx->fts_offset);

	ccx_common_logging.log_ftn("Last               FTS: %s",
			print_mstime_static(get_fts_max(ctx)));
	ccx_common_logging.log_ftn("      GOP start FTS: %s\n",
			print_mstime_static(fts_at_gop_start));

	// Times are based on last GOP and/or sync time
	ccx_common_logging.log_ftn("Max FTS diff. to   PTS:       %6lldms              GOP:       %6lldms\n\n",
			get_fts_max(ctx)+(LLONG)(1000.0/current_fps)-ptslenms,
			get_fts_max(ctx)+(LLONG)(1000.0/current_fps)-goplenms);
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
