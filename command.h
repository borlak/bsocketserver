#include "connection.h"

#ifndef __COMMAND_H
#define __COMMAND_H

#define CMD(x) void (x)(CONNECTION *connection, ...)
#define RANK_ANY		0	// not logged in with a player
#define RANK_PLAYER		1	// logged in with a player
#define RANK_BUILDER	2
#define RANK_ADMIN		3
#define RANK_OWNER		4

int parse_command(CONNECTION *connection, char *cmd_buf);

CMD(do_login);

struct command {
	char *name;
	int rank;				// ACL for command
	int connection_states;	// what connection states can use this command
	int log;				// log level
	void (*function)(CONNECTION *connection, ...);
};

#endif // __COMMAND_H

