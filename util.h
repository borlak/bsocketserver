#include <sys/time.h>

#ifndef __UTIL_H
#define __UTIL_H

#define LOG_LIMIT		1000000
#define LOG_NONE		0
#define LOG_NORMAL		1
#define LOG_DEBUG		2
#define LOG_VERBOSE		3
#define LOG_ERROR		4

#define SB_NO_COMPARE		0
#define SB_COMPARE_FIRST	1

void timetest			(char *str);
void logit				(int log_level, const char *str, ...);
void str_dup			(char **destination, char *str, int flags);
float bround(float num);
int timeval_subtract	(struct timeval *result, struct timeval *x, struct timeval *y);
time_t get_time			();
float get_microseconds	();
void update_time		();
void get_uptime			(char *buf);
int get_line			(int sock, char *buf, int size);

#endif
