for /f "delims=" %%i in ('cd') do set output=%%i
set CARGO_TARGET_DIR=%output%
cd ..\src\rust
cargo build --features "hardsubx_ocr"
cd ..\..\windows
copy debug\ccx_rust.lib .\ccx_rust.lib