ccextractor
===========

CCExtractor - Carlos' version (mainstream).

GSOC - 0.72
-----------
- Fix for WTV files with incorrect timing
- Added support for fps change using data from AVC video track in a H264 TS file.

GSOC - 0.71
-----------
- Added feature to receive captions in BIN format according to CCExtractor's own
  protocol over TCP (-tcp port [-tcppassword password])
- Added ability to send captions to the server described above or to the
  online repository (-sendto host[:port])
- Added -stdin parameter for reading input stream from standard input
- Compilation in Cygwin using linux/Makefile
- Code tested with coverity for defect density 0.42
- Correction of mp4 timing, when one timestamp points timing of two atom
