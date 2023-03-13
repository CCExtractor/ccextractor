for /f "delims=" %%i in ('cd') do set output=%%i
set CARGO_TARGET_DIR=%output%
cd ..\src\rust
cargo build --features "hardsubx_ocr" --target x86_64-pc-windows-msvc
cd ..\..\windows
copy x86_64-pc-windows-msvc\debug\ccx_rust.lib .\ccx_rust.lib