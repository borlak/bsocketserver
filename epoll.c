#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <pthread.h>

int make_socket_non_blocking(int sfd)
{
	int flags, s;

	if((flags = fcntl(sfd, F_GETFL, 0)) == -1) {
		perror("make_socket_non_blocking (getfl): fcntl");
		return -1;
	}

	flags |= O_NONBLOCK;
	if((s = fcntl(sfd, F_SETFL, flags)) == -1) {
		perror ("make_socket_non_blocking (non_block): fcntl");
		return -1;
	}

	return 0;
}

int set_reuseaddr(int sfd)
{
	int optval = 1;
	
	if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))) {
		perror ("set_reuseaddr (SO_REUSEADDR): setsockopt");
		return -1;
	}

	return 0;
}

int set_nodelay(int sfd) {
	int optval = 1;
	
	if(setsockopt(sfd, SOL_SOCKET, TCP_NODELAY, &optval, sizeof(optval))) {
		perror ("set_reuseaddr (SO_REUSEADDR): setsockopt");
		return -1;
	}

	return 0;	
}

int create_server(int port)
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int s, sfd;
	char port_str[12] = {'\0'};
	
	sprintf(port_str, "%d", port);

	memset(&hints, 0, sizeof (struct addrinfo));
	hints.ai_family		= AF_UNSPEC;	/* Return IPv4 and IPv6 choices */
	hints.ai_socktype	= SOCK_STREAM;	/* We want a TCP socket */
	hints.ai_flags		= AI_PASSIVE;	/* All interfaces */

	if((s = getaddrinfo(NULL, port_str, &hints, &result)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror (s));
		return -1;
	}

	for(rp = result; rp != NULL; rp = rp->ai_next) {
		if ((sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1)
			continue;

		if(set_reuseaddr(sfd) == -1)
			abort();

		if((s = bind(sfd, rp->ai_addr, rp->ai_addrlen)) == 0)
			break;

		close(sfd);
	}

	if(rp == NULL) {
		fprintf(stderr, "Could not bind\n");
		return -1;
	}

	if ((s = make_socket_non_blocking(sfd)) == -1)
		return -1;

	if((s = listen(sfd, SOMAXCONN)) == -1) {
		perror("listen");
		return -1;
	}
	
	freeaddrinfo(result);
	printf("Server started on port %d using fd %d\n", port, sfd);
	return sfd;
}


