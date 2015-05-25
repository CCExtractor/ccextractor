#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "utility.h"

static int inputfile_capacity=0;

static const char *DEF_VAL_STARTCREDITSNOTBEFORE="0";
static const char *DEF_VAL_STARTCREDITSNOTAFTER="5:00"; // To catch the theme after the teaser in TV shows
static const char *DEF_VAL_STARTCREDITSFORATLEAST="2";
static const char *DEF_VAL_STARTCREDITSFORATMOST="5";
static const char *DEF_VAL_ENDCREDITSFORATLEAST="2";
static const char *DEF_VAL_ENDCREDITSFORATMOST="5";

int stringztoms (const char *s, struct ccx_boundary_time *bt)
{
	unsigned ss=0, mm=0, hh=0;
	int value=-1;
	int colons=0;
	const char *c=s;
	while (*c)
	{
		if (*c==':')
		{
			if (value==-1) // : at the start, or ::, etc
				return -1;
			colons++;
			if (colons>2) // Max 2, for HH:MM:SS
				return -1;
			hh=mm;
			mm=ss;
			ss=value;
			value=-1;
		}
		else
		{
			if (!isdigit (*c)) // Only : or digits, so error
				return -1;
			if (value==-1)
				value=*c-'0';
			else
				value=value*10+*c-'0';
		}
		c++;
	}
	hh = mm;
	mm = ss;
	ss = value;
	if (mm > 59 || ss > 59)
		return -1;
	bt->set = 1;
	bt->hh = hh;
	bt->mm = mm;
	bt->ss = ss;
	LLONG secs = (hh * 3600 + mm * 60 + ss);
	bt->time_in_ms = secs*1000;
	return 0;
}
int process_cap_file (char *filename)
{
	int ret = 0;
	FILE *fi = fopen (filename,"rt");
	if (fi == NULL)
	{
		mprint ("\rUnable to open capitalization file: %s\n", filename);
		return -1;
	}
	char line[35]; // For screen width (32)+CRLF+0
	int num = 0;
	while (fgets (line,35,fi))
	{
		num++;
		if (line[0] == '#') // Comment
			continue;
		char *c = line+strlen (line)-1;
		while (c >= line && (*c == 0xd || *c == 0xa))
		{
			*c = 0;
			c--;
		}
		if (strlen (line)>32)
		{
			mprint ("Word in line %d too long, max = 32 characters.\n",num);
			ret = -1;
			goto end;
		}
		if (strlen (line)>0)
		{
			if (add_word (line))
			{
				ret = -1;
				goto end;
			}
		}
	}
end:
	fclose (fi);
	return ret;
}
int isanumber (char *s)
{
	while (*s)
	{
		if (!isdigit (*s))
			return 0;
		s++;
	}
	return 1;
}

int parsedelay (struct ccx_s_options *opt, char *par)
{
	int sign = 0;
	char *c = par;
	while (*c)
	{
		if (*c == '-' || *c == '+')
		{
			if (c != par) // Sign only at the beginning
				return 1;
			if (*c == '-')
				sign = 1;
		}
		else
		{
			if (!isdigit (*c))
				return 1;
			opt->subs_delay = opt->subs_delay*10 + (*c-'0');
		}
		c++;
	}
	if (sign)
		opt->subs_delay = -opt->subs_delay;
	return 0;
}

int append_file_to_queue (struct ccx_s_options *opt,char *filename)
{
	char *c=(char *) malloc (strlen (filename)+1);
	if (c==NULL)
		return -1;
	strcpy (c,filename);
	if (inputfile_capacity<=opt->num_input_files)
	{
		inputfile_capacity+=10;
		opt->inputfile=(char **) realloc (opt->inputfile,sizeof (char *) * inputfile_capacity);
		if (opt->inputfile==NULL)
		{
			free(c);
			return -1;
		}
	}
	opt->inputfile[opt->num_input_files]=c;
	opt->num_input_files++;
	return 0;
}

int add_file_sequence (struct ccx_s_options *opt, char *filename)
{
	int m,n;
	n = strlen (filename)-1;
	// Look for the last digit in filename
	while (n>=0 && !isdigit (filename[n]))
		n--;
	if (n==-1) // None. No expansion needed
		return append_file_to_queue(opt, filename);
	m=n;
	while (m>=0 && isdigit (filename[m]))
		m--;
	m++;
	// Here: Significant digits go from filename[m] to filename[n]
	char *num = (char *) malloc (n-m+2);
	if(!num)
		return -1;
	strncpy (num,filename+m, n-m+1);
	num[n-m+1]=0;
	int i = atoi (num);
	char *temp = (char *) malloc (n-m+3); // For overflows
	if(!temp)
	{
		free(num);
		return -1;
	}
	// printf ("Expanding %d to %d, initial value=%d\n",m,n,i);
	for (;;)
	{
		FILE *f=fopen (filename,"r");
		if (f==NULL) // Doesn't exist or we can't read it. We're done
			break;
		fclose (f);
		if (append_file_to_queue (opt, filename)) // Memory panic
		{
			free(num);
			free(temp);
			return -1;
		}
		i++;
		sprintf (temp,"%d",i);
		if (strlen (temp)>strlen (num)) // From 999 to 1000, etc.
			break;
		strncpy (filename+m+(strlen (num)-strlen (temp)),temp,strlen (temp));
		memset (filename+m,'0',strlen (num)-strlen (temp));
	}
	free(num);
	free(temp);
	return 0;
}

void set_output_format (struct ccx_s_options *opt, const char *format)
{
	while (*format=='-')
		format++;

	if (opt->send_to_srv && strcmp(format, "bin")!=0)
	{
		mprint("Output format is changed to bin\n");
		format = "bin";
	}

	if (strcmp (format,"srt")==0)
		opt->write_format=CCX_OF_SRT;
	else if (strcmp (format,"sami")==0 || strcmp (format,"smi")==0)
		opt->write_format=CCX_OF_SAMI;
	else if (strcmp (format,"transcript")==0 || strcmp (format,"txt")==0)
	{
		opt->write_format=CCX_OF_TRANSCRIPT;
	}
	else if (strcmp (format,"timedtranscript")==0 || strcmp (format,"ttxt")==0)
	{
		opt->write_format=CCX_OF_TRANSCRIPT;
		if (opt->date_format==ODF_NONE)
			opt->date_format=ODF_HHMMSSMS;
		// Sets the right things so that timestamps and the mode are printed.
		if (!opt->transcript_settings.isFinal){
			opt->transcript_settings.showStartTime = 1;
			opt->transcript_settings.showEndTime = 1;
			opt->transcript_settings.showCC = 0;
			opt->transcript_settings.showMode = 1;
		}
	}
	else if (strcmp (format,"report")==0)
	{
		opt->write_format=CCX_OF_NULL;
		opt->messages_target=0;
		opt->print_file_reports=1;
		opt->ts_autoprogram=1;
	}
	else if (strcmp (format,"raw")==0)
		opt->write_format=CCX_OF_RAW;
	else if (strcmp (format, "smptett")==0)
		opt->write_format=CCX_OF_SMPTETT ;
	else if (strcmp (format,"bin")==0)
		opt->write_format=CCX_OF_RCWT;
	else if (strcmp (format,"null")==0)
		opt->write_format=CCX_OF_NULL;
	else if (strcmp (format,"dvdraw")==0)
		opt->write_format=CCX_OF_DVDRAW;
	else if (strcmp (format,"spupng")==0)
		opt->write_format=CCX_OF_SPUPNG;
	else
		fatal (EXIT_MALFORMED_PARAMETER, "Unknown output file format: %s\n", format);
}

