#ifndef _BITSTREAM_
#define _BITSTREAM_


// The structure holds the current position in the bitstream.
// pos points to the current byte position and bpos counts the
// bits left unread at the current byte pos. No bit read means
// bpos = 8.
// end is used to check that nothing is read at "end" or after it.
struct bitstream
{
    unsigned char *pos;
    int bpos;
    unsigned char *end;
    // Indicate how many bits are left in the stream after the previous
    // read call.  A negative number indicates that a read after the
    // end of the stream was attempted.
    int64_t bitsleft;
    // Indicate an error occured while parsing the bitstream.
    // This is meant to store high level syntax errors, i.e a function
    // using the bitstream functions found a syntax error.
    int error;
    // Internal (private) variable - used to store the the bitstream
    // position until it is decided if the bitstream pointer will be
    // increased by the calling function, or not.
    unsigned char *_i_pos;
    int _i_bpos;
};

#define read_u8(bstream) (uint8_t)bitstream_get_num(bstream,1,1)
#define read_u16(bstream) (uint16_t)bitstream_get_num(bstream,2,1)
#define read_u32(bstream) (uint32_t)bitstream_get_num(bstream,4,1)
#define read_u64(bstream) (uint64_t)bitstream_get_num(bstream,8,1)
#define read_i8(bstream) (int8_t)bitstream_get_num(bstream,1,1)
#define read_i16(bstream) (int16_t)bitstream_get_num(bstream,2,1)
#define read_i32(bstream) (int32_t)bitstream_get_num(bstream,4,1)
#define read_i64(bstream) (int64_t)bitstream_get_num(bstream,8,1)

#define next_u8(bstream) (uint8_t)bitstream_get_num(bstream,1,0)
#define next_u16(bstream) (uint16_t)bitstream_get_num(bstream,2,0)
#define next_u32(bstream) (uint32_t)bitstream_get_num(bstream,4,0)
#define next_u64(bstream) (uint64_t)bitstream_get_num(bstream,8,0)
#define next_i8(bstream) (int8_t)bitstream_get_num(bstream,1,0)
#define next_i16(bstream) (int16_t)bitstream_get_num(bstream,2,0)
#define next_i32(bstream) (int32_t)bitstream_get_num(bstream,4,0)
#define next_i64(bstream) (int64_t)bitstream_get_num(bstream,8,0)


int init_bitstream(struct bitstream *bstr, unsigned char *start, unsigned char *end);
uint64_t next_bits(struct bitstream *bstr, unsigned bnum);
uint64_t read_bits(struct bitstream *bstr, unsigned bnum);
int skip_bits(struct bitstream *bstr, unsigned bnum);
int is_byte_aligned(struct bitstream *bstr);
void make_byte_aligned(struct bitstream *bstr);
unsigned char *next_bytes(struct bitstream *bstr, unsigned bynum);
unsigned char *read_bytes(struct bitstream *bstr, unsigned bynum);
uint64_t bitstream_get_num(struct bitstream *bstr, unsigned bytes, int advance);
uint64_t ue(struct bitstream *bstr);
int64_t se(struct bitstream *bstr);
uint64_t u(struct bitstream *bstr, unsigned bnum);
int64_t i(struct bitstream *bstr, unsigned bnum);
uint8_t reverse8(uint8_t data);

#endif
