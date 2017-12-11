/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2013 Peter Miller
 * Written by Peter Miller <pmiller@opensource.org.au>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBEXPLAIN_UID_FROM_PID_H
#define LIBEXPLAIN_UID_FROM_PID_H

#include <libexplain/ac/sys/types.h>

/**
  * The explain_uid_from_pid function may be used to determin the
  * effective user-ID of a process.
  *
  * @param pid
  *    The ID of he proicess of interest
  * @returns
  *    the usr ID pof th process, or (pid_t)-1 if couldnot be determined.
  */
uid_t explain_uid_from_pid(pid_t pid);

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_UID_FROM_PID_H */
