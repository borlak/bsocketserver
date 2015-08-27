/*****************************************************
 *     Created by Michael Morrison                   *
 *                                                   *
 *  Creates a "smart buffer", that will keep a       *
 *  predefined allocated buffer, so any extensions   *
 *  should be fast.                                  *
 *                                                   *
 *  This code is meant to be independent of the rest *
 *  of the codebase.                                 *
 *                                                   *
 *****************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "stdh.h"
#include "smart_buf.h"
#include "util.h"

struct smart_buf_settings {
	long	size;
	int		max_allocated_free;		// how many smart_bufs of this size to keep waiting in memory for use
	int		available_free;			// don't change this from 0, the file will update this as smart bufs are created/used
} smart_buf_sizes[] = {
	{16,		8192,	0},
	{32,		16384,	0},
	{64,		16384,	0},
	{128,		256,	0},
	{256,		16384,	0},
	{512,		64,		0},
	{1024,		32,		0},
	{2048,		16,		0},
	{4096,		8,		0},
	{8192,		128,	0},
	{16384,		512,	0}, // outbuf for requests
	{32768,		2,		0},
	{65536,		2,		0},
	{131072,	2,		0},
	{262144,	2,		0},
	{524288,	2,		0},
	{1048576,	1,		0}, 
	{2097152,	1,		0}, 
	{4194304,	1,		0},
	{8388608,	1,		0},
	{16777216,	0,		0},
	{33554432,	0,		0},
	{67108864,	0,		0},
	{134217728,	0,		0},
	{0,0,0}
};

SMART_BUF **smart_buf_free_list = 0;
int smart_bufs_in_use			= 0;
int smart_buf_extends			= 0;
int smart_buf_preallocating		= 0;	// in process of preallocating?

/**
 * Must call this for your program if you use smart bufs.  
 * This will setup the free list, and you have the option of pre-allocating smart bufs
 * for ready use in your program.
 */
void smart_buf_init(int flags) {
	SMART_BUF *prev = 0, *sb = 0;
	long long bytes = 0;
	int i, count;
	for(i = 0; smart_buf_sizes[i].size; i++)
		;
	smart_buf_free_list = (SMART_BUF**)malloc(sizeof(SMART_BUF*) * i);
	memset(smart_buf_free_list, 0, sizeof(SMART_BUF*) * i);
	
	if(flags == SB_PREALLOCATE_YES) {
		smart_buf_preallocating = 1;
		for(i = 0; smart_buf_sizes[i].size; i++) {
			prev = 0;
			count = smart_buf_sizes[i].max_allocated_free;
			while(smart_buf_sizes[i].available_free < smart_buf_sizes[i].max_allocated_free) {
				if(count-- == 0) break;
				
				bytes += smart_buf_sizes[i].size;
				
				if(prev) {
					sb			= smart_buf_create(smart_buf_sizes[i].size/2);
					sb->next	= prev;
					prev		= sb;
				}
				else {
					prev = smart_buf_create(smart_buf_sizes[i].size/2);
				}
			}
			if(prev) {
				do {
					sb			= prev;
					prev		= prev->next;
					sb->next	= 0;
					smart_buf_free(sb);
				} while(prev);
			}
		}
		logit(LOG_NORMAL, "Preallocated %d MB of smart bufs", (int)(bytes / 1000000));
		smart_buf_preallocating = 0;
	}
}

/**
 * Returns an appropriate smart buf max size based on the string size,
 * used for creation or extending.
 * Returns 0 if string size doesn't exist (string is too big)
 * @param int string_size
 * @return int new max size
 */
int smart_buf_get_size(int string_size) {
	int i;
	for(i = 0; smart_buf_sizes[i].size; i++)
		if(smart_buf_sizes[i].size >= string_size*2)
			return smart_buf_sizes[i].size;
	return 0;
}

/**
 * Creates a new smart_buf.  *str must be null terminated.
 * @param char str
 * @return SMART_BUF*
 */
SMART_BUF *smart_buf_create(int buf_size) {
	SMART_BUF *new_buf = 0;
	int size, i;
	
	if(buf_size <= 0)
		return 0;

	if(!smart_buf_preallocating)
		smart_bufs_in_use++;
	
	size = smart_buf_get_size(buf_size);
	
	for(i = 0; smart_buf_sizes[i].size; i++)
		if(smart_buf_sizes[i].size == size) {
			if(smart_buf_free_list[i]) {
				new_buf					= smart_buf_free_list[i];
				smart_buf_free_list[i]	= new_buf->next;
				new_buf->next			= 0;
				smart_buf_clear(new_buf);
				smart_buf_sizes[i].available_free--;
			}
			break;
		}
	
	if(!new_buf) {
		new_buf	= (SMART_BUF*)malloc(sizeof(SMART_BUF));
		memset(new_buf, 0, sizeof(SMART_BUF));
		new_buf->size	= size;
		new_buf->buf	= (char*)malloc(sizeof(char) * (size+1)); // +1 terminating char
		new_buf->buf[0]	= '\0';
	}

	new_buf->state = SB_STATE_CREATED;

	return new_buf;
}

