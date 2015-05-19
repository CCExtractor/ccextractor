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
#pragma warning( suppress : 4005)
#define ERROR           51
#define UNKNOWN_COMMAND 52
#define WRONG_PASSWORD  53
#define CONN_LIMIT      54

#define DFT_PORT "2048" /* Default port for server and client */
#define WRONG_PASSWORD_DELAY 2 /* Seconds */
#define BUFFER_SIZE 50

int srv_sd = -1; /* Server socket descriptor */

/*
 * Established connection to speciefied addres.
 * Returns socked id
 */
int tcp_connect(const char *addr, const char *port);

/*
 * Asks password from stdin, sends it to the server and waits for
 * it's response
 */
int ask_passwd(int sd);

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

/* Convinence functions */
ssize_t write_byte(int fd, char status);
ssize_t read_byte(int fd, char *status);

void init_sockets (void);

#if DEBUG_OUT
void pr_command(char c);
#endif

void connect_to_srv(const char *addr, const char *port, const char *cc_desc)
{
	if (NULL == addr)
	{
		mprint("Server addres is not set\n");
		fatal(EXIT_FAILURE, "Unable to connect\n");
	}

	if (NULL == port)
		port = DFT_PORT;

	mprint("\n\r----------------------------------------------------------------------\n");
	mprint("Connecting to %s:%s\n", addr, port);

	if ((srv_sd = tcp_connect(addr, port)) < 0)
		fatal(EXIT_FAILURE, "Unable to connect\n");

	if (ask_passwd(srv_sd) < 0)
		fatal(EXIT_FAILURE, "Unable to connect\n");

	if (cc_desc != NULL &&
			write_block(srv_sd, CC_DESC, cc_desc, strlen(cc_desc)) < 0)
	{
		fatal(EXIT_FAILURE, "Unable to connect\n");
	}

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
	if (write_block(srv_sd, BIN_MODE, NULL, 0) <= 0)
	{
		printf("Can't send BIN header\n");
		return;
	}

	char ok;
	if (read_byte(srv_sd, &ok) != 1)
		return;

#if DEBUG_OUT
	fprintf(stderr, "[S] ");
	pr_command(ok);
	fprintf(stderr, "\n");
#endif

	if (ERROR == ok)
	{
		printf("Internal server error\n");
		return;
	}

	ssize_t rc;
	if ((rc = writen(srv_sd, data, len)) != (int) len)
	{
		if (rc < 0)
			mprint("write() error: %s", strerror(errno));
		return;
	}
}

int net_send_cc(const unsigned char *data, int len, void *private_data, struct cc_subtitle *sub)
{
	assert(srv_sd > 0);

#if DEBUG_OUT
	fprintf(stderr, "[C] Sending %u bytes\n", len);
#endif

	ssize_t rc;
	if ((rc = writen(srv_sd, data, len)) != (int) len)
	{
		if (rc < 0)
			mprint("write() error: %s", strerror(errno));
		return rc;
	}

	/* nanosleep((struct timespec[]){{0, 100000000}}, NULL); */
	/* Sleep(100); */
	return rc;
}

/*
 * command | lenght        | data         | \r\n
 * 1 byte  | INT_LEN bytes | lenght bytes | 2 bytes
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
	if (buf != NULL)
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

	/* Try each address until we sucessfully connect */
	for (p = ai; p != NULL; p = p->ai_next) {
		sockfd = socket(p->ai_family, SOCK_STREAM, p->ai_protocol);

		if (-1 == sockfd) {
#if _WIN32
			wprintf(L"socket() eror: %ld\n", WSAGetLastError());
#else
			mprint("socket() error: %s\n", strerror(errno));
#endif
			if (p->ai_next != NULL)
				mprint("trying next addres ...\n");

			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == 0)
			break;
#if _WIN32
		wprintf(L"connect() eror: %ld\n", WSAGetLastError());
#else
		mprint("connect() error: %s\n", strerror(errno));
#endif
		if (p->ai_next != NULL)
			mprint("trying next addres ...\n");

#if _WIN32
		closesocket(sockfd);
#else
		close(sockfd);
#endif
	}

	freeaddrinfo(ai);

	if (NULL == p)
		return -1;

	return sockfd;
}

