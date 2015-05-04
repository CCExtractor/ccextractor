cmake_policy(SET CMP0037 NEW)

SET (CMAKE_C_FLAGS "-O0 -Wall -g -std=gnu99")

if (WITH_FFMPEG)
SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_FFMPEG")
endif (WITH_FFMPEG)

AUX_SOURCE_DIRECTORY("${PROJECT_SOURCE_DIR}/lib_ccx/" SOURCEFILE)
AUX_SOURCE_DIRECTORY("${PROJECT_SOURCE_DIR}/gpacmp4/" SOURCEFILE)
#AUX_SOURCE_DIRECTORY("${PROJECT_SOURCE_DIR}/libpng/" SOURCEFILE)
add_library(ccx ${SOURCEFILE})


if (MINGW OR CYGWIN)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGPAC_CONFIG_WIN32")
endif (MINGW OR CYGWIN)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGPAC_CONFIG_LINUX")
endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGPAC_CONFIG_DARWIN")
endif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")


FILE(GLOB HeaderFiles *.h)
file(WRITE ccx.pc "prefix=${CMAKE_INSTALL_PREFIX}\n"
                  "includedir=\${prefix}/include\n"
                  "libdir=\${prefix}/lib\n\n"
                  "Name: ccx\n"
                  "Description: Closed Caption Extraction library\n"
                  "Version: 0.75\n"
                  "Cflags: -I\${includedir}/\n"
                  "Libs: -L\${libdir} -lccx -lpng\n"
                  "Libs.private: -lpng\n" )

install (TARGETS ccx DESTINATION lib)
install (FILES ${HeaderFiles} DESTINATION include)
install (FILES ccx.pc DESTINATION lib/pkgconfig )
