#include "moniclient_callback.h"
#include "p2p.h"
#include "protobuffer.h"
#include "moniclient_expat.h"
#include "json/cJSON.h"
#include "client.h"
#include "tool.h"
#include "dbaccess.h"
#include "forward.h"
#include "getrefsvr.h"
#include "userstatus.h"


//static sess_t getrefsvr(const char* cname)
//{
//	if (client()->sessions == NULL) return NULL;
//	sess_t rtn = (sess_t)xhash_get(client()->sessions, cname);
//
//	return rtn;
//}


static void auth_hook(char* jstr,sess_t sess)
{
	// 首先验证身份，回复auth信息给moniclient/app
	cJSON *jsonroot, *json;
	jsonroot = cJSON_Parse(jstr);
	if (jsonroot == NULL)
		return;
	char *pwd, *name, *seqno;

	json = cJSON_GetObjectItem(jsonroot, "name");
	if (json == NULL || json->valuestring == NULL) {
		cJSON_Delete(jsonroot);
		return;
	}
	name = json->valuestring;

	snprintf(sess->sname, sizeof(sess->sname), "%s", name);

	// 如果有旧连接，则将旧连接断开
	xht ht = client()->sessions;
	sess_t oldsess = NULL;
	oldsess = xhash_get(ht, sess->sname);
	if (oldsess != NULL)
	{
		// 待完善
		printf("kill oldsess %s\n", oldsess->sname);
		p2p_kill((p2p_t)oldsess);
	}

	json = cJSON_GetObjectItem(jsonroot, "seqno");
	if (json == NULL || json->valuestring == NULL) {
		cJSON_Delete(jsonroot);
		return;
	}
	seqno = json->valuestring;

	json = cJSON_GetObjectItem(jsonroot, "pwd");
	if (json == NULL || json->valuestring == NULL) {
		cJSON_Delete(jsonroot);
		return;
	}
	pwd = json->valuestring;


	// 针对moniclient，验证通过，回复auth ok
	int cnt = 0;
	char buffer[2048] = { 0 };
	p2p_t p2p = (p2p_t)sess;
	response_pkt_p pkt = NULL;
	unsigned char ucheck;
	cnt += sprintf_s(buffer + cnt, sizeof(buffer) - cnt, "$auth,%s,%s{\"rslt\":\"ok\",\"seqno\":\"%s\"}", "backcard", sess->sname, seqno);
	ucheck = checksum((unsigned char*)buffer, cnt);
	cnt += sprintf_s(buffer + cnt, sizeof(buffer) - cnt, "*%02X\r\n", ucheck);
	pkt = response_pkt_new(client()->pktpool, cnt);
	memcpy(pkt->data, buffer, cnt);
	pkt->len = cnt;
	pkt->data[pkt->len] = '\0';
	p2p_stream_push(p2p, pkt);
	xhash_put(client()->sessions, sess->sname, sess);

	log_write(p2p->sess.client->log, LOG_NOTICE, "send auth ack to %s:%s.", sess->sname, buffer);

	cJSON_Delete(jsonroot);
}


//收到moniclient发送的数据，转发给printsvr/monitor
void  moniclient_exp_cb(int msgid ,void* msg,int len,void* param)
{
	sess_t sess,destsess;
	p2p_t p2p;
	int  msgcode = 0;
	unsigned char* frame = (unsigned char*)msg;
	moniclient_exp_t exp = (moniclient_exp_t)param;
	
	sess = (sess_t)_BASE(exp).param;
	p2p = (p2p_t)sess;
	if (exp == NULL || sess == NULL) return;

	char * jstr = NULL;

	if (len - 5 < 0){
		p2p_kill(p2p);
		return;
	}

	jstr = (char*)frame;
	jstr[len - 5] = '\0';
	while (*jstr != '{')jstr++;

	switch(msgid)
	{
	case cunpack_ping:
		{
			log_write(client()->log, LOG_NOTICE, "recv ping from %s.", sess->sname);
			protobuffer_send_p2p(p2p, eProto_pong, sess->sname);
		}
		break;

	case cunpack_auth:
		{
			log_write(client()->log, LOG_NOTICE, "recv auth %s.", frame);
			auth_hook(jstr,sess);
		}
		break;

	default:
		break;
	}
	
}
