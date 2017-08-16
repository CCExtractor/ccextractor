#include "wrapper.h"
#include "ccextractor.h"

/*
output=pythonapi wrapper
*/

//void set_pythonapi_via_python(struct ccx_s_options *api_options, PyObject * func){
//    printf("Inside set pythonapi\n");
//    array.reporter = func;
//    api_add_param(api_options,"-pythonapi");
//}

void set_pythonapi(struct ccx_s_options *api_options){ 
    api_add_param(api_options,"-pythonapi");
}
#if defined(PYTHONAPI)
void set_pythonapi_via_python(struct ccx_s_options *api_options, PyObject * func){
//void set_pythonapi_via_python(struct ccx_s_options *api_options){
    printf("inside the set_pythonapi_via_python\n"); 
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

