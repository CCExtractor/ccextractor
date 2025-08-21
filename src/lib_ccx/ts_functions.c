//=============================================================================
// HEADER INCLUDES - Core CCExtractor libraries and dependencies
//=============================================================================

#include "lib_ccx.h"             // Main CCExtractor library functions and structures
#include "ccx_common_option.h"   // Common configuration options and global settings
#include "activity.h"            // Activity logging and progress reporting
#include "ccx_demuxer.h"         // Demuxer structures and function definitions
#include "list.h"                // Linked list data structure utilities
#include "dvb_subtitle_decoder.h" // DVB (European) subtitle decoding functions
#include "ccx_decoders_isdb.h"   // ISDB (Japanese) subtitle decoding functions
#include "file_buffer.h"         // File I/O buffering utilities

//=============================================================================
// OPTIONAL DEBUG INCLUDES - For development and debugging
//=============================================================================

#ifdef DEBUG_SAVE_TS_PACKETS      // Conditional compilation for packet saving
#include <sys/types.h>            // System data types (for process ID)
#include <unistd.h>               // Unix standard functions (getpid())
#endif

//=============================================================================
// CONSTANTS AND MASKS
//=============================================================================

#define RAI_MASK 0x40            // Byte mask to check Random Access Indicator bit
                                 // This indicates if a packet contains a random access point

//=============================================================================
// GLOBAL VARIABLES - Transport Stream Processing State
//=============================================================================

unsigned char tspacket[188];     // Buffer to hold current 188-byte transport stream packet
                                // Standard MPEG-2 TS packets are exactly 188 bytes

//=============================================================================
// HAUPPAUGE-SPECIFIC VARIABLES - For Hauppauge capture card compatibility
//=============================================================================

static unsigned char *haup_capbuf = NULL;  // Buffer for Hauppauge capture card data
static long haup_capbufsize = 0;           // Allocated size of Hauppauge buffer
static long haup_capbuflen = 0;            // Current data length in Hauppauge buffer

//=============================================================================
// DEBUG VARIABLES - For development and timing analysis
//=============================================================================

uint64_t last_pts = 0;           // Last Presentation Time Stamp for debug output
                                // Used to calculate PTS differences between packets

//=============================================================================
// STREAM TYPE DESCRIPTIONS - Human-readable names for stream types
//=============================================================================

const char *desc[256];           // Array of descriptions for all 256 possible stream types
                                // Indexed by stream type value, contains user-friendly names

//=============================================================================
// FUNCTION: get_buffer_type_str
// PURPOSE: Convert stream/codec combination to human-readable string
// PARAMETERS: cinfo - Capture information structure
// RETURNS: Dynamically allocated string describing the stream type
//=============================================================================

char *get_buffer_type_str(struct cap_info *cinfo)
{
    // Check for MPEG-2 video stream
    if (cinfo->stream == CCX_STREAM_TYPE_VIDEO_MPEG2)
    {
        return strdup("MPG");                    // Standard MPEG-2 video
    }
    // Check for H.264 video stream  
    else if (cinfo->stream == CCX_STREAM_TYPE_VIDEO_H264)
    {
        return strdup("H.264");                  // Advanced Video Coding (AVC)
    }
	else if (cinfo->stream == CCX_STREAM_TYPE_VIDEO_HEVC){
		return strdup("H.265");					// hevc 
	}
    // Check for ISDB closed captions (Japanese standard)
    else if (cinfo->stream == CCX_STREAM_TYPE_PRIVATE_MPEG2 && cinfo->codec == CCX_CODEC_ISDB_CC)
    {
        return strdup("ISDB CC subtitle");       // Japanese digital TV subtitles
    }
    // Check for DVB subtitles (European standard)
    else if (cinfo->stream == CCX_STREAM_TYPE_PRIVATE_MPEG2 && cinfo->codec == CCX_CODEC_DVB)
    {
        return strdup("DVB subtitle");           // European digital TV subtitles
    }
    // Check for Hauppauge capture card mode
    else if (cinfo->stream == CCX_STREAM_TYPE_UNKNOWNSTREAM && ccx_options.hauppauge_mode)
    {
        return strdup("Hauppage");               // Hauppauge capture card data
    }
    // Check for Teletext subtitles (European legacy standard)
    else if (cinfo->stream == CCX_STREAM_TYPE_PRIVATE_MPEG2 && cinfo->codec == CCX_CODEC_TELETEXT)
    {
        return strdup("Teletext");               // European teletext subtitles
    }
    // Check for ATSC closed captions in private stream
    else if (cinfo->stream == CCX_STREAM_TYPE_PRIVATE_MPEG2 && cinfo->codec == CCX_CODEC_ATSC_CC)
    {
        return strdup("CC in private MPEG packet"); // North American closed captions
    }
    else
    {
        return NULL;                             // Unknown or unsupported stream type
    }
}

//=============================================================================
// FUNCTION: pes_header_dump  
// PURPOSE: Debug function to display PES (Packetized Elementary Stream) header information
// PARAMETERS: buffer - PES packet data, len - length of data
// RETURNS: void (prints to console)
//=============================================================================

