#include "wrapper.h"
#include "ccextractor.h"

void set_pythonapi(struct ccx_s_options *api_options){ 
    api_add_param(api_options,"-pythonapi");
}
#if defined(PYTHONAPI)
void set_pythonapi_via_python(struct ccx_s_options *api_options, PyObject * func){
    //setting the user specified callback function as a array element which would be global in C
    array.reporter = func;
    api_add_param(api_options,"-pythonapi");
}
#endif
/*
autoprogram wrapper
*/
void setautoprogram(struct ccx_s_options *api_options){
    api_add_param(api_options,"-autoprogram");
}

/*
stdout wrapper
*/
void setstdout(struct ccx_s_options *api_options){
    api_add_param(api_options,"-stdout");
}

/*
pesheader wrapper
*/
void setpesheader(struct ccx_s_options *api_options){
    api_add_param(api_options,"-pesheader");
}

/*
debugdvbsub wrapper
*/
void setdebugdvbsub(struct ccx_s_options *api_options){
    api_add_param(api_options,"-debugdvbsub");
}

