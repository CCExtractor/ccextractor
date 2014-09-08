/* CCExtractor, carlos at ccextractor org
Credits: See CHANGES.TXT
License: GPL 2.0
*/
#include <stdio.h>
#include "ccextractor.h"
#include "configuration.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include "ffmpeg_intgr.h"

void xds_cea608_test();

struct ccx_s_options ccx_options;

extern unsigned char *filebuffer;
extern int bytesinbuffer; // Number of bytes we actually have on buffer

// global TS PCR value, moved from telxcc. TODO: Rename, see if how to relates to fts_global
uint32_t global_timestamp = 0, min_global_timestamp=0;
int global_timestamp_inited=0;

int saw_caption_block;

// Stuff common to both loops
unsigned char *buffer = NULL;
LLONG past; /* Position in file, if in sync same as ftell()  */
unsigned char *pesheaderbuf = NULL;
LLONG inputsize;
LLONG total_inputsize=0, total_past=0; // Only in binary concat mode

int last_reported_progress;
int processed_enough; // If 1, we have enough lines, time, etc. 

 
// Small buffer to help us with the initial sync
unsigned char startbytes[STARTBYTESLENGTH]; 
unsigned int startbytes_pos;
int startbytes_avail;

/* Stats */
int stat_numuserheaders;
int stat_dvdccheaders;
int stat_scte20ccheaders;
int stat_replay5000headers;
int stat_replay4000headers;
int stat_dishheaders;
int stat_hdtv;
int stat_divicom;
unsigned total_pulldownfields;
unsigned total_pulldownframes;
int cc_stats[4];
int false_pict_header;
int resets_708=0;

/* GOP-based timing */
int saw_gop_header=0;
int frames_since_last_gop=0;


/* Time info for timed-transcript */
int max_gop_length=0; // (Maximum) length of a group of pictures
int last_gop_length=0; // Length of the previous group of pictures

// int hex_mode=HEX_NONE; // Are we processing an hex file?

/* 608 contexts - note that this shouldn't be global, they should be 
per program */
ccx_decoder_608_context context_cc608_field_1, context_cc608_field_2;

/* Parameters */
void init_options (struct ccx_s_options *options)
{
#ifdef _WIN32
	options->buffer_input = 1; // In Windows buffering seems to help
#else
	options->buffer_input = 0; // In linux, not so much.
#endif	
	options->nofontcolor=0; // 1 = don't put <font color> tags 
	options->notypesetting=0; // 1 = Don't put <i>, <u>, etc typesetting tags

	options->no_bom = 0; // Use BOM by default.

	options->settings_608.direct_rollup = 0;
	options->settings_608.no_rollup = 0;
	options->settings_608.force_rollup = 0;
	options->settings_608.screens_to_process = -1;
	options->settings_608.default_color = COL_TRANSPARENT; // Defaults to transparant/no-color.

	/* Select subtitle codec */
	options->codec = CCX_CODEC_ANY;
	options->nocodec = CCX_CODEC_NONE;

	/* Credit stuff */
	options->start_credits_text=NULL;
	options->end_credits_text=NULL;
	options->extract = 1; // Extract 1st field only (primary language)
	options->cc_channel = 1; // Channel we want to dump in srt mode
	options->binary_concat=1; // Disabled by -ve or --videoedited
	options->use_gop_as_pts = 0; // Use GOP instead of PTS timing (0=do as needed, 1=always, -1=never)
	options->fix_padding = 0; // Replace 0000 with 8080 in HDTV (needed for some cards)
	options->trim_subs=0; // "	Remove spaces at sides?	"
	options->gui_mode_reports=0; // If 1, output in stderr progress updates so the GUI can grab them
	options->no_progress_bar=0; // If 1, suppress the output of the progress to stdout
	options->sentence_cap =0 ; // FIX CASE? = Fix case?
	options->sentence_cap_file=NULL; // Extra words file?
	options->live_stream=0; // 0 -> A regular file 
	options->messages_target=1; // 1=stdout
	options->print_file_reports=0;
	/* Levenshtein's parameters, for string comparison */
	options->levdistmincnt=2; // Means 2 fails or less is "the same"...
	options->levdistmaxpct=10; // ...10% or less is also "the same"	
	options->investigate_packets = 0; // Look for captions in all packets when everything else fails
	options->fullbin=0; // Disable pruning of padding cc blocks
	options->nosync=0; // Disable syncing
	options->hauppauge_mode=0; // If 1, use PID=1003, process specially and so on
	options->wtvconvertfix = 0; // Fix broken Windows 7 conversion
	options->wtvmpeg2 = 0; 
	options->auto_myth = 2; // 2=auto
	/* MP4 related stuff */
	options->mp4vidtrack=0; // Process the video track even if a CC dedicated track exists.
	/* General stuff */
	options->usepicorder = 0; // Force the use of pic_order_cnt_lsb in AVC/H.264 data streams
	options->autodash=0; // Add dashes (-) before each speaker automatically?
	options->teletext_mode=CCX_TXT_AUTO_NOT_YET_FOUND; // 0=Disabled, 1 = Not found, 2=Found

	options->transcript_settings = ccx_encoders_default_transcript_settings;
	options->millis_separator=',';
	
	options->encoding = CCX_ENC_UTF_8;
	options->write_format=CCX_OF_SRT; // 0=Raw, 1=srt, 2=SMI
	options->date_format=ODF_NONE; 
	options->output_filename=NULL;
	options->out_elementarystream_filename=NULL;
	options->debug_mask=CCX_DMT_GENERIC_NOTICES; // dbg_print will use this mask to print or ignore different types
	options->debug_mask_on_debug=CCX_DMT_VERBOSE; // If we're using temp_debug to enable/disable debug "live", this is the mask when temp_debug=1
	options->ts_autoprogram =0; // Try to find a stream with captions automatically (no -pn needed)
	options->ts_cappid = 0; // PID for stream that holds caption information
	options->ts_forced_cappid = 0; // If 1, never mess with the selected PID
	options->ts_forced_program=0; // Specific program to process in TS files, if ts_forced_program_selected==1
	options->ts_forced_program_selected=0; 
	options->ts_datastreamtype = -1; // User WANTED stream type (i.e. use the stream that has this type)
	options->ts_forced_streamtype=CCX_STREAM_TYPE_UNKNOWNSTREAM; // User selected (forced) stream type
	/* Networking */
	options->udpaddr = NULL;
	options->udpport=0; // Non-zero => Listen for UDP packets on this port, no files.
	options->send_to_srv = 0;
	options->tcpport = NULL;
	options->tcp_password = NULL;
	options->tcp_desc = NULL;
	options->srv_addr = NULL;
	options->srv_port = NULL;
	options->line_terminator_lf=0; // 0 = CRLF
	options->noautotimeref=0; // Do NOT set time automatically?
	options->input_source=CCX_DS_FILE; // Files, stdin or network	
}

