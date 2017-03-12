SHELL = /bin/sh

CC = gcc
CFLAGS = -O3 -std=gnu99 -s
INCLUDE = -I$(srcdir)/../src/gpacmp4/ -I$(srcdir)/../src/libpng -I$(srcdir)/../src/lib_hash -I$(srcdir)/../src/protobuf-c -I$(srcdir)/../src/zlib -I$(srcdir)/../src/lib_ccx -I$(srcdir)/../src/.
INCLUDE += -I$(srcdir)/../src/zvbi -I$(srcdir)/../src/utf8proc
ALL_FLAGS = -Wno-write-strings -D_FILE_OFFSET_BITS=64 -DVERSION_FILE_PRESENT
LDFLAGS = -lm

if SYS_IS_LINUX
CFLAGS +=-DGPAC_CONFIG_LINUX
endif
TARGET = ccextractor

OBJS_DIR = objs
VPATH = $(srcdir)/../src:$(srcdir)/../src/gpacmp4:$(srcdir)/../src/libpng:$(srcdir)/../src/zlib:$(srcdir)/../src/lib_ccx:$(srcdir)/../src/zvbi:$(srcdir)/../src/lib_hash:$(srcdir)/../src/utf8proc:$(srcdir)/../src/protobuf-c

SRCS_DIR = $(srcdir)/../src
SRCS_C = $(wildcard $(SRCS_DIR)/*.c)
OBJS = $(SRCS_C:$(SRCS_DIR)/%.c=$(OBJS_DIR)/%.o)

SRCS_CCX_DIR = $(SRCS_DIR)/lib_ccx
SRCS_CCX = $(wildcard $(SRCS_CCX_DIR)/*.c)
OBJS_CCX = $(SRCS_CCX:$(SRCS_CCX_DIR)/%.c=$(OBJS_DIR)/%.o)

SRCS_PNG_DIR = $(SRCS_DIR)/libpng
SRCS_PNG = $(wildcard $(SRCS_PNG_DIR)/*.c)
OBJS_PNG = $(SRCS_PNG:$(SRCS_PNG_DIR)/%.c=$(OBJS_DIR)/%.o)

SRCS_ZVBI_DIR = $(SRCS_DIR)/zvbi
SRCS_ZVBI = $(wildcard $(SRCS_ZVBI_DIR)/*.c)
OBJS_ZVBI = $(SRCS_ZVBI:$(SRCS_ZVBI_DIR)/%.c=$(OBJS_DIR)/%.o)

SRCS_GPACMP4_DIR = $(SRCS_DIR)/gpacmp4
SRCS_GPACMP4_C = $(wildcard $(SRCS_GPACMP4_DIR)/*.c)
SRCS_GPACMP4_CPP = $(wildcard $(SRCS_GPACMP4_DIR)/*.cpp)
OBJS_GPACMP4 = $(SRCS_GPACMP4_C:$(SRCS_GPACMP4_DIR)/%.c=$(OBJS_DIR)/%.o) \
			   $(SRCS_GPACMP4_CPP:$(SRCS_GPACMP4_DIR)/%.cpp=$(OBJS_DIR)/%.o)

SRCS_ZLIB_DIR = $(SRCS_DIR)/zlib
SRCS_ZLIB = $(wildcard $(SRCS_ZLIB_DIR)/*.c)
OBJS_ZLIB = $(SRCS_ZLIB:$(SRCS_ZLIB_DIR)/%.c=$(OBJS_DIR)/%.o)

SRCS_HASH_DIR = $(SRCS_DIR)/lib_hash
SRCS_HASH = $(wildcard $(SRCS_HASH_DIR)/*.c)
OBJS_HASH = $(SRCS_HASH:$(SRCS_HASH_DIR)/%.c=$(OBJS_DIR)/%.o)

SRCS_UTF8_DIR = $(SRCS_DIR)/utf8proc
SRCS_UTF8 = $(SRCS_UTF8_DIR)/utf8proc.c
OBJS_UTF8 = $(SRCS_UTF8:$(SRCS_UTF8_DIR)/%.c=$(OBJS_DIR)/%.o)

INSTLALL = cp -f -p
INSTLALL_PROGRAM = $(INSTLALL)
DESTDIR = /usr/bin

if HARDSUBX_IS_ENABLED
CFLAGS+=-DENABLE_HARDSUBX
CFLAGS+= $(shell pkg-config --cflags libavcodec)
CFLAGS+= $(shell pkg-config --cflags libavformat)
CFLAGS+= $(shell pkg-config --cflags libavutil)
CFLAGS+= $(shell pkg-config --cflags libswscale)
AV_LDFLAGS= $(shell pkg-config --libs libavcodec )
AV_LDFLAGS+= $(shell pkg-config --libs libavformat )
AV_LDFLAGS+= $(shell pkg-config --libs libavutil )
AV_LDFLAGS+= $(shell pkg-config --libs libswscale )
LDFLAGS+= $(AV_LDFLAGS)
endif

if OCR_IS_ENABLED
CFLAGS+=-DENABLE_OCR -DPNG_NO_CONFIG_H
LEPT_LDFLAGS= $(shell pkg-config --libs lept)

if TESSERACT_PRESENT
TESS_LDFLAGS= $(shell pkg-config --libs tesseract)
TESS_CFLAGS= $(shell pkg-config --cflags tesseract)
else
#fix for raspberry pi not having a pkgconfig file for tesseract
if TESSERACT_PRESENT_RPI
TESS_LDFLAGS= -ltesseract
TESS_CFLAGS= -I/usr/include/tesseract
endif
endif

CFLAGS  += $(TESS_CFLAGS)
CFLAGS  += $(shell pkg-config --cflags lept)
LDFLAGS += $(TESS_LDFLAGS)
LDFLAGS += $(LEPT_LDFLAGS)
endif


if FFMPEG_IS_ENABLED
CFLAGS+=-DENABLE_FFMPEG
CFLAGS+= $(shell pkg-config --cflags libavcodec)
CFLAGS+= $(shell pkg-config --cflags libavformat)
CFLAGS+= $(shell pkg-config --cflags libavutil)
LDFLAGS+= $(shell pkg-config --libs libavcodec )
LDFLAGS+= $(shell pkg-config --libs libavformat )
LDFLAGS+= $(shell pkg-config --libs libavutil )
endif

all-local: pre-build objs_dir $(TARGET)

.PHONY: objs_dir
objs_dir: 
	mkdir -p $(OBJS_DIR)

$(TARGET): $(OBJS) $(OBJS_PNG) $(OBJS_GPACMP4) $(OBJS_ZVBI) $(OBJS_ZLIB) $(OBJS_HASH) $(OBJS_CCX) $(OBJS_UTF8)
	$(CC) $(ALL_FLAGS) $(CFLAGS) $(OBJS) $(OBJS_CCX) $(OBJS_PNG) $(OBJS_ZVBI) $(OBJS_GPACMP4) $(OBJS_ZLIB) $(OBJS_HASH) $(OBJS_UTF8) $(LDFLAGS) -o $@

$(OBJS_DIR)/%.o: %.c
	$(CC) -c $(ALL_FLAGS) $(INCLUDE) $(CFLAGS) $< -o $@  

$(OBJS_DIR)/%.o: %.cpp
	$(CC) -c $(ALL_FLAGS) $(INCLUDE) $(CFLAGS) $< -o $@ -I$(srcdir)/../src/gpacmp4 

$(OBJS_DIR)/ccextractor.o: ccextractor.c
	$(CC) -c $(ALL_FLAGS) $(INCLUDE) $(CFLAGS) -O0 $< -o $@  

clean-local:
	rm -rf $(TARGET) 2>/dev/null || true
	rm -rf $(OBJS_CCX) $(OBJS_PNG) $(OBJS_ZLIB) $(OBJS_GPACMP4) $(OBJS_HASH) $(OBJS_UTF8) $(OBJS) 2>/dev/null || true
	rm -rdf $(OBJS_DIR) 2>/dev/null || true
	rm -rf .depend 2>/dev/null || true

install-exec-local: $(TARGET)
	$(INSTLALL_PROGRAM) $(TARGET) $(DESTDIR)

uninstall-local:
	rm -iv $(DESTDIR)/$(TARGET) 

.PHONY: depend dep
depend dep:
	$(CC) $(CFLAGS) $(INCLUDE) -E -MM $(SRCS_C) $(SRCS_PNG) $(SRCS_ZVBI) $(SRCS_ZLIB) $(SRCS_HASH) $(SRCS_UTF8) $(SRCS_CCX) \
		$(SRCS_GPACMP4_C) $(SRCS_GPACMP4_CPP) |\
		sed 's/^[a-zA-Z_0-9]*.o/$(OBJS_DIR)\/&/' > .depend

.PHONY: pre-build
pre-build:
	$(srcdir)/pre-build.sh

-include .depend
