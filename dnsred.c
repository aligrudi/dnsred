/*
 * dnsred: redirect DNS queries
 *
 * Copyright (C) 2015 Ali Gholami Rudi <ali at rudi dot ir>
 *
 * This program is released under the Modified BSD license.
 */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static int mksocket(char *addr, char *port, int tcp)
{
	struct addrinfo hints, *addrinfo;
	int fd;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = tcp ? SOCK_STREAM : SOCK_DGRAM;
	hints.ai_flags = addr ? 0 : AI_PASSIVE;
	if (getaddrinfo(addr, port, &hints, &addrinfo))
		return -1;
	fd = socket(addrinfo->ai_family, addrinfo->ai_socktype,
			addrinfo->ai_protocol);
	if (addr) {
		struct timeval to;
		to.tv_sec = 3;
		to.tv_usec = 0;
		if (connect(fd, addrinfo->ai_addr, addrinfo->ai_addrlen) < 0)
			return -1;
		setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof(to));
		setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &to, sizeof(to));
	} else {
		int yes = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
		if (bind(fd, addrinfo->ai_addr, addrinfo->ai_addrlen) < 0)
			return -1;
	}
	freeaddrinfo(addrinfo);
	return fd;
}

int main(int argc, char *argv[])
{
	char msg[2048];
	int msg_n;
	struct sockaddr_storage addr;
	unsigned addr_n = sizeof(addr);
	char *host, *port;
	int ifd;
	int tcp = 0;
	int i;
	for (i = 1; i < argc && argv[i][0] == '-'; i++) {
		switch (argv[i][1]) {
		case 't':
			tcp = 1;
			break;
		case 'u':
			tcp = 0;
			break;
		default:
			printf("usage: dnsred [options] dest_host dest_port\n");
			printf("options:\n");
			printf("  -u   send UDP requests\n");
			printf("  -t   send TCP requests\n");
			return 1;
		}
	}
	host = i < argc ? argv[i++] : "4.2.2.4";
	port = i < argc ? argv[i++] : "53";
	ifd = mksocket(NULL, "53", 0);
	while ((msg_n = recvfrom(ifd, msg, sizeof(msg), 0,
				(void *) &addr, &addr_n)) > 0) {
		int ofd = mksocket(host, port, tcp);
		int len = htons(msg_n);
		if (tcp)
			send(ofd, &len, 2, 0);
		send(ofd, msg, msg_n, 0);
		if (tcp)
			recv(ofd, &len, 2, 0);
		msg_n = recv(ofd, msg, tcp ? ntohs(len) : sizeof(msg), 0);
		close(ofd);
		if (msg_n > 0)
			sendto(ifd, msg, msg_n, 0, (void *) &addr, addr_n);
	}
	close(ifd);
	return 0;
}
