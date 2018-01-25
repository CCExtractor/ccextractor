#include <stdio.h>
#include "ccextractor.h"

#ifdef PYTHON_API
void my_pythonapi(struct ccx_s_options *api_options, PyObject *callback_func);
void setautoprogram(struct ccx_s_options *api_options);
void setstdout(struct ccx_s_options *api_options);
void setpesheader(struct ccx_s_options *api_options);
void setdebugdvbsub(struct ccx_s_options *api_options);
#endif
