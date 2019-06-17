#ifndef AVC_FUNCTION_H
#define AVC_FUNCTION_H


struct avc_ctx
{
	unsigned char cc_count;
	// buffer to hold cc data
	unsigned char *cc_data;
	long cc_databufsize;
	int cc_buffer_saved; // Was the CC buffer saved after it was last updated?

	int got_seq_para;
	unsigned nal_ref_idc;
	LLONG seq_parameter_set_id;
	int log2_max_frame_num;
	int pic_order_cnt_type;
	int log2_max_pic_order_cnt_lsb;
	int frame_mbs_only_flag;

	// Use and throw stats for debug, remove this ugliness soon
	long num_nal_unit_type_7;
	long num_vcl_hrd;
	long num_nal_hrd;
	long num_jump_in_frames;
	long num_unexpected_sei_length;

	int ccblocks_in_avc_total;
	int ccblocks_in_avc_lost;

	LLONG frame_num;
	LLONG lastframe_num;
	int currref;
	int maxidx;
	int lastmaxidx;

	// Used to find tref zero in PTS mode
	int minidx;
	int lastminidx;

	// Used to remember the max temporal reference number (poc mode)
	int maxtref;
	int last_gop_maxtref;

	// Used for PTS ordering of CC blocks
	LLONG currefpts;
	LLONG last_pic_order_cnt_lsb;
	LLONG last_slice_pts;
};

struct avc_ctx *init_avc(void);
void dinit_avc(struct avc_ctx **ctx);
void do_NAL (struct encoder_ctx *enc_ctx, struct lib_cc_decode *ctx, unsigned char *NAL_start, LLONG NAL_length, struct cc_subtitle *sub);
size_t process_avc(struct encoder_ctx *enc_ctx, struct lib_cc_decode *ctx, unsigned char *avcbuf, size_t avcbuflen, struct cc_subtitle *sub);
#endif
