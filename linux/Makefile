SHELL = /bin/sh

CC = gcc
SYS := $(shell gcc -dumpmachine)
CFLAGS = -O3 -std=gnu99
INCLUDE = -I../src/gpacmp4/ -I../src/libpng -I../src/zlib -I../src/lib_ccx -I../src/.
ALL_FLAGS = -Wno-write-strings -D_FILE_OFFSET_BITS=64
LDFLAGS = -lm

ifneq (, $(findstring linux, $(SYS)))
CFLAGS +=-DGPAC_CONFIG_LINUX
endif
TARGET = ccextractor

OBJS_DIR = objs
VPATH = ../src:../src/gpacmp4:../src/libpng:../src/zlib:../src/lib_ccx

SRCS_DIR = ../src
SRCS_C = $(wildcard $(SRCS_DIR)/*.c)
OBJS = $(SRCS_C:$(SRCS_DIR)/%.c=$(OBJS_DIR)/%.o)

SRCS_CCX_DIR = $(SRCS_DIR)/lib_ccx
SRCS_CCX = $(wildcard $(SRCS_CCX_DIR)/*.c)
OBJS_CCX = $(SRCS_CCX:$(SRCS_CCX_DIR)/%.c=$(OBJS_DIR)/%.o)

SRCS_PNG_DIR = $(SRCS_DIR)/libpng
SRCS_PNG = $(wildcard $(SRCS_PNG_DIR)/*.c)
OBJS_PNG = $(SRCS_PNG:$(SRCS_PNG_DIR)/%.c=$(OBJS_DIR)/%.o)

SRCS_GPACMP4_DIR = $(SRCS_DIR)/gpacmp4
SRCS_GPACMP4_C = $(wildcard $(SRCS_GPACMP4_DIR)/*.c)
SRCS_GPACMP4_CPP = $(wildcard $(SRCS_GPACMP4_DIR)/*.cpp)
OBJS_GPACMP4 = $(SRCS_GPACMP4_C:$(SRCS_GPACMP4_DIR)/%.c=$(OBJS_DIR)/%.o) \
			   $(SRCS_GPACMP4_CPP:$(SRCS_GPACMP4_DIR)/%.cpp=$(OBJS_DIR)/%.o)

SRCS_ZLIB_DIR = $(SRCS_DIR)/zlib
SRCS_ZLIB = $(wildcard $(SRCS_ZLIB_DIR)/*.c)
OBJS_ZLIB = $(SRCS_ZLIB:$(SRCS_ZLIB_DIR)/%.c=$(OBJS_DIR)/%.o)

INSTLALL = cp -f -p
INSTLALL_PROGRAM = $(INSTLALL)
DESTDIR = /usr/bin

ifeq ($(ENABLE_OCR),yes)
CFLAGS+=-DENABLE_OCR
TESS_LDFLAGS+= $(shell pkg-config --libs tesseract)
LEPT_LDFLAGS+= $(shell pkg-config --libs lept)

#error checking of library are there or not
ifeq ($(TESS_LDFLAGS),$(EMPTY))
$(error **ERROR** "tesseract not found")
else
#TODO print the version of library found
$(info  "tesseract found")
endif
ifeq ($(LEPT_LDFLAGS),$(EMPTY))
$(error **ERROR** "leptonica not found")
else
#TODO print the version of library found
$(info  "Leptonica found")
endif

CFLAGS  += $(shell pkg-config --cflags tesseract)
CFLAGS  += $(shell pkg-config --cflags lept)
LDFLAGS += $(TESS_LDFLAGS)
LDFLAGS += $(LEPT_LDFLAGS)
endif


ifeq ($(ENABLE_FFMPEG),yes)
CFLAGS+=-DENABLE_FFMPEG
CFLAGS+= $(shell pkg-config --cflags libavcodec)
CFLAGS+= $(shell pkg-config --cflags libavformat)
CFLAGS+= $(shell pkg-config --cflags libavutil)
LDFLAGS+= $(shell pkg-config --libs libavcodec )
LDFLAGS+= $(shell pkg-config --libs libavformat )
LDFLAGS+= $(shell pkg-config --libs libavutil )
endif

.PHONY: all
all: objs_dir $(TARGET) 

.PHONY: objs_dir
objs_dir: 
	mkdir -p $(OBJS_DIR)

$(TARGET): $(OBJS) $(OBJS_PNG) $(OBJS_GPACMP4) $(OBJS_ZLIB) $(OBJS_CCX)
	$(CC) $(ALL_FLAGS) $(CFLAGS) $(OBJS) $(OBJS_CCX) $(OBJS_PNG) $(OBJS_GPACMP4) $(OBJS_ZLIB) $(LDFLAGS) -o $@

$(OBJS_DIR)/%.o: %.c
	$(CC) -c $(ALL_FLAGS) $(INCLUDE) $(CFLAGS) $< -o $@  

$(OBJS_DIR)/%.o: %.cpp
	$(CC) -c $(ALL_FLAGS) $(INCLUDE) $(CFLAGS) $< -o $@ -I../src/gpacmp4 

.PHONY: clean
clean:
	rm -rf $(TARGET) 2>/dev/null || true
	rm -rf $(OBJS_CCX) $(OBJS_PNG) $(OBJS_ZLIB) $(OBJS_GPACMP4) $(OBJS) 2>/dev/null || true
	rm -rdf $(OBJS_DIR) 2>/dev/null || true
	rm -rf .depend 2>/dev/null || true

.PHONY: install
install: $(TARGET)
	$(INSTLALL_PROGRAM) $(TARGET) $(DESTDIR)

.PHONY: uninstall
uninstall: 
	rm -iv $(DESTDIR)/$(TARGET) 

.PHONY: depend dep
depend dep:
	$(CC) $(CFLAGS) -E -MM $(SRCS_C) $(SRCS_PNG) $(SRCS_ZLIB) $(SRCS_CXX) \
		$(SRCS_GPACMP4_C) $(SRCS_GPACMP4_CPP) |\
		sed 's/^[a-zA-Z_0-9]*.o/$(OBJS_DIR)\/&/' > .depend

-include .depend
