/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#include <libexplain/ac/limits.h>
#include <libexplain/ac/sys/resource.h>
#include <libexplain/ac/unistd.h>

#include <libexplain/buffer/efbig.h>
#include <libexplain/buffer/gettext.h>
#include <libexplain/buffer/pretty_size.h>
#include <libexplain/option.h>


static unsigned long long
get_max_file_size(int file_size_bits)
{
    int             long_long_bits;
    struct rlimit   szlim;
    unsigned long long max_file_size;

    long_long_bits = sizeof(unsigned long long) * CHAR_BIT;
    if (file_size_bits > long_long_bits || file_size_bits <= 0)
        file_size_bits = long_long_bits;
    if (file_size_bits >= long_long_bits)
        max_file_size = ~(unsigned long long)0;
    else
        max_file_size = (unsigned long long)1 << file_size_bits;
    if (getrlimit(RLIMIT_FSIZE, &szlim) < 0 || szlim.rlim_cur == RLIM_INFINITY)
        return max_file_size;
    if ((unsigned long long)szlim.rlim_cur < max_file_size)
        max_file_size = szlim.rlim_cur;
    return max_file_size;
}


unsigned long long
explain_get_max_file_size_by_pathname(const char *pathname)
{
    long            nbits;

#ifdef _PC_FILESIZEBITS
    nbits = pathconf(pathname, _PC_FILESIZEBITS);
#else
    (void)pathname;
    nbits = sizeof(off_t) * CHAR_BIT;
#endif
    return get_max_file_size(nbits);
}


static void
report_error(explain_string_buffer_t *sb, const char *caption,
    unsigned long long actual, unsigned long long maximum)
{
    explain_string_buffer_printf_gettext
    (
        sb,
        /*
         * xgettext: This message is used when a path given in a path is
         * larger that the (dialect specific) maximum path length.
         *
         * %1$s => the name of the offending system call argument.
         */
        i18n("%s is larger than the maximum file size"),
        caption
    );

    if (explain_option_dialect_specific())
    {
        explain_string_buffer_puts(sb, " (");
        explain_buffer_pretty_size(sb, actual);
        explain_string_buffer_puts(sb, " > ");
        explain_buffer_pretty_size(sb, maximum);
        explain_string_buffer_putc(sb, ')');
    }
}


void
explain_buffer_efbig(explain_string_buffer_t *sb, const char *pathname,
    unsigned long long length, const char *length_caption)
{
    unsigned long long maximum;

    maximum = explain_get_max_file_size_by_pathname(pathname);
    report_error(sb, length_caption, length, maximum);
}


unsigned long long
explain_get_max_file_size_by_fildes(int fildes)
{
    long            nbits;

#ifdef _PC_FILESIZEBITS
    nbits = fpathconf(fildes, _PC_FILESIZEBITS);
#else
    (void)fildes;
    nbits = sizeof(off_t) * CHAR_BIT;
#endif
    return get_max_file_size(nbits);
}


void
explain_buffer_efbig_fildes(explain_string_buffer_t *sb, int fildes,
    unsigned long long length, const char *length_caption)
{
    unsigned long long maximum;

    maximum = explain_get_max_file_size_by_fildes(fildes);
    report_error(sb, length_caption, length, maximum);
}


/* vim: set ts=8 sw=4 et : */
