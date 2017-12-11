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

#ifndef LIBEXPLAIN_BUFFER_EPERM_H
#define LIBEXPLAIN_BUFFER_EPERM_H

#include <libexplain/string_buffer.h>

/**
  * The explain_buffer_eperm-setgid function may be used to print an
  * explanation of an EPERM error, for getgid or getgroups system calls.
  *
  * @param sb
  *     The string buffer to print into.
  */
void explain_buffer_eperm_setgid(explain_string_buffer_t *sb);

void explain_buffer_eperm_accept(explain_string_buffer_t *sb);

void explain_buffer_eperm_kill(explain_string_buffer_t *sb);

/**
  * The explain_buffer_eperm_sys_time function may be used to print an
  * explanation of an EPERM error, for syscalls that need CAP_SYS_TIME.
  *
  * @param sb
  *     The string buffer to print into.
  * @param syscall_name
  *     The name of the offending system call.
  */
void explain_buffer_eperm_sys_time(explain_string_buffer_t *sb,
    const char *syscall_name);

/**
  * The explain_buffer_eperm_sys_tty_config function may be used to print an
  * explanation of an EPERM error, for syscalls that need CAP_SYS_TTY_CONFIG.
  *
  * @param sb
  *     The string buffer to print into.
  * @param syscall_name
  *     The name of the offending system call.
  */
void explain_buffer_eperm_sys_tty_config(explain_string_buffer_t *sb,
    const char *syscall_name);

/**
  * The explain_buffer_eperm_net_admin function may be used to print an
  * explanation of an EPERM error, for syscalls that need CAP_NET_ADMIN
  * capability.
  *
  * @param sb
  *     The string buffer to print into.
  * @param syscall_name
  *     The name of the offending system call.
  */
void explain_buffer_eperm_net_admin(explain_string_buffer_t *sb,
    const char *syscall_name);

/**
  * The explain_buffer_eperm_mknod function is use to print an explanation
  * of a EPERM error, in the case where the file system is incapable of
  * making a node of the type specified.
  *
  * @param sb
  *     The string buffer to print into.
  * @param pathname
  *     The path of the attempted node creation.
  * @param pathname_caption
  *     The name of the syscall argument containinbg the path of the
  *     attempted node creation.
  * @param mode
  *     The S_IFMT bits describe what sort of node was attempted.
  */
void explain_buffer_eperm_mknod(explain_string_buffer_t *sb,
    const char *pathname, const char *pathname_caption, int mode);

/**
  * The explain_buffer_eperm_unlink functions is used to print an
  * explanation of a EPERM error, in the case where the sticky bit has
  * been set.
  *
  * @param sb
  *     The string buffer to print into.
  * @param pathname
  *     The path of the attempted node creation.
  * @param pathname_caption
  *     The name of the syscall argument containinbg the path of the
  *     attempted node creation.
  * @param syscall_name
  *     The name of the offended system call.
  */
void explain_buffer_eperm_unlink(explain_string_buffer_t *sb,
    const char *pathname, const char *pathname_caption,
    const char *syscall_name);

/**
  * The explain_buffer_eperm_unlink functions is used to print an
  * explanation of a EPERM error, in the case where the circumstances
  * are a bit vague or non-specific.
  *
  * @param sb
  *     The string buffer to print into.
  * @param syscall_name
  *     The name of the offended system call.
  */
void explain_buffer_eperm_vague(explain_string_buffer_t *sb,
    const char *syscall_name);

/**
  * The explain_buffer_eperm_cap_sys_admin function is used to print an
  * explanation of a EPERM error, in the case where the CAP_SYS_ADMIN
  * capability ids required.
  *
  * @param sb
  *     The string buffer to print into.
  * @param syscall_name
  *     The name of the offended system call.
  */
void explain_buffer_eperm_cap_sys_admin(explain_string_buffer_t *sb,
    const char *syscall_name);

#endif /* LIBEXPLAIN_BUFFER_EPERM_H */
/* vim: set ts=8 sw=4 et : */
