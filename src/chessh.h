#ifndef HAVE_CHESSH_BINDINGS
#define HAVE_CHESSH_BINDINGS

#include <stdio.h>
#include <stdint.h>

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
	CHESSH_BLACK
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
	CHESSH_EVENT_UNKNOWN
} chessh_event_type;

typedef struct {
	chessh_event_type type; /* always CHESSH_EVENT_MOVE */
	chessh_move move;
} chessh_event_move;

typedef struct {
	chessh_event_type type; /* Always CHESSH_EVENT_FOUND_OP */
	chessh_player player;   /* Set to the player you're playing as, not against. */
} chessh_event_found_op;

typedef union {
	chessh_event_type type;
	chessh_event_move move;
	chessh_event_found_op found_op;
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
 * @param user The user to log in as
 * @param pass The password of said user
 * @see chessh_disconnect
 * @return The chessh connection object
 */
CHESSH *chessh_connect(char const * const host, int const port,
		char const * const user, char const * const pass);

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

#endif
