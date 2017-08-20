cmake_minimum_required (VERSION 3.0.2)

project (CCExtractor)

option (WITH_FFMPEG "Build using FFmpeg demuxer and decoder" OFF)
option (WITH_OCR "Build with OCR (Optical Character Recognition) feature" OFF)
option (WITH_SHARING "Build with sharing and translation support" OFF)

# Version number
set (CCEXTRACTOR_VERSION_MAJOR 0)
set (CCEXTRACTOR_VERSION_MINOR 85b)

# Get project directory
get_filename_component(BASE_PROJ_DIR ../ ABSOLUTE)

# Get the latest commit hash of the working branch
IF(EXISTS "${BASE_PROJ_DIR}/.git")
    execute_process(
            COMMAND git rev-parse HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
ELSE(EXISTS "${BASE_PROJ_DIR}/.git")
    set(GIT_BRANCH "Unknown")
    set(GIT_COMMIT_HASH "Unknown")
ENDIF(EXISTS "${BASE_PROJ_DIR}/.git")

#Get the date
string(TIMESTAMP COMPILATION_DATE %Y-%m-%d)

message(STATUS "Commit : ${GIT_COMMIT_HASH}")
message(STATUS "Compilation Date: ${COMPILATION_DATE}")

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/CCExtractorConfig.h.in"
  "${PROJECT_SOURCE_DIR}/lib_ccx/compile_info_real.h"
  )

add_definitions(-DVERSION_FILE_PRESENT)

include_directories ("${PROJECT_SOURCE_DIR}")
include_directories ("${PROJECT_SOURCE_DIR}/lib_ccx/")
include_directories ("${PROJECT_SOURCE_DIR}/utf8proc/")
include_directories ("${PROJECT_SOURCE_DIR}/gpacmp4/")
include_directories ("${PROJECT_SOURCE_DIR}/protobuf-c/")
include_directories ("${PROJECT_SOURCE_DIR}/zvbi/")
include_directories ("${PROJECT_SOURCE_DIR}/lib_hash/")
aux_source_directory ("${PROJECT_SOURCE_DIR}/protobuf-c/" SOURCEFILE)
aux_source_directory ("${PROJECT_SOURCE_DIR}/zvbi/" SOURCEFILE)
aux_source_directory ("${PROJECT_SOURCE_DIR}/lib_hash/" SOURCEFILE)

#Windows specific libraries and linker flags
if(WIN32)
    include_directories ("${PROJECT_SOURCE_DIR}/win_iconv/")
    include_directories ("${PROJECT_SOURCE_DIR}/libpng/")
    aux_source_directory ("${PROJECT_SOURCE_DIR}/win_iconv/" SOURCEFILE)
    aux_source_directory ("${PROJECT_SOURCE_DIR}/libpng/" SOURCEFILE)
    set (EXTRA_LIBS ${EXTRA_LIBS} -lws2_32 -lwinmm)
else (WIN32)
    # Adding some platform specific library path
    link_directories (/opt/local/lib)
    link_directories (/usr/local/lib)
endif(WIN32)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -Wall -g -std=gnu99 -Wno-write-strings -D_FILE_OFFSET_BITS=64")
add_subdirectory (lib_ccx)

aux_source_directory (${PROJECT_SOURCE_DIR} SOURCEFILE)
set (EXTRA_LIBS ${EXTRA_LIBS} ccx)
set (EXTRA_LIBS ${EXTRA_LIBS} m)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set (EXTRA_LIBS ${EXTRA_LIBS} iconv)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")


find_package (PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PNG libpng )
  if(PNG_FOUND)
    set (EXTRA_LIBS ${EXTRA_LIBS} png)
  else (PNG_FOUND)
    include_directories ("${PROJECT_SOURCE_DIR}/libpng/")
    include_directories ("${PROJEXT_SOURCE_DIR}/zlib/")
    aux_source_directory ("${PROJECT_SOURCE_DIR}/libpng/" SOURCEFILE)
    aux_source_directory ("${PROJECT_SOURCE_DIR}/zlib/" SOURCEFILE)
  endif(PNG_FOUND)
else(PKG_CONFIG_FOUND)
    include_directories ("${PROJECT_SOURCE_DIR}/libpng/")
    include_directories ("${PROJEXT_SOURCE_DIR}/zlib/")
    aux_source_directory ("${PROJECT_SOURCE_DIR}/libpng/" SOURCEFILE)
    aux_source_directory ("${PROJECT_SOURCE_DIR}/zlib/" SOURCEFILE)
endif (PKG_CONFIG_FOUND)

set (EXTRA_LIBS ${EXTRA_LIBS} -lz)

########################################################
# Build using FFmpeg libraries
########################################################

if (PKG_CONFIG_FOUND AND WITH_FFMPEG)

  pkg_check_modules (AVFORMAT REQUIRED libavformat)
  pkg_check_modules (AVUTIL REQUIRED libavutil)
  pkg_check_modules (AVCODEC REQUIRED libavcodec)
  pkg_check_modules (SWSCALE REQUIRED libswscale)

  set (EXTRA_LIBS ${EXTRA_LIBS} ${AVFORMAT_LIBRARIES})
  set (EXTRA_LIBS ${EXTRA_LIBS} ${AVUTIL_LIBRARIES})
  set (EXTRA_LIBS ${EXTRA_LIBS} ${AVCODEC_LIBRARIES})
  set (EXTRA_LIBS ${EXTRA_LIBS} ${SWSCALE_LIBRARIES})

  set (EXTRA_INCLUDES ${EXTRA_INCLUDES} ${AVFORMAT_INCLUDE_DIRS})
  set (EXTRA_INCLUDES ${EXTRA_INCLUDES} ${AVUTIL_INCLUDE_DIRS})
  set (EXTRA_INCLUDES ${EXTRA_INCLUDES} ${AVCODEC_INCLUDE_DIRS})
  set (EXTRA_INCLUDES ${EXTRA_INCLUDES} ${SWSCALE_INCLUDE_DIRS})

  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_FFMPEG")
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_HARDSUBX")
endif (PKG_CONFIG_FOUND AND WITH_FFMPEG)

########################################################
# Build with OCR using leptonica and tesseract libraries
########################################################

if (WITH_OCR)
  find_package(PkgConfig)

  pkg_check_modules (TESSERACT REQUIRED tesseract)
  pkg_check_modules (LEPTONICA REQUIRED lept)

  set (EXTRA_LIBS ${EXTRA_LIBS} ${TESSERACT_LIBRARIES})
  set (EXTRA_LIBS ${EXTRA_LIBS} ${LEPTONICA_LIBRARIES})

  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_OCR")
endif (WITH_OCR)

########################################################
# Build with CC sharing and translation support
########################################################

if (WITH_SHARING)
  find_package(PkgConfig)

  pkg_check_modules (NANOMSG REQUIRED libnanomsg)
  set (EXTRA_LIBS ${EXTRA_LIBS} ${NANOMSG_STATIC_LIBRARIES})

  include_directories ("${PROJECT_SOURCE_DIR}/protobuf-c/")
  aux_source_directory ("${PROJECT_SOURCE_DIR}/protobuf-c/" SOURCEFILE)

  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_SHARING")
endif (WITH_SHARING)

add_executable (ccextractor ${SOURCEFILE})
target_link_libraries (ccextractor ${EXTRA_LIBS})
target_include_directories (ccextractor PUBLIC ${EXTRA_INCLUDES})

install (TARGETS ccextractor DESTINATION bin)
