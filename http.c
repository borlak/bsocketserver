#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include "stdh.h"
#include "smart_buf.h"
#include "profile.h"
#include "connection.h"
#include "http.h"
#include "util.h"

int read_headers(char *buf, int headers_wanted);
void ajax_headers(SMART_BUF *buf, int content_length);
void http_favicon(int client);
void http_stats(int client);
void http_bad_request(int);
void http_not_found(int);
void http_unimplemented(int);
void http_goodbye(int);
void http_test(int);

long bad_requests = 0;
long requests_handled = 0;

extern int running; // ss_main.c

int httpd_handle_request(int client, CONNECTION *connection)
{
	SMART_BUF *outbuf = 0;
	char *p = 0;
	char method[255] = {'\0'};
	char buf[MAX_READ_BUF] = {'\0'};
	char params[MAX_READ_BUF] = {'\0'};
	char param[MAX_PARAM_BUF] = {'\0'};
	char jsonp[256] = {'\0'};
	int i=0, j=0, c=0, count=0;
	char *query_string = '\0';
	int len = 0;
	int flags = 0;
	int methodType = METHOD_UNKNOWN;
	int headers = 0;
	
	headers	= read_headers(connection->in->buf, HEADER_KEEPALIVE);
	p		= connection->in->buf;
	len		= strlen(p);
	requests_handled++;
	
	i = 0; j = 0;
	while(!isspace(p[j]) && (j < len) && (i < 5 - 1))
	{
		method[i] = p[j];
		i++; j++;
	}
	method[i] = '\0';

	if(!strcasecmp(method, "GET"))
		methodType = METHOD_GET;
	else if(!strcasecmp(method, "POST"))
		methodType = METHOD_POST;
	else
	{
		bad_requests++;
		return -1;
	}

	while(isspace(p[j]) && (j < len))
		j++;
	 
	i = 0;
	while(!isspace(p[j]) && (i < MAX_READ_BUF - 1) && (j < len))
	{
		buf[i] = p[j];
		i++; j++;
	}
	buf[i] = '\0';

	if(methodType == METHOD_GET)
	{
		if(!strcmp(buf,"/favicon.ico"))
		{
			http_favicon(client);
			return headers;
		}
		else 
		{
			query_string = buf;
			while((*query_string != '?') && (*query_string != '\0'))
				query_string++;
			if (*query_string == '?')
			{
				*query_string = '\0';
				query_string++;
				i = 0;
				params[i++] = '?';
				while(!isspace(*query_string) && *query_string != '\0' && (i < MAX_READ_BUF - 1)) {
					params[i++] = *query_string++;
				}
				params[i] = '\0';
			}
		}
	}
	else
	{
		bad_requests++;
		return -1;
	}
	
	outbuf = smart_buf_create(8192);
	smart_buf_clear(outbuf);
	
	// handle request
	j = 0;
	while(params[j] != '\0') {
		switch(params[j]) {
		case '?':
		case '&':
			j++;
			
			// string may be formatted ?&b= ....
			while(params[j] == '&')
				j++;
			
			// get the parameter
			i = 0;
			while(params[j] != '\0' && params[j] != '=' && i < MAX_PARAM_BUF-1)
				param[i++] = params[j++];
			param[i] = '\0';

			while(params[j] == '=')
				j++;
			
			// get the value
			i = 0;
			while(params[j] != '\0' && params[j] != '&' && i < MAX_READ_BUF-1)
				buf[i++] = params[j++];
			buf[i] = '\0';
			
			logit(LOG_VERBOSE, "param %s = %s", param, buf);
			break;
		default:
			j++;
			break;
		}
	}

	if(settings.profiling)
		profile_settime(&connection->profile->t_ajax);
	
	if(outbuf->buf[0] != '\0') {
		smart_buf_clear(connection->out);
		ajax_headers(connection->out, strlen(outbuf->buf));
		smart_buf_add(connection->out, outbuf->buf);
		connection_send(connection);
	}

	if(settings.profiling)
		profile_settime(&connection->profile->t_write);
	
	smart_buf_free(outbuf);
	return headers;
}

