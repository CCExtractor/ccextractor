/*
 * GXF Demuxer
 * Copyright (c) 2015 Anshul Maheshwari <er.anshul.maheshwari@gmail.com>
 * License: GPL 2.0
 *
 *
 * You should have received a copy of the GNU General Public
 * License along with CCExtractor; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include "ccx_gxf.h"
#include "ccx_common_common.h"
#include "ccx_common_structs.h"
#include "lib_ccx.h"
#include "activity.h"
#include "utility.h"
#include "ccx_demuxer.h"
#include "file_buffer.h"

#define STR_LEN 256u

#define CLOSED_CAP_DID 0x61
#define CLOSED_C708_SDID 0x01
#define CLOSED_C608_SDID 0x02

#define debug(fmt, ...) ccx_common_logging.debug_ftn(CCX_DMT_PARSE, "GXF:%s:%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define log(fmt, ...) ccx_common_logging.log_ftn("GXF:%d: " fmt, __LINE__, ##__VA_ARGS__)

#undef CCX_GXF_ENABLE_AD_VBI
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

/**
 * @brief parses a packet header, extracting type and length
 * @param ctx Demuxer Ctx used for reading from file
 * @param type detected packet type is stored here
 * @param length detected packet length, excluding header is stored here
 * @return CCX_EINVAL if header not found or contains invalid data,
 *         CCX_EOF if eof reached while reading packet
 *         CCX_OK if stream was fine enough to be parsed
 */
static int parse_packet_header(struct ccx_demuxer *ctx, GXFPktType *type, int *length)
{
	unsigned char pkt_header[16];
	int index = 0;
	long long result;

	result = buffered_read(ctx, pkt_header, 16);
	ctx->past += result;
	if (result != 16)
		return CCX_EOF;

	/**
	 * Veify 5 byte packet leader, must be 0x00 0x00 0x00 0x00 0x00 0x01
	 */
	if (RB32(pkt_header))
		return CCX_EINVAL;
	index += 4;

	if (pkt_header[index] != 1)
		return CCX_EINVAL;
	index++;

	*type = pkt_header[index];
	index++;

	*length = RB32(&pkt_header[index]);
	index += 4;

	if ((*length >> 24) || *length < 16)
		return CCX_EINVAL;
	*length -= 16;

	/**
	 * Reserved as per Rdd-14-2007
	 */
	index += 4;

	/**
	 * verify packet trailer, must be 0xE1 0xE2
	 */
	if (pkt_header[index] != 0xe1)
		return CCX_EINVAL;
	index++;

	if (pkt_header[index] != 0xe2)
		return CCX_EINVAL;
	index++;

	return CCX_OK;
}

static int parse_material_sec(struct ccx_demuxer *demux, int len)
{
	struct ccx_gxf *ctx = demux->private_data;
	int result;
	int ret = CCX_OK;

	if (!ctx)
		goto error;

	while (len > 2)
	{
		unsigned char tag = buffered_get_byte(demux);
		unsigned char tag_len = buffered_get_byte(demux);
		len -= 2;
		if (len < tag_len)
			break;

		if (len < tag_len)
		{
			ret = CCX_EINVAL;
			goto error;
		}
		else
		{
			len -= tag_len;
		}
		switch (tag)
		{
			case MAT_NAME:
				result = buffered_read(demux, (unsigned char *)ctx->media_name, tag_len);
				demux->past += tag_len;
				if (result != tag_len)
				{
					ret = CCX_EOF;
					goto error;
				}
				break;
			case MAT_FIRST_FIELD:
				ctx->first_field_nb = buffered_get_be32(demux);
				break;
			case MAT_LAST_FIELD:
				ctx->last_field_nb = buffered_get_be32(demux);
				break;
			case MAT_MARK_IN:
				ctx->mark_in = buffered_get_be32(demux);
				break;
			case MAT_MARK_OUT:
				ctx->mark_out = buffered_get_be32(demux);
				break;
			case MAT_SIZE:
				ctx->stream_size = buffered_get_be32(demux);
				break;
			default:
				/* Not Supported */
				result = buffered_skip(demux, tag_len);
				demux->past += result;
				break;
		}
	}

error:
	result = buffered_skip(demux, len);
	demux->past += result;
	if (result != len)
		ret = CCX_EOF;
	return ret;
}

