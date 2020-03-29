#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "utility.h"
#include <math.h>
#include "avc_functions.h"

#define dvprint(...) dbg_print(CCX_DMT_VIDES, __VA_ARGS__)
// Functions to parse a AVC/H.264 data stream, see ISO/IEC 14496-10

// local functions
static unsigned char *remove_03emu(unsigned char *from, unsigned char *to);
static void sei_rbsp(struct avc_ctx *ctx, unsigned char *seibuf, unsigned char *seiend);
static unsigned char *sei_message(struct avc_ctx *ctx, unsigned char *seibuf, unsigned char *seiend);
static void user_data_registered_itu_t_t35(struct avc_ctx *ctx, unsigned char *userbuf, unsigned char *userend);
static void seq_parameter_set_rbsp(struct avc_ctx *ctx, unsigned char *seqbuf, unsigned char *seqend);
static void slice_header(struct encoder_ctx *enc_ctx, struct lib_cc_decode *ctx, unsigned char *heabuf, unsigned char *heaend, int nal_unit_type, struct cc_subtitle *sub);

double roundportable(double x) { return floor(x + 0.5); }

int ebsp_to_rbsp(char *rbsp, char *ebsp, int length);
void dinit_avc(struct avc_ctx **ctx)
{
	struct avc_ctx *lctx = *ctx;
	if (lctx->ccblocks_in_avc_lost > 0)
	{
		mprint("Total caption blocks received: %d\n", lctx->ccblocks_in_avc_total);
		mprint("Total caption blocks lost: %d\n", lctx->ccblocks_in_avc_lost);
	}
	freep(&lctx->cc_data);
	freep(ctx);
}

struct avc_ctx *init_avc(void)
{
	struct avc_ctx *ctx = malloc(sizeof(struct avc_ctx));
	if (!ctx)
		return NULL;

	ctx->cc_data = (unsigned char *)malloc(1024);
	if (!ctx->cc_data)
	{
		free(ctx);
		return NULL;
	}

	ctx->cc_count = 0;
	// buffer to hold cc data
	ctx->cc_databufsize = 1024;
	ctx->cc_buffer_saved = CCX_TRUE; // Was the CC buffer saved after it was last updated?

	ctx->got_seq_para = 0;
	ctx->nal_ref_idc = 0;
	ctx->seq_parameter_set_id = 0;
	ctx->log2_max_frame_num = 0;
	ctx->pic_order_cnt_type = 0;
	ctx->log2_max_pic_order_cnt_lsb = 0;
	ctx->frame_mbs_only_flag = 0;

	ctx->ccblocks_in_avc_total = 0;
	ctx->ccblocks_in_avc_lost = 0;
	ctx->frame_num = -1;
	ctx->lastframe_num = -1;

	ctx->currref = 0;
	ctx->maxidx = -1;
	ctx->lastmaxidx = -1;

	// Used to find tref zero in PTS mode
	ctx->minidx = 10000;
	ctx->lastminidx = 10000;

	// Used to remember the max temporal reference number (poc mode)
	ctx->maxtref = 0;
	ctx->last_gop_maxtref = 0;

	// Used for PTS ordering of CC blocks
	ctx->currefpts = 0;

	ctx->last_pic_order_cnt_lsb = -1;
	ctx->last_slice_pts = -1;

	ctx->num_nal_unit_type_7 = 0;
	ctx->num_vcl_hrd = 0;
	ctx->num_nal_hrd = 0;
	ctx->num_jump_in_frames = 0;
	ctx->num_unexpected_sei_length = 0;
	return ctx;
}

void do_NAL(struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx, unsigned char *NAL_start, LLONG NAL_length, struct cc_subtitle *sub)
{
	unsigned char *NAL_stop;
	enum ccx_avc_nal_types nal_unit_type = *NAL_start & 0x1F;

	NAL_stop = NAL_length + NAL_start;
	NAL_stop = remove_03emu(NAL_start + 1, NAL_stop); // Add +1 to NAL_stop for TS, without it for MP4. Still don't know why

	dvprint("BEGIN NAL unit type: %d length %d ref_idc: %d - Buffered captions before: %d\n",
		nal_unit_type, NAL_stop - NAL_start - 1, dec_ctx->avc_ctx->nal_ref_idc, !dec_ctx->avc_ctx->cc_buffer_saved);

	if (NAL_stop == NULL) // remove_03emu failed.
	{
		mprint("\rNotice: NAL of type %u had to be skipped because remove_03emu failed.\n", nal_unit_type);
		return;
	}

	if (nal_unit_type == CCX_NAL_TYPE_ACCESS_UNIT_DELIMITER_9)
	{
		// Found Access Unit Delimiter
	}
	else if (nal_unit_type == CCX_NAL_TYPE_SEQUENCE_PARAMETER_SET_7)
	{
		// Found sequence parameter set
		// We need this to parse NAL type 1 (CCX_NAL_TYPE_CODED_SLICE_NON_IDR_PICTURE_1)
		dec_ctx->avc_ctx->num_nal_unit_type_7++;
		seq_parameter_set_rbsp(dec_ctx->avc_ctx, NAL_start + 1, NAL_stop);
		dec_ctx->avc_ctx->got_seq_para = 1;
	}
	else if (dec_ctx->avc_ctx->got_seq_para && (nal_unit_type == CCX_NAL_TYPE_CODED_SLICE_NON_IDR_PICTURE_1 ||
						    nal_unit_type == CCX_NAL_TYPE_CODED_SLICE_IDR_PICTURE)) // Only if nal_unit_type=1
	{
		// Found coded slice of a non-IDR picture
		// We only need the slice header data, no need to implement
		// slice_layer_without_partitioning_rbsp( );
		slice_header(enc_ctx, dec_ctx, NAL_start + 1, NAL_stop, nal_unit_type, sub);
	}
	else if (dec_ctx->avc_ctx->got_seq_para && nal_unit_type == CCX_NAL_TYPE_SEI)
	{
		// Found SEI (used for subtitles)
		//set_fts(ctx->timing); // FIXME - check this!!!
		sei_rbsp(dec_ctx->avc_ctx, NAL_start + 1, NAL_stop);
	}
	else if (dec_ctx->avc_ctx->got_seq_para && nal_unit_type == CCX_NAL_TYPE_PICTURE_PARAMETER_SET)
	{
		// Found Picture parameter set
	}
	if (temp_debug)
	{
		int len = NAL_stop - (NAL_start + 1);
		dbg_print(CCX_DMT_VIDES, "\n After decoding, the actual thing was (length =%d)\n", len);
		dump(CCX_DMT_VIDES, NAL_start + 1, len > 160 ? 160 : len, 0, 0);
	}

	dvprint("END   NAL unit type: %d length %d ref_idc: %d - Buffered captions after: %d\n",
		nal_unit_type, NAL_stop - NAL_start - 1, dec_ctx->avc_ctx->nal_ref_idc, !dec_ctx->avc_ctx->cc_buffer_saved);
}

