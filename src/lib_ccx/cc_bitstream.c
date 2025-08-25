#include "lib_ccx.h"

// Hold functions to read streams on a bit or byte oriented basis
// plus some data related helper functions.

extern uint64_t ccxr_next_bits(struct bitstream *bs, uint32_t bnum);
extern uint64_t ccxr_read_bits(struct bitstream *bs, uint32_t bnum);
extern int ccxr_skip_bits(struct bitstream *bs, uint32_t bnum);
extern int ccxr_is_byte_aligned(struct bitstream *bs);
extern void ccxr_make_byte_aligned(struct bitstream *bs);
extern const uint8_t *ccxr_next_bytes(struct bitstream *bs, size_t bynum);
extern const uint8_t *ccxr_read_bytes(struct bitstream *bs, size_t bynum);
extern uint64_t ccxr_read_exp_golomb_unsigned(struct bitstream *bs);
extern int64_t ccxr_read_exp_golomb(struct bitstream *bs);
extern uint8_t ccxr_reverse8(uint8_t data);
extern uint64_t ccxr_bitstream_get_num(struct bitstream *bs, unsigned bytes, int advance);
extern int64_t ccxr_read_int(struct bitstream *bs, unsigned bnum);

// Guidelines for all bitsream functions:
// * No function shall advance the pointer past the end marker
// * If bitstream.bitsleft < 0 do not attempt any read access,
//   but decrease bitsleft by the number of bits that were
//   attempted to read.

// Initialize bitstream
int init_bitstream(struct bitstream *bstr, unsigned char *start, unsigned char *end)
{
	bstr->pos = start;
	bstr->bpos = 8;
	bstr->end = end;
	bstr->bitsleft = (bstr->end - bstr->pos) * 8;
	bstr->error = 0;
	bstr->_i_pos = NULL;
	bstr->_i_bpos = 0;

	if (bstr->bitsleft < 0)
	{
		// See if we can somehow recover of this disaster by reporting the problem instead of terminating.
		mprint("init_bitstream: bitstream has negative length!");
		return 1;
	}
	return 0;
}

// Read bnum bits from bitstream bstr with the most significant
// bit read first without advancing the bitstream pointer.
// A 64 bit unsigned integer is returned.  0 is returned when
// there are not enough bits left in the bitstream.
uint64_t next_bits(struct bitstream *bstr, unsigned bnum)
{
	return ccxr_next_bits(bstr, bnum);
}

// Read bnum bits from bitstream bstr with the most significant
// bit read first. A 64 bit unsigned integer is returned.
uint64_t read_bits(struct bitstream *bstr, unsigned bnum)
{
	return ccxr_read_bits(bstr, bnum);
}

// This function will advance the bitstream by bnum bits, if possible.
// Advancing of more than 64 bits is possible.
// Return TRUE when successful, otherwise FALSE
int skip_bits(struct bitstream *bstr, unsigned bnum)
{
	return ccxr_skip_bits(bstr, bnum);
}

// Return TRUE if the current position in the bitstream is on a byte
// boundary, i.e., the next bit in the bitstream is the first bit in
// a byte, otherwise return FALSE
int is_byte_aligned(struct bitstream *bstr)
{
	return ccxr_is_byte_aligned(bstr);
}

// Move bitstream to next byte border. Adjust bitsleft.
void make_byte_aligned(struct bitstream *bstr)
{
	ccxr_make_byte_aligned(bstr);
}

// Return pointer to first of bynum bytes from the bitstream if the
// following conditions are TRUE:
// The bitstream is byte aligned and there are enough bytes left in
// it to read bynum bytes.  Otherwise return NULL.
// This function does not advance the bitstream pointer.
unsigned char *next_bytes(struct bitstream *bstr, unsigned bynum)
{
	return (unsigned char *)ccxr_next_bytes(bstr, bynum);
}

// Return pointer to first of bynum bytes from the bitstream if the
// following conditions are TRUE:
// The bitstream is byte aligned and there are enough bytes left in
// it to read bynum bytes.  Otherwise return NULL.
// This function does advance the bitstream pointer.
unsigned char *read_bytes(struct bitstream *bstr, unsigned bynum)
{
	return (unsigned char *)ccxr_read_bytes(bstr, bynum);
}

// Return an integer number with "bytes" precision from the current
// bitstream position.  Allowed "bytes" values are 1,2,4,8.
// This function does advance the bitstream pointer when "advance" is
// set to TRUE.
// Numbers come MSB (most significant first), and we need to account for
// little-endian and big-endian CPUs.
uint64_t bitstream_get_num(struct bitstream *bstr, unsigned bytes, int advance)
{
	return ccxr_bitstream_get_num(bstr, bytes, advance);
}

// Read unsigned Exp-Golomb code from bitstream
uint64_t read_exp_golomb_unsigned(struct bitstream *bstr)
{
	return ccxr_read_exp_golomb_unsigned(bstr);
}

// Read signed Exp-Golomb code from bitstream
int64_t read_exp_golomb(struct bitstream *bstr)
{
	return ccxr_read_exp_golomb(bstr);
}

// Read unsigned integer with bnum bits length.  Basically an
// alias for read_bits().
uint64_t read_int_unsigned(struct bitstream *bstr, unsigned bnum)
{
	return read_bits(bstr, bnum);
}

// Read signed integer with bnum bits length.
int64_t read_int(struct bitstream *bstr, unsigned bnum)
{
	return ccxr_read_int(bstr, bnum);
}

// Return the value with the bit order reversed.
uint8_t reverse8(uint8_t data)
{
	return ccxr_reverse8(data);
}
