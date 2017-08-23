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
def comparing_text_font_grids(text, font):
    temp = []

    for letter,font_line in zip(text,font):
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
            temp.append(buff)
    return (temp,font)

    
def generate_output_srt(filename,d):
    try:
        d['text'] = [unicode(item,'utf-8') for item in d['text']]
    except:
        d['text'] = [unicode(item,'utf-8','ignore')[:32] for item in d['text']]
        print d['text']
    d['text'],d['font']= comparing_text_font_grids(d['text'],d['font'])
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
