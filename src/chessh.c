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

#define LOGIN 0x00
#define MAKE_MOVE 0x01
#define GET_BOARD 0x02
#define GET_VALID_MOVES 0x03
#define INIT_GAME 0x04
#define BOARD_INFO 0x05
#define MOVE_INFO 0x06
#define NOTIFY 0x07

#define draw_offer 0x00
#define white_wins 0x01
#define black_wins 0x02
#define forced_draw 0x03
#define internal_server_error 0x04
#define your_turn 0x05
#define illegal_move 0x06
#define needs_promotion 0x07

static int create_socket(char const * const host, int const port);
static int send_string(CHESSH const * const endpoint, char const * const string);
static int get_move(CHESSH const * const endpoint, chessh_move *ret);

CHESSH *chessh_connect(char const * const host, int const port,
		char const * const user, char const * const pass) {
	CHESSH *ret;
	if ((ret = malloc(sizeof *ret)) == NULL) {
		goto error1;
	}
	if ((ret->fd = create_socket(host, port)) < 0) {
		goto error2;
	}
	if ((ret->file = fdopen(ret->fd, "r+")) == NULL) {
		goto error3;
	}

	if (send_string(ret, user) < 0 ||
	    send_string(ret, pass)) {
		goto error4;
	}

	return ret;
error4:
	fclose(ret->file);
error3:
	close(ret->fd);
error2:
	free(ret);
error1:
	return NULL;
}

void chessh_disconnect(CHESSH *connection) {
	close(connection->fd);
	free(connection);
}

int chessh_wait(CHESSH *connection, chessh_event * const event) {
	int type;
	type = fgetc(connection->file);
	if (type == EOF) {
		return -1;
	}
	switch (type) {
	/* None of these commands should ever get sent TO the client */
	case LOGIN: case GET_BOARD: case GET_VALID_MOVES: default:
		return -1;
	case MAKE_MOVE:
		event->type = CHESSH_EVENT_MOVE;
		return get_move(connection, &event->move.move);
	case INIT_GAME:
		event->type = CHESSH_EVENT_FOUND_OP;
		switch (fgetc(connection->file)) {
		case -1:
			return -1;
		case 0:
			event->found_op.player = CHESSH_WHITE;
			return 0;
		default:
			event->found_op.player = CHESSH_BLACK;
			return 0;
		}
		return -1;
	}
	return 0;
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
 * @param endpoint The CHESSH * endpoint to send the string through
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
	return fwrite((void *) buff, len+1, 1, endpoint->file) < 1 ? -1:0;
}

/*! @brief Parses a move from an endpoint
 *
 * @param endpoint The endpoint to get a move from
 * @param ret The return location of the move
 * @return 0 on success, -1 on failure
 */
static int get_move(CHESSH const * const endpoint, chessh_move *ret) {
	int c1 = fgetc(endpoint->file);
	int c2 = fgetc(endpoint->file);
	if (c1 == EOF || c2 == EOF || c1 > 0xff || c2 > 0xff) {
		return -1;
	}
	ret->r_i = (c1 >> 5) & 7;
	ret->c_i = (c1 >> 2) & 7;
	ret->has_promotion = c1 & 2;
	ret->padding = 0;
	ret->r_f = (c2 >> 5) & 7;
	ret->c_f = (c2 >> 2) & 7;
	ret->promotion = c2 & 3;
	return 0;
}
