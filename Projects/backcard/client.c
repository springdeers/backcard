#include "client.h"
#include "p2p.h"
#include "p2pclient.h"
#include "tool.h"

static _s_client_t  _g_client = NULL;


void sess_free(sess_t sess) {
	p2p_t     p2p     = NULL;
	response_pkt_pool_t pool = client()->pktpool;
	if(pool == NULL) return;

	switch(sess->type) {
	case front_mcu_SESS:
	
	case back_server_SESS:
		p2p = (p2p_t)sess;
		log_debug(ZONE, "free p2p[%s] resource", p2p->ip);

		if(p2p->ip) free(p2p->ip);
		if(p2p->wbufpending) response_pkt_free(p2p->wbufpending);

		while(jqueue_size(p2p->rcmdq) > 0) 
			response_pkt_free((response_pkt_p)jqueue_pull(p2p->rcmdq));	

		jqueue_free(p2p->rcmdq);

		while(jqueue_size(p2p->wbufq) > 0) 
			response_pkt_free((response_pkt_p)jqueue_pull(p2p->wbufq));	

		jqueue_free(p2p->wbufq);

		if(NULL != p2p->buf) _sx_buffer_free(p2p->buf);		

		break;
	default:
		break;
	}

	free(sess);
}

int  client_init(client_t client, config_t config)
{
	// frontend是业务接入端口，包含bd和inet两个链路
	client->frontend_ip = s_strdup("0.0.0.0");
	client->frontend_port = j_atoi(config_get_one(config, "frontend.port", 0), 0);
	if (client->frontend_port == 0)
		client->frontend_port = 7120;

	// backend是向监控服务器开放的端口
	client->backend_ip = s_strdup("0.0.0.0");
	client->backend_port = j_atoi(config_get_one(config, "backend.port", 0), 0);
	if (client->backend_port == 0)
		client->backend_port = 7126;

	// printsvr是认证与告警服务器
	client->printsvr_ip = s_strdup(config_get_one(config, "printsvr.ip", 0));
	if (client->printsvr_ip == NULL)
		client->printsvr_ip = s_strdup("127.0.0.1");
	client->printsvr_port = j_atoi(config_get_one(config, "printsvr.port", 0), 0);
	if (client->printsvr_port == 0)
		client->printsvr_port = 5433;

	client->parker_ip = s_strdup(config_get_one(config, "parker.ip", 0));
	if (client->parker_ip == NULL)
		client->parker_ip = s_strdup("127.0.0.1");
	client->parker_port = j_atoi(config_get_one(config, "parker.port", 0),0);
	if (client->parker_port == 0)
		client->parker_port = 8687;

	client->appattr.db_name = config_get_one(config, "db.name", 0);
	client->appattr.db_ip = config_get_one(config, "db.ip", 0);
	client->appattr.db_port = j_atoi(config_get_one(config, "db.port", 0), 0);
	client->appattr.db_pwd = config_get_one(config, "db.pwd", 0);
	client->appattr.db_user = config_get_one(config, "db.user", 0);

	client->log_type = j_atoi(config_get_one(config, "log.type", 0), 0);	// 日志输出类型

	if (client->log_type == log_STDOUT)
		client->log = log_new(log_STDOUT, NULL, NULL);
	else if (client->log_type == log_FILE)
		client->log = log_new(log_FILE, "../log/backcard.log", NULL);

	//////////////////////////////////////////////////////////////////////////
	client->pktpool = response_pkt_pool_new();
	client->user_act = users_act_new(8192);

	//client()->plat_conf = (conf_t)malloc(sizeof(conf_st));
	client->stream_maxq = 1024;
	client->sessions = xhash_new(1023);
	client->session_que = jqueue_new();
	client->zombie_que = jqueue_new();
	client->dead_sess = jqueue_new();
	client->subjects = subject_new(512);

	client->mio = mio_new(FD_SETSIZE);
	client->enable_p2p = TRUE;

	return 1;
}

void client_free(client_t c)
{
	if(c == NULL) return;

	if (c->log)
		log_write(c->log, LOG_NOTICE, "server shutting down");

	/* cleanup dead sess */
	if(c->dead_sess){
		while(jqueue_size(c->dead_sess) > 0) 
			sess_free((sess_t) jqueue_pull(c->dead_sess));	
		jqueue_free(c->dead_sess);
	}	

	if(c->sessions)
		xhash_free(c->sessions);

	if(c->frontend_ip)
		free(c->frontend_ip);

	if(c->backend_ip)
		free(c->backend_ip);

	if (c->printsvr_ip)
		free(c->printsvr_ip);

	/*if (c->subjects)
		subject_free(c->subjects);*/

	if(c->log) log_free(c->log);

	if(c->config) config_free(c->config);

	if(c->mio)
		mio_free(c->mio);
}

void xhash_walk_throgh(xht h, const char *key, void *val, void *arg)
{
	if(val != NULL)
	{	
		int rtn = 0;
		p2p_t p2p = (p2p_t)val;
		response_pkt_p p = response_pkt_clone((response_pkt_p)arg);
		rtn = p2p_stream_push(p2p,p);
	}
}

void client_push_pkt(client_t c,response_pkt_p pkt)
{
	p2p_t p2p = NULL;
	_jqueue_node_t node;
	response_pkt_p p;
	if(c== NULL || pkt == NULL)
		return;

	node = jqueue_iter_first(c->session_que);
	while(node)
	{
		p2p = (p2p_t)NODEVAL(node);

		p = response_pkt_clone(pkt);

		p2p_stream_push(p2p,p);

		node = jqueue_iter_next(c->session_que,node);
	}
}

void mhash_cache_clear(mhash_cache_t cache)
{
	if(cache)
	{
		if(cache->t)
		{
			void* p = NULL;
			if(xhash_iter_first(cache->t)){
				do {
					xhash_iter_get(cache->t, NULL, &p);
					if(p){
						free(p);
					}
				} while(xhash_iter_next(cache->t));
			}
			if(xhash_iter_first(cache->t)){
				do {
					xhash_iter_zap(cache->t);
				} while(xhash_count(cache->t));
			}
		}
	}
}

void mhash_cache_free(mhash_cache_t cache)
{
	if(cache)
	{
		if(cache->t)
		{
			void* p = NULL;
			if(xhash_iter_first(cache->t)){
				do {
					xhash_iter_get(cache->t, NULL, &p);
					if(p){
						free(p);
					}
				} while(xhash_iter_next(cache->t));
			}

			xhash_free(cache->t);
			cache->t = NULL;
		}
	}
}

_s_client_t client()
{
	if(_g_client == NULL)
		_g_client = calloc(1,sizeof(struct _s_client));

	return _g_client;
}

_s_client_t getclient()
{
	return client();
}
