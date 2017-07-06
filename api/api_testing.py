import ccextractor as cc
import os
import time
import sys
s =  cc.api_init_options()
cc.check_configuration_file(s)
for i in sys.argv[1:]:
    cc.api_add_param(s,str(i))
compile_ret = cc.compile_params(s,len(sys.argv[1:]));
start_ret = cc.api_start(s);

#os.system('clear')
