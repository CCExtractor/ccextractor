include(FetchContent)
FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG v0.5.1
    FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(Corrosion)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(PROFILE "dev")
else()
    set(PROFILE "release")
endif()

if(WITH_OCR AND WITH_HARDSUBX)
    set(FEATURES "hardsubx_ocr")
else()
    set(FEATURES "")
endif()

if(WITH_FFMPEG)
    if(FEATURES STREQUAL "")
        set(FEATURES "enable_mp4_ffmpeg")
    else()
        set(FEATURES "${FEATURES};enable_mp4_ffmpeg")
    endif()
endif()

# Check rust version
set(MSRV "1.87.0")
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

# Corrosion hard-codes the Command Line Tools SDK's usr/lib as a link
# directory on macOS. When that SDK is newer than the Xcode toolchain doing
# the link (e.g. macOS 27 SDK, Xcode 26 ld), ld rejects its .tbd stubs.
# The system libraries rustc asks for resolve through the compiler's own SDK.
if(APPLE)
    set_property(TARGET ccx_rust-static PROPERTY INTERFACE_LINK_DIRECTORIES "")
endif()

add_test(
    NAME ccx_rust_test
    COMMAND $<TARGET_FILE:Rust::Cargo> test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
