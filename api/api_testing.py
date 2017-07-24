import ccextractor as cc
import os
import time
import sys
s =  cc.api_init_options()
cc.check_configuration_file(s)
for i in sys.argv[1:]:
    cc.api_add_param(s,str(i))
#cc.setstdout(s)
compile_ret = cc.compile_params(s,len(sys.argv[1:]));
start_ret = cc.api_start(s);

print "\n"
print "The extracted captions with respective timings are as follows:"
###one line functions to directly check the extracted captions that would be otherwise accessible in python.
###just uncomment the next line.
cc.show_extracted_captions_with_timings()

###THE FOLLOWING LOOP LETS USER USE THE EXTRACTED CAPTIONS ALONG WITH THEIR TIMINGS.
#for i in xrange(cc.cc_to_python_get_number_of_subs()):
#    caption = cc.cc_to_python_get_modified_sub(i)
#    print caption.srt_counter
#    print "start time = {}\t end time = {}".format(caption.start_time,caption.end_time)
#    for j in xrange(cc.cc_to_python_get_modified_sub_buffer_size(i)):
#        print cc.cc_to_python_get_modified_sub_buffer(i,j)

"""
#print cc.cc_to_python_get_subs_number_of_lines()
#printing python_subs.subs
#for item in xrange(cc.cc_to_python_get_subs_number_of_lines()):
#    print cc.cc_to_python_get_sub(item)

#os.system('clear')
"""