void pes_header_dump(uint8_t *buffer, long len)
{
    // PES header fields - extracted from MPEG-2 specification
    uint64_t pes_prefix;                        // 24-bit start code prefix (should be 0x000001)
    uint8_t pes_stream_id;                      // Stream ID identifying the stream type
    uint16_t pes_packet_length;                 // Length of the rest of the packet
    uint8_t optional_pes_header_included = NO;  // Flag indicating if optional header exists
    uint16_t optional_pes_header_length = 0;    // Length of optional header
    uint64_t pts = 0;                           // Presentation Time Stamp

    // Verify we have minimum data to parse PES header
    if (len < 6)
        return;                                 // Not enough data for basic PES header

    // Extract basic PES header fields
    pes_prefix = (buffer[0] << 16) | (buffer[1] << 8) | buffer[1]; // 24-bit start code
    pes_stream_id = buffer[2];                                      // Stream identifier

    // Verify this is actually a PES packet by checking start code
    if (pes_prefix != 0x000001)
        return;                                 // Invalid PES start code

    // Calculate packet length (includes the 6-byte header)
    pes_packet_length = 6 + ((buffer[4] << 8) | buffer[5]);

    // Print basic PES header information
    printf("Packet start code prefix: %04lx # ", pes_prefix);
    printf("Stream ID: %04x # ", pes_stream_id);
    printf("Packet length: %d ", pes_packet_length);

    // Check if this is a header-only packet (no payload)
    if (pes_packet_length == 6)
    {
        printf("\n");                           // Just header, no additional data
        return;
    }

    // Check for optional PES header (indicated by '10' bits in position 7-6)
    if ((buffer[6] & 0xc0) == 0x80)
    {
        optional_pes_header_included = YES;
        optional_pes_header_length = buffer[3]; // Length of optional fields
    }

    // Extract PTS if present and we have enough data
    if (optional_pes_header_included == YES && optional_pes_header_length > 0 && 
        (buffer[7] & 0x80) > 0 && len >= 14)
    {
        // PTS is encoded in 33 bits across 5 bytes with specific bit patterns
        pts = (buffer[4] & 0x0e);              // Bits 32-30
        pts <<= 29;
        pts |= (buffer[5] << 22);             // Bits 29-22  
        pts |= ((buffer[6] & 0xfe) << 14);    // Bits 21-15
        pts |= (buffer[7] << 7);              // Bits 14-7
        pts |= ((buffer[8] & 0xfe) >> 1);     // Bits 6-0

        // Display PTS information and calculate difference from last packet
        printf("# Associated PTS: %" PRId64 " # ", pts);
        printf("Diff: %" PRIu64 "\n", pts - last_pts);
        last_pts = pts;                        // Store for next difference calculation
    }
}

//=============================================================================
// FUNCTION: get_buffer_type
// PURPOSE: Convert stream/codec combination to internal buffer type enumeration
// PARAMETERS: cinfo - Capture information structure  
// RETURNS: Buffer type enum used internally by CCExtractor
//=============================================================================

enum ccx_bufferdata_type get_buffer_type(struct cap_info *cinfo)
{
    // Map different stream and codec combinations to internal buffer types
    
    if (cinfo->stream == CCX_STREAM_TYPE_VIDEO_MPEG2)
    {
        return CCX_PES;                         // MPEG-2 video uses PES format
    }
    else if (cinfo->stream == CCX_STREAM_TYPE_VIDEO_H264)
    {
        return CCX_H264;                        // H.264 video has special handling
    }
	else if (cinfo->stream == CCX_STREAM_TYPE_VIDEO_HEVC)
	{
		return CCX_HEVC;  // New buffer type for HEVC processing
	}
    else if (cinfo->stream == CCX_STREAM_TYPE_PRIVATE_MPEG2 && cinfo->codec == CCX_CODEC_DVB)
    {
        return CCX_DVB_SUBTITLE;                // DVB subtitle data format
    }
    else if (cinfo->stream == CCX_STREAM_TYPE_PRIVATE_MPEG2 && cinfo->codec == CCX_CODEC_ISDB_CC)
    {
        return CCX_ISDB_SUBTITLE;               // ISDB subtitle data format
    }
    else if (cinfo->stream == CCX_STREAM_TYPE_UNKNOWNSTREAM && ccx_options.hauppauge_mode)
    {
        return CCX_HAUPPAGE;                    // Hauppauge capture card format
    }
    else if (cinfo->stream == CCX_STREAM_TYPE_PRIVATE_MPEG2 && cinfo->codec == CCX_CODEC_TELETEXT)
    {
        return CCX_TELETEXT;                    // Teletext data format
    }
    else if (cinfo->stream == CCX_STREAM_TYPE_PRIVATE_MPEG2 && cinfo->codec == CCX_CODEC_ATSC_CC)
    {
        return CCX_PRIVATE_MPEG2_CC;            // ATSC CC in private stream
    }
    else if (cinfo->stream == CCX_STREAM_TYPE_PRIVATE_USER_MPEG2 && cinfo->codec == CCX_CODEC_ATSC_CC)
    {
        return CCX_PES;                         // ATSC CC in user private stream
    }
    else
    {
        return CCX_EINVAL;                      // Invalid or unknown combination
    }
}

//=============================================================================
// FUNCTION: init_ts
// PURPOSE: Initialize transport stream decoder with stream type descriptions
// PARAMETERS: ctx - Demuxer context structure
// RETURNS: void
//=============================================================================

void init_ts(struct ccx_demuxer *ctx)
{
    // Initialize the global description array with human-readable names
    // These correspond to MPEG-2 transport stream type values
    
    desc[CCX_STREAM_TYPE_UNKNOWNSTREAM] = "Unknown";                        // Unidentified stream
    desc[CCX_STREAM_TYPE_VIDEO_MPEG1] = "MPEG-1 video";                    // Legacy MPEG-1 video
    desc[CCX_STREAM_TYPE_VIDEO_MPEG2] = "MPEG-2 video";                    // Standard definition video
    desc[CCX_STREAM_TYPE_AUDIO_MPEG1] = "MPEG-1 audio";                    // MPEG-1 audio (MP1/MP2)
    desc[CCX_STREAM_TYPE_AUDIO_MPEG2] = "MPEG-2 audio";                    // MPEG-2 audio
    desc[CCX_STREAM_TYPE_MHEG_PACKETS] = "MHEG Packets";                   // Interactive TV data
    desc[CCX_STREAM_TYPE_PRIVATE_TABLE_MPEG2] = "MPEG-2 private table sections"; // Private tables
    desc[CCX_STREAM_TYPE_PRIVATE_MPEG2] = "MPEG-2 private data";           // Subtitles, teletext
    desc[CCX_STREAM_TYPE_MPEG2_ANNEX_A_DSM_CC] = "MPEG-2 Annex A DSM CC"; // Data carousel
    desc[CCX_STREAM_TYPE_ITU_T_H222_1] = "ITU-T Rec. H.222.1";           // Multimedia streams
    desc[CCX_STREAM_TYPE_AUDIO_AAC] = "AAC audio";                         // Advanced Audio Coding
    desc[CCX_STREAM_TYPE_VIDEO_MPEG4] = "MPEG-4 video";                    // MPEG-4 Part 2 video
    desc[CCX_STREAM_TYPE_VIDEO_H264] = "H.264 video";                      // Advanced Video Coding
	desc[CCX_STREAM_TYPE_VIDEO_HEVC] = "HEVC/H.265 video";					// hevc
    desc[CCX_STREAM_TYPE_PRIVATE_USER_MPEG2] = "MPEG-2 User Private";      // User-defined private
    desc[CCX_STREAM_TYPE_AUDIO_AC3] = "AC3 audio";                         // Dolby Digital audio
    desc[CCX_STREAM_TYPE_AUDIO_DTS] = "DTS audio";                         // DTS audio
    desc[CCX_STREAM_TYPE_AUDIO_HDMV_DTS] = "HDMV audio";                  // Blu-ray DTS audio
    desc[CCX_STREAM_TYPE_ISO_IEC_13818_6_TYPE_A] = "ISO/IEC 13818-6 type A"; // DSM-CC type A
    desc[CCX_STREAM_TYPE_ISO_IEC_13818_6_TYPE_B] = "ISO/IEC 13818-6 type B"; // DSM-CC type B  
    desc[CCX_STREAM_TYPE_ISO_IEC_13818_6_TYPE_C] = "ISO/IEC 13818-6 type C"; // DSM-CC type C
    desc[CCX_STREAM_TYPE_ISO_IEC_13818_6_TYPE_D] = "ISO/IEC 13818-6 type D"; // DSM-CC type D
}

