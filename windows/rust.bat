cd ..\src\rust
set CARGO_TARGET_DIR = "..\..\windows" && cargo build
cd ..\..\windows
copy debug\ccx_rust.lib .\ccx_rust.lib