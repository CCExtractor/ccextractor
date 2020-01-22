// Reference: https://www.govinfo.gov/app/details/CFR-2011-title47-vol1/CFR-2011-title47-vol1-sec15-119/summary
// Implementatin issue: https://github.com/CCExtractor/ccextractor/issues/1120

#include "lib_ccx.h"
#include <stdbool.h>

// Adds a parity bit if needed
unsigned char odd_parity(const unsigned char byte)
{
	return byte | !(cc608_parity(byte) % 2) << 7;
}

// TODO: deal with "\n" vs "\r\n"
// TODO: colors. I don't have example files to work with and double check what I do

// TO Codes (Tab Offset)
// Are the column offset the caption should be placed at to be more precise
// than preamble codes as preamble codes can only be precise by 4 columns
// e.g.
// indent * 4 + tab_offset
// where tab_offset can be 1 to 3. If the offset isn't within 1 to 3, it is
// unneeded and disappears, as the indent is enough.

enum control_code {
	// Mid-Row Codes
	WH,    // White;                          second byte: 0x20
	WHU,   // White Underline;                second byte: 0x21
	/*
	... 12 unimplemented
	*/
	I,     // Italics;                        second byte: 0x2e
	IU,    // Italics Underline;              second byte: 0x2f
	// Miscellaneous Control Codes
	RCL,   // Resume Caption Loading;         second byte: 0x20
	ENM,   // Erase Non-Displayed Memory;     second byte: 0x2d
	EOC,   // End of Caption (Flip Memories); second byte: 0x2f
	TO1,   // Tab Offset 1 Column;            second byte: 0x21
	TO2,   // Tab Offset 2 Column;            second byte: 0x22
	TO3,   // Tab Offset 3 Column;            second byte: 0x23
	// Preamble Address Codes; prefixed with and underscore (`_`), because identifier cannot start with digits
	_0100,
	_0104,
	_0108,
	_0112,
	_0116,
	_0120,
	_0124,
	_0128,
	_0200,
	_0204,
	_0208,
	_0212,
	_0216,
	_0220,
	_0224,
	_0228,
	_0300,
	_0304,
	_0308,
	_0312,
	_0316,
	_0320,
	_0324,
	_0328,
	_0400,
	_0404,
	_0408,
	_0412,
	_0416,
	_0420,
	_0424,
	_0428,
	_0500,
	_0504,
	_0508,
	_0512,
	_0516,
	_0520,
	_0524,
	_0528,
	_0600,
	_0604,
	_0608,
	_0612,
	_0616,
	_0620,
	_0624,
	_0628,
	_0700,
	_0704,
	_0708,
	_0712,
	_0716,
	_0720,
	_0724,
	_0728,
	_0800,
	_0804,
	_0808,
	_0812,
	_0816,
	_0820,
	_0824,
	_0828,
	_0900,
	_0904,
	_0908,
	_0912,
	_0916,
	_0920,
	_0924,
	_0928,
	_1000,
	_1004,
	_1008,
	_1012,
	_1016,
	_1020,
	_1024,
	_1028,
	_1100,
	_1104,
	_1108,
	_1112,
	_1116,
	_1120,
	_1124,
	_1128,
	_1200,
	_1204,
	_1208,
	_1212,
	_1216,
	_1220,
	_1224,
	_1228,
	_1300,
	_1304,
	_1308,
	_1312,
	_1316,
	_1320,
	_1324,
	_1328,
	_1400,
	_1404,
	_1408,
	_1412,
	_1416,
	_1420,
	_1424,
	_1428,
	_1500,
	_1504,
	_1508,
	_1512,
	_1516,
	_1520,
	_1524,
	_1528,
};

const char WH_ASSEMBLY[]  = "{Wh}";
const char WHU_ASSEMBLY[] = "{WhU}";
const char I_ASSEMBLY[]   = "{I}";
const char IU_ASSEMBLY[]  = "{IU}";

const char RCL_ASSEMBLY[] = "{RCL}";
const char ENM_ASSEMBLY[] = "{ENM}";
const char EOC_ASSEMBLY[] = "{EOC}";
const char TO1_ASSEMBLY[] = "{TO1}";
const char TO2_ASSEMBLY[] = "{TO2}";
const char TO3_ASSEMBLY[] = "{TO3}";