char *http_system_curl_request(char *url, char *data) {
	static SMART_BUF *sbuf = 0;
	FILE *fp = 0;
	char command[1024] = {'\0'};
	char *buf = 0;
	
	if(!sbuf)
		sbuf = smart_buf_create(1000000);

	smart_buf_clear(sbuf);
	
	sprintf(command, "curl -s -G --connect-timeout 1 -m 10 %s %s", data, url);
 
	if((fp = popen(command, "r"))) {
		char tempbuf[1048576];
		
		while(!feof(fp) && !ferror(fp)) {
			// fread returns 0 if EOF, even if it reads 10k bytes...
			memset(tempbuf, '\0', 1048576);
			fread(tempbuf, 524288, 1, fp);
			smart_buf_add(sbuf, tempbuf);
		}
		
		pclose(fp);
	}

	if(strlen(sbuf->buf) == 0)
		return 0;
	
	buf = malloc(sizeof(char) * strlen(sbuf->buf) + 1);
	strcpy(buf, sbuf->buf);
	return buf;
}

int read_headers(char *buf, int headers_wanted) {
	char header_name[MAX_HEADER_BUF] = {'\0'};
	char header_value[MAX_HEADER_BUF] = {'\0'};
	int i, j;
	int headers = 0;
	int number_of_headers = __builtin_popcount(headers_wanted); // how many bits are set
	int current_headers = 0;
	
	if(!headers_wanted) return 0;
	
	// skip first GET/POST string
	while(*buf != '\0' && *buf != '\n' && *buf != '\r')
		buf++;
	// skip any \r\ns
	while(*buf != '\0' && (*buf == '\n' || *buf == '\r'))
		buf++;
	
	while(*buf != '\0') {
		// read a header
		i = j = 0;
		while(*buf != '\0' && *buf != ':' && i < MAX_HEADER_BUF-1)
			header_name[i++] = *buf++;
		header_name[i] = '\0';
		
		// skip colon and space
		while(*buf != '\0' && (*buf == ':' || isspace(*buf)))
			buf++;
		
		while(*buf != '\0' && *buf != '\n' && *buf != '\r' && j < MAX_HEADER_BUF-1)
			header_value[j++] = *buf++;
		header_value[j] = '\0';
		
		// see if this is one of the headers we want
		if(headers_wanted & HEADER_KEEPALIVE && !strcmp(header_name, "Connection") && !strcasecmp(header_value, "keep-alive")) {
			headers |= HEADER_KEEPALIVE;
			current_headers++;
		}
	
		// don't want to read more than we have too
		if(current_headers == number_of_headers)
			break;
	}
	return headers;
}

void ajax_headers(SMART_BUF *smartbuf, int content_length) {
	static char header_buf[512] = {'\0'};
	char buf[1024] = {'\0'};
	
	if(header_buf[0] == '\0') {
		sprintf(header_buf, "HTTP/1.1 200 OK\r\n"
						    SERVER_STRING
						    "Cache-Control: no-cache, must-revalidate\r\n"
						    "Expires: Sat, 26 Jul 1997 05:00:00 GMT\r\n"
						    "Keep-Alive: timeout=%d, max=%d\r\n"
						    "Connection: Keep-Alive\r\n"
						    "Content-Type: application/json\r\n",
			settings.connection_timeout, settings.requests_per_connection
		);
	}
	
	sprintf(buf, "%sContent-Length: %d\r\n\r\n", header_buf, content_length);
	smart_buf_add(smartbuf, buf);
}

/**********************************************************************/
/* Send back headers for caching the favicon way off in the future
 * so it doesn't keep requesting favicons...
 * Parameters: client socket */
/**********************************************************************/
void http_favicon(int client) {
	static char buf[256] = {'\0'};
	
	if(buf[0] == '\0') {
		sprintf(buf, "HTTP/1.1 200 OK\r\n"
					 SERVER_STRING
					 "Cache-Control: public\r\n"
					 "Expires: Sat, 26 Jul 2015 05:00:00 GMT\r\n"
					 "Content-Type: image/x-icon\r\n"
					 "Content-Length: 0\r\n"
					 "\r\n"
		);
	}
	send(client, buf, sizeof(buf), MSG_DONTWAIT);
}


