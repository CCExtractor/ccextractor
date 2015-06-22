cmake_policy (SET CMP0037 NEW)

set (CMAKE_C_FLAGS "-O0 -Wall -g -std=gnu99")

if (WITH_FFMPEG)
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_FFMPEG")
endif (WITH_FFMPEG)

if (WITH_OCR)
  find_package(PkgConfig)

  pkg_check_modules(TESSERACT REQUIRED tesseract)
  pkg_check_modules(LEPTONICA REQUIRED lept)

  set (EXTRA_LIBS ${EXTRA_LIBS} ${TESSERACT_STATIC_LIBRARIES})
  set (EXTRA_LIBS ${EXTRA_LIBS} ${LEPTONICA_STATIC_LIBRARIES})
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_OCR ${TESSERACT_CFLAGS} ${LEPTONICA_CFLAGS}")
endif (WITH_OCR)

aux_source_directory ("${PROJECT_SOURCE_DIR}/lib_ccx/" SOURCEFILE)
aux_source_directory ("${PROJECT_SOURCE_DIR}/gpacmp4/" SOURCEFILE)

add_library (ccx ${SOURCEFILE})

if (MINGW OR CYGWIN)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGPAC_CONFIG_WIN32")
endif (MINGW OR CYGWIN)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGPAC_CONFIG_LINUX")
endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGPAC_CONFIG_DARWIN")
endif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")


file (GLOB HeaderFiles *.h)
file (WRITE ccx.pc "prefix=${CMAKE_INSTALL_PREFIX}\n"
                  "includedir=\${prefix}/include\n"
                  "libdir=\${prefix}/lib\n\n"
                  "Name: ccx\n"
                  "Description: Closed Caption Extraction library\n"
                  "Version: 0.75\n"
                  "Cflags: -I\${includedir}/\n"
                  "Libs: -L\${libdir} -lccx -lpng\n"
                  "Libs.private: -lpng\n"
                  )

install (TARGETS ccx DESTINATION lib)
install (FILES ${HeaderFiles} DESTINATION include)
install (FILES ccx.pc DESTINATION lib/pkgconfig)
