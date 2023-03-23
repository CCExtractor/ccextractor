cmake_minimum_required (VERSION 3.0.2)

project (CCExtractor)

option (WITH_FFMPEG "Build using FFmpeg demuxer and decoder" OFF)
option (WITH_OCR "Build with OCR (Optical Character Recognition) feature" OFF)
option (WITH_SHARING "Build with sharing and translation support" OFF)
option (WITH_HARDSUBX "Build with support for burned-in subtitles" OFF)
option (WITHOUT_RUST "Build without Rust library" OFF)

# Version number
set (CCEXTRACTOR_VERSION_MAJOR 0)
set (CCEXTRACTOR_VERSION_MINOR 89)

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

add_definitions(-DVERSION_FILE_PRESENT -DFT2_BUILD_LIBRARY -DGPAC_DISABLE_VTT -DGPAC_DISABLE_OD_DUMP -DGPAC_DISABLE_REMOTERY -DNO_GZIP -DGPAC_HAVE_CONFIG_H)

if(UNIX)
  if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_definitions(-DGPAC_CONFIG_DARWIN)
  else ()
    add_definitions(-DGPAC_CONFIG_LINUX)
  endif()
endif(UNIX)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  add_definitions(-DGPAC_64_BITS)
endif()

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/lib_ccx)
include_directories(${PROJECT_SOURCE_DIR}/lib_ccx/zvbi)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty/gpacmp4)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty/protobuf-c)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty/lib_hash)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty/libpng)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND ${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "arm64")
  include_directories(${PROJECT_SOURCE_DIR}/thirdparty/libpng/arm)
  aux_source_directory(${PROJECT_SOURCE_DIR}/thirdparty/libpng/arm SOURCEFILE)
endif ()

include_directories(${PROJECT_SOURCE_DIR}/thirdparty/zlib)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty/freetype/include)
aux_source_directory(${PROJECT_SOURCE_DIR}/thirdparty/gpacmp4/ SOURCEFILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/thirdparty/lib_hash/ SOURCEFILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/thirdparty/libpng/ SOURCEFILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/thirdparty/protobuf-c/ SOURCEFILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/thirdparty/zlib/ SOURCEFILE)
aux_source_directory(${PROJECT_SOURCE_DIR}/lib_ccx/zvbi/ SOURCEFILE)

set(UTF8PROC_SOURCE ${PROJECT_SOURCE_DIR}/thirdparty/utf8proc/utf8proc.c)

set(FREETYPE_SOURCE
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/autofit/autofit.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/base/ftbase.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/base/ftbbox.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/base/ftbdf.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/base/ftbitmap.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/base/ftcid.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/base/ftfntfmt.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/base/ftfstype.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/base/ftgasp.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/base/ftglyph.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/base/ftgxval.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/base/ftinit.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/base/ftlcdfil.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/base/ftmm.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/base/ftotval.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/base/ftpatent.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/base/ftpfr.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/base/ftstroke.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/base/ftsynth.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/base/ftsystem.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/base/fttype1.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/base/ftwinfnt.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/bdf/bdf.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/bzip2/ftbzip2.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/cache/ftcache.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/cff/cff.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/cid/type1cid.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/gzip/ftgzip.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/lzw/ftlzw.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/pcf/pcf.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/pfr/pfr.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/psaux/psaux.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/pshinter/pshinter.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/psnames/psnames.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/raster/raster.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/sfnt/sfnt.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/smooth/smooth.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/truetype/truetype.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/type1/type1.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/type42/type42.c
        ${PROJECT_SOURCE_DIR}/thirdparty/freetype/winfonts/winfnt.c
        )
#Windows specific libraries and linker flags
if(WIN32)
    include_directories ("${PROJECT_SOURCE_DIR}/thirdparty/win_spec_incld/")
    include_directories ("${PROJECT_SOURCE_DIR}/thirdparty/win_iconv/")
    aux_source_directory ("${PROJECT_SOURCE_DIR}/thirdparty/win_iconv/" SOURCEFILE)
    set (EXTRA_LIBS ${EXTRA_LIBS} ws2_32 winmm Bcrypt)
else (WIN32)
    # Adding some platform specific library path
    link_directories (/opt/local/lib)
    link_directories (/usr/local/lib)
endif(WIN32)

if(MSVC)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W3 /wd4005 /wd4996")
else (MSVC)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-write-strings -g -std=gnu99")
endif(MSVC)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FILE_OFFSET_BITS=64")
add_subdirectory (lib_ccx)

aux_source_directory(${PROJECT_SOURCE_DIR} SOURCEFILE)
set (EXTRA_LIBS ${EXTRA_LIBS} ccx)
# set (EXTRA_LIBS ${EXTRA_LIBS} m)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set (EXTRA_LIBS ${EXTRA_LIBS} iconv)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")


set (EXTRA_LIBS ${EXTRA_LIBS} -lm -lpthread -ldl)

find_package (PkgConfig)

########################################################
# Build using FFmpeg libraries
########################################################

