#include <png.h>
#include "zlib.h"
#include "gpac/setup.h"
#include "gpac/version.h"
#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "utility.h"
#include "activity.h"
#include "ccx_encoders_helpers.h"
#include "ccx_common_common.h"
#include "ccx_decoders_708.h"
#include "compile_info.h"
#include "../lib_hash/sha2.h"
#include <string.h>
#include <stdio.h>
#if __has_include(<utf8proc.h>)
#include <utf8proc.h>
#else
#include <utf8proc/utf8proc.h>
#endif
#ifdef ENABLE_OCR
#include <tesseract/capi.h>
#include <leptonica/allheaders.h>
#endif

#ifdef ENABLE_HARDSUBX
#include "hardsubx.h"
#endif

#ifdef _WIN32
#define DEFAULT_FONT_PATH "C:\\Windows\\Fonts\\calibri.ttf"
#define DEFAULT_FONT_PATH_ITALICS "C:\\Windows\\Fonts\\calibrii.ttf"
#elif __APPLE__ // MacOS
#define DEFAULT_FONT_PATH "/System/Library/Fonts/Helvetica.ttc"
#define DEFAULT_FONT_PATH_ITALICS "/System/Library/Fonts/Helvetica-Oblique.ttf"
#else // Assume Linux
#define DEFAULT_FONT_PATH "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf"
#define DEFAULT_FONT_PATH_ITALICS "/usr/share/fonts/truetype/noto/NotoSans-Italic.ttf"
#endif

extern void ccxr_init_logger();

static int inputfile_capacity = 0;

void set_binary_mode()
{
#ifdef WIN32
	setmode(fileno(stdin), O_BINARY);
#endif
}

