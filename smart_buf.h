#ifndef __SMART_BUFFER_H
#define __SMART_BUFFER_H

#define ValidString(str)	((str) && (str[0]) != '\0')

#define SB_NO_COMPARE		0
#define SB_COMPARE_NONE		0
#define SB_COMPARE_FIRST	1

#define SB_PREALLOCATE_NO	0
#define SB_PREALLOCATE_YES	1

#define SB_STATE_CREATED		0
#define SB_STATE_WRITING		1
#define SB_STATE_EXTENDING		2
#define SB_STATE_READ_READY		3
#define SB_STATE_READ_FINISHED	4
#define SB_STATE_FREED			5

void smart_buf_init(int flags);
SMART_BUF *smart_buf_create(int buf_size);
void *smart_buf_set(SMART_BUF *buf, char *str, int flags);
char *smart_buf_add(SMART_BUF *buf, char *str);
char *smart_buf_add_n(SMART_BUF *buf, char *str, int len);
void smart_buf_clear(SMART_BUF *buf);
void smart_buf_free(SMART_BUF *buf);
char *smart_buf_stats();

struct smart_buf {
	SMART_BUF *next, *prev;	// used for the free list
	char	*buf;
	int		size;			// current max allocated memory for the buf, not size of the string in *buf
	int		state;			// used for processes that want to be sure to only read when buffer is in a 'ready' state
};

#endif
