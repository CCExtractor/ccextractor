for /f "delims=" %%i in ('cd') do set output=%%i
set CARGO_TARGET_DIR=%output%
cd ..\src\rust
cargo build --target=i686-pc-windows-msvc --features "hardsubx_ocr"
cd ..\..\windows
copy i686-pc-windows-msvc\debug\ccx_rust.lib .\ccx_rust.lib