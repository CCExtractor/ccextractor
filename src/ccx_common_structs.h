#ifndef _CC_COMMON_STRUCTS
#define _CC_COMMON_STRUCTS

enum ccx_common_logging_gui {
	CCX_COMMON_LOGGING_GUI_XDS_PROGRAM_NAME, // Called with xds_program_name
	CCX_COMMON_LOGGING_GUI_XDS_PROGRAM_ID_NR, // Called with current_xds_min, current_xds_hour, current_xds_date, current_xds_month
	CCX_COMMON_LOGGING_GUI_XDS_PROGRAM_DESCRIPTION, // Called with line_num, xds_desc
	CCX_COMMON_LOGGING_GUI_XDS_CALL_LETTERS // Called with current_xds_call_letters
};

struct ccx_common_logging_t {
	int debug_mask; // The debug mask that is used to determine if things should be printed or not.	
	void(*fatal_ftn) (int exit_code, const char *fmt, ...); // Used when an unrecoverable error happens. This should log output/save the error and then exit the program.
	void(*debug_ftn) (LLONG mask, const char *fmt, ...); // Used to process debug output. Mask can be ignored (custom set by debug_mask).
	void(*log_ftn)(const char *fmt, ...); // Used to print things. Replacement of standard printf, to allow more control.
	void(*gui_ftn)(enum ccx_common_logging_gui message_type, ...); // Used to display things in a gui (if appropriate). Is called with the message_type and appropriate variables (described in enum)
} ccx_common_logging;
#endif