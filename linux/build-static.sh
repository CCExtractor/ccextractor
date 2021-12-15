#!/usr/bin/env -S sh -ex

####################################################################
# setup by tracey apr 2012
# updated version dec 2016
# see: http://www.ccextractor.org/doku.php
####################################################################


# build it static!
# simplest way is with linux alpine
# hop onto box with docker on it and cd to dir of the file you are staring at
# You will get a static-compiled binary and english language library file in the end.
if [ ! -e /tmp/cc/ccextractor-README.txt ]; then
  rm -rf /tmp/cc;
  mkdir -p -m777 /tmp/cc;
  mkdir -p -m777 ../lib/tessdata/;
  cp ccextractor-README.txt /tmp/cc/;
  sudo docker run -v /tmp/cc:/tmp/cc --rm -it alpine:latest /tmp/cc/ccextractor-README.txt;
  # NOTE: _AFTER_ testing/validating, you can promote it from "ccextractor.next" to "ccextractor"... ;-)
  cp /tmp/cc/*traineddata ../lib/tessdata/;
  chmod go-w ../lib/tessdata/;
  exit 0;
fi

# NOW we are inside docker container...
cd /tmp/cc;


# we want tesseract (for OCR)
echo '
http://dl-cdn.alpinelinux.org/alpine/v3.5/main
http://dl-cdn.alpinelinux.org/alpine/v3.5/community
' >| /etc/apk/repositories;
apk update;  apk upgrade;

apk add --update bash zsh alpine-sdk perl;

# (needed by various static builds below)
# Even though we're going to (re)builid tesseract from source statically, get its dependencies setup by
# installing it now, too.
apk add autoconf automake libtool tesseract-ocr-dev;


# Now comes the not-so-fun parts...  Many packages _only_ provide .so files in their distros -- not the .a
# needed files for building something with it statically.  Step through them now...


# libgif
wget https://sourceforge.net/projects/giflib/files/giflib-5.1.4.tar.gz;
zcat giflib*tar.gz | tar xf -;
cd giflib*/;
./configure --disable-shared --enable-static;  make;  make install;
hash -r;
cd -;


# libwebp
git clone https://github.com/webmproject/libwebp;
cd libwebp;
./autogen.sh;
./configure --disable-shared --enable-static;  make;  make install;
cd -;


# leptonica
wget http://www.leptonica.org/source/leptonica-1.73.tar.gz;
zcat leptonica*tar.gz | tar xf -;
cd leptonica*/;
./configure --disable-shared --enable-static;  make;  make install;
hash -r;
cd -;


# tesseract
git clone https://github.com/tesseract-ocr/tesseract;
cd tesseract;
./autogen.sh;
./configure --disable-shared --enable-static;  make;  make install;
cd -;


# ccextractor -- build static
git clone https://github.com/CCExtractor/ccextractor;
cd ccextractor/linux/;
perl -i -pe 's/O3 /O3 -static /' Makefile;
# quick patch:
perl -i -pe 's/(strchr|strstr)\(/$1((char *)/'  ../src/thirdparty/gpacmp4/url.c  ../src/thirdparty/gpacmp4/error.c;
set +e; # this _will_ FAIL at the end..
make ENABLE_OCR=yes;
set -e;
# I confess hand-compiling (cherrypicking which .a to use when there are 2, etc.) is fragile...
# But it was the _only_ way I could get a fully static build after hours of thrashing...
gcc -Wno-write-strings -Wno-pointer-sign -D_FILE_OFFSET_BITS=64 -DVERSION_FILE_PRESENT -O3 -std=gnu99 -s -DGPAC_CONFIG_LINUX -DENABLE_OCR -DPNG_NO_CONFIG_H -I/usr/local/include/tesseract -I/usr/local/include/leptonica objs/*.o -o ccextractor \
  --static -lm \
  /usr/local/lib/libtesseract.a \
  /usr/local/lib/liblept.a \
  /usr/local/lib/libgif.a \
  /usr/local/lib/libwebp.a \
  /usr/lib/libjpeg.a \
  /usr/lib/libtiff.a \
  /usr/lib/libgomp.a \
   -lstdc++;

cp ccextractor /tmp/cc/ccextractor.next;
cd -;

# get english lang trained data
wget https://github.com/tesseract-ocr/tessdata/raw/master/eng.traineddata;
