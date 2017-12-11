/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2009-2011, 2013 Peter Miller
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <libexplain/ac/limits.h>
#include <libexplain/ac/linux/kdev_t.h>
#include <libexplain/ac/sys/stat.h> /* for major()/minor() except Solaris */
#include <libexplain/ac/sys/sysmacros.h> /* for major()/minor() on Solaris */

#include <libexplain/buffer/dev_t.h>
#include <libexplain/buffer/device_name.h>
#include <libexplain/buffer/mount_point.h>
#include <libexplain/buffer/pointer.h>
#include <libexplain/option.h>
#include <libexplain/parse_bits.h>
#include <libexplain/is_efault.h>


void
explain_buffer_dev_t(explain_string_buffer_t *sb, dev_t dev)
{
#if defined(MKDEV)
    explain_string_buffer_printf
    (
        sb,
        "MKDEV(%d, %d)",
        (int)MAJOR(dev),
        (int)MINOR(dev)
    );
#elif defined(makedev)
    explain_string_buffer_printf
    (
        sb,
        "makedev(%ld, %ld)",
        (long)major(dev),
        (long)minor(dev)
    );
#else
    explain_string_buffer_printf(sb, "0x%04lX", (long)dev);
#endif

    if (explain_option_dialect_specific())
    {
        explain_string_buffer_t path_buf;
        struct stat     st;
        char            path[PATH_MAX];

        /* find and print the device name */
        explain_string_buffer_init(&path_buf, path, sizeof(path));
        if (explain_buffer_device_name(&path_buf, dev, &st) >= 0)
        {
            explain_string_buffer_putc(sb, ' ');
            explain_string_buffer_puts_quoted(sb, path);
        }

        /* find and print the mount point */
        explain_buffer_mount_point_dev(sb, dev);
    }
}


int
explain_parse_dev_t_or_die(const char *text, const char *caption)
{
    return explain_parse_bits_or_die(text, 0, 0, caption);
}


/* vim: set ts=8 sw=4 et : */
