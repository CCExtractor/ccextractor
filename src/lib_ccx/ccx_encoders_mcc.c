#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>

#include "ccx_encoders_mcc.h"

#define MORE_DEBUG CCX_FALSE

static const char *MonthStr[12] = {"January", "February", "March", "April",
				   "May", "June", "July", "August", "September",
				   "October", "November", "December"};

static const char *DayOfWeekStr[7] = {"Sunday", "Monday", "Tuesday", "Wednesday",
				      "Thursday", "Friday", "Saturday"};

static const uint32 framerate_translation[16] = {0, 2397, 2400, 2500, 2997, 3000, 5000, 5994, 6000, 0, 0, 0, 0, 0};

static void debug_log(char *file, int line, ...);
static struct ccx_mcc_caption_time convert_to_caption_time(LLONG mstime);
static void generate_mcc_header(int fh, int fr_code, int dropframe_flag);
static uint8 *add_boilerplate(struct encoder_ctx *ctx, unsigned char *cc_data, int cc_count, int fr_code);
static uint16 count_compressed_chars(uint8 *data_ptr, uint16 num_elements);
static void compress_data(uint8 *data_ptr, uint16 num_elements, uint8 *out_data_ptr);
static void byte_to_ascii(uint8 hex_byte, uint8 *msn, uint8 *lsn);
static void write_string(int fh, char *string);

static void random_chars(char buffer[], int len);
static void uuid4(char *buffer);

static void ms_to_frame(struct encoder_ctx *ctx, struct ccx_mcc_caption_time *caption_time_ptr, int fr_code, int dropframe_flag)
{
	int64 actual_time_in_ms = (((caption_time_ptr->hour * 3600) + (caption_time_ptr->minute * 60) +
				    (caption_time_ptr->second)) *
				   1000) +
				  caption_time_ptr->millisecond;

	caption_time_ptr->hour = ctx->next_caption_time.hour;
	caption_time_ptr->minute = ctx->next_caption_time.minute;
	caption_time_ptr->second = ctx->next_caption_time.second;
	caption_time_ptr->frame = ctx->next_caption_time.frame;
	caption_time_ptr->millisecond = 0;

	ctx->next_caption_time.frame = ctx->next_caption_time.frame + 1;

	uint8 frame_roll_over = framerate_translation[fr_code] / 100;
	if ((framerate_translation[fr_code] % 100) > 75)
		frame_roll_over++;

	if (ctx->next_caption_time.frame >= frame_roll_over)
	{
		ctx->next_caption_time.frame = 0;
		ctx->next_caption_time.second = ctx->next_caption_time.second + 1;
	}

	if ((dropframe_flag == CCX_TRUE) && (ctx->next_caption_time.second == 0) &&
	    (ctx->next_caption_time.frame == 0) && ((ctx->next_caption_time.minute % 10) != 0))
	{
		ctx->next_caption_time.frame = 2;
	}

	if (ctx->next_caption_time.second >= 60)
	{
		ctx->next_caption_time.second = 0;
		ctx->next_caption_time.minute = ctx->next_caption_time.minute + 1;
	}

	if (ctx->next_caption_time.minute >= 60)
	{
		ctx->next_caption_time.minute = 0;
		ctx->next_caption_time.hour = ctx->next_caption_time.hour + 1;
	}

	int64 frame_time_in_ms = (((caption_time_ptr->hour * 3600) + (caption_time_ptr->minute * 60) + (caption_time_ptr->second)) * 1000) +
				 ((caption_time_ptr->frame * 100000) / framerate_translation[fr_code]);

	int64 delta_in_ms;

	if (actual_time_in_ms > frame_time_in_ms)
	{
		delta_in_ms = actual_time_in_ms - frame_time_in_ms;
	}
	else
	{
		delta_in_ms = frame_time_in_ms - actual_time_in_ms;
	}

	if (delta_in_ms > 1000)
	{
		LOG("ERROR: Larger than expected delta in Caption Time Conversion: %lld, DF%d, %dfps",
		    delta_in_ms, dropframe_flag, framerate_translation[fr_code]);
	}
}

