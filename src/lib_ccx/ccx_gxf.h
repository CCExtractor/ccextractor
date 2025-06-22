#ifndef CCX_GXF
#define CCX_GXF

#include "ccx_demuxer.h"
#include "lib_ccx.h"

#define STR_LEN 256u
typedef enum
{
	PKT_MAP = 0xbc,
	PKT_MEDIA = 0xbf,
	PKT_EOS = 0xfb,
	PKT_FLT = 0xfc,
	PKT_UMF = 0xfd,
} GXFPktType;

typedef enum
{
	MAT_NAME = 0x40,
	MAT_FIRST_FIELD = 0x41,
	MAT_LAST_FIELD = 0x42,
	MAT_MARK_IN = 0x43,
	MAT_MARK_OUT = 0x44,
	MAT_SIZE = 0x45,
} GXFMatTag;

typedef enum
{
	/* Media file name */
	TRACK_NAME = 0x4c,

	/*Auxiliary Information. The exact meaning depends on the track type. */
	TRACK_AUX = 0x4d,

	/* Media file system version */
	TRACK_VER = 0x4e,

	/* MPEG auxiliary information */
	TRACK_MPG_AUX = 0x4f,

	/**
	 * Frame rate
	 * 1 = 60 frames/sec
	 * 2 = 59.94 frames/sec
	 * 3 = 50 frames/sec
	 * 4 = 30 frames/sec
	 * 5 = 29.97 frames/sec
	 * 6 = 25 frames/sec
	 * 7 = 24 frames/sec
	 * 8 = 23.98 frames/sec
	 * -1 = Not applicable for this track type
	 * -2 = Not available
	 */
	TRACK_FPS = 0x50,

	/**
	 * Lines per frame
	 * 1 = 525
	 * 2 = 625
	 * 4 = 1080
	 * 5 = Reserved
	 * 6 = 720
	 * -1 = Not applicable
	 * -2 = Not available
	 */
	TRACK_LINES = 0x51,

	/**
	 * Fields per frame
	 * 1 = Progressive
	 * 2 = Interlaced
	 * -1 = Not applicable
	 * -2 = Not available
	 */
	TRACK_FPF = 0x52,

} GXFTrackTag;

typedef enum
{
	/**
	 * A video track encoded using JPEG (ITU-R T.81 or ISO/IEC
	 *	10918-1) for 525 line material.
	 */
	TRACK_TYPE_MOTION_JPEG_525 = 3,

	/* A video track encoded using JPEG (ITU-R T.81 or ISO/IEC 10918-1) for 625 line material */
	TRACK_TYPE_MOTION_JPEG_625 = 4,

	/* SMPTE 12M time code tracks */
	TRACK_TYPE_TIME_CODE_525 = 7,

	/* SMPTE 12M time code tracks */
	TRACK_TYPE_TIME_CODE_625 = 8,

	/* A mono 24-bit PCM audio track */
	TRACK_TYPE_AUDIO_PCM_24 = 9,

	/* A mono 16-bit PCM audio track. */
	TRACK_TYPE_AUDIO_PCM_16 = 10,

	/* A video track encoded using ISO/IEC 13818-2 (MPEG-2). */
	TRACK_TYPE_MPEG2_525 = 11,

	/* A video track encoded using ISO/IEC 13818-2 (MPEG-2). */
	TRACK_TYPE_MPEG2_625 = 12,

	/**
	 * A video track encoded using SMPTE 314M or ISO/IEC 61834-2 DV
	 * encoded at 25 Mb/s for 525/60i
	 */
	TRACK_TYPE_DV_BASED_25MB_525 = 13,

	/**
	 * A video track encoded using SMPTE 314M or ISO/IEC 61834-2 DV encoding at 25 Mb/s
	 * for 625/50i.
	 */
	TRACK_TYPE_DV_BASED_25MB_625 = 14,

	/**
	 * A video track encoded using SMPTE 314M DV encoding at 50Mb/s
	 * for 525/50i.
	 */
	TRACK_TYPE_DV_BASED_50MB_525 = 15,

	/**
	 * A video track encoded using SMPTE 314M DV encoding at 50Mb/s for 625/50i
	 */
	TRACK_TYPE_DV_BASED_50_MB_625 = 16,

	/* An AC-3 audio track */
	TRACK_TYPE_AC_3_16b_audio = 17,

	/* A non-PCM AES data track */
	TRACK_TYPE_COMPRESSED_24B_AUDIO = 18,

	/* Ignore it as nice decoder */
	TRACK_TYPE_RESERVED = 19,

	/**
	 * A video track encoded using ISO/IEC 13818-2 (MPEG-2) main profile at main
	 * level or high level, or 4:2:2 profile at main level or high level.
	 */
	TRACK_TYPE_MPEG2_HD = 20,

	/* SMPTE 291M 10-bit type 2 component ancillary data. */
	TRACK_TYPE_ANCILLARY_DATA = 21,

	/* A video track encoded using ISO/IEC 11172-2 (MPEG-1) */
	TRACK_TYPE_MPEG1_525 = 22,

	/* A video track encoded using ISO/IEC 11172-2 (MPEG-1). */
	TRACK_TYPE_MPEG1_625 = 23,

	/* SMPTE 12M time codes For HD material. */
	TRACK_TYPE_TIME_CODE_HD = 24,

} GXFTrackType;