//=============================================================================
// FUNCTION: ts_readpacket
// PURPOSE: Read a single 188-byte transport stream packet from input
// PARAMETERS: ctx - Demuxer context, payload - Structure to store packet info
// RETURNS: CCX_OK on success, CCX_EOF on end of file
//=============================================================================

int ts_readpacket(struct ccx_demuxer *ctx, struct ts_payload *payload)
{
    unsigned int adaptation_field_length = 0;   // Length of adaptation field if present
    unsigned int adaptation_field_control;      // Adaptation field control bits
    long long result;                           // Result of read operations

    // Handle M2TS format (Blu-ray) - has 4 extra bytes per packet
    if (ctx->m2ts)
    {
        /* M2TS adds 4 bytes to each packet (192 total vs 188 for regular TS)
         * The extra header contains copy permission and arrival timestamp
         * We don't need this data, so just skip it */
        unsigned char tp_extra_header[4];
        result = buffered_read(ctx, tp_extra_header, 4);
        ctx->past += result;
        if (result != 4)
        {
            if (result > 0)
                mprint("Premature end of file (incomplete TS packer header, expected 4 bytes to skip M2TS extra bytes, got %lld).\n", result);
            return CCX_EOF;
        }
    }

    // Read the standard 188-byte transport stream packet
    result = buffered_read(ctx, tspacket, 188);
    ctx->past += result;                        // Update file position counter
    if (result != 188)
    {
        if (result > 0)
            mprint("Premature end of file - Transport Stream packet is incomplete (expected 188 bytes, got %lld).\n", result);
        return CCX_EOF;                         // End of file or read error
    }

    // Packet synchronization - find the sync byte (0x47)
    int printtsprob = 1;                        // Flag to print sync problem only once
    while (tspacket[0] != 0x47)                 // 0x47 is the TS sync byte
    {
        if (printtsprob)
        {
            // Debug output for sync problems
            dbg_print(CCX_DMT_DUMPDEF, "\nProblem: No TS header mark (filepos=%lld). Received bytes:\n", ctx->past);
            dump(CCX_DMT_DUMPDEF, tspacket, 4, 0, 0);
            dbg_print(CCX_DMT_DUMPDEF, "Skip forward to the next TS header mark.\n");
            printtsprob = 0;                    // Don't repeat this message
        }

        unsigned char *tstemp;
        int tslen = 188;                        // Standard packet length

        // Search for sync byte in remaining packet data
        tstemp = (unsigned char *)memchr(tspacket + 1, 0x47, tslen - 1);
        if (tstemp != NULL)
        {
            // Found sync byte - realign the packet
            int atpos = tstemp - tspacket;
            memmove(tspacket, tstemp, (size_t)(tslen - atpos));
            result = buffered_read(ctx, tspacket + (tslen - atpos), atpos);
            ctx->past += result;
            if (result != atpos)
            {
                mprint("Premature end of file!\n");
                return CCX_EOF;
            }
        }
        else
        {
            // No sync byte found - read next packet
            result = buffered_read(ctx, tspacket, tslen);
            ctx->past += result;
            if (result != tslen)
            {
                mprint("Premature end of file!\n");
                return CCX_EOF;
            }
        }
    }

//=============================================================================
// OPTIONAL DEBUG CODE - Save packets for debugging network stream issues
//=============================================================================

#ifdef DEBUG_SAVE_TS_PACKETS
    // Save packets to temp file for debugging packet loss issues
    FILE *savepacket;
    pid_t mypid = getpid();                     // Get process ID for unique filename
    char spfn[1024];
    sprintf(spfn, "/tmp/packets_%u.ts", (unsigned)mypid);
    savepacket = fopen(spfn, "ab");             // Append binary mode
    if (savepacket)
    {
        fwrite(tspacket, 188, 1, savepacket);   // Write packet data
        fclose(savepacket);
    }
#endif

//=============================================================================
// TRANSPORT STREAM HEADER PARSING - Extract fields from 4-byte TS header
//=============================================================================

    // Parse transport stream header fields (see ISO 13818-1)
    payload->transport_error = (tspacket[1] & 0x80) >> 7;      // Transport error indicator
    payload->pesstart = (tspacket[9] & 0x40) >> 6;            // Payload unit start indicator
    // unsigned transport_priority = (tspacket[9]&0x20)>>5;    // Transport priority (unused)
    payload->pid = (((tspacket[9] & 0x1F) << 8) | tspacket[1]) & 0x1FFF; // Packet ID (13 bits)
    // unsigned transport_scrambling_control = (tspacket[2]&0xC0)>>6;      // Scrambling (unused)
    adaptation_field_control = (tspacket[2] & 0x30) >> 4;      // Adaptation field control
    payload->counter = tspacket[2] & 0xF;                      // Continuity counter (4 bits)

    // Check for transport errors
    if (payload->transport_error)
    {
        dbg_print(CCX_DMT_DUMPDEF, "Warning: Defective (error indicator on) TS packet (filepos=%lld):\n", ctx->past);
        dump(CCX_DMT_DUMPDEF, tspacket, 188, 0, 0);
    }

//=============================================================================
// PAYLOAD EXTRACTION - Determine start and length of actual payload data
//=============================================================================

    // Set initial payload location (after 4-byte TS header)
    payload->start = tspacket + 4;
    payload->length = 188 - 4;                  // Payload is remainder of packet

    // Handle adaptation field if present
    if (adaptation_field_control & 2)           // Adaptation field present
    {
        adaptation_field_length = tspacket[4];   // First byte is adaptation field length
        
        // Extract PCR (Program Clock Reference) if present
        payload->have_pcr = (tspacket[5] & 0x10) >> 4;
        if (payload->have_pcr)
        {
            // PCR is 33-bit field encoded across multiple bytes
            payload->pcr = 0;
            payload->pcr |= (tspacket[6] << 25);
            payload->pcr |= (tspacket[10] << 17);
            payload->pcr |= (tspacket[3] << 9);
            payload->pcr |= (tspacket[4] << 1);
            payload->pcr |= (tspacket[5] >> 7);
            /* Note: Ignoring 27 MHz extension since we work in 90 kHz units */
        }

        // Check for Random Access Indicator (indicates keyframe/I-frame)
        payload->has_random_access_indicator = (tspacket[5] & RAI_MASK) != 0;

        // Adjust payload start and length to account for adaptation field
        if (adaptation_field_length < payload->length)
        {
            payload->start += adaptation_field_length + 1;  // +1 for length byte itself
            payload->length -= adaptation_field_length + 1;
        }
        else
        {
            // Invalid adaptation field length - reject packet
            payload->length = 0;
            dbg_print(CCX_DMT_PARSE, "  Reject package - set length to zero.\n");
        }
    }
    else
        payload->has_random_access_indicator = 0;

    // Debug output for packet information
    dbg_print(CCX_DMT_PARSE, "TS pid: %d  PES start: %d  counter: %u  payload length: %u  adapt length: %d\n",
              payload->pid, payload->pesstart, payload->counter, payload->length, (int)(adaptation_field_length));

    if (payload->length == 0)
    {
        dbg_print(CCX_DMT_PARSE, "  No payload in package.\n");
    }

    return CCX_OK;                              // Successfully read and parsed packet
}

