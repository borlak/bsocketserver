#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include "stdh.h"
#include "epoll.h"
#include "smart_buf.h"
#include "profile.h"
#include "connection.h"
#include "trie.h"
 
void cleanup(void);
int running = 1;
pthread_mutex_t msg_mutex = PTHREAD_MUTEX_INITIALIZER;
int ssfd = 0; // host socket
int essfd; // ss = socket server, e = epoll

extern void *bserver_update(void *arg);

void signal_handler(int sig) {
	logit(LOG_ERROR, "Received signal %d, quitting...\n", sig);
	running = 0;
}

void initialize_settings(void)
{
	settings.http_port					= 80;
	settings.socket_port				= 4444;
	settings.log_level					= LOG_NORMAL;
	settings.enable_socket				= true;
	settings.enable_http				= true;
	settings.max_thread_usage			= 10;
	settings.profiling					= false;
	settings.connection_timeout			= 120;

	str_dup(&settings.files_md5, "", 0);
}

static void show_usage(void)
{
	printf("Borlak Server - provides HTTP, BOSH, WebSocket, Socket, and Flash Socket functionality\n");
	printf(""
		"Options:\n"
		"  -p <num>    HTTP port to listen on (default 80)\n"
		"  -s <num>    Flash socket port to listen on (default 4444)\n"
		"  -d <what>   Disable http/sock Server\n"
		"  -z          Turn on Profiling\n"
		"  -o <num>    Number of seconds a connection will timeout from inactivity\n"
		"  -t <num>    Number of worker threads\n"
		"  -v          Turn on verbose output/logging\n"
		"  -vv         Very verbose output/logging\n"
		"  -vvv        Annoyingly verbose output/logging\n"
		"  -h          Print this help/usage message\n"
		"\n");
	return;
}

