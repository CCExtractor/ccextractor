#!/bin/bash

cd ..
./autogen.sh
./configure
make dist
mv *.gz package_creators/
cd package_creators