/**********************************************************************/
/* Return statistics about this server.
 * Parameters: client socket */
/**********************************************************************/
void http_stats(int client) {
	char buf[4096] = {'\0'};
	char stats_buf[4096] = {'\0'};
	
//	strcat(stats_buf, http_stats());
	
	snprintf(buf, 4096, "HTTP/1.1 200 OK\r\n"
						SERVER_STRING
						"Cache-Control: no-cache, must-revalidate\r\n"
						"Expires: Sat, 26 Jul 1997 05:00:00 GMT\r\n"
						"Content-Type: text/html\r\n"
					    "Connection: Keep-Alive\r\n"
						"Content-Length: %lu\r\n"
						"\r\n"
						"%s",
		strlen(stats_buf),
		stats_buf
	);
	send(client, buf, sizeof(buf), MSG_DONTWAIT);
}

/**********************************************************************/
/* Inform the client that a request it has made has a problem.
 * Parameters: client socket */
/**********************************************************************/
void http_bad_request(int client)
{
	static char buf[1024] = {'\0'};

	if(buf[0] == '\0') {
		char *bad_request_message = "<P>Your browser sent a bad request.\r\n";
		
		sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n"
					 "Content-type: text/html\r\n"
					 "Connection: Keep-Alive\r\n"
					 "Content-Length: %lu\r\n"
					 "\r\n"
					 "%s",
			strlen(bad_request_message),
			bad_request_message
		);
	}
	send(client, buf, sizeof(buf), MSG_DONTWAIT);
}

void http_goodbye(int client) {
	static char buf[1024] = {'\0'};

	if(buf[0] == '\0') {
		sprintf(buf, "HTTP/1.1 200 OK\r\n"
					 SERVER_STRING
					 "Content-Length: 0\r\n"
					 "Connection: close\r\n"
					 "Content-Type: text/html; charset=UTF-8\r\n"
					 "\r\n"
		);
	}
	send(client, buf, sizeof(buf), MSG_DONTWAIT);
}

/**********************************************************************/
/* Give a client a 404 not found status message. */
/**********************************************************************/
void http_not_found(int client)
{
	static char buf[1024] = {'\0'};

	if(buf[0] == '\0') {
		char *not_found_message = "<HTML><TITLE>Not Found</TITLE>\r\n"
					 "<BODY>Resource not found.</BODY></HTML>\r\n";
		
		sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n"
					 SERVER_STRING
					 "Content-Type: text/html\r\n"
					 "Content-Length: %lu\r\n"
					 "Connection: Keep-Alive\r\n"
					 "Status: 404 Not Found\r\n"
					 "\r\n"
					 "%s",
			strlen(not_found_message),
			not_found_message
		);
	}
	send(client, buf, strlen(buf), MSG_DONTWAIT);
}



/**********************************************************************/
/* Inform the client that the requested web method has not been
 * implemented.
 * Parameter: the client socket */
/**********************************************************************/
void http_unimplemented(int client)
{
	static char buf[1024] = {'\0'};
	
	if(buf[0] == '\0') {
		char *unimplemented_message = "<HTML><HEAD><TITLE>Method Not Implemented</TITLE></HEAD>\r\n"
									  "<BODY><P>HTTP request method not supported.</BODY></HTML>\r\n";
		
		sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n"
					 SERVER_STRING
					 "Content-Type: text/html\r\n"
					 "Content-Length: %lu\r\n"
					 "Connection: Keep-Alive\r\n"
					 "\r\n"
					 "%s",
			strlen(unimplemented_message),
			unimplemented_message
		 );
	}

	send(client, buf, strlen(buf), MSG_DONTWAIT);
}

void http_test(int client) {
	static char buf[1024] = {'\0'};
	
	if(buf[0] == '\0') {
		sprintf(buf, "HTTP/1.1 200 OK\r\n"
					 SERVER_STRING
					 "Keep-Alive: timeout=%d, max=%d\r\n"
					 "Connection: Keep-Alive\r\n"
					 "Content-Type: text/plain\r\n"
					 "\r\n"
					 "Test",
			settings.connection_timeout, settings.requests_per_connection
		);
	}
	
	send(client, buf, strlen(buf), MSG_DONTWAIT);
}

