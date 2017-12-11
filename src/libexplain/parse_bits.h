/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008-2010, 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_PARSE_BITS_H
#define LIBEXPLAIN_PARSE_BITS_H

#include <libexplain/ac/stddef.h>

#include <libexplain/sizeof.h>

typedef struct explain_parse_bits_table_t explain_parse_bits_table_t;
struct explain_parse_bits_table_t
{
    const char      *name;
    int             value;
};

/**
  * The explain_parse_bits function may be used to parse an input
  * string against a table of bitfields.  There may be symbols (from the
  * bits table) or numeric conatsnts (using C notation) and there may be
  * plus (+) or bit-wise-or (|) operators.
  *
  * @param text
  *    The text to be parsed.
  * @param table
  *    The table of symbols for the parser.
  * @param table_size
  *    The lentgh of the table of symbols.
  * @param result
  *    Where to put the results of parsing and evluating the expression.
  * @returns
  *    0 in success, -1 on error
  * @note
  *    this function is <b>not</b> thread safe
  */
int explain_parse_bits(const char *text,
    const explain_parse_bits_table_t *table, size_t table_size, int *result);

/**
  * The explain_parse_bits_get_error function may be used to obtain
  * the error message emitted by the parser by the previous call to
  * #explain_parse_bits.
  */
const char *explain_parse_bits_get_error(void);

/**
  * The explain_parse_bits_or_die function may be used to parse an input
  * string against a table of bitfields.  There may be symbols (from the
  * bits table) or numeric conatsnts (using C notation) and there may be
  * plus (+) or bit-wise-or (|) operators.
  *
  * @param text
  *    The text to be parsed.
  * @param table
  *    The table of symbols for the parser.
  * @param table_size
  *    The lentgh of the table of symbols.
  * @param caption
  *    Caption to add to start of error message, or NULL for none
  * @returns
  *    The value of the expression.  Does not return on error, but
  *    prints diagnostic and exits EXIT_FAILURE.
  * @note
  *    this function is <b>not</b> thread safe
  */
int explain_parse_bits_or_die(const char *text,
    const explain_parse_bits_table_t *table, size_t table_size,
    const char *caption);

/**
  * The explain_parse_bits_find_by_name function is used to search a
  * parse-bits table for the given name.
  *
  * @param name
  *      The name to search for in the table
  * @param table
  *      The table to be searched.
  * @param table_size
  *      The number of members in the table to be searched.
  * @returns
  *      pointer to table entry on succes, or NULL on no match
  */
const explain_parse_bits_table_t *explain_parse_bits_find_by_name(
    const char *name, const explain_parse_bits_table_t *table,
    size_t table_size);

/**
  * The explain_parse_bits_find_by_name_fuzzy function is used to
  * search a parse-bits table for the given name, using fuzzy matching.
  * This is best used after explain_parse_bits_find_by_name has
  * already failed, for the purpose of producing a better error message.
  *
  * @param name
  *      The name to search for in the table
  * @param table
  *      The table to be searched.
  * @param table_size
  *      The number of members in the table to be searched.
  * @returns
  *      pointer to table entry on succes, or NULL on no match
  */
const explain_parse_bits_table_t *explain_parse_bits_find_by_name_fuzzy(
    const char *name, const explain_parse_bits_table_t *table,
    size_t table_size);

/**
  * The explain_parse_bits_find_by_name_fuzzy function is used to search a
  * parse-bits table for the given name, using fuzzy matching.
  *
  * @param name
  *      The name to search for in the table
  * @param table
  *      The table to be searched.
  * @param table_size
  *      The number of members in the table to be searched.
  * @returns
  *      pointer to table entry on succes, or NULL on no match
  */
const explain_parse_bits_table_t *explain_parse_bits_find_by_name_fuzzy(
    const char *name, const explain_parse_bits_table_t *table,
    size_t table_size);

/**
  * The explain_parse_bits_find_by_value function is used to search a
  * parse-bits table for the given value.
  *
  * @param value
  *      The value to search for in the table
  * @param table
  *      The table to be searched.
  * @param table_size
  *      The number of members in the table to be searched.
  * @returns
  *      pointer to table entry on succes, or NULL on no match
  */
const explain_parse_bits_table_t *explain_parse_bits_find_by_value(
    int value, const explain_parse_bits_table_t *table,
    size_t table_size);

struct explain_string_buffer_t; /* forward */

/**
  * The explain_parse_bits_print function may be used to break a
  * bit-set value into its component bits and print the result.
  *
  * @param sb
  *      The string buffer to print into.
  * @param value
  *      The value to dismantle and search for bits in the table
  * @param table
  *      The table to be searched.
  * @param table_size
  *      The number of members in the table to be searched.
  */
void explain_parse_bits_print(struct explain_string_buffer_t *sb,
    int value, const explain_parse_bits_table_t *table, int table_size);

/**
  * The explain_parse_bits_print_single function may be used to
  * lookup a value and print the name.
  *
  * @param sb
  *      The string buffer to print into.
  * @param value
  *      The value to dismantle and search for in the table
  * @param table
  *      The table to be searched.
  * @param table_size
  *      The number of members in the table to be searched.
  */
void explain_parse_bits_print_single(struct explain_string_buffer_t *sb,
    int value, const explain_parse_bits_table_t *table, int table_size);

#endif /* LIBEXPLAIN_PARSE_BITS_H */
/* vim: set ts=8 sw=4 et : */
