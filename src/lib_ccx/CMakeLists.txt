cmake_policy (SET CMP0037 NEW)

if(MSVC)
    set (CMAKE_C_FLAGS "-W3 /wd4005 /wd4996")
else (MSVC)
    set (CMAKE_C_FLAGS "-Wall -Wno-pointer-sign -g -std=gnu99")
endif(MSVC)

if(WIN32)
    add_definitions(-DWIN32)
endif(WIN32)

if (WITH_FFMPEG)
    find_package(PkgConfig)

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
endif (WITH_FFMPEG)

if (WITH_OCR)
  find_package(PkgConfig)

  pkg_check_modules (TESSERACT REQUIRED tesseract)
  pkg_check_modules (LEPTONICA REQUIRED lept)

  set (EXTRA_INCLUDES ${EXTRA_INCLUDES} ${TESSERACT_INCLUDE_DIRS})
  set (EXTRA_INCLUDES ${EXTRA_INCLUDES} ${LEPTONICA_INCLUDE_DIRS})

  set (EXTRA_LIBS ${EXTRA_LIBS} ${TESSERACT_LIBRARIES})
  set (EXTRA_LIBS ${EXTRA_LIBS} ${LEPTONICA_LIBRARIES})

  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_OCR")
endif (WITH_OCR)

if (WITH_SHARING)
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_SHARING")
endif (WITH_SHARING)

aux_source_directory ("${PROJECT_SOURCE_DIR}/lib_ccx/" SOURCEFILE)
aux_source_directory ("${PROJECT_SOURCE_DIR}/gpacmp4/" SOURCEFILE)

add_library (ccx ${SOURCEFILE} ccx_dtvcc.h ccx_dtvcc.c ccx_encoders_mcc.c ccx_encoders_mcc.h)
target_link_libraries (ccx ${EXTRA_LIBS})
target_include_directories (ccx PUBLIC ${EXTRA_INCLUDES})

if (WITH_HARDSUBX)
    find_package(PkgConfig)

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

    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_HARDSUBX")
endif (WITH_HARDSUBX)

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
