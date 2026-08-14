cmake_policy (SET CMP0037 NEW)

if(MSVC)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W3 /wd4005 /wd4996")
else (MSVC)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wno-pointer-sign -g -std=gnu99")
endif(MSVC)

if(WIN32)
    add_definitions(-DWIN32)
endif(WIN32)

find_package(PkgConfig)
if (WITH_GPAC)
    pkg_check_modules (GPAC gpac)
    if (GPAC_FOUND)
        set (EXTRA_INCLUDES ${EXTRA_INCLUDES} ${GPAC_INCLUDE_DIRS})
        set (EXTRA_LIBS ${EXTRA_LIBS} ${GPAC_LIBRARIES})
        add_definitions(-DENABLE_GPAC)
        set (GPAC_FOUND ${GPAC_FOUND} PARENT_SCOPE)
        set (GPAC_LIBRARIES ${GPAC_LIBRARIES} PARENT_SCOPE)
    else ()
        message(STATUS "GPAC not found via pkg-config. Falling back to FetchContent...")
        include(FetchContent)
        FetchContent_Declare(
            gpac
            GIT_REPOSITORY https://github.com/gpac/gpac.git
            GIT_TAG        master
        )
        FetchContent_MakeAvailable(gpac)
        
        # Link against the fetched GPAC
        set (EXTRA_INCLUDES ${EXTRA_INCLUDES} ${gpac_SOURCE_DIR}/include)
        set (EXTRA_LIBS ${EXTRA_LIBS} libgpac)
        add_definitions(-DENABLE_GPAC)
        set (GPAC_FOUND TRUE PARENT_SCOPE)
        set (GPAC_LIBRARIES libgpac PARENT_SCOPE)
    endif ()
endif ()

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
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_FFMPEG_MP4")
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


aux_source_directory ("${PROJECT_SOURCE_DIR}/lib_ccx/" SOURCEFILE)

add_library (ccx ${SOURCEFILE} ccx_dtvcc.h ccx_dtvcc.c ccx_encoders_mcc.c ccx_encoders_mcc.h)
# PRIVATE prevents transitive propagation to consumers (ccextractor).
# The parent CMakeLists.txt adds all needed libs explicitly, controlling
# link order so FFmpeg libs appear after ccx_rust (required by GNU ld).
target_link_libraries (ccx PRIVATE ${EXTRA_LIBS})
target_include_directories (ccx PUBLIC ${EXTRA_INCLUDES})

if (WITH_HARDSUBX)
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

    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DENABLE_HARDSUBX")
endif (WITH_HARDSUBX)

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
