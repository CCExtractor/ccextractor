include(FetchContent)
FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG 64289b1d79d6d19cd2e241db515381a086bb8407 # v0.5.0
    FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(Corrosion)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(PROFILE "debug")
else()
    set(PROFILE "release")
endif()

if(WITH_OCR AND WITH_HARDSUBX)
    set(FEATURES "hardsubx_ocr")
else()
    set(FEATURES "")
endif()

# Check rust version
set(MSRV "1.54.0")
if(Rust_VERSION VERSION_GREATER_EQUAL ${MSRV})
    message(STATUS "rustc >= MSRV(${MSRV})")
else()
    message(FATAL_ERROR "Minimum supported rust version(MSRV) is ${MSRV}, please upgrade rust")
endif(Rust_VERSION VERSION_GREATER_EQUAL ${MSRV})

corrosion_import_crate(
    MANIFEST_PATH Cargo.toml
    PROFILE ${PROFILE}
    FEATURES ${FEATURES}
)

add_test(
    NAME ccx_rust_test
    COMMAND $<TARGET_FILE:Rust::Cargo> test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
