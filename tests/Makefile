SHELL = /bin/sh

CC = gcc
# SYS := $(shell gcc -dumpmachine)
CFLAGS = -O3 -std=gnu99 -s -D ENABLE_OCR
#ALL_FLAGS = -Wno-write-strings -D_FILE_OFFSET_BITS=64 -DVERSION_FILE_PRESENT
LDFLAGS = -lm


CFLAGS+= $(shell pkg-config --cflags check)
LDFLAGS+= $(shell pkg-config --libs check)

SRC_TEST=$(shell find ./ -iname '*_test.c')

# TODO : rewrite...
SRC=../src/lib_ccx/ccx_encoders_splitbysentence.c
SRC+=$(SRC_TEST)

all:	#$(TARGET)
	$(CC) $(SRC) $(ALL_FLAGS) $(INCLUDE) $(CFLAGS) $(LDFLAGS) -o runtest	# $< -o $@
	./runtest