static void set_track_frame_rate(struct ccx_gxf_video_track *vid_track, int8_t val)
{
	switch (val)
	{
		case 1:
			vid_track->frame_rate.num = 60;
			vid_track->frame_rate.den = 1;
			break;
		case 2:
			vid_track->frame_rate.num = 60000;
			vid_track->frame_rate.den = 1001;
			break;
		case 3:
			vid_track->frame_rate.num = 50;
			vid_track->frame_rate.den = 1;
			break;
		case 4:
			vid_track->frame_rate.num = 30;
			vid_track->frame_rate.den = 1;
			break;
		case 5:
			vid_track->frame_rate.num = 30000;
			vid_track->frame_rate.den = 1001;
			break;
		case 6:
			vid_track->frame_rate.num = 25;
			vid_track->frame_rate.den = 1;
			break;
		case 7:
			vid_track->frame_rate.num = 24;
			vid_track->frame_rate.den = 1;
			break;
		case 8:
			vid_track->frame_rate.num = 24000;
			vid_track->frame_rate.den = 1001;
			break;
		case -1:
			/* Not applicable for this track type */
		case -2:
			/*  Not available */
		default:
			/* Do nothing in case of no frame rate */
			break;
	}
}
static int parse_mpeg525_track_desc(struct ccx_demuxer *demux, int len)
{
	struct ccx_gxf *ctx = demux->private_data;
	struct ccx_gxf_video_track *vid_track = ctx->vid_track;
	int result;
	int ret = CCX_OK;

	/* Auxiliary Information */
	// char auxi_info[8];
	debug("Mpeg 525 %d\n", len);
	while (len > 2)
	{
		unsigned char tag = buffered_get_byte(demux);
		unsigned char tag_len = buffered_get_byte(demux);
		int val;
		len -= 2;
		if (len < tag_len)
			break;

		if (len < tag_len)
		{
			ret = CCX_EINVAL;
			goto error;
		}
		else
		{
			len -= tag_len;
		}
		switch (tag)
		{
			case TRACK_NAME:
				result = buffered_read(demux, (unsigned char *)vid_track->track_name, tag_len);
				demux->past += tag_len;
				if (result != tag_len)
				{
					ret = CCX_EOF;
					goto error;
				}
				break;
			case TRACK_VER:
				vid_track->fs_version = buffered_get_be32(demux);
				break;
			case TRACK_FPS:
				val = buffered_get_be32(demux);
				set_track_frame_rate(vid_track, val);
				break;
			case TRACK_LINES:
				vid_track->line_per_frame = buffered_get_be32(demux);
				break;
			case TRACK_FPF:
				vid_track->field_per_frame = buffered_get_be32(demux);
				break;
			case TRACK_AUX:
			case TRACK_MPG_AUX:
			default:
				/* Not Supported */
				result = buffered_skip(demux, tag_len);
				demux->past += result;
				break;
		}
	}
error:
	result = buffered_skip(demux, len);
	demux->past += result;
	if (result != len)
		ret = CCX_EOF;
	return ret;
}

static int parse_ad_track_desc(struct ccx_demuxer *demux, int len)
{
	struct ccx_gxf *ctx = demux->private_data;
	struct ccx_gxf_ancillary_data_track *ad_track = ctx->ad_track;
	int result;
	int ret = CCX_OK;
	/* Auxiliary Information */
	char auxi_info[8];
	debug("Ancillary Data %d\n", len);

	if (!ctx->ad_track)
		goto error;

	ad_track = ctx->ad_track;

	while (len > 2)
	{
		unsigned char tag = buffered_get_byte(demux);
		unsigned char tag_len = buffered_get_byte(demux);
		len -= 2;
		if (len < tag_len)
			break;

		if (len < tag_len)
		{
			ret = CCX_EINVAL;
			goto error;
		}
		else
		{
			len -= tag_len;
		}
		switch (tag)
		{
			case TRACK_NAME:
				result = buffered_read(demux, (unsigned char *)ad_track->track_name, tag_len);
				demux->past += tag_len;
				if (result != tag_len)
				{
					ret = CCX_EOF;
					goto error;
				}
				break;
			case TRACK_AUX:
				result = buffered_read(demux, (unsigned char *)auxi_info, 8);
				demux->past += 8;
				if (result != 8)
				{
					ret = CCX_EOF;
					goto error;
				}
				if (tag_len != 8)
				{
					ret = CCX_EINVAL;
					goto error;
				}
				ad_track->ad_format = auxi_info[2];
				ad_track->nb_field = auxi_info[3];
				ad_track->field_size = *((int16_t *)(auxi_info + 4));	     //RB16(auxi_info + 4);
				ad_track->packet_size = *((int16_t *)(auxi_info + 6)) * 256; //RB16(auxi_info + 6);
				debug("ad_format %d nb_field %d field_size %d packet_size %d track id %d\n",
				      ad_track->ad_format, ad_track->nb_field, ad_track->field_size, ad_track->packet_size, ad_track->id);
				break;
			case TRACK_VER:
				ad_track->fs_version = buffered_get_be32(demux);
				break;
			case TRACK_FPS:
				ad_track->frame_rate = buffered_get_be32(demux);
				break;
			case TRACK_LINES:
				ad_track->line_per_frame = buffered_get_be32(demux);
				break;
			case TRACK_FPF:
				ad_track->field_per_frame = buffered_get_be32(demux);
				break;
			case TRACK_MPG_AUX:
			default:
				/* Not Supported */
				result = buffered_skip(demux, tag_len);
				demux->past += result;
				break;
		}
	}

error:
	result = buffered_skip(demux, len);
	demux->past += result;
	if (result != len)
		ret = CCX_EOF;
	return ret;
}

