cmake_minimum_required (VERSION 3.0.2)

project (CCExtractor)

option(WITH_FFMPEG "Build using FFmpeg demuxer and decoder" OFF)
#Version number
set (CCEXTRACTOR_VERSION_MAJOR 0)
set (CCEXTRACTOR_VERSION_MINOR 75)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/CCExtractorConfig.h.in"
  "${PROJECT_BINARY_DIR}/CCExtractorConfig.h"
  )


include_directories ("${PROJECT_SOURCE_DIR}")
include_directories ("${PROJECT_SOURCE_DIR}/lib_ccx")
include_directories ("${PROJECT_SOURCE_DIR}/gpacmp4/")
include_directories ("${PROJECT_SOURCE_DIR}/libccx_common/")

#Adding some platform specific library path
LINK_DIRECTORIES(/opt/local/lib)
LINK_DIRECTORIES(/usr/local/lib)

SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -Wall -g -std=gnu99 -Wno-write-strings -D_FILE_OFFSET_BITS=64")
add_subdirectory (lib_ccx)

AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR} SOURCEFILE)
set (EXTRA_LIBS ${EXTRA_LIBS} ccx)
set (EXTRA_LIBS ${EXTRA_LIBS} png)
set (EXTRA_LIBS ${EXTRA_LIBS} m)

########################################################
# Build using FFmpeg libraries
#
if (WITH_FFMPEG)

find_package(PkgConfig)
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(AVUTIL REQUIRED libavutil)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
set (EXTRA_LIBS ${EXTRA_LIBS} ${AVFORMAT_STATIC_LIBRARIES} )
set (EXTRA_LIBS ${EXTRA_LIBS} ${AVUTIL_STATIC_LIBRARIES} )
set (EXTRA_LIBS ${EXTRA_LIBS} ${AVCODEC_STATIC_LIBRARIES} )

SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_FFMPEG")
endif (WITH_FFMPEG)

add_executable(ccextractor ${SOURCEFILE})
target_link_libraries (ccextractor ${EXTRA_LIBS})

install (TARGETS ccextractor DESTINATION bin)
