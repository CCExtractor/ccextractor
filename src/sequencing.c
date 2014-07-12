#include "ccextractor.h"
#include "708.h"

// Defined by the maximum number of B-Frames per anchor frame.
//#define MAXBFRAMES 50 - from ccextractor.h
//  They can be (temporally) before or after the anchor. Reserve
// enough space.
//#define SORTBUF (2*MAXBFRAMES+1) - from ccextractor.h
// B-Frames can be (temporally) before or after the anchor
int cc_data_count[SORTBUF];
// Store fts;
static LLONG cc_fts[SORTBUF];
// Store HD CC packets
unsigned char cc_data_pkts[SORTBUF][10*31*3+1]; // *10, because MP4 seems to have different limits

// Set to true if data is buffered
int has_ccdata_buffered = 0;
// The sequence number of the current anchor frame.  All currently read
// B-Frames belong to this I- or P-frame.
static int anchor_seq_number = -1;

// Remember the current field for 608 decoding
int current_field=1; // 1 = field 1, 2 = field 2, 3 = 708

static int in_xds_mode ; // Stolen from 608.c We need this for debug

void init_hdcc (void)
{
    for (int j=0; j<SORTBUF; j++)
    {
        cc_data_count[j] = 0;
        cc_fts[j] = 0;
    }
    memset(cc_data_pkts, 0, SORTBUF*(31*3+1));
    has_ccdata_buffered = 0;
}

// Buffer caption blocks for later sorting/flushing.
void store_hdcc(unsigned char *cc_data, int cc_count, int sequence_number, LLONG current_fts_now,struct cc_subtitle *sub)
{
    // Uninitialized?
    if (anchor_seq_number < 0)
    {
        anchor_hdcc( sequence_number);
    }

    int seq_index = sequence_number - anchor_seq_number + MAXBFRAMES;

    if (seq_index < 0 || seq_index > 2*MAXBFRAMES)
    {
        // Maybe missing an anchor frame - try to recover
        dbg_print(CCX_DMT_VERBOSE, "Too many B-frames, or missing anchor frame. Trying to recover ..\n");

        process_hdcc(sub);
        anchor_hdcc( sequence_number);
        seq_index = sequence_number - anchor_seq_number + MAXBFRAMES;
    }

    has_ccdata_buffered = 1;

	// In GOP mode the fts is set only once for the whole GOP. Recreate
    // the right time according to the sequence number.
    if (ccx_options.use_gop_as_pts==1)
    {
        current_fts_now += (LLONG) (sequence_number*1000.0/current_fps);
    }

	if (cc_count)
	{		
		if (cc_data)
		{		
			// Changed by CFS to concat, i.e. don't assume there's no data already for this seq_index.
			// Needed at least for MP4 samples. // TODO: make sure we don't overflow
			cc_fts[seq_index] = current_fts_now; // CFS: Maybe do even if there's no data?
			if (stream_mode!=CCX_SM_MP4) // CFS: Very ugly hack, but looks like overwriting is needed for at least some ES
				cc_data_count[seq_index]  = 0;
			memcpy(cc_data_pkts[seq_index]+cc_data_count[seq_index]*3, cc_data, cc_count*3+1);
		}
		cc_data_count[seq_index] += cc_count;
	}
    // DEBUG STUFF
/*
    printf("\nCC blocks, channel 0:\n");
    for ( int i=0; i < cc_count*3; i+=3)
    {
        printf("%s", debug_608toASC( cc_data+i, 0) );
    }
    printf("\n");
*/
}

// Set a new anchor frame that new B-frames refer to.
void anchor_hdcc(int seq)
{
    // Re-init the index
    anchor_seq_number = seq;
}

// Sort/flash caption block buffer
void process_hdcc (struct cc_subtitle *sub)
{
    // Remember the current value
    LLONG store_fts_now = fts_now;

    dbg_print(CCX_DMT_VERBOSE, "Flush HD caption blocks\n");

    int reset_cb = -1;

    for (int seq=0; seq<SORTBUF; seq++)
    {
        // We rely on this.
        if (ccx_bufferdatatype == CCX_H264)
            reset_cb = 1;

	// If fts_now is unchanged we rely on cc block counting,
	// otherwise reset counters as they get changed by do_cb()
	// below. This only happens when current_pts does not get
	// updated, like it used do happen for elementary streams.
	// Since use_gop_as_pts this is not needed anymore, but left
	// here for posterity.
	if (reset_cb < 0 && cc_fts[seq] && seq<SORTBUF-1 && cc_fts[seq+1])
	{
	    if (cc_fts[seq] != cc_fts[seq+1])
		reset_cb = 1;
	    else
		reset_cb = 0;
	}
	if (reset_cb == 1)
	{
	    cb_field1 = 0;
	    cb_field2 = 0;
	    cb_708 = 0;
	}

        // Skip sequence numbers without data
        if (cc_data_count[seq] == 0)
            continue;

        if (cc_data_pkts[seq][cc_data_count[seq]*3]!=0xFF)
        {
            // This is not optional. Something is wrong.
            dbg_print(CCX_DMT_VERBOSE, "Missing 0xFF marker at end\n");
            // A "continue;" here would ignore this caption, but we
            // process it.
        }

        // Re-create original time
        fts_now = cc_fts[seq];

        for (int j=0; j<(cc_data_count[seq])*3; j=j+3)
        {
            unsigned char cc_valid = (*(cc_data_pkts[seq]+j) & 4) >>2;
            unsigned char cc_type = (*(cc_data_pkts[seq]+j)) & 3;
            
            if (cc_valid && (cc_type==0 || cc_type==1))
            {
                // For EIA-608 data we verify parity.
                if (!cc608_parity_table[cc_data_pkts[seq][j+2]])
                {
                    // If the second byte doesn't pass parity, ignore pair
                    continue;
                }
                if (!cc608_parity_table[cc_data_pkts[seq][j+1]])
                {
                    // The first byte doesn't pass parity, we replace it with a solid blank
                    // and process the pair. 
                    cc_data_pkts[seq][j+1]=0x7F;
                }
            }             
            do_cb(cc_data_pkts[seq]+j, sub);

        } // for loop over packets
    }

    // Restore the value
    fts_now = store_fts_now;

    // Now that we are done, clean up.
    init_hdcc();
}