const char _0100_ASSEMBLY[] = "{0100}";
const char _0104_ASSEMBLY[] = "{0104}";
const char _0108_ASSEMBLY[] = "{0108}";
const char _0112_ASSEMBLY[] = "{0112}";
const char _0116_ASSEMBLY[] = "{0116}";
const char _0120_ASSEMBLY[] = "{0120}";
const char _0124_ASSEMBLY[] = "{0124}";
const char _0128_ASSEMBLY[] = "{0128}";
const char _0200_ASSEMBLY[] = "{0200}";
const char _0204_ASSEMBLY[] = "{0204}";
const char _0208_ASSEMBLY[] = "{0208}";
const char _0212_ASSEMBLY[] = "{0212}";
const char _0216_ASSEMBLY[] = "{0216}";
const char _0220_ASSEMBLY[] = "{0220}";
const char _0224_ASSEMBLY[] = "{0224}";
const char _0228_ASSEMBLY[] = "{0228}";
const char _0300_ASSEMBLY[] = "{0300}";
const char _0304_ASSEMBLY[] = "{0304}";
const char _0308_ASSEMBLY[] = "{0308}";
const char _0312_ASSEMBLY[] = "{0312}";
const char _0316_ASSEMBLY[] = "{0316}";
const char _0320_ASSEMBLY[] = "{0320}";
const char _0324_ASSEMBLY[] = "{0324}";
const char _0328_ASSEMBLY[] = "{0328}";
const char _0400_ASSEMBLY[] = "{0400}";
const char _0404_ASSEMBLY[] = "{0404}";
const char _0408_ASSEMBLY[] = "{0408}";
const char _0412_ASSEMBLY[] = "{0412}";
const char _0416_ASSEMBLY[] = "{0416}";
const char _0420_ASSEMBLY[] = "{0420}";
const char _0424_ASSEMBLY[] = "{0424}";
const char _0428_ASSEMBLY[] = "{0428}";
const char _0500_ASSEMBLY[] = "{0500}";
const char _0504_ASSEMBLY[] = "{0504}";
const char _0508_ASSEMBLY[] = "{0508}";
const char _0512_ASSEMBLY[] = "{0512}";
const char _0516_ASSEMBLY[] = "{0516}";
const char _0520_ASSEMBLY[] = "{0520}";
const char _0524_ASSEMBLY[] = "{0524}";
const char _0528_ASSEMBLY[] = "{0528}";
const char _0600_ASSEMBLY[] = "{0600}";
const char _0604_ASSEMBLY[] = "{0604}";
const char _0608_ASSEMBLY[] = "{0608}";
const char _0612_ASSEMBLY[] = "{0612}";
const char _0616_ASSEMBLY[] = "{0616}";
const char _0620_ASSEMBLY[] = "{0620}";
const char _0624_ASSEMBLY[] = "{0624}";
const char _0628_ASSEMBLY[] = "{0628}";
const char _0700_ASSEMBLY[] = "{0700}";
const char _0704_ASSEMBLY[] = "{0704}";
const char _0708_ASSEMBLY[] = "{0708}";
const char _0712_ASSEMBLY[] = "{0712}";
const char _0716_ASSEMBLY[] = "{0716}";
const char _0720_ASSEMBLY[] = "{0720}";
const char _0724_ASSEMBLY[] = "{0724}";
const char _0728_ASSEMBLY[] = "{0728}";
const char _0800_ASSEMBLY[] = "{0800}";
const char _0804_ASSEMBLY[] = "{0804}";
const char _0808_ASSEMBLY[] = "{0808}";
const char _0812_ASSEMBLY[] = "{0812}";
const char _0816_ASSEMBLY[] = "{0816}";
const char _0820_ASSEMBLY[] = "{0820}";
const char _0824_ASSEMBLY[] = "{0824}";
const char _0828_ASSEMBLY[] = "{0828}";
const char _0900_ASSEMBLY[] = "{0900}";
const char _0904_ASSEMBLY[] = "{0904}";
const char _0908_ASSEMBLY[] = "{0908}";
const char _0912_ASSEMBLY[] = "{0912}";
const char _0916_ASSEMBLY[] = "{0916}";
const char _0920_ASSEMBLY[] = "{0920}";
const char _0924_ASSEMBLY[] = "{0924}";
const char _0928_ASSEMBLY[] = "{0928}";
const char _1000_ASSEMBLY[] = "{1000}";
const char _1004_ASSEMBLY[] = "{1004}";
const char _1008_ASSEMBLY[] = "{1008}";
const char _1012_ASSEMBLY[] = "{1012}";
const char _1016_ASSEMBLY[] = "{1016}";
const char _1020_ASSEMBLY[] = "{1020}";
const char _1024_ASSEMBLY[] = "{1024}";
const char _1028_ASSEMBLY[] = "{1028}";
const char _1100_ASSEMBLY[] = "{1100}";
const char _1104_ASSEMBLY[] = "{1104}";
const char _1108_ASSEMBLY[] = "{1108}";
const char _1112_ASSEMBLY[] = "{1112}";
const char _1116_ASSEMBLY[] = "{1116}";
const char _1120_ASSEMBLY[] = "{1120}";
const char _1124_ASSEMBLY[] = "{1124}";
const char _1128_ASSEMBLY[] = "{1128}";
const char _1200_ASSEMBLY[] = "{1200}";
const char _1204_ASSEMBLY[] = "{1204}";
const char _1208_ASSEMBLY[] = "{1208}";
const char _1212_ASSEMBLY[] = "{1212}";
const char _1216_ASSEMBLY[] = "{1216}";
const char _1220_ASSEMBLY[] = "{1220}";
const char _1224_ASSEMBLY[] = "{1224}";
const char _1228_ASSEMBLY[] = "{1228}";
const char _1300_ASSEMBLY[] = "{1300}";
const char _1304_ASSEMBLY[] = "{1304}";
const char _1308_ASSEMBLY[] = "{1308}";
const char _1312_ASSEMBLY[] = "{1312}";
const char _1316_ASSEMBLY[] = "{1316}";
const char _1320_ASSEMBLY[] = "{1320}";
const char _1324_ASSEMBLY[] = "{1324}";
const char _1328_ASSEMBLY[] = "{1328}";
const char _1400_ASSEMBLY[] = "{1400}";
const char _1404_ASSEMBLY[] = "{1404}";
const char _1408_ASSEMBLY[] = "{1408}";
const char _1412_ASSEMBLY[] = "{1412}";
const char _1416_ASSEMBLY[] = "{1416}";
const char _1420_ASSEMBLY[] = "{1420}";
const char _1424_ASSEMBLY[] = "{1424}";
const char _1428_ASSEMBLY[] = "{1428}";
const char _1500_ASSEMBLY[] = "{1500}";
const char _1504_ASSEMBLY[] = "{1504}";
const char _1508_ASSEMBLY[] = "{1508}";
const char _1512_ASSEMBLY[] = "{1512}";
const char _1516_ASSEMBLY[] = "{1516}";
const char _1520_ASSEMBLY[] = "{1520}";
const char _1524_ASSEMBLY[] = "{1524}";
const char _1528_ASSEMBLY[] = "{1528}";