// Process inbuf bytes in buffer holding and AVC (H.264) video stream.
// The number of processed bytes is returned.
size_t process_avc(struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx, unsigned char *avcbuf, size_t avcbuflen, struct cc_subtitle *sub)
{
	unsigned char *buffer_position = avcbuf;
	unsigned char *NAL_start;
	unsigned char *NAL_stop;

	// At least 5 bytes are needed for a NAL unit
	if (avcbuflen <= 5)
	{
		fatal(CCX_COMMON_EXIT_BUG_BUG,
		      "NAL unit needs at last 5 bytes in the buffer to process AVC video stream...");
	}

	// Warning there should be only leading zeros, nothing else
	if (!(buffer_position[0] == 0x00 && buffer_position[1] == 0x00))
	{
		fatal(CCX_COMMON_EXIT_BUG_BUG,
		      "Broken AVC stream - Leading bytes are non-zero...");
	}
	buffer_position = buffer_position + 2;

	int firstloop = 1; // Check for valid start code at buffer start

	// Loop over NAL units
	while (buffer_position < avcbuf + avcbuflen - 2) // buffer_position points to 0x01 plus at least two bytes
	{
		int zeropad = 0; // Count leading zeros

		// Find next NAL_start
		while (buffer_position < avcbuf + avcbuflen)
		{
			if (*buffer_position == 0x01)
			{
				// OK, found a start code
				break;
			}
			else if (firstloop && *buffer_position != 0x00)
			{
				// Not 0x00 or 0x01
				fatal(CCX_COMMON_EXIT_BUG_BUG,
				      "Broken AVC stream - Leading bytes are non-zero...");
			}
			buffer_position++;
			zeropad++;
		}
		firstloop = 0;
		if (buffer_position >= avcbuf + avcbuflen)
		{
			// No new start sequence
			break;
		}
		NAL_start = buffer_position + 1;

		// Find next start code or buffer end
		LLONG restlen;
		do
		{
			// Search for next 000000 or 000001
			buffer_position++;
			restlen = avcbuf - buffer_position + avcbuflen - 2; // leave room for two more bytes

			// Find the next zero
			if (restlen > 0)
			{
				buffer_position = (unsigned char *)memchr(buffer_position, 0x00, (size_t)restlen);

				if (!buffer_position)
				{
					// No 0x00 till the end of the buffer
					NAL_stop = avcbuf + avcbuflen;
					buffer_position = NAL_stop;
					break;
				}

				if (buffer_position[1] == 0x00 && (buffer_position[2] | 0x01) == 0x01)
				{
					// Found new start code
					NAL_stop = buffer_position;
					buffer_position = buffer_position + 2; // Move after the two leading 0x00
					break;
				}
			}
			else
			{
				NAL_stop = avcbuf + avcbuflen;
				buffer_position = NAL_stop;
				break;
			}
		} while (restlen); // Should never be true - loop is exited via break

		if (*NAL_start & 0x80)
		{
			dump(CCX_DMT_GENERIC_NOTICES, NAL_start - 4, 10, 0, 0);
			fatal(CCX_COMMON_EXIT_BUG_BUG,
			      "Broken AVC stream - forbidden_zero_bit not zero ...");
		}

		dec_ctx->avc_ctx->nal_ref_idc = *NAL_start >> 5;
		dvprint("process_avc: zeropad %d\n", zeropad);
		do_NAL(enc_ctx, dec_ctx, NAL_start, NAL_stop - NAL_start, sub);
	}

	return avcbuflen;
}

#define ZEROBYTES_SHORTSTARTCODE 2

// Copied for reference decoder, see if it behaves different that Volker's code
int EBSPtoRBSP(unsigned char *streamBuffer, int end_bytepos, int begin_bytepos)
{
	int i, j, count;
	count = 0;

	if (end_bytepos < begin_bytepos)
		return end_bytepos;

	j = begin_bytepos;

	for (i = begin_bytepos; i < end_bytepos; ++i)
	{ //starting from begin_bytepos to avoid header information
		//in NAL unit, 0x000000, 0x000001 or 0x000002 shall not occur at any byte-aligned position
		if (count == ZEROBYTES_SHORTSTARTCODE && streamBuffer[i] < 0x03)
			return -1;
		if (count == ZEROBYTES_SHORTSTARTCODE && streamBuffer[i] == 0x03)
		{
			//check the 4th byte after 0x000003, except when cabac_zero_word is used, in which case the last three bytes of this NAL unit must be 0x000003
			if ((i < end_bytepos - 1) && (streamBuffer[i + 1] > 0x03))
				return -1;
			//if cabac_zero_word is used, the final byte of this NAL unit(0x03) is discarded, and the last two bytes of RBSP must be 0x0000
			if (i == end_bytepos - 1)
				return j;

			++i;
			count = 0;
		}
		streamBuffer[j] = streamBuffer[i];
		if (streamBuffer[i] == 0x00)
			++count;
		else
			count = 0;
		++j;
	}

	return j;
}
typedef unsigned int u32;

// Remove 0x000003 emulation sequences. We do this on-site (same buffer) because the new array
// will never be longer than the source.
u32 avc_remove_emulation_bytes(const unsigned char *buffer_src, unsigned char *buffer_dst, u32 nal_size);

unsigned char *remove_03emu(unsigned char *from, unsigned char *to)
{
	int num = to - from;
	int newsize = EBSPtoRBSP(from, num, 0); //TODO: Do something if newsize == -1 (broken NAL)
	if (newsize == -1)
		return NULL;
	return from + newsize;
}

// Process SEI payload in AVC data. This function combines sei_rbsp()
// and rbsp_trailing_bits().
void sei_rbsp(struct avc_ctx *ctx, unsigned char *seibuf, unsigned char *seiend)
{
	unsigned char *tbuf = seibuf;
	while (tbuf < seiend - 1) // Use -1 because of trailing marker
	{
		tbuf = sei_message(ctx, tbuf, seiend - 1);
	}
	if (tbuf == seiend - 1)
	{
		if (*tbuf != 0x80)
			mprint("Strange rbsp_trailing_bits value: %02X\n", *tbuf);
		else
			dvprint("\n");
	}
	else
	{
		// TODO: This really really looks bad
		mprint("WARNING: Unexpected SEI unit length...trying to continue.");
		temp_debug = 1;
		mprint("\n Failed block (at sei_rbsp) was:\n");
		dump(CCX_DMT_GENERIC_NOTICES, (unsigned char *)seibuf, seiend - seibuf, 0, 0);

		ctx->num_unexpected_sei_length++;
	}
}