void set_input_format (struct ccx_s_options *opt, const char *format)
{
	if (opt->input_source == CCX_DS_TCP && strcmp(format, "bin")!=0)
	{
		mprint("Intput format is changed to bin\n");
		format = "bin";
	}

	while (*format=='-')
		format++;
	if (strcmp (format,"es")==0) // Does this actually do anything?
		opt->auto_stream = CCX_SM_ELEMENTARY_OR_NOT_FOUND;
	else if (strcmp(format, "ts") == 0)
	{
		opt->auto_stream = CCX_SM_TRANSPORT;
		opt->m2ts = 0;
	}	
	else if (strcmp(format, "m2ts") == 0)
	{
		opt->auto_stream = CCX_SM_TRANSPORT;
		opt->m2ts = 1;
	}
	else if (strcmp (format,"ps")==0 || strcmp (format,"nots")==0)
		opt->auto_stream = CCX_SM_PROGRAM;
	else if (strcmp (format,"asf")==0 || strcmp (format,"dvr-ms")==0)
		opt->auto_stream = CCX_SM_ASF;
	else if (strcmp (format,"wtv")==0)
		opt->auto_stream = CCX_SM_WTV;
	else if (strcmp (format,"raw")==0)
		opt->auto_stream = CCX_SM_MCPOODLESRAW;
	else if (strcmp (format,"bin")==0)
		opt->auto_stream = CCX_SM_RCWT;
	else if (strcmp (format,"mp4")==0)
		opt->auto_stream = CCX_SM_MP4;
#ifdef WTV_DEBUG
	else if (strcmp (format,"hex")==0)
		opt->auto_stream = CCX_SM_HEX_DUMP;
#endif
	else
		fatal (EXIT_MALFORMED_PARAMETER, "Unknown input file format: %s\n", format);
}

