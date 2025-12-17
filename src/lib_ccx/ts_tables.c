#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "dvb_subtitle_decoder.h"
#include "ccx_decoders_isdb.h"
#include "utility.h"
#include "activity.h"

static unsigned pmt_warning_shown = 0; // Only display warning once
void process_ccx_mpeg_descriptor(unsigned char *data, unsigned length);

unsigned get_printable_stream_type(enum ccx_stream_type stream_type)
{
	enum ccx_stream_type tmp_stream_type = stream_type;

	switch (stream_type)
	{
		case CCX_STREAM_TYPE_VIDEO_MPEG2:
		case CCX_STREAM_TYPE_VIDEO_H264:
		case CCX_STREAM_TYPE_PRIVATE_MPEG2:
		case CCX_STREAM_TYPE_PRIVATE_TABLE_MPEG2:
		case CCX_STREAM_TYPE_MHEG_PACKETS:
		case CCX_STREAM_TYPE_MPEG2_ANNEX_A_DSM_CC:
		case CCX_STREAM_TYPE_ISO_IEC_13818_6_TYPE_A:
		case CCX_STREAM_TYPE_ISO_IEC_13818_6_TYPE_B:
		case CCX_STREAM_TYPE_ISO_IEC_13818_6_TYPE_C:
		case CCX_STREAM_TYPE_ISO_IEC_13818_6_TYPE_D:
		case CCX_STREAM_TYPE_ITU_T_H222_1:
		case CCX_STREAM_TYPE_VIDEO_MPEG1:
		case CCX_STREAM_TYPE_AUDIO_MPEG1:
		case CCX_STREAM_TYPE_AUDIO_MPEG2:
		case CCX_STREAM_TYPE_AUDIO_AAC:
		case CCX_STREAM_TYPE_VIDEO_MPEG4:
		case CCX_STREAM_TYPE_AUDIO_AC3:
		case CCX_STREAM_TYPE_AUDIO_DTS:
		case CCX_STREAM_TYPE_AUDIO_HDMV_DTS:
			break;

		case CCX_STREAM_TYPE_VIDEO_HEVC:
			break;

		default:
			if (stream_type >= 0x80 && stream_type <= 0xFF)
				tmp_stream_type = CCX_STREAM_TYPE_PRIVATE_USER_MPEG2;
			else
				tmp_stream_type = CCX_STREAM_TYPE_UNKNOWNSTREAM;
			break;
	}

	return tmp_stream_type;
}

void clear_PMT_array(struct ccx_demuxer *ctx)
{
	if (ctx->flag_ts_forced_pn == CCX_FALSE)
	{
		ctx->nb_program = 0;
	}
}

int need_program(struct ccx_demuxer *ctx)
{
	if (ctx->nb_program == 0)
		return CCX_TRUE;

	if (ctx->nb_program == 1 && ctx->ts_autoprogram == CCX_TRUE)
		return CCX_FALSE;

	return CCX_FALSE;
}
int update_pinfo(struct ccx_demuxer *ctx, int pid, int program_number)
{
	if (!ctx)
		return -1;

	if (ctx->nb_program >= MAX_PROGRAM)
		return -1;

	ctx->pinfo[ctx->nb_program].pid = pid;
	ctx->pinfo[ctx->nb_program].program_number = program_number;
	ctx->pinfo[ctx->nb_program].analysed_PMT_once = CCX_FALSE;
	ctx->pinfo[ctx->nb_program].name[0] = '\0';
	ctx->pinfo[ctx->nb_program].pcr_pid = -1;
	ctx->pinfo[ctx->nb_program].has_all_min_pts = 0;
	for (int i = 0; i < COUNT; i++)
	{
		ctx->pinfo[ctx->nb_program].got_important_streams_min_pts[i] = UINT64_MAX;
	}
	ctx->nb_program++;

	return CCX_OK;
}

/* Process Program Map Table - The PMT contains a list of streams in a program.
   Input: pos => Index in the PAT array
   Returns: Changes in the selected PID=1, No changes=0, if changes then if the
   buffer had anything it should be flushed.
   PMT specs: ISO13818-1 / table 2-28
   */

