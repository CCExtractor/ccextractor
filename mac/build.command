gcc -std=gnu99 -Wno-write-strings -DGPAC_CONFIG_DARWIN -D_FILE_OFFSET_BITS=64 -Dfopen64=fopen -Dopen64=open -Dlseek64=lseek -I ../src/gpacmp4 -I ../src/lib_ccx -I ../src/libpng -I ../src/zlib -o ccextractor $(find ../src/ -name '*.cpp' \! -name 'win_*') $(find ../src/ -name '*.c' \! -name 'win_*') -liconv