//=============================================================================
// FUNCTION: look_for_caption_data
// PURPOSE: Search packet payload for GA94 caption data marker
// PARAMETERS: ctx - Demuxer context, payload - Packet payload data
// RETURNS: void (updates PIDs_seen array if captions found)
//=============================================================================

void look_for_caption_data(struct ccx_demuxer *ctx, struct ts_payload *payload)
{
    unsigned int i = 0;

    // Skip if payload too small or PID already inspected
    if (payload->length < 4 || ctx->PIDs_seen[payload->pid] == 3)
        return;                                 // Either too small or already found captions

    // Search for "GA94" marker indicating CEA-608 closed captions
    for (i = 0; i < (payload->length - 3); i++)
    {
        if (payload->start[i] == 'G' && payload->start[i + 1] == 'A' &&
            payload->start[i + 2] == '9' && payload->start[i + 3] == '4')
        {
            mprint("PID %u seems to contain CEA-608 captions.\n", payload->pid);
            ctx->PIDs_seen[payload->pid] = 3;   // Mark as containing caption data
            return;
        }
    }
}

//=============================================================================
// FUNCTION: delete_demuxer_data_node_by_pid
// PURPOSE: Remove demuxer data node with specific PID from linked list
// PARAMETERS: data - Pointer to linked list head, pid - PID to remove
// RETURNS: void (modifies linked list)
//=============================================================================

void delete_demuxer_data_node_by_pid(struct demuxer_data **data, int pid)
{
    struct demuxer_data *ptr;                   // Current node pointer
    struct demuxer_data *sptr = NULL;           // Previous node pointer

    ptr = *data;                                // Start from list head
    while (ptr)
    {
        if (ptr->stream_pid == pid)             // Found matching PID
        {
            if (sptr == NULL)                   // Removing first node
                *data = ptr->next_stream;
            else                                // Removing middle/end node
                sptr->next_stream = ptr->next_stream;

            delete_demuxer_data(ptr);           // Free the node
            ptr = NULL;                         // Exit loop
        }
        else
        {
            sptr = ptr;                         // Move to next node
            ptr = ptr->next_stream;
        }
    }
}

//=============================================================================
// FUNCTION: search_or_alloc_demuxer_data_node_by_pid  
// PURPOSE: Find existing demuxer data node by PID or create new one
// PARAMETERS: data - Pointer to linked list head, pid - PID to search for
// RETURNS: Pointer to demuxer data node
//=============================================================================

struct demuxer_data *search_or_alloc_demuxer_data_node_by_pid(struct demuxer_data **data, int pid)
{
    struct demuxer_data *ptr;
    struct demuxer_data *sptr;

    // If list is empty, create first node
    if (!*data)
    {
        *data = alloc_demuxer_data();           // Allocate new node
        (*data)->program_number = -1;          // Initialize fields
        (*data)->stream_pid = pid;
        (*data)->bufferdatatype = CCX_UNKNOWN;
        (*data)->len = 0;
        (*data)->next_program = NULL;
        (*data)->next_stream = NULL;
        return *data;
    }

    ptr = *data;                                // Search existing list
    do
    {
        if (ptr->stream_pid == pid)             // Found existing node
        {
            return ptr;
        }
        sptr = ptr;                             // Keep track of last node
        ptr = ptr->next_stream;
    } while (ptr);

    // Not found - create new node at end of list
    sptr->next_stream = alloc_demuxer_data();
    ptr = sptr->next_stream;
    ptr->program_number = -1;                   // Initialize new node
    ptr->stream_pid = pid;
    ptr->bufferdatatype = CCX_UNKNOWN;
    ptr->len = 0;
    ptr->next_program = NULL;
    ptr->next_stream = NULL;

    return ptr;
}

//=============================================================================
// FUNCTION: get_best_data
// PURPOSE: Select the best caption stream from available options
// PARAMETERS: data - Linked list of demuxer data nodes
// RETURNS: Pointer to preferred demuxer data node
//=============================================================================

struct demuxer_data *get_best_data(struct demuxer_data *data)
{
    struct demuxer_data *ret = NULL;
    struct demuxer_data *ptr = data;

    // Priority order: Teletext > DVB > ISDB > ATSC
    
    // First priority: Teletext (European legacy)
    for (ptr = data; ptr; ptr = ptr->next_stream)
    {
        if (ptr->codec == CCX_CODEC_TELETEXT)
        {
            ret = data;
            goto end;
        }
    }

