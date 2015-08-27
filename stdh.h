#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "typedefs.h"
#include "macros.h"
#include "util.h"

#ifndef __STDHH
#define __STDHH

#define VERSION		"1.0"
#define false		0
#define true		1
#define MAXEVENTS	64

struct bsettings
{
	int http_port;
	int socket_port;
	int log_level;
	long int max_thread_usage;
	int enable_http;
	int enable_socket;
	int profiling;
	int connection_timeout;			// how long until a connection times out fron inactivity
	char *files_md5;				// md5sum of all the *.[ch] files used for comparing versions, created by update_rebuild.sh script
};

struct bsettings settings;

#endif