static int parse_track_sec(struct ccx_demuxer *demux, int len, struct demuxer_data *data)
{
	int result;
	int ret = CCX_OK;
	struct ccx_gxf *ctx = demux->private_data;

	if (!ctx)
		goto error;

	while (len > 4)
	{
		GXFTrackType track_type = buffered_get_byte(demux);
		unsigned char track_id = buffered_get_byte(demux);
		unsigned char track_len = buffered_get_be16(demux);
		len -= 4;
		if (len < track_len)
		{
			ret = CCX_EINVAL;
			goto error;
		}

		if ((track_type & 0x80) != 0x80)
		{
			len -= track_len;
			result = buffered_skip(demux, track_len);
			demux->past += result;
			if (result != track_len)
			{
				ret = CCX_EOF;
				goto error;
			}
			else
				continue;
		}
		track_type &= 0x7f;

		if ((track_id & 0xc0) != 0xc0)
		{
			len -= track_len;
			result = buffered_skip(demux, track_len);
			demux->past += result;
			if (result != track_len)
			{
				ret = CCX_EOF;
				goto error;
			}
			else
				continue;
		}
		track_id &= 0xcf;

		if (track_type == TRACK_TYPE_ANCILLARY_DATA)
		{
			if (!ctx->ad_track)
				ctx->ad_track = malloc(sizeof(struct ccx_gxf_ancillary_data_track));

			if (!ctx->ad_track)
			{
				log("Ignored Ancillary track due to insufficient memory\n");
				goto error;
			}
			ctx->ad_track->id = track_id;
			parse_ad_track_desc(demux, track_len);
			/* Ancillary data track have raw Closed caption with cctype */
			data->bufferdatatype = CCX_RAW_TYPE;
			len -= track_len;
		}
		/* if already have ad_track context then not parse mpeg for cc*/
		else if (track_type == TRACK_TYPE_MPEG2_525)
		{
			if (!ctx->vid_track)
				ctx->vid_track = malloc(sizeof(struct ccx_gxf_ancillary_data_track));

			if (!ctx->vid_track)
			{
				log("Ignored mpeg track due to insufficient memory\n");
				goto error;
			}
			parse_mpeg525_track_desc(demux, track_len);
			data->bufferdatatype = CCX_PES;
			len -= track_len;
		}
		else
		{
			result = buffered_skip(demux, track_len);
			demux->past += result;
			len -= track_len;
			if (result != track_len)
			{
				ret = CCX_EOF;
				goto error;
			}
			else
				continue;
		}
	}
error:
	result = buffered_skip(demux, len);
	demux->past += result;
	if (result != len)
		ret = CCX_EOF;
	return ret;
}

/**
 * Parse Caption Distribution Packet
 * General Syntax of cdp
 * cdp() {
 *   cdp_header();
 *   time_code_section();
 *   ccdata_section();
 *   ccsvcinfo_section();
 *   cdp_footer();
 * }
 * function does not parse cdp in chunk, user should provide complete cdp
 * with header and footer inclusive of checksum
 * @return CCX_EINVAL if cdp data fields are not valid
 */

int parse_ad_cdp(unsigned char *cdp, size_t len, struct demuxer_data *data)
{

	int ret = CCX_OK;
	uint16_t cdp_length;
	uint16_t cdp_framerate;
	uint16_t cc_data_present;
	uint16_t caption_service_active;
	uint16_t cdp_header_sequence_counter;

	/* Do not accept packet whose length does not fit header and footer */
	if (len < 11)
	{
		log("Short packet cant accommodate header and footer\n");
		return CCX_EINVAL;
	}

	/* Verify cdp header identifier */
	if ((cdp[0] != 0x96) || (cdp[1] != 0x69))
	{
		log(" could not find CDP identifier of 0x96 0x69\n");
		return CCX_EINVAL;
	}
	cdp += 2;

	cdp_length = *cdp++;
	if (cdp_length != len)
	{
		log("Cdp length is not valid\n");
		return CCX_EINVAL;
	}

	/**
	 * cdp_frame_rate
	 *===============================
	 * value |   frame rate
	 *================================
	 * 0     |   Forbidden
	 * 1     |    ~23.976
	 * 2     |     24
	 * 3     |     25
	 * 4     |    ~29.97
	 * 5     |     30
	 * 6     |     50
	 * 7     |    ~59.94
	 * 8     |     60
	 */
	cdp_framerate = (*cdp++ & 0xf0) >> 4;
	cc_data_present = (*cdp & 0x40) >> 6;
	caption_service_active = (*cdp++ & 0x02) >> 1;
	cdp_header_sequence_counter = RB16(cdp);
	cdp += 2;

	debug(" CDP length: %d words\n", cdp_length);
	debug(" CDP frame rate: 0x%x\n", cdp_framerate);
	debug(" CC data present: %d\n", cc_data_present);
	debug("caption service active: %d\n", caption_service_active);
	debug("header sequence counter: %d  (0x%x)\n", cdp_header_sequence_counter, cdp_header_sequence_counter);

	/**
	 * CDP spec does not allow to have multiple section in single packet
	 * therefor if else if ladder does not require switch with while
	 */
	if (*cdp == 0x71)
	{
		cdp++;
		log("Ignore Time code section\n");
		return -1;
	}
	else if (*cdp == 0x72)
	{
		uint16_t cc_count;

		cdp++;
		cc_count = *cdp++ & 0x1f;
		debug("cc_count: %d\n", cc_count);

		for (int i = 0; i < cc_count * 3; i++)
		{
			data->buffer[data->len + i] = cdp[i];
		}
		data->len += cc_count * 3;
		cdp += cc_count * 3;
	}
	else if (*cdp == 0x73)
	{
		cdp++;
		log("Ignore service information section\n");
		return -1;
	}
	else if (*cdp >= 0x75 && *cdp <= 0xEF)
	{
		log(" Please share sample, newer version of SMPTE-334 specification are followed\n"
		    "New section id 0x%x\n",
		    *cdp);
		cdp++;
		return -1;
	}

	/* check cdp footer */
	if (*cdp == 0x74)
	{
		cdp++;
		if (cdp_header_sequence_counter != RB16(cdp))
		{
			log("Incomplete cdp packet\n");
			return CCX_EINVAL;
		}
		cdp += 2;
		/**
		 * TODO check cdp packet initgrity with packet checksum
		 * packet_checksum – This 8-bit field shall contain the 8-bit value necessary to make the arithmetic sum of the
		 * entire packet (first byte of cdp_identifier to packet_checksum, inclusive) modulo 256 equal zero.
		 */
	}

	return ret;
}

