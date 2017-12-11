/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010-2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_OUTPUT_H
#define LIBEXPLAIN_OUTPUT_H

#include <libexplain/gcc_attributes.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @file
  * @brief Output Redirection
  *
  * It is possible to change how and where libexplain sends its output,
  * and even how it calls the exit(2) function.  This functionality is
  * used by the explain_*_or_die and explain_*_on_error functions.
  *
  * By default, libexplain will wrap and print error messages on stderr,
  * and call the exit(2) system call to terminate execution.
  *
  * Clients of the libexplain library may choose to use some message
  * handling facilities provided by libexplain, or they may choose to
  * implement their own.
  *
  * @section syslog syslog
  *     To cause all output to be sent to syslog, use
  *     @code
  *     explain_output_register(explain_output_syslog_new());
  *     @endcode
  *
  * @section stderr "stderr and syslog"
  *     The "tee" output class can be used to duplicate output.
  *     To cause all output to be sent to both stderr and syslog, use
  *     @code
  *     explain_output_register
  *     (
  *         explain_output_tee_new
  *         (
  *             explain_output_stderr_new(),
  *             explain_output_syslog_new()
  *         )
  *     );
  *     @endcode
  *     If you need more than two, use several instances of "tee", cascaded.
  *
  * @section file "stderr and a file"
  *     To cause all output to be sent to both stderr and a regular file, use
  *     @code
  *     explain_output_register
  *     (
  *         explain_output_tee_new
  *         (
  *             explain_output_stderr_new(),
  *             explain_output_file_new(filename, 0)
  *         )
  *     );
  *     @endcode
  *
  * @section cxx C++
  *     It would be possible to create an error handler that accumulated
  *     a string when its message method was called, and threw an
  *     exception when its "exit" method was called.  This is why
  *     "message" and "exit" are tied together in this way.
  *
  *     FIXME: write such a thing, and include it in the library
  */

/**
  * The explain_output_t type is a synonym for struct explain_output_t.
  */
typedef struct explain_output_t explain_output_t;

/**
  * The explain_output_t struct is used to remember state for the
  * classes that control libexplain's output and exit methods.
  *
  * Derived classes may choose to add additonal instance variables.
  */
struct explain_output_t
{
    /**
      * The vtable instance variable is used to remember the location
      * if this instance's class's method pointers.
      */
    const struct explain_output_vtable_t *vtable;

    /**
      * POSIX and the C standard both say that it should not call
      * 'exit', because the behavior is undefined if 'exit' is called
      * more than once.  So we call '_exit' instead of 'exit'.
      *
      * This important if a libexplain funcion is called by an atexit
      * hander, where exit has been called by libexplain.
      */
    int exit_has_been_used;
};

/**
  * The explain_output_vtable_t type is a synonym for struct
  * explain_output_vtable_t.
  */
typedef struct explain_output_vtable_t explain_output_vtable_t;

/**
  * The explain_output_vtable_t struct describes the methods of a
  * class derived from the explain_output_t pure abstract class.
  */
struct explain_output_vtable_t
{
    /**
      * The destructor method is called when (if) the output instance is
      * destroyed.  May be NULL, if no cleanup is required.
      *
      * @param op
      *     Pointer to the explain_output_t instance to be operated on.
      */
    void (*destructor)(explain_output_t *op);

    /**
      * The message method is ised to print text.  Different
      * output "classes" handle this differently.
      *
      * @param op
      *     Pointer to the explain_output_t instance to be "printed" on.
      * @param text
      *     The text of the message to be printed.
      *     It has not been wrapped.
      */
    void (*message)(explain_output_t *op, const char *text);

    /**
      * The exit method is used to terminate execution.  Different
      * "classes" handle this differently.
      * May be NULL, in which case exit(status) will be called.
      *
      * @param op
      *     Pointer to the explain_output_t instance to be operated on.
      * @param status
      *     The exist status requested.
      *
      * @note
      *     The "exit" method shall not return. if it does, exit(status)
      *     will be called anyway.  This is because the rest of the
      *     libexplain code assumes that "exit" means "::exit".
      */
    void (*exit)(explain_output_t *op, int status);

    /**
      * The size instance variable is used to remember how large
      * this instance is, in bytes.  Used by the #explain_output_new
      * function, below.
      */
    unsigned int size;
};

/**
  * The explain_output_new function may be used to create a new
  * dynamically allocated instance of explain_output_t.
  *
  * @param vtable
  *     The struct containing the pointers to the methods of the derived
  *     class.
  * @returns
  *     NULL on error (i.e. malloc failed), or a pointer to a new
  *     dynamically allocated instance of the class.
  */
explain_output_t *explain_output_new(const explain_output_vtable_t *vtable);

