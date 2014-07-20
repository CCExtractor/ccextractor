#ifndef _CC_DECODER_COMMON
#define _CC_DECODER_COMMON

/**
 * Raw Subtitle struct used as output of decoder (cc608) 
 * and input for encoder (sami, srt, transcript or smptett etc)
 */
struct cc_subtitle
{
	/**
         * A generic data which contain data according to decoder
	 * just now only struct cc_eia608_screen is placed here
	 * @warn decoder cant output multiple types of data
         */
	void *data;
	/** number of data */
	unsigned int nb_data;
	/** flag to tell that decoder has given output */
	int got_output;
};

/**
 * @param data raw cc608 data to be processed 
 *
 * @param length length of data passed
 *
 * @param context context of cc608 where important information related to 608
 * 		  are stored.
 * 
 * @param sub pointer to subtitle should be memset to 0 when passed first time
 *            subtitle are stored when structure return
 *
 * @return number of bytes used from data, -1 when any error is encountered
 */
int process608(const unsigned char *data, int length, struct s_context_cc608 *context, struct cc_subtitle *sub);

/**
 * Issue a EraseDisplayedMemory here so if there's any captions pending
 * they get written to cc_subtitle
 */
void handle_end_of_data(struct s_context_cc608 *context, struct cc_subtitle *sub);
#endif
