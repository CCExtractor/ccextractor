/*
 * DVB subtitle decoding
 * Copyright (c) 2014 Anshul
 * License: LGPL
 *
 * This file is part of CCEXtractor
 * You should have received a copy of the GNU Lesser General Public
 * License along with CCExtractor; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */
/**
 * @file dvbsub.c
 */

#ifndef DVBSUBDEC_H
#define DVBSUBDEC_H

#define MAX_LANGUAGE_PER_DESC 5

#include "lib_ccx.h"
#ifdef __cplusplus
extern "C"
{
#endif

	struct dvb_config
	{
		unsigned char n_language;
		unsigned int lang_index[MAX_LANGUAGE_PER_DESC];
		/* subtitle type */
		unsigned char sub_type[MAX_LANGUAGE_PER_DESC];
		/* composition page id */
		unsigned short composition_id[MAX_LANGUAGE_PER_DESC];
		/* ancillary_page_id */
		unsigned short ancillary_id[MAX_LANGUAGE_PER_DESC];
	};

	/**
	 * @param cfg Structure containg configuration
	 *
	 * @return DVB context kept as void* for abstraction
	 *
	 */
	void *dvbsub_init_decoder(struct dvb_config *cfg);

	int dvbsub_close_decoder(void **dvb_ctx);

	/**
	 * @param dvb_ctx    PreInitialized DVB context using DVB
	 * @param buf        buffer containing segment data, first sync byte need to 0x0f.
	 *                   does not include data_identifier and subtitle_stream_id.
	 * @param buf_size   size of buf buffer
	 * @param sub        output subtitle data
	 *
	 * @return           -1 on error
	 */
	int dvbsub_decode(struct encoder_ctx *enc_ctx, struct lib_cc_decode *dec_ctx, const unsigned char *buf, int buf_size, struct cc_subtitle *sub);

	/**
	 * @func parse_dvb_description
	 *
	 * data pointer to this function should be after description length and description tag is parsed
	 *
	 * @param cfg preallocated dvb_config structure where parsed description will be stored,Not to be NULL
	 *
	 * @return return -1 if invalid data found other wise 0 if everything goes well
	 * errno is set is to EINVAL if invalid data is found
	 */
	int parse_dvb_description(struct dvb_config *cfg, unsigned char *data,
				  unsigned int len);

	/*
	 * @func dvbsub_get_context_size
	 * Get the size of DVBSubContext for external allocation
	 *
	 * @return size in bytes of DVBSubContext structure
	 */
	size_t dvbsub_get_context_size(void);

	/*
	 * @func dvbsub_copy_context
	 * Copy DVBSubContext from src to dst
	 *
	 * @param dst destination pointer (must be allocated with at least dvbsub_get_context_size() bytes)
	 * @param src source DVBSubContext pointer
	 */
	void dvbsub_copy_context(void *dst, void *src);

	/*
	 * @func dvbsub_set_write the output structure in dvb
	 * set ccx_s_write structure in dvb_ctx
	 *
	 * @param dvb_ctx context of dvb which was returned by dvbsub_init_decoder
	 *
	 * @param out output context returned by init_write
	 *
	 */
	void dvbsub_set_write(void *dvb_ctx, struct ccx_s_write *out);

#ifdef __cplusplus
}
#endif
#endif