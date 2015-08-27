#include <ctype.h>
#include "stdh.h"
#include "command.h"

static const struct command commands[] = {
	{ "LOGIN",	RANK_ANY,	CONN_SOCKET,	LOG_NONE,	do_login	},
	{ 0, RANK_OWNER+1, CONN_FREED, LOG_NONE, 0 } // last command / null terminator
};

int parse_command(CONNECTION *connection, char *cmd_buf) {
	char cmd[32] = {'\0'};
	char *p = cmd;
	int i;

	// skip any preceding whitespace
	while(isspace(*cmd_buf) && *cmd_buf != '\0') {
		cmd_buf++;
	}
	// copy until whitespace or EOF
	while(!isspace(*cmd_buf) && *cmd_buf != '\0') {
		*p++ = *cmd_buf++;
	}
	*p = '\0';

	// check if command exists
	for(i = 0; commands[i].name; i++) {
		if(!strcmp(commands[i].name, cmd)) {
			break;
		}
	}

	if(commands[i].name == 0) {
		connection_write(connection, "Unknown command");
		return false;
	}

	if(!IsSet(commands[i].connection_states, connection->state)) {
		connection_write(connection, "Invalid connection state for command.");
		return false;
	}

	// check rank here
//	if(commands[i].rank ...)

	// call function
	(*commands[i].function) (connection);
	return true;
}

CMD(do_login) {
	connection_write(connection, "logging in");
}