#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "asf_constants.h"

// Indicate first / subsequent calls to asf_getmoredata()
int firstcall;

asf_data asf_data_container;

// For ASF parsing
// 0, 1, 2, 3 means none, BYTE, WORD, DWORD
#define ASF_TypeLength(A) ( A == 3 ? 4 : A )


uint32_t asf_readval(void *val, int ltype)
{
	uint32_t rval;

	switch (ltype)
	{
		case 0:
			rval = 0;
			break;
		case 1:
			rval = *((uint8_t*)val);
			break;
		case 2:
			rval = *((uint16_t*)val);
			break;
		case 4:
			rval = *((uint32_t*)val);
			break;
		default:
			fatal (CCX_COMMON_EXIT_BUG_BUG, "Wrong type ...\n");
			break;
	}
	return rval;
}

char *guidstr(void *val)
{
	static char sbuf[40];

	sprintf(sbuf,"%08lX-%04X-%04X-",
			(long)*((uint32_t*)((char*)val+0)),
			(int)*((uint16_t*)((char*)val+4)),
			(int)*((uint16_t*)((char*)val+6)));
	for(int ii=0; ii<2; ii++)
		sprintf(sbuf+19+ii*2,"%02X-",*((unsigned char*)val+8+ii));
	for(int ii=0; ii<6; ii++)
		sprintf(sbuf+24+ii*2,"%02X",*((unsigned char*)val+10+ii));

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
LLONG asf_getmoredata(struct lib_ccx_ctx *ctx)
{
	int enough = 0;
	int payload_read = 0;

	// The fist call to this function (per file) is indicated with
	// firstcall == 1
	// Except for the first call of this function we will reenter
	// the Data Packet loop below.
	int reentry = 1;

	// Variables for Header Object
	int64_t DataPacketsCount = 0;
	int BroadcastFlag = 0;
	int SeekableFlag = 0;
	uint32_t MinPacketSize = 0;
	uint32_t MaxPacketSize = 0;

	// Data Object Loop
	int datapacketlength = 0; // Collect the read header bytes

	// Payload parsing information
	int SequenceType = 0; // ASF
	int PaddingLType = 0; // ASF
	uint32_t Sequence = 0;
	uint32_t SendTime = 0;

	int payloadparsersize = 0; // Infered (PacketLType + SequenceType + PaddingLType + 6);

	uint32_t OffsetMediaLength = 0; // ASF
	uint32_t ReplicatedLength = 0; // ASF

	// Last media number. Used to determine a new PES, mark uninitialized.
	uint32_t curmedianumber = 0xFFFFFFFF;

	unsigned char *curpos;
	int64_t getbytes;

	// Read Header Object and the Top-level Data Object header only once
	if(firstcall)
	{
		asf_data_container = (asf_data) {
			.parsebuf = (unsigned char*)malloc(1024),
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
					.VideoJump = 0
				},
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
				.PaddingLength = 0
		};
		// Initialize the Payload Extension System
		for(int stream=0; stream<STREAMNUM; stream++)
		{
			for(int payext=0; payext<PAYEXTNUM; payext++)
			{
				asf_data_container.PayloadExtSize[stream][payext] = 0;
			}
			asf_data_container.PayloadExtPTSEntry[stream] = -1;
		}

		buffered_read(ctx, asf_data_container.parsebuf,30);
		ctx->past+=result;
		if (result!=30)
		{
			mprint("Premature end of file!\n");
			end_of_file=1;
			return payload_read;
		}

		// Expecting ASF header
		if (!memcmp(asf_data_container.parsebuf, ASF_HEADER, 16))
		{
			dbg_print(CCX_DMT_PARSE, "\nASF header\n");
		}
		else
		{
			fatal(EXIT_MISSING_ASF_HEADER, "Missing ASF header. Abort.\n");
		}
		asf_data_container.HeaderObjectSize = *((int64_t*)(asf_data_container.parsebuf + 16));
		dbg_print(CCX_DMT_PARSE, "Length: %lld\n", asf_data_container.HeaderObjectSize);
		dbg_print(CCX_DMT_PARSE,"\nNumber of header objects: %ld\n",
				(long)*((uint32_t*)(asf_data_container.parsebuf + 24)));


		if (asf_data_container.HeaderObjectSize > asf_data_container.parsebufsize) {
			asf_data_container.parsebuf = (unsigned char*)realloc(asf_data_container.parsebuf, (size_t)asf_data_container.HeaderObjectSize);
			if (!asf_data_container.parsebuf)
				fatal(EXIT_NOT_ENOUGH_MEMORY, "Out of memory");
			asf_data_container.parsebufsize = (long)asf_data_container.HeaderObjectSize;
		}

		curpos = asf_data_container.parsebuf + 30;
		getbytes = asf_data_container.HeaderObjectSize - 30;

		buffered_read(ctx, curpos, (int) getbytes);
		ctx->past+=result;
		if (result!=getbytes)
		{
			mprint("Premature end of file!\n");
			end_of_file=1;
			return payload_read;
		}

		dbg_print(CCX_DMT_PARSE, "Reading header objects\n");

		while (curpos < asf_data_container.parsebuf + asf_data_container.HeaderObjectSize)
		{
			int64_t hpobjectsize = *((int64_t*)(curpos+16)); // Local

			if( !memcmp(curpos, ASF_FILE_PROPERTIES, 16 ) )
			{
				// Mandatory Object, only one.
				dbg_print(CCX_DMT_PARSE, "\nFile Properties Object     (size: %lld)\n", hpobjectsize);

				asf_data_container.FileSize = *((int64_t*)(curpos + 40));
				DataPacketsCount = *((int64_t*)(curpos+56));
				BroadcastFlag = 0x1 & curpos[88];
				SeekableFlag = 0x2 & curpos[88];
				MinPacketSize = *((uint32_t*)(curpos+92));
				MaxPacketSize = *((uint32_t*)(curpos+96));

				dbg_print(CCX_DMT_PARSE, "FileSize: %lld   Packet count: %lld\n", asf_data_container.FileSize, DataPacketsCount);
				dbg_print(CCX_DMT_PARSE, "Broadcast: %d - Seekable: %d\n", BroadcastFlag, SeekableFlag);
				dbg_print(CCX_DMT_PARSE, "MiDPS: %d   MaDPS: %d\n", MinPacketSize, MaxPacketSize);

			}
			else if( !memcmp(curpos,ASF_STREAM_PROPERTIES, 16 ) )
			{
				dbg_print(CCX_DMT_PARSE, "\nStream Properties Object     (size: %lld)\n", hpobjectsize);
				if( !memcmp(curpos+24, ASF_VIDEO_MEDIA, 16 ) )
				{
					asf_data_container.StreamProperties.VideoStreamNumber = *(curpos + 72) & 0x7F;
					dbg_print(CCX_DMT_PARSE, "Stream Type: ASF_Video_Media\n");
					dbg_print(CCX_DMT_PARSE, "Video Stream Number: %d\n", asf_data_container.StreamProperties.VideoStreamNumber);

				}
				else if( !memcmp(curpos+24, ASF_AUDIO_MEDIA, 16 ) )
				{
					asf_data_container.StreamProperties.AudioStreamNumber = *(curpos + 72) & 0x7F;
					dbg_print(CCX_DMT_PARSE, "Stream Type: ASF_Audio_Media\n");
					dbg_print(CCX_DMT_PARSE, "Audio Stream Number: %d\n", asf_data_container.StreamProperties.AudioStreamNumber);
				}
				else
				{
					dbg_print(CCX_DMT_PARSE, "Stream Type: %s\n",
							guidstr(curpos+24));
					dbg_print(CCX_DMT_PARSE, "Stream Number: %d\n", *(curpos+72) & 0x7F );
				}
			}
			else if( !memcmp(curpos,ASF_HEADER_EXTENSION, 16 ) )
			{
				dbg_print(CCX_DMT_PARSE, "\nHeader Extension Object     (size: %lld)\n", hpobjectsize);

				int32_t HeaderExtensionDataSize = *((uint32_t*)(curpos+42));
				// Process Header Extension Data
				if ( HeaderExtensionDataSize )
				{
					unsigned char *hecurpos=curpos+46;

					if ( HeaderExtensionDataSize != hpobjectsize - 46 )
						fatal(EXIT_NOT_CLASSIFIED, "HeaderExtensionDataSize size wrong");

					dbg_print(CCX_DMT_PARSE, "\nReading Header Extension Sub-Objects\n");
					while( hecurpos < curpos+46 + HeaderExtensionDataSize )
					{
						int64_t heobjectsize = *((int64_t*)(hecurpos+16)); // Local

						if( !memcmp(hecurpos,ASF_EXTENDED_STREAM_PROPERTIES, 16 ) )
						{
							dbg_print(CCX_DMT_PARSE, "\nExtended Stream Properties Object     (size: %lld)\n", heobjectsize);
							int StreamNumber = *((uint16_t*)(hecurpos+72));
							int StreamNameCount = *((uint16_t*)(hecurpos+84));
							int PayloadExtensionSystemCount = *((uint16_t*)(hecurpos+86));

							unsigned char *estreamproppos=hecurpos+88;

							int streamnamelength;

							dbg_print(CCX_DMT_PARSE, "Stream Number: %d NameCount: %d  ESCount: %d\n",
									StreamNumber, StreamNameCount, PayloadExtensionSystemCount);

							if ( StreamNumber >= STREAMNUM )
								fatal(CCX_COMMON_EXIT_BUG_BUG, "STREAMNUM too small. Send bug report!/n");

							for(int i=0; i<StreamNameCount; i++)
							{
								dbg_print(CCX_DMT_PARSE,"%2d. Stream Name Field\n",i);
								streamnamelength=*((uint16_t*)(estreamproppos+2));
								estreamproppos+=4+streamnamelength;
							}
							int extensionsystemdatasize;
							int extensionsysteminfolength;

							if ( PayloadExtensionSystemCount > PAYEXTNUM )
								fatal(CCX_COMMON_EXIT_BUG_BUG, "PAYEXTNUM too small. Send bug report!/n");

							for(int i=0; i<PayloadExtensionSystemCount; i++)
							{
								extensionsystemdatasize = *((uint16_t*)(estreamproppos+16));
								extensionsysteminfolength = *((uint32_t*)(estreamproppos+18));

								asf_data_container.PayloadExtSize[StreamNumber][i] = extensionsystemdatasize;

								dbg_print(CCX_DMT_PARSE,"%2d. Payload Extension GUID: %s Size %d Info Length %d\n",
										i,guidstr(estreamproppos+0),
										extensionsystemdatasize,
										extensionsysteminfolength);

								// For DVR-MS presentation timestamp
								if( !memcmp(estreamproppos, DVRMS_PTS, 16 ) )
								{
									dbg_print(CCX_DMT_PARSE, "Found DVRMS_PTS\n");
									asf_data_container.PayloadExtPTSEntry[StreamNumber] = i;
								}

								estreamproppos+=22+extensionsysteminfolength;
							}

							// Now, there can be a Stream Properties Object.  The only way to
							// find out is to check if there are bytes left in the current
							// object.
							if ( (estreamproppos - hecurpos) < heobjectsize )
							{
								int64_t spobjectsize = *((int64_t*)(estreamproppos+16)); // Local
								if( memcmp(estreamproppos, ASF_STREAM_PROPERTIES, 16 ) )
									fatal(EXIT_NOT_CLASSIFIED, "Stream Properties Object expected\n");

								if( !memcmp(estreamproppos+24, ASF_VIDEO_MEDIA, 16 ) )
								{
									dbg_print(CCX_DMT_PARSE, "Stream Type: ASF_Video_Media (size: %lld)\n",
											spobjectsize);
									asf_data_container.StreamProperties.VideoStreamNumber = StreamNumber;
								}
								else if( !memcmp(estreamproppos+24, ASF_AUDIO_MEDIA, 16 ) )
								{
									dbg_print(CCX_DMT_PARSE, "Stream Type: ASF_Audio_Media (size: %lld)\n",
											spobjectsize);
									asf_data_container.StreamProperties.AudioStreamNumber = StreamNumber;
								}
								else if( !memcmp(estreamproppos+24, ASF_BINARY_MEDIA, 16 ) )
								{
									// dvr-ms files identify audio streams as binary streams
									// but use the "major media type" accordingly to identify
									// the steam.  (There might be other audio identifiers.)
									if( !memcmp(estreamproppos+78, DVRMS_AUDIO, 16 ) ) {
										dbg_print(CCX_DMT_PARSE, "Binary media: DVR-MS Audio Stream (size: %lld)\n",
												spobjectsize);
									}
									else if( !memcmp(estreamproppos+78, DVRMS_NTSC, 16 ) )
									{
										dbg_print(CCX_DMT_PARSE, "Binary media: NTSC captions (size: %lld)\n",
												spobjectsize);
										asf_data_container.StreamProperties.CaptionStreamNumber = StreamNumber;
										asf_data_container.StreamProperties.CaptionStreamStyle = 1;

									}
									else if( !memcmp(estreamproppos+78, DVRMS_ATSC, 16 ) )
									{
										dbg_print(CCX_DMT_PARSE, "Binary media: ATSC captions (size: %lld)\n",
												spobjectsize);
										asf_data_container.StreamProperties.CaptionStreamNumber = StreamNumber;
										asf_data_container.StreamProperties.CaptionStreamStyle = 2;
									}
									else
									{
										dbg_print(CCX_DMT_PARSE, "Binary media: Major Media Type GUID: %s (size: %lld)\n",
												guidstr(estreamproppos+78), spobjectsize);
									}
								}
								else
								{
									dbg_print(CCX_DMT_PARSE, "Unknown Type GUID: %s (size: %lld)\n",
											guidstr(estreamproppos+24), spobjectsize);
								}
							}
							else
							{
								dbg_print(CCX_DMT_PARSE,"No Stream Properties Object\n");
							}

						}
						else if( !memcmp(hecurpos,ASF_METADATA, 16 ) )
						{
							dbg_print(CCX_DMT_PARSE, "\nMetadata Object     (size: %lld)\n", heobjectsize);
						}
						else if( !memcmp(hecurpos,ASF_METADATA_LIBRARY, 16 ) )
						{
							dbg_print(CCX_DMT_PARSE, "\nMetadata Library Object     (size: %lld)\n", heobjectsize);
						}
						else if( !memcmp(hecurpos,ASF_COMPATIBILITY2, 16 ) )
						{
							dbg_print(CCX_DMT_PARSE, "\nCompatibility Object 2     (size: %lld)\n", heobjectsize);
						}
						else if( !memcmp(hecurpos,ASF_PADDING, 16 ) )
						{
							dbg_print(CCX_DMT_PARSE, "\nPadding Object     (size: %lld)\n", heobjectsize);
						}
						else
						{
							dbg_print(CCX_DMT_PARSE, "\nGUID: %s  size: %lld\n",
									guidstr(hecurpos), heobjectsize);
							dump(CCX_DMT_PARSE, hecurpos, 16, 0, 0);
						}

						hecurpos += heobjectsize;
					}
					if (hecurpos - (curpos+46) != HeaderExtensionDataSize)
						fatal(EXIT_NOT_CLASSIFIED, "HE Parsing problem: read bytes %ld != header length %lld\nAbort!\n",
								(long)(hecurpos - (curpos+46)), HeaderExtensionDataSize);
				}
				dbg_print(CCX_DMT_PARSE, "\nHeader Extension Object  -  End\n");

			}
			else if( !memcmp(curpos,ASF_CONTENT_DESCRIPTION, 16 ) )
			{
				dbg_print(CCX_DMT_PARSE, "\nContend Description Object     (size: %lld)\n", hpobjectsize);
			}
			else if( !memcmp(curpos,ASF_EXTENDED_CONTENT_DESCRIPTION, 16 ) )
			{
				dbg_print(CCX_DMT_PARSE, "\nExtended Contend Description Object     (size: %lld)\n", hpobjectsize);

				int ContentDescriptorsCount = *((uint16_t*)(curpos+24));
				unsigned char *econtentpos=curpos+26;
				int DescriptorNameLength;
				int DescriptorValueDataType;
				int DescriptorValueLength;
				unsigned char *edescval;

				for(int i=0; i<ContentDescriptorsCount; i++)
				{
					DescriptorNameLength = *((uint16_t*)(econtentpos));
					DescriptorValueDataType = *((uint16_t*)(econtentpos+2+DescriptorNameLength));
					DescriptorValueLength = *((uint16_t*)(econtentpos+4+DescriptorNameLength));
					edescval = econtentpos+6+DescriptorNameLength;

					dbg_print(CCX_DMT_PARSE, "%3d. %ls = ",i,(wchar_t*)(econtentpos+2));
					switch(DescriptorValueDataType)
					{
						case 0: // Unicode string
							dbg_print(CCX_DMT_PARSE, "%ls (Unicode)\n",(wchar_t*)edescval);
							break;
						case 1: // byte string
							dbg_print(CCX_DMT_PARSE, ":");
							for(int ii=0; ii<DescriptorValueLength && ii<9; ii++)
							{
								dbg_print(CCX_DMT_PARSE, "%02X:",*((unsigned char*)(edescval+ii)));
							}
							if (DescriptorValueLength>8)
								dbg_print(CCX_DMT_PARSE, "skipped %d more",DescriptorValueLength-8);
							dbg_print(CCX_DMT_PARSE, " (BYTES)\n");
							break;
						case 2: // BOOL
							dbg_print(CCX_DMT_PARSE, "%d (BOOL)\n",*((int32_t*)edescval));
							break;
						case 3: // DWORD
							dbg_print(CCX_DMT_PARSE, "%u (DWORD)\n",*((uint32_t*)edescval));
							break;
						case 4: // QWORD
							dbg_print(CCX_DMT_PARSE, "%llu (QWORD)\n",*((uint64_t*)edescval));
							break;
						case 5: // WORD
							dbg_print(CCX_DMT_PARSE, "%u (WORD)\n",(int)*((uint16_t*)edescval));
							break;
						default:
							fatal(CCX_COMMON_EXIT_BUG_BUG, "Wrong type ...\n");
							break;
					}

					if(!memcmp(econtentpos+2, L"WM/VideoClosedCaptioning"
								,DescriptorNameLength))
					{
						// This flag would be really usefull if it would be
						// reliable - it isn't.
						asf_data_container.VideoClosedCaptioningFlag = *((int32_t*)edescval);
						dbg_print(CCX_DMT_PARSE, "Found WM/VideoClosedCaptioning flag: %d\n",
								asf_data_container.VideoClosedCaptioningFlag);
					}

					econtentpos+=6+DescriptorNameLength+DescriptorValueLength;
				}
			}
			else if( !memcmp(curpos,ASF_STREAM_BITRATE_PROPERTIES, 16 ) )
			{
				dbg_print(CCX_DMT_PARSE, "\nStream Bitrate Properties Object     (size: %lld)\n", hpobjectsize);
			}
			else
			{
				dbg_print(CCX_DMT_PARSE, "\nGUID: %s  size: %lld\n",
						guidstr(curpos), hpobjectsize);
				dump(CCX_DMT_PARSE, curpos, 16, 0, 0);
			}

			curpos += hpobjectsize;
		}
		if (curpos - asf_data_container.parsebuf != asf_data_container.HeaderObjectSize)
			fatal(EXIT_NOT_CLASSIFIED, "Parsing problem: read bytes %ld != header length %lld\nAbort!\n",
					(long)(curpos - asf_data_container.parsebuf), asf_data_container.HeaderObjectSize);

		if (asf_data_container.StreamProperties.VideoStreamNumber == 0)
			fatal(EXIT_NOT_CLASSIFIED, "No Video Stream Properties Object found.  Unable to continue ...\n");

		// Wouldn't it be nice if  VideoClosedCaptioningFlag  would be usable, unfortunately
		// it is not reliable.

		// Now decide where we are going to expect the captions
		ccx_bufferdatatype = CCX_PES; // Except for NTSC captions
		if (asf_data_container.StreamProperties.CaptionStreamNumber > 0
				&& (asf_data_container.StreamProperties.CaptionStreamStyle == 1 ||
					(asf_data_container.StreamProperties.CaptionStreamStyle == 2 && !ccx_options.wtvconvertfix)))
		{
			mprint("\nNote: If you converted a WTV into a DVR-MS and CCEextractor finds no captions,  try passing -wtvconvertfix to work around bug in the conversion process.");
		}
		if (asf_data_container.StreamProperties.CaptionStreamNumber > 0
				&& (asf_data_container.StreamProperties.CaptionStreamStyle == 1 ||
					(asf_data_container.StreamProperties.CaptionStreamStyle == 2 && ccx_options.wtvconvertfix)))
		{
			//if (debug_parse)
			mprint("\nNTSC captions in stream #%d\n\n", asf_data_container.StreamProperties.CaptionStreamNumber);
			ccx_bufferdatatype = CCX_RAW;
			asf_data_container.StreamProperties.DecodeStreamNumber = asf_data_container.StreamProperties.CaptionStreamNumber;
		}
		else if (asf_data_container.StreamProperties.CaptionStreamNumber > 0 && asf_data_container.StreamProperties.CaptionStreamStyle == 2)
		{
			//if (debug_parse)
			mprint("\nATSC captions (probably) in stream #%d - Decode the video stream #%d instead\n\n",
					asf_data_container.StreamProperties.CaptionStreamNumber, asf_data_container.StreamProperties.VideoStreamNumber);
			asf_data_container.StreamProperties.DecodeStreamNumber = asf_data_container.StreamProperties.VideoStreamNumber;
		}
		else
		{
			asf_data_container.StreamProperties.DecodeStreamNumber = asf_data_container.StreamProperties.VideoStreamNumber;
			//if (debug_parse)
			mprint("\nAssume CC info in video stream #%d (No caption stream found)\n\n",
					asf_data_container.StreamProperties.DecodeStreamNumber);
		}

		// When reading "Payload parsing information" it occured that "Packet Lenght"
		// was not set (Packet Length Type 00) and for "Single Payloads" this means
		// the Payload Data Length cannot be infered.  Use MinPacketSize, MaxPacketSize instead.
		if ((MinPacketSize > 0) && (MinPacketSize == MaxPacketSize))
			asf_data_container.PacketSize = MinPacketSize;

		// Now the Data Object, except for the packages
		buffered_read(ctx, asf_data_container.parsebuf, 50); // No realloc needed.
		ctx->past+=result;
		if (result!=50)
		{
			mprint("Premature end of file!\n");
			end_of_file=1;
			return payload_read;
		}

		// Expecting ASF Data object
		if (!memcmp(asf_data_container.parsebuf, ASF_DATA, 16))
		{
			dbg_print(CCX_DMT_PARSE, "\nASF Data Object\n");
		}
		else
		{
			fatal(EXIT_NOT_CLASSIFIED, "Missing ASF Data Object. Abort.\n");
		}

		asf_data_container.DataObjectSize = *((int64_t*)(asf_data_container.parsebuf + 16));
		asf_data_container.TotalDataPackets = *((uint32_t*)(asf_data_container.parsebuf + 40));
		dbg_print(CCX_DMT_PARSE, "Size: %lld\n", asf_data_container.DataObjectSize);
		dbg_print(CCX_DMT_PARSE, "Number of data packets: %ld\n", (long)asf_data_container.TotalDataPackets);


		reentry = 0; // Make sure we read the Data Packet Headers
	} // End of if (firstcall)
	firstcall = 0;

	// Start loop over Data Packets
	while (asf_data_container.datapacketcur < asf_data_container.TotalDataPackets && !enough)
	{
		// Skip reading the headers the first time when reentering the loop
		if(!reentry)
		{
			int ecinfo = 0;

			datapacketlength = 0;

			dbg_print(CCX_DMT_PARSE, "\nReading packet %d/%d\n", asf_data_container.datapacketcur + 1, asf_data_container.TotalDataPackets);

			// First packet
			buffered_read(ctx, asf_data_container.parsebuf, 1); // No realloc needed.
			ctx->past+=result;
			asf_data_container.dobjectread += result;
			if (result!=1)
			{
				mprint("Premature end of file!\n");
				end_of_file=1;
				return payload_read;
			}
			datapacketlength+=1;
			if (*asf_data_container.parsebuf & 0x80)
			{
				int ecdatalength = *asf_data_container.parsebuf & 0x0F; // Small, no realloc needed
				if (*asf_data_container.parsebuf & 0x60)
				{
					fatal(EXIT_NOT_CLASSIFIED, "Error Correction Length Type not 00 - reserved - aborting ...\n");
				}
				buffered_read(ctx, asf_data_container.parsebuf + 1, ecdatalength);
				ctx->past+=result;
				asf_data_container.dobjectread += result;
				if (result!=ecdatalength)
				{
					mprint("Premature end of file!\n");
					end_of_file=1;
					return payload_read;
				}
				datapacketlength+=ecdatalength;
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
			buffered_read(ctx, asf_data_container.parsebuf + ecinfo, 2 - ecinfo); // No realloc needed
			ctx->past+=result;
			asf_data_container.dobjectread += result;
			if (result!=2)
			{
				mprint("Premature end of file!\n");
				end_of_file=1;
				return payload_read;
			}
			datapacketlength+=2;

			asf_data_container.MultiplePayloads = *asf_data_container.parsebuf & 0x01;

			SequenceType = (*asf_data_container.parsebuf >> 1) & 0x03;
			SequenceType = ASF_TypeLength(SequenceType);

			PaddingLType = (*asf_data_container.parsebuf >> 3) & 0x03;
			PaddingLType = ASF_TypeLength(PaddingLType);

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

			payloadparsersize = asf_data_container.PacketLType + SequenceType + PaddingLType + 6;

			buffered_read(ctx, asf_data_container.parsebuf + 2, payloadparsersize); // No realloc needed
			ctx->past+=result;
			asf_data_container.dobjectread += result;
			if (result!=payloadparsersize)
			{
				mprint("Premature end of file!\n");
				end_of_file=1;
				return payload_read;
			}
			datapacketlength+=payloadparsersize;

			asf_data_container.PacketLength = asf_readval(asf_data_container.parsebuf + 2, asf_data_container.PacketLType);
			Sequence = asf_readval(asf_data_container.parsebuf + 2 + asf_data_container.PacketLType, SequenceType);
			asf_data_container.PaddingLength = asf_readval(asf_data_container.parsebuf + 2 + asf_data_container.PacketLType + SequenceType, PaddingLType);
			// Data Packet ms time stamp
			SendTime = *((uint32_t*)(asf_data_container.parsebuf + 2 + asf_data_container.PacketLType + SequenceType + PaddingLType));

			// If Packet Length is not set use global setting if possible
			if (asf_data_container.PacketLength == 0)
			{
				asf_data_container.PacketLength = asf_data_container.PacketSize;
				// For multiple payloads we can get away without a given
				// Packet Lenght as individual payload length are given
				if (asf_data_container.PacketLength == 0 && asf_data_container.MultiplePayloads == 0)
					fatal(EXIT_NOT_CLASSIFIED, "No idea how long the data packet will be. Abort.\n");
			}

			dbg_print(CCX_DMT_PARSE, "Lengths - Packet: %d / Sequence %d / Padding %d\n",
					asf_data_container.PacketLength, Sequence, asf_data_container.PaddingLength);

			asf_data_container.PayloadLType = 0; // Payload Length Type. <>0 for multiple payloads
			asf_data_container.PayloadLength = 0; // Payload Length (for multiple payloads)
			asf_data_container.NumberOfPayloads = 1; // Number of payloads.

			if (asf_data_container.MultiplePayloads != 0)
			{
				unsigned char plheader[1];

				buffered_read(ctx, plheader, 1);
				ctx->past+=result;
				asf_data_container.dobjectread += result;
				if (result!=1)
				{
					mprint("Premature end of file!\n");
					end_of_file=1;
					return payload_read;
				}
				datapacketlength+=1;
				asf_data_container.PayloadLType = (*plheader >> 6) & 0x03;
				asf_data_container.PayloadLType = ASF_TypeLength(asf_data_container.PayloadLType);

				// Number of payloads
				asf_data_container.NumberOfPayloads = *plheader & 0x3F;
			}
			asf_data_container.payloadcur = 0;
		}
		else
		{   // Rely on
			// NumberOfPayloads, payloadcur, PayloadLength, PaddingLength
			// and related variables being kept as static variables to be
			// able to reenter the loop here.
			dbg_print(CCX_DMT_PARSE, "\nReentry into asf_getmoredata()\n");
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

				int payloadheadersize = 1 + asf_data_container.MediaNumberLType + asf_data_container.OffsetMediaLType + asf_data_container.ReplicatedLType;

				buffered_read(ctx, asf_data_container.parsebuf, payloadheadersize); // No realloc needed
				ctx->past+=result;
				asf_data_container.dobjectread += result;
				if (result!=payloadheadersize)
				{
					mprint("Premature end of file!\n");
					end_of_file=1;
					return payload_read;
				}
				datapacketlength+=payloadheadersize;

				asf_data_container.PayloadStreamNumber = *asf_data_container.parsebuf & 0x7F;
				asf_data_container.KeyFrame = (*asf_data_container.parsebuf & 0x80) && 1;

				asf_data_container.PayloadMediaNumber = asf_readval(asf_data_container.parsebuf + 1, asf_data_container.MediaNumberLType);
				OffsetMediaLength = asf_readval(asf_data_container.parsebuf + 1 + asf_data_container.MediaNumberLType, asf_data_container.OffsetMediaLType);
				ReplicatedLength = asf_readval(asf_data_container.parsebuf + 1 + asf_data_container.MediaNumberLType + asf_data_container.OffsetMediaLType, asf_data_container.ReplicatedLType);

				if (ReplicatedLength == 1)
					fatal(EXIT_NOT_CLASSIFIED, "Cannot handle compressed data...\n");

				if ((long)ReplicatedLength > asf_data_container.parsebufsize)
				{
					asf_data_container.parsebuf = (unsigned char*)realloc(asf_data_container.parsebuf, ReplicatedLength);
					if (!asf_data_container.parsebuf)
						fatal(EXIT_NOT_ENOUGH_MEMORY, "Out of memory");
					asf_data_container.parsebufsize = ReplicatedLength;
				}
				buffered_read(ctx, asf_data_container.parsebuf, (long)ReplicatedLength);
				ctx->past+=result;
				asf_data_container.dobjectread += result;
				if (result!=ReplicatedLength)
				{
					mprint("Premature end of file!\n");
					end_of_file=1;
					return payload_read;
				}
				// Parse Replicated data
				unsigned char *reppos = asf_data_container.parsebuf;
				int MediaObjectSize = 0;
				int PresentationTimems = 0; //Payload ms time stamp
				int extsize = 0;
				// int32_t dwVersion = 0;
				// int32_t unknown = 0;
				int64_t rtStart = 0; // dvr-ms 100ns time stamp start
				int64_t rtEnd = 0; // dvr-ms 100ns time stamp end

				// Always at least 8 bytes long, see 7.3.1
				MediaObjectSize = *((uint16_t*)(asf_data_container.parsebuf));
				PresentationTimems = *((uint16_t*)(asf_data_container.parsebuf + 4));
				reppos += 8;

				dbg_print(CCX_DMT_PARSE, "Stream# %d[%d] Media# %d Offset/Size: %d/%d\n",
						asf_data_container.PayloadStreamNumber, asf_data_container.KeyFrame, asf_data_container.PayloadMediaNumber,
						OffsetMediaLength, MediaObjectSize);

				// Loop over Payload Extension Systems
				for (int i = 0; i<asf_data_container.PayloadExtPTSEntry[asf_data_container.PayloadStreamNumber]; i++)
				{
					if (asf_data_container.PayloadExtSize[asf_data_container.PayloadStreamNumber][i] == 0xffff)
					{
						extsize = *((uint16_t*)(reppos+0));
						reppos += 2;
					}
					else
					{
						extsize = asf_data_container.PayloadExtSize[asf_data_container.PayloadStreamNumber][i];
					}
					reppos += extsize;
					//printf("%2d. Ext. System - size: %d\n", i, extsize);
				}
				if (asf_data_container.PayloadExtPTSEntry[asf_data_container.PayloadStreamNumber] > 0)
				{
					// dwVersion = *((uint32_t*)(reppos+0));
					// unknown = *((uint32_t*)(reppos+4));
					rtStart = *((int64_t*)(reppos+8));
					rtEnd = *((int64_t*)(reppos+16));

					//printf("dwVersion: %d    unknown: 0x%04X\n", dwVersion, unknown);
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
						asf_data_container.PayloadStreamNumber, print_mstime(SendTime));
				dbg_print(CCX_DMT_PARSE,"   PayloadTime: %s",
						print_mstime(PresentationTimems));
				dbg_print(CCX_DMT_PARSE,"   dvr-ms PTS: %s+%lld\n",
						print_mstime(rtStart/10000), (rtEnd-rtStart)/10000);

				datapacketlength+=ReplicatedLength;

				// Only multiple payload packages have this value
				if (asf_data_container.MultiplePayloads != 0)
				{
					unsigned char plheader[4];

					buffered_read(ctx, plheader, asf_data_container.PayloadLType);
					ctx->past+=result;
					asf_data_container.dobjectread += result;
					if (result != asf_data_container.PayloadLType)
					{
						mprint("Premature end of file!\n");
						end_of_file=1;
						return payload_read;
					}
					asf_data_container.PayloadLength = asf_readval(plheader, asf_data_container.PayloadLType);
				}
				else
				{
					asf_data_container.PayloadLength = asf_data_container.PacketLength - datapacketlength - asf_data_container.PaddingLength;
				}
				dbg_print(CCX_DMT_PARSE, "Size - Replicated %d + Payload %d = %d\n",
						ReplicatedLength, asf_data_container.PayloadLength, ReplicatedLength + asf_data_container.PayloadLength);

				// Remember the last video time stamp - only when captions are separate
				// from video stream.
				if (asf_data_container.PayloadStreamNumber == asf_data_container.StreamProperties.VideoStreamNumber
						&& asf_data_container.StreamProperties.DecodeStreamNumber != asf_data_container.StreamProperties.VideoStreamNumber
						&& OffsetMediaLength == 0)
				{
					asf_data_container.StreamProperties.prevVideoStreamMS = asf_data_container.StreamProperties.currVideoStreamMS;
					asf_data_container.StreamProperties.currVideoStreamMS = asf_data_container.StreamProperties.VideoStreamMS;

					// Use PresentationTimems (SendTime might also work) when the
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
						asf_data_container.StreamProperties.VideoStreamMS = PresentationTimems + 1;
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
				if (asf_data_container.PayloadStreamNumber == asf_data_container.StreamProperties.DecodeStreamNumber
						&& OffsetMediaLength == 0)
				{
					asf_data_container.StreamProperties.prevDecodeStreamPTS = asf_data_container.StreamProperties.currDecodeStreamPTS;
					asf_data_container.StreamProperties.currDecodeStreamPTS = asf_data_container.StreamProperties.DecodeStreamPTS;

					// Use PresentationTimems (SendTime might also work) when the
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
						asf_data_container.StreamProperties.DecodeStreamPTS = PresentationTimems + 1;
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
					current_pts = asf_data_container.StreamProperties.currDecodeStreamPTS*(MPEG_CLOCK_FREQ / 1000);
					if (pts_set==0)
						pts_set=1;
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
			if ( curmedianumber != 0xFFFFFFFF  // Is initialized
					&& asf_data_container.PayloadStreamNumber == asf_data_container.StreamProperties.DecodeStreamNumber
					&& asf_data_container.PayloadMediaNumber != curmedianumber)
			{
				if (asf_data_container.StreamProperties.DecodeStreamNumber == asf_data_container.StreamProperties.CaptionStreamNumber)
					dbg_print(CCX_DMT_PARSE, "\nCaption stream object");
				else
					dbg_print(CCX_DMT_PARSE, "\nVideo stream object");
				dbg_print(CCX_DMT_PARSE, " read with PTS: %s\n",
						print_mstime(asf_data_container.StreamProperties.currDecodeStreamPTS));


				// Enough for now
				enough = 1;
				break;
			}

			// Read it!!
			if (asf_data_container.PayloadStreamNumber == asf_data_container.StreamProperties.DecodeStreamNumber)
			{
				curmedianumber = asf_data_container.PayloadMediaNumber; // Remember current value

				// Read the data
				dbg_print(CCX_DMT_PARSE, "Reading Stream #%d data ...\n", asf_data_container.PayloadStreamNumber);

				int want = (long)((BUFSIZE - inbuf)>asf_data_container.PayloadLength ?
						asf_data_container.PayloadLength : (BUFSIZE - inbuf));
				if (want < (long)asf_data_container.PayloadLength)
					fatal(CCX_COMMON_EXIT_BUG_BUG, "Buffer size to small for ASF payload!\nPlease file a bug report!\n");
				buffered_read (ctx, ctx->buffer+inbuf,want);
				payload_read+=(int) result;
				inbuf+=result;
				ctx->past+=result;
				if (result != asf_data_container.PayloadLength)
				{
					mprint("Premature end of file!\n");
					end_of_file=1;
					return payload_read;
				}
				asf_data_container.dobjectread += asf_data_container.PayloadLength;
			}
			else
			{
				// Skip non-cc data
				dbg_print(CCX_DMT_PARSE, "Skipping Stream #%d data ...\n", asf_data_container.PayloadStreamNumber);
				buffered_skip(ctx, (int)asf_data_container.PayloadLength);
				ctx->past+=result;
				if (result != asf_data_container.PayloadLength)
				{
					mprint("Premature end of file!\n");
					end_of_file=1;
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
		buffered_skip(ctx, (long)asf_data_container.PaddingLength);
		ctx->past+=result;
		if (result != asf_data_container.PaddingLength)
		{
			mprint("Premature end of file!\n");
			end_of_file=1;
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
		buffered_skip(ctx, (int)(asf_data_container.FileSize - asf_data_container.HeaderObjectSize - asf_data_container.DataObjectSize));
		ctx->past+=result;
		// Don not set end_of_file (although it is true) as this would
		// produce an premature end error.
		//end_of_file=1;

		// parsebuf is freed automatically when the program closes.
	}

	return payload_read;
}
