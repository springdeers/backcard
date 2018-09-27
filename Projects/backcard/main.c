// AppServer.cpp : 定义控制台应用程序的入口点。
//
#include "./mio/mio.h"
#include "client.h"
#include "p2p.h"
#include "tool.h"
#include "p2pclient.h"
#include <time.h>
#include "protobuffer.h"
#include "hiredis.h"
#include "printsvr_expat.h"
#include "printsvr_callback.h"
#include "dbaccess.h"
#include "json/cJSON.h"
#include "forward.h"
#include "scom/coms.h"
#include "net.h"
#include "my_curl/my_curl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define _MAC_WIN_WSA_START(errcode_rtn) \
{\
	WORD wVersionRequested;\
	WSADATA wsaData;\
	int err;\
	wVersionRequested = MAKEWORD( 2, 2 );\
	err = WSAStartup( wVersionRequested, &wsaData );\
	if ( err != 0 ) {\
		return errcode_rtn;\
	}\
}

#define SUB_DB_MAX		30

// 唯一的应用程序对象
int g_shutdown = 0;
int g_seqno2printsvr = 0;
int g_warntime = 20;

redisContext* g_rediscontext = NULL;
static void _sensor_signal(int signum)
{
	g_shutdown = 1;
}

static int init_database(_s_client_t client);

static char * offline_jsonstring(user_status_t s)
{
	char  * rslt = NULL;
	char    sztime[64];
	cJSON * root = cJSON_CreateObject();

	SYSTEMTIME T;
	GetLocalTime(&T);
	sprintf(sztime, "%04d-%02d-%02d %02d:%02d:%02d", T.wYear, T.wMonth, T.wDay, T.wHour, T.wMinute, T.wSecond);

	cJSON_AddStringToObject(root, "taskid", s->key);

	cJSON_AddStringToObject(root, "time", sztime);

	cJSON_AddStringToObject(root, "net", "inet");

	/*snprintf(szvalue, sizeof(szvalue), "%d", g_instapp->seqno2smcache);
	cJSON_AddStringToObject(root, "seqno", szvalue);*/

	rslt = cJSON_PrintUnformatted(root);
	free(root);				// 避免内存泄漏

	return rslt;

}

static redisContext* redisinit()
{
	redisContext* c = redisConnect("127.0.0.1", 6379);
	if (c->err)
	{
		redisFree(c);
		printf("Connect to redisServer faile\n");
		return  NULL ;
	}
	return c;
	printf("Connect to redisServer Success\n");
}


static int  mysqldb_que_ping(int waits)
{
	static int sqlcon_cnt = 0;
	static int code = 1;
	int i = 0;
	char * towncode;
	mysqlquery_t sub_query;
	static time_t last = 0;
	time_t now = time(0);
	if (last == 0)
		last = now;

	if (now - last < waits)
		return 1;

	last = now;

	sqlcon_cnt = jqueue_size(client()->sub_query_que);

	for (i = 0; i < sqlcon_cnt; i++)
	{
		mysqlquery_t query = (mysqlquery_t)jqueue_pull(client()->sub_query_que);	// 取出一个连接

		if (query && query->m_con)
		{
			code = mysql_ping(query->m_con);

			// 如果mysql_ping失败，则尝试重连该分数据库
			if (code)
			{
				towncode = xhash_get(client()->xsubquery2towncode, query);
				dbconfig_t dbconfig = (dbconfig_t)xhash_get(client()->xdbconfig, towncode);

				// 针对该towncode，查找对应分数据库连接属性，并尝试重建连接
				sub_query = mysqldb_connect_init(dbconfig->ip, atoi(dbconfig->port), dbconfig->username, dbconfig->password, dbconfig->dbname, NULL);
				if (sub_query == NULL)
				{
					log_write(client()->log, LOG_NOTICE, "sub_query[%d] reconnect to database [%s] failed. %d @%s\n", i, dbconfig->dbname, __LINE__, __FILE__);
					jqueue_push(client()->sub_query_que, query, 0);
					continue;
				}
				else
				{
					mysql_query(sub_query->m_con, "SET NAMES 'gb2312'");
					log_write(client()->log, LOG_NOTICE, "sub_query[%d] reconnected db [%s] ok.", i, dbconfig->dbname);

					xhash_zap(client()->xsubquery2towncode, query);	// 将原来的失效连接从哈希表中移除
					xhash_put(client()->xtowncode2subquery, towncode, sub_query);	// 以towncode为键，存储store_pid[i]线程与该分数据库的连接
					xhash_put(client()->xsubquery2towncode, sub_query, towncode); // 以store_pid[i]线程与某分数据库的连接为键，存储该分数据库对应的towncode值
					jqueue_push(client()->sub_query_que, sub_query, 0);				// 将该连接放入队列中，供store_pid[i]线程进行mysql_ping操作
				}
			}
			// 如果mysql_ping成功，则将连接放回队列
			else
			{
				jqueue_push(client()->sub_query_que, query, 0);
			}
		}

		log_write(client()->log, LOG_NOTICE, "mysql query[%d] code %d[%s]", i, code, code == 0 ? "success" : "failed");
	}

	return code;
}


#define DEFAULT_PATH "./"
#define BD_USER_MAX		150
#define ALL_USER_MAX	5000

static void   _mysql_querycallback(void* conn, int type, int code);
static void   _mysql_ping(void* conn, int waits);