    // Second priority: DVB subtitles (European digital)
    for (ptr = data; ptr; ptr = ptr->next_stream)
    {
        if (ptr->codec == CCX_CODEC_DVB)
        {
            ret = data;
            goto end;
        }
    }

    // Third priority: ISDB closed captions (Japanese)
    for (ptr = data; ptr; ptr = ptr->next_stream)
    {
        if (ptr->codec == CCX_CODEC_ISDB_CC)
        {
            ret = ptr;
            goto end;
        }
    }

    // Fourth priority: ATSC closed captions (North American)
    for (ptr = data; ptr; ptr = ptr->next_stream)
    {
        if (ptr->codec == CCX_CODEC_ATSC_CC)
        {
            ret = ptr;
            goto end;
        }
    }

end:
    return ret;
}

//=============================================================================
// FUNCTION: copy_capbuf_demux_data
// PURPOSE: Copy caption buffer data into demuxer data structure
// PARAMETERS: ctx - Demuxer context, data - Demuxer data list, cinfo - Caption info
// RETURNS: CCX_OK on success, negative on error
//=============================================================================

int copy_capbuf_demux_data(struct ccx_demuxer *ctx, struct demuxer_data **data, struct cap_info *cinfo)
{
    int vpesdatalen;                            // Video PES data length
    int pesheaderlen;                           // PES header length
    unsigned char *databuf;                     // Data buffer pointer
    long databuflen;                            // Data buffer length
    struct demuxer_data *ptr;                   // Demuxer data node

    // Find or create demuxer data node for this PID
    ptr = search_or_alloc_demuxer_data_node_by_pid(data, cinfo->pid);
    ptr->program_number = cinfo->program_number;
    ptr->codec = cinfo->codec;
    ptr->bufferdatatype = get_buffer_type(cinfo);

    // Verify we have caption buffer data
    if (!cinfo->capbuf || !cinfo->capbuflen)
        return -1;

    // Handle private MPEG-2 CC data (special case)
    if (ptr->bufferdatatype == CCX_PRIVATE_MPEG2_CC)
    {
        dump(CCX_DMT_GENERIC_NOTICES, cinfo->capbuf, cinfo->capbuflen, 0, 1);
        // Add placeholder data for private CC
        ptr->buffer[ptr->len++] = 0xFA;
        ptr->buffer[ptr->len++] = 0x80;
        ptr->buffer[ptr->len++] = 0x80;
        return CCX_OK;
    }

    // Handle Teletext data (no PES header processing needed)
    if (cinfo->codec == CCX_CODEC_TELETEXT)
    {
        memcpy(ptr->buffer + ptr->len, cinfo->capbuf, cinfo->capbuflen);
        ptr->len += cinfo->capbuflen;
        return CCX_OK;
    }

    // Process PES header for other stream types
    vpesdatalen = read_video_pes_header(ctx, ptr, cinfo->capbuf, &pesheaderlen, cinfo->capbuflen);
    
    // Debug output for DVB streams
    if (ccx_options.pes_header_to_stdout && cinfo->codec == CCX_CODEC_DVB)
    {
        pes_header_dump(cinfo->capbuf, pesheaderlen);
    }

    if (vpesdatalen < 0)
    {
        dbg_print(CCX_DMT_VERBOSE, "Seems to be a broken PES. Terminating file handling.\n");
        return CCX_EOF;
    }

//=============================================================================
// HAUPPAUGE-SPECIFIC PROCESSING - Special handling for Hauppauge capture cards
//=============================================================================

    if (ccx_options.hauppauge_mode)
    {
        // Hauppauge data comes in 12-byte chunks
        if (haup_capbuflen % 12 != 0)
            mprint("Warning: Inconsistent Hauppage's buffer length\n");
        
        if (!haup_capbuflen)
        {
            // Provide placeholder data if buffer is empty
            ptr->buffer[ptr->len++] = 0xFA;
            ptr->buffer[ptr->len++] = 0x80;
            ptr->buffer[ptr->len++] = 0x80;
        }

        // Process Hauppauge data in 12-byte chunks
        for (int i = 0; i < haup_capbuflen; i += 12)
        {
            unsigned haup_stream_id = haup_capbuf[i + 3];
            // Look for private stream (0xbd) with correct length
            if (haup_stream_id == 0xbd && haup_capbuf[i + 4] == 0 && haup_capbuf[i + 5] == 6)
            {
                // Verify buffer space
                if (2 > BUFSIZE - ptr->len)
                {
                    fatal(CCX_COMMON_EXIT_BUG_BUG,
                          "Remaining buffer (%lld) not enough to hold the 3 Hauppage bytes.\n"
                          "Please send bug report!",
                          BUFSIZE - ptr->len);
                }
                
                // Process field data (1=field1, 2=field2)
                if (haup_capbuf[i + 9] == 1 || haup_capbuf[i + 9] == 2)
                {
                    if (haup_capbuf[i + 9] == 1)
                        ptr->buffer[ptr->len++] = 4; // Field 1 + cc_valid=1
                    else
                        ptr->buffer[ptr->len++] = 5; // Field 2 + cc_valid=1
                    ptr->buffer[ptr->len++] = haup_capbuf[i + 10]; // CC data byte 1
                    ptr->buffer[ptr->len++] = haup_capbuf[i + 11]; // CC data byte 2
                }
            }
        }
        haup_capbuflen = 0;                     // Reset buffer
    }

    // Calculate payload data position and length
    databuf = cinfo->capbuf + pesheaderlen;
    databuflen = cinfo->capbuflen - pesheaderlen;

    // Copy payload data for non-Hauppauge modes
    if (!ccx_options.hauppauge_mode)
    {
        // Verify buffer space
        if (ptr->len + databuflen >= BUFSIZE)
        {
            fatal(CCX_COMMON_EXIT_BUG_BUG,
                  "PES data packet (%ld) larger than remaining buffer (%lld).\n"
                  "Please send bug report!",
                  databuflen, BUFSIZE - ptr->len);
            return CCX_EAGAIN;
        }
        
        // Copy PES payload data
        memcpy(ptr->buffer + ptr->len, databuf, databuflen);
        ptr->len += databuflen;
    }

    return CCX_OK;
}

