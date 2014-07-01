#include "ccextractor.h"
#include "wtv_constants.h"

int check_stream_id(int stream_id, int video_streams[], int num_streams);
int add_skip_chunks(struct wtv_chunked_buffer *cb, uint32_t offset, uint32_t flag);
void init_chunked_buffer(struct wtv_chunked_buffer *cb);
uint64_t get_meta_chunk_start(uint64_t offset);
uint64_t time_to_pes_time(uint64_t time);
void add_chunk(struct wtv_chunked_buffer *cb, uint64_t value);
int qsort_cmpint (const void * a, const void * b);
void get_sized_buffer(struct wtv_chunked_buffer *cb, uint32_t size);
void skip_sized_buffer(struct wtv_chunked_buffer *cb, uint32_t size);
int read_header(struct wtv_chunked_buffer *cb);
LLONG get_data(struct wtv_chunked_buffer *cb);

// Helper function for qsort (64bit int sort)
int qsort_cmpint (const void * a, const void * b)
{
    if(a<b) return -1;
    if(a>b) return 1;
    return 0;
}

// Check if the passed stream_id is in the list of stream_ids we care about
// Return true if it is.
int check_stream_id(int stream_id, int video_streams[], int num_streams) {
    int x;
    for(x=0; x<num_streams; x++) 
        if(video_streams[x]==stream_id)
            return 1;
    return 0;
}

// Init passes wtv_chunked_buffer struct
void init_chunked_buffer(struct wtv_chunked_buffer *cb) {
    cb->count=0;
    cb->buffer=NULL;
    cb->buffer_size=0;
    int x;
    for(x=0; x<256; x++)
        cb->skip_chunks[x]=-1;
}

// Calculate the actual file offset of the passed skip value.
uint64_t get_meta_chunk_start(uint64_t offset) {
	return offset*WTV_CHUNK_SIZE&WTV_META_CHUNK_MASK;
}

// Convert WTV time to PES time
uint64_t time_to_pes_time(uint64_t time) 
{
	return ((time/10000)*90);
}

// Read the actual values of the passed lookup offset and add them to
// the list of chunks to skip as nessasary. Returns false on error.
int add_skip_chunks(struct wtv_chunked_buffer *cb, uint32_t offset, uint32_t flag)
{

	uint64_t start = past;
	buffered_seek((int)((offset*WTV_CHUNK_SIZE) - start));
    uint64_t seek_back=0-((offset*WTV_CHUNK_SIZE)-start);

	uint32_t value;
	buffered_read((unsigned char*)&value, 4);
    if(result!=4)
        return 0;
    seek_back-=4;
    while(value!=0) {
        dbg_print(CCX_DMT_PARSE, "value: %llx\n", get_meta_chunk_start(value));
        buffered_read((unsigned char*)&value, 4);
        if(result!=4)
            return 0;
        add_chunk(cb, get_meta_chunk_start(value));
        seek_back-=4;
    }
    buffered_seek((int)seek_back);
    dbg_print(CCX_DMT_PARSE, "filebuffer_pos: %x\n", filebuffer_pos);
    return 1;
}

void add_chunk(struct wtv_chunked_buffer *cb, uint64_t value)
{
    int x;
    for(x=0; x<cb->count; x++)
        if(cb->skip_chunks[x]==value)
            return;
    cb->skip_chunks[cb->count]=value;
    cb->count++;
}

// skip_sized_buffer. Same as get_sized_buffer, only without actually copying any data
// in to the buffer.
void skip_sized_buffer(struct wtv_chunked_buffer *cb, uint32_t size) {
    if(cb->buffer!=NULL && cb->buffer_size>0) {
        free(cb->buffer);
    }
    cb->buffer=NULL;
    cb->buffer_size=0;
    uint64_t start = cb->filepos;
    if(cb->skip_chunks[cb->chunk]!=-1 && start+size>cb->skip_chunks[cb->chunk]) {
        buffered_seek((int)((cb->skip_chunks[cb->chunk]-start)+(WTV_META_CHUNK_SIZE)+(size-(cb->skip_chunks[cb->chunk]-start))));
        cb->filepos+=(cb->skip_chunks[cb->chunk]-start)+(WTV_META_CHUNK_SIZE)+(size-(cb->skip_chunks[cb->chunk]-start));
        cb->chunk++;
    }
    else {
        buffered_seek(size);
        cb->filepos+=size;
    }
    past=cb->filepos;
}

