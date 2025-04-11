#include "lib_ccx.h"
#include "utility.h"

/**
 * ccx_log - A wrapper function for mprint to be used by the Rust code
 * 
 * This function is a simple wrapper around mprint to provide a consistent
 * logging interface for the Rust code.
 * 
 * @param fmt The format string
 * @param ... Variable arguments for the format string
 */
#ifdef __APPLE__
// On macOS, we need to ensure the function is exported with the correct name
__attribute__((visibility("default")))
void _ccx_log(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    mprint(fmt, args);
    va_end(args);
}

// Also provide the original name for compatibility
__attribute__((visibility("default")))
void ccx_log(const char *fmt, ...)
{
    // For compatibility, we'll just call mprint directly
    va_list args;
    va_start(args, fmt);
    mprint(fmt, args);
    va_end(args);
}
#else
__attribute__((visibility("default")))
void ccx_log(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    mprint(fmt, args);
    va_end(args);
}
#endif 