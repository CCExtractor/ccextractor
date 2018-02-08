import ccx_to_python_g608 as g608
import python_srt_generator as srt_generator
text,font,color = [],[],[]
filename = " "
srt_counter = " "
def generate_output_srt(line, encoding):
    global text,font,color
    global filename, srt_counter
    if "filename:" in line:
        filename = str(str(line.split(":")[1]).split("\n")[0])
        with open(filename, 'w+') as fh:
            pass
    elif "srt_counter-" in line:
        srt_counter = str(line.split("-")[1])
        with open(filename, 'a+') as fh:
            fh.write(srt_counter)
    elif "start_time" in line:
        with open(filename, 'a+') as fh:
            data = line.split("-")
            end_time = str(data[-1].split("\n")[0])
            start_time = str(data[1].split("\t")[0])
            fh.write(start_time)
            fh.write(" --> ")
            fh.write(end_time)
            fh.write("\n")
            fh.flush()
    elif "***END OF FRAME***" in line:
        d={}
        d['text']=text
        d['color']=color
        d['font']=font
        srt_generator.generate_output_srt(filename,d, encoding)
        text,font,color = [],[],[]
    else:
        g608.g608_grid_former(line,text,color,font)



