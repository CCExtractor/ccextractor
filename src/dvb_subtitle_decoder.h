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

#include "ccextractor.h"
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
 * @param composition_id composition-page_id found in Subtitle descriptors
 *                       associated with 	subtitle stream in the	PMT
 *                       it could be -1 if not found in PMT.
 * @param ancillary_id ancillary-page_id found in Subtitle descriptors
 *                     associated with 	subtitle stream in the	PMT.
 *                       it could be -1 if not found in PMT.
 *
 * @return DVB context kept as void* for abstraction
 *
 */
void* dvbsub_init_decoder(int composition_id, int ancillary_id);

int dvbsub_close_decoder(void *dvb_ctx);

/**
 * @param dvb_ctx    PreInitialized DVB context using DVB
 * @param data       output subtitle data, to be implemented
 * @param data_size  Output subtitle data  size. pass the pointer to an intiger, NOT to be NULL.
 * @param buf        buffer containg segment data, first sync byte needto 0x0f.
 *                   does not include data_identifier and subtitle_stream_id.
 * @param buf_size   size of buf buffer
 *
 * @return           -1 on error
 */
int dvbsub_decode(void *dvb_ctx, void *data, int *data_size,
		const unsigned char *buf, int buf_size);
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
int parse_dvb_description(struct dvb_config* cfg, unsigned char*data,
		unsigned int len);

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