boolean mcc_encode_cc_data(struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx, unsigned char *cc_data, int cc_count)
{
	ASSERT(cc_data);
	ASSERT(enc_ctx);
	ASSERT(dec_ctx);

	struct ccx_mcc_caption_time caption_time = convert_to_caption_time(enc_ctx->timing->fts_now + enc_ctx->timing->fts_global);

	if (enc_ctx->header_printed_flag == CCX_FALSE)
	{
		dec_ctx->saw_caption_block = CCX_TRUE;
		enc_ctx->header_printed_flag = CCX_TRUE;
		enc_ctx->cdp_hdr_seq = 0;
		generate_mcc_header(enc_ctx->out->fh, dec_ctx->current_frame_rate, enc_ctx->force_dropframe);

		enc_ctx->next_caption_time.hour = caption_time.hour;
		enc_ctx->next_caption_time.minute = caption_time.minute;
		enc_ctx->next_caption_time.second = caption_time.second;
		uint64 frame_number = caption_time.millisecond * framerate_translation[dec_ctx->current_frame_rate];
		frame_number /= 100000;
		if (frame_number > (framerate_translation[dec_ctx->current_frame_rate] / 100))
		{
			LOG("WARN: Normalized Frame Number %d to %d", frame_number, (framerate_translation[dec_ctx->current_frame_rate] / 100));
			frame_number = framerate_translation[dec_ctx->current_frame_rate] / 100;
		}
		enc_ctx->next_caption_time.frame = frame_number;
		LOG("Captions start at: %02d:%02d:%02d:%02d / %02d:%02d:%02d,%03d",
		    caption_time.hour, caption_time.minute, caption_time.second, frame_number,
		    caption_time.hour, caption_time.minute, caption_time.second, caption_time.millisecond);
	}

#if MORE_DEBUG
	LOG("MCC Encoding %d byte packet at time: %02d:%02d:%02d;%02d", cc_count, caption_time.hour,
	    caption_time.minute, caption_time.second, caption_time.frame);
#endif

	uint8 *w_boilerplate_buffer = add_boilerplate(enc_ctx, cc_data, cc_count, dec_ctx->current_frame_rate);
	uint16 w_boilerplate_buff_size = ((cc_count * 3) + 16);

	ms_to_frame(enc_ctx, &caption_time, dec_ctx->current_frame_rate, enc_ctx->force_dropframe);

	uint16 num_chars_needed = count_compressed_chars(w_boilerplate_buffer, w_boilerplate_buff_size);

#if MORE_DEBUG
	LOG("With CDP Boiler Plate %d byte packet at time: %02d:%02d:%02d;%02d requires %d bytes compressed",
	    w_boilerplate_buff_size, caption_time.hour, caption_time.minute,
	    caption_time.second, caption_time.frame, num_chars_needed);
#endif

	char *compressed_data_buffer = malloc(num_chars_needed + 13);

	sprintf(compressed_data_buffer, "%02d:%02d:%02d:%02d\t", caption_time.hour, caption_time.minute,
		caption_time.second, caption_time.frame);

	compress_data(w_boilerplate_buffer, w_boilerplate_buff_size, (uint8 *)&compressed_data_buffer[12]);
	free(w_boilerplate_buffer);

#if MORE_DEBUG
	LOG("Writing Compressed %d byte packet at time: %02d:%02d:%02d;%02d", (num_chars_needed + 13),
	    caption_time.hour, caption_time.minute, caption_time.second, caption_time.frame);
#endif

	strcat(compressed_data_buffer, "\n");

	write_wrapped(enc_ctx->out->fh, compressed_data_buffer, strlen(compressed_data_buffer));

	free(compressed_data_buffer);

	return true; // Needed to avoid warning
		     // With void function type - throws an error
} // mcc_encode_cc_data()

