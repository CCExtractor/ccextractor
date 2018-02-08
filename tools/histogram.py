#!/usr/bin/python
# Author   : Harry Yu
# Email    : harryyunull@gmail.com
# Link     : https://github.com/harrynull

from __future__ import print_function
from builtins import range
from builtins import object
Version = "1.0.0"

import sys
import time
import datetime
from io import open

class Line(object):
    def __init__(self, lines):
        lines = [line.replace("\n", "").strip() for line in lines]
        self.id = int(lines[0]) # the first line is the number of the subtile
        self.start_time, self.end_time = lines[1].split(" --> ")
        self.content = lines[2:]
        
def time_to_minute(time):
    time_tuple = datetime.datetime.strptime(time[:-4], '%H:%M:%S').timetuple()
    return time_tuple.tm_hour*60+time_tuple.tm_min

def read_srt_file(filename):
    lines = []
    with open(filename, encoding = "utf-8") as file:
        raw_lines = []
        for line in file:
            if line == "\n" or line == "\r\n":
                lines.append(Line(raw_lines))
                raw_lines = []
            else:
                raw_lines.append(line)
    return lines

def main():
    if len(sys.argv)<2:
        print("{0}: missing file name.\nUse {0} /path/to/srt/file".format(sys.argv[0]))
        exit()
    elif len(sys.argv)>2:
        print("{0}: too many arguments:\nUse {0} /path/to/srt/file".format(sys.argv[0]))
        exit()
        
    print("histogram.py %s Made by Harry Yu" % Version)
    print("Histogram for %s\n" % sys.argv[1])
    
    # Read subtitles from file
    lines = read_srt_file(sys.argv[1])

    # Count how many subtitles per minute.
    subtitle_per_minute = {}
    for line in lines:
        start_time = time_to_minute(line.start_time)
        end_time = time_to_minute(line.end_time)
        for i in range(start_time, end_time + 1):
            subtitle_per_minute[i]=subtitle_per_minute.get(i, 0) + 1
            
    # Print the result
    for i in range(0, max(subtitle_per_minute.keys())):
        print("Minute %d\t%d\t%s"%(i,subtitle_per_minute.get(i, 0),"+"*subtitle_per_minute.get(i, 0)))
    
    print("\nTotal subtitles: %d"%sum(subtitle_per_minute.values()))
    
if __name__ == '__main__':
    main()
