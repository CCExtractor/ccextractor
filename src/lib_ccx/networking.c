#include "lib_ccx.h"
#include "networking.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#define DEBUG_OUT 0

/* Protocol constants: */
#define INT_LEN         10
#define OK              1
#define PASSWORD        2
#define BIN_MODE        3
#define CC_DESC         4
#define BIN_HEADER      5
#define BIN_DATA        6
#define EPG_DATA        7
#pragma warning( suppress : 4005)
#define ERROR           51
#define UNKNOWN_COMMAND 52
#define WRONG_PASSWORD  53
#define CONN_LIMIT      54
#define PING            55

/* #include <time.h> */

#define DFT_PORT "2048" /* Default port for server and client */
#define WRONG_PASSWORD_DELAY 2 /* Seconds */
#define BUFFER_SIZE 50
#define NO_RESPONCE_INTERVAL 20
#define PING_INTERVAL 3

int srv_sd = -1; /* Server socket descriptor */

const char *srv_addr;
const char *srv_port;
const char *srv_cc_desc;
const char *srv_pwd;
unsigned char *srv_header;
size_t srv_header_len;

/*
 * Established connection to speciefied addres.
 * Returns socked id
 */
int tcp_connect(const char *addr, const char *port);

int check_password(int fd, const char *pwd);

int tcp_bind(const char *port, int *family);

/*
 * Writes/reads data according to protocol to descriptor
 * block format: * command | lenght        | data         | \r\n
 * 1 byte  | INT_LEN bytes | lenght bytes | 2 bytes
 */
ssize_t write_block(int fd, char command, const char *buf, size_t buf_len);
ssize_t read_block(int fd, char *command, char *buf, size_t *buf_len);

/* Reads n bytes from descriptor */
ssize_t readn(int fd, void *vptr, size_t n);

/* Writes n bytes to descriptor */
ssize_t writen(int fd, const void *vptr, size_t n);

/* Convenience functions */
ssize_t write_byte(int fd, char status);
ssize_t read_byte(int fd, char *status);

void init_sockets (void);

#if DEBUG_OUT
void pr_command(char c);
#endif

void handle_write_error();
int set_nonblocking(int fd);

void connect_to_srv(const char *addr, const char *port, const char *cc_desc, const char *pwd)
{
	if (NULL == addr)
	{
		mprint("Server address is not set\n");
		fatal(EXIT_FAILURE, "Unable to connect, address passed is null\n");
	}

	if (NULL == port)
		port = DFT_PORT;

	mprint("\n\r----------------------------------------------------------------------\n");
	mprint("Connecting to %s:%s\n", addr, port);

	if ((srv_sd = tcp_connect(addr, port)) < 0)
		fatal(EXIT_FAILURE, "connect_to_srv: Unable to connect (tcp_connect error).\n");

	if (write_block(srv_sd, PASSWORD, pwd, pwd ? strlen(pwd) : 0) < 0)
		fatal(EXIT_FAILURE, "connect_to_srv: Unable to connect (sending password).\n");

	if (write_block(srv_sd, CC_DESC, cc_desc, cc_desc ? strlen(cc_desc) : 0) < 0)
		fatal(EXIT_FAILURE, "connect_to_srv: Unable to connect (sending cc_desc).\n");

	srv_addr = addr;
	srv_port = port;
	srv_cc_desc = cc_desc;
	srv_pwd = pwd;

	mprint("Connected to %s:%s\n", addr, port);
}

void net_send_header(const unsigned char *data, size_t len)
{
	assert(srv_sd > 0);

#if DEBUG_OUT
	fprintf(stderr, "Sending header (len = %u): \n", len);
	fprintf(stderr, "File created by %02X version %02X%02X\n", data[3], data[4], data[5]);
	fprintf(stderr, "File format revision: %02X%02X\n", data[6], data[7]);
#endif

	if (write_block(srv_sd, BIN_HEADER, data, len) <= 0)
	{
		printf("Can't send BIN header\n");
		return;
	}

	if (srv_header != NULL)
		return;

	if ((srv_header = malloc(len)) == NULL)
		fatal(EXIT_FAILURE, "Not enough memory to send header");

	memcpy(srv_header, data, len);
	srv_header_len = len;
}

