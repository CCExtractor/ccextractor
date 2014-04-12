#ifndef CCX_PLATFORM_H
#define CCX_PLATFORM_H

#define __STDC_FORMAT_MACROS

#ifdef _WIN32

#include <io.h>
#include <windows.h>
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#include "inttypes.h"
#define UINT64_MAX   _UI64_MAX
typedef int socklen_t;
typedef uint32_t in_addr_t;
#define IN_CLASSD(i)       (((INT32)(i) & 0xf0000000) == 0xe0000000)
#define IN_MULTICAST(i)    IN_CLASSD(i)

#else // _WIN32

#include <unistd.h>
#define __STDC_LIMIT_MACROS
#include <inttypes.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#endif // _WIN32


#include "disable_warnings.h"
#ifdef _MSC_VER
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
#if defined(MSC_VER)
extern "C" int __cdecl _fseeki64(FILE *, __int64, int);
extern "C" __int64 __cdecl _ftelli64(FILE *);
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
#ifdef __CYGWIN__
// Cygwin internally maps these functions to 64bit usage, but there are
// no explicit xxxx64 functions.
#define FOPEN64 fopen
#define OPEN open
#define LSEEK lseek
#else
#define FOPEN64 fopen64
#define OPEN open64
#define LSEEK lseek64
#endif
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


typedef int64_t LLONG;


#endif // CCX_PLATFORM_H