const char *disassemble_code(const enum control_code code, unsigned int *length)
{
	switch (code)
	{
		// Mid-Row Codes
		case WH:
			*length = sizeof(WH_ASSEMBLY) - 1; // `- 1` to not include the '\0' at the end of the `char *`
			return WH_ASSEMBLY;
		case WHU:
			*length = sizeof(WHU_ASSEMBLY) - 1;
			return WHU_ASSEMBLY;
		case I:
			*length = sizeof(I_ASSEMBLY) - 1;
			return I_ASSEMBLY;
		case IU:
			*length = sizeof(IU_ASSEMBLY) - 1;
			return IU_ASSEMBLY;
		// Miscellaneous Control Codes
		case RCL:
			*length = sizeof(RCL_ASSEMBLY) - 1;
			return RCL_ASSEMBLY;
		case ENM:
			*length = sizeof(ENM_ASSEMBLY) - 1;
			return ENM_ASSEMBLY;
		case EOC:
			*length = sizeof(EOC_ASSEMBLY) - 1;
			return EOC_ASSEMBLY;
		case TO1:
			*length = sizeof(TO1_ASSEMBLY) - 1;
			return TO1_ASSEMBLY;
		case TO2:
			*length = sizeof(TO2_ASSEMBLY) - 1;
			return TO2_ASSEMBLY;
		case TO3:
			*length = sizeof(TO3_ASSEMBLY) - 1;
			return TO3_ASSEMBLY;
		// Preamble Address Codes
		case _0100:
			*length = sizeof(_0100_ASSEMBLY) - 1;
			return _0100_ASSEMBLY;
		case _0104:
			*length = sizeof(_0104_ASSEMBLY) - 1;
			return _0104_ASSEMBLY;
		case _0108:
			*length = sizeof(_0108_ASSEMBLY) - 1;
			return _0108_ASSEMBLY;
		case _0112:
			*length = sizeof(_0112_ASSEMBLY) - 1;
			return _0112_ASSEMBLY;
		case _0116:
			*length = sizeof(_0116_ASSEMBLY) - 1;
			return _0116_ASSEMBLY;
		case _0120:
			*length = sizeof(_0120_ASSEMBLY) - 1;
			return _0120_ASSEMBLY;
		case _0124:
			*length = sizeof(_0124_ASSEMBLY) - 1;
			return _0124_ASSEMBLY;
		case _0128:
			*length = sizeof(_0128_ASSEMBLY) - 1;
			return _0128_ASSEMBLY;
		case _0200:
			*length = sizeof(_0200_ASSEMBLY) - 1;
			return _0200_ASSEMBLY;
		case _0204:
			*length = sizeof(_0204_ASSEMBLY) - 1;
			return _0204_ASSEMBLY;
		case _0208:
			*length = sizeof(_0208_ASSEMBLY) - 1;
			return _0208_ASSEMBLY;
		case _0212:
			*length = sizeof(_0212_ASSEMBLY) - 1;
			return _0212_ASSEMBLY;
		case _0216:
			*length = sizeof(_0216_ASSEMBLY) - 1;
			return _0216_ASSEMBLY;
		case _0220:
			*length = sizeof(_0220_ASSEMBLY) - 1;
			return _0220_ASSEMBLY;
		case _0224:
			*length = sizeof(_0224_ASSEMBLY) - 1;
			return _0224_ASSEMBLY;
		case _0228:
			*length = sizeof(_0228_ASSEMBLY) - 1;
			return _0228_ASSEMBLY;
		case _0300:
			*length = sizeof(_0300_ASSEMBLY) - 1;
			return _0300_ASSEMBLY;
		case _0304:
			*length = sizeof(_0304_ASSEMBLY) - 1;
			return _0304_ASSEMBLY;
		case _0308:
			*length = sizeof(_0308_ASSEMBLY) - 1;
			return _0308_ASSEMBLY;
		case _0312:
			*length = sizeof(_0312_ASSEMBLY) - 1;
			return _0312_ASSEMBLY;
		case _0316:
			*length = sizeof(_0316_ASSEMBLY) - 1;
			return _0316_ASSEMBLY;
		case _0320:
			*length = sizeof(_0320_ASSEMBLY) - 1;
			return _0320_ASSEMBLY;
		case _0324:
			*length = sizeof(_0324_ASSEMBLY) - 1;
			return _0324_ASSEMBLY;
		case _0328:
			*length = sizeof(_0328_ASSEMBLY) - 1;
			return _0328_ASSEMBLY;
		case _0400:
			*length = sizeof(_0400_ASSEMBLY) - 1;
			return _0400_ASSEMBLY;
		case _0404:
			*length = sizeof(_0404_ASSEMBLY) - 1;
			return _0404_ASSEMBLY;
		case _0408:
			*length = sizeof(_0408_ASSEMBLY) - 1;
			return _0408_ASSEMBLY;
		case _0412:
			*length = sizeof(_0412_ASSEMBLY) - 1;
			return _0412_ASSEMBLY;
		case _0416:
			*length = sizeof(_0416_ASSEMBLY) - 1;
			return _0416_ASSEMBLY;
		case _0420:
			*length = sizeof(_0420_ASSEMBLY) - 1;
			return _0420_ASSEMBLY;
		case _0424:
			*length = sizeof(_0424_ASSEMBLY) - 1;
			return _0424_ASSEMBLY;
		case _0428:
			*length = sizeof(_0428_ASSEMBLY) - 1;
			return _0428_ASSEMBLY;
		case _0500:
			*length = sizeof(_0500_ASSEMBLY) - 1;
			return _0500_ASSEMBLY;
		case _0504:
			*length = sizeof(_0504_ASSEMBLY) - 1;
			return _0504_ASSEMBLY;
		case _0508:
			*length = sizeof(_0508_ASSEMBLY) - 1;
			return _0508_ASSEMBLY;
		case _0512:
			*length = sizeof(_0512_ASSEMBLY) - 1;
			return _0512_ASSEMBLY;
		case _0516:
			*length = sizeof(_0516_ASSEMBLY) - 1;
			return _0516_ASSEMBLY;
		case _0520:
			*length = sizeof(_0520_ASSEMBLY) - 1;
			return _0520_ASSEMBLY;
		case _0524:
			*length = sizeof(_0524_ASSEMBLY) - 1;
			return _0524_ASSEMBLY;
		case _0528:
			*length = sizeof(_0528_ASSEMBLY) - 1;
			return _0528_ASSEMBLY;
		case _0600:
			*length = sizeof(_0600_ASSEMBLY) - 1;
			return _0600_ASSEMBLY;
		case _0604:
			*length = sizeof(_0604_ASSEMBLY) - 1;
			return _0604_ASSEMBLY;
		case _0608:
			*length = sizeof(_0608_ASSEMBLY) - 1;
			return _0608_ASSEMBLY;
		case _0612:
			*length = sizeof(_0612_ASSEMBLY) - 1;
			return _0612_ASSEMBLY;
		case _0616:
			*length = sizeof(_0616_ASSEMBLY) - 1;
			return _0616_ASSEMBLY;
		case _0620:
			*length = sizeof(_0620_ASSEMBLY) - 1;
			return _0620_ASSEMBLY;
		case _0624:
			*length = sizeof(_0624_ASSEMBLY) - 1;
			return _0624_ASSEMBLY;
		case _0628:
			*length = sizeof(_0628_ASSEMBLY) - 1;
			return _0628_ASSEMBLY;
		case _0700:
			*length = sizeof(_0700_ASSEMBLY) - 1;
			return _0700_ASSEMBLY;
		case _0704:
			*length = sizeof(_0704_ASSEMBLY) - 1;
			return _0704_ASSEMBLY;
		case _0708:
			*length = sizeof(_0708_ASSEMBLY) - 1;
			return _0708_ASSEMBLY;
		case _0712:
			*length = sizeof(_0712_ASSEMBLY) - 1;
			return _0712_ASSEMBLY;
		case _0716:
			*length = sizeof(_0716_ASSEMBLY) - 1;
			return _0716_ASSEMBLY;
		case _0720:
			*length = sizeof(_0720_ASSEMBLY) - 1;
			return _0720_ASSEMBLY;
		case _0724:
			*length = sizeof(_0724_ASSEMBLY) - 1;
			return _0724_ASSEMBLY;
		case _0728:
			*length = sizeof(_0728_ASSEMBLY) - 1;
			return _0728_ASSEMBLY;
		case _0800:
			*length = sizeof(_0800_ASSEMBLY) - 1;
			return _0800_ASSEMBLY;
		case _0804:
			*length = sizeof(_0804_ASSEMBLY) - 1;
			return _0804_ASSEMBLY;
		case _0808:
			*length = sizeof(_0808_ASSEMBLY) - 1;
			return _0808_ASSEMBLY;
		case _0812:
			*length = sizeof(_0812_ASSEMBLY) - 1;
			return _0812_ASSEMBLY;
		case _0816:
			*length = sizeof(_0816_ASSEMBLY) - 1;
			return _0816_ASSEMBLY;
		case _0820:
			*length = sizeof(_0820_ASSEMBLY) - 1;
			return _0820_ASSEMBLY;
		case _0824:
			*length = sizeof(_0824_ASSEMBLY) - 1;
			return _0824_ASSEMBLY;
		case _0828:
			*length = sizeof(_0828_ASSEMBLY) - 1;
			return _0828_ASSEMBLY;
		case _0900:
			*length = sizeof(_0900_ASSEMBLY) - 1;
			return _0900_ASSEMBLY;
		case _0904:
			*length = sizeof(_0904_ASSEMBLY) - 1;
			return _0904_ASSEMBLY;
		case _0908:
			*length = sizeof(_0908_ASSEMBLY) - 1;
			return _0908_ASSEMBLY;
		case _0912:
			*length = sizeof(_0912_ASSEMBLY) - 1;
			return _0912_ASSEMBLY;
		case _0916:
			*length = sizeof(_0916_ASSEMBLY) - 1;
			return _0916_ASSEMBLY;
		case _0920:
			*length = sizeof(_0920_ASSEMBLY) - 1;
			return _0920_ASSEMBLY;
		case _0924:
			*length = sizeof(_0924_ASSEMBLY) - 1;
			return _0924_ASSEMBLY;
		case _0928:
			*length = sizeof(_0928_ASSEMBLY) - 1;
			return _0928_ASSEMBLY;
		case _1000:
			*length = sizeof(_1000_ASSEMBLY) - 1;
			return _1000_ASSEMBLY;
		case _1004:
			*length = sizeof(_1004_ASSEMBLY) - 1;
			return _1004_ASSEMBLY;
		case _1008:
			*length = sizeof(_1008_ASSEMBLY) - 1;
			return _1008_ASSEMBLY;
		case _1012:
			*length = sizeof(_1012_ASSEMBLY) - 1;
			return _1012_ASSEMBLY;
		case _1016:
			*length = sizeof(_1016_ASSEMBLY) - 1;
			return _1016_ASSEMBLY;
		case _1020:
			*length = sizeof(_1020_ASSEMBLY) - 1;
			return _1020_ASSEMBLY;
		case _1024:
			*length = sizeof(_1024_ASSEMBLY) - 1;
			return _1024_ASSEMBLY;
		case _1028:
			*length = sizeof(_1028_ASSEMBLY) - 1;
			return _1028_ASSEMBLY;
		case _1100:
			*length = sizeof(_1100_ASSEMBLY) - 1;
			return _1100_ASSEMBLY;
		case _1104:
			*length = sizeof(_1104_ASSEMBLY) - 1;
			return _1104_ASSEMBLY;
		case _1108:
			*length = sizeof(_1108_ASSEMBLY) - 1;
			return _1108_ASSEMBLY;
		case _1112:
			*length = sizeof(_1112_ASSEMBLY) - 1;
			return _1112_ASSEMBLY;
		case _1116:
			*length = sizeof(_1116_ASSEMBLY) - 1;
			return _1116_ASSEMBLY;
		case _1120:
			*length = sizeof(_1120_ASSEMBLY) - 1;
			return _1120_ASSEMBLY;
		case _1124:
			*length = sizeof(_1124_ASSEMBLY) - 1;
			return _1124_ASSEMBLY;
		case _1128:
			*length = sizeof(_1128_ASSEMBLY) - 1;
			return _1128_ASSEMBLY;
		case _1200:
			*length = sizeof(_1200_ASSEMBLY) - 1;
			return _1200_ASSEMBLY;
		case _1204:
			*length = sizeof(_1204_ASSEMBLY) - 1;
			return _1204_ASSEMBLY;
		case _1208:
			*length = sizeof(_1208_ASSEMBLY) - 1;
			return _1208_ASSEMBLY;
		case _1212:
			*length = sizeof(_1212_ASSEMBLY) - 1;
			return _1212_ASSEMBLY;
		case _1216:
			*length = sizeof(_1216_ASSEMBLY) - 1;
			return _1216_ASSEMBLY;
		case _1220:
			*length = sizeof(_1220_ASSEMBLY) - 1;
			return _1220_ASSEMBLY;
		case _1224:
			*length = sizeof(_1224_ASSEMBLY) - 1;
			return _1224_ASSEMBLY;
		case _1228:
			*length = sizeof(_1228_ASSEMBLY) - 1;
			return _1228_ASSEMBLY;
		case _1300:
			*length = sizeof(_1300_ASSEMBLY) - 1;
			return _1300_ASSEMBLY;
		case _1304:
			*length = sizeof(_1304_ASSEMBLY) - 1;
			return _1304_ASSEMBLY;
		case _1308:
			*length = sizeof(_1308_ASSEMBLY) - 1;
			return _1308_ASSEMBLY;
		case _1312:
			*length = sizeof(_1312_ASSEMBLY) - 1;
			return _1312_ASSEMBLY;
		case _1316:
			*length = sizeof(_1316_ASSEMBLY) - 1;
			return _1316_ASSEMBLY;
		case _1320:
			*length = sizeof(_1320_ASSEMBLY) - 1;
			return _1320_ASSEMBLY;
		case _1324:
			*length = sizeof(_1324_ASSEMBLY) - 1;
			return _1324_ASSEMBLY;
		case _1328:
			*length = sizeof(_1328_ASSEMBLY) - 1;
			return _1328_ASSEMBLY;
		case _1400:
			*length = sizeof(_1400_ASSEMBLY) - 1;
			return _1400_ASSEMBLY;
		case _1404:
			*length = sizeof(_1404_ASSEMBLY) - 1;
			return _1404_ASSEMBLY;
		case _1408:
			*length = sizeof(_1408_ASSEMBLY) - 1;
			return _1408_ASSEMBLY;
		case _1412:
			*length = sizeof(_1412_ASSEMBLY) - 1;
			return _1412_ASSEMBLY;
		case _1416:
			*length = sizeof(_1416_ASSEMBLY) - 1;
			return _1416_ASSEMBLY;
		case _1420:
			*length = sizeof(_1420_ASSEMBLY) - 1;
			return _1420_ASSEMBLY;
		case _1424:
			*length = sizeof(_1424_ASSEMBLY) - 1;
			return _1424_ASSEMBLY;
		case _1428:
			*length = sizeof(_1428_ASSEMBLY) - 1;
			return _1428_ASSEMBLY;
		case _1500:
			*length = sizeof(_1500_ASSEMBLY) - 1;
			return _1500_ASSEMBLY;
		case _1504:
			*length = sizeof(_1504_ASSEMBLY) - 1;
			return _1504_ASSEMBLY;
		case _1508:
			*length = sizeof(_1508_ASSEMBLY) - 1;
			return _1508_ASSEMBLY;
		case _1512:
			*length = sizeof(_1512_ASSEMBLY) - 1;
			return _1512_ASSEMBLY;
		case _1516:
			*length = sizeof(_1516_ASSEMBLY) - 1;
			return _1516_ASSEMBLY;
		case _1520:
			*length = sizeof(_1520_ASSEMBLY) - 1;
			return _1520_ASSEMBLY;
		case _1524:
			*length = sizeof(_1524_ASSEMBLY) - 1;
			return _1524_ASSEMBLY;
		case _1528:
			*length = sizeof(_1528_ASSEMBLY) - 1;
			return _1528_ASSEMBLY;
		default:
			fatal(-1, "Cannot disassemble code");
	}
}

