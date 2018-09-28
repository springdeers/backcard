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
#include "my_curl/my_curl.h"


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


static void auth_hook(p2pclient_t p2pclient, char *jstr)
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

	log_write(client()->log, LOG_NOTICE, "recv authack ok from printsvr.");
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

// �˿����󷵻����ݵĴ�����
static size_t backcard_get_cb(void *ptr, size_t size, size_t nmemb, void *stream)
{
	// �յ����˿���������������������͸�moniclient
	char jstr_moniclient[1024] = { 0 };
	char *jstr = (char *)ptr;
	char *rslt , *cardid, *comid, *name, *error;

	cJSON *jsonroot, *json;
	jsonroot = cJSON_Parse(jstr);
	if (jsonroot == NULL)
	{
		return -1;
	}

	json = cJSON_GetObjectItem(jsonroot, "rslt");
	if (json == NULL || json->valuestring == NULL) {
		cJSON_Delete(jsonroot);
		return -1;
	}
	rslt = json->valuestring;

	json = cJSON_GetObjectItem(jsonroot, "cardid");
	if (json == NULL || json->valuestring == NULL) {
		cJSON_Delete(jsonroot);
		return -1;
	}
	cardid = json->valuestring;

	json = cJSON_GetObjectItem(jsonroot, "comid");
	if (json == NULL || json->valuestring == NULL) {
		cJSON_Delete(jsonroot);
		return -1;
	}
	comid = json->valuestring;

	json = cJSON_GetObjectItem(jsonroot, "error");
	if (json == NULL || json->valuestring == NULL) {
		cJSON_Delete(jsonroot);
		return -1;
	}
	error = json->valuestring;

	json = cJSON_GetObjectItem(jsonroot, "name");
	if (json == NULL || json->valuestring == NULL) {
		cJSON_Delete(jsonroot);
		return -1;
	}
	name = json->valuestring;

	if (strcmp(rslt,"ok") == 0)
	{ 
		// ���� �˿��ɹ� ����ؿͻ���
		memset(jstr_moniclient, 0, sizeof(jstr_moniclient));
		sprintf(jstr_moniclient, "\"stage\":\"%s\",\"cardid\":\"%s\",\"comid\":\"%s\",\"name\":\"%s\",\"param\":\"%s\"}", "backcardok", cardid, comid, name, "nil");
		forward_to_moniclient("show", jstr_moniclient);
		cJSON_Delete(jsonroot);
		return 0;
	}
	else
	{
		// ���� �˿�ʧ�� ����ؿͻ���
		memset(jstr_moniclient, 0, sizeof(jstr_moniclient));
		sprintf(jstr_moniclient, "\"stage\":\"%s\",\"cardid\":\"%s\",\"comid\":\"%s\",\"name\":\"%s\",\"param\":\"%s\"}", "backcarderr", cardid, comid, name, error);
		forward_to_moniclient("show", jstr_moniclient);
		cJSON_Delete(jsonroot);
		return -1;
	}
}


// ��ӡ���񷵻ص����ݴ�����
static void printack_hook(char *jstr)
{
	char * rslt;
	char jstr_moniclient[1024] = { 0 };
	char *cardid, *comid, *name, *error;

	// ����jstr���鿴rsltֵ
	cJSON *jsonroot, *json;
	jsonroot = cJSON_Parse(jstr);
	if (jsonroot == NULL)
	{
		return -1;
	}

	json = cJSON_GetObjectItem(jsonroot, "cardid");
	if (json == NULL || json->valuestring == NULL) {
		cJSON_Delete(jsonroot);
		return;
	}
	cardid = json->valuestring;

	json = cJSON_GetObjectItem(jsonroot, "comid");
	if (json == NULL || json->valuestring == NULL) {
		cJSON_Delete(jsonroot);
		return;
	}
	comid = json->valuestring;

	json = cJSON_GetObjectItem(jsonroot, "name");
	if (json == NULL || json->valuestring == NULL) {
		cJSON_Delete(jsonroot);
		return;
	}
	name = json->valuestring;

	json = cJSON_GetObjectItem(jsonroot, "error");
	if (json == NULL || json->valuestring == NULL) {
		cJSON_Delete(jsonroot);
		return;
	}
	error = json->valuestring;

	json = cJSON_GetObjectItem(jsonroot, "rslt");
	if (json == NULL || json->valuestring == NULL) {
		cJSON_Delete(jsonroot);
		return;
	}
	rslt = json->valuestring;

	if (strcmp(rslt, "ok") != 0)
	{
		// ���� ��ӡʧ�� ����ؿͻ���
		memset(jstr_moniclient, 0, sizeof(jstr_moniclient));
		sprintf(jstr_moniclient, "\"stage\":\"%s\",\"cardid\":\"%s\",\"comid\":\"%s\",\"name\":\"%s\",\"param\":\"%s\"}", "printerr", cardid, comid, name, error);
		forward_to_moniclient("show", jstr_moniclient);		
	}
	else
	{
		// ���� ��ӡ�ɹ� ����ؿͻ���
		memset(jstr_moniclient, 0, sizeof(jstr_moniclient));
		sprintf(jstr_moniclient, "\"stage\":\"%s\",\"cardid\":\"%s\",\"comid\":\"%s\",\"name\":\"%s\",\"param\":\"%s\"}", "printok", cardid, comid, name, "nil");
		forward_to_moniclient("show", jstr_moniclient);


		// ��Parker�����˿�����
		char url[256] = { 0 };
		sprintf(url, "http://%s:%d/user/backcard?cardid=%s&comid=%s", client()->parker_ip, client()->parker_port, cardid, comid);
		curl_get_req(url, backcard_get_cb);

		// ���� �����˿� ����ؿͻ���
		memset(jstr_moniclient, 0, sizeof(jstr_moniclient));
		sprintf(jstr_moniclient, "\"stage\":\"%s\",\"cardid\":\"%s\",\"comid\":\"%s\",\"name\":\"%s\",\"param\":\"%s\"}", "backcarding", cardid, comid, name, "nil");
		forward_to_moniclient("show", jstr_moniclient);
	}

	cJSON_Delete(jsonroot);
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
		log_write(client()->log, LOG_NOTICE, "recv initack from %s: %s", "printsvr",frame);	 //��ʾ�յ�Initsession�ظ�
		initack_hook(sess, jstr);
	}

	case cunpack_pong:
	{
		log_write(client()->log, LOG_NOTICE, "recv pong from printsvr.");
	}
	break;

	case cunpack_authack:
	{
		// �յ�auth�ظ������ok����������������Ͽ�����
		log_write(client()->log, LOG_NOTICE, "recv authack from %s :%s", sess->sname,frame);	 //��ʾ�յ�Initsession�ظ�
		auth_hook(p2pclient, jstr);
	}
	break;

	case cunpack_printack:
	{
		// �յ�printsvr�ظ�����״̬���͸�moniclient�����ok�������˿�
		log_write(client()->log, LOG_NOTICE, "recv printack from %s: %s", sess->sname,frame);	 //��ʾ�յ�Initsession�ظ�
		printack_hook(jstr);
	}
	break;

	

	default:
		break;
	}
}
