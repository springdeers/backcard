#include "coms.h"
#include "client.h"
#include "p2p.h"
#include <time.h>
#include "expat/devexpat.h"
#include "my_curl/my_curl.h"
#include "json/cJSON.h"
#include "forward.h"



/************************************************************************/
/* create lxl 2018-09-26                                                */
/************************************************************************/

//private global ..
static coms_t _coms = 0;

// 查询分数请求返回数据的处理函数
static size_t score_get_cb(void *ptr, size_t size, size_t nmemb, void *stream)
{
	// 收到的成绩结果，如果正常，则拼串发送给printsvr
	char  jstr_moniclient[1024] = { 0 };
	char  *jstr_printsvr = (char*)ptr;
	printf("score_get_cb: \n%s\n", jstr_printsvr);

	cJSON *jsonroot, *json;
	char * code, *cardid, *comid, *name;
	jsonroot = cJSON_Parse(jstr_printsvr);
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

	json = cJSON_GetObjectItem(jsonroot, "code");
	if (json == NULL || json->valuestring == NULL) {
		cJSON_Delete(jsonroot);
		return;
	}
	code = json->valuestring;

	if (atoi(code) == -1)
	{
		// 发送查询失败信息给监控客户端
		memset(jstr_moniclient, 0, sizeof(jstr_moniclient));
		sprintf(jstr_moniclient, "{\"stage\":\"%s\",\"cardid\":\"%s\",\"comid\":\"%s\",\"name\":\"%s\",\"param\":\"%s\"}", "queryfail", cardid, comid, name, "nil");
		forward_to_moniclient("show", jstr_moniclient);
		cJSON_Delete(jsonroot);
		return -1;
	}

	// 发送 查询成功 给监控客户端
	memset(jstr_moniclient, 0, sizeof(jstr_moniclient));
	sprintf(jstr_moniclient, "{\"stage\":\"%s\",\"cardid\":\"%s\",\"comid\":\"%s\",\"name\":\"%s\",\"param\":\"%s\"}", "queryok", cardid, comid, name, "nil");
	forward_to_moniclient("show", jstr_moniclient);

	// 向printsvr发起打印请求
	forward_to_printsvr("print", jstr_printsvr);

	// 发送 正在打印成绩 给监控客户端
	memset(jstr_moniclient, 0, sizeof(jstr_moniclient));
	sprintf(jstr_moniclient, "{\"stage\":\"%s\",\"cardid\":\"%s\",\"comid\":\"%s\",\"name\":\"%s\",\"param\":\"%s\"}", "printing", cardid, comid, name, "nil");
	forward_to_moniclient("show", jstr_moniclient);

	cJSON_Delete(jsonroot);
}



//收到一消息时，调用此函数
static void _on_com_message_cb(int msgid, void* msg, int len, void* param)
{
	com_t com = (com_t)param;
	char url[1024];
	char jstr_moniclient[1024] = { 0 };
	char *name;
	int comid = com->comno;

	unsigned char * frame = (unsigned char*)msg;
	//int cardid = frame[5] * 16 * 16 + frame[6] * 16 + frame[7];

	unsigned int cardid = 0;
	cardid = frame[5];
	cardid <<= 8;
	cardid |= frame[6];
	cardid <<= 8;
	cardid |= frame[7];
	
	// test begin

	//static int i = -1;
	//int cid = 304068 + i++;
	//if (i > 15)
	//	i = 0;
	//char cardid[10] = { 0 };
	////_itoa(cid, cardid, 10);
	////_itoa_s(cid,cardid,)
	//_itoa(cid, cardid, 10);
	////cardid = "304068";
	//comid = "101";
	//name = cardid;

	//if (strcmp(cardid, "304068") != 0)
	//{
	//	printf("cardid is not 304068, return ....\n");
	//	return;
	//}

	// test end

	// 发送 正在查询成绩 给监控客户端
	memset(jstr_moniclient, 0, sizeof(jstr_moniclient));
	sprintf(jstr_moniclient, "{\"stage\":\"%s\",\"cardid\":\"%d\",\"comid\":\"%d\",\"name\":\"%s\",\"param\":\"%s\"}", "querying", cardid, comid, "NOBODY", "nil");
	forward_to_moniclient("show", jstr_moniclient);

	// 向Parker发送http请求，获取成绩
	sprintf(url, "http://%s:%d/user/score?cardid=%d&comid=%d", client()->parker_ip,client()->parker_port,cardid,comid);
	log_write(client()->log, LOG_NOTICE, "http [userscore] curl_get_req sent ....\n %s", url);
	curl_get_req(url, score_get_cb);
	
}

