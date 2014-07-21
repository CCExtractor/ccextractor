#ifndef NETWORKING_H
#define NETWORKING_H

#include <sys/types.h>

void connect_to_srv(const char *addr, const char *port);

void net_send_header(const char *data, size_t len);
void net_send_cc(const char *data, size_t len);

#endif /* end of include guard: NETWORKING_H */
