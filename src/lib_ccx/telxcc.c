/*!
(c) 2011-2013 Forers, s. r. o.: telxcc

telxcc conforms to ETSI 300 706 Presentation Level 1.5: Presentation Level 1 defines the basic Teletext page,
characterised by the use of spacing attributes only and a limited alphanumeric and mosaics repertoire.
Presentation Level 1.5 decoder responds as Level 1 but the character repertoire is extended via packets X/26.
Selection of national option sub-sets related features from Presentation Level 2.5 feature set have been implemented, too.
(X/28/0 Format 1, X/28/4, M/29/0 and M/29/4 packets)

Further documentation:
ETSI TS 101 154 V1.9.1 (2009-09), Technical Specification
  Digital Video Broadcasting (DVB); Specification for the use of Video and Audio Coding in Broadcasting Applications based on the MPEG-2 Transport Stream
ETSI EN 300 231 V1.3.1 (2003-04), European Standard (Telecommunications series)
  Television systems; Specification of the domestic video Programme Delivery Control system (PDC)
ETSI EN 300 472 V1.3.1 (2003-05), European Standard (Telecommunications series)
  Digital Video Broadcasting (DVB); Specification for conveying ITU-R System B Teletext in DVB bitstreams
ETSI EN 301 775 V1.2.1 (2003-05), European Standard (Telecommunications series)
  Digital Video Broadcasting (DVB); Specification for the carriage of Vertical Blanking Information (VBI) data in DVB bitstreams
ETS 300 706 (May 1997)
  Enhanced Teletext Specification
ETS 300 708 (March 1997)
  Television systems; Data transmission within Teletext
ISO/IEC STANDARD 13818-1 Second edition (2000-12-01)
  Information technology — Generic coding of moving pictures and associated audio information: Systems
ISO/IEC STANDARD 6937 Third edition (2001-12-15)
  Information technology — Coded graphic character set for text communication — Latin alphabet
Werner Brückner -- Teletext in digital television
*/

#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "hamming.h"
#include "teletext.h"
#include <signal.h>
#include "activity.h"
#include "ccx_encoders_helpers.h"

#ifdef __MINGW32__
// switch stdin and all normal files into binary mode -- needed for Windows
#include <fcntl.h>
int _CRT_fmode = _O_BINARY;

// for better UX in Windows we want to detect that app is not run by "double-clicking" in Explorer
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0502
#define _WIN32_IE 0x0400
#include <windows.h>
#include <commctrl.h>
#endif

uint64_t last_pes_pts = 0; // PTS of last PES packet (debug purposes)
static int de_ctr = 0;	   // a keeps count of packets with flag subtitle ON and data packets
static const char *TTXT_COLOURS[8] = {
    // black,   red,       green,     yellow,    blue,      magenta,   cyan,      white
    "#000000", "#ff0000", "#00ff00", "#ffff00", "#0000ff", "#ff00ff", "#00ffff", "#ffffff"};

// 1-byte alignment; just to be sure, this struct is being used for explicit type conversion
// FIXME: remove explicit type conversion from buffer to structs
#pragma pack(push)
#pragma pack(1)
typedef struct
{
	uint8_t _clock_in;     // clock run in
	uint8_t _framing_code; // framing code, not needed, ETSI 300 706: const 0xe4
	uint8_t address[2];
	uint8_t data[40];
} teletext_packet_payload_t;
#pragma pack(pop)

// application config global variable
struct ccx_s_teletext_config tlt_config = {0};

// macro -- output only when increased verbosity was turned on
#define VERBOSE_ONLY if (tlt_config.verbose == YES)

// current charset (charset can be -- and always is -- changed during transmission)
struct s_primary_charset
{
	uint8_t current;
	uint8_t g0_m29;
	uint8_t g0_x28;
} primary_charset = {
    0x00, UNDEFINED, UNDEFINED};

// entities, used in colour mode, to replace unsafe HTML tag chars
struct
{
	uint16_t character;
	const char *entity;
} const ENTITIES[] = {
    {'<', "&lt;"},
    {'>', "&gt;"},
    {'&', "&amp;"}};

// Latin-Russian characters mapping, issue #1086
struct
{
	uint16_t lat_char;
	const char *rus_char;
} const LAT_RUS[] = {
    {65, "А"}, {66, "Б"}, {87, "В"}, {71, "Г"}, {68, "Д"}, {69, "Е"}, {86, "Ж"}, {90, "З"}, {73, "И"}, {74, "Й"}, {75, "К"}, {76, "Л"}, {77, "М"}, {78, "Н"}, {79, "О"}, {80, "П"}, {82, "Р"}, {83, "С"}, {84, "Т"}, {85, "У"}, {70, "Ф"}, {72, "Х"}, {67, "Ц"}, {238, "Ч"}, {235, "Ш"}, {249, "Щ"}, {35, "Ы"}, {88, "Ь"}, {234, "Э"}, {224, "Ю"}, {81, "Я"}, {97, "а"}, {98, "б"}, {119, "в"}, {103, "г"}, {100, "д"}, {101, "е"}, {118, "ж"}, {122, "з"}, {105, "и"}, {106, "й"}, {107, "к"}, {108, "л"}, {109, "м"}, {110, "н"}, {111, "о"}, {112, "п"}, {114, "р"}, {115, "с"}, {116, "т"}, {117, "у"}, {102, "ф"}, {104, "х"}, {99, "ц"}, {231, "ч"}, {226, "ш"}, {251, "щ"}, {121, "ъ"}, {38, "ы"}, {120, "ь"}, {244, "э"}, {232, "ю"}, {113, "я"}};

#define array_length(a) (sizeof(a) / sizeof(a[0]))

// extracts magazine number from teletext page
#define MAGAZINE(p) ((p >> 8) & 0xf)

// extracts page number from teletext page
#define PAGE(p) (p & 0xff)

typedef enum
{
	LATIN = 0,
	CYRILLIC1,
	CYRILLIC2,
	CYRILLIC3,
	GREEK,
	ARABIC,
	HEBREW
} g0_charsets_type;

g0_charsets_type default_g0_charset;

// Note: All characters are encoded in UCS-2

// --- G0 ----------------------------------------------------------------------

// G0 charsets
uint16_t G0[5][96] = {
    {// Latin G0 Primary Set
     0x0020, 0x0021, 0x0022, 0x00a3, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
     0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
     0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
     0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005a, 0x00ab, 0x00bd, 0x00bb, 0x005e, 0x0023,
     0x002d, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
     0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 0x0078, 0x0079, 0x007a, 0x00bc, 0x00a6, 0x00be, 0x00f7, 0x007f},
    {// Cyrillic G0 Primary Set - Option 1 - Serbian/Croatian
     0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x044b, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
     0x0030, 0x0031, 0x3200, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
     0x0427, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413, 0x0425, 0x0418, 0x0408, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e,
     0x041f, 0x040c, 0x0420, 0x0421, 0x0422, 0x0423, 0x0412, 0x0403, 0x0409, 0x040a, 0x0417, 0x040b, 0x0416, 0x0402, 0x0428, 0x040f,
     0x0447, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433, 0x0445, 0x0438, 0x0428, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e,
     0x043f, 0x042c, 0x0440, 0x0441, 0x0442, 0x0443, 0x0432, 0x0423, 0x0429, 0x042a, 0x0437, 0x042b, 0x0436, 0x0422, 0x0448, 0x042f},
    {// Cyrillic G0 Primary Set - Option 2 - Russian/Bulgarian
     0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x044b, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
     0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
     0x042e, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413, 0x0425, 0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e,
     0x041f, 0x042f, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412, 0x042c, 0x042a, 0x0417, 0x0428, 0x042d, 0x0429, 0x0427, 0x042b,
     0x044e, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433, 0x0445, 0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e,
     0x043f, 0x044f, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432, 0x044c, 0x044a, 0x0437, 0x0448, 0x044d, 0x0449, 0x0447, 0x044b},
    {// Cyrillic G0 Primary Set - Option 3 - Ukrainian
     0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x00ef, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
     0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
     0x042e, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413, 0x0425, 0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e,
     0x041f, 0x042f, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412, 0x042c, 0x0049, 0x0417, 0x0428, 0x042d, 0x0429, 0x0427, 0x00cf,
     0x044e, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433, 0x0445, 0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e,
     0x043f, 0x044f, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432, 0x044c, 0x0069, 0x0437, 0x0448, 0x044d, 0x0449, 0x0447, 0x00ff},
    {// Greek G0 Primary Set
     0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
     0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
     0x0390, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, 0x0398, 0x0399, 0x039a, 0x039b, 0x039c, 0x039d, 0x039e, 0x039f,
     0x03a0, 0x03a1, 0x03a2, 0x03a3, 0x03a4, 0x03a5, 0x03a6, 0x03a7, 0x03a8, 0x03a9, 0x03aa, 0x03ab, 0x03ac, 0x03ad, 0x03ae, 0x03af,
     0x03b0, 0x03b1, 0x03b2, 0x03b3, 0x03b4, 0x03b5, 0x03b6, 0x03b7, 0x03b8, 0x03b9, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf,
     0x03c0, 0x03c1, 0x03c2, 0x03c3, 0x03c4, 0x03c5, 0x03c6, 0x03c7, 0x03c8, 0x03c9, 0x03ca, 0x03cb, 0x03cc, 0x03cd, 0x03ce, 0x03cf}
    //{ // Arabic G0 Primary Set
    //},
    //{ // Hebrew G0 Primary Set
    //}
};