bool is_odd_channel(const unsigned char channel)
{
	// generally (as per the reference) channel 1 and 3 and channel 2 and 4
	// behave in a similar way
	return channel % 2 == 1;
}

unsigned get_first_byte(const unsigned char channel, const enum control_code code) 
{
	switch (code)
	{
		// Mid-Row Codes
		case WH:
		case WHU:
		case I:
		case IU:
			if (is_odd_channel(channel))
			{
				return 0x11;
			}
			else
			{
				return 0x19;
			}
		// Miscellaneous Control Codes
		case RCL:
		case ENM:
		case EOC:
			if (is_odd_channel(channel))
			{
				return 0x14;
			}
			else
			{
				return 0x1c;
			}
		case TO1:
		case TO2:
		case TO3:
			if (is_odd_channel(channel))
			{
				return 0x17;
			}
			else
			{
				return 0x1f;
			}
		// Preamble Address Codes
		case _0100:
		case _0104:
		case _0108:
		case _0112:
		case _0116:
		case _0120:
		case _0124:
		case _0128:
		case _0200:
		case _0204:
		case _0208:
		case _0212:
		case _0216:
		case _0220:
		case _0224:
		case _0228:
			if (is_odd_channel(channel))
			{
				return 0x11;
			}
			else
			{
				return 0x19;
			}
		case _0300:
		case _0304:
		case _0308:
		case _0312:
		case _0316:
		case _0320:
		case _0324:
		case _0328:
		case _0400:
		case _0404:
		case _0408:
		case _0412:
		case _0416:
		case _0420:
		case _0424:
		case _0428:
			if (is_odd_channel(channel))
			{
				return 0x12;
			}
			else
			{
				return 0x1a;
			}
		case _0500:
		case _0504:
		case _0508:
		case _0512:
		case _0516:
		case _0520:
		case _0524:
		case _0528:
		case _0600:
		case _0604:
		case _0608:
		case _0612:
		case _0616:
		case _0620:
		case _0624:
		case _0628:
			if (is_odd_channel(channel))
			{
				return 0x15;
			}
			else
			{
				return 0x1d;
			}
		case _0700:
		case _0704:
		case _0708:
		case _0712:
		case _0716:
		case _0720:
		case _0724:
		case _0728:
		case _0800:
		case _0804:
		case _0808:
		case _0812:
		case _0816:
		case _0820:
		case _0824:
		case _0828:
			if (is_odd_channel(channel))
			{
				return 0x16;
			}
			else
			{
				return 0x1e;
			}
		case _0900:
		case _0904:
		case _0908:
		case _0912:
		case _0916:
		case _0920:
		case _0924:
		case _0928:
		case _1000:
		case _1004:
		case _1008:
		case _1012:
		case _1016:
		case _1020:
		case _1024:
		case _1028:
			if (is_odd_channel(channel))
			{
				return 0x17;
			}
			else
			{
				return 0x1f;
			}
		case _1100:
		case _1104:
		case _1108:
		case _1112:
		case _1116:
		case _1120:
		case _1124:
		case _1128:
			if (is_odd_channel(channel))
			{
				return 0x10;
			}
			else
			{
				return 0x18;
			}
		case _1200:
		case _1204:
		case _1208:
		case _1212:
		case _1216:
		case _1220:
		case _1224:
		case _1228:
		case _1300:
		case _1304:
		case _1308:
		case _1312:
		case _1316:
		case _1320:
		case _1324:
		case _1328:
			if (is_odd_channel(channel))
			{
				return 0x13;
			}
			else
			{
				return 0x1b;
			}
		case _1400:
		case _1404:
		case _1408:
		case _1412:
		case _1416:
		case _1420:
		case _1424:
		case _1428:
		case _1500:
		case _1504:
		case _1508:
		case _1512:
		case _1516:
		case _1520:
		case _1524:
		case _1528:
			if (is_odd_channel(channel))
			{
				return 0x14;
			}
			else
			{
				return 0x1c;
			}
		default:
			fatal(-1, "[SCC] Cannot get first byte for code");
	}
}

