#!/bin/sh

./tarball.sh
makepkg -g >> PKGBUILD
makepkg -sc
rm -f ./*.tar.gz
sed -i '$ d' PKGBUILD
read -p "Do you wish to install ccextractor? [Y/n] " yn
case $yn in
    [Nn]* ) exit;;
    * ) sudo pacman -U ./*.pkg.tar.xz; rm -f ./*.pkg.tar.xz;;
esac
