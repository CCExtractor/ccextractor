import ccextractorapi as cc
s =  cc.api_init_options()
cc.checking_configuration_file(s)
params = ['-parsePAT','/vagrant/ClosedCaptionEIA.ts']
for item in params:
    cc.api_add_param(s,str(item))
compile_ret = cc.compile_params(s,len(params));
#for item in xrange(cc.api_param_count(s)):
#    params = cc.api_param(s,item)
#    print item,params
start_ret = cc.api_start(s);

