#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "stdh.h"
#include "macros.h"
#include "smart_buf.h"
#include "profile.h"
#include "buffer_list.h"
#include "connection.h"
#include "command.h"

CONNECTION *connection_hash[CONNECTION_HASH_KEY]	= {0};
CONNECTION *connection_free_list					= 0;
CONNECTION *connection_update_list					= 0;

long connection_count = 0;
unsigned long long bytes_sent = 0;
unsigned long long bytes_polled = 0;

extern int essfd;

CONNECTION *connection_create(long fd) {
	CONNECTION *connection;
	
	NewObject(connection_free_list, connection, CONNECTION)	

	connection_count++;
	connection->state			= CONN_CREATED;
	connection->fd				= fd;
	connection->in				= smart_buf_create(1024);
	connection->out				= smart_buf_create(1024);
	connection->last_updated	= get_time();
	connection->requests		= 0;
	connection->cleanup_ready	= 0;

	if(settings.profiling)
		connection->profile = profile_create();
	
	AddToHashList(connection_hash, connection, fd, CONNECTION_HASH_KEY)
	
	logit(LOG_DEBUG, "Created connection %d", fd);
	
	return connection;
}

CONNECTION *connection_get(long fd) {
	CONNECTION *connection;
	for(connection = connection_hash[fd%CONNECTION_HASH_KEY]; connection; connection = connection->next_hash)
		if(fd == connection->fd)
			break;
	
	if(connection)
		logit(LOG_DEBUG, "connection_get: Found connection %d",fd);
	else
		logit(LOG_DEBUG, "connection_get: failed to find connection %d",fd);
	
	return connection;
}

int connection_free(long fd) {
	CONNECTION *connection = connection_get(fd);
	
	if(connection) {
		RemoveFromHashList(connection_hash, connection, fd, CONNECTION_HASH_KEY)

		smart_buf_free(connection->in);
		smart_buf_free(connection->out);

		while(connection->buffer_list)
			buffer_list_free(connection, connection->buffer_list);

		if(settings.profiling)
			profile_free(connection->profile);
		
		connection->state = CONN_FREED;
		connection_count--;

		AddToList(connection_free_list, connection)
		
		return true;
	}
	return false;
}

void connection_close(long fd) {
	logit(LOG_DEBUG, "connection_close: closing connection %d", fd);
	if(connection_free(fd)) {
		struct epoll_event event;
		
		if(epoll_ctl(essfd, EPOLL_CTL_DEL, fd, &event) == -1) {
			perror ("epoll_ctl");
			abort ();
		}
		close(fd);
	}
}

int connection_send(CONNECTION *connection) {
	int len = strlen(connection->out->buf);
	bytes_sent += len;
	connection->last_updated = get_time();
	if(send(connection->fd, connection->out->buf, len, MSG_DONTWAIT) < 0) {
		return -1;
	}
	smart_buf_clear(connection->out);
	return true;
}

/**
 * Go through every connection and make sure they have a last_updated of at least
 * or less than arg1 seconds.  If not, close the connection.  
 * This is a prevention for a "connection attack" or simply to help deal lingering
 * connections.
 */
void connection_update(int seconds) {
	CONNECTION *connection, *connection_next;
	BUFFER_LIST *buffer_list;
	int i;
	int commands;
	time_t current_time = get_time();
	
	for(i = 0; i < CONNECTION_HASH_KEY; i++) {
		for(connection = connection_hash[i%CONNECTION_HASH_KEY]; connection; connection = connection_next) {
			connection_next = connection->next_hash;
			commands = 0;

			// check if socket is sitting idle
			if(current_time - connection->last_updated > seconds) {
				connection_close(connection->fd);
			}

			for(buffer_list = connection->buffer_list; buffer_list; buffer_list = buffer_list->next) {
				if(buffer_list->state == BUFFER_LIST_READ_READY) {
					parse_command(connection, buffer_list->buf->buf);
					buffer_list->state = BUFFER_LIST_READ_FINISHED;
					commands++;
				}
			}

			// check for data to send, and send it
			if(connection->out->buf[0] != '\0') {
				if(connection_send(connection) < 0) {
					connection_close(connection->fd);
				}
			}

			if(commands > 0) {
				buffer_list_cleanup(connection);
			}
		}
	}
}

int connection_write(CONNECTION *connection, char *str) {
	if(!ValidString(str)) {
		return true;
	}

	smart_buf_add(connection->out, str);
	return true;
}

// returns true if successful or 0 if max buffer was exceeded, which
// should cause the socket to be closed as an enemy
int connection_read(CONNECTION *connection, char *str) {
	BUFFER_LIST *cur_buf_list		= connection->buffer_list;
	BUFFER_LIST *new_buf_list		= 0;
	BUFFER_LIST *dummy				= 0;
	char buf[CONNECTION_MAX_BUFFER]	= {'\0'};
	char *p = 0;
	int len = 0;

	if(cur_buf_list == 0) {
		connection->buffer_list = buffer_list_create();
		cur_buf_list = connection->buffer_list;
	} else {
		while(cur_buf_list->state >= BUFFER_LIST_READ_READY) {
			if(cur_buf_list->next == 0) {
				new_buf_list = buffer_list_create();
				AddToListEnd(connection->buffer_list, new_buf_list, dummy)
				cur_buf_list = new_buf_list;
				break;
			}
			cur_buf_list = cur_buf_list->next;
		}
	}

	cur_buf_list->state = BUFFER_LIST_WRITING;

	// copy the string until EOF or newline, then skip any remaining newlines
	p = buf;
	while(*str != '\n' && *str != '\0') {
		*p++ = *str++;
		len++;

		if(len > CONNECTION_MAX_BUFFER-2) {
			return false;
		}
	}
	*p = '\0';
	
	if(smart_buf_add_n(cur_buf_list->buf, buf, len) == 0) {
		logit(LOG_NORMAL, "connection_read: failed to add to buf!");
		return false;
	}

	if(*str == '\n') {
		logit(LOG_DEBUG, "Connection [%d] has new command to process [%s]", connection->fd, cur_buf_list->buf->buf);
		// at this point don't touch cur_buf any longer
		cur_buf_list->state = BUFFER_LIST_READ_READY;
	}

	// now lets keep processing *str in case there is more to write
	while(*str == '\n' || *str == '\r') {
		str++;
	}

	if(*str != '\0') {
		return connection_read(connection, str);
	}

	return true;
}
