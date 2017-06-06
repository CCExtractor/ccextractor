#!/bin/bash



cd ..
cp linux/autogen.sh .
cp linux/pre-build.sh .
cp linux/configure.ac .
cp linux/Makefile.am .
sed -i -e 's:../src:src:g' Makefile.am
sed -i -e 's:../src:src:g' configure.ac
sed -i -e 's:../.git:.git:g' pre-build.sh
sed -i -e 's:../src:src:g' pre-build.sh
sed -i -e 's:../manpage:manpage:g' Makefile.am
./autogen.sh
./configure
make dist
mv *.gz package_creators
rm -rf Makefile
rm -rf Makefile.am
rm -rf configure.ac
rm -rf autogen.sh
rm -rf pre-build.sh
rm -rf configure
rm -rf Makefile.in
rm -rf config.status
rm -rf config.log
rm -rf aclocal.m4
rm -rf build-conf
rm -rf autom4te.cache
cd package_creators
