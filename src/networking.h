#ifndef NETWORKING_H
#define NETWORKING_H

#include <sys/types.h>

void connect_to_srv(const char *addr, const char *port);

void net_send_header(const unsigned char *data, size_t len);
void net_send_cc(const unsigned char *data, size_t len);

int start_tcp_srv(const char *port, const char *pwd);

int start_upd_srv(const char *addr, unsigned port);

#endif /* end of include guard: NETWORKING_H */
