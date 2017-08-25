import sys
import os
import subprocess

output_formats = ['.srt','.ass','.ssa','.webvtt','.sami','.txt','.original','.python','.py']
args_list = sys.argv[1:]
args_count = len(args_list)
if args_count>1:
    print "wrong usage"
    exit(0)
directory = args_list[0]
if not os.path.isdir(directory):
    print "error: path given is not a directory"
    exit(0)
files = []
for item in os.listdir(directory):
    ext = os.path.splitext(item)[1]
    if ext not in output_formats:
        files.append(os.path.join(directory,item))
for sample in files:
    print "Processing file: "+sample
    #command=['../linux/ccextractor',sample]
    command = ['python','api_testing.py',sample]
    subprocess.call(command)
    print "Finished processing file: "+sample
