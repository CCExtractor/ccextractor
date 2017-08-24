#include "extractor.h"
 
void python_extract_g608_grid(unsigned h1, unsigned m1, unsigned s1, unsigned ms1, unsigned h2, unsigned m2, unsigned s2, unsigned ms2, char* buffer, int identifier, int srt_counter){
    /*
     * identifier = 0 ---> adding start and end time
     * identifier = 1 ---> subtitle
     * identifier = 2 ---> color
     * identifier = 3 ---> font
     */
    int i;
    char* output;
    char* start_time = time_wrapper("%02u:%02u:%02u,%03u",h1,m1,s1,ms1);
    char* end_time = time_wrapper("%02u:%02u:%02u,%03u",h2,m2,s2,ms2);
    //check if the caption with same start and end time already exists
    for(i=0;i<array.sub_count;i++){
        if ((strcmp(start_time,array.subs[i].start_time)==0)&&(strcmp(end_time,array.subs[i].end_time)==0)){
            switch(identifier){
                case 1:
#if defined(PYTHONAPI) 
                    asprintf(&output,"text[%d]:%s\n",srt_counter,buffer);
                    run(array.reporter,output);
                    free(output);
#endif
                    return;
                 case 2:
#if defined(PYTHONAPI) 
                    asprintf(&output,"color[%d]:%s\n",srt_counter,buffer);
                    run(array.reporter,output);
                    free(output);
#endif
                    return;
                 case 3:
#if defined(PYTHONAPI) 
                    asprintf(&output,"font[%d]:%s\n",srt_counter,buffer);
                    run(array.reporter,output);
                    free(output);
#endif
                    return;
                 case 4:
                 // writing end of frame 
#if defined(PYTHONAPI) 
                    asprintf(&output,"***END OF FRAME***\n",srt_counter,buffer);
                    run(array.reporter,output);
                    free(output);
#endif
                    return;
            }// end of switch
        }
   }//end of for loop

    // if the start time and end time do not match then
    // we are just initializing a new element in the array and 
    // initiating its buffer count and grid count
    array.sub_count++;
    array.subs = realloc(array.subs,sizeof(struct python_subs_modified)*array.sub_count);
 
    array.subs[array.sub_count-1].start_time = start_time;
    array.subs[array.sub_count-1].end_time = end_time;
    char* temp;
#if defined(PYTHONAPI) 
    asprintf(&output,"srt_counter-%d\n",srt_counter);
    run(array.reporter,output);
    free(output);
    asprintf(&temp,"start_time-%s\t end_time-%s\n",start_time,end_time);
    run(array.reporter,temp);  
    free(temp);
#endif
}