/**
 * parse ancillary data payload
 */
static int parse_ad_pyld(struct ccx_demuxer *demux, int len, struct demuxer_data *data)
{
	int ret = CCX_OK;
	int result = 0;
#ifndef CCX_GXF_ENABLE_AD_VBI
	int i;

	uint16_t d_id;
	uint16_t sd_id;
	uint16_t dc;
	struct ccx_gxf *ctx = demux->private_data;

	len -= 6;
	d_id = buffered_get_le16(demux);
	sd_id = buffered_get_le16(demux);
	dc = buffered_get_le16(demux);
	dc = (dc & 0xFF);

	if (ctx->cdp_len < len / 2)
	{
		ctx->cdp = realloc(ctx->cdp, len / 2);
		if (ctx->cdp == NULL)
		{
			log("Could not allocate buffer %d\n", len / 2);
			ret = CCX_ENOMEM;
			goto error;
		}
		/* exclude did sdid bytes in cdp_len */
		ctx->cdp_len = ((len - 2) / 2);
	}

	if (((d_id & 0xff) == CLOSED_CAP_DID) && ((sd_id & 0xff) == CLOSED_C708_SDID))
	{
		/* leaving 2 byte of checksum */
		for (i = 0; len > 2; i++, len -= 2)
		{
			unsigned short dat = buffered_get_le16(demux);
			/**
			 * check parity for 0xFE and 0x01 they may be converted by GXF
			 * from 0xFF and 0x00 respectively and ignoring first 2 bit or byte
			 * from 10 bit code in 16bit variable and we hope that they have not
			 * changed its parity otherwise we have lost all 0xFF and 0x00
			 */
			if (dat == 0x2fe)
				ctx->cdp[i] = 0xFF;
			else if (dat == 0x201)
				ctx->cdp[i] = 0x01;
			else
				ctx->cdp[i] = dat & 0xFF;
		}
		parse_ad_cdp(ctx->cdp, ctx->cdp_len, data);
		//TODO check checksum
	}
	else if (((d_id & 0xff) == CLOSED_CAP_DID) && ((sd_id & 0xff) == CLOSED_C608_SDID))
	{
		log("Need Sample\n");
	}
	else
	{
		/**
		 * ignore other service like following
		 * Program description (DTV) with DID = 62h(162h)  and SDID 1 (101h)
		 * Data broadcast (DTV) with DID = 62h(162h) and SDID 2 (102h)
		 * VBI data 62h 62h 62h DID with DID = 62h(162h) and SDID 3 (103h)
		 */
	}
error:
#endif
	result = buffered_skip(demux, len);
	demux->past += result;
	if (result != len)
		ret = CCX_EOF;

	return ret;
}

/**
 * VBI in ancillary data is not specified in GXF specs
 * but while traversing file, we found vbi data presence
 * in Multimedia file, so if there are some video which
 * show caption on tv or there software but ccextractor
 * is not able to see the caption, there might be need
 * of parsing vbi
 */
static int parse_ad_vbi(struct ccx_demuxer *demux, int len, struct demuxer_data *data)
{
	int ret = CCX_OK;
	int result = 0;

#ifdef CCX_GXF_ENABLE_AD_VBI
	data->len += len;
	result = buffered_read(demux, data->buffer, len);
#else
	result = buffered_skip(demux, len);
#endif
	demux->past += result;
	if (result != len)
		ret = CCX_EOF;
	return ret;
}