int parse_PMT(struct ccx_demuxer *ctx, unsigned char *buf, int len, struct program_info *pinfo)
{
	int must_flush = 0;
	int ret = 0;
	unsigned char desc_len = 0;
	unsigned char *sbuf = buf;
	unsigned int olen = len;
	uint32_t crc;

	uint8_t table_id;
	uint16_t section_length;
	uint16_t program_number;
	uint8_t version_number;
	uint8_t current_next_indicator;
	uint8_t section_number;
	uint8_t last_section_number;

	uint16_t pi_length;

	// Minimum PMT size: table_id(1) + section_length(2) + program_number(2) +
	// version/current(1) + section_number(1) + last_section_number(1) +
	// PCR_PID(2) + program_info_length(2) + CRC(4) = 16 bytes
	if (len < 16)
	{
		dbg_print(CCX_DMT_PMT, "PMT packet too short (%d bytes), ignoring\n", len);
		return 0;
	}

	crc = (*(int32_t *)(sbuf + olen - 4));
	table_id = buf[0];

	/* TO-DO: We're currently parsing the PMT making assumptions that there's only one section with table_id=2,
	but that doesn't have to be the case. There's a sample (friends_tbs.ts) that shows a previous section with
	table_id = 0xc0. I can't find any place that says that 0xc0 (Program Information Table) must come before
	table_id = 2, so we should process sections in any order.
	Check https://github.com/CCExtractor/ccextractor/issues/385 for more info
	*/
	if (table_id == 0xC0)
	{
		/*
		 * Acc to System Information for Satellite Distribution
		 * of Digital Television for Cable and MMDS (ANSI/SCTE 57 2003 )
		 * PROGRAM INFORMATION Table found in PMT
		 */
		dbg_print(CCX_DMT_PMT, "PMT: PROGRAM INFORMATION Table need implementation");
		// For now, just parse its length and remove it from the buffer
		unsigned c0length = (buf[1] << 8 | buf[2]) & 0xFFF; // 12 bytes
		dbg_print(CCX_DMT_PMT, "Program information table length: %u", c0length);
		memmove(buf, buf + c0length + 3, len - c0length - 3); // First 3 bytes are for the table_id and the length, don't count
		table_id = buf[0];
		// return 0;
	}
	else if (table_id == 0xC1)
	{
		// SCTE 57 2003
		dbg_print(CCX_DMT_PMT, "PMT: PROGRAM NAME Table need implementation");
		unsigned c0length = (buf[1] << 8 | buf[2]) & 0xFFF; // 12 bytes
		dbg_print(CCX_DMT_PMT, "Program name message length: %u", c0length);
		memmove(buf, buf + c0length + 3, len - c0length - 3); // First 3 bytes are for the table_id and the length, don't count
		table_id = buf[0];
		// return 0;
	}
	else if (table_id != 0x2)
	{
		mprint("Please Report: Unknown table id in PMT expected 0x02 found 0x%X\n", table_id);
		return 0;
	}

	section_length = (((buf[1] & 0x0F) << 8) | buf[2]);
	if (section_length > (len - 3))
	{
		return 0; // We don't have the full section yet. We will parse again when we have it.
	}

	program_number = ((buf[3] << 8) | buf[4]);
	version_number = (buf[5] & 0x3E) >> 1;

	current_next_indicator = buf[5] & 0x01;
	// This table is not active, no need to evaluate
	if (!current_next_indicator && pinfo->version != 0xFF) // 0xFF means we don't have one yet
		return 0;

	// Bounds check: saved_section is 1021 bytes
	if (len > 1021)
	{
		dbg_print(CCX_DMT_PMT, "PMT section too large (%d bytes), truncating to 1021\n", len);
		len = 1021;
	}
	memcpy(pinfo->saved_section, buf, len);

	if (pinfo->analysed_PMT_once == CCX_TRUE && pinfo->version == version_number)
	{
		if (pinfo->version == version_number)
		{
			/* Same Version number and there was valid or similar CRC last time */
			if (pinfo->valid_crc == CCX_TRUE || pinfo->crc == crc)
				return 0;
		}
		else if ((pinfo->version + 1) % 32 != version_number)
			mprint("TS PMT:Glitch in version number increment");
	}
	pinfo->version = version_number;

	section_number = buf[6];
	last_section_number = buf[7];
	if (last_section_number > 0)
	{
		mprint("Long PMTs are not supported - skipped.\n");
		return 0;
	}

	pinfo->pcr_pid = (((buf[8] & 0x1F) << 8) | buf[9]);
	pi_length = (((buf[10] & 0x0F) << 8) | buf[11]);

	if (12 + pi_length > len)
	{
		// If we would support long PMTs, this would be wrong.
		mprint("program_info_length cannot be longer than the payload_length - skipped\n");
		return 0;
	}
	buf += 12 + pi_length;
	len -= (12 + pi_length);

	unsigned stream_data = section_length - 9 - pi_length - 4; // prev. bytes and CRC

	dbg_print(CCX_DMT_PMT, "Read PMT packet  (id: %u) program number: %u\n",
		  table_id, program_number);
	dbg_print(CCX_DMT_PMT, "  section length: %u  number: %u  last: %u\n",
		  section_length, section_number, last_section_number);
	dbg_print(CCX_DMT_PMT, "  version_number: %u  current_next_indicator: %u\n",
		  version_number, current_next_indicator);
	dbg_print(CCX_DMT_PMT, "  PCR_PID: %u  data length: %u  payload_length: %u\n",
		  pinfo->pcr_pid, stream_data, len);

	if (!pmt_warning_shown && stream_data + 4 > len)
	{
		dbg_print(CCX_DMT_GENERIC_NOTICES, "\rWarning: Probably parsing incomplete PMT, expected data longer than available payload.\n");
		pmt_warning_shown = 1;
	}
	// Make a note of the program number for all PIDs, so we can report it later
	for (unsigned i = 0; i < stream_data && (i + 4) < len; i += 5)
	{
		enum ccx_stream_type stream_type = buf[i];
		unsigned elementary_PID = (((buf[i + 1] & 0x1F) << 8) | buf[i + 2]);
		unsigned ES_info_length = (((buf[i + 3] & 0x0F) << 8) | buf[i + 4]);
		if (ctx->PIDs_programs[elementary_PID] == NULL)
		{
			ctx->PIDs_programs[elementary_PID] = (struct PMT_entry *)malloc(sizeof(struct PMT_entry));
			if (ctx->PIDs_programs[elementary_PID] == NULL)
				fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory to process PMT.");
		}
		ctx->PIDs_programs[elementary_PID]->elementary_PID = elementary_PID;
		ctx->PIDs_programs[elementary_PID]->stream_type = stream_type;
		ctx->PIDs_programs[elementary_PID]->program_number = program_number;
		ctx->PIDs_programs[elementary_PID]->printable_stream_type = get_printable_stream_type(stream_type);
		dbg_print(CCX_DMT_VERBOSE, "%6u | %3X (%3u) | %s\n", elementary_PID, stream_type, stream_type,
			  desc[ctx->PIDs_programs[elementary_PID]->printable_stream_type]);
		process_ccx_mpeg_descriptor(buf + i + 5, ES_info_length);
		i += ES_info_length;
	}
	dbg_print(CCX_DMT_VERBOSE, "---\n");

	dbg_print(CCX_DMT_PMT, "\nProgram map section (PMT)\n");

	for (unsigned i = 0; i < stream_data && (i + 4) < len; i += 5)
	{
		enum ccx_stream_type stream_type = buf[i];
		unsigned elementary_PID = (((buf[i + 1] & 0x1F) << 8) | buf[i + 2]);
		unsigned ES_info_length = (((buf[i + 3] & 0x0F) << 8) | buf[i + 4]);

		if (!ccx_options.print_file_reports ||
		    stream_type != CCX_STREAM_TYPE_PRIVATE_MPEG2 ||
		    !ES_info_length)
		{
			i += ES_info_length;
			continue;
		}

		unsigned char *es_info = buf + i + 5;
		for (desc_len = 0; (buf + i + 5 + ES_info_length) > es_info; es_info += desc_len)
		{
			enum ccx_mpeg_descriptor descriptor_tag = (enum ccx_mpeg_descriptor)(*es_info++);
			desc_len = (*es_info++);

			if (descriptor_tag == CCX_MPEG_DSC_DVB_SUBTITLE)
			{
				int k = 0;
				for (int j = 0; j < SUB_STREAMS_CNT; j++)
				{
					if (ctx->freport.dvb_sub_pid[j] == 0)
						k = j;
					if (ctx->freport.dvb_sub_pid[j] == elementary_PID)
					{
						k = j;
						break;
					}
				}
				ctx->freport.dvb_sub_pid[k] = elementary_PID;
			}
			if (IS_VALID_TELETEXT_DESC(descriptor_tag))
			{
				int k = 0;
				for (int j = 0; j < SUB_STREAMS_CNT; j++)
				{
					if (ctx->freport.tlt_sub_pid[j] == 0)
						k = j;
					if (ctx->freport.tlt_sub_pid[j] == elementary_PID)
					{
						k = j;
						break;
					}
				}
				ctx->freport.tlt_sub_pid[k] = elementary_PID;
			}
		}
		i += ES_info_length;
	}

	for (unsigned i = 0; i < stream_data && (i + 4) < len; i += 5)
	{
		enum ccx_stream_type stream_type = buf[i];
		unsigned elementary_PID = (((buf[i + 1] & 0x1F) << 8) | buf[i + 2]);
		unsigned ES_info_length = (((buf[i + 3] & 0x0F) << 8) | buf[i + 4]);

		if (stream_type == CCX_STREAM_TYPE_PRIVATE_MPEG2 &&
		    ES_info_length)
		{
			unsigned char *es_info = buf + i + 5;
			for (desc_len = 0; (buf + i + 5 + ES_info_length) > es_info; es_info += desc_len)
			{
				void *ptr;
				enum ccx_mpeg_descriptor descriptor_tag = (enum ccx_mpeg_descriptor)(*es_info++);
				desc_len = (*es_info++);
				if (CCX_MPEG_DESC_DATA_COMP == descriptor_tag)
				{
					int16_t component_id = 0;
					if (!IS_FEASIBLE(ctx->codec, ctx->nocodec, CCX_CODEC_ISDB_CC))
						continue;
					if (desc_len < 2)
						break;

					component_id = RB16(es_info);
					if (component_id != 0x08)
						break;
					mprint("*****ISDB subtitles detected\n");
					ptr = init_isdb_decoder();
					if (ptr == NULL)
						break;
					update_capinfo(ctx, elementary_PID, stream_type, CCX_CODEC_ISDB_CC, program_number, ptr);
				}
				if (CCX_MPEG_DSC_DVB_SUBTITLE == descriptor_tag)
				{
					struct dvb_config cnf;
					int is_dvb_subtitle = 1;
#ifndef ENABLE_OCR
					if (ccx_options.write_format != CCX_OF_SPUPNG)
					{
						mprint("DVB subtitles detected, OCR subsystem not present. Use --out=spupng for graphic output\n");
						continue;
					}
#endif
					if (!IS_FEASIBLE(ctx->codec, ctx->nocodec, CCX_CODEC_DVB))
						continue;

					memset((void *)&cnf, 0, sizeof(struct dvb_config));
					ret = parse_dvb_description(&cnf, es_info, desc_len);
					if (ret < 0)
						break;
					ptr = dvbsub_init_decoder(&cnf, pinfo->initialized_ocr);
					if (!pinfo->initialized_ocr)
						pinfo->initialized_ocr = 1;
					if (ptr == NULL)
						break;
					update_capinfo(ctx, elementary_PID, stream_type, CCX_CODEC_DVB, program_number, ptr);
					max_dif = 30;

					// [ADD THIS BLOCK: Stream Discovery]
					if (ccx_options.split_dvb_subs)
					{
						if (ctx->potential_stream_count >= MAX_POTENTIAL_STREAMS)
						{
							mprint("Warning: Max subtitle streams reached in PMT parsing.\n");
						}
						else
						{
							int exists = 0;
							for (int k = 0; k < ctx->potential_stream_count; k++)
							{
								// Check PID + Logical Type for uniqueness
								if (ctx->potential_streams[k].pid == elementary_PID &&
								    ctx->potential_streams[k].stream_type == CCX_STREAM_TYPE_DVB_SUB)
								{
									exists = 1;
									break;
								}
							}

							if (!exists)
							{
								struct ccx_stream_metadata *meta = &ctx->potential_streams[ctx->potential_stream_count++];
								meta->pid = elementary_PID;
								meta->stream_type = CCX_STREAM_TYPE_DVB_SUB;
								meta->mpeg_type = stream_type;

								// Extract language from DVB config
								if (cnf.n_language > 0)
									snprintf(meta->lang, 4, "%.3s", (char *)&cnf.lang_index[0]);
								else
									strcpy(meta->lang, "und");
								meta->lang[3] = '\0';
							}
						}
					}
				}
			}
		}
		else if (stream_type == CCX_STREAM_TYPE_PRIVATE_USER_MPEG2 && ES_info_length)
		{
			// if this any generally used video stream tyoe get clashed with ATSC/SCTE standard
			// then this code can go in some atsc flag
			unsigned char *es_info = buf + i + 5;
			for (desc_len = 0; (buf + i + 5 + ES_info_length) > es_info; es_info += desc_len)
			{
				enum ccx_mpeg_descriptor descriptor_tag = (enum ccx_mpeg_descriptor)(*es_info++);
				int nb_service;
				int is_608;
				int ser_i;
				desc_len = (*es_info++);
				if (CCX_MPEG_DSC_CAPTION_SERVICE == descriptor_tag)
				{
					nb_service = es_info[0] & 0x1f;
					for (ser_i = 0; ser_i < nb_service; ser_i++)
					{
						dbg_print(CCX_DMT_PMT, "CC SERVICE %d: language (%c%c%c)", nb_service,
							  es_info[1], es_info[2], es_info[3]);
						is_608 = es_info[4] >> 7;
						dbg_print(CCX_DMT_PMT, "%s", is_608 ? " CEA-608" : " CEA-708");
						dbg_print(CCX_DMT_PMT, "%s", is_608 ? " CEA-608" : " CEA-708");
					}
				}
				update_capinfo(ctx, elementary_PID, stream_type, CCX_CODEC_ATSC_CC, program_number, NULL);
			}
		}

		if (IS_FEASIBLE(ctx->codec, ctx->nocodec, CCX_CODEC_TELETEXT) && ES_info_length && stream_type == CCX_STREAM_TYPE_PRIVATE_MPEG2) // MPEG-2 Packetized Elementary Stream packets containing private data
		{
			unsigned char *es_info = buf + i + 5;
			for (desc_len = 0; (buf + i + 5 + ES_info_length) - es_info; es_info += desc_len)
			{
				enum ccx_mpeg_descriptor descriptor_tag = (enum ccx_mpeg_descriptor)(*es_info++);
				desc_len = (*es_info++);
				if (!IS_VALID_TELETEXT_DESC(descriptor_tag))
					continue;
				int is_teletext = 1;
				update_capinfo(ctx, elementary_PID, stream_type, CCX_CODEC_TELETEXT, program_number, NULL);
				mprint("VBI/teletext stream ID %u (0x%x) for SID %u (0x%x)\n",
				       elementary_PID, elementary_PID, program_number, program_number);

				// [ADD THIS BLOCK: Teletext Stream Discovery]
				if (ccx_options.split_dvb_subs && is_teletext)
				{
					if (ctx->potential_stream_count >= MAX_POTENTIAL_STREAMS)
					{
						mprint("Warning: Max subtitle streams reached in PMT parsing.\n");
					}
					else
					{
						int exists = 0;
						for (int k = 0; k < ctx->potential_stream_count; k++)
						{
							// Check PID + Logical Type for uniqueness
							if (ctx->potential_streams[k].pid == elementary_PID &&
							    ctx->potential_streams[k].stream_type == CCX_STREAM_TYPE_TELETEXT)
							{
								exists = 1;
								break;
							}
						}

						if (!exists)
						{
							struct ccx_stream_metadata *meta = &ctx->potential_streams[ctx->potential_stream_count++];
							meta->pid = elementary_PID;
							meta->stream_type = CCX_STREAM_TYPE_TELETEXT;
							meta->mpeg_type = stream_type;
							strcpy(meta->lang, "und"); // Teletext language extraction would need more work
							meta->lang[3] = '\0';
						}
					}
				}
			}
		}

		if (!IS_FEASIBLE(ctx->codec, ctx->nocodec, CCX_CODEC_TELETEXT) &&
		    stream_type == CCX_STREAM_TYPE_PRIVATE_MPEG2) // MPEG-2 Packetized Elementary Stream packets containing private data
		{
			unsigned descriptor_tag = buf[i + 5];
			if (descriptor_tag == 0x45)
			{
				update_capinfo(ctx, elementary_PID, stream_type, CCX_CODEC_ATSC_CC, program_number, NULL);
				// mprint ("VBI stream ID %u (0x%x) for SID %u (0x%x) - teletext is disabled, will be processed as closed captions.\n",
				//		elementary_PID, elementary_PID, program_number, program_number);
			}
		}

		// Support H.264, MPEG-2 and HEVC video streams
		if (stream_type == CCX_STREAM_TYPE_VIDEO_H264 || stream_type == CCX_STREAM_TYPE_VIDEO_MPEG2 || stream_type == CCX_STREAM_TYPE_VIDEO_HEVC)
		{
			if (stream_type == CCX_STREAM_TYPE_VIDEO_HEVC)
				mprint("Detected HEVC video stream (0x24) - enabling ATSC CC parsing.\n");
			update_capinfo(ctx, elementary_PID, stream_type, CCX_CODEC_ATSC_CC, program_number, NULL);
		}

		if (need_cap_info_for_pid(ctx, elementary_PID) == CCX_TRUE)
		{
			// We found the user selected CAPPID in PMT. We make a note of its type and don't
			// touch anything else
			if (stream_type >= 0x80 && stream_type <= 0xFF)
			{
				mprint("I can't tell the stream type of the manually selected PID.\n");
				mprint("Please pass -streamtype to select manually.\n");
				fatal(EXIT_FAILURE, "-streamtype has to be manually selected.");
			}
			update_capinfo(ctx, elementary_PID, stream_type, CCX_CODEC_NONE, program_number, NULL);
			continue;
		}

		// For the print command below
		unsigned tmp_stream_type = get_printable_stream_type(stream_type);
		dbg_print(CCX_DMT_VERBOSE, "  %s stream [0x%02x]  -  PID: %u\n",
			  desc[tmp_stream_type],
			  stream_type, elementary_PID);
		i += ES_info_length;
	}

	pinfo->analysed_PMT_once = CCX_TRUE;

	ret = verify_crc32(sbuf, olen);
	if (ret == CCX_FALSE)
		pinfo->valid_crc = CCX_FALSE;
	else
		pinfo->valid_crc = CCX_TRUE;

	pinfo->crc = (*(int32_t *)(sbuf + olen - 4));

	return must_flush;
}

