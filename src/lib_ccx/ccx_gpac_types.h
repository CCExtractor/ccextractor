/*
 * Minimal GPAC-compatible type definitions for the FFmpeg MP4 bridge.
 * When ENABLE_FFMPEG_MP4 is defined and GPAC headers may not be available,
 * these provide the subset of GPAC types needed by the C bridge functions.
 */
#ifndef CCX_GPAC_TYPES_H
#define CCX_GPAC_TYPES_H

#include <stdint.h>

#ifndef GPAC_VERSION_MAJOR
/* Only define these if real GPAC headers are not included */

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t s32;

#ifndef GF_4CC
#define GF_4CC(a,b,c,d) ((((u32)a)<<24)|(((u32)b)<<16)|(((u32)c)<<8)|((u32)d))
#endif

/* Minimal GF_AVCConfig - only fields used by process_avc_sample */
typedef struct {
    u8 nal_unit_size;
} GF_AVCConfig_Minimal;

/* Minimal GF_HEVCConfig - only fields used by process_hevc_sample */
typedef struct {
    u8 nal_unit_size;
} GF_HEVCConfig_Minimal;

/* Minimal GF_ISOSample - only fields used by process_*_sample */
typedef struct {
    u32 dataLength;
    char *data;
    u64 DTS;
    u32 CTS_Offset;
} GF_ISOSample_Minimal;

#endif /* GPAC_VERSION_MAJOR */

#endif /* CCX_GPAC_TYPES_H */
