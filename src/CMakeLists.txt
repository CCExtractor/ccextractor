cmake_minimum_required (VERSION 3.0.2)

project (CCExtractor)

option (WITH_FFMPEG "Build using FFmpeg demuxer and decoder" OFF)
option (WITH_OCR "Build with OCR (Optical Character Recognition) feature" OFF)

#Version number
set (CCEXTRACTOR_VERSION_MAJOR 0)
set (CCEXTRACTOR_VERSION_MINOR 76)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/CCExtractorConfig.h.in"
  "${PROJECT_BINARY_DIR}/CCExtractorConfig.h"
  )


include_directories ("${PROJECT_SOURCE_DIR}")
include_directories ("${PROJECT_SOURCE_DIR}/lib_ccx")
include_directories ("${PROJECT_SOURCE_DIR}/gpacmp4/")

#Adding some platform specific library path
link_directories (/opt/local/lib)
link_directories (/usr/local/lib)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -Wall -g -std=gnu99 -Wno-write-strings -D_FILE_OFFSET_BITS=64")
add_subdirectory (lib_ccx)

aux_source_directory (${PROJECT_SOURCE_DIR} SOURCEFILE)
set (EXTRA_LIBS ${EXTRA_LIBS} ccx)
set (EXTRA_LIBS ${EXTRA_LIBS} m)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
set (EXTRA_LIBS ${EXTRA_LIBS} iconv)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")


find_package (PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PNG libpng )
  if(PNG_FOUND)
    set (EXTRA_LIBS ${EXTRA_LIBS} png)
  else (PNG_FOUND)
    include_directories ("${PROJECT_SOURCE_DIR}/libpng/")
    aux_source_directory ("${PROJECT_SOURCE_DIR}/libpng/" SOURCEFILE)
    aux_source_directory ("${PROJECT_SOURCE_DIR}/zlib/" SOURCEFILE)
  endif(PNG_FOUND)
endif (PKG_CONFIG_FOUND)

########################################################
# Build using FFmpeg libraries
########################################################

if (PKG_CONFIG_FOUND AND WITH_FFMPEG)

  pkg_check_modules (AVFORMAT REQUIRED libavformat)
  pkg_check_modules (AVUTIL REQUIRED libavutil)
  pkg_check_modules (AVCODEC REQUIRED libavcodec)

  set (EXTRA_LIBS ${EXTRA_LIBS} ${AVFORMAT_STATIC_LIBRARIES})
  set (EXTRA_LIBS ${EXTRA_LIBS} ${AVUTIL_STATIC_LIBRARIES})
  set (EXTRA_LIBS ${EXTRA_LIBS} ${AVCODEC_STATIC_LIBRARIES})

  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_FFMPEG")
endif (PKG_CONFIG_FOUND AND WITH_FFMPEG)

########################################################
# Build with OCR using leptonica and tesseract libraries
########################################################

if (WITH_OCR)
  find_package(PkgConfig)

  pkg_check_modules (TESSERACT REQUIRED tesseract)
  pkg_check_modules (LEPTONICA REQUIRED lept)

  set (EXTRA_LIBS ${EXTRA_LIBS} ${TESSERACT_STATIC_LIBRARIES})
  set (EXTRA_LIBS ${EXTRA_LIBS} ${LEPTONICA_STATIC_LIBRARIES})

  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_OCR ${TESSERACT_CFLAGS} ${LEPTONICA_CFLAGS}")
endif (WITH_OCR)

add_executable (ccextractor ${SOURCEFILE})
target_link_libraries (ccextractor ${EXTRA_LIBS})

install (TARGETS ccextractor DESTINATION bin)
