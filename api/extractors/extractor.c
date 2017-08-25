#include "extractor.h"
 
void python_extract_g608_grid(unsigned h1, unsigned m1, unsigned s1, unsigned ms1, unsigned h2, unsigned m2, unsigned s2, unsigned ms2, char* buffer, int identifier, int srt_counter, int encoding){
    /*
     * identifier = 0 ---> adding start and end time
     * identifier = 1 ---> subtitle
     * identifier = 2 ---> color
     * identifier = 3 ---> font
     */
    int i;
    int asprintf_ret;
    char* output;
    char* start_time = time_wrapper("%02u:%02u:%02u,%03u",h1,m1,s1,ms1);
    char* end_time = time_wrapper("%02u:%02u:%02u,%03u",h2,m2,s2,ms2);
    //check if the caption with same start and end time already exists
    for(i=0;i<array.sub_count;i++){
        if ((strcmp(start_time,array.subs[i].start_time)==0)&&(strcmp(end_time,array.subs[i].end_time)==0)){
            switch(identifier){
                case 1:
#if defined(PYTHONAPI) 
                    asprintf_ret = asprintf(&output,"text[%d]:%s\n",srt_counter,buffer);
                    if (asprintf_ret==-1)
                    {
                        printf("Error: Some problem with asprintf return value in extractor.c\nExiting.");
                        exit(500);
                    }
                    run(array.reporter,output, encoding);
                    free(output);
#endif
                    return;
                 case 2:
#if defined(PYTHONAPI) 
                    asprintf_ret = asprintf(&output,"color[%d]:%s\n",srt_counter,buffer);
                    if (asprintf_ret==-1)
                    {
                        printf("Error: Some problem with asprintf return value in extractor.c\nExiting.");
                        exit(500);
                    }
                    run(array.reporter,output, encoding);
                    free(output);
#endif
                    return;
                 case 3:
#if defined(PYTHONAPI) 
                    asprintf_ret = asprintf(&output,"font[%d]:%s\n",srt_counter,buffer);
                    if (asprintf_ret==-1)
                    {
                        printf("Error: Some problem with asprintf return value in extractor.c\nExiting.");
                        exit(500);
                    }
                    run(array.reporter,output, encoding);
                    free(output);
#endif
                    return;
                 case 4:
                 // writing end of frame 
#if defined(PYTHONAPI) 
                    asprintf_ret = asprintf(&output,"***END OF FRAME***\n",srt_counter,buffer);
                    if (asprintf_ret==-1)
                    {
                        printf("Error: Some problem with asprintf return value in extractor.c\nExiting.");
                        exit(500);
                    }
                    run(array.reporter,output, encoding);
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
    asprintf_ret = asprintf(&output,"srt_counter-%d\n",srt_counter);
    if (asprintf_ret==-1)
    {
        printf("Error: Some problem with asprintf return value in extractor.c\nExiting.");
        exit(500);
    }
    run(array.reporter,output, encoding);
    free(output);
    asprintf_ret = asprintf(&temp,"start_time-%s\t end_time-%s\n",start_time,end_time);
    if (asprintf_ret==-1)
    {
        printf("Error: Some problem with asprintf return value in extractor.c\nExiting.");
        exit(500);
    }
    run(array.reporter,temp, encoding);  
    free(temp);
#endif
}
