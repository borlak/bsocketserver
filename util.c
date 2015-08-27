#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <stdarg.h>
#include "stdh.h"
#include "macros.h"
#include "smart_buf.h"

struct timeval mytime;
time_t current_time		= 0;
time_t start_time		= 0;

void update_time() {
	gettimeofday(&mytime, 0);
	current_time = mytime.tv_sec;
	if(start_time == 0)
		start_time = current_time;
}

time_t get_time() {
	return current_time;
}

float get_microseconds() {
	return (float)mytime.tv_usec/1000000.0;
}

void get_uptime(char *buf) {
	int days, hours, minutes, seconds;
	time_t uptime = current_time - start_time;
	
	days		= uptime / 86400;
	uptime		-= days * 86400;
	hours		= uptime / 3600;
	uptime		-= hours * 3600;
	minutes		= uptime / 60;
	uptime		-= minutes * 60;
	seconds		= uptime;
	
	sprintf(buf, "%d days %d:%d:%d", days, hours, minutes, seconds);
}

void timetest(char *str) {
	struct timeval tv;
	struct timezone tz;
	struct tm *tm;
	gettimeofday(&tv, &tz);
	tm=localtime(&tv.tv_sec);
	printf("%-20s %d:%02d:%02d %d \n", str?str:"", tm->tm_hour, tm->tm_min,
		tm->tm_sec, (int)tv.tv_usec);
}

void logit(int log_level, const char *str, ...)
{
	static SMART_BUF *log_buf = 0;
	static SMART_BUF *pre_buf = 0;
	va_list ap;

	if(!log_buf) {
		log_buf = smart_buf_create(4096);
		pre_buf = smart_buf_create(4096);
	} else {
		smart_buf_clear(log_buf);
		smart_buf_clear(pre_buf);
	}
	
	if(settings.log_level < log_level)
		return;

	va_start(ap, str);
	vsnprintf(pre_buf->buf, pre_buf->size, str, ap);
	va_end(ap);

	strftime(log_buf->buf, log_buf->size, "%m/%d/%Y %H:%M:%S :: ", localtime(&current_time) );
	smart_buf_add(log_buf, pre_buf->buf);

	fprintf(stdout, "%s\n", log_buf->buf);
}

float bround(float num) {
    return (float)(int)(num+0.5);
}

void str_dup(char **destination, char *str, int flags)
{
	if(!str)
	{
		perror("STRDUP error -- invalid source str");
		str = "";
	}

	if(ValidString(*destination))
	{
		if(flags == SB_COMPARE_FIRST) {
			if(!strcmp(*destination, str))
				return;
		}
		DeleteObject(*destination)
	}

	*destination = (char*) malloc(sizeof(char) * (strlen(str)+1));
	strcpy(*destination,str);
}

int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)
{
	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_usec < y->tv_usec) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		int nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}

	/* Compute the time remaining to wait.
	tv_usec is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}

/**********************************************************************/
/* Get a line from a socket, whether the line ends in a newline,
 * carriage return, or a CRLF combination.  Terminates the string read
 * with a null character.  If no newline indicator is found before the
 * end of the buffer, the string is terminated with a null.  If any of
 * the above three line terminators is read, the last character of the
 * string will be a linefeed and the string will be terminated with a
 * null character.
 * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 * Returns: the number of bytes stored (excluding null) */
/**********************************************************************/
int get_line(int sock, char *buf, int size)
{
	int i = 0;
	char c = '\0';
	int n;

	while ((i < size - 1) && (c != '\n'))
	{
		n = recv(sock, &c, 1, 0);
		if (n > 0)
		{
			if (c == '\r')
			{
				n = recv(sock, &c, 1, MSG_PEEK);
				if ((n > 0) && (c == '\n'))
					recv(sock, &c, 1, 0);
				else
					c = '\n';
			}
			buf[i] = c;
			i++;
		}
		else
			c = '\n';
	}
	buf[i] = '\0';
	return i;
}
