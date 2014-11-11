cmake_policy(SET CMP0037 NEW)

SET (CMAKE_C_FLAGS "-O0 -Wall -g -std=gnu99")
AUX_SOURCE_DIRECTORY("${PROJECT_SOURCE_DIR}/lib_ccx/" SOURCEFILE)
AUX_SOURCE_DIRECTORY("${PROJECT_SOURCE_DIR}/gpacmp4/" SOURCEFILE)
#AUX_SOURCE_DIRECTORY("${PROJECT_SOURCE_DIR}/libpng/" SOURCEFILE)
add_library(ccx ${SOURCEFILE})

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
