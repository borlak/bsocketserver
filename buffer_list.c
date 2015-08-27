#include "stdh.h"
#include "smart_buf.h"
#include "profile.h"
#include "connection.h"
#include "buffer_list.h"
#include "macros.h"
#include "util.h"

BUFFER_LIST *buffer_list_free_list = 0;

long buffer_list_count = 0;

BUFFER_LIST *buffer_list_create() {
	BUFFER_LIST *buffer_list;
	
	NewObject(buffer_list_free_list, buffer_list, BUFFER_LIST)

	buffer_list_count++;
	buffer_list->state		= BUFFER_LIST_CREATED;
	buffer_list->buf		= smart_buf_create(1024);

	return buffer_list;
}

// only call this during a cleanup phase
void buffer_list_free(CONNECTION *connection, BUFFER_LIST *obj) {
	RemoveFromList(connection->buffer_list, obj)
	obj->state = BUFFER_LIST_REMOVED;
	smart_buf_free(obj->buf);
	AddToList(buffer_list_free_list, obj)
	buffer_list_count--;
}

// goes through each buffer list object and sees if it is a certain
// state, and then removes it from the list.  this isn't "exactly"
// thread safe, but is the best compromise I came up with before
// getting into locks
void buffer_list_cleanup(CONNECTION *connection) {
	BUFFER_LIST *obj, *obj_next;
	for(obj = connection->buffer_list; obj; obj = obj_next) {
		obj_next = obj->next;

		if(obj->state == BUFFER_LIST_READ_FINISHED) {
			buffer_list_free(connection, obj);
			continue;
		}
	}
}