static void generate_mcc_header(int fh, int fr_code, int dropframe_flag)
{
	char uuid_str[50];
	char date_str[50];
	char time_str[30];
	char tcr_str[25];
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	sprintf(uuid_str, "UUID=");
	uuid4(&uuid_str[5]);
	uuid_str[41] = '\n';
	uuid_str[42] = '\0';

	ASSERT(tm.tm_wday < 7);
	ASSERT(tm.tm_mon < 12);
	sprintf(date_str, "Creation Date=%s, %s %d, %d\n", DayOfWeekStr[tm.tm_wday], MonthStr[tm.tm_mon], tm.tm_mday, tm.tm_year + 1900);
	sprintf(time_str, "Creation Time=%d:%02d:%02d\n", tm.tm_hour, tm.tm_min, tm.tm_sec);

	switch (fr_code)
	{
		case 1:
		case 2:
			sprintf(tcr_str, "Time Code Rate=24\n\n");
			break;
		case 3:
			sprintf(tcr_str, "Time Code Rate=25\n\n");
			break;
		case 4:
		case 5:
			if (dropframe_flag == CCX_TRUE)
			{
				sprintf(tcr_str, "Time Code Rate=30DF\n\n");
			}
			else
			{
				sprintf(tcr_str, "Time Code Rate=30\n\n");
			}
			break;
		case 6:
			sprintf(tcr_str, "Time Code Rate=50\n\n");
			break;
		case 7:
		case 8:
			if (dropframe_flag == CCX_TRUE)
			{
				sprintf(tcr_str, "Time Code Rate=60DF\n\n");
			}
			else
			{
				sprintf(tcr_str, "Time Code Rate=60\n\n");
			}
			break;
		default:
			LOG("ERROR: Invalid Framerate Code: %d", fr_code);
			break;
	}

	write_string(fh, "File Format=MacCaption_MCC V1.0\n\n");
	write_string(fh, "///////////////////////////////////////////////////////////////////////////////////\n");
	write_string(fh, "// Telestream, LLC\n");
	write_string(fh, "// Ancillary Data Packet Transfer File\n");
	write_string(fh, "//\n// Permission to generate this format is granted provided that\n");
	write_string(fh, "//   1. This ANC Transfer file format is used on an as-is basis and no warranty is given, and\n");
	write_string(fh, "//   2. This entire descriptive information text is included in a generated .mcc file.\n");
	write_string(fh, "//\n// General file format:\n");
	write_string(fh, "//   HH:MM:SS:FF(tab)[Hexadecimal ANC data in groups of 2 characters]\n");
	write_string(fh, "//     Hexadecimal data starts with the Ancillary Data Packet DID (Data ID defined in S291M)\n");
	write_string(fh, "//       and concludes with the Check Sum following the User Data Words.\n");
	write_string(fh, "//     Each time code line must contain at most one complete ancillary data packet.\n");
	write_string(fh, "//     To transfer additional ANC Data successive lines may contain identical time code.\n");
	write_string(fh, "//     Time Code Rate=[24, 25, 30, 30DF, 50, 60, 60DF]\n//\n");
	write_string(fh, "//   ANC data bytes may be represented by one ASCII character according to the following schema:\n");
	write_string(fh, "//     G  FAh 00h 00h\n");
	write_string(fh, "//     H  2 x (FAh 00h 00h)\n");
	write_string(fh, "//     I  3 x (FAh 00h 00h)\n");
	write_string(fh, "//     J  4 x (FAh 00h 00h)\n");
	write_string(fh, "//     K  5 x (FAh 00h 00h)\n");
	write_string(fh, "//     L  6 x (FAh 00h 00h)\n");
	write_string(fh, "//     M  7 x (FAh 00h 00h)\n");
	write_string(fh, "//     N  8 x (FAh 00h 00h)\n");
	write_string(fh, "//     O  9 x (FAh 00h 00h)\n");
	write_string(fh, "//     P  FBh 80h 80h\n");
	write_string(fh, "//     Q  FCh 80h 80h\n");
	write_string(fh, "//     R  FDh 80h 80h\n");
	write_string(fh, "//     S  96h 69h\n");
	write_string(fh, "//     T  61h 01h\n");
	write_string(fh, "//     U  E1h 00h 00h 00h\n");
	write_string(fh, "//     Q  FCh 80h 80h\n");
	write_string(fh, "//     Q  FCh 80h 80h\n");
	write_string(fh, "//     Z  00h\n//\n");
	write_string(fh, "///////////////////////////////////////////////////////////////////////////////////\n\n");
	write_string(fh, uuid_str);
	write_string(fh, "Creation Program=CCExtractor\n");
	write_string(fh, date_str);
	write_string(fh, time_str);
	write_string(fh, tcr_str);
} // generate_mcc_header()