int net_send_cc(const unsigned char *data, int len, void *private_data, struct cc_subtitle *sub)
{
	assert(srv_sd > 0);

#if DEBUG_OUT
	fprintf(stderr, "[C] Sending %u bytes\n", len);
#endif

	if (write_block(srv_sd, BIN_DATA, data, len) <= 0)
	{
		printf("Can't send BIN data\n");
		return -1;
	}

	/* nanosleep((struct timespec[]){{0, 4000000}}, NULL); */
	/* Sleep(100); */
	return 1;
}

void net_check_conn()
{
	time_t now;
	static time_t last_ping = 0;
	char c = 0;
	int rc;

	if (srv_sd <= 0)
		return;

	now = time(NULL);

	if (last_ping == 0)
		last_ping = now;

	do {
		c = 0;
		rc = read_byte(srv_sd, &c);
		if (c == PING) {
#if DEBUG_OUT
			fprintf(stderr, "[S] Received PING\n");
#endif
			last_ping = now;
		}
	} while (rc > 0 && c == PING);

	if (now - last_ping > NO_RESPONCE_INTERVAL)
	{
		fprintf(stderr,
				"[S] No PING received from the server in %u sec, reconnecting\n",
				NO_RESPONCE_INTERVAL);
		close(srv_sd);
		srv_sd = -1;

		connect_to_srv(srv_addr, srv_port, srv_cc_desc, srv_pwd);

		net_send_header(srv_header, srv_header_len);
		last_ping = now;
	}

	static time_t last_send_ping = 0;
	if (now - last_send_ping >= PING_INTERVAL)
	{
		if (write_block(srv_sd, PING, NULL, 0) < 0)
		{
			printf("Unable to send data\n");
			exit(EXIT_FAILURE);
		}

		last_send_ping = now;
	}
}

void net_send_epg(
		const char *start,
		const char *stop,
		const char *title,
		const char *desc,
		const char *lang,
		const char *category
		)
{
	size_t st;
	size_t sp;
	size_t t;
	size_t d;
	size_t l;
	size_t c;
	size_t len;
	char *epg;
	char *end;

	/* nanosleep((struct timespec[]){{0, 100000000}}, NULL); */
	assert(srv_sd > 0);
	if (NULL == start)
		return;
	if (NULL == stop)
		return;

	st = strlen(start) + 1;
	sp = strlen(stop) + 1;

	t = 1;
	if (title != NULL)
		t += strlen(title);

	d = 1;
	if (desc != NULL)
		d += strlen(desc);

    l = 1;
    if (lang != NULL)
        l += strlen(lang);

    c = 1;
    if (category != NULL)
        c += strlen(category);

    len = st + sp + t + d + l + c;

	epg = (char *) calloc(len, sizeof(char));
	if (NULL == epg)
		return;

	end = epg;

	memcpy(end, start, st);
	end += st;

	memcpy(end, stop, sp);
	end += sp;

	if (title != NULL)
		memcpy(end, title, t);
	end += t;

	if (desc != NULL)
		memcpy(end, desc, d);
	end += d;

    if (lang != NULL)
        memcpy(end, lang, l);
    end += l;

    if (category != NULL)
        memcpy(end, category, c);
    end += c;

#if DEBUG_OUT
	fprintf(stderr, "[C] Sending EPG: %u bytes\n", len);
#endif

	if (write_block(srv_sd, EPG_DATA, epg, len) <= 0)
		fprintf(stderr, "Can't send EPG data\n");

    	free(epg);

	return;
}

int net_tcp_read(int socket, void *buffer, size_t length)
{
	assert(buffer != NULL);
	assert(length > 0);

	time_t now = time(NULL);
	static time_t last_ping = 0;
	if (last_ping == 0)
		last_ping = now;

	if (now - last_ping > PING_INTERVAL)
	{
		last_ping = now;
		if (write_byte(socket, PING) <= 0)
			fatal(EXIT_FAILURE, "Unable to send keep-alive packet to client\n");
	}

	int rc;
	char c;
	size_t l;

	do
	{
		l = length;

		if ((rc = read_block(socket, &c, buffer, &l)) <= 0)
			return rc;
	}
	while (c != BIN_DATA && c != BIN_HEADER);

	return l;
}