// array positions where chars from G0_LATIN_NATIONAL_SUBSETS are injected into G0[LATIN]
const uint8_t G0_LATIN_NATIONAL_SUBSETS_POSITIONS[13] = {
    0x03, 0x04, 0x20, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x5b, 0x5c, 0x5d, 0x5e};

// ETS 300 706, chapter 15.2, table 32: Function of Default G0 and G2 Character Set Designation
// and National Option Selection bits in packets X/28/0 Format 1, X/28/4, M/29/0 and M/29/4

// Latin National Option Sub-sets
struct
{
	const char *language;
	uint16_t characters[13];
} const G0_LATIN_NATIONAL_SUBSETS[14] = {
    {// 0
     "English",
     {0x00a3, 0x0024, 0x0040, 0x00ab, 0x00bd, 0x00bb, 0x005e, 0x0023, 0x002d, 0x00bc, 0x00a6, 0x00be, 0x00f7}},
    {// 1
     "French",
     {0x00e9, 0x00ef, 0x00e0, 0x00eb, 0x00ea, 0x00f9, 0x00ee, 0x0023, 0x00e8, 0x00e2, 0x00f4, 0x00fb, 0x00e7}},
    {// 2
     "Swedish, Finnish, Hungarian",
     {0x0023, 0x00a4, 0x00c9, 0x00c4, 0x00d6, 0x00c5, 0x00dc, 0x005f, 0x00e9, 0x00e4, 0x00f6, 0x00e5, 0x00fc}},
    {// 3
     "Czech, Slovak",
     {0x0023, 0x016f, 0x010d, 0x0165, 0x017e, 0x00fd, 0x00ed, 0x0159, 0x00e9, 0x00e1, 0x011b, 0x00fa, 0x0161}},
    {// 4
     "German",
     {0x0023, 0x0024, 0x00a7, 0x00c4, 0x00d6, 0x00dc, 0x005e, 0x005f, 0x00b0, 0x00e4, 0x00f6, 0x00fc, 0x00df}},
    {// 5
     "Portuguese, Spanish",
     {0x00e7, 0x0024, 0x00a1, 0x00e1, 0x00e9, 0x00ed, 0x00f3, 0x00fa, 0x00bf, 0x00fc, 0x00f1, 0x00e8, 0x00e0}},
    {// 6
     "Italian",
     {0x00a3, 0x0024, 0x00e9, 0x00b0, 0x00e7, 0x00bb, 0x005e, 0x0023, 0x00f9, 0x00e0, 0x00f2, 0x00e8, 0x00ec}},
    {// 7
     "Rumanian",
     {0x0023, 0x00a4, 0x0162, 0x00c2, 0x015e, 0x0102, 0x00ce, 0x0131, 0x0163, 0x00e2, 0x015f, 0x0103, 0x00ee}},
    {// 8
     "Polish",
     {0x0023, 0x0144, 0x0105, 0x017b, 0x015a, 0x0141, 0x0107, 0x00f3, 0x0119, 0x017c, 0x015b, 0x0142, 0x017a}},
    {// 9
     "Turkish",
     {0x0054, 0x011f, 0x0130, 0x015e, 0x00d6, 0x00c7, 0x00dc, 0x011e, 0x0131, 0x015f, 0x00f6, 0x00e7, 0x00fc}},
    {// a
     "Serbian, Croatian, Slovenian",
     {0x0023, 0x00cb, 0x010c, 0x0106, 0x017d, 0x0110, 0x0160, 0x00eb, 0x010d, 0x0107, 0x017e, 0x0111, 0x0161}},
    {// b
     "Estonian",
     {0x0023, 0x00f5, 0x0160, 0x00c4, 0x00d6, 0x017e, 0x00dc, 0x00d5, 0x0161, 0x00e4, 0x00f6, 0x017e, 0x00fc}},
    {// c
     "Lettish, Lithuanian",
     {0x0023, 0x0024, 0x0160, 0x0117, 0x0119, 0x017d, 0x010d, 0x016b, 0x0161, 0x0105, 0x0173, 0x017e, 0x012f}}};

// References to the G0_LATIN_NATIONAL_SUBSETS array
const uint8_t G0_LATIN_NATIONAL_SUBSETS_MAP[56] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x01, 0x02, 0x03, 0x04, 0xff, 0x06, 0xff,
    0x00, 0x01, 0x02, 0x09, 0x04, 0x05, 0x06, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x0a, 0xff, 0x07,
    0xff, 0xff, 0x0b, 0x03, 0x04, 0xff, 0x0c, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0x09, 0xff, 0xff, 0xff, 0xff};

// --- G2 ----------------------------------------------------------------------

const uint16_t G2[1][96] = {
    {// Latin G2 Supplementary Set
     0x0020, 0x00a1, 0x00a2, 0x00a3, 0x0024, 0x00a5, 0x0023, 0x00a7, 0x00a4, 0x2018, 0x201c, 0x00ab, 0x2190, 0x2191, 0x2192, 0x2193,
     0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00d7, 0x00b5, 0x00b6, 0x00b7, 0x00f7, 0x2019, 0x201d, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
     0x0020, 0x0300, 0x0301, 0x0302, 0x0303, 0x0304, 0x0306, 0x0307, 0x0308, 0x0000, 0x030a, 0x0327, 0x005f, 0x030b, 0x0328, 0x030c,
     0x2015, 0x00b9, 0x00ae, 0x00a9, 0x2122, 0x266a, 0x20ac, 0x2030, 0x03B1, 0x0000, 0x0000, 0x0000, 0x215b, 0x215c, 0x215d, 0x215e,
     0x03a9, 0x00c6, 0x0110, 0x00aa, 0x0126, 0x0000, 0x0132, 0x013f, 0x0141, 0x00d8, 0x0152, 0x00ba, 0x00de, 0x0166, 0x014a, 0x0149,
     0x0138, 0x00e6, 0x0111, 0x00f0, 0x0127, 0x0131, 0x0133, 0x0140, 0x0142, 0x00f8, 0x0153, 0x00df, 0x00fe, 0x0167, 0x014b, 0x0020}
    //	{ // Cyrillic G2 Supplementary Set
    //	},
    //	{ // Greek G2 Supplementary Set
    //	},
    //	{ // Arabic G2 Supplementary Set
    //	}
};

