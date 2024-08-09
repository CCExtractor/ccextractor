#ifndef _CCX_DECODERS_708_ENCODING_H_
#define _CCX_DECODERS_708_ENCODING_H_

#define CCX_DTVCC_MUSICAL_NOTE_CHAR 9836 // Unicode Character 'BEAMED SIXTEENTH NOTES'

#ifndef DISABLE_RUST
extern unsigned char dtvcc_get_internal_from_G0(unsigned char g0_char);
extern unsigned char dtvcc_get_internal_from_G1(unsigned char g1_char);
extern unsigned char dtvcc_get_internal_from_G2(unsigned char g2_char);
extern unsigned char dtvcc_get_internal_from_G3(unsigned char g3_char);
#else
unsigned char dtvcc_get_internal_from_G0(unsigned char g0_char);
unsigned char dtvcc_get_internal_from_G1(unsigned char g1_char);
unsigned char dtvcc_get_internal_from_G2(unsigned char g2_char);
unsigned char dtvcc_get_internal_from_G3(unsigned char g3_char);
#endif

#endif /*_CCX_DECODERS_708_ENCODING_H_*/
