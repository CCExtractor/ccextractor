#include "extractor.h"
 
/*WRITE wrapper for extracting \
 * start_time,
 * end_time,
 * captions
 */
void python_extract(int srt_counter, unsigned h1, unsigned m1, unsigned s1, unsigned ms1, unsigned h2, unsigned m2, unsigned s2, unsigned ms2, char* buffer){
    //check if the srt_counter value already exists
    int i;
    for(i=0;i<array.sub_count;i++){
        if (srt_counter==array.subs[i].srt_counter){
                array.subs[i].buffer_count++;
                array.subs[i].buffer = realloc(array.subs[i].buffer,sizeof(char*)*array.subs[i].buffer_count);
                array.subs[i].buffer[array.subs[i].buffer_count-1] =  malloc(sizeof(char)*strlen(buffer));
                strcpy (array.subs[i].buffer[array.subs[i].buffer_count-1], buffer);
                //array.update_status =1;
                return;
            }
        }
    array.sub_count++;
    array.subs = realloc(array.subs,sizeof(struct python_subs_modified)*array.sub_count);
    array.subs[array.sub_count-1].srt_counter= srt_counter;
    
    array.subs[array.sub_count-1].start_time = time_wrapper("%02u:%02u:%02u,%03u",h1,m1,s1,ms1);
    array.subs[array.sub_count-1].end_time = time_wrapper("%02u:%02u:%02u,%03u",h2,m2,s2,ms2);
    
    array.subs[array.sub_count-1].buffer_count=1;
    array.subs[array.sub_count-1].buffer = malloc(sizeof(char*)*array.subs[array.sub_count-1].buffer_count);
    array.subs[array.sub_count-1].buffer[array.subs[array.sub_count-1].buffer_count-1] =  malloc(sizeof(char)*strlen(buffer));
    strcpy(array.subs[array.sub_count-1].buffer[array.subs[array.sub_count-1].buffer_count-1], buffer);
    //array.update_status =1;
}

void python_extract_time_based(unsigned h1, unsigned m1, unsigned s1, unsigned ms1, unsigned h2, unsigned m2, unsigned s2, unsigned ms2, char* buffer){
    //check if the caption with same start and end time already exists
    int i;
    char* start_time = time_wrapper("%02u:%02u:%02u,%03u",h1,m1,s1,ms1);
    char* end_time = time_wrapper("%02u:%02u:%02u,%03u",h2,m2,s2,ms2);
    for(i=0;i<array.sub_count;i++){
        if ((strcmp(start_time,array.subs[i].start_time)==0)&&(strcmp(end_time,array.subs[i].end_time)==0)){
                //array.subs[i].buffer_count++;
                //array.subs[i].buffer = realloc(array.subs[i].buffer,sizeof(char*)*array.subs[i].buffer_count);
                //array.subs[i].buffer[array.subs[i].buffer_count-1] =  malloc(sizeof(char)*strlen(buffer));
                //strcpy (array.subs[i].buffer[array.subs[i].buffer_count-1], buffer);
                //__wrap_write(fp,buffer, strlen(buffer));
                fprintf(array.fp,"%s\n",buffer);
                fflush(array.fp);
           //array.update_status =1;
                return;
            }
        }
    array.sub_count++;
    array.subs = realloc(array.subs,sizeof(struct python_subs_modified)*array.sub_count);
    
    array.subs[array.sub_count-1].start_time = start_time;
    array.subs[array.sub_count-1].end_time = end_time;
    
    //array.subs[array.sub_count-1].buffer_count=1;
    //array.subs[array.sub_count-1].buffer = malloc(sizeof(char*)*array.subs[array.sub_count-1].buffer_count);
    //array.subs[array.sub_count-1].buffer[array.subs[array.sub_count-1].buffer_count-1] =  malloc(sizeof(char)*strlen(buffer));
    //strcpy(array.subs[array.sub_count-1].buffer[array.subs[array.sub_count-1].buffer_count-1], buffer);
    
    fprintf(array.fp,"start_time:%s\t",start_time);
    fprintf(array.fp,"end_time:%s\n",end_time);
    fprintf(array.fp,"%s\n",buffer);
    fflush(array.fp);
    //__wrap_write(fp,buffer, strlen(buffer));

    //array.update_status =1;
}

void python_extract_transcript(char* buffer){
    //check if the caption with same start and end time already exists
    python_extract_time_based(0,0,0,0,0,0,0,0,buffer);
    array.is_transcript=1; 
}


/*
void python_extract_sami(LLONG ms_start,LLONG ms_end,char* buffer){
    //check if the caption with same start and end time already exists
    int i;
    unsigned h1,m1,s1,ms1;
	unsigned h2,m2,s2,ms2;
	millis_to_time (ms_start,&h1,&m1,&s1,&ms1);
	millis_to_time (ms_end-1,&h2,&m2,&s2,&ms2); // -1 To prevent overlapping with next line.
    python_extract_time_based(h1,m1,s1,ms1,h2,m2,s2,ms2,buffer);
}
*/