unsigned get_second_byte(const enum control_code code)
{
	switch (code)
	{
		// Mid-Row Codes
		case WH:
			return 0x20;
		case WHU:
			return 0x21;
		case I:
			return 0x2e;
		case IU:
			return 0x2f;
		// Miscellaneous Control Codes
		case RCL:
			return 0x20;
		case ENM:
			return 0x2e;
		case EOC:
			return 0x2f;
		case TO1:
			return 0x21;
		case TO2:
			return 0x22;
		case TO3:
			return 0x23;
		// Preamble Address Codes
		case _0100:
		case _0300:
		case _0500:
		case _0700:
		case _0900:
		case _1100:
		case _1200:
		case _1400:
			return 0x50;
		case _0200:
		case _0400:
		case _0600:
		case _0800:
		case _1000:
		case _1300:
		case _1500:
			return 0x70;
		case _0104:
		case _0304:
		case _0504:
		case _0704:
		case _0904:
		case _1104:
		case _1204:
		case _1404:
			return 0x52;
		case _0204:
		case _0404:
		case _0604:
		case _0804:
		case _1004:
		case _1304:
		case _1504:
			return 0x72;
		case _0108:
		case _0308:
		case _0508:
		case _0708:
		case _0908:
		case _1108:
		case _1208:
		case _1408:
			return 0x54;
		case _0208:
		case _0408:
		case _0608:
		case _0808:
		case _1008:
		case _1308:
		case _1508:
			return 0x74;
		case _0112:
		case _0312:
		case _0512:
		case _0712:
		case _0912:
		case _1112:
		case _1212:
		case _1412:
			return 0x56;
		case _0212:
		case _0412:
		case _0612:
		case _0812:
		case _1012:
		case _1312:
		case _1512:
			return 0x76;

		case _0116:
		case _0316:
		case _0516:
		case _0716:
		case _0916:
		case _1116:
		case _1216:
		case _1416:
			return 0x58;
		case _0216:
		case _0416:
		case _0616:
		case _0816:
		case _1016:
		case _1316:
		case _1516:
			return 0x78;
		case _0120:
		case _0320:
		case _0520:
		case _0720:
		case _0920:
		case _1120:
		case _1220:
		case _1420:
			return 0x5a;
		case _0220:
		case _0420:
		case _0620:
		case _0820:
		case _1020:
		case _1320:
		case _1520:
			return 0x7a;
		case _0124:
		case _0324:
		case _0524:
		case _0724:
		case _0924:
		case _1124:
		case _1224:
		case _1424:
			return 0x5c;
		case _0224:
		case _0424:
		case _0624:
		case _0824:
		case _1024:
		case _1324:
		case _1524:
			return 0x7c;
		case _0128:
		case _0328:
		case _0528:
		case _0728:
		case _0928:
		case _1128:
		case _1228:
		case _1428:
			return 0x5f;
		case _0228:
		case _0428:
		case _0628:
		case _0828:
		case _1028:
		case _1328:
		case _1528:
			return 0x7f;
		default:
			fatal(-1, "[SCC] Unknown control code");
	}
}

