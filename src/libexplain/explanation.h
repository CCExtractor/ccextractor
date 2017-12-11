/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_EXPLANATION_H
#define LIBEXPLAIN_EXPLANATION_H

#include <libexplain/ac/limits.h> /* for PATH_MAX except Solaris */
#include <libexplain/ac/sys/param.h> /* for PATH_MAX except Solaris */

#include <libexplain/string_buffer.h>

typedef struct explain_explanation_t explain_explanation_t;
struct explain_explanation_t
{
    char system_call[PATH_MAX * 3 + 200];
    explain_string_buffer_t system_call_sb;
    int errnum;
    char explanation[PATH_MAX * 2 + 200];
    explain_string_buffer_t explanation_sb;
    char footnotes[1000];
    explain_string_buffer_t footnotes_sb;
};

/**
  * The explain_explanation_init function is used to initialize an
  * explanation struct for use.
  *
  * @param exp
  *     The explanation struct of interest
  * @param errnum
  *     The errno value provoking this message
  */
void explain_explanation_init(explain_explanation_t *exp, int errnum);

/**
  * The explain_explanation_assemble function may be used to
  * carefully glue the problem statement and the explanation together,
  * using internationalization, for which localizations may re-arrange
  * the order.
  *
  * @param exp
  *     The explanation of interest.  You are expected to have filled out
  *     the two string buffers already.
  * @param result
  *     where to print the resulting completed explanation.
  */
void explain_explanation_assemble(explain_explanation_t *exp,
    explain_string_buffer_t *result);

/**
  * The explain_explanation_assemble_gai function may be used to
  * carefully glue the problem statement and the explanation together,
  * using internationalization, for which localizations may re-arrange
  * the order.  (specific to getaddrinfo)
  *
  * @param exp
  *     The explanation of interest.  You are expected to have filled out
  *     the two string buffers already.
  * @param result
  *     where to print the resulting completed explanation.
  */
void explain_explanation_assemble_gai(explain_explanation_t *exp,
    explain_string_buffer_t *result);

/**
  * The explain_explanation_assemble_netdb function may be used to
  * carefully glue the problem statement and the explanation together,
  * using internationalization, for which localizations may re-arrange
  * the order.  (Specific to gethostbyname, et al.)
  *
  * @param exp
  *     The explanation of interest.  You are expected to have filled out
  *     the two string buffers already.
  * @param result
  *     where to print the resulting completed explanation.
  */
void explain_explanation_assemble_netdb(explain_explanation_t *exp,
    explain_string_buffer_t *result);

#endif /* LIBEXPLAIN_EXPLANATION_H */
/* vim: set ts=8 sw=4 et : */