void print_usage(void)
{
	mprint("Originally based on McPoodle's tools. Check his page for lots of information\n");
	mprint("on closed captions technical details.\n");
	mprint("(http://www.theneitherworld.com/mcpoodle/SCC_TOOLS/DOCS/SCC_TOOLS.HTML)\n\n");
	mprint("This tool home page:\n");
	mprint("http://www.ccextractor.org\n");
	mprint("  Extracts closed captions and teletext subtitles from video streams.\n");
	mprint("    (DVB, .TS, ReplayTV 4000 and 5000, dvr-ms, bttv, Tivo, Dish Network,\n");
	mprint("     .mp4, HDHomeRun are known to work).\n\n");
	mprint("  Syntax:\n");
	mprint("  ccextractor [options] inputfile1 [inputfile2...] [-o outputfilename]\n");
	mprint("\n");
	mprint("To see This Help Message: -h or --help\n\n");
	mprint("File name related options:\n");
	mprint("            inputfile: file(s) to process\n");
	mprint("    -o outputfilename: Use -o parameters to define output filename if you don't\n");
	mprint("                       like the default ones (same as infile plus _1 or _2 when\n");
	mprint("                       needed and file extension, e.g. .srt).\n");
	mprint("              --stdout: Write output to stdout (console) instead of file. If\n");
	mprint("                       stdout is used, then -o can't be used. Also\n");
	mprint("                       --stdout will redirect all messages to stderr (error).\n");
	mprint("           --pesheader: Dump the PES Header to stdout (console). This is\n");
	mprint("                       used for debugging purposes to see the contents\n");
	mprint("                       of each PES packet header.\n");
	mprint("         --debugdvbsub: Write the DVB subtitle debug traces to console.\n");
	mprint("      --ignoreptsjumps: Ignore PTS jumps (default).\n");
	mprint("         --fixptsjumps: fix pts jumps. Use this parameter if you\n");
	mprint("                       experience timeline resets/jumps in the output.\n");
	mprint("               --stdin: Reads input from stdin (console) instead of file.\n");
	mprint("                       Alternatively, - can be used instead of -stdin\n");
	mprint("Output file segmentation:\n");
	mprint("    --outinterval x output in interval of x seconds\n");
	mprint("   --segmentonkeyonly: When segmenting files, do it only after a I frame\n");
	mprint("                            trying to behave like FFmpeg\n\n");
	mprint("Network support:\n");
	mprint("      --udp [[src@]host:]port: Read the input via UDP (listening in the specified\n");
	mprint("                              port) instead of reading a file. Host and src can be a\n");
	mprint("                              hostname or IPv4 address. If host is not specified\n");
	mprint("                              then listens on the local host.\n\n");
	mprint("            --sendto host[:port]: Sends data in BIN format to the server\n");
	mprint("                                 according to the CCExtractor's protocol over\n");
	mprint("                                 TCP. For IPv6 use [address]:port\n");
	mprint("            --tcp port: Reads the input data in BIN format according to\n");
	mprint("                        CCExtractor's protocol, listening specified port on the\n");
	mprint("                        local host\n");
	mprint("            --tcp-password password: Sets server password for new connections to\n");
	mprint("                                   tcp server\n");
	mprint("            --tcp-description description: Sends to the server short description about\n");
	mprint("                                  captions e.g. channel name or file name\n");
	mprint("Options that affect what will be processed:\n");
	mprint("      --output-field 1 / 2 / both:\n");
	mprint("                       				Values: 1 = Output Field 1\n");
	mprint("                       						2 = Output Field 2\n");
	mprint("                       						both = Both Output Field 1 and 2\n");
	mprint("                       				Defaults to 1\n");
	mprint("Use --append to prevent overwriting of existing files. The output will be\n");
	mprint("      appended instead.\n");
	mprint("                 --cc2: When in srt/sami mode, process captions in channel 2\n");
	mprint("                       instead of channel 1. Alternatively, --CC2 can also be used.\n");
	mprint("	--service N1[cs1],N2[cs2]...:\n");
	mprint("                       Enable CEA-708 (DTVCC) captions processing for the listed\n");
	mprint("                       services. The parameter is a comma delimited list\n");
	mprint("                       of services numbers, such as \"1,2\" to process the\n");
	mprint("                       primary and secondary language services.\n");
	mprint("                       Pass \"all\" to process all services found.\n");
	mprint("\n");
	mprint("                       If captions in a service are stored in 16-bit encoding,\n");
	mprint("                       you can specify what charset or encoding was used. Pass\n");
	mprint("                       its name after service number (e.g. \"1[EUC-KR],3\" or\n");
	mprint("                       \"all[EUC-KR]\") and it will encode specified charset to\n");
	mprint("                       UTF-8 using iconv. See iconv documentation to check if\n");
	mprint("                       required encoding/charset is supported.\n");
	mprint("\n");
	mprint("Input formats:\n");
	mprint("       With the exception of McPoodle's raw format, which is just the closed\n");
	mprint("       caption data with no other info, CCExtractor can usually detect the\n");
	mprint("       input format correctly. To force a specific format:\n\n");
	mprint("                  -in=format\n\n");
	mprint("       where format is one of these:\n");
	mprint("                       ts   -> For Transport Streams.\n");
	mprint("                       ps   -> For Program Streams.\n");
	mprint("                       es   -> For Elementary Streams.\n");
	mprint("                       asf  -> ASF container (such as DVR-MS).\n");
	mprint("                       wtv  -> Windows Television (WTV)\n");
	mprint("                       bin  -> CCExtractor's own binary format.\n");
	mprint("                       raw  -> For McPoodle's raw files.\n");
	mprint("                       mp4  -> MP4/MOV/M4V and similar.\n");
	mprint("                       m2ts -> BDAV MPEG-2 Transport Stream\n");
	mprint("                       mkv  -> Matroska container and WebM.\n");
	mprint("                       mxf  -> Material Exchange Format (MXF).\n");
#ifdef WTV_DEBUG
	mprint("                       hex  -> Hexadecimal dump as generated by wtvccdump.\n");
#endif
	mprint("       --ts, --ps, --es, --mp4, --wtv, --mkv and --asf/--dvr-ms can be used as shorts.\n\n");
	mprint("Output formats:\n\n");
	mprint("                 --out=format\n\n");
	mprint("       where format is one of these:\n");
	mprint("                      srt     -> SubRip (default, so not actually needed).\n");
	mprint("                      ass/ssa -> SubStation Alpha.\n");
	mprint("                      ccd     -> Scenarist Closed Caption Disassembly format\n");
	mprint("                      scc     -> Scenarist Closed Caption format\n");
	mprint("                      webvtt  -> WebVTT format\n");
	mprint("                      webvtt-full -> WebVTT format with styling\n");
	mprint("                      sami    -> MS Synchronized Accesible Media Interface.\n");
	mprint("                      bin     -> CC data in CCExtractor's own binary format.\n");
	mprint("                      raw     -> CC data in McPoodle's Broadcast format.\n");
	mprint("                      dvdraw  -> CC data in McPoodle's DVD format.\n");
	mprint("                      mcc     -> CC data compressed using MacCaption Format.\n");
	mprint("                      txt     -> Transcript (no time codes, no roll-up\n");
	mprint("                                 captions, just the plain transcription.\n");
	mprint("                      ttxt    -> Timed Transcript (transcription with time\n");
	mprint("                                 info)\n");
	mprint("                      g608    -> Grid 608 format.\n");
#ifdef WITH_LIBCURL
	mprint("                      curl    -> POST plain transcription frame-by-frame to a\n");
	mprint("                                 URL specified by -curlposturl. Don't produce\n");
	mprint("                                 any file output.\n");
#endif
	mprint("                      smptett -> SMPTE Timed Text (W3C TTML) format.\n");
	mprint("                      spupng  -> Set of .xml and .png files for use with\n");
	mprint("                                 dvdauthor's spumux.\n");
	mprint("                                 See \"Notes on spupng output format\"\n");
	mprint("                      null    -> Don't produce any file output\n");
	mprint("                      report  -> Prints to stdout information about captions\n");
	mprint("                                 in specified input. Don't produce any file\n");
	mprint("                                 output\n\n");
	mprint("       --srt, --dvdraw, --sami, --webvtt, --txt, --ttxt and --null can be used as shorts.\n\n");

	mprint("Options that affect how input files will be processed.\n");

	mprint("       --goptime: Use GOP for timing instead of PTS. This only applies\n");
	mprint("                       to Program or Transport Streams with MPEG2 data and\n");
	mprint("                       overrides the default PTS timing.\n");
	mprint("                       GOP timing is always used for Elementary Streams.\n");
	mprint("	   --no-goptime: Never use GOP timing (use PTS), even if ccextractor\n");
	mprint("                       detects GOP timing is the reasonable choice.\n");
	mprint("       --fixpadding: Fix padding - some cards (or providers, or whatever)\n");
	mprint("                       seem to send 0000 as CC padding instead of 8080. If you\n");
	mprint("                       get bad timing, this might solve it.\n");
	mprint("               --90090: Use 90090 (instead of 90000) as MPEG clock frequency.\n");
	mprint("                       (reported to be needed at least by Panasonic DMR-ES15\n");
	mprint("                       DVD Recorder)\n");
	mprint("       --videoedited: By default, ccextractor will process input files in\n");
	mprint("                       sequence as if they were all one large file (i.e.\n");
	mprint("                       split by a generic, non video-aware tool. If you\n");
	mprint("                       are processing video hat was split with a editing\n");
	mprint("                       tool, use --ve so ccextractor doesn't try to rebuild\n");
	mprint("                       the original timing.\n");
	mprint("   -s --stream [secs]: Consider the file as a continuous stream that is\n");
	mprint("                       growing as ccextractor processes it, so don't try\n");
	mprint("                       to figure out its size and don't terminate processing\n");
	mprint("                       when reaching the current end (i.e. wait for more\n");
	mprint("                       data to arrive). If the optional parameter secs is\n");
	mprint("                       present, it means the number of seconds without any\n");
	mprint("                       new data after which ccextractor should exit. Use\n");
	mprint("                       this parameter if you want to process a live stream\n");
	mprint("                       but not kill ccextractor externally.\n");
	mprint("                       Note: If -s is used then only one input file is\n");
	mprint("                       allowed.\n");
	mprint("      --usepicorder: Use the pic_order_cnt_lsb in AVC/H.264 data streams\n");
	mprint("                       to order the CC information.  The default way is to\n");
	mprint("                       use the PTS information.  Use this switch only when\n");
	mprint("                       needed.\n");
	mprint("                --myth: Force MythTV code branch.\n");
	mprint("              --no-myth: Disable MythTV code branch.\n");
	mprint("                       The MythTV branch is needed for analog captures where\n");
	mprint("                       the closed caption data is stored in the VBI, such as\n");
	mprint("                       those with bttv cards (Hauppage 250 for example). This\n");
	mprint("                       is detected automatically so you don't need to worry\n");
	mprint("                       about this unless autodetection doesn't work for you.\n");
	mprint("       --wtvconvertfix: This switch works around a bug in Windows 7's built in\n");
	mprint("                       software to convert *.wtv to *.dvr-ms. For analog NTSC\n");
	mprint("                       recordings the CC information is marked as digital\n");
	mprint("                       captions. Use this switch only when needed.\n");
	mprint("            --wtvmpeg2: Read the captions from the MPEG2 video stream rather\n");
	mprint("                       than the captions stream in WTV files\n");
	mprint("     --program-number: In TS mode, specifically select a program to process.\n");
	mprint("                       Not needed if the TS only has one. If this parameter\n");
	mprint("                       is not specified and CCExtractor detects more than one\n");
	mprint("                       program in the input, it will list the programs found\n");
	mprint("                       and terminate without doing anything, unless\n");
	mprint("                       --autoprogram (see below) is used.\n");
	mprint("         --autoprogram: If there's more than one program in the stream, just use\n");
	mprint("                       the first one we find that contains a suitable stream.\n");
	mprint("        --multiprogram: Uses multiple programs from the same input stream.\n");
	mprint("             --datapid: Don't try to find out the stream for caption/teletext\n");
	mprint("                       data, just use this one instead.\n");
	mprint("      --datastreamtype: Instead of selecting the stream by its PID, select it\n");
	mprint("                       by its type (pick the stream that has this type in\n");
	mprint("                       the PMT)\n");
	mprint("          --streamtype: Assume the data is of this type, don't autodetect. This\n");
	mprint("                       parameter may be needed if --datapid or -datastreamtype\n");
	mprint("                       is used and CCExtractor cannot determine how to process\n");
	mprint("                       the stream. The value will usually be 2 (MPEG video) or\n");
	mprint("                       6 (MPEG private data).\n");
	mprint("    	  --hauppauge: If the video was recorder using a Hauppauge card, it\n");
	mprint("                       might need special processing. This parameter will\n");
	mprint("                       force the special treatment.\n");
	mprint("         --mp4vidtrack: In MP4 files the closed caption data can be embedded in\n");
	mprint("                       the video track or in a dedicated CC track. If a\n");
	mprint("                       dedicated track is detected it will be processed instead\n");
	mprint("                       of the video track. If you need to force the video track\n");
	mprint("                       to be processed instead use this option.\n");
	mprint("       --no-autotimeref: Some streams come with broadcast date information. When\n");
	mprint("                       such data is available, CCExtractor will set its time\n");
	mprint("                       reference to the received data. Use this parameter if\n");
	mprint("                       you prefer your own reference. Note: Current this only\n");
	mprint("                       affects Teletext in timed transcript with -datets.\n");
	mprint("           --no-scte20: Ignore SCTE-20 data if present.\n");
	mprint("  --webvtt-create-css: Create a separate file for CSS instead of inline.\n");
	mprint("              --deblev: Enable debug so the calculated distance for each two\n");
	mprint("                       strings is displayed. The output includes both strings,\n");
	mprint("                       the calculated distance, the maximum allowed distance,\n");
	mprint("                       and whether the strings are ultimately considered  \n");
	mprint("                       equivalent or not, i.e. the calculated distance is \n");
	mprint("                       less or equal than the max allowed..\n");
	mprint("	   --analyzevideo  Analyze the video stream even if it's not used for\n");
	mprint("                       subtitles. This allows to provide video information.\n");
	mprint("  --timestamp-map      Enable the X-TIMESTAMP-MAP header for WebVTT (HLS)\n");
	mprint("Levenshtein distance:\n");
	mprint("           --no-levdist: Don't attempt to correct typos with Levenshtein distance.\n");
	mprint(" --levdistmincnt value: Minimum distance we always allow regardless\n");
	mprint("                       of the length of the strings.Default 2. \n");
	mprint("                       This means that if the calculated distance \n");
	mprint("                       is 0,1 or 2, we consider the strings to be equivalent.\n");
	mprint(" --levdistmaxpct value: Maximum distance we allow, as a percentage of\n");
	mprint("                       the shortest string length. Default 10%.\n");
	mprint("                       For example, consider a comparison of one string of \n");
	mprint("	                    30 characters and one of 60 characters. We want to \n");
	mprint("                       determine whether the first 30 characters of the longer\n");
	mprint("                       string are more or less the same as the shortest string,\n");
	mprint("	                    i.e. whether the longest string  is the shortest one\n");
	mprint("                       plus new characters and maybe some corrections. Since\n");
	mprint("                       the shortest string is 30 characters and  the default\n");
	mprint("                       percentage is 10%, we would allow a distance of up \n");
	mprint("                       to 3 between the first 30 characters.\n");
	mprint("\n");
	mprint("Options that affect what kind of output will be produced:\n");
	mprint("            --chapters: (Experimental) Produces a chapter file from MP4 files.\n");
	mprint("                       Note that this must only be used with MP4 files,\n");
	mprint("                       for other files it will simply generate subtitles file.\n");
	mprint("                 --bom: Append a BOM (Byte Order Mark) to output files.\n");
	mprint("                       Note that most text processing tools in linux will not\n");
	mprint("                       like BOM.\n");
	mprint("                       --no-bom: Do not append a BOM (Byte Order Mark) to output\n");
	mprint("                       files. Note that this may break files when using\n");
	mprint("                       Windows. This is the default in non-Windows builds.\n");
	mprint("             --unicode: Encode subtitles in Unicode instead of Latin-1.\n");
	mprint("                --utf8: Encode subtitles in UTF-8 (no longer needed.\n");
	mprint("                       because UTF-8 is now the default).\n");
	mprint("              --latin1: Encode subtitles in Latin-1\n");
	mprint("        --no-fontcolor: For .srt/.sami/.vtt, don't add font color tags.\n");
	mprint("       --no-htmlescape: For .srt/.sami/.vtt, don't covert html unsafe character\n");
	mprint("      --no-typesetting: For .srt/.sami/.vtt, don't add typesetting tags.\n");
	mprint("                --trim: Trim lines.\n");
	mprint("        --defaultcolor: Select a different default color (instead of\n");
	mprint("                       white). This causes all output in .srt/.smi/.vtt\n");
	mprint("                       files to have a font tag, which makes the files\n");
	mprint("                       larger. Add the color you want in RGB, such as\n");
	mprint("                       --defaultcolor #FF0000 for red.\n");
	mprint("         --sentencecap: Sentence capitalization. Use if you hate\n");
	mprint("                       ALL CAPS in subtitles.\n");
	mprint("        --capfile file: Add the contents of 'file' to the list of words\n");
	mprint("                       that must be capitalized. For example, if file\n");
	mprint("                       is a plain text file that contains\n\n");
	mprint("                       Tony\n");
	mprint("                       Alan\n\n");
	mprint("                       Whenever those words are found they will be written\n");
	mprint("                       exactly as they appear in the file.\n");
	mprint("                       Use one line per word. Lines starting with # are\n");
	mprint("                       considered comments and discarded.\n\n");
	mprint("                 --kf: Censors profane words from subtitles.\n");
	mprint("--profanity-file <file>: Add the contents of <file> to the list of words that.\n");
	mprint("                         must be censored. The content of <file>, follows the\n");
	mprint("                         same syntax as for the capitalization file\n");
	mprint("      --splitbysentence: Split output text so each frame contains a complete\n");
	mprint("                       sentence. Timings are adjusted based on number of\n");
	mprint("                       characters\n");
	mprint("          --unixts REF: For timed transcripts that have an absolute date\n");
	mprint("                       instead of a timestamp relative to the file start), use\n");
	mprint("                       this time reference (UNIX timestamp). 0 => Use current\n");
	mprint("                       system time.\n");
	mprint("                       ccextractor will automatically switch to transport\n");
	mprint("                       stream UTC timestamps when available.\n");
	mprint("              --datets: In transcripts, write time as YYYYMMDDHHMMss,ms.\n");
	mprint("               --sects: In transcripts, write time as ss,ms\n");
	mprint("                --UCLA: Transcripts are generated with a specific format\n");
	mprint("                       that is convenient for a specific project, feel\n");
	mprint("                       free to play with it but be aware that this format\n");
	mprint("                       is really live - don't rely on its output format\n");
	mprint("                       not changing between versions.\n");
	mprint("            --latrusmap Map Latin symbols to Cyrillic ones in special cases\n");
	mprint("                       of Russian Teletext files (issue #1086)\n");
	mprint("                 --xds: In timed transcripts, all XDS information will be saved\n");
	mprint("                       to the output file.\n");
	mprint("                  --lf: Use LF (UNIX) instead of CRLF (DOS, Windows) as line\n");
	mprint("                       terminator.\n");
	mprint("                  --df: For MCC Files, force dropframe frame count.\n");
	mprint("            --autodash: Based on position on screen, attempt to determine\n");
	mprint("                       the different speakers and a dash (-) when each\n");
	mprint("                       of them talks (.srt/.vtt only, --trim required).\n");
	mprint("          --xmltv mode: produce an XMLTV file containing the EPG data from\n");
	mprint("                       the source TS file. Mode: 1 = full output\n");
	mprint("                       2 = live output. 3 = both\n");
	mprint(" --xmltvliveinterval x: interval of x seconds between writing live mode xmltv output.\n");
	mprint("--xmltvoutputinterval x: interval of x seconds between writing full file xmltv output.\n");
	mprint("    --xmltvonlycurrent: Only print current events for xmltv output.\n");
	mprint("                 --sem: Create a .sem file for each output file that is open\n");
	mprint("                       and delete it on file close.\n");
	mprint("             --dvblang: For DVB subtitles, select which language's caption\n");
	mprint("                       stream will be processed. e.g. 'eng' for English.\n");
	mprint("                       If there are multiple languages, only this specified\n");
	mprint("                       language stream will be processed (default).\n");
	mprint("       --split-dvb-subs: Extract each DVB subtitle stream to a separate file.\n");
	mprint("                       Each file will be named with the base filename plus a\n");
	mprint("                       language suffix (e.g., output_deu.srt, output_fra.srt).\n");
	mprint("                       For streams without language tags, uses PID as suffix.\n");
	mprint("                       Incompatible with: stdout output, manual PID selection,\n");
	mprint("                       multiprogram mode. Only works with SRT, SAMI, WebVTT.\n");
	mprint("        --no-dvb-dedup: Disable DVB subtitle deduplication. By default, CCExtractor\n");
	mprint("                       filters out duplicate DVB subtitles to prevent repetition.\n");
	mprint("                       Use this flag to output all subtitles as-is.\n");
	mprint("             --ocrlang: Manually select the name of the Tesseract .traineddata\n");
	mprint("                       file. Helpful if you want to OCR a caption stream of\n");
	mprint("                       one language with the data of another language.\n");
	mprint("                       e.g. '-dvblang chs --ocrlang chi_tra' will decode the\n");
	mprint("                       Chinese (Simplified) caption stream but perform OCR\n");
	mprint("                       using the Chinese (Traditional) trained data\n");
	mprint("                       This option is also helpful when the traineddata file\n");
	mprint("                       has non standard names that don't follow ISO specs\n");
	mprint("          --quant mode: How to quantize the bitmap before passing it to tesseract\n");
	mprint("                       for OCR'ing.\n");
	mprint("                       0: Don't quantize at all.\n");
	mprint("                       1: Use CCExtractor's internal function (default).\n");
	mprint("                       2: Reduce distinct color count in image for faster results.\n");
	mprint("                 --oem: Select the OEM mode for Tesseract.\n");
	mprint("                       Available modes :\n");
	mprint("                       0: OEM_TESSERACT_ONLY - the fastest mode.\n");
	mprint("                       1: OEM_LSTM_ONLY - use LSTM algorithm for recognition.\n");
	mprint("                       2: OEM_TESSERACT_LSTM_COMBINED - both algorithms.\n");
	mprint("                       Default value depends on the tesseract version linked :\n");
	mprint("                       Tesseract v3 : default mode is 0,\n");
	mprint("                       Tesseract v4 : default mode is 1.\n");
	mprint("                 --psm: Select the PSM mode for Tesseract.\n");
	mprint("                       Available Page segmentation modes:\n");
	mprint("                       0    Orientation and script detection (OSD) only.\n");
	mprint("                       1    Automatic page segmentation with OSD.\n");
	mprint("                       2    Automatic page segmentation, but no OSD, or OCR.\n");
	mprint("                       3    Fully automatic page segmentation, but no OSD. (Default)\n");
	mprint("                       4    Assume a single column of text of variable sizes.\n");
	mprint("                       5    Assume a single uniform block of vertically aligned text.\n");
	mprint("                       6    Assume a single uniform block of text.\n");
	mprint("                       7    Treat the image as a single text line.\n");
	mprint("                       8    Treat the image as a single word.\n");
	mprint("                       9    Treat the image as a single word in a circle.\n");
	mprint("                       10    Treat the image as a single character.\n");
	mprint("                       11    Sparse text. Find as much text as possible in no particular order.\n");
	mprint("                       12    Sparse text with OSD.\n");
	mprint("                       13    Raw line. Treat the image as a single text line,\n");
	mprint("                       bypassing hacks that are Tesseract-specific.\n");
	mprint("       --ocr-line-split: Split subtitle images into lines before OCR.\n");
	mprint("                       Uses PSM 7 (single text line mode) for each line,\n");
	mprint("                       which can improve accuracy for multi-line bitmap subtitles\n");
	mprint("                       (VOBSUB, DVD, DVB).\n");
	mprint("     --no-ocr-blacklist: Disable the OCR character blacklist. By default,\n");
	mprint("                       CCExtractor blacklists characters like |, \\, `, _, ~\n");
	mprint("                       that are commonly misrecognized (e.g. 'I' as '|').\n");
	mprint("             --mkvlang: For MKV subtitles, select which language's caption\n");
	mprint("                       stream will be processed. e.g. 'eng' for English.\n");
	mprint("                       Language codes can be either the 3 letters bibliographic\n");
	mprint("                       ISO-639-2 form (like \"fre\" for french) or a language\n");
	mprint("                       code followed by a dash and a country code for specialities\n");
	mprint("                       in languages (like \"fre-ca\" for Canadian French).\n");
	mprint("          --no-spupngocr When processing DVB don't use the OCR to write the text as\n");
	mprint("                       comments in the XML file.\n");
	mprint("                --font: Specify the full path of the font that is to be used when\n");
	mprint("                       generating SPUPNG files. If not specified, you need to\n");
	mprint("                       have the default font installed (Helvetica for macOS, Calibri\n");
	mprint("                       for Windows, and Noto for other operating systems at their)\n");
	mprint("                       default location)\n");
	mprint("             --italics: Specify the full path of the italics font that is to be used when\n");
	mprint("                       generating SPUPNG files. If not specified, you need to\n");
	mprint("                       have the default font installed (Helvetica Oblique for macOS, Calibri Italic\n");
	mprint("                       for Windows, and NotoSans Italic for other operating systems at their)\n");
	mprint("                       default location)\n");
	mprint("\n");
	mprint("Options that affect how ccextractor reads and writes (buffering):\n");

	mprint("         --bufferinput: Forces input buffering.\n");
	mprint("       --no-bufferinput: Disables input buffering.\n");
	mprint("      --buffersize val: Specify a size for reading, in bytes (suffix with K or\n");
	mprint("                       or M for kilobytes and megabytes). Default is 16M.\n");
	mprint("                 --koc: keep-output-close. If used then CCExtractor will close\n");
	mprint("                       the output file after writing each subtitle frame and\n");
	mprint("                       attempt to create it again when needed.\n");
	mprint("          --forceflush: Flush the file buffer whenever content is written.\n");
	mprint("\n");

	mprint("Options that affect the built-in 608 closed caption decoder:\n");

	mprint("                 --dru: Direct Roll-Up. When in roll-up mode, write character by\n");
	mprint("                       character instead of line by line. Note that this\n");
	mprint("                       produces (much) larger files.\n");
	mprint("           --no-rollup: If you hate the repeated lines caused by the roll-up\n");
	mprint("                       emulation, you can have ccextractor write only one\n");
	mprint("                       line at a time, getting rid of these repeated lines.\n");
	mprint("     --ru1 / ru2 / ru3: roll-up captions can consist of 2, 3 or 4 visible\n");
	mprint("                       lines at any time (the number of lines is part of\n");
	mprint("                       the transmission). If having 3 or 4 lines annoys\n");
	mprint("                       you you can use --ru to force the decoder to always\n");
	mprint("                       use 1, 2 or 3 lines. Note that 1 line is not\n");
	mprint("                       a real mode rollup mode, so CCExtractor does what\n");
	mprint("                       it can.\n");
	mprint("                       In --ru1 the start timestamp is actually the timestamp\n");
	mprint("                       of the first character received which is possibly more\n");
	mprint("                       accurate.\n");
	mprint("\n");

	mprint("Options that affect timing:\n");

	mprint("            --delay ms: For srt/sami/webvtt, add this number of milliseconds to\n");
	mprint("                       all times. For example, --delay 400 makes subtitles\n");
	mprint("                       appear 400ms late. You can also use negative numbers\n");
	mprint("                       to make subs appear early.\n");

	mprint("Options that affect what segment of the input file(s) to process:\n");

	mprint("        --startat time: Only write caption information that starts after the\n");
	mprint("                       given time.\n");
	mprint("                       Time can be seconds, MM:SS or HH:MM:SS.\n");
	mprint("                       For example, --startat 3:00 means 'start writing from\n");
	mprint("                       minute 3.\n");
	mprint("          --endat time: Stop processing after the given time (same format as\n");
	mprint("                       -startat).\n");
	mprint("                       The --startat and --endat options are honored in all\n");
	mprint("                       output formats.  In all formats with timing information\n");
	mprint("                       the times are unchanged.\n");
	mprint("      --screenfuls num: Write 'num' screenfuls and terminate processing.\n\n");

	mprint("Options that affect which codec is to be used have to be searched in input\n");

	mprint("      --codec dvbsub    select the dvb subtitle from all elementary stream,\n"
	       "                        if stream of dvb subtitle type is not found then \n"
	       "                        nothing is selected and no subtitle is generated\n"
	       "      --no-codec dvbsub   ignore dvb subtitle and follow default behaviour\n"
	       "      --codec teletext   select the teletext subtitle from elementary stream\n"
	       "      --no-codec teletext ignore teletext subtitle\n");

	mprint("Adding start and end credits:\n");

	mprint("        --startcreditstext txt: Write this text as start credits. If there are\n");
	mprint("                                several lines, separate them with the\n");
	mprint("                                characters \\n, for example Line1\\nLine 2.\n");
	mprint("  --startcreditsnotbefore time: Don't display the start credits before this\n");
	mprint("                                time (S, or MM:SS). Default: %s\n", DEF_VAL_STARTCREDITSNOTBEFORE);
	mprint("   --startcreditsnotafter time: Don't display the start credits after this\n");
	mprint("                                time (S, or MM:SS). Default: %s\n", DEF_VAL_STARTCREDITSNOTAFTER);
	mprint(" --startcreditsforatleast time: Start credits need to be displayed for at least\n");
	mprint("                                this time (S, or MM:SS). Default: %s\n", DEF_VAL_STARTCREDITSFORATLEAST);
	mprint("  --startcreditsforatmost time: Start credits should be displayed for at most\n");
	mprint("                                this time (S, or MM:SS). Default: %s\n", DEF_VAL_STARTCREDITSFORATMOST);
	mprint("          --endcreditstext txt: Write this text as end credits. If there are\n");
	mprint("                                several lines, separate them with the\n");
	mprint("                                characters \\n, for example Line1\\nLine 2.\n");
	mprint("   --endcreditsforatleast time: End credits need to be displayed for at least\n");
	mprint("                                this time (S, or MM:SS). Default: %s\n", DEF_VAL_ENDCREDITSFORATLEAST);
	mprint("    --endcreditsforatmost time: End credits should be displayed for at most\n");
	mprint("                                this time (S, or MM:SS). Default: %s\n", DEF_VAL_ENDCREDITSFORATMOST);
	mprint("\n");

	mprint("Options that affect debug data:\n");

	mprint("               --debug: Show lots of debugging output.\n");
	mprint("                 --608: Print debug traces from the EIA-608 decoder.\n");
	mprint("                       If you need to submit a bug report, please send\n");
	mprint("                       the output from this option.\n");
	mprint("                 --708: Print debug information from the (currently\n");
	mprint("                       in development) EIA-708 (DTV) decoder.\n");
	mprint("              --goppts: Enable lots of time stamp output.\n");
	mprint("            --xdsdebug: Enable XDS debug data (lots of it).\n");
	mprint("               --vides: Print debug info about the analysed elementary\n");
	mprint("                       video stream.\n");
	mprint("               --cbraw: Print debug trace with the raw 608/708 data with\n");
	mprint("                       time stamps.\n");
	mprint("              --no-sync: Disable the syncing code.  Only useful for debugging\n");
	mprint("                       purposes.\n");
	mprint("             --fullbin: Disable the removal of trailing padding blocks\n");
	mprint("                       when exporting to bin format.  Only useful for\n");
	mprint("                       for debugging purposes.\n");
	mprint("          --parsedebug: Print debug info about the parsed container\n");
	mprint("                       file. (Only for TS/ASF files at the moment.)\n");
	mprint("            --parsePAT: Print Program Association Table dump.\n");
	mprint("            --parsePMT: Print Program Map Table dump.\n");
	mprint("              --dumpdef: Hex-dump defective TS packets.\n");
	mprint(" --investigate-packets: If no CC packets are detected based on the PMT, try\n");
	mprint("                       to find data in all packets by scanning.\n");
	mprint("\n");

	mprint("Teletext related options:\n");

	mprint("          --tpage page: Use this page for subtitles (if this parameter\n");
	mprint("                       is not used, try to autodetect). In Spain the\n");
	mprint("                       page is always 888, may vary in other countries.\n");
	mprint("            --tverbose: Enable verbose mode in the teletext decoder.\n\n");
	mprint("            --teletext: Force teletext mode even if teletext is not detected.\n");
	mprint("                       If used, you should also pass --datapid to specify\n");
	mprint("                       the stream ID you want to process.\n");
	mprint("          --no-teletext: Disable teletext processing. This might be needed\n");
	mprint("                       for video streams that have both teletext packets\n");
	mprint("                       and CEA-608/708 packets (if teletext is processed\n");
	mprint("                       then CEA-608/708 processing is disabled).\n");
	mprint("\n");

	mprint("Transcript customizing options:\n");

	mprint("    --customtxt format: Use the passed format to customize the (Timed) Transcript\n");
	mprint("                       output. The format must be like this: 1100100 (7 digits).\n");
	mprint("                       These indicate whether the next things should be\n");
	mprint("                       displayed or not in the (timed) transcript. They\n");
	mprint("                       represent (in order): \n");
	mprint("                           - Display start time\n");
	mprint("                           - Display end time\n");
	mprint("                           - Display caption mode\n");
	mprint("                           - Display caption channel\n");
	mprint("                           - Use a relative timestamp ( relative to the sample)\n");
	mprint("                           - Display XDS info\n");
	mprint("                           - Use colors\n");
	mprint("                       Examples:\n");
	mprint("                       0000101 is the default setting for transcripts\n");
	mprint("                       1110101 is the default for timed transcripts\n");
	mprint("                       1111001 is the default setting for -ucla\n");
	mprint("                       Make sure you use this parameter after others that might\n");
	mprint("                       affect these settings (-out, -ucla, -xds, -txt, \n");
	mprint("                       --ttxt ...)\n");

	mprint("\n");

	mprint("Communication with other programs and console output:\n");

	mprint("   --gui-mode-reports: Report progress and interesting events to stderr\n");
	mprint("                       in a easy to parse format. This is intended to be\n");
	mprint("                       used by other programs. See docs directory for.\n");
	mprint("                       details.\n");
	mprint("    --no-progress-bar: Suppress the output of the progress bar\n");
	mprint("               --quiet: Don't write any message.\n");
	mprint("\n");
	mprint("Burned-in subtitle extraction:\n");
	mprint("         --hardsubx : Enable the burned-in subtitle extraction subsystem.\n");
	mprint("\n");
	mprint("                      NOTE: This is needed to use the below burned-in \n");
	mprint("                		    subtitle extractor options\n");
	mprint("\n");
	mprint("       --tickertext : Search for burned-in ticker text at the bottom of\n");
	mprint("                     the screen.\n");
	mprint("\n");
	mprint("         --ocr-mode : Set the OCR mode to either frame-wise, word-wise\n");
	mprint("                     or letter wise.\n");
	mprint("                     e.g. --ocr-mode frame (default), --ocr-mode word, \n");
	mprint("                     --ocr-mode letter\n");
	mprint("\n");
	mprint("         --subcolor : Specify the color of the subtitles\n");
	mprint("                     Possible values are in the set \n");
	mprint("                     {white,yellow,green,cyan,blue,magenta,red}.\n");
	mprint("                     Alternatively, a custom hue value between 1 and 360 \n");
	mprint("                     may also be specified.\n");
	mprint("                     e.g. --subcolor white or --subcolor 270 (for violet).\n");
	mprint("                     Refer to an HSV color chart for values.\n");
	mprint("\n");
	mprint(" --min-sub-duration : Specify the minimum duration that a subtitle line \n");
	mprint("                     must exist on the screen.\n");
	mprint("                     The value is specified in seconds.\n");
	mprint("                     A lower value gives better results, but takes more \n");
	mprint("                     processing time.\n");
	mprint("                     The recommended value is 0.5 (default).\n");
	mprint("                     e.g. --min-sub-duration 1.0 (for a duration of 1 second)\n");
	mprint("\n");
	mprint("   --detect-italics : Specify whether italics are to be detected from the \n");
	mprint("                     OCR text.\n");
	mprint("                     Italic detection automatically enforces the OCR mode \n");
	mprint("                     to be word-wise");
	mprint("\n");
	mprint("      --conf-thresh : Specify the classifier confidence threshold between\n");
	mprint("                      1 and 100.\n");
	mprint("                     Try and use a threshold which works for you if you get \n");
	mprint("                     a lot of garbage text.\n");
	mprint("                     e.g. --conf-thresh 50\n");
	mprint("\n");
	mprint(" --whiteness-thresh : For white subtitles only, specify the luminance \n");
	mprint("                     threshold between 1 and 100\n");
	mprint("                     This threshold is content dependent, and adjusting\n");
	mprint("                     values may give you better results\n");
	mprint("                     Recommended values are in the range 80 to 100.\n");
	mprint("                     The default value is 95\n");
	mprint("\n");
	mprint("		--hcc	   : This option will be used if the file should have both\n");
	mprint("					 closed captions and burned in subtitles\n");
	mprint("            An example command for burned-in subtitle extraction is as follows:\n");
	mprint("               ccextractor video.mp4 --hardsubx -subcolor white --detect-italics \n");
	mprint("                   --whiteness-thresh 90 --conf-thresh 60\n");
	mprint("\n");
	mprint("\n         --version : Display current CCExtractor version and detailed information.\n");
	mprint("\n");
	mprint("Notes on File name related options:\n");
	mprint("  You can pass as many input files as you need. They will be processed in order.\n");
	mprint("  If a file name is suffixed by +, ccextractor will try to follow a numerical\n");
	mprint("  sequence. For example, DVD001.VOB+ means DVD001.VOB, DVD002.VOB and so on\n");
	mprint("  until there are no more files.\n");
	mprint("  Output will be one single file (either raw or srt). Use this if you made your\n");
	mprint("  recording in several cuts (to skip commercials for example) but you want one\n");
	mprint("  subtitle file with contiguous timing.\n");
	mprint("\n");
	mprint("Notes on Options that affect what will be processed:\n");
	mprint("  In general, if you want English subtitles you don't need to use these options\n");
	mprint("  as they are broadcast in field 1, channel 1. If you want the second language\n");
	mprint("  (usually Spanish) you may need to try -2, or -cc2, or both.\n");
	mprint("\n");
	mprint("Notes on Levenshtein distance:\n");
	mprint("  When processing teletext files CCExtractor tries to correct typos by\n");
	mprint("  comparing consecutive lines. If line N+1 is almost identical to line N except\n");
	mprint("  for minor changes (plus next characters) then it assumes that line N that a\n");
	mprint("  typo that was corrected in N+1. This is currently implemented in teletext\n");
	mprint("  because it's where samples files that could benefit from this were available.\n");
	mprint("  You can adjust, or disable, the algorithm settings with the following\n");
	mprint("  parameters.\n");
	mprint("\n");
	mprint("Notes on times:\n");
	mprint("  --startat and --endat times are used first, then -delay.\n");
	mprint("  So if you use --srt -startat 3:00 --endat 5:00 --delay 120000, ccextractor will\n");
	mprint("  generate a .srt file, with only data from 3:00 to 5:00 in the input file(s)\n");
	mprint("  and then add that (huge) delay, which would make the final file start at\n");
	mprint("  5:00 and end at 7:00.\n");
	mprint("\n");
	mprint("Notes on codec options:\n");
	mprint("  If codec type is not selected then first elementary stream suitable for\n");
	mprint("  subtitle is selected, please consider --teletext --noteletext override this\n");
	mprint("  option.\n");
	mprint("  no-codec and codec parameter must not be same if found to be same \n"
	       "  then parameter of no-codec is ignored, this flag should be passed \n"
	       "  once, more then one are not supported yet and last parameter would \n"
	       "  taken in consideration\n");
	mprint("\n");
	mprint("Notes on adding credits:\n");
	mprint("  CCExtractor can _try_ to add a custom message (for credits for example) at\n");
	mprint("  the start and end of the file, looking for a window where there are no\n");
	mprint("  captions. If there is no such window, then no text will be added.\n");
	mprint("  The start window must be between the times given and must have enough time\n");
	mprint("  to display the message for at least the specified time.\n");
	mprint("\n");
	mprint("Notes on the CEA-708 decoder:\n");
	mprint("  By default, ccextractor now extracts both CEA-608 and CEA-708 subtitles\n");
	mprint("  if they are present in the input. This results in two output files: one\n");
	mprint("  for CEA-608 and one for CEA-708.\n");
	mprint("  To extract only CEA-608 subtitles, use -1, -2, or -12.\n");
	mprint("  To extract only CEA-708 subtitles, use -svc.\n");
	mprint("  To extract both CEA-608 and CEA-708 subtitles, use both -1/-2/-12 and -svc.\n\n");
	mprint("  While it is starting to be useful, it's\n");
	mprint("  a work in progress. A number of things don't work yet in the decoder\n");
	mprint("  itself, and many of the auxiliary tools (case conversion to name one)\n");
	mprint("  won't do anything yet. Feel free to submit samples that cause problems\n");
	mprint("  and feature requests.\n");
	mprint("\n");
	mprint("Notes on spupng output format:\n");
	mprint("  One .xml file is created per output field. A set of .png files are created in\n");
	mprint("  a directory with the same base name as the corresponding .xml file(s), but with\n");
	mprint("  a .d extension. Each .png file will contain an image representing one caption\n");
	mprint("  and named subNNNN.png, starting with sub0000.png.\n");
	mprint("  For example, the command:\n");
	mprint("      ccextractor --out=spupng input.mpg\n");
	mprint("  will create the files:\n");
	mprint("      input.xml\n");
	mprint("      input.d/sub0000.png\n");
	mprint("      input.d/sub0001.png\n");
	mprint("      ...\n");
	mprint("  The command:\n");
	mprint("      ccextractor --out=spupng -o /tmp/output --output-field both input.mpg\n");
	mprint("  will create the files:\n");
	mprint("      /tmp/output_1.xml\n");
	mprint("      /tmp/output_1.d/sub0000.png\n");
	mprint("      /tmp/output_1.d/sub0001.png\n");
	mprint("      ...\n");
	mprint("      /tmp/output_2.xml\n");
	mprint("      /tmp/output_2.d/sub0000.png\n");
	mprint("      /tmp/output_2.d/sub0001.png\n");
	mprint("      ...");
	mprint("\n");
}

