#!/bin/bash

if [[ $(rpmbuild 2>&1) == *"not found"* ]]
then
    echo "ERROR: 'rpmbuild' package not found. Please install it and try again."
    exit 0
fi

workdir=`pwd`
echo "%_topdir $workdir/RPMBUILD" >> $HOME/.rpmmacros
./tarball.sh
retval=$?
if [ $retval -ne 0 ]; then
    echo "Sorry, the package could not be created as the tarball building process failed with return code $retval"
    rm -f ./*.tar.gz
    exit $retval
fi
mkdir -p RPMBUILD/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}
mv *.tar.gz RPMBUILD/SOURCES/
cp ccextractor.spec RPMBUILD/SPECS
cd RPMBUILD/SPECS
rpmbuild -ba ccextractor.spec
retval=$?
if [ $retval -ne 0 ]; then
    echo "Sorry, the package could not be created as rpmbuild failed with return code $retval"
    exit $retval
fi
cd ../..
cp RPMBUILD/RPMS/x86_64/*.rpm .
cp RPMBUILD/SRPMS/*.rpm .
rm -rf RPMBUILD
