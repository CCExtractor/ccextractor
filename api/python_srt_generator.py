from __future__ import print_function
from builtins import zip
from builtins import str
import ccextractor as cc
import re
"""
            #Handling underline
            buff = ""
            underline_flag = 0
            for i,font_type in enumerate(font_line): 
                if font_type == 'U' and not underline_flag:
                        buff = buff + '<u> '
                        underline_flag = 1
                        underline=1
                elif font_type =="R" and underline_flag: 
                        buff = buff + '</u>'
                        underline_flag = 0
                        continue;
                buff +=  letter[i]
            #adding a new line after buff has seen underline
            #need to cross check with CCExtractor output as to how they are doing
            if underline:
                buff+= "\n"
            else:
                buff=""
"""
encodings_map = {
        '0':'unicode',
        '1':'latin1',
        '2':'utf-8',
        '3':'ascii',
}

color_text_start={
        "0":"",
        "1":"<font color=\"#00ff00\">",
        "2":"<font color=\"#0000ff\">",
        "3":"<font color=\"#00ffff\">",
        "4":"<font color=\"#ff0000\">",
        "5":"<font color=\"#ffff00\">",
        "6":"<font color=\"#ff00ff\">",
        "7":"<font color=\"",
        "8":"",
        "9":""
};
color_text_end={
        "0":"",
        "1":"</font",
        "2":"</font>",
        "3":"</font>",
        "4":"</font>",
        "5":"</font>",
        "6":"</font>",
        "7":"</font>",
        "8":"",
        "9":""
};
no_color_tag = ['0','8','9']
def comparing_text_font_grids(text, font, color):
    original_text = text
    original_color = color
    temp_color = []
    for letter,color_line in zip(original_text,color):
        color = 0
        prev = color_line[0]
        buff = color_text_start[str(prev)]
        if prev not in no_color_tag:
            color_flag = 1
        else:
            color_flag = 0
        if letter.count(" ")<32:
            for i,color_type in enumerate(color_line): 
                if color_type not in no_color_tag and prev!=color_type and not color_flag:
                        color = 1
                        buff = buff + color_text_start[str(color_type)]
                        color_flag = 1
                elif prev!=color_type and color_flag: 
                        color = 1
                        buff = buff + color_text_end[str(prev)]
                        color_flag = 0
                buff += letter[i]
                prev=color_type
            if color_flag:
                color_flag=0
                buff+=color_text_end[str(prev)]
        if color:
            temp_color.append((buff,1))
        else:
            temp_color.append((letter,0))
    temp_font_italics=[]
    for letter,font_line in zip(original_text,font):
        if letter.count(" ")<32:
            buff=""
            underline,italics = 0,0
            #Handling italics
            italics_flag = 0
            for i,font_type in enumerate(font_line): 
                if font_type == 'I' and not italics_flag:
                        italics=1
                        buff = buff + '<i>'
                        italics_flag = 1
                elif font_type =="R" and italics_flag: 
                        italics=1
                        buff = buff + '</i>'
                        italics_flag = 0
                buff +=  letter[i]
            if italics_flag:
                buff+='</i>'
            if italics:
                temp_font_italics.append((buff,1))
            else:
                temp_font_italics.append((letter,0))
        else:
            temp_font_italics.append((letter,0))
    final = []
    for i,j in zip(temp_color,temp_font_italics):
        if i[1] and not j[1]:
            final.append(i[0])
        elif j[1] and not i[1]:
            final.append(j[0])
        else:
            if not i[1]:
                final.append(i[0])
            else:
                print("error")
    return (final,font,color)

    
def generate_output_srt(filename,d, encoding):
    if encoding in list(encodings_map.keys()):
        if  encoding!='0':
            encoding_format = encodings_map[encoding]
        else:
            encoding_format = ""
    else:
        print("encoding error in python")
        return
    if encoding_format:
        d['text'] = [str(item,encoding_format) for item in d['text']]
    else:
        d['text'] = [str(item) for item in d['text']]
    d['text'],d['font'],d['color']= comparing_text_font_grids(d['text'],d['font'],d['color'])
    for item in d['text']:
        if item.count(" ")<32:
            o=item
            with open(filename,'ab+') as fh:
                if encoding_format:
                    fh.write(o.encode(encoding_format))
                else:
                    fh.write(str(o))
                fh.write("\n")
                fh.flush()
    with open(filename,'ab+') as fh:
        fh.write("\n")
        fh.flush()
