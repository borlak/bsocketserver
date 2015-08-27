#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "stdh.h"
#include "macros.h"
#include "profile.h"

PROFILE *profile_list		= 0;
PROFILE *profile_free_list	= 0;

PROFILE *profile_create() {
	PROFILE *profile;
	
	NewObject(profile_free_list, profile, PROFILE)
		
	return profile;
}

void profile_free(PROFILE *profile) {
	RemoveFromList(profile_list, profile)
	AddToList(profile_free_list, profile)
	profile = 0;
}

void profile_settime(struct timeval *time) {
	gettimeofday(time, 0);
}

void profile_reset(PROFILE *profile) {
	memset(profile, 0, sizeof(PROFILE));
}

void profile_log(PROFILE *profile) {
	float begin_accept, accept_getname, getname_ctl, ctl_read, total;
	
	begin_accept	= profile->t_accept.tv_sec && profile->t_begin.tv_sec		? (profile->t_accept.tv_sec - profile->t_begin.tv_sec) + (profile->t_accept.tv_usec - profile->t_begin.tv_usec)/1000000.0 : 0.0;
	accept_getname	= profile->t_getname.tv_sec && profile->t_accept.tv_sec		? (profile->t_getname.tv_sec - profile->t_accept.tv_sec) + (profile->t_getname.tv_usec - profile->t_accept.tv_usec)/1000000.0 : 0.0;
	getname_ctl		= profile->t_ctl.tv_sec && profile->t_getname.tv_sec		? (profile->t_ctl.tv_sec - profile->t_getname.tv_sec) + (profile->t_ctl.tv_usec - profile->t_getname.tv_usec)/1000000.0 : 0.0;
	ctl_read		= profile->t_read.tv_sec && profile->t_ctl.tv_sec			? (profile->t_read.tv_sec - profile->t_ctl.tv_sec) + (profile->t_read.tv_usec - profile->t_ctl.tv_usec)/1000000.0 : 0.0;

	total = begin_accept + accept_getname + getname_ctl + ctl_read;
	
	logit(LOG_NORMAL, "\n"
		"PROFILE: Begin to Accept:   %f\n"
		"         Accept to Getname: %f\n"
		"         Getname to CTL:    %f\n"
		"         CTL to Read:       %f\n"
		"         Write to HTTP:     %f\n"
		"         Total Time:        %f\n",
		begin_accept,
		accept_getname,
		getname_ctl,
		ctl_read,
		total
	);
}