// This combines sei_message() and sei_payload().
unsigned char *sei_message(struct avc_ctx *ctx, unsigned char *seibuf, unsigned char *seiend)
{
	int payload_type = 0;
	while (*seibuf == 0xff)
	{
		payload_type += 255;
		seibuf++;
	}
	payload_type += *seibuf;
	seibuf++;

	int payload_size = 0;
	while (*seibuf == 0xff)
	{
		payload_size += 255;
		seibuf++;
	}
	payload_size += *seibuf;
	seibuf++;

	int broken = 0;
	unsigned char *payload_start = seibuf;
	seibuf += payload_size;

	dvprint("Payload type: %d size: %d - ", payload_type, payload_size);
	if (seibuf > seiend)
	{
		// TODO: What do we do here?
		broken = 1;
		if (payload_type == 4)
		{
			dbg_print(CCX_DMT_VERBOSE, "Warning: Subtitles payload seems incorrect (too long), continuing but it doesn't look good..");
		}
		else
		{
			dbg_print(CCX_DMT_VERBOSE, "Warning: Non-subtitles payload seems incorrect (too long), continuing but it doesn't look good..");
		}
	}
	dbg_print(CCX_DMT_VERBOSE, "\n");
	// Ignore all except user_data_registered_itu_t_t35() payload
	if (!broken && payload_type == 4)
		user_data_registered_itu_t_t35(ctx, payload_start, payload_start + payload_size);

	return seibuf;
}

void copy_ccdata_to_buffer(struct avc_ctx *ctx, char *source, int new_cc_count)
{
	ctx->ccblocks_in_avc_total++;
	if (ctx->cc_buffer_saved == CCX_FALSE)
	{
		mprint("Warning: Probably loss of CC data, unsaved buffer being rewritten, trailing end might get lost\n");
		ctx->ccblocks_in_avc_lost++;
	}
	memcpy(ctx->cc_data + ctx->cc_count * 3, source, new_cc_count * 3 + 1);
	ctx->cc_count += new_cc_count;
	ctx->cc_buffer_saved = CCX_FALSE;
}

void user_data_registered_itu_t_t35(struct avc_ctx *ctx, unsigned char *userbuf, unsigned char *userend)
{
	unsigned char *tbuf = userbuf;
	unsigned char *cc_tmp_data;
	unsigned char process_cc_data_flag;
	int user_data_type_code;
	int user_data_len;
	int local_cc_count = 0;
	// int cc_count;
	int itu_t_t35_country_code = *((uint8_t *)tbuf);
	tbuf++;
	int itu_t_35_provider_code = *tbuf * 256 + *(tbuf + 1);
	tbuf += 2;

	// ANSI/SCTE 128 2008:
	// itu_t_t35_country_code == 0xB5
	// itu_t_35_provider_code == 0x0031
	// see spec for details - no example -> no support

	// Example files (sample.ts, ...):
	// itu_t_t35_country_code == 0xB5
	// itu_t_35_provider_code == 0x002F
	// user_data_type_code == 0x03 (cc_data)
	// user_data_len == next byte (length after this byte up to (incl) marker.)
	// cc_data struct (CEA-708)
	// marker == 0xFF

	if (itu_t_t35_country_code != 0xB5)
	{
		mprint("Not a supported user data SEI\n");
		mprint("  itu_t_35_country_code: %02x\n", itu_t_t35_country_code);
		return;
	}

	switch (itu_t_35_provider_code)
	{
		case 0x0031: // ANSI/SCTE 128
			dbg_print(CCX_DMT_VERBOSE, "Caption block in ANSI/SCTE 128...");
			if (*tbuf == 0x47 && *(tbuf + 1) == 0x41 && *(tbuf + 2) == 0x39 && *(tbuf + 3) == 0x34) // ATSC1_data() - GA94
			{
				dbg_print(CCX_DMT_VERBOSE, "ATSC1_data()...\n");
				tbuf += 4;
				unsigned char user_data_type_code = *tbuf;
				tbuf++;
				switch (user_data_type_code)
				{
					case 0x03:
						dbg_print(CCX_DMT_VERBOSE, "cc_data (finally)!\n");
						/*
						   cc_count = 2; // Forced test
						   process_cc_data_flag = (*tbuf & 2) >> 1;
						   mandatory_1 = (*tbuf & 1);
						   mandatory_0 = (*tbuf & 4) >>2;
						   if (!mandatory_1 || mandatory_0)
						   {
						   printf ("Essential tests not passed.\n");
						   break;
						   }
						 */
						local_cc_count = *tbuf & 0x1F;
						process_cc_data_flag = (*tbuf & 0x40) >> 6;

						/* if (!process_cc_data_flag)
						   {
						   mprint ("process_cc_data_flag == 0, skipping this caption block.\n");
						   break;
						   } */
						/*
							  The following tests are not passed in Comcast's sample videos. *tbuf here is always 0x41.
							  if (! (*tbuf & 0x80)) // First bit must be 1
							  {
							  printf ("Fixed bit should be 1, but it's 0 - skipping this caption block.\n");
							  break;
							  }
							  if (*tbuf & 0x20) // Third bit must be 0
							  {
							  printf ("Fixed bit should be 0, but it's 1 - skipping this caption block.\n");
							  break;
							  } */
						tbuf++;
						/*
						   Another test that the samples ignore. They contain 00!
						   if (*tbuf!=0xFF)
						   {
						   printf ("Fixed value should be 0xFF, but it's %02X - skipping this caption block.\n", *tbuf);
						   } */
						// OK, all checks passed!
						tbuf++;
						cc_tmp_data = tbuf;

						/* TODO: I don't think we have user_data_len here
						   if (cc_count*3+3 != user_data_len)
						   fatal(CCX_COMMON_EXIT_BUG_BUG,
						   "Syntax problem: user_data_len != cc_count*3+3."); */

						// Enough room for CC captions?
						if (cc_tmp_data + local_cc_count * 3 >= userend)
							fatal(CCX_COMMON_EXIT_BUG_BUG,
							      "Syntax problem: Too many caption blocks.");
						if (cc_tmp_data[local_cc_count * 3] != 0xFF)
						{
							// See GitHub Issue #1001 for the related change
							mprint("\rWarning! Syntax problem: Final 0xFF marker missing. Continuing...\n");
							break; // Skip Block
						}
						// Save the data and process once we know the sequence number
						if (((ctx->cc_count + local_cc_count) * 3) + 1 > ctx->cc_databufsize)
						{
							ctx->cc_data = (unsigned char *)realloc(ctx->cc_data, (size_t)((ctx->cc_count + local_cc_count) * 6) + 1);
							if (!ctx->cc_data)
								fatal(EXIT_NOT_ENOUGH_MEMORY, "In user_data_registered_itu_t_t35: Out of memory to allocate buffer for CC data.");
							ctx->cc_databufsize = (long)((ctx->cc_count + local_cc_count) * 6) + 1;
						}
						// Copy new cc data into cc_data
						copy_ccdata_to_buffer(ctx, (char *)cc_tmp_data, local_cc_count);
						break;
					case 0x06:
						dbg_print(CCX_DMT_VERBOSE, "bar_data (unsupported for now)\n");
						break;
					default:
						dbg_print(CCX_DMT_VERBOSE, "SCTE/ATSC reserved.\n");
				}
			}
			else if (*tbuf == 0x44 && *(tbuf + 1) == 0x54 && *(tbuf + 2) == 0x47 && *(tbuf + 3) == 0x31) // afd_data() - DTG1
			{
				;
				// Active Format Description Data. Actually unrelated to captions. Left
				// here in case we want to do some reporting eventually. From specs:
				// "Active Format Description (AFD) should be included in video user
				// data whenever the rectangular picture area containing useful
				// information does not extend to the full height or width of the coded
				// frame. AFD data may also be included in user data when the
				// rectangular picture area containing
				// useful information extends to the fullheight and width of the
				// coded frame."
			}
			else
			{
				dbg_print(CCX_DMT_VERBOSE, "SCTE/ATSC reserved.\n");
			}
			break;
		case 0x002F:
			dbg_print(CCX_DMT_VERBOSE, "ATSC1_data() - provider_code = 0x002F\n");

			user_data_type_code = *((uint8_t *)tbuf);
			if (user_data_type_code != 0x03)
			{
				dbg_print(CCX_DMT_VERBOSE, "Not supported  user_data_type_code: %02x\n",
					  user_data_type_code);
				return;
			}
			tbuf++;
			user_data_len = *((uint8_t *)tbuf);
			tbuf++;

			local_cc_count = *tbuf & 0x1F;
			process_cc_data_flag = (*tbuf & 0x40) >> 6;
			if (!process_cc_data_flag)
			{
				mprint("process_cc_data_flag == 0, skipping this caption block.\n");
				break;
			}
			cc_tmp_data = tbuf + 2;

			if (local_cc_count * 3 + 3 != user_data_len)
				fatal(CCX_COMMON_EXIT_BUG_BUG,
				      "Syntax problem: user_data_len != cc_count*3+3.");

			// Enough room for CC captions?
			if (cc_tmp_data + local_cc_count * 3 >= userend)
				fatal(CCX_COMMON_EXIT_BUG_BUG,
				      "Syntax problem: Too many caption blocks.");
			if (cc_tmp_data[local_cc_count * 3] != 0xFF)
				fatal(CCX_COMMON_EXIT_BUG_BUG,
				      "Syntax problem: Final 0xFF marker missing.");

			// Save the data and process once we know the sequence number
			if ((((local_cc_count + ctx->cc_count) * 3) + 1) > ctx->cc_databufsize)
			{
				ctx->cc_data = (unsigned char *)realloc(ctx->cc_data, (size_t)(((local_cc_count + ctx->cc_count) * 6) + 1));
				if (!ctx->cc_data)
					fatal(EXIT_NOT_ENOUGH_MEMORY, "In user_data_registered_itu_t_t35: Not enough memory trying to allocate buffer for CC data.");
				ctx->cc_databufsize = (long)(((local_cc_count + ctx->cc_count) * 6) + 1);
			}
			// Copy new cc data into cc_data - replace command below.
			copy_ccdata_to_buffer(ctx, (char *)cc_tmp_data, local_cc_count);

			//dump(tbuf,user_data_len-1,0);
			break;
		default:
			mprint("Not a supported user data SEI\n");
			mprint("  itu_t_35_provider_code: %04x\n", itu_t_35_provider_code);
			break;
	}
}

