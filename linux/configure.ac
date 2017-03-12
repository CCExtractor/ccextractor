AC_INIT([CCExtractor], [0.85], [carlos@ccextractor.org])
AM_INIT_AUTOMAKE([foreign])

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
AM_CONDITIONAL(SYS_IS_LINUX, [ string=`gcc -dumpmachine` \
                                    && test -z "${string##*$linux*}" ])
AM_CONDITIONAL(AVFLAGS_ARE_EMPTY, [ test ! -z  `pkg-config --libs --silence-errors libavcodec` \
                            || test ! -z  `pkg-config --libs --silence-errors libavformat` \
                            || test ! -z `pkg-config --libs --silence-errors libavutil` \
                            || test ! -z `pkg-config --libs --silence-errors libswscale` ])
AM_CONDITIONAL(HARDSUBX_IS_ENABLED, [ test x$hardsubx = xtrue ])
AM_CONDITIONAL(TESSERACT_PRESENT, [ test ! -z  `pkg-config --libs --silence-errors tesseract` ])
AM_CONDITIONAL(TESSERACT_PRESENT_RPI, [ test -d "/usr/include/tesseract" && test `ls -A /usr/include/tesseract | wc -l` -gt 0 ])
AM_CONDITIONAL(LEPTONICA_ABSENT, [ test -z  `pkg-config --libs --silence-errors lept` ])
AM_CONDITIONAL(OCR_IS_ENABLED, [ test x$ocr = xtrue ])
AM_CONDITIONAL(FFMPEG_IS_ENABLED, [ test x$ffmpeg = xtrue ])

AC_OUTPUT([Makefile])
