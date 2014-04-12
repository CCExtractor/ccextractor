#include "ccextractor.h"

// Hold functions to read streams on a bit or byte oriented basis
// plus some data related helper functions.


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
    bstr->bitsleft = (bstr->end - bstr->pos)*8;
    bstr->error = 0;
    bstr->_i_pos = NULL;
    bstr->_i_bpos = 0;

    if(bstr->bitsleft < 0)
	{		
		// See if we can somehow recover of this disaster by reporting the problem instead of terminating.
        mprint ( "init_bitstream: bitstream has negative length!");
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
    uint64_t res = 0;

    if(bnum > 64)
        fatal(EXIT_BUG_BUG, "next_bits: 64 is maximum bit number, argument: %u!", bnum);

    // Sanity check
    if(bstr->end - bstr->pos < 0)
        fatal(EXIT_BUG_BUG, "next_bits: bitstream has negative length!");

    // Keep a negative bitstream.bitsleft, but correct it.
    if (bstr->bitsleft <= 0)
    {
        bstr->bitsleft -= bnum;
        return 0;
    }

    // Calculate the remaining number of bits in bitstream after reading.
    bstr->bitsleft = 0LL + (bstr->end - bstr->pos - 1)*8 + bstr->bpos - bnum;
    if (bstr->bitsleft < 0)
        return 0;

    // Special case for reading zero bits. Return zero
    if(bnum == 0)
        return 0;

    int vbit = bstr->bpos;
    unsigned char *vpos = bstr->pos;

    if(vbit < 1 || vbit > 8)
    {
        fatal(EXIT_BUG_BUG, "next_bits: Illegal bit position value %d!", vbit);
    }

    while( 1 )
    {
        if(vpos >= bstr->end)
        {
            // We should not get here ...
            fatal(EXIT_BUG_BUG, "next_bits: Reading after end of data ...");
        }

        res |= (*vpos & (0x01 << (vbit-1)) ? 1 : 0);
        vbit--;
        bnum--;

        if(vbit == 0)
        {
            vpos++;
            vbit = 8;
        }

        if(bnum)
        {
            res <<= 1;
        }
        else
            break;
    }

    // Remember the bitstream position
    bstr->_i_bpos = vbit;
    bstr->_i_pos = vpos;

    return res;
}


// Read bnum bits from bitstream bstr with the most significant
// bit read first. A 64 bit unsigned integer is returned.
uint64_t read_bits(struct bitstream *bstr, unsigned bnum)
{
    uint64_t res = next_bits(bstr, bnum);

    // Special case for reading zero bits. Also abort when not enough
    // bits are left. Return zero
    if(bnum == 0 || bstr->bitsleft < 0)
        return 0;

    // Advance the bitstream
    bstr->bpos = bstr->_i_bpos;
    bstr->pos = bstr->_i_pos;

    return res;
}


// This function will advance the bitstream by bnum bits, if possible.
// Advancing of more than 64 bits is possible.
// Return TRUE when successfull, otherwise FALSE
int skip_bits(struct bitstream *bstr, unsigned bnum)
{
    // Sanity check
    if(bstr->end - bstr->pos < 0)
        fatal(EXIT_BUG_BUG, "skip_bits: bitstream has negative length!");

    // Keep a negative bstr->bitsleft, but correct it.
    if (bstr->bitsleft < 0)
    {
        bstr->bitsleft -= bnum;
        return 0;
    }

    // Calculate the remaining number of bits in bitstream after reading.
    bstr->bitsleft = 0LL + (bstr->end - bstr->pos - 1)*8 + bstr->bpos - bnum;
    if (bstr->bitsleft < 0)
        return 0;

    // Special case for reading zero bits. Return zero
    if(bnum == 0)
        return 1;

    bstr->bpos -= bnum%8;
    bstr->pos += bnum/8;

    if (bstr->bpos < 1)
    {
        bstr->bpos += 8;
        bstr->pos += 1;
    }
    return 1;
}


// Return TRUE if the current position in the bitstream is on a byte
// boundary, i.e., the next bit in the bitstream is the first bit in
// a byte, otherwise return FALSE
int is_byte_aligned(struct bitstream *bstr)
{
    // Sanity check
    if(bstr->end - bstr->pos < 0)
        fatal(EXIT_BUG_BUG, "is_byte_aligned: bitstream has negative length!");

    int vbit = bstr->bpos;

    if(vbit == 0 || vbit > 8)
    {
        fatal(EXIT_BUG_BUG, "is_byte_aligned: Illegal bit position value %d!\n", vbit);
    }

    if (vbit == 8)
        return 1;
    else
        return 0;
}