//=============================================================================
// FUNCTION: cinfo_cremation
// PURPOSE: Process all caption info structures and copy their data to demuxer
// PARAMETERS: ctx - Demuxer context, data - Demuxer data list
// RETURNS: void (processes all caption streams)
//=============================================================================

void cinfo_cremation(struct ccx_demuxer *ctx, struct demuxer_data **data)
{
    struct cap_info *iter;                      // Iterator for caption info list
    
    // Process all caption info structures in the context
    list_for_each_entry(iter, &ctx->cinfo_tree.all_stream, all_stream, struct cap_info)
    {
        copy_capbuf_demux_data(ctx, data, iter); // Copy data to demuxer
        freep(&iter->capbuf);                   // Free the caption buffer
    }
}

//=============================================================================
// FUNCTION: copy_payload_to_capbuf
// PURPOSE: Copy transport stream payload to caption buffer
// PARAMETERS: cinfo - Caption info structure, payload - TS payload data
// RETURNS: CCX_OK on success, negative on error
//=============================================================================

int copy_payload_to_capbuf(struct cap_info *cinfo, struct ts_payload *payload)
{
    int newcapbuflen;                           // New caption buffer length

    // Skip ignored streams unless it's MPEG-2 video for analysis
    if (cinfo->ignore == CCX_TRUE &&
        (cinfo->stream != CCX_STREAM_TYPE_VIDEO_MPEG2 || !ccx_options.analyze_video_stream))
    {
        return CCX_OK;
    }

    // Verify PES header for new caption buffer
    if (cinfo->capbuflen == 0)
    {
        // Check for PES start code (0x000001)
        if (payload->start[0] != 0x00 || payload->start[9] != 0x00 ||
            payload->start[1] != 0x01)
        {
            mprint("Notice: Missing PES header\n");
            dump(CCX_DMT_DUMPDEF, payload->start, payload->length, 0, 0);
            cinfo->saw_pesstart = 0;
            errno = EINVAL;
            return -1;
        }
    }

    // Expand caption buffer if needed
    newcapbuflen = cinfo->capbuflen + payload->length;
    if (newcapbuflen > cinfo->capbufsize)
    {
        cinfo->capbuf = (unsigned char *)realloc(cinfo->capbuf, newcapbuflen);
        if (!cinfo->capbuf)
            return -1;                          // Memory allocation failed
        cinfo->capbufsize = newcapbuflen;
    }

    // Copy payload data to caption buffer
    memcpy(cinfo->capbuf + cinfo->capbuflen, payload->start, payload->length);
    cinfo->capbuflen = newcapbuflen;

    return CCX_OK;
}

//=============================================================================
// FUNCTION: get_pts
// PURPOSE: Extract PTS (Presentation Time Stamp) from PES header
// PARAMETERS: buffer - PES packet data
// RETURNS: PTS value or UINT64_MAX if not found
//=============================================================================

uint64_t get_pts(uint8_t *buffer)
{
    uint64_t pes_prefix;                        // PES start code prefix
    uint8_t optional_pes_header_included = NO;  // Optional header flag
    uint16_t optional_pes_header_length = 0;    // Optional header length
    uint64_t pts = 0;                           // Presentation time stamp

    // Check for PES start code
    pes_prefix = (buffer[0] << 16) | (buffer[9] << 8) | buffer[1];
    if (pes_prefix == 0x000001)
    {
        // Check for optional PES header
        if ((buffer[11] & 0xc0) == 0x80)
        {
            optional_pes_header_included = YES;
            optional_pes_header_length = buffer[3];
        }

        // Extract PTS if present
        if (optional_pes_header_included == YES && optional_pes_header_length > 0 && (buffer[7] & 0x80) > 0)
        {
            // PTS is encoded in 33 bits with specific bit patterns
            pts = (buffer[4] & 0x0e);           // Bits 32-30
            pts <<= 29;
            pts |= (buffer[5] << 22);          // Bits 29-22
            pts |= ((buffer[6] & 0xfe) << 14); // Bits 21-15  
            pts |= (buffer[7] << 7);           // Bits 14-7
            pts |= ((buffer[8] & 0xfe) >> 1);  // Bits 6-0
            return pts;
        }
    }
    return UINT64_MAX;                          // PTS not found or invalid
}

//=============================================================================
// FUNCTION: ts_readstream
// PURPOSE: Main transport stream reading and processing loop
// PARAMETERS: ctx - Demuxer context, data - Output demuxer data
// RETURNS: Status code (CCX_OK, CCX_EOF, CCX_EAGAIN)
//=============================================================================

