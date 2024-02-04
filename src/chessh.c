#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <poll.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "chessh.h"

static int create_socket(char const * const host, int const port);
static int send_string(CHESSH const * const endpoint, char const * const string);
static int write_resilient(int const fd, void const * const buff, size_t const len);

CHESSH *chessh_connect(char const * const host, int const port,
		char const * const user, char const * const pass) {
	CHESSH *ret;
	if ((ret = malloc(sizeof *ret)) == NULL) {
		goto error1;
	}
	if ((ret->fd = create_socket(host, port)) < 0) {
		goto error2;
	}

	if (send_string(ret, user) < 0 ||
	    send_string(ret, pass)) {
		goto error2;
	}

	return ret;
error2:
	free(ret);
error1:
	return NULL;
}

void chessh_disconnect(CHESSH *connection) {
	close(connection->fd);
	free(connection);
}

/*! @brief Creates a new client socket
 *
 * @param host The hostname of the server to connect to
 * @param port THe port of the server to connect to
 */
static int create_socket(char const * const host, int const port) {
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

/*! @brief Sends a string through a CHESSH * endpoint
 *
 * @param endpoint THe CHESSH * endpoint to send the string through
 * @param string The string to send
 */
static int send_string(CHESSH const * const endpoint, char const * const string) {
	size_t len;
	char buff[257];
	len = strlen(string);
	if (len >= 256) {
		return -1;
	}
	buff[0] = len;
	memcpy(buff+1, string, len);
	return write_resilient(endpoint->fd, (void *) buff, len+1);
}

/*! @brief Writes a buffer to a file, chaining multiple `write` calls if
 * necessary
 *
 * @param fd The file to send to
 * @param buff The buffer to write
 * @param len The size of the buffer
 */
static int write_resilient(int const fd, void const * const buff, size_t const len) {
	size_t progress = 0;
	char *cbuff = (char *) buff;

	while (progress < len) {
		ssize_t dp;
		dp = write(fd, cbuff + progress, len - progress);
		if (dp < 0) {
			if (errno == EINTR) {
				continue;
			}
			/* TODO: Slowloris prevention? */
			if (errno == EAGAIN) {
				struct pollfd pollfd;
				pollfd.fd = fd;
				pollfd.events = POLLOUT;
				poll(&pollfd, 1, 3000);
				if (!(pollfd.revents & POLLOUT)) {
					return -1;
				}
				continue;
			}
			return -1;
		}
		progress += dp;
	}

	return 0;
}
