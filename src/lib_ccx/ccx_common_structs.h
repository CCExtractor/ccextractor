#ifndef _CC_COMMON_STRUCTS
#define _CC_COMMON_STRUCTS

#include "ccx_common_constants.h"

enum ccx_common_logging_gui {
	CCX_COMMON_LOGGING_GUI_XDS_PROGRAM_NAME,        // Called with xds_program_name
	CCX_COMMON_LOGGING_GUI_XDS_PROGRAM_ID_NR,       // Called with current_xds_min, current_xds_hour, current_xds_date, current_xds_month
	CCX_COMMON_LOGGING_GUI_XDS_PROGRAM_DESCRIPTION, // Called with line_num, xds_desc
	CCX_COMMON_LOGGING_GUI_XDS_CALL_LETTERS         // Called with current_xds_call_letters
};

struct ccx_common_logging_t {
	LLONG debug_mask;                                              // The debug mask that is used to determine if things should be printed or not.
	void(*fatal_ftn) (int exit_code, const char *fmt, ...);        // Used when an unrecoverable error happens. This should log output/save the error and then exit the program.
	void(*debug_ftn) (LLONG mask, const char *fmt, ...);           // Used to process debug output. Mask can be ignored (custom set by debug_mask).
	void(*log_ftn)(const char *fmt, ...);                          // Used to print things. Replacement of standard printf, to allow more control.
	void(*gui_ftn)(enum ccx_common_logging_gui message_type, ...); // Used to display things in a gui (if appropriate). Is called with the message_type and appropriate variables (described in enum)
};
extern struct ccx_common_logging_t ccx_common_logging;

enum subdatatype
{
	CC_DATATYPE_GENERIC=0,
	CC_DATATYPE_DVB=1
};

enum subtype
{
	CC_BITMAP,
	CC_608,
	CC_708,
	CC_TEXT,
	CC_RAW,
};

/**
* Raw Subtitle struct used as output of decoder (cc608)
* and input for encoder (sami, srt, transcript or smptett etc)
*
* if subtype CC_BITMAP then data contain nb_data numbers of rectangle
* which have to be displayed at same time.
*/
struct cc_subtitle
{
	/**
	* A generic data which contain data according to decoder
	* @warn decoder cant output multiple types of data
	*/
	void *data;
	enum subdatatype datatype;

	/** number of data */
	unsigned int nb_data;

	/**  type of subtitle */
	enum subtype type;

	/** Encoding type of Text, must be ignored in case of subtype as bitmap or cc_screen*/
	enum ccx_encoding_type  enc_type;

	/* set only when all the data is to be displayed at same time */
	LLONG start_time;
	LLONG end_time;

	/* flags */
	int flags;

	/* index of language table */
	int lang_index;

	/** flag to tell that decoder has given output */
	int got_output;
	
	char mode[5];
	char info[4];

	struct cc_subtitle *next;
	struct cc_subtitle *prev;
};
#endif
