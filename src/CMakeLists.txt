cmake_minimum_required (VERSION 3.0.2)

project (CCExtractor)

#Version number
set (CCEXTRACTOR_VERSION_MAJOR 0)
set (CCEXTRACTOR_VERSION_MINOR 74)

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

SET (CMAKE_C_FLAGS "-O3 -Wall -std=gnu99 -Wno-write-strings -D_FILE_OFFSET_BITS=64")
add_subdirectory (lib_ccx)

AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR} SOURCEFILE)
set (EXTRA_LIBS ${EXTRA_LIBS} ccx)
set (EXTRA_LIBS ${EXTRA_LIBS} png)
set (EXTRA_LIBS ${EXTRA_LIBS} m)

add_executable(ccextractor ${SOURCEFILE})
target_link_libraries (ccextractor ${EXTRA_LIBS})
