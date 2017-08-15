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
def generate_output_srt_time( fh, data):
    data = data.split("-")
    end_time = str(data[-1].split("\n")[0])
    start_time = str(data[1].split("\t")[0])
    #return (start_time,end_time)
    fh.write(start_time)
    fh.write(" --> ")
    fh.write(end_time)
    fh.write("\n")

    
    
def generate_output_srt( fh, d):
    for item in d:
        if "                                " not in item:
            o = item
            index = item.find("\x11")
            if index:
                o = item[:index]
            fh.write(o)
            fh.write("\n")
    fh.write("\n")
