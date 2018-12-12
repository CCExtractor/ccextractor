#include "ccx_common_constants.h"

// RCWT header (11 bytes):
//byte(s)   value   description (All values below are hex numbers, not
//                  actual numbers or values
//0-2       CCCCED  magic number, for Closed Caption CC Extractor Data
//3         CC      Creating program.  Legal values: CC = CC Extractor
//4-5       0050    Program version number
//6-7       0001    File format version
//8-10      000000  Padding, required  :-)
unsigned char rcwt_header[11]={0xCC, 0xCC, 0xED, 0xCC, 0x00, 0x50, 0, 1, 0, 0, 0};

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
	"00 - illegal (0)",
	"01 - I",
	"02 - P",
	"03 - B",
	"04 - illegal (D)",
	"05 - illegal (5)",
	"06 - illegal (6)",
	"07 - illegal (7)"
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

/**
 * After Adding a new language here, don't forget
 * to increase NB_LANGUAGE define ccx_common_constants.h
 */
const char *language[NB_LANGUAGE] =
{
	/*0*/"und", //Undefined
	/*1*/"eng",
	/*2*/"afr",
	/*3*/"amh",
	/*4*/"ara",
	/*5*/"asm",
	/*6*/"aze",
	/*7*/"bel",
	/*8*/"ben",
	/*9*/"bod",
	/*10*/"bos",
	/*11*/"bul",
	/*12*/"cat",
	/*13*/"ceb",
	/*14*/"ces",
	/*15*/"chs",
	/*16*/"chi",
	/*17*/"chr",
	/*18*/"cym",
	/*19*/"dan",
	/*20*/"deu",
	/*21*/"dzo",
	/*22*/"ell",
	/*23*/"enm",
	/*24*/"epo",
	/*25*/"equ",
	/*26*/"est",
	/*27*/"eus",
	/*28*/"fas",
	/*29*/"fin",
	/*30*/"fra",
	/*31*/"frk",
	/*32*/"frm",
	/*33*/"gle",
	/*34*/"glg",
	/*35*/"grc",
	/*36*/"guj",
	/*37*/"hat",
	/*38*/"heb",
	/*39*/"hin",
	/*40*/"hrv",
	/*41*/"hun",
	/*42*/"iku",
	/*43*/"ind",
	/*44*/"isl",
	/*45*/"ita",
	/*46*/"jav",
	/*47*/"jpn",
	/*48*/"kan",
	/*49*/"kat",
	/*50*/"kaz",
	/*51*/"khm",
	/*52*/"kir",
	/*53*/"kor",
	/*54*/"kur",
	/*55*/"lao",
	/*56*/"lat",
	/*57*/"lav",
	/*58*/"lit",
	/*59*/"mal",
	/*60*/"mar",
	/*61*/"mkd",
	/*62*/"mlt",
	/*63*/"msa",
	/*64*/"mya",
	/*65*/"nep",
	/*66*/"nld",
	/*67*/"nor",
	/*68*/"ori",
	/*69*/"osd",
	/*70*/"pan",
	/*71*/"pol",
	/*72*/"por",
	/*73*/"pus",
	/*74*/"ron",
	/*75*/"rus",
	/*76*/"san",
	/*77*/"sin",
	/*78*/"slk",
	/*79*/"slv",
	/*80*/"spa",
	/*81*/"sqi",
	/*82*/"srp",
	/*83*/"swa",
	/*84*/"swe",
	/*85*/"syr",
	/*86*/"tam",
	/*87*/"tel",
	/*88*/"tgk",
	/*89*/"tgl",
	/*90*/"tha",
	/*91*/"tir",
	/*92*/"tur",
	/*93*/"uig",
	/*94*/"ukr",
	/*95*/"urd",
	/*96*/"uzb",
	/*97*/"vie",
	/*98*/"yid",
	NULL
};
