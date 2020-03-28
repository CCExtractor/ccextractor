/**
 * Copyright (c) 2015 Anshul Maheshwari <er.anshul.maheshwari@gmail.com>
 */

#include "ccx_demuxer_mxf.h"
#include "ccx_demuxer.h"
#include "ccx_common_common.h"
#include "file_buffer.h"
#include "utility.h"
#include "lib_ccx.h"

#define debug(fmt, ...) ccx_common_logging.debug_ftn(CCX_DMT_PARSE, "MXF:%s:%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define log(fmt, ...) ccx_common_logging.log_ftn("MXF:%d: " fmt, __LINE__, ##__VA_ARGS__)
#define IS_KLV_KEY(x, y) (!memcmp(x, y, sizeof(y)))

enum MXFCaptionType
{
	MXF_CT_VBI,
	MXF_CT_ANC,
};

typedef uint8_t UID[16];
typedef struct KLVPacket
{
	UID key;
	uint64_t length;
} KLVPacket;

typedef struct MXFCodecUL
{
	UID uid;
	enum MXFCaptionType type;
} MXFCodecUL;

typedef int ReadFunc(struct ccx_demuxer *ctx, uint64_t size);

typedef struct
{
	int track_id;
	uint8_t track_number[4];
} MXFTrack;

typedef struct MXFReadTableEntry
{
	const UID key;
	ReadFunc *read;
} MXFReadTableEntry;

typedef struct MXFContext
{
	enum MXFCaptionType type;
	int cap_track_id;
	UID cap_essence_key;
	MXFTrack tracks[32];
	int nb_tracks;
	int cap_count;
	struct ccx_rational edit_rate;
} MXFContext;

typedef struct MXFLocalTAGS
{
	uint16_t tag;
	char *name;
	int length;
} MXFLocalTAGS;

//S337 table 4 says 14 bytes can determine partition pack type
static const uint8_t mxf_header_partition_pack_key[] = {0x06, 0x0e, 0x2b, 0x34, 0x02, 0x05, 0x01, 0x01, 0x0d, 0x01, 0x02, 0x01, 0x01, 0x02};
static const uint8_t mxf_essence_element_key[] = {0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01, 0x0d, 0x01, 0x03, 0x01};
static const uint8_t mxf_klv_key[] = {0x06, 0x0e, 0x2b, 0x34};

static const MXFCodecUL mxf_caption_essence_container[] = {
    {{0x6, 0xE, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x09, 0xD, 0x1, 0x3, 0x1, 0x2, 0xD, 0x0, 0x0}, MXF_CT_VBI},
    {{0x6, 0xE, 0x2B, 0x34, 0x04, 0x01, 0x01, 0x09, 0xD, 0x1, 0x3, 0x1, 0x2, 0xE, 0x0, 0x0}, MXF_CT_ANC},
};

static const struct ccx_rational framerate_rationals[16] = {
    {0, 0},
    {1001, 24000},
    {1, 24},
    {1, 25},
    {1001, 30000},
    {1, 30},
    {1, 50},
    {1001, 60000},
    {1, 60},
    {0, 0}};

/* Only Used Tags defined in enum */
enum MXFLocalTag
{
	MXF_TAG_LTRACK_ID = 0x3006,
	MXF_TAG_TRACK_ID = 0x4801,
	MXF_TAG_TRACK_NUMBER = 0x4804,
	MXF_TAG_EDIT_RATE = 0x4b01,
};

