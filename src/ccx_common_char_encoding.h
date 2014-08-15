#ifndef __CCX_COMMON_CHAR_ENCODING__
#define __CCX_COMMON_CHAR_ENCODING__

void get_char_in_latin_1(unsigned char *buffer, unsigned char c);
void get_char_in_unicode(unsigned char *buffer, unsigned char c);
int get_char_in_utf_8(unsigned char *buffer, unsigned char c);
unsigned char cctolower(unsigned char c);
unsigned char cctoupper(unsigned char c);

#endif