#ifndef CCX_LOG_H
#define CCX_LOG_H

#include <stdarg.h>

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
__attribute__((visibility("default")))
#endif
void ccx_log(const char *fmt, ...);

#ifdef __APPLE__
/**
 * _ccx_log - Internal function for macOS compatibility
 * 
 * This function is provided for compatibility with macOS/arm64 architecture.
 * 
 * @param fmt The format string
 * @param ... Variable arguments for the format string
 */
__attribute__((visibility("default")))
void _ccx_log(const char *fmt, ...);
#endif

#endif /* CCX_LOG_H */ 