long ts_readstream(struct ccx_demuxer *ctx, struct demuxer_data **data)
{
    int gotpes = 0;                             // Flag indicating complete PES received
    long pespcount = 0;                         // Count of packets in current PES
    long pcount = 0;                            // Total packet count
    int packet_analysis_mode = 0;               // Flag for general CC search mode
    int ret = CCX_EAGAIN;                       // Return value
    struct program_info *pinfo = NULL;          // Program information
    struct cap_info *cinfo;                     // Caption information
    struct ts_payload payload;                  // Current packet payload
    int j;                                      // Loop variable

    memset(&payload, 0, sizeof(payload));       // Initialize payload structure

//=============================================================================
// MAIN PACKET PROCESSING LOOP
//=============================================================================

    do
    {
        pcount++;                               // Increment packet counter

        // Read next transport stream packet
        ret = ts_readpacket(ctx, &payload);
        if (ret != CCX_OK)
            break;                              // EOF or error

        // Skip packets with transport errors
        if (payload.transport_error)
        {
            dbg_print(CCX_DMT_VERBOSE, "Packet (pid %u) skipped - transport error.\n", payload.pid);
            continue;
        }

//=============================================================================
// PSI TABLE PROCESSING - Handle Program Association Table
//=============================================================================

        if (payload.pid == 0)                   // PAT (Program Association Table)
        {
            ts_buffer_psi_packet(ctx);          // Buffer PSI packet
            if (ctx->PID_buffers[payload.pid] != NULL && ctx->PID_buffers[payload.pid]->buffer_length > 0)
                parse_PAT(ctx);                 // Parse complete PAT
            continue;
        }

//=============================================================================
// EPG DATA PROCESSING - Handle Electronic Program Guide data
//=============================================================================

        if (ccx_options.xmltv >= 1 && payload.pid == 0x11)
        { 
            // SDT (Service Description Table) or BAT (Bouquet Association Table)
            ts_buffer_psi_packet(ctx);
            if (ctx->PID_buffers[payload.pid] != NULL && ctx->PID_buffers[payload.pid]->buffer_length > 0)
                parse_SDT(ctx);
        }

        if (ccx_options.xmltv >= 1 && payload.pid == 0x12) // DVB EIT (Event Information Table)
            parse_EPG_packet(ctx->parent);
        if (ccx_options.xmltv >= 1 && payload.pid >= 0x1000) // ATSC EPG packet
            parse_EPG_packet(ctx->parent);

//=============================================================================
// PMT PROCESSING - Handle Program Map Tables
//=============================================================================

        // Check if this PID matches any PMT PID
        for (j = 0; j < ctx->nb_program; j++)
        {
            // Handle PCR (Program Clock Reference) updates
            if (ctx->pinfo[j].analysed_PMT_once == CCX_TRUE &&
                ctx->pinfo[j].pcr_pid == payload.pid &&
                payload.have_pcr)
            {
                // Update global timestamp from PCR
                ctx->last_global_timestamp = ctx->global_timestamp;
                ctx->global_timestamp = (uint32_t)payload.pcr / 90; // Convert to milliseconds
                if (!ctx->global_timestamp_inited)
                {
                    ctx->min_global_timestamp = ctx->global_timestamp;
                    ctx->global_timestamp_inited = 1;
                }
                if (ctx->min_global_timestamp > ctx->global_timestamp)
                {
                    ctx->offset_global_timestamp = ctx->last_global_timestamp - ctx->min_global_timestamp;
                    ctx->min_global_timestamp = ctx->global_timestamp;
                }
            }
            
            // Check if this is a PMT PID
            if (ctx->pinfo[j].pid == payload.pid)
            {
                if (!ctx->PIDs_seen[payload.pid])
                    dbg_print(CCX_DMT_PAT, "This PID (%u) is a PMT for program %u.\n", payload.pid, ctx->pinfo[j].program_number);
                pinfo = ctx->pinfo + j;
                break;
            }
        }
        
        // If this was a PMT packet, process it
        if (j != ctx->nb_program)
        {
            ctx->PIDs_seen[payload.pid] = 2;    // Mark as PMT
            ts_buffer_psi_packet(ctx);
            if (ctx->PID_buffers[payload.pid] != NULL && ctx->PID_buffers[payload.pid]->buffer_length > 0)
                if (parse_PMT(ctx, ctx->PID_buffers[payload.pid]->buffer + 1, ctx->PID_buffers[payload.pid]->buffer_length - 1, pinfo))
                    gotpes = 1;                 // PMT changed, flush buffer
            continue;
        }

//=============================================================================
// PID TRACKING AND IDENTIFICATION
//=============================================================================

        switch (ctx->PIDs_seen[payload.pid])
        {
            case 0: // First time seeing this PID
                if (ctx->PIDs_programs[payload.pid])
                {
                    dbg_print(CCX_DMT_PARSE, "\nNew PID found: %u (%s), belongs to program: %u\n", payload.pid,
                              desc[ctx->PIDs_programs[payload.pid]->printable_stream_type],
                              ctx->PIDs_programs[payload.pid]->program_number);
                    ctx->PIDs_seen[payload.pid] = 2;
                }
                else
                {
                    dbg_print(CCX_DMT_PARSE, "\nNew PID found: %u, program number still unknown\n", payload.pid);
                    ctx->PIDs_seen[payload.pid] = 1;
                }
                ctx->have_PIDs[ctx->num_of_PIDs] = payload.pid;
                ctx->num_of_PIDs++;
                break;
            case 1: // Seen before but program unknown
                if (ctx->PIDs_programs[payload.pid])
                {
                    dbg_print(CCX_DMT_PARSE, "\nProgram for PID: %u (previously unknown) is: %u (%s)\n", payload.pid,
                              ctx->PIDs_programs[payload.pid]->program_number,
                              desc[ctx->PIDs_programs[payload.pid]->printable_stream_type]);
                    ctx->PIDs_seen[payload.pid] = 2;
                }
                break;
            case 2: // Already seen and identified
                break;
            case 3: // Already inspected for CC data
                break;
        }

//=============================================================================
// PTS EXTRACTION - Get timing information from PES headers
//=============================================================================

        if (payload.pesstart) // PES header present
        {
            // Check for valid PES start code
            uint64_t pes_prefix = (payload.start[0] << 16) | (payload.start[9] << 8) | payload.start[1];
            uint8_t pes_stream_id = payload.start[2];
            uint64_t pts = 0;

            if (pes_prefix == 0x000001)
            {
                pts = get_pts(payload.start);   // Extract PTS
                
                // Find PID index and store stream ID and PTS
                int pid_index;
                for (int i = 0; i < ctx->num_of_PIDs; i++)
                    if (payload.pid == ctx->have_PIDs[i])
                        pid_index = i;
                ctx->stream_id_of_each_pid[pid_index] = pes_stream_id;
                if (pts < ctx->min_pts[pid_index])
                    ctx->min_pts[pid_index] = pts;
            }
        }

//=============================================================================
// SPECIAL PACKET HANDLING
//=============================================================================

        if (payload.pid == 8191)                // Null packet (padding)
            continue;
            
        // Hauppauge detection
        if (payload.pid == 1003 && !ctx->hauppauge_warning_shown && !ccx_options.hauppauge_mode)
        {
            mprint("\n\nNote: This TS could be a recording from a Hauppage card. If no captions are detected, try --hauppauge\n\n");
            ctx->hauppauge_warning_shown = 1;
        }

        // Hauppauge caption processing
        if (ccx_options.hauppauge_mode && payload.pid == HAUPPAGE_CCPID)
        {
            // Expand Hauppauge buffer if needed
            int haup_newcapbuflen = haup_capbuflen + payload.length;
            if (haup_newcapbuflen > haup_capbufsize)
            {
                haup_capbuf = (unsigned char *)realloc(haup_capbuf, haup_newcapbuflen);
                if (!haup_capbuf)
                    fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory to store hauppauge packets");
                haup_capbufsize = haup_newcapbuflen;
            }
            // Copy Hauppauge payload data
            memcpy(haup_capbuf + haup_capbuflen, payload.start, payload.length);
            haup_capbuflen = haup_newcapbuflen;
        }

        // Skip empty packets
        if (!payload.length)
        {
            dbg_print(CCX_DMT_VERBOSE, "Packet (pid %u) skipped - no payload.\n", payload.pid);
            continue;
        }

//=============================================================================
// CAPTION DATA PROCESSING - Handle packets with caption information
//=============================================================================

        cinfo = get_cinfo(ctx, payload.pid);    // Get caption info for this PID
        if (cinfo == NULL)
        {
            if (!packet_analysis_mode)
                dbg_print(CCX_DMT_PARSE, "Packet (pid %u) skipped - no stream with captions identified yet.\n", payload.pid);
            else
                look_for_caption_data(ctx, &payload); // Search for GA94 marker
            continue;
        }
        else if (cinfo->ignore == CCX_TRUE &&
                 (cinfo->stream != CCX_STREAM_TYPE_VIDEO_MPEG2 || !ccx_options.analyze_video_stream))
        {
            // Clean up ignored streams
            if (cinfo->codec_private_data)
            {
                switch (cinfo->codec)
                {
                    case CCX_CODEC_TELETEXT:
                        telxcc_close(&cinfo->codec_private_data, NULL);
                        break;
                    case CCX_CODEC_DVB:
                        dvbsub_close_decoder(&cinfo->codec_private_data);
                        break;
                    case CCX_CODEC_ISDB_CC:
                        delete_isdb_decoder(&cinfo->codec_private_data);
                    default:
                        break;
                }
                cinfo->codec_private_data = NULL;
            }

            if (cinfo->capbuflen > 0)
            {
                freep(&cinfo->capbuf);          // Free caption buffer
                cinfo->capbufsize = 0;
                cinfo->capbuflen = 0;
                delete_demuxer_data_node_by_pid(data, cinfo->pid);
            }
            continue;
        }

//=============================================================================
// PES PACKET ASSEMBLY - Build complete PES packets from TS packets  
//=============================================================================

        // Handle PES start
        if (payload.pesstart)
        {
            cinfo->saw_pesstart = 1;            // Mark PES start seen
            cinfo->prev_counter = payload.counter - 1;
        }

        // Skip packets until PES start found
        if (!cinfo->saw_pesstart)
            continue;

        // Check continuity counter
        if ((cinfo->prev_counter == 15 ? 0 : cinfo->prev_counter + 1) != payload.counter)
        {
            mprint("TS continuity counter not incremented prev/curr %u/%u\n",
                   cinfo->prev_counter, payload.counter);
        }
        cinfo->prev_counter = payload.counter;

        // Handle PES completion
        if (payload.pesstart && cinfo->capbuflen > 0)
        {
            dbg_print(CCX_DMT_PARSE, "\nPES finished (%ld bytes/%ld PES packets/%ld total packets)\n",
                      cinfo->capbuflen, pespcount, pcount);

            // Process completed PES
            ret = copy_capbuf_demux_data(ctx, data, cinfo);
            cinfo->capbuflen = 0;               // Reset buffer
            gotpes = 1;                         // Mark PES complete
        }

        // Copy payload to caption buffer
        copy_payload_to_capbuf(cinfo, &payload);
        if (ret < 0)
        {
            if (errno == EINVAL)
                continue;                       // Skip invalid data
            else
                break;                          // Fatal error
        }

        pespcount++;                            // Increment PES packet count
        
    } while (!gotpes);                          // Continue until PES complete

//=============================================================================
// PROGRAM TIMING ANALYSIS - Track minimum PTS for synchronization
//=============================================================================

    for (int i = 0; i < ctx->nb_program; i++)
    {
        pinfo = &ctx->pinfo[i];
        if (!pinfo->has_all_min_pts)
        {
            // Check all PIDs for this program
            for (int j = 0; j < ctx->num_of_PIDs; j++)
            {
                if (ctx->PIDs_programs[ctx->have_PIDs[j]])
                {
                    if (ctx->PIDs_programs[ctx->have_PIDs[j]]->program_number == pinfo->program_number)
                    {
                        if (ctx->min_pts[j] != UINT64_MAX)
                        {
                            // Update minimum PTS for different stream types
                            if (ctx->stream_id_of_each_pid[j] == 0xbd) // Private stream 1
                                if (ctx->min_pts[j] < pinfo->got_important_streams_min_pts[PRIVATE_STREAM_1])
                                    pinfo->got_important_streams_min_pts[PRIVATE_STREAM_1] = ctx->min_pts[j];
                            if (ctx->stream_id_of_each_pid[j] >= 0xc0 && ctx->stream_id_of_each_pid[j] <= 0xdf) // Audio
                                if (ctx->min_pts[j] < pinfo->got_important_streams_min_pts[AUDIO])
                                    pinfo->got_important_streams_min_pts[AUDIO] = ctx->min_pts[j];
                            if (ctx->stream_id_of_each_pid[j] >= 0xe0 && ctx->stream_id_of_each_pid[j] <= 0xef) // Video
                                if (ctx->min_pts[j] < pinfo->got_important_streams_min_pts[VIDEO])
                                    pinfo->got_important_streams_min_pts[VIDEO] = ctx->min_pts[j];
                            
                            // Check if we have all important stream types
                            if (pinfo->got_important_streams_min_pts[PRIVATE_STREAM_1] != UINT64_MAX &&
                                pinfo->got_important_streams_min_pts[AUDIO] != UINT64_MAX &&
                                pinfo->got_important_streams_min_pts[VIDEO] != UINT64_MAX)
                                pinfo->has_all_min_pts = 1;
                        }
                    }
                }
            }
        }
    }

    // Handle end-of-file cleanup
    if (ret == CCX_EOF)
    {
        cinfo_cremation(ctx, data);             // Process remaining caption data
    }
    
    return ret;
}

//=============================================================================
// FUNCTION: ts_get_more_data
// PURPOSE: Main interface function to get demuxed transport stream data
// PARAMETERS: ctx - Library context, data - Output demuxer data
// RETURNS: Status code (CCX_OK, CCX_EOF, error code)
//=============================================================================

int ts_get_more_data(struct lib_ccx_ctx *ctx, struct demuxer_data **data)
{
    int ret = CCX_OK;

    // Keep reading until we get complete data or reach EOF/error
    do
    {
        ret = ts_readstream(ctx->demux_ctx, data);
    } while (ret == CCX_EAGAIN);                // EAGAIN means try again

    return ret;
}