// get_sized_buffer will alloc and set a buffer in the passed wtv_chunked_buffer struct
// it will handle any meta data chunks that need to be skipped in the file
// Will print error messages and return a null buffer on error.
void get_sized_buffer(struct wtv_chunked_buffer *cb, uint32_t size) {
    if(cb->buffer!=NULL && cb->buffer_size>0) {
        free(cb->buffer);
    }
    if(size>WTV_MAX_ALLOC) {
        mprint("\nRequested buffer of %i > %i bytes (WTV_MAX_ALLOC)!\n", size, WTV_MAX_ALLOC);
        cb->buffer=NULL;
        return;
    }
	cb->buffer = (uint8_t*)malloc(size);
    cb->buffer_size=size;
	uint64_t start = cb->filepos;

	if(cb->skip_chunks[cb->chunk]!=-1 && start+size>cb->skip_chunks[cb->chunk]) {
        buffered_read(cb->buffer, (int)(cb->skip_chunks[cb->chunk]-start));
        cb->filepos+=cb->skip_chunks[cb->chunk]-start;
        buffered_seek(WTV_META_CHUNK_SIZE);
        cb->filepos+=WTV_META_CHUNK_SIZE;
        buffered_read(cb->buffer+(cb->skip_chunks[cb->chunk]-start), (int)(size-(cb->skip_chunks[cb->chunk]-start)));
        cb->filepos+=size-(cb->skip_chunks[cb->chunk]-start);
        cb->chunk++;
	}
	else {
		buffered_read(cb->buffer, size);
        cb->filepos+=size;
        if(result!=size) {
            free(cb->buffer);
            cb->buffer_size=0;
            past=cb->filepos;
            cb->buffer=NULL;
            mprint("\nPremature end of file!\n");
            return;
        }           
    }
    past=cb->filepos;
	return;
}

