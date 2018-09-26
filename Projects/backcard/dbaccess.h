#ifndef __DBACCESS_H__
#define __DBACCESS_H__

#include "myquery/mysqldb.h"
#include "client.h"
#include "rediscommand.h"


typedef struct _conf_st
{
	int warntime;			// 消失报警的阈值时间，单位是分钟
}conf_st, *conf_t;

typedef struct _plog_st
{
	char *type;
	int taskid;
	char *executor;
	char *time;
	char *lat;
	char *lon;
	char *hgt;
	char *net;
	char *source;
	char *desc;
}plog_st, *plog_t;

typedef struct _boxpos_st
{
	int taskid;
	char *executor;
	char *time;
	char *lat;
	char *lon;
	char *hgt;
	char *net;
	char *source;
}boxpos_st, *boxpos_t;


typedef struct _warning_st
{
	char *type;
	int taskid;
	char *executor;
	char *time;
	char *lat;
	char *lon;
	char *hgt;
	char *net;
	char *source;
	
}warning_st, *warning_t;

typedef struct _status_st
{
	char *battery;
	char *beam;
	int taskid;
	char *executor;
	char *time;
	char *lat;
	char *lon;
	char *hgt;
	char *net;
	char *source;

}status_st, *status_t;


typedef struct _xuserinfo_st
{
	char *userid;
	char *towncode;
	char *bdnum;
}xuserinfo_st, *xuserinfo_t;



typedef struct _position_st
{
	char *userid;
	char *bdid;
	char *towncode;
	char *lat;
	char *lon;
	char *hgt;
	char *time;
	int stype;	// 位置报告来源。   0：北斗上报  1：互联网上报
	char *tabletime;
	void* sqlobj;
}position_st, *position_t;


typedef struct _event_st
{
	char *userid;
	char *bdid;
	char *towncode;
	char *time;
	char *lat;
	char *lon;
	char *hgt;
	char *evttype;
	char *evttypname;
	char *description;
	int stype;	// 位置报告来源。   0：北斗上报  1：互联网上报
	char *tabletime;
	void* sqlobj;
}event_st, *event_t;

typedef struct _dbconfig_st
{
	char *towncode;
	char *ip;
	char *port;
	char *dbname;
	char *username;
	char *password;
}dbconfig_st, *dbconfig_t;

//////////////////////////////////////////////////////////////////////////


char * redis_get_towncode(char * userid);
char * redis_get_userid(char * bdid);


int db_auth_excecutor_simple(mysqlquery_t dbinst, char *name, char ** pwd);
int db_auth_excecutor(mysqlquery_t dbinst, char *name, char *taskid, char ** pwd);

#endif