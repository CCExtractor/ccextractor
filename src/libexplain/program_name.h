/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2012, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_PROGRAM_NAME_H
#define LIBEXPLAIN_PROGRAM_NAME_H

/**
  * @file
  * @brief obtain the name of the program
  */

#ifdef __cplusplus
extern "C" {
#endif

/**
  * The explain_program_name_get function may be used to obtain the
  * command name of the process.  Depending on how capable /proc is
  * on your system, or, failing that, how capable lsof(1) is on your
  * system, this may or may not produce a sensable result.  It works
  * well on Linux.
  *
  * @returns
  *    pointer to string containing the command name (no slashes) of the
  *    calling process.
  */
const char *explain_program_name_get(void);

/**
  * The explain_program_name_set function may be used to set
  * the libexplain libraries' idea of the command name of the
  * calling process, setting the string to be returned by the
  * explain_program_name_get function.  This overrides the automatic
  * behaviour, which can be quite desirable in commands that can be
  * invoked with more than one name, e.g. if they are a hard link
  * synoym.
  *
  * This also sets the option to include the program name in all of the
  * error messages issued by the explain_*_or_die functions.
  *
  * @param name
  *     The name of the calling process.
  */
void explain_program_name_set(const char *name);

/**
  * The explain_option_assemble_program_name_set option is used to
  * override any EXPLAIN_OPTIONS or default setting as to whether or
  * not the #explain_output_error, #explain_output_error_and_die and
  * #explain_output_warning functions should include the program name
  * at the start the messages.
  *
  * If not explicitly set, is controlled by the EXPLAIN_OPTIONS
  * environemnt variable, or defaults to false if not set there either.
  *
  * @param yesno
  *     true (non-zero) to include the program name, zero (false)
  *     to omit the program name.
  */
void explain_program_name_assemble(int yesno);

#ifdef __cplusplus
}
#endif

#endif /* LIBEXPLAIN_PROGRAM_NAME_H */
/* vim: set ts=8 sw=4 et : */