// read_header will read all the required information from
// the wtv header/root sections and calculate the skip_chunks.
// If successful, will return with the file positioned 
// at the start of the data dir
int read_header(struct wtv_chunked_buffer *cb) {
	startbytes_avail = (int)buffered_read_opt(startbytes, STARTBYTESLENGTH);
	return_to_buffer(startbytes, startbytes_avail);

    uint8_t *parsebuf;
    parsebuf = (uint8_t*)malloc(1024);
    buffered_read(parsebuf,0x42);
    past+=result;
    if (result!=0x42)
    {
        mprint("\nPremature end of file!\n");
        end_of_file=1;
        return 0;
    }
    // Expecting WTV header
    if( !memcmp(parsebuf, WTV_HEADER, 16 ) )
    {
		dbg_print(CCX_DMT_PARSE, "\nWTV header\n");
    }
    else
    {
        mprint("\nMissing WTV header. Abort.\n");
        end_of_file=1;
        return 0;
    }

    //Next read just enough to get the location of the root directory
    uint32_t filelen;
    uint32_t root_dir;
    memcpy(&filelen, parsebuf+0x30, 4);
    dbg_print(CCX_DMT_PARSE, "filelen: %x\n", filelen);
    memcpy(&root_dir, parsebuf+0x38, 4);
    dbg_print(CCX_DMT_PARSE, "root_dir: %x\n", root_dir);   

    //Seek to start of the root dir. Typically 0x1100
    buffered_skip((root_dir*WTV_CHUNK_SIZE)-0x42);
    past+=(root_dir*WTV_CHUNK_SIZE)-0x42;

    if (result!=(root_dir*WTV_CHUNK_SIZE)-0x42)
    {
        end_of_file=1;
        return 0;
    }

    // Read and calculate the meta data chunks in the file we need to skip over
    // while parsing the file.
    int end=0;
    while(!end)
    {
        buffered_read(parsebuf, 32);
        int x;
    	for(x=0; x<16; x++)
            dbg_print(CCX_DMT_PARSE, "%02X ", parsebuf[x]);
        dbg_print(CCX_DMT_PARSE, "\n");
        
        if (result!=32)
        {
            mprint("\nPremature end of file!\n");
            end_of_file=1;
            free(parsebuf);
            return 0;
        }
        past+=32;
        if( !memcmp(parsebuf, WTV_EOF, 16 ))
        {
            dbg_print(CCX_DMT_PARSE, "WTV EOF\n");
            end=1;
        }
        else {
            
            uint16_t len;
            uint64_t file_length;
            memcpy(&len, parsebuf+16, 2);
            dbg_print(CCX_DMT_PARSE, "len: %x\n", len);
            memcpy(&file_length, parsebuf+24, 8);
            dbg_print(CCX_DMT_PARSE, "file_length: %x\n", file_length);
            if(len>1024)
            {
                mprint("Too large for buffer!\n");
                end_of_file=1;
                free(parsebuf);
                return 0;
            }
            buffered_read(parsebuf, len-32);
            if (result!=len-32)
            {
                mprint("Premature end of file!\n");
                end_of_file=1;
                free(parsebuf);
                return 0;
            }
            past+=len-32;
            // Read a unicode string
    		uint32_t text_len;
            memcpy(&text_len, parsebuf, 4); //text_len is number of unicode chars, not bytes.
            dbg_print(CCX_DMT_PARSE, "text_len: %x\n", text_len);
    		char *string;
    		string = (char*)malloc(text_len+1); // alloc for ascii
            string[text_len]='\0';
    		for(x=0; x<text_len; x++) {
                memcpy(&string[x], parsebuf+8+(x*2), 1); // unicode converted to ascii
    		}
            dbg_print(CCX_DMT_PARSE, "string: %s\n", string);
            // Ignore everything that doesn't contain the text ".entries."
	        if(strstr(string, WTV_TABLE_ENTRIES)!=NULL) { 
		        uint32_t value;
		        uint32_t flag;
                memcpy(&value, parsebuf+(text_len*2)+8, 4);
                memcpy(&flag, parsebuf+(text_len*2)+4+8, 4);
                dbg_print(CCX_DMT_PARSE, "value: %x\n", value);
                dbg_print(CCX_DMT_PARSE, "flag: %x\n", flag);
                if(!add_skip_chunks(cb, value, flag)) {  
                    mprint("Premature end of file!\n");
                    end_of_file=1;
                    free(parsebuf);
                    return 0;
                }
	        }
            free(string);
        }
    }
    // Our list of skip_chunks is now complete
    // Sort it (not sure if this is needed, but it doesn't hurt).
    qsort(cb->skip_chunks, cb->count, sizeof(uint64_t), qsort_cmpint);
    dbg_print(CCX_DMT_PARSE, "skip_chunks: ");
    int x;
    for(x=0; x<cb->count; x++)
        dbg_print(CCX_DMT_PARSE, "%llx, ", cb->skip_chunks[x]);
    dbg_print(CCX_DMT_PARSE, "\n");

    // Seek forward to the start of the data dir
    // Typically 0x40000
	buffered_skip((int)((cb->skip_chunks[cb->chunk]+WTV_META_CHUNK_SIZE)-past));
    cb->filepos=(cb->skip_chunks[cb->chunk]+WTV_META_CHUNK_SIZE);
	cb->chunk++;
    past=cb->filepos;
    free(parsebuf);
    return 1;
}

