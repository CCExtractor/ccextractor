#ifndef __Timing_H__
#define __Timing_H__

#include "ccx_common_platform.h"
#include "ccx_common_constants.h"
struct gop_time_code
{
	int drop_frame_flag;
	int time_code_hours;
	int time_code_minutes;
	int marker_bit;
	int time_code_seconds;
	int time_code_pictures;
	int inited;
	LLONG ms;
};

struct ccx_common_timing_settings_t
{
	int disable_sync_check;   // If 1, timeline jumps will be ignored. This is important in several input formats that are assumed to have correct timing, no matter what.
	int no_sync;              // If 1, there will be no sync at all. Mostly useful for debugging.
	int is_elementary_stream; // Needs to be set, as it's used in set_fts.
	LLONG *file_position;     // The position of the file
};
extern struct ccx_common_timing_settings_t ccx_common_timing_settings;

struct ccx_boundary_time
{
	int hh,mm,ss;
	LLONG time_in_ms;
	int set;
};

struct ccx_common_timing_ctx
{
	int pts_set;          // 0 = No, 1 = received, 2 = min_pts set
	int min_pts_adjusted; // 0 = No, 1=Yes (don't adjust again)
	LLONG current_pts;
	enum ccx_frame_type current_picture_coding_type;
	int current_tref;     // Store temporal reference of current frame
	LLONG min_pts;
	LLONG max_pts;
	LLONG sync_pts;
	LLONG minimum_fts;    // No screen should start before this FTS
	LLONG fts_now;        // Time stamp of current file (w/ fts_offset, w/o fts_global)
	LLONG fts_offset;     // Time before first sync_pts
	LLONG fts_fc_offset;  // Time before first GOP
	LLONG fts_max;        // Remember the maximum fts that we saw in current file
	LLONG fts_global;     // Duration of previous files (-ve mode)
	int sync_pts2fts_set; // 0 = No, 1 = Yes
	LLONG sync_pts2fts_fts;
	LLONG sync_pts2fts_pts;
};
// Count 608 (per field) and 708 blocks since last set_fts() call
extern int cb_field1, cb_field2, cb_708;

extern int MPEG_CLOCK_FREQ; // This is part of the standard

extern int max_dif;
extern unsigned pts_big_change;


extern enum ccx_frame_type current_picture_coding_type;
extern double current_fps;
extern int frames_since_ref_time;
extern unsigned total_frames_count;

extern struct gop_time_code gop_time, first_gop_time, printed_gop;
extern LLONG fts_at_gop_start;
extern int gop_rollover;

void ccx_common_timing_init(LLONG *file_position, int no_sync);

void dinit_timing_ctx(struct ccx_common_timing_ctx **arg);
struct ccx_common_timing_ctx *init_timing_ctx(struct ccx_common_timing_settings_t *cfg);

void set_current_pts(struct ccx_common_timing_ctx *ctx, LLONG pts);
void add_current_pts(struct ccx_common_timing_ctx *ctx, LLONG pts);
int set_fts(struct ccx_common_timing_ctx *ctx);
LLONG get_fts(struct ccx_common_timing_ctx *ctx, int current_field);
LLONG get_fts_max(struct ccx_common_timing_ctx *ctx);
char *print_mstime_static(LLONG mstime);
size_t print_mstime_buff(LLONG mstime, char *fmt, char *buf);
void print_debug_timing(struct ccx_common_timing_ctx *ctx);
int gop_accepted(struct gop_time_code* g);
void calculate_ms_gop_time(struct gop_time_code *g);

#endif