void ts_buffer_psi_packet(struct ccx_demuxer *ctx)
{
	unsigned char *payload_start = tspacket + 4;
	unsigned payload_length = 188 - 4;
	//	unsigned transport_error_indicator = (tspacket[1]&0x80)>>7;
	unsigned payload_start_indicator = (tspacket[1] & 0x40) >> 6;
	// 	unsigned transport_priority = (tspacket[1]&0x20)>>5;
	unsigned pid = (((tspacket[1] & 0x1F) << 8) | tspacket[2]) & 0x1FFF;
	// 	unsigned transport_scrambling_control = (tspacket[3]&0xC0)>>6;
	unsigned adaptation_field_control = (tspacket[3] & 0x30) >> 4;
	unsigned ccounter = tspacket[3] & 0xF;
	unsigned adaptation_field_length = 0;

	if (adaptation_field_control & 2)
	{
		adaptation_field_length = tspacket[4];
		payload_start = payload_start + adaptation_field_length + 1;
		payload_length = tspacket + 188 - payload_start;
	}

	if (ctx->PID_buffers[pid] == NULL)
	{ // First packet for this pid. Create a buffer
		ctx->PID_buffers[pid] = malloc(sizeof(struct PSI_buffer));
		if (ctx->PID_buffers[pid] == NULL)
		{
			dbg_print(CCX_DMT_GENERIC_NOTICES, "\rWarning: Out of memory allocating PSI buffer for PID %u.\n", pid);
			return;
		}
		ctx->PID_buffers[pid]->buffer = NULL;
		ctx->PID_buffers[pid]->buffer_length = 0;
		ctx->PID_buffers[pid]->ccounter = 0;
		ctx->PID_buffers[pid]->prev_ccounter = 0xff;
	}

	// skip the packet if the adaptation field length or payload length are out of bounds or broken
	if (adaptation_field_length > 184 || payload_length > 184)
	{
		payload_length = 0;
		dbg_print(CCX_DMT_GENERIC_NOTICES, "\rWarning: Bad packet, adaptation field too long, skipping.\n");
	}

	if (payload_start_indicator)
	{
		if (ctx->PID_buffers[pid]->ccounter > 0)
		{
			ctx->PID_buffers[pid]->ccounter = 0;
		}
		ctx->PID_buffers[pid]->prev_ccounter = ccounter;

		if (ctx->PID_buffers[pid]->buffer != NULL)
			free(ctx->PID_buffers[pid]->buffer);
		else
		{
			// must be first packet for PID
		}
		ctx->PID_buffers[pid]->buffer = (uint8_t *)malloc(payload_length);
		if (ctx->PID_buffers[pid]->buffer == NULL)
		{
			dbg_print(CCX_DMT_GENERIC_NOTICES, "\rWarning: Out of memory allocating buffer for PID %u.\n", pid);
			ctx->PID_buffers[pid]->buffer_length = 0;
			return;
		}
		memcpy(ctx->PID_buffers[pid]->buffer, payload_start, payload_length);
		ctx->PID_buffers[pid]->buffer_length = payload_length;
		ctx->PID_buffers[pid]->ccounter++;
	}
	else if (ccounter == ctx->PID_buffers[pid]->prev_ccounter + 1 || (ctx->PID_buffers[pid]->prev_ccounter == 0x0f && ccounter == 0))
	{
		ctx->PID_buffers[pid]->prev_ccounter = ccounter;
		void *tmp = realloc(ctx->PID_buffers[pid]->buffer, ctx->PID_buffers[pid]->buffer_length + payload_length);
		if (tmp == NULL)
		{
			dbg_print(CCX_DMT_GENERIC_NOTICES, "\rWarning: Out of memory reallocating buffer for PID %u.\n", pid);
			return;
		}
		ctx->PID_buffers[pid]->buffer = (uint8_t *)tmp;
		memcpy(ctx->PID_buffers[pid]->buffer + ctx->PID_buffers[pid]->buffer_length, payload_start, payload_length);
		ctx->PID_buffers[pid]->ccounter++;
		ctx->PID_buffers[pid]->buffer_length += payload_length;
	}
	else if (ctx->PID_buffers[pid]->prev_ccounter <= 0x0f)
	{
		dbg_print(CCX_DMT_GENERIC_NOTICES, "\rWarning: Out of order packets detected for PID: %u.\n\
       ctx->PID_buffers[pid]->prev_ccounter:%" PRIu32 " ctx->ctx->PID_buffers[pid]->ccounter:%" PRIu32 "\n",
			  pid, ctx->PID_buffers[pid]->prev_ccounter, ctx->PID_buffers[pid]->ccounter);
	}
}

