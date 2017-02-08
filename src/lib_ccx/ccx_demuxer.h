#ifndef CCX_DEMUXER_H
#define CCX_DEMUXER_H
#include "ccx_common_constants.h"
#include "ccx_common_option.h"
#include "ts_functions.h"
#include "list.h"
#include "activity.h"
#include "utility.h"

/* Report information */
#define SUB_STREAMS_CNT 10
#define MAX_PID 65536
#define MAX_NUM_OF_STREAMIDS 51
#define MAX_PSI_PID 8191
#define TS_PMT_MAP_SIZE 128
#define MAX_PROGRAM 128
#define MAX_PROGRAM_NAME_LEN 128

enum STREAM_TYPE
{
	PRIVATE_STREAM_1 = 0,
	AUDIO,
	VIDEO,
	COUNT
};
struct ccx_demux_report
{
        unsigned program_cnt;
        unsigned dvb_sub_pid[SUB_STREAMS_CNT];
        unsigned tlt_sub_pid[SUB_STREAMS_CNT];
        unsigned mp4_cc_track_cnt;
};

struct program_info
{
	int pid;
	int program_number;
	int initialized_ocr; // Avoid initializing the OCR more than once
	uint8_t analysed_PMT_once:1;
	uint8_t version;
	uint8_t saved_section[1021];
	int32_t crc;
	uint8_t valid_crc:1;
	char name[MAX_PROGRAM_NAME_LEN];
	/**
	 * -1 pid represent that pcr_pid is not available
	 */
	int16_t pcr_pid;
	uint64_t got_important_streams_min_pts[COUNT];
	int has_all_min_pts;
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
	int prev_counter;
	void *codec_private_data;
	int ignore;

	/**
	  List joining all stream in TS
	*/
	struct list_head all_stream;
	/**
	  List joining all sibling Stream in Program
	*/
	struct list_head sib_head;
	struct list_head sib_stream;
	/**
	  List joining all sibling Stream in Program
	*/
	struct list_head pg_stream;

};
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
	int flag_ts_forced_cappid;
	int ts_datastreamtype;


	struct program_info pinfo[MAX_PROGRAM];
	int nb_program;
	/* subtitle codec type */
	enum ccx_code_type codec;
	enum ccx_code_type nocodec;
	struct cap_info cinfo_tree;

	/* File handles */
	FILE *fh_out_elementarystream;
	int infd;   // descriptor number to input.
	LLONG past; /* Position in file, if in sync same as ftell()  */

	// TODO relates to fts_global
	int64_t global_timestamp;
	int64_t min_global_timestamp;
	int64_t offset_global_timestamp;
	int64_t last_global_timestamp;
	int global_timestamp_inited;

	struct PSI_buffer *PID_buffers[MAX_PSI_PID];
	int PIDs_seen[MAX_PID];

	/*51 possible stream ids in total, 0xbd is private stream, 0xbe is padding stream, 
	0xbf private stream 2, 0xc0 - 0xdf audio, 0xe0 - 0xef video 
	(stream ids range from 0xbd to 0xef so 0xef - 0xbd + 1 = 51)*/
	//uint8_t found_stream_ids[MAX_NUM_OF_STREAMIDS]; 

	uint8_t stream_id_of_each_pid[MAX_PSI_PID + 1];
	uint64_t min_pts[MAX_PSI_PID + 1];
	int have_PIDs[MAX_PSI_PID + 1];
	int num_of_PIDs;

	struct PMT_entry *PIDs_programs[MAX_PID];
	struct ccx_demux_report freport;

	/* Hauppauge support */
	unsigned hauppauge_warning_shown; // Did we detect a possible Hauppauge capture and told the user already?

	int multi_stream_per_prog;

	unsigned char *last_pat_payload;
	unsigned last_pat_length;

	unsigned char *filebuffer;
	LLONG filebuffer_start;      // Position of buffer start relative to file
	unsigned int filebuffer_pos; // Position of pointer relative to buffer start
	unsigned int bytesinbuffer;  // Number of bytes we actually have on buffer

	int warning_program_not_found_shown;

	// Remember if the last header was valid. Used to suppress too much output
	// and the expected unrecognized first header for TiVo files.
	int strangeheader;
#ifdef ENABLE_FFMPEG
	void *ffmpeg_ctx;
#endif

	void *parent;

	//Will contain actual Demuxer Context
	void *private_data;
	void (*print_cfg)(struct ccx_demuxer *ctx);
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
	int stream_pid;
	enum ccx_code_type codec;
	enum ccx_bufferdata_type bufferdatatype;
	unsigned char *buffer;
	size_t len;
	unsigned int rollover_bits; // The PTS rolls over every 26 hours and that can happen in the middle of a stream.
	LLONG pts;
	struct ccx_rational tb;
	struct demuxer_data *next_stream;
	struct demuxer_data *next_program;
};

struct cap_info *get_sib_stream_by_type(struct cap_info* program, enum ccx_code_type type);
struct ccx_demuxer *init_demuxer(void *parent, struct demuxer_cfg *cfg);
void ccx_demuxer_delete(struct ccx_demuxer **ctx);
struct demuxer_data* alloc_demuxer_data(void);
void delete_demuxer_data(struct demuxer_data *data);
int update_capinfo(struct ccx_demuxer *ctx, int pid, enum ccx_stream_type stream, enum ccx_code_type codec, int pn, void *private_data);
struct cap_info * get_cinfo(struct ccx_demuxer *ctx, int pid);
int need_cap_info(struct ccx_demuxer *ctx, int program_number);
int need_cap_info_for_pid(struct ccx_demuxer *ctx, int pid);
struct demuxer_data *get_best_data(struct demuxer_data *data);
struct demuxer_data *get_data_stream(struct demuxer_data *data, int pid);
int get_best_stream(struct ccx_demuxer *ctx);
void ignore_other_stream(struct ccx_demuxer *ctx, int pid);
void dinit_cap (struct ccx_demuxer *ctx);
int get_programme_number(struct ccx_demuxer *ctx, int pid);
struct cap_info* get_best_sib_stream(struct cap_info* program);
void ignore_other_sib_stream(struct cap_info* head, int pid);
#endif
