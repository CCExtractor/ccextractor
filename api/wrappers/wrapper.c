#include "wrapper.h"

/*
autoprogram wrapper
*/
void setautoprogram(struct ccx_s_options *api_options){
    api_add_param(api_options,"-autoprogram");
    mprint("Done setting autoprogram\n");
}

/*
stdout wrapper
*/
void setstdout(struct ccx_s_options *api_options){
    api_add_param(api_options,"-stdout");
    mprint("Done setting stdout\n");
}
