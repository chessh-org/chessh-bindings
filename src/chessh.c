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
#define REGISTER 0x08
#define AUTH_RESPONSE 0x09

static int create_socket(char const * const host, int const port);
static int send_string(CHESSH const * const endpoint, char const * const string);
static int get_board(CHESSH const * const endpoint, chessh_board *ret);
static int get_two_pieces(CHESSH const * const endpoint, chessh_board *ret, int r, int c);
static long get_word(CHESSH const * const endpoint);
static int chessh_auth(CHESSH const * const endpoint, unsigned char cmd,
		char const * const user, char const * const pass);
static int get_string(CHESSH const * const endpoint, char *ret);

CHESSH *chessh_connect(char const * const host, int const port) {
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

	return ret;
error3:
	close(ret->fd);
error2:
	free(ret);
error1:
	return NULL;
}

int chessh_login(CHESSH *endpoint, char const * const user, char const * const pass) {
	return chessh_auth(endpoint, LOGIN, user, pass);
}

int chessh_register(CHESSH *endpoint, char const * const user, char const * const pass) {
	return chessh_auth(endpoint, REGISTER, user, pass);
}

void chessh_disconnect(CHESSH *connection) {
	close(connection->fd);
	free(connection);
}

int chessh_wait(CHESSH *connection, chessh_event * const event) {
	int type;
	int c;
	type = fgetc(connection->file);
	if (type == EOF) {
		return -1;
	}
	switch (type) {
	/* None of these commands should ever get sent TO the client */
	case LOGIN: case GET_BOARD: case GET_VALID_MOVES: case REGISTER: default:
		do {
			putchar(type);
			type = fgetc(connection->file);
		} while (type != EOF);
		return -1;
	case MAKE_MOVE:
		event->type = CHESSH_EVENT_MOVE;
		return chessh_get_move(connection, &event->move.move);
	case INIT_GAME:
		event->type = CHESSH_EVENT_FOUND_OP;
		switch (fgetc(connection->file)) {
		case EOF:
			return -1;
		case 0:
			event->found_op.player = CHESSH_WHITE;
			return 0;
		default:
			event->found_op.player = CHESSH_BLACK;
			return 0;
		}
		return -1;
	case NOTIFY:
		event->type = CHESSH_EVENT_NOTIFY;
		c = fgetc(connection->file);
		if (c == EOF || c < 0 || c >= CHESSH_NOTIFY_LAST) {
			return -1;
		}
		event->notify.event = c;
		return 0;
	case BOARD_INFO:
		event->type = CHESSH_EVENT_BOARD_INFO;
		return get_board(connection, &event->board.board);
	case MOVE_INFO:
		event->type = CHESSH_EVENT_MOVE_INFO;
		event->move_info.move_count = get_word(connection);
		return event->move_info.move_count < 0 ? -1 : 0;
	case AUTH_RESPONSE:
		event->type = CHESSH_EVENT_AUTH_RESPONSE;
		event->auth_response.code = fgetc(connection->file);
		if (get_string(connection, event->auth_response.elaboration)) {
			return -1;
		}
		return 0;
	}
	return -1;
}

int chessh_make_move(CHESSH const * const endpoint, chessh_move *move) {
	int c1, c2;
	c1 = (move->r_i << 5) | (move->c_i << 2) | (move->has_promotion << 1);
	c2 = (move->r_f << 5) | (move->c_f << 2) | (move->promotion);
	if (fputc(c1, endpoint->file) == EOF ||
	    fputc(c2, endpoint->file) == EOF) {
		return -1;
	}
	return 0;
}

int chessh_request_board(CHESSH const * const endpoint) {
	return fputc(GET_BOARD, endpoint->file) == EOF ? -1 : 0;
}

int chessh_request_moves(CHESSH const * const endpoint) {
	return fputc(GET_VALID_MOVES, endpoint->file) == EOF ? -1 : 0;
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
int chessh_get_move(CHESSH const * const endpoint, chessh_move *ret) {
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

/*! @brief Gets a board from an endpoint
 *
 * @param endpoint The endpoint to get a board from
 * @param ret The return location of the board
 * @return 0 on success, -1 on failure */
static int get_board(CHESSH const * const endpoint, chessh_board *ret) {
	int code;
	for (int i = 0; i < 8; ++i) {
		for (int j = 0; j < 8; j += 2) {
			if ((code = get_two_pieces(endpoint, ret, i, j)) != 0) {
				return code;
			}
		}
	}
	return 0;
}

/*! @brief Gets two pieces from a single byte of an endpoint
 *
 * @param endpoint The endpoint to get two pieces from
 * @param ret The board to write those two pieces two
 * @param r The row that the two pieces lie on
 * @param c The column that the first of the two pieces lie on
 * @return 0 on success, -1 on failure
 */
static int get_two_pieces(CHESSH const * const endpoint, chessh_board *ret, int r, int c) {
	int ch;
	if (r < 0 || r >= 8 || c < 0 || c >= 7) {
		return -1;
	}
	ch = fgetc(endpoint->file);
	if (ch == EOF || ch < 0 || ch > 0xff) {
		return -1;
	}
	(*ret)[r][c].player   = (ch >> 7 ) & 1;
	(*ret)[r][c].piece    = (ch >> 4 ) & 7;
	(*ret)[r][c+1].player = (ch >> 3 ) & 1;
	(*ret)[r][c+1].piece  = (ch >> 0 ) & 7;
	return 0;
}

/*! @brief Gets a 2 byte word from a chessh endpoint
 *
 * @param endpoint The endpoint to get a word from
 * @return The word on success, or -1 on failure */
static long get_word(CHESSH const * const endpoint) {
	int c1, c2;
	c1 = fgetc(endpoint->file);
	c2 = fgetc(endpoint->file);
	if (c1 == EOF || c2 == EOF || c1 < 0 || c2 < 0 || c1 > 0xff || c2 > 0xff) {
		return -1;
	}
	return (long) c1 << 8 | c2;
}

/*! @brief Sends an initial request (either login or register)
 *
 * @param endpoint The endpoint to send login information to
 * @param cmd The command associated with that login information
 * @param user The username of the login
 * @param pass The password of the login
 */
static int chessh_auth(CHESSH const * const endpoint, unsigned char cmd,
		char const * const user, char const * const pass) {
	if (fputc(cmd, endpoint->file) == EOF) {
		return -1;
	}
	if (send_string(endpoint, user) ||
	    send_string(endpoint, pass)) {
		return -1;
	}
	return 0;
}

/*! @brief Gets a string from an endpoint
 * 
 * @param endpoint The endpoint ot get a string from
 * @param ret The location to store the string. `ret` must be at least 256 bytes
 * large.
 */
static int get_string(CHESSH const * const endpoint, char *ret) {
	int len;
	len = fgetc(endpoint->file);
	if (len < 0x00 || len > 0xff) {
		return -1;
	}
	if (fread(ret, len, 1, endpoint->file) < 1) {
		return -1;
	}
	return 0;
}