void usage (void)
{
	mprint ("Originally based on McPoodle's tools. Check his page for lots of information\n");
	mprint ("on closed captions technical details.\n");
	mprint ("(http://www.theneitherworld.com/mcpoodle/SCC_TOOLS/DOCS/SCC_TOOLS.HTML)\n\n");
	mprint ("This tool home page:\n");
	mprint ("http://www.ccextractor.org\n");
	mprint ("  Extracts closed captions and teletext subtitles from video streams.\n");
	mprint ("    (DVB, .TS, ReplayTV 4000 and 5000, dvr-ms, bttv, Tivo, Dish Network,\n");
	mprint ("     .mp4, HDHomeRun are known to work).\n\n");
	mprint ("  Syntax:\n");
	mprint ("  ccextractor [options] inputfile1 [inputfile2...] [-o outputfilename]\n");
	mprint ("               [-o1 outputfilename1] [-o2 outputfilename2]\n\n");
	mprint ("File name related options:\n");
	mprint ("            inputfile: file(s) to process\n");
	mprint ("    -o outputfilename: Use -o parameters to define output filename if you don't\n");
	mprint ("                       like the default ones (same as infile plus _1 or _2 when\n");
	mprint ("                       needed and file extension, e.g. .srt).\n");
	mprint ("                           -o or -o1 -> Name of the first (maybe only) output\n");
	mprint ("                                        file.\n");
	mprint ("                           -o2       -> Name of the second output file, when\n");
	mprint ("                                        it applies.\n");
	mprint ("         -cf filename: Write 'clean' data to a file. Cleans means the ES\n");
	mprint ("                       without TS or PES headers.\n");
	mprint ("              -stdout: Write output to stdout (console) instead of file. If\n");
	mprint ("                       stdout is used, then -o, -o1 and -o2 can't be used. Also\n");
	mprint ("                       -stdout will redirect all messages to stderr (error).\n\n");
	mprint ("               -stdin: Reads input from stdin (console) instead of file.\n");
	mprint ("You can pass as many input files as you need. They will be processed in order.\n");
	mprint ("If a file name is suffixed by +, ccextractor will try to follow a numerical\n");
	mprint ("sequence. For example, DVD001.VOB+ means DVD001.VOB, DVD002.VOB and so on\n");
	mprint ("until there are no more files.\n");
	mprint ("Output will be one single file (either raw or srt). Use this if you made your\n");
	mprint ("recording in several cuts (to skip commercials for example) but you want one\n");
	mprint ("subtitle file with contiguous timing.\n\n");
	mprint ("Network support:\n");
	mprint ("            -udp port: Read the input via UDP (listening in the specified port)\n");
	mprint ("                       instead of reading a file.\n\n");
	mprint ("            -udp [host:]port: Read the input via UDP (listening in the specified\n");
	mprint ("                              port) instead of reading a file. Host can be a\n");
	mprint ("                              hostname or IPv4 address. If host is not specified\n");
	mprint ("                              then listens on the local host.\n\n");
	mprint ("            -sendto host[:port]: Sends data in BIN format to the server according\n");
	mprint ("                                 to the CCExtractor's protocol over TCP. For IPv6\n");
	mprint ("                                 use [addres]:port\n");
	mprint ("            -tcp port: Reads the input data in BIN format according to CCExtractor's\n");
	mprint ("                       protocol, listening specified port on the local host\n");
	mprint ("            -tcppassword password: Sets server password for new connections to tcp server\n");
	mprint ("            -tcpdesc description: Sends to the server short description about captions e.g.\n");
	mprint ("                                  channel name or file name\n");
	mprint ("Options that affect what will be processed:\n");
	mprint ("          -1, -2, -12: Output Field 1 data, Field 2 data, or both\n");
	mprint ("                       (DEFAULT is -1)\n");
	mprint ("                 -cc2: When in srt/sami mode, process captions in channel 2\n");
	mprint ("                       instead of channel 1.\n");
	mprint ("-svc --service N,N...: Enabled CEA-708 captions processing for the listed\n");
	mprint ("                       services. The parameter is a command delimited list\n");
	mprint ("                       of services numbers, such as \"1,2\" to process the\n");
	mprint ("                       primary and secondary language services.\n");
	mprint ("In general, if you want English subtitles you don't need to use these options\n");
	mprint ("as they are broadcast in field 1, channel 1. If you want the second language\n");
	mprint ("(usually Spanish) you may need to try -2, or -cc2, or both.\n\n");
	mprint ("Input formats:\n");
	mprint ("       With the exception of McPoodle's raw format, which is just the closed\n");
	mprint ("       caption data with no other info, CCExtractor can usually detect the\n");
	mprint ("       input format correctly. To force a specific format:\n\n");
	mprint ("                  -in=format\n\n");
	mprint ("       where format is one of these:\n");
	mprint ("                       ts   -> For Transport Streams.\n");
	mprint ("                       ps   -> For Program Streams.\n");
	mprint ("                       es   -> For Elementary Streams.\n");
	mprint ("                       asf  -> ASF container (such as DVR-MS).\n");
	mprint ("                       wtv  -> Windows Television (WTV)\n");
	mprint ("                       bin  -> CCExtractor's own binary format.\n");
	mprint ("                       raw  -> For McPoodle's raw files.\n");
	mprint ("                       mp4  -> MP4/MOV/M4V and similar.\n");
#ifdef WTV_DEBUG
	mprint ("                       hex  -> Hexadecimal dump as generated by wtvccdump.\n");
#endif
	mprint ("       -ts, -ps, -es, -mp4, -wtv and -asf (or --dvr-ms) can be used as shorts.\n\n");
	mprint ("Output formats:\n\n");
	mprint ("                 -out=format\n\n");
	mprint ("       where format is one of these:\n");
	mprint ("                      srt     -> SubRip (default, so not actually needed).\n");
	mprint ("                      sami    -> MS Synchronized Accesible Media Interface.\n");
	mprint ("                      bin     -> CC data in CCExtractor's own binary format.\n");
	mprint ("                      raw     -> CC data in McPoodle's Broadcast format.\n");
	mprint ("                      dvdraw  -> CC data in McPoodle's DVD format.\n");
	mprint ("                      txt     -> Transcript (no time codes, no roll-up\n");
	mprint ("                                 captions, just the plain transcription.\n");
	mprint ("                      ttxt    -> Timed Transcript (transcription with time\n");
	mprint ("                                 info)\n");
	mprint ("                      smptett -> SMPTE Timed Text (W3C TTML) format.\n");
	mprint ("                      spupng  -> Set of .xml and .png files for use with\n");
	mprint ("                                 dvdauthor's spumux.\n");
	mprint ("                                 See \"Notes on spupng output format\"\n");
	mprint ("                      null    -> Don't produce any file output\n");
	mprint ("                      report  -> Prints to stdout information about captions\n");
	mprint ("                                 in specified input. Don't produce any file\n");
	mprint ("                                 output\n\n");
	mprint ("       Note: Teletext output can only be srt, txt or ttxt for now.\n\n");

	mprint ("Options that affect how input files will be processed.\n");

	mprint ("        -gt --goptime: Use GOP for timing instead of PTS. This only applies\n");
	mprint ("                       to Program or Transport Streams with MPEG2 data and\n");
	mprint ("                       overrides the default PTS timing.\n");
	mprint ("                       GOP timing is always used for Elementary Streams.\n");
	mprint ("    -nogt --nogoptime: Never use GOP timing (use PTS), even if ccextractor\n");
	mprint ("                       detects GOP timing is the reasonable choice.\n");
	mprint ("     -fp --fixpadding: Fix padding - some cards (or providers, or whatever)\n");
	mprint ("                       seem to send 0000 as CC padding instead of 8080. If you\n");
	mprint ("                       get bad timing, this might solve it.\n");
	mprint ("               -90090: Use 90090 (instead of 90000) as MPEG clock frequency.\n");
	mprint ("                       (reported to be needed at least by Panasonic DMR-ES15\n");
	mprint ("                       DVD Recorder)\n");
	mprint ("    -ve --videoedited: By default, ccextractor will process input files in\n");
	mprint ("                       sequence as if they were all one large file (i.e.\n");
	mprint ("                       split by a generic, non video-aware tool. If you\n");
	mprint ("                       are processing video hat was split with a editing\n");
	mprint ("                       tool, use -ve so ccextractor doesn't try to rebuild\n");
	mprint ("                       the original timing.\n");
	mprint ("   -s --stream [secs]: Consider the file as a continuous stream that is\n");
	mprint ("                       growing as ccextractor processes it, so don't try\n");
	mprint ("                       to figure out its size and don't terminate processing\n");
	mprint ("                       when reaching the current end (i.e. wait for more\n");
	mprint ("                       data to arrive). If the optional parameter secs is\n");
	mprint ("                       present, it means the number of seconds without any\n");
	mprint ("                       new data after which ccextractor should exit. Use\n");
	mprint ("                       this parameter if you want to process a live stream\n");
	mprint ("                       but not kill ccextractor externally.\n");
	mprint ("                       Note: If -s is used then only one input file is\n");
	mprint ("                       allowed.\n");
	mprint ("  -poc  --usepicorder: Use the pic_order_cnt_lsb in AVC/H.264 data streams\n");
	mprint ("                       to order the CC information.  The default way is to\n");
	mprint ("                       use the PTS information.  Use this switch only when\n");
	mprint ("                       needed.\n");
	mprint ("                -myth: Force MythTV code branch.\n");
	mprint ("              -nomyth: Disable MythTV code branch.\n");
	mprint ("                       The MythTV branch is needed for analog captures where\n");
	mprint ("                       the closed caption data is stored in the VBI, such as\n");
	mprint ("                       those with bttv cards (Hauppage 250 for example). This\n");
	mprint ("                       is detected automatically so you don't need to worry\n");
	mprint ("                       about this unless autodetection doesn't work for you.\n");
	mprint ("       -wtvconvertfix: This switch works around a bug in Windows 7's built in\n");
	mprint ("                       software to convert *.wtv to *.dvr-ms. For analog NTSC\n");
	mprint ("                       recordings the CC information is marked as digital\n");
	mprint ("                       captions. Use this switch only when needed.\n");
	mprint ("            -wtvmpeg2: Read the captions from the MPEG2 video stream rather\n");
	mprint ("                       than the captions stream in WTV files\n");
	mprint (" -pn --program-number: In TS mode, specifically select a program to process.\n");
	mprint ("                       Not needed if the TS only has one. If this parameter\n");
	mprint ("                       is not specified and CCExtractor detects more than one\n");
	mprint ("                       program in the input, it will list the programs found\n");
	mprint ("                       and terminate without doing anything, unless\n");
	mprint ("                       -autoprogram (see below) is used.\n");
	mprint ("         -autoprogram: If there's more than one program in the stream, just use\n");
	mprint ("                       the first one we find that contains a suitable stream.\n");
	mprint ("             -datapid: Don't try to find out the stream for caption/teletext\n");
	mprint ("                       data, just use this one instead.\n");
	mprint ("      -datastreamtype: Instead of selecting the stream by its PID, select it\n");
	mprint ("                       by its type (pick the stream that has this type in\n");
	mprint ("                       the PMT)\n");
	mprint ("          -streamtype: Assume the data is of this type, don't autodetect. This\n");
	mprint ("                       parameter may be needed if -datapid or -datastreamtype\n");
	mprint ("                       is used and CCExtractor cannot determine how to process\n");
	mprint ("                       the stream. The value will usually be 2 (MPEG video) or\n");
	mprint ("                       6 (MPEG private data).\n");
	mprint ("    -haup --hauppauge: If the video was recorder using a Hauppauge card, it\n");
	mprint ("                       might need special processing. This parameter will\n");
	mprint ("                       force the special treatment.\n");
	mprint ("         -mp4vidtrack: In MP4 files the closed caption data can be embedded in\n");
	mprint ("                       the video track or in a dedicated CC track. If a\n");
	mprint ("                       dedicated track is detected it will be processed instead\n");
	mprint ("                       of the video track. If you need to force the video track\n");
	mprint ("                       to be processed instead use this option.\n");
	mprint ("       -noautotimeref: Some streams come with broadcast date information. When\n");
	mprint ("                       such data is available, CCExtractor will set its time\n");
	mprint ("                       reference to the received data. Use this parameter if\n");
	mprint ("                       you prefer your own reference. Note: Current this only\n");
	mprint ("                       affects Teletext in timed transcript with -datets.\n");
	mprint ("\n");
	mprint ("Options that affect what kind of output will be produced:\n");
	mprint("				  -bom: Append a BOM (Byte Order Mark) to output files.");
	mprint("						Note that most text processing tools in linux will not");
	mprint("						like BOM.");
	mprint("						This is the default in Windows builds.");
	mprint("				-nobom: Do not append a BOM (Byte Order Mark) to output files.");
	mprint("						Note that this may break files when using Windows.");
	mprint("						This is the default in non-Windows builds.");
	mprint ("             -unicode: Encode subtitles in Unicode instead of Latin-1.\n");
	mprint ("                -utf8: Encode subtitles in UTF-8 (no longer needed.\n");
	mprint ("                       because UTF-8 is now the default).\n");
	mprint ("              -latin1: Encode subtitles in UTF-8 instead of Latin-1\n");
	mprint ("  -nofc --nofontcolor: For .srt/.sami, don't add font color tags.\n");
	mprint ("-nots --notypesetting: For .srt/.sami, don't add typesetting tags.\n");
	mprint ("                -trim: Trim lines.\n");
	mprint ("   -dc --defaultcolor: Select a different default color (instead of\n");
	mprint ("                       white). This causes all output in .srt/.smi\n");
	mprint ("                       files to have a font tag, which makes the files\n");
	mprint ("                       larger. Add the color you want in RGB, such as\n");
	mprint ("                       -dc #FF0000 for red.\n");
	mprint ("    -sc --sentencecap: Sentence capitalization. Use if you hate\n");
	mprint ("                       ALL CAPS in subtitles.\n");
	mprint ("  --capfile -caf file: Add the contents of 'file' to the list of words\n");
	mprint ("                       that must be capitalized. For example, if file\n");
	mprint ("                       is a plain text file that contains\n\n");
	mprint ("                       Tony\n");
	mprint ("                       Alan\n\n");
	mprint ("                       Whenever those words are found they will be written\n");
	mprint ("                       exactly as they appear in the file.\n");
	mprint ("                       Use one line per word. Lines starting with # are\n");
	mprint ("                       considered comments and discarded.\n\n");
	mprint ("          -unixts REF: For timed transcripts that have an absolute date\n");
	mprint ("                       instead of a timestamp relative to the file start), use\n");
	mprint ("                       this time reference (UNIX timestamp). 0 => Use current\n");
	mprint ("                       system time.\n");
	mprint ("                       ccextractor will automatically switch to transport\n");
	mprint ("                       stream UTC timestamps when available.\n");
	mprint ("              -datets: In transcripts, write time as YYYYMMDDHHMMss,ms.\n");
	mprint ("               -sects: In transcripts, write time as ss,ms\n");
	mprint ("                -UCLA: Transcripts are generated with a specific format\n");
	mprint ("                       that is convenient for a specific project, feel\n");
	mprint ("                       free to play with it but be aware that this format\n");
	mprint ("                       is really live - don't rely on its output format\n");
	mprint ("                       not changing between versions.\n");
	mprint ("                  -lf: Use LF (UNIX) instead of CRLF (DOS, Windows) as line\n");
	mprint ("                       terminator.\n");
	mprint ("            -autodash: Based on position on screen, attempt to determine\n");
	mprint ("                       the different speakers and a dash (-) when each\n");
	mprint ("                       of them talks (.srt only, -trim required).\n");
	mprint ("              -xmltv:  produce an XMLTV file containing the EPG data from\n");
	mprint ("                       the source TS file.\n\n");

	mprint ("Options that affect how ccextractor reads and writes (buffering):\n");

	mprint ("    -bi --bufferinput: Forces input buffering.\n");
	mprint (" -nobi -nobufferinput: Disables input buffering.\n");
	mprint (" -bs --buffersize val: Specify a size for reading, in bytes (suffix with K or\n");
	mprint ("                       or M for kilobytes and megabytes). Default is 16M.\n");
	mprint ("\n");
	mprint ("Note: -bo is only used when writing raw files, not .srt or .sami\n\n");

	mprint ("Options that affect the built-in closed caption decoder:\n");

	mprint ("                 -dru: Direct Roll-Up. When in roll-up mode, write character by\n");
	mprint ("                       character instead of line by line. Note that this\n");
	mprint ("                       produces (much) larger files.\n");
	mprint ("     -noru --norollup: If you hate the repeated lines caused by the roll-up\n");
	mprint ("                       emulation, you can have ccextractor write only one\n");
	mprint ("                       line at a time, getting rid of these repeated lines.\n");
	mprint ("     -ru1 / ru2 / ru3: roll-up captions can consist of 2, 3 or 4 visible\n");
	mprint ("                       lines at any time (the number of lines is part of\n");
	mprint ("                       the transmission). If having 3 or 4 lines annoys\n");
	mprint ("                       you you can use -ru to force the decoder to always\n");
	mprint ("                       use 1, 2 or 3 lines. Note that 1 line is not\n");
	mprint ("                       a real mode rollup mode, so CCExtractor does what\n");
	mprint ("                       it can.\n");
	mprint ("                       In -ru1 the start timestamp is actually the timestamp\n");
	mprint ("                       of the first character received which is possibly more\n");
	mprint ("                       accurate.\n");
	mprint ("\n");

	mprint ("Options that affect timing:\n");

	mprint ("            -delay ms: For srt/sami, add this number of milliseconds to\n");
	mprint ("                       all times. For example, -delay 400 makes subtitles\n");
	mprint ("                       appear 400ms late. You can also use negative numbers\n");
	mprint ("                       to make subs appear early.\n");
	mprint ("Notes on times: -startat and -endat times are used first, then -delay.\n");
	mprint ("So if you use -srt -startat 3:00 -endat 5:00 -delay 120000, ccextractor will\n");
	mprint ("generate a .srt file, with only data from 3:00 to 5:00 in the input file(s)\n");
	mprint ("and then add that (huge) delay, which would make the final file start at\n");
	mprint ("5:00 and end at 7:00.\n\n");

	mprint ("Options that affect what segment of the input file(s) to process:\n");

	mprint ("        -startat time: Only write caption information that starts after the\n");
	mprint ("                       given time.\n");
	mprint ("                       Time can be seconds, MM:SS or HH:MM:SS.\n");
	mprint ("                       For example, -startat 3:00 means 'start writing from\n");
	mprint ("                       minute 3.\n");
	mprint ("          -endat time: Stop processing after the given time (same format as\n");
	mprint ("                       -startat).\n");
	mprint ("                       The -startat and -endat options are honored in all\n");
	mprint ("                       output formats.  In all formats with timing information\n");
	mprint ("                       the times are unchanged.\n");
	mprint ("-scr --screenfuls num: Write 'num' screenfuls and terminate processing.\n\n");

	mprint ("Options that affect which codec is to be used have to be searched in input\n");

	mprint ("  If codec type is not selected then first elementry stream suitable for \n"
			"  subtitle is selected, please consider -teletext -noteletext override this\n"
			"  option.\n"
			"      -codec dvbsub    select the dvb subtitle from all elementry stream,\n"
			"                        if stream of dvb subtitle type is not found then \n"
			"                        nothing is selected and no subtitle is generated\n"
			"      -nocodec dvbsub   ignore dvb subtitle and follow default behaviour\n"
			"      -codec teletext   select the teletext subtitle from elementry stream\n"
			"      -nocodec teletext ignore teletext subtitle\n"
			"  NOTE: option given in form -foo=bar ,-foo = bar and --foo=bar are invalid\n"
			"        valid option are only in form -foo bar\n"
			"        nocodec and codec parameter must not be same if found to be same \n"
			"        then parameter of nocodec is ignored, this flag should be passed \n"
			"        once more then one are not supported yet and last parameter would \n"
			"        taken in consideration\n");

	mprint ("Adding start and end credits:\n");

	mprint ("  CCExtractor can _try_ to add a custom message (for credits for example) at\n");
	mprint ("  the start and end of the file, looking for a window where there are no\n");
	mprint ("  captions. If there is no such window, then no text will be added.\n");
	mprint ("  The start window must be between the times given and must have enough time\n");
	mprint ("  to display the message for at least the specified time.\n");
	mprint ("        --startcreditstext txt: Write this text as start credits. If there are\n");
	mprint ("                                several lines, separate them with the\n");
	mprint ("                                characters \\n, for example Line1\\nLine 2.\n");
	mprint ("  --startcreditsnotbefore time: Don't display the start credits before this\n");
	mprint ("                                time (S, or MM:SS). Default: %s\n", DEF_VAL_STARTCREDITSNOTBEFORE);
	mprint ("   --startcreditsnotafter time: Don't display the start credits after this\n");
	mprint ("                                time (S, or MM:SS). Default: %s\n", DEF_VAL_STARTCREDITSNOTAFTER);
	mprint (" --startcreditsforatleast time: Start credits need to be displayed for at least\n");
	mprint ("                                this time (S, or MM:SS). Default: %s\n", DEF_VAL_STARTCREDITSFORATLEAST);
	mprint ("  --startcreditsforatmost time: Start credits should be displayed for at most\n");
	mprint ("                                this time (S, or MM:SS). Default: %s\n", DEF_VAL_STARTCREDITSFORATMOST);
	mprint ("          --endcreditstext txt: Write this text as end credits. If there are\n");
	mprint ("                                several lines, separate them with the\n");
	mprint ("                                characters \\n, for example Line1\\nLine 2.\n");
	mprint ("   --endcreditsforatleast time: End credits need to be displayed for at least\n");
	mprint ("                                this time (S, or MM:SS). Default: %s\n", DEF_VAL_ENDCREDITSFORATLEAST);
	mprint ("    --endcreditsforatmost time: End credits should be displayed for at most\n");
	mprint ("                                this time (S, or MM:SS). Default: %s\n", DEF_VAL_ENDCREDITSFORATMOST);
	mprint ("\n");

	mprint ("Options that affect debug data:\n");

	mprint ("               -debug: Show lots of debugging output.\n");
	mprint ("                 -608: Print debug traces from the EIA-608 decoder.\n");
	mprint ("                       If you need to submit a bug report, please send\n");
	mprint ("                       the output from this option.\n");
	mprint ("                 -708: Print debug information from the (currently\n");
	mprint ("                       in development) EIA-708 (DTV) decoder.\n");
	mprint ("              -goppts: Enable lots of time stamp output.\n");
	mprint ("            -xdsdebug: Enable XDS debug data (lots of it).\n");
	mprint ("               -vides: Print debug info about the analysed elementary\n");
	mprint ("                       video stream.\n");
	mprint ("               -cbraw: Print debug trace with the raw 608/708 data with\n");
	mprint ("                       time stamps.\n");
	mprint ("              -nosync: Disable the syncing code.  Only useful for debugging\n");
	mprint ("                       purposes.\n");
	mprint ("             -fullbin: Disable the removal of trailing padding blocks\n");
	mprint ("                       when exporting to bin format.  Only useful for\n");
	mprint ("                       for debugging purposes.\n");
	mprint ("          -parsedebug: Print debug info about the parsed container\n");
	mprint ("                       file. (Only for TS/ASF files at the moment.)\n");
	mprint ("            -parsePAT: Print Program Association Table dump.\n");
	mprint ("            -parsePMT: Print Program Map Table dump.\n");
	mprint (" -investigate_packets: If no CC packets are detected based on the PMT, try\n");
	mprint ("                       to find data in all packets by scanning.\n\n");

	mprint ("Teletext related options:\n");

	mprint ("          -tpage page: Use this page for subtitles (if this parameter\n");
	mprint ("                       is not used, try to autodetect). In Spain the\n");
	mprint ("                       page is always 888, may vary in other countries.\n");
	mprint ("            -tverbose: Enable verbose mode in the teletext decoder.\n\n");
	mprint ("            -teletext: Force teletext mode even if teletext is not detected.\n");
	mprint ("                       If used, you should also pass -datapid to specify\n");
	mprint ("                       the stream ID you want to process.\n");
	mprint ("          -noteletext: Disable teletext processing. This might be needed\n");
	mprint ("                       for video streams that have both teletext packets\n");
	mprint ("                       and CEA-608/708 packets (if teletext is processed\n");
	mprint ("                       then CEA-608/708 processing is disabled).\n");
	mprint ("\n");

	mprint("Transcript customizing options:\n");

	mprint("    -customtxt format: Use the passed format to customize the (Timed) Transcript\n");
	mprint("                       output. The format must be like this: 1100100 (7 digits).\n");
	mprint("                       These indicate whether the next things should be displayed\n");
	mprint("                       or not in the (timed) transcript. They represent (in order):\n");
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
	mprint("                       affect these settings (-out, -ucla, -xds, -txt, -ttxt, ...)\n");

	mprint("\n");

	mprint ("Communication with other programs and console output:\n");

	mprint ("   --gui_mode_reports: Report progress and interesting events to stderr\n");
	mprint ("                       in a easy to parse format. This is intended to be\n");
	mprint ("                       used by other programs. See docs directory for.\n");
	mprint ("                       details.\n");
	mprint ("    --no_progress_bar: Suppress the output of the progress bar\n");
	mprint ("               -quiet: Don't write any message.\n");
	mprint ("\n");
	mprint ("Notes on the CEA-708 decoder: While it is starting to be useful, it's\n");
	mprint ("a work in progress. A number of things don't work yet in the decoder\n");
	mprint ("itself, and many of the auxiliary tools (case conversion to name one)\n");
	mprint ("won't do anything yet. Feel free to submit samples that cause problems\n");
	mprint ("and feature requests.\n");
	mprint ("\n");
	mprint("Notes on spupng output format:\n");
	mprint("One .xml file is created per output field. A set of .png files are created in\n");
	mprint("a directory with the same base name as the corresponding .xml file(s), but with\n");
	mprint("a .d extension. Each .png file will contain an image representing one caption\n");
	mprint("and named subNNNN.png, starting with sub0000.png.\n");
	mprint("For example, the command:\n");
	mprint("    ccextractor -out=spupng input.mpg\n");
	mprint("will create the files:\n");
	mprint("    input.xml\n");
	mprint("    input.d/sub0000.png\n");
	mprint("    input.d/sub0001.png\n");
	mprint("    ...\n");
	mprint("The command:\n");
	mprint("    ccextractor -out=spupng -o /tmp/output -12 input.mpg\n");
	mprint("will create the files:\n");
	mprint("    /tmp/output_1.xml\n");
	mprint("    /tmp/output_1.d/sub0000.png\n");
	mprint("    /tmp/output_1.d/sub0001.png\n");
	mprint("    ...\n");
	mprint("    /tmp/output_2.xml\n");
	mprint("    /tmp/output_2.d/sub0000.png\n");
	mprint("    /tmp/output_2.d/sub0001.png\n");
	mprint("    ...\n");
}

