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

#ifndef LIBEXPLAIN_LSOF_H
#define LIBEXPLAIN_LSOF_H

#define LIBEXPLAIN_LSOF_FD_mem (-'m')
#define LIBEXPLAIN_LSOF_FD_txt (-'t')
#define LIBEXPLAIN_LSOF_FD_cwd (-'c')
#define LIBEXPLAIN_LSOF_FD_rtd (-'r')
#define LIBEXPLAIN_LSOF_FD_NOFD (-'N')

typedef struct explain_lsof_t explain_lsof_t;
struct explain_lsof_t
{
    int pid;
    int fildes;
    void (*n_callback)(explain_lsof_t *context, const char *name);
};

/**
  * The explain_lsof function may be used to
  *
  * @param options
  *    The options to be passed to the lsof(1) command.
  * @param context
  *    The context, used to remember pid and fildes, and call the
  *    appropriate callbacks, as the data is seen.
  */
void explain_lsof(const char *options, explain_lsof_t *context);

#endif /* LIBEXPLAIN_LSOF_H */
/* vim: set ts=8 sw=4 et : */