static int parse_ad_field(struct ccx_demuxer *demux, int len, struct demuxer_data *data)
{
	int ret = CCX_OK;
	int result = 0;
	char tag[5];
	int field_identifier;

	tag[4] = '\0';

	len -= 4;
	result = buffered_read(demux, (unsigned char *)tag, 4);
	demux->past += result;
	if (strncmp(tag, "finf", 4))
		log("Warning: No finf tag\n");

	len -= 4;
	if (buffered_get_le32(demux) != 4)
		log("Warning: expected 4 acc GXF specs\n");

	len -= 4;
	field_identifier = buffered_get_le32(demux);
	debug("LOG: field identifier %d\n", field_identifier);

	len -= 4;
	result = buffered_read(demux, (unsigned char *)tag, 4);
	demux->past += result;
	if (strncmp(tag, "LIST", 4))
		log("Warning: No List tag\n");

	len -= 4;
	/* Read Byte size of the ancillary data field section vector */
	if (buffered_get_le32(demux) != len)
		log("Warning: Unexpected sample size (!=%d)\n", len);

	len -= 4;
	result = buffered_read(demux, (unsigned char *)tag, 4);
	demux->past += result;
	if (strncmp(tag, "anc ", 4))
		log("Warning: No anc tag\n");

	while (len > 28)
	{
		int line_nb;
		int luma_flag;
		int hanc_vanc_flag;
		int hdr_len;
		int pyld_len;

		len -= 4;
		result = buffered_read(demux, (unsigned char *)tag, 4);
		demux->past += result;

		len -= 4;
		hdr_len = buffered_get_le32(demux);

		/**
		 * IN GXF video there are 2 pad but if I ignore first pad tag then there is data inside it
		 * There must be data that time but it was not there
		 * or it may be the consequence of pad tag inserted later and data is still present
		 */
		if (!strncmp(tag, "pad ", 4))
		{
			if (hdr_len != len)
				log("Warning: expected %d got %d\n", len, hdr_len);

			len -= hdr_len;
			result = buffered_skip(demux, hdr_len);
			demux->past += result;
			if (result != hdr_len)
				ret = CCX_EOF;
			continue;
		}
		else if (!strncmp(tag, "pos ", 4))
		{
			if (hdr_len != 12)
				log("Warning: expected 4 got %d\n", hdr_len);
		}
		else
		{
			log("Warning: Instead pos tag got %s tag\n", tag);
			if (hdr_len != 12)
				log("Warning: expected 4 got %d\n", hdr_len);
		}

		len -= 4;
		line_nb = buffered_get_le32(demux);
		debug("Line nb: %d\n", line_nb);

		len -= 4;
		luma_flag = buffered_get_le32(demux);
		debug("luma color diff flag: %d\n", luma_flag);

		len -= 4;
		hanc_vanc_flag = buffered_get_le32(demux);
		debug("hanc/vanc flag: %d\n", hanc_vanc_flag);

		len -= 4;
		result = buffered_read(demux, (unsigned char *)tag, 4);
		demux->past += result;

		len -= 4;
		pyld_len = buffered_get_le32(demux);
		debug("pyld len: %d\n", pyld_len);

		if (!strncmp(tag, "pyld", 4))
		{
			len -= pyld_len;
			ret = parse_ad_pyld(demux, pyld_len, data);
			if (ret == CCX_EOF)
				break;
		}
		else if (!strncmp(tag, "vbi ", 4))
		{
			len -= pyld_len;
			ret = parse_ad_vbi(demux, pyld_len, data);
			if (ret == CCX_EOF)
				break;
		}
		else
		{
			log("Warning: No pyld/vbi tag got %s tag\n", tag);
		}
	}

	result = buffered_skip(demux, len);
	demux->past += result;
	if (result != len)
		ret = CCX_EOF;
	return ret;
}

/**
 * @param vid_format following format are supported to set valid timebase
 * 	in demuxer data
 * value   |  Meaning
 *=====================================
 *  0      | 525 interlaced lines at 29.97 frames / sec
 *  1      | 625 interlaced lines at 25 frames / sec
 *  2      | 720 progressive lines at 59.94 frames / sec
 *  3      | 720 progressive lines at 60 frames / sec
 *  4      | 1080 progressive lines at 23.98 frames / sec
 *  5      | 1080 progressive lines at 24 frames / sec
 *  6      | 1080 progressive lines at 25 frames / sec
 *  7      | 1080 progressive lines at 29.97 frames / sec
 *  8      | 1080 progressive lines at 30 frames / sec
 *  9      | 1080 interlaced lines at 25 frames / sec
 *  10     | 1080 interlaced lines at 29.97 frames / sec
 *  11     | 1080 interlaced lines at 30 frames / sec
 *  12     | 1035 interlaced lines at 30 frames / sec
 *  13     | 1035 interlaced lines at 29.97 frames / sec
 *  14     | 720 progressive lines at 50 frames / sec
 *  15     | 525 progressive lines at 59.94 frames / sec
 *  16     | 525 progressive lines at 60 frames / sec
 *  17     | 525 progressive lines at 29.97 frames / sec
 *  18     | 525 progressive lines at 30 frames / sec
 *  19     | 525 progressive lines at 50 frames / sec
 *  20     | 525 progressive lines at 25 frames / sec
 *
 * @param data already allocated data, passing NULL will end up in seg-fault
 *        data len may be zero, while setting timebase we don not care about
 *        actual data
 */
static void set_data_timebase(int32_t vid_format, struct demuxer_data *data)
{
	debug("LOG:Format Video %d\n", vid_format);

	switch (vid_format)
	{
		case 0:
		case 7:
		case 10:
		case 13:
		case 17:
			data->tb.den = 30000;
			data->tb.num = 1001;
			break;
		case 1:
		case 6:
		case 9:
		case 20:
			data->tb.den = 25;
			data->tb.num = 1;
			break;
		case 2:
		case 15:
			data->tb.den = 60000;
			data->tb.num = 1001;
			break;
		case 3:
		case 16:
			data->tb.den = 60;
			data->tb.num = 1;
			break;
		case 4:
			data->tb.den = 24000;
			data->tb.num = 1001;
			break;
		case 5:
			data->tb.den = 24;
			data->tb.num = 1;
			break;
		case 8:
		case 11:
		case 12:
		case 18:
			data->tb.den = 30;
			data->tb.num = 1;
			break;
		case 14:
		case 19:
			data->tb.den = 50;
			data->tb.num = 1;
			break;
		default:
			break;
	}
}

