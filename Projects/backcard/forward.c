#include "forward.h"
#include "dbaccess.h"
#include "getrefsvr.h"

extern int g_warntime;

void forward_to_moniclient(char *head, char* jstr)
{
	p2p_t p2p;
	sess_t destsess;

	char *moniclient = "moniclient";
	destsess = getrefsvr(moniclient);

	if (destsess == NULL)
	{
		log_write(client()->log, LOG_NOTICE, "destsess = getrefsvr(%s) -> NULL.", moniclient);
		return;
	}

	int cnt = 0;
	char buffer[2048] = { 0 };
	response_pkt_p pkt = NULL;
	unsigned char ucheck;
	cnt += sprintf_s(buffer + cnt, sizeof(buffer) - cnt, "$%s,%s,%s,%s", head, "backcard", destsess->sname, jstr);
	ucheck = checksum((unsigned char*)buffer, cnt);
	cnt += sprintf_s(buffer + cnt, sizeof(buffer) - cnt, "*%02X\r\n", ucheck);
	//buffer[cnt] = '\0';
	p2p = (p2p_t)destsess;
	pkt = response_pkt_new(client()->pktpool, cnt);
	memcpy(pkt->data, buffer, cnt);
	pkt->len = cnt;
	pkt->data[pkt->len] = '\0';
	p2p_stream_push(p2p, pkt);

	log_write(client()->log, LOG_NOTICE, "++++ send %s to %s:%s. ++++", head, destsess->sname, buffer);
}

void forward_to_printsvr(char *head, char* jstr)
{
	char buffer[2048];
	int count = 0;
	int endpos = 0;
	response_pkt_p pkt;

	count += sprintf_s(buffer + count, sizeof(buffer) - count, "$%s,%s,%s,%s", head, "backcard", "printsvr", jstr);
	count += sprintf_s(buffer + count, sizeof(buffer) - count, "*%02X\r\n", checksum((unsigned char *)buffer, count));
	endpos = count;
	buffer[endpos + 1] = '\0';

	if (client()->p2p_printsvr != NULL)
	{
		pkt = response_pkt_new(((p2pclient_t)(client()->p2p_printsvr))->pktpool, endpos + 1);
		memcpy(pkt->data, buffer, endpos + 1);
		pkt->len = endpos + 1;
		p2pclient_write((p2pclient_t)(client()->p2p_printsvr), pkt);

		log_write(client()->log, LOG_NOTICE, "send %s to printsvr:%s.", head, buffer);
	}
	else
	{
		log_write(client()->log, LOG_NOTICE, "p2p_printsvr is NULL.");
	}
}