enum ccx_stream_mode_enum stream_mode = CCX_SM_ELEMENTARY_OR_NOT_FOUND; // Data parse mode: 0=elementary, 1=transport, 2=program stream, 3=ASF container
enum ccx_stream_mode_enum auto_stream = CCX_SM_AUTODETECT;


int rawmode = 0; // Broadcast or DVD
// See -d from 

int cc_to_stdout=0; // If 1, captions go to stdout instead of file


LLONG subs_delay=0; // ms to delay (or advance) subs

int startcredits_displayed=0, end_credits_displayed=0;
LLONG last_displayed_subs_ms=0; // When did the last subs end?
LLONG screens_to_process=-1; // How many screenfuls we want?
char *basefilename=NULL; // Input filename without the extension
char **inputfile=NULL; // List of files to process

const char *extension; // Output extension
int current_file=-1; // If current_file!=1, we are processing *inputfile[current_file]

int num_input_files=0; // How many?

/* Hauppauge support */
unsigned hauppauge_warning_shown=0; // Did we detect a possible Hauppauge capture and told the user already?
unsigned teletext_warning_shown=0; // Did we detect a possible PAL (with teletext subs) and told the user already?


struct ccx_s_write wbout1, wbout2; // Output structures

/* File handles */
FILE *fh_out_elementarystream;
int infd=-1; // descriptor number to input. Set to -1 to indicate no file is open.
char *basefilename_for_stdin=(char *) "stdin"; // Default name for output files if input is stdin
char *basefilename_for_network=(char *) "network"; // Default name for output files if input is network
int PIDs_seen[65536];
struct PMT_entry *PIDs_programs[65536];

int temp_debug=0; // This is a convenience variable used to enable/disable debug on variable conditions. Find references to understand.

#ifdef DEBUG_TELEXCC
int main_telxcc (int argc, char *argv[]);
#endif
LLONG process_raw_with_field (void);

void sigint_handler()
{
	if (ccx_options.print_file_reports)
		print_file_report();

	exit(EXIT_SUCCESS);
}


