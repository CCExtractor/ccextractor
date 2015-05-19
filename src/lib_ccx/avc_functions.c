#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "utility.h"
#include <math.h>

#define dvprint(...) dbg_print( CCX_DMT_VIDES, __VA_ARGS__)
// Functions to parse a AVC/H.264 data stream, see ISO/IEC 14496-10

int ccblocks_in_avc_total=0;
int ccblocks_in_avc_lost=0;

// local functions
static unsigned char *remove_03emu(unsigned char *from, unsigned char *to);
static void sei_rbsp (unsigned char *seibuf, unsigned char *seiend);
static unsigned char *sei_message (unsigned char *seibuf, unsigned char *seiend);
static void user_data_registered_itu_t_t35 (unsigned char *userbuf, unsigned char *userend);
static void seq_parameter_set_rbsp (unsigned char *seqbuf, unsigned char *seqend);
static void slice_header (struct lib_ccx_ctx *ctx, unsigned char *heabuf, unsigned char *heaend, int nal_unit_type, struct cc_subtitle *sub);

static unsigned char cc_count;
// buffer to hold cc data
static unsigned char *cc_data = NULL;
static long cc_databufsize = 1024;
int cc_buffer_saved=1; // Was the CC buffer saved after it was last updated?

static int got_seq_para=0;
static unsigned nal_ref_idc;
static LLONG seq_parameter_set_id;
static int log2_max_frame_num=0;
static int pic_order_cnt_type;
static int log2_max_pic_order_cnt_lsb=0;
static int frame_mbs_only_flag;

// Use and throw stats for debug, remove this uglyness soon
long num_nal_unit_type_7=0;
long num_vcl_hrd=0;
long num_nal_hrd=0;
long num_jump_in_frames=0;
long num_unexpected_sei_length=0;

double roundportable(double x) { return floor(x + 0.5); }

int ebsp_to_rbsp(char* rbsp, char* ebsp, int length);

void init_avc(void)
{
	cc_data = (unsigned char*)malloc(1024);
}

void do_NAL (struct lib_ccx_ctx *ctx, unsigned char *NALstart, LLONG NAL_length, struct cc_subtitle *sub)
{
	unsigned char *NALstop;
	unsigned nal_unit_type = *NALstart & 0x1F;

	NALstop = NAL_length+NALstart;
	NALstop = remove_03emu(NALstart+1, NALstop); // Add +1 to NALstop for TS, without it for MP4. Still don't know why

	if (NALstop==NULL) // remove_03emu failed.
	{
		mprint ("\rNotice: NAL of type %u had to be skipped because remove_03emu failed.\n", nal_unit_type);
		return;
	}

	if ( nal_unit_type == CCX_NAL_TYPE_ACCESS_UNIT_DELIMITER_9 )
	{
		// Found Access Unit Delimiter
	}
	else if ( nal_unit_type == CCX_NAL_TYPE_SEQUENCE_PARAMETER_SET_7 )
	{
		// Found sequence parameter set
		// We need this to parse NAL type 1 (CCX_NAL_TYPE_CODED_SLICE_NON_IDR_PICTURE_1)
		num_nal_unit_type_7++;
		seq_parameter_set_rbsp(NALstart+1, NALstop);
		got_seq_para = 1;
	}
	else if ( got_seq_para && (nal_unit_type == CCX_NAL_TYPE_CODED_SLICE_NON_IDR_PICTURE_1 ||
				nal_unit_type == CCX_NAL_TYPE_CODED_SLICE_IDR_PICTURE)) // Only if nal_unit_type=1
	{
		// Found coded slice of a non-IDR picture
		// We only need the slice header data, no need to implement
		// slice_layer_without_partitioning_rbsp( );
		slice_header(ctx, NALstart+1, NALstop, nal_unit_type, sub);
	}
	else if ( got_seq_para && nal_unit_type == CCX_NAL_TYPE_SEI )
	{
		// Found SEI (used for subtitles)
		//set_fts(); // FIXME - check this!!!
		sei_rbsp(NALstart+1, NALstop);
	}
	else if ( got_seq_para && nal_unit_type == CCX_NAL_TYPE_PICTURE_PARAMETER_SET )
	{
		// Found Picture parameter set
	}
	if (temp_debug)
	{
		mprint ("NAL process failed.\n");
		mprint ("\n After decoding, the actual thing was (length =%d)\n", NALstop-(NALstart+1));
		dump (CCX_DMT_GENERIC_NOTICES,NALstart+1, NALstop-(NALstart+1),0, 0);
	}

}