static uint8 *add_boilerplate(struct encoder_ctx *ctx, unsigned char *cc_data, int cc_count, int fr_code)
{
	ASSERT(cc_data);
	ASSERT(cc_count > 0);

	uint8 data_size = cc_count * 3;
	uint8 *buff_ptr = malloc(data_size + 16);
	uint8 cdp_frame_rate = CDP_FRAME_RATE_FORBIDDEN;

	switch (fr_code)
	{
		case 1:
			cdp_frame_rate = CDP_FRAME_RATE_23_976;
			break;
		case 2:
			cdp_frame_rate = CDP_FRAME_RATE_24;
			break;
		case 3:
			cdp_frame_rate = CDP_FRAME_RATE_25;
			break;
		case 4:
			cdp_frame_rate = CDP_FRAME_RATE_29_97;
			break;
		case 5:
			cdp_frame_rate = CDP_FRAME_RATE_30;
			break;
		case 6:
			cdp_frame_rate = CDP_FRAME_RATE_50;
			break;
		case 7:
			cdp_frame_rate = CDP_FRAME_RATE_59_94;
			break;
		case 8:
			cdp_frame_rate = CDP_FRAME_RATE_60;
			break;
		default:
			LOG("ERROR: Unknown Framerate: %d", fr_code);
			break;
	}

	//   This function encodes the Ancillary Data (ANC) Packet, which wraps the Caption
	//   Distribution Packet (CDP), including the Closed Captioning Data (ccdata_section) as
	//   described in the CEA-708 Spec. Below is the list of specs that were leveraged for
	//   this encode:
	//
	//   SMPTE ST 334-1 - Vertical Ancillary Data Mapping of Caption Data and Other Related Data
	//                    (Specifically: SMPTE ST 334-1:2015 - Revision of SMPTE 334-1-2007)
	//   SMPTE ST 334-2 - Caption Distribution Packet (CDP) Definition
	//                    (Specifically: SMPTE ST 334-2:2015 - Revision of SMPTE 334-2-2007)
	//   CEA-708-D - CEA Standard - Digital Television (DTV) Closed Captioning - August 2008

	buff_ptr[0] = ANC_DID_CLOSED_CAPTIONING;
	buff_ptr[1] = ANC_SDID_CEA_708;
	buff_ptr[2] = data_size + 12;
	buff_ptr[3] = CDP_IDENTIFIER_VALUE_HIGH;
	buff_ptr[4] = CDP_IDENTIFIER_VALUE_LOW;
	buff_ptr[5] = data_size + 12;
	buff_ptr[6] = ((cdp_frame_rate << 4) | 0x0F);
	buff_ptr[7] = 0x43; // Timecode not Present; Service Info not Present; Captions Present
	buff_ptr[8] = (uint8)((ctx->cdp_hdr_seq & 0xF0) >> 8);
	buff_ptr[9] = (uint8)(ctx->cdp_hdr_seq & 0x0F);
	buff_ptr[10] = CC_DATA_ID;
	buff_ptr[11] = cc_count | 0xE0;
	memcpy(&buff_ptr[12], cc_data, data_size);
	uint8 *data_ptr = &buff_ptr[data_size + 12];
	data_ptr[0] = CDP_FOOTER_ID;
	data_ptr[1] = (uint8)((ctx->cdp_hdr_seq & 0xF0) >> 8);
	data_ptr[2] = (uint8)(ctx->cdp_hdr_seq & 0x0F);
	data_ptr[3] = 0;

	for (int loop = 0; loop < (data_size + 15); loop++)
	{
		data_ptr[3] = data_ptr[3] + buff_ptr[loop];
	}

	ctx->cdp_hdr_seq++;

	return buff_ptr;
} // add_boilerplate()

