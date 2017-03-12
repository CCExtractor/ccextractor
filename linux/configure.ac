AC_INIT([CCExtractor], [0.85], [carlos@ccextractor.org])
AM_INIT_AUTOMAKE([foreign])

AC_CONFIG_SRCDIR([../src/ccextractor.c])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

AC_CHECK_LIB([m], [cos], [], [echo "math library not found. Please install the math library before proceeding"; exit -1])
AC_CHECK_LIB([tesseract], [TessVersion], [HAS_TESSERACT=1], [HAS_TESSERACT=0])
AC_CHECK_LIB([lept], [getLeptonicaVersion], [HAS_LEPT=1], [HAS_LEPT=0])
AC_CHECK_LIB([avcodec], [avcodec_version], [HAS_AVCODEC=1], [HAS_AVCODEC=0])
AC_CHECK_LIB([avformat], [avformat_version], [HAS_AVFORMAT=1], [HAS_AVFORMAT=0])
AC_CHECK_LIB([avutil], [avutil_version], [HAS_AVUTIL=1], [HAS_AVUTIL=0])
AC_CHECK_LIB([swscale], [swscale_version], [HAS_SWSCALE=1], [HAS_SWSCALE=0])

AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h inttypes.h limits.h locale.h malloc.h netdb.h netinet/in.h stddef.h stdint.h stdlib.h string.h sys/socket.h sys/time.h sys/timeb.h termios.h unistd.h wchar.h])

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

AC_FUNC_ERROR_AT_LINE
AC_FUNC_FSEEKO
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_FUNC_STRERROR_R
AC_CHECK_FUNCS([floor ftruncate gethostbyname gettimeofday inet_ntoa mblen memchr memmove memset mkdir modf pow realpath rmdir select setlocale socket sqrt strcasecmp strchr strdup strerror strndup strrchr strstr strtol])

AC_ARG_ENABLE([hardsubx],
[  --enable-hardsubx    Enable extraction of burned in (hard) subtitles],
[case "${enableval}" in
  yes) hardsubx=true ;;
  no)  hardsubx=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-hardsubx]) ;;
esac],[hardsubx=false])
AC_ARG_ENABLE([ocr],
[  --enable-ocr    Enable OCR],
[case "${enableval}" in
  yes) ocr=true ;;
  no)  ocr=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-ocr]) ;;
esac],[ocr=false])
AC_ARG_ENABLE([ffmpeg],
[  --enable-ffmpeg    Enable FFmpeg integration],
[case "${enableval}" in
  yes) ffmpeg=true ;;
  no)  ffmpeg=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-ffmpeg]) ;;
esac],[ffmpeg=false])

AS_IF([ test x$hardsubx = xtrue && test $HAS_AVCODEC -gt 0 ], [echo "avcodec library found..."])
AS_IF([ test x$hardsubx = xtrue && test ! $HAS_AVCODEC -gt 0 ], [echo "avcodec library not found. Please install the avcodec library before proceeding"; exit -1])
AS_IF([ test x$hardsubx = xtrue && test $HAS_AVFORMAT -gt 0 ], [echo "avformat library found..."])
AS_IF([ test x$hardsubx = xtrue && test ! $HAS_AVFORMAT -gt 0 ], [echo "avformat library not found. Please install the avformat library before proceeding"; exit -1])
AS_IF([ test x$hardsubx = xtrue && test $HAS_AVUTIL -gt 0 ], [echo "avutil library found..."])
AS_IF([ test x$hardsubx = xtrue && test ! $HAS_AVUTIL -gt 0 ], [echo "avutil library not found. Please install the avutil library before proceeding"; exit -1])
AS_IF([ test x$hardsubx = xtrue && test $HAS_SWSCALE -gt 0 ], [echo "swscale library found..."])
AS_IF([ test x$hardsubx = xtrue && test ! $HAS_SWSCALE -gt 0 ], [echo "swscale library not found. Please install the swscale library before proceeding"; exit -1])
AS_IF([ (test x$ocr = xtrue || test x$hardsubx = xtrue) && test $HAS_TESSERACT -gt 0 ], [echo "tesseract library found... "`tesseract --version | grep tesseract`])
AS_IF([ (test x$ocr = xtrue || test x$hardsubx = xtrue) && test ! $HAS_TESSERACT -gt 0 ], [echo "tesseract library not found. Please install the tesseract library before proceeding"; exit -1])
AS_IF([ (test x$ocr = xtrue || test x$hardsubx = xtrue) && test $HAS_LEPT -gt 0 ], [echo "leptonica library found... "`tesseract --version | grep leptonica`])
AS_IF([ (test x$ocr = xtrue || test x$hardsubx = xtrue) && test ! $HAS_LEPT -gt 0 ], [echo "leptonica library not found. Please install the leptonica library before proceeding"; exit -1])

AM_CONDITIONAL(SYS_IS_LINUX, [ string=`gcc -dumpmachine` \
                                    && test -z "${string##*$linux*}" ])
AM_CONDITIONAL(HARDSUBX_IS_ENABLED, [ test x$hardsubx = xtrue ])
AM_CONDITIONAL(TESSERACT_PRESENT, [ test ! -z  `pkg-config --libs --silence-errors tesseract` ])
AM_CONDITIONAL(TESSERACT_PRESENT_RPI, [ test -d "/usr/include/tesseract" && test `ls -A /usr/include/tesseract | wc -l` -gt 0 ])
AM_CONDITIONAL(OCR_IS_ENABLED, [ test x$ocr = xtrue || test x$hardsubx = xtrue ])
AM_CONDITIONAL(FFMPEG_IS_ENABLED, [ test x$ffmpeg = xtrue ])

AC_OUTPUT([Makefile])
