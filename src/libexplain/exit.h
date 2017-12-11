/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009, 2010, 2013 Peter Miller
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

/**
  * @file
  * @brief "Intercept and explain program exit status"
  */
#ifndef LIBEXPLAIN_EXIT_H
#define LIBEXPLAIN_EXIT_H

/**
  * The explain_exit_on_exit function may be used to cause the value of
  * the argument to exit(), or the return from main, to be printed on exit.
  *
  * These three functions may be called more than once.  The last to
  * be called dictates the operation.  The exit status will never be
  * printed more than once.
  *
  * @note
  *     You need an ANSI C '89 compliant C compiler, or this does nothing.
  */
void explain_exit_on_exit(void);

/**
  * The explain_exit_on_error function may be used to cause the value of
  * the argument to exit(), or the return from main, to be printed on exit,
  * int the case where that value is not EXIT_SUCCESS.
  *
  * These three functions may be called more than once.  The last to
  * be called dictates the operation.  The exit status will never be
  * printed more than once.
  *
  * @note
  *     You need an ANSI C '89 compliant C compiler, or this does nothing.
  */
void explain_exit_on_error(void);

/**
  * The explain_exit_cancel function may be used to cause the value of
  * the argument to exit(), or the return from main, to be printed on exit,
  * int the case where that value is not EXIT_SUCCESS.
  *
  * These three functions may be called more than once.  The last to
  * be called dictates the operation.  The exit status will never be
  * printed more than once.
  *
  * @note
  *     You need an ANSI C '89 compliant C compiler, or this does nothing.
  */
void explain_exit_cancel(void);

#endif /* LIBEXPLAIN_EXIT_H */
/* vim: set ts=8 sw=4 et : */