const uint16_t G2_ACCENTS[15][52] = {
    // A B C D E F G H I J K L M N O P Q R S T U V W X Y Z a b c d e f g h i j k l m n o p q r s t u v w x y z
    {// grave
     0x00c0, 0x0000, 0x0000, 0x0000, 0x00c8, 0x0000, 0x0000, 0x0000, 0x00cc, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00d2, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x00d9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00e0, 0x0000, 0x0000, 0x0000, 0x00e8, 0x0000,
     0x0000, 0x0000, 0x00ec, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00f2, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00f9, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000},
    {// acute
     0x00c1, 0x0000, 0x0106, 0x0000, 0x00c9, 0x0000, 0x0000, 0x0000, 0x00cd, 0x0000, 0x0000, 0x0139, 0x0000, 0x0143, 0x00d3, 0x0000,
     0x0000, 0x0154, 0x015a, 0x0000, 0x00da, 0x0000, 0x0000, 0x0000, 0x00dd, 0x0179, 0x00e1, 0x0000, 0x0107, 0x0000, 0x00e9, 0x0000,
     0x0123, 0x0000, 0x00ed, 0x0000, 0x0000, 0x013a, 0x0000, 0x0144, 0x00f3, 0x0000, 0x0000, 0x0155, 0x015b, 0x0000, 0x00fa, 0x0000,
     0x0000, 0x0000, 0x00fd, 0x017a},
    {// circumflex
     0x00c2, 0x0000, 0x0108, 0x0000, 0x00ca, 0x0000, 0x011c, 0x0124, 0x00ce, 0x0134, 0x0000, 0x0000, 0x0000, 0x0000, 0x00d4, 0x0000,
     0x0000, 0x0000, 0x015c, 0x0000, 0x00db, 0x0000, 0x0174, 0x0000, 0x0176, 0x0000, 0x00e2, 0x0000, 0x0109, 0x0000, 0x00ea, 0x0000,
     0x011d, 0x0125, 0x00ee, 0x0135, 0x0000, 0x0000, 0x0000, 0x0000, 0x00f4, 0x0000, 0x0000, 0x0000, 0x015d, 0x0000, 0x00fb, 0x0000,
     0x0175, 0x0000, 0x0177, 0x0000},
    {// tilde
     0x00c3, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0128, 0x0000, 0x0000, 0x0000, 0x0000, 0x00d1, 0x00d5, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0168, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00e3, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0129, 0x0000, 0x0000, 0x0000, 0x0000, 0x00f1, 0x00f5, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0169, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000},
    {// macron
     0x0100, 0x0000, 0x0000, 0x0000, 0x0112, 0x0000, 0x0000, 0x0000, 0x012a, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x014c, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x016a, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0101, 0x0000, 0x0000, 0x0000, 0x0113, 0x0000,
     0x0000, 0x0000, 0x012b, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x014d, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x016b, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000},
    {// breve
     0x0102, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x011e, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x016c, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0103, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x011f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x016d, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000},
    {// dot
     0x0000, 0x0000, 0x010a, 0x0000, 0x0116, 0x0000, 0x0120, 0x0000, 0x0130, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x017b, 0x0000, 0x0000, 0x010b, 0x0000, 0x0117, 0x0000,
     0x0121, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x017c},
    {// umlaut
     0x00c4, 0x0000, 0x0000, 0x0000, 0x00cb, 0x0000, 0x0000, 0x0000, 0x00cf, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00d6, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x00dc, 0x0000, 0x0000, 0x0000, 0x0178, 0x0000, 0x00e4, 0x0000, 0x0000, 0x0000, 0x00eb, 0x0000,
     0x0000, 0x0000, 0x00ef, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00f6, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00fc, 0x0000,
     0x0000, 0x0000, 0x00ff, 0x0000},
    {0},
    {// ring
     0x00c5, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x016e, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00e5, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x016f, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000},
    {// cedilla
     0x0000, 0x0000, 0x00c7, 0x0000, 0x0000, 0x0000, 0x0122, 0x0000, 0x0000, 0x0000, 0x0136, 0x013b, 0x0000, 0x0145, 0x0000, 0x0000,
     0x0000, 0x0156, 0x015e, 0x0162, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00e7, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0137, 0x013c, 0x0000, 0x0146, 0x0000, 0x0000, 0x0000, 0x0157, 0x015f, 0x0163, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000},
    {0},
    {// double acute
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0150, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0170, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0151, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0171, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000},
    {// ogonek
     0x0104, 0x0000, 0x0000, 0x0000, 0x0118, 0x0000, 0x0000, 0x0000, 0x012e, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0172, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0105, 0x0000, 0x0000, 0x0000, 0x0119, 0x0000,
     0x0000, 0x0000, 0x012f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0173, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000},
    {// caron
     0x0000, 0x0000, 0x010c, 0x010e, 0x011a, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x013d, 0x0000, 0x0147, 0x0000, 0x0000,
     0x0000, 0x0158, 0x0160, 0x0164, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x017d, 0x0000, 0x0000, 0x010d, 0x010f, 0x011b, 0x0000,
     0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x013e, 0x0000, 0x0148, 0x0000, 0x0000, 0x0000, 0x0159, 0x0161, 0x0165, 0x0000, 0x0000,
     0x0000, 0x0000, 0x0000, 0x017e}};
void page_buffer_add_string(struct TeletextCtx *ctx, const char *s)
{
	if (ctx->page_buffer_cur_size < (ctx->page_buffer_cur_used + strlen(s) + 1))
	{
		int add = strlen(s) + 4096; // So we don't need to realloc often
		ctx->page_buffer_cur_size = ctx->page_buffer_cur_size + add;
		ctx->page_buffer_cur = (char *)realloc(ctx->page_buffer_cur, ctx->page_buffer_cur_size);
		if (!ctx->page_buffer_cur)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory to process teletext page.\n");
	}
	memcpy(ctx->page_buffer_cur + ctx->page_buffer_cur_used, s, strlen(s));
	ctx->page_buffer_cur_used += strlen(s);
	ctx->page_buffer_cur[ctx->page_buffer_cur_used] = 0;
}

void ucs2_buffer_add_char(struct TeletextCtx *ctx, uint64_t c)
{
	if (ctx->ucs2_buffer_cur_size < (ctx->ucs2_buffer_cur_used + 2))
	{
		int add = 4096; // So we don't need to realloc often
		ctx->ucs2_buffer_cur_size = ctx->ucs2_buffer_cur_size + add;
		ctx->ucs2_buffer_cur = (uint64_t *)realloc(ctx->ucs2_buffer_cur, ctx->ucs2_buffer_cur_size * sizeof(uint64_t));
		if (!ctx->ucs2_buffer_cur)
			fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory to process teletext page.\n");
	}
	ctx->ucs2_buffer_cur[ctx->ucs2_buffer_cur_used++] = c;
	ctx->ucs2_buffer_cur[ctx->ucs2_buffer_cur_used] = 0;
}

void page_buffer_add_char(struct TeletextCtx *ctx, char c)
{
	char t[2];
	t[0] = c;
	t[1] = 0;
	page_buffer_add_string(ctx, t);
}

// ETS 300 706, chapter 8.2
uint8_t unham_8_4(uint8_t a)
{
	uint8_t r = UNHAM_8_4[a];
	if (r == 0xff)
	{
		dbg_print(CCX_DMT_TELETEXT, "- Unrecoverable data error; UNHAM8/4(%02x)\n", a);
	}
	return (r & 0x0f);
}

// ETS 300 706, chapter 8.3
uint32_t unham_24_18(uint32_t a)
{
	uint8_t test = 0;

	// Tests A-F correspond to bits 0-6 respectively in 'test'.
	for (uint8_t i = 0; i < 23; i++)
		test ^= ((a >> i) & 0x01) * (i + 33);
	// Only parity bit is tested for bit 24
	test ^= ((a >> 23) & 0x01) * 32;

	if ((test & 0x1f) != 0x1f)
	{
		// Not all tests A-E correct
		if ((test & 0x20) == 0x20)
		{
			// F correct: Double error
			return 0xffffffff;
		}
		// Test F incorrect: Single error
		a ^= 1 << (30 - test);
	}

	return (a & 0x000004) >> 2 | (a & 0x000070) >> 3 | (a & 0x007f00) >> 4 | (a & 0x7f0000) >> 5;
}

// Default G0 Character Set
void set_g0_charset(uint32_t triplet)
{
	// ETS 300 706, Table 32
	if ((triplet & 0x3c00) == 0x1000)
	{
		if ((triplet & 0x0380) == 0x0000)
			default_g0_charset = CYRILLIC1;
		else if ((triplet & 0x0380) == 0x0200)
			default_g0_charset = CYRILLIC2;
		else if ((triplet & 0x0380) == 0x0280)
			default_g0_charset = CYRILLIC3;
		else
			default_g0_charset = LATIN;
	}
	else
		default_g0_charset = LATIN;
}

// Latin National Subset Selection
void remap_g0_charset(uint8_t c)
{
	if (c != primary_charset.current)
	{
		uint8_t m = G0_LATIN_NATIONAL_SUBSETS_MAP[c];
		if (m == 0xff)
		{
			fprintf(stderr, "- G0 Latin National Subset ID 0x%1x.%1x is not implemented\n", (c >> 3), (c & 0x7));
		}
		else
		{
			for (uint8_t j = 0; j < 13; j++)
				G0[LATIN][G0_LATIN_NATIONAL_SUBSETS_POSITIONS[j]] = G0_LATIN_NATIONAL_SUBSETS[m].characters[j];
			VERBOSE_ONLY fprintf(stderr, "- Using G0 Latin National Subset ID 0x%1x.%1x (%s)\n", (c >> 3), (c & 0x7), G0_LATIN_NATIONAL_SUBSETS[m].language);
			primary_charset.current = c;
		}
	}
}

// wide char (16 bits) to utf-8 conversion
void ucs2_to_utf8(char *r, uint16_t ch)
{
	if (ch < 0x80)
	{
		r[0] = ch & 0x7f;
		r[1] = 0;
		r[2] = 0;
	}
	else if (ch < 0x800)
	{
		r[0] = (ch >> 6) | 0xc0;
		r[1] = (ch & 0x3f) | 0x80;
		r[2] = 0;
	}
	else
	{
		r[0] = (ch >> 12) | 0xe0;
		r[1] = ((ch >> 6) & 0x3f) | 0x80;
		r[2] = (ch & 0x3f) | 0x80;
	}
	r[3] = 0;
}

// check parity and translate any reasonable teletext character into ucs2
uint16_t telx_to_ucs2(uint8_t c)
{
	if (PARITY_8[c] == 0)
	{
		dbg_print(CCX_DMT_TELETEXT, "- Unrecoverable data error; PARITY(%02x)\n", c);
		return 0x20;
	}

	uint16_t r = c & 0x7f;
	if (r >= 0x20)
		r = G0[default_g0_charset][r - 0x20];
	return r;
}

