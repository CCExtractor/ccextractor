# Install script for directory: /Users/Gurpreet/Desktop/CCextractor/src/lib_ccx

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

if("${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/Users/Gurpreet/Desktop/CCextractor/src/cmake-build-debug/lib_ccx/libccx.a")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libccx.a" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libccx.a")
    execute_process(COMMAND "/Library/Developer/CommandLineTools/usr/bin/ranlib" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libccx.a")
  endif()
endif()

if("${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include" TYPE FILE FILES
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/activity.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/asf_constants.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/avc_functions.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/bitstream.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_common_char_encoding.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_common_common.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_common_constants.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_common_option.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_common_platform.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_common_structs.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_common_timing.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_decoders_608.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_decoders_708.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_decoders_708_encoding.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_decoders_708_output.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_decoders_common.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_decoders_isdb.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_decoders_structs.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_decoders_vbi.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_decoders_xds.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_demuxer.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_dtvcc.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_encoders_common.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_encoders_helpers.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_encoders_spupng.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_encoders_structs.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_encoders_xds.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_gxf.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_mp4.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_share.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx_sub_entry_message.pb-c.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/compile_info.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/configuration.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/disable_warnings.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/dvb_subtitle_decoder.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/dvd_subtitle_decoder.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ffmpeg_intgr.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/file_buffer.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/hamming.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/hardsubx.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/lib_ccx.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/list.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/networking.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ocr.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/spupng_encoder.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/stdintmsc.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/teletext.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ts_functions.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/utility.h"
    "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/wtv_constants.h"
    )
endif()

if("${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "/Users/Gurpreet/Desktop/CCextractor/src/lib_ccx/ccx.pc")
endif()

