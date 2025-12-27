#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "asf_constants.h"
#include "activity.h"
#include "file_buffer.h"

// Indicate first / subsequent calls to asf_get_more_data()
int firstcall;

asf_data asf_data_container;

// For ASF parsing
// 0, 1, 2, 3 means none, BYTE, WORD, DWORD
#define ASF_TypeLength(A) (A == 3 ? 4 : A)

uint32_t asf_readval(void *val, int ltype)
{
	uint32_t rval;

	switch (ltype)
	{
		case 0:
			rval = 0;
			break;
		case 1:
			rval = *((uint8_t *)val);
			break;
		case 2:
			rval = *((uint16_t *)val);
			break;
		case 4:
			rval = *((uint32_t *)val);
			break;
		default:
			fatal(CCX_COMMON_EXIT_BUG_BUG, "In asf_readval(): Invalid ltype, cannot continue processing this stream.\n");
			break;
	}
	return rval;
}

char *gui_data_string(void *val)
{
	static char sbuf[40];

	snprintf(sbuf, sizeof(sbuf), "%08lX-%04X-%04X-",
		 (long)*((uint32_t *)((char *)val + 0)),
		 (int)*((uint16_t *)((char *)val + 4)),
		 (int)*((uint16_t *)((char *)val + 6)));
	for (int ii = 0; ii < 2; ii++)
		snprintf(sbuf + 19 + ii * 2, sizeof(sbuf) - 19 - ii * 2, "%02X-", *((unsigned char *)val + 8 + ii));
	for (int ii = 0; ii < 6; ii++)
		snprintf(sbuf + 24 + ii * 2, sizeof(sbuf) - 24 - ii * 2, "%02X", *((unsigned char *)val + 10 + ii));

	return sbuf;
}

/* ASF container specific data parser
 * The following function reads an ASF file and returns the included
 * video stream.  The function returns after a complete Media Object
 * is read (the Media Object Number increases by one).  A Media Object
 * seems to consist of one frame.
 * When the function is called the next time it continues to read
 * where it stopped before, static variables make sure that parameters
 * are remembered between calls. */
