// ASF GUIDs
// 10.1
#define ASF_HEADER "\x30\x26\xB2\x75\x8E\x66\xCF\x11\xA6\xD9\x00\xAA\x00\x62\xCE\x6C"
#define ASF_DATA "\x36\x26\xB2\x75\x8E\x66\xCF\x11\xA6\xD9\x00\xAA\x00\x62\xCE\x6C"

// 10.2
#define ASF_FILE_PROPERTIES "\xA1\xDC\xAB\x8C\x47\xA9\xCF\x11\x8E\xE4\x00\xC0\x0C\x20\x53\x65"
#define ASF_STREAM_PROPERTIES "\x91\x07\xDC\xB7\xB7\xA9\xCF\x11\x8E\xE6\x00\xC0\x0C\x20\x53\x65"
#define ASF_HEADER_EXTENSION "\xB5\x03\xBF\x5F\x2E\xA9\xCF\x11\x8E\xE3\x00\xC0\x0C\x20\x53\x65"
#define ASF_CONTENT_DESCRIPTION "\x33\x26\xB2\x75\x8E\x66\xCF\x11\xA6\xD9\x00\xAA\x00\x62\xCE\x6C"
#define ASF_EXTENDED_CONTENT_DESCRIPTION "\x40\xA4\xD0\xD2\x07\xE3\xD2\x11\x97\xF0\x00\xA0\xC9\x5E\xA8\x50"
#define ASF_STREAM_BITRATE_PROPERTIES "\xCE\x75\xF8\x7B\x8D\x46\xD1\x11\x8D\x82\x00\x60\x97\xC9\xA2\xB2"
// 10.3
#define ASF_EXTENDED_STREAM_PROPERTIES "\xCB\xA5\xE6\x14\x72\xC6\x32\x43\x83\x99\xA9\x69\x52\x06\x5B\x5A"
#define ASF_METADATA "\xEA\xCB\xF8\xC5\xAF\x5B\x77\x48\x84\x67\xAA\x8C\x44\xFA\x4C\xCA"
#define ASF_METADATA_LIBRARY "\x94\x1C\x23\x44\x98\x94\xD1\x49\xA1\x41\x1D\x13\x4E\x45\x70\x54"
#define ASF_COMPATIBILITY2 "\x5D\x8B\xF1\x26\x84\x45\xEC\x47\x9F\x5F\x0E\x65\x1F\x04\x52\xC9"
// Actually 10.2
#define ASF_PADDING "\x74\xD4\x06\x18\xDF\xCA\x09\x45\xA4\xBA\x9A\xAB\xCB\x96\xAA\xE8"
// 10.4
#define ASF_AUDIO_MEDIA "\x40\x9E\x69\xF8\x4D\x5B\xCF\x11\xA8\xFD\x00\x80\x5F\x5C\x44\x2B"
#define ASF_VIDEO_MEDIA "\xC0\xEF\x19\xBC\x4D\x5B\xCF\x11\xA8\xFD\x00\x80\x5F\x5C\x44\x2B"
#define ASF_BINARY_MEDIA "\xE2\x65\xFB\x3A\xEF\x47\xF2\x40\xAC\x2C\x70\xA9\x0D\x71\xD3\x43"

// ASF_BINARY_MEDIA : Major media types
#define DVRMS_AUDIO "\x9D\x8C\x17\x31\xE1\x03\x28\x45\xB5\x82\x3D\xF9\xDB\x22\xF5\x03"
#define DVRMS_NTSC "\x80\xEA\x0A\x67\x82\x3A\xD0\x11\xB7\x9B\x00\xAA\x00\x37\x67\xA7"
#define DVRMS_ATSC "\x89\x8A\x8B\xB8\x49\xB0\x80\x4C\xAD\xCF\x58\x98\x98\x5E\x22\xC1"

// 10.13 - Undocumented DVR-MS properties
#define DVRMS_PTS "\x2A\xC0\x3C\xFD\xDB\x06\xFA\x4C\x80\x1C\x72\x12\xD3\x87\x45\xE4"

typedef struct {
	int VideoStreamNumber;
	int AudioStreamNumber;
	int CaptionStreamNumber;
	int CaptionStreamStyle;  // 1 = NTSC, 2 = ATSC
	int DecodeStreamNumber;  // The stream that is chosen to be decoded
	int DecodeStreamPTS;     // This will be used for the next returned block
	int currDecodeStreamPTS; // Time of the data returned by the function
	int prevDecodeStreamPTS; // Previous time
	int VideoStreamMS;       // See ableve, just for video
	int currVideoStreamMS;
	int prevVideoStreamMS;
	int VideoJump;           // Remember a jump in the video timeline
} asf_data_stream_properties;

#define STREAMNUM  10
#define PAYEXTNUM 10

typedef struct {
	// Generic buffer to hold data
	unsigned char *parsebuf;
	long parsebufsize;
	// Header Object variables
	int64_t HeaderObjectSize;
	int64_t FileSize;
	uint32_t PacketSize;
	// Stream Properties Object variables
	asf_data_stream_properties StreamProperties;
	// Extended Stream Properties  - for DVR-MS presentation timestamp
	// Store the Payload Extension System Data Size.  First index holds the
	// stream number and the second index holds the Extension System entry.
	// I.e. PayloadExtSize[1][2] is the third Payload Extension System
	// entry for stream 1. (The streams are numbered starting from 1)
	// FIXME: What happens if we have more than 9 streams with more than
	// 10 entries.
	int PayloadExtSize[STREAMNUM][PAYEXTNUM];
	int PayloadExtPTSEntry[STREAMNUM];
	// Data object Header variables
	int64_t DataObjectSize;
	uint32_t TotalDataPackets;
	int VideoClosedCaptioningFlag;
	// Payload data
	int PayloadLType;            // ASF - Payload Length Type. <>0 for multiple payloads
	uint32_t PayloadLength;      // ASF - Payload Length
	int NumberOfPayloads;        // ASF - Number of payloads.
	int payloadcur;              // local
	int PayloadStreamNumber;     // ASF
	int KeyFrame;                // ASF
	uint32_t PayloadMediaNumber; // ASF
	// Data Object Loop
	uint32_t datapacketcur;      // Current packet number
	int64_t dobjectread;         // Bytes read in Data Object
	// Payload parsing information
	int MultiplePayloads;        // ASF
	int PacketLType;             // ASF
	int ReplicatedLType;         // ASF
	int OffsetMediaLType;        // ASF
	int MediaNumberLType;        // ASF
	int StreamNumberLType;       // ASF
	uint32_t PacketLength;
	uint32_t PaddingLength;
} asf_data;
