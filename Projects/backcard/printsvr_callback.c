#include "printsvr_callback.h"
#include "p2p.h"
#include "protobuffer.h"
#include "printsvr_expat.h"
#include "client.h"
#include "tool.h"
#include "myquery/mysqldb.h"
#include "json/cJSON.h"
#include "dbaccess.h"
#include "p2pclient.h"
#include "forward.h"
#include "getrefsvr.h"


//static sess_t getrefsvr(const char* cname)
//{
//	if (client()->sessions == NULL) return NULL;
//	sess_t rtn = (sess_t)xhash_get(client()->sessions, cname);
//
//	return rtn;
//}


static unsigned char checksum(unsigned char buffer[], int len)
{
	unsigned char data = 0;
	int i = 0;
	for (; i < len; i++)
		data ^= buffer[i];

	return data;
}


static void save_x_log(char *type, char *jstr)
{
	p2p_t p2p;
	sess_t destsess;


	log_write(client()->log, LOG_NOTICE, "save_x_log1: %s:%s.", type, jstr);


	cJSON *jsonroot, *json;
	jsonroot = cJSON_Parse(jstr);
	if (jsonroot == NULL)
		return;
	char *taskid, *executor, *net;

	json = cJSON_GetObjectItem(jsonroot, "taskid");
	if (json == NULL || json->valuestring == NULL) {
		cJSON_Delete(jsonroot);
		return;
	}
	taskid = json->valuestring;

	log_write(client()->log, LOG_NOTICE, "save_x_log2.");
	json = cJSON_GetObjectItem(jsonroot, "executor");
	if (json == NULL || json->valuestring == NULL) {
		cJSON_Delete(jsonroot);
		return;
	}
	executor = json->valuestring;

	log_write(client()->log, LOG_NOTICE, "save_x_log3.");
	json = cJSON_GetObjectItem(jsonroot, "net");
	if (json == NULL || json->valuestring == NULL) {
		cJSON_Delete(jsonroot);
		return;
	}
	net = json->valuestring;

	log_write(client()->log, LOG_NOTICE, "save_x_log4.");

	//////////////////////////////////////////////////////////////////////////
	// ����data
	json = cJSON_GetObjectItem(jsonroot, "data");
	if (json == NULL) {
		cJSON_Delete(jsonroot);
		return;
	}

	log_write(client()->log, LOG_NOTICE, "save_x_log5.");

	cJSON* item = cJSON_GetObjectItem(json, "desc");
	/*if (item == NULL || item->valuestring == NULL)
	{
		cJSON_Delete(jsonroot);
		return;
	}
	char *desc = item->valuestring;
*/
	char *desc = "";
	if (item != NULL && item->valuestring != NULL)
	{
		char *desc = item->valuestring;
	}


	log_write(client()->log, LOG_NOTICE, "save_x_log6.");
	//////////////////////////////////////////////////////////////////////////
	//����loc
	json = cJSON_GetObjectItem(jsonroot, "loc");
	if (json == NULL) {
		cJSON_Delete(jsonroot);
		return;
	}


	log_write(client()->log, LOG_NOTICE, "save_x_log6.");

	item = cJSON_GetObjectItem(json, "lon");
	if (item == NULL || item->valuestring == NULL)
	{
		cJSON_Delete(jsonroot);
		return;
	}
	char *lon = item->valuestring;

	log_write(client()->log, LOG_NOTICE, "save_x_log7.");

	item = cJSON_GetObjectItem(json, "lat");
	if (item == NULL || item->valuestring == NULL)
	{
		cJSON_Delete(jsonroot);
		return;
	}
	char *lat = item->valuestring;

	log_write(client()->log, LOG_NOTICE, "save_x_log8.");

	item = cJSON_GetObjectItem(json, "hgt");
	if (item == NULL || item->valuestring == NULL)
	{
		cJSON_Delete(jsonroot);
		return;
	}
	char *hgt = item->valuestring;

	log_write(client()->log, LOG_NOTICE, "save_x_log9.");
	SYSTEMTIME t;
	GetLocalTime(&t);
	char strtime[64] = { 0 };

	sprintf_s(strtime, sizeof(strtime), "%04d-%02d-%02d %02d:%02d:%02d", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

	log_write(client()->log, LOG_NOTICE, "save_x_log10.");
	log_write(client()->log, LOG_NOTICE, "save_x_log: save [%s] log:%s\n", type, jstr);

	cJSON_Delete(jsonroot);
}

static auth_hook(p2pclient_t p2pclient, char *jstr)
{
	cJSON *jsonroot, *json;
	jsonroot = cJSON_Parse(jstr);
	if (jsonroot == NULL)
		return;

	char *rslt;

	json = cJSON_GetObjectItem(jsonroot, "rslt");
	if (json == NULL || json->valuestring == NULL) {
		cJSON_Delete(jsonroot);
		return;
	}
	rslt = json->valuestring;

	if (strcmp(rslt, "fail") == 0)
	{
		p2pclient_close(p2pclient);
		cJSON_Delete(jsonroot);
		return;
	}

	log_write(client()->log, LOG_NOTICE, "recv auth ok from printsvr.");
}

static void initack_hook(sess_t sess,char *jstr)
{
	cJSON *jsonroot, *json;

	jsonroot = cJSON_Parse(jstr);
	if (jsonroot == NULL) return;

	strncpy_s(sess->sname, sizeof(sess->sname), "printsvr", strlen("printsvr"));

	json = cJSON_GetObjectItem(jsonroot, "role");
	if (json == NULL || json->valuestring == NULL)
	{
		cJSON_Delete(jsonroot);
		return;
	}
	json = cJSON_GetObjectItem(jsonroot, "rslt");
	if (json == NULL || json->valuestring == NULL)
	{
		cJSON_Delete(jsonroot);
		return;
	}

	if (strcmp(json->valuestring, "ok") != 0)				// �����Լ�������initʧ�� 
	{
		cJSON_Delete(jsonroot);
		p2pclient_close((p2pclient_t)sess);				// �ر�����
		return;
	}

	cJSON_Delete(jsonroot);

	sess->binitsess = TRUE;

	init_sess_st authpara;
	strcpy_s(authpara.from, sizeof(authpara.from) - 1, "backcard");
	strcpy_s(authpara.to, sizeof(authpara.to) - 1, "printsvr");
	protobuffer_send(client()->p2p_printsvr, eProto_auth, &authpara);
}

void printsvr_exp_cb(int msgid, void* msg, int len, void* param)
{
	sess_t sess,destsess;
	unsigned char* frame = (unsigned char*)msg;

	printsvr_exp_t exp = (printsvr_exp_t)param;
	sess = (sess_t)_BASE(exp).param;
	p2p_t p2p = (p2p_t)sess;
	p2pclient_t p2pclient = (p2pclient_t)sess;

	if (exp == NULL || sess == NULL) return;

	if (len - 5 < 0){
		p2pclient_close(p2pclient);
		return;
	}

	char *jstr = (char*)frame;
	jstr[len - 5] = '\0';
	while (*jstr != '{')jstr++;

	switch (msgid)
	{
	case cunpack_initack:
	{
		// �յ�����printsvr��initack��Ϣ���������Ϊok������auth��Ϣ
		log_write(client()->log, LOG_NOTICE, "recv initack from %s .\n", sess->sname);	 //��ʾ�յ�Initsession�ظ�
		initack_hook(sess, jstr);
	}

	case cunpack_pong:
	{
		log_write(client()->log, LOG_NOTICE, "==== pong from printsvr. ====");
	}
	break;

	case cunpack_auth:
	{
		// �յ�auth�ظ������ok����������������Ͽ�����
		log_write(client()->log, LOG_NOTICE, "recv auth ack from %s .\n", sess->sname);	 //��ʾ�յ�Initsession�ظ�
		auth_hook(p2pclient, jstr);
	}
	break;

	

	default:
		break;
	}
}
