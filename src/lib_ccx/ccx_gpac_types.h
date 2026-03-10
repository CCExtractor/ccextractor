#ifndef CCX_GPAC_TYPES_H
#define CCX_GPAC_TYPES_H

/* GPAC compatible type definitions used when building without GPAC headers */
#include <stdint.h>

typedef uint32_t u32;
typedef int32_t s32;
typedef int64_t s64;
typedef double Double;
typedef uint8_t u8;
typedef uint64_t u64;

typedef struct
{
	char *data;
	uint32_t dataLength;
	uint64_t DTS;
	uint32_t CTS_Offset;
} GF_ISOSample;

typedef struct
{
	uint8_t nal_unit_size;
} GF_AVCConfig;
typedef struct
{
	uint8_t nal_unit_size;
} GF_HEVCConfig;
typedef void GF_ISOFile;
typedef void GF_GenericSampleDescription;

#define GF_4CC(a, b, c, d) ((((u32)(a)) << 24) | (((u32)(b)) << 16) | (((u32)(c)) << 8) | ((u32)(d)))

#ifndef GF_ISOM_SUBTYPE_C708
#define GF_ISOM_SUBTYPE_C708 GF_4CC('c', '7', '0', '8')
#endif

#ifndef GF_QT_SUBTYPE_C608
#define GF_QT_SUBTYPE_C608 GF_4CC('c', '6', '0', '8')
#endif

#endif /* CCX_GPAC_TYPES_H */
