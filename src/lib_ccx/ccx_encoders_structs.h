#ifndef CCX_ENCODERS_STRUCTS_H
#define CCX_ENCODERS_STRUCTS_H

typedef struct ccx_encoders_transcript_format {
	// TODO: add more options, and (perhaps) reduce other ccextractor options?
	int showStartTime, showEndTime; // show start and/or end time.
	int showMode; // show which mode if available (E.G.: POP, RU1, ...)
	int showCC; // show which CC channel has been captured.
	int relativeTimestamp; // timestamps relative to start of sample or in UTC?
	int xds; // show XDS or not
	int useColors; // add colors or no colors
	int isFinal; // used to determine if these parameters should be changed afterwards.

} ccx_encoders_transcript_format;

struct ccx_s_write
{
	int fh;
	int temporarily_closed; // 1 means the file was created OK before but we released the handle
	char *filename;
	void* spupng_data;
	int with_semaphore; // 1 means create a .sem file when the file is open and delete it when it's closed
	char *semaphore_filename;
	int with_playlist; // for m3u8 /webvtt: If 1, we'll generate a playlist and segments
	char *playlist_filename; 
	int renaming_extension; // used for file rotations
	int append_mode; /* append the file. Prevent overwriting of files */

};

#endif