void parse_708services (char *s)
{
	char *c, *e, *l;
	if (s==NULL)
		return;
	l=s+strlen (s);
	for (c=s; c<l && *c; )
	{
		int svc=-1;
		while (*c && !isdigit (*c))
			c++;
		if (!*c) // We're done
			break;
		e=c;
		while (isdigit (*e))
			e++;
		*e=0;
		svc=atoi (c);
		if (svc<1 || svc>CCX_DECODERS_708_MAX_SERVICES)
			fatal (EXIT_MALFORMED_PARAMETER, "Invalid service number (%d), valid range is 1-%d.",svc,CCX_DECODERS_708_MAX_SERVICES);
		cea708services[svc-1]=1;
		do_cea708=1;
		c=e+1;
	}
}

long atol_size (char *s)
{
	long val=atoi (s);
	if (toupper (s[strlen (s)-1])=='M')
		val=val*1024*1024;
	else if (toupper (s[strlen (s)-1])=='K')
		val=val*1024;
	return val;
}

int atoi_hex (char *s)
{
	if (strlen (s)>2 && s[0]=='0' && (s[1]=='x' || s[1]=='X'))
	{
		// Hexadecimal
		return strtol(s+2, NULL, 16);
	}
	else
	{
		return atoi (s);
	}
}