int main(int argc, char **argv)
{	
	p2pclient_t  p2p = 0;
	int      counter = 0;
	char*    config_file  = NULL;
	time_t   lastcheckperson = time(0);
	jqueue_t inactivequeue = jqueue_new();

#ifdef HAVE_WINSOCK2_H
	_MAC_WIN_WSA_START(0);
#endif

	jabber_signal(SIGTERM, _sensor_signal);
	
#ifndef _WIN32
	jabber_signal(SIGPIPE, SIG_IGN);
#endif

	set_debug_flag(0);

	client()->config = config_new();
	config_file = "../../etc/backcard.xml";
	if (config_load(client()->config, config_file) != 0)
	{
		fputs("sensor: couldn't load config, aborting\n", stderr);
		client_free(client());
		return 2;
	}

	client_init(client(),client()->config);

	//init_database(client()); //连接数据库

	curl_global_init(CURL_GLOBAL_ALL);

	char * url = "http://127.0.0.1:8687/user/score?cardid=304068&comid=101";
	char * response = NULL;
	curl_get_req(url, response);
	}
	p2p_listener_start(client());

	//连接printsvr
	//p2p = p2pclient_new(client()->printsvr_ip,client()->printsvr_port,stream_SESS); //tcp连接
	//if(!p2pclient_connect(p2p,client()->mio,client()->dead_sess,client(),stream_SESS,NULL,client())){
	//	printf("connect to  printsvr failed ,exit..\n");
	//	client_free(client());
	//	return 1;
	//}else{
	//	p2p->expat = printsvr_exp_new(&p2p->sess); //回调函数
	//	exp_set_callback(p2p->expat, printsvr_exp_cb);
	//	init_sess_st initpara;
	//	strcpy_s(initpara.from, sizeof(initpara.from) - 1, "backcard");
	//	strcpy_s(initpara.to, sizeof(initpara.to) - 1, "printsvr");
	//	p2pclient_init(p2p, &initpara);
	//}
	//client()->p2p_printsvr = p2p;

	if (!coms_openinit(coms_get(), client()->config)) return -1;

	while (!g_shutdown)
	{
		static time_t last = 0;
		time_t now = time(NULL);
		if (last == 0) last = now;

		mio_run(client()->mio, 0);

		clear_deads_from_lives();

		save_deadsession(7);

		if (now - last > 2){
			remove_zombie_from_lives();
			last = now;
			clear_zombies();
		}
		pingsession(2);
		_mysql_ping(client()->sqlobj, 3);
		//check_inactive_client(5);

		do_coms_job(coms_get(),now);

	}

	client_free(client());

	return 0;
}


static void _mysql_querycallback(void* conn, int type)
{
	time_t now = time(NULL);

	if (type == eSqlQueryerr_errorquery || type == eSqlQueryerr_errorping){
		mysqlquery_t conn = NULL;

		mysqldb_close(client()->sqlobj);
		conn = mysqldb_connect_reinit(&client()->sqlobj, client()->appattr.db_ip, client()->appattr.db_port, client()->appattr.db_user, client()->appattr.db_pwd, client()->appattr.db_name);

		if (!mysqldb_isclosed(client()->sqlobj))
			log_write(client()->log, LOG_NOTICE, "connected db ok.");
		else
			log_write(client()->log, LOG_NOTICE, "connected db failed.");

	}
	else if (type == eSqlConnectOk)
	{
		mysqlquery_t connptr = (mysqlquery_t)conn;
		mysql_query(connptr->m_con, "SET NAMES 'gb2312'");

		log_write(client()->log, LOG_NOTICE, "connected db[%s] ok.", client()->appattr.db_name);
	}
}

static int init_database(_s_client_t client)
{
	mysqlquery_t sqlobj = NULL;
	sqlobj = mysqldb_connect_init(client->appattr.db_ip, client->appattr.db_port, client->appattr.db_user, client->appattr.db_pwd, client->appattr.db_name, _mysql_querycallback);
	if (sqlobj == NULL)
	{
		log_write(client->log, LOG_NOTICE, "connect to database failed. line%d @%s.", __LINE__, __FILE__);
		return -1;
	}
	client->sqlobj = sqlobj;
	
	return 0;
}

static void _mysql_ping(void* conn, int waits)
{
	int i = 0;
	int count = 0;
	time_t now = time(0);
	int pingcode;
	mysqlquery_t mysqlconn = (mysqlquery_t)conn;

	static time_t last = 0;

	if (last == 0) last = now;

	if (now - last < waits) return;
	last = now;

	pingcode = mysqldb_ping(mysqlconn);

	log_write(client()->log, LOG_NOTICE, "mysql query code %d[%s]", pingcode, pingcode == 0 ? "success" : "failed");
}

static int ping_maindb(mysqlquery_t query,int waits)
{
	static time_t last = 0;
	time_t now = time(0);
	if (last == 0)
		last = now;

	if (now - last < waits)
		return 1;

	last = now;

	int code = 1;

	if (query && query->m_con)
	{
		code = mysql_ping(query->m_con);
		log_write(client()->log, LOG_NOTICE, "mysql query code %d[%s]", code, code == 0 ? "success" : "failed");
	}
		
	if (code == 1 && query && query->_cb){
		log_write(client()->log, LOG_NOTICE, "Trying to reconnect database...");
		query->_cb(query, eSqlQueryerr_errorping);
	}

	return code;
}
