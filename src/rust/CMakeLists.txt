if (WITH_OCR AND WITH_HARDSUBX)
    set(FEATURE_ARG "\"hardsubx_ocr\"")
    set(FEATURE_FLAG "--features")
else ()
    set(FEATURE_ARG "")
    set(FEATURE_FLAG "")
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CARGO_CMD cargo build ${FEATURE_FLAG} ${FEATURE_ARG})
    set(TARGET_DIR "debug")
else ()
    set(CARGO_CMD cargo build ${FEATURE_FLAG} ${FEATURE_ARG} --release)
    set(TARGET_DIR "release")
endif ()

# Check rust version
set(MSRV "1.54.0")
execute_process(COMMAND rustc --version
                OUTPUT_VARIABLE Rust_Version)
string(REGEX MATCH "[0-9]+.[0-9]+.[0-9]+" Rust_Version_string ${Rust_Version})
message(STATUS "Detected rustc version ${Rust_Version_string}")
if (Rust_Version_string VERSION_GREATER_EQUAL ${MSRV})
    message(STATUS "rustc >= MSRV(${MSRV})")
else()
    message(FATAL_ERROR "Minimum supported rust version(MSRV) is ${MSRV}, please upgrade rust")
endif(Rust_Version_string VERSION_GREATER_EQUAL ${MSRV})

if(WIN32)
    set(CCX_RUST_SO "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_DIR}/libccx_rust.lib")
    message("${CCX_RUST_SO}")
    add_custom_target(ccx_rust ALL
        COMMENT "Compiling ccx_rust module"
        COMMAND set CARGO_TARGET_DIR=${CMAKE_CURRENT_BINARY_DIR} ${CARGO_CMD} 
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set_target_properties(ccx_rust PROPERTIES LOCATION ${CCX_RUST_SO})
else()
    set(CCX_RUST_SO "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_DIR}/libccx_rust.a")
    add_custom_target(ccx_rust ALL
        COMMENT "Compiling ccx_rust module"
        COMMAND CARGO_TARGET_DIR=${CMAKE_CURRENT_BINARY_DIR} ${CARGO_CMD} 
        COMMAND cp ${CCX_RUST_SO} ${CMAKE_CURRENT_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set_target_properties(ccx_rust PROPERTIES LOCATION ${CCX_RUST_SO})
endif()

add_test(NAME ccx_rust_test 
    COMMAND cargo test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
