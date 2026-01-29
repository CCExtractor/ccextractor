#include "ttml_isobmff.h"
#include "lib_ccx.h"
#include "utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Detect TTML tracks in ISO-BMFF container
 * According to ISO 14496-30, TTML tracks have:
 * - Handler type: 'subt' 
 * - Sample entry: 'stpp'
 */
int detect_ttml_isobmff_tracks(GF_ISOFile *isom_file, struct lib_ccx_ctx *ctx)
{
    uint32_t track_count;
    uint32_t ttml_track_count = 0;
    
    if (!isom_file) {
        mprint("Error: NULL ISO file handle\n");
        return -1;
    }
    
    track_count = gf_isom_get_track_count(isom_file);
    mprint("Total tracks in file: %u\n", track_count);
    
    for (uint32_t i = 1; i <= track_count; i++) {
        u32 handler_type = gf_isom_get_media_type(isom_file, i);
        u32 subtype = gf_isom_get_media_subtype(isom_file, i, 1);
        
        // Check for TTML track (handler='subt', subtype='stpp')
        if (handler_type == GF_ISOM_HANDLER_TYPE_SUBT && 
            subtype == GF_ISOM_SUBTYPE_STPP) {
            mprint("Found TTML track (ISO-BMFF): Track ID %u\n", i);
            ttml_track_count++;
            
            // Validate this track
            ttml_isobmff_track_t track_info;
            memset(&track_info, 0, sizeof(track_info));
            
            int validation_result = validate_ttml_isobmff_track(isom_file, i, &track_info);
            
            if (validation_result == TTML_VALID) {
                mprint("  ✓ Track %u: VALID TTML structure\n", i);
                mprint("    - Timescale: %u\n", track_info.timescale);
                mprint("    - Samples: %u\n", track_info.sample_count);
                if (track_info.namespace_uri) {
                    mprint("    - Namespace: %s\n", track_info.namespace_uri);
                }
            } else {
                mprint("  ✗ Track %u: VALIDATION FAILED (error code: %d)\n", i, validation_result);
            }
            
            free_ttml_track_info(&track_info);
        }
    }
    
    if (ttml_track_count == 0) {
        mprint("No TTML tracks found in ISO-BMFF container\n");
    } else {
        mprint("Total TTML tracks found: %u\n", ttml_track_count);
    }
    
    return ttml_track_count;
}

/**
 * Validate TTML track structure according to ISO 14496-30
 */
int validate_ttml_isobmff_track(GF_ISOFile *isom_file, uint32_t track_num, ttml_isobmff_track_t *track_info)
{
    if (!isom_file || !track_info) {
        return TTML_INVALID_SAMPLE_STRUCTURE;
    }
    
    // Get track information
    track_info->track_id = gf_isom_get_track_id(isom_file, track_num);
    track_info->timescale = gf_isom_get_media_timescale(isom_file, track_num);
    track_info->sample_count = gf_isom_get_sample_count(isom_file, track_num);
    
    // Validate timescale (must be > 0)
    if (track_info->timescale == 0) {
        mprint("    Error: Invalid timescale (0)\n");
        return TTML_INVALID_TIMING;
    }
    
    // Validate sample count
    if (track_info->sample_count == 0) {
        mprint("    Warning: Track has no samples\n");
    }
    
    // Try to get namespace from sample description
    // In a full implementation, we would parse the stpp box structure
    // For now, we check basic structure validity
    
    // Check if handler type is correct
    u32 handler_type = gf_isom_get_media_type(isom_file, track_num);
    if (handler_type != GF_ISOM_HANDLER_TYPE_SUBT) {
        mprint("    Error: Invalid handler type\n");
        return TTML_MISSING_REQUIRED_BOX;
    }
    
    // Check if sample entry is stpp
    u32 subtype = gf_isom_get_media_subtype(isom_file, track_num, 1);
    if (subtype != GF_ISOM_SUBTYPE_STPP) {
        mprint("    Error: Invalid sample entry (expected 'stpp')\n");
        return TTML_MISSING_REQUIRED_BOX;
    }
    // Set as valid
    track_info->is_valid = 1;
    
    return TTML_VALID;
}

/**
 * Extract TTML content from ISO-BMFF track
 */
int extract_ttml_from_isobmff(GF_ISOFile *isom_file, uint32_t track_num, const char *output_file)
{
    FILE *out_fp = NULL;
    GF_ISOSample *sample = NULL;
    uint32_t sample_count;
    
    if (!isom_file || !output_file) {
        return -1;
    }
    
    sample_count = gf_isom_get_sample_count(isom_file, track_num);
    
    out_fp = fopen(output_file, "wb");
    if (!out_fp) {
        mprint("Error: Cannot open output file %s\n", output_file);
        return -1;
    }
    
    // Write TTML XML header
    fprintf(out_fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(out_fp, "<tt xmlns=\"http://www.w3.org/ns/ttml\">\n");
    fprintf(out_fp, "<body>\n");
    
    // Extract each sample
    for (uint32_t i = 1; i <= sample_count; i++) {
        sample = gf_isom_get_sample(isom_file, track_num, i, NULL);
        
        if (sample && sample->data && sample->dataLength > 0) {
            // Write sample data (TTML XML fragments)
            fwrite(sample->data, 1, sample->dataLength, out_fp);
            fprintf(out_fp, "\n");
        }
        
        if (sample) {
            gf_isom_sample_del(&sample);
        }
    }
    
    fprintf(out_fp, "</body>\n");
    fprintf(out_fp, "</tt>\n");
    
    fclose(out_fp);
    
    mprint("TTML content extracted to: %s\n", output_file);
    
    return 0;
}

/**
 * Free allocated resources
 */
void free_ttml_track_info(ttml_isobmff_track_t *track)
{
    if (track) {
        if (track->namespace_uri) {
            free(track->namespace_uri);
            track->namespace_uri = NULL;
        }
        if (track->schema_location) {
            free(track->schema_location);
            track->schema_location = NULL;
        }
    }
}

/**
 * Process TTML track - main entry point
 */
int process_ttml_track(struct lib_ccx_ctx *ctx, GF_ISOFile *isom_file, uint32_t track_num)
{
    ttml_isobmff_track_t track_info;
    memset(&track_info, 0, sizeof(track_info));
    
    int validation_result = validate_ttml_isobmff_track(isom_file, track_num, &track_info);
    
    if (validation_result != TTML_VALID) {
        mprint("TTML track validation failed\n");
        free_ttml_track_info(&track_info);
        return -1;
    }
    
    // If validation requested, we're done
    // if (ctx->extract_only) {
    //     mprint("TTML track structure is VALID per ISO 14496-30\n");
    // }
    
    free_ttml_track_info(&track_info);
    
    return 0;
}
