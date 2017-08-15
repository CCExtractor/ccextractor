#include <stdio.h>
#include "ccextractor.h"

#if defined(PYTHONAPI)
void set_pythonapi(struct ccx_s_options *api_options, PyObject * func);
#else
void set_pythonapi(struct ccx_s_options *api_options);
#endif
void setautoprogram(struct ccx_s_options *api_options);
void setstdout(struct ccx_s_options *api_options);
void setpesheader(struct ccx_s_options *api_options);
void setdebugdvbsub(struct ccx_s_options *api_options);