/* Program Allocation Table. It contains a list of all programs and the
   PIDs of their Program Map Table.
   Returns: gotpes */

int parse_PAT(struct ccx_demuxer *ctx)
{
	int gotpes = 0;
	int is_multiprogram = 0;
	unsigned char pointer_field = 0;
	unsigned char *payload_start = NULL;
	unsigned int payload_length = 0;
	unsigned int section_number = 0;
	unsigned int last_section_number = 0;

	pointer_field = *(ctx->PID_buffers[0]->buffer);

	payload_start = ctx->PID_buffers[0]->buffer + pointer_field + 1;
	payload_length = ctx->PID_buffers[0]->buffer_length - (pointer_field + 1);

	section_number = payload_start[6];
	last_section_number = payload_start[7];

	unsigned section_length = (((payload_start[1] & 0x0F) << 8) | payload_start[2]);
	payload_length = ctx->PID_buffers[0]->buffer_length - 8;
	unsigned programm_data = section_length - 5 - 4; // prev. bytes and CRC

	if (programm_data + 4 > payload_length)
	{
		return 0; // We don't have the full section yet. We will parse again when we have it.
	}

	if (section_number > last_section_number) // Impossible: Defective PAT
	{
		dbg_print(CCX_DMT_PAT, "Skipped defective PAT packet, section_number=%u but last_section_number=%u\n",
			  section_number, last_section_number);
		return gotpes;
	}
	if (last_section_number > 0)
	{
		dbg_print(CCX_DMT_PAT, "Long PAT packet (%u / %u), skipping.\n",
			  section_number, last_section_number);
		return gotpes;
		/* fatal(CCX_COMMON_EXIT_BUG_BUG,
		   "Sorry, long PATs not yet supported!\n"); */
	}

	if (ctx->last_pat_payload != NULL && payload_length == ctx->last_pat_length &&
	    !memcmp(payload_start, ctx->last_pat_payload, payload_length))
	{
		// dbg_print(CCX_DMT_PAT, "PAT hasn't changed, skipping.\n");
		return CCX_OK;
	}

	if (ctx->last_pat_payload != NULL)
	{
		mprint("Notice: PAT changed, clearing all variables.\n");
		dinit_cap(ctx);
		clear_PMT_array(ctx);
		memset(ctx->PIDs_seen, 0, sizeof(int) * 65536); // Forget all we saw
		if (!tlt_config.user_page)			// If the user didn't select a page...
			tlt_config.page = 0;			// ..forget whatever we detected.

		gotpes = 1;
	}

	if (ctx->last_pat_length < payload_length + 8)
	{
		ctx->last_pat_payload = (unsigned char *)realloc(ctx->last_pat_payload, payload_length + 8); // Extra 8 in case memcpy copies dwords, etc
		if (ctx->last_pat_payload == NULL)
		{
			fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory to process PAT.\n");
			return -1;
		}
	}
	memcpy(ctx->last_pat_payload, payload_start, payload_length);
	ctx->last_pat_length = payload_length;

	unsigned table_id = payload_start[0];
	unsigned transport_stream_id = ((payload_start[3] << 8) | payload_start[4]);
	unsigned version_number = (payload_start[5] & 0x3E) >> 1;

	// Means current OR next (so you can build a long PAT before it
	// actually is to be in use).
	unsigned current_next_indicator = payload_start[5] & 0x01;

	if (!current_next_indicator)
		// This table is not active, no need to evaluate
		return 0;

	payload_start += 8;

	dbg_print(CCX_DMT_PAT, "Read PAT packet (id: %u) ts-id: 0x%04x\n",
		  table_id, transport_stream_id);
	dbg_print(CCX_DMT_PAT, "  section length: %u  number: %u  last: %u\n",
		  section_length, section_number, last_section_number);
	dbg_print(CCX_DMT_PAT, "  version_number: %u  current_next_indicator: %u\n",
		  version_number, current_next_indicator);

	dbg_print(CCX_DMT_PAT, "\nProgram association section (PAT)\n");

	ctx->freport.program_cnt = 0;
	for (unsigned i = 0; i < programm_data; i += 4)
	{
		unsigned program_number = ((payload_start[i] << 8) | payload_start[i + 1]);
		if (!program_number)
			continue;
		ctx->freport.program_cnt++;
	}

	is_multiprogram = (ctx->freport.program_cnt > 1);

	for (unsigned int i = 0; i < programm_data; i += 4)
	{
		unsigned program_number = ((payload_start[i] << 8) | payload_start[i + 1]);
		unsigned prog_map_pid = ((payload_start[i + 2] << 8) | payload_start[i + 3]) & 0x1FFF;
		int j = 0;

		dbg_print(CCX_DMT_PAT, "  Program number: %u  -> PMTPID: %u\n",
			  program_number, prog_map_pid);

		if (!program_number)
			continue;

		/**
		 * loop never break at j == ctx->nb_program when program_number
		 * is already there in pinfo array and if we have program number
		 * already in our array we don't need to update our array
		 * so we break if program_number already exist and make j != ctx->nb_program
		 *
		 * Loop without break means j would be equal to ctx->nb_program
		 */
		for (j = 0; j < ctx->nb_program; j++)
		{
			if (ctx->pinfo[j].program_number == program_number)
			{
				if (ctx->flag_ts_forced_pn == CCX_TRUE && ctx->pinfo[j].pid == CCX_UNKNOWN)
				{
					ctx->pinfo[j].pid = prog_map_pid;
					ctx->pinfo[j].analysed_PMT_once = CCX_FALSE;
				}
				break;
			}
		}
		if (j == ctx->nb_program && ctx->flag_ts_forced_pn == CCX_FALSE)
			update_pinfo(ctx, prog_map_pid, program_number);
	} // for

	if (is_multiprogram && !ctx->flag_ts_forced_pn)
	{
		mprint("\nThis TS file has more than one program. These are the program numbers found: \n");
		for (unsigned j = 0; j < programm_data; j += 4)
		{
			unsigned pn = ((payload_start[j] << 8) | payload_start[j + 1]);
			if (pn)
				mprint("%u\n", pn);
			activity_program_number(pn);
		}
	}

	return gotpes;
}

