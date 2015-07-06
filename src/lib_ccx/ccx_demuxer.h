#ifndef CCX_DEMUXER_H
#define CCX_DEMUXER_H
#include "ccx_common_constants.h"
#include "ccx_common_option.h"
#include "ts_functions.h"
/* Report information */
#define SUB_STREAMS_CNT 10
struct ccx_demux_report
{
        unsigned program_cnt;
        unsigned dvb_sub_pid[SUB_STREAMS_CNT];
        unsigned tlt_sub_pid[SUB_STREAMS_CNT];
        unsigned mp4_cc_track_cnt;
};

struct cap_info
{
	int pid;
	int program_number;
	enum ccx_stream_type stream; 
	enum ccx_code_type codec;
	long capbufsize;
	unsigned char *capbuf;
	long capbuflen; // Bytes read in capbuf
	int saw_pesstart;
};

struct program_info
{
	int pid;
	int program_number;
};
#define MAX_PID 65536
#define TS_PMT_MAP_SIZE 128
#define MAX_PROGRAM 5
struct ccx_demuxer
{
	int m2ts;
	enum ccx_stream_mode_enum stream_mode;
	enum ccx_stream_mode_enum auto_stream;

	// Small buffer to help us with the initial sync
	unsigned char startbytes[STARTBYTESLENGTH];
	unsigned int startbytes_pos;
	int startbytes_avail;

	// User Specified Param
	int ts_autoprogram;
	int ts_allprogram;
	int flag_ts_forced_pn;
	int ts_datastreamtype;


	struct program_info pinfo[MAX_PROGRAM];
	int nb_program;
	/* subtitle codec type */
	enum ccx_code_type codec;
	enum ccx_code_type nocodec;
	struct cap_info cinfo[MAX_PID];
	int nb_cap;

	/* File handles */
	FILE *fh_out_elementarystream;
	int infd; // descriptor number to input.
	LLONG past; /* Position in file, if in sync same as ftell()  */

	// TODO relates to fts_global
	uint32_t global_timestamp;
	uint32_t min_global_timestamp;
	int global_timestamp_inited;

	int PIDs_seen[MAX_PID];

	// PMTs table
	struct PAT_entry pmt_array[TS_PMT_MAP_SIZE];
	uint16_t pmt_array_length;

	struct PMT_entry *PIDs_programs[MAX_PID];
	struct ccx_demux_report freport;

	/* Hauppauge support */
	unsigned hauppauge_warning_shown; // Did we detect a possible Hauppauge capture and told the user already?

	void *parent;
	void (*print_cfg)(struct ccx_demuxer *ctx);
	void (*print_report)(struct ccx_demuxer *ctx);
	void (*reset)(struct ccx_demuxer *ctx);
	void (*close)(struct ccx_demuxer *ctx);
	int (*open)(struct ccx_demuxer *ctx, const char *file_name);
	int (*is_open)(struct ccx_demuxer *ctx);
	int (*get_stream_mode)(struct ccx_demuxer *ctx);
	LLONG (*get_filesize) (struct ccx_demuxer *ctx);
	int (*write_es)(struct ccx_demuxer *ctx, unsigned char* buf, size_t len);
};

struct demuxer_data
{
	int program_number;
	enum ccx_bufferdata_type bufferdatatype;
	unsigned char *buffer;
	int nb_buffer;
	int len;
	int windex;
	//int index;
};
struct ccx_demuxer *init_demuxer(void *parent, struct demuxer_cfg *cfg);
void ccx_demuxer_delete(struct ccx_demuxer **ctx);
struct demuxer_data* alloc_demuxer_data(void);
#endif
