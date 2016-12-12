SHELL = /bin/sh

CC=gcc
# SYS := $(shell gcc -dumpmachine)
CFLAGS=-O0 -std=gnu99 -D ENABLE_OCR -g -ggdb -rdynamic
#-Q -da -v

# enable COVERAGE
# CFLAGS+=-fprofile-arcs -ftest-coverage

# add debug flag
ifdef DEBUG
CFLAGS+=-DDEBUG
endif

#ALL_FLAGS = -Wno-write-strings -D_FILE_OFFSET_BITS=64 -DVERSION_FILE_PRESENT
LDFLAGS=-lm -g

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
	# explicit output name :  -o $@
	$(CC) -c $(ALL_FLAGS) $(CFLAGS) $<

runtest: $(OBJS)
	@echo "+----------------------------------------------+"
	@echo "|                 BUILD TESTS                  |"
	@echo "+----------------------------------------------+"
	$(CC) -c $(ALL_FLAGS) $(CFLAGS) $@.c
	$(CC) $(SRC) $@.o $^ $(ALL_FLAGS) $(CFLAGS) $(LDFLAGS) -o $@

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
	# coverage info
	rm *.gcda || true
	rm *.gcno || true
	# debug info
	rm *.c.* || true