// Process sequence parameters in AVC data.
void seq_parameter_set_rbsp(struct avc_ctx *ctx, unsigned char *seqbuf, unsigned char *seqend)
{
	LLONG tmp, tmp1;
	struct bitstream q1;
	init_bitstream(&q1, seqbuf, seqend);

	dvprint("SEQUENCE PARAMETER SET (bitlen: %lld)\n", q1.bitsleft);
	tmp = read_int_unsigned(&q1, 8);
	LLONG profile_idc = tmp;
	dvprint("profile_idc=                                   % 4lld (%#llX)\n", tmp, tmp);
	tmp = read_int_unsigned(&q1, 1);
	dvprint("constraint_set0_flag=                          % 4lld (%#llX)\n", tmp, tmp);
	tmp = read_int_unsigned(&q1, 1);
	dvprint("constraint_set1_flag=                          % 4lld (%#llX)\n", tmp, tmp);
	tmp = read_int_unsigned(&q1, 1);
	dvprint("constraint_set2_flag=                          % 4lld (%#llX)\n", tmp, tmp);
	tmp = read_int_unsigned(&q1, 1);
	dvprint("constraint_set3_flag=                          % 4lld (%#llX)\n", tmp, tmp);
	tmp = read_int_unsigned(&q1, 1);
	dvprint("constraint_set4_flag=                          % 4lld (%#llX)\n", tmp, tmp);
	tmp = read_int_unsigned(&q1, 1);
	dvprint("constraint_set5_flag=                          % 4lld (%#llX)\n", tmp, tmp);
	tmp = read_int_unsigned(&q1, 2);
	dvprint("reserved=                                      % 4lld (%#llX)\n", tmp, tmp);
	tmp = read_int_unsigned(&q1, 8);
	dvprint("level_idc=                                     % 4lld (%#llX)\n", tmp, tmp);
	ctx->seq_parameter_set_id = read_exp_golomb_unsigned(&q1);
	dvprint("seq_parameter_set_id=                          % 4lld (%#llX)\n", ctx->seq_parameter_set_id, ctx->seq_parameter_set_id);
	if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 || profile_idc == 244 || profile_idc == 44 || profile_idc == 83 || profile_idc == 86 || profile_idc == 118 || profile_idc == 128)
	{
		LLONG chroma_format_idc = read_exp_golomb_unsigned(&q1);
		dvprint("chroma_format_idc=                             % 4lld (%#llX)\n", chroma_format_idc, chroma_format_idc);
		if (chroma_format_idc == 3)
		{
			tmp = read_int_unsigned(&q1, 1);
			dvprint("separate_colour_plane_flag=                    % 4lld (%#llX)\n", tmp, tmp);
		}
		tmp = read_exp_golomb_unsigned(&q1);
		dvprint("bit_depth_luma_minus8=                         % 4lld (%#llX)\n", tmp, tmp);
		tmp = read_exp_golomb_unsigned(&q1);
		dvprint("bit_depth_chroma_minus8=                       % 4lld (%#llX)\n", tmp, tmp);
		tmp = read_int_unsigned(&q1, 1);
		dvprint("qpprime_y_zero_transform_bypass_flag=          % 4lld (%#llX)\n", tmp, tmp);
		tmp = read_int_unsigned(&q1, 1);
		dvprint("seq_scaling_matrix_present_flag=               % 4lld (%#llX)\n", tmp, tmp);
		if (tmp == 1)
		{
			// WVI: untested, just copied from specs.
			for (int i = 0; i < ((chroma_format_idc != 3) ? 8 : 12); i++)
			{
				tmp = read_int_unsigned(&q1, 1);
				dvprint("seq_scaling_list_present_flag[%d]=                 % 4lld (%#llX)\n", i, tmp, tmp);
				if (tmp)
				{
					// We use a "dummy"/slimmed-down replacement here. Actual/full code can be found in the spec (ISO/IEC 14496-10:2012(E)) chapter 7.3.2.1.1.1 - Scaling list syntax
					if (i < 6)
					{
						// Scaling list size 16
						// TODO: replace with full scaling list implementation?
						int nextScale = 8;
						int lastScale = 8;
						for (int j = 0; j < 16; j++)
						{
							if (nextScale != 0)
							{
								int64_t delta_scale = read_exp_golomb(&q1);
								nextScale = (lastScale + delta_scale + 256) % 256;
							}
							lastScale = (nextScale == 0) ? lastScale : nextScale;
						}
						// END of TODO
					}
					else
					{
						// Scaling list size 64
						// TODO: replace with full scaling list implementation?
						int nextScale = 8;
						int lastScale = 8;
						for (int j = 0; j < 64; j++)
						{
							if (nextScale != 0)
							{
								int64_t delta_scale = read_exp_golomb(&q1);
								nextScale = (lastScale + delta_scale + 256) % 256;
							}
							lastScale = (nextScale == 0) ? lastScale : nextScale;
						}
						// END of TODO
					}
				}
			}
		}
	}
	ctx->log2_max_frame_num = (int)read_exp_golomb_unsigned(&q1);
	dvprint("log2_max_frame_num4_minus4=                    % 4d (%#X)\n", ctx->log2_max_frame_num, ctx->log2_max_frame_num);
	ctx->log2_max_frame_num += 4; // 4 is added due to the formula.
	ctx->pic_order_cnt_type = (int)read_exp_golomb_unsigned(&q1);
	dvprint("pic_order_cnt_type=                            % 4d (%#X)\n", ctx->pic_order_cnt_type, ctx->pic_order_cnt_type);
	if (ctx->pic_order_cnt_type == 0)
	{
		ctx->log2_max_pic_order_cnt_lsb = (int)read_exp_golomb_unsigned(&q1);
		dvprint("log2_max_pic_order_cnt_lsb_minus4=             % 4d (%#X)\n", ctx->log2_max_pic_order_cnt_lsb, ctx->log2_max_pic_order_cnt_lsb);
		ctx->log2_max_pic_order_cnt_lsb += 4; // 4 is added due to formula.
	}
	else if (ctx->pic_order_cnt_type == 1)
	{
		// CFS: Untested, just copied from specs.
		tmp = read_int_unsigned(&q1, 1);
		dvprint("delta_pic_order_always_zero_flag=              % 4lld (%#llX)\n", tmp, tmp);
		tmp = read_exp_golomb(&q1);
		dvprint("offset_for_non_ref_pic=                        % 4lld (%#llX)\n", tmp, tmp);
		tmp = read_exp_golomb(&q1);
		dvprint("offset_for_top_to_bottom_field                 % 4lld (%#llX)\n", tmp, tmp);
		LLONG num_ref_frame_in_pic_order_cnt_cycle = read_exp_golomb_unsigned(&q1);
		dvprint("num_ref_frame_in_pic_order_cnt_cycle           % 4lld (%#llX)\n", num_ref_frame_in_pic_order_cnt_cycle, num_ref_frame_in_pic_order_cnt_cycle);
		for (int i = 0; i < num_ref_frame_in_pic_order_cnt_cycle; i++)
		{
			tmp = read_exp_golomb(&q1);
			dvprint("offset_for_ref_frame [%d / %d] =               % 4lld (%#llX)\n", i, num_ref_frame_in_pic_order_cnt_cycle, tmp, tmp);
		}
	}
	else
	{
		// Nothing needs to be parsed when pic_order_cnt_type == 2
	}

	tmp = read_exp_golomb_unsigned(&q1);
	dvprint("max_num_ref_frames=                            % 4lld (%#llX)\n", tmp, tmp);
	tmp = read_int_unsigned(&q1, 1);
	dvprint("gaps_in_frame_num_value_allowed_flag=          % 4lld (%#llX)\n", tmp, tmp);
	tmp = read_exp_golomb_unsigned(&q1);
	dvprint("pic_width_in_mbs_minus1=                       % 4lld (%#llX)\n", tmp, tmp);
	tmp = read_exp_golomb_unsigned(&q1);
	dvprint("pic_height_in_map_units_minus1=                % 4lld (%#llX)\n", tmp, tmp);
	ctx->frame_mbs_only_flag = (int)read_int_unsigned(&q1, 1);
	dvprint("frame_mbs_only_flag=                           % 4d (%#X)\n", ctx->frame_mbs_only_flag, ctx->frame_mbs_only_flag);
	if (!ctx->frame_mbs_only_flag)
	{
		tmp = read_int_unsigned(&q1, 1);
		dvprint("mb_adaptive_fr_fi_flag=                        % 4lld (%#llX)\n", tmp, tmp);
	}
	tmp = read_int_unsigned(&q1, 1);
	dvprint("direct_8x8_inference_f=                        % 4lld (%#llX)\n", tmp, tmp);
	tmp = read_int_unsigned(&q1, 1);
	dvprint("frame_cropping_flag=                           % 4lld (%#llX)\n", tmp, tmp);
	if (tmp)
	{
		tmp = read_exp_golomb_unsigned(&q1);
		dvprint("frame_crop_left_offset=                        % 4lld (%#llX)\n", tmp, tmp);
		tmp = read_exp_golomb_unsigned(&q1);
		dvprint("frame_crop_right_offset=                       % 4lld (%#llX)\n", tmp, tmp);
		tmp = read_exp_golomb_unsigned(&q1);
		dvprint("frame_crop_top_offset=                         % 4lld (%#llX)\n", tmp, tmp);
		tmp = read_exp_golomb_unsigned(&q1);
		dvprint("frame_crop_bottom_offset=                      % 4lld (%#llX)\n", tmp, tmp);
	}
	tmp = read_int_unsigned(&q1, 1);
	dvprint("vui_parameters_present=                        % 4lld (%#llX)\n", tmp, tmp);
	if (tmp)
	{
		dvprint("\nVUI parameters\n");
		tmp = read_int_unsigned(&q1, 1);
		dvprint("aspect_ratio_info_pres=                        % 4lld (%#llX)\n", tmp, tmp);
		if (tmp)
		{
			tmp = read_int_unsigned(&q1, 8);
			dvprint("aspect_ratio_idc=                              % 4lld (%#llX)\n", tmp, tmp);
			if (tmp == 255)
			{
				tmp = read_int_unsigned(&q1, 16);
				dvprint("sar_width=                                     % 4lld (%#llX)\n", tmp, tmp);
				tmp = read_int_unsigned(&q1, 16);
				dvprint("sar_height=                                    % 4lld (%#llX)\n", tmp, tmp);
			}
		}
		tmp = read_int_unsigned(&q1, 1);
		dvprint("overscan_info_pres_flag=                       % 4lld (%#llX)\n", tmp, tmp);
		if (tmp)
		{
			tmp = read_int_unsigned(&q1, 1);
			dvprint("overscan_appropriate_flag=                     % 4lld (%#llX)\n", tmp, tmp);
		}
		tmp = read_int_unsigned(&q1, 1);
		dvprint("video_signal_type_present_flag=                % 4lld (%#llX)\n", tmp, tmp);
		if (tmp)
		{
			tmp = read_int_unsigned(&q1, 3);
			dvprint("video_format=                                  % 4lld (%#llX)\n", tmp, tmp);
			tmp = read_int_unsigned(&q1, 1);
			dvprint("video_full_range_flag=                         % 4lld (%#llX)\n", tmp, tmp);
			tmp = read_int_unsigned(&q1, 1);
			dvprint("colour_description_present_flag=               % 4lld (%#llX)\n", tmp, tmp);
			if (tmp)
			{
				tmp = read_int_unsigned(&q1, 8);
				dvprint("colour_primaries=                              % 4lld (%#llX)\n", tmp, tmp);
				tmp = read_int_unsigned(&q1, 8);
				dvprint("transfer_characteristics=                      % 4lld (%#llX)\n", tmp, tmp);
				tmp = read_int_unsigned(&q1, 8);
				dvprint("matrix_coefficients=                           % 4lld (%#llX)\n", tmp, tmp);
			}
		}
		tmp = read_int_unsigned(&q1, 1);
		dvprint("chroma_loc_info_present_flag=                  % 4lld (%#llX)\n", tmp, tmp);
		if (tmp)
		{
			tmp = read_exp_golomb_unsigned(&q1);
			dvprint("chroma_sample_loc_type_top_field=                  % 4lld (%#llX)\n", tmp, tmp);
			tmp = read_exp_golomb_unsigned(&q1);
			dvprint("chroma_sample_loc_type_bottom_field=               % 4lld (%#llX)\n", tmp, tmp);
		}
		tmp = read_int_unsigned(&q1, 1);
		dvprint("timing_info_present_flag=                      % 4lld (%#llX)\n", tmp, tmp);
		if (tmp)
		{
			tmp = read_int_unsigned(&q1, 32);
			LLONG num_units_in_tick = tmp;
			dvprint("num_units_in_tick=                             % 4lld (%#llX)\n", tmp, tmp);
			tmp = read_int_unsigned(&q1, 32);
			LLONG time_scale = tmp;
			dvprint("time_scale=                                    % 4lld (%#llX)\n", tmp, tmp);
			tmp = read_int_unsigned(&q1, 1);
			int fixed_frame_rate_flag = (int)tmp;
			dvprint("fixed_frame_rate_flag=                         % 4lld (%#llX)\n", tmp, tmp);
			// Change: use num_units_in_tick and time_scale to calculate FPS. (ISO/IEC 14496-10:2012(E), page 397 & further)
			if (fixed_frame_rate_flag)
			{
				double clock_tick = (double)num_units_in_tick / time_scale;
				dvprint("clock_tick= %f\n", clock_tick);
				if (current_fps != (double)time_scale / (2 * num_units_in_tick))
				{
					current_fps = (double)time_scale / (2 * num_units_in_tick); // Based on formula D-2, p. 359 of the ISO/IEC 14496-10:2012(E) spec.
					mprint("Changed fps using NAL to: %f\n", current_fps);
				}
			}
		}
		tmp = read_int_unsigned(&q1, 1);
		dvprint("nal_hrd_parameters_present_flag=               % 4lld (%#llX)\n", tmp, tmp);
		if (tmp)
		{
			dvprint("nal_hrd. Not implemented for now. Hopefully not needed. Skipping rest of NAL\n");
			//printf("Boom nal_hrd\n");
			// exit(1);
			ctx->num_nal_hrd++;
			return;
		}
		tmp1 = read_int_unsigned(&q1, 1);
		dvprint("vcl_hrd_parameters_present_flag=               %llX\n", tmp1);
		if (tmp)
		{
			// TODO.
			mprint("vcl_hrd. Not implemented for now. Hopefully not needed. Skipping rest of NAL\n");
			ctx->num_vcl_hrd++;
			// exit(1);
		}
		if (tmp || tmp1)
		{
			tmp = read_int_unsigned(&q1, 1);
			dvprint("low_delay_hrd_flag=                                % 4lld (%#llX)\n", tmp, tmp);
			return;
		}
		tmp = read_int_unsigned(&q1, 1);
		dvprint("pic_struct_present_flag=                       % 4lld (%#llX)\n", tmp, tmp);
		tmp = read_int_unsigned(&q1, 1);
		dvprint("bitstream_restriction_flag=                    % 4lld (%#llX)\n", tmp, tmp);
		// ..
		// The hope was to find the GOP length in max_dec_frame_buffering, but
		// it was not set in the testfile.  Ignore the rest here, it's
		// currently not needed.
	}
	//exit(1);
}