/**
  * The explain_output_stderr_new function may be used to create a new
  * dynamically allocated instance of an explain_output_t class that
  * writes to stderr, and exits via exit(2);
  *
  * This is the default output handler.
  *
  * Long error messages will be wrapped to fit the size of the current
  * terminal line.
  *
  * You can select whether or not the second and subsequent lines of wrapped
  * error messages are indented.
  * <ul>
  * <li> calling the #explain_option_hanging_indent_set function at the
  *      beginning of main(); or,
  * <li> setting the EXPLAIN_OPTIONS environment variable, setting the
  *      "hanging-indent=N" option; or,
  * <li> the default is not to indent the second and subsequent lines of
  *      wrapped error messages.
  * </ul>
  * The above list is in order of highest precedence to lowest.
  * A hanging indent of zero means "no indent".
  *
  * @returns
  *     NULL on error (i.e. malloc failed), or a pointer to a new
  *     dynamically allocated instance of the stderrr class.
  */
explain_output_t *explain_output_stderr_new(void);

/**
  * The explain_output_syslog_new function may be used to create a new
  * dynamically allocated instance of an explain_output_t
  * class that writes to syslog, and exits via exit(2);
  *
  * The following values are used:                                          <br>
  *     option = 0                                                          <br>
  *     facility = LOG_USER                                                 <br>
  *     level = LOG_ERR                                                     <br>
  * See syslog(3) for more information.
  *
  * @returns
  *     NULL on error (i.e. malloc failed), or a pointer to a new
  *     dynamically allocated instance of the syslog class.
  */
explain_output_t *explain_output_syslog_new(void);

/**
  * The explain_output_syslog_new1 function may be used to create a new
  * dynamically allocated instance of an explain_output_t class that
  * writes to syslog, and exits via exit(2);
  *
  * The following values are used:                                          <br>
  *     option = 0                                                          <br>
  *     facility = LOG_USER                                                 <br>
  * See syslog(3) for more information.
  *
  * @param level
  *     The syslog level to be used, see syslog(3) for a definition.
  * @returns
  *     NULL on error (i.e. malloc failed), or a pointer to a new
  *     dynamically allocated instance of the syslog class.
  */
explain_output_t *explain_output_syslog_new1(int level);

/**
  * The explain_output_syslog_new3 function may be used to create a new
  * dynamically allocated instance of an explain_output_t class that
  * writes to syslog, and exits via exit(2);
  *
  * If you want different facilities or levels, create multiple instances.
  *
  * @param option
  *     The syslog option to be used, see syslog(3) for a definition.
  * @param facility
  *     The syslog facility to be used, see syslog(3) for a definition.
  * @param level
  *     The syslog level to be used, see syslog(3) for a definition.
  * @returns
  *     NULL on error (i.e. malloc failed), or a pointer to a new
  *     dynamically allocated instance of the syslog class.
  */
explain_output_t *explain_output_syslog_new3(int option, int facility,
    int level);

/**
  * The explain_output_file_new function may be used to create a new
  * dynamically allocated instance of an #explain_output_t class that
  * writes to a file, and exits via exit(2).
  *
  * @param filename
  *     The file to be oopened and written to.
  * @param append
  *     true (non-zero) if messages are to be appended to the file,
  *     false (zero) if the file is to be preplaced with new contents.
  * @returns
  *     NULL on error (i.e. malloc failed), or a pointer to a new
  *     dynamically allocated instance of the syslog class.
  */
explain_output_t *explain_output_file_new(const char *filename, int append);

/**
  * The explain_output_tee_new function may be used to create a new
  * dynamically allocated instance of an explain_output_t class that
  * writes to <b>two</b> other output classes.
  *
  * @param first
  *     The first output class to write to.
  * @param second
  *     The second output class to write to.
  * @returns
  *     NULL on error (i.e. malloc failed), or a pointer to a new
  *     dynamically allocated instance of the syslog class.
  *
  * @note
  *     The output subsystem will "own" the first and second objects
  *     after this call.  You may not make any reference to these
  *     pointers ever again.  The output subsystem will destroy these
  *     objects and free the memory when it feels like it.
  */
explain_output_t *explain_output_tee_new(explain_output_t *first,
    explain_output_t *second);

/**
  * The explain_output_message function is used to print text.  It is
  * printed via the registered output class, see #explain_output_register
  * for how.
  *
  * @param text
  *     The text of the message to be printed.
  *     It has not been wrapped.
  */
void explain_output_message(const char *text);

/**
  * The explain_output_error function is used to print a formatted error
  * message.
  *
  * If the program name option has been selected, the program name will
  * be prepended to the error message before it is printed.
  * To select the option
  * <ul>
  * <li> calling the #explain_program_name_assemble function at the start of
  *      your program's main() function; or,
  * <li> using the EXPLAIN_OPTIONS environment variable, giving the
  *      "program-name" or "no-program-name" option; or,
  * <li> the default is to print the program name at the start of error
  *      and warning messages.
  * </ul>
  * These methods are presented here in order of highest to lowest precedence.
  *
  * The printing is done via the #explain_output_message function, which
  * will do wrapping and indenting if the appropriate output class and
  * options have been selected.
  *
  * @param fmt
  *     The format text of the message to be printed.
  *     See printf(3) for more information.
  */
void explain_output_error(const char *fmt, ...)
                                                 LIBEXPLAIN_FORMAT_PRINTF(1, 2);

