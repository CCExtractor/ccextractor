#ifndef CCX_ENCODERS_STRUCTS_H
#define CCX_ENCODERS_STRUCTS_H

typedef struct ccx_encoders_transcript_format {
	// TODO: add more options, and (perhaps) reduce other ccextractor options?
	int showStartTime, showEndTime; // Show start and/or end time.
	int showMode; // Show which mode if available (E.G.: POP, RU1, ...)
	int showCC; // Show which CC channel has been captured.
	int relativeTimestamp; // Timestamps relative to start of sample or in UTC?
	int xds; // Show XDS or not
	int useColors; // Add colors or no colors
	int isFinal; // Used to determine if these parameters should be changed afterwards.

} ccx_encoders_transcript_format;

struct ccx_s_write
{
	int fh;
	char *filename;
	void* spupng_data;
};

#endif
