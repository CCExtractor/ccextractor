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
void my_pythonapi(struct ccx_s_options *api_options, PyObject* func);
%pythoncode %{
def g608_grid_former(line,text,color,font):
    if "text[" in line:
        line = str(line.split(":", 1)[1])
        line = str(line.split("\n")[0])
        text.append(line)
    if "color[" in line:
        line = str(line.split(":", 1)[1])
        line = str(line.split("\n")[0])
        color.append(line)
    if "font[" in line:
        line = str(line.split(":", 1)[1])
        line = str(line.split("\n")[0])
        font.append(line)

def print_g608_grid(case,text,color,font):
    help_string = """
    Case is the value that would give the desired output.
    case = 0 --> print start_time,end_time,text,color,font
    case = 1 --> print start_time,end_time,text
    case = 2 --> print start_time,end_time,color
    case = 3 --> print start_time,end_time,font
    case = 4 --> print start_time,end_time,text,color
    case = 5 --> print start_time,end_time,text,font
    case = 6 --> print start_time,end_time,color,font
    """
    if case==0:
        if text:
            print "\n".join(text)
        if color:
            print "\n".join(color)
        if font:
            print "\n".join(font)
        
    elif case==1:
        if text:
            print "\n".join(text)
    elif case==2:
        if color:
            print "\n".join(color)
    elif case==3:
        if font:
            print "\n".join(font)
    elif case==4:
        if text:
            print "\n".join(text)
        if color:
            print "\n".join(color)
    elif case==5:
        if text:
            print "\n".join(text)
        if font:
            print "\n".join(font)
    elif case==6:
        if color:
            print "\n".join(color)
        if font:
            print "\n".join(font)
    else:
        print help_string

%}
   %include "../src/lib_ccx/ccx_common_common.h"
   %include "../src/ccextractor.h"    
   %include "../api//wrappers/wrapper.h"    