void add_padding(int fd, const char disassemble)
{

	if (disassemble)
	{
		write(fd, "_", 1);
	}
	else
	{
		write(fd, "80", 2); // 0x80 == odd_parity(0x00)
	}
}

void check_padding(const int fd, bool disassemble, unsigned int *bytes_written)
{
	if (*bytes_written % 2 == 1)
	{
		add_padding(fd, disassemble);
		++*bytes_written;
	}
}

void write_character(const int fd, const unsigned char character, const bool disassemble, unsigned int *bytes_written)
{
	if (disassemble)
	{
		write(fd, &character, 1);
	}
	else
	{
		if (*bytes_written % 2 == 1)
			write(fd, " ", 1);

		fdprintf(fd, "%02x", odd_parity(character));
	}
	++*bytes_written; // increment int pointed to by (unsigned int *) bytes_written
}

/**
 * @param bytes_written Number of control codes written, this doesn't
 *                      correspond to the actual number of bytes written on
 *                      disk. It is the same with the scc and ccd
 *                      (disassembly) format. It's purpose is to know if
 *                      padding should be added
 */
void write_control_code(const int fd, const unsigned char channel, const enum control_code code, const bool disassemble, unsigned int *bytes_written)
{
	check_padding(fd, disassemble, bytes_written);
	if (disassemble)
	{
		unsigned int length;
		const char *assembly_code = disassemble_code(code, &length);
		write(fd, assembly_code, length);
	}
	else
	{
		if (*bytes_written)
			write(fd, " ", 1);

		fdprintf(fd, "%02x%02x", odd_parity(get_first_byte(channel, code)), odd_parity(get_second_byte(code)));
	}
	*bytes_written += 2;
}

