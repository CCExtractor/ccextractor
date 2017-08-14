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
def generate_output_srt( fh, data):
    for item in data['text']:
        if "                                " not in item:
            o = item
            index = item.find("\x11")
            if index:
                o = item[:index]
            fh.write(o)
            fh.write("\n")