static uint16 count_compressed_chars(uint8 *data_ptr, uint16 num_elements)
{
	uint16 num_chars = 1;

	//   ANC data bytes may be represented by one ASCII character according to the following schema:
	//     G  FAh 00h 00h
	//     H  2 x (FAh 00h 00h)
	//     I  3 x (FAh 00h 00h)
	//     J  4 x (FAh 00h 00h)
	//     K  5 x (FAh 00h 00h)
	//     L  6 x (FAh 00h 00h)
	//     M  7 x (FAh 00h 00h)
	//     N  8 x (FAh 00h 00h)
	//     O  9 x (FAh 00h 00h)
	//     P  FBh 80h 80h
	//     Q  FCh 80h 80h
	//     R  FDh 80h 80h
	//     S  96h 69h
	//     T  61h 01h
	//     U  E1h 00h 00h 00h
	//     Z  00h

	while (num_elements > 0)
	{
		switch (data_ptr[0])
		{
			case 0xFA:
				if ((num_elements >= 3) && (data_ptr[1] == 0x00) && (data_ptr[2] == 0x00))
				{
					uint8 numFaoos = 0;
					while ((num_elements >= 3) && (data_ptr[0] == 0xFA) && (data_ptr[1] == 0x00) &&
					       (data_ptr[2] == 0x00) && (numFaoos < 9))
					{
						data_ptr = &data_ptr[3];
						num_elements = num_elements - 3;
						numFaoos++;
					}
					num_chars = num_chars + 1;
				}
				else
				{
					data_ptr = &data_ptr[1];
					num_elements = num_elements - 1;
					num_chars = num_chars + 2;
				}
				break;
			case 0xFB:
			case 0xFC:
			case 0xFD:
				if ((num_elements >= 3) && (data_ptr[1] == 0x80) && (data_ptr[2] == 0x80))
				{
					data_ptr = &data_ptr[3];
					num_elements = num_elements - 3;
					num_chars = num_chars + 1;
				}
				else
				{
					data_ptr = &data_ptr[1];
					num_elements = num_elements - 1;
					num_chars = num_chars + 2;
				}
				break;
			case 0x96:
				if ((num_elements >= 2) && data_ptr[1] == 0x69)
				{
					data_ptr = &data_ptr[2];
					num_elements = num_elements - 2;
					num_chars = num_chars + 1;
				}
				else
				{
					data_ptr = &data_ptr[1];
					num_elements = num_elements - 1;
					num_chars = num_chars + 2;
				}
				break;
			case 0x61:
				if ((num_elements >= 2) && data_ptr[1] == 0x01)
				{
					data_ptr = &data_ptr[2];
					num_elements = num_elements - 2;
					num_chars = num_chars + 1;
				}
				else
				{
					data_ptr = &data_ptr[1];
					num_elements = num_elements - 1;
					num_chars = num_chars + 2;
				}
				break;
			case 0xE1:
				if ((num_elements >= 4) && (data_ptr[1] == 0x00) && (data_ptr[2] == 0x00) && (data_ptr[3] == 0x00))
				{
					data_ptr = &data_ptr[4];
					num_elements = num_elements - 4;
					num_chars = num_chars + 1;
				}
				else
				{
					data_ptr = &data_ptr[1];
					num_elements = num_elements - 1;
					num_chars = num_chars + 2;
				}
				break;
			case 0x00:
				data_ptr = &data_ptr[1];
				num_elements = num_elements - 1;
				num_chars = num_chars + 1;
				break;
			default:
				data_ptr = &data_ptr[1];
				num_elements = num_elements - 1;
				num_chars = num_chars + 2;
				break;
		}
	}
	return num_chars;
} // count_compressed_chars()

