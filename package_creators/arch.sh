#!/bin/bash

./tarball.sh
retval=$?
if [ $retval -ne 0 ]; then
    echo "Sorry, the package could not be created as the tarball building process failed with return code $retval"
    rm -f ./*.tar.gz
    exit $retval
fi
makepkg -g >> PKGBUILD
makepkg -sc
retval=$?
if [ $retval -ne 0 ]; then
    echo "Sorry, the package could not be created as makepkg failed with return code $retval"
    rm -rf ./*.tar.gz src
    sed -i '$ d' PKGBUILD
    exit $retval
fi
rm -f ./*.tar.gz
sed -i '$ d' PKGBUILD
read -p "Do you wish to install ccextractor? [y/N] " yn
case $yn in
    [Yy]* ) su -c "pacman -U ./*.pkg.tar.xz"; rm -f ./*.pkg.tar.xz;;
    * ) exit;;
esac
