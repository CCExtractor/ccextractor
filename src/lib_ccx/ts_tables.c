#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "dvb_subtitle_decoder.h"
#include "utility.h"
#include "activity.h"

static unsigned pmt_warning_shown=0; // Only display warning once
void process_ccx_mpeg_descriptor (unsigned char *data, unsigned length);

unsigned get_printable_stream_type (unsigned ccx_stream_type)
{
	unsigned tmp_stream_type=ccx_stream_type;
	switch (ccx_stream_type)
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
		default:
			if (ccx_stream_type>=0x80 && ccx_stream_type<=0xFF)
				tmp_stream_type=CCX_STREAM_TYPE_PRIVATE_USER_MPEG2;
			else
				tmp_stream_type = CCX_STREAM_TYPE_UNKNOWNSTREAM;
			break;
	}
	return tmp_stream_type;
}

void clear_PMT_array (struct ccx_demuxer *ctx)
{
	if(ctx->flag_ts_forced_pn == CCX_FALSE)
	{
		ctx->nb_program = 0;
	}
}

int need_program(struct ccx_demuxer *ctx)
{
	if(ctx->nb_program == 0)
		return CCX_TRUE;

	if(ctx->nb_program == 1 && ctx->ts_autoprogram == CCX_TRUE)
		return CCX_FALSE;

	return CCX_FALSE;
}
int update_pinfo(struct ccx_demuxer *ctx, int pid, int program_number)
{
	if(!ctx)
		return -1;

	if(ctx->nb_program >= MAX_PROGRAM)
		return -1;

	ctx->pinfo[ctx->nb_program].pid = pid;
	ctx->pinfo[ctx->nb_program].program_number = program_number;
	ctx->pinfo[ctx->nb_program].analysed_PMT_once = CCX_FALSE;
	ctx->nb_program++;

	return CCX_OK;
}
/* Process Program Map Table - The PMT contains a list of streams in a program.
   Input: pos => Index in the PAT array
   Returns: Changes in the selected PID=1, No changes=0, if changes then if the
   buffer had anything it should be flushed.
   PMT specs: ISO13818-1 / table 2-28
   */

