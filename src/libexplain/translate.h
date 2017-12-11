/*
 * libexplain - a library of system-call-specific strerror replacements
 * Copyright (C) 2011, 2013 Peter Miller
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

#ifndef LIBEXPLAIN_TRANSLATE_H
#define LIBEXPLAIN_TRANSLATE_H

/**
  * The explain_translate_real_uid function may be used to translate the string
  * "real UID" and return the translated string.  Putting it in one place makes
  * for consistent translation.
  */
const char *explain_translate_real_uid(void);

/**
  * The explain_translate_effective_uid function may be used to translate the
  * string "effective UID" and return the translated string.  Putting it in one
  * place makes for consistent translation.
  */
const char *explain_translate_effective_uid(void);

/**
  * The explain_translate_real_gid function may be used to translate the string
  * "real GID" and return the translated string.  Putting it in one place makes
  * for consistent translation.
  */
const char *explain_translate_real_gid(void);

/**
  * The explain_translate_effective_gid function may be used to translate the
  * string "effective GID" and return the translated string.  Putting it in one
  * place makes for consistent translation.
  */
const char *explain_translate_effective_gid(void);

/**
  * The explain_translate_shared_mkemory_segment function may be used
  * to translate the string "shared memory segment" and return the
  * translated string.  Putting it in one place makes for consistent
  * translation.
  */
const char *explain_translate_shared_memory_segment(void);

/* vim: set ts=8 sw=4 et : */
#endif /* LIBEXPLAIN_TRANSLATE_H */
