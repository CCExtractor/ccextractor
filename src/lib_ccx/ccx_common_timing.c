#include "lib_ccx.h"
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

double current_fps = (double)30000.0 / 1001; /* 29.97 */ // TODO: Get from framerates_values[] instead

int frames_since_ref_time = 0;
unsigned total_frames_count;

struct gop_time_code gop_time, first_gop_time, printed_gop;
LLONG fts_at_gop_start = 0;
int gop_rollover = 0;

struct ccx_common_timing_settings_t ccx_common_timing_settings;

void ccxr_add_current_pts(struct ccx_common_timing_ctx *ctx, LLONG pts);
void ccxr_set_current_pts(struct ccx_common_timing_ctx *ctx, LLONG pts);
int ccxr_set_fts(struct ccx_common_timing_ctx *ctx);
LLONG ccxr_get_fts(struct ccx_common_timing_ctx *ctx, int current_field);
LLONG ccxr_get_fts_max(struct ccx_common_timing_ctx *ctx);
LLONG ccxr_get_visible_start(struct ccx_common_timing_ctx *ctx, int current_field);
LLONG ccxr_get_visible_end(struct ccx_common_timing_ctx *ctx, int current_field);
char *ccxr_print_mstime_static(LLONG mstime, char *buf);
void ccxr_print_debug_timing(struct ccx_common_timing_ctx *ctx);
void ccxr_calculate_ms_gop_time(struct gop_time_code *g);
int ccxr_gop_accepted(struct gop_time_code *g);

void ccx_common_timing_init(LLONG *file_position, int no_sync)
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
	if (!ctx)
		return NULL;

	ctx->pts_set = 0;
	ctx->current_tref = 0;
	ctx->current_pts = 0;
	ctx->current_picture_coding_type = CCX_FRAME_TYPE_RESET_OR_UNKNOWN;
	ctx->min_pts_adjusted = 0;
	ctx->seen_known_frame_type = 0;
	ctx->pending_min_pts = 0x01FFFFFFFFLL;
	ctx->unknown_frame_count = 0;
	ctx->first_large_gap_pts = 0x01FFFFFFFFLL;
	ctx->seen_large_gap = 0;
	ctx->min_pts = 0x01FFFFFFFFLL; // 33 bit
	ctx->max_pts = 0;
	ctx->sync_pts = 0;
	ctx->minimum_fts = 0;
	ctx->sync_pts2fts_set = 0;
	ctx->sync_pts2fts_fts = 0;
	ctx->sync_pts2fts_pts = 0;

	ctx->fts_now = 0;	// Time stamp of current file (w/ fts_offset, w/o fts_global)
	ctx->fts_offset = 0;	// Time before first sync_pts
	ctx->fts_fc_offset = 0; // Time before first GOP
	ctx->fts_max = 0;	// Remember the maximum fts that we saw in current file
	ctx->fts_global = 0;	// Duration of previous files (-ve mode), see c1global
	ctx->pts_reset = 0;	// 0 = No, 1 = Yes. PTS resets when current_pts is lower than prev

	return ctx;
}

void add_current_pts(struct ccx_common_timing_ctx *ctx, LLONG pts)
{
	return ccxr_add_current_pts(ctx, pts);
}

void set_current_pts(struct ccx_common_timing_ctx *ctx, LLONG pts)
{
	return ccxr_set_current_pts(ctx, pts);
}

int set_fts(struct ccx_common_timing_ctx *ctx)
{
	return ccxr_set_fts(ctx);
}

LLONG get_fts(struct ccx_common_timing_ctx *ctx, int current_field)
{
	return ccxr_get_fts(ctx, current_field);
}

LLONG get_fts_max(struct ccx_common_timing_ctx *ctx)
{
	return ccxr_get_fts_max(ctx);
}

/**
 * SCC Time formatting
 * Note: buf must have at least 32 bytes available from the write position
 */
size_t print_scc_time(struct ccx_boundary_time time, char *buf)
{
	char *fmt = "%02u:%02u:%02u;%02u";
	double frame;
	// Format produces "HH:MM:SS;FF" = 11 chars + null, use 32 for safety
	const size_t max_time_len = 32;

	frame = ((double)(time.time_in_ms - 1000 * (time.ss + 60 * (time.mm + 60 * time.hh))) * 29.97 / 1000);

	return (size_t)snprintf(buf + time.set, max_time_len, fmt, time.hh, time.mm, time.ss, (unsigned)frame);
}

struct ccx_boundary_time get_time(LLONG time)
{
	if (time < 0) // Avoid loss of data warning with abs()
		time = -time;

	struct ccx_boundary_time result;
	result.hh = (unsigned)(time / 1000 / 60 / 60);
	result.mm = (unsigned)(time / 1000 / 60 - 60 * result.hh);
	result.ss = (unsigned)(time / 1000 - 60 * (result.mm + 60 * result.hh));
	result.time_in_ms = time;
	result.set = (time < 0 ? 1 : 0);

	return result;
}

/**
 * Fill buffer with a time string using specified format
 * @param fmt has to contain 4 format specifiers for h, m, s and ms respectively
 * Note: buf must have at least 32 bytes available from the write position
 */
size_t print_mstime_buff(LLONG mstime, char *fmt, char *buf)
{
	unsigned hh, mm, ss, ms;
	int signoffset = (mstime < 0 ? 1 : 0);
	// Typical format produces "HH:MM:SS:MSS" = 12 chars + null, use 32 for safety
	const size_t max_time_len = 32;

	if (mstime < 0) // Avoid loss of data warning with abs()
		mstime = -mstime;

	hh = (unsigned)(mstime / 1000 / 60 / 60);
	mm = (unsigned)(mstime / 1000 / 60 - 60 * hh);
	ss = (unsigned)(mstime / 1000 - 60 * (mm + 60 * hh));
	ms = (unsigned)(mstime - 1000 * (ss + 60 * (mm + 60 * hh)));

	buf[0] = '-';

	return (size_t)snprintf(buf + signoffset, max_time_len, fmt, hh, mm, ss, ms);
}

/* Fill a static buffer with a time string (hh:mm:ss:ms) corresponding
   to the microsecond value in mstime. */
char *print_mstime_static(LLONG mstime)
{
	static char buf[15]; // 14 should be long enough
	return ccxr_print_mstime_static(mstime, buf);
}

/* Helper function for to display debug timing info. */
void print_debug_timing(struct ccx_common_timing_ctx *ctx)
{
	return ccxr_print_debug_timing(ctx);
}

void calculate_ms_gop_time(struct gop_time_code *g)
{
	return ccxr_calculate_ms_gop_time(g);
}

int gop_accepted(struct gop_time_code *g)
{
	return ccxr_gop_accepted(g);
}
