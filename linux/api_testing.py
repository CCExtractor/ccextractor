import ccextractorapi as cc
import os

s =  cc.api_init_options()
cc.checking_configuration_file(s)

params = ['/vagrant/DISNEY.ts']
for item in params:
    cc.api_add_param(s,str(item))
compile_ret = cc.compile_params(s,len(params));
start_ret = cc.api_start(s);

