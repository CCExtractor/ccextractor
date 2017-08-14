import ccextractor as cc
import ccx_to_python_g608 as g608
import python_srt_generator as srt_generator
###
#DO NOT TOUCH THIS FUNCTION
###
def follow(thefile):
    thefile.seek(0,2)
    while 1:
        line = thefile.readline()
        if not line:
            #time.sleep(0.1)
            continue
        yield line

###
#DO NOT TOUCH THIS FUNCTION
###
def tail(queue):
    fn = queue.get()
    with open(fn,"w") as f:
        pass
    logfile = open(fn,"r")
    loglines = follow(logfile)
    for line in loglines:
        user_choice(line)

###
#modify this function according to user necessity
###
import datetime
##
#help_string to be included in the documentation
#it is the help string for telling what kind of output the user wants to STDOUT 
#the redirection of output to STDOUT can be changed easily to whatsoever usage the user wants to put in.
##
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
text,font,color = [],[],[]
filename = " "

def user_choice(line):
    global text,font,color
    global filename
    if "filename:" in line:
        filename = str(str(line.split(":")[1]).split("\n")[0])
        fh = srt_generator.generate_file_handle(filename,'w')
        srt_generator.delete_file_handle(fh)
    if "start_time" in line:
        if filename!=" ":
            data = g608.return_g608_grid(1,text,color,font)
            fh = srt_generator.generate_file_handle(filename,'a')
            srt_generator.generate_output_srt( fh, data)
            srt_generator.delete_file_handle(fh)
            text,font,color = [],[],[]
    g608.g608_grid_former(line,text,color,font)

    #d = {}
    #    print datetime.datetime.now()

    #else:
    #    line = alternator(line)
    #print line