static int parse_mpeg_packet(struct ccx_demuxer *demux, int len, struct demuxer_data *data)
{
	int ret = CCX_OK;
	int result = 0;

	result = buffered_read(demux, data->buffer + data->len, len);
	data->len += len;
	demux->past += result;
	if (result != len)
		ret = CCX_EOF;
	return ret;
}

/**
 * This packet contain RIFF data
 * @param demuxer Demuxer must contain vaild ad_track structure
 */
static int parse_ad_packet(struct ccx_demuxer *demux, int len, struct demuxer_data *data)
{
	int ret = CCX_OK;
	int result = 0;
	int i;
	int val;
	char tag[4];

	struct ccx_gxf *ctx = demux->private_data;
	struct ccx_gxf_ancillary_data_track *ad_track = ctx->ad_track;

	len -= 4;
	result = buffered_read(demux, (unsigned char *)tag, 4);
	demux->past += result;
	if (strncmp(tag, "RIFF", 4))
		log("Warning: No RIFF header\n");

	len -= 4;
	if (buffered_get_le32(demux) != 65528)
		log("Warning: ADT packet with non trivial length\n");

	len -= 4;
	result = buffered_read(demux, (unsigned char *)tag, 4);
	demux->past += result;
	if (strncmp(tag, "rcrd", 4))
		log("Warning: No rcrd tag\n");

	len -= 4;
	result = buffered_read(demux, (unsigned char *)tag, 4);
	demux->past += result;
	if (strncmp(tag, "desc", 4))
		log("Warning: No desc tag\n");

	len -= 4;
	if (buffered_get_le32(demux) != 20)
		log("Warning: Unexpected desc length(!=20)\n");

	len -= 4;
	if (buffered_get_le32(demux) != 2)
		log("Warning: Unsupported version (!=2)\n");

	len -= 4;
	/**
	 * The number of fields in the ancillary data field section vector. Shall be 10 for high
	 * definition ancillary data and 14 for standard definition ancillary data.
	 */
	val = buffered_get_le32(demux);
	if (ad_track->nb_field != val)
		log("Warning: Ambiguous number of fields\n");

	len -= 4;
	/**
	 * The length of the ancillary data field descriptions, ancillary data sample payloads,
	 * and the associated padding.
	 * This field shall be set to 6340 for high definition video formats and to 4676 for
	 * standard definition video formats.
	 */
	val = buffered_get_le32(demux);
	if (ad_track->field_size != val)
		log("Warning: Ambiguous field size\n");

	len -= 4;
	/* Read byte size of the complete ancillary media packet */
	if (buffered_get_le32(demux) != 65536)
		log("Warning: Unexpected buffer size (!=65536)\n");

	len -= 4;
	val = buffered_get_le32(demux);
	set_data_timebase(val, data);

	len -= 4;
	result = buffered_read(demux, (unsigned char *)tag, 4);
	demux->past += result;
	if (strncmp(tag, "LIST", 4))
		log("Warning: No LIST tag\n");

	len -= 4;
	/* Read Byte size of the ancillary data field section vector */
	if (buffered_get_le32(demux) != len)
		log("Warning: Unexpected field sec  size (!=%d)\n", len);

	len -= 4;
	result = buffered_read(demux, (unsigned char *)tag, 4);
	demux->past += result;
	if (strncmp(tag, "fld ", 4))
		log("Warning: No fld tag\n");

	for (i = 0; i < ad_track->nb_field; i++)
	{
		len -= ad_track->field_size;
		parse_ad_field(demux, ad_track->field_size, data);
	}
	result = buffered_skip(demux, len);
	demux->past += result;
	if (result != len)
		ret = CCX_EOF;
	return ret;
}

/**
 *    +-----------------------------+-----------------------------+
 *    | Bits (0 is LSB; 7 is MSB)   |          Meaning            |
 *    +-----------------------------+-----------------------------+
 *    |                             |        Picture Coding       |
 *    |                             |         00 = NOne           |
 *    |          0:1                |         01 = I frame        |
 *    |                             |         10 = P Frame        |
 *    |                             |         11 = B Frame        |
 *    +-----------------------------+-----------------------------+
 *    |                             |      Picture Structure      |
 *    |                             |         00 = NOne           |
 *    |          2:3                |         01 = I frame        |
 *    |                             |         10 = P Frame        |
 *    |                             |         11 = B Frame        |
 *    +-----------------------------+-----------------------------+
 *    |          4:7                |      Not Used MUST be 0     |
 *    +-----------------------------+-----------------------------+
 */
