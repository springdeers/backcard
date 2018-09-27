#include "client.h"
#include "p2p.h"
#include "p2pclient.h"
#include "tool.h"

static _s_client_p  _g_client = NULL;

void mytime2str(mytime_t t,char* str)
{
	if(t == NULL || str == NULL) return;
	sprintf(str,"%d-%02d-%02d %02d:%02d:%02d",t->year,t->month,t->day,t->hour,t->minute,t->second);
}

void str2mytime(char* str,mytime_t t)
{
	char* p = str;

	if(t == NULL || str == NULL) return;
	//"Y-M-D h:m:s"

	while(*p!='\0')
	{
		while(*p == ' ')p++;
		t->year = atoi(p);

		while(*p != '-')p++;
		p++;
		while(*p == ' ')p++;
		t->month = atoi(p);

		while(*p != '-')p++;
		p++;
		while(*p == ' ')p++;
		t->day = atoi(p);

		while(*p != ' ')p++;
		while(*p == ' ')p++;
		t->hour = atoi(p);

		while(*p != ':')p++;
		p++;
		while(*p == ' ')p++;
		t->minute = atoi(p);

		while(*p != ':')p++;
		p++;
		while(*p == ' ')p++;
		t->second = atoi(p);
		break;
	}
}

void weektimesection_2str(weektimesection_t t,char* str)
{
	if(t == NULL || str == NULL) return;
	sprintf(str,"%d %02d:%02d:%02d~%02d:%02d:%02d",t->weekmask,t->hourStart,t->minuteStart,t->secondStart,t->hourEnd,t->minuteEnd,t->secondEnd);
}

unsigned int sow_weektime(weektime_t wt){

	int i = 0;
	int mask = wt->weekmask;

	while ((mask & 0x01) == 0 && i<7){
		i++;
		mask = wt->weekmask>>i;		
	}

	return i*86400+wt->hour*3600 + wt->minute * 60 + wt->second;
}

#define _SOW(day,hour,minute,second) (day*86400+hour*3600 + minute * 60 + second)


int weektime_in(weektime_t t,weektimesection_t s)
{
	if(t->weekmask & s->weekmask){		
		if(_DAYSECONDS(t->hour,t->minute,t->second) > _DAYSECONDS(s->hourStart,s->minuteStart,s->secondStart) 
			&& _DAYSECONDS(t->hour,t->minute,t->second)< _DAYSECONDS(s->hourEnd,s->minuteEnd,s->secondEnd))
			return 1;
		else 
			return 0;
	}

	return 0;
}

int mytime_cmp_sub_nodate(mytime_t l,mytime_t r)
{
	if(l == NULL || r == NULL) return FALSE;

	if(l->hour < r->hour)
		return -1;
	else if(l->hour > r->hour)
		return 1;

	if(l->minute < r->minute)
		return -1;
	else if(l->minute > r->minute)
		return 1;

	if(l->second < r->second)
		return -1;
	else if(l->second > r->second)
		return 1;

	return 0;

}

int mytime_cmp_sub_datetime(mytime_t l,mytime_t r)
{
	if(l == NULL || r == NULL) return FALSE;

	if(l->year < r->year) 
		return -1;
	else if(l->year > r->year)
		return 1;

	if(l->month < r->month)
		return -1;
	else if(l->month > r->month)
		return 1;

	if(l->day < r->day)
		return -1;
	else if(l->day > r->day)
		return 1;

	if(l->hour < r->hour)
		return -1;
	else if(l->hour > r->hour)
		return 1;

	if(l->minute < r->minute)
		return -1;
	else if(l->minute > r->minute)
		return 1;

	if(l->second < r->second)
		return -1;
	else if(l->second > r->second)
		return 1;

	return 0;

}

int mytime_cmp_sub(mytime_t l,mytime_t r)
{
	if(l == NULL || r == NULL) return FALSE;

	if((l->year == 0 && l->month == 0 && l->day == 0)|| (r->year == 0 && r->month == 0 && r->day == 0)){
		return mytime_cmp_sub_nodate( l, r);
	}else{
		return mytime_cmp_sub_datetime( l, r);
	}
}

