# Comment out the following line if you don't have libpng

CPPFLAGS += -DGPAC_CONFIG_LINUX -D_FILE_OFFSET_BITS=64 -I../src/gpacmp4 -I../src/libpng/ -I../src/zlib/
CFLAGS += -g -Wno-write-strings

CXXFLAGS = $(CFLAGS)
CC = $(CXX)

bindir = $(HOME)/bin
INSTALL = cp -f -p
INSTALL_PROGRAM = cp -f -p

VPATH = ../src:../src/gpacmp4:../src/libpng:../src/zlib

src_OBJS = \
	608_helpers.o 608.o 608_sami.o 608_smptett.o 608_srt.o 608_spupng.o \
	708_encoding.o 708.o activity.o asf_functions.o avc_functions.o \
	cc_bitstream.o cc_decoders_common.o ccextractor.o constants.o \
	encoding.o es_functions.o es_userdata.o file_functions.o \
	general_loop.o mp4.o myth.o output.o params_dump.o params.o \
	sequencing.o stream_functions.o telxcc.o timing.o ts_functions.o \
	utility.o wtv_functions.o xds.o

gpacmp4_OBJS = \
	avc_ext.o avilib.o av_parsers.o base_encoding.o bitstream.o \
	box_code_3gpp.o box_code_apple.o box_code_base.o box_code_isma.o \
	box_code_meta.o box_funcs.o data_map.o desc_private.o descriptors.o \
	error.o gpac_ogg.o hinting.o ipmpx_code.o ipmpx_parse.o \
	isom_intern.o isom_read.o isom_store.o isom_write.o list.o \
	media.o media_odf.o meta.o movie_fragments.o \
	odf_code.o odf_codec.o odf_command.o os_divers.o qos.o sample_descs.o \
	slc.o stbl_read.o stbl_write.o track.o tx3g.o url.o utf.o

src_SRC = \
	../src/608.cpp ../src/608_helpers.cpp ../src/608_sami.cpp \
	../src/608_smptett.cpp ../src/608_spupng.cpp ../src/608_srt.cpp \
	../src/708.cpp ../src/708_encoding.cpp ../src/activity.cpp \
	../src/asf_functions.cpp ../src/avc_functions.cpp ../src/cc_bitstream.cpp \
	../src/cc_decoders_common.cpp ../src/ccextractor.cpp ../src/constants.cpp \
	../src/encoding.cpp ../src/es_functions.cpp ../src/es_userdata.cpp \
	../src/file_functions.cpp ../src/general_loop.cpp ../src/myth.cpp \
	../src/output.cpp ../src/params.cpp ../src/params_dump.cpp \
	../src/sequencing.cpp ../src/stream_functions.cpp ../src/telxcc.cpp \
	../src/timing.cpp ../src/ts_functions.cpp ../src/utility.cpp \
	../src/wtv_functions.cpp ../src/xds.cpp

gpacmp4_SRC = \
	../src/gpacmp4/avc_ext.c ../src/gpacmp4/avilib.c \
	../src/gpacmp4/av_parsers.c ../src/gpacmp4/base_encoding.c \
	../src/gpacmp4/bitstream.c ../src/gpacmp4/box_code_3gpp.c \
	../src/gpacmp4/box_code_apple.c ../src/gpacmp4/box_code_base.c \
	../src/gpacmp4/box_code_isma.c ../src/gpacmp4/box_code_meta.c \
	../src/gpacmp4/box_funcs.c ../src/gpacmp4/data_map.c \
	../src/gpacmp4/desc_private.c ../src/gpacmp4/descriptors.c \
	../src/gpacmp4/error.c ../src/gpacmp4/gpac_ogg.c \
	../src/gpacmp4/hinting.c ../src/gpacmp4/ipmpx_code.c \
	../src/gpacmp4/ipmpx_parse.c ../src/gpacmp4/isom_intern.c \
	../src/gpacmp4/isom_read.c ../src/gpacmp4/isom_store.c \
	../src/gpacmp4/isom_write.c ../src/gpacmp4/list.c \
	../src/gpacmp4/media.c ../src/gpacmp4/media_odf.c \
	../src/gpacmp4/meta.c ../src/gpacmp4/movie_fragments.c \
	../src/gpacmp4/mp4.cpp ../src/gpacmp4/odf_code.c \
	../src/gpacmp4/odf_codec.c ../src/gpacmp4/odf_command.c \
	../src/gpacmp4/os_divers.c ../src/gpacmp4/qos.c \
	../src/gpacmp4/sample_descs.c ../src/gpacmp4/slc.c \
	../src/gpacmp4/stbl_read.c ../src/gpacmp4/stbl_write.c \
	../src/gpacmp4/track.c ../src/gpacmp4/tx3g.c \
	../src/gpacmp4/url.c ../src/gpacmp4/utf.c

libpng_SRC = \
	../src/libpng/png.c ../src/libpng/pngerror.c ../src/libpng/pngmem.c \
	../src/libpng/pngset.c ../src/libpng/pngtrans.c ../src/libpng/pngwio.c \
	../src/libpng/pngwrite.c ../src/libpng/pngwtran.c ../src/libpng/pngwutil.c

libpng_OBJS = \
	png.o pngerror.o pngmem.o pngset.o pngtrans.o pngwio.o \
	pngwrite.o pngwtran.o pngwutil.o

zlib_SRC = \
	../src/zlib/adler32.c ../src/zlib/crc32.c ../src/zlib/deflate.c \
	../src/zlib/inffast.c ../src/zlib/inflate.c ../src/zlib/inftrees.c \
	../src/zlib/trees.c ../src/zlib/zutil.c

zlib_OBJS = \
	adler32.o crc32.o deflate.o inffast.o inflate.o inftrees.o \
	trees.o zutil.o

.PHONY: all
all: ccextractor

.PHONY: clean
clean:
	rm -f *.o ccextractor depend

ccextractor: $(src_OBJS) $(gpacmp4_OBJS) $(libpng_OBJS) $(zlib_OBJS)

.PHONY: install
install: ccextractor
	$(INSTALL_PROGRAM) ccextractor $(bindir)

depend:
	$(CXX) $(CXXFLAGS) -E -MM $(src_SRC) $(gpacmp4_SRC) $(libpng_SRC) $(zlib_SRC) > depend

include depend