typedef enum ccx_ad_pres_format
{
	PRES_FORMAT_SD = 1,
	PRES_FORMAT_HD = 2,

} GXFAncDataPresFormat;

enum mpeg_picture_coding
{
	CCX_MPC_NONE = 0,
	CCX_MPC_I_FRAME = 1,
	CCX_MPC_P_FRAME = 2,
	CCX_MPC_B_FRAME = 3,
};

enum mpeg_picture_struct
{
	CCX_MPS_NONE = 0,
	CCX_MPS_TOP_FIELD = 1,
	CCX_MPS_BOTTOM_FIELD = 2,
	CCX_MPS_FRAME = 3,
};

struct ccx_gxf_video_track
{
	/* Name of Media File  */
	char track_name[STR_LEN];

	/* Media File system Version */
	uint32_t fs_version;

	/**
	 * Frame Rate Calculate time stamp on basis of this
	 */
	struct ccx_rational frame_rate;

	/**
	 * Lines per frame (valid value for AD tracks)
	 * May be used while parsing vbi
	 */
	uint32_t line_per_frame;

	/**
	 *  Field per frame (Need when parsing vbi)
	 * 1 = Progressive
	 * 2 = Interlaced
	 * -1 = Not applicable
	 * -2 = Not available
	 */
	uint32_t field_per_frame;

	enum mpeg_picture_coding p_code;
	enum mpeg_picture_struct p_struct;
};

struct ccx_gxf_ancillary_data_track
{
	/* Name of Media File  */
	char track_name[STR_LEN];

	/* ID of track */
	unsigned char id;

	/* Presentation Format */
	enum ccx_ad_pres_format ad_format;

	/* Number of ancillary data fields per ancillary data media packet */
	int nb_field;

	/* Byte size of each ancillary data field */
	int field_size;

	/**
	 * Byte size of the ancillary data media packet in 256 byte units:
	 * This value shall be 256, indicating an ancillary data media packet size
	 * of 65536 bytes
	 */
	int packet_size;

	/* Media File system Version */
	uint32_t fs_version;

	/**
	 * Frame Rate XXX AD track do have vaild but this field may
	 * be ignored since related to only video
	 */
	uint32_t frame_rate;

	/**
	 * Lines per frame (valid value for AD tracks)
	 * XXX may be ignored since related to raw video frame
	 */
	uint32_t line_per_frame;

	/* Field per frame Might need if parsed vbi*/
	uint32_t field_per_frame;
};

struct ccx_gxf
{
    int nb_streams;

    /* Name of Media File  */
    char media_name[STR_LEN];

    /**
     *  The first field number shall represent the position on a playout
     *  time line of the first recorded field on a track
     */
    int32_t first_field_nb;

    /**
     * The last field number shall represent the position on a playout
     *  time line of the last recorded field plus one.
     */
    int32_t last_field_nb;

    /**
     * The mark in field number shall represent the position on a playout
     *  time line of the first field to be played from a track.
     */
    int32_t mark_in;

    /**
     * The mark out field number shall represent the position on a playout
     * time line of the last field to be played plus one
     */
    int32_t mark_out;

    /**
     * Estimated size in kb for bytes multiply by 1024
     */
    int32_t stream_size;

    struct ccx_gxf_ancillary_data_track *ad_track;

    struct ccx_gxf_video_track *vid_track;

    /**
     * cdp data buffer
     */
    unsigned char *cdp;
    size_t cdp_len;
};

int ccx_gxf_probe(unsigned char *buf, int len);
int ccx_gxf_get_more_data(struct lib_ccx_ctx *ctx, struct demuxer_data **data);
struct ccx_gxf *ccx_gxf_init(struct ccx_demuxer *arg);
#endif
