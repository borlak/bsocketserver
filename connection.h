#ifndef __CONNECTION_H
#define __CONNECTION_H

#define CONNECTION_HASH_KEY		36787	// large prime

// states
#define CONN_CREATED		0x00000000	// just created, useless
#define CONN_SOCKET			0x00000001	// just connected, socket, not logged in
#define CONN_LOGGED_IN		0x00000002	// logged in
#define CONN_CHAR_CREATION	0x00000004	// char creation process
#define CONN_PLAYING		0x00000008	// playing the game
#define CONN_QUIT			0x00000010	// has just quit, no player, just socket
#define CONN_FREED			0x80000000	// connection is freed, nothing should use it

// type of connection
#define CONN_TYPE_UNDEFINED		0
#define CONN_TYPE_TCP			1	// tcp socket
#define CONN_TYPE_DGRAM			2	// datagram socket
#define CONN_TYPE_HTTP			3	// http (get) request
#define CONN_TYPE_BOSH			4	// bidirectional(streams)-over-synchronous-http
#define CONN_TYPE_WEBSOCKET		5	// http websockets
#define CONN_TYPE_FLASHSOCKET	6	// sockets through Adobe Flash

#define CONNECTION_MAX_BUFFER	4096

CONNECTION *connection_create(long fd);
CONNECTION *connection_get(long fd);
int connection_free(long fd);
void connection_close(long fd);
int connection_send(CONNECTION *connection);
int connection_write(CONNECTION *connection, char *str);
void connection_update(int seconds);
int connection_read(CONNECTION *connection, char *str);

struct connection {
	CONNECTION *next_hash;
	CONNECTION *prev_hash;
	CONNECTION *next;
	CONNECTION *prev;
	PROFILE *profile;

	BUFFER_LIST	*buffer_list;
	SMART_BUF	*in;
	SMART_BUF	*out;
	int			fd;
	int			state;
	int			type;
	int			cleanup_ready;
	
	/* last time connection was updated / data was sent.  this is used to help determine
	 * if a connection is timed out or hanging
	 */
	time_t last_updated;
	
	/* number of requests a particular connection has handled, ie. in the case of keep-alive */
	int requests;
};

#endif