// Move bitstream to next byte border. Adjust bitsleft.
void make_byte_aligned(struct bitstream *bstr)
{
    // Sanity check
    if(bstr->end - bstr->pos < 0)
        fatal(EXIT_BUG_BUG, "make_byte_aligned: bitstream has negative length!");

    int vbit = bstr->bpos;

    if(vbit == 0 || vbit > 8)
    {
        fatal(EXIT_BUG_BUG, "make_byte_aligned: Illegal bit position value %d!\n", vbit);
    }

    // Keep a negative bstr->bitsleft, but correct it.
    if (bstr->bitsleft < 0)
    {
	// Pay attention to the bit alignment
        bstr->bitsleft = (bstr->bitsleft-7)/8 *8;
        return;
    }

    if(bstr->bpos != 8)
    {
        bstr->bpos = 8;
        bstr->pos += 1;
    }
    // Reset, in case a next_???() function was used before
    bstr->bitsleft = 0LL + 8*(bstr->end-bstr->pos-1)+bstr->bpos;

    return;
}


// Return pointer to first of bynum bytes from the bitstream if the
// following conditions are TRUE:
// The bitstream is byte aligned and there are enough bytes left in
// it to read bynum bytes.  Otherwise return NULL.
// This function does not advance the bitstream pointer.
unsigned char *next_bytes(struct bitstream *bstr, unsigned bynum)
{
    // Sanity check
    if(bstr->end - bstr->pos < 0)
        fatal(EXIT_BUG_BUG, "next_bytes: bitstream has negative length!");

    // Keep a negative bstr->bitsleft, but correct it.
    if (bstr->bitsleft < 0)
    {
        bstr->bitsleft -= bynum*8;
        return NULL;
    }

    bstr->bitsleft = 0LL + (bstr->end - bstr->pos - 1)*8 + bstr->bpos - bynum*8;

    if (!is_byte_aligned(bstr) || bstr->bitsleft < 0 || bynum < 1)
        return NULL;

    // Remember the bitstream position
    bstr->_i_bpos = 8;
    bstr->_i_pos = bstr->pos + bynum;

    return bstr->pos;
}


// Return pointer to first of bynum bytes from the bitstream if the
// following conditions are TRUE:
// The bitstream is byte aligned and there are enough bytes left in
// it to read bynum bytes.  Otherwise return NULL.
// This function does advance the bitstream pointer.
unsigned char *read_bytes(struct bitstream *bstr, unsigned bynum)
{
    unsigned char *res = next_bytes(bstr, bynum);

    // Advance the bitstream when a read was possible
    if(res)
    {
        bstr->bpos = bstr->_i_bpos;
        bstr->pos = bstr->_i_pos;
    }
    return res;
}


// Return an integer number with "bytes" precision from the current
// bitstream position.  Allowed "bytes" values are 1,2,4,8.
// This function does advance the bitstream pointer when "advance" is
// set to TRUE.
// Numbers come MSB (most significant first), and we need to account for
// little-endian and big-endian CPUs.
uint64_t bitstream_get_num(struct bitstream *bstr, unsigned bytes, int advance)
{
    void *bpos;
    uint64_t rval=0;

    if (advance)
        bpos = read_bytes(bstr, bytes);
    else
        bpos = next_bytes(bstr, bytes);

    if (!bpos)
        return 0;

    switch (bytes)
    {
		case 1:
		case 2:
		case 4:
		case 8:
			break;
		default:
			fatal (EXIT_BUG_BUG, "bitstream_get_num: Illegal precision value [%u]!",
				   bytes);
			break;
    }
	for (unsigned i=0;i<bytes;i++)
	{
		unsigned char *ucpos=((unsigned char *)bpos) +bytes-i-1; // Read backwards		
		unsigned char uc=*ucpos;		
		rval=(rval<<8) + uc;
	}
    return rval; 
}


// Read unsigned Exp-Golomb code from bitstream
uint64_t ue(struct bitstream *bstr)
{
    uint64_t res = 0;
    int zeros=0;

    while(!read_bits(bstr,1))
        zeros++;

    res = (0x01 << zeros) - 1 + read_bits(bstr,zeros);

    return res;
}


// Read signed Exp-Golomb code from bitstream
int64_t se(struct bitstream *bstr)
{
    int64_t res = 0;

    res = ue(bstr);
    
    // The following function might truncate when res+1 overflows
    //res = (res+1)/2 * (res % 2 ? 1 : -1);
    // Use this:
    res = (res/2+(res%2 ? 1 : 0)) * (res % 2 ? 1 : -1);

    return res;
}


// Read unsigned integer with bnum bits length.  Basically an
// alias for read_bits().
uint64_t u(struct bitstream *bstr, unsigned bnum)
{
    return read_bits(bstr, bnum);
}


// Read signed integer with bnum bits length.
int64_t i(struct bitstream *bstr, unsigned bnum)
{
    uint64_t res = read_bits(bstr, bnum);

    // Special case for reading zero bits. Return zero
    if(bnum == 0)
        return 0;

    return (0xFFFFFFFFFFFFFFFFULL << bnum) | res;
}


// Return the value with the bit order reversed.
uint8_t reverse8(uint8_t data)
{
    uint8_t res = 0;

    for (int k=0;k<8;k++)
    {
        res <<= 1;
        res |= (data & (0x01 << k) ? 1 : 0);
    }

    return res;
}
