%module ccextractor
%{
    #define SWIG_FILE_WITH_INIT
    #include "../src/lib_ccx/lib_ccx.h"
    #include "../src/lib_ccx/configuration.h"
    #include "../src/lib_ccx/ccx_common_option.h"
    #include "../src/lib_ccx/ccx_mp4.h"
    #include "../src/lib_ccx/hardsubx.h"
    #include "../src/lib_ccx/ccx_share.h"
    #include "../src/ccextractor.h"
    #include "../src/wrappers/wrapper.h"
%}
struct ccx_s_options* api_init_options();
void check_configuration_file(struct ccx_s_options api_options);
int compile_params(struct ccx_s_options *api_options,int argc);
void api_add_param(struct ccx_s_options* api_options,char* arg);
int api_start(struct ccx_s_options api_options);
void my_pythonapi(struct ccx_s_options *api_options, PyObject *func);