int parse_PMT (struct ccx_demuxer *ctx, struct ts_payload *payload, unsigned char *buf, int len,  struct program_info *pinfo)
{
	int must_flush=0;
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

	uint16_t PCR_PID;
	uint16_t pi_length;

	crc = (*(int32_t*)(sbuf+olen-4));
	table_id = buf[0];
	if(table_id != 0x2)
	{
		mprint("Warning: As per MPEG table defination PMT must have table id 0x2\n");
	}

	section_length = (((buf[1] & 0x0F) << 8)| buf[2]);
	if(section_length > (len - 3))
	{
		mprint("Long PMTs are not supported - skipped.\n");
		return 0;
	}

	program_number = ((buf[3] << 8)	| buf[4]);
	version_number = (buf[5] & 0x3E) >> 1;

	current_next_indicator = buf[5] & 0x01;
	// This table is not active, no need to evaluate
	if (!current_next_indicator)
		return 0;

	memcpy (pinfo->saved_section, buf, len);

	if (pinfo->analysed_PMT_once == CCX_TRUE && pinfo->version == version_number)
	{
		if (pinfo->version == version_number)
		{
			/* Same Version number and there was valid or similar CRC last time */
			if ( (pinfo->valid_crc == CCX_TRUE ||
				pinfo->crc == crc) &&
				need_capInfo(ctx, program_number) == CCX_FALSE)
				return 0;

		}
		else if ( (pinfo->version+1)%32 != version_number)
			mprint("TS PMT:Glitch in version number incremnt");
	}
	pinfo->version = version_number;


	section_number = buf[6];
	last_section_number = buf[7];
	if ( last_section_number > 0 )
	{
		mprint("Long PMTs are not supported - skipped.\n");
		return 0;
	}

	PCR_PID = (((buf[8] & 0x1F) << 8)
			| buf[9]);
	pi_length = (((buf[10] & 0x0F) << 8)
			| buf[11]);

	if( 12 + pi_length >  len )
	{
		// If we would support long PMTs, this would be wrong.
		mprint("program_info_length cannot be longer than the payload_length - skipped\n");
		return 0;
	}
	buf += 12 + pi_length;
	len -= (12 + pi_length);


	unsigned stream_data = section_length - 9 - pi_length - 4; // prev. bytes and CRC

	dbg_print(CCX_DMT_PARSE, "Read PMT packet  (id: %u) program number: %u\n",
			table_id, program_number);
	dbg_print(CCX_DMT_PARSE, "  section length: %u  number: %u  last: %u\n",
			section_length, section_number, last_section_number);
	dbg_print(CCX_DMT_PARSE, "  version_number: %u  current_next_indicator: %u\n",
			version_number, current_next_indicator);
	dbg_print(CCX_DMT_PARSE, "  PCR_PID: %u  data length: %u  payload_length: %u\n",
			PCR_PID, stream_data, len);

	if (!pmt_warning_shown && stream_data+4 > len )
	{
		dbg_print (CCX_DMT_GENERIC_NOTICES, "\rWarning: Probably parsing incomplete PMT, expected data longer than available payload.\n");
		pmt_warning_shown=1;
	}
	// Make a note of the program number for all PIDs, so we can report it later
	for( unsigned i=0; i < stream_data && (i+4)<len; i+=5)
	{
		unsigned ccx_stream_type = buf[i];
		unsigned elementary_PID = (((buf[i+1] & 0x1F) << 8)
				| buf[i+2]);
		unsigned ES_info_length = (((buf[i+3] & 0x0F) << 8)
				| buf[i+4]);
		if (ctx->PIDs_programs[elementary_PID]==NULL)
		{
			ctx->PIDs_programs[elementary_PID]=(struct PMT_entry *) malloc (sizeof (struct PMT_entry));
			if (ctx->PIDs_programs[elementary_PID]==NULL)
				fatal (EXIT_NOT_ENOUGH_MEMORY, "Not enough memory to process PMT.");
		}
		ctx->PIDs_programs[elementary_PID]->elementary_PID=elementary_PID;
		ctx->PIDs_programs[elementary_PID]->ccx_stream_type=ccx_stream_type;
		ctx->PIDs_programs[elementary_PID]->program_number=program_number;
		ctx->PIDs_programs[elementary_PID]->printable_stream_type=get_printable_stream_type (ccx_stream_type);
		dbg_print(CCX_DMT_PMT, "%6u | %3X (%3u) | %s\n",elementary_PID,ccx_stream_type,ccx_stream_type,
				desc[ctx->PIDs_programs[elementary_PID]->printable_stream_type]);
		process_ccx_mpeg_descriptor (buf+i+5,ES_info_length);
		i += ES_info_length;
	}
	dbg_print(CCX_DMT_PMT, "---\n");

	dbg_print(CCX_DMT_VERBOSE, "\nProgram map section (PMT)\n");

	for (unsigned i=0; i < stream_data && (i+4)<len; i+=5)
	{
		unsigned ccx_stream_type = buf[i];
		unsigned elementary_PID = (((buf[i+1] & 0x1F) << 8)
				| buf[i+2]);
		unsigned ES_info_length = (((buf[i+3] & 0x0F) << 8)
				| buf[i+4]);

		if (!ccx_options.print_file_reports ||
				ccx_stream_type != CCX_STREAM_TYPE_PRIVATE_MPEG2 ||
				!ES_info_length)
		{
			i += ES_info_length;
			continue;
		}

		unsigned char *es_info = buf + i + 5;
		for (desc_len = 0;(buf + i + 5 + ES_info_length) > es_info; es_info += desc_len)
		{
			enum ccx_mpeg_descriptor descriptor_tag = (enum ccx_mpeg_descriptor)(*es_info++);
			desc_len = (*es_info++);

			if(descriptor_tag == CCX_MPEG_DSC_DVB_SUBTITLE)
			{
				int k = 0;
				for (int j = 0; j < SUB_STREAMS_CNT; j++) {
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
			if(IS_VALID_TELETEXT_DESC(descriptor_tag))
			{
				int k = 0;
				for (int j = 0; j < SUB_STREAMS_CNT; j++) {
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
/*XXX
	if (TS_program_number || !ctx->ts_autoprogram)
	{
		if( payload.pid != pmtpid)
		{
			// This isn't the PMT we are interested in (note: If TS_program_number=0 &&
			// autoprogram then we need to check this PMT in case there's a suitable
			// stream)
			return 0;
		}
		if (program_number != TS_program_number) // Is this the PMT of the program we want?
		{
			// Only use PMTs with matching program number
			dbg_print(CCX_DMT_PARSE,"Reject this PMT packet (pid: %u) program number: %u\n",
					pmtpid, program_number);
			return 0;
		}
	}
*/
	for( unsigned i=0; i < stream_data && (i+4)<len; i+=5)
	{
		unsigned ccx_stream_type = buf[i];
		unsigned elementary_PID = (((buf[i+1] & 0x1F) << 8)
				| buf[i+2]);
		unsigned ES_info_length = (((buf[i+3] & 0x0F) << 8)
				| buf[i+4]);


		if(IS_FEASIBLE(ctx->codec, ctx->nocodec, CCX_CODEC_DVB) && ccx_stream_type == CCX_STREAM_TYPE_PRIVATE_MPEG2 &&
				ES_info_length  )
		{
			unsigned char *es_info = buf + i + 5;
			for (desc_len = 0;(buf + i + 5 + ES_info_length) > es_info ;es_info += desc_len)
			{
				enum ccx_mpeg_descriptor descriptor_tag = (enum ccx_mpeg_descriptor)(*es_info++);
				desc_len = (*es_info++);
#ifndef ENABLE_OCR
				if(ccx_options.write_format != CCX_OF_SPUPNG && CCX_MPEG_DSC_DVB_SUBTITLE == descriptor_tag)
				{
					mprint ("DVB subtitles detected, OCR subsystem not present. Use -out=spupng for graphic output\n");
					continue;
				}
#endif
				if(CCX_MPEG_DSC_DVB_SUBTITLE == descriptor_tag)
				{
					struct dvb_config cnf;
					void *ptr;
					memset((void*)&cnf,0,sizeof(struct dvb_config));
					ret = parse_dvb_description(&cnf,es_info,desc_len);
					if(ret < 0)
						break;
					ptr = dvbsub_init_decoder(&cnf);
					if (ptr == NULL)
						break;
					update_capinfo(ctx, elementary_PID, ccx_stream_type, CCX_CODEC_DVB, program_number, ptr);
					max_dif = 30;
				}
			}
		}

		if (IS_FEASIBLE(ctx->codec,ctx->nocodec,CCX_CODEC_TELETEXT) && ES_info_length
				&& ccx_stream_type == CCX_STREAM_TYPE_PRIVATE_MPEG2) // MPEG-2 Packetized Elementary Stream packets containing private data
		{
			unsigned char *es_info = buf + i + 5;
			for (desc_len = 0;(buf + i + 5 + ES_info_length) - es_info ;es_info += desc_len)
			{
				enum ccx_mpeg_descriptor descriptor_tag = (enum ccx_mpeg_descriptor)(*es_info++);
				void *ptr;
				desc_len = (*es_info++);
				if(!IS_VALID_TELETEXT_DESC(descriptor_tag))
					continue;
				ptr = telxcc_init();
				if (ptr == NULL)
					break;
				update_capinfo(ctx, elementary_PID, ccx_stream_type, CCX_CODEC_TELETEXT, program_number, ptr);
				//mprint ("VBI/teletext stream ID %u (0x%x) for SID %u (0x%x)\n",
				//		elementary_PID, elementary_PID, program_number, program_number);
			}

		}

		if (!IS_FEASIBLE(ctx->codec,ctx->nocodec,CCX_CODEC_TELETEXT) &&
				ccx_stream_type == CCX_STREAM_TYPE_PRIVATE_MPEG2) // MPEG-2 Packetized Elementary Stream packets containing private data
		{
			unsigned descriptor_tag = buf[i + 5];
			if (descriptor_tag == 0x45)
			{
				update_capinfo(ctx, elementary_PID, ccx_stream_type, CCX_CODEC_ATSC_CC, program_number, NULL);
				//mprint ("VBI stream ID %u (0x%x) for SID %u (0x%x) - teletext is disabled, will be processed as closed captions.\n",
				//		elementary_PID, elementary_PID, program_number, program_number);
			}
		}

		if (ccx_stream_type==CCX_STREAM_TYPE_VIDEO_H264 || ccx_stream_type==CCX_STREAM_TYPE_VIDEO_MPEG2)
		{
			update_capinfo(ctx, elementary_PID, ccx_stream_type, CCX_CODEC_ATSC_CC, program_number, NULL);
			//mprint ("Decode captions from program %d - %s stream [0x%02x]  -  PID: %u\n",
			//	program_number , desc[ccx_stream_type], ccx_stream_type, elementary_PID);
		}

		if(need_capInfo_for_pid(ctx, elementary_PID) == CCX_TRUE)
		{
			// We found the user selected CAPPID in PMT. We make a note of its type and don't
			// touch anything else
			if (ccx_stream_type>=0x80 && ccx_stream_type<=0xFF)
			{
				mprint ("I can't tell the stream type of the manually selected PID.\n");
				mprint ("Please pass -streamtype to select manually.\n");
				fatal (EXIT_FAILURE, "(user assistance needed)");
			}
			update_capinfo(ctx, elementary_PID, ccx_stream_type, CCX_CODEC_NONE, program_number, NULL);
			continue;
		}

		// For the print command below
		unsigned tmp_stream_type = get_printable_stream_type (ccx_stream_type);
		dbg_print(CCX_DMT_VERBOSE, "  %s stream [0x%02x]  -  PID: %u\n",
				desc[tmp_stream_type],
				ccx_stream_type, elementary_PID);
		i += ES_info_length;
	}

	pinfo->analysed_PMT_once = CCX_TRUE;

	ret = verify_crc32(sbuf, olen);
	if (ret == CCX_FALSE)
		pinfo->valid_crc = CCX_FALSE;
	else
		pinfo->valid_crc = CCX_TRUE;

	pinfo->crc = (*(int32_t*)(sbuf+olen-4));

	return must_flush;
}

int write_section(struct ccx_demuxer *ctx, struct ts_payload *payload, unsigned char*buf, int size,  struct program_info *pinfo)
{
	static int in_error = 0;  // If a broken section arrives we will skip everything until the next PES start
	if (size < 0)
	{
		in_error = 1;
		return 0;
	}
	if (payload->pesstart)
		in_error = 0;
	if (in_error) 
		return 0; 
	if (payload->pesstart)
	{
		if (size <= 0xFFF + 3)
		{
			memcpy(payload->section_buf, buf, size);
			payload->section_index = size;
			payload->section_size = -1;
		}
		else
		{
			mprint("Invalid section buffer size %d \n", size);
			return 0;
		}
	}
	else
	{
		if ( (payload->section_index + size) < (0xFFF + 3))
		{
			memcpy(payload->section_buf + payload->section_index, buf, size);
			payload->section_index += size;
		}
		else
		{
			mprint("Invalid section buffer size %d section index %d\n", size, payload->section_index);
			return 0;
		}

	}

	if(payload->section_size == -1 && payload->section_index >= 3)
		payload->section_size = (RB16(payload->section_buf + 1) & 0xfff) + 3 ;

	if(payload->section_index >= (unsigned)payload->section_size)
	{
		if(parse_PMT(ctx, payload, payload->section_buf,payload->section_size, pinfo))
			return 1;
	}
	return 0;

}

/* Program Allocation Table. It contains a list of all programs and the
   PIDs of their Program Map Table.
   Returns: gotpes */

int parse_PAT (struct ccx_demuxer *ctx, struct ts_payload *payload)
{
	int gotpes = 0;
	int is_multiprogram = 0;
	unsigned char pointer_field = 0;
	unsigned char *payload_start = NULL;
	unsigned int payload_length = 0;
	unsigned int section_number = 0;
	unsigned int last_section_number = 0;

//	if(need_capInfo(ctx, 0) == CCX_FALSE)
//		return CCX_OK;

	if (payload->pesstart)
		pointer_field = *(payload->start);
	payload->start += pointer_field + 1;
	payload->length -= pointer_field + 1;

	payload_start = payload->start;
	payload_length = payload->length;

	section_number = payload_start[6];
	last_section_number = payload_start[7];

	/* if ((forced_cappid || telext_mode==CCX_TXT_IN_USE) && cap_stream_type!=CCX_STREAM_TYPE_UNKNOWNSTREAM) // Already know what we need, skip
		return 0;  */

	if (!payload->pesstart)
		// Not the first entry. Ignore it, may be Pat larger then 184.
		return 0;

	if (section_number > last_section_number) // Impossible: Defective PAT
	{
		dbg_print(CCX_DMT_PAT, "Skipped defective PAT packet, section_number=%u but last_section_number=%u\n",
				section_number, last_section_number);
		return gotpes;
	}
	if ( last_section_number > 0 )
	{
		dbg_print(CCX_DMT_PAT, "Long PAT packet (%u / %u), skipping.\n",
				section_number, last_section_number);
		return gotpes;
		/* fatal(CCX_COMMON_EXIT_BUG_BUG,
		   "Sorry, long PATs not yet supported!\n"); */
	}

	if (ctx->last_pat_payload != NULL && payload_length == ctx->last_pat_length &&
			!memcmp (payload_start, ctx->last_pat_payload, payload_length))
	{
		// dbg_print(CCX_DMT_PAT, "PAT hasn't changed, skipping.\n");
		return CCX_OK;
	}

	if (ctx->last_pat_payload != NULL)
	{
		mprint ("Notice: PAT changed, clearing all variables.\n");
		dinit_cap(ctx);
		clear_PMT_array(ctx);
		memset (ctx->PIDs_seen,0,sizeof (int) *65536); // Forget all we saw
		if (!tlt_config.user_page) // If the user didn't select a page...
			tlt_config.page=0; // ..forget whatever we detected.

		gotpes=1;
	}

	if(ctx->last_pat_length < payload_length + 8)
	{
		ctx->last_pat_payload = (unsigned char *) realloc (ctx->last_pat_payload, payload_length+8); // Extra 8 in case memcpy copies dwords, etc
		if (ctx->last_pat_payload == NULL)
		{
			fatal (EXIT_NOT_ENOUGH_MEMORY, "Not enough memory to process PAT.\n");
			return -1;
		}
	}
	memcpy (ctx->last_pat_payload, payload_start, payload_length);
	ctx->last_pat_length = payload_length;


	unsigned table_id = payload_start[0];
	unsigned section_length = (((payload_start[1] & 0x0F) << 8)
			| payload_start[2]);
	unsigned transport_stream_id = ((payload_start[3] << 8)
			| payload_start[4]);
	unsigned version_number = (payload_start[5] & 0x3E) >> 1;

	// Means current OR next (so you can build a long PAT before it
	// actually is to be in use).
	unsigned current_next_indicator = payload_start[5] & 0x01;



	if (!current_next_indicator)
		// This table is not active, no need to evaluate
		return 0;

	payload_start += 8;
	payload_length = tspacket+188-payload_start;

	unsigned programm_data = section_length - 5 - 4; // prev. bytes and CRC

	dbg_print(CCX_DMT_PAT, "Read PAT packet (id: %u) ts-id: 0x%04x\n",
			table_id, transport_stream_id);
	dbg_print(CCX_DMT_PAT, "  section length: %u  number: %u  last: %u\n",
			section_length, section_number, last_section_number);
	dbg_print(CCX_DMT_PAT, "  version_number: %u  current_next_indicator: %u\n",
			version_number, current_next_indicator);

	if ( programm_data+4 > payload_length )
	{
		fatal(CCX_COMMON_EXIT_BUG_BUG,
				"Sorry, PAT too long!\n");
	}

	unsigned ts_prog_num = 0;
	dbg_print(CCX_DMT_PAT, "\nProgram association section (PAT)\n");

	ctx->freport.program_cnt=0;
	for( unsigned i=0; i < programm_data; i+=4)
	{
		unsigned program_number = ((payload_start[i] << 8)
				| payload_start[i+1]);
		if( !program_number )
			continue;
		ctx->freport.program_cnt++;
	}

	is_multiprogram = (ctx->freport.program_cnt>1);

	for( unsigned int i = 0; i < programm_data; i+=4)
	{
		unsigned program_number = ((payload_start[i] << 8)
				| payload_start[i+1]);
		unsigned prog_map_pid = ((payload_start[i+2] << 8)
				| payload_start[i+3]) & 0x1FFF;
		int j = 0;

		dbg_print(CCX_DMT_PAT, "  Program number: %u  -> PMTPID: %u\n",
				program_number, prog_map_pid);

		if( !program_number )
			continue;

		for (j = 0; j < ctx->nb_program; j++)
		{
			if (ctx->pinfo[j].program_number == program_number)
			{
				if(ctx->flag_ts_forced_pn == CCX_TRUE && ctx->pinfo[j].pid == CCX_UNKNOWN)
				{
					ctx->pinfo[j].pid = prog_map_pid;
					ctx->pinfo[j].analysed_PMT_once = CCX_FALSE;
				}
				break;
			}
		}
		if( j == ctx->nb_program && ctx->flag_ts_forced_pn == CCX_FALSE)
			update_pinfo(ctx, prog_map_pid, program_number);
	} // for

	if (is_multiprogram && !ts_prog_num)
	{
		mprint ("\nThis TS file has more than one program. These are the program numbers found: \n");
		for( unsigned j=0; j < programm_data; j+=4)
		{
			unsigned pn = ((payload_start[j] << 8)
					| payload_start[j+1]);
			if (pn)
				mprint ("%u\n",pn);
			activity_program_number (pn);
		}
	}

	return gotpes;
}

void process_ccx_mpeg_descriptor (unsigned char *data, unsigned length)
{
	const char *txt_teletext_type[]={"Reserved", "Initial page", "Subtitle page", "Additional information page", "Programme schedule page",
		"Subtitle page for hearing impaired people"};
	int i,l;
	if (!data || !length)
		return;
	switch (data[0])
	{
		case CCX_MPEG_DSC_ISO639_LANGUAGE:
			if (length<2)
				return;
			l=data[1];
			if (l+2<length)
				return;
			for (i=0;i<l;i+=4)
			{
				char c1=data[i+2], c2=data[i+3], c3=data[i+4];
				dbg_print(CCX_DMT_PMT, "             ISO639: %c%c%c\n",c1>=0x20?c1:' ',
																   c2>=0x20?c2:' ',
																   c3>=0x20?c3:' ');
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
			if (length<2)
				return;
			l=data[1];
			if (l+2<length)
				return;
			for (i=0;i<l;i+=5)
			{
				char c1=data[i+2], c2=data[i+3], c3=data[i+4];
				unsigned teletext_type=(data[i+5]&0xF8)>>3; // 5 MSB
				// unsigned magazine_number=data[i+5]&0x7; // 3 LSB
				unsigned teletext_page_number=data[i+6];
				dbg_print (CCX_DMT_PMT, "                        ISO639: %c%c%c\n",c1>=0x20?c1:' ',
																   c2>=0x20?c2:' ',
																   c3>=0x20?c3:' ');
				dbg_print (CCX_DMT_PMT, "                 Teletext type: %s (%02X)\n", (teletext_type<6? txt_teletext_type[teletext_type]:"Reserved for future use"),
					teletext_type);
				dbg_print (CCX_DMT_PMT, "                  Initial page: %02X\n",teletext_page_number);
			}
			break;
		case CCX_MPEG_DSC_DVB_SUBTITLE:
			dbg_print(CCX_DMT_PMT, "             DVB Subtitle descriptor\n");
			break;

		default:
			if (data[0]==CCX_MPEG_DSC_REGISTRATION) // Registration descriptor, could be useful eventually
				break;
			if (data[0]==CCX_MPEG_DSC_DATA_STREAM_ALIGNMENT) // Data stream alignment descriptor
				break;
			if (data[0]>=0x13 && data[0]<=0x3F) // Reserved
				break;
			if (data[0]>=0x40 && data[0]<=0xFF) // User private
				break;
			//mprint ("Still unsupported MPEG descriptor type=%d (%02X)\n",data[0],data[0]);
			break;
	}
}