void process_ccx_mpeg_descriptor(unsigned char *data, unsigned length)
{
	const char *txt_teletext_type[] = {"Reserved", "Initial page", "Subtitle page", "Additional information page", "Programme schedule page",
					   "Subtitle page for hearing impaired people"};
	int i, l;
	if (!data || !length)
		return;
	switch (data[0])
	{
		case CCX_MPEG_DSC_ISO639_LANGUAGE:
			if (length < 2)
				return;
			l = data[1];
			if (l + 2 < length)
				return;
			for (i = 0; i < l; i += 4)
			{
				char c1 = data[i + 2], c2 = data[i + 3], c3 = data[i + 4];
				dbg_print(CCX_DMT_PMT, "             ISO639: %c%c%c\n", c1 >= 0x20 ? c1 : ' ',
					  c2 >= 0x20 ? c2 : ' ',
					  c3 >= 0x20 ? c3 : ' ');
			}
			break;
		case CCX_MPEG_DSC_VBI_DATA_DESCRIPTOR:
			dbg_print(CCX_DMT_PMT, "DVB VBI data descriptor (not implemented)\n");
			break;
		case CCX_MPEG_DSC_VBI_TELETEXT_DESCRIPTOR:
			dbg_print(CCX_DMT_PMT, "DVB VBI teletext descriptor\n");
			break;
		case CCX_MPEG_DSC_TELETEXT_DESCRIPTOR:
			dbg_print(CCX_DMT_PMT, "             DVB teletext descriptor\n");
			if (length < 2)
				return;
			l = data[1];
			if (l + 2 < length)
				return;
			for (i = 0; i < l; i += 5)
			{
				char c1 = data[i + 2], c2 = data[i + 3], c3 = data[i + 4];
				unsigned teletext_type = (data[i + 5] & 0xF8) >> 3; // 5 MSB
				// unsigned magazine_number=data[i+5]&0x7; // 3 LSB
				unsigned teletext_page_number = data[i + 6];
				dbg_print(CCX_DMT_PMT, "                        ISO639: %c%c%c\n", c1 >= 0x20 ? c1 : ' ',
					  c2 >= 0x20 ? c2 : ' ',
					  c3 >= 0x20 ? c3 : ' ');
				dbg_print(CCX_DMT_PMT, "                 Teletext type: %s (%02X)\n", (teletext_type < 6 ? txt_teletext_type[teletext_type] : "Reserved for future use"),
					  teletext_type);
				dbg_print(CCX_DMT_PMT, "                  Initial page: %02X\n", teletext_page_number);
			}
			break;
		case CCX_MPEG_DSC_DVB_SUBTITLE:
			dbg_print(CCX_DMT_PMT, "             DVB Subtitle descriptor\n");
			break;

		default:
			if (data[0] == CCX_MPEG_DSC_REGISTRATION) // Registration descriptor, could be useful eventually
				break;
			if (data[0] == CCX_MPEG_DSC_DATA_STREAM_ALIGNMENT) // Data stream alignment descriptor
				break;
			if (data[0] >= 0x13 && data[0] <= 0x3F) // Reserved
				break;
			if (data[0] >= 0x40 && data[0] <= 0xFF) // User private
				break;
			// mprint ("Still unsupported MPEG descriptor type=%d (%02X)\n",data[0],data[0]);
			break;
	}
}

