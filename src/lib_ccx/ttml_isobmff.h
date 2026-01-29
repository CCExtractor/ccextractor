#ifndef TTML_ISOBMFF_H
#define TTML_ISOBMFF_H

#include "ccx_common_platform.h"
#include "ccx_common_structs.h"
#include <gpac/isomedia.h>
#include "lib_ccx.h"


// TTML track types according to ISO 14496-30
#define GF_ISOM_SUBTYPE_STPP GF_4CC('s', 't', 'p', 'p')  // TTML in ISO-BMFF
#define GF_ISOM_HANDLER_TYPE_SUBT GF_4CC('s', 'u', 'b', 't') // Subtitle handler

// TTML ISO-BMFF Track Structure
typedef struct {
    uint32_t track_id;
    uint32_t timescale;
    char *namespace_uri;
    char *schema_location;
    uint8_t is_valid;
    uint32_t sample_count;
} ttml_isobmff_track_t;

// Validation error codes
#define TTML_VALID 0
#define TTML_INVALID_NAMESPACE 1
#define TTML_INVALID_TIMING 2
#define TTML_MISSING_REQUIRED_BOX 3
#define TTML_INVALID_SAMPLE_STRUCTURE 4
#define TTML_ERROR_NO_TRACK 5

// Function declarations
int detect_ttml_isobmff_tracks(GF_ISOFile *isom_file, struct lib_ccx_ctx *ctx);
int validate_ttml_isobmff_track(GF_ISOFile *isom_file, uint32_t track_num, ttml_isobmff_track_t *track_info);
int extract_ttml_from_isobmff(GF_ISOFile *isom_file, uint32_t track_num, const char *output_file);
void free_ttml_track_info(ttml_isobmff_track_t *track);
int process_ttml_track(struct lib_ccx_ctx *ctx, GF_ISOFile *isom_file, uint32_t track_num);

#endif // TTML_ISOBMFF_H
