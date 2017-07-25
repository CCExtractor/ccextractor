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
   #include "../api/wrappers/wrapper.h"    
%}
%pythoncode %{
    import threading
    import time
    from threading import Thread
    #captions_timings_list = []
    def thread_updater():
        Thread(target = caption_generator).start()
    def caption_generator():
        global caption_timings_list
        new_count = cvar.array.sub_count
        old_count = cvar.array.old_sub_count
        if old_count!=new_count:
            for i in xrange(old_count,new_count):
                caption = cc_to_python_get_modified_sub(i)
                #caption_list = []
                print "start time = {}\t end time = {}".format(caption.start_time,caption.end_time)
                for j in xrange(cc_to_python_get_modified_sub_buffer_size(i)):
                    print cc_to_python_get_modified_sub_buffer(i,j)
                    #caption_list.append(cc_to_python_get_modified_sub_buffer(i,j))
                #captions_timings_list.append((caption.start_time,caption.end_time,caption_list))
            old_count=new_count
        else:
            if cvar.array.has_api_start_exited:
                return
            time.sleep(0.1)
            caption_generator()
%}
   %include "../src/lib_ccx/ccx_common_common.h"
   %include "../src/ccextractor.h"    
   %include "../api//wrappers/wrapper.h"    
