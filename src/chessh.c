#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "chessh.h"

static int create_socket(char *host, int port);

CHESSH *chessh_connect(char *host, int port) {
	CHESSH *ret;
	if ((ret = malloc(sizeof *ret)) == NULL) {
		return NULL;
	}
	if ((ret->fd = create_socket(host, port)) < 0) {
		free(ret);
		return NULL;
	}
	return ret;
}

void chessh_disconnect(CHESSH *connection) {
	close(connection->fd);
	free(connection);
}

/*! \brief Creates a new client socket
 *
 * \param host The hostname of the server to connect to
 * \param port THe port of the server to connect to
 */
static int create_socket(char *host, int port) {
	char portstr[20];
	struct addrinfo hints;
	struct addrinfo *addr;
	int res, sockfd;

	snprintf(portstr, sizeof portstr - 1, "%d", port);
	portstr[sizeof portstr - 1] = '\0';

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	hints.ai_flags = 0;

	if ((res = getaddrinfo(host, portstr, &hints, &addr)) != 0) {
		return -1;
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	for (struct addrinfo *i = addr; i != NULL; i = i->ai_next) {
		if (connect(sockfd, i->ai_addr, i->ai_addrlen) == 0) {
			goto connected;
		}
	}
	close(sockfd);
	sockfd = -1;
connected:
	freeaddrinfo(addr);
	return sockfd;
}