uint16_t bcd_page_to_int(uint16_t bcd)
{
	return ((bcd & 0xf00) >> 8) * 100 + ((bcd & 0xf0) >> 4) * 10 + (bcd & 0xf);
}

void telx_case_fix(struct TeletextCtx *context)
{
	if (context->page_buffer_cur == NULL)
		return;

	// Capitalizing first letter of every sentence
	int line_len = strlen(context->page_buffer_cur);
	for (int i = 0; i < line_len; i++)
	{
		switch (context->page_buffer_cur[i])
		{
			case ' ':
				// case 0x89: // This is a transparent space
			case '-':
				break;
			case '.': // Fallthrough
			case '?': // Fallthrough
			case '!':
			case ':':
				context->new_sentence = 1;
				break;
			default:
				if (context->new_sentence && i != 0 && context->page_buffer_cur[i - 1] != '\n')
					context->page_buffer_cur[i] = cctoupper(context->page_buffer_cur[i]);

				else if (!context->new_sentence && i != 0 && context->page_buffer_cur[i - 1] != '\n')
					context->page_buffer_cur[i] = cctolower(context->page_buffer_cur[i]);

				context->new_sentence = 0;
				break;
		}
	}
	telx_correct_case(context->page_buffer_cur);
}

void telxcc_dump_prev_page(struct TeletextCtx *ctx, struct cc_subtitle *sub)
{
	char info[4];
	if (!ctx->page_buffer_prev)
		return;

	snprintf(info, 4, "%.3u", bcd_page_to_int(tlt_config.page));
	add_cc_sub_text(sub, ctx->page_buffer_prev, ctx->prev_show_timestamp,
			ctx->prev_hide_timestamp, info, "TLT", CCX_ENC_UTF_8);

	if (ctx->page_buffer_prev)
		free(ctx->page_buffer_prev);
	if (ctx->ucs2_buffer_prev)
		free(ctx->ucs2_buffer_prev);
	// Switch "dump" buffers
	ctx->page_buffer_prev_used = ctx->page_buffer_cur_used;
	ctx->page_buffer_prev_size = ctx->page_buffer_cur_size;
	ctx->page_buffer_prev = ctx->page_buffer_cur;
	ctx->page_buffer_cur_size = 0;
	ctx->page_buffer_cur_used = 0;
	ctx->page_buffer_cur = NULL;
	// Also switch compare buffers
	ctx->ucs2_buffer_prev_used = ctx->ucs2_buffer_cur_used;
	ctx->ucs2_buffer_prev_size = ctx->ucs2_buffer_cur_size;
	ctx->ucs2_buffer_prev = ctx->ucs2_buffer_cur;
	ctx->ucs2_buffer_cur_size = 0;
	ctx->ucs2_buffer_cur_used = 0;
	ctx->ucs2_buffer_cur = NULL;
}

// Note: c1 and c2 are just used for debug output, not for the actual comparison
int fuzzy_memcmp(const char *c1, const char *c2, const uint64_t *ucs2_buf1, unsigned ucs2_buf1_len,
		 const uint64_t *ucs2_buf2, unsigned ucs2_buf2_len)
{
	size_t l;
	size_t short_len = ucs2_buf1_len < ucs2_buf2_len ? ucs2_buf1_len : ucs2_buf2_len;
	size_t max = (short_len * tlt_config.levdistmaxpct) / 100;
	unsigned upto = (ucs2_buf1_len < ucs2_buf2_len) ? ucs2_buf1_len : ucs2_buf2_len;
	if (max < tlt_config.levdistmincnt)
		max = tlt_config.levdistmincnt;

	// For the second string, only take the first chars (up to the first string length, that's upto).
	l = (size_t)levenshtein_dist(ucs2_buf1, ucs2_buf2, ucs2_buf1_len, upto);
	int res = (l > max);
	dbg_print(CCX_DMT_LEVENSHTEIN, "\rLEV | %s | %s | Max: %d | Calc: %d | Match: %d\n", c1, c2, max, l, !res);
	return res;
}