/**
 *
 * @param row 0-14 (inclusive)
 * @param column 0-31 (inclusive)
 *
 * //TODO: Preamble code need to take into account font as well
 *
 */
enum control_code get_preamble_code(const unsigned char row, const unsigned char column)
{
	switch (row)
	{
		case 0:
			switch (column / 4)
			{
				case 0:
					return _0100;
				case 1:
					return _0104;
				case 2:
					return _0108;
				case 3:
					return _0112;
				case 4:
					return _0116;
				case 5:
					return _0120;
				case 6:
					return _0124;
				case 7:
					return _0128;
			}
			break;
		case 1:
			switch (column / 4)
			{
				case 0:
					return _0200;
				case 1:
					return _0204;
				case 2:
					return _0208;
				case 3:
					return _0212;
				case 4:
					return _0216;
				case 5:
					return _0220;
				case 6:
					return _0224;
				case 7:
					return _0228;
			}
			break;
		case 2:
			switch (column / 4)
			{
				case 0:
					return _0300;
				case 1:
					return _0304;
				case 2:
					return _0308;
				case 3:
					return _0312;
				case 4:
					return _0316;
				case 5:
					return _0320;
				case 6:
					return _0324;
				case 7:
					return _0328;
			}
			break;
		case 3:
			switch (column / 4)
			{
				case 0:
					return _0400;
				case 1:
					return _0404;
				case 2:
					return _0408;
				case 3:
					return _0412;
				case 4:
					return _0416;
				case 5:
					return _0420;
				case 6:
					return _0424;
				case 7:
					return _0428;
			}
			break;
		case 4:
			switch (column / 4)
			{
				case 0:
					return _0500;
				case 1:
					return _0504;
				case 2:
					return _0508;
				case 3:
					return _0512;
				case 4:
					return _0516;
				case 5:
					return _0520;
				case 6:
					return _0524;
				case 7:
					return _0528;
			}
			break;
		case 5:
			switch (column / 4)
			{
				case 0:
					return _0600;
				case 1:
					return _0604;
				case 2:
					return _0608;
				case 3:
					return _0612;
				case 4:
					return _0616;
				case 5:
					return _0620;
				case 6:
					return _0624;
				case 7:
					return _0628;
			}
			break;
		case 6:
			switch (column / 4)
			{
				case 0:
					return _0700;
				case 1:
					return _0704;
				case 2:
					return _0708;
				case 3:
					return _0712;
				case 4:
					return _0716;
				case 5:
					return _0720;
				case 6:
					return _0724;
				case 7:
					return _0728;
			}
			break;
		case 7:
			switch (column / 4)
			{
				case 0:
					return _0800;
				case 1:
					return _0804;
				case 2:
					return _0808;
				case 3:
					return _0812;
				case 4:
					return _0816;
				case 5:
					return _0820;
				case 6:
					return _0824;
				case 7:
					return _0828;
			}
			break;
		case 8:
			switch (column / 4)
			{
				case 0:
					return _0900;
				case 1:
					return _0904;
				case 2:
					return _0908;
				case 3:
					return _0912;
				case 4:
					return _0916;
				case 5:
					return _0920;
				case 6:
					return _0924;
				case 7:
					return _0928;
			}
			break;
		case 9:
			switch (column / 4)
			{
				case 0:
					return _1000;
				case 1:
					return _1004;
				case 2:
					return _1008;
				case 3:
					return _1012;
				case 4:
					return _1016;
				case 5:
					return _1020;
				case 6:
					return _1024;
				case 7:
					return _1028;
			}
			break;
		case 10:
			switch (column / 4)
			{
				case 0:
					return _1100;
				case 1:
					return _1104;
				case 2:
					return _1108;
				case 3:
					return _1112;
				case 4:
					return _1116;
				case 5:
					return _1120;
				case 6:
					return _1124;
				case 7:
					return _1128;
			}
			break;
		case 11:
			switch (column / 4)
			{
				case 0:
					return _1200;
				case 1:
					return _1204;
				case 2:
					return _1208;
				case 3:
					return _1212;
				case 4:
					return _1216;
				case 5:
					return _1220;
				case 6:
					return _1224;
				case 7:
					return _1228;
			}
			break;
		case 12:
			switch (column / 4)
			{
				case 0:
					return _1300;
				case 1:
					return _1304;
				case 2:
					return _1308;
				case 3:
					return _1312;
				case 4:
					return _1316;
				case 5:
					return _1320;
				case 6:
					return _1324;
				case 7:
					return _1328;
			}
			break;
		case 13:
			switch (column / 4)
			{
				case 0:
					return _1400;
				case 1:
					return _1404;
				case 2:
					return _1408;
				case 3:
					return _1412;
				case 4:
					return _1416;
				case 5:
					return _1420;
				case 6:
					return _1424;
				case 7:
					return _1428;
			}
			break;
		case 14:
			switch (column / 4)
			{
				case 0:
					return _1500;
				case 1:
					return _1504;
				case 2:
					return _1508;
				case 3:
					return _1512;
				case 4:
					return _1516;
				case 5:
					return _1520;
				case 6:
					return _1524;
				case 7:
					return _1528;
			}
			break;
		default:
			fatal(-1, "Invalid row number.");

	}
}

