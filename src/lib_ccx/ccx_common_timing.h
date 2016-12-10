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
	// if 1, timeline jumps will be ignored. This is important in several input formats that are assumed to have correct timing, no matter what
	int disable_sync_check;
	// if 1, there will be no sync at all. Mostly useful for debugging
	int no_sync;
	// needs to be set, as it's used in set_fts
	int is_elementary_stream;
	// the position of the file
	LLONG *file_position;
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
	// 0 = No, 1 = received, 2 = min_pts set
	int pts_set;
	LLONG current_pts;
	enum ccx_frame_type current_picture_coding_type;
	// store temporal reference of current frame
	int current_tref;
	LLONG min_pts;
	LLONG max_pts;
	LLONG sync_pts;
	// no screen should start before this FTS
	LLONG minimum_fts;
	// time stamp of current file (w/ fts_offset, w/o fts_global)
	LLONG fts_now;
	// time before first sync_pts
	LLONG fts_offset;
	// time before first GOP
	LLONG fts_fc_offset;
	// remember the maximum fts that we saw in current file
	LLONG fts_max;
	// duration of previous files (-ve mode)
	LLONG fts_global;
	// 0 = No, 1 = Yes
	int sync_pts2fts_set;
	LLONG sync_pts2fts_fts; 
	LLONG sync_pts2fts_pts;
};
// count 608 (per field) and 708 blocks since last set_fts() call
extern int cb_field1, cb_field2, cb_708;

// this is part of the standard
extern int MPEG_CLOCK_FREQ;

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

void set_current_pts 	  (struct ccx_common_timing_ctx *ctx, LLONG pts);
void add_current_pts  	  (struct ccx_common_timing_ctx *ctx, LLONG pts);
int set_fts	     	  (struct ccx_common_timing_ctx *ctx);
LLONG get_fts	    	  (struct ccx_common_timing_ctx *ctx, int current_field);
LLONG get_fts_max    	  (struct ccx_common_timing_ctx *ctx);
char *print_mstime        (LLONG mstime);
char *print_mstime2buf	  (LLONG mstime, char *buf);
size_t mstime_sprintf  	  (LLONG mstime, char *fmt, char *buf);
void print_debug_timing   (struct ccx_common_timing_ctx *ctx);
int gop_accepted       	  (struct gop_time_code* g);
void calculate_ms_gop_time(struct gop_time_code *g);

#endif
