/*****************************************************
 *     Created by Michael Morrison                   *
 *                                                   *
 *  This implementation of MySQL is not thread-safe  *
 *                                                   *
 *****************************************************/

#include <mysql.h>
#include "stdh.h"
#include "mysql.h"

MYSQL *mysql;

// this should only be called once...
int connect_db() {
	mysql = mysql_init(NULL);
	
	if(!mysql_real_connect(mysql, DB_IP, DB_USER, DB_PASSWORD, DB_NAME, 0, NULL, 0)) {
		logit(LOG_ERROR, "Error %u: %s", mysql_errno(mysql), mysql_error(mysql));
		return 0;
	}
	logit(LOG_NORMAL, "Connected as %s@%s to %s.", DB_USER, DB_IP, DB_NAME);
	return 1;
} 

MYSQL_RES *query(char *str) {
	if(!mysql_query(mysql, str))
		return mysql_store_result(mysql);
	return 0;
}



