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
void ccx_log(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    mprint(fmt, args);
    va_end(args);
} 