static void compress_data(uint8 *data_ptr, uint16 num_elements, uint8 *out_data_ptr)
{

	//   ANC data bytes may be represented by one ASCII character according to the following schema:
	//     G  FAh 00h 00h
	//     H  2 x (FAh 00h 00h)
	//     I  3 x (FAh 00h 00h)
	//     J  4 x (FAh 00h 00h)
	//     K  5 x (FAh 00h 00h)
	//     L  6 x (FAh 00h 00h)
	//     M  7 x (FAh 00h 00h)
	//     N  8 x (FAh 00h 00h)
	//     O  9 x (FAh 00h 00h)
	//     P  FBh 80h 80h
	//     Q  FCh 80h 80h
	//     R  FDh 80h 80h
	//     S  96h 69h
	//     T  61h 01h
	//     U  E1h 00h 00h 00h
	//     Z  00h

	while (num_elements > 0)
	{
		switch (data_ptr[0])
		{
			case 0xFA:
				if ((num_elements >= 3) && (data_ptr[1] == 0x00) && (data_ptr[2] == 0x00))
				{
					uint8 numFaoos = 0;
					while ((num_elements >= 3) && (data_ptr[0] == 0xFA) && (data_ptr[1] == 0x00) &&
					       (data_ptr[2] == 0x00) && (numFaoos < 9))
					{
						data_ptr = &data_ptr[3];
						num_elements = num_elements - 3;
						numFaoos++;
					}
					switch (numFaoos)
					{
						case 1:
							*out_data_ptr = 'G';
							break;
						case 2:
							*out_data_ptr = 'H';
							break;
						case 3:
							*out_data_ptr = 'I';
							break;
						case 4:
							*out_data_ptr = 'J';
							break;
						case 5:
							*out_data_ptr = 'K';
							break;
						case 6:
							*out_data_ptr = 'L';
							break;
						case 7:
							*out_data_ptr = 'M';
							break;
						case 8:
							*out_data_ptr = 'N';
							break;
						case 9:
							*out_data_ptr = 'O';
							break;
						default:
							LOG("ERROR: Invalid Branch: %d", numFaoos);
					}
					out_data_ptr = &out_data_ptr[1];
				}
				else
				{
					byte_to_ascii(data_ptr[0], &out_data_ptr[0], &out_data_ptr[1]);
					out_data_ptr = &out_data_ptr[2];
					data_ptr = &data_ptr[1];
					num_elements = num_elements - 1;
				}
				break;
			case 0xFB:
			case 0xFC:
			case 0xFD:
				if ((num_elements >= 3) && (data_ptr[1] == 0x80) && (data_ptr[2] == 0x80))
				{
					switch (data_ptr[0])
					{
						case 0xFB:
							*out_data_ptr = 'P';
							break;
						case 0xFC:
							*out_data_ptr = 'Q';
							break;
						case 0xFD:
							*out_data_ptr = 'R';
							break;
						default:
							ASSERT(0);
					}
					data_ptr = &data_ptr[3];
					num_elements = num_elements - 3;
					out_data_ptr = &out_data_ptr[1];
				}
				else
				{
					byte_to_ascii(data_ptr[0], &out_data_ptr[0], &out_data_ptr[1]);
					out_data_ptr = &out_data_ptr[2];
					data_ptr = &data_ptr[1];
					num_elements = num_elements - 1;
				}
				break;
			case 0x96:
				if ((num_elements >= 2) && data_ptr[1] == 0x69)
				{
					data_ptr = &data_ptr[2];
					num_elements = num_elements - 2;
					*out_data_ptr = 'S';
					out_data_ptr = &out_data_ptr[1];
				}
				else
				{
					byte_to_ascii(data_ptr[0], &out_data_ptr[0], &out_data_ptr[1]);
					out_data_ptr = &out_data_ptr[2];
					data_ptr = &data_ptr[1];
					num_elements = num_elements - 1;
				}
				break;
			case 0x61:
				if ((num_elements >= 2) && data_ptr[1] == 0x01)
				{
					data_ptr = &data_ptr[2];
					num_elements = num_elements - 2;
					*out_data_ptr = 'T';
					out_data_ptr = &out_data_ptr[1];
				}
				else
				{
					byte_to_ascii(data_ptr[0], &out_data_ptr[0], &out_data_ptr[1]);
					out_data_ptr = &out_data_ptr[2];
					data_ptr = &data_ptr[1];
					num_elements = num_elements - 1;
				}
				break;
			case 0xE1:
				if ((num_elements >= 4) && (data_ptr[1] == 0x00) && (data_ptr[2] == 0x00) && (data_ptr[3] == 0x00))
				{
					data_ptr = &data_ptr[4];
					num_elements = num_elements - 4;
					*out_data_ptr = 'U';
					out_data_ptr = &out_data_ptr[1];
				}
				else
				{
					byte_to_ascii(data_ptr[0], &out_data_ptr[0], &out_data_ptr[1]);
					out_data_ptr = &out_data_ptr[2];
					data_ptr = &data_ptr[1];
					num_elements = num_elements - 1;
				}
				break;
			case 0x00:
				data_ptr = &data_ptr[1];
				num_elements = num_elements - 1;
				*out_data_ptr = 'Z';
				out_data_ptr = &out_data_ptr[1];
				break;
			default:
				byte_to_ascii(data_ptr[0], &out_data_ptr[0], &out_data_ptr[1]);
				out_data_ptr = &out_data_ptr[2];
				data_ptr = &data_ptr[1];
				num_elements = num_elements - 1;
				break;
		}
	}
	*out_data_ptr = '\0';
} // compress_data()

