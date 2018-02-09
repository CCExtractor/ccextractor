from __future__ import print_function
from builtins import str
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

def return_g608_grid(case,text,color,font):
    ret_val = {'text':" ",'color':" ",'font':" "}
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
            ret_val['text']=text
        if color:
            ret_val['color']=color
        if font:
            ret_val['font']=font
        
    elif case==1:
        if text:
            ret_val['text']=text
    elif case==2:
        if color:
            ret_val['color']=color
    elif case==3:
        if font:
            ret_val['font']=font
    elif case==4:
        if text:
            ret_val['text']=text
        if color:
            ret_val['color']=color
    elif case==5:
        if text:
            ret_val['text']=text
        if font:
            ret_val['font']=font
    elif case==6:
        if color:
            ret_val['color']=color
        if font:
            ret_val['font']=font
    else:
        print(help_string)
    return ret_val
