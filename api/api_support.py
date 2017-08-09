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
text,font,color = [],[],[]
def g608_grid_former(line):
    global text,font,color
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

def user_choice(line):
    global text,font,color
    if "start_time" in line:
        if font:
            print "\n".join(font)
        text,font,color = [],[],[]
    g608_grid_former(line)

    #d = {}
    #    print datetime.datetime.now()

    #else:
    #    line = alternator(line)
    #print line