LLONG get_data(struct wtv_chunked_buffer *cb) {
    static int video_streams[32];
    static int num_streams=0;
    while(1)
    {
        int bytesread = 0;
        // Read the 32 bytes containing the GUID and length and stream_id info
        get_sized_buffer(cb, 32);

        if(cb->buffer==NULL) {end_of_file=1; return 0; } //Make this a macro?

        uint8_t guid[16];
        memcpy(&guid, cb->buffer, 16); // Read the GUID
    	int x;
    	for(x=0; x<16; x++)
            dbg_print(CCX_DMT_PARSE, "%02X ", guid[x]);
        dbg_print(CCX_DMT_PARSE, "\n");
        uint32_t len;
        memcpy(&len, cb->buffer+16, 4); // Read the length
        len-=32;
        dbg_print(CCX_DMT_PARSE, "len %X\n", len);
		uint32_t pad;
		pad = len%8==0 ? 0 : 8- (len % 8);  // Calculate the padding to add to the length
                                            // to get to the next GUID
        dbg_print(CCX_DMT_PARSE, "pad %X\n", pad);
        uint32_t stream_id;
        memcpy(&stream_id, cb->buffer+20, 4);
		stream_id = stream_id & 0x7f;       // Read and calculate the stream_id
		dbg_print(CCX_DMT_PARSE, "stream_id: %X\n", stream_id);

        for(x=0; x<num_streams; x++)
            dbg_print(CCX_DMT_PARSE, "video stream_id: %X\n", video_streams[x]);
        if( !memcmp(guid, WTV_EOF, 16 ))
        {
            // This is the end of the data in this file
            // read the padding at the end of the file
            // and return one more byte
            dbg_print(CCX_DMT_PARSE, "WTV EOF\n");
            uint8_t *parsebuf;
            parsebuf = (uint8_t*)malloc(1024);
            do {
                buffered_read(parsebuf, 1024);
                past+=1024;
            } while (result==1024);
            past+=result;
            free(parsebuf);
            free(cb->buffer);
            cb->buffer=NULL;
            //return one more byte so the final percentage is shown correctly
            *(buffer+inbuf)=0x00;
            end_of_file=1;
            inbuf++;
            return 1;
        }
        if( !memcmp(guid, WTV_STREAM2, 16 ) )
        {
            // The WTV_STREAM2 GUID appares near the start of the data dir
            // It maps stream_ids to the type of stream
			dbg_print(CCX_DMT_PARSE, "WTV STREAM2\n");
            get_sized_buffer(cb, 0xc+16);
            if(cb->buffer==NULL) {end_of_file=1; return 0; }
            static unsigned char stream_type[16];
            memcpy(&stream_type, cb->buffer+0xc, 16); //Read the stream type GUID
            const void *stream_guid;
            if(ccx_options.wtvmpeg2)
                stream_guid = WTV_STREAM_VIDEO; // We want mpeg2 data if the user set -wtvmpeg2
            else
                stream_guid = WTV_STREAM_MSTVCAPTION; // Otherwise we want the MSTV captions stream
            if(!memcmp(stream_type, stream_guid, 16 ) )
            {
                video_streams[num_streams]=stream_id; // We keep a list of stream ids
                num_streams++;                        // Even though there should only be 1
            }
            len-=28;
        }
        if( !memcmp(guid, WTV_TIMING, 16 )  && check_stream_id(stream_id, video_streams, num_streams))
        {
            // The WTV_TIMING GUID contains a timestamp for the given stream_id
            dbg_print(CCX_DMT_PARSE, "WTV TIMING\n");
            get_sized_buffer(cb, 0x8+0x8);
            if(cb->buffer==NULL) {end_of_file=1; return 0; }
            int64_t time;
            memcpy(&time, cb->buffer+0x8, 8); // Read the timestamp
            dbg_print(CCX_DMT_PARSE, "TIME: %ld\n", time);
            if(time!=-1) { // Ignore -1 timestamps
                current_pts = time_to_pes_time(time); // Convert from WTV to PES time 
                pts_set = 1;
                frames_since_ref_time = 0;
                set_fts();  
            }
            len-=16;
        }
        if( !memcmp(guid, WTV_DATA, 16 )  
            && check_stream_id(stream_id, video_streams, num_streams) && current_pts!=0
            && (ccx_options.wtvmpeg2 || (!ccx_options.wtvmpeg2 && len==2)))
        {
            // This is the data for a stream we want to process
            dbg_print(CCX_DMT_PARSE, "\nWTV DATA\n");
            get_sized_buffer(cb, len);
            if(cb->buffer==NULL) {end_of_file=1; return 0; }
            memcpy(buffer+inbuf, cb->buffer, len);
            inbuf+=result;
            bytesread+=(int) len;
            frames_since_ref_time++;
            set_fts(); 
            if(pad>0) { //Make sure we skip any padding too, since we are returning here
                skip_sized_buffer(cb, pad);
            }
            return bytesread;
        }
        if(len+pad>0) {
            // skip any remaining data
            // For any unhandled GUIDs this will be len+pad
            // For others it will just be pad
            skip_sized_buffer(cb, len+pad);
        }
    }
}

LLONG wtv_getmoredata(void)
{
    static long parsebufsize = 1024;
	static struct wtv_chunked_buffer cb;
    if(firstcall)
    {
        init_chunked_buffer(&cb);
        if(ccx_options.wtvmpeg2)
            ccx_bufferdatatype=CCX_PES;
        else
            ccx_bufferdatatype=CCX_RAW;
        if(read_header(&cb)==0)
            // read_header returned an error
            // read_header will have printed the error message
            return 0;
        firstcall=0;
    }
    return get_data(&cb);
}