void process_page(struct TeletextCtx *ctx, teletext_page_t *page, struct cc_subtitle *sub)
{
	if ((tlt_config.extraction_start.set && page->hide_timestamp < tlt_config.extraction_start.time_in_ms) ||
	    (tlt_config.extraction_end.set && page->show_timestamp > tlt_config.extraction_end.time_in_ms) ||
	    page->hide_timestamp == 0)
	{
		return;
	}
#ifdef DEBUG
	for (uint8_t row = 1; row < 25; row++)
	{
		fprintf(stdout, "DEBUG[%02u]: ", row);
		for (uint8_t col = 0; col < 40; col++)
			fprintf(stdout, "%3x ", page->text[row][col]);
		fprintf(stdout, "\n");
	}
	fprintf(stdout, "\n");
#endif
	char u[4] = {0, 0, 0, 0};

	// optimization: slicing column by column -- higher probability we could find boxed area start mark sooner
	uint8_t page_is_empty = YES;
	for (uint8_t col = 0; col < 40; col++)
	{
		for (uint8_t row = 1; row < 25; row++)
		{
			if (page->text[row][col] == 0x0b)
			{
				page_is_empty = NO;
				goto page_is_empty;
			}
		}
	}
page_is_empty:
	if (page_is_empty == YES)
		return;

	if (page->show_timestamp > page->hide_timestamp)
		page->hide_timestamp = page->show_timestamp;

	char timecode_show[24] = {0}, timecode_hide[24] = {0};

	int time_reported = 0;
	char c_tempb[256]; // For buffering
	uint8_t line_count = 0;

	timestamp_to_srttime(page->show_timestamp, timecode_show);
	timecode_show[12] = 0;
	timestamp_to_srttime(page->hide_timestamp, timecode_hide);
	timecode_hide[12] = 0;

	// process data
	for (uint8_t row = 1; row < 25; row++)
	{
		// anchors for string trimming purpose
		uint8_t col_start = 40;
		uint8_t col_stop = 40;

		uint8_t box_open = NO;
		for (int8_t col = 0; col < 40; col++)
		{
			// replace all 0/B and 0/A characters with 0/20, as specified in ETS 300 706:
			// Unless operating in "Hold Mosaics" mode, each character space occupied by a
			// spacing attribute is displayed as a SPACE
			if (page->text[row][col] == 0xb) // open the box
			{
				if (col_start == 40)
				{
					col_start = col;
					line_count++;
				}
				else
				{
					page->text[row][col] = 0x20;
				}
				box_open = YES;
			}
			else if (page->text[row][col] == 0xa) // close the box
			{
				page->text[row][col] = 0x20;
				box_open = NO;
			}
			// characters between 0xA and 0xB shouldn't be displayed
			// page->text[row][col] > 0x20 added to preserve color information
			else if (!box_open && col_start < 40 && page->text[row][col] > 0x20)
			{
				page->text[row][col] = 0x20;
			}
		}
		// line is empty
		if (col_start > 39)
			continue;

		for (uint8_t col = col_start + 1; col <= 39; col++)
		{
			if (page->text[row][col] > 0x20)
			{
				if (col_stop > 39)
					col_start = col;
				col_stop = col;
			}
		}
		// line is empty
		if (col_stop > 39)
			continue;

		// ETS 300 706, chapter 12.2: Alpha White ("Set-After") - Start-of-row default condition.
		// used for colour changes _before_ start box mark
		// white is default as stated in ETS 300 706, chapter 12.2
		// black(0), red(1), green(2), yellow(3), blue(4), magenta(5), cyan(6), white(7)
		uint8_t foreground_color = 0x7;
		uint8_t font_tag_opened = NO;

		if (line_count > 1)
		{
			switch (tlt_config.write_format)
			{
				case CCX_OF_TRANSCRIPT:
					page_buffer_add_string(ctx, " ");
					break;
				case CCX_OF_SMPTETT:
					page_buffer_add_string(ctx, "<br/>");
					break;
				default:
					page_buffer_add_string(ctx, "\r\n");
			}
		}

		if (tlt_config.gui_mode_reports)
		{
			if (!time_reported)
			{
				char timecode_show_mmss[6], timecode_hide_mmss[6];
				memcpy(timecode_show_mmss, timecode_show + 3, 5);
				memcpy(timecode_hide_mmss, timecode_hide + 3, 5);
				timecode_show_mmss[5] = 0;
				timecode_hide_mmss[5] = 0;
				// Note, only MM:SS here as we need to save space in the preview window
				fprintf(stderr, "###TIME###%s-%s\n###SUBTITLES###", timecode_show_mmss, timecode_hide_mmss);
				time_reported = 1;
			}
			else
				fprintf(stderr, "###SUBTITLE###");
		}

		for (uint8_t col = 0; col <= col_stop; col++)
		{
			// v is just a shortcut
			uint16_t v = page->text[row][col];

			if (col < col_start)
			{
				if (v <= 0x7)
					foreground_color = (uint8_t)v;
			}

			if (col == col_start)
			{
				if ((foreground_color != 0x7) && !tlt_config.nofontcolor)
				{
					sprintf(c_tempb, "<font color=\"%s\">", TTXT_COLOURS[foreground_color]);
					page_buffer_add_string(ctx, c_tempb);
					font_tag_opened = YES;
				}
			}

			if (col >= col_start)
			{
				if (v <= 0x7)
				{
					// ETS 300 706, chapter 12.2: Unless operating in "Hold Mosaics" mode,
					// each character space occupied by a spacing attribute is displayed as a SPACE.
					if (!tlt_config.nofontcolor)
					{
						if (font_tag_opened == YES)
						{
							page_buffer_add_string(ctx, "</font>");
							font_tag_opened = NO;
						}

						page_buffer_add_string(ctx, " ");
						// black is considered as white for telxcc purpose
						// telxcc writes <font/> tags only when needed
						if ((v > 0x0) && (v < 0x7))
						{
							sprintf(c_tempb, "<font color=\"%s\">", TTXT_COLOURS[v]);
							page_buffer_add_string(ctx, c_tempb);
							font_tag_opened = YES;
						}
					}
					else
						v = 0x20;
				}

				if (v >= 0x20)
				{
					ucs2_to_utf8(u, v);
					uint64_t ucs2_char = (u[0] << 24) | (u[1] << 16) | (u[2] << 8) | u[3];
					ucs2_buffer_add_char(ctx, ucs2_char);

					if (font_tag_opened == NO && tlt_config.latrusmap)
					{
						for (uint8_t i = 0; i < array_length(LAT_RUS); i++)
						{
							if (v == LAT_RUS[i].lat_char)
							{
								page_buffer_add_string(ctx, LAT_RUS[i].rus_char);
								// already processed char
								v = 0;
								break;
							}
						}
					}

					// translate some chars into entities, if in colour mode
					if (!tlt_config.nofontcolor && !tlt_config.nohtmlescape)
					{
						for (uint8_t i = 0; i < array_length(ENTITIES); i++)
							if (v == ENTITIES[i].character)
							{
								page_buffer_add_string(ctx, ENTITIES[i].entity);
								// v < 0x20 won't be printed in next block
								v = 0;
								break;
							}
					}
				}
				if (v >= 0x20)
				{
					page_buffer_add_string(ctx, u);
					if (tlt_config.gui_mode_reports) // For now we just handle the easy stuff
						fprintf(stderr, "%s", u);
				}
			}
		}

		// no tag will left opened!
		if ((!tlt_config.nofontcolor) && (font_tag_opened == YES))
		{
			page_buffer_add_string(ctx, "</font>");
			font_tag_opened = NO;
		}

		if (tlt_config.gui_mode_reports)
		{
			fprintf(stderr, "\n");
		}
	}
	time_reported = 0;

	if (ctx->sentence_cap)
		telx_case_fix(ctx);

	switch (tlt_config.write_format)
	{
		case CCX_OF_TRANSCRIPT:
		case CCX_OF_SRT:
			if (ctx->page_buffer_prev_used == 0)
				ctx->prev_show_timestamp = page->show_timestamp;
			if (ctx->page_buffer_prev_used == 0 ||
			    (tlt_config.dolevdist &&
			     fuzzy_memcmp(ctx->page_buffer_prev, ctx->page_buffer_cur,
					  ctx->ucs2_buffer_prev, ctx->ucs2_buffer_prev_used,
					  ctx->ucs2_buffer_cur, ctx->ucs2_buffer_cur_used) == 0))
			{
				// If empty previous buffer, we just start one with the
				// current page and do nothing. Wait until we see more.
				if (ctx->page_buffer_prev)
					free(ctx->page_buffer_prev);

				ctx->page_buffer_prev_used = ctx->page_buffer_cur_used;
				ctx->page_buffer_prev_size = ctx->page_buffer_cur_size;
				ctx->page_buffer_prev = ctx->page_buffer_cur;
				ctx->page_buffer_cur_size = 0;
				ctx->page_buffer_cur_used = 0;
				ctx->page_buffer_cur = NULL;

				if (ctx->ucs2_buffer_prev)
					free(ctx->ucs2_buffer_prev);
				ctx->ucs2_buffer_prev_used = ctx->ucs2_buffer_cur_used;
				ctx->ucs2_buffer_prev_size = ctx->ucs2_buffer_cur_size;
				ctx->ucs2_buffer_prev = ctx->ucs2_buffer_cur;
				ctx->ucs2_buffer_cur_size = 0;
				ctx->ucs2_buffer_cur_used = 0;
				ctx->ucs2_buffer_cur = NULL;
				ctx->prev_hide_timestamp = page->hide_timestamp;
				break;
			}
			else
			{
				// OK, the old and new buffer don't match. So write the old
				telxcc_dump_prev_page(ctx, sub);
				ctx->prev_hide_timestamp = page->hide_timestamp;
				ctx->prev_show_timestamp = page->show_timestamp;
			}
			break;
		default:
			add_cc_sub_text(sub, ctx->page_buffer_cur, page->show_timestamp,
					page->hide_timestamp + 1, NULL, "TLT", CCX_ENC_UTF_8);
	}

	// Also update GUI...

	ctx->page_buffer_cur_used = 0;
	if (ctx->page_buffer_cur)
		ctx->page_buffer_cur[0] = 0;
	if (tlt_config.gui_mode_reports)
		fflush(stderr);
}