int ask_passwd(int sd)
{
	assert(sd >= 0);

	size_t len;
	char pw[BUFFER_SIZE] = { 0 };

	char ok;

	do {
		do {
			if (read_byte(sd, &ok) != 1)
			{
				fatal(EXIT_FAILURE, "read() error: %s", strerror(errno));
			}

#if DEBUG_OUT
			fprintf(stderr, "[S] ");
			pr_command(ok);
			fprintf(stderr, "\n");
#endif

			if (OK == ok)
			{
				return 1;
			}
			else if (CONN_LIMIT == ok)
			{
				mprint("Too many connections to the server, try later\n");
				return -1;
			}
			else if (ERROR == ok)
			{
				mprint("Internal server error\n");
				return -1;
			}

		} while(ok != PASSWORD);

		printf("Enter password: ");
		fflush(stdout);

		char *p = pw;
		while ((unsigned)(p - pw) < sizeof(pw) && ((*p = fgetc(stdin)) != '\n'))
			p++;
		len = p - pw; /* without \n */

		if (write_block(sd, PASSWORD, pw, len) < 0)
			return -1;

		if (read_byte(sd, &ok) != 1)
			return -1;

#if DEBUG_OUT
		fprintf(stderr, "[S] ");
		pr_command(ok);
		fprintf(stderr, "\n");
#endif

		if (UNKNOWN_COMMAND == ok)
		{
			printf("Wrong password\n");
			fflush(stdout);
		}
		else if (ERROR == ok)
		{
			mprint("Internal server error\n");
			return -1;
		}
	} while(OK != ok);

	return 1;
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
			fatal(EXIT_FAILURE, "malloc() error: %s", strerror(errno));

		if ((sockfd = accept(listen_sd, cliaddr, &clilen)) < 0)
		{
			if (EINTR == errno) /* TODO not necessary */
			{
				continue;
			}
			else
			{
#if _WIN32
				wprintf(L"accept() eror: %ld\n", WSAGetLastError());
				exit(EXIT_FAILURE);
#else
				fatal(EXIT_FAILURE, "accept() error: %s\n", strerror(errno));
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
			mprint("%s:%s Connceted\n", host, serv);
		}

		free(cliaddr);

		if (pwd != NULL && (rc = check_password(sockfd, pwd)) <= 0)
			goto close_conn;

#if DEBUG_OUT
		fprintf(stderr, "[S] OK\n");
#endif
		if (write_byte(sockfd, OK) != 1)
			goto close_conn;

		char c;
		size_t len = BUFFER_SIZE;
		char buf[BUFFER_SIZE];

		do {
			if (read_block(sockfd, &c, buf, &len) <= 0)
				goto close_conn;
		} while (c != BIN_MODE);

#if DEBUG_OUT
		fprintf(stderr, "[S] OK\n");
#endif
		if (write_byte(sockfd, OK) != 1)
			goto close_conn;

		break;

close_conn:
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
	assert(pwd != NULL);

	char c;
	int rc;
	size_t len;
	char buf[BUFFER_SIZE];

	while(1)
	{
		len = BUFFER_SIZE;
#if DEBUG_OUT
		fprintf(stderr, "[S] PASSWORD\n");
#endif
		if ((rc = write_byte(fd, PASSWORD)) <= 0)
			return rc;

		if ((rc = read_block(fd, &c, buf, &len)) <= 0)
			return rc;

		if (c != PASSWORD)
			return -1;

		if (strlen(pwd) != len || strncmp(pwd, buf, len) != 0)
		{
			sleep(WRONG_PASSWORD_DELAY);

#if DEBUG_OUT
			fprintf(stderr, "[S] WRONG_PASSWORD\n");
#endif
			if ((rc = write_byte(fd, WRONG_PASSWORD)) <= 0)
				return rc;

			continue;
		}

		return 1;
	}
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
	/* Try each address until we sucessfully bind */
	for (p = ai; p != NULL; p = p->ai_next)
	{
		sockfd = socket(p->ai_family, SOCK_STREAM, p->ai_protocol);

		if (-1 == sockfd)
		{
#if _WIN32
			wprintf(L"socket() eror: %ld\n", WSAGetLastError());
#else
			mprint("socket() error: %s\n", strerror(errno));
#endif

			if (p->ai_next != NULL)
				mprint("trying next addres ...\n");

			continue;
		}

		if (AF_INET6 == p->ai_family)
		{
			int no = 0;
			if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&no, sizeof(no)) < 0)
			{
#if _WIN32
				wprintf(L"setsockopt() eror: %ld\n", WSAGetLastError());
#else
				mprint("setsockopt() error: %s\n", strerror(errno));
#endif

				if (p->ai_next != NULL)
					mprint("trying next addres ...\n");

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
				mprint("trying next addres ...\n");

			continue;
		}

		*family = p->ai_family;

		break;
	}

	freeaddrinfo(ai);

	if (NULL == p)
		return -1;

	if (listen(sockfd, SOMAXCONN) != 0)
	{
#if _WIN32
		wprintf(L"listen() eror: %ld\n", WSAGetLastError());
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
		fwrite(buf, sizeof(char), len, stderr);
		fprintf(stderr, " ");
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
			else
			{
#if _WIN32
				wprintf(L"recv() eror: %ld\n", WSAGetLastError());
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
#if _WIN32
				wprintf(L"send() eror: %ld\n", WSAGetLastError());
#else
				mprint("send() error: %s\n", strerror(errno));
#endif
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

int start_upd_srv(const char *addr_str, unsigned port)
{
	init_sockets();

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
		wprintf(L"socket() eror: %ld\n", WSAGetLastError());
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
	if (IN_MULTICAST(addr))
		servaddr.sin_addr.s_addr = htonl(addr);
	else
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
	{
#if _WIN32
		wprintf(L"bind() error: %ld\n", WSAGetLastError());
		exit(EXIT_FAILURE);
#else
		fatal(CCX_COMMON_EXIT_BUG_BUG, "bind() error: %s\n", strerror(errno));
#endif
	}

	if (IN_MULTICAST(addr)) {
		struct ip_mreq group;
		group.imr_multiaddr.s_addr = htonl(addr);
		group.imr_interface.s_addr = htonl(INADDR_ANY);
		if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0)
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
