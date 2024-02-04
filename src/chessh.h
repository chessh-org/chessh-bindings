#ifndef HAVE_CHESSH_BINDINGS
#define HAVE_CHESSH_BINDINGS

typedef struct {
	int fd;
} CHESSH;

/*! \brief Connects to a chessh API endpoint
 * 
 * \param host The hostname of the API endpoint
 * \param port The port of the API endpoint
 * \see chessh_disconnect
 * \return The chessh connection object
 */
extern CHESSH *chessh_connect(char *host, int port);

/*! \brief Disconnects from and frees a chessh API endpoint
 *
 * \param connection The connection to close
 * \see chessh_connect
 */
extern void chessh_disconnect(CHESSH *connection);

#endif
