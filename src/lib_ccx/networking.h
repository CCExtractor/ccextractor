#ifndef NETWORKING_H
#define NETWORKING_H

#include <sys/types.h>

void connect_to_srv(const char *addr, const char *port, const char *cc_desc, const char *pwd);

void net_send_header(const unsigned char *data, size_t len);
int net_send_cc(const unsigned char *data, int length, void *private_data, struct cc_subtitle *sub);

void net_check_conn();

void net_send_epg(
		const char *start,
		const char *stop,
		const char *title,
		const char *desc,
		const char *lang,
		const char *category
		);

int net_tcp_read(int socket, void *buffer, size_t length);
int net_udp_read(int socket, void *buffer, size_t length, const char *src_str, const char *addr_str);

int start_tcp_srv(const char *port, const char *pwd);

int start_upd_srv(const char *src, const char *addr, unsigned port);

#endif /* end of include guard: NETWORKING_H */
