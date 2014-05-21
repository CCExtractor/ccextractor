#ifndef __608_H__

enum cc_modes
{
	MODE_POPON = 0,
	MODE_ROLLUP_2 = 1,
	MODE_ROLLUP_3 = 2,
	MODE_ROLLUP_4 = 3,
	MODE_TEXT = 4,
	MODE_PAINTON = 5,
	// Fake modes to emulate stuff
	MODE_FAKE_ROLLUP_1 = 100
};

struct eia608_screen // A CC buffer
{
	unsigned char characters[15][33];
	unsigned char colors[15][33];
	unsigned char fonts[15][33]; // Extra char at the end for a 0
	int row_used[15]; // Any data in row?
	int empty; // Buffer completely empty?    	
};

struct s_context_cc608
{
	struct eia608_screen buffer1;
	struct eia608_screen buffer2;
	int cursor_row, cursor_column;
	int visible_buffer;
	int srt_counter; // Number of subs currently written
	int screenfuls_counter; // Number of meaningful screenfuls written
	LLONG current_visible_start_ms; // At what time did the current visible buffer became so?
	// unsigned current_visible_start_cc; // At what time did the current visible buffer became so?
	enum cc_modes mode;
	unsigned char last_c1, last_c2;
	int channel; // Currently selected channel
	unsigned char color; // Color we are currently using to write
	unsigned char font; // Font we are currently using to write
	int rollup_base_row;
	LLONG ts_start_of_current_line; /* Time at which the first character for current line was received, =-1 no character received yet */
	LLONG ts_last_char_received; /* Time at which the last written character was received, =-1 no character received yet */
	int new_channel; // The new channel after a channel change		
	int my_field; // Used for sanity checks
	long bytes_processed_608; // To be written ONLY by process_608
	struct ccx_s_write *out;
};

#ifdef __cplusplus
extern "C"
{
#endif
extern unsigned char *enc_buffer;
extern unsigned char str[2048]; 
extern unsigned enc_buffer_used;
extern unsigned enc_buffer_capacity;
#ifdef __cplusplus
}
#endif

extern int new_sentence;
extern const char *color_text[][2];

int write_cc_buffer_as_srt(struct eia608_screen *data, struct s_context_cc608 *context);
void write_stringz_as_srt(char *string, struct s_context_cc608 *context, LLONG ms_start, LLONG ms_end);
unsigned get_decoder_line_encoded (unsigned char *buffer, int line_num, struct eia608_screen *data);
void capitalize (int line_num, struct eia608_screen *data);
void correct_case (int line_num, struct eia608_screen *data);
int write_cc_buffer_as_sami(struct eia608_screen *data, struct s_context_cc608 *context);
void write_stringz_as_sami(char *string, struct s_context_cc608 *context, LLONG ms_start, LLONG ms_end);
int write_cc_buffer_as_smptett(struct eia608_screen *data, struct s_context_cc608 *context);
void write_stringz_as_smptett(char *string, struct s_context_cc608 *context, LLONG ms_start, LLONG ms_end);
void correct_case (int line_num, struct eia608_screen *data);
void capitalize (int line_num, struct eia608_screen *data);
void find_limit_characters (unsigned char *line, int *first_non_blank, int *last_non_blank);
unsigned get_decoder_line_basic (unsigned char *buffer, int line_num, struct eia608_screen *data);
unsigned get_decoder_line_encoded_for_gui (unsigned char *buffer, int line_num, struct eia608_screen *data);
unsigned get_decoder_line_encoded (unsigned char *buffer, int line_num, struct eia608_screen *data);
void delete_all_lines_but_current (struct eia608_screen *data, int row);
void try_to_add_start_credits(struct s_context_cc608 *context);
void try_to_add_end_credits(struct s_context_cc608 *context);
void write_cc_buffer_to_gui(struct eia608_screen *data, struct s_context_cc608 *context);

void handle_end_of_data(struct s_context_cc608 *context);
void process608(const unsigned char *data, int length, struct s_context_cc608 *context);
void get_char_in_latin_1 (unsigned char *buffer, unsigned char c);
void get_char_in_unicode (unsigned char *buffer, unsigned char c);
int get_char_in_utf_8 (unsigned char *buffer, unsigned char c);
unsigned char cctolower (unsigned char c);
unsigned char cctoupper (unsigned char c);
int general_608_init (void);
LLONG get_visible_end (void);

#define CC608_SCREEN_WIDTH  32

#define REQUEST_BUFFER_CAPACITY(length) if (length>enc_buffer_capacity) \
{enc_buffer_capacity=length*2; enc_buffer=(unsigned char*) realloc (enc_buffer, enc_buffer_capacity); \
    if (enc_buffer==NULL) { fatal (EXIT_NOT_ENOUGH_MEMORY, "Not enough memory, bailing out\n"); } \
}


enum color_code
{
    COL_WHITE = 0,
    COL_GREEN = 1,
    COL_BLUE = 2,
    COL_CYAN = 3,
    COL_RED = 4,
    COL_YELLOW = 5,
    COL_MAGENTA = 6,
    COL_USERDEFINED = 7,
    COL_BLACK = 8,
    COL_TRANSPARENT = 9
};


enum font_bits
{
    FONT_REGULAR = 0,
    FONT_ITALICS = 1,
    FONT_UNDERLINED = 2,
    FONT_UNDERLINED_ITALICS = 3
};






enum command_code
{
    COM_UNKNOWN = 0,
    COM_ERASEDISPLAYEDMEMORY = 1,
    COM_RESUMECAPTIONLOADING = 2,
    COM_ENDOFCAPTION = 3,
    COM_TABOFFSET1 = 4,
    COM_TABOFFSET2 = 5,
    COM_TABOFFSET3 = 6,
    COM_ROLLUP2 = 7,
    COM_ROLLUP3 = 8,
    COM_ROLLUP4 = 9,
    COM_CARRIAGERETURN = 10,
    COM_ERASENONDISPLAYEDMEMORY = 11,
    COM_BACKSPACE = 12,
	COM_RESUMETEXTDISPLAY = 13,
	COM_ALARMOFF =14,
	COM_ALARMON = 15,
	COM_DELETETOENDOFROW = 16,
	COM_RESUMEDIRECTCAPTIONING = 17,
	// Non existing commands we insert to have the decoder
	// special stuff for us.
	COM_FAKE_RULLUP1 = 18
};



#define __608_H__
#endif
