#include <stdio.h>
#include "ccextractor.h"

#if defined(PYTHONAPI)
void set_pythonapi_via_python(struct ccx_s_options *api_options, PyObject * func);
#define my_pythonapi(args, func) set_pythonapi_via_python(args, func)
#else
#define my_pythonapi(args, func) set_pythonapi(args)
#endif
void set_pythonapi(struct ccx_s_options *api_options);
void setautoprogram(struct ccx_s_options *api_options);
void setstdout(struct ccx_s_options *api_options);
void setpesheader(struct ccx_s_options *api_options);
void setdebugdvbsub(struct ccx_s_options *api_options);

