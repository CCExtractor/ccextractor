#!/bin/bash
TYPE="debian"   # can be one of 'slackware', 'debian', 'rpm'
PROGRAM_NAME="ccextractor"
VERSION="0.85"
RELEASE="1"
LICENSE="GPL-2.0"
MAINTAINER="carlos@ccextractor.org"
REQUIRES="gcc,libcurl4-gnutls-dev,tesseract-ocr,tesseract-ocr-dev,libleptonica-dev"

../linux/pre-build.sh

out=$((LC_ALL=C dpkg -s checkinstall) 2>&1)

if [[ $out == *"is not installed"* ]]
then
    read -r -p "You have not installed the package 'checkinstall'. Would you like to install it? [Y/N] " response
    if [[ $response =~ ^([yY][eE][sS]|[yY])$ ]]
    then
        apt-get install -y checkinstall
    else
        exit 0
    fi
fi


(cd ..; ./autogen.sh; ./configure; make; sudo checkinstall \
    -y \
    --pkgrelease=$RELEASE \
    --pkggroup="CCExtractor" \
    --backup=no \
    --install=no \
    --type $TYPE \
    --pkgname=$PROGRAM_NAME \
    --pkgversion=$VERSION \
    --pkglicense=$LICENSE \
    --pakdir="package_creators/build" \
    --maintainer=$MAINTAINER \
    --nodoc \
    --requires=$REQUIRES)