void process_telx_packet(struct TeletextCtx *ctx, data_unit_t data_unit_id, teletext_packet_payload_t *packet, uint64_t timestamp, struct cc_subtitle *sub)
{
	// variable names conform to ETS 300 706, chapter 7.1.2
	uint8_t address, m, y, designation_code;
	address = (unham_8_4(packet->address[1]) << 4) | unham_8_4(packet->address[0]);
	m = address & 0x7;
	if (m == 0)
		m = 8;
	y = (address >> 3) & 0x1f;
	designation_code = (y > 25) ? unham_8_4(packet->data[0]) : 0x00;
	if (y == 0)
	{

		// CC map
		uint8_t i = (unham_8_4(packet->data[1]) << 4) | unham_8_4(packet->data[0]);
		uint8_t flag_subtitle = (unham_8_4(packet->data[5]) & 0x08) >> 3;
		uint16_t page_number;
		uint8_t charset;
		uint8_t c;
		ctx->cc_map[i] |= flag_subtitle << (m - 1);

		if ((flag_subtitle == YES) && (i < 0xff))
		{
			int thisp = (m << 8) | (unham_8_4(packet->data[1]) << 4) | unham_8_4(packet->data[0]);
			char t1[10];
			sprintf(t1, "%x", thisp); // Example: 1928 -> 788
			thisp = atoi(t1);
			if (!ctx->seen_sub_page[thisp])
			{
				ctx->seen_sub_page[thisp] = 1;
				mprint("\rNotice: Teletext page with possible subtitles detected: %03d\n", thisp);
			}
		}
		if ((tlt_config.page == 0) && (flag_subtitle == YES) && (i < 0xff))
		{
			tlt_config.page = (m << 8) | (unham_8_4(packet->data[1]) << 4) | unham_8_4(packet->data[0]);
			mprint("- No teletext page specified, first received suitable page is %03x, not guaranteed\n", tlt_config.page);
		}

		// Page number and control bits
		page_number = (m << 8) | (unham_8_4(packet->data[1]) << 4) | unham_8_4(packet->data[0]);
		charset = ((unham_8_4(packet->data[7]) & 0x08) | (unham_8_4(packet->data[7]) & 0x04) | (unham_8_4(packet->data[7]) & 0x02)) >> 1;
		// uint8_t flag_suppress_header = unham_8_4(packet->data[6]) & 0x01;
		// uint8_t flag_inhibit_display = (unham_8_4(packet->data[6]) & 0x08) >> 3;

		// ETS 300 706, chapter 9.3.1.3:
		// When set to '1' the service is designated to be in Serial mode and the transmission of a page is terminated
		// by the next page header with a different page number.
		// When set to '0' the service is designated to be in Parallel mode and the transmission of a page is terminated
		// by the next page header with a different page number but the same magazine number.
		// The same setting shall be used for all page headers in the service.
		// ETS 300 706, chapter 7.2.1: Page is terminated by and excludes the next page header packet
		// having the same magazine address in parallel transmission mode, or any magazine address in serial transmission mode.
		ctx->transmission_mode = (transmission_mode_t)(unham_8_4(packet->data[7]) & 0x01);

		// FIXME: Well, this is not ETS 300 706 kosher, however we are interested in DATA_UNIT_EBU_TELETEXT_SUBTITLE only
		if ((ctx->transmission_mode == TRANSMISSION_MODE_PARALLEL) && (data_unit_id != DATA_UNIT_EBU_TELETEXT_SUBTITLE) && !(de_ctr && flag_subtitle && ctx->receiving_data == YES))
			return;

		if ((ctx->receiving_data == YES) && (((ctx->transmission_mode == TRANSMISSION_MODE_SERIAL) && (PAGE(page_number) != PAGE(tlt_config.page))) ||
						     ((ctx->transmission_mode == TRANSMISSION_MODE_PARALLEL) && (PAGE(page_number) != PAGE(tlt_config.page)) && (m == MAGAZINE(tlt_config.page)))))
		{
			ctx->receiving_data = NO;
			if (!(de_ctr && flag_subtitle))
				return;
		}

		// Page transmission is terminated, however now we are waiting for our new page
		if (page_number != tlt_config.page && !(de_ctr && flag_subtitle && ctx->receiving_data == YES))
			return;

		// Now we have the begining of page transmission; if there is page_buffer pending, process it
		if (ctx->page_buffer.tainted == YES)
		{
			// Convert telx to UCS-2 before processing
			for (uint8_t yt = 1; yt <= 23; ++yt)
			{
				for (uint8_t it = 0; it < 40; it++)
				{
					if (ctx->page_buffer.text[yt][it] != 0x00 && ctx->page_buffer.g2_char_present[yt][it] == 0)
						ctx->page_buffer.text[yt][it] = telx_to_ucs2(ctx->page_buffer.text[yt][it]);
				}
			}
			// it would be nice, if subtitle hides on previous video frame, so we contract 40 ms (1 frame @25 fps)
			ctx->page_buffer.hide_timestamp = timestamp - 40;
			if (ctx->page_buffer.hide_timestamp > timestamp)
			{
				ctx->page_buffer.hide_timestamp = 0;
			}
			process_page(ctx, &ctx->page_buffer, sub);
			de_ctr = 0;
		}

		ctx->page_buffer.show_timestamp = timestamp;
		ctx->page_buffer.hide_timestamp = 0;
		memset(ctx->page_buffer.text, 0x00, sizeof(ctx->page_buffer.text));
		memset(ctx->page_buffer.g2_char_present, 0x00, sizeof(ctx->page_buffer.g2_char_present));
		ctx->page_buffer.tainted = NO;
		ctx->receiving_data = YES;
		if (default_g0_charset == LATIN) // G0 Character National Option Sub-sets selection required only for Latin Character Sets
		{
			primary_charset.g0_x28 = UNDEFINED;
			c = (primary_charset.g0_m29 != UNDEFINED) ? primary_charset.g0_m29 : charset;
			remap_g0_charset(c);
		}
		/*
		// I know -- not needed; in subtitles we will never need disturbing teletext page status bar
		// displaying tv station name, current time etc.
		if (flag_suppress_header == NO) {
			for (uint8_t i = 14; i < 40; i++) page_buffer.text[y][i] = telx_to_ucs2(packet->data[i]);
			//page_buffer.tainted = YES;
		}
		*/
	}
	else if ((m == MAGAZINE(tlt_config.page)) && (y >= 1) && (y <= 23) && (ctx->receiving_data == YES))
	{
		// ETS 300 706, chapter 9.4.1: Packets X/26 at presentation Levels 1.5, 2.5, 3.5 are used for addressing
		// a character location and overwriting the existing character defined on the Level 1 page
		// ETS 300 706, annex B.2.2: Packets with Y = 26 shall be transmitted before any packets with Y = 1 to Y = 25;
		// so page_buffer.text[y][i] may already contain any character received
		// in frame number 26, skip original G0 character
		for (uint8_t i = 0; i < 40; i++)
		{
			if (ctx->page_buffer.text[y][i] == 0x00)
				ctx->page_buffer.text[y][i] = packet->data[i];
		}
		ctx->page_buffer.tainted = YES;
		--de_ctr;
	}
	else if ((m == MAGAZINE(tlt_config.page)) && (y == 26) && (ctx->receiving_data == YES))
	{
		// ETS 300 706, chapter 12.3.2: X/26 definition
		uint8_t x26_row = 0;
		uint8_t x26_col = 0;

		uint32_t triplets[13] = {0};
		for (uint8_t i = 1, j = 0; i < 40; i += 3, j++)
			triplets[j] = unham_24_18((packet->data[i + 2] << 16) | (packet->data[i + 1] << 8) | packet->data[i]);

		for (uint8_t j = 0; j < 13; j++)
		{
			uint8_t data;
			uint8_t mode;
			uint8_t address;
			uint8_t row_address_group;
			// invalid data (HAM24/18 uncorrectable error detected), skip group
			if (triplets[j] == 0xffffffff)
			{
				dbg_print(CCX_DMT_TELETEXT, "- Unrecoverable data error; UNHAM24/18()=%04x\n", triplets[j]);
				continue;
			}

			data = (triplets[j] & 0x3f800) >> 11;
			mode = (triplets[j] & 0x7c0) >> 6;
			address = triplets[j] & 0x3f;
			row_address_group = (address >= 40) && (address <= 63);

			// ETS 300 706, chapter 12.3.1, table 27: set active position
			if ((mode == 0x04) && (row_address_group == YES))
			{
				x26_row = address - 40;
				if (x26_row == 0)
					x26_row = 24;
				x26_col = 0;
			}

			// ETS 300 706, chapter 12.3.1, table 27: termination marker
			if ((mode >= 0x11) && (mode <= 0x1f) && (row_address_group == YES))
				break;

			// ETS 300 706, chapter 12.3.1, table 27: character from G2 set
			if ((mode == 0x0f) && (row_address_group == NO))
			{
				x26_col = address;
				if (data > 31)
				{
					ctx->page_buffer.text[x26_row][x26_col] = G2[0][data - 0x20];
					ctx->page_buffer.g2_char_present[x26_row][x26_col] = 1;
				}
			}

			// ETS 300 706 v1.2.1, chapter 12.3.4, Table 29: G0 character without diacritical mark (display '@' instead of '*')
			if ((mode == 0x10) && (row_address_group == NO))
			{
				x26_col = address;
				if (data == 64) // check for @ symbol
				{
					remap_g0_charset(0);
					ctx->page_buffer.text[x26_row][x26_col] = 0x40;
				}
			}

			// ETS 300 706, chapter 12.3.1, table 27: G0 character with diacritical mark
			if ((mode >= 0x11) && (mode <= 0x1f) && (row_address_group == NO))
			{
				x26_col = address;

				// A - Z
				if ((data >= 65) && (data <= 90))
					ctx->page_buffer.text[x26_row][x26_col] = G2_ACCENTS[mode - 0x11][data - 65];
				// a - z
				else if ((data >= 97) && (data <= 122))
					ctx->page_buffer.text[x26_row][x26_col] = G2_ACCENTS[mode - 0x11][data - 71];
				// other
				else
					ctx->page_buffer.text[x26_row][x26_col] = telx_to_ucs2(data);

				ctx->page_buffer.g2_char_present[x26_row][x26_col] = 1;
			}
		}
	}
	else if ((m == MAGAZINE(tlt_config.page)) && (y == 28) && (ctx->receiving_data == YES))
	{
		// TODO:
		//   ETS 300 706, chapter 9.4.7: Packet X/28/4
		//   Where packets 28/0 and 28/4 are both transmitted as part of a page, packet 28/0 takes precedence over 28/4 for all but the colour map entry coding.
		if ((designation_code == 0) || (designation_code == 4))
		{
			// ETS 300 706, chapter 9.4.2: Packet X/28/0 Format 1
			// ETS 300 706, chapter 9.4.7: Packet X/28/4
			uint32_t triplet0 = unham_24_18((packet->data[3] << 16) | (packet->data[2] << 8) | packet->data[1]);

			if (triplet0 == 0xffffffff)
			{
				// invalid data (HAM24/18 uncorrectable error detected), skip group
				dbg_print(CCX_DMT_TELETEXT, "! Unrecoverable data error; UNHAM24/18()=%04x\n", triplet0);
			}
			else
			{
				// ETS 300 706, chapter 9.4.2: Packet X/28/0 Format 1 only
				if ((triplet0 & 0x0f) == 0x00)
				{
					// ETS 300 706, Table 32
					set_g0_charset(triplet0); // Deciding G0 Character Set
					if (default_g0_charset == LATIN)
					{
						primary_charset.g0_x28 = (triplet0 & 0x3f80) >> 7;
						remap_g0_charset(primary_charset.g0_x28);
					}
				}
			}
		}
	}
	else if ((m == MAGAZINE(tlt_config.page)) && (y == 29))
	{
		// TODO:
		//   ETS 300 706, chapter 9.5.1 Packet M/29/0
		//   Where M/29/0 and M/29/4 are transmitted for the same magazine, M/29/0 takes precedence over M/29/4.
		if ((designation_code == 0) || (designation_code == 4))
		{
			// ETS 300 706, chapter 9.5.1: Packet M/29/0
			// ETS 300 706, chapter 9.5.3: Packet M/29/4
			uint32_t triplet0 = unham_24_18((packet->data[3] << 16) | (packet->data[2] << 8) | packet->data[1]);

			if (triplet0 == 0xffffffff)
			{
				// invalid data (HAM24/18 uncorrectable error detected), skip group
				dbg_print(CCX_DMT_TELETEXT, "! Unrecoverable data error; UNHAM24/18()=%04x\n", triplet0);
			}
			else
			{
				// ETS 300 706, table 11: Coding of Packet M/29/0
				// ETS 300 706, table 13: Coding of Packet M/29/4
				if ((triplet0 & 0xff) == 0x00)
				{
					set_g0_charset(triplet0);
					if (default_g0_charset == LATIN)
					{
						primary_charset.g0_m29 = (triplet0 & 0x3f80) >> 7;
						// X/28 takes precedence over M/29
						if (primary_charset.g0_x28 == UNDEFINED)
						{
							remap_g0_charset(primary_charset.g0_m29);
						}
					}
				}
			}
		}
	}
	else if ((m == 8) && (y == 30))
	{
		// ETS 300 706, chapter 9.8: Broadcast Service Data Packets
		if (ctx->states.programme_info_processed == NO)
		{
			// ETS 300 706, chapter 9.8.1: Packet 8/30 Format 1
			if (unham_8_4(packet->data[0]) < 2)
			{
				uint32_t t = 0;
				time_t t0;
				mprint("- Programme Identification Data = ");
				for (uint8_t i = 20; i < 40; i++)
				{
					char u[4] = {0, 0, 0, 0};
					uint8_t c = telx_to_ucs2(packet->data[i]);
					// strip any control codes from PID, eg. TVP station
					if (c < 0x20)
						continue;

					ucs2_to_utf8(u, c);
					mprint("%s", u);
				}
				mprint("\n");

				// OMG! ETS 300 706 stores timestamp in 7 bytes in Modified Julian Day in BCD format + HH:MM:SS in BCD format
				// + timezone as 5-bit count of half-hours from GMT with 1-bit sign
				// In addition all decimals are incremented by 1 before transmission.
				// 1st step: BCD to Modified Julian Day
				t += (packet->data[10] & 0x0f) * 10000;
				t += ((packet->data[11] & 0xf0) >> 4) * 1000;
				t += (packet->data[11] & 0x0f) * 100;
				t += ((packet->data[12] & 0xf0) >> 4) * 10;
				t += (packet->data[12] & 0x0f);
				t -= 11111;
				// 2nd step: conversion Modified Julian Day to unix timestamp
				t = (t - 40587) * 86400;
				// 3rd step: add time
				t += 3600 * (((packet->data[13] & 0xf0) >> 4) * 10 + (packet->data[13] & 0x0f));
				t += 60 * (((packet->data[14] & 0xf0) >> 4) * 10 + (packet->data[14] & 0x0f));
				t += (((packet->data[15] & 0xf0) >> 4) * 10 + (packet->data[15] & 0x0f));
				t -= 40271;
				// 4th step: conversion to time_t
				t0 = (time_t)t;
				// ctime output itself is \n-ended
				mprint("- Universal Time Co-ordinated = %s", ctime(&t0));

				dbg_print(CCX_DMT_TELETEXT, "- Transmission mode = %s\n", (ctx->transmission_mode == TRANSMISSION_MODE_SERIAL ? "serial" : "parallel"));

				if (tlt_config.write_format == CCX_OF_TRANSCRIPT && tlt_config.date_format == ODF_DATE && !tlt_config.noautotimeref)
				{
					mprint("- Broadcast Service Data Packet received, resetting UTC referential value to %s", ctime(&t0));
					utc_refvalue = t;
					ctx->states.pts_initialized = NO;
				}

				ctx->states.programme_info_processed = YES;
			}
		}
	}
}

