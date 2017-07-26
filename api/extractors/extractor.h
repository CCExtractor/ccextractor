#include <stdio.h>
#include <stdlib.h>
#include "ccextractor.h"

void python_extract(int srt_counter, unsigned h1, unsigned m1, unsigned s1, unsigned ms1, unsigned h2, unsigned m2, unsigned s2, unsigned ms2, char* buffer);
void python_extract_time_based(unsigned h1, unsigned m1, unsigned s1, unsigned ms1, unsigned h2, unsigned m2, unsigned s2, unsigned ms2, char* buffer);
void python_extract_transcript(char* buffer);