void update_tid_lut(struct MXFContext *ctx, uint32_t track_id, uint8_t *track_number, struct ccx_rational edit_rate)
{
	int i;
	//Update essence element key if we have track Id of caption
	if (ctx->cap_track_id == track_id)
	{
		memcpy(ctx->cap_essence_key, mxf_essence_element_key, 12);
		memcpy(ctx->cap_essence_key + 12, track_number, 4);
		ctx->edit_rate = edit_rate;
	}

	for (i = 0; i < ctx->nb_tracks; i++)
	{
		if (ctx->tracks[i].track_id == track_id &&
		    memcmp(ctx->tracks[i].track_number, track_number, 4))
		{
			memcpy(ctx->tracks[i].track_number, track_number, 4);
			ctx->edit_rate = edit_rate;
			return;
		}
	}
	ctx->tracks[ctx->nb_tracks].track_id = track_id;
	memcpy(ctx->tracks[ctx->nb_tracks].track_number, track_number, 4);
	ctx->edit_rate = edit_rate;
	// Increase number of track only if new track is discovered
	ctx->nb_tracks++;
}

void update_cap_essence_key(struct MXFContext *ctx, uint32_t track_id)
{
	int i;
	for (i = 0; i < ctx->nb_tracks; i++)
	{
		if (ctx->tracks[i].track_id == track_id)
		{
			memcpy(ctx->cap_essence_key, mxf_essence_element_key, 12);
			memcpy(ctx->cap_essence_key + 12, ctx->tracks[i].track_number, 4);
			debug("Key %02X%02X%02X%02X%02X%02X%02X%02X.%02X%02X%02X%02X%02X%02X%02X%02X essence_key\n",
			      ctx->cap_essence_key[0], ctx->cap_essence_key[1], ctx->cap_essence_key[2], ctx->cap_essence_key[3],
			      ctx->cap_essence_key[4], ctx->cap_essence_key[5], ctx->cap_essence_key[5], ctx->cap_essence_key[7],
			      ctx->cap_essence_key[8], ctx->cap_essence_key[9], ctx->cap_essence_key[10], ctx->cap_essence_key[11],
			      ctx->cap_essence_key[12], ctx->cap_essence_key[13], ctx->cap_essence_key[14], ctx->cap_essence_key[15]);
			return;
		}
	}
}
static int mxf_read_header_partition_pack(struct ccx_demuxer *demux, uint64_t size)
{
	int ret;
	int len = 0;
	int i = 0;
	int j = 0;

	uint8_t essence_ul[16];
	uint8_t nb_essence_container;
	struct MXFContext *ctx = demux->private_data;

	if (!ctx)
		return CCX_EINVAL;

	if (size < 88)
	{
		log("Minimum Length of Partition pack is 88 but found %d\n", size);
		return CCX_EINVAL;
	}

	/**
	 * reading major number id code not compatible to other then 001
	 */
	ret = buffered_get_be16(demux);
	if (ret != 0x01)
	{
		log("UnSupported Partition Major number %d\n", ret);
	}
	len += 2;

	ret = buffered_skip(demux, 78);
	if (ret != 78)
		return CCX_EOF;
	demux->past += ret;
	len += ret;

	nb_essence_container = buffered_get_be32(demux);
	len += 4;

	ret = buffered_get_be32(demux);
	if (ret != 16)
		log("Invalid UL length\n");
	len += 4;

	if (size - len < (nb_essence_container * 16))
	{
		log("Partition pack has invalid essense container count(%d) to fit in partition size(%d)\n",
		    nb_essence_container, size);
		return CCX_EINVAL;
	}

	for (i = 0; i < nb_essence_container; i++)
	{
		ret = buffered_read(demux, essence_ul, 16);
		if (ret != 16)
			return CCX_EOF;
		demux->past += 16;
		len += 16;
		for (j = 0; j < sizeof(mxf_caption_essence_container) / sizeof(*mxf_caption_essence_container); j++)
		{
			if (IS_KLV_KEY(essence_ul, mxf_caption_essence_container[j].uid))
			{
				ctx->type = mxf_caption_essence_container[j].type;
			}
		}
	}
	return len;
}

