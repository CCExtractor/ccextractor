#ifndef _CCX_ENCODERS_HELPERS_H
#define _CCX_ENCODERS_HELPERS_H

#include "ccx_common_common.h"
#include "ccx_decoders_structs.h"
#include "ccx_decoders_608.h"

extern char **spell_lower;
extern char **spell_correct;
extern int spell_words;
extern int spell_capacity;

extern unsigned char usercolor_rgb[8];

struct ccx_encoders_helpers_settings_t {
	int trim_subs;
	int no_font_color;
	int no_type_setting;
	enum ccx_encoding_type encoding;
} ccx_encoders_helpers_settings;

// Helper functions
void correct_case(int line_num, struct eia608_screen *data);
void capitalize(int line_num, struct eia608_screen *data);
void find_limit_characters(unsigned char *line, int *first_non_blank, int *last_non_blank);
unsigned get_decoder_line_encoded_for_gui(unsigned char *buffer, int line_num, struct eia608_screen *data);
unsigned get_decoder_line_encoded(unsigned char *buffer, int line_num, struct eia608_screen *data);

int string_cmp(const void *p1, const void *p2);
int string_cmp2(const void *p1, const void *p2, void *arg);
int add_built_in_words();
int add_word(const char *word);

void shell_sort(void *base, int nb, size_t size, int(*compar)(const void*p1, const void *p2, void*arg), void *arg);

void ccx_encoders_helpers_perform_shellsort_words();
void ccx_encoders_helpers_setup(enum ccx_encoding_type encoding, int no_font_color, int no_type_setting, int trim_subs);
#endif