// Process inbuf bytes in buffer holding and AVC (H.264) video stream.
// The number of processed bytes is returned.
LLONG process_avc (struct lib_ccx_ctx *ctx, unsigned char *avcbuf, LLONG avcbuflen ,struct cc_subtitle *sub)
{
	unsigned char *bpos = avcbuf;
	unsigned char *NALstart;
	unsigned char *NALstop;

	// At least 5 bytes are needed for a NAL unit
	if(avcbuflen <= 5)
	{
		fatal(CCX_COMMON_EXIT_BUG_BUG,
				"NAL unit need at last 5 bytes ...");
	}

	// Warning there should be only leading zeros, nothing else
	if( !(bpos[0]==0x00 && bpos[1]==0x00) )
	{
		fatal(CCX_COMMON_EXIT_BUG_BUG,
				"Broken AVC stream - no 0x0000 ...");
	}
	bpos = bpos+2;

	int firstloop=1; // Check for valid start code at buffer start

	// Loop over NAL units
	while(bpos < avcbuf + avcbuflen - 2) // bpos points to 0x01 plus at least two bytes
	{
		int zeropad=0; // Count leading zeros

		// Find next NALstart
		while (bpos < avcbuf + avcbuflen)
		{
			if(*bpos == 0x01)
			{
				// OK, found a start code
				break;
			}
			else if(firstloop && *bpos != 0x00)
			{
				// Not 0x00 or 0x01
				fatal(CCX_COMMON_EXIT_BUG_BUG,
						"Broken AVC stream - no 0x00 ...");
			}
			bpos++;
			zeropad++;
		}
		firstloop=0;
		if (bpos >= avcbuf + avcbuflen)
		{
			// No new start sequence
			break;
		}
		NALstart = bpos+1;

		// Find next start code or buffer end
		LLONG restlen;
		do
		{
			// Search for next 000000 or 000001
			bpos++;
			restlen = avcbuf - bpos + avcbuflen - 2; // leave room for two more bytes

			// Find the next zero
			if (restlen > 0)
			{
				bpos = (unsigned char *) memchr (bpos, 0x00, (size_t) restlen);

				if(!bpos)
				{
					// No 0x00 till the end of the buffer
					NALstop = avcbuf + avcbuflen;
					bpos = NALstop;
					break;
				}

				if(bpos[1]==0x00 && (bpos[2]|0x01)==0x01)
				{
					// Found new start code
					NALstop = bpos;
					bpos = bpos + 2; // Move after the two leading 0x00
					break;
				}
			}
			else
			{
				NALstop = avcbuf + avcbuflen;
				bpos = NALstop;
				break;
			}
		} while(restlen); // Should never be true - loop is exited via break

		if(*NALstart & 0x80)
		{
			dump(CCX_DMT_GENERIC_NOTICES, NALstart-4,10, 0, 0);
			fatal(CCX_COMMON_EXIT_BUG_BUG,
					"Broken AVC stream - forbidden_zero_bit not zero ...");
		}

		nal_ref_idc = *NALstart >> 5;
		unsigned nal_unit_type = *NALstart & 0x1F;

		dvprint("BEGIN NAL unit type: %d length %d  zeros: %d  ref_idc: %d - Buffered captions before: %d\n",
				nal_unit_type,  NALstop-NALstart-1, zeropad, nal_ref_idc, !cc_buffer_saved);

		do_NAL (ctx, NALstart, NALstop-NALstart, sub);

		dvprint("END   NAL unit type: %d length %d  zeros: %d  ref_idc: %d - Buffered captions after: %d\n",
				nal_unit_type,  NALstop-NALstart-1, zeropad, nal_ref_idc, !cc_buffer_saved);
	}

	return avcbuflen;
}

#define ZEROBYTES_SHORTSTARTCODE 2

