#include "lib_ccx.h"
#include "ccx_common_option.h"

// Defined by the maximum number of B-Frames per anchor frame.
//#define MAXBFRAMES 50 - from lib_ccx.h
//  They can be (temporally) before or after the anchor. Reserve
// enough space.
//#define SORTBUF (2*MAXBFRAMES+1) - from lib_ccx.h
// B-Frames can be (temporally) before or after the anchor

void init_hdcc (struct lib_cc_decode *ctx)
{
	for (int j=0; j<SORTBUF; j++)
	{
		ctx->cc_data_count[j] = 0;
		ctx->cc_fts[j] = 0;
	}
	memset(ctx->cc_data_pkts, 0, SORTBUF*(31*3+1));
	ctx->has_ccdata_buffered = 0;
}

// Buffer caption blocks for later sorting/flushing.
void store_hdcc(struct lib_cc_decode *ctx, unsigned char *cc_data, int cc_count, int sequence_number, LLONG current_fts_now, struct cc_subtitle *sub)
{
	//stream_mode = ctx->demux_ctx->get_stream_mode(ctx->demux_ctx);
	// Uninitialized?
	if (ctx->anchor_seq_number < 0)
	{
		anchor_hdcc( ctx, sequence_number);
	}

	int seq_index = sequence_number - ctx->anchor_seq_number + MAXBFRAMES;

	if (seq_index < 0 || seq_index > 2*MAXBFRAMES)
	{
		// Maybe missing an anchor frame - try to recover
		dbg_print(CCX_DMT_VERBOSE, "Too many B-frames, or missing anchor frame. Trying to recover ..\n");

		process_hdcc(ctx, sub);
		anchor_hdcc( ctx, sequence_number);
		seq_index = sequence_number - ctx->anchor_seq_number + MAXBFRAMES;
	}

	ctx->has_ccdata_buffered = 1;

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
			ctx->cc_fts[seq_index] = current_fts_now; // CFS: Maybe do even if there's no data?
			//if (stream_mode!=CCX_SM_MP4) // CFS: Very ugly hack, but looks like overwriting is needed for at least some ES
				ctx->cc_data_count[seq_index]  = 0;
			memcpy(ctx->cc_data_pkts[seq_index] + ctx->cc_data_count[seq_index] * 3, cc_data, cc_count * 3 + 1);
		}
		ctx->cc_data_count[seq_index] += cc_count;
	}
	// DEBUG STUFF
	/*
	   printf("\nCC blocks, channel 0:\n");
	   for ( int i=0; i < cc_count*3; i+=3)
	   {
	   printf("%s", debug_608_to_ASC( cc_data+i, 0) );
	   }
	   printf("\n");
	 */
}

// Set a new anchor frame that new B-frames refer to.
void anchor_hdcc(struct lib_cc_decode *ctx, int seq)
{
	// Re-init the index
	ctx->anchor_seq_number = seq;
}

// Sort/flash caption block buffer
void process_hdcc (struct lib_cc_decode *ctx, struct cc_subtitle *sub)
{
	// Remember the current value
	LLONG store_fts_now = ctx->timing->fts_now;
	int reset_cb = -1;

	dbg_print(CCX_DMT_VERBOSE, "Flush HD caption blocks\n");

	for (int seq=0; seq<SORTBUF; seq++)
	{

		// We rely on this.
		if (ctx->in_bufferdatatype == CCX_H264)
			reset_cb = 1;

		// If fts_now is unchanged we rely on cc block counting,
		// otherwise reset counters as they get changed by do_cb()
		// below. This only happens when current_pts does not get
		// updated, like it used do happen for elementary streams.
		// Since use_gop_as_pts this is not needed anymore, but left
		// here for posterity.
		if (reset_cb < 0 && ctx->cc_fts[seq] && seq<SORTBUF-1 && ctx->cc_fts[seq+1])
		{
			if (ctx->cc_fts[seq] != ctx->cc_fts[seq+1])
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
		if (ctx->cc_data_count[seq] == 0)
			continue;

		if (ctx->cc_data_pkts[seq][ctx->cc_data_count[seq]*3]!=0xFF)
		{
			// This is not optional. Something is wrong.
			dbg_print(CCX_DMT_VERBOSE, "Missing 0xFF marker at end\n");
			// A "continue;" here would ignore this caption, but we
			// process it.
		}

		// Re-create original time
		ctx->timing->fts_now = ctx->cc_fts[seq];
		process_cc_data( ctx, ctx->cc_data_pkts[seq], ctx->cc_data_count[seq], sub);

	}

	// Restore the value
	ctx->timing->fts_now = store_fts_now;

	// Now that we are done, clean up.
	init_hdcc(ctx);
}
