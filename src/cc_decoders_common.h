#ifndef _CC_DECODER_COMMON
#define _CC_DECODER_COMMON

/* flag raised when end of display marker arrives in Dvb Subtitle */
#define SUB_EOD_MARKER (1 << 0 )
struct cc_bitmap
{
	int x;
	int y;
	int w;
	int h;
	int nb_colors;
	unsigned char *data[2];
	int linesize[2];
};
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
	/**  type of subtitle */
	enum subtype type;
	/* set only when all the data is to be displayed at same time */
	LLONG start_time;
	LLONG end_time;
	/* flags */
	int flags;
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