/**
	Process slice header in AVC data.
	Slice Header is parsed to get sequence of frames
*/
void slice_header(struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx, unsigned char *heabuf, unsigned char *heaend, int nal_unit_type, struct cc_subtitle *sub)
{
	LLONG tmp;
	struct bitstream q1;
	int max_frame_num;
	LLONG slice_type, bottom_field_flag = 0, pic_order_cnt_lsb = -1;
	int current_index;
	int ird_pic_flag;
	LLONG field_pic_flag = 0; // Moved here because it's needed for ctx->avc_ctx->pic_order_cnt_type==2

	if (init_bitstream(&q1, heabuf, heaend))
	{
		mprint("Skipping slice header due to failure in init_bitstream.\n");
		return;
	}

	ird_pic_flag = ((nal_unit_type == 5) ? 1 : 0);

	dvprint("\nSLICE HEADER\n");
	tmp = read_exp_golomb_unsigned(&q1);
	dvprint("first_mb_in_slice=     % 4lld (%#llX)\n", tmp, tmp);
	slice_type = read_exp_golomb_unsigned(&q1);
	dvprint("slice_type=            % 4llX\n", slice_type);
	tmp = read_exp_golomb_unsigned(&q1);
	dvprint("pic_parameter_set_id=  % 4lld (%#llX)\n", tmp, tmp);

	dec_ctx->avc_ctx->lastframe_num = dec_ctx->avc_ctx->frame_num;
	max_frame_num = (int)((1 << dec_ctx->avc_ctx->log2_max_frame_num) - 1);

	// Needs log2_max_frame_num_minus4 + 4 bits
	dec_ctx->avc_ctx->frame_num = read_int_unsigned(&q1, dec_ctx->avc_ctx->log2_max_frame_num);
	dvprint("frame_num=             % 4llX\n", dec_ctx->avc_ctx->frame_num);

	if (!dec_ctx->avc_ctx->frame_mbs_only_flag)
	{
		field_pic_flag = read_int_unsigned(&q1, 1);
		dvprint("field_pic_flag=        % 4llX\n", field_pic_flag);
		if (field_pic_flag)
		{
			// bottom_field_flag
			bottom_field_flag = read_int_unsigned(&q1, 1);
			dvprint("bottom_field_flag=     % 4llX\n", bottom_field_flag);

			// When bottom_field_flag is set the video is interlaced,
			// override current_fps.
			current_fps = framerates_values[dec_ctx->current_frame_rate];
		}
	}

	dvprint("ird_pic_flag=            % 4d\n", ird_pic_flag);

	if (nal_unit_type == 5)
	{
		tmp = read_exp_golomb_unsigned(&q1);
		dvprint("idr_pic_id=            % 4lld (%#llX)\n", tmp, tmp);
		//TODO
	}
	if (dec_ctx->avc_ctx->pic_order_cnt_type == 0)
	{
		pic_order_cnt_lsb = read_int_unsigned(&q1, dec_ctx->avc_ctx->log2_max_pic_order_cnt_lsb);
		dvprint("pic_order_cnt_lsb=     % 4llX\n", pic_order_cnt_lsb);
	}
	if (dec_ctx->avc_ctx->pic_order_cnt_type == 1)
	{
		fatal(CCX_COMMON_EXIT_BUG_BUG, "In slice_header: AVC: ctx->avc_ctx->pic_order_cnt_type == 1 not yet supported.");
	}

	//Ignore slice with same pic order or pts
	if (ccx_options.usepicorder)
	{
		if (dec_ctx->avc_ctx->last_pic_order_cnt_lsb == pic_order_cnt_lsb)
			return;
		dec_ctx->avc_ctx->last_pic_order_cnt_lsb = pic_order_cnt_lsb;
	}
	else
	{
		if (dec_ctx->timing->current_pts == dec_ctx->avc_ctx->last_slice_pts)
			return;
		dec_ctx->avc_ctx->last_slice_pts = dec_ctx->timing->current_pts;
	}
#if 0
	else
	{
		/* CFS: Warning!!: Untested stuff, copied from specs (8.2.1.3) */
		LLONG FrameNumOffset = 0;
		if (ird_pic_flag == 1)
			FrameNumOffset = 0;
		else if (lastframe_num > frame_num)
			FrameNumOffset = lastframe_num + max_frame_num;
		else
			FrameNumOffset = lastframe_num;

		LLONG tempPicOrderCnt = 0;
		if (ird_pic_flag == 1)
			tempPicOrderCnt = 0;
		else if (ctx->avc_ctx->nal_ref_idc == 0)
			tempPicOrderCnt = 2 * (FrameNumOffset + frame_num) - 1;
		else
			tempPicOrderCnt = 2 * (FrameNumOffset + frame_num);

		LLONG TopFieldOrderCnt = tempPicOrderCnt;
		LLONG BottomFieldOrderCnt = tempPicOrderCnt;

		if (!field_pic_flag) {
			TopFieldOrderCnt = tempPicOrderCnt;
			BottomFieldOrderCnt = tempPicOrderCnt;
		}
		else if (bottom_field_flag)
			BottomFieldOrderCnt = tempPicOrderCnt;
		else
			TopFieldOrderCnt = tempPicOrderCnt;

		//pic_order_cnt_lsb=tempPicOrderCnt;
		//pic_order_cnt_lsb=read_int_unsigned(&q1,tempPicOrderCnt);
		//fatal(CCX_COMMON_EXIT_BUG_BUG, "AVC: ctx->avc_ctx->pic_order_cnt_type != 0 not yet supported.");
		//TODO
		// Calculate picture order count (POC) according to 8.2.1
	}
#endif
	// The rest of the data in slice_header() is currently unused.

	// A reference pic (I or P is always the last displayed picture of a POC
	// sequence. B slices can be reference pics, so ignore ctx->avc_ctx->nal_ref_idc.
	int isref = 0;
	switch (slice_type)
	{
		// P-SLICES
		case 0:
		case 5:
			// I-SLICES
		case 2:
		case 7:
			isref = 1;
			break;
	}

	int maxrefcnt = (int)((1 << dec_ctx->avc_ctx->log2_max_pic_order_cnt_lsb) - 1);

	// If we saw a jump set maxidx, lastmaxidx to -1
	LLONG dif = dec_ctx->avc_ctx->frame_num - dec_ctx->avc_ctx->lastframe_num;
	if (dif == -max_frame_num)
		dif = 0;
	if (dec_ctx->avc_ctx->lastframe_num > -1 && (dif < 0 || dif > 1))
	{
		dec_ctx->avc_ctx->num_jump_in_frames++;
		dvprint("\nJump in frame numbers (%lld/%lld)\n", dec_ctx->avc_ctx->frame_num, dec_ctx->avc_ctx->lastframe_num);
		// This will prohibit setting current_tref on potential
		// jumps.
		dec_ctx->avc_ctx->maxidx = -1;
		dec_ctx->avc_ctx->lastmaxidx = -1;
	}

	// Sometimes two P-slices follow each other, see garbled_dishHD.mpg,
	// in this case we only treat the first as a reference pic
	if (isref && dec_ctx->frames_since_last_gop <= 3) // Used to be == 1, but the sample file
	{						  // 2014 SugarHouse Casino Mummers Parade Fancy Brigades_new.ts was garbled
		// Probably doing a proper PTS sort would be a better solution.
		isref = 0;
		dbg_print(CCX_DMT_TIME, "Ignoring this reference pic.\n");
	}

	// if slices are buffered - flush
	if (isref)
	{
		dvprint("\nReference pic! [%s]\n", slice_types[slice_type]);
		dbg_print(CCX_DMT_TIME, "\nReference pic! [%s] maxrefcnt: %3d\n",
			  slice_types[slice_type], maxrefcnt);

		// Flush buffered cc blocks before doing the housekeeping
		if (dec_ctx->has_ccdata_buffered)
		{
			process_hdcc(enc_ctx, dec_ctx, sub);
		}
		dec_ctx->last_gop_length = dec_ctx->frames_since_last_gop;
		dec_ctx->frames_since_last_gop = 0;
		dec_ctx->avc_ctx->last_gop_maxtref = dec_ctx->avc_ctx->maxtref;
		dec_ctx->avc_ctx->maxtref = 0;
		dec_ctx->avc_ctx->lastmaxidx = dec_ctx->avc_ctx->maxidx;
		dec_ctx->avc_ctx->maxidx = 0;
		dec_ctx->avc_ctx->lastminidx = dec_ctx->avc_ctx->minidx;
		dec_ctx->avc_ctx->minidx = 10000;

		if (ccx_options.usepicorder)
		{
			// Use pic_order_cnt_lsb

			// Make sure that current_index never wraps for curidx values that
			// are smaller than currref
			dec_ctx->avc_ctx->currref = (int)pic_order_cnt_lsb;
			if (dec_ctx->avc_ctx->currref < maxrefcnt / 3)
			{
				dec_ctx->avc_ctx->currref += maxrefcnt + 1;
			}

			// If we wrapped around lastmaxidx might be larger than
			// the current index - fix this.
			if (dec_ctx->avc_ctx->lastmaxidx > dec_ctx->avc_ctx->currref + maxrefcnt / 2) // implies lastmaxidx > 0
				dec_ctx->avc_ctx->lastmaxidx -= maxrefcnt + 1;
		}
		else
		{
			// Use PTS ordering
			dec_ctx->avc_ctx->currefpts = dec_ctx->timing->current_pts;
			dec_ctx->avc_ctx->currref = 0;
		}

		anchor_hdcc(dec_ctx, dec_ctx->avc_ctx->currref);
	}

	if (ccx_options.usepicorder)
	{
		// Use pic_order_cnt_lsb
		// Wrap (add max index value) current_index if needed.
		if (dec_ctx->avc_ctx->currref - pic_order_cnt_lsb > maxrefcnt / 2)
			current_index = (int)pic_order_cnt_lsb + maxrefcnt + 1;
		else
			current_index = (int)pic_order_cnt_lsb;

		// Track maximum index for this GOP
		if (current_index > dec_ctx->avc_ctx->maxidx)
			dec_ctx->avc_ctx->maxidx = current_index;

		// Calculate tref
		if (dec_ctx->avc_ctx->lastmaxidx > 0)
		{
			dec_ctx->timing->current_tref = current_index - dec_ctx->avc_ctx->lastmaxidx - 1;
			// Set maxtref
			if (dec_ctx->timing->current_tref > dec_ctx->avc_ctx->maxtref)
			{
				dec_ctx->avc_ctx->maxtref = dec_ctx->timing->current_tref;
			}
			// Now an ugly workaround where pic_order_cnt_lsb increases in
			// steps of two. The 1.5 is an approximation, it should be:
			// last_gop_maxtref+1 == last_gop_length*2
			if (dec_ctx->avc_ctx->last_gop_maxtref > dec_ctx->last_gop_length * 1.5)
			{
				dec_ctx->timing->current_tref = dec_ctx->timing->current_tref / 2;
			}
		}
		else
			dec_ctx->timing->current_tref = 0;

		if (dec_ctx->timing->current_tref < 0)
		{
			mprint("current_tref is negative!?\n");
		}
	}
	else
	{
		// Use PTS ordering - calculate index position from PTS difference and
		// frame rate
		// The 2* accounts for a discrepancy between current and actual FPS
		// seen in some files (CCSample2.mpg)
		current_index = (int)roundportable(2 * (dec_ctx->timing->current_pts - dec_ctx->avc_ctx->currefpts) / (MPEG_CLOCK_FREQ / current_fps));

		if (abs(current_index) >= MAXBFRAMES)
		{
			// Probably a jump in the timeline. Warn and handle gracefully.
			mprint("\nFound large gap(%d) in PTS! Trying to recover ...\n", current_index);
			current_index = 0;
		}

		// Track maximum index for this GOP
		if (current_index > dec_ctx->avc_ctx->maxidx)
			dec_ctx->avc_ctx->maxidx = current_index;

		// Track minimum index for this GOP
		if (current_index < dec_ctx->avc_ctx->minidx)
			dec_ctx->avc_ctx->minidx = current_index;

		dec_ctx->timing->current_tref = 1;
		if (current_index == dec_ctx->avc_ctx->lastminidx)
		{
			// This implies that the minimal index (assuming its number is
			// fairly constant) sets the temporal reference to zero - needed to set sync_pts.
			dec_ctx->timing->current_tref = 0;
		}
		if (dec_ctx->avc_ctx->lastmaxidx == -1)
		{
			// Set temporal reference to zero on minimal index and in the first GOP
			// to avoid setting a wrong fts_offset
			dec_ctx->timing->current_tref = 0;
		}
	}

	set_fts(dec_ctx->timing); // Keep frames_since_ref_time==0, use current_tref

	dbg_print(CCX_DMT_TIME, "  picordercnt:%3lld tref:%3d idx:%3d refidx:%3d lmaxidx:%3d maxtref:%3d\n",
		  pic_order_cnt_lsb, dec_ctx->timing->current_tref,
		  current_index, dec_ctx->avc_ctx->currref, dec_ctx->avc_ctx->lastmaxidx, dec_ctx->avc_ctx->maxtref);
	dbg_print(CCX_DMT_TIME, "  sync_pts:%s (%8u)",
		  print_mstime_static(dec_ctx->timing->sync_pts / (MPEG_CLOCK_FREQ / 1000)),
		  (unsigned)(dec_ctx->timing->sync_pts));
	dbg_print(CCX_DMT_TIME, " - %s since GOP: %2u",
		  slice_types[slice_type],
		  (unsigned)(dec_ctx->frames_since_last_gop));
	dbg_print(CCX_DMT_TIME, "  b:%lld  frame# %lld\n", bottom_field_flag, dec_ctx->avc_ctx->frame_num);

	// sync_pts is (was) set when current_tref was zero
	if (dec_ctx->avc_ctx->lastmaxidx > -1 && dec_ctx->timing->current_tref == 0)
	{
		if (ccx_options.debug_mask & CCX_DMT_TIME)
		{
			dbg_print(CCX_DMT_TIME, "\nNew temporal reference:\n");
			print_debug_timing(dec_ctx->timing);
		}
	}

	total_frames_count++;
	dec_ctx->frames_since_last_gop++;

	store_hdcc(enc_ctx, dec_ctx, dec_ctx->avc_ctx->cc_data, dec_ctx->avc_ctx->cc_count, current_index, dec_ctx->timing->fts_now, sub);
	dec_ctx->avc_ctx->cc_buffer_saved = CCX_TRUE; // CFS: store_hdcc supposedly saves the CC buffer to a sequence buffer
	dec_ctx->avc_ctx->cc_count = 0;
}

// max_dec_frame_buffering .. Max frames in buffer
