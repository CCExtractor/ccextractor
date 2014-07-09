#ifndef NETWORKING_H
#define NETWORKING_H

#include <sys/types.h>

void connect_to_srv(const char *addr, const char *port);

void net_append_cc(const char *fmt, ...);
void net_append_cc_n(const char *data, size_t len);
void net_send_cc();

void net_set_new_program(const char *name, size_t len);

#endif /* end of include guard: NETWORKING_H */
