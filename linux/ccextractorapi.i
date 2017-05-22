%module ccextractorapi

%{
     #define SWIG_FILE_WITH_INIT
     #include "../src/lib_ccx/lib_ccx.h"
    #include "../src/lib_ccx/configuration.h"
    #include "../src/lib_ccx/ccx_common_option.h"
    #include "../src/lib_ccx/ccx_mp4.h"
    #include "../src/lib_ccx/hardsubx.h"
    #include "../src/lib_ccx/ccx_share.h"
  #include "../src/ccextractorapi.h"    
%}
    %include "../src/lib_ccx/lib_ccx.h"
    %include "../src/lib_ccx/configuration.h"
    %include "../src/lib_ccx/ccx_common_option.h"
    %include "../src/lib_ccx/ccx_mp4.h"
    %include "../src/lib_ccx/hardsubx.h"
    %include "../src/lib_ccx/ccx_share.h"
  %include "../src/ccextractorapi.h"  
