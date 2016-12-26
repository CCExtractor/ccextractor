#!/bin/bash
TYPE="debian"   # can be one of 'slackware', 'debian', 'rpm'
PROGRAM_NAME="ccextractor"
VERSION="0.84"
RELEASE="1"
LICENSE="GPL-2.0"
MAINTAINER="carlos@ccextractor.org"
REQUIRES="gcc,libcurl4-gnutls-dev,tesseract-ocr,tesseract-ocr-dev,libleptonica-dev"

(cd ../linux; checkinstall \
    -y \
    --pkgrelease=$RELEASE \
    --pkggroup="CCExtractor" \
    --backup=no \
    --install=no \
    --type $TYPE \
    --pkgname=$PROGRAM_NAME \
    --pkgversion=$VERSION \
    --pkglicense=$LICENSE \
    --pakdir="../package_creators/build" \
    --maintainer=$MAINTAINER \
    --nodoc \
    --requires=$REQUIRES)