weektime_t myweektime(weektime_t wt)
{
	struct tm t;
	if(wt == NULL) return NULL;

	_getsystime(&t);

	wt->hour = t.tm_hour;
	wt->minute = t.tm_min;
	wt->second = t.tm_sec;
	wt->weekmask = 0;
	wt->weekmask |= (1<<t.tm_wday);

	return wt;
}

mytime_t mylocaltime(mytime_t dest)
{
	struct tm t;
	if(dest == NULL) return NULL;

	_getsystime(&t);
	t.tm_year += 1900;
	t.tm_mon  += 1;

	dest->year = t.tm_year;
	dest->month = t.tm_mon;
	dest->day = t.tm_mday;
	dest->hour = t.tm_hour;
	dest->minute = t.tm_min;
	dest->second = t.tm_sec;

	return dest;
}

mytime_t localtime2mytime(struct tm* src,mytime_t dest)
{
	if(src == NULL || dest == NULL) return NULL;
	dest->year = src->tm_year;
	dest->month = src->tm_mon;
	dest->day = src->tm_mday;
	dest->hour = src->tm_hour;
	dest->minute = src->tm_min;
	dest->second = src->tm_sec;

	return dest;
}

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

int  client_init(client_p client, config_t config)
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

void client_free(client_p c)
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

void client_push_pkt(client_p c,response_pkt_p pkt)
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

vec_t vec_alloc(vec_t p,int caporcnt,int size)
{
	vec_t rtn = NULL;

	assert(p);
	assert(caporcnt>0);
	if(p == NULL || caporcnt <=0) return NULL;

	if(p == NULL)
	{
		while((rtn = (vec_t)malloc(sizeof(vec_st))) == NULL) sleep(10);
		while(rtn->data = (char*)malloc(caporcnt * size) == NULL) sleep(10);
		rtn->cap = caporcnt;
		rtn->cnt = 0;
		rtn->size = size;
	}else{
		if(caporcnt > p->cap)
		{
			p->data = (char*)realloc(p->data,caporcnt * size);
			rtn = p ; 
		}
		assert(p->size == size);
	}

	return rtn;
}

int     vec_add(vec_t p, int id, int x, int y, int z)
{
	assert(p!= NULL && p->cnt < p->cap && p->data != NULL);
	if(p== NULL || p->cnt >= p->cap|| p->data == NULL) return 0;	

	memcpy(&(p->data[p->cnt*p->size]),p,p->size);
	p->cnt++;

	return p->cnt;
}

void*    vec_data (vec_t p,int i)
{
	assert(p!= NULL && i< p->cnt && p->data != NULL);
	if(p== NULL || i>= p->cnt || p->data == NULL) return 0;

	return &(p->data[i*p->size]);
}

void    vec_clear(vec_t p)
{
	assert(p != NULL);
	p->cnt = 0;
}

void    vec_free(vec_t p)
{
	if(p)
	{
		if(p->data)
			free(p->data);

		free(p);
	}
}

int str2hms(char* str,int* hour,int* minute,int* second)
{
	char* psz = NULL;

	if(str == NULL || hour == NULL || minute == NULL || second == NULL) return 0;

	psz = str;
	while((*psz == ' '|| *psz =='\t') && *psz!='\0') psz++;
	if(*psz=='\0') return 0;

	*hour = atoi(psz);

	psz = strchr(psz,':');
	if(psz){
		psz++;
		*minute = atoi(psz);
	}else
		return 0;

	psz = strchr(psz,':');
	if(psz){
		psz++;
		*second = atoi(psz);
	}else
		return 0;

	return 1;
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

_s_client_p client()
{
	if(_g_client == NULL)
		_g_client = calloc(1,sizeof(struct _s_client));

	return _g_client;
}

_s_client_p getclient()
{
	return client();
}

void printHex(unsigned char buffer[],int len)
{
	int i = 0 ;
	static char map[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	for(; i < len ; i++){
		printf("%c",map[(buffer[i]>>4)&0x0F]);		
		printf("%c",map[(buffer[i])&0x0F]);	
	}
}