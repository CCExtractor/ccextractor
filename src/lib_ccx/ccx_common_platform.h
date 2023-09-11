#ifndef CCX_PLATFORM_H
	#define CCX_PLATFORM_H

	// Default includes (cross-platform)
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <ctype.h>
	#include <time.h>
	#include <fcntl.h>
	#include <stdarg.h>
	#include <errno.h>

	#define __STDC_FORMAT_MACROS

	#ifdef _WIN32
		#define inline _inline
		#define typeof decltype
		#include <io.h>
		#include <ws2tcpip.h>
		#include <windows.h>
		#define STDIN_FILENO 0
		#define STDOUT_FILENO 1
		#define STDERR_FILENO 2
		#include "inttypes.h"
		#undef UINT64_MAX
		#define UINT64_MAX   _UI64_MAX
		typedef int socklen_t;
		typedef int ssize_t;
		typedef uint32_t in_addr_t;
		#ifndef IN_CLASSD
			#define IN_CLASSD(i)       (((INT32)(i) & 0xf0000000) == 0xe0000000)
			#define IN_MULTICAST(i)    IN_CLASSD(i)
		#endif
		#include <direct.h>
		#define mkdir(path, mode) _mkdir(path)
		#ifndef snprintf
			// Added ifndef because VS2013 warns for macro redefinition.
			#define snprintf(buf, len, fmt, ...) _snprintf(buf, len, fmt, __VA_ARGS__)
		#endif
		#define sleep(sec) Sleep((sec) * 1000)

		#include <fcntl.h>
	#else // _WIN32
		#include <unistd.h>
		#define __STDC_LIMIT_MACROS
		#include <inttypes.h>
		#include <stdint.h>
		#include <sys/socket.h>
		#include <netinet/in.h>
		#include <arpa/inet.h>
		#include <netdb.h>
		#include <sys/stat.h>
		#include <sys/types.h>
	#endif // _WIN32

	//#include "disable_warnings.h"

	#if defined(_MSC_VER) && !defined(__clang__)
		#include "stdintmsc.h"
		// Don't bug me with strcpy() deprecation warnings
		#pragma warning(disable : 4996)
	#else
		#include <stdint.h>
	#endif

	#ifdef __OpenBSD__
		#define FOPEN64 fopen
		#define OPEN open
		#define FSEEK fseek
		#define FTELL ftell
		#define LSEEK lseek
		#define FSTAT fstat
	#else
		#ifdef _WIN32
			#define OPEN _open
			// 64 bit file functions
			#if defined(_MSC_VER)
				#define FSEEK _fseeki64
				#define FTELL _ftelli64
			#else
				// For MinGW
				#define FSEEK fseeko64
				#define FTELL ftello64
			#endif
			#define TELL _telli64
			#define LSEEK _lseeki64
			typedef struct _stati64 FSTATSTRUCT;
		#else
			// Linux internally maps these functions to 64bit usage,
			// if _FILE_OFFSET_BITS macro is set to 64
			#define FOPEN64 fopen
			#define OPEN open
			#define LSEEK lseek
			#define FSEEK fseek
			#define FTELL ftell
			#define FSTAT fstat
			#define TELL tell
			#include <stdint.h>
			#endif
	#endif

	#ifndef int64_t_C
		#define int64_t_C(c)     (c ## LL)
		#define uint64_t_C(c)    (c ## ULL)
	#endif

	#ifndef O_BINARY
		#define O_BINARY 0  // Not present in Linux because it's always binary
	#endif

	#ifndef max
		#define max(a,b) (((a) > (b)) ? (a) : (b))
	#endif

	typedef int64_t LLONG;
	typedef uint64_t ULLONG;
	typedef uint8_t UBYTE;

#endif // CCX_PLATFORM_H
