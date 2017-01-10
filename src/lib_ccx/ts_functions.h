#ifndef TS_FUNCTION_H
#define TS_FUNCTION_H

struct ts_payload
{
	unsigned char *start; // Payload start
	unsigned length;      // Payload length
	unsigned pesstart;    // PES or PSI start
	unsigned pid;         // Stream PID
	int counter;          // continuity counter
	int transport_error;  // 0 = packet OK, non-zero damaged
	int has_random_access_indicator; //1 = start of new GOP (Set when the stream may be decoded without errors from this point)
	int have_pcr;
	int64_t pcr;
	unsigned char section_buf[4098];
	int section_index;
	int section_size;
};

struct PAT_entry
{
	unsigned program_number;
	unsigned PMT_PID;
	unsigned char *last_pmt_payload;
	unsigned last_pmt_length;
};

struct PMT_entry
{
	unsigned program_number;
	unsigned elementary_PID;
	enum ccx_stream_type stream_type;
	unsigned printable_stream_type;
};

struct PSI_buffer
{
	uint32_t prev_ccounter;
	uint8_t *buffer;
	uint32_t buffer_length;
	uint32_t ccounter;
};

struct EPG_rating
{
	char country_code[4];
	uint8_t age;
};

struct EPG_event
{
	uint32_t id;
	char start_time_string[21]; //"YYYYMMDDHHMMSS +0000" = 20 chars
	char end_time_string[21];
	uint8_t running_status;
	uint8_t free_ca_mode;
	char ISO_639_language_code[4];
	char *event_name;
	char *text;
	char extended_ISO_639_language_code[4];
	char *extended_text;
	uint8_t has_simple;
	struct EPG_rating *ratings;
	uint32_t num_ratings;
	uint8_t *categories;
	uint32_t num_categories;
	uint16_t service_id;
	long long int count; //incremented by one each time the event is updated
	uint8_t live_output; //boolean flag, true if this event has been output
};

#define EPG_MAX_EVENTS 60*24*7
struct EIT_program
{
	uint32_t array_len;
	struct EPG_event epg_events[EPG_MAX_EVENTS];
};
#endif