if (PKG_CONFIG_FOUND AND WITH_FFMPEG)

  pkg_check_modules (AVFORMAT REQUIRED libavformat)
  pkg_check_modules (AVUTIL REQUIRED libavutil)
  pkg_check_modules (AVCODEC REQUIRED libavcodec)
  pkg_check_modules (AVFILTER REQUIRED libavfilter)
  pkg_check_modules (SWSCALE REQUIRED libswscale)

  set (EXTRA_LIBS ${EXTRA_LIBS} ${AVFORMAT_LIBRARIES})
  set (EXTRA_LIBS ${EXTRA_LIBS} ${AVUTIL_LIBRARIES})
  set (EXTRA_LIBS ${EXTRA_LIBS} ${AVCODEC_LIBRARIES})
  set (EXTRA_LIBS ${EXTRA_LIBS} ${AVFILTER_LIBRARIES})
  set (EXTRA_LIBS ${EXTRA_LIBS} ${SWSCALE_LIBRARIES})

  set (EXTRA_INCLUDES ${EXTRA_INCLUDES} ${AVFORMAT_INCLUDE_DIRS})
  set (EXTRA_INCLUDES ${EXTRA_INCLUDES} ${AVUTIL_INCLUDE_DIRS})
  set (EXTRA_INCLUDES ${EXTRA_INCLUDES} ${AVCODEC_INCLUDE_DIRS})
  set (EXTRA_INCLUDES ${EXTRA_INCLUDES} ${AVFILTER_INCLUDE_DIRS})
  set (EXTRA_INCLUDES ${EXTRA_INCLUDES} ${SWSCALE_INCLUDE_DIRS})

  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_FFMPEG")
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_HARDSUBX")
endif (PKG_CONFIG_FOUND AND WITH_FFMPEG)

########################################################
# Build with OCR using leptonica and tesseract libraries
########################################################

if (PKG_CONFIG_FOUND AND WITH_OCR)

  pkg_check_modules (TESSERACT REQUIRED tesseract)
  pkg_check_modules (LEPTONICA REQUIRED lept)

  set (EXTRA_LIBS ${EXTRA_LIBS} ${TESSERACT_LIBRARIES})
  set (EXTRA_LIBS ${EXTRA_LIBS} ${LEPTONICA_LIBRARIES})

  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_OCR")
endif (PKG_CONFIG_FOUND AND WITH_OCR)

########################################################
# Build with CC sharing and translation support
########################################################

if (PKG_CONFIG_FOUND AND WITH_SHARING)

  pkg_check_modules (NANOMSG REQUIRED libnanomsg)
  set (EXTRA_LIBS ${EXTRA_LIBS} ${NANOMSG_STATIC_LIBRARIES})

  include_directories ("${PROJECT_SOURCE_DIR}/thirdparty/protobuf-c/")
  aux_source_directory ("${PROJECT_SOURCE_DIR}/thirdparty/protobuf-c/" SOURCEFILE)

  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_SHARING")
endif (PKG_CONFIG_FOUND AND WITH_SHARING)

########################################################
# Build for hardsubx using avformat, avutil, avcodec and
# swscale
########################################################

if (PKG_CONFIG_FOUND AND WITH_HARDSUBX)

  pkg_check_modules (AVFORMAT REQUIRED libavformat)
  pkg_check_modules (AVUTIL REQUIRED libavutil)
  pkg_check_modules (AVCODEC REQUIRED libavcodec)
  pkg_check_modules (AVFILTER REQUIRED libavfilter)
  pkg_check_modules (SWSCALE REQUIRED libswscale)

  set (EXTRA_LIBS ${EXTRA_LIBS} ${AVFORMAT_LIBRARIES})
  set (EXTRA_LIBS ${EXTRA_LIBS} ${AVUTIL_LIBRARIES})
  set (EXTRA_LIBS ${EXTRA_LIBS} ${AVCODEC_LIBRARIES})
  set (EXTRA_LIBS ${EXTRA_LIBS} ${AVFILTER_LIBRARIES})
  set (EXTRA_LIBS ${EXTRA_LIBS} ${SWSCALE_LIBRARIES})

  set (EXTRA_INCLUDES ${EXTRA_INCLUDES} ${AVFORMAT_INCLUDE_DIRS})
  set (EXTRA_INCLUDES ${EXTRA_INCLUDES} ${AVUTIL_INCLUDE_DIRS})
  set (EXTRA_INCLUDES ${EXTRA_INCLUDES} ${AVCODEC_INCLUDE_DIRS})
  set (EXTRA_INCLUDES ${EXTRA_INCLUDES} ${AVFILTER_INCLUDE_DIRS})
  set (EXTRA_INCLUDES ${EXTRA_INCLUDES} ${SWSCALE_INCLUDE_DIRS})

  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_HARDSUBX")
endif (PKG_CONFIG_FOUND AND WITH_HARDSUBX)

add_executable (ccextractor ${SOURCEFILE} ${FREETYPE_SOURCE} ${UTF8PROC_SOURCE})

########################################################
# Build with Rust library
########################################################

if (PKG_CONFIG_FOUND AND NOT WITHOUT_RUST)
  add_subdirectory (rust)
  get_target_property(RUST_LIB ccx_rust LOCATION)
  set (EXTRA_LIBS ${EXTRA_LIBS} ${RUST_LIB})
  add_dependencies(ccextractor ccx_rust)
else ()
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DDISABLE_RUST")
endif (PKG_CONFIG_FOUND AND NOT WITHOUT_RUST)


target_link_libraries (ccextractor ${EXTRA_LIBS})
target_include_directories (ccextractor PUBLIC ${EXTRA_INCLUDES})

install (TARGETS ccextractor DESTINATION bin)
