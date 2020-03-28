/* This file contains functions that report the user of the GUI of
relevant events. */

#include "lib_ccx.h"
#include "ccx_common_option.h"

static int credits_shown = 0;
unsigned long net_activity_gui = 0;

/* Print current progress. For percentage, -1 -> streaming mode */
void activity_progress(int percentage, int cur_min, int cur_sec)
{
	if (!ccx_options.no_progress_bar)
	{
		if (percentage == -1)
			mprint("Streaming |  %02d:%02d\r", cur_min, cur_sec);
		else
			mprint("%3d%%  |  %02d:%02d\r", percentage, cur_min, cur_sec);
		if (ccx_options.pes_header_to_stdout || ccx_options.debug_mask & CCX_DMT_DVB) //For PES Header dumping and DVB debug traces
		{
			mprint("\n");
		}
	}
	fflush(stdout);
	if (ccx_options.gui_mode_reports)
	{
		fprintf(stderr, "###PROGRESS#%d#%d#%d\n", percentage, cur_min, cur_sec);
		fflush(stderr);
	}
}

void activity_input_file_open(const char *filename)
{
	if (ccx_options.gui_mode_reports)
	{
		fprintf(stderr, "###INPUTFILEOPEN#%s\n", filename);
		fflush(stderr);
	}
}

void activity_library_process(enum ccx_common_logging_gui message_type, ...)
{
	if (ccx_options.gui_mode_reports)
	{
		va_list args;
		va_start(args, message_type);
		switch (message_type)
		{
			case CCX_COMMON_LOGGING_GUI_XDS_CALL_LETTERS:
				vfprintf(stderr, "###XDSNETWORKCALLLETTERS#%s\n", args);
				break;
			case CCX_COMMON_LOGGING_GUI_XDS_PROGRAM_DESCRIPTION:
				vfprintf(stderr, "###XDSPROGRAMDESC#%d#%s\n", args);
				break;
			case CCX_COMMON_LOGGING_GUI_XDS_PROGRAM_ID_NR:
				vfprintf(stderr, "###XDSPROGRAMIDENTIFICATIONNUMBER#%u#%u#%u#%u\n", args);
				break;
			case CCX_COMMON_LOGGING_GUI_XDS_PROGRAM_NAME:
				vfprintf(stderr, "###XDSPROGRAMNAME#%s\n", args);
				break;
			default:
				break;
		}
		fflush(stderr);
		va_end(args);
	}
}

void activity_video_info(int hor_size, int vert_size,
			 const char *aspect_ratio, const char *framerate)
{
	if (ccx_options.gui_mode_reports)
	{
		fprintf(stderr, "###VIDEOINFO#%u#%u#%s#%s\n",
			hor_size, vert_size, aspect_ratio, framerate);
		fflush(stderr);
	}
}

void activity_message(const char *fmt, ...)
{
	if (ccx_options.gui_mode_reports)
	{
		va_list args;
		fprintf(stderr, "###MESSAGE#");
		va_start(args, fmt);
		fprintf(stderr, fmt, args);
		fprintf(stderr, "\n");
		va_end(args);
		fflush(stderr);
	}
}

void activity_input_file_closed(void)
{
	if (ccx_options.gui_mode_reports)
	{
		fprintf(stderr, "###INPUTFILECLOSED\n");
		fflush(stderr);
	}
}

void activity_program_number(unsigned program_number)
{
	if (ccx_options.gui_mode_reports)
	{
		fprintf(stderr, "###TSPROGRAMNUMBER#%u\n", program_number);
		fflush(stderr);
	}
}

void activity_report_version(void)
{
	if (ccx_options.gui_mode_reports)
	{
		fprintf(stderr, "###VERSION#CCExtractor#%s\n", VERSION);
		fflush(stderr);
	}
}

void activity_report_data_read(void)
{
	if (ccx_options.gui_mode_reports)
	{
		fprintf(stderr, "###DATAREAD#%lu\n", net_activity_gui / 1000);
		fflush(stderr);
	}
}

void activity_header(void)
{
	if (!credits_shown)
	{
		credits_shown = 1;
		mprint("CCExtractor %s, Carlos Fernandez Sanz, Volker Quetschke.\n", VERSION);
		mprint("Teletext portions taken from Petr Kutalek's telxcc\n");
		mprint("--------------------------------------------------------------------------\n");
	}
}