static int mxf_read_timeline_track_metadata(struct ccx_demuxer *demux, uint64_t size)
{
	uint16_t tag, tag_len;
	int len = 0;
	struct MXFContext *ctx = demux->private_data;
	/* Acc B.7 Property shall always be present and non-zero. */
	uint32_t track_id = 0;
	uint8_t track_number[4] = {0};
	struct ccx_rational edit_rate;

	while (len < size)
	{
		tag = buffered_get_be16(demux);
		tag_len = buffered_get_be16(demux);
		len += 4;

		switch (tag)
		{
			case MXF_TAG_TRACK_ID:
				track_id = buffered_get_be32(demux);
				break;
			case MXF_TAG_TRACK_NUMBER:
				buffered_read(demux, track_number, 4);
				demux->past += 4;
				break;
			case MXF_TAG_EDIT_RATE:
				edit_rate.num = buffered_get_be32(demux);
				edit_rate.den = buffered_get_be32(demux);
				break;
			default:
				buffered_skip(demux, tag_len);
				demux->past += tag_len;
		}
		len += tag_len;
	}
	if (track_id != 0)
		update_tid_lut(ctx, track_id, track_number, edit_rate);
	return len;
}

/*
 * Update Track Id of closed caption Track
 *
 */
static int mxf_read_vanc_vbi_desc(struct ccx_demuxer *demux, uint64_t size)
{
	uint16_t tag, tag_len;
	int len = 0;
	int ret;
	struct MXFContext *ctx = demux->private_data;

	while (len < size)
	{
		tag = buffered_get_be16(demux);
		tag_len = buffered_get_be16(demux);
		len += 4;

		switch (tag)
		{
			case MXF_TAG_LTRACK_ID:
				ctx->cap_track_id = buffered_get_be32(demux);
				update_cap_essence_key(ctx, ctx->cap_track_id);
				break;
			default:
				ret = buffered_skip(demux, tag_len);
				if (ret < tag_len)
					break;
				demux->past += tag_len;
		}
		len += tag_len;
	}
	return len;
}

static int mxf_read_cdp_data(struct ccx_demuxer *demux, int size, struct demuxer_data *data)
{
	int len = 0;
	int ret, cc_count;

	ret = buffered_get_be16(demux);
	len += 2;
	if (ret != 0x9669)
	{
		log("Invalid CDP Identifier\n");
		goto error;
	}

	ret = buffered_get_byte(demux);
	len++;
	if (ret != size)
	{
		log("Incomplete CDP packet\n");
		goto error;
	}

	// framerate - top 4 bits are cdp_framing_rate
	ret = buffered_get_byte(demux);
	len++;
	data->tb = framerate_rationals[ret >> 4];

	//skip flag and hdr_seq_cntr
	buffered_skip(demux, 3);
	demux->past += 3;
	len += 3;

	ret = buffered_get_byte(demux);
	len++;
	if (ret != 0x72) // Skip if its not cdata identifier
		goto error;

	cc_count = buffered_get_byte(demux) & 0x1F;
	len++;
	// -4 for cdp footer length
	if ((cc_count * 3) > (size - len - 4))
		log("Incomplete CDP packet\n");

	ret = buffered_read(demux, data->buffer + data->len, cc_count * 3);
	data->len += cc_count * 3;
	demux->past += cc_count * 3;
	len += ret;

error:
	ret = buffered_skip(demux, size - len);
	demux->past += ret;
	len += ret;
	return len;
}

/**
 *  Here is the 16 bytes header before CDP:
 * (bytes) - desc
 * (2) - Number of ANC packets
 * (2) - VANC Line number
 * (1) - Wrapping type (bit 4 is HANC if set, VANC if not).
 *
 *  bits 2-0:
 *
 *      1 == interlaced or spf frame
 *      2 == field 1
 *      3 == field 2
 *      4 == progressive frame
 *
 * (1) - payload sample config
 *
 *      4 == 8 bit luma
 *      5 == 8 bit color difference
 *      6 == 8 bit luma & color difference
 *      7 == 10 bit luma
 *      8 == 10 bit color difference
 *      9 == 10 bit luma & color difference
 *
 *      10 == 8 bit luma with parity
 *      11 == 8 bit color difference with parity
 *      12 == 8 bit luma & color difference with parity
 *
 * (2) - payload sample count (bytes following 16 bytes header)
 * (4) - Data Size with Padding
 * (4) - Always 0x01 ( 0x00 0x00 0x00 0x01)
 *
 * After Header There comes DID and SDID
 * DID 0x61 (did could be 0x80 as well)
 * SDID 0x01 for CEA-708 0x02 for EIA-608 )
 */
