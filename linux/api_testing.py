import ccextractorapi as cc
import os
import time
import sys
sample = " ".join(sys.argv[1:])
print 'Name of sample is {0}'.format(sample)
time.sleep(1)
s =  cc.api_init_options()
cc.checking_configuration_file(s)
params = [str(sample)]
for i in params:
    cc.api_add_param(s,str(i))
compile_ret = cc.compile_params(s,len(params));
start_ret = cc.api_start(s);
os.system('clear')
