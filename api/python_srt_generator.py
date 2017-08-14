import ccextractor as cc
import re

def generate_file_handle(filename, mode):
    fh = open(filename,mode)
#    print "Creating output file: ", fh.name
    return fh

def delete_file_handle(fh):
    fh.close()
##
# data would be a dictionary containg the following keys:
# text, color, font
##
def generate_output_srt_time(data):
    data = data.split("-")
    end_time = str(data[-1].split("\n")[0])
    start_time = str(data[1].split("\t")[0])
    return (start_time,end_time)
    #fh.write(start_time)
    #fh.write("\t")
    #fh.write(end_time)
    #fh.write("\n")

    
    
def generate_output_srt( fh, d):
    start_time = d.keys()[0][0]
    end_time = d.keys()[0][1]
    data = d.items()[0][1]
    if len(data['text'])<1:
        return    
    fh.write(start_time)
    fh.write(" ")
    fh.write("-->")
    fh.write(" ")
    fh.write(end_time)
    fh.write("\n")
    
    for item in data['text']:
        if "                                " not in item:
            o = item
            index = item.find("\x11")
            if index:
                o = item[:index]
            fh.write(o)
            fh.write("\n")