/*
 * command | length        | data         | \r\n
 * 1 byte  | INT_LEN bytes | length bytes | 2 bytes
 */
ssize_t write_block(int fd, char command, const char *buf, size_t buf_len)
{
	assert(fd > 0);
#if DEBUG_OUT
	fprintf(stderr, "[C] ");
#endif

	int rc;
	ssize_t nwritten = 0;

	if ((rc = write_byte(fd, command)) < 0)
		return -1;
	else if (rc != 1)
		return 0;
	nwritten++;

#if DEBUG_OUT
	pr_command(command);
	fprintf(stderr, " ");
#endif

	char len_str[INT_LEN] = {0};
	snprintf(len_str, INT_LEN, "%zu", buf_len);
	if ((rc = writen(fd, len_str, INT_LEN)) < 0)
		return -1;
	else if (rc != INT_LEN)
		return 0;
	nwritten += rc;

#if DEBUG_OUT
	fwrite(len_str, sizeof(char), INT_LEN, stderr);
	fprintf(stderr, " ");
#endif

	if (buf_len > 0)
	{
		if ((rc = writen(fd, buf, buf_len)) < 0)
			return -1;
		else if (rc != (int) buf_len)
			return 0;
		nwritten += rc;
	}

#if DEBUG_OUT
	if (buf != NULL && command != BIN_HEADER && command != BIN_DATA)
	{
		fwrite(buf, sizeof(char), buf_len, stderr);
		fprintf(stderr, " ");
	}
#endif

	if ((rc = write_byte(fd, '\r')) < 0)
		return -1;
	else if (rc != 1)
		return 0;
	nwritten++;

#if DEBUG_OUT
	fprintf(stderr, "\\r");
#endif

	if ((rc = write_byte(fd, '\n')) < 0)
		return -1;
	else if (rc != 1)
		return 0;
	nwritten++;

#if DEBUG_OUT
	fprintf(stderr, "\\n\n");
#endif

	return nwritten;
}

int tcp_connect(const char *host, const char *port)
{
	assert(host != NULL);
	assert(port != NULL);

	init_sockets();

	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	struct addrinfo *ai;
	int rc = getaddrinfo(host, port, &hints, &ai);
	if (rc != 0) {
		mprint("getaddrinfo() error: %s\n", gai_strerror(rc));
		return -1;
	}

	struct addrinfo *p;
	int sockfd;

	/* Try each address until we successfully connect */
	for (p = ai; p != NULL; p = p->ai_next) {
		sockfd = socket(p->ai_family, SOCK_STREAM, p->ai_protocol);

		if (-1 == sockfd) {
#if _WIN32
			wprintf(L"socket() error: %ld\n", WSAGetLastError());
#else
			mprint("socket() error: %s\n", strerror(errno));
#endif
			if (p->ai_next != NULL)
				mprint("trying next address ...\n");

			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == 0)
			break;
#if _WIN32
		wprintf(L"connect() error: %ld\n", WSAGetLastError());
#else
		mprint("connect() error: %s\n", strerror(errno));
#endif
		if (p->ai_next != NULL)
			mprint("trying next address ...\n");

#if _WIN32
		closesocket(sockfd);
#else
		close(sockfd);
#endif
	}

	freeaddrinfo(ai);

	if (NULL == p)
		return -1;

	if (set_nonblocking(sockfd) < 0)
		return -1;

	return sockfd;
}