//收到串口原始数据时，调用此函数
static int _on_comreadcb(unsigned char* rbuffer, int rlen, int code, void* param)
{
	com_t com = (com_t)param;

	rbuffer[rlen] = '\0';

	if (eCode_ok == code) printf("com:%d %s\n", com->comno, rbuffer);

	if (eCode_err == code) com_close(com);

	if (com->expat){
		exp_process_data(com->expat, (char*)rbuffer, rlen);
	}

	return 1;
}

//获得内部的对象
coms_t coms_get()
{
	if (_coms == 0) _coms = calloc(1, sizeof(coms_st));

	return _coms;
}

//打开并初始化串口
int coms_openinit(coms_t coms,config_t config)
{
	int rslt = 0;
	int i = 0;

	char* szvalue;

	coms_get();

	_coms->num = j_atoi(config_get_one(config, "coms.num", 0), 0);
	for (int i = 0; i < _coms->num; i++){
		char ent[64];
		snprintf(ent, sizeof(ent), "coms.com%d.baud", i + 1);
		_coms->baud[i] = j_atoi(config_get_one(config, ent, 0), 0);

		snprintf(ent, sizeof(ent), "coms.com%d.comno", i + 1);
		_coms->comno[i] = j_atoi(config_get_one(config, ent, 0), 0);
	}

	for (; i < _coms->num; i++){
		_coms->comarr[i] = com_new();
		rslt = com_open(_coms->comarr[i], _coms->comno[i], _coms->baud[i], 0, PARITY_NONE, 8, ONESTOPBIT, _on_comreadcb);

		if (rslt == 0)	{
			for (int j = 0; j < i; j++){ com_close(_coms->comarr[j]); com_free(_coms->comarr[j]); }
			return 0;	
		}

		_coms->comarr[i]->expat = (_exp_t)dev_exp_new(_coms->comarr[i]);
		exp_set_callback(_coms->comarr[i]->expat, _on_com_message_cb);
	}

	for (i = 0; i < _coms->num; i++)
	{
		com_sess_init(&_coms->comarr[i]->sess);
	}

	return rslt;
}

//关闭所有串口设备
void   coms_close(coms_t coms)
{
	for (int j = 0; j < coms->num; j++) { com_close(_coms->comarr[j]); com_free(_coms->comarr[j]); }

	_coms->num = 0;
}

//收取串口数据
void do_coms_job(coms_t coms,int now)
{
	com_run(coms->comarr, coms->num, 5/*ms*/);

	static time_t laskcheck_time = 0;
	if (laskcheck_time)  laskcheck_time = time(0);

	if (now - laskcheck_time > 3){
		laskcheck_time = now;

		for (int i = 0; i < coms->num; i++){
			if (coms->comarr[i] && coms->comarr[i]->sess.state == eState_open)
				com_sess_init(&coms->comarr[i]->sess);
			else if (coms->comarr[i] && coms->comarr[i]->sess.state == eState_new){
				com_open(coms->comarr[i], coms->comno[i], coms->baud[i], 0, PARITY_NONE, 8, ONESTOPBIT, _on_comreadcb);
			}
		}
	}
}