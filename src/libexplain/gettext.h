/*
 * libexplain - Explain errno values returned by libc functions
 * Copyright (C) 2008, 2009, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_GETTEXT_H
#define LIBEXPLAIN_GETTEXT_H

#ifndef i18n
#define i18n(x) x
#endif

/**
  * The explain_gettext function may be used to translate a string,
  * if i18n in in use.
  *
  * @param text
  *    The text to be translated (via gettext(3)).
  * @returns
  *    translated text, or the input argument if no translation available.
  */
const char *explain_gettext(const char *text);

#endif /* LIBEXPLAIN_GETTEXT_H */
/* vim: set ts=8 sw=4 et : */
