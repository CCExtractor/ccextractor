#ifndef CCX_MYTH_H
#define CCX_MYTH_H

#include "ccx_common_platform.h"

// Packet read by mpegps_read_packet() in myth.c. Declared here so the Rust
// port in src/rust/src/mythtv can fill it in.
typedef struct AVPacketMythTV
{
	LLONG pts; ///< presentation time stamp in time_base units
	LLONG dts; ///< decompression time stamp in time_base units
	unsigned char *data;
	int size;
	int stream_index;
	int flags;
	int duration; ///< presentation duration in time_base units (0 if not available)
	void (*destruct)(struct AVPacketMythTV *);
	void *priv;
	LLONG pos; ///< byte position in stream, -1 if unknown
	int codec_id;
	int type;
} AVPacketMythTV;

#endif