/**
 * Extends an already allocated smart_buf.  The new_size that is passed in
 * should have come from smart_buf_get_size
 * @param SMART_BUF buf
 * @param int new_size
 * @return SMART_BUF* Pointer to the new memory.
 */
SMART_BUF *smart_buf_extend(SMART_BUF *buf, int new_size) {
	logit(LOG_VERBOSE, "Extending smart buf from %d to %d", buf->size, new_size);
	
	if(!smart_buf_preallocating)
		smart_buf_extends++;

	// +1 terminating char
	if((buf->buf = (char*)realloc(buf->buf, new_size+1)) == 0)
		return 0;

	// zero-fill the new memory, as it is not guaranteed
	memset(&buf->buf[buf->size], '\0', new_size - buf->size);

	buf->size = new_size;
	return buf;
}

/**
 * Sets the smart buf to *str.  *str must be non-empty and null terminated.
 * @param SMART_BUF buf
 * @param char* str
 * @return char* pointer to beginning of the entire string.
 */
void *smart_buf_set(SMART_BUF *buf, char *str, int flags) {
	int new_size;
	
	if(!ValidString(str))
		return 0;
	
	if(flags == SB_COMPARE_FIRST && !strcmp(buf->buf, str))
		return 0;

	if(!(new_size = smart_buf_get_size(strlen(str) + 1)))
		return 0;
	
	if(buf->size < new_size) {
		if(!(buf = smart_buf_extend(buf, new_size)))
			return 0;
	}

	buf->buf[0] = '\0';
	strcpy(buf->buf, str);
	return buf->buf;
}

/**
 * Add to an already existing smart buf.  *str must be null terminated.
 * @param SMART_BUF buf
 * @param char str
 * @return char* pointer to beginning of the entire string.
 */
char *smart_buf_add(SMART_BUF *buf, char *str) {
	int new_size;
	
	if(!ValidString(str))
		return 0;

	if(!(new_size = smart_buf_get_size(strlen(buf->buf) + strlen(str) + 1)))
		return 0;
	
	if(buf->size < new_size) {
		if(!(buf = smart_buf_extend(buf, new_size)))
			return 0;
	}

	strcat(buf->buf, str);	
	return buf->buf;
}

/**
 * Same as smart_buf_add but will only copy over "len" characters.  Another difference
 * is this will put on the terminating null character itself.
 * @param SMART_BUF buf
 * @param char* str
 * @param int len
 * @return char* pointer to beginning of the entire string.
 */
char *smart_buf_add_n(SMART_BUF *buf, char *str, int len) {
	int new_size;
	
	if(!ValidString(str))
		return 0;

	if(!(new_size = smart_buf_get_size(strlen(buf->buf) + len + 1)))
		return 0;
	
	if(buf->size < new_size) {
		if(!(buf = smart_buf_extend(buf, new_size)))
			return 0;
	}

	strncat(buf->buf, str, len);
	return buf->buf;
}

void smart_buf_clear(SMART_BUF *buf) {
	buf->buf[0] = '\0';
}

void smart_buf_delete(SMART_BUF *buf) {
	free(buf->buf);
	free(buf);
	buf = 0;
}

void smart_buf_free(SMART_BUF *buf) {
	int i;
	
	if(!smart_buf_preallocating)
		smart_bufs_in_use--;
	
	for(i = 0; smart_buf_sizes[i].size; i++)
		if(smart_buf_sizes[i].size == buf->size
		&& smart_buf_sizes[i].available_free < smart_buf_sizes[i].max_allocated_free) {
			buf->next				= smart_buf_free_list[i];
			smart_buf_free_list[i]	= buf;
			buf->state				= SB_STATE_FREED;
			smart_buf_sizes[i].available_free++;
            return;
		}
	
	// no slots left, delete..
	if(buf) {
		logit(LOG_DEBUG, "smart buf size %ld not available, deleting..", smart_buf_sizes[i].size);
		smart_buf_delete(buf);
	}
}

SMART_BUF *buf_stats = 0;
char *smart_buf_stats() {
	char buf[2056];
	int i;
	
	if(!buf_stats)
		buf_stats = smart_buf_create(1024);
	smart_buf_clear(buf_stats);
	
	smart_buf_add(buf_stats, "Smart Buf Stats\n");

	buf[0] = '\0';
	for(i = 0; smart_buf_sizes[i].size; i++) {
		sprintf(buf+strlen(buf), "[%-9ld %5d/%-5d]%c",
			smart_buf_sizes[i].size,
			smart_buf_sizes[i].available_free,
			smart_buf_sizes[i].max_allocated_free,
			(i+1) % 4 == 0 ? '\n' : ' ');
	}
	sprintf(buf+strlen(buf), "%cIn Use[%d]  Extends[%d]\n",
		(i+1) % 4 == 0 ? '\n' : ' ', smart_bufs_in_use, smart_buf_extends);
	
	smart_buf_add(buf_stats, buf);
	
	return buf_stats->buf;
}