static void set_mpeg_frame_desc(struct ccx_gxf_video_track *vid_track, unsigned char mpeg_frame_desc_flag)
{
	vid_track->p_code = mpeg_frame_desc_flag & 0x03;
	vid_track->p_struct = (mpeg_frame_desc_flag >> 2) & 0x03;
}
static int parse_media(struct ccx_demuxer *demux, int len, struct demuxer_data *data)
{
	int ret = CCX_OK;
	int result = 0;
	GXFTrackType media_type;
	struct ccx_gxf *ctx = demux->private_data;
	struct ccx_gxf_ancillary_data_track *ad_track;
	//struct ccx_gxf_video_track *vid_track;
	/**
	 * The track number is a reference into the track description section of the map packets. It identifies the media
	 * file to which the current media packet belongs. Track descriptions shall be considered as consecutive
	 * elements of a vector and track numbers are the index into that vector.
	 * Clips shall have 1 to 48 tracks in any combination of types. Tracks shall be numbered from 0 to n–1. Media
	 * packets shall be inserted in the stream starting with track n-1 and proceeding to track 0. Time code packets
	 * shall have the greatest track numbers. Ancillary data packets shall have track numbers less than time code
	 * packets. Audio packets shall have track numbers less than ancillary data packets. Video packet track
	 * numbers shall be less than audio track numbers.
	 */
	unsigned char track_nb;
	unsigned int media_field_nb;

	/**
	 * For ancillary data, the field information contains the first valid ancillary data field number (inclusive) and the
	 * last valid ancillary data field number (exclusive). The first and last valid ancillary data field numbers apply to
	 * the current packet only. If the entire ancillary data packet is valid, the first and last valid ancillary data field
	 * numbers shall be 0 and 10 for high definition ancillary data, and 0 and 14 for standard definition ancillary data.
	 * These values shall be sent starting with the ancillary data field from the lowest number video line continuing to
	 * higher video line numbers. Within each line the ancillary data fields shall not be reordered.
	 */
	unsigned char first_field_nb = 0;
	unsigned char last_field_nb = 0;

	/**
	 *  see description of set_mpeg_frame_desc for details
	 */
	unsigned int mpeg_pic_size;
	unsigned int mpeg_frame_desc_flag;

	/**
	 * Observation 1: media_field_nb comes out equal to time_field number
	 * for ancillary data
	 *
	 * The 32-bit unsigned field number relative to the start of the
	 * material (time line position).
	 * Time line field numbers shall be assigned consecutively from the
	 * start of the material. The first time line field number shall be 0.
	 */
	unsigned int time_field;
	unsigned char valid_time_field;

	if (!ctx)
		goto end;

	len -= 1;
	media_type = buffered_get_byte(demux);
	track_nb = buffered_get_byte(demux);
	len -= 1;
	media_field_nb = buffered_get_be32(demux);
	len -= 4;

	switch (media_type)
	{
		case TRACK_TYPE_ANCILLARY_DATA:
			first_field_nb = buffered_get_be16(demux);
			len -= 2;
			last_field_nb = buffered_get_be16(demux);
			len -= 2;
			break;
		case TRACK_TYPE_MPEG1_525:
		case TRACK_TYPE_MPEG2_525:
			mpeg_pic_size = buffered_get_be32(demux);
			mpeg_frame_desc_flag = mpeg_pic_size >> 24;
			mpeg_pic_size &= 0xFFFFFF;
			len -= 4;
			break;
		default:
			result = buffered_skip(demux, 4);
			demux->past += result;
			len -= 4;
	}

	time_field = buffered_get_be32(demux);
	len -= 4;

	valid_time_field = buffered_get_byte(demux) & 0x01;
	len -= 1;

	result = buffered_skip(demux, 1);
	demux->past += result;
	len -= 1;

	debug("track number%d\n", track_nb);
	debug("field number %d\n", media_field_nb);
	debug("first field number %d\n", first_field_nb);
	debug("last field number %d\n", last_field_nb);
	debug("Pyld len %d\n", len);
	debug("\n");

	if (media_type == TRACK_TYPE_ANCILLARY_DATA)
	{
		if (!ctx->ad_track)
		{
			goto end;
		}
		ad_track = ctx->ad_track;
		if (valid_time_field)
		{
			data->pts = time_field - ctx->first_field_nb;
		}
		else
		{
			data->pts = media_field_nb - ctx->first_field_nb;
		}
		if (len < ad_track->packet_size)
			goto end;

		data->pts /= 2;

		parse_ad_packet(demux, ad_track->packet_size, data);
		len -= ad_track->packet_size;
	}
	else if (media_type == TRACK_TYPE_MPEG2_525 && !ctx->ad_track)
	{
		if (!ctx->vid_track)
		{
			goto end;
		}
		//vid_track = ctx->vid_track;
		if (valid_time_field)
		{
			data->pts = time_field - ctx->first_field_nb;
		}
		else
		{
			data->pts = media_field_nb - ctx->first_field_nb;
		}
		data->tb.num = ctx->vid_track->frame_rate.den;
		data->tb.den = ctx->vid_track->frame_rate.num;
		data->pts /= 2;

		set_mpeg_frame_desc(ctx->vid_track, mpeg_frame_desc_flag);
		parse_mpeg_packet(demux, mpeg_pic_size, data);
		len -= mpeg_pic_size;
	}
	else if (media_type == TRACK_TYPE_TIME_CODE_525)
	{
		/* Need SMPTE 12M to follow parse time code */
	}

end:
	result = buffered_skip(demux, len);
	demux->past += result;
	if (result != len)
		ret = CCX_EOF;
	return ret;
}

/**
 * Dummy function that ignore field locator table packet
 */