void decode_service_descriptors(struct ccx_demuxer *ctx, uint8_t *buf, uint32_t length, uint32_t service_id)
{
	unsigned descriptor_tag;
	unsigned descriptor_length;
	uint32_t x;
	uint32_t offset = 0;

	while (offset + 5 < length)
	{
		descriptor_tag = buf[offset];
		descriptor_length = buf[offset + 1];
		offset += 2;
		if (descriptor_tag == 0x48)
		{ // service descriptor
			// uint8_t service_type = buf[offset];
			uint8_t service_provider_name_length = buf[offset + 1];
			offset += 2;
			if (offset + service_provider_name_length > length)
			{
				dbg_print(CCX_DMT_GENERIC_NOTICES, "\rWarning: Invalid SDT service_provider_name_length detected.\n");
				return;
			}
			offset += service_provider_name_length; // Service provider name. Not sure what this is useful for.
			uint8_t service_name_length = buf[offset];
			offset++;
			if (offset + service_name_length > length)
			{
				dbg_print(CCX_DMT_GENERIC_NOTICES, "\rWarning: Invalid SDT service_name_length detected.\n");
				return;
			}
			for (x = 0; x < ctx->nb_program; x++)
			{
				// Not sure if programs can have multiple names (in different encodings?) Need more samples.
				// For now just assume the last one in the loop is as good as any if there are multiple.
				if (ctx->pinfo[x].program_number == service_id && service_name_length < 199)
				{
					char *s = EPG_DVB_decode_string(&buf[offset], service_name_length); // String encoding is the same as for EPG
					if (strlen(s) < MAX_PROGRAM_NAME_LEN - 1)
					{
						memcpy(ctx->pinfo[x].name, s, service_name_length);
						ctx->pinfo[x].name[service_name_length] = '\0';
					}
					free(s);
				}
			}
			offset += service_name_length;
		}
		else
		{
			// Some other tag
			offset += descriptor_length;
		}
	}
}

