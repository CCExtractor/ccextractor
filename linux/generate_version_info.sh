#!/usr/bin/env bash
echo "Obtaining Git commit"
commit=(`git rev-parse HEAD 2>/dev/null`)
if [ -z "$commit" ]; then
	echo "Git command not present, trying folder approach"
	if [ -d "../.git" ]; then 
		echo "Git folder found, using HEAD file"
		head="`cat ../.git/HEAD`"
		ref_pos=(`expr match "$head" 'ref: '`)
		if [ $ref_pos -eq 5 ]; then	
			echo "HEAD file contains a ref, following"
			commit=(`cat "../.git/${head:5}"`)
			echo "Extracted commit: $commit"
		else
			echo "HEAD contains a commit, using it"
			commit="$head"
			echo "Extracted commit: $commit"	
		fi 
	fi
fi
if [ -z "$commit" ]; then
	commit="Unknown"
fi
echo "Storing variables in file"
echo "Commit: $commit"
# Only append if it does not exist yet in the file
already_in_there=$(grep "CCX_CCEXTRACTOR_COMPILE_REAL_COMMIT_H" ../src/lib_ccx/compile_info_real.h 2>/dev/null)
if [ ! -z "$already_in_there" ]; then
	echo "#ifndef CCX_CCEXTRACTOR_COMPILE_REAL_COMMIT_H" >> ../src/lib_ccx/compile_info_real.h
	echo "#define CCX_CCEXTRACTOR_COMPILE_REAL_COMMIT_H" >> ../src/lib_ccx/compile_info_real.h
	echo "#define GIT_COMMIT \"$commit\"" >> ../src/lib_ccx/compile_info_real.h
	echo "#endif" >> ../src/lib_ccx/compile_info_real.h
endif
echo "Done."
