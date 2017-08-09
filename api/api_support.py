import ccextractor as cc
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
def user_choice(line):
    global text,font,color
    if "start_time" in line:
        cc.print_g608_grid(2,text,color,font)
        text,font,color = [],[],[]
    cc.g608_grid_former(line,text,color,font)
    #d = {}
    #    print datetime.datetime.now()

    #else:
    #    line = alternator(line)
    #print line


