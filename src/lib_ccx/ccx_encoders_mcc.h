#ifndef _CCEXTRACTOR_CCX_ENCODERS_MCC_H
#define _CCEXTRACTOR_CCX_ENCODERS_MCC_H

#include "ccx_common_platform.h"
#include "ccx_common_common.h"
#include "ccx_encoders_common.h"

/*----------------------------------------------------------------------------*/
/*--                               Constants                                --*/
/*----------------------------------------------------------------------------*/

#define ANC_DID_CLOSED_CAPTIONING        0x61
#define ANC_SDID_CEA_708                 0x01
#define CDP_IDENTIFIER_VALUE_HIGH        0x96
#define CDP_IDENTIFIER_VALUE_LOW         0x69
#define CC_DATA_ID                       0x72
#define CDP_FOOTER_ID                    0x74

#define CDP_FRAME_RATE_FORBIDDEN         0x00
#define CDP_FRAME_RATE_23_976            0x01
#define CDP_FRAME_RATE_24                0x02
#define CDP_FRAME_RATE_25                0x03
#define CDP_FRAME_RATE_29_97             0x04
#define CDP_FRAME_RATE_30                0x05
#define CDP_FRAME_RATE_50                0x06
#define CDP_FRAME_RATE_59_94             0x07
#define CDP_FRAME_RATE_60                0x08

/*----------------------------------------------------------------------------*/
/*--                                Types                                   --*/
/*----------------------------------------------------------------------------*/

typedef unsigned char boolean;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;
typedef unsigned long long uint64;
typedef long long int64;

/*----------------------------------------------------------------------------*/
/*--                              Structures                                --*/
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*--                                Macros                                  --*/
/*----------------------------------------------------------------------------*/

#define LOG(...) debug_log(__FILE__, __LINE__, __VA_ARGS__)
#define ASSERT(x) if(!(x)) debug_log(__FILE__, __LINE__, "ASSERT FAILED!")

/*----------------------------------------------------------------------------*/
/*--                          Exposed Variables                             --*/
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*--                          Exposed Functions                             --*/
/*----------------------------------------------------------------------------*/

boolean mcc_encode_cc_data(struct encoder_ctx *ctx, struct lib_cc_decode *dec_ctx, unsigned char *cc_data, int cc_count);

#endif // _CCEXTRACTOR_CCX_ENCODERS_MCC_H
