for /f "delims=" %%i in ('cd') do set output=%%i
set CARGO_TARGET_DIR=%output%
cd ..\src\rust
cargo build %1 --features "hardsubx_ocr" --target x86_64-pc-windows-msvc
cd ..\..\windows
IF "%~1"=="-r" (
copy x86_64-pc-windows-msvc\release\ccx_rust.lib .\ccx_rust.lib
) ELSE (
copy x86_64-pc-windows-msvc\debug\ccx_rust.lib .\ccx_rust.lib
)