@echo OFF
setlocal EnableDelayedExpansion
echo Obtaining Git commit
git rev-parse HEAD 2>NUL > temp.txt
set /p commit=<temp.txt
if not defined commit (
	echo Git command not present, trying folder approach
	if exist "..\.git" (
		echo Git folder found, using HEAD file
		type "..\.git\HEAD" 2>NUL > temp.txt
		for /f "delims=" %%a in (temp.txt) do set head=%%a&goto next
		:next
		set start=%head:~0,5%
		set rest=%head:~5%
		if "%start%"=="ref: " (
			echo HEAD file contains a ref, following the ref
			echo ..\.git\%rest% > temp.txt
			for /f "delims=" %%a in (temp.txt) do set ref_path=%%a&goto nexttt
			:nexttt
			set ref_path=%ref_path:/=\%
			type "%ref_path%" 2>NUL > temp.txt
			for /f "delims=" %%a in (temp.txt) do set commit=%%a&goto nextt
			:nextt
			echo Extracted commit: %commit%
			goto final
		) else (
			echo HEAD contains a commit, using it (%head%)
			set commit = %head%
			goto final
		)		
	)
)
:final
if not defined commit set commit=Unknown
for /F "usebackq tokens=1,2 delims==" %%i in (`wmic os get LocalDateTime /VALUE 2^>NUL`) do if '.%%i.'=='.LocalDateTime.' set ldt=%%j
set builddate=%ldt:~0,4%-%ldt:~4,2%-%ldt:~6,2%
echo storing variables in file
echo commit: %commit%
echo date: %builddate%
echo #ifndef CCX_CCEXTRACTOR_COMPILE_H > ../src/lib_ccx/compile_info.h
echo #define CCX_CCEXTRACTOR_COMPILE_H >> ../src/lib_ccx/compile_info.h
echo #define GIT_COMMIT "%commit%" >> ../src/lib_ccx/compile_info.h
echo #define COMPILE_DATE "%builddate%" >> ../src/lib_ccx/compile_info.h
echo #endif >> ../src/lib_ccx/compile_info.h
echo stored all in compile_info.h
del temp.txt
echo done