int start_tcp_srv(const char *port, const char *pwd)
{
	if (NULL == port)
		port = DFT_PORT;

	mprint("\n\r----------------------------------------------------------------------\n");

	mprint("Binding to %s\n", port);
	int fam;
	int listen_sd = tcp_bind(port, &fam);
	if (listen_sd < 0)
		fatal(EXIT_FAILURE, "Unable to start server\n");

	if (pwd != NULL)
		mprint("Password: %s\n", pwd);

	mprint("Waiting for connections\n");

	int sockfd = -1;

	while (1)
	{
		socklen_t clilen;
		if (AF_INET == fam)
			clilen = sizeof(struct sockaddr_in);
		else
			clilen = sizeof(struct sockaddr_in6);
		struct sockaddr *cliaddr = (struct sockaddr *) malloc(clilen);
		if (NULL == cliaddr)
			fatal(EXIT_FAILURE, "In start_tcp_srv: Out of memory for client address. malloc() error: %s", strerror(errno));

		if ((sockfd = accept(listen_sd, cliaddr, &clilen)) < 0)
		{
			if (EINTR == errno) /* TODO not necessary */
			{   
                		free(cliaddr);
				continue;
			}
			else
			{
#if _WIN32
				wprintf(L"accept() error: %ld\n", WSAGetLastError());
				exit(EXIT_FAILURE);
#else
				fatal(EXIT_FAILURE, "In start_tcp_srv: accept() error: %s\n", strerror(errno));
#endif
			}
		}

		char host[NI_MAXHOST];
		char serv[NI_MAXSERV];
		int rc;
		if ((rc = getnameinfo(cliaddr, clilen,
						host, sizeof(host), serv, sizeof(serv), 0)) != 0)
		{
			mprint("getnameinfo() error: %s\n", gai_strerror(rc));
		}
		else
		{
			mprint("%s:%s Connected\n", host, serv);
		}

		free(cliaddr);

		if (check_password(sockfd, pwd) > 0)
			break;

		mprint("Connection closed\n");
#if _WIN32
		closesocket(sockfd);
#else
		close(sockfd);
#endif
	}

#if _WIN32
	closesocket(listen_sd);
#else
	close(listen_sd);
#endif

	return sockfd;
}

int check_password(int fd, const char *pwd)
{
	char c;
	int rc;
	size_t len = BUFFER_SIZE;
	static char buf[BUFFER_SIZE + 1];

	if ((rc = read_block(fd, &c, buf, &len)) <= 0)
		return rc;

	buf[len] = '\0';

	if (pwd == NULL)
		return 1;

	if (c == PASSWORD && strcmp(pwd, buf) == 0) {
		return 1;
	}

#if DEBUG_OUT
	fprintf(stderr, "[C] Wrong password\n");
#endif

#if DEBUG_OUT
	fprintf(stderr, "[S] PASSWORD\n");
#endif
	if (write_byte(fd, PASSWORD) < 0)
		return -1;
	return -1;
}

int tcp_bind(const char *port, int *family)
{
	init_sockets();

	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	struct addrinfo *ai;
	int rc = getaddrinfo(NULL, port, &hints, &ai);
	if (rc != 0)
	{
		mprint("getaddrinfo() error: %s\n", gai_strerror(rc));
		return -1;
	}

	struct addrinfo *p;
	int sockfd = -1;
	/* Try each address until we successfully bind */
	for (p = ai; p != NULL; p = p->ai_next)
	{
		sockfd = socket(p->ai_family, SOCK_STREAM, p->ai_protocol);

		if (-1 == sockfd)
		{
#if _WIN32
			wprintf(L"socket() error: %ld\n", WSAGetLastError());
#else
			mprint("socket() error: %s\n", strerror(errno));
#endif

			if (p->ai_next != NULL)
				mprint("trying next address ...\n");

			continue;
		}

		if (AF_INET6 == p->ai_family)
		{
			int no = 0;
			if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&no, sizeof(no)) < 0)
			{
#if _WIN32
				wprintf(L"setsockopt() error: %ld\n", WSAGetLastError());
#else
				mprint("setsockopt() error: %s\n", strerror(errno));
#endif

				if (p->ai_next != NULL)
					mprint("trying next address ...\n");

				continue;
			}
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0)
		{
#if _WIN32
			wprintf(L"bind() error: %ld\n", WSAGetLastError());
			closesocket(sockfd);
#else
			mprint("bind() error: %s\n", strerror(errno));
			close(sockfd);
#endif
			if (p->ai_next != NULL)
				mprint("trying next address ...\n");

			continue;
		}

		*family = p->ai_family;

		break;
	}

	freeaddrinfo(ai);

	if (NULL == p) // Went over all addresses, couldn't bind any
		return -1;

	if (listen(sockfd, SOMAXCONN) != 0)
	{
#if _WIN32
		wprintf(L"listen() error: %ld\n", WSAGetLastError());
		closesocket(sockfd);
#else
		perror("listen() error");
		close(sockfd);
#endif
		return -1;
	}

	return sockfd;
}

