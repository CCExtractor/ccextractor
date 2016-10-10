#ifndef DVDSUBDEC_H
#define DVDSUBDEC_H

#include "ccx_decoders_structs.h"

/**
 * @return DVD context 
 */
void *init_dvdsub_decode();

/**
 * @param buffer	buffer containing the spu packet data
 * @param length	Length of the data buffer received
 * @return			-1 on error
 */
int process_spu(struct lib_cc_decode *dec_ctx, unsigned char *buffer, int length, struct cc_subtitle *sub);
#endif