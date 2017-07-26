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
def alternator(line):
    final=""
    for letter,index in enumerate(line):
            if index%2==0:
                final+=letter.upper()
            else:
                final+=letter.lower()
    return final   
def user_choice(line):
    if "start_time" in line:
        print datetime.datetime.now()
    #else:
    #    line = alternator(line)
    print line


