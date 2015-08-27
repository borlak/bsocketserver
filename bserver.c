#include <unistd.h>
#include "stdh.h"
#include "smart_buf.h"
#include "profile.h"
#include "bserver.h"
#include "util.h"
#include "connection.h"

extern unsigned long long bytes_sent;
extern unsigned long long bytes_polled;
extern int connection_count;

unsigned long long bytes_sent_sec;

void *bserver_update() {
	char uptime[50] = {'\0'};
	extern int running;
	long long temp_bytes_sent_sec;
	long ticks = 0;
	long connection_timer = 0;
	
	update_time();
	
	while(running) {
		update_time();
		get_uptime(uptime);

		if(++connection_timer % (UPDATES_SEC / 10) == 0) {
			connection_update(settings.connection_timeout);
		}
		
		if(ticks % UPDATES_SEC == 0) {
			logit(LOG_NORMAL, 
				"Stats -- Uptime:%s  Connections:%d  BytesSent:%llu - %llu/sec",
				uptime, connection_count, bytes_sent, bytes_sent_sec);
			logit(LOG_NORMAL, smart_buf_stats());
		}
		
		temp_bytes_sent_sec	= bytes_sent;
		usleep(1000000/UPDATES_SEC); 
		bytes_sent_sec	= (bytes_sent - temp_bytes_sent_sec) * UPDATES_SEC;
		ticks++;
	}
	
	logit(LOG_NORMAL, "Bserver Update thread has stopped.");
	return 0;
}

char *bserver_stats(void) {
	static char buf[4096] = {'\0'};
	char uptime[50] = {'\0'};

	extern int connection_count;
	
	update_time();
	get_uptime(uptime);

	sprintf(buf,""
		"CONNECTIONS %d\n"
		"BYTES_SENT %lld\n"
		"BYTES_SENT_SEC %lld\n"
		"BYTES_POLLED %lld\n"
		"UPTIME %s\n"
		"TIMESTAMP %lu\n"
		"VERSION %s\n"
		"MD5 %s\n",
		connection_count,
		bytes_sent,
		bytes_sent_sec,
		bytes_polled,
		uptime,
		get_time(),
		VERSION,
		settings.files_md5
	);
	return buf;
}