ssize_t read_block(int fd, char *command, char *buf, size_t *buf_len)
{
	assert(command != NULL);
	assert(buf != NULL);
	assert(buf_len != NULL);
	assert(*buf_len > 0);

	ssize_t rc;
	ssize_t nread = 0;

	if ((rc = readn(fd, command, 1)) < 0)
		return -1;
	else if ((size_t) rc != 1)
		return 0;
	nread += rc;

#if DEBUG_OUT
	fprintf(stderr, "[C] ");
	pr_command(*command);
	fprintf(stderr, " ");
#endif

	char len_str[INT_LEN] = {0};
	if ((rc = readn(fd, len_str, INT_LEN)) < 0)
		return -1;
	else if (rc != INT_LEN)
		return 0;
	nread += rc;

#if DEBUG_OUT
	fwrite(len_str, sizeof(char), INT_LEN, stderr);
	fprintf(stderr, " ");
#endif

	size_t len = atoi(len_str);

	if (len > 0)
	{
		size_t ign_bytes = 0;
		if (len > *buf_len)
		{
			ign_bytes = len - *buf_len;
			mprint("read_block() warning: Buffer overflow, ignoring %d bytes\n",
					ign_bytes);
			len = *buf_len;
		}

		if ((rc = readn(fd, buf, len)) < 0)
			return -1;
		else if ((size_t) rc != len)
			return 0;
		nread += rc;
		*buf_len = len;

		if ((rc = readn(fd, 0, ign_bytes)) < 0)
			return -1;
		else if ((size_t) rc != ign_bytes)
			return 0;
		nread += rc;

#if DEBUG_OUT
		if (*command != BIN_DATA && *command != BIN_HEADER)
		{
			fwrite(buf, sizeof(char), len, stderr);
			fprintf(stderr, " ");
		}
#endif
	}

	char end[2] = {0};
	if ((rc = readn(fd, end, sizeof(end))) < 0)
		return -1;
	else if ((size_t) rc != sizeof(end))
		return 0;
	nread += rc;

	if (end[0] != '\r' || end[1] != '\n')
	{
#if DEBUG_OUT
		fprintf(stderr, "read_block(): No end marker present\n");
		fprintf(stderr, "Closing connection\n");
#endif
		return 0;
	}

#if DEBUG_OUT
	fprintf(stderr, "\\r\\n\n");
#endif

	return nread;
}


#if DEBUG_OUT
void pr_command(char c)
{
	switch(c)
	{
		case OK:
			fprintf(stderr, "OK");
			break;
		case BIN_MODE:
			fprintf(stderr, "BIN_MODE");
			break;
		case WRONG_PASSWORD:
			fprintf(stderr, "WRONG_PASSWORD");
			break;
		case UNKNOWN_COMMAND:
			fprintf(stderr, "UNKNOWN_COMMAND");
			break;
		case ERROR:
			fprintf(stderr, "ERROR");
			break;
		case CONN_LIMIT:
			fprintf(stderr, "CONN_LIMIT");
			break;
		case PASSWORD:
			fprintf(stderr, "PASSWORD");
			break;
		case BIN_HEADER:
			fprintf(stderr, "BIN_HEADER");
			break;
		case BIN_DATA:
			fprintf(stderr, "BIN_DATA");
			break;
		case EPG_DATA:
			fprintf(stderr, "EPG_DATA");
			break;
		case PING:
			fprintf(stderr, "PING");
			break;
		default:
			fprintf(stderr, "UNKNOWN (%d)", (int) c);
			break;
	}
}
#endif

