#ifndef HAVE_CHESSH_BINDINGS
#define HAVE_CHESSH_BINDINGS

typedef enum {
	chessh_rook,
	chessh_knight,
	chessh_bishop,
	chessh_queen,
	chessh_king,
	chessh_pawn,
	chessh_empty,
} chessh_piece_type;

typedef enum {
	chessh_white,
	chessh_black
} chessh_player;

typedef struct {
	int r_i;
	int c_i;
	int r_f;
	int c_f;
} chessh_move;

typedef struct {
	chessh_player player : 1;
	chessh_piece_type piece : 3;
} chessh_piece;

typedef chessh_piece chessh_board[8][8];

typedef struct {
	int fd;
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

#endif