int main(int argc, char *argv[])
{
	FILE *fp_md5 = 0;
	CONNECTION *connection;
	pthread_t *data_thread = (pthread_t*)malloc(sizeof(pthread_t));
	int s;
	struct epoll_event event;
	struct epoll_event *events;
	int c;
	time_t current_time;
	
	update_time();
	current_time = get_time();
	initialize_settings();
	
	while (-1 != (c = getopt(argc, argv,
		"p:" /* HTTP port */
		"s:" /* Flash socket port */
		"o:" /* connection timeout */
		"v" /* Verbose reporting */
		"t" /* Max thread usage */
		"hi" /* Print help */
		"d" /* Disable something */
		"z" /* Profiling */
	)))
	{
		switch (c)
		{
		case 'o':
			settings.connection_timeout = atoi(optarg);
			break;
		case 'p':
			settings.http_port = atoi(optarg);
			break;
		case 's':
			settings.socket_port = atoi(optarg);
			break;
		case 'v':
			settings.log_level++;
			break;
		case 'd':
			if (strcmp(optarg, "http") == 0)
				settings.enable_http = false;
			else if(strcmp(optarg, "sock") == 0)
				settings.enable_socket = false;
			break;
		case 't':
			settings.max_thread_usage = atoi(optarg);
			break;
		case 'z':
			settings.profiling = true;
			break;
		case 'h':
			show_usage();
			exit(EXIT_SUCCESS);
			break;
		}
	}

	// get md5 of *.[ch] files to use as a Version checker
	if((fp_md5 = fopen("/home/bserver/files_md5","r")) != 0) {
		char buf[128] = {'\0'};
		char *p = 0;
		if(fgets(buf, 127, fp_md5)) {
			if((p = index(buf,' ')) != 0) 
				*p = '\0';
			str_dup(&settings.files_md5, buf, 0);
		}
		fclose(fp_md5);
	}

	smart_buf_init(SB_PREALLOCATE_YES);

	signal(SIGINT, signal_handler);

	if((ssfd = create_server(settings.http_port)) == -1)
		abort();

	if((essfd = epoll_create(1000)) == -1) {
		perror("epoll_create");
		abort();
	}

	event.data.fd	= ssfd;
	event.events	= EPOLLIN | EPOLLOUT | EPOLLET;
	if((s = epoll_ctl(essfd, EPOLL_CTL_ADD, ssfd, &event)) == -1) {
		perror("epoll_ctl");
		abort();
	}

	pthread_create(data_thread, NULL, bserver_update, 0);
	
	// Buffer where events are returned
	events = (struct epoll_event*) calloc(MAXEVENTS, sizeof(event));
	
	/* The event loop */
	while(running) {
		int n, i;

		n = epoll_wait(essfd, events, MAXEVENTS, -1);
		update_time();
		current_time = get_time();

		for (i = 0; i < n; i++) {
			logit(LOG_VERBOSE, "epoll unpaused with %d events", n);
			
			if((events[i].events & EPOLLERR)
			|| (events[i].events & EPOLLHUP))
			{
				// epoll "error" -- can happen when a keepalive is dropped
				connection_close(events[i].data.fd);
				continue;
			}
			else if(ssfd == events[i].data.fd)
			{
				/* We have a notification on the listening socket, which
				   means one or more incoming connections. */
				while(1) {
					struct sockaddr in_addr;
					socklen_t in_len;
					int infd;
					char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
					struct timeval t_begin, t_accept, t_getname, t_ctl;
					
					if(settings.profiling)
						gettimeofday(&t_begin, 0);

					in_len = sizeof(in_addr);
					if((infd = accept(ssfd, &in_addr, &in_len)) == -1) 
					{
						if ((errno == EAGAIN)
						||  (errno == EWOULDBLOCK))
						{
							/* We have processed all incoming
							   connections. */
							break;
						}
						else
						{
							perror ("accept");
							break;
						}
					}

					if(settings.profiling)
						gettimeofday(&t_accept, 0);
					
					s = getnameinfo (&in_addr, in_len,
									 hbuf, sizeof(hbuf),
									 sbuf, sizeof(sbuf),
									 NI_NUMERICHOST | NI_NUMERICSERV);

					if(settings.profiling)
						gettimeofday(&t_getname, 0);

					if(s == 0)
						logit(LOG_VERBOSE, "Accepted connection on descriptor %d (host=%s, port=%s)", infd, hbuf, sbuf);

					/* Make the incoming socket non-blocking and add it to the
					   list of fds to monitor. */
					if((s = make_socket_non_blocking(infd)) == -1)
						abort();

					event.data.fd = infd;
					event.events = EPOLLIN | EPOLLET | EPOLLOUT;
					if((s = epoll_ctl(essfd, EPOLL_CTL_ADD, infd, &event)) == -1) {
						perror ("epoll_ctl");
						abort ();
					}
					
					connection = connection_create(infd);
					connection->state = CONN_SOCKET;

					if(settings.profiling) {
						gettimeofday(&t_ctl, 0);
						connection->profile->t_begin = t_begin;
						connection->profile->t_accept = t_accept;
						connection->profile->t_getname = t_getname;
						profile_settime(&connection->profile->t_ctl);
					}
				}
				continue;
			}
			else
			{
				connection = connection_get(events[i].data.fd);
				
				/* We have data on the fd waiting to be read. Read and
				   display it. We must read whatever data is available
				   completely, as we are running in edge-triggered mode
				   and won't get a notification again for the same
				   data. */
				int done = 0;
				int eot = 0; // end of transmission
				ssize_t count, read_to;
				char buf[4096];
				
				while (1) {
					buf[0] = '\0';
					read_to = count = read(events[i].data.fd, buf, sizeof(buf));
					buf[count] = '\0';
					
					if (count == -1)
					{
						/* If errno == EAGAIN, that means we have read all
						   data. So go back to the main loop. */
						if (errno != EAGAIN) {
							perror ("read");
							done = 1;
						}
						break;
					}
					else if(count == 0)
					{
						/* End of file. The remote has closed the
						   connection. */
						done = 1;
						break;
					}

					if(settings.profiling)
						profile_settime(&connection->profile->t_read);

					// update buffer list(s) for the connection
					if(connection_read(connection, buf) == false) {
						logit(LOG_NORMAL, "socket [%d] sent too large a string!  disconnecting.", events[i].data.fd);
						done = true;
					}
					
					// check if end of HTTP request has been received
					eot = 0;
					
					logit(LOG_VERBOSE, "read %li bytes [%s] from socket %d", count, buf, events[i].data.fd);
					break;
				}

				if(done)
				{
					// Closing the descriptor will make epoll remove it
					// from the set of descriptors which are monitored.
					connection_close(events[i].data.fd);
				}
			}
		}
	}

	logit(LOG_NORMAL, "Server stopping...\n");
	free(events);
	logit(LOG_NORMAL, "Freed events...\n");
	close(ssfd);
	logit(LOG_NORMAL, "Closed host sockets...\n");
    
	logit(LOG_NORMAL, "Waiting on threads to finish...\n");
	pthread_exit(0);
	
	free(data_thread);
	cleanup();
	
	return EXIT_SUCCESS;
}

void cleanup(void) {
}