ssize_t readn(int fd, void *vptr, size_t n)
{
	assert(fd > 0);
	size_t nleft;
	ssize_t nread;
	char *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0)
	{
		if (NULL == vptr) {
			char c;
			nread = recv(fd, &c, 1, 0);
		}
		else
		{
			nread = recv(fd, (void*)ptr, nleft, 0);
		}

		if (nread < 0)
		{
			if (errno == EINTR)
			{
				nread = 0;
			}
			else if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				break;
			}
			else
			{
#if _WIN32
				wprintf(L"recv() error: %ld\n", WSAGetLastError());
#else
				mprint("recv() error: %s\n", strerror(errno));
#endif
				return -1;
			}
		}
		else if (0 == nread)
		{
			break; /* EOF */
		}

		nleft -= nread;
		ptr += nread;
	}

	return n - nleft;
}

ssize_t writen(int fd, const void *vptr, size_t n)
{
	assert(fd > 0);
	assert((n > 0 && vptr != NULL) || (n == 0 && vptr == NULL));

	size_t nleft;
	ssize_t nwritten;
	const char *ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0)
	{
		if ((nwritten = send(fd, ptr, nleft, 0)) < 0)
		{
			if (errno == EINTR)
			{
				nwritten = 0;
			}
			else
			{
				handle_write_error();
				return -1;
			}
		}
		else if (0 == nwritten)
		{
			break;
		}

		nleft -= nwritten;
		ptr += nwritten;
	}

	return n;
}

ssize_t write_byte(int fd, char ch)
{
	assert(fd > 0);

	return writen(fd, &ch, 1);
}

ssize_t read_byte(int fd, char *ch)
{
	assert(fd > 0);
	assert(ch != NULL);

	return readn(fd, ch, 1);
}

int start_upd_srv(const char *src_str, const char *addr_str, unsigned port)
{
	init_sockets();

	in_addr_t src;
	if (src_str != NULL)
	{
		struct hostent *host = gethostbyname(src_str);
		if (NULL == host)
		{
			fatal(EXIT_MALFORMED_PARAMETER, "Cannot look up udp network address: %s\n",
					src_str);
		}
		else if (host->h_addrtype != AF_INET)
		{
			fatal(EXIT_MALFORMED_PARAMETER, "No support for non-IPv4 network addresses: %s\n",
					src_str);
		}

		src = ntohl(((struct in_addr *)host->h_addr_list[0])->s_addr);
	}

	in_addr_t addr;
	if (addr_str != NULL)
	{
		struct hostent *host = gethostbyname(addr_str);
		if (NULL == host)
		{
			fatal(EXIT_MALFORMED_PARAMETER, "Cannot look up udp network address: %s\n",
					addr_str);
		}
		else if (host->h_addrtype != AF_INET)
		{
			fatal(EXIT_MALFORMED_PARAMETER, "No support for non-IPv4 network addresses: %s\n",
					addr_str);
		}

		addr = ntohl(((struct in_addr *)host->h_addr_list[0])->s_addr);
	}
	else
	{
		addr = INADDR_ANY;
	}

	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (-1 == sockfd) {
#if _WIN32
		wprintf(L"socket() error: %ld\n", WSAGetLastError());
		exit(EXIT_FAILURE);
#else
		mprint("socket() error: %s\n", strerror(errno));
#endif
	}

	if (IN_MULTICAST(addr))
	{
		int on = 1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0)
		{
#if _WIN32
			wprintf(L"setsockopt() error: %ld\n", WSAGetLastError());
#else
			mprint("setsockopt() error: %s\n", strerror(errno));
#endif
		}
	}

	struct sockaddr_in servaddr;
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
#if _WIN32
	// Doesn't seem correct, if there's more than one multicast stream with the same
	// port number we get corruption - IP address needs to be specified, but 
	// in Windows we get an error 10049 (cannot bind).
	// http ://stackoverflow.com/questions/6140734/cannot-bind-to-multicast-address-windows
	servaddr.sin_addr.s_addr = htonl(IN_MULTICAST(addr) ? INADDR_ANY : addr);
#else
	servaddr.sin_addr.s_addr = htonl(addr);