unsigned char sha256_buf[16384];

char *calculateSHA256(char *location)
{
	int size_read, bytes_read, fh = 0;
	SHA256_CTX ctx256;

	CC_SHA256_Init(&ctx256);

#ifdef _WIN32
	fh = OPEN(location, O_RDONLY | O_BINARY);
#else
	fh = OPEN(location, O_RDONLY);
#endif

	if (fh < 0)
	{
		return "Could not open file";
	}
	size_read = 0;
	while ((bytes_read = read(fh, sha256_buf, 16384)) > 0)
	{
		size_read += bytes_read;
		CC_SHA256_Update(&ctx256, (unsigned char *)sha256_buf, bytes_read);
	}
	close(fh);
	SHA256_End(&ctx256, sha256_buf);
	return sha256_buf;
}

void version(char *location)
{
	char *hash = calculateSHA256(location);
	mprint("CCExtractor detailed version info\n");
	mprint("	Version: %s\n", VERSION);
	mprint("	Git commit: %s\n", GIT_COMMIT);
	mprint("	Compilation date: %s\n", COMPILE_DATE);
	mprint("	CEA-708 decoder: \n");
	mprint("	File SHA256: %s\n", hash);

	mprint("Libraries used by CCExtractor\n");
#ifdef ENABLE_OCR
	mprint("	Tesseract Version: %s\n", (const char *)TessVersion());
	char *leptversion = getLeptonicaVersion();
	mprint("	Leptonica Version: %s\n", leptversion);
	lept_free(leptversion);
#endif // ENABLE_OCR
#ifdef GPAC_AVAILABLE
	mprint("	libGPAC Version: %s\n", GPAC_VERSION);
#endif
	mprint("	zlib: %s\n", ZLIB_VERSION);
	mprint("	utf8proc Version: %s\n", (const char *)utf8proc_version());
	mprint("	libpng Version: %s\n", PNG_LIBPNG_VER_STRING);
	mprint("	FreeType \n");
	mprint("	libhash\n");
	mprint("	nuklear\n");
	mprint("	libzvbi\n");
}

int atoi_hex(char *s)
{
	if (strlen(s) > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
	{
		// Hexadecimal
		return strtol(s + 2, NULL, 16);
	}
	else
	{
		return atoi(s);
	}
}
