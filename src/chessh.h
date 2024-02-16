#ifndef HAVE_CHESSH_BINDINGS
#define HAVE_CHESSH_BINDINGS

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
	CHESSH_ROOK,
	CHESSH_KNIGHT,
	CHESSH_BISHOP,
	CHESSH_QUEEN,
	CHESSH_KING,
	CHESSH_PAWN,
	CHESSH_EMPTY,
} chessh_piece_type;

typedef enum {
	CHESSH_WHITE,
	CHESSH_BLACK,
} chessh_player;

typedef struct {
	uint16_t r_i : 3;
	uint16_t c_i : 3;
	uint16_t has_promotion : 1;
	uint16_t padding : 1;
	uint16_t r_f : 3;
	uint16_t c_f : 3;
	uint16_t promotion : 2;
} chessh_move;

typedef struct {
	chessh_player player : 1;
	chessh_piece_type piece : 3;
} chessh_piece;

typedef chessh_piece chessh_board[8][8];

typedef enum {
	CHESSH_EVENT_MOVE,
	CHESSH_EVENT_FOUND_OP,
	CHESSH_EVENT_NOTIFY,
	CHESSH_EVENT_BOARD_INFO,
	CHESSH_EVENT_MOVE_INFO,
	CHESSH_EVENT_AUTH_RESPONSE,
	CHESSH_EVENT_UNKNOWN,
} chessh_event_type;

typedef enum {
	CHESSH_NOTIFY_DRAW_OFFER,
	CHESSH_NOTIFY_WHITE_WINS,
	CHESSH_NOTIFY_BLACK_WINS,
	CHESSH_NOTIFY_FORCED_DRAW,
	CHESSH_NOTIFY_INTERNAL_SERVER_ERROR,
	CHESSH_NOTIFY_YOUR_TURN,
	CHESSH_NOTIFY_ILLEGAL_MOVE,
	CHESSH_NOTIFY_NEEDS_PROMOTION,
	CHESSH_NOTIFY_LAST,
} chessh_notification;

typedef enum {
	CHESSH_REGISTRATION_SUCCESSFUL = 0x00,
	CHESSH_REGISTRATION_FAILED = 0x01,
	CHESSH_AUTHENTICATION_SUCCESSFUL = 0x80,
	CHESSH_AUTHENTICATION_FAILED = 0x81,
} chessh_auth_response_code;

typedef struct {
	chessh_event_type type; /* always CHESSH_EVENT_MOVE */
	chessh_move move;
} chessh_event_move;

typedef struct {
	chessh_event_type type; /* Always CHESSH_EVENT_FOUND_OP */
	chessh_player player;   /* Set to the player you're playing as, not against. */
} chessh_event_found_op;

typedef struct {
	chessh_event_type type; /* Always CHESSH_EVENT_NOTIFY */
	chessh_notification event;
} chessh_event_notify;

typedef struct {
	chessh_event_type type; /* Always CHESSH_EVENT_BOARD_INFO */
	chessh_board board;
} chessh_event_board_info;

/* XXX: You need to call chessh_get_move to actually get the moves. */
typedef struct {
	chessh_event_type type; /* Always CHESSH_EVENT_MOVE_INFO */
	long move_count;
} chessh_event_move_info;

typedef struct {
	chessh_event_type type; /* Always CHESSH_EVENT_AUTH_RESPONSE */
	chessh_auth_response_code code;
	char elaboration[256];
} chessh_event_auth_response;

typedef union {
	chessh_event_type type;
	chessh_event_move move;
	chessh_event_found_op found_op;
	chessh_event_notify notify;
	chessh_event_board_info board;
	chessh_event_move_info move_info;
	chessh_event_auth_response auth_response;
} chessh_event;

typedef struct {
	int fd;
	FILE *file;
	chessh_player player;
} CHESSH;

/*! @brief Connects to a chessh API endpoint
 * 
 * @param host The hostname of the API endpoint
 * @param port The port of the API endpoint
 * @see chessh_disconnect
 * @return The chessh connection object
 */
CHESSH *chessh_connect(char const * const host, int const port);

/*! @brief Logs in to a chessh API endpoint
 * 
 * @param endpoint The endpoint to log in to
 * @param user The user to log in as
 * @param pass The password of that user
 * @return 0 on success, -1 on failure
 * @see chessh_register
 */
int chessh_login(CHESSH *endpoint, char const * const user, char const * const pass);

/*! @brief Registers a new user with a chessh API endpoint
 * 
 * @param endpoint The endpoint to log in to
 * @param user The username to register
 * @param pass The password of that user
 * @return 0 on success, -1 on failure
 * @see chessh_login
 */
int chessh_register(CHESSH *endpoint, char const * const user, char const * const pass);

/*! @brief Disconnects from and frees a chessh API endpoint
 *
 * @param connection The connection to close
 * @see chessh_connect
 */
void chessh_disconnect(CHESSH *connection);

/*! @brief Waits for an event from the server
 *
 * @param connection The connection to wait for an event from
 * @param event The return location of the event
 * @return 0 on success, -1 on error
 */
int chessh_wait(CHESSH *connection, chessh_event * const event);

/*! @brief Parses a move from an endpoint
 *
 * @param endpoint The endpoint to get a move from
 * @param ret The return location of the move
 * @return 0 on success, -1 on failure
 */
int chessh_get_move(CHESSH const * const endpoint, chessh_move *ret);

/*! @brief Makes a chess move
 *
 * @param endpoint The endpoint to make a move on
 * @param move The move to make
 * @return 0 on success, -1 on failure
 */
int chessh_make_move(CHESSH const * const endpoint, chessh_move *move);

/*! @brief Requests the server to get the current board
 *
 * @param endpoint The endpoint to request a board for
 * @return 0 on success, -1 on failure
 */
int chessh_request_board(CHESSH const * const endpoint);

/*! @brief Requests the server to get the current valid moves
 *
 * @param endpoint The endpoint to request moves on
 * @return 0 on success, -1 on failure
 */
int chessh_request_moves(CHESSH const * const endpoint);

static inline chessh_player chessh_get_player(CHESSH const * const endpoint) {
	return endpoint->player;
}

/*! @brief Checks if a chessh endpoint has an event in the queue
 *
 * @param endpoint The endpoint to request moves on
 * @param timeout The time to wait for an event if there isn't one yet in
 * milliseconds
 * @return true if there is an event, false if there isn't. */
bool chessh_has_event(CHESSH const * const endpoint, int timeout);

#endif
