#ifndef __PROFILE_H
#define __PROFILE_H

struct profile {
	PROFILE		*next;
	PROFILE		*prev;
	
	struct timeval		t_begin;
	struct timeval		t_accept;
	struct timeval		t_getname;
	struct timeval		t_ctl;
	struct timeval		t_read;
	struct timeval		t_write;
};

PROFILE *profile_create();
void profile_free(PROFILE *profile);
void profile_settime(struct timeval *time);
void profile_reset(PROFILE *profile);
void profile_log(PROFILE *profile);

#endif