static void random_chars(char buffer[], int len)
{
	for (int i = 0; i < len; i++)
	{
		sprintf(buffer + i, "%X", rand() % 16);
	}
}

static void uuid4(char *buffer)
{
	int i = 0;
	random_chars(buffer + i, 8);
	i += 8;
	buffer[i++] = '-';
	random_chars(buffer + i, 4);
	i += 4;
	buffer[i++] = '-';
	buffer[i++] = '4'; // uuid version (0b0100)
	random_chars(buffer + i, 3);
	i += 3;
	buffer[i++] = '-';
	buffer[i++] = "89AB"[rand() % 4]; // uuid variant (0b10??)
	random_chars(buffer + i, 3);
	i += 3;
	buffer[i++] = '-';
	random_chars(buffer + i, 12);
	i += 12;
	buffer[i++] = 0;
}

static void debug_log(char *file, int line, ...)
{
	va_list args;
	char message[1024];

	va_start(args, line);
	char *fmt = va_arg(args, char *);
	vsprintf(message, fmt, args);
	va_end(args);

	char *basename = strrchr(file, '/');
	basename = basename ? basename + 1 : file;

	if (message[(strlen(message) - 1)] == '\n')
		message[(strlen(message) - 1)] = '\0';

	dbg_print(CCX_DMT_VERBOSE, "[%s:%d] - %s\n", basename, line, message);
} // debug_log()

static struct ccx_mcc_caption_time convert_to_caption_time(LLONG mstime)
{
	struct ccx_mcc_caption_time retval;

	if (mstime < 0) // Avoid loss of data warning with abs()
		mstime = -mstime;

	retval.hour = (mstime / 1000 / 60 / 60);
	retval.minute = (mstime / 1000 / 60 - 60 * retval.hour);
	retval.second = (mstime / 1000 - 60 * (retval.minute + 60 * retval.hour));
	retval.millisecond = (mstime - 1000 * (retval.second + 60 * (retval.minute + 60 * retval.hour)));

	return retval;
} // convert_to_caption_time()

static void byte_to_ascii(uint8 hex_byte, uint8 *msn, uint8 *lsn)
{
	ASSERT(msn);
	ASSERT(lsn);

	*msn = (hex_byte & 0xF0);
	*msn = *msn >> 4;
	if (*msn < 0x0A)
		*msn = *msn + '0';
	else
		*msn = (*msn - 0x0A) + 'A';

	*lsn = (hex_byte & 0x0F);
	if (*lsn < 0x0A)
		*lsn = *lsn + '0';
	else
		*lsn = (*lsn - 0x0A) + 'A';
} // byteToAscii()

static void write_string(int fh, char *string)
{
	write_wrapped(fh, string, strlen(string));
}
