#ifndef ACTIVITY_H
#define ACTIVITY_H

extern unsigned long net_activity_gui;
void activity_header (void);
void activity_progress (int percentaje, int cur_min, int cur_sec);
void activity_report_version (void);
void activity_input_file_closed (void);
void activity_input_file_open (const char *filename);
void activity_message (const char *fmt, ...);
void  activity_video_info (int hor_size,int vert_size,
    const char *aspect_ratio, const char *framerate);
void activity_program_number (unsigned program_number);
void activity_library_process(enum ccx_common_logging_gui message_type, ...);
void activity_report_data_read (void);

#endif
