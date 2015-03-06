#include "lib_ccx.h"
#include "ccx_common_option.h"
#include "dvb_subtitle_decoder.h"
#include "utility.h"
#include <stdbool.h>
#ifdef WIN32
	#include "..\\win_iconv\\win_iconv.c"
#else
	#include "iconv.h"
#endif


// Fills given string with given (event.*_time_string) ATSC time converted to XMLTV style time string
void EPG_ATSC_calc_time(char *output, uint32_t time) {
	struct tm timeinfo;
	timeinfo.tm_year = 1980-1900;
	timeinfo.tm_mon = 0;
	timeinfo.tm_mday = 6;
	timeinfo.tm_sec = 0+time;
	timeinfo.tm_min = 0;
	timeinfo.tm_hour = 0;
	timeinfo.tm_isdst = -1;
	mktime(&timeinfo);
	sprintf(output, "%02d%02d%02d%02d%02d%02d +0000", timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

// Fills event.start_time_string in XMLTV format with passed DVB time
void EPG_DVB_calc_start_time(struct EPG_event *event, uint64_t time) {
	uint64_t mjd = time>>24;
	event->start_time_string[0]='\0';
	if (mjd > 0) {
		long   y,m,d ,k;

		// algo: ETSI EN 300 468 - ANNEX C
		y =  (long) ((mjd  - 15078.2) / 365.25);
		m =  (long) ((mjd - 14956.1 - (long)(y * 365.25) ) / 30.6001);
		d =  (long) (mjd - 14956 - (long)(y * 365.25) - (long)(m * 30.6001));
		k =  (m == 14 || m == 15) ? 1 : 0;
		y = y + k + 1900;
		m = m - 1 - k*12;

		sprintf(event->start_time_string, "%02d%02d%02d%06x +0000",y,m,d,time&0xffffff);
	}
}

// Fills event.end_time_string in XMLTV with passed DVB time + duration
void EPG_DVB_calc_end_time(struct EPG_event *event, uint64_t time, uint32_t duration) {
	uint64_t mjd = time>>24;
	event->end_time_string[0]='\0';
	if (mjd > 0) {
		long   y,m,d ,k;
		struct tm timeinfo;

		// algo: ETSI EN 300 468 - ANNEX C
		y =  (long) ((mjd  - 15078.2) / 365.25);
		m =  (long) ((mjd - 14956.1 - (long)(y * 365.25) ) / 30.6001);
		d =  (long) (mjd - 14956 - (long)(y * 365.25) - (long)(m * 30.6001));
		k =  (m == 14 || m == 15) ? 1 : 0;
		y = y + k + 1900;
		m = m - 1 - k*12;

		timeinfo.tm_year = y-1900;
		timeinfo.tm_mon = m-1;
		timeinfo.tm_mday = d;
		
		timeinfo.tm_sec = (time&0x0f) + (10*((time&0xf0)>>4)) + (duration&0x0f) + (10*((duration&0xf0)>>4));
		timeinfo.tm_min = ((time&0x0f00)>>8) + (10*((time&0xf000)>>4)>>8) + ((duration&0x0f00)>>8) + (10*((duration&0xf000)>>4)>>8);
		timeinfo.tm_hour = ((time&0x0f0000)>>16) + (10*((time&0xf00000)>>4)>>16) + ((duration&0x0f0000)>>16) + (10*((duration&0xf00000)>>4)>>16);
		timeinfo.tm_isdst = -1;
		mktime(&timeinfo);
		sprintf(event->end_time_string, "%02d%02d%02d%02d%02d%02d +0000", timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
	}
}

// returns english string description of the passed DVB category ID
char *EPG_DVB_content_type_to_string(uint8_t cat) {
	struct table {
		uint8_t cat;
		char *name;
	};
	struct table t[] = {
		{0x00, "reserved"}, { 0x10, "movie/drama (general)" }, { 0x11, "detective/thriller" }, { 0x12, "adventure/western/war" }, { 0x13, "science fiction/fantasy/horror" },{ 0x14, "comedy" }, 
		{ 0x15, "soap/melodram/folkloric" }, { 0x16, "romance" }, { 0x17, "serious/classical/religious/historical movie/drama" }, { 0x18, "adult movie/drama" }, { 0x1E, "reserved" },
		{ 0x1F, "user defined" }, { 0x20, "news/current affairs (general)" }, { 0x21, "news/weather report" }, { 0x22, "news magazine" }, { 0x23, "documentary" }, { 0x24, "discussion/interview/debate" }, 
		{ 0x2E, "reserved" }, { 0x2F, "user defined" }, { 0x30, "show/game show (general)" }, { 0x31, "game show/quiz/contest" }, { 0x32, "variety show" }, { 0x33, "talk show" }, { 0x3E, "reserved" }, 
		{ 0x3F, "user defined" }, { 0x40, "sports (general)" }, { 0x41, "special events" }, { 0x42, "sports magazine" }, { 0x43, "football/soccer" }, { 0x44, "tennis/squash" }, { 0x45, "team sports" },
		{ 0x46, "athletics" }, { 0x47, "motor sport" }, { 0x48, "water sport" }, { 0x49, "winter sport" }, { 0x4A, "equestrian" }, { 0x4B, "martial sports" }, { 0x4E, "reserved" }, { 0x4F, "user defined" }, 
		{ 0x50, "childrens's/youth program (general)" }, { 0x51, "pre-school children's program" }, { 0x52, "entertainment (6-14 year old)" }, { 0x53, "entertainment (10-16 year old)" }, 
		{ 0x54, "information/education/school program" }, { 0x55, "cartoon/puppets" }, { 0x5E, "reserved" }, { 0x5F, "user defined" }, { 0x60, "music/ballet/dance (general)" }, { 0x61, "rock/pop" },
		{ 0x62, "serious music/classic music" }, { 0x63, "folk/traditional music" }, { 0x64, "jazz" }, { 0x65, "musical/opera" }, { 0x66, "ballet" }, { 0x6E, "reserved" }, { 0x6F, "user defined" }, 
		{ 0x70, "arts/culture (without music, general)" },{ 0x71, "performing arts" }, { 0x72, "fine arts" }, { 0x73, "religion" }, { 0x74, "popular culture/traditional arts" }, { 0x75, "literature" },
		{ 0x76, "film/cinema" }, { 0x77, "experimental film/video" }, { 0x78, "broadcasting/press" }, { 0x79, "new media" }, { 0x7A, "arts/culture magazine" }, { 0x7B, "fashion" }, { 0x7E, "reserved" },
		{ 0x7F, "user defined" }, { 0x80, "social/political issues/economics (general)" }, { 0x81, "magazines/reports/documentary" }, { 0x82, "economics/social advisory" }, { 0x83, "remarkable people" },
		{ 0x8E, "reserved" }, { 0x8F, "user defined" }, { 0x90, "education/science/factual topics (general)" }, { 0x91, "nature/animals/environment" }, { 0x92, "technology/natural science" }, 
		{ 0x93, "medicine/physiology/psychology" }, { 0x94, "foreign countries/expeditions" }, { 0x95, "social/spiritual science" }, { 0x96, "further education" }, { 0x97, "languages" },
		{ 0x9E, "reserved" }, { 0x9F, "user defined" }, { 0xA0, "leisure hobbies (general)" }, { 0xA1, "tourism/travel" }, { 0xA2, "handicraft" }, { 0xA3, "motoring" }, { 0xA4, "fitness & health" },
		{ 0xA5, "cooking" }, { 0xA6, "advertisement/shopping" }, { 0xA7, "gardening" }, { 0xAE, "reserved" }, { 0xAF, "user defined" }, { 0xB0, "original language" }, { 0xB1, "black & white" },
		{ 0xB2, "unpublished" }, { 0xB3, "live broadcast" }, { 0xBE, "reserved" }, { 0xBF, "user defined" }, { 0xEF, "reserved" }, { 0xFF, "user defined" }, {0x00, NULL},
	};
	struct table *p = t;
	while(p->name!=NULL) {
		if(cat==p->cat)
			return p->name;
		p++;
	}
	return "undefined content";
	
}

// Prints given event to already opened XMLTV file. 
void EPG_print_event(struct EPG_event *event, uint32_t channel, FILE *f) {
	int i;
	fprintf(f, "  <program  ");
	fprintf(f, "start=\"");
	fprintf(f, "%s", event->start_time_string);
	fprintf(f, "\" ");
	fprintf(f, "stop=\"");
	fprintf(f, "%s", event->end_time_string);
	fprintf(f, "\" ");
	fprintf(f, "channel=\"%i\">\n", channel);
	if(event->has_simple) {
		fprintf(f, "    <title lang=\"%s\">%s</title>\n",event->ISO_639_language_code, event->event_name);
		fprintf(f, "    <sub-title lang=\"%s\">%s</sub-title>\n",event->ISO_639_language_code, event->text);
	}
	if(event->extended_text!=NULL)
		fprintf(f, "    <desc lang=\"%s\">%s</desc>\n", event->extended_ISO_639_language_code, event->extended_text);

	for(i=0; i<event->num_ratings; i++)
		if(event->ratings[i].age>0 && event->ratings[i].age<0x10)
			fprintf(f, "    <rating system=\"dvb:%s\">%i</rating>\n", event->ratings[i].country_code, event->ratings[i].age+3);
	for(i=0; i<event->num_categories; i++)
		fprintf(f, "    <category lang=\"en\">%s</category>\n", EPG_DVB_content_type_to_string(event->categories[i]));
	fprintf(f, "    <ts-meta-id>%i</ts-meta-id>\n", event->id);
	fprintf(f, "  </program>\n");

	
}

// Creates fills and closes a new XMLTV file for live mode output.
// File should include only events not previously output.
void EPG_output_live(struct lib_ccx_ctx *ctx) {
	int c=false, i, j;
	FILE *f;
	char *filename, *finalfilename;
	for(i=0; i<pmt_array_length; i++) {
		for(j=0; j<ctx->eit_programs[i].array_len; j++)
			if(ctx->eit_programs[i].epg_events[j].live_output==false) {
			c=true;
			}
	}
	if(!c)
		return;

	filename = malloc(strlen(ctx->wbout1.filename)+30);
	sprintf(filename, "%s_%i.xml.part", ctx->basefilename, ctx->epg_last_live_output);
	f = fopen(filename, "w");

	fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE tv SYSTEM \"xmltv.dtd\">\n\n<tv>\n");
	for(i=0; i<pmt_array_length; i++) {
		fprintf(f, "  <channel id=\"%i\">\n", pmt_array[i].program_number);
		fprintf(f, "    <display-name>%i</display-name>\n", pmt_array[i].program_number);
		fprintf(f, "  </channel>\n");
	}
	for(i=0; i<pmt_array_length; i++) {
		for(j=0; j<ctx->eit_programs[i].array_len; j++)
			if(ctx->eit_programs[i].epg_events[j].live_output==false) {
				ctx->eit_programs[i].epg_events[j].live_output=true;
				EPG_print_event(&ctx->eit_programs[i].epg_events[j], pmt_array[i].program_number, f);
			}
	}
	fprintf(f, "</tv>");
	fclose(f);
	finalfilename = malloc(strlen(ctx->wbout1.filename)+30);
	memcpy(finalfilename, filename, strlen(filename)-5);
	finalfilename[strlen(filename)-5]='\0';
	rename(filename, finalfilename);
	free(filename);
	free(finalfilename);
}

// Creates fills and closes a new XMLTV file for full output mode.
// File should include all events in memory.
void EPG_output(struct lib_ccx_ctx *ctx) {
	FILE *f;
	char *filename;
	int i,j;
	filename = malloc(strlen(ctx->wbout1.filename)+9);
	memcpy(filename, ctx->basefilename, strlen(ctx->basefilename)+1);
	strcat(filename, "_epg.xml");
	f = fopen(filename, "w");
	free(filename);
	fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE tv SYSTEM \"xmltv.dtd\">\n\n<tv>\n");
	for(i=0; i<pmt_array_length; i++) {
		fprintf(f, "  <channel id=\"%i\">\n", pmt_array[i].program_number);
		fprintf(f, "    <display-name>%i</display-name>\n", pmt_array[i].program_number);
		fprintf(f, "  </channel>\n");
	}
	for(i=0; i<pmt_array_length; i++) {
		for(j=0; j<ctx->eit_programs[i].array_len; j++)
			EPG_print_event(&ctx->eit_programs[i].epg_events[j], pmt_array[i].program_number, f);
	}

	if(pmt_array_length==0) //Stream has no PMT, fall back to unordered events
		for(j=0; j<ctx->eit_programs[TS_PMT_MAP_SIZE].array_len; j++)
			EPG_print_event(&ctx->eit_programs[TS_PMT_MAP_SIZE].epg_events[j], ctx->eit_programs[TS_PMT_MAP_SIZE].epg_events[j].service_id, f);
	fprintf(f, "</tv>");
	fclose(f);
}

// Free all memory allocated for given event
void EPG_free_event(struct EPG_event *event) {
	if(event->has_simple) {
		free(event->event_name);
		free(event->text);
	}
	if(event->extended_text!=NULL)
		free(event->extended_text);
	if(event->num_ratings>0)
		free(event->ratings);
	if(event->num_categories>0)
		free(event->categories);
}

//compare 2 events. Return false if they are different.
int EPG_event_cmp(struct EPG_event *e1, struct EPG_event *e2) {
	if(e1->id != e2->id || (strcmp(e1->start_time_string, e2->start_time_string)!=0) || (strcmp(e1->end_time_string, e2->end_time_string)!=0))
		return false;
	// could add full checking of strings here if desired.
	return true;
}

// Add given event to array of events.
// Return FALSE if nothing changed, TRUE if this is a new or updated event.
int EPG_add_event(struct lib_ccx_ctx *ctx, int32_t pmt_map, struct EPG_event *event) {
	int isnew=true, j;
	
	for(j=0; j<ctx->eit_programs[pmt_map].array_len; j++) {
		if(ctx->eit_programs[pmt_map].epg_events[j].id==event->id) {
			if(EPG_event_cmp(event, &ctx->eit_programs[pmt_map].epg_events[j]))
				return false; //event already in array, nothing to do
			else { //event with this id is already in the array but something has changed. Update it.
				event->count=ctx->eit_programs[pmt_map].epg_events[j].count;
				EPG_free_event(&ctx->eit_programs[pmt_map].epg_events[j]);
				memcpy(&ctx->eit_programs[pmt_map].epg_events[j], event, sizeof(struct EPG_event));
				return true;
			}
		}
	}
	// id not in array. Add new event;
	event->count=0;
	memcpy(&ctx->eit_programs[pmt_map].epg_events[ctx->eit_programs[pmt_map].array_len], event, sizeof(struct EPG_event));
	ctx->eit_programs[pmt_map].array_len++;
	return true;
}

//EN 300 468 V1.3.1 (1998-02)
//6.2.4 Content descriptor
void EPG_decode_content_descriptor(uint8_t *offset, uint32_t descriptor_length, struct EPG_event *event) {
	int i;
	int num_items = descriptor_length/2;
	if(num_items == 0) {
		dbg_print (CCX_DMT_GENERIC_NOTICES, "\rWarning: Invalid EIT content_descriptor length detected.\n");
		return;
	}
	event->categories = malloc(1*num_items);
	event->num_categories = num_items;
	for(i=0; i<num_items; i++) {
		event->categories[i] = offset[0];
		offset+=2;
	}
}

//EN 300 468 V1.3.1 (1998-02)
//6.2.20 Parental rating descripto
void EPG_decode_parental_rating_descriptor(uint8_t *offset, uint32_t descriptor_length, struct EPG_event *event) {
	int i;
	int num_items = descriptor_length/4;
	struct EPG_rating *ratings;
	
	if(num_items == 0) {
		dbg_print (CCX_DMT_GENERIC_NOTICES, "\rWarning: Invalid EIT parental_rating length detected.\n");
		return;
	}
	event->ratings = malloc(sizeof(struct EPG_rating)*num_items);

	ratings = event->ratings;
	event->num_ratings = num_items;
	for(i=0; i<descriptor_length/4; i++) {
		ratings[i].country_code[0] = offset[0];
		ratings[i].country_code[1] = offset[1];
		ratings[i].country_code[2] = offset[2];
		ratings[i].country_code[3] = '\0';
		if(offset[3] == 0x00 || offset[3] >=0x10)
			ratings[i].age = 0;
		else
			ratings[i].age = offset[3];
		offset+=4;
	}
}

// an ungly function to convert from dvb codepages to UTF-8859-9 using iconv
// returns a null terminated UTF8-strings
// EN 300 468 V1.7.1 (2006-05)
// A.2 Selection of Character table
char* EPG_DVB_decode_string(uint8_t *in, size_t size) {
	uint8_t *out;
	uint16_t decode_buffer_size = (size*4)+1;
	uint8_t *decode_buffer = malloc(decode_buffer_size);
	char *dp = &decode_buffer[0];
	size_t obl=decode_buffer_size;
	uint16_t osize=0;
	iconv_t cd=(iconv_t)(-1);
	int skipiconv = false;
	int ret=-1;
	int x;
	if(size==0) { // 0 length strings are valid
		decode_buffer[0]='\0';
		return decode_buffer;
	}

	if(in[0]>=0x20) {
		skipiconv=true;
	}
	else if(in[0]==0x01) {
		size--;	in++;
		cd = iconv_open("UTF-8", "ISO8859-5"); //tested
	}
	else if(in[0]==0x02) {
		size--;	in++;
		cd = iconv_open("UTF-8", "ISO8859-6");
	}
	else if(in[0]==0x03) {
		size--;	in++;
		cd = iconv_open("UTF-8", "ISO8859-7");
	}
	else if(in[0]==0x04) {
		size--;	in++;
		cd = iconv_open("UTF-8", "ISO8859-8");
	}
	else if(in[0]==0x05) {
		size--;	in++;
		cd = iconv_open("UTF-8", "ISO8859-9");
	}
	else if(in[0]==0x06) {
		size--;	in++;
		cd = iconv_open("UTF-8", "ISO8859-10");
	}
	else if(in[0]==0x07) {
		size--;	in++;
		cd = iconv_open("UTF-8", "ISO8859-11");
	}
	else if(in[0]==0x08) {
		size--;	in++;
		cd = iconv_open("UTF-8", "ISO8859-12"); //This doesn't even exist?
	}
	else if(in[0]==0x09) {
		size--;	in++;
		cd = iconv_open("UTF-8", "ISO8859-13");
	}
	else if(in[0]==0x0a) {
		size--;	in++;
		cd = iconv_open("UTF-8", "ISO8859-14");
	}
	else if(in[0]==0x0b) {
		size--;	in++;
		cd = iconv_open("UTF-8", "ISO8859-15"); //tested, german
	}
	else if(in[0]==0x10) {
		char from[14];
		uint16_t cpn = (in[1] << 8) | in[2];
		size-=3; in+=3;
		snprintf(from, sizeof(from), "ISO8859-%d", cpn);
		cd = iconv_open("UTF-8", from);
	}
	else if(in[0]==0x11) {
		size--;	in++;
		cd = iconv_open("UTF-8", "ISO-10646/UTF8");
	}
	else if(in[0]==0x12) {
		size--;	in++;
		cd = iconv_open("UTF-8", "KS_C_5601-1987");
	}
	else if(in[0]==0x13) {
		size--;	in++;
		cd = iconv_open("UTF-8", "GB2312");
	}
	else if(in[0]==0x14) {
		size--;	in++;
		cd = iconv_open("UTF-8", "BIG-5");
	}
	else if(in[0]==0x15) {
		size--;	in++;
		cd = iconv_open("UTF-8", "UTF-8");	
	}
	else {
		dbg_print (CCX_DMT_GENERIC_NOTICES, "\rWarning: EPG_DVB_decode_string(): Reserved encoding detected: %02x.\n", in[0]);
		size--;	in++;
		cd = iconv_open("UTF-8", "ISO8859-9");
	}

	if((long)cd != -1 && !skipiconv) {
		ret = iconv(cd, (char **)&in, &size, &dp, &obl);
		obl=decode_buffer_size-obl;
		decode_buffer[obl]=0x00;
	}
	else {
		uint16_t newsize=0;
		if(!skipiconv)
			dbg_print (CCX_DMT_GENERIC_NOTICES, "\rWarning: EPG_DVB_decode_string(): Failed to convert codepage.\n");
		/* 
			http://dvbstreamer.sourcearchive.com/documentation/2.1.0-2/dvbtext_8c_source.html
			libiconv doesn't support ISO 6937, but glibc does?! 
			so fall back to ISO8859-1 and strip out the non spacing 
			diacritical mark/graphical characters etc.
			ALSO: http://lists.gnu.org/archive/html/bug-gnu-libiconv/2009-09/msg00000.html
		*/
		for(x=0; x<size; x++) {
			if(in[x]<=(uint8_t)127) {
				decode_buffer[newsize]=in[x];
				newsize++;	
			}
		}
		size=newsize;
		decode_buffer[size]=0x00;
	}
	osize=strlen(decode_buffer);
	out = malloc(osize+1);
	memcpy(out, decode_buffer, osize);
	out[osize]=0x00;
	free(decode_buffer);
	if (cd != (iconv_t)-1)
		iconv_close(cd);
	return out;
}

//EN 300 468 V1.3.1 (1998-02)
//6.2.27 Short event descriptor
void EPG_decode_short_event_descriptor(uint8_t *offset, uint32_t descriptor_length, struct EPG_event *event) {
	uint8_t text_length;
	uint8_t event_name_length;
	event->has_simple=true;
	event->ISO_639_language_code[0] = offset[0];
	event->ISO_639_language_code[1] = offset[1];
	event->ISO_639_language_code[2] = offset[2];
	event->ISO_639_language_code[3] = 0x00;

	event_name_length = offset[3];
	if(event_name_length+4>descriptor_length) {
		dbg_print (CCX_DMT_GENERIC_NOTICES, "\rWarning: Invalid short_event_descriptor size detected.\n");
		return;
	}
	event->event_name = EPG_DVB_decode_string(&offset[4], event_name_length);

	text_length = offset[4+event_name_length];
	if(text_length+event_name_length+4>descriptor_length) {
		dbg_print (CCX_DMT_GENERIC_NOTICES, "\rWarning: Invalid short_event_descriptor size detected.\n");
		return;
	}
	event->text = EPG_DVB_decode_string(&offset[5+event_name_length], text_length);
	
}

//EN 300 468 V1.3.1 (1998-02)
//6.2.9 Extended event descriptor
void EPG_decode_extended_event_descriptor(uint8_t *offset, uint32_t descriptor_length, struct EPG_event *event) {
	uint8_t descriptor_number=offset[0]>>4;
	uint8_t last_descriptor_number=(offset[0]&0x0f);
	uint32_t text_length;
	uint32_t oldlen=0;
	uint8_t length_of_items = offset[4];
	event->extended_ISO_639_language_code[0] = offset[1];
	event->extended_ISO_639_language_code[1] = offset[2];
	event->extended_ISO_639_language_code[2] = offset[3];
	event->extended_ISO_639_language_code[3] = 0x00;
	
	offset=offset+length_of_items+5;
	if(length_of_items>descriptor_length-5) {
		dbg_print (CCX_DMT_GENERIC_NOTICES, "\rWarning: Invalid extended_event_descriptor size detected.\n");
		return;
	}

	text_length = offset[0];
	if(text_length>descriptor_length-5-length_of_items-1) {
		dbg_print (CCX_DMT_GENERIC_NOTICES, "\rWarning: Invalid extended_event_text_length size detected.\n");
		return;
	}
	
	//TODO: can this leak memory with a malformed descriptor?
	if(descriptor_number>0) {
		if(offset[1]<0x20) {
			offset++;
			text_length--;
		}
		uint8_t *net = malloc(strlen(event->extended_text)+text_length+1);
		oldlen=strlen(event->extended_text);
		memcpy(net, event->extended_text, strlen(event->extended_text));
		free(event->extended_text);
		event->extended_text=net;
	}
	else
		event->extended_text = malloc(text_length+1);

	memcpy(&event->extended_text[oldlen], &offset[1], text_length);

	event->extended_text[oldlen + text_length] = '\0';
	
	if(descriptor_number==last_descriptor_number) {
		uint8_t *old = event->extended_text;
		event->extended_text = EPG_DVB_decode_string(event->extended_text, strlen(event->extended_text));
		free(old);
	}
	
}

// decode an ATSC multiple_string
// extremly basic implementation
// only handles single segment, single language ANSI string!
void EPG_ATSC_decode_multiple_string(uint8_t *offset, uint32_t length, struct EPG_event *event) {
	uint8_t number_strings = offset[0];
	int i, j;
	char ISO_639_language_code[4];
	offset++;
	for(i=0; i<number_strings; i++) {
		uint8_t number_segments = offset[3];
		ISO_639_language_code[0] = offset[0];
		ISO_639_language_code[1] = offset[1];
		ISO_639_language_code[2] = offset[2];
		ISO_639_language_code[3] = 0x00;
		offset+=4;
		for (j=0; j< number_segments; j++) {
			uint8_t compression_type = offset[0];
			uint8_t mode = offset[1];
			uint8_t number_bytes = offset[2];
			offset+=3;
			if(mode==0 && compression_type==0 && j==0) {
				event->has_simple=true;
				event->ISO_639_language_code[0]=ISO_639_language_code[0];
				event->ISO_639_language_code[1]=ISO_639_language_code[1];
				event->ISO_639_language_code[2]=ISO_639_language_code[2];
				event->ISO_639_language_code[3]=0x00;
				event->event_name = malloc(number_bytes+1);
				memcpy(event->event_name, &offset[0], number_bytes);
				event->event_name[number_bytes]='\0';
				event->text = malloc(number_bytes+1);
				memcpy(event->text, &offset[0], number_bytes);
				event->text[number_bytes]='\0';
			}
			else {
				dbg_print (CCX_DMT_GENERIC_NOTICES, "\rWarning: Unsupported ATSC multiple_string encoding detected!.\n");
			}
			offset+=number_bytes;
		}
	}
}

// decode ATSC EIT table.
void EPG_ATSC_decode_EIT(struct lib_ccx_ctx *ctx, uint8_t *payload_start, uint32_t size) {
	uint8_t table_id = payload_start[0];
	struct EPG_event event;
	uint8_t num_events_in_section;
	uint8_t *offset;
	int hasnew=false;
	int i, j;
	uint16_t source_id = ((payload_start[3]) << 8) | payload_start[4];
	int32_t pmt_map = -1;
	event.has_simple=false;
	event.extended_text=NULL;
	event.num_ratings=0;
	event.num_categories=0;
	event.live_output=false;
	for (i = 0; i < pmt_array_length; i++)
	{
		if (pmt_array[i].program_number == ctx->ATSC_source_pg_map[source_id])
			pmt_map=i;
	}

	//Don't know how to stroe EPG until we know the programs. Ignore it.
	if(pmt_map==-1)
		pmt_map=TS_PMT_MAP_SIZE;
	
	num_events_in_section = payload_start[9];
	offset=&payload_start[10];

	for(j=0; j<num_events_in_section; j++) {
		uint16_t descriptors_loop_length;
		uint8_t title_length, emt_location;
		uint32_t length_in_seconds, start_time, full_id;
		uint16_t event_id = ((offset[0]&0x3F) << 8) | offset[1];
		full_id = (source_id << 16) | event_id;
		event.id=full_id;
		event.service_id=source_id;
		start_time = (offset[2] << 24) | (offset[3] << 16) | (offset[4] << 8)| (offset[5] << 0);
		EPG_ATSC_calc_time(event.start_time_string, start_time);
		emt_location = (offset[6]&0x30)>>4;
		length_in_seconds = (((offset[6]&0x0F) << 16) | (offset[7] << 8) | (offset[8] << 0));
		EPG_ATSC_calc_time(event.end_time_string, start_time+length_in_seconds);
		title_length = offset[9];
		EPG_ATSC_decode_multiple_string(&offset[10], title_length, &event);

		descriptors_loop_length = ((offset[10+title_length] & 0x0f) << 8) | offset[10+title_length+1];
		
		hasnew |= EPG_add_event(ctx, pmt_map, &event);
		offset+=12+descriptors_loop_length+title_length;
	}
	if((ccx_options.xmltv==1 || ccx_options.xmltv==3) && ccx_options.xmltvoutputinterval==0 && hasnew)
		EPG_output(ctx);
}

// decode ATSC VCT table.
void EPG_ATSC_decode_VCT(struct lib_ccx_ctx *ctx, uint8_t *payload_start, uint32_t size) {
	uint8_t table_id = payload_start[0];
	uint8_t num_channels_in_section = payload_start[9];
	uint8_t *offset = &payload_start[10];
	int i;
	for (i=0; i< num_channels_in_section; i++) {
		char short_name[7*2];
		uint16_t program_number = offset[24]<<8 | offset[25];
		uint16_t source_id = offset[28]<<8 | offset[29];
		uint16_t descriptors_loop_length = ((offset[30] & 0x03) << 8) | offset[31];
		memcpy(short_name, &offset[0], 7*2);
		offset+=32+descriptors_loop_length;
		ctx->ATSC_source_pg_map[source_id]=program_number;
	}
}


void EPG_DVB_decode_EIT(struct lib_ccx_ctx *ctx, uint8_t *payload_start, uint32_t size) {

	uint8_t table_id = payload_start[0];
	//uint8_t section_syntax_indicator = (0xf1&0x80)>>7;
	uint16_t section_length = (payload_start[1]&0x0F)<<8 | payload_start[2];
	uint16_t service_id = (payload_start[3] << 8) | payload_start[4];
	int32_t pmt_map = -1;
	int i;
	int hasnew=false;
	struct EPG_event event;
	uint8_t section_number = payload_start[6];
	uint8_t last_section_number = payload_start[7];
	uint8_t segment_last_section_number = payload_start[12];
	uint32_t events_length = section_length - 11;
	uint8_t *offset=payload_start;
	uint32_t remaining=events_length;
	for (i = 0; i < pmt_array_length; i++)
	{
		if (pmt_array[i].program_number == service_id)
			pmt_map=i;
	}

	//For any service we don't have an PMT for (yet), store it in the special last array pos.
	if(pmt_map==-1)
		pmt_map=TS_PMT_MAP_SIZE;

	if(events_length>size-14) {
		dbg_print (CCX_DMT_GENERIC_NOTICES, "\rWarning: Invalid EIT packet size detected.\n");
	}

	while(remaining>4) {	
		uint16_t descriptors_loop_length;
		uint8_t *descp;
		uint32_t duration;
		uint64_t start_time;
		event.id=(offset[14] << 8) | offset[15];
		event.has_simple=false;
		event.extended_text=NULL;
		event.num_ratings=0;
		event.num_categories=0;
		event.live_output=false;
		event.service_id=service_id;

		//40 bits
		start_time = ((uint64_t)offset[16] << 32) | ((uint64_t)offset[17] << 24) | ((uint64_t)offset[18] << 16) | ((uint64_t)offset[19] << 8)| ((uint64_t)offset[20] << 0);
		EPG_DVB_calc_start_time(&event, start_time);
		//24 bits
		duration = (offset[21] << 16 ) | (offset[22] << 8 ) | (offset[23] << 0 );
		EPG_DVB_calc_end_time(&event, start_time, duration);
		event.running_status = (offset[24] & 0xE0) >> 5;
		event.free_ca_mode = (offset[24] & 0x10) >> 4;
		//12 bits
		descriptors_loop_length = ((offset[24] & 0x0f) << 8) | offset[25];
		descp = &offset[26];
		if(descriptors_loop_length > remaining-16) {
			dbg_print (CCX_DMT_GENERIC_NOTICES, "\rWarning: Invalid EIT descriptors_loop_length detected.\n");
			return;
		}
		while(descp<&(offset[26])+descriptors_loop_length) {
			if(descp+descp[1]+2>payload_start+size) {
				dbg_print (CCX_DMT_GENERIC_NOTICES, "\rWarning: Invalid EIT descriptor_loop_length detected.\n");
				EPG_free_event(&event);
				return;
			}
			if(descp[0]==0x4d)
				EPG_decode_short_event_descriptor(descp+2, descp[1], &event);
			if(descp[0]==0x4e)
				EPG_decode_extended_event_descriptor(descp+2, descp[1], &event);
			if(descp[0]==0x54)
				EPG_decode_content_descriptor(descp+2, descp[1], &event);
			if(descp[0]==0x55)
				EPG_decode_parental_rating_descriptor(descp+2, descp[1], &event);
			if(descp[1]+2==0) {
				dbg_print (CCX_DMT_GENERIC_NOTICES, "\rWarning: Invalid EIT descriptor_length detected.\n");
				return;
			}
			descp=descp+(descp[1]+2);
		}
		remaining=remaining-(descriptors_loop_length+12);
		offset=offset+descriptors_loop_length+12;
		hasnew |= EPG_add_event(ctx, pmt_map, &event);

		if(hasnew && section_number==0 && table_id==0x4e)
			ctx->eit_current_events[pmt_map]=event.id;
	}
	
	if((ccx_options.xmltv==1 || ccx_options.xmltv==3) && ccx_options.xmltvoutputinterval==0 && hasnew)
		EPG_output(ctx);
	
}
	//handle outputing to xml files
void EPG_handle_output(struct lib_ccx_ctx *ctx) {

	int cur_sec = (int) ((ctx->global_timestamp-ctx->min_global_timestamp) / 1000);
	if(ccx_options.xmltv==1 || ccx_options.xmltv==3) { //full outout
		if(ccx_options.xmltvoutputinterval!=0 && cur_sec>ctx->epg_last_output+ccx_options.xmltvliveinterval) {
			ctx->epg_last_output=cur_sec;
			EPG_output(ctx);
		}
	}
	if(ccx_options.xmltv==2 || ccx_options.xmltv==3) { //live output
		if(cur_sec>ctx->epg_last_live_output+ccx_options.xmltvliveinterval) {
			ctx->epg_last_live_output=cur_sec;
			EPG_output_live(ctx);
		}
	}
}

//determin table type and call the correct function to handle it
void EPG_parse_table(struct lib_ccx_ctx *ctx, uint8_t *b, uint32_t size) {
	uint8_t pointer_field=b[0];
	uint8_t *payload_start = &b[pointer_field + 1];
	uint8_t table_id = payload_start[0];
	
	switch (table_id) {
	case 0x0cb:
		EPG_ATSC_decode_EIT(ctx, payload_start, size);
		break;
	case 0xc8:
		EPG_ATSC_decode_VCT(ctx, payload_start, size);
		break;
	default:
		if (table_id>=0x4e && table_id<=0x6f)
			EPG_DVB_decode_EIT(ctx, payload_start, size);
		break;
	}
	EPG_handle_output(ctx);
}

// recounsructs DVB EIT and ATSC tables
int parse_EPG_packet(struct lib_ccx_ctx *ctx) {
	unsigned char *payload_start = tspacket + 4;
	unsigned payload_length = 188 - 4;
	unsigned transport_error_indicator = (tspacket[1]&0x80)>>7;
	unsigned payload_start_indicator = (tspacket[1]&0x40)>>6;
	// unsigned transport_priority = (tspacket[1]&0x20)>>5;
	unsigned pid = (((tspacket[1] & 0x1F) << 8) | tspacket[2]) & 0x1FFF;
	// unsigned transport_scrambling_control = (tspacket[3]&0xC0)>>6;
	unsigned adaptation_field_control = (tspacket[3]&0x30)>>4;
	unsigned ccounter = tspacket[3] & 0xF;
	unsigned adaptation_field_length = 0;
	int buffer_map = 0xfff;
	if ( adaptation_field_control & 2 )
	{
		adaptation_field_length = tspacket[4];
		payload_start = payload_start + adaptation_field_length + 1;
		payload_length = tspacket+188-payload_start;
	}
	
	if((pid!=0x12 && pid!=0x1ffb && pid<0x1000) || pid==0x1fff)
		return 0;
	
	if(pid!=0x12)
		buffer_map = pid-0x1000;

	if(payload_start_indicator) {
		if(ctx->epg_buffers[buffer_map].ccounter>0) {
			ctx->epg_buffers[buffer_map].ccounter=0;
			EPG_parse_table(ctx, ctx->epg_buffers[buffer_map].buffer, ctx->epg_buffers[buffer_map].buffer_length);
		}


		ctx->epg_buffers[buffer_map].prev_ccounter = ccounter;
		
		if(ctx->epg_buffers[buffer_map].buffer!=NULL)
			free(ctx->epg_buffers[buffer_map].buffer);
		else {
			// must be first EIT packet
		}
		ctx->epg_buffers[buffer_map].buffer = (uint8_t *)malloc(payload_length);
		memcpy(ctx->epg_buffers[buffer_map].buffer, payload_start, payload_length);
		ctx->epg_buffers[buffer_map].buffer_length=payload_length;
		ctx->epg_buffers[buffer_map].ccounter++;
		
	}
	else if(ccounter==ctx->epg_buffers[buffer_map].prev_ccounter+1 || (ctx->epg_buffers[buffer_map].prev_ccounter==0x0f && ccounter==0)) {
		ctx->epg_buffers[buffer_map].prev_ccounter = ccounter;
		ctx->epg_buffers[buffer_map].buffer = (uint8_t *)realloc(ctx->epg_buffers[buffer_map].buffer, ctx->epg_buffers[buffer_map].buffer_length+payload_length);
		memcpy(ctx->epg_buffers[buffer_map].buffer+ctx->epg_buffers[buffer_map].buffer_length, payload_start, payload_length);
		ctx->epg_buffers[buffer_map].ccounter++;
		ctx->epg_buffers[buffer_map].buffer_length+=payload_length;
	}
	else {
		dbg_print (CCX_DMT_GENERIC_NOTICES, "\rWarning: Out of order EPG packets detected.\n");
	}
}

// Free all memory used for EPG parsing
void EPG_free(struct lib_ccx_ctx *ctx) {
	int i = 0, j;
	if(ccx_options.xmltv==2 || ccx_options.xmltv==3) {
		EPG_output_live(ctx);
	}
	for (i = 0; i < TS_PMT_MAP_SIZE; i++)
	{
		for(j = 0; j < ctx->eit_programs[i].array_len; j++)
		{
			if(ctx->eit_programs[i].epg_events[j].has_simple) {
				free(ctx->eit_programs[i].epg_events[j].event_name);
				free(ctx->eit_programs[i].epg_events[j].text);
			}
			if(ctx->eit_programs[i].epg_events[j].extended_text!=NULL)
				free(ctx->eit_programs[i].epg_events[j].extended_text);
			if(ctx->eit_programs[i].epg_events[j].num_ratings>0)
				free(ctx->eit_programs[i].epg_events[j].ratings);
			if(ctx->eit_programs[i].epg_events[j].num_categories>0)
				free(ctx->eit_programs[i].epg_events[j].categories);
		}
		ctx->eit_programs[i].array_len=0;
	}
	
	for (i = 0; i < 0xfff; i++) {
		if(ctx->epg_buffers[i].buffer!=NULL)
			free(ctx->epg_buffers[i].buffer);
	}
}