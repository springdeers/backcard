#include "net.h"
#include "util/util.h"
#include "userstatus.h"
#include "client.h"
#include "./mio/mio.h"
#include "p2p.h"
#include "p2pclient.h"
#include "protobuffer.h"

#define NULL 0

void   check_inactive_client(int sec)
{
	jqueue_t q = users_act_check(client()->user_act, sec);
	if (q == NULL) return;

	int cnt = jqueue_size(q);

	for (int i = 0; i < cnt; i++){
		user_status_t s = jqueue_pull(q);

		if (s == NULL) continue;
		char buffer[1024] = { 0 };

		SYSTEMTIME t;
		GetLocalTime(&t);
		char strtime[64] = { 0 };

		sprintf_s(strtime, sizeof(strtime), "%04d-%02d-%02d %02d:%02d:%02d", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

		sprintf_s(buffer, sizeof(buffer), "{\"taskid\":\"%s\",\"executor\":\"%s\",\"net\":\"inet\",\"data\":{\"time\":\"%s\",\"type\":\"disappear\"},\"loc\":{\"lon\":\"%s\",\"lat\":\"%s\",\"hgt\":\"%s\"}}", s->key, s->arg.executor, strtime, s->arg.lon, s->arg.lat, s->arg.hgt);

		log_write(client()->log, LOG_NOTICE, "-------______ taskid[%s] has just disappeared. ______-------", s->key);

		log_write(client()->log, LOG_NOTICE, "save_warning: %s", buffer);

		free(s);
	}

	jqueue_free(q);
}


void remove_zombie_from_lives()
{
	int  i = 0;
	int  count = jqueue_size(client()->session_que);
	time_t now = time(0);

	for (; i < count; i++)
	{
		sess_t sess = (sess_t)jqueue_pull(client()->session_que);

		if (sess == NULL) continue;

		if (check_zombie(sess, now))
		{
			if (sess->type == back_server_SESS)
			{
				//log_write(client()->log, LOG_NOTICE, "%s @%d is a zombie, I will kill our sess.", sess->sname, sess->fd->fd);
				//p2p_kill((p2p_t)sess);
				jqueue_push(client()->zombie_que, sess, 0);
			}

			else if (sess->type == stream_SESS)
			{
				//log_write(client()->log, LOG_NOTICE, "No response from %s, I will kill myself here and now...",sess->sname);
				//p2pclient_close((p2pclient_t)sess);
				jqueue_push(client()->zombie_que, sess, 0);
			}

		}
		else
			jqueue_push(client()->session_que, sess, 0);
	}
}

void clear_zombies()
{
	int  i = 0;
	int  count = jqueue_size(client()->zombie_que);

	while (i++ < count)
	{
		sess_t sess = (sess_t)jqueue_pull(client()->zombie_que);

		if (sess == NULL) continue;

		if (sess->type == back_server_SESS)
		{
			log_write(client()->log, LOG_NOTICE, "%s @%d is a zombie, I will kill our sess.", sess->sname, sess->fd->fd);
			p2p_kill((p2p_t)sess);
		}
		else if (sess->type == stream_SESS)
		{
			log_write(client()->log, LOG_NOTICE, "No response from %s, I will kill myself here and now...", sess->sname);
			p2pclient_close((p2pclient_t)sess);
		}
	}
}

void clear_deads_from_lives()
{
	int count = jqueue_size(client()->dead_sess);
	int i = 0;

	for (; i < count; i++){
		sess_t sess = (sess_t)jqueue_pull(client()->dead_sess);
		jqueue_push(client()->dead_sess, sess, 0);
		check_dead_from_lives(sess);
	}
}

void check_dead_from_lives(sess_t sessdead)
{
	int i = 0;
	int count = jqueue_size(client()->session_que);

	for (; i < count; i++){
		void* p = jqueue_pull(client()->session_que);
		if (sessdead == p) break;

		jqueue_push(client()->session_que, p, 0);
	}
}

BOOL check_zombie(sess_t sess, time_t now){
	BOOL rtn = FALSE;

	if ((now - sess->last_activity) > 20){
		rtn = TRUE;
	}

	return rtn;
}

//Ping–≠“È
void pingsession(int waits)
{
	int i = 0;
	int count = 0;
	time_t now = time(0);
	static time_t last = 0;
	if (last == 0) last = now;

	if (now - last < waits) return;

	last = now;

	count = jqueue_size(client()->session_que);

	for (; i < count; i++)
	{
		sess_t sess = (sess_t)jqueue_pull(client()->session_que);
		jqueue_push(client()->session_que, sess, 0);

		if (sess->binitsess == TRUE && sess->type == stream_SESS)
		{
			p2pclient_t  p2p = (p2pclient_t)sess;
			protobuffer_send(p2p, eProto_ping, NULL);
			log_write(client()->log, LOG_NOTICE, "--- ping %s. ---", sess->sname);
		}
	}
}

void save_deadsession(int waits)
{
	int count = 0;
	int i = 0;
	time_t now = time(0);
	static time_t last = 0;

	if (last == 0) last = now;

	if (now - last < waits) return;

	last = now;

	count = jqueue_size(client()->dead_sess);

	for (; i < count; i++)
	{
		sess_t dead = (sess_t)jqueue_pull(client()->dead_sess);
		if (dead->type == stream_SESS){
			p2pclient_t p2p = (p2pclient_t)dead;

			BOOL connected = p2pclient_connect(p2p, client()->mio, client()->dead_sess, client(), stream_SESS, NULL, client());

			if (connected){
				init_sess_st initpara;
				strcpy_s(initpara.from, sizeof(initpara.from) - 1, "backcard");
				strcpy_s(initpara.to, sizeof(initpara.to) - 1, dead->sname);
				p2pclient_init(p2p, &initpara);
			}
		}
		else if (dead->type == back_server_SESS){
			sess_free(dead);
		}
	}
}

int send2namend(char* name, char* buffer, int bufferlen)
{
	int rt = 0;
	if (name == NULL || buffer == NULL) return 0;

	sess_t sess = (sess_t)xhash_get(client()->sessions, name);
	if (sess)
	{
		response_pkt_p pkt;
		p2p_t p2p = (p2p_t)sess;

		buffer[bufferlen + 1] = '\0';

		pkt = response_pkt_new(client()->pktpool, bufferlen + 1);
		memcpy(pkt->data, buffer, bufferlen + 1);
		pkt->len = bufferlen + 1;

		p2p_stream_push(p2p, pkt);
		printf("send2named buffer = %s.\r\n", pkt->data);

		rt = 1;
	}

	return rt;
}
