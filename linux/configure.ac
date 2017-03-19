#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(ccextractor, 0.85, https://github.com/CCExtractor/ccextractor/issues)
AC_CONFIG_AUX_DIR([build-conf])
AC_CONFIG_SRCDIR([../src/ccextractor.c])
AM_INIT_AUTOMAKE([foreign subdir-objects])


# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for libraries.
AC_CHECK_LIB([m], [sin], [], [AC_MSG_ERROR(Math library not installed. Install it before proceeding.)])
AC_CHECK_LIB([lept], [getLeptonicaVersion], [HAS_LEPT=1], [HAS_LEPT=0])
AC_CHECK_LIB([tesseract], [TessVersion], [HAS_TESSERACT=1], [HAS_TESSERACT=0])
AC_CHECK_LIB([avcodec], [avcodec_version], [HAS_AVCODEC=1], [HAS_AVCODEC=0])
AC_CHECK_LIB([avformat], [avformat_version], [HAS_AVFORMAT=1], [HAS_AVFORMAT=0])
AC_CHECK_LIB([avutil], [avutil_version], [HAS_AVUTIL=1], [HAS_AVUTIL=0])
AC_CHECK_LIB([swscale], [swscale_version], [HAS_SWSCALE=1], [HAS_SWSCALE=0])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h inttypes.h limits.h locale.h malloc.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h sys/socket.h sys/time.h sys/timeb.h termios.h unistd.h wchar.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FSEEKO
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_FUNC_STRERROR_R
AC_CHECK_FUNCS([floor ftruncate gethostbyname gettimeofday inet_ntoa mblen memchr memmove memset mkdir modf pow realpath rmdir select setlocale socket sqrt strcasecmp strchr strdup strerror strndup strrchr strstr strtol])

# Checks for arguments with configure
AC_ARG_ENABLE([hardsubx],
AC_HELP_STRING([--enable-hardsubx], [Enables extraction of burnt subtitles (hard subtitles)]),
[case "${enableval}" in
  yes) hardsubx=true ;;
  no)  hardsubx=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-hardsubx]) ;;
esac],[hardsubx=false])

AC_ARG_ENABLE([ocr],
AC_HELP_STRING([--enable-ocr], [Enables Optical Character Recognition]),
[case "${enableval}" in
  yes) ocr=true ;;
  no)  ocr=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-ocr]) ;;
esac],[ocr=false])

AC_ARG_ENABLE([ffmpeg],
AC_HELP_STRING([--enable-ffmpeg], [Enable FFmpeg integration]),
[case "${enableval}" in
  yes) ffmpeg=true ;;
  no)  ffmpeg=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-ffmpeg]) ;;
esac],[ffmpeg=false])


#Checks and prompts if libraries found/not found to avoild failure while building
AS_IF([ test x$hardsubx = xtrue && test $HAS_AVCODEC -gt 0 ], [AC_MSG_NOTICE(checking for avcodec presence... yes)])
AS_IF([ test x$hardsubx = xtrue && test ! $HAS_AVCODEC -gt 0 ], [AC_MSG_ERROR(checking for avcodec presence... no. Install avcodec library, required to proceed configuration.)])
AS_IF([ test x$hardsubx = xtrue && test $HAS_AVFORMAT -gt 0 ], [AC_MSG_NOTICE(checking for avformat presence.. yes)])
AS_IF([ test x$hardsubx = xtrue && test ! $HAS_AVFORMAT -gt 0 ], [AC_MSG_ERROR(checking for avformat presence.. no. Install avformat library, required to proceed configuration.)])
AS_IF([ test x$hardsubx = xtrue && test $HAS_AVUTIL -gt 0 ], [AC_MSG_NOTICE(checking for avutil presence... yes)])
AS_IF([ test x$hardsubx = xtrue && test ! $HAS_AVUTIL -gt 0 ], [AC_MSG_ERROR(checking for avutil presence... no. Please install the avutil library before proceeding)])
AS_IF([ test x$hardsubx = xtrue && test $HAS_SWSCALE -gt 0 ], [AC_MSG_NOTICE(checking for swscale presence... yes)])
AS_IF([ test x$hardsubx = xtrue && test ! $HAS_SWSCALE -gt 0 ], [AC_MSG_ERROR(checking for swscale presence... no. Install swscale library, required to proceed configuration.)])
AS_IF([ (test x$ocr = xtrue || test x$hardsubx = xtrue) && test $HAS_TESSERACT -gt 0 ], [TESS_VER=`tesseract --version 2>&1 | grep tesseract` && AC_MSG_NOTICE(checking for tesseract...yes $TESS_VER)])
AS_IF([ (test x$ocr = xtrue || test x$hardsubx = xtrue) && test ! $HAS_TESSERACT -gt 0 ], [AC_MSG_ERROR(checking for tesseract... no. Install tesseract library, required to proceed configuration.)])
AS_IF([ (test x$ocr = xtrue || test x$hardsubx = xtrue) && test $HAS_LEPT -gt 0 ], [LEPT_VER=`tesseract --version 2>&1 | grep leptonica` && AC_MSG_NOTICE(checking for leptonica... yes $LEPT_VER)])
AS_IF([ (test x$ocr = xtrue || test x$hardsubx = xtrue) && test ! $HAS_LEPT -gt 0 ], [AC_MSG_ERROR(checking for leptonica... no. Install leptonica library, required to proceed configuration.)])

#AM_CONDITIONAL(s) for setting values to enable/disable flags in Makefile.am
AM_CONDITIONAL(HARDSUBX_IS_ENABLED, [ test x$hardsubx = xtrue ])
AM_CONDITIONAL(OCR_IS_ENABLED, [ test x$ocr = xtrue || test x$hardsubx = xtrue ])
AM_CONDITIONAL(FFMPEG_IS_ENABLED, [ test x$ffmpeg = xtrue ])
AM_CONDITIONAL(TESSERACT_PRESENT, [ test ! -z  `pkg-config --libs --silence-errors tesseract` ])
AM_CONDITIONAL(TESSERACT_PRESENT_RPI, [ test -d "/usr/include/tesseract" && test `ls -A /usr/include/tesseract | wc -l` -gt 0 ])
AM_CONDITIONAL(SYS_IS_LINUX, [ string=`gcc -dumpmachine` && test -z "${string##*$linux*}" ])


AC_CONFIG_FILES([Makefile])
AC_OUTPUT