enum control_code get_tab_offset_code(const unsigned char column)
{
	switch (column % 4)
	{
		case 0:
			return 0;
		case 1:
			return TO1;
		case 2:
			return TO2;
		case 3:
			return TO3;
		default:
			// unreachable
			exit(EXIT_FAILURE);
	}
}

enum control_code get_font_code(enum font_bits font) {
	switch (font)
	{
		case FONT_REGULAR:
			return WH;
		case FONT_UNDERLINED:
			return WHU;
		case FONT_ITALICS:
			return I;
		case FONT_UNDERLINED_ITALICS:
			return IU;
		default:
			fatal(-1, "Unknown font");
	}
}

void add_timestamp(int fd, LLONG time, const bool disassemble)
{
	write(fd, "\n\n", disassemble ? 1 : 2);
	unsigned hour, minute, second, frame;
	millis_to_time(time, &hour, &minute, &second, &frame);
	// Should be SMPTE format
	// This frame number seems like it couldn't be more wrong. Doesn't take
	// into account timebase
	fdprintf(fd, "%02d:%02d:%02d:%02.f\t", hour, minute, second, (float) frame / 30);
}

void clear_screen(int fd, LLONG end_time, const unsigned char channel, const bool disassemble)
{
	add_timestamp(fd, end_time, disassemble);
	unsigned int bytes_written = 0;
	write_control_code(fd, channel, RCL, disassemble, &bytes_written);
	write_control_code(fd, channel, EOC, disassemble, &bytes_written);
	write_control_code(fd, channel, ENM, disassemble, &bytes_written);
}

int write_cc_buffer_as_scenarist(const struct eia608_screen *data, struct encoder_ctx *context, const char disassemble)
{
	unsigned int bytes_written = 0;
	enum font_bits current_font = FONT_REGULAR;
	unsigned char current_row = 14;
	unsigned char current_column = 0;

	add_timestamp(context->out->fh, data->start_time, disassemble);
	write_control_code(context->out->fh, data->channel, RCL, disassemble, &bytes_written);
	for (uint8_t row = 0; row < 15; ++row)
	{
		// If there is nothing to display on this row, skip it.
		if (!data->row_used[row])
			continue;

		int first, last;
		find_limit_characters(data->characters[row], &first, &last, CCX_DECODER_608_SCREEN_WIDTH);

		for (unsigned char column = first; column <= last; ++column)
		{
			enum control_code position_code;
			enum control_code tab_offset_code;
			enum control_code font_code = 0;
			if (current_row != row || current_column != column || current_font != data->fonts[row][column])
			{
				if (current_font != data->fonts[row][column])
				{
					if (data->characters[row][column] == ' ')
					{
						// The MID-ROW code is going to move the cursor to the
						// right so if the next character is a space, skip it
						position_code = get_preamble_code(row, column);
						tab_offset_code = get_tab_offset_code(column++);
					}
					else
					{
						// Column - 1 because the preamble code is going to
						// move the cursor to the right
						position_code = get_preamble_code(row, column - 1);
						tab_offset_code = get_tab_offset_code(column - 1);
					}
					font_code = get_font_code(data->fonts[row][column]);
				}
				else
				{
					position_code = get_preamble_code(row, column);
					tab_offset_code = get_tab_offset_code(column);
				}
				current_row = row;
				current_column = column;
				write_control_code(context->out->fh, data->channel, position_code, disassemble, &bytes_written);
				if (tab_offset_code)
					write_control_code(context->out->fh, data->channel, tab_offset_code, disassemble, &bytes_written);
				if (current_font != data->fonts[row][column])
					write_control_code(context->out->fh, data->channel, font_code, disassemble, &bytes_written);
				current_font = data->fonts[row][column];
			}
			write_character(context->out->fh, data->characters[row][column], disassemble, &bytes_written);
			++current_column;
		}
		check_padding(context->out->fh, disassemble, &bytes_written);
	}
	clear_screen(context->out->fh, data->end_time, data->channel, disassemble);
	return 1;
}

int write_cc_buffer_as_ccd(const struct eia608_screen *data, struct encoder_ctx *context)
{
	if (!context->wrote_ccd_channel_header)
	{
		fdprintf(context->out->fh, "CHANNEL %d\n", data->channel);
		context->wrote_ccd_channel_header = true;
	}
	return write_cc_buffer_as_scenarist(data, context, true);
}


int write_cc_buffer_as_scc(const struct eia608_screen *data, struct encoder_ctx *context)
{
	return write_cc_buffer_as_scenarist(data, context, false);
}