static int mxf_read_vanc_data(struct ccx_demuxer *demux, uint64_t size, struct demuxer_data *data)
{
	int len = 0;
	int ret;
	int cdp_size;
	unsigned char vanc_header[16];
	uint8_t DID;
	uint8_t SDID;
	// uint8_t count; /* Currently unused */

	if (size < 19)
		goto error;

	ret = buffered_read(demux, vanc_header, 16);

	demux->past += ret;
	if (ret < 16)
		return CCX_EOF;
	len += ret;

	for (int i = 0; i < vanc_header[1]; i++)
	{
		DID = buffered_get_byte(demux);
		len++;
		if (!(DID == 0x61 || DID == 0x80))
		{
			goto error;
		}

		SDID = buffered_get_byte(demux);
		len++;
		if (SDID == 0x01)
			debug("Caption Type 708\n");
		else if (SDID == 0x02)
			debug("Caption Type 608\n");

		cdp_size = buffered_get_byte(demux);
		if (cdp_size + 19 > size)
		{
			debug("Incomplete cdp(%d) in anc data(%d)\n", cdp_size, size);
			goto error;
		}
		len++;

		ret = mxf_read_cdp_data(demux, cdp_size, data);
		len += ret;
		//len += (3 + count + 4);
	}

error:
	ret = buffered_skip(demux, size - len);
	demux->past += ret;
	len += ret;
	return len;
}

static int mxf_read_essence_element(struct ccx_demuxer *demux, uint64_t size, struct demuxer_data *data)
{
	int ret;
	struct MXFContext *ctx = demux->private_data;

	if (ctx->type == MXF_CT_ANC)
	{
		data->bufferdatatype = CCX_RAW_TYPE;
		ret = mxf_read_vanc_data(demux, size, data);
		data->pts = ctx->cap_count;
		ctx->cap_count++;
	}
	else
	{
		ret = buffered_skip(demux, size);
		demux->past += ret;
	}

	return ret;
}

static const MXFReadTableEntry mxf_read_table[] = {
    /* According to section 7.1 of S377 partition key byte 14 and 15 have variable values */
    {{0x06, 0x0e, 0x2b, 0x34, 0x02, 0x05, 0x01, 0x01, 0x0d, 0x01, 0x02, 0x01, 0x01, 0x02, 0x04, 0x00}, mxf_read_header_partition_pack},
    /* Structural Metadata Sets */
    {{0x06, 0x0e, 0x2b, 0x34, 0x02, 0x53, 0x01, 0x01, 0x0d, 0x01, 0x01, 0x01, 0x01, 0x01, 0x3B, 0x00}, mxf_read_timeline_track_metadata},
    {{0x06, 0x0e, 0x2b, 0x34, 0x02, 0x53, 0x01, 0x01, 0x0d, 0x01, 0x01, 0x01, 0x01, 0x01, 0x5c, 0x00}, mxf_read_vanc_vbi_desc},
    /* Generic Track */
    {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, NULL},
};

static int mxf_read_sync(struct ccx_demuxer *ctx, const uint8_t *key, size_t size)
{
	size_t i;
	unsigned char b;
	int ret;
	for (i = 0; i < size; i++)
	{
		ret = buffered_read_byte(ctx, &b);
		if (ret != 1)
		{
			return CCX_EOF;
		}
		ctx->past++;

		if (b == key[0])
			i = 0;
		else if (b != key[i])
			i = -1;
	}

	return !(i == size);
}

