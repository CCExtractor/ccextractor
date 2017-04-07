#!/bin/bash
cd ../linux
./autogen.sh
./configure
make
help2man --section=1 --output=ccextractor.man ./ccextractor
mv ccextractor.man ../manpage/
./clean
cd ../manpage
sed -i -e 's/Syntax:/.SH SYNOPSIS/g' ccextractor.man
sed -i '/.SS "File name related options:"/i \
.SH OPTIONS' ccextractor.man