int do_cb (unsigned char *cc_block, struct cc_subtitle *sub)
{
    unsigned char cc_valid = (*cc_block & 4) >>2;
    unsigned char cc_type = *cc_block & 3;

    int timeok = 1;

    if ( ccx_options.fix_padding
         && cc_valid==0 && cc_type <= 1 // Only fix NTSC packets
         && cc_block[1]==0 && cc_block[2]==0 )
    {
        /* Padding */
        cc_valid=1;
        cc_block[1]=0x80;
        cc_block[2]=0x80;
    }

	if ( ccx_options.write_format!=CCX_OF_RAW && // In raw we cannot skip padding because timing depends on it
		 ccx_options.write_format!=CCX_OF_DVDRAW &&
		(cc_block[0]==0xFA || cc_block[0]==0xFC || cc_block[0]==0xFD )
		&& (cc_block[1]&0x7F)==0 && (cc_block[2]&0x7F)==0) // CFS: Skip non-data, makes debugging harder.
		return 1; 

    // Print raw data with FTS.
    dbg_print(CCX_DMT_CBRAW, "%s   %d   %02X:%02X:%02X", print_mstime(fts_now + fts_global),in_xds_mode,
               cc_block[0], cc_block[1], cc_block[2]);

    /* In theory the writercwtdata() function could return early and not
     * go through the 608/708 cases below.  We do that to get accurate
     * counts for cb_field1, cb_field2 and cb_708.
     * Note that printdata() and do_708() must not be called for
     * the CCX_OF_RCWT case. */

    if (cc_valid || cc_type==3)
    {
        cc_stats[cc_type]++;

        switch (cc_type)
        {
        case 0:
            dbg_print(CCX_DMT_CBRAW, "    %s   ..   ..\n",  debug_608toASC( cc_block, 0));

            current_field=1;
            saw_caption_block = 1;

            if (ccx_options.extraction_start.set && 
				get_fts() < ccx_options.extraction_start.time_in_ms)
                timeok = 0;
            if (ccx_options.extraction_end.set && 
				get_fts() > ccx_options.extraction_end.time_in_ms)
            {
                timeok = 0;
                processed_enough=1;
            }
            if (timeok)
            {
                if(ccx_options.write_format!=CCX_OF_RCWT)
                    printdata (cc_block+1,2,0,0, sub);
                else
                    writercwtdata(cc_block);
            }
            cb_field1++;
            break;
        case 1:
            dbg_print(CCX_DMT_CBRAW, "    ..   %s   ..\n",  debug_608toASC( cc_block, 1));

            current_field=2;
            saw_caption_block = 1;

            if (ccx_options.extraction_start.set && 
				get_fts() < ccx_options.extraction_start.time_in_ms)
                timeok = 0;
            if (ccx_options.extraction_end.set && 
				get_fts() > ccx_options.extraction_end.time_in_ms)
            {
                timeok = 0;
                processed_enough=1;
            }
            if (timeok)
            {
                if(ccx_options.write_format!=CCX_OF_RCWT)
                    printdata (0,0,cc_block+1,2, sub);
                else
                    writercwtdata(cc_block);
            }
            cb_field2++;
            break;
        case 2: //EIA-708
            // DTVCC packet data
            // Fall through
        case 3: //EIA-708
            dbg_print(CCX_DMT_CBRAW, "    ..   ..   DD\n");

            // DTVCC packet start
            current_field=3;

            if (ccx_options.extraction_start.set && 
				get_fts() < ccx_options.extraction_start.time_in_ms)
                timeok = 0;
            if (ccx_options.extraction_end.set && 
				get_fts() > ccx_options.extraction_end.time_in_ms)
            {
                timeok = 0;
                processed_enough=1;
            }
            char temp[4];
            temp[0]=cc_valid;
            temp[1]=cc_type;
            temp[2]=cc_block[1];
            temp[3]=cc_block[2];
            if (timeok)
            {
                if(ccx_options.write_format!=CCX_OF_RCWT)
                   do_708 ((const unsigned char *) temp, 4);
                else
                    writercwtdata(cc_block);
            }
            cb_708++;
            // Check for bytes read
            // printf ("Warning: Losing EIA-708 data!\n");
            break;
        default:
            fatal(EXIT_BUG_BUG, "Cannot be reached!");
        } // switch (cc_type)
    } // cc_valid
    else
    {
        dbg_print(CCX_DMT_CBRAW, "    ..   ..   ..\n");
        dbg_print(CCX_DMT_VERBOSE, "Found !(cc_valid || cc_type==3) - ignore this block\n");
    }

    return 1;
}
