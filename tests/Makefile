SHELL = /bin/sh

CC=gcc
# SYS := $(shell gcc -dumpmachine)
CFLAGS=-std=gnu99 -s -D ENABLE_OCR -fprofile-arcs -ftest-coverage -g -O0 -Q

# add debug flag
ifdef DEBUG
CFLAGS+=-D DEBUG
endif

#ALL_FLAGS = -Wno-write-strings -D_FILE_OFFSET_BITS=64 -DVERSION_FILE_PRESENT
LDFLAGS=-lm

CFLAGS+=$(shell pkg-config --cflags check)
LDFLAGS+=$(shell pkg-config --libs check)

# TODO: need to rewrite this. Need new way to load sources for testing
SRC=$(wildcard ../src/lib_ccx/ccx_encoders_splitbysentence.c)
OBJS=

SRC_SUITE=$(wildcard *_suite.c)
OBJ_SUITE=$(patsubst %_suite.c, %_suite.o, $(SRC_SUITE))

OBJS+=$(OBJ_SUITE)

all: clean test

%.o: %.c
	$(CC) -c $(ALL_FLAGS) $(CFLAGS) $< -o $@

runtest: $(OBJS)
	@echo "+----------------------------------------------+"
	@echo "|                 BUILD TESTS                  |"
	@echo "+----------------------------------------------+"
	$(CC) $(SRC) $@.c $^ $(ALL_FLAGS) $(CFLAGS) $(LDFLAGS) -o $@

.PHONY: test
test: runtest
	@echo "+----------------------------------------------+"
	@echo "|                 START TESTS                  |"
	@echo "+----------------------------------------------+"
	./runtest

.PHONY: clean
clean:
	rm runtest || true
	rm *.o || true