void decode_SDT_services_loop(struct ccx_demuxer *ctx, uint8_t *buf, uint32_t length)
{
	// unsigned descriptor_tag = buf[0];
	// unsigned descriptor_length = buf[1];
	// uint32_t x;
	uint32_t offset = 0;

	while (offset + 5 < length)
	{
		uint16_t serive_id = ((buf[offset + 0]) << 8) | buf[offset + 1];
		uint32_t descriptors_loop_length = (((buf[offset + 3] & 0x0F) << 8) | buf[offset + 4]);
		offset += 5;
		if (offset + descriptors_loop_length > length)
		{
			dbg_print(CCX_DMT_GENERIC_NOTICES, "\rWarning: Invalid SDT descriptors_loop_length detected.\n");
			return;
		}
		decode_service_descriptors(ctx, &buf[offset], descriptors_loop_length, serive_id);
		offset += descriptors_loop_length;
	}
}

void parse_SDT(struct ccx_demuxer *ctx)
{
	unsigned char pointer_field = 0;
	unsigned char *payload_start = NULL;
	unsigned int payload_length = 0;
	// unsigned int section_number = 0;
	// unsigned int last_section_number = 0;

	pointer_field = *(ctx->PID_buffers[0x11]->buffer);
	payload_start = ctx->PID_buffers[0x11]->buffer + pointer_field + 1;
	payload_length = ctx->PID_buffers[0x11]->buffer_length - (pointer_field + 1);

	// section_number = payload_start[6];
	// last_section_number = payload_start[7];

	unsigned table_id = payload_start[0];
	unsigned section_length = (((payload_start[1] & 0x0F) << 8) | payload_start[2]);
	// unsigned transport_stream_id = ((payload_start[3] << 8)
	//		| payload_start[4]);
	// unsigned version_number = (payload_start[5] & 0x3E) >> 1;

	if (section_length > payload_length - 4)
	{
		return;
	}
	unsigned current_next_indicator = payload_start[5] & 0x01;

	// uint16_t original_network_id = ((payload_start[8]) << 8) | payload_start[9];

	if (!current_next_indicator)
		// This table is not active, no need to evaluate
		return;
	if (table_id != 0x42)
		// This table isn't for the active TS
		return;

	decode_SDT_services_loop(ctx, &payload_start[11], section_length - 4 - 8);
}
