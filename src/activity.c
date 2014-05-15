/* This file contains functions that report the user of the GUI of
relevant events. */

#include "ccextractor.h"

static int credits_shown=0;
unsigned long net_activity_gui=0;

/* Print current progress. For percentaje, -1 -> streaming mode */
void activity_progress (int percentaje, int cur_min, int cur_sec)
{
    if (!ccx_options.no_progress_bar)
    {
        if (percentaje==-1)
            mprint ("\rStreaming |  %02d:%02d", cur_min, cur_sec);
        else
            mprint ("\r%3d%%  |  %02d:%02d",percentaje, cur_min, cur_sec);
    }
    fflush (stdout);
    if (ccx_options.gui_mode_reports)
    {
        fprintf (stderr, "###PROGRESS#%d#%d#%d\n", percentaje, cur_min, cur_sec);
        fflush (stderr);
    }
}

void activity_input_file_open (const char *filename)
{
    if (ccx_options.gui_mode_reports)
    {
        fprintf (stderr, "###INPUTFILEOPEN#%s\n", filename);
        fflush (stderr);        
    }
}

void activity_xds_program_identification_number (unsigned minutes, unsigned hours, unsigned date, unsigned month)
{
    if (ccx_options.gui_mode_reports)
    {
		fprintf (stderr, "###XDSPROGRAMIDENTIFICATIONNUMBER#%u#%u#%u#%u\n", minutes,hours,date,month);
        fflush (stderr); 
    }
}

void activity_xds_network_call_letters (const char *program_name)
{
    if (ccx_options.gui_mode_reports)
    {
        fprintf (stderr, "###XDSNETWORKCALLLETTERS#%s\n", program_name);
        fflush (stderr);        
    }
}

void activity_xds_program_name (const char *program_name)
{
    if (ccx_options.gui_mode_reports)
    {
        fprintf (stderr, "###XDSPROGRAMNAME#%s\n", program_name);
        fflush (stderr);        
    }
}

void activity_xds_program_description (int line_num, const char *program_desc)
{
    if (ccx_options.gui_mode_reports)
    {
		fprintf (stderr, "###XDSPROGRAMDESC#%d#%s\n", line_num, program_desc);
        fflush (stderr);        
    }
}



void  activity_video_info (int hor_size,int vert_size, 
    const char *aspect_ratio, const char *framerate)
{
    if (ccx_options.gui_mode_reports)
    {
        fprintf (stderr, "###VIDEOINFO#%u#%u#%s#%s\n",
            hor_size,vert_size, aspect_ratio, framerate);
    fflush (stderr);             
    }
}
                            

void activity_message (const char *fmt, ...)
{
    if (ccx_options.gui_mode_reports)
    {
        va_list args;        
        fprintf (stderr, "###MESSAGE#");
        va_start(args, fmt);
        fprintf(stderr, fmt, args);
        fprintf(stderr, "\n");
        va_end(args);
        fflush (stderr);        
    }
}

void activity_input_file_closed (void)
{
    if (ccx_options.gui_mode_reports)
    {
        fprintf (stderr, "###INPUTFILECLOSED\n");
        fflush (stderr);        
    }
}

void activity_program_number (unsigned program_number)
{
	if (ccx_options.gui_mode_reports)
	{
		fprintf (stderr, "###TSPROGRAMNUMBER#%u\n",program_number);
		fflush (stderr);        
	}
}

void activity_report_version (void)
{
    if (ccx_options.gui_mode_reports)
    {
        fprintf (stderr, "###VERSION#CCExtractor#%s\n",VERSION);
        fflush (stderr);
    }
}

void activity_report_data_read (void)
{
    if (ccx_options.gui_mode_reports)
    {
        fprintf (stderr, "###DATAREAD#%lu\n",net_activity_gui/1000);
        fflush (stderr);
    }
}


void activity_header (void)
{
	if (!credits_shown)
	{
		credits_shown=1;
		mprint ("CCExtractor %s, Carlos Fernandez Sanz, Volker Quetschke.\n", VERSION);
		mprint ("Teletext portions taken from Petr Kutalek's telxcc\n");
		mprint ("--------------------------------------------------------------------------\n");		
	}
}

