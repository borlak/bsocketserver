#ifndef __BUFFER_LIST_H
#define __BUFFER_LIST_H

#define BUFFER_LIST_CREATED			0
#define BUFFER_LIST_WRITING			1
#define BUFFER_LIST_READ_READY		2
#define BUFFER_LIST_READ_FINISHED	3
#define BUFFER_LIST_REMOVED			4

BUFFER_LIST *buffer_list_create();
void buffer_list_free(CONNECTION *connection, BUFFER_LIST *buffer_list);
void buffer_list_cleanup(CONNECTION *connection);

struct buffer_list {
	BUFFER_LIST	*next;
	BUFFER_LIST	*prev;
	
	SMART_BUF	*buf;
	int			state;
};

#endif // __BUFFER_LIST_H
