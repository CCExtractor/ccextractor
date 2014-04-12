#include "ccextractor.h"

/* Provide the current time since the file (or the first file) started
 * in ms using PTS time information.
 * Requires: frames_since_ref_time, current_tref
 */

// int ignore_fts=1; // Don't do any kind of timing stuff. Assume it's being done externally (as happens in MP4)

void set_fts(void)
{
    int pts_jump = 0;

    // ES don't have PTS unless GOP timing is used
    if (!pts_set && stream_mode==CCX_SM_ELEMENTARY_OR_NOT_FOUND)
        return;

    // First check for timeline jump (only when min_pts was
    // set (implies sync_pts).
    int dif = 0;
    if (pts_set == 2)
    {
        dif=(int) (current_pts-sync_pts)/MPEG_CLOCK_FREQ;

        // Used to distinguish gaps with missing caption information from
        // jumps in the timeline.  (Currently only used for dvr-ms/NTSC
        // recordings)
        if ( CaptionGap )
            dif = 0;

        // Disable sync check for raw formats - they have the right timeline.
        // Also true for bin formats, but -nosync might have created a
        // broken timeline for debug purposes.
		// Disable too in MP4, specs doesn't say that there can't be a jump
        switch (stream_mode)
        {
            case CCX_SM_MCPOODLESRAW:
            case CCX_SM_RCWT:
			case CCX_SM_MP4:
			case CCX_SM_HEX_DUMP:
                dif = 0;
                break;
            default:
                break;
        }

        if (dif < -0.2 || dif >=5 )
        {
            // ATSC specs: More than 3501 ms means missing component
            mprint ("\nWarning: Reference clock has changed abruptly (%d seconds filepos=%lld), attempting to synchronize\n", (int) dif, past);
            mprint ("Last sync PTS value: %lld\n",sync_pts);
            mprint ("Current PTS value: %lld\n",current_pts);
            pts_jump = 1;
            pts_big_change=1;

            // Discard the gap if it is not on an I-frame or temporal reference
            // zero.
            if(current_tref != 0 && current_picture_coding_type != CCX_FRAME_TYPE_I_FRAME)
            {
                fts_now = fts_max;
                mprint ("Change did not occur on first frame - probably a broken GOP\n");
                return;
            }
        }
    }

    // Set min_pts, fts_offset
    if (pts_set!=0)
    {
        pts_set=2;

        // Use this part only the first time min_pts is set. Later treat
        // it as a reference clock change
        if (current_pts<min_pts && !pts_jump)
        {
            // If this is the first GOP, and seq 0 was not encountered yet
            // we might reset min_pts/fts_offset again

            min_pts=current_pts;

            // Avoid next async test
            sync_pts = LLONG(current_pts
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
                fts_offset = LLONG((total_frames_count
                                    -frames_since_ref_time+1)
                                   *1000.0/current_fps);
            }
            dbg_print(CCX_DMT_TIME, "\nFirst sync time    PTS: %s %+lldms (time before this PTS)\n",
                   print_mstime(min_pts/(MPEG_CLOCK_FREQ/1000)),
                   fts_offset );
            dbg_print(CCX_DMT_TIME, "Total_frames_count %u frames_since_ref_time %u\n",
                   total_frames_count, frames_since_ref_time);
        }

        // -nosync diasbles syncing
        if (pts_jump && !ccx_options.nosync)
        {
            // The current time in the old time base is calculated using
            // sync_pts (set at the beginning of the last GOP) plus the
            // time of the frames since then.
            fts_offset = fts_offset
                + (sync_pts-min_pts)/(MPEG_CLOCK_FREQ/1000)
                + LLONG(frames_since_ref_time*1000/current_fps);
            fts_max = fts_offset;

            // Start counting again from here
            pts_set=1; // Force min to be set again

            // Avoid next async test - the gap might have occured on
            // current_tref != 0.
            sync_pts = LLONG(current_pts
                             -current_tref*1000.0/current_fps
                             *(MPEG_CLOCK_FREQ/1000));
            // Set min_pts = sync_pts as this is used for fts_now
            min_pts = sync_pts;

            dbg_print(CCX_DMT_TIME, "\nNew min PTS time: %s %+lldms (time before this PTS)\n",
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
			fts_now = LLONG((current_pts-min_pts)/(MPEG_CLOCK_FREQ/1000)
							+ fts_offset);
		}
		else
		{
			// No PTS info at all!!
			fatal(EXIT_BUG_BUG,
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
            fatal(EXIT_BUG_BUG, "Cannot be reached!");
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

    mprint("Sync time stamps:  PTS: %s                ",
           print_mstime((sync_pts)/(MPEG_CLOCK_FREQ/1000)) );
    mprint("GOP: %s      \n", print_mstime(gop_time.ms));

    // Length first GOP to last GOP
    LLONG goplenms = LLONG(gop_time.ms - first_gop_time.ms);
    // Length at last sync point
    LLONG ptslenms = unsigned((sync_pts-tempmin_pts)/(MPEG_CLOCK_FREQ/1000)
                              + fts_offset);

    mprint("Last               FTS: %s",
           print_mstime(get_fts_max()));
    mprint("      GOP start FTS: %s\n",
           print_mstime(fts_at_gop_start));

    // Times are based on last GOP and/or sync time
    mprint("Max FTS diff. to   PTS:       %6lldms              GOP:       %6lldms\n\n",
           get_fts_max()+LLONG(1000.0/current_fps)-ptslenms,
           get_fts_max()+LLONG(1000.0/current_fps)-goplenms);
}

void calculate_ms_gop_time (struct gop_time_code *g)
{
    int seconds=(g->time_code_hours*3600)+(g->time_code_minutes*60)+g->time_code_seconds;
    g->ms = LLONG( 1000*(seconds + g->time_code_pictures/current_fps) );
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