void tlt_write_rcwt(struct lib_cc_decode *ctx, uint8_t data_unit_id, uint8_t *packet, uint64_t timestamp, struct cc_subtitle *sub)
{
	ctx->writedata((unsigned char *)&data_unit_id, sizeof(uint8_t), NULL, sub);
	ctx->writedata((unsigned char *)&timestamp, sizeof(uint64_t), NULL, sub);
	ctx->writedata((unsigned char *)packet, 44, NULL, sub);
}

void tlt_read_rcwt(void *codec, unsigned char *buf, struct cc_subtitle *sub)
{
	struct TeletextCtx *ctx = codec;

	data_unit_t id = buf[0];

	memcpy(&(ctx->last_timestamp), &buf[1], sizeof(uint64_t));

	if (utc_refvalue != UINT64_MAX)
	{
		ctx->last_timestamp += utc_refvalue * 1000;
	}

	teletext_packet_payload_t *pl = (teletext_packet_payload_t *)&buf[9];

	ctx->tlt_packet_counter++;
	process_telx_packet(ctx, id, pl, ctx->last_timestamp, sub);
}

int tlt_print_seen_pages(struct lib_cc_decode *dec_ctx)
{
	struct TeletextCtx *ctx = NULL;

	if (dec_ctx->codec != CCX_CODEC_TELETEXT)
	{
		errno = EINVAL;
		return -1;
	}

	ctx = dec_ctx->private_data;

	for (int i = 0; i < MAX_TLT_PAGES; i++)
	{
		if (ctx->seen_sub_page[i] == 0)
			continue;
		printf("%d ", i);
	}
	return CCX_OK;
}
void set_tlt_delta(struct lib_cc_decode *dec_ctx, uint64_t pts)
{
	struct TeletextCtx *ctx = dec_ctx->private_data;
	uint32_t t = (uint32_t)(pts / 90);
	if (ctx->states.pts_initialized == NO)
	{
		if (utc_refvalue == UINT64_MAX)
		{
			ctx->delta = 0 - (uint64_t)t;
		}
		else
		{
			ctx->delta = (uint64_t)(1000 * utc_refvalue - t);
		}
		ctx->t0 = t;

		ctx->states.pts_initialized = YES;
		if ((ctx->using_pts == NO) && (ctx->global_timestamp == 0))
		{
			// We are using global PCR, nevertheless we still have not received valid PCR timestamp yet
			ctx->states.pts_initialized = NO;
		}
	}
}
int tlt_process_pes_packet(struct lib_cc_decode *dec_ctx, uint8_t *buffer, uint16_t size, struct cc_subtitle *sub, int sentence_cap)
{
	uint64_t pes_prefix;
	uint8_t pes_stream_id;
	uint16_t pes_packet_length;
	uint8_t optional_pes_header_included = NO;
	uint16_t optional_pes_header_length = 0;
	// extension
	uint8_t pes_scrambling_control;
	uint8_t pes_priority;
	uint8_t data_alignment_indicator;
	uint8_t copyright;
	uint8_t original_or_copy;
	uint8_t pts_dts_flag;
	uint8_t escr_flag;
	uint8_t es_rate;
	uint8_t dsm_flag;
	uint8_t aci_flag;
	uint8_t pes_crc_flag;
	uint8_t pes_ext_flag;
	// extension
	uint32_t t = 0;
	uint16_t i;
	struct TeletextCtx *ctx = dec_ctx->private_data;
	ctx->sentence_cap = sentence_cap;

	if (!ctx)
	{
		mprint("Teletext: Context cant be NULL, use telxcc_init\n");
		return CCX_EINVAL;
	}

	ctx->tlt_packet_counter++;
	if (size < 6)
		return CCX_OK;

	// Packetized Elementary Stream (PES) 32-bit start code
	pes_prefix = (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
	pes_stream_id = buffer[3];

	// check for PES header
	if (pes_prefix != 0x000001)
		return 0;

	// stream_id is not "Private Stream 1" (0xbd)
	if (pes_stream_id != 0xbd)
		return 0;

	// PES packet length
	// ETSI EN 301 775 V1.2.1 (2003-05) chapter 4.3: (N x 184) - 6 + 6 B header
	pes_packet_length = 6 + ((buffer[4] << 8) | buffer[5]);
	// Can be zero. If the "PES packet length" is set to zero, the PES packet can be of any length.
	// A value of zero for the PES packet length can be used only when the PES packet payload is a video elementary stream.

	if (ccx_options.pes_header_to_stdout)
	{
		pes_scrambling_control = (uint8_t)(buffer[6] << 2) >> 6;
		pes_priority = (uint8_t)(buffer[6] << 4) >> 7;
		data_alignment_indicator = (uint8_t)(buffer[6] << 5) >> 7;
		copyright = (uint8_t)(buffer[6] << 6) >> 7;
		original_or_copy = (uint8_t)(buffer[6] << 7) >> 7;
		pts_dts_flag = buffer[7] >> 6;
		escr_flag = (uint8_t)(buffer[7] << 2) >> 7;
		es_rate = (uint8_t)(buffer[7] << 3) >> 7;
		dsm_flag = (uint8_t)(buffer[7] << 4) >> 7;
		aci_flag = (uint8_t)(buffer[7] << 5) >> 7;
		pes_crc_flag = (uint8_t)(buffer[7] << 6) >> 7;
		pes_ext_flag = (uint8_t)(buffer[7] << 7) >> 7;

		printf("Packet start code prefix: %04lx # ", pes_prefix);
		printf("Stream ID: %04x # ", pes_stream_id);
		printf("Packet length: %d ", pes_packet_length);
		printf("PESSC: 0x%x ", pes_scrambling_control);
		printf("PESP: 0x%x ", pes_priority);
		printf("DAI: 0x%x ", data_alignment_indicator);
		printf("CY: 0x%x\n", copyright);
		printf("OOC: 0x%x ", original_or_copy);
		printf("PTSDTS: 0x%x ", pts_dts_flag);
		printf("ESCR: 0x%x ", escr_flag);
		printf("Rate: 0x%x\n", es_rate);
		printf("DSM: 0x%x ", dsm_flag);
		printf("ACI: 0x%x ", aci_flag);
		printf("PESCRC: 0x%x ", pes_crc_flag);
		printf("EXT: 0x%x\n", pes_ext_flag);
	}

	if (pes_packet_length == 6)
	{
		if (ccx_options.pes_header_to_stdout)
			printf("\n");
		else
			return CCX_OK;
	}

	// truncate incomplete PES packets
	if (pes_packet_length > size)
		pes_packet_length = size;

	// optional PES header marker bits (10.. ....)
	if ((buffer[6] & 0xc0) == 0x80)
	{
		optional_pes_header_included = YES;
		optional_pes_header_length = buffer[8];
	}

	// should we use PTS or PCR?
	if (ctx->using_pts == UNDEFINED)
	{
		if ((optional_pes_header_included == YES) && ((buffer[7] & 0x80) > 0))
		{
			ctx->using_pts = YES;
			dbg_print(CCX_DMT_TELETEXT, "- PID 0xbd PTS available\n");
		}
		else
		{
			ctx->using_pts = NO;
			dbg_print(CCX_DMT_TELETEXT, "- PID 0xbd PTS unavailable, using TS PCR\n");
		}
	}

	// If there is no PTS available, use global PCR
	if (ctx->using_pts == NO)
	{
		t = ctx->global_timestamp;
		// t = get_pts();
	}
	else
	{
		// PTS is 33 bits wide, however, timestamp in ms fits into 32 bits nicely (PTS/90)
		// presentation and decoder timestamps use the 90 KHz clock, hence PTS/90 = [ms]
		uint64_t pts = 0;
		// __MUST__ assign value to uint64_t and __THEN__ rotate left by 29 bits
		// << is defined for signed int (as in "C" spec.) and overflow occurs
		pts = (buffer[9] & 0x0e);
		pts <<= 29;
		pts |= (buffer[10] << 22);
		pts |= ((buffer[11] & 0xfe) << 14);
		pts |= (buffer[12] << 7);
		pts |= ((buffer[13] & 0xfe) >> 1);
		t = (uint32_t)(pts / 90);

		set_current_pts(dec_ctx->timing, pts);
		set_fts(dec_ctx->timing);

		if (ccx_options.pes_header_to_stdout)
		{
			// printf("# Associated PTS: %d \n", pts);
			printf("# Associated PTS: %" PRId64 " # ", pts);
			printf("Diff: %" PRIu64 "\n", pts - last_pes_pts);
			// printf("Diff: %d # ", pts - last_pes_pts);
			last_pes_pts = pts;
		}
	}

	/*if (ctx->states.pts_initialized == NO)
	{
		if (utc_refvalue == UINT64_MAX)
			ctx->delta = 0 - (uint64_t)t;
		else
			ctx->delta = (uint64_t) (1000 * utc_refvalue - t);
		ctx->t0 = t;

		ctx->states.pts_initialized = YES;
		if ((ctx->using_pts == NO) && (ctx->global_timestamp == 0))
		{
			// We are using global PCR, nevertheless we still have not received valid PCR timestamp yet
			ctx->states.pts_initialized = NO;
		}
	}*/

	if (t < ctx->t0)
		ctx->delta = ctx->last_timestamp;
	ctx->last_timestamp = t + ctx->delta;
	if (ctx->delta < 0 && ctx->last_timestamp > t)
	{
		ctx->last_timestamp = 0;
	}
	ctx->t0 = t;

	// skip optional PES header and process each 46-byte teletext packet
	i = 7;
	if (optional_pes_header_included == YES)
		i += 3 + optional_pes_header_length;

	while (i <= pes_packet_length - 6)
	{
		uint8_t data_unit_id = buffer[i++];
		uint8_t data_unit_len = buffer[i++];

		if ((data_unit_id == DATA_UNIT_EBU_TELETEXT_NONSUBTITLE) || (data_unit_id == DATA_UNIT_EBU_TELETEXT_SUBTITLE))
		{
			// teletext payload has always size 44 bytes
			if (data_unit_len == 44)
			{
				// reverse endianess (via lookup table), ETS 300 706, chapter 7.1
				for (uint8_t j = 0; j < data_unit_len; j++)
					buffer[i + j] = REVERSE_8[buffer[i + j]];

				if (tlt_config.write_format == CCX_OF_RCWT)
					tlt_write_rcwt(dec_ctx, data_unit_id, &buffer[i], ctx->last_timestamp, sub);
				else
				{
					// FIXME: This explicit type conversion could be a problem some day -- do not need to be platform independent
					process_telx_packet(ctx, (data_unit_t)data_unit_id, (teletext_packet_payload_t *)&buffer[i], ctx->last_timestamp, sub);
				}
			}
		}

		i += data_unit_len;
	}
	return CCX_OK;
}

// Called only when teletext is detected or forced and it's going to be used for extraction.
void *telxcc_init(void)
{
	struct TeletextCtx *ctx = malloc(sizeof(struct TeletextCtx));

	if (!ctx)
		return NULL;
	memset(ctx->seen_sub_page, 0, MAX_TLT_PAGES * sizeof(short int));
	memset(ctx->cc_map, 0, 256);

	ctx->page_buffer_prev = NULL;
	ctx->page_buffer_cur = NULL;
	ctx->page_buffer_cur_size = 0;
	ctx->page_buffer_cur_used = 0;
	ctx->page_buffer_prev_size = 0;
	ctx->page_buffer_prev_used = 0;
	// Current and previous page compare strings. This is plain text (no colors,
	// tags, etc) in UCS2 (fixed length), so we can compare easily.
	ctx->ucs2_buffer_prev = NULL;
	ctx->ucs2_buffer_cur = NULL;
	ctx->ucs2_buffer_cur_size = 0;
	ctx->ucs2_buffer_cur_used = 0;
	ctx->ucs2_buffer_prev_size = 0;
	ctx->ucs2_buffer_prev_used = 0;

	// Buffer timestamp
	ctx->last_timestamp = 0;
	memset(&ctx->page_buffer, 0, sizeof(teletext_page_t));
	ctx->states.programme_info_processed = NO;
	ctx->states.pts_initialized = NO;
	ctx->tlt_packet_counter = 0;
	ctx->transmission_mode = TRANSMISSION_MODE_SERIAL;
	ctx->receiving_data = NO;

	ctx->using_pts = UNDEFINED;
	ctx->delta = 0;
	ctx->t0 = 0;

	ctx->sentence_cap = 0;
	ctx->new_sentence = 0;
	ctx->splitbysentence = 0;

	return ctx;
}

void telxcc_update_gt(void *codec, uint32_t global_timestamp)
{
	struct TeletextCtx *ctx = codec;
	ctx->global_timestamp = global_timestamp;
}

// Close output
void telxcc_close(void **ctx, struct cc_subtitle *sub)
{
	struct TeletextCtx *ttext = *ctx;

	if (!ttext)
		return;

	mprint("\nTeletext decoder: %" PRIu32 " packets processed \n", ttext->tlt_packet_counter);
	if (tlt_config.write_format != CCX_OF_RCWT && sub)
	{
		// output any pending close caption
		if (ttext->page_buffer.tainted == YES)
		{
			// Convert telx to UCS-2 before processing
			for (uint8_t yt = 1; yt <= 23; ++yt)
			{
				for (uint8_t it = 0; it < 40; it++)
				{
					if (ttext->page_buffer.text[yt][it] != 0x00 && ttext->page_buffer.g2_char_present[yt][it] == 0)
						ttext->page_buffer.text[yt][it] = telx_to_ucs2(ttext->page_buffer.text[yt][it]);
				}
			}
			// this time we do not subtract any frames, there will be no more frames
			ttext->page_buffer.hide_timestamp = ttext->last_timestamp;
			process_page(ttext, &ttext->page_buffer, sub);
		}

		telxcc_dump_prev_page(ttext, sub);
	}
	freep(&ttext->ucs2_buffer_cur);
	freep(&ttext->page_buffer_cur);
	freep(ctx);
}
