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

	if (strcmp(json->valuestring, "ok") != 0)				// 发给自己，但是init失败 
	{
		cJSON_Delete(jsonroot);
		p2pclient_close((p2pclient_t)sess);				// 关闭连接
		return;
	}

	cJSON_Delete(jsonroot);

	sess->binitsess = TRUE;

	init_sess_st authpara;
	strcpy_s(authpara.from, sizeof(authpara.from) - 1, "backcard");
	strcpy_s(authpara.to, sizeof(authpara.to) - 1, "printsvr");
	protobuffer_send(client()->p2p_printsvr, eProto_auth, &authpara);
}

// 退卡请求返回数据的处理函数
static size_t backcard_get_cb(void *ptr, size_t size, size_t nmemb, void *stream)
{
	// 收到的退卡请求结果，如果正常，则发送给moniclient
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
		// 发送 退卡成功 给监控客户端
		memset(jstr_moniclient, 0, sizeof(jstr_moniclient));
		sprintf(jstr_moniclient, "{\"stage\":\"%s\",\"cardid\":\"%s\",\"comid\":\"%s\",\"name\":\"%s\",\"param\":\"%s\"}", "backcardok", cardid, comid, name, "nil");
		forward_to_moniclient("show", jstr_moniclient);
		cJSON_Delete(jsonroot);
		return 0;
	}
	else
	{
		// 发送 退卡失败 给监控客户端
		memset(jstr_moniclient, 0, sizeof(jstr_moniclient));
		sprintf(jstr_moniclient, "{\"stage\":\"%s\",\"cardid\":\"%s\",\"comid\":\"%s\",\"name\":\"%s\",\"param\":\"%s\"}", "backcarderr", cardid, comid, name, error);
		forward_to_moniclient("show", jstr_moniclient);
		cJSON_Delete(jsonroot);
		return -1;
	}
}


// 打印服务返回的数据处理函数
static void printack_hook(char *jstr)
{
	char * rslt;
	char jstr_moniclient[1024] = { 0 };
	char *cardid, *comid, *name, *error;

	// 解析jstr，查看rslt值
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
		// 发送 打印失败 给监控客户端
		memset(jstr_moniclient, 0, sizeof(jstr_moniclient));
		sprintf(jstr_moniclient, "{\"stage\":\"%s\",\"cardid\":\"%s\",\"comid\":\"%s\",\"name\":\"%s\",\"param\":\"%s\"}", "printerr", cardid, comid, name, error);
		forward_to_moniclient("show", jstr_moniclient);		
	}
	else
	{
		// 发送 打印成功 给监控客户端
		memset(jstr_moniclient, 0, sizeof(jstr_moniclient));
		sprintf(jstr_moniclient, "{\"stage\":\"%s\",\"cardid\":\"%s\",\"comid\":\"%s\",\"name\":\"%s\",\"param\":\"%s\"}", "printok", cardid, comid, name, "nil");
		forward_to_moniclient("show", jstr_moniclient);

		// 发送 正在退卡 给监控客户端
		memset(jstr_moniclient, 0, sizeof(jstr_moniclient));
		sprintf(jstr_moniclient, "{\"stage\":\"%s\",\"cardid\":\"%s\",\"comid\":\"%s\",\"name\":\"%s\",\"param\":\"%s\"}", "backcarding", cardid, comid, name, "nil");
		forward_to_moniclient("show", jstr_moniclient);

		// 向Parker发起退卡请求
		char url[256] = { 0 };
		sprintf(url, "http://%s:%d/user/backcard?cardid=%s&comid=%s", client()->parker_ip, client()->parker_port, cardid, comid);
		log_write(client()->log, LOG_NOTICE, "http [backcard] curl_get_req sent ....\n %s", url);
		curl_get_req(url, backcard_get_cb);
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
		// 收到来自printsvr的initack消息，如果内容为ok，则发送auth消息
		log_write(client()->log, LOG_NOTICE, "recv initack from %s: %s", "printsvr",frame);	 //提示收到Initsession回复
		initack_hook(sess, jstr);
	}

	case cunpack_pong:
	{
		log_write(client()->log, LOG_NOTICE, "recv pong from printsvr.");
	}
	break;

	case cunpack_authack:
	{
		// 收到auth回复，如果ok则继续，否则主动断开连接
		log_write(client()->log, LOG_NOTICE, "recv authack from %s :%s", sess->sname,frame);	 //提示收到Initsession回复
		auth_hook(p2pclient, jstr);
	}
	break;

	case cunpack_printack:
	{
		// 收到printsvr回复，将状态发送给moniclient，如果ok则申请退卡
		log_write(client()->log, LOG_NOTICE, "recv printack from %s: %s", sess->sname,frame);	 //提示收到Initsession回复
		printack_hook(jstr);
	}
	break;

	

	default:
		break;
	}
}
