#ifndef _CCX_ENCODERS_HELPERS_H
#define _CCX_ENCODERS_HELPERS_H

#include "ccx_common_constants.h"
#include "ccx_decoders_structs.h"
#include "ccx_decoders_608.h"

// Helper functions
void correct_case(int line_num, struct eia608_screen *data);
void capitalize(int line_num, struct eia608_screen *data);
void find_limit_characters(unsigned char *line, int *first_non_blank, int *last_non_blank);
unsigned get_decoder_line_encoded_for_gui(unsigned char *buffer, int line_num, struct eia608_screen *data);
unsigned get_decoder_line_encoded(unsigned char *buffer, int line_num, struct eia608_screen *data);

#endif