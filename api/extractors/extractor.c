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
            if (identifier==1){
                // adding the subtitle to the buffer of the array 
                array.subs[i].buffer = realloc(array.subs[i].buffer,sizeof(char*)*array.subs[i].buffer_count);
                array.subs[i].buffer[array.subs[i].buffer_count-1] =  malloc(sizeof(char)*strlen(buffer));
                strcpy (array.subs[i].buffer[array.subs[i].buffer_count-1], buffer);
#if defined(PYTHONAPI) 
                asprintf(&output,"text[%d]:%s\n",srt_counter,buffer);
                run(array.reporter,output);
#endif

                //fprintf(array.fp,"text[%d]:%s\n",srt_counter,buffer);
                //fflush(array.fp);
                array.subs[i].buffer_count++;
                return;
                }
            else{
                if(identifier==2){
                // forming the g608_grid_color  
                    array.subs[i].g608_grid_color = realloc(array.subs[i].g608_grid_color,sizeof(char*)*array.subs[i].g608_grid_color_count);
                    array.subs[i].g608_grid_color[array.subs[i].g608_grid_color_count-1] =  malloc(sizeof(char)*strlen(buffer));
                    strcpy (array.subs[i].g608_grid_color[array.subs[i].g608_grid_color_count-1], buffer);
#if defined(PYTHONAPI) 
                    asprintf(&output,"color[%d]:%s\n",srt_counter,buffer);
                    run(array.reporter,output);
#endif

                    //fprintf(array.fp,"color[%d]:%s\n",srt_counter,buffer);
                    //fflush(array.fp);
                    array.subs[i].g608_grid_color_count++;
                    return;
                    }
                else if(identifier==3){
                // forming the g608_grid  
                    array.subs[i].g608_grid_font = realloc(array.subs[i].g608_grid_font,sizeof(char*)*array.subs[i].g608_grid_font_count);
                    array.subs[i].g608_grid_font[array.subs[i].g608_grid_font_count-1] =  malloc(sizeof(char)*strlen(buffer));
                    strcpy (array.subs[i].g608_grid_font[array.subs[i].g608_grid_font_count-1], buffer);
#if defined(PYTHONAPI) 
                    asprintf(&output,"font[%d]:%s\n",srt_counter,buffer);
                    run(array.reporter,output);
#endif
                    //fprintf(array.fp,"font[%d]:%s\n",srt_counter,buffer);
                    //fflush(array.fp);
                    array.subs[i].g608_grid_font_count++;
                    return;
                    }
                else if(identifier==4){
                // writing end of frame 
#if defined(PYTHONAPI) 
                    asprintf(&output,"***END OF FRAME***\n",srt_counter,buffer);
                    run(array.reporter,output);
#endif
                    //fprintf(array.fp,"***END OF FRAME***\n",srt_counter,buffer);
                    //fflush(array.fp);
                    array.subs[i].g608_grid_font_count++;
                    return;
                    }
                }
            }
        }

    // if the start time and end time do not match then
    // we are just initializing a new element in the array and 
    // initiating its buffer count and grid count
    array.sub_count++;
    array.subs = realloc(array.subs,sizeof(struct python_subs_modified)*array.sub_count);
 
    array.subs[array.sub_count-1].start_time = start_time;
    array.subs[array.sub_count-1].end_time = end_time;
#if defined(PYTHONAPI) 
    asprintf(&output,"srt_counter-%d\n",srt_counter);
    run(array.reporter,output);
    asprintf(&output,"start_time-%s\t end_time-%s\n",start_time,end_time);
    run(array.reporter,output);  
#endif
    //fprintf(array.fp,"srt_counter-%s\t",start_time);
    //fprintf(array.fp,"start_time-%s\t",start_time);
    //fprintf(array.fp,"end_time-%s\n",end_time);
    array.subs[array.sub_count-1].buffer_count=1;
    array.subs[array.sub_count-1].buffer=NULL;
    
    array.subs[array.sub_count-1].g608_grid_color_count=1;
    array.subs[array.sub_count-1].g608_grid_font_count=1;
    array.subs[array.sub_count-1].g608_grid_color=NULL;
    array.subs[array.sub_count-1].g608_grid_font=NULL;
}
