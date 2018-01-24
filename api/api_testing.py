###
#MANDATORY UPDATES IN EVERY PYTHON SCRIPT
###
import sys

import ccextractor as cc


def callback(line, encoding):
    print line


def init_ccextractor(callback):
    """
    :param callback: The callback which we use to handle
                     the extracted subtitle info
    :return return the initialized options
    """
    optionos = cc.api_init_options()
    cc.check_configuration_file(optionos)
    for arg in sys.argv[1:]:
        cc.api_add_param(optionos, arg)
    compile_ret = cc.compile_params(optionos, len(sys.argv[1:]))

    # use my_pythonapi to add callback in C source code
    cc.my_pythonapi(optionos, callback)

    return optionos


def main():
    options = init_ccextractor(callback)
    cc.api_start(options)


if __name__=="__main__":
    main()