static int64_t klv_decode_ber_length(struct ccx_demuxer *ctx)
{
	uint64_t size = buffered_get_byte(ctx);
	if (size & 0x80)
	{ /* long form */
		int bytes_num = size & 0x7f;
		/* SMPTE 379M 5.3.4 guarantee that bytes_num must not exceed 8 bytes */
		if (bytes_num > 8)
			return -1;
		size = 0;
		while (bytes_num--)
			size = size << 8 | buffered_get_byte(ctx);
	}
	return size;
}

static int klv_read_packet(KLVPacket *klv, struct ccx_demuxer *ctx)
{
	long long result;
	int ret;

	ret = mxf_read_sync(ctx, mxf_klv_key, 4);
	if (ret != 0)
		return ret;

	memcpy(klv->key, mxf_klv_key, 4);
	result = buffered_read(ctx, klv->key + 4, 12);
	ctx->past += result;
	if (result != 12)
		return CCX_EOF;

	klv->length = klv_decode_ber_length(ctx);

	return klv->length == -1 ? -1 : 0;
}

static const MXFReadTableEntry *getMXFReader(UID key)
{
	const MXFReadTableEntry *entry;

	for (entry = mxf_read_table; entry->read; entry++)
	{
		if (IS_KLV_KEY(key, entry->key))
			return entry;
	}
	return NULL;
}

static int read_packet(struct ccx_demuxer *demux, struct demuxer_data *data)
{
	int ret;
	KLVPacket klv;
	const MXFReadTableEntry *reader;
	struct MXFContext *ctx = demux->private_data;
	while ((ret = klv_read_packet(&klv, demux)) == 0)
	{
		debug("Key %02X%02X%02X%02X%02X%02X%02X%02X.%02X%02X%02X%02X%02X%02X%02X%02X size %" PRIu64 "\n",
		      klv.key[0], klv.key[1], klv.key[2], klv.key[3],
		      klv.key[4], klv.key[5], klv.key[5], klv.key[7],
		      klv.key[8], klv.key[9], klv.key[10], klv.key[11],
		      klv.key[12], klv.key[13], klv.key[14], klv.key[15],
		      klv.length);

		if (IS_KLV_KEY(klv.key, ctx->cap_essence_key))
		{
			mxf_read_essence_element(demux, klv.length, data);
			if (data->len > 0)
				break;
			continue;
		}

		reader = getMXFReader(klv.key);
		if (reader == NULL)
		{
			ret = buffered_skip(demux, klv.length);
			demux->past += ret;
			debug("Unknown or Dark key\n");
			continue;
		}

		ret = reader->read(demux, klv.length);
		if (ret < 0)
			break;
	}
	return ret;
}

/**
 * Fills the pointer of data with cc data, will read whole file if no cc data
 * is found.
 */
int ccx_mxf_getmoredata(struct lib_ccx_ctx *ctx, struct demuxer_data **ppdata)
{
	int ret = 0;
	struct demuxer_data *data;

	if (!*ppdata)
	{
		*ppdata = alloc_demuxer_data();
		if (!*ppdata)
			return -1;

		data = *ppdata;
		data->program_number = 1;
		data->stream_pid = 1;
		data->codec = CCX_CODEC_ATSC_CC;
		data->tb.num = 1001;
		data->tb.den = 30000;
	}
	else
	{
		data = *ppdata;
	}

	ret = read_packet(ctx->demux_ctx, data);

	return ret;
}

int ccx_probe_mxf(struct ccx_demuxer *ctx)
{
	int i;
	unsigned char *buf;
	for (i = 0; i + 14 < ctx->startbytes_avail; i++)
	{
		buf = ctx->startbytes + i;
		if (!memcmp(buf, mxf_header_partition_pack_key, 14))
			return CCX_TRUE;
	}
	return CCX_FALSE;
}

struct MXFContext *ccx_mxf_init(struct ccx_demuxer *demux)
{
	struct MXFContext *ctx = NULL;

	ctx = malloc(sizeof(*ctx));
	if (!ctx)
		return NULL;

	memset(ctx, 0, sizeof(struct MXFContext));
	return ctx;
}
