#ifndef __CLIENT_H__
#define __CLIENT_H__
#include "mio/mio.h"
#include "util/xhash.h"
#include "util/util.h"
#include "responsepkt.h"
#include "subject.h"
#include "userstatus.h"
#include "rediscommand.h"
#include "dbaccess.h"

#include <time.h>

typedef enum {
	stream_SESS  = 0x00,
	mon_SESS     = 0x01,
	back_server_SESS   = 0x03,
	front_mcu_SESS     = 0x04,	
	p2pclient_SESS = 0x06,
	storage_SESS = 0x07
}sess_type_t;

typedef enum {
	state_init_SESS = 0x00,
	state_handshake_SESS  = 0x01,
	state_open_SESS = 0x02
}sess_state_t;


typedef struct _serverattr_st{
	char* db_driver;
	char* db_name;
	char* db_ip;
	int   db_port;
	char* db_user;
	char* db_pwd;
	char* infosvr_ip;
	int   infosvr_port;
	int   listen_pos_port;
	float allow_group_distance;//m
	float allow_pos_deviation;
	int   allow_pos_timeout;//s
	int   persons_cache_size;
	int   rules_cache_size;
	int   areas_cache_size;
	int   groups_cache_size;
	char* stgip;
	int   stgport;

}serverattr_st,serverattr_t;

struct _s_client{
	mio_t               mio;
	serverattr_st       appattr;

	// client 
	char			   *frontend_ip;
	int			        frontend_port;
	mio_fd_t			frontend_fd;

	char			   *backend_ip;
	int			        backend_port;
	mio_fd_t			backend_fd;

	//server port..
	char			   *printsvr_ip;
	int			        printsvr_port;
	mio_fd_t			printsvr_fd;
	void               *p2p_printsvr;

	void               *devmap;
	subject_t          subjects;
	//
	users_act_t        user_act;
	//
	void               *mysqlconn;
	int                 monitor_id;
	int                 tracker_id;
	//conf_t				plat_conf;

	xht                 sessions;
	jqueue_t            session_que;
	jqueue_t            zombie_que;
	jqueue_t            dead_sess;
	BOOL                enable_p2p;
	int                 stream_maxq;
	config_t            config;
	response_pkt_pool_t pktpool;	
	char                response_buff[40960];
	/** logging */
	log_t              log;
	/** log data */
	log_type_t         log_type;
	char              *log_facility;
	char              *log_ident;
	void*               sqlobj;

	char				*aes_pwd;

	int					redis_clife;
	int					redis_blife;
	redisContext		*redisUserinfo;

	xht					xtowncode2subquery;
	xht					xsubquery2towncode;
	xht					xdbconfig;
	jqueue_t			sub_query_que;
};
typedef struct _s_client  *_s_client_t, *client_t;

typedef struct _sess_st {
	sess_type_t  type;
	char		 skey[64];
	char         sname[64];
	char         role[64];
	char         subject[64];
	time_t       last_activity;
	_s_client_t  client;
	mio_fd_t	 fd;
	int          state;
	void*        param;  
	BOOL         binitsess;//是否已经Initsess了
	BOOL		 auth;
	int			 auth_cnt;	// 登录验证的次数
	char         towncode[64];
	int          usertype;
} sess_st, *sess_t;

typedef struct _mhash_cache_st{
	xht    t;
}mhash_cache_st,*mhash_cache_t;

void mhash_cache_free(mhash_cache_t cache);
void mhash_cache_clear(mhash_cache_t cache);

typedef int (*pftmio_callback)(mio_t m, mio_action_t a, mio_fd_t fd, void *data, void *arg);
void sess_free(sess_t sess);
void client_free(client_t c);
void client_push_pkt(client_t c,response_pkt_p pkt);

_s_client_t client();
_s_client_t getclient();
int         client_init(client_t client, config_t cfg);

#define _county_type (0)
#define _town_type   (1)

#endif