int asf_get_more_data(struct lib_ccx_ctx *ctx, struct demuxer_data **ppdata)
{
	int enough = 0;
	int payload_read = 0;

	// The fist call to this function (per file) is indicated with
	// firstcall == 1
	// Except for the first call of this function we will reenter
	// the Data Packet loop below.
	int reentry = 1;

	// Variables for Header Object
	int64_t data_packets_count = 0;
	int broadcast_flag = 0;
	int seekable_flag = 0;
	uint32_t min_packet_size = 0;
	uint32_t max_packet_size = 0;

	// Data Object Loop
	int data_packet_length = 0; // Collect the read header bytes

	// Payload parsing information
	int sequence_type = 0;	// ASF
	int padding_l_type = 0; // ASF
	uint32_t sequence = 0;
	uint32_t send_time = 0;

	int payload_parser_size = 0; // Inferred (PacketLType + sequence_type + padding_l_type + 6);

	uint32_t offset_media_length = 0; // ASF
	uint32_t replicated_length = 0;	  // ASF

	// Last media number. Used to determine a new PES, mark uninitialized.
	uint32_t current_media_number = 0xFFFFFFFF;

	unsigned char *current_position;
	int64_t get_bytes;
	size_t result = 0;
	struct demuxer_data *data;

	if (!*ppdata)
		*ppdata = alloc_demuxer_data();
	if (!*ppdata)
		return -1;

	data = *ppdata;

	// Read Header Object and the Top-level Data Object header only once
	if (firstcall)
	{
		asf_data_container = (asf_data){
		    .parsebuf = (unsigned char *)malloc(1024),
		    .parsebufsize = 1024,
		    .FileSize = 0,
		    .PacketSize = 0,
		    .StreamProperties = {
			// Make sure the stream numbers are invalid when a new file begins
			// so that they are only set when detected.
			.VideoStreamNumber = 0,
			.AudioStreamNumber = 0,
			.CaptionStreamNumber = 0,
			.CaptionStreamStyle = 0,
			.DecodeStreamNumber = 0,
			.DecodeStreamPTS = 0,
			.currDecodeStreamPTS = 0,
			.prevDecodeStreamPTS = 0,
			.VideoStreamMS = 0,
			.currVideoStreamMS = 0,
			.prevVideoStreamMS = 0,
			.VideoJump = 0},
		    .VideoClosedCaptioningFlag = 0,
		    .PayloadLType = 0,
		    .PayloadLength = 0,
		    .NumberOfPayloads = 0,
		    .payloadcur = 0,
		    .PayloadStreamNumber = 0,
		    .KeyFrame = 0,
		    .PayloadMediaNumber = 0,
		    .datapacketcur = 0,
		    .dobjectread = 50,
		    .MultiplePayloads = 0,
		    .PacketLType = 0,
		    .ReplicatedLType = 0,
		    .OffsetMediaLType = 0,
		    .MediaNumberLType = 0,
		    .StreamNumberLType = 0,
		    .PacketLength = 0,
		    .PaddingLength = 0};
		// Check for allocation failure
		if (!asf_data_container.parsebuf)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "In asf_getmoredata: Out of memory allocating initial parse buffer.");

		// Initialize the Payload Extension System
		for (int stream = 0; stream < STREAMNUM; stream++)
		{
			for (int payext = 0; payext < PAYEXTNUM; payext++)
			{
				asf_data_container.PayloadExtSize[stream][payext] = 0;
			}
			asf_data_container.PayloadExtPTSEntry[stream] = -1;
		}

		result = buffered_read(ctx->demux_ctx, asf_data_container.parsebuf, 30);
		ctx->demux_ctx->past += result;
		if (result != 30)
		{
			mprint("Premature end of file!\n");
			end_of_file = 1;
			return payload_read;
		}

		// Expecting ASF header
		if (!memcmp(asf_data_container.parsebuf, ASF_HEADER, 16))
		{
			dbg_print(CCX_DMT_PARSE, "\nASF header\n");
		}
		else
		{
			fatal(EXIT_MISSING_ASF_HEADER, "Missing ASF header. Could not read ASF file. Abort.\n");
		}
		asf_data_container.HeaderObjectSize = *((int64_t *)(asf_data_container.parsebuf + 16));
		dbg_print(CCX_DMT_PARSE, "Length: %lld\n", asf_data_container.HeaderObjectSize);
		dbg_print(CCX_DMT_PARSE, "\nNumber of header objects: %ld\n",
			  (long)*((uint32_t *)(asf_data_container.parsebuf + 24)));

		if (asf_data_container.HeaderObjectSize > asf_data_container.parsebufsize)
		{
			unsigned char *tmp = (unsigned char *)realloc(asf_data_container.parsebuf, (size_t)asf_data_container.HeaderObjectSize);
			if (!tmp)
			{
				free(asf_data_container.parsebuf);
				fatal(EXIT_NOT_ENOUGH_MEMORY, "In asf_getmoredata: Out of memory requesting buffer for data container.");
			}
			asf_data_container.parsebuf = tmp;
			asf_data_container.parsebufsize = (long)asf_data_container.HeaderObjectSize;
		}

		current_position = asf_data_container.parsebuf + 30;
		get_bytes = asf_data_container.HeaderObjectSize - 30;

		result = buffered_read(ctx->demux_ctx, current_position, (int)get_bytes);
		ctx->demux_ctx->past += result;
		if (result != get_bytes)
		{
			mprint("Premature end of file!\n");
			end_of_file = 1;
			return payload_read;
		}

		dbg_print(CCX_DMT_PARSE, "Reading header objects\n");

		while (current_position < asf_data_container.parsebuf + asf_data_container.HeaderObjectSize)
		{
			int64_t hpobjectsize = *((int64_t *)(current_position + 16)); // Local

			if (!memcmp(current_position, ASF_FILE_PROPERTIES, 16))
			{
				// Mandatory Object, only one.
				dbg_print(CCX_DMT_PARSE, "\nFile Properties Object     (size: %lld)\n", hpobjectsize);

				asf_data_container.FileSize = *((int64_t *)(current_position + 40));
				data_packets_count = *((int64_t *)(current_position + 56));
				broadcast_flag = 0x1 & current_position[88];
				seekable_flag = 0x2 & current_position[88];
				min_packet_size = *((uint32_t *)(current_position + 92));
				max_packet_size = *((uint32_t *)(current_position + 96));

				dbg_print(CCX_DMT_PARSE, "FileSize: %lld   Packet count: %lld\n", asf_data_container.FileSize, data_packets_count);
				dbg_print(CCX_DMT_PARSE, "Broadcast: %d - Seekable: %d\n", broadcast_flag, seekable_flag);
				dbg_print(CCX_DMT_PARSE, "MiDPS: %d   MaDPS: %d\n", min_packet_size, max_packet_size);
			}
			else if (!memcmp(current_position, ASF_STREAM_PROPERTIES, 16))
			{
				dbg_print(CCX_DMT_PARSE, "\nStream Properties Object     (size: %lld)\n", hpobjectsize);
				if (!memcmp(current_position + 24, ASF_VIDEO_MEDIA, 16))
				{
					asf_data_container.StreamProperties.VideoStreamNumber = *(current_position + 72) & 0x7F;
					dbg_print(CCX_DMT_PARSE, "Stream Type: ASF_Video_Media\n");
					dbg_print(CCX_DMT_PARSE, "Video Stream Number: %d\n", asf_data_container.StreamProperties.VideoStreamNumber);
				}
				else if (!memcmp(current_position + 24, ASF_AUDIO_MEDIA, 16))
				{
					asf_data_container.StreamProperties.AudioStreamNumber = *(current_position + 72) & 0x7F;
					dbg_print(CCX_DMT_PARSE, "Stream Type: ASF_Audio_Media\n");
					dbg_print(CCX_DMT_PARSE, "Audio Stream Number: %d\n", asf_data_container.StreamProperties.AudioStreamNumber);
				}
				else
				{
					dbg_print(CCX_DMT_PARSE, "Stream Type: %s\n",
						  gui_data_string(current_position + 24));
					dbg_print(CCX_DMT_PARSE, "Stream Number: %d\n", *(current_position + 72) & 0x7F);
				}
			}
			else if (!memcmp(current_position, ASF_HEADER_EXTENSION, 16))
			{
				dbg_print(CCX_DMT_PARSE, "\nHeader Extension Object     (size: %lld)\n", hpobjectsize);

				int32_t header_extension_data_size = *((uint32_t *)(current_position + 42));
				// Process Header Extension Data
				if (header_extension_data_size)
				{
					unsigned char *header_current_position = current_position + 46;

					if (header_extension_data_size != hpobjectsize - 46)
						fatal(EXIT_NOT_CLASSIFIED, "In asf_getmoredata: Incorrect HeaderExtensionDataSize value, cannot continue.");

					dbg_print(CCX_DMT_PARSE, "\nReading Header Extension Sub-Objects\n");
					while (header_current_position < current_position + 46 + header_extension_data_size)
					{
						int64_t header_object_size = *((int64_t *)(header_current_position + 16)); // Local

						if (!memcmp(header_current_position, ASF_EXTENDED_STREAM_PROPERTIES, 16))
						{
							dbg_print(CCX_DMT_PARSE, "\nExtended Stream Properties Object     (size: %lld)\n", header_object_size);
							int stream_number = *((uint16_t *)(header_current_position + 72));
							int stream_name_count = *((uint16_t *)(header_current_position + 84));
							int payload_ext_system_count = *((uint16_t *)(header_current_position + 86));

							unsigned char *stream_prop_position = header_current_position + 88;

							int stream_name_length;

							dbg_print(CCX_DMT_PARSE, "Stream Number: %d NameCount: %d  ESCount: %d\n",
								  stream_number, stream_name_count, payload_ext_system_count);

							if (stream_number >= STREAMNUM)
								fatal(CCX_COMMON_EXIT_BUG_BUG, "In asf_getmoredata: STREAMNUM too small. Please file a bug report on GitHub.\n");

							for (int i = 0; i < stream_name_count; i++)
							{
								dbg_print(CCX_DMT_PARSE, "%2d. Stream Name Field\n", i);
								stream_name_length = *((uint16_t *)(stream_prop_position + 2));
								stream_prop_position += 4 + stream_name_length;
							}
							int ext_system_data_size;
							int ext_system_info_length;

							if (payload_ext_system_count > PAYEXTNUM)
								fatal(CCX_COMMON_EXIT_BUG_BUG, "In asf_getmoredata: PAYEXTNUM too small. Please file a bug report on GitHub.\n");

							for (int i = 0; i < payload_ext_system_count; i++)
							{
								ext_system_data_size = *((uint16_t *)(stream_prop_position + 16));
								ext_system_info_length = *((uint32_t *)(stream_prop_position + 18));

								asf_data_container.PayloadExtSize[stream_number][i] = ext_system_data_size;

								dbg_print(CCX_DMT_PARSE, "%2d. Payload Extension GUID: %s Size %d Info Length %d\n",
									  i, gui_data_string(stream_prop_position + 0),
									  ext_system_data_size,
									  ext_system_info_length);

								// For DVR-MS presentation timestamp
								if (!memcmp(stream_prop_position, DVRMS_PTS, 16))
								{
									dbg_print(CCX_DMT_PARSE, "Found DVRMS_PTS\n");
									asf_data_container.PayloadExtPTSEntry[stream_number] = i;
								}

								stream_prop_position += 22 + ext_system_info_length;
							}

							// Now, there can be a Stream Properties Object.  The only way to
							// find out is to check if there are bytes left in the current
							// object.
							if ((stream_prop_position - header_current_position) < header_object_size)
							{
								int64_t stream_prop_object_size = *((int64_t *)(stream_prop_position + 16)); // Local
								if (memcmp(stream_prop_position, ASF_STREAM_PROPERTIES, 16))
									fatal(EXIT_NOT_CLASSIFIED, "Stream Properties Object expected\n");

								if (!memcmp(stream_prop_position + 24, ASF_VIDEO_MEDIA, 16))
								{
									dbg_print(CCX_DMT_PARSE, "Stream Type: ASF_Video_Media (size: %lld)\n",
										  stream_prop_object_size);
									asf_data_container.StreamProperties.VideoStreamNumber = stream_number;
								}
								else if (!memcmp(stream_prop_position + 24, ASF_AUDIO_MEDIA, 16))
								{
									dbg_print(CCX_DMT_PARSE, "Stream Type: ASF_Audio_Media (size: %lld)\n",
										  stream_prop_object_size);
									asf_data_container.StreamProperties.AudioStreamNumber = stream_number;
								}
								else if (!memcmp(stream_prop_position + 24, ASF_BINARY_MEDIA, 16))
								{
									// dvr-ms files identify audio streams as binary streams
									// but use the "major media type" accordingly to identify
									// the steam.  (There might be other audio identifiers.)
									if (!memcmp(stream_prop_position + 78, DVRMS_AUDIO, 16))
									{
										dbg_print(CCX_DMT_PARSE, "Binary media: DVR-MS Audio Stream (size: %lld)\n",
											  stream_prop_object_size);
									}
									else if (!memcmp(stream_prop_position + 78, DVRMS_NTSC, 16))
									{
										dbg_print(CCX_DMT_PARSE, "Binary media: NTSC captions (size: %lld)\n",
											  stream_prop_object_size);
										asf_data_container.StreamProperties.CaptionStreamNumber = stream_number;
										asf_data_container.StreamProperties.CaptionStreamStyle = 1;
									}
									else if (!memcmp(stream_prop_position + 78, DVRMS_ATSC, 16))
									{
										dbg_print(CCX_DMT_PARSE, "Binary media: ATSC captions (size: %lld)\n",
											  stream_prop_object_size);
										asf_data_container.StreamProperties.CaptionStreamNumber = stream_number;
										asf_data_container.StreamProperties.CaptionStreamStyle = 2;
									}
									else
									{
										dbg_print(CCX_DMT_PARSE, "Binary media: Major Media Type GUID: %s (size: %lld)\n",
											  gui_data_string(stream_prop_position + 78), stream_prop_object_size);
									}
								}
								else
								{
									dbg_print(CCX_DMT_PARSE, "Unknown Type GUID: %s (size: %lld)\n",
										  gui_data_string(stream_prop_position + 24), stream_prop_object_size);
								}
							}
							else
							{
								dbg_print(CCX_DMT_PARSE, "No Stream Properties Object\n");
							}
						}
						else if (!memcmp(header_current_position, ASF_METADATA, 16))
						{
							dbg_print(CCX_DMT_PARSE, "\nMetadata Object     (size: %lld)\n", header_object_size);
						}
						else if (!memcmp(header_current_position, ASF_METADATA_LIBRARY, 16))
						{
							dbg_print(CCX_DMT_PARSE, "\nMetadata Library Object     (size: %lld)\n", header_object_size);
						}
						else if (!memcmp(header_current_position, ASF_COMPATIBILITY2, 16))
						{
							dbg_print(CCX_DMT_PARSE, "\nCompatibility Object 2     (size: %lld)\n", header_object_size);
						}
						else if (!memcmp(header_current_position, ASF_PADDING, 16))
						{
							dbg_print(CCX_DMT_PARSE, "\nPadding Object     (size: %lld)\n", header_object_size);
						}
						else
						{
							dbg_print(CCX_DMT_PARSE, "\nGUID: %s  size: %lld\n",
								  gui_data_string(header_current_position), header_object_size);
							dump(CCX_DMT_PARSE, header_current_position, 16, 0, 0);
						}

						header_current_position += header_object_size;
					}
					if (header_current_position - (current_position + 46) != header_extension_data_size)
						fatal(EXIT_NOT_CLASSIFIED, "Header Extension Parsing problem: read bytes %ld != header length %lld\nAbort!\n",
						      (long)(header_current_position - (current_position + 46)), header_extension_data_size);
				}
				dbg_print(CCX_DMT_PARSE, "\nHeader Extension Object  -  End\n");
			}
			else if (!memcmp(current_position, ASF_CONTENT_DESCRIPTION, 16))
			{
				dbg_print(CCX_DMT_PARSE, "\nContend Description Object     (size: %lld)\n", hpobjectsize);
			}
			else if (!memcmp(current_position, ASF_EXTENDED_CONTENT_DESCRIPTION, 16))
			{
				dbg_print(CCX_DMT_PARSE, "\nExtended Content Description Object     (size: %lld)\n", hpobjectsize);

				int content_descriptor_count = *((uint16_t *)(current_position + 24));
				unsigned char *ext_content_position = current_position + 26;
				int descriptor_name_length;
				int descriptor_value_data_type;
				int descriptor_value_length;
				unsigned char *extended_description_value;

				for (int i = 0; i < content_descriptor_count; i++)
				{
					descriptor_name_length = *((uint16_t *)(ext_content_position));
					descriptor_value_data_type = *((uint16_t *)(ext_content_position + 2 + descriptor_name_length));
					descriptor_value_length = *((uint16_t *)(ext_content_position + 4 + descriptor_name_length));
					extended_description_value = ext_content_position + 6 + descriptor_name_length;

					dbg_print(CCX_DMT_PARSE, "%3d. %ls = ", i, (wchar_t *)(ext_content_position + 2));
					switch (descriptor_value_data_type)
					{
						case 0: // Unicode string
							dbg_print(CCX_DMT_PARSE, "%ls (Unicode)\n", (wchar_t *)extended_description_value);
							break;
						case 1: // byte string
							dbg_print(CCX_DMT_PARSE, ":");
							for (int ii = 0; ii < descriptor_value_length && ii < 9; ii++)
							{
								dbg_print(CCX_DMT_PARSE, "%02X:", *((unsigned char *)(extended_description_value + ii)));
							}
							if (descriptor_value_length > 8)
								dbg_print(CCX_DMT_PARSE, "skipped %d more", descriptor_value_length - 8);
							dbg_print(CCX_DMT_PARSE, " (BYTES)\n");
							break;
						case 2: // BOOL
							dbg_print(CCX_DMT_PARSE, "%d (BOOL)\n", *((int32_t *)extended_description_value));
							break;
						case 3: // DWORD
							dbg_print(CCX_DMT_PARSE, "%u (DWORD)\n", *((uint32_t *)extended_description_value));
							break;
						case 4: // QWORD
							dbg_print(CCX_DMT_PARSE, "%llu (QWORD)\n", *((uint64_t *)extended_description_value));
							break;
						case 5: // WORD
							dbg_print(CCX_DMT_PARSE, "%u (WORD)\n", (int)*((uint16_t *)extended_description_value));
							break;
						default:
							fatal(CCX_COMMON_EXIT_BUG_BUG, "In asf_getmoredata: Impossible value for DescriptorValueDataType. Please file a bug report in GitHub.\n");
							break;
					}

					if (!memcmp(ext_content_position + 2, L"WM/VideoClosedCaptioning", descriptor_name_length))
					{
						// This flag would be really useful if it would be
						// reliable - it isn't.
						asf_data_container.VideoClosedCaptioningFlag = *((int32_t *)extended_description_value);
						dbg_print(CCX_DMT_PARSE, "Found WM/VideoClosedCaptioning flag: %d\n",
							  asf_data_container.VideoClosedCaptioningFlag);
					}

					ext_content_position += 6 + descriptor_name_length + descriptor_value_length;
				}
			}
			else if (!memcmp(current_position, ASF_STREAM_BITRATE_PROPERTIES, 16))
			{
				dbg_print(CCX_DMT_PARSE, "\nStream Bitrate Properties Object     (size: %lld)\n", hpobjectsize);
			}
			else
			{
				dbg_print(CCX_DMT_PARSE, "\nGUID: %s  size: %lld\n",
					  gui_data_string(current_position), hpobjectsize);
				dump(CCX_DMT_PARSE, current_position, 16, 0, 0);
			}

			current_position += hpobjectsize;
		}
		if (current_position - asf_data_container.parsebuf != asf_data_container.HeaderObjectSize)
			fatal(EXIT_NOT_CLASSIFIED, "Header Object Parsing problem: read bytes %ld != header length %lld\nAbort!\n",
			      (long)(current_position - asf_data_container.parsebuf), asf_data_container.HeaderObjectSize);

		if (asf_data_container.StreamProperties.VideoStreamNumber == 0)
			fatal(EXIT_NOT_CLASSIFIED, "No Video Stream Properties Object found.  Unable to continue ...\n");

		// Wouldn't it be nice if  VideoClosedCaptioningFlag  would be usable, unfortunately
		// it is not reliable.

		// Now decide where we are going to expect the captions
		data->bufferdatatype = CCX_PES; // Except for NTSC captions
		if (asf_data_container.StreamProperties.CaptionStreamNumber > 0 && (asf_data_container.StreamProperties.CaptionStreamStyle == 1 ||
										    (asf_data_container.StreamProperties.CaptionStreamStyle == 2 && !ccx_options.wtvconvertfix)))
		{
			mprint("\nNote: If you converted a WTV into a DVR-MS and CCExtractor finds no captions, try passing -wtvconvertfix to work around bug in the conversion process.");
		}
		if (asf_data_container.StreamProperties.CaptionStreamNumber > 0 && (asf_data_container.StreamProperties.CaptionStreamStyle == 1 ||
										    (asf_data_container.StreamProperties.CaptionStreamStyle == 2 && ccx_options.wtvconvertfix)))
		{
			// if (debug_parse)
			mprint("\nNTSC captions in stream #%d\n\n", asf_data_container.StreamProperties.CaptionStreamNumber);
			data->bufferdatatype = CCX_RAW;
			asf_data_container.StreamProperties.DecodeStreamNumber = asf_data_container.StreamProperties.CaptionStreamNumber;
		}
		else if (asf_data_container.StreamProperties.CaptionStreamNumber > 0 && asf_data_container.StreamProperties.CaptionStreamStyle == 2)
		{
			// if (debug_parse)
			mprint("\nATSC captions (probably) in stream #%d - Decode the video stream #%d instead\n\n",
			       asf_data_container.StreamProperties.CaptionStreamNumber, asf_data_container.StreamProperties.VideoStreamNumber);
			asf_data_container.StreamProperties.DecodeStreamNumber = asf_data_container.StreamProperties.VideoStreamNumber;
		}
		else
		{
			asf_data_container.StreamProperties.DecodeStreamNumber = asf_data_container.StreamProperties.VideoStreamNumber;
			// if (debug_parse)
			mprint("\nAssume CC info in video stream #%d (No caption stream found)\n\n",
			       asf_data_container.StreamProperties.DecodeStreamNumber);
		}

		// When reading "Payload parsing information" it occured that "Packet Lenght"
		// was not set (Packet Length Type 00) and for "Single Payloads" this means
		// the Payload Data Length cannot be infered.  Use min_packet_size, max_packet_size instead.
		if ((min_packet_size > 0) && (min_packet_size == max_packet_size))
			asf_data_container.PacketSize = min_packet_size;

		// Now the Data Object, except for the packages
		result = buffered_read(ctx->demux_ctx, asf_data_container.parsebuf, 50); // No realloc needed.
		ctx->demux_ctx->past += result;
		if (result != 50)
		{
			mprint("Premature end of file!\n");
			end_of_file = 1;
			return payload_read;
		}

		// Expecting ASF Data object
		if (!memcmp(asf_data_container.parsebuf, ASF_DATA, 16))
		{
			dbg_print(CCX_DMT_PARSE, "\nASF Data Object\n");
		}
		else
		{
			fatal(EXIT_NOT_CLASSIFIED, "In asf_getmoredata: Missing ASF Data Object. Abort.\n");
		}

		asf_data_container.DataObjectSize = *((int64_t *)(asf_data_container.parsebuf + 16));
		asf_data_container.TotalDataPackets = *((uint32_t *)(asf_data_container.parsebuf + 40));
		dbg_print(CCX_DMT_PARSE, "Size: %lld\n", asf_data_container.DataObjectSize);
		dbg_print(CCX_DMT_PARSE, "Number of data packets: %ld\n", (long)asf_data_container.TotalDataPackets);

		reentry = 0; // Make sure we read the Data Packet Headers
	}		     // End of if (firstcall)
	firstcall = 0;

	// Start loop over Data Packets
	while (asf_data_container.datapacketcur < asf_data_container.TotalDataPackets && !enough)
	{
		// Skip reading the headers the first time when reentering the loop
		if (!reentry)
		{
			int ecinfo = 0;

			data_packet_length = 0;

			dbg_print(CCX_DMT_PARSE, "\nReading packet %d/%d\n", asf_data_container.datapacketcur + 1, asf_data_container.TotalDataPackets);

			// First packet
			result = buffered_read(ctx->demux_ctx, asf_data_container.parsebuf, 1); // No realloc needed.
			ctx->demux_ctx->past += result;
			asf_data_container.dobjectread += result;
			if (result != 1)
			{
				mprint("Premature end of file!\n");
				end_of_file = 1;
				return payload_read;
			}
			data_packet_length += 1;
			if (*asf_data_container.parsebuf & 0x80)
			{
				int ecdatalength = *asf_data_container.parsebuf & 0x0F; // Small, no realloc needed
				if (*asf_data_container.parsebuf & 0x60)
				{
					fatal(EXIT_NOT_CLASSIFIED, "Error Correction Length Type not 00 - reserved - aborting ...\n");
				}
				result = buffered_read(ctx->demux_ctx, asf_data_container.parsebuf + 1, ecdatalength);
				ctx->demux_ctx->past += result;
				asf_data_container.dobjectread += result;
				if (result != ecdatalength)
				{
					mprint("Premature end of file!\n");
					end_of_file = 1;
					return payload_read;
				}
				data_packet_length += ecdatalength;
				if (asf_data_container.parsebuf[1] & 0x0F)
					fatal(EXIT_NOT_CLASSIFIED, "Error correction present.  Unable to continue ...\n");
			}
			else
			{
				// When no ecinfo is present the byte we just read belongs
				// to the payload parsing information.
				ecinfo = 1;
			}

			// Now payload parsing information
			result = buffered_read(ctx->demux_ctx, asf_data_container.parsebuf + ecinfo, 2 - ecinfo); // No realloc needed
			ctx->demux_ctx->past += result;
			asf_data_container.dobjectread += result;
			if (result != 2)
			{
				mprint("Premature end of file!\n");
				end_of_file = 1;
				return payload_read;
			}
			data_packet_length += 2;

			asf_data_container.MultiplePayloads = *asf_data_container.parsebuf & 0x01;

			sequence_type = (*asf_data_container.parsebuf >> 1) & 0x03;
			sequence_type = ASF_TypeLength(sequence_type);

			padding_l_type = (*asf_data_container.parsebuf >> 3) & 0x03;
			padding_l_type = ASF_TypeLength(padding_l_type);

			asf_data_container.PacketLType = (*asf_data_container.parsebuf >> 5) & 0x03;
			asf_data_container.PacketLType = ASF_TypeLength(asf_data_container.PacketLType);

			asf_data_container.ReplicatedLType = (asf_data_container.parsebuf[1]) & 0x03;
			asf_data_container.ReplicatedLType = ASF_TypeLength(asf_data_container.ReplicatedLType);

			asf_data_container.OffsetMediaLType = (asf_data_container.parsebuf[1] >> 2) & 0x03;
			asf_data_container.OffsetMediaLType = ASF_TypeLength(asf_data_container.OffsetMediaLType);

			asf_data_container.MediaNumberLType = (asf_data_container.parsebuf[1] >> 4) & 0x03;
			asf_data_container.MediaNumberLType = ASF_TypeLength(asf_data_container.MediaNumberLType);

			asf_data_container.StreamNumberLType = (asf_data_container.parsebuf[1] >> 6) & 0x03;
			asf_data_container.StreamNumberLType = ASF_TypeLength(asf_data_container.StreamNumberLType);

			payload_parser_size = asf_data_container.PacketLType + sequence_type + padding_l_type + 6;

			result = buffered_read(ctx->demux_ctx, asf_data_container.parsebuf + 2, payload_parser_size); // No realloc needed
			ctx->demux_ctx->past += result;
			asf_data_container.dobjectread += result;
			if (result != payload_parser_size)
			{
				mprint("Premature end of file!\n");
				end_of_file = 1;
				return payload_read;
			}
			data_packet_length += payload_parser_size;

			asf_data_container.PacketLength = asf_readval(asf_data_container.parsebuf + 2, asf_data_container.PacketLType);
			sequence = asf_readval(asf_data_container.parsebuf + 2 + asf_data_container.PacketLType, sequence_type);
			asf_data_container.PaddingLength = asf_readval(asf_data_container.parsebuf + 2 + asf_data_container.PacketLType + sequence_type, padding_l_type);
			// Data Packet ms time stamp
			send_time = *((uint32_t *)(asf_data_container.parsebuf + 2 + asf_data_container.PacketLType + sequence_type + padding_l_type));

			// If Packet Length is not set use global setting if possible
			if (asf_data_container.PacketLength == 0)
			{
				asf_data_container.PacketLength = asf_data_container.PacketSize;
				// For multiple payloads we can get away without a given
				// Packet length as individual payload length are given
				if (asf_data_container.PacketLength == 0 && asf_data_container.MultiplePayloads == 0)
					fatal(EXIT_NOT_CLASSIFIED, "In asf_getmoredata: Cannot determine packet length. Unable to continue processing this file.\n");
			}

			dbg_print(CCX_DMT_PARSE, "Lengths - Packet: %d / sequence %d / Padding %d\n",
				  asf_data_container.PacketLength, sequence, asf_data_container.PaddingLength);

			asf_data_container.PayloadLType = 0;	 // Payload Length Type. <>0 for multiple payloads
			asf_data_container.PayloadLength = 0;	 // Payload Length (for multiple payloads)
			asf_data_container.NumberOfPayloads = 1; // Number of payloads.

			if (asf_data_container.MultiplePayloads != 0)
			{
				unsigned char plheader[1];

				result = buffered_read(ctx->demux_ctx, plheader, 1);
				ctx->demux_ctx->past += result;
				asf_data_container.dobjectread += result;
				if (result != 1)
				{
					mprint("Premature end of file!\n");
					end_of_file = 1;
					return payload_read;
				}
				data_packet_length += 1;
				asf_data_container.PayloadLType = (*plheader >> 6) & 0x03;
				asf_data_container.PayloadLType = ASF_TypeLength(asf_data_container.PayloadLType);

				// Number of payloads
				asf_data_container.NumberOfPayloads = *plheader & 0x3F;
			}
			asf_data_container.payloadcur = 0;
		}
		else
		{ // Rely on
			// NumberOfPayloads, payloadcur, PayloadLength, PaddingLength
			// and related variables being kept as static variables to be
			// able to reenter the loop here.
			dbg_print(CCX_DMT_PARSE, "\nReentry into asf_get_more_data()\n");
		}

		// The following repeats NumberOfPayloads times
		while (asf_data_container.payloadcur < asf_data_container.NumberOfPayloads && !enough)
		{
			// Skip reading the Payload headers the first time when reentering the loop
			if (!reentry)
			{
				if (asf_data_container.NumberOfPayloads < 2)
					dbg_print(CCX_DMT_PARSE, "\nSingle payload\n");
				else
					dbg_print(CCX_DMT_PARSE, "\nMultiple payloads %d/%d\n", asf_data_container.payloadcur + 1, asf_data_container.NumberOfPayloads);

				int payload_header_size = 1 + asf_data_container.MediaNumberLType + asf_data_container.OffsetMediaLType + asf_data_container.ReplicatedLType;

				result = buffered_read(ctx->demux_ctx, asf_data_container.parsebuf, payload_header_size); // No realloc needed
				ctx->demux_ctx->past += result;
				asf_data_container.dobjectread += result;
				if (result != payload_header_size)
				{
					mprint("Premature end of file!\n");
					end_of_file = 1;
					return payload_read;
				}
				data_packet_length += payload_header_size;

				asf_data_container.PayloadStreamNumber = *asf_data_container.parsebuf & 0x7F;
				asf_data_container.KeyFrame = (*asf_data_container.parsebuf & 0x80) && 1;

				asf_data_container.PayloadMediaNumber = asf_readval(asf_data_container.parsebuf + 1, asf_data_container.MediaNumberLType);
				offset_media_length = asf_readval(asf_data_container.parsebuf + 1 + asf_data_container.MediaNumberLType, asf_data_container.OffsetMediaLType);
				replicated_length = asf_readval(asf_data_container.parsebuf + 1 + asf_data_container.MediaNumberLType + asf_data_container.OffsetMediaLType, asf_data_container.ReplicatedLType);

				if (replicated_length == 1)
					fatal(EXIT_NOT_CLASSIFIED, "Cannot handle compressed data...\n");

				if ((long)replicated_length > asf_data_container.parsebufsize)
				{
					unsigned char *tmp = (unsigned char *)realloc(asf_data_container.parsebuf, replicated_length);
					if (!tmp)
					{
						free(asf_data_container.parsebuf);
						fatal(EXIT_NOT_ENOUGH_MEMORY, "In asf_getmoredata: Not enough memory for buffer, unable to continue.\n");
					}
					asf_data_container.parsebuf = tmp;
					asf_data_container.parsebufsize = replicated_length;
				}
				result = buffered_read(ctx->demux_ctx, asf_data_container.parsebuf, (long)replicated_length);
				ctx->demux_ctx->past += result;
				asf_data_container.dobjectread += result;
				if (result != replicated_length)
				{
					mprint("Premature end of file!\n");
					end_of_file = 1;
					return payload_read;
				}
				// Parse Replicated data
				unsigned char *replicate_position = asf_data_container.parsebuf;
				int media_object_size = 0;
				int presentation_time_millis = 0; // Payload ms time stamp
				int extsize = 0;
				// int32_t dwVersion = 0;
				// int32_t unknown = 0;
				int64_t rtStart = 0; // dvr-ms 100ns time stamp start
				int64_t rtEnd = 0;   // dvr-ms 100ns time stamp end

				// Always at least 8 bytes long, see 7.3.1
				media_object_size = *((uint16_t *)(asf_data_container.parsebuf));
				presentation_time_millis = *((uint16_t *)(asf_data_container.parsebuf + 4));
				replicate_position += 8;

				dbg_print(CCX_DMT_PARSE, "Stream# %d[%d] Media# %d Offset/Size: %d/%d\n",
					  asf_data_container.PayloadStreamNumber, asf_data_container.KeyFrame, asf_data_container.PayloadMediaNumber,
					  offset_media_length, media_object_size);

				// Loop over Payload Extension Systems
				for (int i = 0; i < asf_data_container.PayloadExtPTSEntry[asf_data_container.PayloadStreamNumber]; i++)
				{
					if (asf_data_container.PayloadExtSize[asf_data_container.PayloadStreamNumber][i] == 0xffff)
					{
						extsize = *((uint16_t *)(replicate_position + 0));
						replicate_position += 2;
					}
					else
					{
						extsize = asf_data_container.PayloadExtSize[asf_data_container.PayloadStreamNumber][i];
					}
					replicate_position += extsize;
					// printf("%2d. Ext. System - size: %d\n", i, extsize);
				}
				if (asf_data_container.PayloadExtPTSEntry[asf_data_container.PayloadStreamNumber] > 0)
				{
					// dwVersion = *((uint32_t*)(replicate_position+0));
					// unknown = *((uint32_t*)(replicate_position+4));
					rtStart = *((int64_t *)(replicate_position + 8));
					rtEnd = *((int64_t *)(replicate_position + 16));

					// printf("dwVersion: %d    unknown: 0x%04X\n", dwVersion, unknown);
				}

				// Avoid problems with unset PTS times
				if (rtStart == -1)
				{
					rtStart = 0;
					rtEnd = 0;
					dbg_print(CCX_DMT_PARSE, "dvr-ms time not defined\n");
				}

				// print_mstime uses a static buffer
				dbg_print(CCX_DMT_PARSE, "Stream #%d PacketTime: %s",
					  asf_data_container.PayloadStreamNumber, print_mstime_static(send_time));
				dbg_print(CCX_DMT_PARSE, "   PayloadTime: %s",
					  print_mstime_static(presentation_time_millis));
				dbg_print(CCX_DMT_PARSE, "   dvr-ms PTS: %s+%lld\n",
					  print_mstime_static(rtStart / 10000), (rtEnd - rtStart) / 10000);

				data_packet_length += replicated_length;

				// Only multiple payload packages have this value
				if (asf_data_container.MultiplePayloads != 0)
				{
					unsigned char plheader[4];

					result = buffered_read(ctx->demux_ctx, plheader, asf_data_container.PayloadLType);
					ctx->demux_ctx->past += result;
					asf_data_container.dobjectread += result;
					if (result != asf_data_container.PayloadLType)
					{
						mprint("Premature end of file!\n");
						end_of_file = 1;
						return payload_read;
					}
					asf_data_container.PayloadLength = asf_readval(plheader, asf_data_container.PayloadLType);
				}
				else
				{
					asf_data_container.PayloadLength = asf_data_container.PacketLength - data_packet_length - asf_data_container.PaddingLength;
				}
				dbg_print(CCX_DMT_PARSE, "Size - Replicated %d + Payload %d = %d\n",
					  replicated_length, asf_data_container.PayloadLength, replicated_length + asf_data_container.PayloadLength);

				// Remember the last video time stamp - only when captions are separate
				// from video stream.
				if (asf_data_container.PayloadStreamNumber == asf_data_container.StreamProperties.VideoStreamNumber && asf_data_container.StreamProperties.DecodeStreamNumber != asf_data_container.StreamProperties.VideoStreamNumber && offset_media_length == 0)
				{
					asf_data_container.StreamProperties.prevVideoStreamMS = asf_data_container.StreamProperties.currVideoStreamMS;
					asf_data_container.StreamProperties.currVideoStreamMS = asf_data_container.StreamProperties.VideoStreamMS;

					// Use presentation_time_millis (send_time might also work) when the
					// dvr-ms time stamp is not present.
					if (asf_data_container.PayloadExtPTSEntry[asf_data_container.PayloadStreamNumber] > 0)
					{
						// When rstart is not set, keep the previous value
						if (rtStart > 0)
							asf_data_container.StreamProperties.VideoStreamMS = (int)(rtStart / 10000);
					}
					else
					{
						// Add 1ms to avoid 0ms start times getting rejected
						asf_data_container.StreamProperties.VideoStreamMS = presentation_time_millis + 1;
					}
					// This checks if there is a video time jump in the timeline
					// between caption information.
					if (abs(asf_data_container.StreamProperties.currVideoStreamMS - asf_data_container.StreamProperties.prevVideoStreamMS) > 500)
					{
						// Found a more than 500ms jump in the video timeline
						asf_data_container.StreamProperties.VideoJump = 1;
						// This is remembered until the next caption block is
						// found.
					}
				}

				// Remember the PTS values
				if (asf_data_container.PayloadStreamNumber == asf_data_container.StreamProperties.DecodeStreamNumber && offset_media_length == 0)
				{
					asf_data_container.StreamProperties.prevDecodeStreamPTS = asf_data_container.StreamProperties.currDecodeStreamPTS;
					asf_data_container.StreamProperties.currDecodeStreamPTS = asf_data_container.StreamProperties.DecodeStreamPTS;

					// Use presentation_time_millis (send_time might also work) when the
					// dvr-ms time stamp is not present.
					if (asf_data_container.PayloadExtPTSEntry[asf_data_container.PayloadStreamNumber] > 0)
					{
						// When rstart is not set, keep the previous value
						if (rtStart > 0)
							asf_data_container.StreamProperties.DecodeStreamPTS = (int)(rtStart / 10000);
					}
					else
					{
						// Add 1ms to avoid 0ms start times getting rejected
						asf_data_container.StreamProperties.DecodeStreamPTS = presentation_time_millis + 1;
					}

					// Check the caption stream for jumps in the timeline. This
					// is needed when caption information is transmitted in a
					// different stream then the video information. (For example
					// NTSC recordings.) Otherwise a gap in the caption
					// information would look like a jump in the timeline.
					if (asf_data_container.StreamProperties.DecodeStreamNumber != asf_data_container.StreamProperties.VideoStreamNumber)
					{
						// Check if there is a gap larger than 500ms.
						if (asf_data_container.StreamProperties.currDecodeStreamPTS - asf_data_container.StreamProperties.prevDecodeStreamPTS > 500)
						{
							// Found more than 500ms since the previous caption,
							// now check the video timeline. If there was a also
							// a jump this needs synchronizing, otherwise it was
							// just a gap in the captions.
							if (!asf_data_container.StreamProperties.VideoJump)
								ccx_common_timing_settings.disable_sync_check = 1;
							else
								ccx_common_timing_settings.disable_sync_check = 0;
						}
						asf_data_container.StreamProperties.VideoJump = 0;
					}

					// Remember, we are reading the previous package.
					data->pts = asf_data_container.StreamProperties.currDecodeStreamPTS * (MPEG_CLOCK_FREQ / 1000);
				}
			}

			// A new media number. The old "object" finished, we stop here to
			// continue later.
			// To continue later we need to remember:
			// NumberOfPayloads
			// payloadcur
			// PayloadLength
			// PaddingLength

			// Now, the next loop is no reentry anymore:
			reentry = 0;

			// Video streams need several packages to complete a PES.  Leave
			// the loop when the next package starts a new Media Object.
			if (current_media_number != 0xFFFFFFFF // Is initialized
			    && asf_data_container.PayloadStreamNumber == asf_data_container.StreamProperties.DecodeStreamNumber && asf_data_container.PayloadMediaNumber != current_media_number)
			{
				if (asf_data_container.StreamProperties.DecodeStreamNumber == asf_data_container.StreamProperties.CaptionStreamNumber)
					dbg_print(CCX_DMT_PARSE, "\nCaption stream object");
				else
					dbg_print(CCX_DMT_PARSE, "\nVideo stream object");
				dbg_print(CCX_DMT_PARSE, " read with PTS: %s\n",
					  print_mstime_static(asf_data_container.StreamProperties.currDecodeStreamPTS));

				// Enough for now
				enough = 1;
				break;
			}

			// Read it!!
			if (asf_data_container.PayloadStreamNumber == asf_data_container.StreamProperties.DecodeStreamNumber)
			{
				current_media_number = asf_data_container.PayloadMediaNumber; // Remember current value

				// Read the data
				dbg_print(CCX_DMT_PARSE, "Reading Stream #%d data ...\n", asf_data_container.PayloadStreamNumber);

				data->stream_pid = asf_data_container.StreamProperties.DecodeStreamNumber;
				data->program_number = 1;
				data->codec = CCX_CODEC_ATSC_CC;

				int want = (long)((BUFSIZE - data->len) > asf_data_container.PayloadLength ? asf_data_container.PayloadLength : (BUFSIZE - data->len));
				if (want < (long)asf_data_container.PayloadLength)
					fatal(CCX_COMMON_EXIT_BUG_BUG, "Buffer size too small for ASF payload!\nPlease file a bug report!\n");
				result = buffered_read(ctx->demux_ctx, data->buffer + data->len, want);
				payload_read += (int)result;
				data->len += result;
				ctx->demux_ctx->past += result;
				if (result != asf_data_container.PayloadLength)
				{
					mprint("Premature end of file!\n");
					end_of_file = 1;
					return payload_read;
				}
				asf_data_container.dobjectread += asf_data_container.PayloadLength;
			}
			else
			{
				// Skip non-cc data
				dbg_print(CCX_DMT_PARSE, "Skipping Stream #%d data ...\n", asf_data_container.PayloadStreamNumber);
				result = buffered_skip(ctx->demux_ctx, (int)asf_data_container.PayloadLength);
				ctx->demux_ctx->past += result;
				if (result != asf_data_container.PayloadLength)
				{
					mprint("Premature end of file!\n");
					end_of_file = 1;
					return payload_read;
				}
				asf_data_container.dobjectread += result;
			}

			asf_data_container.payloadcur++;
		}
		if (enough)
			break;

		// Skip padding bytes
		dbg_print(CCX_DMT_PARSE, "Skip %d padding\n", asf_data_container.PaddingLength);
		result = buffered_skip(ctx->demux_ctx, (long)asf_data_container.PaddingLength);
		ctx->demux_ctx->past += result;
		if (result != asf_data_container.PaddingLength)
		{
			mprint("Premature end of file!\n");
			end_of_file = 1;
			return payload_read;
		}
		asf_data_container.dobjectread += result;

		asf_data_container.datapacketcur++;
		dbg_print(CCX_DMT_PARSE, "Bytes read: %lld/%lld\n", asf_data_container.dobjectread, asf_data_container.DataObjectSize);
	}

	if (asf_data_container.datapacketcur == asf_data_container.TotalDataPackets)
	{
		dbg_print(CCX_DMT_PARSE, "\nWe read the last packet!\n\n");

		// Skip the rest of the file
		dbg_print(CCX_DMT_PARSE, "Skip the rest: %d\n", (int)(asf_data_container.FileSize - asf_data_container.HeaderObjectSize - asf_data_container.DataObjectSize));
		result = buffered_skip(ctx->demux_ctx, (int)(asf_data_container.FileSize - asf_data_container.HeaderObjectSize - asf_data_container.DataObjectSize));
		ctx->demux_ctx->past += result;
		// Don not set end_of_file (although it is true) as this would
		// produce an premature end error.
		// end_of_file=1;

		// parsebuf is freed automatically when the program closes.
	}

	if (!payload_read)
		return CCX_EOF;
	return payload_read;
}