#endif

	if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
	{
#if _WIN32
		wprintf(L"bind() error: %ld\n", WSAGetLastError());
		exit(EXIT_FAILURE);
#else
		fatal(CCX_COMMON_EXIT_BUG_BUG, "In start_upd_srv: bind() error: %s\n", strerror(errno));
#endif
	}

	if (IN_MULTICAST(addr)) {
		int setsockopt_return = 0;
		if (src_str != NULL) {
			struct ip_mreq_source multicast_req;
			multicast_req.imr_sourceaddr.s_addr = htonl(src);
			multicast_req.imr_multiaddr.s_addr = htonl(addr);
			multicast_req.imr_interface.s_addr = htonl(INADDR_ANY);
			setsockopt_return = setsockopt(sockfd, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP, (char *)&multicast_req, sizeof(multicast_req));
		}
		else
		{
			struct ip_mreq multicast_req;
			multicast_req.imr_multiaddr.s_addr = htonl(addr);
			multicast_req.imr_interface.s_addr = htonl(INADDR_ANY);
			setsockopt_return = setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&multicast_req, sizeof(multicast_req));
		}

		if (setsockopt_return < 0)
		{
#if _WIN32
			wprintf(L"setsockopt() error: %ld\n", WSAGetLastError());
#else
			mprint("setsockopt() error: %s\n", strerror(errno));
#endif
			fatal(CCX_COMMON_EXIT_BUG_BUG, "Cannot join multicast group.");
		}
	}

	mprint("\n\r----------------------------------------------------------------------\n");
	if (addr == INADDR_ANY)
	{
		mprint("\rReading from UDP socket %u\n", port);
	}
	else if (src_str != NULL)
	{
		struct in_addr source;
		struct in_addr group;
		char src_ip[15];
		char addr_ip[15];
		source.s_addr = htonl(src);
		memset(src_ip, 0, sizeof(char) * 15);
		memcpy(src_ip, inet_ntoa(source), sizeof(src_ip));
		group.s_addr = htonl(addr);
		memset(addr_ip, 0, sizeof(char) * 15);
		memcpy(addr_ip, inet_ntoa(group), sizeof(addr_ip));

		mprint("\rReading from UDP socket %s@%s:%u\n", src_ip, addr_ip, port);
	}
	else
	{
		struct in_addr in;
		in.s_addr = htonl(addr);
		mprint("\rReading from UDP socket %s:%u\n", inet_ntoa(in), port);
	}

	return sockfd;
}

void init_sockets (void)
{
	static int socket_inited = 0;
	if (!socket_inited)
	{
		// Initialize Winsock
#ifdef _WIN32
		WSADATA wsaData = {0};
		int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iResult != 0)
		{
			wprintf(L"WSAStartup failed: %d\n", iResult);
			exit(EXIT_FAILURE);
		}
#endif

		socket_inited = 1;
	}
}

void handle_write_error()
{
#if _WIN32
	long err = WSAGetLastError();
#else
	char *err = strerror(errno);
#endif

	if (srv_sd < 0)
		return;

	char c = 0;
	int rc;
	do {
		c = 0;
		rc = read_byte(srv_sd, &c);
		if (rc < 0)
		{
#if _WIN32
			wprintf(L"send() error: %ld\n", err);
#else
			mprint("send() error: %s\n", err);
#endif
			return;
		}
	} while (rc > 0 && c == PING);

	switch (c)
	{
		case PASSWORD:
			mprint("Wrong password (use -tcppassword)\n");
			break;
		case CONN_LIMIT:
			mprint("Too many connections to the server, please wait\n");
			break;
		case ERROR:
			mprint("Internal server error");
			break;
		default:
#if _WIN32
			wprintf(L"send() error: %ld\n", err);
#else
			mprint("send() error: %s\n", err);
#endif
			break;
	}

	return;
}

int set_nonblocking(int fd)
{
    int f;
#ifdef O_NONBLOCK
    if ((f = fcntl(fd, F_GETFL, 0)) < 0)
        f = 0;

    return fcntl(fd, F_SETFL, f | O_NONBLOCK);
#else
    f = 1;
	#if _WIN32
		return ioctlsocket(fd, FIONBIO, &f);
	#else
		return ioctl(fd, FIONBIO, &f);
	#endif
#endif
}