int main(int argc, char *argv[])
{
	char *c;
	struct encoder_ctx enc_ctx[2];
	struct cc_subtitle dec_sub;
	void *ffmpeg_ctx = NULL;

	// Need to set the 608 data for the report to the correct variable.
	file_report.data_from_608 = &ccx_decoder_608_report;
	// Same applies for 708 data
	file_report.data_from_708 = &ccx_decoder_708_report;

	// Initialize some constants
	init_ts();
	init_avc();

	init_options (&ccx_options);

	// Init timing
	ccx_common_timing_init(&past,ccx_options.nosync);

	// Prepare write structures
	init_write(&wbout1);
	init_write(&wbout2);	
	
	// Prepare time structures
	init_boundary_time (&ccx_options.extraction_start);
	init_boundary_time (&ccx_options.extraction_end);
	init_boundary_time (&ccx_options.startcreditsnotbefore);
	init_boundary_time (&ccx_options.startcreditsnotafter);
	init_boundary_time (&ccx_options.startcreditsforatleast);
	init_boundary_time (&ccx_options.startcreditsforatmost);
	init_boundary_time (&ccx_options.endcreditsforatleast);
	init_boundary_time (&ccx_options.endcreditsforatmost);

	int show_myth_banner = 0;
	
	memset (&cea708services[0],0,CCX_DECODERS_708_MAX_SERVICES*sizeof (int)); // Cannot (yet) be moved because it's needed in parse_parameters.
	memset (&dec_sub, 0,sizeof(dec_sub));

	parse_configuration(&ccx_options);
	parse_parameters (argc,argv);	

	if (num_input_files==0 && ccx_options.input_source==CCX_DS_FILE)
	{
		usage ();
		fatal (EXIT_NO_INPUT_FILES, "(This help screen was shown because there were no input files)\n");
	}
	if (num_input_files>1 && ccx_options.live_stream)
	{
		fatal(EXIT_TOO_MANY_INPUT_FILES, "Live stream mode accepts only one input file.\n");
	}
	if (num_input_files && ccx_options.input_source==CCX_DS_NETWORK)
	{
		fatal(EXIT_TOO_MANY_INPUT_FILES, "UDP mode is not compatible with input files.\n");
	}
	if (ccx_options.input_source==CCX_DS_NETWORK || ccx_options.input_source==CCX_DS_TCP)
	{
		ccx_options.buffer_input=1; // Mandatory, because each datagram must be read complete.
	}
	if (num_input_files && ccx_options.input_source==CCX_DS_TCP)
	{
		fatal(EXIT_TOO_MANY_INPUT_FILES, "TCP mode is not compatible with input files.\n");
	}

	if (num_input_files > 0)
	{
		wbout1.multiple_files = 1;
		wbout1.first_input_file = inputfile[0];
		wbout2.multiple_files = 1;
		wbout2.first_input_file = inputfile[0];
	}

	// teletext page number out of range
	if ((tlt_config.page != 0) && ((tlt_config.page < 100) || (tlt_config.page > 899))) {
		fatal (EXIT_NOT_CLASSIFIED, "Teletext page number could not be lower than 100 or higher than 899\n");
	}

	if (ccx_options.output_filename!=NULL)
	{
		// Use the given output file name for the field specified by
		// the -1, -2 switch. If -12 is used, the filename is used for
		// field 1.
		if (ccx_options.extract==2)
			wbout2.filename=ccx_options.output_filename;
		else
			wbout1.filename=ccx_options.output_filename;
	}

	switch (ccx_options.write_format)
	{
		case CCX_OF_RAW:
			extension = ".raw";
			break;
		case CCX_OF_SRT:
			extension = ".srt";
			break;
		case CCX_OF_SAMI:
			extension = ".smi";
			break;
		case CCX_OF_SMPTETT:
			extension = ".ttml";
			break;
		case CCX_OF_TRANSCRIPT:
			extension = ".txt";
			break;
		case CCX_OF_RCWT:
			extension = ".bin";
			break;
		case CCX_OF_SPUPNG:
			extension = ".xml";
			break;
		case CCX_OF_NULL:
			extension = "";
			break;
		case CCX_OF_DVDRAW:
			extension = ".dvdraw";
			break;
		default:
			fatal (CCX_COMMON_EXIT_BUG_BUG, "write_format doesn't have any legal value, this is a bug.\n");			
	}
	params_dump();

	// default teletext page
	if (tlt_config.page > 0) {
		// dec to BCD, magazine pages numbers are in BCD (ETSI 300 706)
		tlt_config.page = ((tlt_config.page / 100) << 8) | (((tlt_config.page / 10) % 10) << 4) | (tlt_config.page % 10);
	}

	if (auto_stream==CCX_SM_MCPOODLESRAW && ccx_options.write_format==CCX_OF_RAW)
	{
		fatal (EXIT_INCOMPATIBLE_PARAMETERS, "-in=raw can only be used if the output is a subtitle file.\n");
	}
	if (auto_stream==CCX_SM_RCWT && ccx_options.write_format==CCX_OF_RCWT && ccx_options.output_filename==NULL)
	{
		fatal (EXIT_INCOMPATIBLE_PARAMETERS,
			   "CCExtractor's binary format can only be used simultaneously for input and\noutput if the output file name is specified given with -o.\n");
	}

	buffer = (unsigned char *) malloc (BUFSIZE);
	subline = (unsigned char *) malloc (SUBLINESIZE);
	pesheaderbuf = (unsigned char *) malloc (188); // Never larger anyway

	switch (ccx_options.input_source)
	{
		case CCX_DS_FILE:
			basefilename = (char *) malloc (strlen (inputfile[0])+1);
			break;
		case CCX_DS_STDIN:
			basefilename = (char *) malloc (strlen (basefilename_for_stdin)+1);
			break;
		case CCX_DS_NETWORK:
		case CCX_DS_TCP:
			basefilename = (char *) malloc (strlen (basefilename_for_network)+1);
			break;
	}		
	if (basefilename == NULL)
		fatal (EXIT_NOT_ENOUGH_MEMORY, "Not enough memory\n");		
	switch (ccx_options.input_source)
	{
		case CCX_DS_FILE:
			strcpy (basefilename, inputfile[0]);
			break;
		case CCX_DS_STDIN:
			strcpy (basefilename, basefilename_for_stdin);
			break;
		case CCX_DS_NETWORK:
		case CCX_DS_TCP:
			strcpy (basefilename, basefilename_for_network);
			break;
	}		
	for (c=basefilename+strlen (basefilename)-1; c>basefilename &&
		*c!='.'; c--) {;} // Get last .
	if (*c=='.')
		*c=0;

	if (wbout1.filename==NULL)
	{
		wbout1.filename = (char *) malloc (strlen (basefilename)+3+strlen (extension)); 
		wbout1.filename[0]=0;
	}
	if (wbout2.filename==NULL)
	{
		wbout2.filename = (char *) malloc (strlen (basefilename)+3+strlen (extension));
		wbout2.filename[0]=0;
	}
	if (buffer == NULL || pesheaderbuf==NULL ||
		wbout1.filename == NULL || wbout2.filename == NULL ||
		subline==NULL || init_file_buffer() )
	{
		fatal (EXIT_NOT_ENOUGH_MEMORY, "Not enough memory\n");		
	}

	if (ccx_options.send_to_srv)
	{
		connect_to_srv(ccx_options.srv_addr, ccx_options.srv_port, ccx_options.tcp_desc);
	}

	if (ccx_options.write_format!=CCX_OF_NULL)
	{
		/* # DVD format uses one raw file for both fields, while Broadcast requires 2 */
		if (ccx_options.write_format==CCX_OF_DVDRAW)
		{
			if (wbout1.filename[0]==0)
			{
				strcpy (wbout1.filename,basefilename);
				strcat (wbout1.filename,".raw");
			}
			if (cc_to_stdout)
			{
				wbout1.fh=STDOUT_FILENO;
				mprint ("Sending captions to stdout.\n");
			}
			else
			{
				mprint ("Creating %s\n", wbout1.filename);			
				wbout1.fh=open (wbout1.filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
				if (wbout1.fh==-1)
				{
					fatal(CCX_COMMON_EXIT_FILE_CREATION_FAILED, "Failed\n");
				}
			}
		}
		else
		{
			if (cc_to_stdout && ccx_options.extract==12)			
				fatal (EXIT_INCOMPATIBLE_PARAMETERS, "You can't extract both fields to stdout at the same time in broadcast mode.");
			
			if (ccx_options.write_format == CCX_OF_SPUPNG && cc_to_stdout)
				fatal (EXIT_INCOMPATIBLE_PARAMETERS, "You cannot use -out=spupng with -stdout.");

			if (ccx_options.extract!=2)
			{
				if (cc_to_stdout)
				{
					wbout1.fh=STDOUT_FILENO;
					mprint ("Sending captions to stdout.\n");
				}
				else if (!ccx_options.send_to_srv)
				{
					if (wbout1.filename[0]==0)
					{
						strcpy (wbout1.filename,basefilename);
						if (ccx_options.extract==12) // _1 only added if there's two files
							strcat (wbout1.filename,"_1");
						strcat (wbout1.filename,(const char *) extension);
					}
					mprint ("Creating %s\n", wbout1.filename);				
					wbout1.fh=open (wbout1.filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
					if (wbout1.fh==-1)
					{
						fatal(CCX_COMMON_EXIT_FILE_CREATION_FAILED, "Failed (errno=%d)\n", errno);
					}
				}
				switch (ccx_options.write_format)
				{
				case CCX_OF_RAW:
					writeraw(BROADCAST_HEADER, sizeof(BROADCAST_HEADER), &wbout1);
					break;
				case CCX_OF_DVDRAW:
					break;
				case CCX_OF_RCWT:
					if (init_encoder(enc_ctx, &wbout1))
						fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory\n");
					break;
				default:
					if (!ccx_options.no_bom){
						if (ccx_options.encoding == CCX_ENC_UTF_8){ // Write BOM
							writeraw(UTF8_BOM, sizeof(UTF8_BOM), &wbout1);
						}
						if (ccx_options.encoding == CCX_ENC_UNICODE){ // Write BOM				
							writeraw(LITTLE_ENDIAN_BOM, sizeof(LITTLE_ENDIAN_BOM), &wbout1);
						}
					}
					if (init_encoder(enc_ctx, &wbout1)){
						fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory\n");
					}
				}
			}
			if (ccx_options.extract == 12 && ccx_options.write_format != CCX_OF_RAW)
				mprint (" and \n");
			if (ccx_options.extract!=1)
			{
				if (cc_to_stdout)
				{
					wbout1.fh=STDOUT_FILENO;
					mprint ("Sending captions to stdout.\n");
				}
				else if(ccx_options.write_format == CCX_OF_RAW
					&& ccx_options.extract == 12)
				{
					memcpy(&wbout2, &wbout1,sizeof(wbout1));
				}
				else if (!ccx_options.send_to_srv)
				{
					if (wbout2.filename[0]==0)
					{
						strcpy (wbout2.filename,basefilename);				
						if (ccx_options.extract==12) // _ only added if there's two files
							strcat (wbout2.filename,"_2");
						strcat (wbout2.filename,(const char *) extension);
					}
					mprint ("Creating %s\n", wbout2.filename);
					wbout2.fh=open (wbout2.filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY, S_IREAD | S_IWRITE);
					if (wbout2.fh==-1)
					{
						fatal(CCX_COMMON_EXIT_FILE_CREATION_FAILED, "Failed\n");
					}
					if(ccx_options.write_format == CCX_OF_RAW)
						writeraw (BROADCAST_HEADER,sizeof (BROADCAST_HEADER),&wbout2);
				}

				switch (ccx_options.write_format)
				{
					case CCX_OF_RAW:
					case CCX_OF_DVDRAW:
						break;
					case CCX_OF_RCWT:
						if( init_encoder(enc_ctx+1,&wbout2) )
							fatal (EXIT_NOT_ENOUGH_MEMORY, "Not enough memory\n");
						break;
					default:
						if (!ccx_options.no_bom){
							if (ccx_options.encoding == CCX_ENC_UTF_8){ // Write BOM
								writeraw(UTF8_BOM, sizeof(UTF8_BOM), &wbout2);
							}
							if (ccx_options.encoding == CCX_ENC_UNICODE){ // Write BOM				
								writeraw(LITTLE_ENDIAN_BOM, sizeof(LITTLE_ENDIAN_BOM), &wbout2);
							}
						}
						if (init_encoder(enc_ctx + 1, &wbout2)){
							fatal(EXIT_NOT_ENOUGH_MEMORY, "Not enough memory\n");
						}
				}
			}
		}
	}

	if (ccx_options.transcript_settings.xds)
	{
		if (ccx_options.write_format != CCX_OF_TRANSCRIPT)
		{
			ccx_options.transcript_settings.xds = 0;
			mprint ("Warning: -xds ignored, XDS can only be exported to transcripts at this time.\n");
		}
	}

	if (ccx_options.teletext_mode == CCX_TXT_IN_USE) // Here, it would mean it was forced by user
		telxcc_init();

	fh_out_elementarystream = NULL;
	if (ccx_options.out_elementarystream_filename!=NULL)
	{
		if ((fh_out_elementarystream = fopen (ccx_options.out_elementarystream_filename,"wb"))==NULL)
		{
			fatal(CCX_COMMON_EXIT_FILE_CREATION_FAILED, "Unable to open clean file: %s\n", ccx_options.out_elementarystream_filename);
		}
	}	

	build_parity_table();

	// Initialize HDTV caption buffer
	init_hdcc();

	// Initialize libraries
	init_libraries();

	if (ccx_options.line_terminator_lf)
		encoded_crlf_length = encode_line(encoded_crlf, (unsigned char *) "\n");
	else
		encoded_crlf_length = encode_line(encoded_crlf, (unsigned char *) "\r\n");

	encoded_br_length = encode_line(encoded_br, (unsigned char *) "<br>");
	

	time_t start, final;
	time(&start);

	processed_enough=0;
	if (ccx_options.binary_concat)
	{
		total_inputsize=gettotalfilessize();
		if (total_inputsize==-1)
			fatal (EXIT_UNABLE_TO_DETERMINE_FILE_SIZE, "Failed to determine total file size.\n");
	}

	m_signal(SIGINT, sigint_handler);

	while (switch_to_next_file(0) && !processed_enough)
	{
		prepare_for_new_file();
#ifdef ENABLE_FFMPEG
		close_input_file();
		ffmpeg_ctx =  init_ffmpeg(inputfile[0]);
		if(ffmpeg_ctx)
		{
			int i =0;
			buffer = malloc(1024);
			if(!buffer)
			{
				mprint("no memory left\n");
				break;
			}
			do
			{
				int ret = 0;
				char *bptr = buffer;
				memset(bptr,0,1024);
				int len = ff_get_ccframe(ffmpeg_ctx, bptr, 1024);
				if(len == AVERROR(EAGAIN))
				{
					continue;
				}
				else if(len == AVERROR_EOF)
					break;
				else if(len == 0)
					continue;
				else if(len < 0 )
				{
					mprint("Error extracting Frame\n");
					break;

				}
				store_hdcc(bptr,len, i++,fts_now,&dec_sub);
				if(dec_sub.got_output)
				{
					encode_sub(enc_ctx, &dec_sub);
					dec_sub.got_output = 0;
				}
			}while(1);

			free(buffer);
			continue;
		}
		else
		{
			mprint ("\rFailed to initialized ffmpeg falling back to legacy\n");
		}
#endif

		if (auto_stream == CCX_SM_AUTODETECT)
		{
			detect_stream_type();			
			switch (stream_mode)
			{
				case CCX_SM_ELEMENTARY_OR_NOT_FOUND:
					mprint ("\rFile seems to be an elementary stream, enabling ES mode\n");
					break;
				case CCX_SM_TRANSPORT:
					mprint ("\rFile seems to be a transport stream, enabling TS mode\n");
					break;
				case CCX_SM_PROGRAM:
					mprint ("\rFile seems to be a program stream, enabling PS mode\n");
					break;
				case CCX_SM_ASF:
					mprint ("\rFile seems to be an ASF, enabling DVR-MS mode\n");
					break;
				case CCX_SM_WTV:
					mprint ("\rFile seems to be a WTV, enabling WTV mode\n");
					break;
				case CCX_SM_MCPOODLESRAW:
					mprint ("\rFile seems to be McPoodle raw data\n");
					break;
				case CCX_SM_RCWT:
					mprint ("\rFile seems to be a raw caption with time data\n");
					break;
				case CCX_SM_MP4:
					mprint ("\rFile seems to be a MP4\n");
					break;
#ifdef WTV_DEBUG
				case CCX_SM_HEX_DUMP:
					mprint ("\rFile seems to be an hexadecimal dump\n");					
					break;
#endif
				case CCX_SM_MYTH:
				case CCX_SM_AUTODETECT:
					fatal(CCX_COMMON_EXIT_BUG_BUG, "Cannot be reached!");
					break;
			}
		}
		else
		{
			stream_mode=auto_stream;
		}
	
		/* -----------------------------------------------------------------
		MAIN LOOP
		----------------------------------------------------------------- */

		// The myth loop autodetect will only be used with ES or PS streams
		switch (ccx_options.auto_myth)
		{
			case 0:
				// Use whatever stream mode says
				break;
			case 1:
				// Force stream mode to myth
				stream_mode=CCX_SM_MYTH;
				break;
			case 2:
				// autodetect myth files, but only if it does not conflict with
				// the current stream mode
				switch (stream_mode)
				{
					case CCX_SM_ELEMENTARY_OR_NOT_FOUND:
					case CCX_SM_PROGRAM:
						if ( detect_myth() )
						{
							stream_mode=CCX_SM_MYTH;
						}
						break;
					default:
						// Keep stream_mode
						break;
				}
				break;					
		}

		// Disable sync check for raw formats - they have the right timeline.
		// Also true for bin formats, but -nosync might have created a
		// broken timeline for debug purposes.
		// Disable too in MP4, specs doesn't say that there can't be a jump
		switch (stream_mode)
		{
		case CCX_SM_MCPOODLESRAW:
		case CCX_SM_RCWT:
		case CCX_SM_MP4:
#ifdef WTV_DEBUG
		case CCX_SM_HEX_DUMP:
#endif
			ccx_common_timing_settings.disable_sync_check = 1;
			break;
		default:
			break;
		}
				
		switch (stream_mode)
		{
			case CCX_SM_ELEMENTARY_OR_NOT_FOUND:
				if (!ccx_options.use_gop_as_pts) // If !0 then the user selected something
					ccx_options.use_gop_as_pts = 1; // Force GOP timing for ES
				ccx_common_timing_settings.is_elementary_stream = 1;
			case CCX_SM_TRANSPORT:
			case CCX_SM_PROGRAM:
			case CCX_SM_ASF:
			case CCX_SM_WTV:
				if (!ccx_options.use_gop_as_pts) // If !0 then the user selected something
					ccx_options.use_gop_as_pts = 0; 
				mprint ("\rAnalyzing data in general mode\n");
				general_loop(&enc_ctx);
				break;
			case CCX_SM_MCPOODLESRAW:
				mprint ("\rAnalyzing data in McPoodle raw mode\n");
				raw_loop(&enc_ctx);
				break;
			case CCX_SM_RCWT:
				mprint ("\rAnalyzing data in CCExtractor's binary format\n");
				rcwt_loop(&enc_ctx);
				break;
			case CCX_SM_MYTH:
				mprint ("\rAnalyzing data in MythTV mode\n");
				show_myth_banner = 1;
				myth_loop(&enc_ctx);
				break;
			case CCX_SM_MP4:				
				mprint ("\rAnalyzing data with GPAC (MP4 library)\n");
				close_input_file(); // No need to have it open. GPAC will do it for us
				processmp4 (inputfile[0],&enc_ctx);
				break;
#ifdef WTV_DEBUG
			case CCX_SM_HEX_DUMP:
				close_input_file(); // processhex will open it in text mode
				processhex (inputfile[0]);
				break;
#endif
			case CCX_SM_AUTODETECT:
				fatal(CCX_COMMON_EXIT_BUG_BUG, "Cannot be reached!");
				break;
		}

		mprint("\n");
		dbg_print(CCX_DMT_DECODER_608, "\nTime stamps after last caption block was written:\n");
		dbg_print(CCX_DMT_DECODER_608, "Last time stamps:  PTS: %s (%+2dF)		",
			   print_mstime( (LLONG) (sync_pts/(MPEG_CLOCK_FREQ/1000)
								   +frames_since_ref_time*1000.0/current_fps) ),
			   frames_since_ref_time);
		dbg_print(CCX_DMT_DECODER_608, "GOP: %s	  \n", print_mstime(gop_time.ms) );

		// Blocks since last PTS/GOP time stamp.
		dbg_print(CCX_DMT_DECODER_608, "Calc. difference:  PTS: %s (%+3lldms incl.)  ",
			print_mstime( (LLONG) ((sync_pts-min_pts)/(MPEG_CLOCK_FREQ/1000)
			+ fts_offset + frames_since_ref_time*1000.0/current_fps)),
			fts_offset + (LLONG) (frames_since_ref_time*1000.0/current_fps) );
		dbg_print(CCX_DMT_DECODER_608, "GOP: %s (%+3dms incl.)\n",
			print_mstime((LLONG)(gop_time.ms
			-first_gop_time.ms
			+get_fts_max()-fts_at_gop_start)),
			(int)(get_fts_max()-fts_at_gop_start));
		// When padding is active the CC block time should be within
		// 1000/29.97 us of the differences.
		dbg_print(CCX_DMT_DECODER_608, "Max. FTS:	   %s  (without caption blocks since then)\n",
			print_mstime(get_fts_max()));

		if (stat_hdtv)
		{
			mprint ("\rCC type 0: %d (%s)\n", cc_stats[0], cc_types[0]);
			mprint ("CC type 1: %d (%s)\n", cc_stats[1], cc_types[1]);
			mprint ("CC type 2: %d (%s)\n", cc_stats[2], cc_types[2]);
			mprint ("CC type 3: %d (%s)\n", cc_stats[3], cc_types[3]);
		}
		mprint ("\nTotal frames time:	  %s  (%u frames at %.2ffps)\n",
			print_mstime( (LLONG)(total_frames_count*1000/current_fps) ),
			total_frames_count, current_fps);
		if (total_pulldownframes)
			mprint ("incl. pulldown frames:  %s  (%u frames at %.2ffps)\n",
					print_mstime( (LLONG)(total_pulldownframes*1000/current_fps) ),
					total_pulldownframes, current_fps);
		if (pts_set >= 1 && min_pts != 0x01FFFFFFFFLL)
		{
			LLONG postsyncms = (LLONG) (frames_since_last_gop*1000/current_fps);
			mprint ("\nMin PTS:				%s\n",
					print_mstime( min_pts/(MPEG_CLOCK_FREQ/1000) - fts_offset));
			if (pts_big_change)
				mprint ("(Reference clock was reset at some point, Min PTS is approximated)\n");
			mprint ("Max PTS:				%s\n",
					print_mstime( sync_pts/(MPEG_CLOCK_FREQ/1000) + postsyncms));

			mprint ("Length:				 %s\n",
					print_mstime( sync_pts/(MPEG_CLOCK_FREQ/1000) + postsyncms
								  - min_pts/(MPEG_CLOCK_FREQ/1000) + fts_offset ));
		}
		// dvr-ms files have invalid GOPs
		if (gop_time.inited && first_gop_time.inited && stream_mode != CCX_SM_ASF)
		{
			mprint ("\nInitial GOP time:	   %s\n",
				print_mstime(first_gop_time.ms));
			mprint ("Final GOP time:		 %s%+3dF\n",
				print_mstime(gop_time.ms),
				frames_since_last_gop);
			mprint ("Diff. GOP length:	   %s%+3dF",
				print_mstime(gop_time.ms - first_gop_time.ms),
				frames_since_last_gop);
			mprint ("	(%s)\n",
				print_mstime(gop_time.ms - first_gop_time.ms
				+(LLONG) ((frames_since_last_gop)*1000/29.97)) );
		}

		if (false_pict_header)
			mprint ("\nNumber of likely false picture headers (discarded): %d\n",false_pict_header);

		if (stat_numuserheaders)
			mprint("\nTotal user data fields: %d\n", stat_numuserheaders);
		if (stat_dvdccheaders)
			mprint("DVD-type user data fields: %d\n", stat_dvdccheaders);
		if (stat_scte20ccheaders)
			mprint("SCTE-20 type user data fields: %d\n", stat_scte20ccheaders);
		if (stat_replay4000headers)
			mprint("ReplayTV 4000 user data fields: %d\n", stat_replay4000headers);
		if (stat_replay5000headers)
			mprint("ReplayTV 5000 user data fields: %d\n", stat_replay5000headers);
		if (stat_hdtv)
			mprint("HDTV type user data fields: %d\n", stat_hdtv);
		if (stat_dishheaders)
			mprint("Dish Network user data fields: %d\n", stat_dishheaders);
		if (stat_divicom)
		{
			mprint("CEA608/Divicom user data fields: %d\n", stat_divicom);

			mprint("\n\nNOTE! The CEA 608 / Divicom standard encoding for closed\n");
			mprint("caption is not well understood!\n\n");
			mprint("Please submit samples to the developers.\n\n\n");
		}

		// Add one frame as fts_max marks the beginning of the last frame,
		// but we need the end.
		fts_global += fts_max + (LLONG) (1000.0/current_fps);
		// CFS: At least in Hauppage mode, cb_field can be responsible for ALL the 
		// timing (cb_fields having a huge number and fts_now and fts_global being 0 all
		// the time), so we need to take that into account in fts_global before resetting
		// counters.
		if (cb_field1!=0)
			fts_global += cb_field1*1001/3;
		else
			fts_global += cb_field2*1001/3;
		// Reset counters - This is needed if some captions are still buffered
		// and need to be written after the last file is processed.		
		cb_field1 = 0; cb_field2 = 0; cb_708 = 0;
		fts_now = 0;
		fts_max = 0;		
	} // file loop
	close_input_file();
	
	if (fh_out_elementarystream!=NULL)
		fclose (fh_out_elementarystream);	

	flushbuffer (&wbout1,false);
	flushbuffer (&wbout2,false);

	prepare_for_new_file (); // To reset counters used by handle_end_of_data()

	if (wbout1.fh!=-1)
	{
		if (ccx_options.write_format==CCX_OF_SMPTETT || ccx_options.write_format==CCX_OF_SAMI || 
			ccx_options.write_format==CCX_OF_SRT || ccx_options.write_format==CCX_OF_TRANSCRIPT
			|| ccx_options.write_format==CCX_OF_SPUPNG )
		{
			handle_end_of_data(&context_cc608_field_1, &dec_sub);
			if (dec_sub.got_output)
			{
				encode_sub(enc_ctx,&dec_sub);
				dec_sub.got_output = 0;
			}
		}
		else if(ccx_options.write_format==CCX_OF_RCWT)
		{
			// Write last header and data
			writercwtdata (NULL);
		}
		dinit_encoder(enc_ctx);
	}
	if (wbout2.fh!=-1)
	{
		if (ccx_options.write_format==CCX_OF_SMPTETT || ccx_options.write_format==CCX_OF_SAMI || 
			ccx_options.write_format==CCX_OF_SRT || ccx_options.write_format==CCX_OF_TRANSCRIPT
			|| ccx_options.write_format==CCX_OF_SPUPNG )
		{
			handle_end_of_data(&context_cc608_field_2, &dec_sub);
			if (dec_sub.got_output)
			{
				encode_sub(enc_ctx,&dec_sub);
				dec_sub.got_output = 0;
			}
		}
		dinit_encoder(enc_ctx+1);
	}
	telxcc_close();
	flushbuffer (&wbout1,true);
	flushbuffer (&wbout2,true);
	time (&final);

	long proc_time=(long) (final-start);
	mprint ("\rDone, processing time = %ld seconds\n", proc_time);
	if (proc_time>0)
	{
		LLONG ratio=(get_fts_max()/10)/proc_time;
		unsigned s1=(unsigned) (ratio/100);
		unsigned s2=(unsigned) (ratio%100);	
		mprint ("Performance (real length/process time) = %u.%02u\n", 
			s1, s2);
	}
	dbg_print(CCX_DMT_708, "The 708 decoder was reset [%d] times.\n",resets_708);
	if (ccx_options.teletext_mode == CCX_TXT_IN_USE)
		mprint ( "Teletext decoder: %"PRIu32" packets processed, %"PRIu32" SRT frames written.\n", tlt_packet_counter, tlt_frames_produced);

	if (processed_enough)
	{
		mprint ("\rNote: Processing was cancelled before all data was processed because\n");
		mprint ("\rone or more user-defined limits were reached.\n");
	} 
	if (ccblocks_in_avc_lost>0)
	{
		mprint ("Total caption blocks received: %d\n", ccblocks_in_avc_total);
		mprint ("Total caption blocks lost: %d\n", ccblocks_in_avc_lost);
	}

	mprint ("This is beta software. Report issues to carlos at ccextractor org...\n");
	if (show_myth_banner)
	{
		mprint ("NOTICE: Due to the major rework in 0.49, we needed to change part of the timing\n");
		mprint ("code in the MythTV's branch. Please report results to the address above. If\n");
		mprint ("something is broken it will be fixed. Thanks\n");		
	}
	return EXIT_OK;
}

void init_libraries(){
	// Set logging functions for libraries
	ccx_common_logging.debug_ftn = &dbg_print;
	ccx_common_logging.debug_mask = ccx_options.debug_mask;
	ccx_common_logging.fatal_ftn = &fatal;
	ccx_common_logging.log_ftn = &mprint;
	ccx_common_logging.gui_ftn = &activity_library_process;

	// Init shared decoder settings
	ccx_decoders_common_settings_init(subs_delay, ccx_options.write_format);
	// Init encoder helper variables
	ccx_encoders_helpers_setup(ccx_options.encoding, ccx_options.nofontcolor, ccx_options.notypesetting, ccx_options.trim_subs);

	// Prepare 608 context
	context_cc608_field_1 = ccx_decoder_608_init_library(
		ccx_options.settings_608,
		ccx_options.cc_channel,
		1,
		ccx_options.trim_subs,
		ccx_options.encoding,
		&processed_enough,
		&cc_to_stdout
		);
	context_cc608_field_2 = ccx_decoder_608_init_library(
		ccx_options.settings_608,
		ccx_options.cc_channel,
		2,
		ccx_options.trim_subs,
		ccx_options.encoding,
		&processed_enough,
		&cc_to_stdout
		);

	// Init 708 decoder(s)
	ccx_decoders_708_init_library(basefilename,extension,ccx_options.print_file_reports);

	// Set output structures for the 608 decoder
	context_cc608_field_1.out = &wbout1;
	context_cc608_field_2.out = &wbout2;

	// Init XDS buffers
	ccx_decoders_xds_init_library(&ccx_options.transcript_settings, subs_delay, ccx_options.millis_separator);
	//xds_cea608_test();
}