/**
  * The explain_output_error_and_die function is used to print text,
  * and then terminate immediately.  The printing is done via the
  * #explain_output_message function, #explain_output_exit function.
  *
  * @param fmt
  *     The format text of the message to be printed.
  *     See printf(3) for more information.
  */
void explain_output_error_and_die(const char *fmt, ...)
                                                  LIBEXPLAIN_FORMAT_PRINTF(1, 2)
                                                            LIBEXPLAIN_NORETURN;

/**
  * The explain_output_warning function is used to print a formatted
  * error message, including the word "warning".
  *
  * If the program name option has been selected, the program name will be
  * prepended to the error message before it is printed.  To select the option
  * <ul>
  * <li> call the #explain_program_name_assemble  function at the start of your
  *      program's main() function; or,
  * <li> use the EXPLAIN_OPTIONS environment variable, giving the
  *      "program-name" or "no-program-name" option; or,
  * <li> the default is to always print the program name at the start of error
  *      and warning messages.
  * </ul>
  * These methods are presented here in order of highest to lowest precedence.
  *
  * The printing is done via the #explain_output_message function, which
  * will do wrapping and indenting if the appropriate output class and
  * options have been selected.
  *
  * @param fmt
  *     The format text of the message to be printed.
  *     See printf(3) for more information.
  */
void explain_output_warning(const char *fmt, ...)
                                                 LIBEXPLAIN_FORMAT_PRINTF(1, 2);

/**
  * The explain_output_method_message function is used to print text.
  * Different output "classes" handle this differently.
  *
  * @param op
  *     Pointer to the explain_output_t instance to be "printed" on.
  * @param text
  *     The text of the message to be printed.
  *     It has not been wrapped.
  */
void explain_output_method_message(explain_output_t *op, const char *text);

/**
  * The explain_output_exit function is used to terminate
  * execution.  It is executed via the registered output class,
  * #explain_output_register for how.
  *
  * @param status
  *     The exist status requested.
  */
void explain_output_exit(int status)                        LIBEXPLAIN_NORETURN;

/**
  * The explain_output_exit_failure function is used to terminate
  * execution, with exit status EXIT_FAILURE.  It is executed via the
  * registered output class, #explain_output_register for how.
  */
void explain_output_exit_failure(void)                      LIBEXPLAIN_NORETURN;

/**
  * The explain_output_method_exit function is used to terminate
  * execution.  Different "classes" handle this differently.
  *
  * @param op
  *     Pointer to the explain_output_t instance to be operated on.
  * @param status
  *     The exist status requested.
  */
void explain_output_method_exit(explain_output_t *op, int status)
                                                            LIBEXPLAIN_NORETURN;

/**
  * The explain_output_register function is used to change libexplain's
  * default output handling facilities with something else.  The NULL
  * pointer restores libexplain's default processing.
  *
  * If no output class is registered, the default is to wrap and print
  * to stderr, and to exit via the exit(2) system call.
  *
  * @param op
  *     Pointer to the explain_output_t instance to be operated on.
  *     The NULL pointer will reset to the default style (stderr).
  *
  * @note
  *     The output subsystem will "own" the pointer after this call.
  *     You may not make any reference to this pointer ever again.  The
  *     output subsystem will destroy the object and free the memory
  *     when it feels like it.
  */
void explain_output_register(explain_output_t *op);

/**
  * The explain_output_method_destructor function is used to destroy an
  * output instance, when its lifetime is over, think of it as a class
  * specific free() function.  It is called by #explain_output_register
  * when a new output instance is given.
  *
  * It is safe for op to be NULL.
  * It is safe for op->vtable->destructor to be NULL.
  *
  * @param op
  *     Pointer to the explain_output_t instance to be operated on.
  */
void explain_output_method_destructor(explain_output_t *op);

/**
  * The explain_option_assemble_program_name_set option is used to
  * override any EXPLAIN_OPTIONS or default setting as to whether or
  * not the #explain_output_error, #explain_output_error_and_die and
  * #explain_output_warning functions should include the program name
  * at the start the messages.
  *
  * @param yesno
  *     true (non-zero) to include the program name, zero (false)
  *     to omit the program name.
  */
void explain_program_name_assemble(int yesno);

/**
  * The explain_option_hanging_indent_set function is used to cause
  * the output wrapping to use hanging indents.  By default no hanging
  * indent is used, but this can sometimes obfuscate the end of one
  * error message and the beginning of another.  A hanging indent
  * results in continuation lines starting with white spoace, similar to
  * RFC822 headers.
  *
  * This can be set using the "hanging-indent=N" string in the
  * EXPLAIN_OPTIONS environment variable.
  *
  * Using this function will override any environment variable setting.
  *
  * @param columns
  *     The number of columns of hanging indent to be used.  A value of
  *     0 means no hanging indent (all lines flush with left margin).
  *     A common value to use is 4: it doesn't consume to much of each
  *     line, and it is a clear indent.
  */
void explain_option_hanging_indent_set(int columns);

#ifdef __cplusplus
}
#endif

#endif /* LIBEXPLAIN_OUTPUT_H */
/* vim: set ts=8 sw=4 et : */
