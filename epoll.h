#ifndef __EPOLLH
#define __EPOLLH

int make_socket_non_blocking(int sfd);
int set_reuseaddr(int sfd);
int set_nodelay(int sfd);
int create_server(int port);

#endif
