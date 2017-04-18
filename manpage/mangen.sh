#!/bin/bash


#Checks for help2man
if [[ $(help2man 2>&1) == *"not found"* ]]
then
    echo "ERROR: 'help2man' package not found. Please install it and try again."
    exit 0
fi

#Manual Pager Generation
unamestr=`uname -s`
if [[ "$unamestr" == 'Linux' ]]; then
    cd ../linux
elif [[ "$unamestr" == 'Darwin' ]]; then
    cd ../mac
fi
ccx=`./ccextractor 2>&1`
if [[ $ccx != *"no input files"* ]]
then
    echo "CCExtractor binary not found, please wait while it's being built."
    ./build 2> /dev/null 1>/dev/null
fi
help2man --section=1 --output=ccextractor.1 ./ccextractor
mv ccextractor.1 ../manpage/
cd ../manpage
sed -i -e 's/Syntax:/.SH SYNOPSIS/g' ccextractor.1
sed -i '/.SS "File name related options:"/i \
.SH OPTIONS' ccextractor.1
echo "Manpage generation successful!"