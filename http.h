#include "connection.h"

#ifndef __HTTPD_H
#define __HTTPD_H

#define VERSION "0.1.2"
#define SERVER_STRING "Server: BorlakHttpd/" VERSION "\r\n"

#define MAX_READ_BUF	4096
#define MAX_PARAM_BUF	25
#define MAX_HEADER_BUF	128

#define METHOD_UNKNOWN  0
#define METHOD_GET      1
#define METHOD_POST     2

#define HEADER_KEEPALIVE	0x00000001

int httpd_handle_request(int client, CONNECTION *connection);
char *http_system_curl_request(char *url, char *data);
void http_goodbye(int);

#endif