// Copied for reference decoder, see if it behaves different that Volker's code
int EBSPtoRBSP(unsigned char *streamBuffer, int end_bytepos, int begin_bytepos)
{
	int i, j, count;
	count = 0;

	if(end_bytepos < begin_bytepos)
		return end_bytepos;

	j = begin_bytepos;

	for(i = begin_bytepos; i < end_bytepos; ++i)
	{ //starting from begin_bytepos to avoid header information
		//in NAL unit, 0x000000, 0x000001 or 0x000002 shall not occur at any byte-aligned position
		if(count == ZEROBYTES_SHORTSTARTCODE && streamBuffer[i] < 0x03)
			return -1;
		if(count == ZEROBYTES_SHORTSTARTCODE && streamBuffer[i] == 0x03)
		{
			//check the 4th byte after 0x000003, except when cabac_zero_word is used, in which case the last three bytes of this NAL unit must be 0x000003
			if((i < end_bytepos-1) && (streamBuffer[i+1] > 0x03))
				return -1;
			//if cabac_zero_word is used, the final byte of this NAL unit(0x03) is discarded, and the last two bytes of RBSP must be 0x0000
			if(i == end_bytepos-1)
				return j;

			++i;
			count = 0;
		}
		streamBuffer[j] = streamBuffer[i];
		if(streamBuffer[i] == 0x00)
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
u32 avc_remove_emulation_bytes(const unsigned char *buffer_src, unsigned char *buffer_dst, u32 nal_size) ;

unsigned char *remove_03emu(unsigned char *from, unsigned char *to)
{
	int num=to-from;
	int newsize = EBSPtoRBSP (from,num,0); //TODO: Do something if newsize == -1 (broken NAL)
	if (newsize==-1)
		return NULL;
	return from+newsize;
}


// Process SEI payload in AVC data. This function combines sei_rbsp()
// and rbsp_trailing_bits().
void sei_rbsp (unsigned char *seibuf, unsigned char *seiend)
{
	unsigned char *tbuf = seibuf;
	while(tbuf < seiend - 1) // Use -1 because of trailing marker
	{
		tbuf = sei_message(tbuf, seiend - 1);
	}
	if(tbuf == seiend - 1 )
	{
		if(*tbuf != 0x80)
			mprint("Strange rbsp_trailing_bits value: %02X\n",*tbuf);
	}
	else
	{
		// TODO: This really really looks bad
		mprint ("WARNING: Unexpected SEI unit length...trying to continue.");
		temp_debug = 1;
		mprint ("\n Failed block (at sei_rbsp) was:\n");
		dump (CCX_DMT_GENERIC_NOTICES,(unsigned char *) seibuf, seiend-seibuf,0,0);

		num_unexpected_sei_length++;
	}
}


// This combines sei_message() and sei_payload().
unsigned char *sei_message (unsigned char *seibuf, unsigned char *seiend)
{
	int payloadType = 0;
	while (*seibuf==0xff)
	{
		payloadType+=255;
		seibuf++;
	}
	payloadType += *seibuf;
	seibuf++;


	int payloadSize = 0;
	while (*seibuf==0xff)
	{
		payloadSize+=255;
		seibuf++;
	}
	payloadSize += *seibuf;
	seibuf++;

	int broken=0;
	unsigned char *paystart = seibuf;
	seibuf+=payloadSize;

	dvprint("Payload type: %d size: %d - ", payloadType, payloadSize);
	if(seibuf > seiend )
	{
		// TODO: What do we do here?
		broken=1;
		if (payloadType==4)
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
	if(!broken && payloadType == 4)
		user_data_registered_itu_t_t35(paystart, paystart+payloadSize);

	return seibuf;
}

void copy_ccdata_to_buffer (char *source, int new_cc_count)
{
	ccblocks_in_avc_total++;
	if (cc_buffer_saved==0)
	{
		mprint ("Warning: Probably loss of CC data, unsaved buffer being rewritten\n");
		ccblocks_in_avc_lost++;
	}
	memcpy(cc_data+cc_count*3, source, new_cc_count*3+1);
	cc_count+=new_cc_count;
	cc_buffer_saved=0;
}


void user_data_registered_itu_t_t35 (unsigned char *userbuf, unsigned char *userend)
{
	unsigned char *tbuf = userbuf;
	unsigned char *cc_tmpdata;
	unsigned char process_cc_data_flag;
	int user_data_type_code;
	int user_data_len;
	int local_cc_count=0;
	// int cc_count;
	int itu_t_t35_country_code = *((uint8_t*)tbuf);
	tbuf++;
	int itu_t_35_provider_code = *tbuf * 256 + *(tbuf+1);
	tbuf+=2;

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

	if(itu_t_t35_country_code != 0xB5)
	{
		mprint("Not a supported user data SEI\n");
		mprint("  itu_t_35_country_code: %02x\n", itu_t_t35_country_code);
		return;
	}

	switch (itu_t_35_provider_code)
	{
		case 0x0031: // ANSI/SCTE 128
			dbg_print(CCX_DMT_VERBOSE, "Caption block in ANSI/SCTE 128...");
			if (*tbuf==0x47 && *(tbuf+1)==0x41 && *(tbuf+2)==0x39 && *(tbuf+3)==0x34) // ATSC1_data() - GA94
			{
				dbg_print(CCX_DMT_VERBOSE, "ATSC1_data()...\n");
				tbuf+=4;
				unsigned char user_data_type_code=*tbuf;
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
						local_cc_count= *tbuf & 0x1F;
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
						cc_tmpdata = tbuf;

						/* TODO: I don't think we have user_data_len here
						   if (cc_count*3+3 != user_data_len)
						   fatal(CCX_COMMON_EXIT_BUG_BUG,
						   "Syntax problem: user_data_len != cc_count*3+3."); */

						// Enough room for CC captions?
						if (cc_tmpdata+local_cc_count*3 >= userend)
							fatal(CCX_COMMON_EXIT_BUG_BUG,
									"Syntax problem: Too many caption blocks.");
						if (cc_tmpdata[local_cc_count*3]!=0xFF)
							fatal(CCX_COMMON_EXIT_BUG_BUG,
									"Syntax problem: Final 0xFF marker missing.");

						// Save the data and process once we know the sequence number
						if (local_cc_count*3+1 > cc_databufsize)
						{
							cc_data = (unsigned char*)realloc(cc_data, (size_t) cc_count*6+1);
							if (!cc_data)
								fatal(EXIT_NOT_ENOUGH_MEMORY, "Out of memory");
							cc_databufsize = (long) cc_count*6+1;
						}
						// Copy new cc data into cc_data
						copy_ccdata_to_buffer ((char *) cc_tmpdata, local_cc_count);
						break;
					case 0x06:
						dbg_print(CCX_DMT_VERBOSE, "bar_data (unsupported for now)\n");
						break;
					default:
						dbg_print(CCX_DMT_VERBOSE, "SCTE/ATSC reserved.\n");
				}

			}
			else if (*tbuf==0x44 && *(tbuf+1)==0x54 && *(tbuf+2)==0x47 && *(tbuf+3)==0x31) // afd_data() - DTG1
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

			user_data_type_code = *((uint8_t*)tbuf);
			if(user_data_type_code != 0x03)
			{
				dbg_print(CCX_DMT_VERBOSE, "Not supported  user_data_type_code: %02x\n",
						user_data_type_code);
				return;
			}
			tbuf++;
			user_data_len = *((uint8_t*)tbuf);
			tbuf++;

			local_cc_count = *tbuf & 0x1F;
			process_cc_data_flag = (*tbuf & 0x40) >> 6;
			if (!process_cc_data_flag)
			{
				mprint ("process_cc_data_flag == 0, skipping this caption block.\n");
				break;
			}
			cc_tmpdata = tbuf+2;

			if (local_cc_count*3+3 != user_data_len)
				fatal(CCX_COMMON_EXIT_BUG_BUG,
						"Syntax problem: user_data_len != cc_count*3+3.");

			// Enough room for CC captions?
			if (cc_tmpdata+local_cc_count*3 >= userend)
				fatal(CCX_COMMON_EXIT_BUG_BUG,
						"Syntax problem: Too many caption blocks.");
			if (cc_tmpdata[local_cc_count*3]!=0xFF)
				fatal(CCX_COMMON_EXIT_BUG_BUG,
						"Syntax problem: Final 0xFF marker missing.");

			// Save the data and process once we know the sequence number
			if (cc_count*3+1 > cc_databufsize)
			{
				cc_data = (unsigned char*)realloc(cc_data, (size_t) cc_count*6+1);
				if (!cc_data)
					fatal(EXIT_NOT_ENOUGH_MEMORY, "Out of memory");
				cc_databufsize = (long) cc_count*6+1;
			}
			// Copy new cc data into cc_data - replace command below.
			copy_ccdata_to_buffer ((char *) cc_tmpdata, local_cc_count);

			//dump(tbuf,user_data_len-1,0);
			break;
		default:
			mprint("Not a supported user data SEI\n");
			mprint("  itu_t_35_provider_code: %04x\n", itu_t_35_provider_code);
			break;
	}
}


// Process sequence parameters in AVC data.
void seq_parameter_set_rbsp (unsigned char *seqbuf, unsigned char *seqend)
{
	LLONG tmp, tmp1;
	struct bitstream q1;
	init_bitstream(&q1, seqbuf, seqend);

	dvprint("SEQUENCE PARAMETER SET (bitlen: %lld)\n", q1.bitsleft);
	tmp=u(&q1,8);
	LLONG profile_idc = tmp;
	dvprint("profile_idc=                                   % 4lld (%#llX)\n",tmp,tmp);
	tmp=u(&q1,1);
	dvprint("constraint_set0_flag=                          % 4lld (%#llX)\n",tmp,tmp);
	tmp=u(&q1,1);
	dvprint("constraint_set1_flag=                          % 4lld (%#llX)\n",tmp,tmp);
	tmp=u(&q1,1);
	dvprint("constraint_set2_flag=                          % 4lld (%#llX)\n",tmp,tmp);
	tmp = u(&q1, 1);
	dvprint("constraint_set3_flag=                          % 4lld (%#llX)\n",tmp,tmp);
	tmp = u(&q1, 1);
	dvprint("constraint_set4_flag=                          % 4lld (%#llX)\n",tmp,tmp);
	tmp = u(&q1, 1);
	dvprint("constraint_set5_flag=                          % 4lld (%#llX)\n",tmp,tmp);
	tmp=u(&q1,2);
	dvprint("reserved=                                      % 4lld (%#llX)\n",tmp,tmp);
	tmp=u(&q1,8);
	dvprint("level_idc=                                     % 4lld (%#llX)\n",tmp,tmp);
	seq_parameter_set_id = ue(&q1);
	dvprint("seq_parameter_set_id=                          % 4lld (%#llX)\n", seq_parameter_set_id,seq_parameter_set_id);
	if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122
			|| profile_idc == 244 || profile_idc == 44 || profile_idc == 83
			|| profile_idc == 86 || profile_idc == 118 || profile_idc == 128){
		LLONG chroma_format_idc = ue(&q1);
		dvprint("chroma_format_idc=                             % 4lld (%#llX)\n", chroma_format_idc,chroma_format_idc);
		if (chroma_format_idc == 3){
			tmp = u(&q1, 1);
			dvprint("separate_colour_plane_flag=                    % 4lld (%#llX)\n", tmp, tmp);
		}
		tmp = ue(&q1);
		dvprint("bit_depth_luma_minus8=                         % 4lld (%#llX)\n", tmp, tmp);
		tmp = ue(&q1);
		dvprint("bit_depth_chroma_minus8=                       % 4lld (%#llX)\n", tmp, tmp);
		tmp = u(&q1,1);
		dvprint("qpprime_y_zero_transform_bypass_flag=          % 4lld (%#llX)\n", tmp, tmp);
		tmp = u(&q1, 1);
		dvprint("seq_scaling_matrix_present_flag=               % 4lld (%#llX)\n", tmp, tmp);
		if (tmp == 1)
		{
			// WVI: untested, just copied from specs.
			for (int i = 0; i < ((chroma_format_idc != 3) ? 8 : 12); i++)
			{
				tmp = u(&q1, 1);
				dvprint("seq_scaling_list_present_flag[%d]=                 % 4lld (%#llX)\n",i,tmp, tmp);
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
								int64_t delta_scale = se(&q1);
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
								int64_t delta_scale = se(&q1);
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
	log2_max_frame_num = (int)ue(&q1);
	dvprint("log2_max_frame_num4_minus4=                    % 4d (%#X)\n", log2_max_frame_num,log2_max_frame_num);
	log2_max_frame_num += 4; // 4 is added due to the formula.
	pic_order_cnt_type = (int)ue(&q1);
	dvprint("pic_order_cnt_type=                            % 4d (%#X)\n", pic_order_cnt_type,pic_order_cnt_type);
	if( pic_order_cnt_type == 0 )
	{
		log2_max_pic_order_cnt_lsb = (int)ue(&q1);
		dvprint("log2_max_pic_order_cnt_lsb_minus4=             % 4d (%#X)\n", log2_max_pic_order_cnt_lsb,log2_max_pic_order_cnt_lsb);
		log2_max_pic_order_cnt_lsb += 4; // 4 is added due to formula.
	}
	else if( pic_order_cnt_type == 1 )
	{
		// CFS: Untested, just copied from specs.
		tmp= u(&q1,1);
		dvprint("delta_pic_order_always_zero_flag=              % 4lld (%#llX)\n",tmp,tmp);
		tmp = se(&q1);
		dvprint("offset_for_non_ref_pic=                        % 4lld (%#llX)\n",tmp,tmp);
		tmp = se(&q1);
		dvprint("offset_for_top_to_bottom_field                 % 4lld (%#llX)\n",tmp,tmp);
		LLONG num_ref_frame_in_pic_order_cnt_cycle = ue (&q1);
		dvprint("num_ref_frame_in_pic_order_cnt_cycle           % 4lld (%#llX)\n", num_ref_frame_in_pic_order_cnt_cycle,num_ref_frame_in_pic_order_cnt_cycle);
		for (int i=0; i<num_ref_frame_in_pic_order_cnt_cycle; i++)
		{
			tmp=se(&q1);
			dvprint("offset_for_ref_frame [%d / %d] =               % 4lld (%#llX)\n", i, num_ref_frame_in_pic_order_cnt_cycle, tmp,tmp);
		}
	}
	else
	{
		// Nothing needs to be parsed when pic_order_cnt_type == 2
	}

	tmp=ue(&q1);
	dvprint("max_num_ref_frames=                            % 4lld (%#llX)\n",tmp,tmp);
	tmp=u(&q1,1);
	dvprint("gaps_in_frame_num_value_allowed_flag=          % 4lld (%#llX)\n",tmp,tmp);
	tmp=ue(&q1);
	dvprint("pic_width_in_mbs_minus1=                       % 4lld (%#llX)\n",tmp,tmp);
	tmp=ue(&q1);
	dvprint("pic_height_in_map_units_minus1=                % 4lld (%#llX)\n",tmp,tmp);
	frame_mbs_only_flag = (int)u(&q1,1);
	dvprint("frame_mbs_only_flag=                           % 4d (%#X)\n", frame_mbs_only_flag,frame_mbs_only_flag);
	if ( !frame_mbs_only_flag )
	{
		tmp=u(&q1,1);
		dvprint("mb_adaptive_fr_fi_flag=                        % 4lld (%#llX)\n",tmp,tmp);
	}
	tmp=u(&q1,1);
	dvprint("direct_8x8_inference_f=                        % 4lld (%#llX)\n",tmp,tmp);
	tmp=u(&q1,1);
	dvprint("frame_cropping_flag=                           % 4lld (%#llX)\n",tmp,tmp);
	if ( tmp )
	{
		tmp=ue(&q1);
		dvprint("frame_crop_left_offset=                        % 4lld (%#llX)\n",tmp,tmp);
		tmp=ue(&q1);
		dvprint("frame_crop_right_offset=                       % 4lld (%#llX)\n",tmp,tmp);
		tmp=ue(&q1);
		dvprint("frame_crop_top_offset=                         % 4lld (%#llX)\n",tmp,tmp);
		tmp=ue(&q1);
		dvprint("frame_crop_bottom_offset=                      % 4lld (%#llX)\n",tmp,tmp);
	}
	tmp=u(&q1,1);
	dvprint("vui_parameters_present=                        % 4lld (%#llX)\n",tmp,tmp);
	if ( tmp )
	{
		dvprint("\nVUI parameters\n");
		tmp=u(&q1,1);
		dvprint("aspect_ratio_info_pres=                        % 4lld (%#llX)\n",tmp,tmp);
		if ( tmp )
		{
			tmp=u(&q1,8);
			dvprint("aspect_ratio_idc=                              % 4lld (%#llX)\n",tmp,tmp);
			if ( tmp == 255 )
			{
				tmp=u(&q1,16);
				dvprint("sar_width=                                     % 4lld (%#llX)\n",tmp,tmp);
				tmp=u(&q1,16);
				dvprint("sar_height=                                    % 4lld (%#llX)\n",tmp,tmp);
			}
		}
		tmp = u(&q1,1);
		dvprint("overscan_info_pres_flag=                       % 4lld (%#llX)\n",tmp,tmp);
		if ( tmp )
		{
			tmp=u(&q1,1);
			dvprint("overscan_appropriate_flag=                     % 4lld (%#llX)\n",tmp,tmp);
		}
		tmp = u(&q1,1);
		dvprint("video_signal_type_present_flag=                % 4lld (%#llX)\n",tmp,tmp);
		if ( tmp )
		{
			tmp=u(&q1,3);
			dvprint("video_format=                                  % 4lld (%#llX)\n",tmp,tmp);
			tmp=u(&q1,1);
			dvprint("video_full_range_flag=                         % 4lld (%#llX)\n",tmp,tmp);
			tmp=u(&q1,1);
			dvprint("colour_description_present_flag=               % 4lld (%#llX)\n",tmp,tmp);
			if ( tmp )
			{
				tmp=u(&q1,8);
				dvprint("colour_primaries=                              % 4lld (%#llX)\n",tmp,tmp);
				tmp=u(&q1,8);
				dvprint("transfer_characteristics=                      % 4lld (%#llX)\n",tmp,tmp);
				tmp=u(&q1,8);
				dvprint("matrix_coefficients=                           % 4lld (%#llX)\n",tmp,tmp);
			}
		}
		tmp = u(&q1,1);
		dvprint("chroma_loc_info_present_flag=                  % 4lld (%#llX)\n",tmp,tmp);
		if ( tmp )
		{
			tmp=ue(&q1);
			dvprint("chroma_sample_loc_type_top_field=                  % 4lld (%#llX)\n",tmp,tmp);
			tmp=ue(&q1);
			dvprint("chroma_sample_loc_type_bottom_field=               % 4lld (%#llX)\n",tmp,tmp);
		}
		tmp = u(&q1,1);
		dvprint("timing_info_present_flag=                      % 4lld (%#llX)\n",tmp,tmp);
		if ( tmp )
		{
			tmp=u(&q1,32);
			LLONG num_units_in_tick = tmp;
			dvprint("num_units_in_tick=                             % 4lld (%#llX)\n",tmp,tmp);
			tmp=u(&q1,32);
			LLONG time_scale = tmp;
			dvprint("time_scale=                                    % 4lld (%#llX)\n",tmp,tmp);
			tmp=u(&q1,1);
			int fixed_frame_rate_flag = (int) tmp;
			dvprint("fixed_frame_rate_flag=                         % 4lld (%#llX)\n",tmp,tmp);
			// Change: use num_units_in_tick and time_scale to calculate FPS. (ISO/IEC 14496-10:2012(E), page 397 & further)
			if (fixed_frame_rate_flag){
				double clock_tick = (double) num_units_in_tick / time_scale;
				dvprint("clock_tick= %f\n", clock_tick);
				current_fps = (double)time_scale / (2 * num_units_in_tick); // Based on formula D-2, p. 359 of the ISO/IEC 14496-10:2012(E) spec.
				mprint("Changed fps using NAL to: %f\n", current_fps);
			}
		}
		tmp = u(&q1,1);
		dvprint("nal_hrd_parameters_present_flag=               % 4lld (%#llX)\n",tmp,tmp);
		if ( tmp )
		{
			dvprint ("nal_hrd. Not implemented for now. Hopefully not needed. Skiping rest of NAL\n");
			//printf("Boom nal_hrd\n");
			// exit(1);
			num_nal_hrd++;
			return;
		}
		tmp1 = u(&q1,1);
		dvprint("vcl_hrd_parameters_present_flag=               %llX\n", tmp1);
		if ( tmp )
		{
			// TODO.
			mprint ("vcl_hrd. Not implemented for now. Hopefully not needed. Skiping rest of NAL\n");
			num_vcl_hrd++;
			// exit(1);
		}
		if ( tmp || tmp1 )
		{
			tmp = u(&q1,1);
			dvprint("low_delay_hrd_flag=                                % 4lld (%#llX)\n",tmp,tmp);
			return;
		}
		tmp = u(&q1,1);
		dvprint("pic_struct_present_flag=                       % 4lld (%#llX)\n",tmp,tmp);
		tmp = u(&q1,1);
		dvprint("bitstream_restriction_flag=                    % 4lld (%#llX)\n",tmp,tmp);
		// ..
		// The hope was to find the GOP length in max_dec_frame_buffering, but
		// it was not set in the testfile.  Ignore the rest here, it's
		// currently not needed.
	}
	//exit(1);
}


// Process slice header in AVC data.
void slice_header (struct lib_ccx_ctx *ctx, unsigned char *heabuf, unsigned char *heaend, int nal_unit_type, struct cc_subtitle *sub)
{
	LLONG tmp;
	struct bitstream q1;
	if (init_bitstream(&q1, heabuf, heaend))
	{
		mprint ("Skipping slice header due to failure in init_bitstream.\n");
		return;
	}

	LLONG slice_type, bottom_field_flag=0, pic_order_cnt_lsb=-1;
	static LLONG frame_num=-1, lastframe_num = -1;
	static int currref=0, maxidx=-1, lastmaxidx=-1;
	// Used to find tref zero in PTS mode
	static int minidx=10000, lastminidx=10000;
	int curridx;
	int IdrPicFlag = ((nal_unit_type == 5 )?1:0);

	// Used to remember the max temporal reference number (poc mode)
	static int maxtref = 0;
	static int last_gop_maxtref = 0;

	// Used for PTS ordering of CC blocks
	static LLONG currefpts = 0;

	dvprint("\nSLICE HEADER\n");
	tmp = ue(&q1);
	dvprint("first_mb_in_slice=     % 4lld (%#llX)\n",tmp,tmp);
	slice_type = ue(&q1);
	dvprint("slice_type=            %llX\n", slice_type);
	tmp = ue(&q1);
	dvprint("pic_parameter_set_id=  % 4lld (%#llX)\n",tmp,tmp);

	lastframe_num = frame_num;
	int maxframe_num = (int) ((1<<log2_max_frame_num) - 1);

	// Needs log2_max_frame_num_minus4 + 4 bits
	frame_num = u(&q1,log2_max_frame_num);
	dvprint("frame_num=             %llX\n", frame_num);

	LLONG field_pic_flag = 0; // Moved here because it's needed for pic_order_cnt_type==2
	if( !frame_mbs_only_flag )
	{
		field_pic_flag = u(&q1,1);
		dvprint("field_pic_flag=        %llX\n", field_pic_flag);
		if( field_pic_flag )
		{
			// bottom_field_flag
			bottom_field_flag = u(&q1,1);
			dvprint("bottom_field_flag=     %llX\n", bottom_field_flag);

			// TODO - Do this right.
			// When bottom_field_flag is set the video is interlaced,
			// override current_fps.
			current_fps = framerates_values[7];
		}
	}

	dvprint("IdrPicFlag=            %d\n", IdrPicFlag	);

	if( nal_unit_type == 5 )
	{
		tmp=ue(&q1);
		dvprint("idr_pic_id=     % 4lld (%#llX)\n",tmp,tmp);
		//TODO
	}
	if( pic_order_cnt_type == 0 )
	{
		pic_order_cnt_lsb=u(&q1,log2_max_pic_order_cnt_lsb);
		dvprint("pic_order_cnt_lsb=     %llX\n", pic_order_cnt_lsb);
	}
	if( pic_order_cnt_type == 1 )
	{
		fatal(CCX_COMMON_EXIT_BUG_BUG, "AVC: pic_order_cnt_type == 1 not yet supported.");
	}
#if 0
	else
	{
		/* CFS: Warning!!: Untested stuff, copied from specs (8.2.1.3) */
		LLONG FrameNumOffset = 0;
		if (IdrPicFlag == 1)
			FrameNumOffset=0;
		else if (lastframe_num > frame_num)
			FrameNumOffset = lastframe_num + maxframe_num;
		else
			FrameNumOffset = lastframe_num;

		LLONG tempPicOrderCnt=0;
		if (IdrPicFlag == 1)
			tempPicOrderCnt=0;
		else if (nal_ref_idc == 0)
			tempPicOrderCnt = 2*(FrameNumOffset + frame_num) -1 ;
		else
			tempPicOrderCnt = 2*(FrameNumOffset + frame_num);

		LLONG TopFieldOrderCnt = tempPicOrderCnt;
		LLONG BottomFieldOrderCnt = tempPicOrderCnt;

		if (!field_pic_flag) {
			TopFieldOrderCnt = tempPicOrderCnt;
			BottomFieldOrderCnt = tempPicOrderCnt;
		} else if (bottom_field_flag)
			BottomFieldOrderCnt = tempPicOrderCnt;
		else
			TopFieldOrderCnt = tempPicOrderCnt;

		//pic_order_cnt_lsb=tempPicOrderCnt;
		//pic_order_cnt_lsb=u(&q1,tempPicOrderCnt);
		//fatal(CCX_COMMON_EXIT_BUG_BUG, "AVC: pic_order_cnt_type != 0 not yet supported.");
		//TODO
		// Calculate picture order count (POC) according to 8.2.1
	}
#endif
	// The rest of the data in slice_header() is currently unused.

	// A reference pic (I or P is always the last displayed picture of a POC
	// sequence. B slices can be reference pics, so ignore nal_ref_idc.
	int isref = 0;
	switch (slice_type)
	{
		// P-SLICES
		case 0:
		case 5:
			// I-SLICES
		case 2:
		case 7:
			isref=1;
			break;
	}

	int maxrefcnt = (int) ((1<<log2_max_pic_order_cnt_lsb) - 1);

	// If we saw a jump set maxidx, lastmaxidx to -1
	LLONG dif = frame_num - lastframe_num;
	if (dif == -maxframe_num)
		dif = 0;
	if ( lastframe_num > -1 && (dif < 0 || dif > 1) )
	{
		num_jump_in_frames++;
		dvprint("\nJump in frame numbers (%lld/%lld)\n", frame_num, lastframe_num);
		// This will prohibit setting current_tref on potential
		// jumps.
		maxidx = -1;
		lastmaxidx = -1;
	}

	// Sometimes two P-slices follow each other, see garbled_dishHD.mpg,
	// in this case we only treat the first as a reference pic
	if (isref && ctx->frames_since_last_gop <= 3) // Used to be == 1, but the sample file
	{ // 2014 SugarHouse Casino Mummers Parade Fancy Brigades_new.ts was garbled
		// Probably doing a proper PTS sort would be a better solution.
		isref = 0;
		dbg_print(CCX_DMT_TIME, "Ignoring this reference pic.\n");
	}

	// if slices are buffered - flush
	if (isref && !bottom_field_flag)
	{
		dvprint("\nReference pic! [%s]\n", slice_types[slice_type]);
		dbg_print(CCX_DMT_TIME, "\nReference pic! [%s] maxrefcnt: %3d\n",
				slice_types[slice_type], maxrefcnt);

		// Flush buffered cc blocks before doing the housekeeping
		if (has_ccdata_buffered)
		{
			process_hdcc(ctx, sub);
		}
		ctx->last_gop_length = ctx->frames_since_last_gop;
		ctx->frames_since_last_gop = 0;
		last_gop_maxtref = maxtref;
		maxtref = 0;
		lastmaxidx = maxidx;
		maxidx = 0;
		lastminidx = minidx;
		minidx = 10000;

		if ( ccx_options.usepicorder ) {
			// Use pic_order_cnt_lsb

			// Make sure that curridx never wraps for curidx values that
			// are smaller than currref
			currref = (int)pic_order_cnt_lsb;
			if (currref < maxrefcnt/3)
			{
				currref += maxrefcnt+1;
			}

			// If we wrapped arround lastmaxidx might be larger than
			// the current index - fix this.
			if (lastmaxidx > currref + maxrefcnt/2) // implies lastmaxidx > 0
				lastmaxidx -=maxrefcnt+1;
		} else {
			// Use PTS ordering
			currefpts = current_pts;
			currref = 0;
		}

		anchor_hdcc( currref );
	}

	if ( ccx_options.usepicorder ) {
		// Use pic_order_cnt_lsb
		// Wrap (add max index value) curridx if needed.
		if( currref - pic_order_cnt_lsb > maxrefcnt/2 )
			curridx = (int)pic_order_cnt_lsb + maxrefcnt+1;
		else
			curridx = (int)pic_order_cnt_lsb;

		// Track maximum index for this GOP
		if ( curridx > maxidx )
			maxidx = curridx;

		// Calculate tref
		if ( lastmaxidx > 0 ) {
			current_tref = curridx - lastmaxidx -1;
			// Set maxtref
			if( current_tref > maxtref ) {
				maxtref = current_tref;
			}
			// Now an ugly workaround where pic_order_cnt_lsb increases in
			// steps of two. The 1.5 is an approximation, it should be:
			// last_gop_maxtref+1 == last_gop_length*2
			if ( last_gop_maxtref > ctx->last_gop_length*1.5 ) {
				current_tref = current_tref/2;
			}
		}
		else
			current_tref = 0;

		if ( current_tref < 0 ) {
			mprint("current_tref is negative!?\n");
		}
	} else {
		// Use PTS ordering - calculate index position from PTS difference and
		// frame rate
		// The 2* accounts for a discrepancy between current and actual FPS
		// seen in some files (CCSample2.mpg)
		curridx = (int)roundportable(2*(current_pts - currefpts)/(MPEG_CLOCK_FREQ/current_fps));

		if (abs(curridx) >= MAXBFRAMES) {
			// Probably a jump in the timeline. Warn and handle gracefully.
			mprint("\nFound large gap in PTS! Trying to recover ...\n");
			curridx = 0;
		}

		// Track maximum index for this GOP
		if ( curridx > maxidx )
			maxidx = curridx;

		// Track minimum index for this GOP
		if ( curridx < minidx )
			minidx = curridx;

		current_tref = 1;
		if ( curridx == lastminidx ) {
			// This implies that the minimal index (assuming its number is
			// fairly constant) sets the temporal reference to zero - needed to set sync_pts.
			current_tref = 0;
		}
		if ( lastmaxidx == -1) {
			// Set temporal reference to zero on minimal index and in the first GOP
			// to avoid setting a wrong fts_offset
			current_tref = 0;
		}
	}

	set_fts(); // Keep frames_since_ref_time==0, use current_tref

	dbg_print(CCX_DMT_TIME, "PTS: %s (%8u)",
			print_mstime(current_pts/(MPEG_CLOCK_FREQ/1000)),
			(unsigned) (current_pts));
	dbg_print(CCX_DMT_TIME, "  picordercnt:%3lld tref:%3d idx:%3d refidx:%3d lmaxidx:%3d maxtref:%3d\n",
			pic_order_cnt_lsb, current_tref,
			curridx, currref, lastmaxidx, maxtref);
	dbg_print(CCX_DMT_TIME, "FTS: %s",
			print_mstime(get_fts()));
	dbg_print(CCX_DMT_TIME, "  sync_pts:%s (%8u)",
			print_mstime(sync_pts/(MPEG_CLOCK_FREQ/1000)),
			(unsigned) (sync_pts));
	dbg_print(CCX_DMT_TIME, " - %s since GOP: %2u",
			slice_types[slice_type],
			(unsigned) (ctx->frames_since_last_gop));
	dbg_print(CCX_DMT_TIME, "  b:%lld  frame# %lld\n", bottom_field_flag, frame_num);

	// sync_pts is (was) set when current_tref was zero
	if ( lastmaxidx > -1 && current_tref == 0 )
	{
		if (ccx_options.debug_mask & CCX_DMT_TIME )
		{
			dbg_print(CCX_DMT_TIME, "\nNew temporal reference:\n");
			print_debug_timing();
		}
	}

	total_frames_count++;
	ctx->frames_since_last_gop++;

	store_hdcc(ctx, cc_data, cc_count, curridx, fts_now, sub);
	cc_buffer_saved=1; // CFS: store_hdcc supposedly saves the CC buffer to a sequence buffer
	cc_count=0;
}

// max_dec_frame_buffering .. Max frames in buffer
