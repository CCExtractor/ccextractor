for /f "delims=" %%i in ('cd') do set output=%%i
set CARGO_TARGET_DIR=%output%
cd ..\src\rust
REM Default to x86_64 if RUST_TARGET is not set
IF "%RUST_TARGET%"=="" set RUST_TARGET=x86_64-pc-windows-msvc
REM Allow overriding FFmpeg version via environment variable
IF "%FFMPEG_VERSION%"=="" (
    cargo build %1 --features "hardsubx_ocr" --target %RUST_TARGET%
) ELSE (
    cargo build %1 --features "hardsubx_ocr,%FFMPEG_VERSION%" --target %RUST_TARGET%
)
cd ..\..\windows
IF "%~1"=="-r" (
copy %RUST_TARGET%\release\ccx_rust.lib .\ccx_rust.lib
) ELSE (
copy %RUST_TARGET%\release-with-debug\ccx_rust.lib .\ccx_rust.lib
)
