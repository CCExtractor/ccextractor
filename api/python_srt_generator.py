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
def comparing_text_font_grids(text, font, color):
    original_text = text
    temp_color = []
    for letter,color_line in zip(original_text,color):
        color = 0
        color_flag = 1
        prev = color_line[0]
        buff = color_text_start[str(prev)]
        if "                                " not in letter:
            for i,color_type in enumerate(color_line): 
                if prev!=color_type and not color_flag:
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
            temp_color.append(buff)
        else:
            temp_color.append(letter)
    temp_font_italics=[]
    for letter,font_line in zip(original_text,font):
        if "                                " not in letter:
            buff=""
            underline,italics = 0,0
            #Handling italics
            italics_flag = 0
            for i,font_type in enumerate(font_line): 
                if font_type == 'I' and not italics_flag:
                        buff = buff + '<i>'
                        italics_flag = 1
                elif font_type =="R" and italics_flag: 
                        buff = buff + '</i>'
                        italics_flag = 0
                buff +=  letter[i]
            if italics_flag:
                buff+='</i>'
            temp_font_italics.append(buff)
        else:
            temp_font_italics.append(letter)
    final = []
    for i,j in zip(temp_color,temp_font_italics):
        if i!=j:
            if i in original_text:
                final.append(j)
            else:
                final.append(i)
        else:
            final.append(i)
    return (final,font,color)

    
def generate_output_srt(filename,d):
    try:
        d['text'] = [unicode(item,'utf-8') for item in d['text']]
    except:
        d['text'] = [unicode(item,'utf-8','ignore')[:32] for item in d['text']]
    d['text'],d['font'],d['color']= comparing_text_font_grids(d['text'],d['font'],d['color'])
    for item in d['text']:
        if "                                " not in item:
            o=item
            with open(filename,'ab+') as fh:
                fh.write(o.encode('utf8'))
                fh.write("\n")
                fh.flush()
    with open(filename,'ab+') as fh:
        fh.write("\n")
        fh.flush()
