AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS  = -I m4/

bin_PROGRAMS = ccextractor
ccextractor_SOURCES = \
				../src/ccextractor.c \
				../src/ccextractor.h \
				../src/thirdparty/gpacmp4/avc_ext.c \
				../src/thirdparty/gpacmp4/avilib.c \
				../src/thirdparty/gpacmp4/av_parsers.c \
				../src/thirdparty/gpacmp4/base_encoding.c \
				../src/thirdparty/gpacmp4/bitstream.c \
				../src/thirdparty/gpacmp4/box_code_3gpp.c \
				../src/thirdparty/gpacmp4/box_code_adobe.c \
				../src/thirdparty/gpacmp4/box_code_apple.c \
				../src/thirdparty/gpacmp4/box_code_base.c \
				../src/thirdparty/gpacmp4/box_code_drm.c \
				../src/thirdparty/gpacmp4/box_dump.c \
				../src/thirdparty/gpacmp4/box_code_meta.c \
				../src/thirdparty/gpacmp4/box_funcs.c \
				../src/thirdparty/gpacmp4/color.c \
				../src/thirdparty/gpacmp4/configfile.c \
				../src/thirdparty/gpacmp4/data_map.c \
				../src/thirdparty/gpacmp4/desc_private.c \
				../src/thirdparty/gpacmp4/descriptors.c \
				../src/thirdparty/gpacmp4/drm_sample.c \
				../src/thirdparty/gpacmp4/error.c \
				../src/thirdparty/gpacmp4/gpac_ogg.c \
				../src/thirdparty/gpacmp4/hint_track.c \
				../src/thirdparty/gpacmp4/hinting.c \
				../src/thirdparty/gpacmp4/ipmpx_code.c \
				../src/thirdparty/gpacmp4/ipmpx_parse.c \
				../src/thirdparty/gpacmp4/isom_intern.c \
				../src/thirdparty/gpacmp4/isom_read.c \
				../src/thirdparty/gpacmp4/isom_store.c \
				../src/thirdparty/gpacmp4/isom_write.c \
				../src/thirdparty/gpacmp4/list.c \
				../src/thirdparty/gpacmp4/math.c \
				../src/thirdparty/gpacmp4/media.c \
				../src/thirdparty/gpacmp4/media_odf.c \
				../src/thirdparty/gpacmp4/meta.c \
				../src/thirdparty/gpacmp4/movie_fragments.c \
				../src/thirdparty/gpacmp4/odf_code.c \
				../src/thirdparty/gpacmp4/odf_codec.c \
				../src/thirdparty/gpacmp4/odf_command.c \
				../src/thirdparty/gpacmp4/os_config_init.c \
				../src/thirdparty/gpacmp4/os_divers.c \
				../src/thirdparty/gpacmp4/os_file.c \
				../src/thirdparty/gpacmp4/qos.c \
				../src/thirdparty/gpacmp4/sample_descs.c \
				../src/thirdparty/gpacmp4/slc.c \
				../src/thirdparty/gpacmp4/stbl_read.c \
				../src/thirdparty/gpacmp4/stbl_write.c \
				../src/thirdparty/gpacmp4/track.c \
				../src/thirdparty/gpacmp4/tx3g.c \
				../src/thirdparty/gpacmp4/url.c \
				../src/thirdparty/gpacmp4/utf.c \
				../src/thirdparty/gpacmp4/os_thread.c \
				../src/thirdparty/gpacmp4/module.c \
				../src/thirdparty/gpacmp4/os_module.c \
				../src/thirdparty/gpacmp4/xml_parser.c \
				../src/thirdparty/gpacmp4/constants.c \
				../src/thirdparty/gpacmp4/gpac/avparse.h \
				../src/thirdparty/gpacmp4/gpac/base_coding.h \
				../src/thirdparty/gpacmp4/gpac/bitstream.h \
				../src/thirdparty/gpacmp4/gpac/color.h \
				../src/thirdparty/gpacmp4/gpac/config_file.h \
				../src/thirdparty/gpacmp4/gpac/configuration.h \
				../src/thirdparty/gpacmp4/gpac/constants.h \
				../src/thirdparty/gpacmp4/gpac/events_constants.h \
				../src/thirdparty/gpacmp4/gpac/ietf.h \
				../src/thirdparty/gpacmp4/gpac/isomedia.h \
				../src/thirdparty/gpacmp4/gpac/list.h \
				../src/thirdparty/gpacmp4/gpac/maths.h \
				../src/thirdparty/gpacmp4/gpac/media_tools.h \
				../src/thirdparty/gpacmp4/gpac/mpeg4_odf.h \
				../src/thirdparty/gpacmp4/gpac/network.h \
				../src/thirdparty/gpacmp4/gpac/revision.h \
				../src/thirdparty/gpacmp4/gpac/setup.h \
				../src/thirdparty/gpacmp4/gpac/tools.h \
				../src/thirdparty/gpacmp4/gpac/utf.h \
				../src/thirdparty/gpacmp4/gpac/version.h \
				../src/thirdparty/gpacmp4/gpac/iso639.h \
				../src/thirdparty/gpacmp4/gpac/internal/avilib.h \
				../src/thirdparty/gpacmp4/gpac/internal/isomedia_dev.h \
				../src/thirdparty/gpacmp4/gpac/internal/media_dev.h \
				../src/thirdparty/gpacmp4/gpac/internal/odf_dev.h \
				../src/thirdparty/gpacmp4/gpac/internal/odf_parse_common.h \
				../src/thirdparty/gpacmp4/gpac/internal/ogg.h \
				../src/thirdparty/libpng/pngstruct.h \
				../src/thirdparty/libpng/pngpriv.h \
				../src/thirdparty/libpng/pnginfo.h \
				../src/thirdparty/libpng/pnglibconf.h \
				../src/thirdparty/libpng/pngconf.h \
				../src/thirdparty/libpng/pngdebug.h \
				../src/thirdparty/libpng/png.h \
				../src/thirdparty/libpng/png.c \
				../src/thirdparty/libpng/pngerror.c \
				../src/thirdparty/libpng/pngget.c \
				../src/thirdparty/libpng/pngmem.c \
				../src/thirdparty/libpng/pngpread.c \
				../src/thirdparty/libpng/pngread.c \
				../src/thirdparty/libpng/pngrio.c \
				../src/thirdparty/libpng/pngrtran.c \
				../src/thirdparty/libpng/pngrutil.c \
				../src/thirdparty/libpng/pngset.c \
				../src/thirdparty/libpng/pngtrans.c \
				../src/thirdparty/libpng/pngwio.c \
				../src/thirdparty/libpng/pngwrite.c \
				../src/thirdparty/libpng/pngwtran.c \
				../src/thirdparty/libpng/pngwutil.c \
				../src/lib_ccx/ccx_common_common.h \
				../src/lib_ccx/ccx_common_option.h \
				../src/lib_ccx/utility.h \
				../src/lib_ccx/activity.h \
				../src/lib_ccx/asf_constants.h \
				../src/lib_ccx/avc_functions.h \
				../src/lib_ccx/bitstream.h \
				../src/lib_ccx/ccx_common_option.c \
				../src/lib_ccx/ccx_common_common.c \
				../src/lib_ccx/utility.c \
				../src/lib_ccx/activity.c \
				../src/lib_ccx/asf_functions.c \
				../src/lib_ccx/avc_functions.c \
				../src/lib_ccx/cc_bitstream.c \
				../src/lib_ccx/ccx_common_char_encoding.c \
				../src/lib_ccx/ccx_common_char_encoding.h \
				../src/lib_ccx/ccx_common_constants.c \
				../src/lib_ccx/ccx_common_constants.h \
				../src/lib_ccx/ccx_common_platform.h \
				../src/lib_ccx/ccx_common_structs.h \
				../src/lib_ccx/ccx_common_timing.c \
				../src/lib_ccx/ccx_common_timing.h \
				../src/lib_ccx/ccx_decoders_608.c \
				../src/lib_ccx/ccx_decoders_608.h \
				../src/lib_ccx/ccx_decoders_708.c \
				../src/lib_ccx/ccx_decoders_708_encoding.c \
				../src/lib_ccx/ccx_decoders_708_encoding.h \
				../src/lib_ccx/ccx_decoders_708.h \
				../src/lib_ccx/ccx_decoders_708_output.c \
				../src/lib_ccx/ccx_decoders_708_output.h \
				../src/lib_ccx/ccx_decoders_common.c \
				../src/lib_ccx/ccx_decoders_common.h \
				../src/lib_ccx/ccx_decoders_isdb.c \
				../src/lib_ccx/ccx_decoders_isdb.h \
				../src/lib_ccx/ccx_decoders_structs.h \
				../src/lib_ccx/ccx_decoders_vbi.c \
				../src/lib_ccx/ccx_decoders_vbi.h \
				../src/lib_ccx/ccx_decoders_xds.c \
				../src/lib_ccx/ccx_decoders_xds.h \
				../src/lib_ccx/ccx_demuxer.c \
				../src/lib_ccx/ccx_demuxer.h \
				../src/lib_ccx/ccx_demuxer_mxf.c \
				../src/lib_ccx/ccx_demuxer_mxf.h \
				../src/lib_ccx/ccx_dtvcc.c \
				../src/lib_ccx/ccx_dtvcc.h \
				../src/lib_ccx/ccx_encoders_common.c \
				../src/lib_ccx/ccx_encoders_common.h \
				../src/lib_ccx/ccx_encoders_curl.c \
				../src/lib_ccx/ccx_encoders_g608.c \
				../src/lib_ccx/ccx_encoders_helpers.c \
				../src/lib_ccx/ccx_encoders_helpers.h \
				../src/lib_ccx/ccx_encoders_mcc.c \
				../src/lib_ccx/ccx_encoders_mcc.h \
				../src/lib_ccx/ccx_encoders_sami.c \
				../src/lib_ccx/ccx_encoders_scc.c \
				../src/lib_ccx/ccx_encoders_smptett.c \
				../src/lib_ccx/ccx_encoders_splitbysentence.c \
				../src/lib_ccx/ccx_encoders_spupng.c \
				../src/lib_ccx/ccx_encoders_srt.c \
				../src/lib_ccx/ccx_encoders_ssa.c \
				../src/lib_ccx/ccx_encoders_structs.h \
				../src/lib_ccx/ccx_encoders_transcript.c \
				../src/lib_ccx/ccx_encoders_webvtt.c \
				../src/lib_ccx/ccx_encoders_xds.c \
				../src/lib_ccx/ccx_encoders_xds.h \
				../src/lib_ccx/ccx_gxf.c \
				../src/lib_ccx/ccx_gxf.h \
				../src/lib_ccx/ccx_mp4.h \
				../src/lib_ccx/ccx_share.c \
				../src/lib_ccx/ccx_share.h \
				../src/lib_ccx/ccx_sub_entry_message.pb-c.c \
				../src/lib_ccx/ccx_sub_entry_message.pb-c.h \
				../src/lib_ccx/compile_info.h \
				../src/lib_ccx/compile_info_real.h \
				../src/lib_ccx/configuration.c \
				../src/lib_ccx/configuration.h \
				../src/lib_ccx/disable_warnings.h \
				../src/lib_ccx/dvb_subtitle_decoder.c \
				../src/lib_ccx/dvb_subtitle_decoder.h \
				../src/lib_ccx/dvd_subtitle_decoder.c \
				../src/lib_ccx/dvd_subtitle_decoder.h \
				../src/lib_ccx/es_functions.c \
				../src/lib_ccx/es_userdata.c \
				../src/lib_ccx/ffmpeg_intgr.c \
				../src/lib_ccx/ffmpeg_intgr.h \
				../src/lib_ccx/file_buffer.h \
				../src/lib_ccx/file_functions.c \
				../src/lib_ccx/general_loop.c \
				../src/lib_ccx/hamming.h \
				../src/lib_ccx/hardsubx.c \
				../src/lib_ccx/hardsubx_classifier.c \
				../src/lib_ccx/hardsubx_decoder.c \
				../src/lib_ccx/hardsubx.h \
				../src/lib_ccx/hardsubx_imgops.c \
				../src/lib_ccx/hardsubx_utility.c \
				../src/lib_ccx/lib_ccx.c \
				../src/lib_ccx/lib_ccx.h \
				../src/lib_ccx/list.h \
				../src/lib_ccx/matroska.c \
				../src/lib_ccx/matroska.h \
				../src/lib_ccx/mp4.c \
				../src/lib_ccx/myth.c \
				../src/lib_ccx/networking.c \
				../src/lib_ccx/networking.h \
				../src/lib_ccx/ocr.c \
				../src/lib_ccx/ocr.h \
				../src/lib_ccx/output.c \
				../src/lib_ccx/params.c \
				../src/lib_ccx/params_dump.c \
				../src/lib_ccx/sequencing.c \
				../src/lib_ccx/stdintmsc.h \
				../src/lib_ccx/stream_functions.c \
				../src/lib_ccx/teletext.h \
				../src/lib_ccx/telxcc.c \
				../src/lib_ccx/ts_functions.c \
				../src/lib_ccx/ts_functions.h \
				../src/lib_ccx/ts_info.c \
				../src/lib_ccx/ts_tables.c \
				../src/lib_ccx/ts_tables_epg.c \
				../src/lib_ccx/wtv_constants.h \
				../src/lib_ccx/wtv_functions.c \
				../src/thirdparty/zlib/adler32.c \
				../src/thirdparty/zlib/compress.c \
				../src/thirdparty/zlib/crc32.c \
				../src/thirdparty/zlib/crc32.h \
				../src/thirdparty/zlib/deflate.c \
				../src/thirdparty/zlib/deflate.h \
				../src/thirdparty/zlib/gzclose.c \
				../src/thirdparty/zlib/gzguts.h \
				../src/thirdparty/zlib/gzlib.c \
				../src/thirdparty/zlib/gzread.c \
				../src/thirdparty/zlib/gzwrite.c \
				../src/thirdparty/zlib/infback.c \
				../src/thirdparty/zlib/inffast.c \
				../src/thirdparty/zlib/inffast.h \
				../src/thirdparty/zlib/inffixed.h \
				../src/thirdparty/zlib/inflate.c \
				../src/thirdparty/zlib/inflate.h \
				../src/thirdparty/zlib/inftrees.c \
				../src/thirdparty/zlib/inftrees.h \
				../src/thirdparty/zlib/trees.c \
				../src/thirdparty/zlib/trees.h \
				../src/thirdparty/zlib/uncompr.c \
				../src/thirdparty/zlib/zconf.h \
				../src/thirdparty/zlib/zlib.h \
				../src/thirdparty/zlib/zutil.c \
				../src/thirdparty/zlib/zutil.h \
				../src/thirdparty/utf8proc/utf8proc.c \
				../src/thirdparty/utf8proc/utf8proc.h \
				../src/thirdparty/lib_hash/sha2.c \
				../src/thirdparty/lib_hash/sha2.h \
				../src/thirdparty/protobuf-c/protobuf-c.c \
				../src/thirdparty/protobuf-c/protobuf-c.h \
				../src/lib_ccx/zvbi/bcd.h \
				../src/lib_ccx/zvbi/bit_slicer.c \
				../src/lib_ccx/zvbi/bit_slicer.h \
				../src/lib_ccx/zvbi/decoder.c \
				../src/lib_ccx/zvbi/macros.h \
				../src/lib_ccx/zvbi/misc.h \
				../src/lib_ccx/zvbi/raw_decoder.c \
				../src/lib_ccx/zvbi/raw_decoder.h \
				../src/lib_ccx/zvbi/sampling_par.c \
				../src/lib_ccx/zvbi/sampling_par.h \
				../src/lib_ccx/zvbi/sliced.h \
				../src/lib_ccx/zvbi/zvbi_decoder.h \
				../src/freetype/* \
				../src/thirdparty/freetype/autofit/autofit.c \
				../src/thirdparty/freetype/base/ftbase.c \
				../src/thirdparty/freetype/base/ftbbox.c \
				../src/thirdparty/freetype/base/ftbdf.c \
				../src/thirdparty/freetype/base/ftbitmap.c \
				../src/thirdparty/freetype/base/ftcid.c \
				../src/thirdparty/freetype/base/ftfntfmt.c \
				../src/thirdparty/freetype/base/ftfstype.c \
				../src/thirdparty/freetype/base/ftgasp.c \
				../src/thirdparty/freetype/base/ftglyph.c \
				../src/thirdparty/freetype/base/ftgxval.c \
				../src/thirdparty/freetype/base/ftinit.c \
				../src/thirdparty/freetype/base/ftlcdfil.c \
				../src/thirdparty/freetype/base/ftmm.c \
				../src/thirdparty/freetype/base/ftotval.c \
				../src/thirdparty/freetype/base/ftpatent.c \
				../src/thirdparty/freetype/base/ftpfr.c \
				../src/thirdparty/freetype/base/ftstroke.c \
				../src/thirdparty/freetype/base/ftsynth.c \
				../src/thirdparty/freetype/base/ftsystem.c \
				../src/thirdparty/freetype/base/fttype1.c \
				../src/thirdparty/freetype/base/ftwinfnt.c \
				../src/thirdparty/freetype/bdf/bdf.c \
				../src/thirdparty/freetype/bzip2/ftbzip2.c \
				../src/thirdparty/freetype/cache/ftcache.c \
				../src/thirdparty/freetype/cff/cff.c \
				../src/thirdparty/freetype/cid/type1cid.c \
				../src/thirdparty/freetype/gzip/ftgzip.c \
				../src/thirdparty/freetype/include/ft2build.h \
				../src/thirdparty/freetype/lzw/ftlzw.c \
				../src/thirdparty/freetype/pcf/pcf.c \
				../src/thirdparty/freetype/pfr/pfr.c \
				../src/thirdparty/freetype/psaux/psaux.c \
				../src/thirdparty/freetype/pshinter/pshinter.c \
				../src/thirdparty/freetype/psnames/psnames.c \
				../src/thirdparty/freetype/raster/raster.c \
				../src/thirdparty/freetype/sfnt/sfnt.c \
				../src/thirdparty/freetype/smooth/smooth.c \
				../src/thirdparty/freetype/truetype/truetype.c \
				../src/thirdparty/freetype/type1/type1.c \
				../src/thirdparty/freetype/type42/type42.c \
				../src/thirdparty/freetype/winfonts/winfnt.c

if SYS_IS_APPLE_SILICON
ccextractor_SOURCES += 	../src/thirdparty/libpng/arm/arm_init.c \
  						../src/thirdparty/libpng/arm/filter_neon_intrinsics.c \
  						../src/thirdparty/libpng/arm/palette_neon_intrinsics.c
endif

ccextractor_CFLAGS = -std=gnu99 -Wno-write-strings -Wno-pointer-sign -D_FILE_OFFSET_BITS=64 -DVERSION_FILE_PRESENT -DFT2_BUILD_LIBRARY -DGPAC_DISABLE_VTT -DGPAC_DISABLE_OD_DUMP -DGPAC_DISABLE_REMOTERY -DNO_GZIP -DGPAC_HAVE_CONFIG_H

ccextractor_CPPFLAGS =-I../src/lib_ccx/ -I../src/thirdparty/gpacmp4/ -I../src/thirdparty/libpng/ -I../src/thirdparty/zlib/ -I../src/lib_ccx/zvbi/ -I../src/thirdparty/lib_hash/ -I../src/thirdparty/protobuf-c/ -I../src/thirdparty -I../src/ -I../src/thirdparty/freetype/include/


ccextractor_LDADD=-lm -lpthread -ldl


if SYS_IS_LINUX
ccextractor_CFLAGS += -O3 -s -DGPAC_CONFIG_LINUX
endif

if SYS_IS_MAC
ccextractor_CFLAGS += -DPAC_CONFIG_DARWIN -Dfopen64=fopen -Dopen64=open -Dlseek64=lseek
ccextractor_LDADD += -liconv -lz
endif

if SYS_IS_64_BIT
ccextractor_CFLAGS += -DGPAC_64_BITS
endif

HARDSUBX_FEATURE_RUST=

if HARDSUBX_IS_ENABLED
ccextractor_CFLAGS += -DENABLE_HARDSUBX
ccextractor_CPPFLAGS+= ${libavcodec_CFLAGS}
ccextractor_CPPFLAGS+= ${libavformat_CFLAGS}
ccextractor_CPPFLAGS+= ${libavutil_CFALGS}
ccextractor_CPPFLAGS+= ${libswscale_CFLAGS}
AV_LIB = ${libavcodec_LIBS}
AV_LIB += ${libavformat_LIBS}
AV_LIB += ${libavutil_LIBS}
AV_LIB += ${libswscale_LIBS}
ccextractor_LDADD += $(AV_LIB)
HARDSUBX_FEATURE_RUST += --features "hardsubx_ocr"
endif

if OCR_IS_ENABLED
ccextractor_CFLAGS += -DENABLE_OCR -DPN3G_NO_CONFIG_H
LEPT_LIB = ${lept_LIBS}
LEPT_CPPFLAG = ${lept_CFLAGS}

if TESSERACT_PRESENT
TESS_LIB = ${tesseract_LIBS}
TESS_CPPFLAG = ${tesseract_CFLAGS}
else

#fix for raspberry pi not having a pkgconfig file for tesseract

if TESSERACT_PRESENT_RPI
TESS_LIB = -ltesseract
TESS_CPPFLAG = -I/usr/include/tesseract
endif
endif

ccextractor_CPPFLAGS  += $(TESS_CPPFLAG)
ccextractor_CPPFLAGS  += $(LEPT_CPPFLAG)
ccextractor_LDADD += $(TESS_LIB)
ccextractor_LDADD += $(LEPT_LIB)
endif

if WITH_RUST
ccextractor_LDADD += ./rust/@RUST_TARGET_SUBDIR@/libccx_rust.a
else
ccextractor_CFLAGS += -DDISABLE_RUST
ccextractor_CPPFLAGS += -DDISABLE_RUST
endif

if DEBUG_RELEASE
CARGO_RELEASE_ARGS=
else
CARGO_RELEASE_ARGS=--release
endif

./rust/@RUST_TARGET_SUBDIR@/libccx_rust.a:
	cd ../src/rust && \
	CARGO_TARGET_DIR=../../linux/rust $(CARGO) build $(HARDSUBX_FEATURE_RUST) $(CARGO_RELEASE_ARGS);

EXTRA_DIST = ../src/thirdparty/gpacmp4/gpac/sync_layer.h ../src/lib_ccx/ccfont2.xbm ../src/thirdparty/utf8proc/utf8proc_data.c fonts/ icon/