static int parse_flt(struct ccx_demuxer *ctx, int len)
{
	int ret = CCX_OK;
	int result = 0;

	result = buffered_skip(ctx, len);
	ctx->past += result;
	if (result != len)
		ret = CCX_EOF;
	return ret;
}

/**
 * Dummy function that ignore unified material format packet
 */
static int parse_umf(struct ccx_demuxer *ctx, int len)
{
	int ret = CCX_OK;
	int result = 0;

	result = buffered_skip(ctx, len);
	ctx->past += result;
	if (result != len)
		ret = CCX_EOF;
	return ret;
}
/**
 * Its this function duty to use len length buffer from demuxer
 *
 * This function gives basic info about tracks, here we get to know
 * whether Ancillary Data track is present or not.
 * If ancillary data track is not present only then it check for
 * presence of mpeg track.
 * return CCX_EINVAL if things are not following specs
 *
 * TODO do buffer cahce to know that you are not reading after eof
 */
static int parse_map(struct ccx_demuxer *ctx, int len, struct demuxer_data *data)
{
	int result = 0;
	int material_sec_len = 0;
	int track_sec_len = 0;
	int ret = CCX_OK;

	len -= 2;
	if (buffered_get_be16(ctx) != 0xe0ff)
		goto error;

	len -= 2;
	material_sec_len = buffered_get_be16(ctx);
	if (material_sec_len > len)
		goto error;

	len -= material_sec_len;
	parse_material_sec(ctx, material_sec_len);

	len -= 2;
	track_sec_len = buffered_get_be16(ctx);
	if (track_sec_len > len)
		goto error;

	len -= track_sec_len;
	parse_track_sec(ctx, track_sec_len, data);
error:
	result = buffered_skip(ctx, len);
	ctx->past += result;
	if (result != len)
		ret = CCX_EOF;
	return ret;
}

/**
 * GXF Media File have 5 Section which are as following
 *     +----------+-------+------+---------------+--------+
 *     |          |       |      |               |        |
 *     |  MAP     |  FLT  |  UMF |  Media Packet |   EOS  |
 *     |          |       |      |               |        |
 *     +----------+-------+------+---------------+--------+
 *
 */

static int read_packet(struct ccx_demuxer *ctx, struct demuxer_data *data)
{
	int len = 0;
	int result = 0;
	int ret;
	GXFPktType type;

	ret = parse_packet_header(ctx, &type, &len);

	switch (type)
	{
		case PKT_MAP:
			debug("pkt type Map %d\n", len);
			ret = parse_map(ctx, len, data);
			break;
		case PKT_MEDIA:
			ret = parse_media(ctx, len, data);
			break;
		case PKT_EOS:
			ret = CCX_EOF;
			break;
		case PKT_FLT:
			debug("pkt type FLT %d\n", len);
			ret = parse_flt(ctx, len);
			break;
		case PKT_UMF:
			debug("pkt type umf %d\n\n", len);
			ret = parse_umf(ctx, len);
			break;
		default:
			debug("pkt type unknown or bad %d\n", len);
			result = buffered_skip(ctx, len);
			ctx->past += result;
			if (result != len || !len)
				ret = CCX_EOF;
	}

	return ret;
}

/**
 * @param buf buffer with atleast acceptable length atleast 7 byte
 *            where we will test only important part of packet header
 *            In GXF packet header is of 16 byte and in header there is
 *            packet leader of 5 bytes 00 00 00 00 01
 *            Stream Starts with Map packet which is known by looking at offset 0x05
 *            of packet header.
 *            TODO Map packet are sent per 100 packets so search MAP packet, there might be
 *            no MAP header at start if GXF is sliced at unknown region
 */
int ccx_gxf_probe(unsigned char *buf, int len)
{
	static const uint8_t startcode[] = {0, 0, 0, 0, 1, 0xbc};
	if (len < sizeof(startcode))
		return CCX_FALSE;

	if (!memcmp(buf, startcode, sizeof(startcode)))
		return CCX_TRUE;
	return CCX_FALSE;
}

int ccx_gxf_get_more_data(struct lib_ccx_ctx *ctx, struct demuxer_data **ppdata)
{
	int ret;
	struct demuxer_data *data;

	if (!*ppdata)
	{
		*ppdata = alloc_demuxer_data();
		if (!*ppdata)
			return -1;

		data = *ppdata;
		//TODO Set to dummy, find and set actual value
		//Complex GXF do have multiple program
		data->program_number = 1;
		data->stream_pid = 1;
		data->codec = CCX_CODEC_ATSC_CC;
	}
	else
	{
		data = *ppdata;
	}

	ret = read_packet(ctx->demux_ctx, data);
	return ret;
}

struct ccx_gxf *ccx_gxf_init(struct ccx_demuxer *arg)
{
	struct ccx_gxf *ctx = (struct ccx_gxf *)malloc(sizeof(struct ccx_gxf));
	if (!ctx)
	{
		log("Unable to allocate Gxf context\n");
		return NULL;
	}
	memset(ctx, 0, sizeof(struct ccx_gxf));

	return ctx;
}

void ccx_gxf_delete(struct ccx_demuxer *arg)
{
	struct ccx_gxf *ctx = arg->private_data;

	freep(&ctx->cdp);
	freep(&arg->private_data);
}