void parse_parameters (struct ccx_s_options *opt, int argc, char *argv[])
{
	char *cea708_service_list=NULL; // List CEA-708 services

	// Sensible default values for credits
	stringztoms (DEF_VAL_STARTCREDITSNOTBEFORE, &opt->startcreditsnotbefore);
	stringztoms (DEF_VAL_STARTCREDITSNOTAFTER, &opt->startcreditsnotafter);
	stringztoms (DEF_VAL_STARTCREDITSFORATLEAST, &opt->startcreditsforatleast);
	stringztoms (DEF_VAL_STARTCREDITSFORATMOST, &opt->startcreditsforatmost);
	stringztoms (DEF_VAL_ENDCREDITSFORATLEAST, &opt->endcreditsforatleast);
	stringztoms (DEF_VAL_ENDCREDITSFORATMOST, &opt->endcreditsforatmost);

	// Parse parameters
	for (int i=1; i<argc; i++)
	{
		if (strcmp (argv[i], "-")==0 || strcmp(argv[i], "-stdin") == 0)
		{

			opt->input_source=CCX_DS_STDIN;
			opt->live_stream=-1;
			continue;
		}
		if (argv[i][0]!='-')
		{
			int rc;
			if (argv[i][strlen (argv[i])-1]!='+')
			{
				rc=append_file_to_queue (opt, argv[i]);
			}
			else
			{
				argv[i][strlen (argv[i])-1]=0;
				rc=add_file_sequence (opt, argv[i]);
			}
			if (rc)
			{
				fatal (EXIT_NOT_ENOUGH_MEMORY, "Fatal: Not enough memory.\n");
			}
			continue;
		}
		if (strcmp (argv[i],"-bi")==0 ||
				strcmp (argv[i],"--bufferinput")==0)
		{
			opt->buffer_input = 1;
			continue;
		}
		if (strcmp (argv[i],"-nobi")==0 ||
				strcmp (argv[i],"--nobufferinput")==0)
		{
			opt->buffer_input = 0;
			continue;
		}
		if ((strcmp (argv[i],"-bs")==0 || strcmp (argv[i],"--buffersize")==0) && i<argc-1)
		{
			FILEBUFFERSIZE = atol_size(argv[i+1]);
			if (FILEBUFFERSIZE<8)
				FILEBUFFERSIZE=8; // Otherwise crashes are guaranteed at least in MythTV
			i++;
			continue;
		}
		if (strcmp (argv[i],"-dru")==0)
		{
			opt->settings_608.direct_rollup = 1;
			continue;
		}
		if (strcmp (argv[i],"-nofc")==0 ||
				strcmp (argv[i],"--nofontcolor")==0)
		{
			opt->nofontcolor=1;
			continue;
		}
		if (strcmp(argv[i], "-bom") == 0){
			opt->no_bom = 0;
			continue;
		}
		if (strcmp(argv[i], "-nobom") == 0){
			opt->no_bom = 1;
			continue;
		}
		if (strcmp (argv[i],"-nots")==0 ||
				strcmp (argv[i],"--notypesetting")==0)
		{
			opt->notypesetting=1;
			continue;
		}

		/* Input file formats */
		if ( strcmp (argv[i],"-es")==0 ||
			strcmp (argv[i],"-ts")==0 ||
			strcmp (argv[i],"-ps")==0 ||
			strcmp (argv[i],"-nots")==0 ||
			strcmp (argv[i],"-asf")==0 ||
			strcmp (argv[i],"-wtv")==0 ||
			strcmp (argv[i],"-mp4")==0 ||
			strcmp (argv[i],"--dvr-ms")==0 )
		{
			set_input_format (opt, argv[i]);
			continue;
		}
		if (strncmp (argv[i],"-in=", 4)==0)
		{
			set_input_format (opt, argv[i]+4);
			continue;
		}

		/*user specified subtitle to be selected */

		if(!strcmp (argv[i],"-codec"))
		{
			i++;
			if(!strcmp (argv[i],"teletext"))
			{
				opt->codec = CCX_CODEC_TELETEXT;
			}
			else if(!strcmp (argv[i],"dvbsub"))
			{
				opt->codec = CCX_CODEC_DVB;
			}
			else
			{
				mprint("Invalid option for codec %s\n",argv[i]);
			}
			continue;
		}
		/*user specified subtitle to be selected */

		if(!strcmp (argv[i],"-nocodec"))
		{
			i++;
			if(!strcmp (argv[i],"teletext"))
			{
				opt->nocodec = CCX_CODEC_TELETEXT;
			}
			else if(!strcmp (argv[i],"dvbsub"))
			{
				opt->nocodec = CCX_CODEC_DVB;
			}
			else
			{
				mprint("Invalid option for codec %s\n",argv[i]);
			}
			continue;
		}
		/* Output file formats */
		if (strcmp (argv[i],"-srt")==0 ||
				strcmp (argv[i],"-dvdraw")==0 ||
				strcmp (argv[i],"-sami")==0 || strcmp (argv[i],"-smi")==0 ||
				strcmp (argv[i],"--transcript")==0 || strcmp (argv[i],"-txt")==0 ||
				strcmp (argv[i],"--timedtranscript")==0 || strcmp (argv[i],"-ttxt")==0 ||
				strcmp (argv[i],"-null")==0)
		{
			set_output_format (opt, argv[i]);
			continue;
		}
		if (strncmp (argv[i],"-out=", 5)==0)
		{
			set_output_format (opt, argv[i]+5);
			continue;
		}

		/* Credit stuff */
		if ((strcmp (argv[i],"--startcreditstext")==0)
				&& i<argc-1)
		{
			opt->start_credits_text=argv[i+1];
			i++;
			continue;
		}
		if ((strcmp (argv[i],"--startcreditsnotbefore")==0)
				&& i<argc-1)
		{
			if (stringztoms (argv[i+1],&opt->startcreditsnotbefore)==-1)
			{
				fatal (EXIT_MALFORMED_PARAMETER, "--startcreditsnotbefore only accepts SS, MM:SS or HH:MM:SS\n");
			}
			i++;
			continue;
		}
		if ((strcmp (argv[i],"--startcreditsnotafter")==0)
				&& i<argc-1)
		{
			if (stringztoms (argv[i+1],&opt->startcreditsnotafter)==-1)
			{
				fatal (EXIT_MALFORMED_PARAMETER, "--startcreditsnotafter only accepts SS, MM:SS or HH:MM:SS\n");
			}
			i++;
			continue;
		}
		if ((strcmp (argv[i],"--startcreditsforatleast")==0)
				&& i<argc-1)
		{
			if (stringztoms (argv[i+1],&opt->startcreditsforatleast)==-1)
			{
				fatal (EXIT_MALFORMED_PARAMETER, "--startcreditsforatleast only accepts SS, MM:SS or HH:MM:SS\n");
			}
			i++;
			continue;
		}
		if ((strcmp (argv[i],"--startcreditsforatmost")==0)
				&& i<argc-1)
		{
			if (stringztoms (argv[i+1],&opt->startcreditsforatmost)==-1)
			{
				fatal (EXIT_MALFORMED_PARAMETER, "--startcreditsforatmost only accepts SS, MM:SS or HH:MM:SS\n");
			}
			i++;
			continue;
		}

		if  ((strcmp (argv[i],"--endcreditstext")==0 )
				&& i<argc-1)
		{
			opt->end_credits_text=argv[i+1];
			i++;
			continue;
		}
		if ((strcmp (argv[i],"--endcreditsforatleast")==0)
				&& i<argc-1)
		{
			if (stringztoms (argv[i+1],&opt->endcreditsforatleast)==-1)
			{
				fatal (EXIT_MALFORMED_PARAMETER, "--endcreditsforatleast only accepts SS, MM:SS or HH:MM:SS\n");
			}
			i++;
			continue;
		}
		if ((strcmp (argv[i],"--endcreditsforatmost")==0)
				&& i<argc-1)
		{
			if (stringztoms (argv[i+1],&opt->endcreditsforatmost)==-1)
			{
				fatal (EXIT_MALFORMED_PARAMETER, "--startcreditsforatmost only accepts SS, MM:SS or HH:MM:SS\n");
			}
			i++;
			continue;
		}

		/* More stuff */
		if (strcmp (argv[i],"-ve")==0 ||
				strcmp (argv[i],"--videoedited")==0)
		{
			opt->binary_concat=0;
			continue;
		}
		if (strcmp (argv[i],"-12")==0)
		{
			opt->extract = 12;
			continue;
		}
		if (strcmp (argv[i],"-gt")==0 ||
				strcmp (argv[i],"--goptime")==0)
		{
			opt->use_gop_as_pts = 1;
			continue;
		}
		if (strcmp (argv[i],"-nogt")==0 ||
				strcmp (argv[i],"--nogoptime")==0)
		{
			opt->use_gop_as_pts = -1; // Don't use even if we would want to
			continue;
		}
		if (strcmp (argv[i],"-fp")==0 ||
				strcmp (argv[i],"--fixpadding")==0)
		{
			opt->fix_padding = 1;
			continue;
		}
		if (strcmp (argv[i],"-90090")==0)
		{
			MPEG_CLOCK_FREQ=90090;
			continue;
		}
		if (strcmp (argv[i],"-noru")==0 ||
				strcmp (argv[i],"--norollup")==0)
		{
			opt->settings_608.no_rollup = 1;
			continue;
		}
		if (strcmp (argv[i],"-ru1")==0)
		{
			opt->settings_608.force_rollup = 1;
			continue;
		}
		if (strcmp (argv[i],"-ru2")==0)
		{
			opt->settings_608.force_rollup = 2;
			continue;
		}
		if (strcmp (argv[i],"-ru3")==0)
		{
			opt->settings_608 .force_rollup = 3;
			continue;
		}
		if (strcmp (argv[i],"-trim")==0)
		{
			opt->trim_subs=1;
			continue;
		}
		if (strcmp (argv[i],"--gui_mode_reports")==0)
		{
			opt->gui_mode_reports=1;
			continue;
		}
		if (strcmp (argv[i],"--no_progress_bar")==0)
		{
			opt->no_progress_bar=1;
			continue;
		}
		if (strcmp (argv[i],"--sentencecap")==0 ||
				strcmp (argv[i],"-sc")==0)
		{
			opt->sentence_cap=1;
			continue;
		}
		if ((strcmp (argv[i],"--capfile")==0 ||
					strcmp (argv[i],"-caf")==0)
				&& i<argc-1)
		{
			opt->sentence_cap=1;
			opt->sentence_cap_file=argv[i+1];
			i++;
			continue;
		}
		if (strcmp (argv[i],"--program-number")==0 ||
				strcmp (argv[i],"-pn")==0)
		{
			if (i==argc-1 // Means no following argument
					|| !isanumber (argv[i+1])) // Means is not a number
				opt->ts_forced_program = (unsigned)-1; // Autodetect
			else
			{
				opt->ts_forced_program=atoi_hex (argv[i+1]);
				opt->ts_forced_program_selected=1;
				i++;
			}
			continue;
		}
		if (strcmp (argv[i],"-autoprogram")==0)
		{
			opt->ts_autoprogram=1;
			continue;
		}
		if (strcmp (argv[i],"--stream")==0 ||
				strcmp (argv[i],"-s")==0)
		{
			if (i==argc-1 // Means no following argument
					|| !isanumber (argv[i+1])) // Means is not a number
				opt->live_stream=-1; // Live stream without timeout
			else
			{
				opt->live_stream=atoi_hex (argv[i+1]);
				i++;
			}
			continue;
		}
		if ((strcmp (argv[i],"--defaultcolor")==0 ||
					strcmp (argv[i],"-dc")==0)
				&& i<argc-1)
		{
			if (strlen (argv[i+1])!=7 || argv[i+1][0]!='#')
			{
				fatal (EXIT_MALFORMED_PARAMETER, "--defaultcolor expects a 7 character parameter that starts with #\n");
			}
			strcpy ((char *) usercolor_rgb,argv[i+1]);
			opt->settings_608.default_color = COL_USERDEFINED;
			i++;
			continue;
		}
		if (strcmp (argv[i],"-delay")==0 && i<argc-1)
		{
			if (parsedelay (opt, argv[i+1]))
			{
				fatal (EXIT_MALFORMED_PARAMETER, "-delay only accept integers (such as -300 or 300)\n");
			}
			i++;
			continue;
		}
		if ((strcmp (argv[i],"-scr")==0 ||
					strcmp (argv[i],"--screenfuls")==0) && i<argc-1)
		{
			opt->settings_608.screens_to_process = atoi_hex(argv[i + 1]);
			if (opt->settings_608.screens_to_process<0)
			{
				fatal (EXIT_MALFORMED_PARAMETER, "--screenfuls only accepts positive integers.\n");
			}
			i++;
			continue;
		}
		if (strcmp (argv[i],"-startat")==0 && i<argc-1)
		{
			if (stringztoms (argv[i+1],&opt->extraction_start)==-1)
			{
				fatal (EXIT_MALFORMED_PARAMETER, "-startat only accepts SS, MM:SS or HH:MM:SS\n");
			}
			i++;
			continue;
		}
		if (strcmp (argv[i],"-endat")==0 && i<argc-1)
		{
			if (stringztoms (argv[i+1],&opt->extraction_end)==-1)
			{
				fatal (EXIT_MALFORMED_PARAMETER, "-endat only accepts SS, MM:SS or HH:MM:SS\n");
			}
			i++;
			continue;
		}
		if (strcmp (argv[i],"-1")==0)
		{
			opt->extract = 1;
			continue;
		}
		if (strcmp (argv[i],"-2")==0)
		{
			opt->extract = 2;
			continue;
		}
		if (strcmp (argv[i],"-cc2")==0 || strcmp (argv[i],"-CC2")==0)
		{
			opt->cc_channel=2;
			continue;
		}
		if (strcmp (argv[i],"-stdout")==0)
		{
			if (opt->messages_target==1) // Only change this if still stdout. -quiet could set it to 0 for example
				opt->messages_target=2; // stderr
			opt->cc_to_stdout=1;
			continue;
		}
		if (strcmp (argv[i],"-quiet")==0)
		{
			opt->messages_target=0;
			continue;
		}
		if (strcmp (argv[i],"-debug")==0)
		{
			opt->debug_mask |= CCX_DMT_VERBOSE;
			continue;
		}
		if (strcmp (argv[i],"-608")==0)
		{
			opt->debug_mask |= CCX_DMT_DECODER_608;
			continue;
		}
		if (strcmp (argv[i],"-deblev")==0)
		{
			opt->debug_mask |= CCX_DMT_LEVENSHTEIN;
			continue;
		}
		if (strcmp (argv[i],"-levdistmincnt")==0 && i<argc-1)
		{
			opt->levdistmincnt = atoi_hex(argv[i+1]);
			i++;
			continue;
		}
		if (strcmp (argv[i],"-levdistmaxpct")==0 && i<argc-1)
		{
			opt->levdistmaxpct = atoi_hex(argv[i+1]);
			i++;
			continue;
		}
		if (strcmp (argv[i],"-708")==0)
		{
			opt->debug_mask |= CCX_DMT_708;
			continue;
		}
		if (strcmp (argv[i],"-goppts")==0)
		{
			opt->debug_mask |= CCX_DMT_TIME;
			continue;
		}
		if (strcmp (argv[i],"-vides")==0)
		{
			opt->debug_mask |= CCX_DMT_VIDES;
			continue;
		}
		if (strcmp (argv[i],"-xds")==0)
		{
			// XDS can be set regardless of -UCLA (isFinal) usage.
			opt->transcript_settings.xds = 1;
			continue;
		}
		if (strcmp (argv[i],"-xdsdebug")==0)
		{
			opt->debug_mask |= CCX_DMT_DECODER_XDS;
			continue;
		}
		if (strcmp (argv[i],"-parsedebug")==0)
		{
			opt->debug_mask |= CCX_DMT_PARSE;
			continue;
		}
		if (strcmp (argv[i],"-parsePAT")==0 || strcmp (argv[i],"-parsepat")==0)
		{
			opt->debug_mask |= CCX_DMT_PAT;
			continue;
		}
		if (strcmp (argv[i],"-parsePMT")==0 || strcmp (argv[i],"-parsepmt")==0)
		{
			opt->debug_mask |= CCX_DMT_PMT;
			continue;
		}
		if (strcmp (argv[i],"-investigate_packets")==0)
		{
			opt->investigate_packets=1;
			continue;
		}
		if (strcmp (argv[i],"-cbraw")==0)
		{
			opt->debug_mask |= CCX_DMT_CBRAW;
			continue;
		}
		if (strcmp (argv[i],"-tverbose")==0)
		{
			opt->debug_mask |= CCX_DMT_TELETEXT;
			tlt_config.verbose=1;
			continue;
		}
		if (strcmp (argv[i],"-fullbin")==0)
		{
			opt->fullbin = 1;
			continue;
		}
		if (strcmp (argv[i],"-nosync")==0)
		{
			opt->nosync = 1;
			continue;
		}
		if (strcmp (argv[i],"-haup")==0 || strcmp (argv[i],"--hauppauge")==0)
		{
			opt->hauppauge_mode = 1;
			continue;
		}
		if (strcmp (argv[i],"-mp4vidtrack")==0)
		{
			opt->mp4vidtrack = 1;
			continue;
		}
		if (strstr (argv[i],"-unicode")!=NULL)
		{
			opt->encoding=CCX_ENC_UNICODE;
			continue;
		}
		if (strstr (argv[i],"-utf8")!=NULL)
		{
			opt->encoding=CCX_ENC_UTF_8;
			continue;
		}
		if (strstr (argv[i],"-latin1")!=NULL)
		{
			opt->encoding=CCX_ENC_LATIN_1;
			continue;
		}
		if (strcmp (argv[i],"-poc")==0 || strcmp (argv[i],"--usepicorder")==0)
		{
			opt->usepicorder = 1;
			continue;
		}
		if (strstr (argv[i],"-myth")!=NULL)
		{
			opt->auto_myth=1;
			continue;
		}
		if (strstr (argv[i],"-nomyth")!=NULL)
		{
			opt->auto_myth=0;
			continue;
		}
		if (strstr (argv[i],"-wtvconvertfix")!=NULL)
		{
			opt->wtvconvertfix=1;
			continue;
		}
		if (strstr (argv[i],"-wtvmpeg2")!=NULL)
		{
			opt->wtvmpeg2=1;
			continue;
		}
		if (strcmp (argv[i],"-o")==0 && i<argc-1)
		{
			opt->output_filename=argv[i+1];
			i++;
			continue;
		}
		if (strcmp (argv[i],"-cf")==0 && i<argc-1)
		{
			opt->out_elementarystream_filename=argv[i+1];
			i++;
			continue;
		}
		if (strcmp (argv[i],"-o1")==0 && i<argc-1)
		{
			opt->wbout1.filename=argv[i+1];
			i++;
			continue;
		}
		if (strcmp (argv[i],"-o2")==0 && i<argc-1)
		{
			opt->wbout2.filename=argv[i+1];
			i++;
			continue;
		}
		if ( (strcmp (argv[i],"-svc")==0 || strcmp (argv[i],"--service")==0) &&
				i<argc-1)
		{
			cea708_service_list=argv[i+1];
			parse_708services (cea708_service_list);
			i++;
			continue;
		}
		if (strcmp (argv[i],"-datapid")==0 && i<argc-1)
		{
			opt->ts_cappid = atoi_hex(argv[i+1]);
			opt->ts_forced_cappid=1;
			i++;
			continue;
		}
		if (strcmp (argv[i],"-datastreamtype")==0 && i<argc-1)
		{
			opt->ts_datastreamtype = atoi_hex(argv[i+1]);
			i++;
			continue;
		}
		if (strcmp (argv[i],"-streamtype")==0 && i<argc-1)
		{
			opt->ts_forced_streamtype = atoi_hex(argv[i+1]);
			i++;
			continue;
		}
		/* Teletext stuff */
		if (strcmp (argv[i],"-tpage")==0 && i<argc-1)
		{
			tlt_config.page = atoi_hex(argv[i+1]);
			tlt_config.user_page = tlt_config.page;
			opt->teletext_mode=CCX_TXT_IN_USE;
			i++;
			continue;
		}
		if (strcmp (argv[i],"-UCLA")==0 || strcmp (argv[i],"-ucla")==0)
		{
			opt->millis_separator='.';
			opt->no_bom = 1;
			if (!opt->transcript_settings.isFinal){
				opt->transcript_settings.showStartTime = 1;
				opt->transcript_settings.showEndTime = 1;
				opt->transcript_settings.showCC = 1;
				opt->transcript_settings.showMode = 1;
				opt->transcript_settings.relativeTimestamp = 0;
				opt->transcript_settings.isFinal = 1;
			}
			continue;
		}
		if (strcmp (argv[i],"-lf")==0 || strcmp (argv[i],"-LF")==0)
		{
			opt->line_terminator_lf = 1;
			continue;
		}
		if (strcmp (argv[i],"-noautotimeref")==0)
		{
			opt->noautotimeref = 1;
			continue;
		}
		if (strcmp (argv[i],"-autodash")==0)
		{
			opt->autodash = 1;
			continue;
		}
		if (strcmp (argv[i],"-xmltv")==0)
		{
			if (i==argc-1 // Means no following argument
					|| !isanumber (argv[i+1])) // Means is not a number
			opt->xmltv = 1;
			else
			{
				opt->xmltv=atoi_hex (argv[i+1]);
				i++;
			}
			continue;
		}
		
		if (strcmp (argv[i],"-xmltvliveinterval")==0)
		{
			if (i==argc-1 // Means no following argument
					|| !isanumber (argv[i+1])) // Means is not a number
			opt->xmltvliveinterval = 10;
			else
			{
				opt->xmltvliveinterval=atoi_hex (argv[i+1]);
				i++;
			}
			continue;
		}
		
		if (strcmp (argv[i],"-xmltvoutputinterval")==0)
		{
			if (i==argc-1 // Means no following argument
					|| !isanumber (argv[i+1])) // Means is not a number
			opt->xmltvoutputinterval = 0;
			else
			{
				opt->xmltvoutputinterval=atoi_hex (argv[i+1]);
				i++;
			}
			continue;
		}
		if (strcmp (argv[i],"-xmltvonlycurrent")==0)
		{
			opt->xmltvonlycurrent=1;
			i++;
			continue;
		}
		
		if (strcmp (argv[i],"-unixts")==0 && i<argc-1)
		{
			uint64_t t = 0;
			t = atoi_hex(argv[i + 1]);
			if (t <= 0)
			{
				time_t now = time(NULL);
				t = time(&now);
			}
			utc_refvalue = t;
			i++;
			opt->noautotimeref= 1; // If set by user don't attempt to fix
			continue;
		}
		if (strcmp (argv[i],"-sects")==0)
		{
			opt->date_format=ODF_SECONDS;
			continue;
		}
		if (strcmp (argv[i],"-datets")==0)
		{
			opt->date_format=ODF_DATE;
			continue;
		}
		if (strcmp (argv[i],"-teletext")==0)
		{
			opt->codec = CCX_CODEC_TELETEXT;
			opt->teletext_mode=CCX_TXT_IN_USE;
			continue;
		}
		if (strcmp (argv[i],"-noteletext")==0)
		{
			opt->nocodec = CCX_CODEC_TELETEXT;
			opt->teletext_mode=CCX_TXT_FORBIDDEN;
			continue;
		}
		/* Custom transcript */
		if (strcmp(argv[i], "-customtxt") == 0 && i<argc - 1){
			char *format = argv[i + 1];
			if (strlen(format) == 7){
				if (opt->date_format == ODF_NONE)
					opt->date_format = ODF_HHMMSSMS; // Necessary for displaying times, if any would be used.
				if (!opt->transcript_settings.isFinal){
					opt->transcript_settings.showStartTime = format[0] - '0';
					opt->transcript_settings.showEndTime = format[1] - '0';
					opt->transcript_settings.showMode = format[2] - '0';
					opt->transcript_settings.showCC = format[3] - '0';
					opt->transcript_settings.relativeTimestamp = format[4] - '0';
					opt->transcript_settings.xds = format[5] - '0';
					opt->transcript_settings.useColors = format[6] - '0';
				} else {
					// Throw exception
					fatal(EXIT_INCOMPATIBLE_PARAMETERS, "customtxt cannot be set after -UCLA is used!");
				}
				i++;
			}
			else {
				fatal(EXIT_MALFORMED_PARAMETER, "Custom TXT format not OK: %s, expected 7 bits string\n",
						format);
			}
			continue;
		}
		/* Network stuff */
		if (strcmp (argv[i],"-udp")==0 && i<argc-1)
		{
			char *colon = strchr(argv[i + 1], ':');
			if (colon)
			{
				*colon = '\0';
				opt->udpaddr = argv[i + 1];
				opt->udpport = atoi_hex(colon + 1);
			}
			else
			{
				opt->udpaddr = NULL;
				opt->udpport = atoi_hex(argv[i + 1]);
			}

			opt->input_source = CCX_DS_NETWORK;
			i++;
			continue;
		}

		if (strcmp (argv[i],"-sendto")==0 && i<argc-1)
		{
			opt->send_to_srv = 1;

			set_output_format(opt, "bin");

			char *addr = argv[i + 1];
			if (*addr == '[')
			{
				addr++;

				opt->srv_addr = addr;

				char *br = strchr(addr, ']');
				if (br == NULL)
					fatal (EXIT_INCOMPATIBLE_PARAMETERS, "Wrong address format, for IPv6 use [address]:port\n");
				*br = '\0';

				br++; /* Colon */
				if (*br != '\0')
					opt->srv_port = br + 1;

				i++;
				continue;
			}

			opt->srv_addr = argv[i + 1];

			char *colon = strchr(argv[i + 1], ':');
			if (colon != NULL)
			{
				*colon = '\0';
				opt->srv_port = colon + 1;
			}

			i++;
			continue;
		}

		if (strcmp (argv[i],"-tcp")==0 && i<argc-1)
		{
			opt->tcpport = argv[i + 1];
			opt->input_source = CCX_DS_TCP;

			set_input_format(opt, "bin");

			i++;
			continue;
		}

		if (strcmp (argv[i],"-tcppassword")==0 && i<argc-1)
		{
			opt->tcp_password = argv[i + 1];

			i++;
			continue;
		}

		if (strcmp (argv[i],"-tcpdesc")==0 && i<argc-1)
		{
			opt->tcp_desc = argv[i + 1];

			i++;
			continue;
		}

		fatal (EXIT_INCOMPATIBLE_PARAMETERS, "Error: Parameter %s not understood.\n", argv[i]);
		// Unrecognized switches are silently ignored
	}
	if(opt->gui_mode_reports)
	{
		opt->no_progress_bar=1;
		// Do it as soon as possible, because it something fails we might not have a chance
		activity_report_version();
	}

	if(opt->sentence_cap)
	{
		if(add_built_in_words())
			fatal (EXIT_NOT_ENOUGH_MEMORY, "Not enough memory for word list");
		if(opt->sentence_cap_file && process_cap_file (opt->sentence_cap_file))
			fatal (EXIT_ERROR_IN_CAPITALIZATION_FILE, "There was an error processing the capitalization file.\n");

		ccx_encoders_helpers_perform_shellsort_words();
	}
	if(opt->ts_forced_program != -1)
		opt->ts_forced_program_selected = 1;

	if(opt->wbout2.filename != NULL && (opt->extract != 2 || opt->extract != 12) )
		mprint("WARN: -o2 ignored! you might want -2 or -12 with -o2\n");
	
	// Init telexcc redundant options
	tlt_config.transcript_settings = &opt->transcript_settings;
	tlt_config.levdistmincnt = opt->levdistmincnt;
	tlt_config.levdistmaxpct = opt->levdistmaxpct;
	tlt_config.extraction_start = opt->extraction_start;
	tlt_config.extraction_end = opt->extraction_end;
	tlt_config.write_format = opt->write_format;
	tlt_config.gui_mode_reports = opt->gui_mode_reports;
	tlt_config.date_format = opt->date_format;
	tlt_config.noautotimeref = opt->noautotimeref;
	tlt_config.send_to_srv = opt->send_to_srv;
	tlt_config.encoding = opt->encoding;
	tlt_config.nofontcolor = opt->nofontcolor;
	tlt_config.millis_separator = opt->millis_separator;
}

int detect_input_file_overwrite(struct lib_ccx_ctx *ctx, const char *output_filename)
{
	for (int i = 0; i < ctx->num_input_files; i++)
	{
		if (!strcmp(ctx->inputfile[i], output_filename)) {
			return 1;
		}
	}
	return 0;
}
