/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2013 Peter Miller
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

#include <libexplain/buffer/enodev.h>
#include <libexplain/buffer/gettext.h>



void
explain_buffer_enodev_anon_inodes(explain_string_buffer_t *sb,
    const char *syscall_name)
{
    (void)syscall_name;
    explain_buffer_gettext
    (
        sb,
        /*
         * xgettext: This message is issued to explain an ENODEV error
         * reported by the eventfs, eventpoll, signalfd and timerfd system
         * call.
         */
        i18n("the kernel could not mount the internal anonymous inode device")
    );
}


/* vim: set ts=8 sw=4 et : */


/* vim: set ts=8 sw=4 et : */
