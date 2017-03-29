#!/bin/sh

./tarball.sh
makepkg -g >> PKGBUILD
makepkg -sic
rm ./*.tar.gz
rm ./*.pkg.tar.xz
sed -i '$ d' PKGBUILD
