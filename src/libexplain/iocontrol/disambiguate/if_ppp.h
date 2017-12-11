/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2010, 2013 Peter Miller
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_IOCONTROL_DISAMBIGUATE_IF_PPP_H
#define LIBEXPLAIN_IOCONTROL_DISAMBIGUATE_IF_PPP_H

#include <libexplain/iocontrol.h>

/**
  * The explain_iocontrol_disambiguate_is_if_ppp function is used
  * to determine whether or not a file descriptor refers to a PPP
  * interface.
  *
  * This is used be the icotl explanations to disambiguate ioctl request
  * numbers in the shared range.  This dictates the return values.
  *
  * @param fildes
  *     The file descriptor of interest
  * @param request
  *     The request number of interest
  * @param data
  *     The accompanying data.
  * @returns
  *     0 if is a PPP interface, -1 if not
  */
int explain_iocontrol_disambiguate_is_if_ppp(int fildes, int request,
    const void *data);

#endif /* LIBEXPLAIN_IOCONTROL_DISAMBIGUATE_IF_PPP_H */
/* vim: set ts=8 sw=4 et : */
