#include "ccextractor.h"

// RCWT header (11 bytes):
//byte(s)   value   description (All values below are hex numbers, not
//                  actual numbers or values
//0-2       CCCCED  magic number, for Closed Caption CC Extractor Data
//3         CC      Creating program.  Legal values: CC = CC Extractor
//4-5       0050    Program version number
//6-7       0001    File format version
//8-10      000000  Padding, required  :-)
const unsigned char rcwt_header[11]={0xCC, 0xCC, 0xED, 0xCC, 0x00, 0x50, 0, 1, 0, 0, 0};

const unsigned char BROADCAST_HEADER[]={0xff, 0xff, 0xff, 0xff};
const unsigned char LITTLE_ENDIAN_BOM[]={0xff, 0xfe};
const unsigned char UTF8_BOM[]={0xef, 0xbb,0xbf};

const unsigned char DVD_HEADER[8]={0x00,0x00,0x01,0xb2,0x43,0x43,0x01,0xf8};
const unsigned char lc1[1]={0x8a};
const unsigned char lc2[1]={0x8f};
const unsigned char lc3[2]={0x16,0xfe};
const unsigned char lc4[2]={0x1e,0xfe};
const unsigned char lc5[1]={0xff};
const unsigned char lc6[1]={0xfe};

const double framerates_values[16]=
{
    0,
    24000.0/1001, /* 23.976 */
    24.0,
    25.0,
    30000.0/1001, /* 29.97 */
    30.0,
    50.0,
    60000.0/1001, /* 59.94 */
    60.0,
    0,
    0,
    0,
    0,
    0
};

const char *framerates_types[16]=
{
    "00 - forbidden",
    "01 - 23.976",
    "02 - 24",
    "03 - 25",
    "04 - 29.97",
    "05 - 30",
    "06 - 50",
    "07 - 59.94",
    "08 - 60",
    "09 - reserved",
    "10 - reserved",
    "11 - reserved",
    "12 - reserved",
    "13 - reserved",
    "14 - reserved",
    "15 - reserved"
};

const char *aspect_ratio_types[16]=
{
    "00 - forbidden",
    "01 - 1:1",
    "02 - 4:3",
    "03 - 16:9",
    "04 - 2.21:1",
    "05 - reserved",
    "06 - reserved",
    "07 - reserved",
    "08 - reserved",
    "09 - reserved",
    "10 - reserved",
    "11 - reserved",
    "12 - reserved",
    "13 - reserved",
    "14 - reserved",
    "15 - reserved"
};


const char *pict_types[8]=
{
    "00 - ilegal (0)",
    "01 - I",
    "02 - P",
    "03 - B",
    "04 - ilegal (D)",
    "05 - ilegal (5)",
    "06 - ilegal (6)",
    "07 - ilegal (7)"
};


const char *slice_types[10]=
{
    "0 - P",
    "1 - B",
    "2 - I",
    "3 - SP",
    "4 - SI",
    "5 - P",
    "6 - B",
    "7 - I",
    "8 - SP",
    "9 - SI"
};

const char *cc_types[4] =
{
    "NTSC line 21 field 1 closed captions",
    "NTSC line 21 field 2 closed captions",
    "DTVCC Channel Packet Data",
    "DTVCC Channel Packet Start"
};

enum
{
    NTSC_CC_f1         = 0,
    NTSC_CC_f2         = 1,
    DTVCC_PACKET_DATA  = 2,
    DTVCC_PACKET_START = 3,
};
