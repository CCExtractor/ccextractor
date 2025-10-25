#!/usr/bin/env bash
builddate=`date --utc --date="@${SOURCE_DATE_EPOCH:-$(date +%s)}" +%Y-%m-%d`
echo "Storing variables in file"
echo "Date: $builddate"
# Only append if it does not exist yet in the file
already_in_there=$(grep "CCX_CCEXTRACTOR_COMPILE_REAL_TIME_H" ../src/lib_ccx/compile_info_real.h 2>/dev/null)
if [ ! -z "$already_in_there" ]; then
	echo "#ifndef CCX_CCEXTRACTOR_COMPILE_REAL_TIME_H" > ../src/lib_ccx/compile_info_real.h
	echo "#define CCX_CCEXTRACTOR_COMPILE_REAL_TIME_H" >> ../src/lib_ccx/compile_info_real.h
	echo "#define COMPILE_DATE \"$builddate\"" >> ../src/lib_ccx/compile_info_real.h
	echo "#endif" >> ../src/lib_ccx/compile_info_real.h
endif
echo "Done."
